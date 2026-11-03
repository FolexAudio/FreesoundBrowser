# PTSL helper (Pro Tools Scripting)

Used by **Freesound Browser** Import to place WAVs into the open Pro Tools session.

## Requirements
1. Pro Tools running with a **session open**
2. PTSL server on `localhost:31416` (Scripting / HUI peripherals as required by your PT version)
3. Python 3 + `pip install grpcio protobuf`

## Test
```bat
python import_to_protools.py --file "C:\path\to\file.wav"
```
