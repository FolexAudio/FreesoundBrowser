# PTSL MCP Server - How It Actually Works

## The Big Question: Is It Always Running?

**No.** The MCP server is NOT a traditional server that you open and leave running.

Here's the actual flow:

```
┌─────────────────────────────────────────────────────────┐
│                    WHAT HAPPENS                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. You open Claude Desktop (or ChatGPT, etc)           │
│                                                         │
│  2. Claude starts up, reads your config file,           │
│     and AUTOMATICALLY launches ptsl_mcp_server.py       │
│     as a background child process                       │
│                                                         │
│  3. Claude and the MCP server talk through pipes        │
│     (stdin/stdout) - invisible to you                   │
│                                                         │
│  4. You chat with Claude normally. When you ask         │
│     something Pro Tools related, Claude silently        │
│     calls the MCP tool behind the scenes.               │
│                                                         │
│  5. You close Claude → MCP server shuts down too        │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

**You never manually start the server.** Claude manages it.

## Step-by-Step Setup for Claude Desktop

### Step 1: Install Python dependencies (one time)

Double-click `setup.bat` in this folder.

Or manually:
```
cd C:\Users\philm\Documents\ptsl-mcp-server
python -m venv venv
venv\Scripts\activate
pip install grpcio grpcio-tools protobuf mcp
```

### Step 2: Configure Claude Desktop

Find your Claude Desktop config file:
- **Windows**: `%APPDATA%\Claude\claude_desktop_config.json`
- Full path: `C:\Users\philm\AppData\Roaming\Claude\claude_desktop_config.json`

Open it in Notepad and add the `mcp_servers` section:

```json
{
  "mcpServers": {
    "protos": {
      "command": "C:\\Users\\philm\\Documents\\ptsl-mcp-server\\venv\\Scripts\\python.exe",
      "args": ["C:\\Users\\philm\\Documents\\ptsl-mcp-server\\ptsl_mcp_server.py"]
    }
  }
}
```

> **Note**: Use double backslashes `\\` in JSON paths on Windows.

If the file already has other entries, just add the `"protos"` block inside `"mcpServers"`.

### Step 3: Restart Claude Desktop

Close Claude completely (right-click system tray → Quit), then reopen it.

Claude will now automatically launch the MCP server in the background.

### Step 4: Test it

In Claude, try asking:

> "Connect to Pro Tools and tell me the session name"

Claude should call the `register_connection` tool, then `get_session_name`.

**Pro Tools must be running first.** The MCP server connects to it on localhost:31416.

## What You'll See in Claude

When MCP tools are available, you'll see a small **🔨 Tools** indicator or a hammer icon in Claude's interface. Clicking it shows the 66 available Pro Tools tools.

When Claude uses a tool, you'll see it expand a "Used [tool_name]" block showing what it called and what Pro Tools returned.

## The Communication Chain

```
You type: "What tracks are in this session?"
    │
    ▼
Claude Desktop (the AI app you see)
    │
    │  JSON-RPC over stdin/stdout pipes
    │  (completely invisible)
    ▼
ptsl_mcp_server.py (background process)
    │
    │  gRPC on localhost:31416
    │  (Protobuf binary protocol)
    ▼
Pro Tools (already running)
    │
    │  Returns track list as JSON
    │
    ▼  (back up the chain to you)
```

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Claude doesn't show tools | Config file wrong or Claude wasn't restarted | Check JSON syntax, restart Claude fully |
| "Failed to connect to Pro Tools" | Pro Tools isn't running | Open Pro Tools first |
| "Not connected" error | register_connection wasn't called | Ask Claude to "connect to Pro Tools" |
| Server won't start | Python/venv path wrong in config | Double-check the full path in config JSON |
| Import errors | Dependencies not installed | Run setup.bat again |

## Quick Test Without Claude

You can test the server manually from a terminal:

```cmd
cd C:\Users\philm\Documents\ptsl-mcp-server
venv\Scripts\python ptsl_mcp_server.py
```

It will appear to "hang" — that's correct. It's waiting for JSON-RPC messages on stdin.
Press Ctrl+C to stop.

To actually test it, you'd need an MCP client sending it messages. Claude Desktop is that client.

## For Hermes Agent (instead of Claude Desktop)

If you're using Hermes Agent instead, add to `~/.hermes/config.yaml`:

```yaml
mcp_servers:
  protos:
    command: "C:\\Users\\philm\\Documents\\ptsl-mcp-server\\venv\\Scripts\\python.exe"
    args: ["C:\\Users\\philm\\Documents\\ptsl-mcp-server\\ptsl_mcp_server.py"]
```

Same principle — Hermes launches and manages the server automatically.
