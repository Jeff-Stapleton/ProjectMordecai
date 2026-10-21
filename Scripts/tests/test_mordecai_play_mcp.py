"""Tests for Scripts/mordecai_play_mcp.py (the mordecai-play MCP server).

In-process tests drive the server with the SDK's own client (mcp.Client(server)) against
tests/fake_bridge.py (attach mode) and tests/fake_game.py (launch_game); raw stdio tests start
the server as a subprocess and speak JSON-RPC to it.  Nothing here launches the real game.
Tests that need the SDK are skipped when it is not installed (py -m pip install -r
Scripts/requirements.txt).  Run: py Scripts/run_python_tests.py -k Mcp
"""

import asyncio
import importlib.util
import io
import json
import os
import queue
import re
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT_ROOT = os.path.dirname(SCRIPTS)
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import mordecai_play_mcp as mpm  # noqa: E402
import playtest_boot as pb  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

HAVE_MCP = importlib.util.find_spec("mcp") is not None and mpm.MCP_IMPORT_ERROR is None
if HAVE_MCP:
    from mcp import Client
    from mcp.types import ImageContent, TextContent

SERVER = os.path.join(SCRIPTS, "mordecai_play_mcp.py")
FAKE_GAME = os.path.join(SCRIPTS, "tests", "fake_game.py")
TOKEN = "mcp-token"
TOOLS = ["launch_game", "get_state", "press_key", "hold_key", "release_key", "walk_to", "click",
         "click_button", "screenshot", "wait_until", "quit_game"]
needs_mcp = unittest.skipUnless(HAVE_MCP, "the mcp SDK is not installed")


def texts(result):
    return [c.text for c in result.content if getattr(c, "type", None) == "text"]


def first_json(text):
    """The JSON object in a tool result's text (after any banner line)."""
    start = text.index("{")
    return json.loads(text[start:text.rindex("}") + 1])


def wait_pid_gone(pid, timeout=15.0):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if not ab.pid_alive(pid):
            return True
        time.sleep(0.1)
    return False


class McpCase(unittest.TestCase):
    """A temp dir, quiet logs, and helpers to call tools on an in-process server."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        for module in (ab, pb, mpm):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)

    def make_server(self, session):
        self.session = session
        self.addCleanup(session.shutdown)
        self.server = mpm.build_server(session, log_level="CRITICAL")
        return self.server

    def calls(self, *calls):
        """Run (tool, args) calls in order on one client; returns the CallToolResults."""
        async def go():
            out = []
            async with Client(self.server) as client:
                for name, args in calls:
                    out.append(await client.call_tool(name, args))
            return out
        return asyncio.run(go())

    def call(self, name, args=None):
        return self.calls((name, args or {}))[0]

    def ok_text(self, result):
        self.assertFalse(result.is_error, texts(result))
        return "\n".join(texts(result))

    def err_text(self, result):
        self.assertTrue(result.is_error, texts(result))
        return "\n".join(texts(result))


class AttachedCase(McpCase):
    """The server in --attach mode on a fresh fake bridge."""

    def world_kwargs(self):
        return {}

    def faults(self):
        return None

    def setUp(self):
        super().setUp()
        self.world = fb.FakeWorld(**self.world_kwargs())
        self.bridge_server = fb.FakeBridgeServer(self.world, token=TOKEN, shot_dir=self.tmp,
                                                 faults=self.faults()).start()
        self.addCleanup(self.bridge_server.stop)
        self.make_server(mpm.PlaySession(attach_port=self.bridge_server.port,
                                         attach_token=TOKEN))


# ---------------------------------------------------------------------------
# The tool list
# ---------------------------------------------------------------------------
@needs_mcp
class TestToolList(McpCase):
    def test_the_eleven_tools_and_their_rules(self):
        self.make_server(mpm.PlaySession())

        async def go():
            async with Client(self.server) as client:
                return await client.list_tools()
        tools = {t.name: t for t in asyncio.run(go()).tools}
        self.assertEqual(sorted(tools), sorted(TOOLS))
        for name, tool in tools.items():
            d = tool.description
            self.assertIn("real input only", d, name)
            self.assertIn("screenshots", d, name)
            self.assertIn("acceptance evidence unless launch_game reported a FRESH build from a "
                          "clean, committed tree", d, name)
            self.assertIn("'DIRTY BUILD'", d, name)
            self.assertLess(len(d), 4096, name)
        props = {n: set((t.input_schema or {}).get("properties", {})) for n, t in tools.items()}
        self.assertEqual(props["launch_game"], {"map", "frontend", "allow_stale",
                                                "expect_experience"})
        self.assertEqual(props["get_state"], {"sections", "radius"})
        self.assertEqual(props["press_key"], {"key", "hold_s"})
        self.assertEqual(props["hold_key"], {"key", "seconds"})
        self.assertEqual(props["release_key"], {"key"})
        self.assertEqual(props["walk_to"], {"x", "y", "tol", "timeout_s"})
        self.assertEqual(props["click"], {"x", "y", "button"})
        self.assertEqual(props["click_button"], {"label", "button_id"})
        self.assertEqual(props["screenshot"], set())
        self.assertEqual(props["wait_until"], {"condition", "timeout_s"})
        self.assertEqual(props["quit_game"], set())
        # A key held with hold_key must be released first (the bridge refuses it, walk_to too).
        for name in ("press_key", "hold_key", "walk_to"):
            self.assertIn("release_key", tools[name].description, name)
        self.assertIn("already held", tools["press_key"].description)
        self.assertIn("keys_held", tools["walk_to"].description)
        # `hit` is the deepest widget at the point (the button's text), not necessarily the button.
        for name in ("click", "click_button"):
            self.assertIn("deepest widget", tools[name].description, name)
            self.assertIn("not necessarily the button", tools[name].description, name)
        self.assertIn(mpm.DIRTY_BANNER, tools["launch_game"].description)

    def test_no_game_yet(self):
        self.make_server(mpm.PlaySession())
        for name, args in (("get_state", {}), ("press_key", {"key": "F"}), ("screenshot", {}),
                           ("quit_game", {})):
            self.assertIn("call launch_game first", self.err_text(self.call(name, args)), name)


# ---------------------------------------------------------------------------
# Every tool against the fake bridge (attach mode)
# ---------------------------------------------------------------------------
@needs_mcp
class TestAttachedTools(AttachedCase):
    def test_get_state(self):
        text = self.ok_text(self.call("get_state", {"sections": ["player", "pickups"]}))
        self.assertTrue(text.startswith("!! " + mpm.ATTACHED_BANNER))
        st = first_json(text)
        self.assertEqual(sorted(st), ["pickups", "player"])
        self.assertEqual(st["player"]["location"][:2], [0.0, 0.0])
        st = first_json(self.ok_text(self.call("get_state", {"sections": "game,focus"})))
        self.assertEqual(sorted(st), ["focus", "game"])
        st = first_json(self.ok_text(self.call("get_state", {})))
        self.assertEqual(sorted(st), sorted(fb.SECTIONS))
        last = [r for r in self.bridge_server.received if r["cmd"] == "state"][-1]
        self.assertEqual(last["radius"], 1500.0)

    def test_get_state_bad_section_is_an_error(self):
        text = self.err_text(self.call("get_state", {"sections": ["nope"]}))
        self.assertIn("bad_request", text)
        self.assertIn("unknown section", text)
        self.assertTrue(text.rstrip().endswith(mpm.ATTACHED_BANNER))

    def test_press_key_taps(self):
        resp = first_json(self.ok_text(self.call("press_key", {"key": "I"})))
        self.assertEqual(resp["keys"], ["I"])
        self.assertTrue(self.world.menu_open)
        self.assertEqual(self.world.active_tab, "inventory")
        req = [r for r in self.bridge_server.received if r["cmd"] == "key"][-1]
        self.assertEqual((req["action"], req["hold_s"]), ("tap", 0.1))

    def test_press_key_several_keys_together(self):
        self.ok_text(self.call("press_key", {"key": ["W", "D"], "hold_s": 0.3}))
        req = [r for r in self.bridge_server.received if r["cmd"] == "key"][-1]
        self.assertEqual(req["keys"], ["W", "D"])
        x, y = self.world.pos
        self.assertGreater(x, 50)
        self.assertGreater(y, 50)

    def test_hold_key_for_seconds(self):
        resp = first_json(self.ok_text(self.call("hold_key", {"key": "W", "seconds": 0.5})))
        self.assertGreaterEqual(resp["held_s"], 0.5)
        self.assertGreater(self.world.pos[0], 200)
        self.assertEqual([e[1] for e in self.world.input_log if e[2] == "W"], ["down", "up"])

    def test_hold_key_until_release(self):
        r1, r2, r3 = self.calls(("hold_key", {"key": "D"}),
                                ("get_state", {"sections": ["input"]}),
                                ("release_key", {"key": "D"}))
        self.ok_text(r1)
        held = [h["key"] for h in first_json(self.ok_text(r2))["input"]["held"]]
        self.assertIn("D", held)
        self.assertTrue(first_json(self.ok_text(r3))["was_held"])
        self.assertGreater(self.world.pos[1], 0)
        self.assertEqual(self.world._held_union(), {})

    def test_a_held_key_must_be_released_before_pressing_or_walking(self):
        r = self.calls(("hold_key", {"key": "W"}), ("press_key", {"key": "W"}),
                       ("hold_key", {"key": "W", "seconds": 0.2}), ("walk_to", {"x": 400, "y": 0}),
                       ("release_key", {"key": "W"}), ("press_key", {"key": "W"}))
        self.ok_text(r[0])
        for res in r[1:3]:
            text = self.err_text(res)
            self.assertIn("[bad_request]", text)
            self.assertIn("already held by this connection; release it first", text)
        text = self.err_text(r[3])
        self.assertIn("GAMEPLAY failure", text)
        self.assertIn("[keys_held]", text)
        self.assertIn("W is already held", text)
        self.assertTrue(first_json(self.ok_text(r[4]))["was_held"])
        self.assertEqual(first_json(self.ok_text(r[5]))["keys"], ["W"])
        self.assertEqual([e[1] for e in self.world.input_log if e[2] == "W"],
                         ["down", "up", "down", "up"])

    def test_release_key_not_held(self):
        self.assertFalse(first_json(self.ok_text(self.call("release_key", {"key": "W"})))[
            "was_held"])

    def test_walk_to(self):
        result = first_json(self.ok_text(self.call("walk_to", {"x": 400, "y": 0, "tol": 60})))
        self.assertLessEqual(result["dist"], 60)
        self.assertEqual(result["side_steps"], 0)
        self.assertLess(abs(self.world.pos[0] - 400), 61)

    def test_click_and_click_button_identify_the_amulet(self):
        self.world.set_position(-400, 0)  # test setup, not gameplay: stand on the amulet
        self.ok_text(self.call("press_key", {"key": "F"}))
        self.ok_text(self.call("press_key", {"key": "I"}))
        resp = first_json(self.ok_text(self.call("click_button", {"label": "Identify"})))
        self.assertEqual(resp["hit"], "STextBlock")  # the label, as in the packaged game
        self.assertEqual(resp["target"]["label"], "Identify")
        amulet = self.world.inventory[0]
        self.assertTrue(amulet["identified"])
        self.assertEqual(amulet["display_name"], "Mysterious Amulet")
        text = self.err_text(self.call("click_button", {"label": "Identify"}))
        self.assertIn("no_button", text)
        self.assertIn("no visible button", text)

    def test_click_at_a_point(self):
        self.world.set_position(-400, 0)
        self.calls(("press_key", {"key": "F"}), ("press_key", {"key": "I"}))
        cx, cy = self.world.identify_center
        resp = first_json(self.ok_text(self.call("click", {"x": cx, "y": cy})))
        self.assertEqual((resp["hit"], resp["button"]), ("STextBlock", "Left"))
        self.assertTrue(self.world.inventory[0]["identified"])

    def test_click_needs_both_coordinates(self):
        self.assertIn("bad arguments", self.err_text(self.call("click", {"x": 10})))
        self.assertIn("label or a button_id", self.err_text(self.call("click_button", {})))

    def test_screenshot_returns_the_image_and_its_path(self):
        result = self.call("screenshot")
        self.assertFalse(result.is_error, texts(result))
        kinds = [type(c) for c in result.content]
        self.assertEqual(kinds, [TextContent, ImageContent])
        image = result.content[1]
        self.assertEqual(image.mime_type, "image/png")
        path = re.search(r"Screenshot: (.+\.png)", result.content[0].text).group(1).strip()
        self.assertTrue(os.path.isfile(path))
        self.assertEqual(os.path.dirname(path), self.tmp)
        import base64
        with open(path, "rb") as f:
            self.assertEqual(base64.b64decode(image.data), f.read())
        self.assertIn("Describe what is actually on screen", result.content[0].text)

    def test_screenshot_over_budget_is_a_jpeg_or_text_only(self):
        self.session.max_image_chars = 60  # the fake's 76-byte PNG is 104 base64 chars
        result = self.call("screenshot")
        self.assertFalse(result.is_error, texts(result))
        # Even the smallest JPEG of a 16x9 frame is bigger than 60 chars: text only.
        self.assertEqual([type(c) for c in result.content], [TextContent])
        self.assertIn("No inline image", result.content[0].text)
        self.session.max_image_chars = 5000
        result = self.call("screenshot")
        self.assertEqual(result.content[1].mime_type, "image/png")

    def test_wait_until(self):
        resp = first_json(self.ok_text(self.call("wait_until", {"condition": "game.ready==true",
                                                               "timeout_s": 2})))
        self.assertEqual(resp["values"], {"game.ready": True})
        text = self.err_text(self.call("wait_until", {"condition": "inventory.count>=4",
                                                      "timeout_s": 0.3}))
        self.assertIn("GAMEPLAY failure", text)
        self.assertIn("[timeout]", text)
        self.assertIn('"inventory.count":0', text)
        self.assertIn("bad_request", self.err_text(self.call("wait_until",
                                                             {"condition": "((", "timeout_s": 1})))

    def test_bad_and_denied_keys_are_refused(self):
        text = self.err_text(self.call("press_key", {"key": "NotAKey"}))
        self.assertIn("[bad_key]", text)
        self.assertIn("refused", text)
        text = self.err_text(self.call("press_key", {"key": "K"}))
        self.assertIn("denied debug key", text)
        self.assertFalse(self.world.is_dead)
        self.assertIn("[bad_key]", self.err_text(self.call("hold_key", {"key": "LeftMouseButton"})))

    def test_bad_argument_types_are_refused_by_the_schema(self):
        text = self.err_text(self.call("walk_to", {"x": "far", "y": 0}))
        self.assertIn("x", text)

    def test_concurrent_calls_are_serialized(self):
        async def go():
            async with Client(self.server) as client:
                return await asyncio.gather(
                    client.call_tool("walk_to", {"x": 300, "y": 0, "tol": 40}),
                    client.call_tool("get_state", {"sections": ["player"]}),
                    client.call_tool("press_key", {"key": "Tab"}),
                    client.call_tool("get_state", {"sections": ["game"]}))
        results = asyncio.run(go())
        for r in results:
            self.assertFalse(r.is_error, texts(r))
        self.assertEqual(self.bridge_server.connection_count(), 1)
        self.assertEqual(self.world.weapon_index, 1)  # the Tab tap happened

        # walk_to is many requests (state, W tap, state, ...); the lock keeps them contiguous.
        def is_walk(req):
            if req["cmd"] == "state":
                return req.get("sections") == list(ab.WALK_SECTIONS)
            return req["cmd"] == "key" and (req.get("key") or req.get("keys")) not in ("Tab",
                                                                                       ["Tab"])
        reqs = [r for r in self.bridge_server.received if r["cmd"] in ("state", "key")]
        walk = [i for i, r in enumerate(reqs) if is_walk(r)]
        self.assertGreater(len(walk), 2)
        self.assertEqual(walk, list(range(walk[0], walk[-1] + 1)),
                         [(r["cmd"], r.get("sections") or r.get("key")) for r in reqs])

    def test_quit_game_quits_the_attached_bridge(self):
        text = self.ok_text(self.call("quit_game"))
        self.assertIn("Game quit", text)
        self.assertIn("attached game", text)
        self.assertTrue(self.bridge_server.quit_event.wait(5))
        self.assertIn("cannot attach", self.err_text(self.call("get_state")))

    def test_launch_is_refused_in_attach_mode(self):
        self.assertIn("--attach", self.err_text(self.call("launch_game")))

    def test_transcript_notes_every_tool_call(self):
        run_dir = os.path.join(self.tmp, "run")
        os.makedirs(run_dir)
        self.session._attach()
        self.session.bridge.transcript = ab.Transcript.for_run_dir(run_dir)
        self.session.bridge.conn.transcript = self.session.bridge.transcript
        self.calls(("get_state", {"sections": ["game"]}), ("press_key", {"key": "F"}))
        with open(os.path.join(run_dir, ab.TRANSCRIPT_NAME), encoding="utf-8") as f:
            entries = [json.loads(line) for line in f]
        notes = [e["msg"]["tool"] for e in entries if e["dir"] == "note"]
        self.assertEqual(notes, ["get_state", "press_key"])
        self.assertEqual([e["msg"]["cmd"] for e in entries if e["dir"] == "send"],
                         ["state", "key"])


@needs_mcp
class TestStuck(AttachedCase):
    def world_kwargs(self):
        return {"obstacles": [(100, 140, -500, 500)]}

    def test_walk_into_a_wall_reports_stuck(self):
        text = self.err_text(self.call("walk_to", {"x": 400, "y": 0, "timeout_s": 30}))
        self.assertIn("GAMEPLAY failure", text)
        self.assertIn("[stuck]", text)
        self.assertIn("stuck at (", text)


@needs_mcp
class TestBridgeErrors(AttachedCase):
    def faults(self):
        return fb.Faults(error_replies={"state": ("failed", "the gatherer exploded")})

    def test_infra_error_message_reaches_the_agent(self):
        text = self.err_text(self.call("get_state"))
        self.assertIn("INFRA failure", text)
        self.assertIn("[failed]", text)
        self.assertIn("the gatherer exploded", text)


@needs_mcp
class TestLostConnection(AttachedCase):
    def faults(self):
        return fb.Faults(drop_on_cmd="click")

    def test_reconnects_after_the_bridge_drops_the_connection(self):
        self.assertIn("disconnected", self.err_text(self.call("click", {"x": 10, "y": 10})))
        # A new connection is opened on the next call.
        self.ok_text(self.call("get_state", {"sections": ["game"]}))


# ---------------------------------------------------------------------------
# launch_game / quit_game / shutdown with tests/fake_game.py as the game
# ---------------------------------------------------------------------------
@needs_mcp
class TestLaunch(McpCase):
    def session_for(self, **overrides):
        o = {"exe_argv": [sys.executable, FAKE_GAME], "skip_freshness": True,
             "run_dir": os.path.join(self.tmp, "run")}
        o.update(overrides)
        return self.make_server(mpm.PlaySession(launch_overrides=o))

    def launched_pid(self, text):
        return int(re.search(r"pid (\d+)", text).group(1))

    def test_launch_play_quit_relaunch(self):
        self.session_for()
        text = self.ok_text(self.call("launch_game"))
        self.assertIn("Game ready", text)
        self.assertIn("kill-on-close job", text)
        self.assertIn(mpm.RULE, text)
        pid = self.launched_pid(text)
        self.assertTrue(ab.pid_alive(pid))
        self.assertIn("already running", self.err_text(self.call("launch_game")))
        st = first_json(self.ok_text(self.call("get_state", {"sections": ["game"]})))
        self.assertTrue(st["game"]["ready"])
        shot = self.call("screenshot")
        self.assertEqual(shot.content[1].mime_type, "image/png")
        self.assertIn(os.path.join(self.tmp, "run"), shot.content[0].text)
        text = self.ok_text(self.call("quit_game"))
        self.assertIn("Game quit: exit code 0", text)
        self.assertIn("bridge_01.png", text)
        self.assertIn("game.log", text)
        self.assertTrue(wait_pid_gone(pid))
        self.assertIn("call launch_game first", self.err_text(self.call("get_state")))
        # A new game can be launched after quit.
        self.session.launch_overrides["run_dir"] = os.path.join(self.tmp, "run2")
        pid2 = self.launched_pid(self.ok_text(self.call("launch_game")))
        self.assertNotEqual(pid, pid2)
        self.session.shutdown()  # server exit kills the game
        self.assertTrue(wait_pid_gone(pid2))
        self.assertIsNone(self.session.bridge)

    def test_a_dead_game_is_reported_and_can_be_relaunched(self):
        self.session_for()
        pid = self.launched_pid(self.ok_text(self.call("launch_game")))
        ab.kill_pid(pid)
        self.assertTrue(wait_pid_gone(pid))
        text = self.err_text(self.call("get_state"))
        self.assertIn("no longer running", text)
        self.session.launch_overrides["run_dir"] = os.path.join(self.tmp, "run2")
        pid2 = self.launched_pid(self.ok_text(self.call("launch_game")))
        self.assertNotEqual(pid, pid2)

    def test_a_failed_launch_is_an_error(self):
        self.session_for(extra_args=["-FakeCrash"])
        text = self.err_text(self.call("launch_game"))
        self.assertIn("INFRA failure", text)
        self.assertIn("[crash]", text)
        self.assertIsNone(self.session.bridge)

    def test_stale_build_is_refused(self):
        self.session_for(skip_freshness=False)
        stale = {"fresh": False, "reasons": ["build commit aaa != HEAD bbb"], "warnings": [],
                 "git_error": None, "build": None, "head": "bbb", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(stale, pb.EXIT_INFRA)):
            text = self.err_text(self.call("launch_game"))
        self.assertIn("STALE BUILD", text)
        self.assertIn("build commit aaa != HEAD bbb", text)
        self.assertIn("allow_stale is for diagnosis only", text)

    def test_allow_stale_banners_every_result(self):
        self.session_for(skip_freshness=False)
        stale = {"fresh": False, "reasons": ["build commit aaa != HEAD bbb"], "warnings": [],
                 "git_error": None, "build": None, "head": "bbb", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(stale, None)) as gate:
            launch = self.call("launch_game", {"allow_stale": True})
        self.assertTrue(gate.call_args[0][1], "allow_stale must reach the gate")
        banner = "!! " + mpm.STALE_BANNER
        text = self.ok_text(launch)
        self.assertTrue(text.startswith(banner), text)
        self.assertIn("STALE (allowed: diagnostic only)", text)
        for name, args in (("get_state", {"sections": ["game"]}), ("press_key", {"key": "Tab"}),
                           ("walk_to", {"x": 100, "y": 0}), ("wait_until",
                                                              {"condition": "game.ready"})):
            self.assertTrue(self.ok_text(self.call(name, args)).startswith(banner), name)
        shot = self.call("screenshot")
        self.assertTrue(shot.content[0].text.startswith(banner))
        err = self.err_text(self.call("press_key", {"key": "NotAKey"}))
        self.assertIn(mpm.STALE_BANNER, err)
        self.assertIn(mpm.STALE_BANNER, self.ok_text(self.call("quit_game")))

    def test_a_fresh_package_built_from_a_dirty_tree_banners_every_result(self):
        # playtest_boot's gate calls a dirty build fresh when its dirty files still match; it is
        # diagnostic only (CLAUDE.md), never a clean FRESH build.
        self.session_for(skip_freshness=False)
        dirty = {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
                 "build": {"commit": "abc1234def5678", "branch": "x", "dirty": True,
                           "dirty_count": 38},
                 "head": "abc1234def5678", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(dirty, None)):
            launch = self.call("launch_game")
        banner = "!! " + mpm.DIRTY_BANNER
        text = self.ok_text(launch)
        self.assertTrue(text.startswith(banner), text)
        self.assertIn("38 uncommitted file(s)", text.splitlines()[0])
        self.assertIn("FRESH but built from a DIRTY tree (diagnostic only)", text)
        self.assertNotIn("FRESH (the package matches HEAD and the working tree)", text)
        self.assertIn("a FRESH build from a clean, committed tree", text)  # the RULE
        self.assertNotIn(mpm.STALE_BANNER, text)
        for name, args in (("get_state", {"sections": ["game"]}), ("press_key", {"key": "Tab"}),
                           ("walk_to", {"x": 100, "y": 0}), ("wait_until",
                                                              {"condition": "game.ready"})):
            self.assertTrue(self.ok_text(self.call(name, args)).startswith(banner), name)
        shot = self.call("screenshot")
        self.assertTrue(shot.content[0].text.startswith(banner))
        self.assertIn(mpm.DIRTY_BANNER, self.err_text(self.call("press_key", {"key": "NotAKey"})))
        self.assertIn(mpm.DIRTY_BANNER, self.ok_text(self.call("quit_game")))

    def test_a_fresh_clean_build_has_no_banner(self):
        self.session_for(skip_freshness=False)
        clean = {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
                 "build": {"commit": "abc1234def5678", "branch": "x", "dirty": False,
                           "dirty_count": 0},
                 "head": "abc1234def5678", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(clean, None)):
            text = self.ok_text(self.call("launch_game"))
        self.assertFalse(text.startswith("!! "), text)
        self.assertIn("FRESH (the package matches HEAD and the working tree)", text)
        self.assertFalse(self.ok_text(self.call("get_state", {"sections": ["game"]}))
                         .startswith("!! "))
        self.ok_text(self.call("quit_game"))


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
class TestHelpers(unittest.TestCase):
    def test_fit_state_text_trims_actors_first(self):
        st = {"player": {"valid": True}, "actors": [{"label": f"A{i}", "pad": "x" * 200}
                                                    for i in range(400)]}
        text, note = mpm.fit_state_text(st, limit=20000)
        self.assertLessEqual(len(text), 20000)
        out = json.loads(text)
        self.assertEqual(out["player"], {"valid": True})
        self.assertLess(len(out["actors"]), 400)
        self.assertEqual(out["actors"][0]["label"], "A0")
        self.assertIn("actors trimmed", note)
        small = {"game": {"ready": True}}
        self.assertEqual(mpm.fit_state_text(small), (json.dumps(small, separators=(",", ":")),
                                                     None))
        text, note = mpm.fit_state_text({"widgets": ["y" * 5000]}, limit=1000)
        self.assertEqual(len(text), 1000)
        self.assertIn("output cut", note)

    @unittest.skipUnless(importlib.util.find_spec("PIL"), "Pillow is not installed")
    def test_image_for_mcp_downscales_a_big_frame(self):
        from PIL import Image as PILImage
        tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.addCleanup(tmp.cleanup)
        path = os.path.join(tmp.name, "big.png")
        img = PILImage.frombytes("RGB", (1280, 720), os.urandom(1280 * 720 * 3))
        img.save(path)
        self.assertGreater(os.path.getsize(path), 1_000_000)
        data, fmt, desc = mpm.image_for_mcp(path, max_b64_chars=80000)
        self.assertEqual(fmt, "jpeg")
        self.assertLessEqual(mpm._b64_len(len(data)), 80000)
        self.assertIn("downscaled preview of the 1280x720 PNG", desc)
        with PILImage.open(io.BytesIO(data)) as im:
            self.assertEqual(im.format, "JPEG")
            self.assertLessEqual(im.width, 1280)
        data, fmt, desc = mpm.image_for_mcp(path, max_b64_chars=100)
        self.assertIsNone(data)
        self.assertIn("over the 100-character", desc)

    def test_image_for_mcp_keeps_a_small_png(self):
        tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.addCleanup(tmp.cleanup)
        path = os.path.join(tmp.name, "s.png")
        png = fb.make_png()
        with open(path, "wb") as f:
            f.write(png)
        data, fmt, _ = mpm.image_for_mcp(path)
        self.assertEqual((data, fmt), (png, "png"))

    def test_mcp_json_registers_the_server(self):
        with open(os.path.join(PROJECT_ROOT, ".mcp.json"), encoding="utf-8") as f:
            cfg = json.load(f)
        entry = cfg["mcpServers"]["mordecai-play"]
        self.assertEqual(entry["type"], "stdio")
        self.assertEqual(entry["command"], "py")
        self.assertEqual(entry["args"], ["-X", "utf8", "Scripts/mordecai_play_mcp.py"])
        self.assertEqual(entry["env"], {"PYTHONUNBUFFERED": "1"})
        self.assertTrue(os.path.isfile(os.path.join(PROJECT_ROOT, *entry["args"][-1].split("/"))))

    def test_requirements_pin_the_2x_sdk(self):
        with open(os.path.join(SCRIPTS, "requirements.txt"), encoding="utf-8") as f:
            self.assertIn("mcp>=2.3,<3", f.read())

    def test_missing_sdk_exits_2_with_an_install_hint(self):
        code = ("import runpy, sys; sys.modules['mcp'] = None; sys.argv = [sys.argv[0]]; "
                f"runpy.run_path({SERVER!r}, run_name='__main__')")
        r = subprocess.run([sys.executable, "-X", "utf8", "-B", "-c", code], capture_output=True,
                           text=True, errors="replace", timeout=120, stdin=subprocess.DEVNULL)
        self.assertEqual(r.returncode, 2, r.stderr)
        self.assertIn("py -m pip install -r Scripts/requirements.txt", r.stderr)
        self.assertEqual(r.stdout, "")


# ---------------------------------------------------------------------------
# Raw stdio: the real server process, no SDK on the client side
# ---------------------------------------------------------------------------
class StdioServer:
    """The server as a subprocess, spoken to line by line."""

    def __init__(self, args, stderr_path):
        self.stderr = open(stderr_path, "w", encoding="utf-8", errors="replace")
        self.proc = subprocess.Popen([sys.executable, "-X", "utf8", "-B", SERVER, *args],
                                     stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=self.stderr, cwd=PROJECT_ROOT,
                                     env=dict(os.environ, PYTHONUNBUFFERED="1"))
        self.lines = queue.Queue()
        self.raw = []
        self._reader = threading.Thread(target=self._read, daemon=True)
        self._reader.start()
        self.next_id = 1

    def _read(self):
        for raw in self.proc.stdout:
            self.raw.append(raw)
            self.lines.put(raw)
        self.lines.put(None)

    def send(self, obj):
        self.proc.stdin.write((json.dumps(obj) + "\n").encode("utf-8"))
        self.proc.stdin.flush()

    def request(self, method, params=None, timeout=90.0):
        rid = self.next_id
        self.next_id += 1
        msg = {"jsonrpc": "2.0", "id": rid, "method": method}
        if params is not None:
            msg["params"] = params
        self.send(msg)
        end = time.monotonic() + timeout
        while True:
            raw = self.lines.get(timeout=max(0.1, end - time.monotonic()))
            if raw is None:
                raise AssertionError(f"the server closed stdout before answering {method}")
            obj = json.loads(raw.decode("utf-8"))
            if obj.get("id") == rid:
                return obj

    def notify(self, method, params=None):
        msg = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            msg["params"] = params
        self.send(msg)

    def initialize(self):
        resp = self.request("initialize", {"protocolVersion": "2025-06-18", "capabilities": {},
                                           "clientInfo": {"name": "raw-test", "version": "0"}})
        self.notify("notifications/initialized")
        return resp

    def call_tool(self, name, arguments=None, timeout=90.0):
        return self.request("tools/call", {"name": name, "arguments": arguments or {}},
                            timeout=timeout)

    def close_stdin_and_wait(self, timeout=30.0):
        self.proc.stdin.close()
        try:
            return self.proc.wait(timeout=timeout)
        finally:
            self.stderr.close()

    def kill(self):
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait(timeout=10)
        if not self.stderr.closed:
            self.stderr.close()


@needs_mcp
class TestStdio(McpCase):
    def start(self, *args):
        server = StdioServer(list(args), os.path.join(self.tmp, "stderr.txt"))
        self.addCleanup(server.kill)
        return server

    def assert_clean_stdout(self, server):
        self.assertTrue(server.raw)
        for raw in server.raw:
            self.assertTrue(raw.endswith(b"\n"), raw)
            obj = json.loads(raw.decode("utf-8"))  # every stdout line is one JSON-RPC message
            self.assertIsInstance(obj, dict, raw)
            self.assertEqual(obj.get("jsonrpc"), "2.0", raw)
            self.assertTrue("id" in obj or "method" in obj, raw)

    def test_initialize_list_get_state_quit_over_stdio(self):
        world = fb.FakeWorld()
        bridge = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(bridge.stop)
        server = self.start("--attach", str(bridge.port), "--token", TOKEN)
        init = server.initialize()
        self.assertEqual(init["result"]["serverInfo"]["name"], "mordecai-play")
        self.assertIn("real input only", init["result"].get("instructions", ""))
        tools = server.request("tools/list")["result"]["tools"]
        self.assertEqual(sorted(t["name"] for t in tools), sorted(TOOLS))
        res = server.call_tool("get_state", {"sections": ["player"]})["result"]
        self.assertFalse(res.get("isError"), res)
        self.assertIn('"player"', res["content"][0]["text"])
        shot = server.call_tool("screenshot")["result"]
        self.assertEqual([c["type"] for c in shot["content"]], ["text", "image"])
        bad = server.call_tool("press_key", {"key": "K"})["result"]
        self.assertTrue(bad["isError"])
        self.assertIn("denied debug key", bad["content"][0]["text"])
        res = server.call_tool("quit_game")["result"]
        self.assertFalse(res.get("isError"), res)
        self.assertTrue(bridge.quit_event.wait(5))
        self.assertEqual(server.close_stdin_and_wait(), 0)
        self.assert_clean_stdout(server)
        with open(os.path.join(self.tmp, "stderr.txt"), encoding="utf-8") as f:
            self.assertIn("[Play-MCP] serving on stdio", f.read())

    def _launch(self, server):
        server.initialize()
        res = server.call_tool("launch_game", timeout=120)["result"]
        self.assertFalse(res.get("isError"), res)
        return int(re.search(r"pid (\d+)", res["content"][0]["text"]).group(1))

    def test_closing_stdin_kills_the_launched_game(self):
        server = self.start("--test-exe-argv", json.dumps([sys.executable, FAKE_GAME]),
                            "--test-skip-freshness", "--test-run-dir",
                            os.path.join(self.tmp, "run"))
        pid = self._launch(server)
        self.assertTrue(ab.pid_alive(pid))
        self.assertEqual(server.close_stdin_and_wait(), 0)
        self.assertTrue(wait_pid_gone(pid), "the game outlived the server")
        self.assert_clean_stdout(server)

    @unittest.skipUnless(sys.platform == "win32", "kill-on-close jobs are Windows only")
    def test_killing_the_server_kills_the_game(self):
        server = self.start("--test-exe-argv", json.dumps([sys.executable, FAKE_GAME]),
                            "--test-skip-freshness", "--test-run-dir",
                            os.path.join(self.tmp, "run"))
        pid = self._launch(server)
        server.proc.kill()  # TerminateProcess: no finally, no atexit; only the job remains
        server.proc.wait(timeout=10)
        self.assertTrue(wait_pid_gone(pid), "the kill-on-close job did not kill the game")


if __name__ == "__main__":
    unittest.main()
