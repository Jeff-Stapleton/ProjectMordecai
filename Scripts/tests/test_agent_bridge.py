"""Unit tests for Scripts/agent_bridge.py against tests/fake_bridge.py and tests/fake_game.py.

Nothing here launches the real game.  Run: py Scripts/run_python_tests.py
"""

import io
import json
import os
import socket
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

FAKE_GAME = os.path.join(SCRIPTS, "tests", "fake_game.py")
AGENT_BRIDGE = os.path.join(SCRIPTS, "agent_bridge.py")
TOKEN = "tok"
WINDOWS = sys.platform == "win32"


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


def wait_pid_gone(pid, timeout=10.0):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if not ab.pid_alive(pid):
            return True
        time.sleep(0.1)
    return False


class QuietCase(unittest.TestCase):
    """Temp dir + client/boot logs captured instead of printed."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        previous = ab.set_log_stream(self.log)
        self.addCleanup(ab.set_log_stream, previous)
        previous_pb = pb.set_log_stream(self.log)
        self.addCleanup(pb.set_log_stream, previous_pb)


class FakeServerCase(QuietCase):
    """A fresh fake bridge per test.  Override world_kwargs() / faults()."""

    def world_kwargs(self):
        return {}

    def faults(self):
        return None

    def setUp(self):
        super().setUp()
        self.world = fb.FakeWorld(**self.world_kwargs())
        self.server = fb.FakeBridgeServer(self.world, token=TOKEN, shot_dir=self.tmp,
                                          faults=self.faults()).start()
        self.addCleanup(self.server.stop)

    def attach(self, **kw):
        bridge = ab.Bridge.attach(self.server.port, token=TOKEN, run_dir=self.tmp, **kw)
        self.addCleanup(bridge.close)
        return bridge

    def connect(self):
        conn = ab.BridgeConnection(self.server.port, run_dir=self.tmp)
        self.addCleanup(conn.close)
        return conn

    def assertBridgeError(self, kind, code, fn, *args, **kwargs):
        with self.assertRaises(ab.BridgeError) as cm:
            fn(*args, **kwargs)
        self.assertEqual((cm.exception.kind, cm.exception.code), (kind, code), str(cm.exception))
        return cm.exception


# ---------------------------------------------------------------------------
# Framing, ids, timeouts, errors, reconnect
# ---------------------------------------------------------------------------
class TestExpectedDuration(unittest.TestCase):
    def test_durations(self):
        self.assertEqual(ab.expected_duration("ping", {}), 0.0)
        self.assertEqual(ab.expected_duration("key", {"action": "tap"}), 0.1)
        self.assertEqual(ab.expected_duration("key", {"action": "tap", "hold_s": 2}), 2.0)
        self.assertEqual(ab.expected_duration("key", {"action": "press", "hold_s": 2}), 0.0)
        self.assertEqual(ab.expected_duration("axis", {"hold_s": 0.5}), 0.5)
        self.assertEqual(ab.expected_duration("click", {}), 0.0)
        self.assertEqual(ab.expected_duration("screenshot", {}), 10.0)
        self.assertEqual(ab.expected_duration("wait", {"seconds": 3}), 3.0)
        self.assertEqual(ab.expected_duration("wait_until", {}), 10.0)
        self.assertEqual(ab.expected_duration("wait_until", {"timeout_s": 42}), 42.0)
        self.assertEqual(ab.expected_duration("wait", {"seconds": "bad"}), 0.0)
        self.assertEqual(ab.RESPONSE_SLACK_S, 15.0)


class TestConnection(FakeServerCase):
    def test_ids_increment_and_are_echoed(self):
        conn = self.connect()
        self.assertEqual(conn.request("ping")["id"], 1)
        self.assertEqual(conn.request("ping")["id"], 2)
        self.assertEqual(self.server.received[-1]["id"], 2)

    def test_tcp_nodelay(self):
        conn = self.connect()
        self.assertNotEqual(conn.sock.getsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY), 0)

    def test_args_sit_next_to_cmd(self):
        conn = self.connect()
        conn.request("state", sections=["game"], radius=10)
        self.assertEqual(self.server.received[-1],
                         {"id": 1, "cmd": "state", "sections": ["game"], "radius": 10})

    def test_transcript_has_every_request_and_response(self):
        conn = self.connect()
        conn.request("ping")
        conn.request("state", sections=["game"])
        entries = read_jsonl(os.path.join(self.tmp, "bridge.jsonl"))
        sent = [e["msg"] for e in entries if e["dir"] == "send"]
        got = [e["msg"] for e in entries if e["dir"] == "recv"]
        self.assertEqual([m["cmd"] for m in sent], ["ping", "state"])
        self.assertEqual([m["id"] for m in got], [1, 2])
        self.assertTrue(all(m["ok"] for m in got))

    def test_error_kinds(self):
        conn = self.connect()
        self.assertBridgeError("infra", "unknown_command", conn.request, "bogus")
        self.assertBridgeError("infra", "bad_request", conn.request, "wait")
        self.assertBridgeError("gameplay", "bad_key", conn.request, "key", key="Foo")
        err = self.assertBridgeError("gameplay", "timeout", conn.request, "wait_until",
                                     condition="inventory.count>=1", timeout_s=0.2)
        self.assertEqual(err.response["values"], {"inventory.count": 0})
        self.assertBridgeError("infra", "bad_request", conn.request, "wait_until",
                               condition="inventory.count >=")

    def test_long_command_uses_its_duration_for_the_socket_timeout(self):
        conn = self.connect()
        t0 = time.monotonic()
        resp = conn.request("wait", seconds=0.5)
        self.assertGreaterEqual(time.monotonic() - t0, 0.45)
        self.assertGreaterEqual(resp["elapsed_s"], 0.45)

    def test_client_timeout_then_late_reply_is_skipped(self):
        conn = self.connect()
        self.assertBridgeError("infra", "no_response", conn.request, "wait", timeout=0.2,
                               seconds=0.6)
        resp = conn.request("ping")  # the late 'wait' reply (id 1) arrives first and is skipped
        self.assertEqual(resp["id"], 2)
        skipped = [e for e in read_jsonl(os.path.join(self.tmp, "bridge.jsonl"))
                   if e["dir"] == "skip"]
        self.assertEqual(skipped[0]["msg"]["id"], 1)

    def test_oversize_line_gets_bad_request_and_the_connection_survives(self):
        conn = self.connect()
        self.assertBridgeError("infra", "bad_request", conn.request, "ping",
                               pad="x" * (fb.MAX_LINE_BYTES + 100))
        self.assertTrue(conn.request("ping")["ok"])

    def test_too_many_connections(self):
        for _ in range(fb.MAX_CONNECTIONS):
            self.connect().request("ping")
        fifth = self.connect()
        self.assertBridgeError("infra", "too_many_connections", fifth.request, "ping")

    def test_pipelined_requests_answer_in_order_on_one_connection(self):
        with socket.create_connection(("127.0.0.1", self.server.port), timeout=5) as s:
            s.sendall(b'{"id":"a","cmd":"wait","seconds":0.3}\n{"id":"b","cmd":"ping"}\r\n\n')
            buf = b""
            while buf.count(b"\n") < 2:
                buf += s.recv(65536)
        first, second = [json.loads(line) for line in buf.splitlines()[:2]]
        self.assertEqual((first["id"], second["id"]), ("a", "b"))
        self.assertNotIn(b"\r", buf)

    def test_missing_id_and_malformed_json(self):
        with socket.create_connection(("127.0.0.1", self.server.port), timeout=5) as s:
            s.sendall(b'{"cmd":"ping"}\n{nope\n[1,2]\n{"id":5}\n')
            buf = b""
            while buf.count(b"\n") < 4:
                buf += s.recv(65536)
        replies = [json.loads(line) for line in buf.splitlines()[:4]]
        self.assertEqual((replies[0]["id"], replies[0]["ok"]), (None, True))
        self.assertEqual([r.get("error") for r in replies[1:]],
                         ["bad_request", "bad_request", "bad_request"])
        self.assertEqual(replies[3]["id"], 5)

    def test_connections_run_independently(self):
        slow, fast = self.connect(), self.connect()
        thread = threading.Thread(target=slow.request, args=("wait",), kwargs={"seconds": 1.0})
        thread.start()
        self.addCleanup(thread.join)
        time.sleep(0.1)
        t0 = time.monotonic()
        fast.request("ping")
        self.assertLess(time.monotonic() - t0, 0.5)

    def test_disconnect_releases_held_keys(self):
        holder, watcher = self.connect(), self.connect()
        holder.request("key", key="W", action="press")
        held = watcher.request("state", sections=["input"])["state"]["input"]["held"]
        self.assertEqual([h["key"] for h in held], ["W"])
        holder.close()
        end = time.monotonic() + 2
        while time.monotonic() < end:
            held = watcher.request("state", sections=["input"])["state"]["input"]["held"]
            if not held:
                break
            time.sleep(0.05)
        self.assertEqual(held, [])
        x = watcher.request("state", sections=["player"])["state"]["player"]["location"][0]
        time.sleep(0.2)
        x2 = watcher.request("state", sections=["player"])["state"]["player"]["location"][0]
        self.assertEqual(x, x2)

    def test_quit_cancels_pending_commands_everywhere(self):
        waiting, quitter = self.connect(), self.connect()
        errors = []

        def run():
            try:
                waiting.request("wait", seconds=5)
            except ab.BridgeError as e:
                errors.append(e)

        thread = threading.Thread(target=run)
        thread.start()
        time.sleep(0.2)
        self.assertTrue(quitter.request("quit")["ok"])
        thread.join(5)
        self.assertEqual([(e.kind, e.code) for e in errors], [("infra", "cancelled")])
        self.assertTrue(self.server.stopped.wait(3))
        self.assertTrue(self.server.quit_event.is_set())

    def test_connect_failure_is_infra(self):
        port = ab.free_port()
        self.assertBridgeError("infra", "connect_failed", ab.BridgeConnection, port,
                               connect_timeout=1)


class FaultCase(FakeServerCase):
    FAULTS = {}

    def faults(self):
        return fb.Faults(**self.FAULTS)


class TestCrlf(FaultCase):
    FAULTS = {"crlf": True}

    def test_crlf_responses(self):
        self.assertTrue(self.connect().request("ping")["ok"])


class TestSplitWrites(FaultCase):
    FAULTS = {"split_writes": 7}

    def test_split_responses_reassemble(self):
        conn = self.connect()
        self.assertEqual(conn.request("ping")["token"], TOKEN)
        self.assertEqual(conn.request("wait", seconds=0)["id"], 2)


class TestGarbageLines(FaultCase):
    FAULTS = {"garbage_before_response": True, "stale_before_response": True}

    def test_malformed_blank_and_stale_lines_are_skipped(self):
        conn = self.connect()
        self.assertEqual(conn.request("ping")["id"], 1)
        self.assertEqual(conn.request("ping")["id"], 2)
        dirs = [e["dir"] for e in read_jsonl(os.path.join(self.tmp, "bridge.jsonl"))]
        self.assertGreaterEqual(dirs.count("skip"), 4)


class TestDelayedReplies(FaultCase):
    FAULTS = {"response_delay_s": 0.6}

    def test_delay_beyond_timeout_then_recovery(self):
        conn = self.connect()
        self.assertBridgeError("infra", "no_response", conn.request, "ping", timeout=0.2)
        self.assertEqual(conn.request("ping")["id"], 2)


class TestDroppedConnection(FaultCase):
    FAULTS = {"drop_after_requests": 1}

    def test_drop_is_infra_and_reconnect_works(self):
        bridge = self.attach()  # the attach ping is request 1
        self.assertBridgeError("infra", "disconnected", bridge.state)
        self.assertBridgeError("infra", "closed", bridge.ping)
        ping = bridge.reconnect()
        self.assertEqual(ping["token"], TOKEN)


class TestDropOnCommand(FaultCase):
    FAULTS = {"drop_on_cmd": "state"}

    def test_drop_on_state(self):
        bridge = self.attach()
        self.assertBridgeError("infra", "disconnected", bridge.state)


class TestInjectedErrors(FaultCase):
    FAULTS = {"error_replies": {"state": ("failed", "boom"), "mouse": ("not_ready", "early")},
              "no_reply_cmds": {"wait"}}

    def test_injected(self):
        bridge = self.attach()
        err = self.assertBridgeError("infra", "failed", bridge.state)
        self.assertIn("boom", str(err))
        self.assertBridgeError("infra", "not_ready", bridge.mouse, 1, 2)
        self.assertBridgeError("infra", "no_response", bridge.request, "wait", timeout=0.3,
                               seconds=0)


# ---------------------------------------------------------------------------
# Bridge methods
# ---------------------------------------------------------------------------
class TestAttach(FakeServerCase):
    def world_kwargs(self):
        return {"loading_s": 0.6}

    def test_token_mismatch(self):
        self.assertBridgeError("infra", "token_mismatch", ab.Bridge.attach, self.server.port,
                               token="other")

    def test_wait_ready(self):
        bridge = self.attach(wait_ready=True, timeout=5)
        self.assertTrue(bridge.ping_info["ready"])
        self.assertGreaterEqual(bridge.ping_info["real_time_s"], 0.55)

    def test_not_ready_timeout(self):
        self.assertBridgeError("infra", "not_ready", ab.Bridge.attach, self.server.port,
                               token=TOKEN, wait_ready=True, timeout=0.2)

    def test_attached_pid_is_never_owned(self):
        bridge = self.attach()
        self.assertEqual(bridge.pid, os.getpid())
        self.assertIsNone(bridge._owned_pid)


class TestReadyLoop(QuietCase):
    """Bridge.launch's ready loop (review finding 11), run in-process against the fake."""

    def start(self, faults, **world):
        server = fb.FakeBridgeServer(fb.FakeWorld(**world), token=TOKEN, faults=faults).start()
        self.addCleanup(server.stop)
        bridge = ab.Bridge(port=server.port, token=TOKEN, pid=os.getpid())
        self.addCleanup(bridge.close)
        patcher = mock.patch.object(ab, "READY_PING_TIMEOUT_S", 0.3)
        patcher.start()
        self.addCleanup(patcher.stop)
        return server, bridge

    def ready(self, faults, ready_timeout=15.0, **world):
        server, bridge = self.start(faults, **world)
        bridge._ready_loop(ready_timeout)
        return server, bridge

    def test_a_stalled_game_thread_costs_no_extra_connections(self):
        # A 2.4 s stall with 0.3 s pings: reconnecting after each timeout opened 5 sockets, and
        # the bridge then filled its 4 slots with the 4 dead ones and refused the live 5th.
        server, bridge = self.ready(fb.Faults(stall_s=2.4))
        self.assertTrue(bridge.ping_info["ready"])
        self.assertEqual((server.accepted, server.refused), (1, 0))
        self.assertGreater(bridge.ping_info["id"], 1, "the late replies were skipped by id")

    def test_too_many_connections_is_retried(self):
        server, bridge = self.ready(fb.Faults(refuse_connections=2))
        self.assertTrue(bridge.ping_info["ready"])
        self.assertEqual((server.accepted, server.refused), (1, 2))

    def test_a_lost_connection_is_reopened(self):
        # Every connection drops at its 3rd request; the game becomes ready after 0.5 s.
        server, bridge = self.ready(fb.Faults(drop_after_requests=2), loading_s=0.5)
        self.assertTrue(bridge.ping_info["ready"])
        self.assertGreaterEqual(server.accepted, 2)

    def test_a_game_that_never_answers_is_not_ready_on_one_connection(self):
        server, bridge = self.start(fb.Faults(no_reply_cmds={"ping"}))
        with self.assertRaises(ab.BridgeError) as cm:
            bridge._ready_loop(1.0)
        self.assertEqual((cm.exception.kind, cm.exception.code), ("infra", "not_ready"))
        self.assertIn("no response", str(cm.exception))
        self.assertEqual((server.accepted, server.refused), (1, 0))


class TestBridgeCommands(FakeServerCase):
    def test_ping(self):
        ping = self.attach().ping()
        for field in ("bridge_version", "pid", "token", "build", "map", "experience",
                      "experience_loaded", "loading_screen", "has_pawn", "ready", "paused",
                      "real_time_s", "world_time_s", "frame"):
            self.assertIn(field, ping)
        self.assertEqual((ping["bridge_version"], ping["token"]), (1, TOKEN))
        self.assertEqual(ping["experience"], "B_MordecaiArenaExperience")

    def test_state_sections(self):
        bridge = self.attach()
        self.assertEqual(set(bridge.state()), set(fb.SECTIONS))
        self.assertEqual(set(bridge.state(["player"])), {"player"})
        self.assertEqual(set(bridge.state("player, inventory")), {"player", "inventory"})
        self.assertBridgeError("infra", "bad_request", bridge.state, ["nope"])

    def test_state_radius_and_all_actors(self):
        bridge = self.attach()
        near = bridge.state(["actors"], radius=100)["actors"]
        self.assertEqual([a["kind"] for a in near], ["pawn"])
        everything = bridge.state(["actors"], radius=5000)["actors"]
        kinds = [a["kind"] for a in everything]
        self.assertEqual((kinds.count("pickup"), kinds.count("enemy")), (6, 3))
        self.assertFalse([a for a in everything if a["kind"] == "enemy"][0]["has_visible_mesh"])
        walls = bridge.state(["actors"], radius=5000, include_all_actors=True)["actors"]
        self.assertEqual(sum(1 for a in walls if a["kind"] == "other"), 4)

    def test_press_release(self):
        bridge = self.attach()
        resp = bridge.press("W")
        self.assertEqual((resp["keys"], resp["action"], resp["ready"]), (["W"], "press", True))
        self.assertIn("env", resp)
        self.assertEqual([h["key"] for h in bridge.state(["input"])["input"]["held"]], ["W"])
        self.assertTrue(bridge.release("W")["was_held"])
        self.assertFalse(bridge.release("W")["was_held"])
        self.assertGreater(bridge.state(["player"])["player"]["location"][0], 0)

    def test_tap_diagonal_and_hold(self):
        bridge = self.attach()
        resp = bridge.tap(["W", "D"], hold_s=0.3)
        self.assertGreaterEqual(resp["held_s"], 0.29)
        self.assertGreaterEqual(resp["frames"], 2)
        x, y = bridge.state(["player"])["player"]["location"][:2]
        self.assertGreater(x, 50)
        self.assertGreater(y, 50)
        self.assertAlmostEqual(x, y, delta=5)
        self.world.set_position(0, 0)
        bridge.hold("S", 0.5)
        x = bridge.state(["player"])["player"]["location"][0]
        self.assertLess(x, -240)
        self.assertGreater(x, -380)

    def test_key_names(self):
        bridge = self.attach()
        self.assertEqual(bridge.tap("esc")["keys"], ["Escape"])
        self.assertTrue(bridge.state(["pause_menu"])["pause_menu"]["open"])
        self.assertEqual(bridge.tap("1")["keys"], ["One"])
        self.assertBridgeError("gameplay", "bad_key", bridge.tap, "Foo")
        self.assertBridgeError("gameplay", "bad_key", bridge.tap, "LeftMouseButton")
        self.assertBridgeError("gameplay", "bad_key", bridge.tap, "Gamepad_LeftX")
        self.assertBridgeError("infra", "bad_request", bridge.tap, ["W", "A", "S", "D", "F"])

    def test_denied_debug_keys(self):
        bridge = self.attach()
        err = self.assertBridgeError("gameplay", "bad_key", bridge.tap, "K")
        self.assertIn("unsafe", str(err))
        self.assertBridgeError("gameplay", "bad_key", bridge.tap, "F5")
        bridge.tap("K", unsafe=True)
        self.assertTrue(bridge.state(["player"])["player"]["is_dead"])

    def test_axis(self):
        bridge = self.attach()
        resp = bridge.axis({"Gamepad_LeftY": 1.0}, hold_s=0.1)
        self.assertEqual(resp["values"], {"Gamepad_LeftY": 1.0})
        self.assertEqual(bridge.state(["input"])["input"]["held"], [])
        bridge.axis({"Gamepad_RightTriggerAxis": 0.5})
        held = bridge.state(["input"])["input"]["held"]
        self.assertEqual(held, [{"key": "Gamepad_RightTriggerAxis", "kind": "axis", "value": 0.5,
                                 "holders": 1}])
        bridge.axis({"Gamepad_RightTriggerAxis": 0})
        self.assertEqual(bridge.state(["input"])["input"]["held"], [])
        self.assertBridgeError("infra", "bad_request", bridge.axis, {"Gamepad_LeftX": 2})
        self.assertBridgeError("infra", "bad_request", bridge.axis, {"Gamepad_LeftTriggerAxis": -1})
        self.assertBridgeError("gameplay", "bad_key", bridge.axis, {"W": 1})
        # The real stick mappings use invalid key names, so the fake's sticks do not move.
        self.assertEqual(bridge.state(["player"])["player"]["location"][:2], [0.0, 0.0])

    def test_mouse_and_click(self):
        bridge = self.attach()
        resp = bridge.mouse(640, 360)
        self.assertEqual((resp["x"], resp["y"]), (640, 360))
        self.assertEqual(len(resp["abs"]), 2)
        focus = bridge.state(["focus"])["focus"]
        self.assertTrue(focus["virtual_cursor"])
        self.assertEqual(focus["cursor"], [640, 360])
        self.assertEqual(bridge.click()["hit"], "SViewport")
        resp = bridge.click(812, 402, button="Right", hold_s=0.05)
        self.assertEqual((resp["button"], resp["hit"]), ("Right", "SViewport"))
        self.assertGreaterEqual(resp["frames"], 1)
        self.assertRaises(ValueError, bridge.click, 1, None)
        self.assertBridgeError("infra", "bad_request", bridge.mouse, -200000, 10)

    def test_screenshot_paths(self):
        bridge = self.attach()
        first = bridge.screenshot()
        second = bridge.screenshot()
        self.assertEqual(os.path.basename(first), "bridge_01.png")
        self.assertEqual(os.path.basename(second), "bridge_02.png")
        self.assertTrue(ab.file_complete(first))
        named = bridge.screenshot(os.path.join(self.tmp, "sub", "custom"))
        self.assertEqual(named, os.path.join(self.tmp, "sub", "custom.png"))
        self.assertTrue(ab.file_complete(named))

    def test_wait_and_wait_until(self):
        bridge = self.attach()
        self.assertGreaterEqual(bridge.wait(0.3)["elapsed_s"], 0.29)
        resp = bridge.wait_until("player.valid && !pause_menu.open && len(pickups.world)==6",
                                 timeout_s=2)
        self.assertEqual(resp["values"], {"player.valid": True, "pause_menu.open": False,
                                          "len(pickups.world)": 6})
        threading.Timer(0.3, lambda: self.world.open_menu("inventory")).start()
        resp = bridge.wait_until('pause_menu.active_tab=="inventory"', timeout_s=3, poll_s=0.05)
        self.assertGreaterEqual(resp["elapsed_s"], 0.2)

    def test_wait_for(self):
        bridge = self.attach()
        threading.Timer(0.2, lambda: self.world.set_position(100, 0)).start()
        st = bridge.wait_for(lambda s: s["player"]["location"][0] >= 100, timeout_s=3,
                             sections=["player"])
        self.assertEqual(st["player"]["location"][0], 100.0)
        err = self.assertBridgeError("gameplay", "wait_for_timeout", bridge.wait_for,
                                     lambda s: False, 0.2, ["game"])
        self.assertIn("game", err.response)

    def test_identify_through_a_click(self):
        bridge = self.attach()
        self.world.set_position(-400, 0)
        bridge.tap("F")
        bridge.tap("I")
        st = bridge.state(["pause_menu", "inventory"])
        rows = st["pause_menu"]["inventory_tab"]["rows"]
        self.assertEqual(rows[0]["text"], "? Unidentified Purple Amulet")
        self.assertEqual(rows[0]["identify_button"]["center"], [812.0, 402.0])
        resp = bridge.click_button("Identify")
        # `hit` is the deepest widget Slate hit-tests: the label, as the packaged game reported
        # for this click (Saved/Playtests/20261020-192656-us079/bridge.jsonl); the button clicked.
        self.assertEqual(resp["hit"], "STextBlock")
        self.assertEqual(resp["target"]["id"], rows[0]["instance_id"])
        st = bridge.state(["pause_menu", "inventory"])
        self.assertEqual(st["pause_menu"]["inventory_tab"]["rows"][0]["text"], "Mysterious Amulet")
        self.assertIsNone(st["pause_menu"]["inventory_tab"]["rows"][0]["identify_button"])
        self.assertTrue(st["inventory"]["items"][0]["identified"])
        err = self.assertBridgeError("gameplay", "no_button", bridge.click_button, "Identify")
        self.assertIn("Inventory", str(err))

    def test_click_button_by_id_and_case(self):
        bridge = self.attach()
        self.world.set_position(-400, 0)
        bridge.tap("F")
        bridge.tap("I")
        instance = bridge.state(["inventory"])["inventory"]["items"][0]["instance_id"]
        bridge.click_button(button_id=instance.lower())
        self.assertTrue(bridge.state(["inventory"])["inventory"]["items"][0]["identified"])
        bridge.click_button("character")  # case-insensitive fallback
        self.assertEqual(bridge.state(["pause_menu"])["pause_menu"]["active_tab"], "character")
        self.assertRaises(ValueError, bridge.click_button)

    def test_a_click_off_the_button_does_nothing(self):
        bridge = self.attach()
        self.world.set_position(-400, 0)
        bridge.tap("F")
        bridge.tap("I")
        self.assertEqual(bridge.click(812, 450)["hit"], "SBorder")
        self.assertFalse(bridge.state(["inventory"])["inventory"]["items"][0]["identified"])

    def test_menu_blocks_game_keys(self):
        bridge = self.attach()
        bridge.tap("I")
        bridge.press("W")
        time.sleep(0.2)
        st = bridge.state(["player", "input", "focus", "game"])
        self.assertEqual(st["player"]["location"][:2], [0.0, 0.0])
        self.assertEqual([h["key"] for h in st["input"]["held"]], ["W"])
        self.assertEqual(st["input"]["pressed"], [])
        self.assertEqual(st["focus"]["commonui_input_mode"], "Menu")
        self.assertTrue(st["game"]["paused"])
        bridge.release("W")
        bridge.tap("Escape")
        self.assertFalse(bridge.state(["pause_menu"])["pause_menu"]["open"])

    def test_full_pickup_loop(self):
        bridge = self.attach()
        route = [("TestPickup_HealthPotion", (400, 0)), ("TestPickup_IronOre", (300, 300)),
                 ("TestPickup_TownStone", (0, 420)), ("TestPickup_UpgradeKey", (-300, 300)),
                 ("TestPickup_MysteriousAmulet", (-400, 0)), (None, (-250, -150)),
                 ("TestPickup_RustySword", (-300, -300))]
        for label, (x, y) in route:
            result = bridge.walk_to(x, y)
            self.assertLessEqual(result["dist"], 50)
            if label is None:
                continue
            bridge.wait_until(f'pickups.focused.label=="{label}"', timeout_s=2)
            prompt = bridge.state(["pickups"])["pickups"]["prompt"]
            self.assertTrue(prompt["visible"])
            self.assertTrue(prompt["text"].startswith("[F] Pick up "))
            bridge.tap("F")
            bridge.wait_until(f'pickups.focused.label!="{label}"', timeout_s=2)
        inv = bridge.state(["inventory"])["inventory"]
        self.assertEqual((inv["count"], inv["ledger_count"]), (4, 2))
        bridge.tap("I")
        bridge.wait_until('pause_menu.open==true && pause_menu.active_tab=="inventory" && '
                          'len(pause_menu.inventory_tab.rows)==4', timeout_s=2)
        rows = bridge.state(["pause_menu"])["pause_menu"]["inventory_tab"]
        self.assertEqual([r["text"] for r in rows["rows"]],
                         ["Upgrade Key", "Rusty Sword", "? Unidentified Purple Amulet",
                          "Health Potion"])
        self.assertEqual(rows["ledger_rows"], ["Iron Ore  x1", "Town Stone  x1"])

    def test_quit_stops_the_bridge(self):
        bridge = self.attach()
        result = bridge.quit(timeout_s=1)
        self.assertTrue(result["ok"])
        self.assertEqual(result["response"]["cmd"], "quit")
        self.assertTrue(self.server.stopped.wait(3))


class TestHeldKeys(FakeServerCase):
    """The C++ held-input rules the fake mirrors (review findings 9 and 10)."""

    def held(self, conn):
        return conn.request("state", sections=["input"])["state"]["input"]["held"]

    def key_log(self, key):
        return [e[1] for e in self.world.input_log if e[2] == key]

    def test_press_or_tap_of_a_key_this_connection_holds_is_bad_request(self):
        bridge = self.attach()
        bridge.press("W")
        for fn, args in ((bridge.tap, ("W",)), (bridge.press, ("W",)),
                         (bridge.tap, (["D", "W"],)), (bridge.hold, ("W", 0.2))):
            err = self.assertBridgeError("infra", "bad_request", fn, *args)
            self.assertIn("key 'W' is already held by this connection; release it first",
                          str(err))
        # The refused requests changed nothing: W is still held, D never went down.
        held = bridge.state(["input"])["input"]["held"]
        self.assertEqual([(h["key"], h["holders"]) for h in held], [("W", 1)])
        self.assertEqual(self.key_log("D"), [])
        self.assertTrue(bridge.release("W")["was_held"])
        self.assertEqual(bridge.tap("W")["keys"], ["W"])
        self.assertEqual(self.key_log("W"), ["down", "up", "down", "up"])

    def test_hold_s_is_range_checked_for_press_and_release_but_ignored(self):
        conn = self.connect()
        self.assertEqual(conn.request("key", key="W", action="press", hold_s=5)["action"], "press")
        self.assertTrue(conn.request("key", key="W", action="release", hold_s=5)["was_held"])
        self.assertBridgeError("infra", "bad_request", conn.request, "key", key="W",
                               action="press", hold_s=99)

    def test_held_input_is_the_union_of_the_connections(self):
        a, b = self.connect(), self.connect()
        a.request("key", key="W", action="press")
        b.request("key", key="W", action="tap", hold_s=0.1)  # b does not hold W: allowed
        self.assertEqual(self.held(a), [{"key": "W", "kind": "key", "value": 1.0, "holders": 1}])
        x0 = self.world.pos[0]
        time.sleep(0.15)
        self.assertGreater(self.world.pos[0], x0, "a's W must still drive the pawn")
        b.request("key", key="W", action="press")
        self.assertEqual(self.held(b), [{"key": "W", "kind": "key", "value": 1.0, "holders": 2}])
        self.assertTrue(b.request("key", key="W", action="release")["was_held"])
        self.assertEqual(self.held(b)[0]["holders"], 1)
        self.assertEqual(self.key_log("W"), ["down"])  # one down (0->1 holders), no up yet
        a.request("key", key="W", action="release")
        self.assertEqual(self.held(a), [])
        self.assertEqual(self.key_log("W"), ["down", "up"])

    def test_a_disconnecting_holder_leaves_the_key_down_for_the_other(self):
        a, b = self.connect(), self.connect()
        a.request("key", key="D", action="press")
        b.request("key", key="D", action="press")
        b.close()
        end = time.monotonic() + 2
        while time.monotonic() < end and self.held(a)[0]["holders"] != 1:
            time.sleep(0.05)
        self.assertEqual(self.held(a), [{"key": "D", "kind": "key", "value": 1.0, "holders": 1}])
        self.assertEqual(self.key_log("D"), ["down"])

    def test_axes_aggregate_by_the_largest_magnitude(self):
        a, b = self.connect(), self.connect()
        a.request("axis", values={"Gamepad_LeftY": 0.3})
        b.request("axis", values={"Gamepad_LeftY": -0.8})
        self.assertEqual(self.held(a), [{"key": "Gamepad_LeftY", "kind": "axis", "value": -0.8,
                                         "holders": 2}])
        b.request("axis", values={"Gamepad_LeftY": 0})
        self.assertEqual(self.held(a), [{"key": "Gamepad_LeftY", "kind": "axis", "value": 0.3,
                                         "holders": 1}])


class TestPeerEof(FakeServerCase):
    """Specs/agent_bridge_v1.md section 2 'Peer EOF', as the C++ FMordecaiAgentBridgeServer does
    it: after a client's EOF the requests it already sent still run and reply, the connection
    stops counting toward the 4-connection limit at once, and it closes -- releasing everything
    it held -- only when those requests have finished.  Only quit cancels pending commands."""

    def raw(self):
        sock = socket.create_connection(("127.0.0.1", self.server.port), timeout=5)
        self.addCleanup(sock.close)
        return sock

    @staticmethod
    def read_lines(sock, n, timeout=5.0):
        sock.settimeout(timeout)
        buf = b""
        while buf.count(b"\n") < n:
            chunk = sock.recv(65536)
            if not chunk:
                break
            buf += chunk
        return [json.loads(line) for line in buf.splitlines() if line.strip()]

    def wait_until_true(self, fn, timeout=3.0):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            if fn():
                return True
            time.sleep(0.02)
        return fn()

    def key_events(self, key):
        with self.world.lock:
            return [(t, e) for t, e, k in self.world.input_log if k == key]

    def held_keys(self):
        with self.world.lock:
            return sorted(self.world._held_union())

    def test_a_client_that_closes_during_a_tap_keeps_its_input_until_the_tap_ends(self):
        sock = self.raw()
        sock.sendall(b'{"id":1,"cmd":"key","key":"W","action":"press"}\n'
                     b'{"id":2,"cmd":"key","key":"D","action":"tap","hold_s":1.0}\n')
        self.assertTrue(self.read_lines(sock, 1)[0]["ok"])  # the press reply: nothing left unread
        self.assertTrue(self.wait_until_true(lambda: "D" in self.held_keys()))
        sock.close()  # a killed client: EOF while its tap runs
        closed_at = self.world.real_time()
        self.assertTrue(self.wait_until_true(lambda: self.server.open_connection_count() == 0),
                        "the EOF connection must stop counting toward the limit at once")
        time.sleep(0.3)
        # The tap still runs, so the connection still holds W and D (no up event yet).
        self.assertEqual(self.server.connection_count(), 1)
        self.assertEqual(self.held_keys(), ["D", "W"])
        self.assertEqual([e for _, e in self.key_events("W")], ["down"])
        self.assertEqual([e for _, e in self.key_events("D")], ["down"])
        # The tap completes (D up), then the connection closes and releases W.
        self.assertTrue(self.wait_until_true(lambda: self.server.connection_count() == 0, 4.0))
        self.assertEqual(self.held_keys(), [])
        (d_down, _), (d_up, d_ev) = self.key_events("D")
        (_, _), (w_up, w_ev) = self.key_events("W")
        self.assertEqual((d_ev, w_ev), ("up", "up"))
        self.assertGreaterEqual(d_up - d_down, 0.95, "the tap was cut short")
        self.assertGreaterEqual(w_up, d_up, "W must be released after the tap, not at the EOF")
        self.assertGreater(w_up - closed_at, 0.4, "W was released at the EOF")

    def test_a_client_that_closes_during_wait_until_frees_its_slot_at_once(self):
        sock = self.raw()
        sock.sendall(b'{"id":1,"cmd":"wait_until","condition":"inventory.count>=99",'
                     b'"timeout_s":30}\n')
        self.assertTrue(self.wait_until_true(lambda: self.server.received))
        time.sleep(0.1)
        sock.close()  # e.g. a CLI wait-until killed by a tool-call timeout
        self.assertTrue(self.wait_until_true(lambda: self.server.open_connection_count() == 0))
        # The slot is free while the wait_until still runs: 4 new clients connect.
        for _ in range(fb.MAX_CONNECTIONS):
            self.assertTrue(self.connect().request("ping")["ok"])
        self.assertEqual(self.server.connection_count(), fb.MAX_CONNECTIONS + 1,
                         "the abandoned wait_until must still be running")
        self.assertEqual(self.server.open_connection_count(), fb.MAX_CONNECTIONS)
        self.assertBridgeError("infra", "too_many_connections", self.connect().request, "ping")

    def test_a_half_closed_client_gets_every_reply_including_quit(self):
        sock = self.raw()
        sock.sendall(b'{"id":"w","cmd":"wait","seconds":0.2}\n{"id":"q","cmd":"quit"}\n')
        sock.shutdown(socket.SHUT_WR)  # like: echo '{"cmd":"quit"}' | ncat 127.0.0.1 <port>
        replies = self.read_lines(sock, 2)
        self.assertEqual([(r["id"], r["ok"], r["cmd"]) for r in replies],
                         [("w", True, "wait"), ("q", True, "quit")])
        self.assertTrue(self.server.stopped.wait(3))
        self.assertTrue(self.server.quit_event.is_set())


class TestCppParity(FakeServerCase):
    """Argument rules the fake copies from MordecaiAgentBridgeProtocol.cpp / Predicate.cpp."""

    def test_condition_literals_and_len(self):
        conn = self.connect()
        for cond, why in (("inventory.count>=1.", "malformed number at position 17"),
                          ("inventory.count==1.e2", "malformed number"),
                          ("len(true)>0", "len() needs a path"),
                          ("len(null)==0", "len() needs a path"),
                          ("len(5)==0", "len() needs a path")):
            err = self.assertBridgeError("infra", "bad_request", conn.request, "wait_until",
                                         condition=cond, timeout_s=0.1)
            self.assertIn("bad condition: ", str(err), cond)
            self.assertIn(why, str(err), cond)
        for cond in ("inventory.count>=-0.5", "inventory.count<.5", "-.5<inventory.count",
                     "1.5e1==15", "len(pickups.world)==6"):
            self.assertTrue(conn.request("wait_until", condition=cond, timeout_s=1)["ok"], cond)
        self.assertRaises(fb.ConditionError, fb.Condition, "a==2.")
        self.assertEqual(fb.Condition("a==2.5").evaluate({"a": 2.5}), (True, {"a": 2.5}))

    def test_pixels_within_100000_are_accepted_and_off_viewport_clicks_hit_nothing(self):
        bridge = self.attach()
        self.assertEqual(bridge.mouse(-5, 10)["x"], -5)
        self.assertEqual(bridge.mouse(100000, -100000)["y"], -100000)
        self.assertIsNone(bridge.click()["hit"])  # at the off-viewport cursor
        for x, y in ((-1, 10), (5000, 5000), (640, -3)):
            self.assertIsNone(bridge.click(x, y)["hit"], (x, y))
        self.assertEqual(bridge.click(640, 360)["hit"], "SViewport")
        err = self.assertBridgeError("infra", "bad_request", bridge.mouse, 100001, 0)
        self.assertIn("'x' must be in -100000..100000 (got 100001)", str(err))
        for cmd, args in (("mouse", {"x": 5}), ("mouse", {}), ("click", {"y": 5}),
                          ("mouse", {"x": 5, "y": None})):
            err = self.assertBridgeError("infra", "bad_request", bridge.request, cmd, **args)
            self.assertIn("both 'x' and 'y' are required (viewport pixels)", str(err))
        # null counts as absent: a click with x:null, y:null clicks at the cursor
        self.assertIn("hit", bridge.request("click", x=None, y=None))

    def test_radius_must_be_greater_than_0(self):
        conn = self.connect()
        err = self.assertBridgeError("infra", "bad_request", conn.request, "state",
                                     sections=["actors"], radius=0)
        self.assertIn("'radius' must be greater than 0", str(err))
        err = self.assertBridgeError("infra", "bad_request", conn.request, "state", radius=-1)
        self.assertIn("'radius' must be in 0..1e+07 (got -1)", str(err))
        self.assertBridgeError("infra", "bad_request", conn.request, "state", radius=2e7)
        self.assertTrue(conn.request("state", sections=["actors"], radius=0.5)["ok"])
        self.assertTrue(conn.request("state", sections=["game"], radius=None)["ok"])

    def test_conditions_longer_than_4096_characters_are_refused(self):
        conn = self.connect()
        fits = "game.ready" + " " * (fb.MAX_CONDITION_LENGTH - len("game.ready"))
        self.assertEqual(len(fits), 4096)
        self.assertTrue(conn.request("wait_until", condition=fits, timeout_s=1)["ok"])
        err = self.assertBridgeError("infra", "bad_request", conn.request, "wait_until",
                                     condition=fits + " ", timeout_s=0.1)
        self.assertIn("bad condition: condition longer than 4096 characters", str(err))
        # Checked before tokenizing: even garbage gets the length message.
        err = self.assertBridgeError("infra", "bad_request", conn.request, "wait_until",
                                     condition="@" * 5000, timeout_s=0.1)
        self.assertIn("condition longer than 4096 characters", str(err))

    def test_nesting_deeper_than_64_is_refused(self):
        conn = self.connect()
        ok = ("!" * 64 + "game.ready", "(" * 64 + "game.ready" + ")" * 64,
              "!(" * 32 + "game.ready" + ")" * 32)
        for cond in ok:  # 64 levels: an even number of '!' keeps game.ready true
            self.assertTrue(conn.request("wait_until", condition=cond, timeout_s=1)["ok"], cond)
        for cond in ("!" * 65 + "game.ready", "(" * 65 + "game.ready" + ")" * 65,
                     "!(" * 32 + "!game.ready" + ")" * 32, "!" * 3000 + "game.ready"):
            err = self.assertBridgeError("infra", "bad_request", conn.request, "wait_until",
                                         condition=cond, timeout_s=0.1)
            self.assertIn("bad condition: condition nested too deeply", str(err), cond[:80])
        # A flat chain is not nesting: 1365 terms parse and evaluate without recursing per term
        # (a left-deep tree raised RecursionError, which the fake turned into 'failed').
        for op, want in (("&&", True), ("||", True)):
            chain = op.join(["1"] * 1365)
            self.assertLessEqual(len(chain), fb.MAX_CONDITION_LENGTH)
            self.assertTrue(conn.request("wait_until", condition=chain, timeout_s=1)["ok"], op)
            self.assertEqual(fb.Condition(chain).evaluate({}), (want, {}))
        chain = "game.ready&&" + "&&".join(["game.ready"] * 300)
        self.assertEqual(conn.request("wait_until", condition=chain, timeout_s=1)["values"],
                         {"game.ready": True})
        self.assertEqual(fb.Condition("||".join(["0"] * 1365)).evaluate({}), (False, {}))

    def test_a_key_listed_twice_is_bad_request(self):
        bridge = self.attach()
        for keys, name in ((["W", "W"], "W"), (["W", "w"], "W"), (["esc", "Escape"], "Escape"),
                           (["D", "1", "One"], "One")):
            err = self.assertBridgeError("infra", "bad_request", bridge.tap, keys)
            self.assertIn(f"key '{name}' is listed twice", str(err), keys)
            self.assertBridgeError("infra", "bad_request", bridge.press, keys)
        # Refused before anything went down.
        self.assertEqual(self.world.input_log, [])
        self.assertEqual(bridge.state(["input"])["input"]["held"], [])
        err = self.assertBridgeError("infra", "bad_request", bridge.axis,
                                     {"Gamepad_LeftY": 1, "gamepad_lefty": 0.5})
        self.assertIn("axis 'Gamepad_LeftY' is listed twice", str(err))
        self.assertEqual(bridge.tap(["W", "D"])["keys"], ["W", "D"])

    def test_click_holds_0_05_s_by_default(self):
        bridge = self.attach()
        resp = bridge.click(640, 360)
        self.assertNotIn("hold_s", self.server.received[-1])
        self.assertGreaterEqual(resp["held_s"], 0.045)

    def test_tab_ids_are_lowercase(self):
        bridge = self.attach()
        bridge.tap("I")
        st = bridge.state(["pause_menu", "widgets"])
        ids = [t["id"] for t in st["pause_menu"]["tabs"]]
        self.assertEqual(ids, [i.lower() for i in ids])
        self.assertEqual(st["pause_menu"]["active_tab"], "inventory")
        menu = [w for w in st["widgets"] if w["class"] == "MordecaiPauseMenuWidget"][0]
        self.assertEqual([b["id"] for b in menu["buttons"]], ids)
        bridge.click_button(button_id="CHARACTER")  # ids compare case-insensitively
        self.assertEqual(bridge.state(["pause_menu"])["pause_menu"]["active_tab"], "character")


class TestEscapeBug(FakeServerCase):
    def world_kwargs(self):
        return {"escape_closes_menu": False}

    def test_escape_does_not_close_the_menu(self):
        bridge = self.attach()
        bridge.tap("I")
        bridge.tap("Escape")
        err = self.assertBridgeError("gameplay", "timeout", bridge.wait_until,
                                     "pause_menu.open==false", timeout_s=0.3)
        self.assertEqual(err.response["values"], {"pause_menu.open": True})


class TestScreenshotFaults(QuietCase):
    def start(self, mode, **faults):
        server = fb.FakeBridgeServer(token=TOKEN, shot_dir=self.tmp,
                                     faults=fb.Faults(screenshot=mode, **faults)).start()
        self.addCleanup(server.stop)
        bridge = ab.Bridge.attach(server.port, token=TOKEN)
        self.addCleanup(bridge.close)
        return bridge

    def test_path_returned_only_when_complete(self):
        bridge = self.start("reply_early", screenshot_complete_delay_s=0.4)
        t0 = time.monotonic()
        path = bridge.screenshot()
        self.assertGreaterEqual(time.monotonic() - t0, 0.3)
        self.assertTrue(ab.file_complete(path))

    def test_truncated_file_is_infra(self):
        bridge = self.start("truncated")
        with self.assertRaises(ab.BridgeError) as cm:
            bridge.screenshot(complete_timeout_s=0.3)
        self.assertEqual((cm.exception.kind, cm.exception.code), ("infra", "screenshot_incomplete"))

    def test_bridge_timeout_is_gameplay_timeout(self):
        bridge = self.start("timeout", screenshot_timeout_s=0.3)
        with self.assertRaises(ab.BridgeError) as cm:
            bridge.screenshot()
        self.assertEqual((cm.exception.kind, cm.exception.code), ("gameplay", "timeout"))

    def test_failed(self):
        bridge = self.start("failed")
        with self.assertRaises(ab.BridgeError) as cm:
            bridge.screenshot()
        self.assertEqual((cm.exception.kind, cm.exception.code), ("infra", "failed"))

    def test_file_complete(self):
        good = os.path.join(self.tmp, "a.png")
        with open(good, "wb") as f:
            f.write(fb.make_png())
        self.assertTrue(ab.file_complete(good))
        self.assertFalse(ab.file_complete(good, expected_bytes=10 ** 6))
        half = os.path.join(self.tmp, "b.png")
        with open(half, "wb") as f:
            f.write(fb.make_png()[:30])
        self.assertFalse(ab.file_complete(half))
        self.assertFalse(ab.file_complete(os.path.join(self.tmp, "missing.png")))


# ---------------------------------------------------------------------------
# walk_to
# ---------------------------------------------------------------------------
class WalkCase(FakeServerCase):
    WORLD = {}

    def world_kwargs(self):
        return dict(self.WORLD)

    def walk_error(self, code, *args, **kwargs):
        bridge = self.attach()
        return self.assertBridgeError("gameplay", code, bridge.walk_to, *args, **kwargs)


class TestWalkToConverges(WalkCase):
    def test_converges(self):
        bridge = self.attach()
        for x, y in [(400, 0), (300, 300), (0, 420), (-300, 300), (-400, 0), (-250, -150)]:
            result = bridge.walk_to(x, y)
            self.assertLessEqual(result["dist"], 50)
            self.assertGreaterEqual(result["pulses"], 1)
            self.assertEqual(result["side_steps"], 0)
            px, py = bridge.state(["player"])["player"]["location"][:2]
            self.assertLessEqual(((px - x) ** 2 + (py - y) ** 2) ** 0.5, 50.5)

    def test_tighter_tolerance(self):
        result = self.attach().walk_to(250, -120, tol=30)
        self.assertLessEqual(result["dist"], 30)

    def test_already_there(self):
        self.assertEqual(self.attach().walk_to(20, 0),
                         {"final": [0.0, 0.0], "dist": 20.0, "pulses": 0, "side_steps": 0})

    def test_only_input_commands_are_sent(self):
        self.attach().walk_to(300, 300)
        cmds = {e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.tmp, "bridge.jsonl"))
                if e["dir"] == "send"}
        self.assertEqual(cmds, {"ping", "state", "key"})

    def test_menu_open_before(self):
        self.world.open_menu("inventory")
        self.walk_error("menu_open", 400, 0)


class TestWalkToStuck(WalkCase):
    WORLD = {"obstacles": [(60, 100, -2000, 2000)]}

    def test_stuck(self):
        err = self.walk_error("stuck", 150, 0, timeout_s=60)
        self.assertRegex(str(err), r"^stuck at \(-?\d+, -?\d+\), target \(150, 0\), dist \d+$")
        self.assertEqual(err.response["side_steps"], 3)


class TestWalkToTimeout(WalkCase):
    WORLD = {"speed": 30}

    def test_explicit_timeout(self):
        err = self.walk_error("walk_timeout", 400, 0, timeout_s=1.0)
        self.assertIn("timed out after 1.0s", str(err))

    def test_default_timeout_is_3_dist_over_600_plus_3(self):
        t0 = time.monotonic()
        err = self.walk_error("walk_timeout", 300, 0)  # 12 uu per 0.4 s pulse: slow, not stuck
        self.assertIn("timed out after 4.5s", str(err))
        self.assertGreaterEqual(time.monotonic() - t0, 4.4)


class TestWalkToMappingMismatch(WalkCase):
    WORLD = {"mapping": {"W": (-1, 0), "S": (1, 0), "D": (0, -1), "A": (0, 1)}}

    def test_mismatch(self):
        err = self.walk_error("mapping_mismatch", 400, 0)
        self.assertIn("movement mapping mismatch", str(err))


class TestWalkToNoMovement(WalkCase):
    WORLD = {"mapping": {}}

    def test_no_movement_is_a_mismatch(self):
        self.walk_error("mapping_mismatch", 0, 400)


class TestWalkToYaw(WalkCase):
    WORLD = {"control_yaw": 15.0}

    def test_rotated_basis(self):
        err = self.walk_error("yaw_not_zero", 400, 0)
        self.assertIn("15.0", str(err))

    def test_yaw_normalization(self):
        self.assertEqual(ab._normalize_yaw(360.5), 0.5)
        self.assertEqual(ab._normalize_yaw(-359.5), 0.5)
        self.assertEqual(ab._normalize_yaw(190), -170)


class TestWalkToSmallYaw(WalkCase):
    WORLD = {"control_yaw": 359.6}

    def test_within_one_degree(self):
        self.assertLessEqual(self.attach().walk_to(200, 0)["dist"], 50)


class TestWalkToDeath(WalkCase):
    WORLD = {"hazards": [(150, 200, -2000, 2000)]}

    def test_death_aborts(self):
        err = self.walk_error("player_dead", 400, 0)
        self.assertIn("dead", str(err))


class TestWalkToMenuOpensMidWalk(WalkCase):
    def test_menu_opens(self):
        self.world.after(0.3, lambda w: w.open_menu("inventory"))
        self.walk_error("menu_open", 800, 0)


class TestWalkToNoPlayer(WalkCase):
    WORLD = {"boot_s": 30}

    def test_no_player(self):
        self.walk_error("no_player", 400, 0)


class TestWalkToKeysHeld(WalkCase):
    def test_a_key_this_connection_holds_fails_fast(self):
        bridge = self.attach()
        bridge.press("W")
        err = self.assertBridgeError("gameplay", "keys_held", bridge.walk_to, 400, 0)
        self.assertIn("W is already held", str(err))
        self.assertIn("release it first", str(err))
        self.assertFalse([r for r in self.server.received
                          if r.get("cmd") == "key" and r.get("action") == "tap"],
                         "walk_to sent a tap the bridge would refuse")
        bridge.release("W")
        self.assertLessEqual(bridge.walk_to(400, 0)["dist"], 50)

    def test_a_key_another_connection_holds_fails_fast(self):
        self.connect().request("key", keys=["S", "D"], action="press")
        err = self.walk_error("keys_held", 0, 400)
        self.assertIn("S+D are already held", str(err))


class TestFindButtons(unittest.TestCase):
    INVALID = {"valid": False, "reason": "no world yet"}

    def test_invalid_or_malformed_sections_yield_no_buttons(self):
        b = ab.Bridge()
        self.assertEqual(b.find_buttons({"pause_menu": self.INVALID, "widgets": self.INVALID}), [])
        self.assertEqual(b.find_buttons({}), [])
        self.assertEqual(b.find_buttons({
            "pause_menu": {"valid": True, "open": True, "inventory_tab": {"rows": self.INVALID}},
            "widgets": [None, "x", {"buttons": self.INVALID},
                        {"buttons": [None, {"label": "OK", "center": ["a", 1]}]}]}), [])
        ok = {"label": "OK", "id": "ok", "center": [10, 20], "enabled": True}
        self.assertEqual(b.find_buttons({"pause_menu": self.INVALID,
                                         "widgets": [{"name": "W", "buttons": [ok]}]}),
                         [dict(ok, source="widget W")])

    def test_click_button_before_the_world_exists_is_no_button(self):
        b = ab.Bridge()
        with mock.patch.object(b, "state", return_value={"pause_menu": self.INVALID,
                                                         "widgets": self.INVALID}):
            with self.assertRaises(ab.BridgeError) as cm:
                b.click_button("Identify")
        self.assertEqual((cm.exception.kind, cm.exception.code), ("gameplay", "no_button"))


@unittest.skipUnless(WINDOWS, "process identity uses the Win32 API")
class TestProcessIdentity(QuietCase):
    """Session pids are identified by creation time (review finding 12)."""

    def child(self):
        p = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        self.addCleanup(lambda: p.poll() is None and p.kill())
        return p

    def test_creation_time_identifies_one_process(self):
        mine = ab.process_create_time(os.getpid())
        self.assertTrue(mine and mine.isdigit(), mine)
        self.assertEqual(ab.process_create_time(os.getpid()), mine)
        p = self.child()
        theirs = ab.process_create_time(p.pid)
        self.assertNotEqual(theirs, mine)
        self.assertTrue(ab.pid_alive(p.pid, sys.executable, theirs))
        self.assertTrue(ab.pid_alive(p.pid, created=int(theirs)))  # an int works too
        self.assertFalse(ab.pid_alive(p.pid, sys.executable, str(int(theirs) + 1)))  # reused pid
        self.assertTrue(ab.is_session_game(p.pid, sys.executable, theirs))
        self.assertTrue(ab.is_session_game(p.pid, sys.executable))  # old session: exe name
        self.assertFalse(ab.is_session_game(p.pid, sys.executable, "1"))
        self.assertFalse(ab.is_session_game(p.pid, "LyraGame.exe", theirs))
        self.assertFalse(ab.is_session_game(p.pid))  # nothing to identify it by

    def test_kill_pid_refuses_a_pid_with_another_creation_time(self):
        p = self.child()
        self.assertIn("refused", ab.kill_pid(p.pid, sys.executable, "1"))
        time.sleep(0.3)
        self.assertIsNone(p.poll(), "a pid with another creation time was killed")
        ab.kill_pid(p.pid, sys.executable, ab.process_create_time(p.pid))
        p.wait(timeout=10)

    def test_access_denied_is_never_the_session_game(self):
        denied = {"pid": 4242, "error": ab.ERROR_ACCESS_DENIED, "alive": False, "image": None,
                  "created": None}
        with mock.patch.object(ab, "query_process", return_value=denied):
            self.assertFalse(ab.pid_alive(4242, "LyraGame.exe"))
            self.assertFalse(ab.pid_alive(4242, created="1234"))
            self.assertTrue(ab.pid_alive(4242))  # bare existence: something holds the pid
            self.assertFalse(ab.is_session_game(4242, "LyraGame.exe", "1234"))
            self.assertIn("refused", ab.kill_pid(4242, "LyraGame.exe"))

    def test_attached_quit_never_kills_a_pid_it_cannot_confirm(self):
        # The session's game is gone and its pid now belongs to another python.exe: the exe
        # name matches, the creation time does not.
        p = self.child()
        server = fb.FakeBridgeServer(token=TOKEN).start()
        self.addCleanup(server.stop)
        bridge = ab.Bridge.attach(server.port, token=TOKEN, pid=p.pid, exe=sys.executable,
                                  pid_created="1")
        self.addCleanup(bridge.close)
        self.assertFalse(bridge.is_alive())
        result = bridge.quit(timeout_s=0.5)
        self.assertIn(f"pid {p.pid} exited", result["shutdown"])  # the recorded process is gone
        time.sleep(0.3)
        self.assertIsNone(p.poll(), "attached quit killed a pid with another creation time")


class TestWasd(unittest.TestCase):
    def test_keys(self):
        self.assertEqual(ab.wasd_for(100, 0, 25), (["W"], (1.0, 0.0)))
        self.assertEqual(ab.wasd_for(-100, 0, 25)[0], ["S"])
        self.assertEqual(ab.wasd_for(0, 100, 25)[0], ["D"])
        self.assertEqual(ab.wasd_for(0, -100, 25)[0], ["A"])
        keys, direction = ab.wasd_for(100, -100, 25)
        self.assertEqual(keys, ["W", "A"])
        self.assertAlmostEqual(direction[0], 2 ** -0.5)
        self.assertEqual(ab.wasd_for(10, -15, 25)[0], ["A"])  # both small: the larger wins


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def run_cli(*args, timeout=90):
    r = subprocess.run([sys.executable, AGENT_BRIDGE] + [str(a) for a in args],
                       capture_output=True, text=True, timeout=timeout)
    try:
        out = json.loads(r.stdout)
    except ValueError:
        out = None
    return r.returncode, out, r.stdout + r.stderr


class CliCase(FakeServerCase):
    """CLI subprocesses against an in-process fake, through a session file."""

    def setUp(self):
        super().setUp()
        self.session = os.path.join(self.tmp, "session.json")
        # exe None: the CLI may wait on this pid but never kills it (it is this test process).
        ab.write_session(self.session, {"pid": os.getpid(), "port": self.server.port,
                                        "token": TOKEN, "run_dir": self.tmp, "exe": None})

    def cli(self, *args, **kw):
        return run_cli("--session", self.session, *args, **kw)


class TestCli(CliCase):
    def test_ping_and_state(self):
        code, out, text = self.cli("ping")
        self.assertEqual(code, 0, text)
        self.assertEqual(out["token"], TOKEN)
        code, out, text = self.cli("state", "--sections", "player,inventory")
        self.assertEqual(code, 0, text)
        self.assertEqual(set(out["state"]), {"player", "inventory"})

    def test_press_and_bad_key(self):
        code, out, text = self.cli("press", "W", "D", "--hold", "0.2")
        self.assertEqual(code, 0, text)
        self.assertEqual(out["keys"], ["W", "D"])
        code, out, text = self.cli("press", "Foo")
        self.assertEqual(code, 1, text)
        self.assertEqual((out["kind"], out["error"]), ("gameplay", "bad_key"))
        code, out, _ = self.cli("press", "K")
        self.assertEqual(code, 1)
        code, out, _ = self.cli("press", "K", "--unsafe")
        self.assertEqual(code, 0)

    def test_walk_to_and_wait_until(self):
        code, out, text = self.cli("walk-to", "400", "0", "--tol", "50")
        self.assertEqual(code, 0, text)
        self.assertLessEqual(out["dist"], 50)
        code, out, text = self.cli("press", "F")
        self.assertEqual(code, 0, text)
        code, out, text = self.cli("wait-until", "inventory.count>=1", "--timeout", "2")
        self.assertEqual(code, 0, text)
        self.assertEqual(out["values"], {"inventory.count": 1})
        code, out, text = self.cli("wait-until", "inventory.count>=4", "--timeout", "0.3")
        self.assertEqual(code, 1, text)
        self.assertEqual(out["error"], "timeout")
        self.assertEqual(out["response"]["values"], {"inventory.count": 1})
        code, out, _ = self.cli("wait-until", "inventory.count>=")
        self.assertEqual(code, 2)

    def test_mouse_click_buttons_axis_wait_screenshot(self):
        self.assertEqual(self.cli("mouse", "640", "360")[0], 0)
        code, out, text = self.cli("click", "640", "360")
        self.assertEqual(code, 0, text)
        self.assertEqual(out["hit"], "SViewport")
        self.assertEqual(self.cli("click")[0], 0)
        self.assertEqual(self.cli("click", "5")[0], 2)
        code, out, _ = self.cli("click-button", "Identify")
        self.assertEqual((code, out["error"]), (1, "no_button"))
        code, out, text = self.cli("axis", "Gamepad_LeftY=1", "Gamepad_LeftX=-0.5", "--hold", "0.1")
        self.assertEqual(code, 0, text)
        self.assertEqual(out["values"], {"Gamepad_LeftY": 1.0, "Gamepad_LeftX": -0.5})
        self.assertEqual(self.cli("axis", "Gamepad_LeftY")[0], 2)
        code, out, _ = self.cli("wait", "0.1")
        self.assertEqual(code, 0)
        code, out, text = self.cli("screenshot")
        self.assertEqual(code, 0, text)
        self.assertTrue(ab.file_complete(out["path"]))
        # Every CLI call appended to the same transcript, each from its own process.
        entries = read_jsonl(os.path.join(self.tmp, "bridge.jsonl"))
        self.assertGreaterEqual(len({e["pid"] for e in entries}), 5)

    def test_port_option_without_a_session(self):
        code, out, text = run_cli("--session", os.path.join(self.tmp, "none.json"), "--port",
                                  self.server.port, "--token", TOKEN, "ping")
        self.assertEqual(code, 0, text)
        code, out, _ = run_cli("--port", self.server.port, "--token", "wrong", "ping")
        self.assertEqual((code, out["error"]), (2, "token_mismatch"))
        code, out, text = run_cli("ping", "--port", self.server.port)  # options after the command
        self.assertEqual(code, 0, text)

    def test_missing_session(self):
        code, out, _ = run_cli("--session", os.path.join(self.tmp, "none.json"), "ping")
        self.assertEqual((code, out["error"]), (2, "no_session"))

    def test_stale_session_is_removed(self):
        dead = subprocess.Popen([sys.executable, "-c", "pass"])
        dead.wait()
        ab.write_session(self.session, {"pid": dead.pid, "port": self.server.port,
                                        "token": TOKEN, "run_dir": self.tmp, "exe": None})
        code, out, _ = self.cli("ping")
        self.assertEqual((code, out["error"]), (2, "stale_session"))
        self.assertFalse(os.path.exists(self.session))

    @unittest.skipUnless(WINDOWS, "process image check is Windows-only")
    def test_quit_kills_a_game_whose_bridge_is_unreachable(self):
        # An older session file without pid_created: identified by its exe name.
        game = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        self.addCleanup(lambda: game.poll() is None and game.kill())
        ab.write_session(self.session, {"pid": game.pid, "port": ab.free_port(), "token": TOKEN,
                                        "run_dir": self.tmp, "exe": sys.executable})
        code, out, text = self.cli("quit")
        self.assertEqual(code, 0, text)
        self.assertIn("bridge unreachable", out["shutdown"])
        self.assertTrue(out["session_removed"])
        game.wait(timeout=10)

    @unittest.skipUnless(WINDOWS, "process creation times are Windows-only")
    def test_quit_kills_the_recorded_game_by_creation_time(self):
        game = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        self.addCleanup(lambda: game.poll() is None and game.kill())
        ab.write_session(self.session, {"pid": game.pid,
                                        "pid_created": ab.process_create_time(game.pid),
                                        "port": ab.free_port(), "token": TOKEN,
                                        "run_dir": self.tmp, "exe": sys.executable})
        code, out, text = self.cli("quit")
        self.assertEqual(code, 0, text)
        self.assertIn("bridge unreachable", out["shutdown"])
        game.wait(timeout=10)

    @unittest.skipUnless(WINDOWS, "process creation times are Windows-only")
    def test_quit_never_kills_a_reused_pid(self):
        # The session's game died; Windows gave its pid to another process with the same exe
        # name.  Only the creation time tells them apart.
        other = subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"])
        self.addCleanup(lambda: other.poll() is None and other.kill())
        ab.write_session(self.session, {"pid": other.pid, "pid_created": "1",
                                        "port": ab.free_port(), "token": TOKEN,
                                        "run_dir": self.tmp, "exe": sys.executable})
        code, out, text = self.cli("quit")
        self.assertEqual((code, out["error"]), (2, "stale_session"), text)
        self.assertFalse(os.path.exists(self.session))
        time.sleep(0.3)
        self.assertIsNone(other.poll(), "quit killed a process that reused the session's pid")

    def test_quit_never_kills_an_unconfirmed_pid(self):
        ab.write_session(self.session, {"pid": os.getpid(), "port": ab.free_port(),
                                        "token": TOKEN, "run_dir": self.tmp, "exe": None})
        code, out, _ = self.cli("quit")
        self.assertEqual((code, out["error"]), (2, "connect_failed"))
        self.assertTrue(os.path.exists(self.session))

    def test_usage_errors(self):
        self.assertEqual(self.cli("bogus")[0], 2)
        self.assertEqual(self.cli()[0], 2)


class TestCliStuck(CliCase):
    def world_kwargs(self):
        return {"obstacles": [(60, 100, -2000, 2000)]}

    def test_walk_to_stuck_is_exit_1(self):
        code, out, text = self.cli("walk-to", "150", "0", "--timeout", "60")
        self.assertEqual(code, 1, text)
        self.assertEqual(out["error"], "stuck")
        self.assertTrue(out["message"].startswith("stuck at ("))


@unittest.skipUnless(WINDOWS, "detached launch and job objects are Windows-only")
class TestCliLaunch(QuietCase):
    """launch (detached) -> commands from separate processes -> quit, with tests/fake_game.py."""

    def test_launch_session_quit(self):
        session = os.path.join(self.tmp, "session.json")
        run_dir = os.path.join(self.tmp, "run")
        argv = json.dumps([sys.executable, FAKE_GAME])
        code, out, text = run_cli("--session", session, "launch", "--run-dir", run_dir,
                                  "--test-exe-argv", argv, "--test-skip-freshness",
                                  "--timeout", "30")
        self.assertEqual(code, 0, text)
        pid = out["session"]["pid"]
        self.addCleanup(lambda: ab.pid_alive(pid) and ab.kill_pid(pid))
        with open(session, encoding="utf-8") as f:
            data = json.load(f)
        for field in ("pid", "pid_created", "port", "token", "run_dir", "exe", "commit",
                      "started_at"):
            self.assertIn(field, data)
        self.assertEqual(data["pid_created"], ab.process_create_time(pid))
        self.assertTrue(out["ping"]["ready"])
        self.assertTrue(ab.pid_alive(pid, data["exe"], data["pid_created"]),
                        "the detached game died with the CLI")

        code, out, text = run_cli("--session", session, "ping")
        self.assertEqual((code, out["token"], out["pid"]), (0, data["token"], pid), text)
        code, out, text = run_cli("--session", session, "launch", "--run-dir", run_dir,
                                  "--test-exe-argv", argv, "--test-skip-freshness")
        self.assertEqual((code, out["error"]), (2, "session_running"), text)
        code, out, text = run_cli("--session", session, "walk-to", "400", "0")
        self.assertEqual(code, 0, text)
        code, out, text = run_cli("--session", session, "press", "F")
        self.assertEqual(code, 0, text)
        code, out, text = run_cli("--session", session, "state", "--sections", "inventory")
        self.assertEqual((code, out["state"]["inventory"]["count"]), (0, 1), text)

        code, out, text = run_cli("--session", session, "quit")
        self.assertEqual(code, 0, text)
        self.assertTrue(out["session_removed"])
        self.assertFalse(os.path.exists(session))
        self.assertTrue(wait_pid_gone(pid), "the game did not exit after quit")
        with open(os.path.join(run_dir, "game.log"), encoding="utf-8-sig") as f:
            log_text = f.read()
        self.assertIn("AgentBridge: quit requested", log_text)
        cmds = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertIn("quit", cmds)
        self.assertEqual(set(cmds), {"ping", "state", "key", "quit"})


# ---------------------------------------------------------------------------
# Bridge.launch end to end with tests/fake_game.py
# ---------------------------------------------------------------------------
@unittest.skipUnless(WINDOWS, "job objects are Windows-only")
class TestLaunch(QuietCase):
    def launch(self, *extra, **kw):
        kw.setdefault("skip_freshness", True)
        kw.setdefault("timeout", 30)
        kw.setdefault("ready_timeout", 15)
        bridge = ab.Bridge.launch(exe_argv=[sys.executable, FAKE_GAME], run_dir=self.tmp,
                                  extra_args=list(extra), **kw)
        self.addCleanup(bridge.close)
        return bridge

    def launch_error(self, kind, code, *extra, **kw):
        with self.assertRaises(ab.BridgeError) as cm:
            self.launch(*extra, **kw)
        self.assertEqual((cm.exception.kind, cm.exception.code), (kind, code), str(cm.exception))
        return cm.exception

    def test_launch_ready_and_quit(self):
        bridge = self.launch()
        self.assertTrue(bridge.ping_info["ready"])
        self.assertEqual(bridge.ping_info["token"], bridge.token)
        self.assertEqual(bridge.ping_info["pid"], bridge.proc.pid)
        self.assertIsNotNone(bridge.job)
        for token in ("-AgentBridge ", f"-AgentBridgePort={bridge.port}",
                      f"-AgentBridgeToken={bridge.token}", f"-AgentBridgeShotDir={self.tmp}",
                      pb.DEFAULT_MAP, "-abslog="):
            self.assertIn(token, bridge.command_line)
        self.assertNotIn("-PlaytestCapture", bridge.command_line)
        with open(bridge.game_log, encoding="utf-8-sig") as f:
            text = f.read()
        self.assertIn("Identified experience LyraExperienceDefinition:B_MordecaiArenaExperience",
                      text)
        self.assertIn(f"AgentBridge: listening on 127.0.0.1:{bridge.port}", text)
        info = bridge.info()
        self.assertEqual((info["pid"], info["port"], info["run_dir"]),
                         (bridge.proc.pid, bridge.port, self.tmp))
        self.assertEqual(info["pid_created"], ab.process_create_time(bridge.proc.pid))
        self.assertTrue(info["pid_created"])
        self.assertTrue(os.path.isfile(os.path.join(self.tmp, "bridge.jsonl")))
        result = bridge.quit()
        self.assertEqual(result["exit_code"], 0, result)
        self.assertIsNotNone(bridge.proc.poll())

    def test_close_kills_a_non_detached_game(self):
        bridge = self.launch()
        bridge.close()
        bridge.proc.wait(timeout=10)
        self.assertIsNotNone(bridge.proc.returncode)

    def test_job_kills_the_game_when_the_launcher_dies(self):
        helper = os.path.join(self.tmp, "launcher.py")
        with open(helper, "w", encoding="utf-8") as f:
            f.write(
                "import json, os, sys\n"
                f"sys.path.insert(0, {SCRIPTS!r})\n"
                "import agent_bridge as ab\n"
                f"b = ab.Bridge.launch(exe_argv=[sys.executable, {FAKE_GAME!r}], "
                f"skip_freshness=True, run_dir={os.path.join(self.tmp, 'run')!r}, timeout=30)\n"
                "print(json.dumps({'pid': b.pid, 'job': b.job is not None}), flush=True)\n"
                "os._exit(0)\n")
        r = subprocess.run([sys.executable, helper], capture_output=True, text=True, timeout=90)
        self.assertEqual(r.returncode, 0, r.stderr)
        out = json.loads(r.stdout.strip().splitlines()[-1])
        self.addCleanup(lambda: ab.pid_alive(out["pid"]) and ab.kill_pid(out["pid"]))
        self.assertTrue(out["job"])
        self.assertTrue(wait_pid_gone(out["pid"]), "kill-on-close job did not kill the game")

    def test_detached_game_survives_close(self):
        bridge = self.launch(detach=True)
        pid = bridge.pid
        self.addCleanup(lambda: ab.pid_alive(pid) and ab.kill_pid(pid))
        self.assertIsNone(bridge.job)
        bridge.close()
        time.sleep(0.3)
        self.assertTrue(ab.pid_alive(pid))
        again = ab.Bridge.attach(bridge.port, token=bridge.token, pid=pid, exe=sys.executable)
        self.assertEqual(again.ping()["pid"], pid)
        self.assertIn(f"pid {pid} exited", again.quit(timeout_s=10)["shutdown"])
        self.assertFalse(ab.pid_alive(pid))

    def test_waits_for_the_loading_screen(self):
        t0 = time.monotonic()
        bridge = self.launch("-FakeLoadingS=1.0")
        self.assertTrue(bridge.ping_info["ready"])
        self.assertFalse(bridge.ping_info["loading_screen"])
        self.assertGreaterEqual(time.monotonic() - t0, 1.0)

    def test_crash_is_infra_and_kills_the_game(self):
        err = self.launch_error("infra", "crash", "-FakeCrash")
        self.assertIn("Critical error", str(err))

    def test_wrong_experience_is_gameplay(self):
        err = self.launch_error("gameplay", "wrong_experience",
                                "-FakeExperience=B_LyraDefaultExperience")
        self.assertIn("B_LyraDefaultExperience", str(err))

    def test_frontend_expects_the_frontend_experience(self):
        bridge = self.launch(frontend=True)
        self.assertNotIn(pb.DEFAULT_MAP, bridge.command_line)
        self.assertEqual(bridge.ping_info["experience"], "B_LyraFrontEnd_Experience")

    def test_exit_before_ready(self):
        self.launch_error("infra", "exited", "-FakeExitAfterLoad=5", "-FakeLoadingS=2")

    def test_bind_failure(self):
        self.launch_error("infra", "bind_failed", "-FakeBindFail")

    def test_token_mismatch(self):
        self.launch_error("infra", "token_mismatch", "-FakeToken=someone-else")

    def test_boot_timeout(self):
        self.launch_error("infra", "boot_timeout", "-FakeBootDelay=20", timeout=1)

    def test_no_experience(self):
        self.launch_error("gameplay", "no_experience", "-FakeNoExperience")

    def test_stale_build_is_refused(self):
        exe = os.path.join(self.tmp, "LyraGame.exe")
        with open(exe, "wb") as f:
            f.write(b"not a game")
        stale = {"fresh": False, "reasons": ["build commit a != HEAD b"], "warnings": [],
                 "git_error": None, "build": None, "head": "b", "branch": "x"}
        with mock.patch.object(ab.pb, "gate", return_value=(stale, pb.EXIT_INFRA)) as gate:
            with self.assertRaises(ab.BridgeError) as cm:
                ab.Bridge.launch(exe=exe, run_dir=self.tmp)
        gate.assert_called_once_with(exe, False)
        self.assertEqual((cm.exception.kind, cm.exception.code), ("infra", "stale_build"))
        self.assertIn("build commit a != HEAD b", str(cm.exception))
        self.assertFalse(os.path.exists(os.path.join(self.tmp, "game.log")))

    def test_allow_stale_runs_and_records_it(self):
        stale = {"fresh": False, "reasons": ["build commit a != HEAD b"], "warnings": [],
                 "git_error": None, "build": None, "head": "b", "branch": "x"}
        with mock.patch.object(ab.pb, "gate", return_value=(stale, None)) as gate:
            bridge = self.launch(allow_stale=True, skip_freshness=False)
        self.assertEqual(gate.call_args[0][1], True)
        self.assertTrue(bridge.stale)
        self.assertTrue(any("STALE BUILD" in w for w in bridge.warnings))
        self.assertTrue(bridge.info()["stale"])

    def test_missing_exe(self):
        with self.assertRaises(ab.BridgeError) as cm:
            ab.Bridge.launch(exe=os.path.join(self.tmp, "nope.exe"), run_dir=self.tmp)
        self.assertEqual(cm.exception.code, "launch_failed")


if __name__ == "__main__":
    unittest.main()
