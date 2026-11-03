# Freesound Browser

In-DAW sound-effects browser for **Pro Tools** and a **Standalone** app. Search [Freesound.org](https://freesound.org), audition previews, download, convert to session WAV, and import into an open Pro Tools session via PTSL.

## Using in Pro Tools (no AAX signing required)

Retail Pro Tools only loads **signed** AAX plug-ins. This release ships **VST3** (and Standalone). You do **not** need to build or PACE-sign an AAX binary to use it in Pro Tools.

**Recommended path:** load the VST3 inside **[Blue Cat’s PatchWork](https://www.bluecataudio.com/Products/Product_PatchWork/)** (the free **demo** is enough):

1. Install PatchWork and insert the **PatchWork** AAX plug-in on a track in Pro Tools (or open it as an instrument / effect as you prefer).
2. Inside PatchWork’s rack, add **Freesound Browser** (VST3). Put `Freesound Browser.vst3` where your host scans for VST3s, and keep the `Tools` folder next to it (see below).
3. Use Freesound Browser as usual (search, preview, Import). Import still talks to Pro Tools through PTSL while the session is open.

Optional later: if you have the AAX SDK and PACE / wraptool signing set up, you can build a native AAX instead. That is **not** required for day-to-day use with PatchWork + VST3.

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

Pro Tools **Import** needs the `Tools` folder next to the binary you run. Search / preview / download work without it.

- **Standalone:** put `Tools` in the **same folder** as `Freesound Browser.exe`.
- **VST3:** if you move `Freesound Browser.vst3` into your plug-in folder, **also copy `Tools`** into that same parent folder (beside the `.vst3` bundle). The `.vst3` alone is not enough for Import.

```
FreesoundBrowser/          (or your VST3 plug-ins folder)
├── Freesound Browser.exe  (optional)
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
