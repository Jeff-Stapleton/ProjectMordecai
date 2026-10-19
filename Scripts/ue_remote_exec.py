#!/usr/bin/env python3
"""Send Python commands to a running Unreal Editor via its remote execution protocol.

Protocol (from UE5 source):
  1. UDP multicast ping → pong (discovery)
  2. Client opens TCP server, sends open_connection via UDP with TCP ip:port
  3. UE connects to TCP server
  4. Client sends command, UE sends command_result over TCP
  5. Client sends close_connection via UDP

Usage:
    python ue_remote_exec.py "print('hello')"
    python ue_remote_exec.py --file script.py
    python ue_remote_exec.py --file script.py --timeout 30
"""

import argparse
import json
import socket
import struct
import sys
import time
import uuid

MCAST_GRP = "239.0.0.1"
MCAST_PORT = 6766
NODE_ID = str(uuid.uuid4())


def make_msg(msg_type, dest="", data=None):
    msg = {
        "version": 1,
        "magic": "ue_py",
        "type": msg_type,
        "source": NODE_ID,
    }
    if dest:
        msg["dest"] = dest
    if data is not None:
        msg["data"] = data
    return json.dumps(msg).encode("utf-8")


def make_udp_sockets(interface="0.0.0.0"):
    """Create send and receive UDP sockets for multicast on a specific interface."""
    send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    send_sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
    if interface != "0.0.0.0":
        send_sock.setsockopt(
            socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(interface)
        )

    recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    recv_sock.bind(("", MCAST_PORT))
    mreq = struct.pack("4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton(interface))
    recv_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    return send_sock, recv_sock


def _try_discover_on(interface, timeout):
    """Try discovery on a single interface. Returns (ue_node_id, ue_info, addr) or Nones."""
    try:
        send_sock, recv_sock = make_udp_sockets(interface)
    except OSError:
        return None, None, None
    try:
        recv_sock.settimeout(timeout)
        send_sock.sendto(make_msg("ping"), (MCAST_GRP, MCAST_PORT))
        start = time.time()
        while time.time() - start < timeout:
            try:
                data, addr = recv_sock.recvfrom(65535)
                msg = json.loads(data.decode("utf-8"))
                if msg.get("source") != NODE_ID and msg.get("type") == "pong":
                    return msg["source"], msg.get("data", {}), addr
            except socket.timeout:
                break
        return None, None, None
    finally:
        send_sock.close()
        recv_sock.close()


def discover(send_sock, recv_sock, timeout=5.0):
    """Ping and wait for pong from UE, trying multiple interfaces on Windows."""
    # First try the sockets we were given (INADDR_ANY / default)
    recv_sock.settimeout(timeout)
    send_sock.sendto(make_msg("ping"), (MCAST_GRP, MCAST_PORT))

    start = time.time()
    while time.time() - start < timeout:
        try:
            data, addr = recv_sock.recvfrom(65535)
            msg = json.loads(data.decode("utf-8"))
            if msg.get("source") != NODE_ID and msg.get("type") == "pong":
                return msg["source"], msg.get("data", {}), addr
        except socket.timeout:
            break

    # Default didn't work — close the original sockets first so fallback
    # sockets can bind to the same port, then try loopback and each local
    # interface explicitly. On Windows with multiple NICs (e.g. Hyper-V/WSL),
    # the default multicast interface may not match what UE is listening on.
    send_sock.close()
    recv_sock.close()
    interfaces = ["127.0.0.1"]
    try:
        for info in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET):
            ip = info[4][0]
            if ip not in interfaces:
                interfaces.append(ip)
    except OSError:
        pass

    per_iface_timeout = max(1.0, timeout / len(interfaces)) if interfaces else timeout
    for iface in interfaces:
        uid, uinfo, uaddr = _try_discover_on(iface, per_iface_timeout)
        if uid:
            return uid, uinfo, uaddr

    return None, None, None


def execute(command: str, timeout: float = 30.0, exec_mode: str = "ExecuteStatement") -> dict:
    """Execute a Python command in UE and return the result."""
    send_sock, recv_sock = make_udp_sockets()

    try:
        # 1. Discover UE
        ue_node_id, ue_info, ue_addr = discover(send_sock, recv_sock, timeout=5.0)
        if not ue_node_id:
            return {"error": "No UE editor found via multicast ping"}

        # 2. Start TCP server for UE to connect to
        tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        tcp_server.bind(("0.0.0.0", 0))  # random port
        tcp_server.listen(1)
        tcp_server.settimeout(timeout)
        tcp_ip, tcp_port = tcp_server.getsockname()

        # Use 127.0.0.1 since we're on the same machine
        command_ip = "127.0.0.1"

        # 3. Send open_connection via UDP
        open_data = {"command_ip": command_ip, "command_port": tcp_port}
        send_sock.sendto(
            make_msg("open_connection", dest=ue_node_id, data=open_data),
            (MCAST_GRP, MCAST_PORT),
        )

        # 4. Wait for UE to connect
        try:
            conn, addr = tcp_server.accept()
        except socket.timeout:
            tcp_server.close()
            return {"error": "UE did not connect to our TCP server within timeout"}

        conn.settimeout(timeout)

        # 5. Send command over TCP
        cmd_data = {
            "command": command,
            "unattended": True,
            "exec_mode": exec_mode,
        }
        cmd_msg = make_msg("command", dest=ue_node_id, data=cmd_data)
        conn.sendall(cmd_msg)

        # 6. Receive result over TCP
        chunks = []
        start = time.time()
        while time.time() - start < timeout:
            try:
                chunk = conn.recv(65535)
                if not chunk:
                    break
                chunks.append(chunk)
                # Try to parse — if valid JSON, we're done
                full = b"".join(chunks)
                try:
                    result = json.loads(full.decode("utf-8"))
                    if result.get("type") == "command_result":
                        conn.close()
                        tcp_server.close()
                        # Send close_connection
                        send_sock.sendto(
                            make_msg("close_connection", dest=ue_node_id),
                            (MCAST_GRP, MCAST_PORT),
                        )
                        return result
                except json.JSONDecodeError:
                    continue  # need more data
            except socket.timeout:
                break

        conn.close()
        tcp_server.close()
        send_sock.sendto(
            make_msg("close_connection", dest=ue_node_id),
            (MCAST_GRP, MCAST_PORT),
        )

        if chunks:
            full = b"".join(chunks).decode("utf-8", errors="replace")
            return {"error": "incomplete_response", "raw": full}
        return {"error": "timeout", "message": f"No response within {timeout}s"}

    finally:
        send_sock.close()
        recv_sock.close()


def main():
    parser = argparse.ArgumentParser(description="Execute Python in Unreal Editor remotely")
    parser.add_argument("command", nargs="?", help="Python command to execute")
    parser.add_argument("--file", "-f", help="Python file to execute")
    parser.add_argument("--timeout", "-t", type=float, default=30.0)
    args = parser.parse_args()

    if args.file:
        with open(args.file, "r") as f:
            command = f.read()
        exec_mode = "ExecuteFile"
    elif args.command:
        command = args.command
        exec_mode = "ExecuteStatement"
    else:
        parser.error("Provide a command or --file")
        return

    result = execute(command, timeout=args.timeout, exec_mode=exec_mode)

    if result.get("type") == "command_result":
        data = result.get("data", {})
        if data.get("success"):
            if data.get("result"):
                print(data["result"])
            for line in data.get("output", []):
                prefix = {"Warning": "WARN", "Error": "ERROR"}.get(line.get("type"), "INFO")
                print(f"[{prefix}] {line.get('output', '')}")
            sys.exit(0)
        else:
            print(f"ERROR: {data.get('result', 'Unknown error')}", file=sys.stderr)
            for line in data.get("output", []):
                print(f"  {line.get('output', '')}", file=sys.stderr)
            sys.exit(1)
    else:
        print(json.dumps(result, indent=2), file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
