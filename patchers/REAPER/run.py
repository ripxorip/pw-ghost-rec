#!/usr/bin/env python3
import sys
import os
from pathlib import Path
import soundfile as sf
import numpy as np
import time

SYNC_PATTERN = np.array([
    1.23e-5, -2.34e-5, 3.45e-5, -4.56e-5,
    5.67e-5, -6.78e-5, 7.89e-5, -8.90e-5,
    9.01e-5, -1.23e-5, 1.35e-5, -2.46e-5,
    3.57e-5, -4.68e-5, 5.79e-5, -6.80e-5
], dtype=np.float32)
SYNC_TOL = 1e-6
DURATION_TOL = 1  # seconds
time_margin = 1  # seconds

def find_sync_offset(audio: np.ndarray) -> int:
    for i in range(len(audio) - len(SYNC_PATTERN) + 1):
        if np.all(np.abs(audio[i:i+len(SYNC_PATTERN)] - SYNC_PATTERN) < SYNC_TOL):
            return i
    return -1

def get_wav_duration(path):
    info = sf.info(str(path))
    return info.frames / info.samplerate

def find_wav_data_offset(path):
    import struct
    with open(path, 'rb') as f:
        riff = f.read(12)
        while True:
            chunk = f.read(8)
            if len(chunk) < 8:
                break
            chunk_id, chunk_size = struct.unpack('<4sI', chunk)
            if chunk_id == b'data':
                return f.tell(), chunk_size
            f.seek(chunk_size, 1)
    raise RuntimeError('data chunk not found')

def float32_to_pcm24(samples):
    import struct
    samples = np.clip(samples, -1.0, 1.0)
    ints = (samples * 8388607.0).astype(np.int32)
    pcm_bytes = bytearray()
    for i in ints:
        pcm_bytes += struct.pack('<i', i)[0:3]
    return pcm_bytes

class ReaperPatcher:
    def __init__(self, proj_path):
        self.src_project = Path(proj_path).expanduser().resolve()
        self.proj_name = self.src_project.name
        # _out is relative to current working directory
        self.dest_project = Path.cwd() / "_out" / self.proj_name
        self.recordings_dir = Path.home() / ".pw-ghost-rec" / "recordings"

    def rsync_project(self):
        print(f"Rsyncing {self.src_project} to {self.dest_project}")
        self.dest_project.parent.mkdir(parents=True, exist_ok=True)
        rsync_cmd = f'rsync -av --delete "{self.src_project}/" "{self.dest_project}/"'
        print(f"Running: {rsync_cmd}")
        os.system(rsync_cmd)

    def print_patch_summary(self, patch_results):
        print("\n" + "="*40)
        print("PATCH SUMMARY")
        print("="*40)
        print("\nPatched files:")
        if patch_results['patched']:
            for wav, rec, diff_mean, diff_max in patch_results['patched']:
                warn = ""
                if diff_mean is not None and (diff_mean > 0.01 or diff_max > 0.1):
                    warn = f" [WARNING: mean={diff_mean:.6f}, max={diff_max:.6f}]"
                elif diff_mean is not None:
                    warn = f" (mean={diff_mean:.6f}, max={diff_max:.6f})"
                print(f"  ✓ {wav.name}  <--  {rec.name}{warn}")
        else:
            print("  (none)")
        print("\nCould NOT patch:")
        if patch_results['not_patched']:
            for wav, reason in patch_results['not_patched']:
                print(f"  ✗ {wav.name}  --  {reason}")
        else:
            print("  (none)")
        print("\n" + "="*40 + "\n")

    def patch_wav_files(self):
        print("Scanning for wav files in project...")
        wav_files = list(self.dest_project.rglob('*.wav'))
        print(f"Found {len(wav_files)} wav files.")
        rec_files = list(self.recordings_dir.rglob('*.wav'))
        print(f"Found {len(rec_files)} candidate recordings.")
        patch_results = {'patched': [], 'not_patched': []}
        for wav in wav_files:
            wav_mtime = wav.stat().st_mtime
            wav_dur = get_wav_duration(wav)
            candidates = []
            for rec in rec_files:
                rec_mtime = rec.stat().st_mtime
                rec_dur = get_wav_duration(rec)
                if abs(wav_mtime - rec_mtime) < time_margin and abs(wav_dur - rec_dur) < DURATION_TOL:
                    candidates.append(rec)
            if not candidates:
                patch_results['not_patched'].append((wav, 'No match found'))
                continue
            best = max(candidates, key=lambda r: r.stat().st_mtime)
            print(f"Patching {wav.name} with {best.name}")
            try:
                diff_mean, diff_max = self.patch_wav_with_reference(wav, best)
                patch_results['patched'].append((wav, best, diff_mean, diff_max))
            except Exception as e:
                patch_results['not_patched'].append((wav, f"Failed: {e}"))
        self.print_patch_summary(patch_results)

    def patch_wav_with_reference(self, ref_path, rec_path):
        ref_audio, ref_sr = sf.read(str(ref_path), dtype='float32')
        rec_audio, rec_sr = sf.read(str(rec_path), dtype='float32')
        if ref_sr != rec_sr:
            raise RuntimeError(f"Sample rates differ: {ref_sr} vs {rec_sr}")
        if ref_audio.ndim > 1:
            ref_audio = ref_audio[:, 0]
        if rec_audio.ndim > 1:
            rec_audio = rec_audio[:, 0]
        ref_sync = find_sync_offset(ref_audio)
        rec_sync = find_sync_offset(rec_audio)
        if ref_sync == -1 or rec_sync == -1:
            raise RuntimeError("Sync pattern not found in one or both files!")
        offset_diff = ref_sync - rec_sync
        if offset_diff > 0:
            rec_audio = np.pad(rec_audio, (offset_diff, 0))
        elif offset_diff < 0:
            rec_audio = rec_audio[-offset_diff:]
        if len(rec_audio) < len(ref_audio):
            rec_audio = np.pad(rec_audio, (0, len(ref_audio) - len(rec_audio)))
        rec_audio = rec_audio[:len(ref_audio)]
        sync_len = len(SYNC_PATTERN)
        sync_start = ref_sync
        compare_start = sync_start + sync_len
        diff_mean = diff_max = None
        if compare_start < len(ref_audio):
            diff = np.abs(ref_audio[compare_start:] - rec_audio[compare_start:])
            diff_mean = float(np.mean(diff))
            diff_max = float(np.max(diff))
        if sync_start + sync_len <= len(rec_audio):
            rec_audio[sync_start:sync_start+sync_len] = rec_audio[sync_start:sync_start+sync_len] * 10000.0
        data_offset, data_size = find_wav_data_offset(ref_path)
        patch_start = sync_start
        patch_len = len(rec_audio) - patch_start
        with open(ref_path, 'rb') as f:
            f.seek(data_offset)
            orig_data = f.read(data_size)
        bytes_per_sample = 3
        preamble_bytes = patch_start * bytes_per_sample
        new_patch_bytes = float32_to_pcm24(rec_audio[patch_start:])
        new_data = orig_data[:preamble_bytes] + new_patch_bytes
        if len(new_data) < len(orig_data):
            new_data += orig_data[len(new_data):]
        elif len(new_data) > len(orig_data):
            new_data = new_data[:len(orig_data)]
        with open(ref_path, 'r+b') as f:
            f.seek(data_offset)
            f.write(new_data)
        print(f"Patched REAPER: {ref_path.name}  with  LOCAL: {rec_path.name} (only data chunk, PCM_24, post-sync)")
        return diff_mean, diff_max

    def rsync_back_to_src_patched(self):
        # After patching, rsync dest_project to src_patched next to the original src_project
        src_patched = self.src_project.parent / f"{self.proj_name}_patched"
        print(f"Rsyncing patched project to {src_patched}")
        src_patched.mkdir(parents=True, exist_ok=True)
        rsync_cmd = f'rsync -av --delete "{self.dest_project}/" "{src_patched}/"'
        print(f"Running: {rsync_cmd}")
        os.system(rsync_cmd)

    def run(self):
        self.rsync_project()
        self.patch_wav_files()
        print("Patching complete. Now syncing back to source project...")
        self.rsync_back_to_src_patched()

def main():
    if len(sys.argv) < 2:
        print("Usage: run.py <REAPER_PROJECT_PATH>")
        sys.exit(1)
    proj_path = sys.argv[1]
    patcher = ReaperPatcher(proj_path)
    patcher.run()

if __name__ == "__main__":
    main()