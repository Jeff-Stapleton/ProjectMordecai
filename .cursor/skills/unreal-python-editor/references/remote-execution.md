# Unreal Remote Execution Protocol

Source: `Engine/Plugins/Experimental/PythonScriptPlugin/Source/PythonScriptPlugin/Private/PythonScriptRemoteExecution.cpp`

## Prerequisites

Enable in Editor Preferences → Plugins → Python → Remote Execution:
- **Enable Remote Execution**: On
- **Multicast Group Endpoint**: `239.0.0.1:6766` (default)
- **Multicast Bind Address**: `0.0.0.0` (**not** 127.0.0.1!)
- **Multicast TTL**: `1` (**not** 0!)
- Restart the editor after changing these settings

## Protocol Overview

- **Magic**: `"ue_py"` (not "ue_python")
- **Payload field**: `"data"` (not "body")
- **Discovery**: UDP multicast (ping/pong)
- **Commands**: TCP (client starts a TCP server, UE connects to it)

### Message Format

```json
{
  "version": 1,
  "magic": "ue_py",
  "type": "<message-type>",
  "source": "<unique-node-id-guid>",
  "dest": "<optional-target-node-id>",
  "data": { ... }
}
```

## Message Flow

### 1. Discovery (UDP)

Client sends `ping` via UDP multicast → UE responds with `pong`:

```
→ { "type": "ping", "source": "<client-id>" }
← { "type": "pong", "source": "<ue-id>", "dest": "<client-id>",
    "data": { "user": "...", "machine": "...", "engine_version": "...",
              "engine_root": "...", "project_root": "...", "project_name": "..." } }
```

### 2. Open Connection (UDP → TCP)

Client starts a TCP server, then sends `open_connection` via UDP:

```
→ { "type": "open_connection", "source": "<client-id>", "dest": "<ue-id>",
    "data": { "command_ip": "127.0.0.1", "command_port": <port> } }
```

UE then connects to the client's TCP server.

### 3. Execute Command (TCP)

Client sends `command` over the TCP connection:

```
→ { "type": "command", "source": "<client-id>", "dest": "<ue-id>",
    "data": { "command": "print('hello')", "unattended": true,
              "exec_mode": "ExecuteStatement" } }
```

**exec_mode values:**
- `"ExecuteStatement"` — single Python statement
- `"ExecuteFile"` — multi-line script (like running a .py file)
- `"EvaluateStatement"` — evaluate expression and return result

### 4. Receive Result (TCP)

UE sends `command_result` back over TCP:

```
← { "type": "command_result", "source": "<ue-id>", "dest": "<client-id>",
    "data": { "success": true, "command": "print('hello')", "result": "None",
              "output": [{ "type": "Info", "output": "hello" }] } }
```

### 5. Close Connection (UDP)

```
→ { "type": "close_connection", "source": "<client-id>", "dest": "<ue-id>" }
```

## Helper Script

Use `Scripts/ue_remote_exec.py` (repo root):

```bash
# Single statement
python ue_remote_exec.py "print('hello')"

# Multi-line script from file (uses ExecuteFile mode)
python ue_remote_exec.py --file my_script.py

# With timeout
python ue_remote_exec.py --file my_script.py --timeout 60
```

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| Ping gets no pong | Check Remote Execution enabled + restart editor |
| Pong works but command times out | Ensure Multicast Bind is `0.0.0.0`, TTL is `1` |
| Firewall blocks | Allow UDP 6766 + TCP dynamic ports for UE |
| `netstat -ano \| findstr 6766` shows nothing | Plugin not loaded — enable Python Editor Script Plugin |
| Pong only on `127.0.0.1`, not LAN interfaces | Windows multi-NIC issue (Hyper-V/WSL). The `ue_remote_exec.py` script handles this by trying loopback + all interfaces. If using a custom script, join multicast on `127.0.0.1` explicitly. |

### Windows Multi-NIC Gotcha

On Windows with multiple network interfaces (e.g. physical LAN + Hyper-V/WSL virtual adapter), UE's Python remote execution may only respond on the **loopback** interface (`127.0.0.1`), not on the LAN multicast group. The `ue_remote_exec.py` script automatically tries multiple interfaces including loopback. If you write your own discovery code, bind the multicast group to `127.0.0.1`:

```python
mreq = struct.pack("4s4s", socket.inet_aton("239.0.0.1"), socket.inet_aton("127.0.0.1"))
recv_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
```
