#!/usr/bin/env python3
"""Mordecai Play MCP Server -- an agent plays the PACKAGED game with real input (Phase 2)

A stdio MCP server named "mordecai-play" (registered project-scoped in .mcp.json, so its tools
are mcp__mordecai-play__*).  It wraps Scripts/agent_bridge.py, the client of the in-game agent
bridge (Specs/agent_bridge_v1.md section 7).  Every input tool injects OS-level key and mouse
events into Slate, the path a player's keyboard and mouse take; no tool calls game functions.

Tools:
  launch_game(map, frontend, allow_stale, expect_experience)   STALE gate, launch, wait until ready
  get_state(sections, radius)                                  game state JSON
  press_key(key, hold_s)                                       tap (down, hold, up)
  hold_key(key, seconds)                                       timed hold, or key down until release_key
  release_key(key)
  walk_to(x, y, tol, timeout_s)                                closed-loop WASD walk
  click(x, y, button)                                          Slate click at a viewport pixel
  click_button(label, button_id)                               aim at a visible button and click it
  screenshot()                                                 the game's own frame (image + path)
  wait_until(condition, timeout_s)                             bridge-side wait on a state condition
  quit_game()                                                  quit, wait for exit, summarize the run

Rules the tools enforce or state: one game at a time, in a kill-on-close job (the game dies with
this server); a lock serializes tool calls; a STALE build is refused unless allow_stale is passed,
and then every result carries "STALE BUILD — not evidence"; a package that is fresh but was
built from a dirty (uncommitted) tree runs, and every result carries "DIRTY BUILD — diagnostic
only, not evidence".  Nothing here is acceptance evidence: story acceptance comes from
Scripts/playtests/*.py on a fresh package built from a clean, committed tree.

Usage:
  Claude Code starts it from .mcp.json (repo root, project scope; an interactive session asks once
  to approve project servers).  Install the SDK once: py -m pip install -r Scripts/requirements.txt
  py -X utf8 Scripts/mordecai_play_mcp.py                  stdio server
  py -X utf8 Scripts/mordecai_play_mcp.py --attach PORT [--token T]   (alias --attach-port)
        diagnostic: drive a bridge that is already running (agent_bridge.py launch, or
        Scripts/tests/fake_bridge.py); launch_game is refused and nothing is evidence
  Options: --image-max-chars N (base64 budget of the screenshot image, default 80000),
           --log-level LEVEL (stderr logging, default WARNING)

Output: a launched game's run dir Saved/Playtests/<YYYYmmdd-HHMMSS>-mcp/ holds game.log,
bridge.jsonl (every request/response plus one note per tool call) and bridge_NN.png.
Logs go to stderr only; stdout is the MCP protocol channel.

Exit codes:
  0 -- the client ended the session (stdin closed)
  2 -- infrastructure problem: the mcp package is missing (install hint on stderr), bad arguments
"""

import argparse
import atexit
import io
import json
import os
import sys
import threading
import traceback

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402

try:
    from mcp.server.mcpserver import Image, MCPServer
    from mcp.server.mcpserver.exceptions import ToolError
    MCP_IMPORT_ERROR = None
except ImportError as _e:  # handled in main(): exit 2 with an install hint
    MCPServer = Image = None
    MCP_IMPORT_ERROR = _e

    class ToolError(Exception):
        """Stand-in so this module imports without the SDK (tests skip; main() exits 2)."""

SERVER_NAME = "mordecai-play"
SERVER_VERSION = "1"
INSTALL_HINT = "py -m pip install -r Scripts/requirements.txt"
STALE_BANNER = "STALE BUILD — not evidence"
DIRTY_BANNER = "DIRTY BUILD — diagnostic only, not evidence"
ATTACHED_BANNER = ("NOT EVIDENCE — attached to an already running bridge (--attach); this server "
                   "did not launch it and did not check the build")
NO_GAME = "no game is running: call launch_game first"
RUN_SUFFIX = "mcp"

MAX_TEXT_CHARS = 60000          # ~15k tokens: under Claude Code's 25k-token MCP output cap
MAX_IMAGE_B64_CHARS = 80000     # base64 budget for the screenshot image (~20k tokens worst case)
ERROR_CONTEXT_CHARS = 3000
# (max width, JPEG quality) tried in order until the image fits the budget
JPEG_LADDER = ((1280, 80), (1280, 65), (1024, 70), (960, 60), (800, 60), (640, 55), (480, 50),
               (320, 50))
CONNECTION_LOST = frozenset({"disconnected", "no_response", "closed", "connect_failed", "crash",
                             "exited"})

RULE = ("RULES: real input only -- input tools inject OS-level key/mouse events through Slate, "
        "the path a player's keyboard and mouse take; nothing here calls game functions. Read the "
        "screenshots and say what is actually on screen before claiming anything is visible. "
        "Nothing here is acceptance evidence unless launch_game reported a FRESH build from a "
        "clean, committed tree (no 'STALE BUILD' or 'DIRTY BUILD' banner), and story acceptance "
        "comes from Scripts/playtests/*.py.")

INSTRUCTIONS = (
    "Play the PACKAGED Project Mordecai build (Development Win64, PackagedBuilds/Playtest) "
    "through its in-game agent bridge. Start with launch_game (DevTestMap, the arena), read "
    "get_state, act with press_key / hold_key / walk_to / click / click_button, look with "
    "screenshot, and end with quit_game. Keys are UE FKey names: W/A/S/D move (W=+X world, "
    "S=-X, D=+Y, A=-Y; the camera never rotates), F interact/pick up, I inventory, Escape pause "
    "menu, Q/E/R/One spells, Tab/BackSlash weapons. Screen coordinates are viewport pixels "
    "(1280x720, origin top-left, the same space as the screenshots). Debug keys with side "
    "effects (K, F1-F12, Insert, Delete, Home, End, PageUp, PageDown, Tilde, ...) are refused. "
    + RULE)


_LOG_STREAM = None


def set_log_stream(stream):
    """Send this module's log lines to `stream` (None = sys.stderr).  Returns the previous one."""
    global _LOG_STREAM
    previous = _LOG_STREAM
    _LOG_STREAM = stream
    return previous


def log(msg):
    print(f"[Play-MCP] {msg}", file=_LOG_STREAM if _LOG_STREAM is not None else sys.stderr,
          flush=True)


def _dumps(obj):
    return json.dumps(obj, ensure_ascii=False, separators=(",", ":"), default=str)


def _dirty_build(bridge):
    """True when the gate found the package fresh but its build_info says it was built from a
    dirty working tree (diagnostic only, as playtest_boot / us079 treat it)."""
    fresh = (bridge.fresh or {}) if bridge is not None else {}
    return bool(fresh.get("fresh")) and bool((fresh.get("build") or {}).get("dirty"))


def _dirty_note(bridge):
    build = (bridge.fresh or {}).get("build") or {}
    count = build.get("dirty_count")
    return (f"commit {pb.short(build.get('commit'))} plus "
            + (f"{count} uncommitted file(s)" if count is not None else "uncommitted changes"))


def _short_reasons(reasons, limit=2, chars=300):
    text = "; ".join(str(r) for r in (reasons or [])[:limit])
    if len(reasons or []) > limit:
        text += f"; (+{len(reasons) - limit} more)"
    return text[:chars]


# ---------------------------------------------------------------------------
# Output shaping (Claude Code caps MCP results at 25k tokens)
# ---------------------------------------------------------------------------
def fit_state_text(state, limit=MAX_TEXT_CHARS):
    """Compact JSON of a state object, trimmed to `limit` characters.

    The actors list goes first (nearest kept); as a last resort the text is cut.
    Returns (text, note or None).
    """
    text = _dumps(state)
    if len(text) <= limit:
        return text, None
    note = None
    actors = state.get("actors") if isinstance(state, dict) else None
    if isinstance(actors, list) and actors:
        keep = len(actors)
        trimmed = state
        while keep > 0 and len(text) > limit:
            keep = keep // 2
            trimmed = dict(state)
            trimmed["actors"] = actors[:keep]
            text = _dumps(trimmed)
        note = (f"actors trimmed to the nearest {keep} of {len(actors)} (ask for fewer sections "
                "or a smaller radius)")
    if len(text) > limit:
        text = text[:limit]
        note = ((note + "; ") if note else "") + (
            f"output cut at {limit} characters: ask for fewer sections or a smaller radius")
    return text, note


def _b64_len(n_bytes):
    return 4 * ((n_bytes + 2) // 3)


def image_for_mcp(path, max_b64_chars=MAX_IMAGE_B64_CHARS):
    """(png_or_jpeg_bytes, image format, description) of a screenshot that fits the budget.

    The PNG is sent as is when it fits; otherwise a downscaled JPEG (Pillow) is sent and the
    full-resolution PNG stays on disk.  Returns (None, None, why) when no image can be sent.
    """
    with open(path, "rb") as f:
        raw = f.read()
    try:
        from PIL import Image as PILImage
    except ImportError:
        PILImage = None
    size = None
    if PILImage is not None:
        try:
            with PILImage.open(io.BytesIO(raw)) as probe:
                size = probe.size
        except Exception:
            size = None
    if _b64_len(len(raw)) <= max_b64_chars:
        dims = f"{size[0]}x{size[1]} " if size else ""
        return raw, "png", f"PNG {dims}({len(raw)} bytes, full resolution)"
    if PILImage is None:
        return None, None, (f"the PNG is {len(raw)} bytes, over the inline budget, and Pillow is "
                            f"not installed to shrink it ({INSTALL_HINT})")
    try:
        img = PILImage.open(io.BytesIO(raw))
        img.load()
        img = img.convert("RGB")
    except Exception as e:
        return None, None, f"cannot decode {path}: {e}"
    data = None
    desc = None
    for width, quality in JPEG_LADDER:
        im = img
        if img.width > width:
            im = img.resize((width, max(1, round(img.height * width / img.width))),
                            PILImage.LANCZOS)
        buf = io.BytesIO()
        im.save(buf, "JPEG", quality=quality, optimize=True)
        data = buf.getvalue()
        desc = (f"JPEG {im.width}x{im.height} q{quality} ({len(data)} bytes), a downscaled "
                f"preview of the {img.width}x{img.height} PNG")
        if _b64_len(len(data)) <= max_b64_chars:
            return data, "jpeg", desc
    return None, None, (f"even the smallest preview ({desc}) is over the {max_b64_chars}-character "
                        "inline budget")


# ---------------------------------------------------------------------------
# The session: at most one game, one lock
# ---------------------------------------------------------------------------
class PlaySession:
    """The game this server drives.  Thread-safe: every tool runs under self.lock.

    attach_port/attach_token: diagnostic mode, connect (lazily, on the first tool call) to a
    bridge that is already running instead of launching one.
    launch_overrides: extra Bridge.launch keyword arguments (tests: exe_argv, skip_freshness,
    run_dir).
    """

    def __init__(self, *, attach_port=None, attach_token=None, launch_overrides=None,
                 max_image_chars=MAX_IMAGE_B64_CHARS, max_text_chars=MAX_TEXT_CHARS):
        self.lock = threading.Lock()
        self.bridge = None
        self.attached = False
        self.attach_port = int(attach_port) if attach_port else None
        self.attach_token = attach_token
        self.launch_overrides = dict(launch_overrides or {})
        self.max_image_chars = int(max_image_chars)
        self.max_text_chars = int(max_text_chars)
        self.closed = False
        self.last_run_dir = None

    # --- banner ---------------------------------------------------------------------
    def banner(self):
        b = self.bridge
        if b is None:
            return None
        if self.attached:
            return ATTACHED_BANNER
        if b.stale:
            return f"{STALE_BANNER} ({_short_reasons((b.fresh or {}).get('reasons'))})"
        if _dirty_build(b):
            return f"{DIRTY_BANNER} (the package was built from {_dirty_note(b)})"
        return None

    def text(self, body, note=None, banner_last=False):
        """body with the evidence banner (first, or last for error messages)."""
        banner = self.banner()
        parts = [body]
        if note:
            parts.append(f"(note: {note})")
        if banner:
            if banner_last:
                parts.append("!! " + banner)
            else:
                parts.insert(0, "!! " + banner)
        return "\n".join(parts)

    # --- bridge lifecycle -------------------------------------------------------------
    def _drop(self, why):
        """Forget the current game; a launched one is killed (process tree + job)."""
        b, self.bridge = self.bridge, None
        attached, self.attached = self.attached, False
        if b is None:
            return
        log(f"{'closing the connection to the attached game' if attached else 'stopping the game'}"
            f": {why}")
        try:
            if not attached and b.job is not None:
                b.job.terminate()
            b.close()
        except Exception as e:
            log(f"WARNING: cleanup failed: {e}")

    def _attach(self):
        try:
            self.bridge = ab.Bridge.attach(self.attach_port, token=self.attach_token,
                                           wait_ready=False, timeout=10.0)
        except ab.BridgeError as e:
            raise ToolError(f"cannot attach to the bridge on 127.0.0.1:{self.attach_port}: {e}")
        self.attached = True
        log(f"attached to 127.0.0.1:{self.attach_port} (pid {self.bridge.pid})")

    def _require(self):
        if self.closed:
            raise ToolError("the server is shutting down")
        b = self.bridge
        if b is None:
            if self.attach_port is None:
                raise ToolError(NO_GAME)
            self._attach()
            return self.bridge
        if not self.attached and not b.is_alive():
            crash = b.crash_lines()
            code = b.proc.returncode if b.proc is not None else None
            self.last_run_dir = b.run_dir
            self._drop("the game exited")
            raise ToolError(f"the game is no longer running (exit code {code}"
                            + (f"; crash: {crash[0]}" if crash else "")
                            + f"; run dir {self.last_run_dir}): call launch_game to start a new one")
        if b.conn is None or b.conn.closed:
            try:
                b.reconnect()
                log("reconnected to the bridge")
            except ab.BridgeError as e:
                if self.attached:
                    self._drop(f"reconnect failed: {e}")
                raise ToolError(f"lost the bridge connection and could not reconnect: {e}")
        return b

    def _error_text(self, tool, e):
        if e.code == "bad_key":
            what = "refused (unknown key, wrong kind of key, or a denied debug key)"
        elif e.kind == "gameplay":
            what = "GAMEPLAY failure (the game did not do what a player would expect)"
        else:
            what = "INFRA failure (bridge, process or connection)"
        msg = f"{tool}: {what} [{e.code}]: {e}"
        ctx = e.response
        if isinstance(ctx, dict):
            if "values" in ctx:
                ctx = {"values": ctx.get("values"), "elapsed_s": ctx.get("elapsed_s")}
            elif "state" in ctx and isinstance(ctx.get("state"), dict):
                ctx = {"state_sections": sorted(ctx["state"])}
            text = _dumps(ctx)
            if len(text) > ERROR_CONTEXT_CHARS:
                text = text[:ERROR_CONTEXT_CHARS] + "..."
            msg += f"\ncontext: {text}"
        return self.text(msg, banner_last=True)

    def run(self, tool, args, fn):
        """Run fn(bridge) under the lock; BridgeError and bad arguments become ToolError."""
        with self.lock:
            b = self._require()
            b._note("mcp_tool", {"tool": tool, "args": args})
            try:
                return fn(b)
            except ab.BridgeError as e:
                message = self._error_text(tool, e)
                if e.code in CONNECTION_LOST and not b.is_alive() and not self.attached:
                    self.last_run_dir = b.run_dir
                    self._drop(f"{tool}: {e}")
                    message += "\nThe game is gone: call launch_game to start a new one."
                raise ToolError(message)
            except (ValueError, TypeError) as e:
                raise ToolError(self.text(f"{tool}: bad arguments: {e}", banner_last=True))
            except ToolError:
                raise
            except Exception as e:  # the SDK would hide it: say what happened
                traceback.print_exc(file=sys.stderr)
                raise ToolError(self.text(f"{tool}: internal error: {type(e).__name__}: {e}",
                                          banner_last=True))

    # --- launch / quit ----------------------------------------------------------------
    def launch(self, map=None, frontend=False, allow_stale=False, expect_experience=None):
        with self.lock:
            if self.closed:
                raise ToolError("the server is shutting down")
            if self.attach_port is not None:
                raise ToolError("this server was started with --attach: it drives an existing "
                                "bridge and does not launch games")
            if self.bridge is not None:
                if self.bridge.is_alive():
                    raise ToolError(f"a game is already running (pid {self.bridge.pid}, port "
                                    f"{self.bridge.port}): call quit_game first (one game at a "
                                    "time)")
                self._drop("the previous game is no longer running")
            kwargs = dict(map=map or ab.DEFAULT_MAP, frontend=bool(frontend),
                          allow_stale=bool(allow_stale), expect_experience=expect_experience,
                          detach=False, run_suffix=RUN_SUFFIX)
            kwargs.update(self.launch_overrides)
            try:
                b = ab.Bridge.launch(**kwargs)
            except ab.BridgeError as e:
                what = "GAMEPLAY failure" if e.kind == "gameplay" else "INFRA failure"
                raise ToolError(f"launch_game: {what} [{e.code}]: {e}")
            except Exception as e:
                traceback.print_exc(file=sys.stderr)
                raise ToolError(f"launch_game: internal error: {type(e).__name__}: {e}")
            self.bridge = b
            self.attached = False
            self.last_run_dir = b.run_dir
            b._note("mcp_tool", {"tool": "launch_game",
                                 "args": {"map": map, "frontend": frontend,
                                          "allow_stale": allow_stale,
                                          "expect_experience": expect_experience}})
            return self.text(self._launch_summary(b))

    def _launch_summary(self, b):
        info = b.info()
        ping = b.ping_info or {}
        build = info.get("build") or {}
        if b.stale:
            freshness = "STALE (allowed: diagnostic only) -- " + _short_reasons(
                info.get("stale_reasons"), limit=4, chars=600)
        elif _dirty_build(b):
            freshness = (f"FRESH but built from a DIRTY tree (diagnostic only) -- the package "
                         f"matches HEAD and the working tree, but it was built from "
                         f"{_dirty_note(b)}; for evidence commit, re-package "
                         "(py Scripts/package_playtest.py) and relaunch")
        elif (b.fresh or {}).get("fresh"):
            freshness = "FRESH (the package matches HEAD and the working tree)"
        else:
            freshness = "not checked"
        lines = [
            f"Game ready: pid {b.pid}, bridge {b.host}:{b.port}, map {info.get('map') or '(front end)'}, "
            f"experience {ping.get('experience')} (expected {info.get('expected_experience')}).",
            f"Build: commit {pb.short(build.get('commit'))} -- {freshness}",
            f"Run dir: {b.run_dir} (game.log, bridge.jsonl, bridge_NN.png)",
            "The game runs in a kill-on-close job: it dies with this server."
            if b.job is not None else "WARNING: no kill-on-close job; quit_game before you stop.",
        ]
        for w in info.get("warnings") or []:
            lines.append(f"Warning: {w}")
        lines.append("ping: " + _dumps(ping))
        lines.append(RULE)
        return "\n".join(lines)

    def quit(self):
        with self.lock:
            b = self.bridge
            if b is None:
                if self.attach_port is None:
                    raise ToolError(NO_GAME)
                self._attach()
                b = self.bridge
            banner = self.banner()
            b._note("mcp_tool", {"tool": "quit_game", "args": {}})
            try:
                result = b.quit()
            except Exception as e:  # quit() handles bridge errors itself; this is a backstop
                traceback.print_exc(file=sys.stderr)
                self._drop(f"quit failed: {e}")
                raise ToolError(f"quit_game: {type(e).__name__}: {e} (the game was killed)")
            run_dir = b.run_dir
            attached = self.attached
            self.bridge, self.attached = None, False
            try:
                b.close()
            except Exception:
                pass
            self.last_run_dir = run_dir
            lines = [f"Game quit: exit code {result.get('exit_code')}"
                     + (f"; {result['shutdown']}" if result.get("shutdown") else "")]
            if attached:
                lines.append("(attached game: this server did not launch it)")
            if run_dir and os.path.isdir(run_dir):
                shots = sorted(n for n in os.listdir(run_dir)
                               if n.lower().endswith(".png"))
                lines.append(f"Run dir: {run_dir}")
                lines.append("Screenshots: " + (", ".join(shots) if shots else "none"))
                game_log = os.path.join(run_dir, "game.log")
                if os.path.isfile(game_log):
                    s = pb.summarize_log(game_log)
                    missing = pb.find_missing_content(game_log)
                    lines.append(f"game.log: {s['lines']} lines, {s['errors']} errors, "
                                 f"{s['warnings']} warnings; missing /MordecaiCore content: "
                                 + (", ".join(missing[:8]) if missing else "none"))
                    for e in s["top_errors"][:5]:
                        lines.append(f"  error x{e['count']}: {e['line'][:200]}")
            body = "\n".join(lines)
            return ("!! " + banner + "\n" + body) if banner else body

    def shutdown(self):
        """Server exit: kill a launched game (an attached one only loses its connection)."""
        if self.closed:
            return
        got = self.lock.acquire(timeout=2.0)
        try:
            self.closed = True
            if self.bridge is not None:
                self._drop("server shutdown")
        finally:
            if got:
                self.lock.release()


# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------
def _desc(text):
    return text.strip() + "\n\n" + RULE


def build_server(session=None, *, log_level="WARNING"):
    """An MCPServer with the mordecai-play tools bound to `session` (a PlaySession)."""
    if MCPServer is None:
        raise RuntimeError(f"the mcp package is not installed ({MCP_IMPORT_ERROR}): {INSTALL_HINT}")
    session = session or PlaySession()
    mcp = MCPServer(SERVER_NAME, version=SERVER_VERSION, instructions=INSTRUCTIONS,
                    log_level=log_level)
    mcp.play_session = session

    @mcp.tool(name="launch_game", structured_output=False, description=_desc("""
Launch the packaged game (PackagedBuilds/Playtest) with the agent bridge and wait until it is
ready (experience loaded, loading screen gone, pawn spawned). One game at a time: call quit_game
first to start another. The game runs in a kill-on-close job and dies with this server.
- map: map to load (default /MordecaiCore/Game/Maps/DevTestMap, the arena)
- frontend: true boots the front end (no map) and expects B_LyraFrontEnd_Experience
- allow_stale: run a STALE package anyway (the package does not match the code under test).
  Diagnostic only: every later result is bannered 'STALE BUILD — not evidence'.
- expect_experience: experience the map must load (default B_MordecaiArenaExperience)
A STALE build without allow_stale is refused: re-package with py Scripts/package_playtest.py.
A package built from a dirty (uncommitted) tree runs, but every result is bannered
'DIRTY BUILD — diagnostic only, not evidence'."""))
    def launch_game(map: str = ab.DEFAULT_MAP, frontend: bool = False, allow_stale: bool = False,
                    expect_experience: str | None = None) -> str:
        return session.launch(map=map, frontend=frontend, allow_stale=allow_stale,
                              expect_experience=expect_experience)

    @mcp.tool(name="get_state", structured_output=False, description=_desc("""
Read the game state as JSON (what the bridge measures; it never invents values: a section that
cannot be gathered has valid:false and a reason).
- sections: any of game, focus, player, input, pickups, inventory, pause_menu, widgets, actors
  (a list or a comma-separated string; default all). Ask for only what you need: actors is big.
- radius: radius in cm for actors (default 1500)
Units: cm, vectors [x,y,z], rotators [pitch,yaw,roll]; screen positions are viewport pixels."""))
    def get_state(sections: list[str] | str | None = None, radius: float = 1500.0) -> str:
        st = session.run("get_state", {"sections": sections, "radius": radius},
                         lambda b: b.state(sections, radius=radius))
        text, note = fit_state_text(st, session.max_text_chars)
        return session.text(text, note)

    @mcp.tool(name="press_key", structured_output=False, description=_desc("""
Tap a key like a player: down, hold hold_s seconds (default 0.1, at least 2 frames), up. The
result returns after the key is released.
- key: a UE FKey name (W, A, S, D, F, I, Escape, Q, E, R, One, Tab, BackSlash, SpaceBar, ...;
  aliases Esc, Space, Enter, Shift, Ctrl, Alt, 0-9), or a list of up to 4 keys pressed together.
Mouse buttons are not keys: use click. Debug keys with side effects are refused.
A key you are still holding with hold_key (no seconds) is refused ('already held by this
connection'): release it with release_key before you press it again."""))
    def press_key(key: str | list[str], hold_s: float = 0.1) -> str:
        resp = session.run("press_key", {"key": key, "hold_s": hold_s},
                           lambda b: b.tap(key, hold_s=hold_s))
        return session.text(_dumps(resp))

    @mcp.tool(name="hold_key", structured_output=False, description=_desc("""
Hold a key (or a list of up to 4 keys together).
- seconds given: hold that long, then release (the bridge times it; the result returns after the
  release, max 30 s).
- seconds omitted: key DOWN and keep it held until release_key, quit_game or the end of the
  session (use it to keep W held while you take a screenshot or read state).
A key held this way must be released with release_key before you press it again (press_key
or hold_key on it is refused as 'already held') and before walk_to (which refuses to start
while W/A/S/D is held: keys_held)."""))
    def hold_key(key: str | list[str], seconds: float | None = None) -> str:
        if seconds is not None and seconds > 0:
            resp = session.run("hold_key", {"key": key, "seconds": seconds},
                               lambda b: b.hold(key, seconds))
        else:
            resp = session.run("hold_key", {"key": key, "seconds": None},
                               lambda b: b.press(key))
        return session.text(_dumps(resp))

    @mcp.tool(name="release_key", structured_output=False, description=_desc("""
Release a key (or list of keys) held by hold_key without seconds. Releasing a key that is not
held succeeds with was_held false."""))
    def release_key(key: str | list[str]) -> str:
        resp = session.run("release_key", {"key": key}, lambda b: b.release(key))
        return session.text(_dumps(resp))

    @mcp.tool(name="walk_to", structured_output=False, description=_desc("""
Walk the player to world (x, y) with W/A/S/D pulses, closed loop: read player.location, tap the
keys toward the target, repeat until within tol cm. It asserts control yaw 0 (W=+X, D=+Y),
calibrates on the first pulse, side-steps when stuck and fails with 'stuck at (x, y)' after 3
side-steps, and aborts if the player dies or the pause menu opens. It refuses to start
(keys_held) while W/A/S/D is held: release_key any key you held with hold_key first.
- tol: stop distance in cm (default 50)
- timeout_s: give up after this long (default 3*distance/600 + 3 s)
Returns {final:[x,y], dist, pulses, side_steps}."""))
    def walk_to(x: float, y: float, tol: float = 50.0, timeout_s: float | None = None) -> str:
        result = session.run("walk_to", {"x": x, "y": y, "tol": tol, "timeout_s": timeout_s},
                             lambda b: b.walk_to(x, y, tol=tol, timeout_s=timeout_s))
        return session.text(_dumps(result))

    @mcp.tool(name="click", structured_output=False, description=_desc("""
Click a mouse button at viewport pixel (x, y) (origin top-left of the game viewport, 1280x720,
the same space as the screenshots), or at the current virtual cursor when x and y are omitted.
Slate hit-tests the point, as for a player; the result's 'hit' is the type of the deepest widget
at that point (on a button usually its label, e.g. 'STextBlock'), not necessarily the button.
The click still goes to the button under it; judge the effect from get_state or a screenshot.
- button: Left (default), Right or Middle"""))
    def click(x: float | None = None, y: float | None = None, button: str = "Left") -> str:
        resp = session.run("click", {"x": x, "y": y, "button": button},
                           lambda b: b.click(x, y, button=button))
        return session.text(_dumps(resp))

    @mcp.tool(name="click_button", structured_output=False, description=_desc("""
Aim at a visible button and click its center, as a player aims with their eyes: the center comes
from state (pause_menu.inventory_tab rows, e.g. 'Identify', then widgets[].buttons); Slate still
decides what the click hits ('hit' is the deepest widget there, e.g. the button's text
'STextBlock', not necessarily the button). Give a label (exact, then case-insensitive) or a button_id (for an
inventory row, the item instance id). Fails if no such button is on screen."""))
    def click_button(label: str | None = None, button_id: str | None = None) -> str:
        if label is None and button_id is None:
            raise ToolError(session.text("click_button: give a label or a button_id",
                                         banner_last=True))
        resp = session.run("click_button", {"label": label, "button_id": button_id},
                           lambda b: b.click_button(label=label, button_id=button_id))
        return session.text(_dumps(resp))

    @mcp.tool(name="screenshot", structured_output=False, description=_desc("""
Capture the game's own rendered frame (it works on a locked machine or behind other windows) and
return it as an image, plus its path. Large frames are sent as a downscaled JPEG preview; the
full-resolution PNG path is in the text (open it with the Read tool for detail). Look at the
image and describe what is on screen; do not assume."""))
    def screenshot() -> list:
        path = session.run("screenshot", {}, lambda b: b.screenshot())
        try:
            data, fmt, desc = image_for_mcp(path, session.max_image_chars)
        except OSError as e:
            raise ToolError(session.text(f"screenshot: cannot read {path}: {e}",
                                         banner_last=True))
        lines = [f"Screenshot: {path}"]
        if data is None:
            lines.append(f"No inline image: {desc}. Open the file with the Read tool.")
            return [session.text("\n".join(lines))]
        lines.append(f"Inline image: {desc}.")
        lines.append("Describe what is actually on screen before you rely on it.")
        return [session.text("\n".join(lines)), Image(data=data, format=fmt)]

    @mcp.tool(name="wait_until", structured_output=False, description=_desc("""
Wait (in the bridge, real time) until a condition on the state is true, or fail after timeout_s
(default 10, max 600) with the last value of every path in the condition.
Grammar: paths into the state (pickups.focused.label, inventory.items[0].identified),
len(path), literals (numbers, "strings", true, false, null), == != < <= > >=, && || ! and
parentheses. A path that does not resolve is null.
Example: pause_menu.open==true && pause_menu.active_tab=="inventory" && len(pause_menu.inventory_tab.rows)==4"""))
    def wait_until(condition: str, timeout_s: float = 10.0) -> str:
        resp = session.run("wait_until", {"condition": condition, "timeout_s": timeout_s},
                           lambda b: b.wait_until(condition, timeout_s=timeout_s))
        return session.text(_dumps(resp))

    @mcp.tool(name="quit_game", structured_output=False, description=_desc("""
Quit the game (the bridge releases all held input and exits the engine), wait for the process to
exit (killing it if it does not), and summarize the run: run dir, screenshots, game.log errors and
any /MordecaiCore content missing from the build."""))
    def quit_game() -> str:
        return session.quit()

    return mcp


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        prog="mordecai_play_mcp.py",
        description="stdio MCP server: play the packaged Mordecai build with real input.")
    parser.add_argument("--attach", "--attach-port", dest="attach", type=int, default=None,
                        metavar="PORT",
                        help="diagnostic: drive the bridge already listening on this port")
    parser.add_argument("--token", default=None, help="expected ping token (with --attach)")
    parser.add_argument("--image-max-chars", type=int,
                        default=int(os.environ.get("MORDECAI_PLAY_IMAGE_MAX_CHARS",
                                                   MAX_IMAGE_B64_CHARS)),
                        help=f"base64 budget of the screenshot image ({MAX_IMAGE_B64_CHARS})")
    parser.add_argument("--log-level", default="WARNING",
                        choices=("DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"))
    # Test-only (tests/fake_game.py stands in for the packaged game).
    parser.add_argument("--test-exe-argv", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-skip-freshness", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--test-run-dir", default=None, help=argparse.SUPPRESS)
    return parser.parse_args(argv)


def main(argv=None):
    if MCP_IMPORT_ERROR is not None:
        print(f"[Play-MCP] ERROR: the mcp package is not installed ({MCP_IMPORT_ERROR}).\n"
              f"[Play-MCP] Install it once: {INSTALL_HINT}", file=sys.stderr, flush=True)
        return 2
    try:
        args = parse_args(argv)
    except SystemExit as e:
        return 2 if e.code else 0
    # stdout is the protocol channel: every log line goes to stderr.
    ab.set_log_stream(sys.stderr)
    pb.set_log_stream(sys.stderr)
    overrides = {}
    if args.test_exe_argv:
        overrides["exe_argv"] = json.loads(args.test_exe_argv)
    if args.test_skip_freshness:
        overrides["skip_freshness"] = True
    if args.test_run_dir:
        overrides["run_dir"] = args.test_run_dir
    session = PlaySession(attach_port=args.attach, attach_token=args.token,
                          launch_overrides=overrides, max_image_chars=args.image_max_chars)
    server = build_server(session, log_level=args.log_level)
    atexit.register(session.shutdown)
    log(f"serving on stdio (pid {os.getpid()}" + (f", attach port {args.attach}" if args.attach
                                                  else "") + ")")
    try:
        server.run("stdio")
    except KeyboardInterrupt:
        pass
    except Exception:
        traceback.print_exc(file=sys.stderr)
    finally:
        session.shutdown()
        log("stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
