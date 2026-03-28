"""Test UDP multicast connectivity to UE5 Python remote execution."""
import json
import socket
import struct
import time
import uuid

MCAST_GRP = "239.0.0.1"
MCAST_PORT = 6766
NODE_ID = str(uuid.uuid4())

def test_ping(timeout=5.0):
    """Try multiple approaches to ping UE remote execution."""
    msg = json.dumps({
        "version": 1,
        "magic": "ue_py",
        "type": "ping",
        "source": NODE_ID,
        "data": {}
    }).encode("utf-8")

    # Approach 1: Bind to multicast port with SO_REUSEADDR
    print(f"[1] Trying: bind to multicast port {MCAST_PORT}...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
        sock.bind(("0.0.0.0", MCAST_PORT))
        mreq = struct.pack("4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton("0.0.0.0"))
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        sock.settimeout(timeout)
        sock.sendto(msg, (MCAST_GRP, MCAST_PORT))
        start = time.time()
        while time.time() - start < timeout:
            try:
                data, addr = sock.recvfrom(4096)
                resp = json.loads(data.decode("utf-8"))
                if resp.get("source") != NODE_ID:
                    print(f"  Got response from {addr}: type={resp.get('type')}, magic={resp.get('magic')}")
                    if resp.get("type") == "pong":
                        print("  SUCCESS: UE editor responded to ping!")
                        sock.close()
                        return True
            except socket.timeout:
                break
            except Exception as e:
                print(f"  Recv error: {e}")
                continue
        print("  No response")
        sock.close()
    except Exception as e:
        print(f"  Error: {e}")

    # Approach 2: Separate send/recv sockets (like ue_remote_exec.py)
    print(f"\n[2] Trying: separate send/recv sockets...")
    try:
        send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        send_sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)

        recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        recv_sock.bind(("", MCAST_PORT))
        mreq = struct.pack("4sl", socket.inet_aton(MCAST_GRP), socket.INADDR_ANY)
        recv_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        recv_sock.settimeout(timeout)

        send_sock.sendto(msg, (MCAST_GRP, MCAST_PORT))
        start = time.time()
        while time.time() - start < timeout:
            try:
                data, addr = recv_sock.recvfrom(4096)
                resp = json.loads(data.decode("utf-8"))
                if resp.get("source") != NODE_ID:
                    print(f"  Got response from {addr}: type={resp.get('type')}")
                    if resp.get("type") == "pong":
                        print("  SUCCESS!")
                        send_sock.close()
                        recv_sock.close()
                        return True
            except socket.timeout:
                break
            except Exception as e:
                print(f"  Recv error: {e}")
        print("  No response")
        send_sock.close()
        recv_sock.close()
    except Exception as e:
        print(f"  Error: {e}")

    # Approach 3: Try 127.0.0.1 bind specifically
    print(f"\n[3] Trying: bind to 127.0.0.1:{MCAST_PORT}...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 0)
        sock.bind(("127.0.0.1", MCAST_PORT))
        mreq = struct.pack("4s4s", socket.inet_aton(MCAST_GRP), socket.inet_aton("127.0.0.1"))
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        sock.settimeout(timeout)
        sock.sendto(msg, (MCAST_GRP, MCAST_PORT))
        start = time.time()
        while time.time() - start < timeout:
            try:
                data, addr = sock.recvfrom(4096)
                resp = json.loads(data.decode("utf-8"))
                if resp.get("source") != NODE_ID:
                    print(f"  Got response from {addr}: type={resp.get('type')}")
                    if resp.get("type") == "pong":
                        print("  SUCCESS!")
                        sock.close()
                        return True
            except socket.timeout:
                break
            except Exception as e:
                print(f"  Recv error: {e}")
        print("  No response")
        sock.close()
    except Exception as e:
        print(f"  Error: {e}")

    print("\nAll approaches failed. Remote execution may not be enabled in UE.")
    return False

if __name__ == "__main__":
    test_ping(timeout=8.0)
