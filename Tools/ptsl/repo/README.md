# PTSL MCP Server — AI Control for Pro Tools

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Python 3.10+](https://img.shields.io/badge/Python-3.10+-green.svg)](https://www.python.org)
[![PTSL 2025.10](https://img.shields.io/badge/PTSL-2025.10-orange.svg)](https://www.avid.com/)

**Control Pro Tools with natural language through AI agents like Claude, Hermes, and OpenClaw.**

This MCP (Model Context Protocol) server exposes all **159 Pro Tools Scripting Library (PTSL) commands** as AI-callable tools, enabling any LLM to control Pro Tools programmatically.

---

## 🎛️ What It Does

Turn this:
```
"Mute track Audio 3, solo the drums, and export the mix as WAV"
```

Into automatic execution in Pro Tools via AI agents.

---

## 🏗️ Architecture

```
┌─────────────────┐      stdio (JSON-RPC)      ┌──────────────────┐
│   AI Agent      │ ──────────────────────────► │  PTSL MCP       │
│   (Claude,      │ ◄────────────────────────── │  Server         │
│    Hermes, etc) │       responses              │  (ptsl_mcp_     │
└─────────────────┘                             │  server.py)     │
                                              └────────┬─────────┘
                                                       │
                                                       │ gRPC (localhost:31416)
                                                       ▼
                                              ┌──────────────────┐
                                              │   Pro Tools      │
                                              │   (running)      │
                                              └──────────────────┘
```

**Flow:**
1. You give a command to your AI agent (Claude, Hermes, etc.)
2. Agent calls an MCP tool (e.g., `set_track_mute_state`)
3. MCP server translates to PTSL gRPC command
4. Pro Tools executes the command
5. Response flows back to the agent

---

## 📦 Installation

### Prerequisites
- **Pro Tools** (2022.12 or later with PTSL enabled)
- **Python 3.10+**
- **Avid PTSL SDK** (for the `.proto` file — included `.py` stubs work out of the box)

### Quick Setup (Windows)

```batch
cd ptsl-mcp-server
setup.bat
```

### Manual Setup

```bash
# Create virtual environment
python -m venv venv

# Activate (Windows)
venv\Scripts\activate

# Install dependencies
pip install grpcio grpcio-tools protobuf mcp
```

---

## 🚀 Usage

### Enable PTSL in Pro Tools
1. Open Pro Tools
2. Go to **Setup → Peripherals → MIDI Controllers**
3. Check **Enable HUI** (this enables the PTSL gRPC server)

### Configure in Claude Desktop

Edit `%APPDATA%\Claude\claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "protools": {
      "command": "C:\\Path\\To\\venv\\Scripts\\python.exe",
      "args": ["C:\\Path\\To\\ptsl_mcp_server.py"]
    }
  }
}
```

### Configure in Hermes Agent

Edit `~/.hermes/config.yaml`:

```yaml
mcp_servers:
  protos:
    command: "/path/to/venv/bin/python"
    args: ["/path/to/ptsl_mcp_server.py"]
```

### Configure in OpenClaw

Edit `C:\Users\philm\.openclaw\openclaw.json`:

```json
{
  "plugins": {
    "ptsl": {
      "command": "C:\\Path\\To\\venv\\Scripts\\python.exe",
      "args": ["C:\\Path\\To\\ptsl_mcp_server.py"]
    }
  }
}
```

---

## 🛠️ All 159 PTSL Commands Exposed

### Session Management (12 commands)
| MCP Tool Name | PTSL Command | Description |
|---------------|--------------|-------------|
| `create_session` | CreateSession | Create a new session |
| `open_session` | OpenSession | Open existing session |
| `close_session` | CloseSession | Close current session |
| `save_session` | SaveSession | Save current session |
| `save_session_as` | SaveSessionAs | Save with new name |
| `get_session_name` | GetSessionName | Get session name |
| `get_session_path` | GetSessionPath | Get session path |
| `get_session_sample_rate` | GetSessionSampleRate | Get sample rate |
| `get_session_bit_depth` | GetSessionBitDepth | Get bit depth |
| `get_session_length` | GetSessionLength | Get session length |
| `get_session_start_time` | GetSessionStartTime | Get start time |
| `get_session_time_code_rate` | GetSessionTimeCodeRate | Get timecode rate |
| `get_session_audio_format` | GetSessionAudioFormat | Get audio format |
| `get_session_ids` | GetSessionIDs | Get session IDs |
| `set_session_audio_format` | SetSessionAudioFormat | Set audio format |
| `set_session_bit_depth` | SetSessionBitDepth | Set bit depth |
| `set_session_length` | SetSessionLength | Set session length |
| `set_session_start_time` | SetSessionStartTime | Set start time |
| `set_session_time_code_rate` | SetSessionTimeCodeRate | Set timecode rate |
| `set_session_interleaved_state` | SetSessionInterleavedState | Set interleaved |
| `get_session_interleaved_state` | GetSessionInterleavedState | Get interleaved |
| `get_session_audio_rate_pull_settings` | GetSessionAudioRatePullSettings | Get pull settings |
| `set_session_audio_rate_pull_settings` | SetSessionAudioRatePullSettings | Set pull settings |
| `get_session_video_rate_pull_settings` | GetSessionVideoRatePullSettings | Get video pull |
| `set_session_video_rate_pull_settings` | SetSessionVideoRatePullSettings | Set video pull |
| `get_session_feet_frames_rate` | GetSessionFeetFramesRate | Get feet/frames |
| `set_session_feet_frames_rate` | SetSessionFeetFramesRate | Set feet/frames |
| `get_session_system_delay_info` | GetSessionSystemDelayInfo | Get system delay |

### Transport Control (10 commands)
| MCP Tool Name | PTSL Command | Description |
|---------------|--------------|-------------|
| `get_transport_state` | GetTransportState | Get play/stop/record state |
| `get_transport_armed` | GetTransportArmed | Check if armed |
| `toggle_play_state` | TogglePlayState | Play/Stop toggle |
| `toggle_record_enable` | ToggleRecordEnable | Record enable toggle |
| `set_playback_mode` | SetPlaybackMode | Set playback mode |
| `set_record_mode` | SetRecordMode | Set record mode |
| `get_playback_mode` | GetPlaybackMode | Get playback mode |
| `get_record_mode` | GetRecordMode | Get record mode |
| `play_half_speed` | PlayHalfSpeed | Play at half speed |
| `record_half_speed` | RecordHalfSpeed | Record at half speed |

### Track Operations (30+ commands)
| MCP Tool Name | PTSL Command | Description |
|---------------|--------------|-------------|
| `get_track_list` | GetTrackList | List all tracks |
| `create_new_tracks` | CreateNewTracks | Create new tracks |
| `delete_tracks` | DeleteTracks | Delete tracks |
| `rename_target_track` | RenameTargetTrack | Rename track |
| `set_track_mute_state` | SetTrackMuteState | Mute/unmute |
| `set_track_solo_state` | SetTrackSoloState | Solo/unsolo |
| `set_track_record_enable_state` | SetTrackRecordEnableState | Arm/disarm |
| `set_track_input_monitor_state` | SetTrackInputMonitorState | Input monitor |
| `set_track_hidden_state` | SetTrackHiddenState | Hide/show |
| `set_track_inactive_state` | SetTrackInactiveState | Activate/deactivate |
| `set_track_frozen_state` | SetTrackFrozenState | Freeze/unfreeze |
| `set_track_online_state` | SetTrackOnlineState | Online/offline |
| `set_track_open_state` | SetTrackOpenState | Open/close |
| `set_track_color` | SetTrackColor | Set track color |
| `set_track_timebase` | SetTrackTimebase | Set timebase |
| `set_track_main_output_assignments` | SetTrackMainOutputAssignments | Set output |
| `set_track_solo_safe_state` | SetTrackSoloSafeState | Solo safe |
| `set_track_record_safe_enable_state` | SetTrackRecordSafeEnableState | Record safe |
| `set_track_dsp_mode_safe_state` | SetTrackDSPModeSafeState | DSP safe |
| `set_track_smart_dsp_state` | SetTrackSmartDspState | Smart DSP |
| `get_track_control_info` | GetTrackControlInfo | Get control info |
| `get_track_control_breakpoints` | GetTrackControlBreakpoints | Get breakpoints |
| `get_track_playlists` | GetTrackPlaylists | Get playlists |
| `select_tracks_by_name` | SelectTracksByName | Select by name |
| `set_track_control_breakpoints` | SetTrackControlBreakpoints | Set breakpoints |

### Editing Commands (25+ commands)
| MCP Tool Name | PTSL Command | Description |
|---------------|--------------|-------------|
| `cut` | Cut | Cut selection |
| `copy` | Copy | Copy selection |
| `paste` | Paste | Paste clipboard |
| `cut_special` | CutSpecial | Cut special |
| `copy_special` | CopySpecial | Copy special |
| `paste_special` | PasteSpecial | Paste special |
| `clear` | Clear | Clear selection |
| `clear_special` | ClearSpecial | Clear special |
| `trim_to_selection` | TrimToSelection | Trim to selection |
| `repeat_selection` | RepeatSelection | Repeat selection |
| `duplicate_selection` | DuplicateSelection | Duplicate |
| `consolidate_clip` | ConsolidateClip | Consolidate clip |
| `create_fades_based_on_preset` | CreateFadesBasedOnPreset | Create fades |
| `create_audio_clips` | CreateAudioClips | Create audio clips |
| `get_clip_list` | GetClipList | List clips |
| `rename_selected_clip` | RenameSelectedClip | Rename clip |
| `rename_target_clip` | RenameTargetClip | Rename target |
| `select_all_clips_on_track` | SelectAllClipsOnTrack | Select all clips |
| `extend_selection_to_target_tracks` | ExtendSelectionToTargetTracks | Extend selection |
| `group_clips` | GroupClips | Group clips |
| `ungroup_clips` | UngroupClips | Ungroup clips |
| `ungroup_all_clips` | UngroupAllClips | Ungroup all |
| `regroup_clips` | RegroupClips | Regroup clips |
| `spot` | Spot | Spot clip |
| `spot_clips_by_id` | SpotClipsByID | Spot by ID |

### Import/Export (8 commands)
| MCP Tool Name | PTSL Command | Description |
|---------------|--------------|-------------|
| `import` | Import | Import files |
| `import_audio_to_clip_list` | ImportAudioToClipList | Import audio |
| `import_video` | ImportVideo | Import video |
| `export_clips_as_files` | ExportClipsAsFiles | Export clips |
| `export_mix` | ExportMix | Export mix |
| `export_selected_tracks_as_aaf_omf` | ExportSelectedTracksAsAAFOMF | Export AAF/OMF |
| `export_session_info_as_text` | ExportSessionInfoAsText | Export info |
| `get_export_mix_source_list` | GetExportMixSourceList | Get sources |
| `get_media_file_info` | GetMediaFileInfo | Get file info |
| `refresh_all_modified_audio_files` | RefreshAllModifiedAudioFiles | Refresh all |
| `refresh_target_audio_files` | RefreshTargetAudioFiles | Refresh target |
| `get_file_location` | GetFileLocation | Get file location |
| `create_signal_path` | CreateSignalPath | Create signal path |

### Memory Locations (8 commands)
| MCP Tool Name | PTSL Command | Description |
|---------------|--------------|-------------|
| `create_memory_location` | CreateMemoryLocation | Create marker |
| `edit_memory_location` | EditMemoryLocation | Edit marker |
| `clear_memory_location` | ClearMemoryLocation | Clear marker |
| `clear_all_memory_locations` | ClearAllMemoryLocations | Clear all |
| `get_memory_locations` | GetMemoryLocations | Get all markers |
| `select_memory_location` | SelectMemoryLocation | Select marker |
| `get_memory_locations_manage_mode` | GetMemoryLocationsManageMode | Get mode |
| `set_memory_locations_manage_mode` | SetMemoryLocationsManageMode | Set mode |
| `add_length_to_location` | AddLengthToLocation | Add length |
| `add_length_to_position` | AddLengthToPosition | Add to position |
| `subtract_locations` | SubtractLocations | Subtract locations |
| `subtract_positions` | SubtractPositions | Subtract positions |

### Utility & Other (30+ commands)
| MCP Tool Name | PTSL Command | Description |
|---------------|--------------|-------------|
| `register_connection` | RegisterConnection | Register with PT |
| `host_ready_check` | HostReadyCheck | Check if ready |
| `get_ptsl_version` | GetPTSLVersion | Get PTSL version |
| `enable_api` | EnableAPI | Enable API |
| `exchange_public_keys` | ExchangePublicKeys | Exchange keys |
| `get_edit_mode` | GetEditMode | Get edit mode |
| `set_edit_mode` | SetEditMode | Set edit mode |
| `get_edit_mode_options` | GetEditModeOptions | Get mode options |
| `set_edit_mode_options` | SetEditModeOptions | Set mode options |
| `get_edit_tool` | GetEditTool | Get edit tool |
| `set_edit_tool` | SetEditTool | Set edit tool |
| `get_edit_selection` | GetEditSelection | Get selection |
| `set_timeline_selection` | SetTimelineSelection | Set selection |
| `get_timeline_selection` | GetTimelineSelection | Get selection |
| `get_main_counter_format` | GetMainCounterFormat | Get counter |
| `set_main_counter_format` | SetMainCounterFormat | Set counter |
| `get_sub_counter_format` | GetSubCounterFormat | Get sub counter |
| `set_sub_counter_format` | SetSubCounterFormat | Set sub counter |
| `get_time_as_type` | GetTimeAsType | Convert time |
| `undo` | Undo | Undo |
| `redo` | Redo | Redo |
| `undo_all` | UndoAll | Undo all |
| `redo_all` | RedoAll | Redo all |
| `clear_undo_queue` | ClearUndoQueue | Clear undo |
| `get_color_palette` | GetColorPalette | Get colors |
| `get_dynamic_properties` | GetDynamicProperties | Get properties |
| `get_playlist_elements` | GetPlaylistElements | Get elements |
| `get_task_status` | GetTaskStatus | Get task status |
| `get_batch_job_status` | GetBatchJobStatus | Get job status |
| `create_batch_job` | CreateBatchJob | Create batch job |
| `complete_batch_job` | CompleteBatchJob | Complete job |
| `cancel_batch_job` | CancelBatchJob | Cancel job |
| `begin_scrub` | BeginScrub | Begin scrub |
| `continue_scrub` | ContinueScrub | Continue scrub |
| `end_scrub` | EndScrub | End scrub |
| `bounce_track` | BounceTrack | Bounce track |
| `get_monitor_output_path` | GetMonitorOutputPath | Get monitor path |
| `recall_zoom_preset` | RecallZoomPreset | Recall zoom |
| `poll_events` | PollEvents | Poll events |
| `subscribe_to_events` | SubscribeToEvents | Subscribe |
| `unsubscribe_from_events` | UnsubscribeFromEvents | Unsubscribe |
| `install_menu_handler` | InstallMenuHandler | Install handler |
| `uninstall_menu_handler` | UninstallMenuHandler | Uninstall |
| `enable_cue_pro_video_plug_in` | EnableCueProVideoPlugIn | Enable Cue Pro |
| `update_video` | UpdateVideo | Update video |
| `write_selected_transcription_to_json_file` | WriteSelectedTranscriptionToJSONFile | Write transcription |

---

## 📝 Example Prompts for AI Agents

### Basic Operations
```
"Create a new session called 'My Mix' at 48kHz/24-bit"
"Open the session at C:\Sessions\MySession.ptx"
"Mute tracks Audio 1, Audio 2, and Audio 3"
"Solo the Drums track"
"Start recording"
```

### Advanced Workflows
```
"Import all WAV files from C:\Audio\ into new tracks"
"Create memory locations at 1:00:00, 2:00:00, and 3:00:00"
"Export the mix to C:\Exports\mix.wav"
"Get all tracks and their current mute/solo states"
```

---

## 🔧 Development

### Project Structure
```
ptsl-mcp-server/
├── ptsl_mcp_server.py    # MCP server (160 tools exposed)
├── ptsl_client.py        # gRPC client wrapper
├── PTSL_pb2.py          # Generated protobuf stubs
├── PTSL_pb2_grpc.py     # Generated gRPC stubs
├── requirements.txt      # Python dependencies
├── setup.bat            # Windows setup script
├── README.md            # This file
└── HOW_IT_WORKS.md      # Detailed explanation
```

### Regenerating gRPC Stubs

If you have the PTSL SDK and want to regenerate the stubs:

```bash
python -m grpc_tools.protoc \
  -I/path/to/PTSL_SDK/Source \
  --python_out=. \
  --grpc_python_out=. \
  /path/to/PTSL_SDK/Source/PTSL.proto
```

---

## ⚠️ Notes

- **Pro Tools must be running** on the same machine (gRPC binds to localhost:31416)
- **Not all commands work with all Pro Tools versions** — check PTSL version compatibility
- **Plugin parameters cannot be read** via PTSL (limitation of the SDK)
- **Pro Tools only runs on macOS and Windows** — no Linux support

---

## 📄 License

MIT License — see [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **Avid Technology** for the PTSL SDK
- **Anthropic** for the Model Context Protocol specification
- **The Open Source Community** for grpcio, protobuf, and mcp libraries

---

## 🔗 Links

- [Model Context Protocol](https://modelcontextprotocol.io/)
- [Pro Tools Scripting Library Documentation](https://www.avid.com/)
- [Issue Tracker](https://github.com/yourusername/ptsl-mcp-server/issues)

---

**Made with ❤️ for the audio engineering community**
