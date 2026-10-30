#!/usr/bin/env python3
"""Send Python commands to a running Unreal Editor via its remote execution protocol.

Protocol (from UE5 source):
  1. UDP multicast ping → pong (discovery)
  2. Client opens TCP server, sends open_connection via UDP with TCP ip:port
  3. UE connects to TCP server
  4. Client sends command, UE sends command_result over TCP
  5. Client sends close_connection via UDP

Discovery: the receive socket joins the multicast group on every local interface,
and the ping goes out of each interface in turn (default first, then loopback and
each local address). On Windows with several NICs (Hyper-V, WSL, Docker) the
editor may answer only a ping sent out of loopback while its pong arrives on
another adapter. A late pong cannot be tied to the interface that carried its
ping, so open_connection and close_connection go out of every interface that
was pinged: the editor ignores an open_connection for a node and endpoint it is
already connected to, and a close_connection after the first.

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


def local_interfaces():
    """Loopback first, then every local IPv4 address."""
    interfaces = ["127.0.0.1"]
    try:
        for info in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET):
            ip = info[4][0]
            if ip not in interfaces:
                interfaces.append(ip)
    except OSError:
        pass
    return interfaces


def make_recv_socket():
    """A UDP socket on the multicast port that has joined the group on every interface."""
    recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    recv_sock.bind(("", MCAST_PORT))
    for iface in ["0.0.0.0"] + local_interfaces():
        mreq = struct.pack("4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton(iface))
        try:
            recv_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        except OSError:
            pass  # already joined through another address of the same adapter, or no multicast
    return recv_sock


def make_send_socket(interface="0.0.0.0"):
    """A UDP socket that sends multicast out of one interface (0.0.0.0 = the default)."""
    send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    send_sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
    if interface != "0.0.0.0":
        send_sock.setsockopt(
            socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(interface)
        )
    return send_sock


def broadcast(send_socks, payload):
    """Send one multicast message out of every socket; True if any send worked."""
    sent = False
    for send_sock in send_socks:
        try:
            send_sock.sendto(payload, (MCAST_GRP, MCAST_PORT))
            sent = True
        except OSError:
            pass
    return sent


def discover(recv_sock, timeout=5.0):
    """Ping out of each interface in turn until UE answers.

    Returns (ue_node_id, send_socks): one send socket per interface that was pinged, for
    broadcast() of open_connection/close_connection; the caller closes them. On timeout
    returns (None, []) and closes them itself.
    """
    send_socks = {}
    deadline = time.time() + timeout
    while time.time() < deadline:
        for iface in ["0.0.0.0"] + local_interfaces():
            try:
                if iface not in send_socks:
                    send_socks[iface] = make_send_socket(iface)
                send_socks[iface].sendto(make_msg("ping"), (MCAST_GRP, MCAST_PORT))
            except OSError:
                continue
            recv_sock.settimeout(0.5)
            wait_until = time.time() + 0.6
            while time.time() < wait_until:
                try:
                    data, _addr = recv_sock.recvfrom(65535)
                    msg = json.loads(data.decode("utf-8"))
                    if msg.get("type") == "pong" and msg.get("dest") == NODE_ID:
                        return msg["source"], list(send_socks.values())
                except socket.timeout:
                    break
                except (json.JSONDecodeError, UnicodeDecodeError):
                    continue
    for send_sock in send_socks.values():
        send_sock.close()
    return None, []


def execute(command: str, timeout: float = 30.0, exec_mode: str = "ExecuteStatement") -> dict:
    """Execute a Python command in UE and return the result."""
    recv_sock = make_recv_socket()
    send_socks = []

    try:
        # 1. Discover UE
        ue_node_id, send_socks = discover(recv_sock, timeout=min(timeout, 15.0))
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

        # 3. Send open_connection via UDP (out of every pinged interface)
        open_data = {"command_ip": command_ip, "command_port": tcp_port}
        broadcast(send_socks, make_msg("open_connection", dest=ue_node_id, data=open_data))

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
                        broadcast(send_socks, make_msg("close_connection", dest=ue_node_id))
                        return result
                except json.JSONDecodeError:
                    continue  # need more data
            except socket.timeout:
                break

        conn.close()
        tcp_server.close()
        broadcast(send_socks, make_msg("close_connection", dest=ue_node_id))

        if chunks:
            full = b"".join(chunks).decode("utf-8", errors="replace")
            return {"error": "incomplete_response", "raw": full}
        return {"error": "timeout", "message": f"No response within {timeout}s"}

    finally:
        for send_sock in send_socks:
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
