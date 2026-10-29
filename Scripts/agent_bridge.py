#!/usr/bin/env python3
"""Agent Bridge Client -- Phase 2 (real input into the PACKAGED game)

Drives the packaged game through UMordecaiAgentBridgeSubsystem, the in-game bridge a
non-Shipping build starts with -AgentBridge.  The contract is Specs/agent_bridge_v1.md.  Every
input command injects OS-level events into Slate, so a key or click takes the same path as a
player's (preprocessors, focus, viewport, CommonUI, Enhanced Input).  This client never asks the
game to call gameplay or UI functions; bridge.jsonl in the run dir is the audit trail of what
was sent.

Library use (standard library only; Pillow is optional):
  import agent_bridge
  bridge = agent_bridge.Bridge.launch()          # STALE gate, launch, wait until ready
  bridge.walk_to(400, 0)                         # closed-loop WASD walk
  bridge.tap("F")
  bridge.wait_until("inventory.count>=1", timeout_s=5)
  path = bridge.screenshot()
  bridge.quit()
  # or: bridge = agent_bridge.Bridge.attach(port, token)

CLI (prints JSON on stdout, logs on stderr):
  py Scripts/agent_bridge.py launch [--map M | --frontend] [--allow-stale]
        (default map DevTestMap; the test maps /MordecaiCore/Game/Maps/MovementTestMap and
        /MordecaiCore/Game/Maps/CombatTestMap load the same arena experience)
  py Scripts/agent_bridge.py ping | state [--sections a,b] [--radius R]
  py Scripts/agent_bridge.py press F [--hold 0.1]     (a tap: the bridge times the hold, then releases)
  py Scripts/agent_bridge.py axis Gamepad_LeftY=1 --hold 0.5
  py Scripts/agent_bridge.py mouse 640 360 | click [X Y] | click-button "Identify"
  py Scripts/agent_bridge.py walk-to 400 0 [--tol 50]
  py Scripts/agent_bridge.py screenshot [PATH] | wait 1.5 | wait-until "inventory.count>=4" [--timeout 10]
  py Scripts/agent_bridge.py quit
  Options for every command: --session FILE (default Saved/Playtests/agent_bridge_session.json),
  --port N [--token T] to talk to a bridge without a session file.

  `launch` starts the game DETACHED (no kill-on-close job) and writes the session file
  {pid, pid_created, port, token, run_dir, exe, commit, started_at, ...}; later calls read it
  and `quit` deletes it.  pid_created is the process creation time (Windows FILETIME ticks): a
  pid counts as the session's game only while its creation time (or, for an older session file
  without it, its exe name) matches, so a reused pid is never killed.  A session whose pid is
  gone or reused is stale: the file is removed and the call fails (2).
  The game outlives the CLI, so always finish with `quit` (a running game also blocks
  package_playtest.py).

Output: <run_dir> = Saved/Playtests/<YYYYmmdd-HHMMSS>-bridge/ with game.log, bridge.jsonl
(every request and response), bridge_NN.png (screenshots).

Exit codes:
  0 -- ok
  1 -- gameplay failure (bridge timeout or bad_key; walk_to stuck, timed out, mapping mismatch,
       dead, menu open or W/A/S/D already held; wrong experience; a button that is not on
       screen)
  2 -- infrastructure problem (STALE BUILD, launch failure, crash, no connection, no or stale
       session, any other bridge error)
"""

import argparse
import collections
import datetime
import itertools
import json
import math
import os
import socket
import subprocess
import sys
import threading
import time
import uuid

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import playtest_boot as pb  # noqa: E402

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 47777
BRIDGE_VERSION = 1
DEFAULT_MAP = pb.DEFAULT_MAP
DEFAULT_EXPERIENCE = pb.DEFAULT_EXPERIENCE
FRONTEND_EXPERIENCE = pb.FRONTEND_EXPERIENCE
SESSION_FILE = os.path.join(pb.PLAYTESTS_DIR, "agent_bridge_session.json")
TRANSCRIPT_NAME = "bridge.jsonl"

GAMEPLAY_ERROR_CODES = frozenset({"timeout", "bad_key"})  # section 6: the rest are infra
RESPONSE_SLACK_S = 15.0       # socket timeout = expected duration of the command + this
CONNECT_TIMEOUT_S = 5.0
READY_TIMEOUT_S = 60.0        # after the experience loads: loading screen + pawn
READY_PING_TIMEOUT_S = 5.0    # one ready-loop ping; a timeout re-pings on the same connection
READY_RETRY_S = 0.25          # pause before a ready-loop retry (reconnect, not_ready)
SCREENSHOT_COMPLETE_S = 5.0   # a screenshot reply's file must be complete within this
QUIT_TIMEOUT_S = 30.0

WALK_SPEED = 600.0            # AMordecaiCharacter MaxWalkSpeed, sprint off (LeftShift toggles x1.5)
WALK_SECTIONS = ("player", "pause_menu")
WALK_KEYS = ("W", "A", "S", "D")
YAW_TOLERANCE_DEG = 1.0
STUCK_PROGRESS_CM = 10.0
STUCK_PULSES = 3
MAX_SIDE_STEPS = 3
SIDE_STEP_S = 0.25

EXIT_OK, EXIT_GAMEPLAY, EXIT_INFRA = 0, 1, 2

# Codes this client raises itself (BridgeError.code), next to the bridge's own error codes.
#   connect_failed, disconnected, no_response, closed, bad_response, stale_build, launch_failed,
#   crash, exited, boot_timeout, no_experience, wrong_experience, bind_failed, token_mismatch,
#   not_ready, screenshot_incomplete, no_player, player_dead, menu_open, yaw_not_zero,
#   mapping_mismatch, stuck, walk_timeout, keys_held, wait_for_timeout, no_button, no_session,
#   stale_session, session_running, kill_failed


# ---------------------------------------------------------------------------
# Logging (stderr: stdout is the CLI's JSON and an MCP server's protocol channel)
# ---------------------------------------------------------------------------
_LOG_STREAM = None


def set_log_stream(stream):
    """Send this module's log lines to `stream` (None = sys.stderr). Returns the previous one."""
    global _LOG_STREAM
    previous = _LOG_STREAM
    _LOG_STREAM = stream
    return previous


def _log_out():
    return _LOG_STREAM if _LOG_STREAM is not None else sys.stderr


def log(msg):
    print(f"[Agent-Bridge] {msg}", file=_log_out(), flush=True)


# ---------------------------------------------------------------------------
# Errors
# ---------------------------------------------------------------------------
class BridgeError(Exception):
    """A failed bridge call.

    .kind      "gameplay" (the game did not do what a player expects: a bridge `timeout` or
               `bad_key`, walk_to stuck, ...) or "infra" (everything else)
    .response  the raw bridge response (or other context dict), if any
    .code      the bridge error code ("timeout", "bad_request", ...) or this client's own code
    """

    def __init__(self, message, kind="infra", response=None, code=None):
        super().__init__(message)
        self.kind = kind if kind in ("gameplay", "infra") else "infra"
        self.response = response
        if code is None and isinstance(response, dict):
            code = response.get("error")
        self.code = code

    def to_dict(self):
        return {"ok": False, "kind": self.kind, "error": self.code, "message": str(self),
                "response": self.response}


def _gameplay(message, code, response=None):
    return BridgeError(message, kind="gameplay", response=response, code=code)


def _infra(message, code, response=None):
    return BridgeError(message, kind="infra", response=response, code=code)


# ---------------------------------------------------------------------------
# Transcript (<run_dir>/bridge.jsonl)
# ---------------------------------------------------------------------------
class Transcript:
    """Append-only JSONL log of every request and response.  Safe across threads and processes
    (one open/append/close per entry)."""

    _locks = {}
    _guard = threading.Lock()

    def __init__(self, path):
        self.path = os.path.abspath(path)
        with Transcript._guard:
            self._lock = Transcript._locks.setdefault(os.path.normcase(self.path),
                                                      threading.Lock())

    @classmethod
    def for_run_dir(cls, run_dir):
        return cls(os.path.join(run_dir, TRANSCRIPT_NAME)) if run_dir else None

    def write(self, direction, conn, payload):
        entry = {"t": datetime.datetime.now().isoformat(timespec="milliseconds"),
                 "pid": os.getpid(), "conn": conn, "dir": direction}
        if isinstance(payload, (dict, list)):
            entry["msg"] = payload
        else:
            entry["line"] = str(payload)[:2000]
        line = json.dumps(entry, ensure_ascii=False, separators=(",", ":"), default=str)
        with self._lock:
            try:
                with open(self.path, "a", encoding="utf-8", newline="\n") as f:
                    f.write(line + "\n")
            except OSError as e:
                log(f"WARNING: cannot write {self.path}: {e}")


# ---------------------------------------------------------------------------
# One connection
# ---------------------------------------------------------------------------
def expected_duration(cmd, args):
    """How long the bridge needs before it replies to this command, in seconds."""
    def num(name, default=0.0):
        try:
            v = args.get(name)
            return default if v is None else max(0.0, float(v))
        except (TypeError, ValueError):
            return default

    if cmd == "key":
        return num("hold_s", 0.1) if args.get("action", "tap") == "tap" else 0.0
    if cmd in ("axis", "click"):
        return num("hold_s")
    if cmd == "screenshot":
        return 10.0
    if cmd == "wait":
        return num("seconds")
    if cmd == "wait_until":
        return num("timeout_s", 10.0)
    return 0.0


class BridgeConnection:
    """One NDJSON connection to the bridge on 127.0.0.1 (TCP_NODELAY).

    request() is serialized by a lock: it assigns the next integer id, sends one line and reads
    lines until the response with that id arrives.  Blank and malformed lines and replies to
    earlier (timed-out) requests are skipped and recorded.  ok:false raises BridgeError
    (timeout and bad_key are gameplay errors, the rest infra).
    """

    _serials = itertools.count(1)

    def __init__(self, port, host=DEFAULT_HOST, run_dir=None, connect_timeout=CONNECT_TIMEOUT_S,
                 transcript=None):
        self.host = host
        self.port = int(port)
        self.transcript = transcript or Transcript.for_run_dir(run_dir)
        self.serial = next(BridgeConnection._serials)
        self._lock = threading.Lock()
        self._next_id = 1
        self._buf = b""
        self._closed = False
        try:
            self.sock = socket.create_connection((host, self.port), timeout=connect_timeout)
        except OSError as e:
            raise _infra(f"cannot connect to the agent bridge at {host}:{self.port}: {e}",
                         "connect_failed") from e
        try:
            self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        except OSError:
            pass
        self._record("open", {"host": host, "port": self.port})

    @property
    def closed(self):
        return self._closed

    def _record(self, direction, payload):
        if self.transcript is not None:
            self.transcript.write(direction, self.serial, payload)

    def close(self):
        if self._closed:
            return
        self._closed = True
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            self.sock.close()
        except OSError:
            pass
        self._record("close", {})

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def request(self, cmd, timeout=None, **args):
        """Send {"id", "cmd", **args}; return the response dict (ok:true) or raise BridgeError.

        timeout: seconds to wait for the reply; default expected_duration() + 15 s.
        """
        if not isinstance(cmd, str) or not cmd:
            raise ValueError("cmd must be a non-empty string")
        with self._lock:
            if self._closed:
                raise _infra(f"{cmd}: the connection is closed", "closed")
            rid = self._next_id
            self._next_id += 1
            msg = {"id": rid, "cmd": cmd}
            msg.update(args)
            wait_s = float(timeout) if timeout is not None else (
                expected_duration(cmd, args) + RESPONSE_SLACK_S)
            data = (json.dumps(msg, ensure_ascii=False, separators=(",", ":")) + "\n").encode(
                "utf-8")
            self._record("send", msg)
            deadline = time.monotonic() + wait_s
            try:
                self.sock.settimeout(max(0.05, wait_s))
                self.sock.sendall(data)
            except OSError as e:
                self._drop()
                raise _infra(f"{cmd}: send failed: {e}", "disconnected") from e
            while True:
                resp = self._read_message(deadline, cmd, wait_s)
                got = resp.get("id")
                if got == rid and not isinstance(got, bool):
                    break
                if got is None and resp.get("ok") is False:
                    # The bridge could not read our id (oversize or malformed line) or refused
                    # the connection; only one request is in flight, so this answers it.
                    break
                self._record("skip", resp)
            self._record("recv", resp)
        ok = resp.get("ok")
        if ok is True:
            return resp
        if ok is not False:
            raise _infra(f"{cmd}: malformed response (no ok field)", "bad_response", resp)
        code = resp.get("error") or "failed"
        kind = "gameplay" if code in GAMEPLAY_ERROR_CODES else "infra"
        message = resp.get("message") or ""
        raise BridgeError(f"{cmd}: {code}" + (f": {message}" if message else ""), kind=kind,
                          response=resp, code=code)

    def _drop(self):
        self._closed = True
        try:
            self.sock.close()
        except OSError:
            pass
        self._record("close", {"reason": "connection lost"})

    def _read_message(self, deadline, cmd, wait_s):
        while True:
            nl = self._buf.find(b"\n")
            if nl >= 0:
                raw, self._buf = self._buf[:nl], self._buf[nl + 1:]
                text = raw.decode("utf-8", errors="replace").strip()
                if not text:
                    continue
                try:
                    obj = json.loads(text)
                except ValueError:
                    self._record("skip", text)
                    log(f"WARNING: skipped a malformed line from the bridge: {text[:120]!r}")
                    continue
                if not isinstance(obj, dict):
                    self._record("skip", text)
                    continue
                return obj
            if len(self._buf) > 64 * 1024 * 1024:
                self._drop()
                raise _infra(f"{cmd}: response line exceeds 64 MiB", "bad_response")
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise _infra(f"{cmd}: no response from the bridge within {wait_s:.1f}s",
                             "no_response")
            try:
                self.sock.settimeout(remaining)
                chunk = self.sock.recv(1 << 16)
            except socket.timeout:
                continue
            except OSError as e:
                self._drop()
                raise _infra(f"{cmd}: connection lost: {e}", "disconnected") from e
            if not chunk:
                self._drop()
                raise _infra(f"{cmd}: the bridge closed the connection", "disconnected")
            self._buf += chunk


# ---------------------------------------------------------------------------
# Process helpers
# ---------------------------------------------------------------------------
def free_port(host=DEFAULT_HOST):
    """A TCP port nothing listens on right now (bind port 0, read it, close)."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, 0))
        return s.getsockname()[1]


PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
STILL_ACTIVE = 259
ERROR_ACCESS_DENIED = 5


def _kernel32():
    import ctypes
    from ctypes import wintypes
    k = ctypes.WinDLL("kernel32", use_last_error=True)
    k.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    k.OpenProcess.restype = wintypes.HANDLE
    k.GetExitCodeProcess.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
    k.GetExitCodeProcess.restype = wintypes.BOOL
    k.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPWSTR,
                                             ctypes.POINTER(wintypes.DWORD)]
    k.QueryFullProcessImageNameW.restype = wintypes.BOOL
    k.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    k.GetProcessTimes.restype = wintypes.BOOL
    k.CloseHandle.argtypes = [wintypes.HANDLE]
    k.CloseHandle.restype = wintypes.BOOL
    return ctypes, wintypes, k


def query_process(pid):
    """What Windows reports about pid, from one PROCESS_QUERY_LIMITED_INFORMATION handle:
    {"pid", "error", "alive", "image", "created"}.

    error: None (not Windows / no pid), 0 (opened), else the OpenProcess error: 5 =
    ERROR_ACCESS_DENIED (a process this user cannot open holds the pid; a game this user
    launched is always openable, so it is never ours), 87 = no such process.  image: the full
    exe path; created: the creation time as a decimal string of FILETIME ticks (100 ns since
    1601); either is None when it cannot be read.
    """
    info = {"pid": pid, "error": None, "alive": False, "image": None, "created": None}
    if sys.platform != "win32" or not pid:
        return info
    ctypes, wintypes, k = _kernel32()
    h = k.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, int(pid))
    if not h:
        info["error"] = ctypes.get_last_error() or -1
        return info
    info["error"] = 0
    try:
        code = wintypes.DWORD()
        info["alive"] = bool(k.GetExitCodeProcess(h, ctypes.byref(code))) and \
            code.value == STILL_ACTIVE
        size = wintypes.DWORD(32768)
        buf = ctypes.create_unicode_buffer(32768)
        if k.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(size)):
            info["image"] = buf.value
        times = [wintypes.FILETIME() for _ in range(4)]
        if k.GetProcessTimes(h, *(ctypes.byref(t) for t in times)):
            created = (int(times[0].dwHighDateTime) << 32) | int(times[0].dwLowDateTime)
            info["created"] = str(created) if created else None
    finally:
        k.CloseHandle(h)
    return info


def process_image(pid):
    """Full path of pid's executable, or None if unknown (Windows only)."""
    return query_process(pid)["image"]


def process_create_time(pid):
    """pid's creation time (decimal string of FILETIME ticks), or None if unknown (Windows
    only).  With the pid it identifies one process: a reused pid has a later creation time."""
    return query_process(pid)["created"]


def _given(value):
    return value is not None and str(value).strip() != ""


def _same_exe(actual, image):
    return os.path.basename(str(actual)).lower() == os.path.basename(str(image)).lower()


def pid_alive(pid, image=None, created=None):
    """True if pid is running and nothing says it is another process.

    With created (a recorded process_create_time()), the pid must have exactly that creation
    time; an unreadable one does not count.  With image (an exe path or name), a live pid whose
    executable has a different file name counts as gone (the pid was reused).  With either,
    ERROR_ACCESS_DENIED counts as gone: whatever holds the pid is not a game this user launched.
    Without both, access denied still means "some process exists".
    """
    if not pid:
        return False
    pid = int(pid)
    if sys.platform == "win32":
        info = query_process(pid)
        if info["error"]:
            return info["error"] == ERROR_ACCESS_DENIED and not (image or _given(created))
        if not info["alive"]:
            return False
        if _given(created) and str(created).strip() != str(info["created"]):
            return False
        if image and info["image"] and not _same_exe(info["image"], image):
            return False
        return True
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def is_session_game(pid, image=None, created=None):
    """Positive identification of the game a session (or launch) started: pid is running and
    its creation time equals `created`, or, for a record without one (an older session file),
    its exe file name is readable and equals `image`'s.  False without image and created,
    on access denied, or off Windows (nothing can be confirmed there)."""
    if not pid or sys.platform != "win32" or not (image or _given(created)):
        return False
    info = query_process(int(pid))
    if info["error"] or not info["alive"]:
        return False
    if _given(created):
        if str(created).strip() != str(info["created"]):
            return False
        return not (image and info["image"] and not _same_exe(info["image"], image))
    return bool(info["image"]) and _same_exe(info["image"], image)


def kill_pid(pid, image=None, created=None):
    """taskkill /PID <pid> /T /F.  Never kills this process or its parent.  With image or
    created (a session's exe / pid_created), only a pid that is_session_game() confirms."""
    if not pid or int(pid) in (os.getpid(), os.getppid()):
        return "refused to kill this process"
    if (image or _given(created)) and not is_session_game(pid, image, created):
        return (f"refused to kill pid {pid}: it is not the recorded game (creation time or exe "
                "does not match: the pid was reused, or it cannot be opened)")
    try:
        r = subprocess.run(["taskkill", "/PID", str(int(pid)), "/T", "/F"], capture_output=True,
                           text=True, errors="replace", timeout=60)
        return (r.stdout + r.stderr).strip() or f"taskkill exited {r.returncode}"
    except (OSError, subprocess.SubprocessError) as e:
        return f"taskkill failed: {e}"


# ---------------------------------------------------------------------------
# Session file (CLI)
# ---------------------------------------------------------------------------
def load_session(path=SESSION_FILE):
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, dict) or not data.get("port"):
            raise ValueError("not a session object")
        return data
    except FileNotFoundError:
        raise _infra(f"no agent bridge session ({path}): run `py Scripts/agent_bridge.py "
                     "launch` first, or pass --port", "no_session")
    except (OSError, ValueError) as e:
        raise _infra(f"unreadable session file {path}: {e}", "no_session")


def write_session(path, data):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=2)
        f.write("\n")
    os.replace(tmp, path)


def remove_session(path):
    try:
        os.remove(path)
        return True
    except FileNotFoundError:
        return False


# ---------------------------------------------------------------------------
# walk_to helpers
# ---------------------------------------------------------------------------
def _clamp(v, lo, hi):
    return max(lo, min(hi, v))


def _normalize_yaw(yaw):
    yaw = math.fmod(float(yaw), 360.0)
    if yaw > 180.0:
        yaw -= 360.0
    elif yaw <= -180.0:
        yaw += 360.0
    return yaw


def wasd_for(dx, dy, threshold):
    """Keys for a world-space move (W=+X, S=-X, D=+Y, A=-Y while control yaw is 0).

    Each component above `threshold` gets its key; if neither is, the larger one does.
    Returns (keys, unit direction those keys intend).
    """
    keys, ix, iy = [], 0.0, 0.0
    if dx > threshold:
        keys.append("W")
        ix = 1.0
    elif dx < -threshold:
        keys.append("S")
        ix = -1.0
    if dy > threshold:
        keys.append("D")
        iy = 1.0
    elif dy < -threshold:
        keys.append("A")
        iy = -1.0
    if not keys:
        if abs(dx) >= abs(dy):
            keys.append("W" if dx >= 0 else "S")
            ix = 1.0 if dx >= 0 else -1.0
        else:
            keys.append("D" if dy >= 0 else "A")
            iy = 1.0 if dy >= 0 else -1.0
    n = math.hypot(ix, iy)
    return keys, (ix / n, iy / n)


# ---------------------------------------------------------------------------
# The bridge
# ---------------------------------------------------------------------------
class Bridge:
    """The packaged game, driven through its agent bridge.

    Make one with Bridge.launch(...) (start the game) or Bridge.attach(port, token) (connect to a
    running one).  Every method sends real input or reads state; none calls game functions.
    Attributes: conn, proc, job, pid, pid_created, port, token, host, run_dir, exe, command_line,
    game_log, fresh (the STALE gate dict), stale, ping_info, warnings, detached.
    """

    def __init__(self, conn=None, *, port=None, token=None, host=DEFAULT_HOST, run_dir=None,
                 pid=None, exe=None, detached=False, pid_created=None):
        self.conn = conn
        self.port = port
        self.token = token
        self.host = host
        self.run_dir = run_dir
        self.exe = exe
        self.detached = detached
        self.proc = None
        self.job = None
        self.pid = pid
        self._owned_pid = pid       # the only pid quit() may wait for or kill
        self.pid_created = pid_created  # _owned_pid's creation time: identifies it (reuse-safe)
        self.command_line = None
        self.game_log = None
        self.fresh = None
        self.stale = False
        self.ping_info = None
        self.warnings = []
        self.started_at = None
        self.map = None
        self.expected_experience = None
        self.boot = None
        self.tail = None
        self.bridge_listen_line = None  # 'AgentBridge: listening on 127.0.0.1:<port>' from game.log
        self._bind_failure = None
        self._t0 = time.time()
        self.shutdown_note = None
        self.transcript = Transcript.for_run_dir(run_dir)

    # --- construction -----------------------------------------------------------------
    @classmethod
    def launch(cls, map=DEFAULT_MAP, expect_experience=None, frontend=False, allow_stale=False,
               detach=False, run_suffix="bridge", timeout=240, extra_args=(), *, exe=None,
               exe_argv=None, skip_freshness=False, run_dir=None, ready_timeout=READY_TIMEOUT_S,
               port=None, host=DEFAULT_HOST):
        """Gate, launch and connect.  Returns a ready Bridge (ping.ready true, token checked).

        map / frontend / expect_experience / timeout behave like playtest_boot.py.  A STALE build
        raises BridgeError(code="stale_build") unless allow_stale (then .stale is True).  The
        game runs in a kill-on-close job unless detach.  Test-only: exe_argv (the program part
        of the command line, e.g. [python, fake_game.py]), skip_freshness, run_dir.
        """
        with pb.log_stream(_log_out()):
            return cls._launch(map, expect_experience, frontend, allow_stale, detach, run_suffix,
                               timeout, extra_args, exe, exe_argv, skip_freshness, run_dir,
                               ready_timeout, port, host)

    @classmethod
    def _launch(cls, map, expect_experience, frontend, allow_stale, detach, run_suffix, timeout,
                extra_args, exe, exe_argv, skip_freshness, run_dir, ready_timeout, port, host):
        if exe_argv:
            exe_argv = [str(a) for a in exe_argv]
            exe = os.path.abspath(exe or exe_argv[0])
        else:
            if sys.platform != "win32":
                raise _infra("the packaged game is a Win64 build: run this on Windows",
                             "launch_failed")
            exe = os.path.abspath(exe or pb.DEFAULT_EXE)
            if not os.path.isfile(exe):
                raise _infra(f"game exe not found: {exe} -- package first: "
                             "py Scripts/package_playtest.py", "launch_failed")
            exe = pb.resolve_exe(exe)
        expected = pb.experience_name(
            expect_experience or (FRONTEND_EXPERIENCE if frontend else DEFAULT_EXPERIENCE))
        map_arg = None if frontend else (map or DEFAULT_MAP)

        if skip_freshness:
            fresh = {"fresh": None, "skipped": True, "reasons": [],
                     "warnings": ["staleness gate skipped (test launch)"], "git_error": None,
                     "build": None, "head": None, "branch": None}
        else:
            fresh, code = pb.gate(exe, allow_stale)
            if code is not None:
                reasons = fresh.get("reasons") or ([fresh["git_error"]] if fresh.get("git_error")
                                                   else [])
                raise _infra("STALE BUILD: " + "; ".join(str(r) for r in reasons)
                             + " -- re-package: py Scripts/package_playtest.py "
                             "(allow_stale is for diagnosis only)", "stale_build", fresh)

        run_dir = os.path.abspath(run_dir) if run_dir else pb.new_run_dir(run_suffix)
        os.makedirs(run_dir, exist_ok=True)
        port = int(port or free_port(host))
        token = uuid.uuid4().hex
        self = cls(port=port, token=token, host=host, run_dir=run_dir, exe=exe, detached=detach)
        self.fresh = fresh
        self.stale = fresh.get("fresh") is False
        self.map = map_arg
        self.expected_experience = expected
        self.game_log = os.path.join(run_dir, "game.log")
        bridge_args = ["-AgentBridge", f"-AgentBridgePort={port}", f"-AgentBridgeToken={token}",
                       f"-AgentBridgeShotDir={pb.quote_value(run_dir)}"]
        game_args = pb.build_game_args(map_arg, self.game_log,
                                       extra=bridge_args + list(extra_args))
        self.command_line = pb.build_command_line(exe, game_args, exe_argv)
        self.started_at = datetime.datetime.now().isoformat(timespec="seconds")
        if self.stale:
            msg = "STALE BUILD (allowed): " + "; ".join(fresh.get("reasons") or [])
            self.warnings.append(msg)
            log(f"WARNING: {msg} -- not acceptance evidence")
        self._note("launch", {"command_line": self.command_line, "port": port, "map": map_arg,
                              "expected_experience": expected, "stale": self.stale,
                              "detached": detach})
        log(f"Run dir: {run_dir}")
        log(f"Launching: {self.command_line}")
        t0 = time.time()
        try:
            self.proc, self.job, _ = pb.launch_game(exe, game_args, keep_running=detach,
                                                    exe_argv=exe_argv, detached=detach)
        except OSError as e:
            raise _infra(f"could not start the game: {e}", "launch_failed") from e
        self.pid = self._owned_pid = self.proc.pid
        self.pid_created = process_create_time(self.proc.pid)
        if not detach and self.job is None:
            self.warnings.append("no kill-on-close job: the game will not die with this process")
        try:
            self._wait_until_ready(t0, timeout, ready_timeout)
        except BaseException:
            self._teardown_failed_launch()
            raise
        log(f"Bridge ready on {host}:{port} (pid {self.pid}, experience "
            f"{self.ping_info.get('experience')})")
        return self

    def _watch_bridge_lines(self, lines):
        """Called with every game.log line read (by wait_for_experience too)."""
        for line in lines:
            if "AgentBridge: failed to listen" in line:
                self._bind_failure = pb.strip_timestamp(line)
            elif "AgentBridge: listening on" in line:
                self.bridge_listen_line = pb.strip_timestamp(line)

    def _scan_log(self):
        """Read new game.log lines into the BootState (crash markers) and the bridge watch."""
        lines = self.tail.poll() if self.tail else []
        if lines:
            with pb.log_stream(_log_out()):
                self.boot.scan(lines, time.time() - self._t0)
        return lines

    def crash_lines(self):
        """Crash markers seen in game.log so far (reads any new lines first)."""
        if self.boot is None:
            return []
        self._scan_log()
        return list(self.boot.crash_lines)

    def _wait_until_ready(self, t0, timeout, ready_timeout):
        self._t0 = t0
        self._bind_failure = None
        self.boot = pb.BootState(self.expected_experience)
        self.tail = _WatchedTail(pb.LogTail(self.game_log), self._watch_bridge_lines)
        outcome = pb.wait_for_experience(self.proc, self.tail, self.boot, timeout, t0)
        if outcome == "crash":
            pb.wait_after_crash(self.proc, self.tail, self.boot, t0)
            raise _infra(f"the game crashed while booting: {self.boot.crash_lines[0]}", "crash")
        if outcome == "no_experience":
            raise _gameplay(f"no experience: {self.boot.failed_identify_line}", "no_experience")
        if outcome == "exited":
            raise _infra(f"the game exited (code {self.proc.returncode}) before the experience "
                         "loaded", "exited")
        failure = self.boot.experience_failure()
        if failure:
            raise _gameplay(failure, "wrong_experience")
        if outcome == "timeout":
            raise _infra(f"experience {self.expected_experience} did not load within "
                         f"{timeout:.0f}s", "boot_timeout")
        self._ready_loop(ready_timeout)

    def _ready_loop(self, ready_timeout):
        """Connect and ping until ping.ready (token checked), within ready_timeout seconds.

        It keeps ONE connection.  A ping that times out (no_response: the game thread is busy)
        is re-sent on the same connection, and the late replies are skipped by id: reconnecting
        would leave closed sockets in the listen backlog of the stalled game thread, which the
        bridge accepts (and counts) before it reads their EOF, so the next connect could be
        refused.  A new connection is opened only when this one is
        lost (disconnected / closed) or the bridge refused it (too_many_connections: another
        client holds the slots, which can free up).
        """
        deadline = time.time() + ready_timeout
        last = "no connection yet"
        while True:
            self._scan_log()
            if self.boot is not None and self.boot.crash_lines:
                raise _infra(f"the game crashed: {self.boot.crash_lines[0]}", "crash")
            if self._bind_failure:
                raise _infra(f"the bridge could not listen ({self._bind_failure}); is another "
                             "game holding the port?", "bind_failed")
            if self.proc is not None and self.proc.poll() is not None:
                raise _infra(f"the game exited (code {self.proc.returncode}) before the bridge "
                             "was ready", "exited")
            if time.time() >= deadline:
                raise _infra(f"the bridge was not ready within {ready_timeout:.0f}s after the "
                             f"experience loaded: {last}", "not_ready")
            if self.conn is None or self.conn.closed:
                try:
                    self.conn = BridgeConnection(self.port, self.host, transcript=self.transcript,
                                                 connect_timeout=2.0)
                except BridgeError as e:
                    self.conn = None
                    last = str(e)
                    time.sleep(READY_RETRY_S)
                    continue
            try:
                ping = self.conn.request("ping", timeout=READY_PING_TIMEOUT_S)
            except BridgeError as e:
                last = str(e)
                if e.code == "no_response":
                    continue  # same connection: its late reply is skipped by id
                if e.code in ("disconnected", "closed", "too_many_connections"):
                    self.conn.close()
                    self.conn = None
                elif e.code != "not_ready":
                    raise
                time.sleep(READY_RETRY_S)
                continue
            if ping.get("token") != self.token:
                raise _infra(f"port {self.port} is answered by another process (token "
                             f"mismatch, pid {ping.get('pid')}): not the game this launch "
                             "started", "token_mismatch", ping)
            if ping.get("pid") not in (None, self.pid):
                self.warnings.append(f"ping pid {ping.get('pid')} != launched pid {self.pid}")
            if ping.get("ready"):
                self.ping_info = ping
                self._check_build(ping)
                return
            last = (f"ready=false (experience_loaded={ping.get('experience_loaded')}, "
                    f"loading_screen={ping.get('loading_screen')}, "
                    f"has_pawn={ping.get('has_pawn')})")
            time.sleep(READY_RETRY_S)

    def _check_build(self, ping):
        reported = (ping.get("build") or {}).get("commit") if isinstance(ping.get("build"),
                                                                          dict) else None
        recorded = ((self.fresh or {}).get("build") or {}).get("commit")
        if reported and recorded and reported != recorded:
            self.warnings.append(f"the game reports build commit {reported}, but "
                                 f"build_info.json says {recorded}")

    def _teardown_failed_launch(self):
        if self.conn is not None:
            self.conn.close()
            self.conn = None
        if self.proc is not None:
            self.shutdown_note = pb.kill_tree(self.proc, self.job)
            log(f"Shutdown after failed launch: {self.shutdown_note}")
        if self.job is not None:
            self.job.close()
            self.job = None
        if self.tail is not None:
            self.tail.close()

    @classmethod
    def attach(cls, port, token=None, *, host=DEFAULT_HOST, run_dir=None, pid=None, exe=None,
               pid_created=None, wait_ready=False, timeout=10.0):
        """Connect to a bridge that is already running.

        token, when given, must match ping.token.  pid (e.g. from the session file) is the
        process quit() may wait for and, if is_session_game() confirms it (creation time
        `pid_created` when recorded, else its exe name matches `exe`), kill.  wait_ready polls
        ping until ready (up to timeout).
        """
        self = cls(port=int(port), token=token, host=host, run_dir=run_dir, pid=pid, exe=exe,
                   detached=True, pid_created=pid_created)
        self.conn = BridgeConnection(self.port, host, transcript=self.transcript)
        try:
            ping = self.conn.request("ping", timeout=timeout)
            if token is not None and ping.get("token") != token:
                raise _infra(f"port {port} is answered by another process (token mismatch, "
                             f"pid {ping.get('pid')})", "token_mismatch", ping)
            if pid is not None and ping.get("pid") not in (None, pid):
                self.warnings.append(f"ping pid {ping.get('pid')} != session pid {pid}")
            if self.pid is None:
                self.pid = ping.get("pid")  # informational only: never killed
            deadline = time.monotonic() + timeout
            while wait_ready and not ping.get("ready"):
                if time.monotonic() >= deadline:
                    raise _infra(f"the bridge was not ready within {timeout:.0f}s", "not_ready",
                                 ping)
                time.sleep(0.25)
                ping = self.conn.request("ping", timeout=timeout)
        except BaseException:
            self.conn.close()
            raise
        self.ping_info = ping
        self._note("attach", {"port": self.port, "pid": ping.get("pid")})
        return self

    def reconnect(self, timeout=10.0):
        """Close the current connection and open a new one.

        Closing is a peer EOF to the bridge (Specs/agent_bridge_v1.md section 2): the old
        connection stops counting toward the 4-connection limit at once, but the bridge still
        runs any request it already received (a tap or axis keeps its input until its hold_s
        ends, a wait / wait_until runs out) and releases everything that connection held only
        when those have finished.  Only quit cancels pending commands.
        """
        if self.conn is not None:
            self.conn.close()
        self.conn = BridgeConnection(self.port, self.host, transcript=self.transcript)
        ping = self.conn.request("ping", timeout=timeout)
        if self.token is not None and ping.get("token") != self.token:
            self.conn.close()
            raise _infra(f"port {self.port} is answered by another process (token mismatch)",
                         "token_mismatch", ping)
        self.ping_info = ping
        return ping

    # --- plumbing ---------------------------------------------------------------------
    def _note(self, what, data):
        if self.transcript is not None:
            self.transcript.write("note", None, {"note": what, **data})

    def request(self, cmd, timeout=None, **args):
        """Raw protocol call (see BridgeConnection.request)."""
        if self.conn is None or self.conn.closed:
            raise _infra(f"{cmd}: not connected to the bridge", "closed")
        try:
            return self.conn.request(cmd, timeout=timeout, **args)
        except BridgeError as e:
            if e.code in ("disconnected", "no_response") and self.proc is not None:
                extra = []
                if self.proc.poll() is not None:
                    extra.append(f"the game exited with code {self.proc.returncode}")
                if self.boot is not None:
                    self._scan_log()
                    if self.boot.crash_lines:
                        extra.append(f"crash: {self.boot.crash_lines[0]}")
                if extra:
                    raise BridgeError(f"{e} ({'; '.join(extra)})", kind=e.kind,
                                      response=e.response, code=e.code) from e
            raise

    def is_alive(self):
        if self.proc is not None:
            return self.proc.poll() is None
        if self._owned_pid:
            return pid_alive(self._owned_pid, self.exe, self.pid_created)
        return self.conn is not None and not self.conn.closed

    def info(self):
        """What a session file or a report needs to know about this game."""
        build = (self.fresh or {}).get("build") or {}
        return {"pid": self.pid, "pid_created": self.pid_created, "port": self.port,
                "token": self.token, "host": self.host,
                "run_dir": self.run_dir, "exe": self.exe, "command_line": self.command_line,
                "game_log": self.game_log, "map": self.map,
                "expected_experience": self.expected_experience,
                "commit": build.get("commit") or (self.fresh or {}).get("head"),
                "build": build or None, "stale": self.stale,
                "stale_reasons": (self.fresh or {}).get("reasons") or [],
                "started_at": self.started_at, "detached": self.detached,
                "job": self.job is not None, "warnings": list(self.warnings)}

    # --- commands ---------------------------------------------------------------------
    def ping(self):
        resp = self.request("ping")
        self.ping_info = resp
        return resp

    def state(self, sections=None, radius=1500, include_all_actors=False):
        """The `state` object: {section: ...} for the requested sections (default all)."""
        args = {"radius": radius}
        if sections is not None:
            if isinstance(sections, str):
                sections = [s.strip() for s in sections.split(",") if s.strip()]
            args["sections"] = list(sections)
        if include_all_actors:
            args["include_all_actors"] = True
        resp = self.request("state", **args)
        st = resp.get("state")
        if not isinstance(st, dict):
            raise _infra("state: the response has no 'state' object", "bad_response", resp)
        return st

    @staticmethod
    def _key_args(key_or_keys):
        if isinstance(key_or_keys, (list, tuple)):
            return {"keys": [str(k) for k in key_or_keys]}
        return {"key": str(key_or_keys)}

    def press(self, key, unsafe=False):
        """Key down; it stays held until release(), this connection closes, or quit."""
        args = self._key_args(key)
        if unsafe:
            args["unsafe"] = True
        return self.request("key", action="press", **args)

    def release(self, key):
        return self.request("key", action="release", **self._key_args(key))

    def tap(self, key_or_keys, hold_s=0.1, unsafe=False):
        """Down, hold until the game has simulated hold_s with it down (2 frames at least), up.  A
        list goes down and up together.  The response has held_s (wall time), simulated_s and
        frames (Specs/agent_bridge_v1.md section 2, Hold time)."""
        args = self._key_args(key_or_keys)
        if unsafe:
            args["unsafe"] = True
        return self.request("key", action="tap", hold_s=hold_s, **args)

    def hold(self, key_or_keys, seconds, unsafe=False):
        """A tap held for `seconds` (the bridge times it, so a dying client cannot leave a key
        stuck)."""
        return self.tap(key_or_keys, hold_s=seconds, unsafe=unsafe)

    def axis(self, values, hold_s=None):
        args = {"values": dict(values)}
        if hold_s is not None:
            args["hold_s"] = hold_s
        return self.request("axis", **args)

    def mouse(self, x, y):
        return self.request("mouse", x=x, y=y)

    def click(self, x=None, y=None, button="Left", hold_s=None):
        """Click at viewport pixel (x, y), or at the current virtual cursor."""
        if (x is None) != (y is None):
            raise ValueError("give both x and y, or neither")
        args = {"button": button}
        if x is not None:
            args.update(x=x, y=y)
        if hold_s is not None:
            args["hold_s"] = hold_s
        return self.request("click", **args)

    @staticmethod
    def _norm_id(value):
        return str(value or "").strip().strip("{}").replace("-", "").lower()

    def find_buttons(self, st=None):
        """Visible buttons from state.pause_menu (Identify buttons) and state.widgets.

        Each: {label, id, center:[x, y], enabled, source}.  Duplicates (same center) collapse.
        Every level is type-checked: before the world exists a section is an object
        {valid:false, reason} instead of an array, and that simply yields no buttons.
        """
        if st is None:
            st = self.state(["pause_menu", "widgets"])
        found, seen = [], set()

        def as_dict(v):
            return v if isinstance(v, dict) else {}

        def as_list(v):
            return [x for x in v if isinstance(x, dict)] if isinstance(v, list) else []

        def add(label, button_id, center, enabled, source):
            if not (isinstance(center, (list, tuple)) and len(center) >= 2):
                return
            try:
                key = (round(float(center[0]), 1), round(float(center[1]), 1))
            except (TypeError, ValueError):
                return
            if key in seen:
                return
            seen.add(key)
            found.append({"label": label, "id": button_id, "center": [center[0], center[1]],
                          "enabled": enabled, "source": source})

        st = as_dict(st)
        inv = as_dict(as_dict(st.get("pause_menu")).get("inventory_tab"))
        for row in as_list(inv.get("rows")):
            b = row.get("identify_button")
            if isinstance(b, dict) and b:
                add(b.get("label"), row.get("instance_id"), b.get("center"), b.get("enabled"),
                    f"pause_menu.inventory_tab row '{row.get('text')}'")
        for w in as_list(st.get("widgets")):
            for b in as_list(w.get("buttons")):
                add(b.get("label"), b.get("id"), b.get("center"), b.get("enabled"),
                    f"widget {w.get('name') or w.get('class')}")
        return found

    def click_button(self, label=None, button_id=None, button="Left", hold_s=None):
        """Aim at a visible button by label (or id) and click its center.

        Aiming is what a player's eyes do; Slate still decides what the click hits (the
        response's `hit` is the deepest widget at the point -- on a button usually its label,
        "STextBlock" -- not necessarily the button).  Raises a gameplay BridgeError if no such
        button is on screen.
        """
        if label is None and button_id is None:
            raise ValueError("click_button needs a label or a button_id")
        buttons = self.find_buttons()
        if button_id is not None:
            want = self._norm_id(button_id)
            matches = [b for b in buttons if self._norm_id(b["id"]) == want]
        else:
            matches = [b for b in buttons if b["label"] == label]
            if not matches:
                want = str(label).strip().casefold()
                matches = [b for b in buttons if str(b["label"] or "").strip().casefold() == want]
        if not matches:
            visible = ", ".join(sorted({str(b["label"]) for b in buttons})) or "none"
            what = f"id '{button_id}'" if button_id is not None else f"label '{label}'"
            raise _gameplay(f"click_button: no visible button with {what} (visible: {visible})",
                            "no_button", {"buttons": buttons})
        target = matches[0]
        x, y = target["center"]
        resp = dict(self.click(x, y, button=button, hold_s=hold_s))
        resp["target"] = target
        resp["matches"] = len(matches)
        return resp

    def screenshot(self, path=None, complete_timeout_s=SCREENSHOT_COMPLETE_S):
        """Ask the game for a viewport PNG; returns its path once the file is complete."""
        args = {}
        if path is not None:
            args["path"] = os.path.abspath(path)
        resp = self.request("screenshot", **args)
        shot = resp.get("path")
        if not isinstance(shot, str) or not shot:
            raise _infra("screenshot: the response has no path", "bad_response", resp)
        expected_bytes = resp.get("bytes") if isinstance(resp.get("bytes"), int) else None
        deadline = time.monotonic() + complete_timeout_s
        while not file_complete(shot, expected_bytes):
            if time.monotonic() >= deadline:
                raise _infra(f"screenshot: {shot} is missing or incomplete "
                             f"{complete_timeout_s:g}s after the bridge reported it",
                             "screenshot_incomplete", resp)
            time.sleep(0.05)
        return shot

    def wait(self, seconds):
        return self.request("wait", seconds=seconds)

    def wait_until(self, condition, timeout_s=10, poll_s=None):
        """Bridge-side wait on a state condition (grammar: Specs/agent_bridge_v1.md section 4).

        Returns {elapsed_s, values}; a timeout raises a gameplay BridgeError whose .response
        holds the last values.
        """
        args = {"condition": condition, "timeout_s": timeout_s}
        if poll_s is not None:
            args["poll_s"] = poll_s
        return self.request("wait_until", **args)

    def wait_for(self, predicate_fn, timeout_s=10, sections=None, poll_s=0.1, radius=1500):
        """Client-side wait: poll state(sections, radius) until predicate_fn(state) is truthy.

        Returns the state that satisfied it; raises a gameplay BridgeError(code
        "wait_for_timeout") with the last state as .response.
        """
        deadline = time.monotonic() + timeout_s
        while True:
            st = self.state(sections, radius=radius)
            if predicate_fn(st):
                return st
            if time.monotonic() >= deadline:
                raise _gameplay(f"wait_for: condition not met within {timeout_s:g}s",
                                "wait_for_timeout", st)
            time.sleep(poll_s)

    @staticmethod
    def _held_walk_keys(st):
        """W/A/S/D keys state.input.held lists (any connection), or [] if input is invalid."""
        inp = st.get("input") if isinstance(st, dict) else None
        if not isinstance(inp, dict) or inp.get("valid") is not True:
            return []
        held = inp.get("held") if isinstance(inp.get("held"), list) else []
        names = [str(h.get("key")) for h in held
                 if isinstance(h, dict) and h.get("kind", "key") == "key"]
        return [k for k in WALK_KEYS if any(n.upper() == k for n in names)]

    def _walk_read(self, tx, ty, check_held=False):
        st = self.state(WALK_SECTIONS + (("input",) if check_held else ()))
        player = st.get("player") or {}
        if not player.get("valid"):
            raise _gameplay(f"walk_to: no player ({player.get('reason') or 'invalid'})",
                            "no_player", st)
        loc = player.get("location")
        if not (isinstance(loc, list) and len(loc) >= 2):
            raise _infra("walk_to: player.location is missing", "bad_response", st)
        x, y = float(loc[0]), float(loc[1])
        if player.get("is_dead"):
            raise _gameplay(f"walk_to aborted: the player is dead at ({x:.0f}, {y:.0f}), "
                            f"target ({tx:.0f}, {ty:.0f})", "player_dead", st)
        pm = st.get("pause_menu") or {}
        if pm.get("open"):
            raise _gameplay(f"walk_to aborted: the pause menu is open (tab "
                            f"{pm.get('active_tab')}) at ({x:.0f}, {y:.0f})", "menu_open", st)
        rot = player.get("control_rotation")
        if not (isinstance(rot, list) and len(rot) >= 2):
            raise _gameplay("walk_to: player.control_rotation is unavailable, so the WASD basis "
                            "cannot be checked", "yaw_not_zero", st)
        yaw = _normalize_yaw(rot[1])
        if abs(yaw) > YAW_TOLERANCE_DEG:
            raise _gameplay(f"walk_to: control yaw is {yaw:.1f} deg, not 0: the WASD basis "
                            "(W=+X, D=+Y) would be rotated", "yaw_not_zero", st)
        held = self._held_walk_keys(st) if check_held else []
        if held:
            # A key this connection holds would make the bridge refuse the tap (bad_request
            # "already held"), and one another connection holds keeps pushing the pawn.
            raise _gameplay(f"walk_to: {'+'.join(held)} {'is' if len(held) == 1 else 'are'} "
                            "already held (a press / hold_key without release): release it "
                            "first (release / release_key), then walk", "keys_held", st)
        return x, y

    def walk_to(self, x, y, tol=50, timeout_s=None):
        """Walk to world (x, y) with W/A/S/D pulses, closed loop (Specs/agent_bridge_v1.md s.6).

        Returns {"final": [x, y], "dist": d, "pulses": n, "side_steps": k}.  Raises a gameplay
        BridgeError: stuck, walk_timeout, mapping_mismatch, yaw_not_zero, player_dead,
        menu_open, no_player, keys_held (W/A/S/D already held when it starts: release it).
        """
        tx, ty, tol = float(x), float(y), float(tol)
        px, py = self._walk_read(tx, ty, check_held=True)
        dist = math.hypot(tx - px, ty - py)
        if timeout_s is None:
            timeout_s = 3.0 * dist / WALK_SPEED + 3.0
        deadline = time.monotonic() + timeout_s
        threshold = max(tol * 0.5, 20.0)
        pulses = side_steps = 0
        side = 1.0
        calibrated = False
        window = collections.deque([dist], maxlen=STUCK_PULSES + 1)
        self._note("walk_to", {"from": [round(px, 1), round(py, 1)], "to": [tx, ty], "tol": tol,
                               "timeout_s": round(timeout_s, 2)})
        while dist > tol:
            if time.monotonic() >= deadline:
                raise _gameplay(f"walk_to timed out after {timeout_s:.1f}s at ({px:.0f}, "
                                f"{py:.0f}), target ({tx:.0f}, {ty:.0f}), dist {dist:.0f}",
                                "walk_timeout", {"final": [px, py], "dist": dist,
                                                 "pulses": pulses})
            keys, intended = wasd_for(tx - px, ty - py, threshold)
            hold = round(_clamp(dist / WALK_SPEED - 0.05, 0.05, 0.4), 3)
            self.tap(keys, hold_s=hold)
            pulses += 1
            nx, ny = self._walk_read(tx, ty)
            if not calibrated:
                mx, my = nx - px, ny - py
                if mx * intended[0] + my * intended[1] <= 0.0:
                    raise _gameplay(
                        f"walk_to: movement mapping mismatch: tapped {'+'.join(keys)} for "
                        f"{hold:.2f}s expecting a move toward ({intended[0]:+.2f}, "
                        f"{intended[1]:+.2f}) (W=+X, S=-X, D=+Y, A=-Y), but the pawn moved "
                        f"({mx:+.1f}, {my:+.1f})", "mapping_mismatch",
                        {"keys": keys, "moved": [mx, my]})
                calibrated = True
            px, py = nx, ny
            dist = math.hypot(tx - px, ty - py)
            window.append(dist)
            if (dist > tol and len(window) == STUCK_PULSES + 1
                    and window[0] - window[-1] < STUCK_PROGRESS_CM):
                if side_steps >= MAX_SIDE_STEPS:
                    raise _gameplay(f"stuck at ({px:.0f}, {py:.0f}), target ({tx:.0f}, "
                                    f"{ty:.0f}), dist {dist:.0f}", "stuck",
                                    {"final": [px, py], "dist": dist, "pulses": pulses,
                                     "side_steps": side_steps})
                rx, ry = tx - px, ty - py
                perp_x, perp_y = -ry * side, rx * side
                n = math.hypot(perp_x, perp_y) or 1.0
                keys_s, _ = wasd_for(perp_x / n, perp_y / n, 0.4)
                self.tap(keys_s, hold_s=SIDE_STEP_S)
                side_steps += 1
                side = -side
                px, py = self._walk_read(tx, ty)
                dist = math.hypot(tx - px, ty - py)
                window.clear()
                window.append(dist)
        result = {"final": [round(px, 1), round(py, 1)], "dist": round(dist, 1),
                  "pulses": pulses, "side_steps": side_steps}
        self._note("walk_to_done", result)
        return result

    def quit(self, timeout_s=QUIT_TIMEOUT_S):
        """Send quit, then wait for the game to exit, killing it if it does not.

        Returns {"ok": true, "cmd": "quit", "response", "exit_code", "shutdown"}.
        """
        resp, notes = None, []
        if self.conn is not None and not self.conn.closed:
            try:
                resp = self.conn.request("quit", timeout=10.0)
            except BridgeError as e:
                notes.append(f"quit request: {e}")
            self.conn.close()
        exit_code = None
        if self.proc is not None:
            try:
                exit_code = self.proc.wait(timeout=timeout_s)
                notes.append(f"exited (code {exit_code})")
            except subprocess.TimeoutExpired:
                notes.append(f"did not exit within {timeout_s:g}s: " + pb.kill_tree(self.proc,
                                                                                    self.job))
                exit_code = self.proc.returncode
        elif self._owned_pid and int(self._owned_pid) not in (os.getpid(), os.getppid()):
            pid, created = self._owned_pid, self.pid_created
            end = time.monotonic() + timeout_s
            while pid_alive(pid, self.exe, created) and time.monotonic() < end:
                time.sleep(0.2)
            if pid_alive(pid, self.exe, created):
                if is_session_game(pid, self.exe, created):
                    notes.append(f"pid {pid} did not exit within {timeout_s:g}s: "
                                 + kill_pid(pid, self.exe, created))
                else:
                    notes.append(f"pid {pid} still running (not killed: its creation time or "
                                 "exe could not be confirmed)")
            else:
                notes.append(f"pid {pid} exited")
        if self.job is not None:
            self.job.close()
            self.job = None
        if self.tail is not None:
            self.tail.close()
        self.shutdown_note = "; ".join(notes)
        self._note("quit", {"shutdown": self.shutdown_note})
        return {"ok": True, "cmd": "quit", "response": resp, "exit_code": exit_code,
                "shutdown": self.shutdown_note}

    def close(self):
        """Close the connection.  A game this Bridge launched without detach is killed (process
        tree + job); a detached or attached game keeps running."""
        if self.conn is not None:
            self.conn.close()
        if self.proc is not None and not self.detached and self.proc.poll() is None:
            self.shutdown_note = pb.kill_tree(self.proc, self.job)
        if self.job is not None and not self.detached:
            self.job.close()
            self.job = None
        if self.tail is not None:
            self.tail.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()


class _WatchedTail:
    """A playtest_boot.LogTail that also hands every line it returns to on_lines()."""

    def __init__(self, tail, on_lines):
        self._tail = tail
        self._on_lines = on_lines

    def poll(self):
        lines = self._tail.poll()
        if lines:
            self._on_lines(lines)
        return lines

    def flush(self):
        lines = self._tail.flush()
        if lines:
            self._on_lines(lines)
        return lines

    def __getattr__(self, name):  # fh, line_count, path, close
        return getattr(self._tail, name)


def file_complete(path, expected_bytes=None):
    """True once a screenshot file is fully written: non-empty, at least expected_bytes, and for
    a PNG, signature at the start and the IEND chunk at the end."""
    try:
        size = os.path.getsize(path)
        if size <= 0 or (expected_bytes and size < expected_bytes):
            return False
        if path.lower().endswith(".png"):
            with open(path, "rb") as f:
                head = f.read(8)
                f.seek(max(0, size - 12))
                tail = f.read(12)
            return head == b"\x89PNG\r\n\x1a\n" and tail[4:8] == b"IEND"
        return True
    except OSError:
        return False


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def _common_options(parser, suppress):
    default = argparse.SUPPRESS if suppress else None
    parser.add_argument("--session", default=argparse.SUPPRESS if suppress else SESSION_FILE,
                        help=f"session file (default: {SESSION_FILE})")
    parser.add_argument("--port", type=int, default=default,
                        help="talk to the bridge on this port instead of the session's")
    parser.add_argument("--token", default=default, help="expected ping token (with --port)")
    parser.add_argument("--host", default=argparse.SUPPRESS if suppress else DEFAULT_HOST,
                        help=argparse.SUPPRESS)


def build_parser():
    parser = argparse.ArgumentParser(
        prog="agent_bridge.py",
        description="Drive the packaged game through its agent bridge (real input only). "
                    "Prints JSON; exit 0 ok, 1 gameplay failure, 2 infra.")
    _common_options(parser, suppress=False)
    common = argparse.ArgumentParser(add_help=False)
    _common_options(common, suppress=True)
    sub = parser.add_subparsers(dest="command", required=True, metavar="command")

    p = sub.add_parser("launch", parents=[common], help="launch the game detached, write the "
                                                        "session file")
    where = p.add_mutually_exclusive_group()
    where.add_argument("--map", default=None,
                       help=f"map (default {DEFAULT_MAP}; the playtest test maps "
                            f"{pb.MOVEMENT_TEST_MAP} and {pb.COMBAT_TEST_MAP} load the same "
                            "experience)")
    where.add_argument("--frontend", action="store_true", help="no map: boot the front end")
    p.add_argument("--expect-experience", default=None)
    p.add_argument("--allow-stale", action="store_true",
                   help="run a STALE package (diagnostic only, never evidence)")
    p.add_argument("--timeout", type=float, default=240, help="experience load timeout (240)")
    p.add_argument("--ready-timeout", type=float, default=READY_TIMEOUT_S,
                   help=argparse.SUPPRESS)
    p.add_argument("--exe", default=None, help="game binary (default: the packaged playtest)")
    p.add_argument("--extra-arg", action="append", default=[], help=argparse.SUPPRESS)
    # Test-only switches (tests/fake_game.py stands in for the game).
    p.add_argument("--test-exe-argv", default=None, help=argparse.SUPPRESS)
    p.add_argument("--test-skip-freshness", action="store_true", help=argparse.SUPPRESS)
    p.add_argument("--run-dir", default=None, help=argparse.SUPPRESS)

    sub.add_parser("ping", parents=[common], help="bridge/game status")
    p = sub.add_parser("state", parents=[common], help="game state JSON")
    p.add_argument("--sections", default=None, help="comma-separated sections")
    p.add_argument("--radius", type=float, default=1500)
    p.add_argument("--all-actors", action="store_true", help="include level geometry")

    p = sub.add_parser("press", parents=[common], help="tap key(s): down, hold, up")
    p.add_argument("keys", nargs="+", help="FKey names, e.g. F, W D (together)")
    p.add_argument("--hold", type=float, default=0.1, help="hold seconds (0.1)")
    p.add_argument("--unsafe", action="store_true", help="allow denied debug keys")

    p = sub.add_parser("axis", parents=[common], help="gamepad axis values")
    p.add_argument("values", nargs="+", help="NAME=VALUE, e.g. Gamepad_LeftY=1")
    p.add_argument("--hold", type=float, default=None, help="hold seconds, then reset to 0")

    p = sub.add_parser("mouse", parents=[common], help="move the virtual cursor")
    p.add_argument("x", type=float)
    p.add_argument("y", type=float)

    p = sub.add_parser("click", parents=[common], help="click at X Y (viewport px) or the cursor")
    p.add_argument("coords", nargs="*", type=float)
    p.add_argument("--button", default="Left", choices=("Left", "Right", "Middle"))
    p.add_argument("--hold", type=float, default=None)

    p = sub.add_parser("click-button", parents=[common], help="click a visible button by label")
    p.add_argument("label", nargs="?", default=None)
    p.add_argument("--id", dest="button_id", default=None, help="button id instead of a label")
    p.add_argument("--button", default="Left", choices=("Left", "Right", "Middle"))

    p = sub.add_parser("walk-to", parents=[common], help="walk to world X Y with WASD")
    p.add_argument("x", type=float)
    p.add_argument("y", type=float)
    p.add_argument("--tol", type=float, default=50)
    p.add_argument("--timeout", type=float, default=None)

    p = sub.add_parser("screenshot", parents=[common], help="viewport PNG; prints its path")
    p.add_argument("path", nargs="?", default=None)

    p = sub.add_parser("wait", parents=[common], help="wait real seconds in the bridge")
    p.add_argument("seconds", type=float)

    p = sub.add_parser("wait-until", parents=[common], help="wait for a state condition")
    p.add_argument("condition")
    p.add_argument("--timeout", type=float, default=10)
    p.add_argument("--poll", type=float, default=None)

    p = sub.add_parser("quit", parents=[common], help="quit the game, delete the session file")
    p.add_argument("--timeout", type=float, default=QUIT_TIMEOUT_S)
    return parser


def _session_bridge(args):
    """Bridge for a CLI call: --port, else the session file (stale pid -> removed, infra)."""
    if args.port:
        return Bridge.attach(args.port, token=args.token, host=args.host), None
    session = load_session(args.session)
    pid, exe, created = session.get("pid"), session.get("exe"), session.get("pid_created")
    if pid and not pid_alive(pid, exe, created):
        remove_session(args.session)
        raise _infra(f"stale session: pid {pid} is no longer the session's game (exited, or the "
                     f"pid was reused); removed {args.session}", "stale_session", session)
    try:
        bridge = Bridge.attach(session["port"], token=session.get("token"),
                               host=session.get("host") or DEFAULT_HOST,
                               run_dir=session.get("run_dir"), pid=pid, exe=exe,
                               pid_created=created)
    except BridgeError as e:
        if e.code == "connect_failed" and pid and not pid_alive(pid, exe, created):
            remove_session(args.session)
            raise _infra(f"stale session: pid {pid} exited; removed {args.session}",
                         "stale_session", session) from e
        raise
    return bridge, session


def _cli_launch(args):
    existing = None
    try:
        existing = load_session(args.session)
    except BridgeError:
        pass
    if existing:
        if existing.get("pid") and pid_alive(existing["pid"], existing.get("exe"),
                                             existing.get("pid_created")):
            raise _infra(f"a game is already running for this session (pid {existing['pid']}, "
                         f"port {existing.get('port')}): `quit` it first", "session_running",
                         existing)
        remove_session(args.session)
    exe_argv = json.loads(args.test_exe_argv) if args.test_exe_argv else None
    bridge = Bridge.launch(map=args.map, expect_experience=args.expect_experience,
                           frontend=args.frontend, allow_stale=args.allow_stale, detach=True,
                           timeout=args.timeout, ready_timeout=args.ready_timeout,
                           extra_args=args.extra_arg, exe=args.exe, exe_argv=exe_argv,
                           skip_freshness=args.test_skip_freshness, run_dir=args.run_dir)
    info = bridge.info()
    session = {k: info[k] for k in ("pid", "pid_created", "port", "token", "host", "run_dir",
                                    "exe", "commit", "started_at", "map", "expected_experience",
                                    "stale", "command_line")}
    write_session(args.session, session)
    bridge.close()  # detached: the game keeps running
    return {"ok": True, "cmd": "launch", "session_file": os.path.abspath(args.session),
            "session": session, "ping": bridge.ping_info, "warnings": bridge.warnings,
            "stale": bridge.stale}


def _cli_quit_unreachable(args, error):
    """quit when the session's bridge does not answer: kill the session pid if it is still the
    game this session launched (is_session_game: its creation time matches pid_created, or, for
    an older session file without it, its exe name matches), then remove the session file."""
    session = load_session(args.session)
    pid, exe, created = session.get("pid"), session.get("exe"), session.get("pid_created")
    if not (pid and is_session_game(pid, exe, created)):
        raise error
    note = kill_pid(pid, exe, created)
    gone = False
    end = time.monotonic() + 15
    while time.monotonic() < end:
        if not pid_alive(pid, exe, created):
            gone = True
            break
        time.sleep(0.2)
    if not gone:
        raise _infra(f"quit: the bridge did not answer ({error}) and pid {pid} could not be "
                     f"killed: {note}", "kill_failed", session)
    return {"ok": True, "cmd": "quit", "response": None, "exit_code": None,
            "shutdown": f"bridge unreachable ({error}); killed pid {pid}: {note}",
            "session_removed": remove_session(args.session)}


def run_command(args):
    if args.command == "launch":
        return _cli_launch(args)
    try:
        bridge, session = _session_bridge(args)
    except BridgeError as e:
        if args.command == "quit" and not args.port and e.code in (
                "connect_failed", "no_response", "disconnected", "token_mismatch"):
            return _cli_quit_unreachable(args, e)
        raise
    try:
        c = args.command
        if c == "ping":
            return bridge.ping()
        if c == "state":
            st = bridge.state(args.sections, radius=args.radius,
                              include_all_actors=args.all_actors)
            return {"ok": True, "cmd": "state", "state": st}
        if c == "press":
            keys = args.keys[0] if len(args.keys) == 1 else args.keys
            return bridge.tap(keys, hold_s=args.hold, unsafe=args.unsafe)
        if c == "axis":
            values = {}
            for item in args.values:
                name, sep, value = item.partition("=")
                if not sep:
                    raise _infra(f"axis value '{item}' is not NAME=VALUE", "bad_request")
                try:
                    values[name] = float(value)
                except ValueError:
                    raise _infra(f"axis value '{item}' is not a number", "bad_request")
            return bridge.axis(values, hold_s=args.hold)
        if c == "mouse":
            return bridge.mouse(args.x, args.y)
        if c == "click":
            if len(args.coords) not in (0, 2):
                raise _infra("click takes X Y or nothing", "bad_request")
            x, y = (args.coords + [None, None])[:2]
            return bridge.click(x, y, button=args.button, hold_s=args.hold)
        if c == "click-button":
            if args.label is None and args.button_id is None:
                raise _infra("click-button needs a LABEL or --id", "bad_request")
            return bridge.click_button(label=args.label, button_id=args.button_id,
                                       button=args.button)
        if c == "walk-to":
            result = bridge.walk_to(args.x, args.y, tol=args.tol, timeout_s=args.timeout)
            return dict({"ok": True, "cmd": "walk_to"}, **result)
        if c == "screenshot":
            return {"ok": True, "cmd": "screenshot", "path": bridge.screenshot(args.path)}
        if c == "wait":
            return bridge.wait(args.seconds)
        if c == "wait-until":
            return bridge.wait_until(args.condition, timeout_s=args.timeout, poll_s=args.poll)
        if c == "quit":
            result = bridge.quit(timeout_s=args.timeout)
            if session is not None:
                result["session_removed"] = remove_session(args.session)
            return result
        raise _infra(f"unknown command {c}", "bad_request")
    finally:
        bridge.close()


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = build_parser().parse_args(argv)
    try:
        result = run_command(args)
    except BridgeError as e:
        log(f"{e.kind.upper()} ERROR: {e}")
        print(json.dumps(e.to_dict(), indent=1, ensure_ascii=False, default=str), flush=True)
        return EXIT_GAMEPLAY if e.kind == "gameplay" else EXIT_INFRA
    except KeyboardInterrupt:
        print(json.dumps({"ok": False, "kind": "infra", "error": "interrupted",
                          "message": "interrupted"}), flush=True)
        return EXIT_INFRA
    except Exception as e:  # never die without JSON on stdout
        import traceback
        traceback.print_exc()
        print(json.dumps({"ok": False, "kind": "infra", "error": "client_error",
                          "message": f"{type(e).__name__}: {e}"}), flush=True)
        return EXIT_INFRA
    print(json.dumps(result, indent=1, ensure_ascii=False, default=str), flush=True)
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
