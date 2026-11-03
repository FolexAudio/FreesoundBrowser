#!/usr/bin/env python3
"""
Import WAV file(s) into the open Pro Tools session via PTSL (gRPC :31416).

For each file: create a new audio track named after the file, import to the
Clip List, then spot the clip onto that new track.
"""

from __future__ import annotations

import argparse
import json
import os
import struct
import sys
import wave

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.join(HERE, "repo")
sys.path.insert(0, REPO)

try:
    from ptsl_client import PTSLClient
except ImportError as e:
    print(json.dumps({"ok": False, "error": f"Missing PTSL stubs: {e}. Run Tools/ptsl/setup.bat"}))
    sys.exit(2)


def wav_channel_format(path: str) -> str:
    """Return TF_Mono or TF_Stereo from WAV header (default stereo)."""
    try:
        with wave.open(path, "rb") as w:
            ch = w.getnchannels()
        return "TF_Mono" if ch <= 1 else "TF_Stereo"
    except Exception:
        # Fallback: peek RIFF fmt channels
        try:
            with open(path, "rb") as f:
                hdr = f.read(36)
            if len(hdr) >= 22 and hdr[0:4] == b"RIFF":
                ch = struct.unpack_from("<H", hdr, 22)[0]
                return "TF_Mono" if ch <= 1 else "TF_Stereo"
        except Exception:
            pass
    return "TF_Stereo"


def sanitize_track_name(path: str) -> str:
    base = os.path.splitext(os.path.basename(path))[0]
    # Pro Tools track names: keep it short / clean
    bad = '\\/:*?"<>|'
    for c in bad:
        base = base.replace(c, "_")
    base = base.strip() or "Freesound"
    return base[:80]


def create_track(client: PTSLClient, name: str, fmt: str) -> dict | None:
    # Ensure unique-ish name if collision
    existing = set()
    listing = client.send_command("GetTrackList", {})
    if listing.get("status") == "Completed":
        for t in (listing.get("body") or {}).get("track_list") or []:
            existing.add(t.get("name") or "")

    final = name
    n = 2
    while final in existing:
        final = f"{name} {n}"
        n += 1

    r = client.send_command("CreateNewTracks", {
        "number_of_tracks": 1,
        "track_name": final,
        "track_type": "TT_Audio",
        "track_format": fmt,
        "track_timebase": "TTB_Samples",
    })
    if r.get("status") != "Completed":
        return None

    body = r.get("body") or {}
    ids = body.get("created_track_ids") or []
    names = body.get("created_track_names") or [final]
    return {
        "id": ids[0] if ids else "",
        "name": names[0] if names else final,
        "raw": r,
    }


def spot_clips(client: PTSLClient, clip_ids: list[str], track_id: str) -> dict:
    return client.send_command("SpotClipsByID", {
        "src_clips": clip_ids,
        "dst_track_id": track_id,
        "dst_location_data": {
            "location_type": "Start",
            "location": {
                "location": "0",
                "time_type": "TLType_Samples",
            },
        },
    })


def import_one(client: PTSLClient, path: str) -> dict:
    track_name = sanitize_track_name(path)
    fmt = wav_channel_format(path)

    track = create_track(client, track_name, fmt)
    if track is None or not track.get("id"):
        return {"ok": False, "file": path, "error": "CreateNewTracks failed"}

    clip = client.send_command("ImportAudioToClipList", {
        "file_list": [path],
        "audio_operations": "AOperations_AddAudio",
    })
    if clip.get("status") != "Completed":
        return {
            "ok": False,
            "file": path,
            "error": "ImportAudioToClipList failed",
            "track": track,
            "detail": clip,
        }

    clip_ids = []
    for entry in (clip.get("body") or {}).get("file_list", []) or []:
        for dest in entry.get("destination_file_list", []) or []:
            for cid in dest.get("clip_id_list", []) or []:
                clip_ids.append(cid)

    if not clip_ids:
        return {
            "ok": True,
            "file": path,
            "message": f"Created track '{track['name']}' and imported to Clip List (no clip id to spot)",
            "track": track["name"],
        }

    spotted = spot_clips(client, clip_ids, track["id"])
    if spotted.get("status") == "Completed":
        return {
            "ok": True,
            "file": path,
            "message": f"Imported onto new track '{track['name']}'",
            "track": track["name"],
            "track_id": track["id"],
            "clip_ids": clip_ids,
        }

    return {
        "ok": True,
        "file": path,
        "message": f"Created track '{track['name']}' + Clip List entry (spot failed — drag from Clip List)",
        "track": track["name"],
        "clip_ids": clip_ids,
        "spot_error": spotted,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", action="append", dest="files", required=True)
    ap.add_argument("--destination", default="NewTrack")
    ap.add_argument("--location", default="Selection")
    args = ap.parse_args()

    files = []
    for f in args.files:
        p = os.path.abspath(f)
        if not os.path.isfile(p):
            print(json.dumps({"ok": False, "error": f"File not found: {p}"}))
            return 1
        files.append(p)

    client = PTSLClient()
    try:
        client.connect()
    except Exception as e:
        print(json.dumps({
            "ok": False,
            "error": f"Cannot connect to Pro Tools PTSL on localhost:31416 ({e}). "
                     "Open Pro Tools with a session (scripting enabled)."
        }))
        return 3

    try:
        reg = client.register("FreesoundTools", "FreesoundBrowser")
        if reg.get("status") != "Completed":
            print(json.dumps({"ok": False, "error": "RegisterConnection failed", "detail": reg}))
            return 4

        ready = client.host_ready()
        if not (ready.get("body") or {}).get("is_host_ready", False):
            print(json.dumps({"ok": False, "error": "Pro Tools host not ready", "detail": ready}))
            return 5

        results = [import_one(client, path) for path in files]
        all_ok = all(r.get("ok") for r in results)
        messages = [r.get("message") or r.get("error") or "" for r in results]

        print(json.dumps({
            "ok": all_ok,
            "message": "; ".join(m for m in messages if m),
            "files": files,
            "results": results,
        }))
        return 0 if all_ok else 6
    finally:
        client.disconnect()


if __name__ == "__main__":
    sys.exit(main())
