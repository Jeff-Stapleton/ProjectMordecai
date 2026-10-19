#!/usr/bin/env python3
"""Check if Unreal Editor is running and responding to Python remote execution.

Exit codes:
  0 — Editor is running and responding
  1 — Editor is not responding (not running or remote exec disabled)
  2 — Editor process found but not responding to remote exec

Usage:
  python check_editor.py          # Just check
  python check_editor.py --wait   # Wait up to 120s for editor to be ready
  python check_editor.py --wait --timeout 180
"""

import argparse
import json
import socket
import struct
import subprocess
import sys
import time
import uuid

MCAST_GRP = "239.0.0.1"
MCAST_PORT = 6766
NODE_ID = str(uuid.uuid4())


def is_editor_process_running():
    """Check if UnrealEditor.exe is running."""
    try:
        result = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq UnrealEditor.exe", "/FO", "CSV", "/NH"],
            capture_output=True, text=True, timeout=5
        )
        return "UnrealEditor.exe" in result.stdout
    except Exception:
        return False


def _try_ping_on_interface(interface, timeout):
    """Try multicast ping on a specific network interface. Returns True if pong received."""
    try:
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
        recv_sock.settimeout(timeout)
    except OSError:
        return False

    msg = json.dumps({
        "version": 1,
        "magic": "ue_py",
        "type": "ping",
        "source": NODE_ID,
        "data": {}
    }).encode("utf-8")

    try:
        send_sock.sendto(msg, (MCAST_GRP, MCAST_PORT))
        start = time.time()
        while time.time() - start < timeout:
            try:
                data, addr = recv_sock.recvfrom(4096)
                resp = json.loads(data.decode("utf-8"))
                if resp.get("type") == "pong" and resp.get("magic") == "ue_py":
                    return True
            except socket.timeout:
                break
            except (json.JSONDecodeError, UnicodeDecodeError):
                continue
        return False
    except Exception:
        return False
    finally:
        send_sock.close()
        recv_sock.close()


def ping_remote_exec(timeout=3.0):
    """Ping the UE remote execution endpoint. Returns True if we get a pong.

    Tries the default interface first, then falls back to loopback and each
    local interface. On Windows with multiple NICs (Hyper-V, WSL, Docker),
    the default multicast interface may not match what UE is listening on.
    """
    # Try default interface first
    if _try_ping_on_interface("0.0.0.0", timeout):
        return True

    # Fallback: try loopback and each local interface
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
        if _try_ping_on_interface(iface, per_iface_timeout):
            return True

    return False


def main():
    parser = argparse.ArgumentParser(description="Check if UE Editor is running with remote exec")
    parser.add_argument("--wait", action="store_true", help="Wait for editor to be ready")
    parser.add_argument("--timeout", type=int, default=120, help="Max seconds to wait (default: 120)")
    args = parser.parse_args()

    if not args.wait:
        process_running = is_editor_process_running()
        remote_ready = ping_remote_exec()

        if remote_ready:
            print("EDITOR_READY")
            sys.exit(0)
        elif process_running:
            print("EDITOR_LOADING")
            sys.exit(2)
        else:
            print("EDITOR_NOT_RUNNING")
            sys.exit(1)
    else:
        print(f"Waiting up to {args.timeout}s for editor...", flush=True)
        start = time.time()
        while time.time() - start < args.timeout:
            if ping_remote_exec(timeout=3.0):
                elapsed = int(time.time() - start)
                print(f"EDITOR_READY (took {elapsed}s)")
                sys.exit(0)

            process_running = is_editor_process_running()
            elapsed = int(time.time() - start)
            if process_running:
                print(f"  [{elapsed}s] Editor process found, waiting for remote exec...", flush=True)
            else:
                print(f"  [{elapsed}s] Editor not running yet...", flush=True)
            time.sleep(5)

        print(f"TIMEOUT after {args.timeout}s")
        sys.exit(1)


if __name__ == "__main__":
    main()
