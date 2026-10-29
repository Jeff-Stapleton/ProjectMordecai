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
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ue_remote_exec  # noqa: E402  (shares its multi-interface discovery)


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


def ping_remote_exec(timeout=3.0):
    """Ping the UE remote execution endpoint. Returns True if we get a pong.

    Uses ue_remote_exec's discovery: the ping goes out of the default interface,
    loopback and each local interface in turn, and the pong is heard on any of
    them. On Windows with multiple NICs (Hyper-V, WSL, Docker) the editor may
    answer only a loopback ping, with the pong arriving on another adapter.
    """
    try:
        recv_sock = ue_remote_exec.make_recv_socket()
    except OSError:
        return False
    try:
        node_id, send_socks = ue_remote_exec.discover(recv_sock, timeout=timeout)
        for send_sock in send_socks:
            send_sock.close()
        return node_id is not None
    finally:
        recv_sock.close()


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
