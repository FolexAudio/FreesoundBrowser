"""
PTSL MCP Server - Model Context Protocol server for Pro Tools Scripting Library.

Exposes PTSL commands as MCP tools so AI agents can control Pro Tools.
Runs locally via stdio transport — no hosting, no internet required.

Usage:
    python ptsl_mcp_server.py

Or configure in Claude Desktop / Hermes config:
    mcp_servers:
      protos:
        command: "python"
        args: ["/path/to/ptsl_mcp_server.py"]
"""

import json
import sys
from typing import Any

from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp.types import Tool, TextContent

from ptsl_client import PTSLClient, list_all_commands

# ─── Server Setup ───────────────────────────────────────────────────────────

app = Server("ptsl-protools")
client = PTSLClient()

# Track connection state
_connected = False
_company_name = "PTSL MCP Server"
_app_name = "AI Agent"


# ─── Tool Definitions ───────────────────────────────────────────────────────
# We define tools as a flat list — each PTSL command becomes a tool.
# The tool schema matches the protobuf RequestBody JSON structure.

TOOLS = {
    # ── Connection ──
    "register_connection": Tool(
        name="register_connection",
        description="Register your application with Pro Tools. MUST be called before any other command. Returns a session_id used for all subsequent requests.",
        inputSchema={
            "type": "object",
            "properties": {
                "company_name": {"type": "string", "description": "Your company/organization name"},
                "application_name": {"type": "string", "description": "Your application name"},
            },
            "required": ["company_name", "application_name"],
        },
    ),
    "host_ready_check": Tool(
        name="host_ready_check",
        description="Check if Pro Tools is running and ready to accept commands.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_ptsl_version": Tool(
        name="get_ptsl_version",
        description="Get the PTSL version running in Pro Tools.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Session File ──
    "create_session": Tool(
        name="create_session",
        description="Create a new Pro Tools session. Specify name, location, sample rate, bit depth, and optional template.",
        inputSchema={
            "type": "object",
            "properties": {
                "session_name": {"type": "string", "description": "Name for the new session"},
                "session_location": {"type": "string", "description": "Directory path for the session"},
                "file_type": {"type": "string", "enum": ["WAVE", "AIFF"], "default": "WAVE"},
                "sample_rate": {"type": "string", "enum": ["44100", "48000", "88200", "96000", "176400", "192000"], "default": "48000"},
                "bit_depth": {"type": "string", "enum": ["16", "24", "32Float"], "default": "24"},
                "is_interleaved": {"type": "boolean", "default": False},
                "input_output_settings": {"type": "string", "default": "StereoMix"},
                "create_from_template": {"type": "boolean", "default": False},
                "template_group": {"type": "string"},
                "template_name": {"type": "string"},
            },
            "required": ["session_name", "session_location"],
        },
    ),
    "open_session": Tool(
        name="open_session",
        description="Open an existing Pro Tools session file (.ptx).",
        inputSchema={
            "type": "object",
            "properties": {
                "session_path": {"type": "string", "description": "Full path to .ptx session file"},
            },
            "required": ["session_path"],
        },
    ),
    "close_session": Tool(
        name="close_session",
        description="Close the current Pro Tools session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "save_session": Tool(
        name="save_session",
        description="Save the current Pro Tools session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "save_session_as": Tool(
        name="save_session_as",
        description="Save the current session with a new name/location.",
        inputSchema={
            "type": "object",
            "properties": {
                "session_name": {"type": "string"},
                "session_location": {"type": "string"},
            },
            "required": ["session_name", "session_location"],
        },
    ),

    # ── Session Info ──
    "get_session_name": Tool(
        name="get_session_name",
        description="Get the name of the currently open Pro Tools session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_session_path": Tool(
        name="get_session_path",
        description="Get the file path of the currently open session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_session_sample_rate": Tool(
        name="get_session_sample_rate",
        description="Get the sample rate of the current session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_session_bit_depth": Tool(
        name="get_session_bit_depth",
        description="Get the bit depth of the current session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_session_length": Tool(
        name="get_session_length",
        description="Get the length of the current session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_session_start_time": Tool(
        name="get_session_start_time",
        description="Get the start time of the current session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_session_time_code_rate": Tool(
        name="get_session_time_code_rate",
        description="Get the timecode rate of the current session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_session_audio_format": Tool(
        name="get_session_audio_format",
        description="Get the audio format of the current session.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Transport ──
    "get_transport_state": Tool(
        name="get_transport_state",
        description="Get current transport state (playing, stopped, recording).",
        inputSchema={"type": "object", "properties": {}},
    ),
    "toggle_play_state": Tool(
        name="toggle_play_state",
        description="Toggle between play and stop. If stopped, starts playback. If playing, stops.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "toggle_record_enable": Tool(
        name="toggle_record_enable",
        description="Toggle record enable on the transport.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_playback_mode": Tool(
        name="set_playback_mode",
        description="Set the playback mode.",
        inputSchema={
            "type": "object",
            "properties": {
                "mode": {"type": "string", "enum": ["Normal", "Loop", "DynamicTransport"]},
            },
            "required": ["mode"],
        },
    ),
    "set_record_mode": Tool(
        name="set_record_mode",
        description="Set the record mode.",
        inputSchema={
            "type": "object",
            "properties": {
                "mode": {"type": "string", "enum": ["Normal", "Loop", "Destructive", "QuickPunch"]},
            },
            "required": ["mode"],
        },
    ),

    # ── Tracks ──
    "get_track_list": Tool(
        name="get_track_list",
        description="Get list of all tracks in the session. Returns track names, types, formats, colors, and attributes.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_filter": {"type": "string", "description": "Optional filter: Selected, Muted, Solo, RecordEnabled, Inactive, Hidden, Frozen, Open, Online"},
                "limit": {"type": "integer", "description": "Max tracks to return (0 = all)"},
                "offset": {"type": "integer", "description": "Starting offset for pagination"},
            },
        },
    ),
    "create_new_tracks": Tool(
        name="create_new_tracks",
        description="Create one or more new tracks in the session.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_count": {"type": "integer", "description": "Number of tracks to create"},
                "track_type": {"type": "string", "enum": ["Audio", "MIDI", "AuxInput", "VCA", "MasterFader", "Instrument"]},
                "track_format": {"type": "string", "enum": ["Mono", "Stereo", "LCRS", "5.0", "5.1", "7.1", "DolbyAtmos", "Adaptive"]},
                "track_name": {"type": "string", "description": "Name for the new track(s)"},
            },
            "required": ["track_count", "track_type"],
        },
    ),
    "select_tracks_by_name": Tool(
        name="select_tracks_by_name",
        description="Select tracks by their names.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "select": {"type": "boolean", "description": "True to select, False to deselect"},
            },
            "required": ["track_names"],
        },
    ),
    # "rename_track" removed — use rename_target_track or rename_selected_clip instead
    "set_track_mute_state": Tool(
        name="set_track_mute_state",
        description="Mute or unmute one or more tracks by name.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}, "description": "Track names to mute/unmute"},
                "enabled": {"type": "boolean", "description": "True = muted, False = unmuted"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_solo_state": Tool(
        name="set_track_solo_state",
        description="Solo or unsolo one or more tracks by name.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_record_enable_state": Tool(
        name="set_track_record_enable_state",
        description="Enable or disable record on one or more tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_input_monitor_state": Tool(
        name="set_track_input_monitor_state",
        description="Enable or disable input monitoring on tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_hidden_state": Tool(
        name="set_track_hidden_state",
        description="Hide or show tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_inactive_state": Tool(
        name="set_track_inactive_state",
        description="Make tracks active or inactive (frees DSP).",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_frozen_state": Tool(
        name="set_track_frozen_state",
        description="Freeze or unfreeze tracks (renders plugins to audio).",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_online_state": Tool(
        name="set_track_online_state",
        description="Set tracks online or offline.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_open_state": Tool(
        name="set_track_open_state",
        description="Open or close track views.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),

    # ── Editing ──
    "cut": Tool(
        name="cut",
        description="Cut the current selection to clipboard.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "copy": Tool(
        name="copy",
        description="Copy the current selection to clipboard.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "paste": Tool(
        name="paste",
        description="Paste from clipboard at current cursor position.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "clear": Tool(
        name="clear",
        description="Delete/clear the current selection.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "consolidate_clip": Tool(
        name="consolidate_clip",
        description="Consolidate selected clips into a single clip.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "trim_to_selection": Tool(
        name="trim_to_selection",
        description="Trim clips to the current selection.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "repeat_selection": Tool(
        name="repeat_selection",
        description="Repeat the selected clips.",
        inputSchema={
            "type": "object",
            "properties": {
                "num_repeats": {"type": "integer", "description": "Number of times to repeat"},
            },
            "required": ["num_repeats"],
        },
    ),
    "duplicate_selection": Tool(
        name="duplicate_selection",
        description="Duplicate the current selection.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "group_clips": Tool(
        name="group_clips",
        description="Group selected clips together.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "ungroup_clips": Tool(
        name="ungroup_clips",
        description="Ungroup selected clips.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "create_fade": Tool(
        name="create_fade",
        description="Create fades on selected clips using a saved preset. Maps to SDK CreateFadesBasedOnPreset.",
        inputSchema={
            "type": "object",
            "properties": {
                "preset_name": {"type": "string", "description": "Name of the fade preset to apply"},
            },
            "required": ["preset_name"],
        },
    ),
    "set_timeline_selection": Tool(
        name="set_timeline_selection",
        description="Set the timeline selection (start/end points).",
        inputSchema={
            "type": "object",
            "properties": {
                "start_point": {"type": "string", "description": "Start position (samples, or timecode)"},
                "end_point": {"type": "string", "description": "End position"},
                "reference": {"type": "string", "enum": ["Absolute", "Relative"]},
            },
            "required": ["start_point", "end_point"],
        },
    ),
    "get_timeline_selection": Tool(
        name="get_timeline_selection",
        description="Get the current timeline selection.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Import / Export ──
    "import": Tool(
        name="import",
        description="Import audio/MIDI files into the session. Place on new or existing tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "file_list": {"type": "array", "items": {"type": "string"}, "description": "List of file paths to import"},
                "destination": {"type": "string", "enum": ["NewTrack", "SelectedTrack", "ExistingTrack"]},
                "location": {"type": "string", "enum": ["Spot", "SessionStart", "SelectionStart"]},
            },
            "required": ["file_list", "destination"],
        },
    ),
    "import_video": Tool(
        name="import_video",
        description="Import a video file into the session.",
        inputSchema={
            "type": "object",
            "properties": {
                "video_path": {"type": "string", "description": "Path to video file"},
            },
            "required": ["video_path"],
        },
    ),
    "export_clips_as_files": Tool(
        name="export_clips_as_files",
        description="Export selected clips as audio files.",
        inputSchema={
            "type": "object",
            "properties": {
                "output_directory": {"type": "string"},
                "file_type": {"type": "string", "enum": ["WAVE", "AIFF", "MP3", "FLAC"]},
            },
            "required": ["output_directory"],
        },
    ),
    "export_mix": Tool(
        name="export_mix",
        description="Bounce/export the mix to an audio file.",
        inputSchema={
            "type": "object",
            "properties": {
                "output_path": {"type": "string", "description": "Full output file path"},
                "file_type": {"type": "string", "enum": ["WAVE", "AIFF", "MP3"]},
                "bit_depth": {"type": "string", "enum": ["16", "24", "32Float"]},
                "sample_rate": {"type": "string", "enum": ["44100", "48000", "88200", "96000"]},
            },
            "required": ["output_path"],
        },
    ),
    "export_session_info_as_text": Tool(
        name="export_session_info_as_text",
        description="Export session information as a text file (track list, clip list, markers, etc).",
        inputSchema={
            "type": "object",
            "properties": {
                "output_path": {"type": "string", "description": "Output file path"},
                "include_file_list": {"type": "boolean", "default": True},
                "include_clip_list": {"type": "boolean", "default": True},
                "include_markers": {"type": "boolean", "default": True},
                "include_plugin_list": {"type": "boolean", "default": True},
                "include_track_edls": {"type": "boolean", "default": True},
            },
            "required": ["output_path"],
        },
    ),

    # ── Memory Locations (Markers) ──
    "get_memory_locations": Tool(
        name="get_memory_locations",
        description="Get all memory locations (markers) in the session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "create_memory_location": Tool(
        name="create_memory_location",
        description="Create a new memory location (marker).",
        inputSchema={
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "start_time": {"type": "string"},
                "end_time": {"type": "string"},
                "time_properties": {"type": "string", "enum": ["Marker", "Selection", "Punch"]},
            },
            "required": ["name", "start_time"],
        },
    ),
    "select_memory_location": Tool(
        name="select_memory_location",
        description="Select/recall a memory location by index.",
        inputSchema={
            "type": "object",
            "properties": {
                "number": {"type": "integer", "description": "Memory location number (1-based)"},
            },
            "required": ["number"],
        },
    ),

    # ── Undo/Redo ──
    "undo": Tool(
        name="undo",
        description="Undo the last action.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "redo": Tool(
        name="redo",
        description="Redo the last undone action.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "undo_all": Tool(
        name="undo_all",
        description="Undo all actions back to the last save.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Edit Mode/Tool ──
    "get_edit_mode": Tool(
        name="get_edit_mode",
        description="Get the current edit mode (Slip, Shuffle, Spot, Grid).",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_edit_mode": Tool(
        name="set_edit_mode",
        description="Set the edit mode.",
        inputSchema={
            "type": "object",
            "properties": {
                "mode": {"type": "string", "enum": ["Slip", "Shuffle", "Spot", "Grid"]},
            },
            "required": ["mode"],
        },
    ),
    "get_edit_tool": Tool(
        name="get_edit_tool",
        description="Get the currently selected edit tool.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_edit_tool": Tool(
        name="set_edit_tool",
        description="Set the edit tool.",
        inputSchema={
            "type": "object",
            "properties": {
                "tool": {"type": "string", "enum": ["Zoom", "Trim", "Selector", "Grabber", "Smart", "Scrubber", "Pencil"]},
            },
            "required": ["tool"],
        },
    ),

    # ── Zoom ──
    "recall_zoom_preset": Tool(
        name="recall_zoom_preset",
        description="Recall a saved zoom preset (1-5).",
        inputSchema={
            "type": "object",
            "properties": {
                "preset_number": {"type": "integer", "minimum": 1, "maximum": 5},
            },
            "required": ["preset_number"],
        },
    ),

    # ── Session Settings ──
    "get_session_ids": Tool(
        name="get_session_ids",
        description="Get the session ID and related identifiers.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_dynamic_properties": Tool(
        name="get_dynamic_properties",
        description="Get dynamic properties of the current session.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Refresh ──
    "refresh_all_modified_audio_files": Tool(
        name="refresh_all_modified_audio_files",
        description="Refresh all modified audio files in the session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "refresh_target_audio_files": Tool(
        name="refresh_target_audio_files",
        description="Refresh specific target audio files.",
        inputSchema={
            "type": "object",
            "properties": {
                "file_paths": {"type": "array", "items": {"type": "string"}, "description": "Audio file paths to refresh"},
            },
            "required": ["file_paths"],
        },
    ),

    # ── Transport (additional) ──
    "play_half_speed": Tool(
        name="play_half_speed",
        description="Start playback at half speed.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "record_half_speed": Tool(
        name="record_half_speed",
        description="Start recording at half speed.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_playback_mode": Tool(
        name="get_playback_mode",
        description="Get the current playback mode (Normal, Loop, DynamicTransport).",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_record_mode": Tool(
        name="get_record_mode",
        description="Get the current record mode (Normal, Loop, Destructive, QuickPunch).",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_transport_armed": Tool(
        name="get_transport_armed",
        description="Get the list of record-armed tracks.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Track (additional) ──
    "delete_tracks": Tool(
        name="delete_tracks",
        description="Delete one or more tracks by name.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}, "description": "Track names to delete"},
            },
            "required": ["track_names"],
        },
    ),
    "bounce_track": Tool(
        name="bounce_track",
        description="Bounce a track to audio.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_name": {"type": "string", "description": "Track to bounce"},
            },
            "required": ["track_name"],
        },
    ),
    "set_track_color": Tool(
        name="set_track_color",
        description="Set the color of one or more tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "color": {"type": "string", "description": "Hex color (e.g. #ff13355f)"},
            },
            "required": ["track_names", "color"],
        },
    ),
    "set_track_timebase": Tool(
        name="set_track_timebase",
        description="Set the timebase for tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "timebase": {"type": "string", "enum": ["Samples", "Ticks", "TimeCode"]},
            },
            "required": ["track_names", "timebase"],
        },
    ),
    "set_track_dsp_mode_safe_state": Tool(
        name="set_track_dsp_mode_safe_state",
        description="Set DSP mode safe state on tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_smart_dsp_state": Tool(
        name="set_track_smart_dsp_state",
        description="Set smart DSP state on tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_solo_safe_state": Tool(
        name="set_track_solo_safe_state",
        description="Set solo safe state on tracks (protects from being unsoloed).",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_record_safe_enable_state": Tool(
        name="set_track_record_safe_enable_state",
        description="Set record safe state on tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "enabled": {"type": "boolean"},
            },
            "required": ["track_names", "enabled"],
        },
    ),
    "set_track_main_output_assignments": Tool(
        name="set_track_main_output_assignments",
        description="Set main output bus assignments for tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "output_path": {"type": "string", "description": "Output bus path"},
            },
            "required": ["track_names", "output_path"],
        },
    ),
    "get_track_control_info": Tool(
        name="get_track_control_info",
        description="Get control information for tracks (volume, pan, sends, etc).",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}, "description": "Optional track name filter"},
            },
        },
    ),
    "get_track_control_breakpoints": Tool(
        name="get_track_control_breakpoints",
        description="Get automation breakpoint data for track controls.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
            },
            "required": ["track_names"],
        },
    ),
    "set_track_control_breakpoints": Tool(
        name="set_track_control_breakpoints",
        description="Set automation breakpoint data for track controls.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
                "breakpoint_data": {"type": "string", "description": "JSON breakpoint data"},
            },
            "required": ["track_names", "breakpoint_data"],
        },
    ),

    # ── Clips ──
    "select_all_clips_on_track": Tool(
        name="select_all_clips_on_track",
        description="Select all clips on the specified track.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_name": {"type": "string"},
            },
            "required": ["track_name"],
        },
    ),
    "rename_selected_clip": Tool(
        name="rename_selected_clip",
        description="Rename the currently selected clip.",
        inputSchema={
            "type": "object",
            "properties": {
                "new_name": {"type": "string"},
            },
            "required": ["new_name"],
        },
    ),
    "rename_target_clip": Tool(
        name="rename_target_clip",
        description="Rename a target clip by ID.",
        inputSchema={
            "type": "object",
            "properties": {
                "clip_id": {"type": "string", "description": "Clip ID"},
                "new_name": {"type": "string"},
            },
            "required": ["clip_id", "new_name"],
        },
    ),
    "rename_target_track": Tool(
        name="rename_target_track",
        description="Rename a track by name.",
        inputSchema={
            "type": "object",
            "properties": {
                "old_name": {"type": "string"},
                "new_name": {"type": "string"},
            },
            "required": ["old_name", "new_name"],
        },
    ),
    "create_audio_clips": Tool(
        name="create_audio_clips",
        description="Create new audio clips on a track.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_name": {"type": "string"},
                "clip_name": {"type": "string"},
                "start_time": {"type": "string"},
                "end_time": {"type": "string"},
            },
            "required": ["track_name", "clip_name"],
        },
    ),
    "spot_clips_by_id": Tool(
        name="spot_clips_by_id",
        description="Spot (place at specific time) clips by their IDs.",
        inputSchema={
            "type": "object",
            "properties": {
                "clip_ids": {"type": "array", "items": {"type": "string"}},
                "start_time": {"type": "string"},
            },
            "required": ["clip_ids", "start_time"],
        },
    ),
    "spot": Tool(
        name="spot",
        description="Spot the current selection to a specific location.",
        inputSchema={
            "type": "object",
            "properties": {
                "start_time": {"type": "string"},
            },
            "required": ["start_time"],
        },
    ),
    "ungroup_all_clips": Tool(
        name="ungroup_all_clips",
        description="Ungroup all clips in the session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "regroup_clips": Tool(
        name="regroup_clips",
        description="Regroup previously ungrouped clips.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_clip_list": Tool(
        name="get_clip_list",
        description="Get list of all clips in the session.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_filter": {"type": "string", "description": "Optional track filter"},
            },
        },
    ),

    # ── Clipboard Special ──
    "cut_special": Tool(
        name="cut_special",
        description="Cut with special options (preserves automation, etc).",
        inputSchema={
            "type": "object",
            "properties": {
                "preserve_special": {"type": "boolean", "default": True},
            },
        },
    ),
    "copy_special": Tool(
        name="copy_special",
        description="Copy with special options.",
        inputSchema={
            "type": "object",
            "properties": {
                "preserve_special": {"type": "boolean", "default": True},
            },
        },
    ),
    "paste_special": Tool(
        name="paste_special",
        description="Paste with special options.",
        inputSchema={
            "type": "object",
            "properties": {
                "preserve_special": {"type": "boolean", "default": True},
            },
        },
    ),
    "clear_special": Tool(
        name="clear_special",
        description="Clear with special options.",
        inputSchema={
            "type": "object",
            "properties": {
                "preserve_special": {"type": "boolean", "default": True},
            },
        },
    ),

    # ── Scrub ──
    "begin_scrub": Tool(
        name="begin_scrub",
        description="Begin audio scrubbing at a position.",
        inputSchema={
            "type": "object",
            "properties": {
                "start_time": {"type": "string"},
            },
            "required": ["start_time"],
        },
    ),
    "continue_scrub": Tool(
        name="continue_scrub",
        description="Continue scrubbing to a new position.",
        inputSchema={
            "type": "object",
            "properties": {
                "time": {"type": "string"},
            },
            "required": ["time"],
        },
    ),
    "end_scrub": Tool(
        name="end_scrub",
        description="End audio scrubbing.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Navigation (position offsets) ──
    "add_length_to_position": Tool(
        name="add_length_to_position",
        description="Add a length offset to the current playhead position.",
        inputSchema={
            "type": "object",
            "properties": {
                "length": {"type": "string", "description": "Length to add (samples or timecode)"},
            },
            "required": ["length"],
        },
    ),
    "add_length_to_location": Tool(
        name="add_length_to_location",
        description="Add a length offset to a memory location.",
        inputSchema={
            "type": "object",
            "properties": {
                "location_number": {"type": "integer"},
                "length": {"type": "string"},
            },
            "required": ["location_number", "length"],
        },
    ),
    "subtract_positions": Tool(
        name="subtract_positions",
        description="Subtract two positions to get a length.",
        inputSchema={
            "type": "object",
            "properties": {
                "position_a": {"type": "string"},
                "position_b": {"type": "string"},
            },
            "required": ["position_a", "position_b"],
        },
    ),
    "subtract_locations": Tool(
        name="subtract_locations",
        description="Subtract two memory locations to get a length.",
        inputSchema={
            "type": "object",
            "properties": {
                "location_a": {"type": "integer"},
                "location_b": {"type": "integer"},
            },
            "required": ["location_a", "location_b"],
        },
    ),

    # ── Batch Jobs ──
    "create_batch_job": Tool(
        name="create_batch_job",
        description="Create a new batch job for processing multiple files.",
        inputSchema={
            "type": "object",
            "properties": {
                "job_name": {"type": "string"},
            },
            "required": ["job_name"],
        },
    ),
    "get_batch_job_status": Tool(
        name="get_batch_job_status",
        description="Get the status of a batch job.",
        inputSchema={
            "type": "object",
            "properties": {
                "job_id": {"type": "string"},
            },
            "required": ["job_id"],
        },
    ),
    "cancel_batch_job": Tool(
        name="cancel_batch_job",
        description="Cancel a running batch job.",
        inputSchema={
            "type": "object",
            "properties": {
                "job_id": {"type": "string"},
            },
            "required": ["job_id"],
        },
    ),
    "complete_batch_job": Tool(
        name="complete_batch_job",
        description="Mark a batch job as complete.",
        inputSchema={
            "type": "object",
            "properties": {
                "job_id": {"type": "string"},
            },
            "required": ["job_id"],
        },
    ),

    # ── Memory Locations (additional) ──
    "edit_memory_location": Tool(
        name="edit_memory_location",
        description="Edit an existing memory location.",
        inputSchema={
            "type": "object",
            "properties": {
                "number": {"type": "integer"},
                "name": {"type": "string"},
                "start_time": {"type": "string"},
                "end_time": {"type": "string"},
                "comment": {"type": "string"},
            },
            "required": ["number"],
        },
    ),
    "clear_memory_location": Tool(
        name="clear_memory_location",
        description="Delete a specific memory location.",
        inputSchema={
            "type": "object",
            "properties": {
                "number": {"type": "integer"},
            },
            "required": ["number"],
        },
    ),
    "clear_all_memory_locations": Tool(
        name="clear_all_memory_locations",
        description="Delete all memory locations in the session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_memory_locations_manage_mode": Tool(
        name="get_memory_locations_manage_mode",
        description="Get the memory locations manage mode setting.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_memory_locations_manage_mode": Tool(
        name="set_memory_locations_manage_mode",
        description="Set the memory locations manage mode.",
        inputSchema={
            "type": "object",
            "properties": {
                "mode": {"type": "string"},
            },
            "required": ["mode"],
        },
    ),

    # ── Undo/Redo (additional) ──
    "clear_undo_queue": Tool(
        name="clear_undo_queue",
        description="Clear the undo history.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "redo_all": Tool(
        name="redo_all",
        description="Redo all undone actions.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Playlists ──
    "get_playlist_elements": Tool(
        name="get_playlist_elements",
        description="Get playlist elements for a track.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_name": {"type": "string"},
            },
            "required": ["track_name"],
        },
    ),

    # ── Session Settings (additional) ──
    "get_session_audio_rate_pull_settings": Tool(
        name="get_session_audio_rate_pull_settings",
        description="Get audio rate pull settings for the session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_session_audio_rate_pull_settings": Tool(
        name="set_session_audio_rate_pull_settings",
        description="Set audio rate pull settings.",
        inputSchema={
            "type": "object",
            "properties": {
                "pull_type": {"type": "string"},
            },
            "required": ["pull_type"],
        },
    ),
    "get_session_video_rate_pull_settings": Tool(
        name="get_session_video_rate_pull_settings",
        description="Get video rate pull settings for the session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_session_video_rate_pull_settings": Tool(
        name="set_session_video_rate_pull_settings",
        description="Set video rate pull settings.",
        inputSchema={
            "type": "object",
            "properties": {
                "pull_type": {"type": "string"},
            },
            "required": ["pull_type"],
        },
    ),
    "get_session_feet_frames_rate": Tool(
        name="get_session_feet_frames_rate",
        description="Get the feet+frames rate setting.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_session_feet_frames_rate": Tool(
        name="set_session_feet_frames_rate",
        description="Set the feet+frames rate.",
        inputSchema={
            "type": "object",
            "properties": {
                "rate": {"type": "string"},
            },
            "required": ["rate"],
        },
    ),
    "get_session_interleaved_state": Tool(
        name="get_session_interleaved_state",
        description="Get whether the session uses interleaved audio.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_session_interleaved_state": Tool(
        name="set_session_interleaved_state",
        description="Set interleaved audio state.",
        inputSchema={
            "type": "object",
            "properties": {
                "is_interleaved": {"type": "boolean"},
            },
            "required": ["is_interleaved"],
        },
    ),
    "get_session_system_delay_info": Tool(
        name="get_session_system_delay_info",
        description="Get system delay compensation info.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_session_length": Tool(
        name="set_session_length",
        description="Set the session length.",
        inputSchema={
            "type": "object",
            "properties": {
                "session_length": {"type": "string"},
            },
            "required": ["session_length"],
        },
    ),
    "set_session_start_time": Tool(
        name="set_session_start_time",
        description="Set the session start time.",
        inputSchema={
            "type": "object",
            "properties": {
                "start_time": {"type": "string"},
            },
            "required": ["start_time"],
        },
    ),

    # ── Counters ──
    "get_main_counter_format": Tool(
        name="get_main_counter_format",
        description="Get the main counter display format.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_main_counter_format": Tool(
        name="set_main_counter_format",
        description="Set the main counter display format.",
        inputSchema={
            "type": "object",
            "properties": {
                "format": {"type": "string"},
            },
            "required": ["format"],
        },
    ),
    "get_sub_counter_format": Tool(
        name="get_sub_counter_format",
        description="Get the sub counter display format.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_sub_counter_format": Tool(
        name="set_sub_counter_format",
        description="Set the sub counter display format.",
        inputSchema={
            "type": "object",
            "properties": {
                "format": {"type": "string"},
            },
            "required": ["format"],
        },
    ),
    "get_time_as_type": Tool(
        name="get_time_as_type",
        description="Get the current time in a specific format.",
        inputSchema={
            "type": "object",
            "properties": {
                "time_format": {"type": "string", "description": "Target time format"},
            },
            "required": ["time_format"],
        },
    ),

    # ── Audio File Operations ──
    "import_audio_to_clip_list": Tool(
        name="import_audio_to_clip_list",
        description="Import audio files directly to the clip list (not on tracks).",
        inputSchema={
            "type": "object",
            "properties": {
                "file_list": {"type": "array", "items": {"type": "string"}},
            },
            "required": ["file_list"],
        },
    ),
    "export_selected_tracks_as_aaf_omf": Tool(
        name="export_selected_tracks_as_aaf_omf",
        description="Export selected tracks as AAF/OMF file.",
        inputSchema={
            "type": "object",
            "properties": {
                "output_path": {"type": "string"},
                "format": {"type": "string", "enum": ["AAF", "OMF"]},
            },
            "required": ["output_path"],
        },
    ),
    "get_media_file_info": Tool(
        name="get_media_file_info",
        description="Get detailed info about a media file.",
        inputSchema={
            "type": "object",
            "properties": {
                "file_path": {"type": "string"},
            },
            "required": ["file_path"],
        },
    ),
    "get_export_mix_source_list": Tool(
        name="get_export_mix_source_list",
        description="Get the list of available sources for mix export.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_monitor_output_path": Tool(
        name="get_monitor_output_path",
        description="Get the monitor output path configuration.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Edit Mode Options ──
    "get_edit_mode_options": Tool(
        name="get_edit_mode_options",
        description="Get current edit mode options/settings.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "set_edit_mode_options": Tool(
        name="set_edit_mode_options",
        description="Set edit mode options.",
        inputSchema={
            "type": "object",
            "properties": {
                "options": {"type": "string", "description": "Edit mode options JSON"},
            },
            "required": ["options"],
        },
    ),

    # ── Color Palette ──
    "get_color_palette": Tool(
        name="get_color_palette",
        description="Get the session color palette.",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Signal Path ──
    "create_signal_path": Tool(
        name="create_signal_path",
        description="Create a new signal path (routing) in the session.",
        inputSchema={
            "type": "object",
            "properties": {
                "source": {"type": "string"},
                "destination": {"type": "string"},
            },
            "required": ["source", "destination"],
        },
    ),

    # ── Menu/API ──
    "enable_api": Tool(
        name="enable_api",
        description="Enable the Pro Tools API.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "install_menu_handler": Tool(
        name="install_menu_handler",
        description="Install a custom menu handler.",
        inputSchema={
            "type": "object",
            "properties": {
                "menu_path": {"type": "string"},
                "handler_id": {"type": "string"},
            },
            "required": ["menu_path", "handler_id"],
        },
    ),
    "uninstall_menu_handler": Tool(
        name="uninstall_menu_handler",
        description="Uninstall a custom menu handler.",
        inputSchema={
            "type": "object",
            "properties": {
                "handler_id": {"type": "string"},
            },
            "required": ["handler_id"],
        },
    ),

    # ── Events ──
    "subscribe_to_events": Tool(
        name="subscribe_to_events",
        description="Subscribe to Pro Tools events (transport changes, track updates, etc).",
        inputSchema={
            "type": "object",
            "properties": {
                "event_ids": {"type": "array", "items": {"type": "string"}, "description": "Event types to subscribe to"},
            },
            "required": ["event_ids"],
        },
    ),
    "unsubscribe_from_events": Tool(
        name="unsubscribe_from_events",
        description="Unsubscribe from Pro Tools events.",
        inputSchema={
            "type": "object",
            "properties": {
                "event_ids": {"type": "array", "items": {"type": "string"}},
            },
            "required": ["event_ids"],
        },
    ),
    "poll_events": Tool(
        name="poll_events",
        description="Poll for pending Pro Tools events (streaming).",
        inputSchema={"type": "object", "properties": {}},
    ),

    # ── Remaining SDK commands ──
    "create_fades_based_on_preset": Tool(
        name="create_fades_based_on_preset",
        description="Create fades on selected clips using a saved preset.",
        inputSchema={
            "type": "object",
            "properties": {
                "preset_name": {"type": "string", "description": "Fade preset name"},
            },
            "required": ["preset_name"],
        },
    ),
    "enable_cue_pro_video_plug_in": Tool(
        name="enable_cue_pro_video_plug_in",
        description="Enable or disable the Cue Pro Video plug-in.",
        inputSchema={
            "type": "object",
            "properties": {
                "enabled": {"type": "boolean"},
            },
            "required": ["enabled"],
        },
    ),
    "exchange_public_keys": Tool(
        name="exchange_public_keys",
        description="Exchange public keys for secure communication.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "extend_selection_to_target_tracks": Tool(
        name="extend_selection_to_target_tracks",
        description="Extend the current selection to include target tracks.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_names": {"type": "array", "items": {"type": "string"}},
            },
            "required": ["track_names"],
        },
    ),
    "get_edit_selection": Tool(
        name="get_edit_selection",
        description="Get the current edit selection boundaries.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "get_file_location": Tool(
        name="get_file_location",
        description="Get the file location/path for a media file by name.",
        inputSchema={
            "type": "object",
            "properties": {
                "file_name": {"type": "string"},
            },
            "required": ["file_name"],
        },
    ),
    "get_task_status": Tool(
        name="get_task_status",
        description="Get the status of an asynchronous task.",
        inputSchema={
            "type": "object",
            "properties": {
                "task_id": {"type": "string"},
            },
            "required": ["task_id"],
        },
    ),
    "get_track_playlists": Tool(
        name="get_track_playlists",
        description="Get all playlists (takes) for a track.",
        inputSchema={
            "type": "object",
            "properties": {
                "track_name": {"type": "string"},
            },
            "required": ["track_name"],
        },
    ),
    "set_session_audio_format": Tool(
        name="set_session_audio_format",
        description="Set the session audio file format.",
        inputSchema={
            "type": "object",
            "properties": {
                "audio_format": {"type": "string", "description": "Audio format setting"},
            },
            "required": ["audio_format"],
        },
    ),
    "set_session_bit_depth": Tool(
        name="set_session_bit_depth",
        description="Set the session bit depth.",
        inputSchema={
            "type": "object",
            "properties": {
                "bit_depth": {"type": "string", "enum": ["16", "24", "32Float"]},
            },
            "required": ["bit_depth"],
        },
    ),
    "set_session_time_code_rate": Tool(
        name="set_session_time_code_rate",
        description="Set the session timecode rate.",
        inputSchema={
            "type": "object",
            "properties": {
                "time_code_rate": {"type": "string", "description": "Timecode rate"},
            },
            "required": ["time_code_rate"],
        },
    ),
    "update_video": Tool(
        name="update_video",
        description="Refresh/update the video playback in the session.",
        inputSchema={"type": "object", "properties": {}},
    ),
    "write_selected_transcription_to_json_file": Tool(
        name="write_selected_transcription_to_json_file",
        description="Write transcription data for selected clips to a JSON file.",
        inputSchema={
            "type": "object",
            "properties": {
                "output_path": {"type": "string"},
            },
            "required": ["output_path"],
        },
    ),
}


# ─── MCP Handlers ──────────────────────────────────────────────────────────

@app.list_tools()
async def list_tools() -> list[Tool]:
    """Return all available PTSL tools."""
    return list(TOOLS.values())


@app.call_tool()
async def call_tool(name: str, arguments: dict[str, Any]) -> list[TextContent]:
    """Handle tool calls by dispatching to PTSL gRPC client."""

    global _connected

    # ── Connection management ──
    if name == "register_connection":
        global _company_name, _app_name
        _company_name = arguments.get("company_name", _company_name)
        _app_name = arguments.get("application_name", _app_name)

        try:
            client.connect()
            result = client.register(_company_name, _app_name)
            if result["status"] == "Completed":
                _connected = True
            return [TextContent(type="text", text=json.dumps(result, indent=2))]
        except Exception as e:
            return [TextContent(type="text", text=json.dumps({
                "status": "ERROR",
                "error": f"Failed to connect to Pro Tools: {str(e)}. Is Pro Tools running on localhost:31416?",
            }, indent=2))]

    # ── All other commands require connection ──
    if not _connected:
        return [TextContent(type="text", text=json.dumps({
            "status": "ERROR",
            "error": "Not connected to Pro Tools. Call register_connection first.",
        }, indent=2))]

    # ── Map tool name to PTSL command name ──
    # Tool names use underscores, PTSL commands use PascalCase
    # e.g., "get_track_list" → "GetTrackList"
    command_name = "".join(word.capitalize() for word in name.split("_"))

    # Special case: "import" tool maps to CId_Import
    if name == "import":
        command_name = "Import"
    # Acronym commands — auto-capitalize doesn't handle these
    elif name == "enable_api":
        command_name = "EnableAPI"
    elif name == "export_selected_tracks_as_aaf_omf":
        command_name = "ExportSelectedTracksAsAAFOMF"
    elif name == "get_ptsl_version":
        command_name = "GetPTSLVersion"
    elif name == "get_session_ids":
        command_name = "GetSessionIDs"
    elif name == "set_track_dsp_mode_safe_state":
        command_name = "SetTrackDSPModeSafeState"
    elif name == "spot_clips_by_id":
        command_name = "SpotClipsByID"
    elif name == "enable_cue_pro_video_plug_in":
        command_name = "EnableCueProVideoPlugIn"
    elif name == "write_selected_transcription_to_json_file":
        command_name = "WriteSelectedTranscriptionToJSONFile"
    elif name == "create_fade":
        command_name = "CreateFadesBasedOnPreset"

    # ── Build request body from arguments ──
    body = _build_body(name, arguments)

    # ── Send command ──
    try:
        result = client.send_command(command_name, body)
        return [TextContent(type="text", text=json.dumps(result, indent=2))]
    except Exception as e:
        return [TextContent(type="text", text=json.dumps({
            "status": "ERROR",
            "error": f"Command failed: {str(e)}",
        }, indent=2))]


def _build_body(tool_name: str, args: dict) -> dict:
    """
    Convert MCP tool arguments to PTSL request body format.
    Maps friendly parameter names to the protobuf JSON field names.
    """
    body = {}

    # ── Create Session ──
    if tool_name == "create_session":
        body = {
            "session_name": args.get("session_name", ""),
            "session_location": args.get("session_location", ""),
            "file_type": f"FT_{args.get('file_type', 'WAVE').upper()}",
            "sample_rate": f"SR_{args.get('sample_rate', '48000')}",
            "bit_depth": f"Bit{args.get('bit_depth', '24')}",
            "input_output_settings": f"IO_{args.get('input_output_settings', 'StereoMix')}",
            "is_interleaved": args.get("is_interleaved", False),
            "is_cloud_project": False,
            "create_from_template": args.get("create_from_template", False),
            "template_group": args.get("template_group", ""),
            "template_name": args.get("template_name", ""),
        }

    # ── Open Session ──
    elif tool_name == "open_session":
        body = {"session_path": args.get("session_path", "")}

    # ── Save Session As ──
    elif tool_name == "save_session_as":
        body = {
            "session_name": args.get("session_name", ""),
            "session_location": args.get("session_location", ""),
        }

    # ── Get Track List ──
    elif tool_name == "get_track_list":
        filters = []
        if args.get("track_filter"):
            filters = [{"filter": args["track_filter"], "is_inverted": False}]
        body = {
            "track_filter_list": filters if filters else [{"filter": "All", "is_inverted": False}],
            "is_filter_list_additive": True,
            "pagination_request": {
                "limit": args.get("limit", 0),
                "offset": args.get("offset", 0),
            },
        }

    # ── Create New Tracks ──
    elif tool_name == "create_new_tracks":
        body = {
            "track_count": args.get("track_count", 1),
            "track_type": f"TT_{args.get('track_type', 'Audio')}",
            "track_format": f"TF_{args.get('track_format', 'Mono')}" if args.get("track_format") else "TF_Mono",
            "track_name": args.get("track_name", ""),
        }

    # ── Track State Commands ──
    elif tool_name in ("set_track_mute_state", "set_track_solo_state",
                        "set_track_record_enable_state", "set_track_input_monitor_state",
                        "set_track_hidden_state", "set_track_inactive_state",
                        "set_track_frozen_state", "set_track_online_state",
                        "set_track_open_state"):
        body = {
            "track_names": args.get("track_names", []),
            "enabled": args.get("enabled", True),
        }

    # ── Select Tracks By Name ──
    elif tool_name == "select_tracks_by_name":
        body = {
            "track_names": args.get("track_names", []),
            "select": args.get("select", True),
        }

    # "rename_track" body builder removed — use rename_target_track instead

    # ── Playback/Record Mode ──
    elif tool_name == "set_playback_mode":
        body = {"mode": f"PM_{args.get('mode', 'Normal')}"}
    elif tool_name == "set_record_mode":
        body = {"mode": f"RM_{args.get('mode', 'Normal')}"}

    # ── Import ──
    elif tool_name == "import":
        body = {
            "file_list": args.get("file_list", []),
            "destination": f"MD_{args.get('destination', 'NewTrack')}",
            "location": f"ML_{args.get('location', 'SessionStart')}",
        }

    # ── Import Video ──
    elif tool_name == "import_video":
        body = {"video_path": args.get("video_path", "")}

    # ── Export ──
    elif tool_name == "export_clips_as_files":
        body = {
            "output_directory": args.get("output_directory", ""),
            "file_type": f"FT_{args.get('file_type', 'WAVE')}",
        }
    elif tool_name == "export_mix":
        body = {
            "output_path": args.get("output_path", ""),
            "file_type": f"FT_{args.get('file_type', 'WAVE')}",
            "bit_depth": f"Bit{args.get('bit_depth', '24')}",
            "sample_rate": f"SR_{args.get('sample_rate', '48000')}",
        }
    elif tool_name == "export_session_info_as_text":
        body = {
            "output_path": args.get("output_path", ""),
            "include_file_list": args.get("include_file_list", True),
            "include_clip_list": args.get("include_clip_list", True),
            "include_markers": args.get("include_markers", True),
            "include_plugin_list": args.get("include_plugin_list", True),
            "include_track_edls": args.get("include_track_edls", True),
            "output_type": "ESI_File",
        }

    # ── Create Memory Location ──
    elif tool_name == "create_memory_location":
        body = {
            "name": args.get("name", ""),
            "start_time": args.get("start_time", "0"),
            "time_properties": f"TP_{args.get('time_properties', 'Marker')}",
        }
        if "end_time" in args:
            body["end_time"] = args["end_time"]

    # ── Select Memory Location ──
    elif tool_name == "select_memory_location":
        body = {"number": args.get("number", 1)}

    # ── Edit Mode/Tool ──
    elif tool_name == "set_edit_mode":
        body = {"mode": f"EM_{args.get('mode', 'Slip')}"}
    elif tool_name == "set_edit_tool":
        body = {"tool": f"ET_{args.get('tool', 'Smart')}"}

    # ── Timeline Selection ──
    elif tool_name == "set_timeline_selection":
        body = {
            "start_point": args.get("start_point", "0"),
            "end_point": args.get("end_point", "0"),
            "reference": args.get("reference", "Absolute"),
        }

    # ── Repeat Selection ──
    elif tool_name == "repeat_selection":
        body = {"num_repeats": args.get("num_repeats", 1)}

    # ── Create Fade ── (maps to real SDK command CreateFadesBasedOnPreset)
    elif tool_name == "create_fade":
        body = {"preset_name": args.get("preset_name", "")}

    # ── Zoom Preset ──
    elif tool_name == "recall_zoom_preset":
        body = {"preset_number": args.get("preset_number", 1)}

    # ── Default: pass arguments through as-is ──
    else:
        body = args if args else {}

    return body


# ─── Main ───────────────────────────────────────────────────────────────────

def main():
    """Run the MCP server over stdio transport."""
    import asyncio
    asyncio.run(_run())


async def _run():
    async with stdio_server() as (read_stream, write_stream):
        await app.run(read_stream, write_stream, app.create_initialization_options())


if __name__ == "__main__":
    main()
