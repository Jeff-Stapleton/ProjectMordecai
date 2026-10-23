"""Tests for Scripts/explore_playtest.py (the exploratory playtest harness).

The agent is never claude here: --agent-cmd runs THIS file as a fake agent (fake_agent_main),
which reads the prompt from stdin like claude -p, writes a stream-json transcript to stdout and
writes findings.jsonl / bridge.jsonl into the run dir, or drives the real mordecai-play MCP server
over stdio against tests/fake_bridge.py or tests/fake_game.py.  Detectors are tested on synthetic
run dirs.  Nothing here launches the real game or calls the Claude API.
Run: py Scripts/run_python_tests.py -k Explore
"""

import contextlib
import importlib.util
import io
import json
import os
import sys
import tempfile
import time
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_ROOT = os.path.dirname(SCRIPTS)
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import explore_playtest as ep  # noqa: E402
import playtest_boot as pb  # noqa: E402

THIS = os.path.abspath(__file__)
FAKE_GAME = os.path.join(SCRIPTS, "tests", "fake_game.py")
HAVE_MCP = importlib.util.find_spec("mcp") is not None
HAVE_PIL = importlib.util.find_spec("PIL") is not None
TOKEN = "explore-token"
KNOWN_TEXT = "Pickup name labels render rotated 90 degrees (hard to read)."


# ---------------------------------------------------------------------------
# The fake agent (runs as a subprocess: py test_explore_playtest.py --fake-agent MODE <claude args>)
# ---------------------------------------------------------------------------
def _emit(obj):
    sys.stdout.write(json.dumps(obj) + "\n")
    sys.stdout.flush()


def _tool_use(name, args=None):
    _emit({"type": "assistant", "message": {"content": [
        {"type": "tool_use", "id": f"tu_{name}", "name": ep.TOOL_PREFIX + name,
         "input": args or {}}]}})


def _write_jsonl(path, entries):
    with open(path, "a", encoding="utf-8", newline="\n") as f:
        for e in entries:
            f.write(json.dumps(e) + "\n")


def _fake_bridge_transcript(run_dir):
    """What the bridge client writes when the MCP server launches a game and reads its state."""
    t = "2026-10-22T20:00:00.000"
    _write_jsonl(os.path.join(run_dir, ep.TRANSCRIPT_NAME), [
        {"t": t, "pid": 1, "conn": None, "dir": "note",
         "msg": {"note": "launch", "command_line": "LyraGame.exe ..."}},
        {"t": t, "pid": 1, "conn": None, "dir": "note",
         "msg": {"note": "mcp_tool", "tool": "launch_game", "args": {}}},
        {"t": t, "pid": 1, "conn": None, "dir": "note",
         "msg": {"note": "mcp_tool", "tool": "get_state", "args": {}}},
        {"t": t, "pid": 1, "conn": 1, "dir": "send", "msg": {"id": 1, "cmd": "state"}},
        {"t": t, "pid": 1, "conn": 1, "dir": "recv", "msg": {"id": 1, "ok": True, "state": {}}},
    ])


def _finding(fid, severity, title, known=None):
    return {"id": fid, "time": "2026-10-22T20:01:00", "severity": severity, "title": title,
            "details": f"details of {title}", "repro": "press_key I", "expected": "something",
            "known_issue": known, "screenshot": None, "player_location": [0, 0, 92],
            "map": "/MordecaiCore/Game/Maps/DevTestMap", "build_commit": None, "source": "agent"}


FINDING_SETS = {
    "clean": [("cosmetic", "Pickup labels rotated", KNOWN_TEXT), ("minor", "HUD text clipped", None)],
    "major": [("major", "Tab does nothing", None)],
    "known_major": [("major", "Sprint is not bound", "Not bound: sprint, jump, attacks")],
    "blocker_known": [("blocker", "Menu cannot be closed", "I does not close the pause menu")],
}


def _drive(server, run_dir, mode):
    """Play through the real MCP server over stdio, as claude would."""
    import asyncio
    from mcp import Client, StdioServerParameters
    params = StdioServerParameters(command=server["command"], args=server["args"],
                                   env=dict(os.environ, **server.get("env", {})))
    calls = [("session_info", {}), ("get_state", {"sections": ["player"]}), ("screenshot", {}),
             ("press_key", {"key": "Tab"}),
             ("report_finding", {"severity": "minor", "title": "Weapon text flickers",
                                 "details": "after Tab"}),
             ("quit_game", {})]
    if mode == "drive_launch":
        calls.insert(0, ("launch_game", {}))

    async def go():
        async with Client(params) as client:
            tools = sorted(t.name for t in (await client.list_tools()).tools)
            _emit({"type": "system", "subtype": "init",
                   "tools": [ep.TOOL_PREFIX + t for t in tools],
                   "mcp_servers": [{"name": ep.SERVER_NAME, "status": "connected"}]})
            for name, args in calls:
                _tool_use(name, args)
                res = await client.call_tool(name, args)
                if res.is_error:
                    sys.stderr.write(f"fake-agent: {name} failed: {res.content}\n")
    asyncio.run(go())


def fake_agent_main(argv):
    mode = argv[argv.index("--fake-agent") + 1]
    config_path = argv[argv.index("--mcp-config") + 1]
    with open(config_path, encoding="utf-8") as f:
        server = json.load(f)["mcpServers"][ep.SERVER_NAME]
    run_dir = server["args"][server["args"].index("--run-dir") + 1]
    prompt = sys.stdin.buffer.read().decode("utf-8")
    sys.stderr.write(json.dumps({"cwd": os.getcwd(), "prompt_chars": len(prompt),
                                 "argv": argv}) + "\n")
    sys.stderr.flush()
    if mode == "crash":
        sys.stderr.write("Traceback (most recent call last):\nRuntimeError: fake agent crashed\n")
        return 3
    if mode == "hang":
        time.sleep(120)
        return 0
    if mode in ("drive", "drive_launch"):
        _drive(server, run_dir, mode)
        _emit({"type": "result", "subtype": "success", "is_error": False, "num_turns": 7,
               "result": "Drove the fake game.", "total_cost_usd": 0.01})
        return 0
    _emit({"type": "system", "subtype": "init",
           "tools": [ep.TOOL_PREFIX + "get_state", ep.TOOL_PREFIX + "screenshot", "Read"],
           "mcp_servers": [{"name": ep.SERVER_NAME, "status": "connected"}]})
    _tool_use("launch_game")
    _tool_use("get_state", {"sections": ["player"]})
    _tool_use("get_state")
    _tool_use("screenshot")
    if mode == "garbage":
        sys.stdout.write("this is not json\n[1, 2, 3]\n\n")
        _emit({"type": "assistant", "message": "not a dict"})
        _emit({"type": "result", "total_cost_usd": "lots", "num_turns": None})
    else:
        _emit({"type": "assistant", "message": {"content": [
            {"type": "text", "text": "Final summary: covered movement and menus."}]}})
        _emit({"type": "result", "subtype": "success", "is_error": False, "num_turns": 12,
               "result": "Covered movement, spells and the pause menu. F01, F02 filed.",
               "total_cost_usd": 0.4242, "duration_ms": 61000})
    if mode == "noplay":
        return 0
    _fake_bridge_transcript(run_dir)
    for i, (sev, title, known) in enumerate(FINDING_SETS.get(mode, []), 1):
        _write_jsonl(os.path.join(run_dir, ep.FINDINGS_NAME),
                     [_finding(f"F{i:02d}", sev, title, known)])
    return 0


def agent_cmd(mode):
    return json.dumps([sys.executable, "-X", "utf8", "-B", THIS, "--fake-agent", mode])


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
class TempCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.playtests = os.path.join(self.tmp, "Playtests")
        os.makedirs(self.playtests)
        patcher = mock.patch.object(pb, "PLAYTESTS_DIR", self.playtests)
        patcher.start()
        self.addCleanup(patcher.stop)

    def harness(self, *argv):
        """(exit code, stdout) of an in-process run."""
        out = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(io.StringIO()):
            code = ep.main(list(argv))
        return code, out.getvalue()

    def run_fake(self, mode, *extra):
        run_dir = os.path.join(self.tmp, "run dir")
        code, out = self.harness("--agent-cmd", agent_cmd(mode), "--test-skip-freshness",
                                 "--run-dir", run_dir, *extra)
        self.run_dir = run_dir
        with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as f:
            self.report = json.load(f)
        with open(os.path.join(run_dir, "report.md"), encoding="utf-8") as f:
            self.md = f.read()
        return code, out

    def agent_stderr_info(self):
        with open(os.path.join(self.run_dir, ep.STDERR_NAME), encoding="utf-8") as f:
            return json.loads(f.readline())


def stale_fresh():
    return {"fresh": False, "reasons": ["build commit aaaaaaa != HEAD bbbbbbb"], "warnings": [],
            "git_error": None, "build": {"commit": "aaaaaaa1111", "dirty": False},
            "head": "bbbbbbb2222", "branch": "feature/x"}


# ---------------------------------------------------------------------------
# Prompt, config, command
# ---------------------------------------------------------------------------
class TestPromptAndCommand(TempCase):
    def test_sections_from_the_real_docs(self):
        known, why = ep.extract_section(ep.PLAN_DOC, ep.KNOWN_ISSUES_HEADING)
        self.assertIsNone(why)
        self.assertTrue(known.startswith("## Known issues backlog"))
        self.assertIn("Found by Phase 2", known)
        self.assertNotIn("## Open decisions", known)
        bindings, why = ep.extract_section(ep.BRIDGE_DOC, ep.BINDINGS_HEADING)
        self.assertIsNone(why)
        self.assertIn("BackSlash", bindings)
        self.assertIn("Spell slots", bindings)
        self.assertNotIn("## 6.", bindings)
        self.assertIsNone(ep.extract_section(ep.PLAN_DOC, "No such heading")[0])
        self.assertIn("cannot read", ep.extract_section(os.path.join(self.tmp, "x.md"), "a")[1])

    def test_build_prompt(self):
        p = ep.build_prompt("CHARTER TEXT", 20, r"C:\runs\a b", focus="spells", known_issues="KI",
                            bindings="BIND")
        for part in ("CHARTER TEXT", "Play-time budget: 20 minutes", r"Run dir: C:\runs\a b",
                     "FOCUS", "spells", "KI", "BIND", "Start by calling launch_game().",
                     "known_issue"):
            self.assertIn(part, p)
        p = ep.build_prompt("C", 5, "R", frontend=True, allow_stale=True, fresh=stale_fresh())
        self.assertIn("launch_game(frontend=true, allow_stale=true)", p)
        self.assertIn("packaged commit aaaaaaa111", p)
        self.assertIn("could not be read", p)
        p = ep.build_prompt("C", 5, "R", attach_port=4321)
        self.assertIn("do NOT call launch_game", p)
        self.assertIn("your first tool call", p)

    def test_mcp_config_has_absolute_paths_and_the_budget(self):
        run_dir = os.path.join(self.tmp, "r")
        cfg = ep.mcp_config(run_dir, 600)
        self.assertEqual(list(cfg["mcpServers"]), ["mordecai-play"])
        s = cfg["mcpServers"]["mordecai-play"]
        self.assertEqual(s["type"], "stdio")
        self.assertEqual(s["command"], sys.executable)
        self.assertTrue(os.path.isabs(s["command"]))
        self.assertEqual(s["args"], ["-X", "utf8", ep.MCP_SERVER, "--run-dir", run_dir,
                                     "--budget-s", "600"])
        self.assertTrue(os.path.isfile(ep.MCP_SERVER))
        self.assertEqual(s["env"], {"PYTHONUNBUFFERED": "1"})
        s = ep.mcp_config(run_dir, 90, attach_port=555, token="t")["mcpServers"]["mordecai-play"]
        self.assertEqual(s["args"][-4:], ["--attach", "555", "--token", "t"])

    def test_agent_command(self):
        cmd = ep.agent_command(["claude"], os.path.join(self.tmp, "c.json"), 10)
        self.assertEqual(cmd, ["claude", "-p", "--mcp-config", os.path.join(self.tmp, "c.json"),
                               "--strict-mcp-config", "--tools", "", "--allowedTools",
                               "mcp__mordecai-play,mcp__mordecai-play__*", "--permission-mode",
                               "dontAsk", "--output-format", "stream-json", "--verbose",
                               "--no-session-persistence", "--max-budget-usd", "10"])
        self.assertEqual(ep.agent_command(["c"], "x", 2.5, "opus")[-4:],
                         ["--max-budget-usd", "2.5", "--model", "opus"])

    def test_dry_run_prints_without_running(self):
        code, out = self.harness("--dry-run", "--focus", "spells and the pause menu",
                                 "--minutes", "3", "--agent-cmd", agent_cmd("hang"))
        self.assertEqual(code, 0)
        self.assertIn("DRY RUN", out)
        self.assertIn("--strict-mcp-config", out)
        self.assertIn("--permission-mode dontAsk", out)
        self.assertIn('--tools ""', out)
        self.assertIn('"--budget-s",\n"180"', out.replace(" ", ""))
        self.assertIn("## Known issues backlog", out)
        self.assertIn("spells and the pause menu", out)
        self.assertIn("Exploratory playtest charter", out)
        self.assertEqual(os.listdir(self.playtests), [])  # no run dir

    def test_bad_arguments_exit_2(self):
        self.assertEqual(self.harness("--minutes", "0")[0], 2)
        self.assertEqual(self.harness("--agent-cmd", "not json")[0], 2)
        self.assertEqual(self.harness("--agent-cmd", '"claude"')[0], 2)


# ---------------------------------------------------------------------------
# Refusals before any run dir
# ---------------------------------------------------------------------------
class TestRefusals(TempCase):
    def test_stale_build_is_refused_before_any_run_dir(self):
        with mock.patch.object(pb, "gate", return_value=(stale_fresh(), pb.EXIT_INFRA)) as gate:
            code, _ = self.harness("--agent-cmd", agent_cmd("clean"))
        self.assertEqual(code, 2)
        self.assertFalse(gate.call_args[0][1])
        self.assertEqual(os.listdir(self.playtests), [])

    def test_no_claude_cli_exit_2(self):
        fresh = dict(stale_fresh(), fresh=True, reasons=[])
        with mock.patch.object(pb, "gate", return_value=(fresh, None)), \
                mock.patch.object(ep.shutil, "which", return_value=None):
            code, _ = self.harness()
        self.assertEqual(code, 2)
        self.assertEqual(os.listdir(self.playtests), [])

    def test_allow_stale_runs_and_says_whose_commit_the_findings_are_against(self):
        with mock.patch.object(pb, "gate", return_value=(stale_fresh(), None)) as gate:
            code, _ = self.harness("--agent-cmd", agent_cmd("clean"), "--allow-stale")
        self.assertTrue(gate.call_args[0][1])
        self.assertEqual(code, 0)
        (run,) = os.listdir(self.playtests)
        self.assertTrue(run.endswith("-explore"))
        run_dir = os.path.join(self.playtests, run)
        with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as f:
            report = json.load(f)
        self.assertTrue(report["build"]["stale"])
        self.assertEqual(report["build"]["commit"], "aaaaaaa1111")
        self.assertTrue(any("STALE BUILD allowed" in d and "aaaaaaa111" in d
                            for d in report["diagnostic"]))
        with open(os.path.join(run_dir, "report.md"), encoding="utf-8") as f:
            self.assertIn("findings apply to the packaged commit aaaaaaa111", f.read())
        with open(os.path.join(run_dir, ep.PROMPT_NAME), encoding="utf-8") as f:
            self.assertIn("allow_stale=true", f.read())


# ---------------------------------------------------------------------------
# Runs with fake agents
# ---------------------------------------------------------------------------
class TestRuns(TempCase):
    def test_clean_run(self):
        code, out = self.run_fake("clean", "--minutes", "2", "--focus", "menus")
        self.assertEqual(code, 0, out)
        r = self.report
        self.assertEqual((r["result"], r["exit_code"], r["played"]), ("PASS", 0, True))
        self.assertEqual(r["evidence"], ep.EVIDENCE)
        self.assertIn("test agent command (--agent-cmd), not claude", r["diagnostic"])
        self.assertIn("staleness gate skipped (test run)", r["diagnostic"])
        self.assertEqual((r["minutes"], r["budget_s"], r["wall_clock_s"]), (2, 120, 420))
        a = r["agent"]
        self.assertEqual(a["exit_code"], 0)
        self.assertFalse(a["killed_by_wall_clock"])
        self.assertEqual((a["cost_usd"], a["num_turns"], a["is_error"]), (0.4242, 12, False))
        self.assertIn("F01, F02 filed", a["result_text"])
        self.assertEqual(a["mcp_servers"], [{"name": "mordecai-play", "status": "connected"}])
        self.assertEqual(a["mordecai_tools_seen"], [ep.TOOL_PREFIX + "get_state",
                                                    ep.TOOL_PREFIX + "screenshot"])
        self.assertEqual(a["tool_calls"][ep.TOOL_PREFIX + "get_state"], 2)
        self.assertEqual(a["server_tool_calls"], {"get_state": 1, "launch_game": 1})
        f = r["findings"]
        self.assertEqual([x["id"] for x in f["agent"]], ["F02", "F01"])  # minor before cosmetic
        self.assertEqual((f["new"], f["known"]), (1, 1))
        self.assertEqual(f["counts"], {"minor": 1, "cosmetic": 1})
        # The agent got the prompt on stdin, in a temp dir outside the repo, with claude's flags.
        info = self.agent_stderr_info()
        with open(os.path.join(self.run_dir, ep.PROMPT_NAME), encoding="utf-8") as fh:
            prompt = fh.read()
        self.assertEqual(info["prompt_chars"], len(prompt))
        self.assertIn("menus", prompt)
        self.assertNotIn(os.path.normcase(PROJECT_ROOT), os.path.normcase(info["cwd"]))
        self.assertFalse(os.path.exists(info["cwd"]), "the temp cwd was not removed")
        argv = info["argv"]
        self.assertIn("--strict-mcp-config", argv)
        self.assertEqual(argv[argv.index("--tools") + 1], "")
        self.assertEqual(argv[argv.index("--mcp-config") + 1],
                         os.path.join(self.run_dir, ep.CONFIG_NAME))
        self.assertNotIn(prompt, argv)
        with open(os.path.join(self.run_dir, ep.CONFIG_NAME), encoding="utf-8") as fh:
            cfg = json.load(fh)
        args = cfg["mcpServers"]["mordecai-play"]["args"]
        self.assertEqual(args[args.index("--run-dir") + 1], self.run_dir)
        self.assertEqual(args[args.index("--budget-s") + 1], "120")
        # report.md
        for part in ("# Exploratory playtest: PASS (exit 0)", "EXPLORATION ONLY", "| F01 | agent |",
                     "| known |", "| NEW |", "HUD text clipped", "Covered movement, spells",
                     "$0.4242", "mordecai-play (connected)", "## Screenshots (open them)",
                     ep.AGENT_LINE, ep.STREAM_NAME, "### F02 (minor, NEW): HUD text clipped",
                     "- Known issue: " + KNOWN_TEXT):
            self.assertIn(part, self.md)
        self.assertIn("EXPLORATORY PLAYTEST: PASS", out)

    def test_new_major_fails(self):
        code, _ = self.run_fake("major")
        self.assertEqual(code, 1)
        self.assertEqual(self.report["result"], "FAIL")
        self.assertIn("1 new major finding(s): F01 Tab does nothing", self.report["exit_reason"])

    def test_known_major_passes(self):
        self.assertEqual(self.run_fake("known_major")[0], 0)

    def test_any_blocker_fails_even_when_known(self):
        code, _ = self.run_fake("blocker_known")
        self.assertEqual(code, 1)
        self.assertIn("crash/blocker", self.report["exit_reason"])

    def test_agent_that_never_plays_is_infra(self):
        code, _ = self.run_fake("noplay")
        self.assertEqual(code, 2)
        self.assertFalse(self.report["played"])
        self.assertIn("never launched", self.report["exit_reason"])
        self.assertEqual(self.report["result"], "INFRA")

    def test_crashing_agent(self):
        code, _ = self.run_fake("crash")
        self.assertEqual(code, 2)
        a = self.report["agent"]
        self.assertEqual(a["exit_code"], 3)
        self.assertIsNone(a["cost_usd"])
        self.assertIsNone(a["result_text"])
        self.assertIn("RuntimeError: fake agent crashed", "\n".join(a["stderr_tail"]))
        self.assertIn("(none: no result event)", self.md)

    def test_wall_clock_kills_the_agent(self):
        t0 = time.monotonic()
        code, _ = self.run_fake("hang", "--test-wall-clock-s", "3")
        self.assertLess(time.monotonic() - t0, 60)
        self.assertEqual(code, 2)
        self.assertTrue(self.report["agent"]["killed_by_wall_clock"])
        self.assertIn("killed: wall clock reached", self.md)

    def test_garbage_stream_does_not_crash_the_harness(self):
        code, _ = self.run_fake("garbage")
        self.assertEqual(code, 0)
        a = self.report["agent"]
        self.assertEqual(a["stream_malformed_lines"], 2)
        self.assertIsNone(a["cost_usd"])
        self.assertIsNone(a["num_turns"])
        self.assertIsNone(a["result_text"])
        self.assertEqual(a["tool_calls"][ep.TOOL_PREFIX + "screenshot"], 1)

    def test_parse_stream_of_a_missing_file(self):
        s = ep.parse_stream(os.path.join(self.tmp, "nope.jsonl"))
        self.assertEqual((s["events"], s["result"], s["tool_calls"]), (0, None, {}))


# ---------------------------------------------------------------------------
# Detectors on synthetic run dirs
# ---------------------------------------------------------------------------
def write_log(path, lines):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\ufeffLog file open\n")
        for line in lines:
            f.write(f"[2026.10.22-20.00.00:000][  1]{line}\n")


def note(name, **data):
    return {"t": "x", "pid": 1, "conn": None, "dir": "note", "msg": dict(note=name, **data)}


class TestDetectors(TempCase):
    def setUp(self):
        super().setUp()
        self.run_dir = os.path.join(self.tmp, "run")
        os.makedirs(self.run_dir)

    def by_detector(self):
        findings, status, shots = ep.run_detectors(self.run_dir)
        out = {}
        for f in findings:
            out.setdefault(f["detector"], []).append(f)
        return out, status, shots

    def test_empty_run_dir(self):
        found, status, shots = self.by_detector()
        self.assertEqual(found, {})
        self.assertEqual(shots, [])
        self.assertIn("no game.log", status["log_errors"]["note"])
        self.assertFalse(ep.played(ep.transcript_entries(self.run_dir)))

    def test_crash_marker_in_game_log(self):
        write_log(os.path.join(self.run_dir, "game.log"),
                  ["LogTemp: Display: fine", "LogWindows: Error: Fatal error: [File:x.cpp] boom"])
        found, _, _ = self.by_detector()
        (crash,) = found["crash"]
        self.assertEqual(crash["severity"], "crash")
        self.assertIn("Fatal error", crash["details"])
        self.assertEqual(crash["id"], "D01")
        code, reason = ep.verdict([], [crash], True)
        self.assertEqual(code, 1)
        self.assertIn("D01 The game crashed", reason)

    def test_game_exited_note_in_a_relaunch_subdir(self):
        sub = os.path.join(self.run_dir, "launch_02")
        os.makedirs(sub)
        _write_jsonl(os.path.join(sub, ep.TRANSCRIPT_NAME),
                     [note("launch"), note("game_exited", exit_code=-1, crash=[],
                                           during="walk_to")])
        found, _, _ = self.by_detector()
        (crash,) = found["crash"]
        self.assertIn("exited while the agent was playing (exit code -1, during walk_to)",
                      crash["details"])
        self.assertTrue(ep.played(ep.transcript_entries(self.run_dir)))

    def test_stuck_waits_and_lost_connections_from_server_notes(self):
        _write_jsonl(os.path.join(self.run_dir, ep.TRANSCRIPT_NAME), [
            note("attach"),
            note("mcp_error", tool="walk_to", code="stuck", kind="gameplay",
                 message="stuck at (950, 0), target (1200, 0), dist 250"),
            note("mcp_error", tool="walk_to", code="walk_timeout", kind="gameplay",
                 message="walk_to timed out"),
            note("mcp_error", tool="wait_until", code="timeout", kind="gameplay",
                 message="wait_until: timeout"),
            note("mcp_error", tool="get_state", code="disconnected", kind="infra",
                 message="get_state: disconnected"),
            note("mcp_error", tool="press_key", code="bad_key", kind="gameplay", message="K"),
        ])
        found, status, _ = self.by_detector()
        stuck = {f["title"]: f for f in found["stuck"]}
        self.assertEqual(status["stuck"], {"ran": True, "findings": 3})
        walk = stuck["walk_to got stuck or timed out 2 time(s)"]
        self.assertEqual(walk["severity"], "minor")
        self.assertIn("stuck at (950, 0)", walk["details"])
        self.assertEqual(stuck["wait_until timed out 1 time(s)"]["severity"], "minor")
        lost = stuck["Lost the bridge connection 1 time(s)"]
        self.assertEqual(lost["severity"], "major")
        self.assertEqual(ep.verdict([], found["stuck"], True)[0], 1)  # a lost connection is new

    def test_stuck_fallback_on_raw_responses(self):
        _write_jsonl(os.path.join(self.run_dir, ep.TRANSCRIPT_NAME), [
            {"t": "x", "pid": 1, "conn": 1, "dir": "recv",
             "msg": {"id": 3, "ok": False, "error": "timeout", "message": "condition not met"}},
            note("walk_to_failed", message="stuck at (1, 2), target (3, 4), dist 9"),
        ])
        found, _, _ = self.by_detector()
        titles = sorted(f["title"] for f in found["stuck"])
        self.assertEqual(titles, ["wait_until timed out 1 time(s)",
                                  "walk_to got stuck or timed out 1 time(s)"])

    def test_log_errors_mordecai_first(self):
        write_log(os.path.join(self.run_dir, "game.log"),
                  ["LogNet: Error: socket thing"] * 5
                  + ["LogMordecaiSpell: Error: no cooldown tag", "LogTemp: Warning: meh"])
        found, _, _ = self.by_detector()
        (f,) = found["log_errors"]
        self.assertEqual(f["severity"], "minor")
        self.assertIn("6 error(s), 1 warning(s), 1 from LogMordecai*", f["title"])
        self.assertTrue(f["details"].startswith("(x1) LogMordecaiSpell: Error: no cooldown tag"))
        self.assertIn("LogNet: Error: socket thing", f["details"])
        write_log(os.path.join(self.run_dir, "game.log"), ["LogNet: Error: only this"])
        (f,) = self.by_detector()[0]["log_errors"]
        self.assertEqual(f["severity"], "question")

    def test_missing_content(self):
        write_log(os.path.join(self.run_dir, "game.log"),
                  ["LogStreaming: Warning: SkipPackage: /MordecaiCore/Data/DA_Thing (0x1) - "
                   "The package to load does not exist"])
        (f,) = self.by_detector()[0]["missing_content"]
        self.assertEqual(f["severity"], "major")
        self.assertIn("/MordecaiCore/Data/DA_Thing", f["details"])

    @unittest.skipUnless(HAVE_PIL, "Pillow is not installed")
    def test_blank_frames(self):
        from PIL import Image
        from tests import fake_bridge as fb
        with open(os.path.join(self.run_dir, "bridge_01.png"), "wb") as f:
            f.write(fb.make_png())  # solid colour
        Image.frombytes("RGB", (64, 36), os.urandom(64 * 36 * 3)).save(
            os.path.join(self.run_dir, "bridge_02.png"))
        found, status, shots = self.by_detector()
        (f,) = found["blank_frames"]
        self.assertEqual(f["severity"], "minor")
        self.assertIn("1 of 2 screenshot(s)", f["title"])
        self.assertIn("bridge_01.png near-uniform", f["details"])
        self.assertEqual([os.path.basename(s["path"]) for s in shots],
                         ["bridge_01.png", "bridge_02.png"])
        self.assertTrue(shots[0]["uniform"])
        self.assertFalse(shots[1]["uniform"])
        self.assertTrue(status["blank_frames"]["ran"])

    def test_agent_findings_are_normalized(self):
        with open(os.path.join(self.run_dir, ep.FINDINGS_NAME), "w", encoding="utf-8") as f:
            f.write(json.dumps({"id": "F01", "severity": "MAJOR", "title": "x"}) + "\n")
            f.write("garbage\n")
            f.write(json.dumps({"severity": "weird", "title": "y", "known_issue": ""}) + "\n")
        findings, bad = ep.load_agent_findings(self.run_dir)
        self.assertEqual(bad, 1)
        self.assertEqual([(f["id"], f["severity"], f["known_issue"]) for f in findings],
                         [("F01", "major", None), ("F02", "question", None)])


# ---------------------------------------------------------------------------
# The real MCP server over stdio, driven by a fake agent (no claude, no real game)
# ---------------------------------------------------------------------------
@unittest.skipUnless(HAVE_MCP, "the mcp SDK is not installed")
class TestDriveMcp(TempCase):
    def test_attached_to_the_fake_bridge(self):
        from tests import fake_bridge as fb
        world = fb.FakeWorld()
        server = fb.FakeBridgeServer(world, token=TOKEN,
                                     shot_dir=os.path.join(self.tmp, "elsewhere")).start()
        self.addCleanup(server.stop)
        code, out = self.run_fake("drive", "--attach-port", str(server.port), "--token", TOKEN,
                                  "--minutes", "1")
        self.assertEqual(code, 0, out)
        r = self.report
        self.assertEqual(r["mode"], "attach")
        self.assertTrue(r["played"])
        self.assertTrue(any(d.startswith("attached to 127.0.0.1") for d in r["diagnostic"]))
        self.assertEqual(r["build"]["skipped"], "attached: this harness did not launch the game")
        self.assertEqual(world.weapon_index, 1)  # the Tab tap reached the fake game
        self.assertTrue(server.quit_event.wait(5))
        (finding,) = r["findings"]["agent"]
        self.assertEqual(finding["title"], "Weapon text flickers")
        self.assertEqual(finding["screenshot"], os.path.join(self.run_dir, "bridge_01.png"))
        self.assertIsNotNone(finding["player_location"])
        self.assertEqual(r["agent"]["server_tool_calls"],
                         {"get_state": 1, "press_key": 1, "quit_game": 1, "screenshot": 1})
        self.assertEqual(len(r["agent"]["mordecai_tools_seen"]), 13)
        self.assertEqual([os.path.basename(s["path"]) for s in r["screenshots"]],
                         ["bridge_01.png"])
        if HAVE_PIL:  # the fake's frame is one solid colour
            self.assertEqual([f["detector"] for f in r["findings"]["detector"]],
                             ["blank_frames"])

    def test_launching_the_fake_game(self):
        code, out = self.run_fake("drive_launch", "--minutes", "1", "--test-exe-argv",
                                  json.dumps([sys.executable, FAKE_GAME]))
        self.assertEqual(code, 0, out)
        r = self.report
        self.assertEqual(r["mode"], "launch")
        self.assertTrue(r["played"])
        self.assertTrue(os.path.isfile(os.path.join(self.run_dir, "game.log")))
        self.assertEqual(r["files"]["game.log"], [os.path.join(self.run_dir, "game.log")])
        self.assertEqual(r["agent"]["server_tool_calls"].get("launch_game"), 1)
        self.assertEqual(r["detectors"]["crash"], {"ran": True, "findings": 0})
        self.assertEqual(len(r["findings"]["agent"]), 1)


if __name__ == "__main__":
    if "--fake-agent" in sys.argv:
        sys.exit(fake_agent_main(sys.argv[1:]))
    unittest.main()
