# Freesound Browser

In-DAW sound-effects browser for **Pro Tools** (VST3 via a host such as PatchWork, optional AAX) and a **Standalone** app. Search [Freesound.org](https://freesound.org), audition previews, download, convert to session WAV, and import into an open Pro Tools session via PTSL.

## Features

- Search, duration / license / tag filters, results table, waveform preview
- Freesound APIv2 with **API Token** and optional **OAuth2** for original downloads
- Settings stored locally (not in source): `%APPDATA%\FreesoundTools\`
- Direct WASAPI preview output (independent of the DAW audio device)
- Download → decode (WAV / AIFF / FLAC / OGG / MP3) → PCM WAV
- **Import to Pro Tools**: creates a new track, imports the clip, spots it (PTSL)

## Requirements

| Tool | Notes |
|------|--------|
| CMake 3.22+ | |
| C++17 compiler | MSVC 2022 recommended on Windows |
| JUCE 8.0.4 | Fetched automatically by CMake |
| AAX SDK | Optional — only for AAX builds (`AAX_SDK_PATH`) |
| Freesound API credential | https://freesound.org/apiv2/apply/ |
| Python 3 + grpcio | Only for Pro Tools PTSL import (`Tools/ptsl/setup.bat`) |

See **BUILD.txt** for step-by-step Windows build instructions.

## Runtime layout (VST3 + Standalone + Import)

```
FreesoundBrowser/
├── Freesound Browser.exe
├── Freesound Browser.vst3/
└── Tools/
    └── ptsl/
        ├── import_to_protools.py
        └── repo/
            ├── ptsl_client.py
            ├── PTSL_pb2.py
            └── PTSL_pb2_grpc.py
```

## Freesound authentication

1. Apply for an API key at https://freesound.org/apiv2/apply/
2. Open the app → **Settings** → paste **API Token**
3. For original-file downloads, add OAuth Client ID / Secret and complete authorization

## License / attribution

Respect each sound’s license shown in the results table. This project is not affiliated with Freesound or Avid.
