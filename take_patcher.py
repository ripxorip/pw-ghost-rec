import sys
import tempfile
import requests
import soundfile as sf
import numpy as np

SYNC_PATTERN = np.array([
    1.23e-5, -2.34e-5, 3.45e-5, -4.56e-5,
    5.67e-5, -6.78e-5, 7.89e-5, -8.90e-5,
    9.01e-5, -1.23e-5, 1.35e-5, -2.46e-5,
    3.57e-5, -4.68e-5, 5.79e-5, -6.80e-5
], dtype=np.float32)
SYNC_TOL = 1e-6


def find_sync_offset(audio: np.ndarray) -> int:
    """Find the offset of the sync pattern in a mono float32 numpy array."""
    for i in range(len(audio) - len(SYNC_PATTERN) + 1):
        if np.all(np.abs(audio[i:i+len(SYNC_PATTERN)] - SYNC_PATTERN) < SYNC_TOL):
            return i
    return -1


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <reference.wav>")
        sys.exit(1)
    ref_path = sys.argv[1]

    # Download to temp file
    with tempfile.NamedTemporaryFile(suffix='.wav', delete=False) as tmp:
        r = requests.get('http://localhost:9123/buffer.wav')
        tmp.write(r.content)
        dl_path = tmp.name

    # Read both wavs
    ref_audio, ref_sr = sf.read(ref_path, dtype='float32')
    dl_audio, dl_sr = sf.read(dl_path, dtype='float32')
    if ref_sr != dl_sr:
        print(f"Sample rates differ: {ref_sr} vs {dl_sr}")
        sys.exit(1)
    if ref_audio.ndim > 1:
        ref_audio = ref_audio[:, 0]
    if dl_audio.ndim > 1:
        dl_audio = dl_audio[:, 0]

    # Find sync in both
    ref_sync = find_sync_offset(ref_audio)
    dl_sync = find_sync_offset(dl_audio)
    if ref_sync == -1 or dl_sync == -1:
        print("Sync pattern not found in one or both files!")
        sys.exit(1)

    # Calculate offset difference
    offset_diff = ref_sync - dl_sync
    # Pad or trim downloaded audio so sync aligns with reference
    if offset_diff > 0:
        # Need to pad the start of downloaded audio
        dl_audio = np.pad(dl_audio, (offset_diff, 0))
    elif offset_diff < 0:
        # Need to trim the start of downloaded audio
        dl_audio = dl_audio[-offset_diff:]
    # Now, ensure length matches reference
    if len(dl_audio) < len(ref_audio):
        dl_audio = np.pad(dl_audio, (0, len(ref_audio) - len(dl_audio)))
    elif len(dl_audio) > len(ref_audio):
        dl_audio = dl_audio[:len(ref_audio)]

    # Apply 100x gain to the sync pattern in the aligned audio
    sync_len = len(SYNC_PATTERN)
    # Find sync again in the aligned audio (should be at ref_sync)
    sync_start = ref_sync
    if sync_start + sync_len <= len(dl_audio):
        dl_audio[sync_start:sync_start+sync_len] = dl_audio[sync_start:sync_start+sync_len] * 1000.0
        print(f"Applied 100x gain to sync pattern at index {sync_start}")
    else:
        print("Warning: sync pattern location out of bounds for gain boost!")

    # Save aligned file with same format as reference, overwriting the reference file
    out_path = ref_path
    ref_info = sf.info(ref_path)
    sf.write(out_path, dl_audio, ref_sr, subtype=ref_info.subtype, format=ref_info.format)
    print(f"Aligned file written to {out_path} (format: {ref_info.format}, subtype: {ref_info.subtype})")

if __name__ == '__main__':
    main()
