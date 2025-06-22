# pw-ghost-rec

## 🎯 Goal
- Work around occasional ASIO/PWAR audio glitches caused by network jitter
- Achieve *perfect recordings* by combining streamed + local audio

## 🧠 Core Idea
- Dual recording system: stream in Reaper via ASIO, simultaneously record pristine audio on Linux
- Replace glitchy Reaper audio *after the fact* using synced local WAV
- ⚡ **Key advantage**: Reaper records one **continuous WAV file**, even during looped or punch-in recording — makes post-hoc replacement trivial

## 🛠️ System Architecture
- **Windows (Reaper)**
    - Records audio streamed from Linux over UDP via ASIO/PWAR
    - Sends OSC commands on record start/stop
- **Linux**
    - Custom PipeWire filter:
        - Injects sync marker impulses (e.g. +1, 0, -1) into stream
        - Buffers incoming audio in a ring buffer (RAM)
    - Listens for OSC from Reaper using liblo
    - On record stop:
        - Extracts matching audio from buffer
        - Sends clean WAV back to Windows

## 🧰 Tools Used
- `PipeWire filter` (C): inserts marker + records audio
- `liblo` (C): listens for OSC from Reaper
- `Python/C++ tool`: trims + syncs clean WAV based on marker
- `Lua ReaScript`:
    - Finds marker in glitchy take
    - Replaces bad take with clean one
    - Supports punch-ins / loop recording
- `OSC`: control interface between Reaper and Linux

## ⚙️ Workflow
- Reaper starts recording → OSC sent
- Linux:
    - Injects marker
    - Begins local caching
- Reaper stops recording → OSC sent
- Linux:
    - Exports synced WAV
    - Sends to Windows
    - Reaper Lua script replaces original audio

## 🧪 Features
- Sample-accurate fixups
- Handles looped recording and punch-ins
- Uses RAM for speed, avoids I/O bottlenecks
- Optional diagnostics overlay or auto-backups

## 🪛 Potential Enhancements
- More marker types (loop start, punch in, etc)
- Background daemon to handle export/replacement automatically
- Reaper overlay: waveform mismatch indicator

## ✅ Why This Wins
- Reaper records **one long file**, making it easy to find/replace sections
- Fully automated
- Open-source, commodity gear
- Flexible + robust: replaces perfection *after* recording
