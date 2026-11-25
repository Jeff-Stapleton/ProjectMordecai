"""Offline tests for Scripts/playtests/us099_walls.py."""

import importlib.util
import io
import json
import os
import sys
import tempfile
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us099_walls.py")
TOKEN = "us099-token"


def load_playtest():
    spec = importlib.util.spec_from_file_location("us099_walls", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us099 = load_playtest()


class Us099PlaytestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.addCleanup(self._tmp.cleanup)
        self.run_dir = os.path.join(self._tmp.name, "run")
        self.log = io.StringIO()
        for module in (us099, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)

    def serve(self, dead=False):
        world = fb.FakeWorld(**fb.MAP_PRESETS[fb.DEFAULT_MAP])
        if dead:
            world.dead_since = world.real_time()
        server = fb.FakeBridgeServer(
            world, token=TOKEN, shot_dir=self._tmp.name
        ).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        code = us099.main([
            "--attach-port", str(server.port), "--token", TOKEN,
            "--run-dir", self.run_dir,
        ])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as handle:
            return code, json.load(handle)

    def test_real_wasd_walk_reaches_corner_alive_and_captures(self):
        world, server = self.serve()
        code, report = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        checks = {check["name"]: check for check in report["checks"]}
        self.assertEqual(checks["walk_sw_corner"]["status"], "PASS")
        self.assertEqual(checks["screenshot_sw_corner"]["status"], "WARN",
                         "the fake bridge intentionally writes a uniform placeholder frame")
        self.assertLessEqual(
            ((world.pos[0] - us099.SW_CORNER[0]) ** 2
             + (world.pos[1] - us099.SW_CORNER[1]) ** 2) ** 0.5,
            us099.CORNER_TOL,
        )
        self.assertEqual([shot["label"] for shot in report["screenshots"]], ["sw_corner"])
        self.assertEqual(report["acs"][0]["id"], "AC-099.3")
        self.assertFalse(report["acs"][0]["accepted"], "attach runs are diagnostic")

        with open(os.path.join(self.run_dir, "bridge.jsonl"), encoding="utf-8") as handle:
            sent = [json.loads(line)["msg"] for line in handle
                    if json.loads(line).get("dir") == "send"]
        commands = {message["cmd"] for message in sent}
        self.assertLessEqual(
            commands, {"ping", "state", "key", "screenshot", "wait", "quit"}
        )
        self.assertTrue(any(message["cmd"] == "key" for message in sent))
        self.assertFalse(any(message.get("route") == "viewport" for message in sent))

    def test_death_before_corner_is_a_failure_and_skips_shot(self):
        _, server = self.serve(dead=True)
        code, report = self.play(server)
        self.assertEqual(code, 1)
        checks = {check["name"]: check for check in report["checks"]}
        self.assertEqual(checks["walk_sw_corner"]["status"], "FAIL")
        self.assertIn("killed the player", checks["walk_sw_corner"]["detail"])
        self.assertEqual(checks["screenshot_sw_corner"]["status"], "SKIP")

    def test_plan_and_launch_path_are_pinned(self):
        self.assertEqual(kit.plan_problems(us099.Us099Run), [])
        self.assertIs(us099.Run, us099.Us099Run)
        self.assertEqual(us099.Us099Run.STORY, "US-099")
        self.assertEqual(us099.Us099Run.RUN_SUFFIX, "us099")
        self.assertEqual(us099.Us099Run.launch_map(), kit.DEFAULT_MAP)
        self.assertEqual(us099.PLAN[0].check, "walk_sw_corner")
        self.assertEqual(us099.PLAN[0].proves, ("AC-099.3",))


if __name__ == "__main__":
    unittest.main()
