#!/usr/bin/env python3
"""Fake Packaged Game -- a stand-in for LyraGame.exe, for offline launch tests

Run it with the Python interpreter, the way a test launches the real game:
  [sys.executable, "Scripts/tests/fake_game.py", "/MordecaiCore/Game/Maps/DevTestMap",
   "-windowed", "-log", "-abslog=C:/run/game.log",
   "-AgentBridge", "-AgentBridgePort=50123", "-AgentBridgeToken=abc", "-AgentBridgeShotDir=C:/run"]

It writes the Lyra log lines that playtest_boot.py waits for to the -abslog= file (the same
text as a real packaged run: 'Identified experience LyraExperienceDefinition:<exp> (Source: ...)'
matches RE_IDENTIFIED and 'EXPERIENCE: OnExperienceLoadComplete(CurrentExperience = ...'
matches RE_LOAD_COMPLETE).  With -AgentBridge it serves tests/fake_bridge.py on
127.0.0.1:<-AgentBridgePort=> (default 47777) and logs the real bridge's listen line.  It exits
0 after a bridge quit command, or when -FakeLifetime= runs out (so an orphan cleans itself up).

Agent_bridge.Bridge.launch(exe_argv=[sys.executable, fake_game.py], skip_freshness=True) and
playtest_boot.main() (through a .bat wrapper) drive it end to end.

Fake-only switches (UE style, case-insensitive):
  -FakeExperience=<name>   experience to identify and load (default B_MordecaiArenaExperience;
                           B_LyraFrontEnd_Experience when no map is given)
  -FakeBootDelay=<s>       seconds before the experience is identified (0.2)
  -FakeLoadDelay=<s>       seconds from identify to OnExperienceLoadComplete (0.1)
  -FakeLoadingS=<s>        loading screen stays up this long after load (0.3): ping.ready false
  -FakeCrash               log a fatal error after identify and exit 3
  -FakeNoExperience        log 'Failed to identify experience' and keep running
  -FakeExitAfterLoad=<n>   exit with code n right after the experience loads
  -FakeBindFail            log the bridge's bind failure line and serve nothing
  -FakeToken=<s>           answer ping with this token instead of -AgentBridgeToken
  -FakeLifetime=<s>        exit after this many seconds (600)
  -FakeWorldJson=<file>    FakeWorld keyword options (JSON object)
  -FakeFaultsJson=<file>   Faults keyword options (JSON object)

Exit codes: 0 -- quit or lifetime over;  2 -- bad arguments;  3 -- -FakeCrash;  n -- -FakeExitAfterLoad
"""

import datetime
import json
import os
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import fake_bridge  # noqa: E402

DEFAULT_EXPERIENCE = "B_MordecaiArenaExperience"
FRONTEND_EXPERIENCE = "B_LyraFrontEnd_Experience"


def parse_ue_args(argv):
    """UE-style tokens -> (positional list, {lower-case name: value or True})."""
    positional, options = [], {}
    for tok in argv:
        if tok.startswith("-") and len(tok) > 1:
            name, sep, value = tok[1:].partition("=")
            options[name.lower()] = value if sep else True
        else:
            positional.append(tok)
    return positional, options


class GameLog:
    """Writes game.log the way UE does: '[2026.10.20-14.00.00:123][  0]LogCategory: text'."""

    def __init__(self, path):
        self.path = path
        self.lock = threading.Lock()
        self.frame = 0
        self.fh = None
        if path:
            os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
            self.fh = open(path, "w", encoding="utf-8", newline="\n")
            self.fh.write("\ufeffLog file open, "
                          + datetime.datetime.now().strftime("%m/%d/%y %H:%M:%S") + "\n")
            self.fh.flush()

    def raw(self, text):
        with self.lock:
            if self.fh:
                self.fh.write(text + "\n")
                self.fh.flush()

    def line(self, text):
        now = datetime.datetime.now(datetime.timezone.utc)
        stamp = now.strftime("%Y.%m.%d-%H.%M.%S") + f":{now.microsecond // 1000:03d}"
        self.raw(f"[{stamp}][{self.frame:3d}]{text}")
        self.frame += 1

    def close(self):
        with self.lock:
            if self.fh:
                self.fh.close()
                self.fh = None


def _load_json(path):
    if not path or path is True:
        return None
    with open(path, "r", encoding="utf-8-sig") as f:
        return json.load(f)


def _float(options, name, default):
    v = options.get(name)
    if v is None or v is True:
        return default
    return float(v)


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    positional, opt = parse_ue_args(argv)
    map_arg = positional[0] if positional else None
    experience = opt.get("fakeexperience") if isinstance(opt.get("fakeexperience"), str) else (
        DEFAULT_EXPERIENCE if map_arg else FRONTEND_EXPERIENCE)
    boot_delay = _float(opt, "fakebootdelay", 0.2)
    load_delay = _float(opt, "fakeloaddelay", 0.1)
    loading_s = _float(opt, "fakeloadings", 0.3)
    lifetime = _float(opt, "fakelifetime", 600.0)
    started = time.monotonic()

    log = GameLog(opt.get("abslog") if isinstance(opt.get("abslog"), str) else None)
    log.raw("LogInit: Command Line:  " + " ".join(argv))
    log.line("LogInit: Display: Engine is initialized. Leaving FEngineLoop::Init()")

    try:
        world_options = dict(_load_json(opt.get("fakeworldjson")) or {})
        faults = fake_bridge.Faults.from_options(_load_json(opt.get("fakefaultsjson")))
    except (OSError, ValueError, TypeError) as e:
        log.line(f"LogFakeGame: Error: bad fake options: {e}")
        log.close()
        return 2
    world_options.setdefault("boot_s", boot_delay + load_delay)
    world_options.setdefault("loading_s", loading_s)
    world_options.setdefault("experience", experience)
    if map_arg:
        world_options.setdefault("map", map_arg)
    world = fake_bridge.FakeWorld.from_options(world_options)

    quit_event = threading.Event()
    server = None
    if opt.get("agentbridge") is True:
        port = int(_float(opt, "agentbridgeport", fake_bridge.DEFAULT_PORT))
        token = opt.get("faketoken") if isinstance(opt.get("faketoken"), str) else (
            opt.get("agentbridgetoken") if isinstance(opt.get("agentbridgetoken"), str) else "")
        shot_dir = opt.get("agentbridgeshotdir") if isinstance(opt.get("agentbridgeshotdir"),
                                                               str) else None
        if opt.get("fakebindfail"):
            log.line(f"LogMordecaiAgentBridge: Error: AgentBridge: failed to listen on "
                     f"127.0.0.1:{port}")
        else:
            server = fake_bridge.FakeBridgeServer(world, port=port, token=token,
                                                  pid=os.getpid(), shot_dir=shot_dir,
                                                  faults=faults, on_quit=quit_event.set)
            try:
                server.start()
                log.line(f"LogMordecaiAgentBridge: Display: AgentBridge: listening on "
                         f"127.0.0.1:{server.port}")
            except OSError:
                server = None
                log.line(f"LogMordecaiAgentBridge: Error: AgentBridge: failed to listen on "
                         f"127.0.0.1:{port}")

    def sleep_until(t):
        while time.monotonic() < t and not quit_event.is_set():
            time.sleep(0.01)

    sleep_until(started + boot_delay)
    source = "OptionsString" if map_arg else "WorldSettings"
    if opt.get("fakenoexperience"):
        log.line("LogLyraExperience: Error: Failed to identify experience, loading screen will "
                 "stay up forever")
    else:
        log.line(f"LogLyraExperience: Identified experience LyraExperienceDefinition:{experience} "
                 f"(Source: {source})")
        if opt.get("fakecrash"):
            log.line("LogWindows: Error: === Critical error: ===")
            log.line("LogWindows: Error: Fatal error: [File:FakeGame.cpp] [Line: 1] fake crash")
            log.close()
            if server:
                server.stop()
            return 3
        sleep_until(started + boot_delay + load_delay)
        log.line(f"LogLyraExperience: EXPERIENCE: OnExperienceLoadComplete(CurrentExperience = "
                 f"LyraExperienceDefinition:{experience}, Server)")
        exit_after = opt.get("fakeexitafterload")
        if exit_after is not None:
            log.line("LogExit: Exiting.")
            log.close()
            if server:
                server.stop()
            return int(exit_after) if isinstance(exit_after, str) else 1
        if loading_s > 0:
            sleep_until(started + boot_delay + load_delay + loading_s)
            log.line(f"LogLoadingScreen: LoadingScreen was visible for "
                     f"{boot_delay + load_delay + loading_s:.2f}s")

    while not quit_event.is_set() and time.monotonic() - started < lifetime:
        quit_event.wait(0.1)
    if quit_event.is_set():
        log.line("LogMordecaiAgentBridge: Display: AgentBridge: quit requested")
    log.line("LogExit: Exiting.")
    log.close()
    if server:
        server.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
