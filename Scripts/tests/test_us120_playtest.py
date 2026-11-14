"""Offline tests for the US-120 Tier 4 script against tests/fake_bridge.py.

Fake physics is not acceptance evidence. These tests pin the plan, screenshot contract,
failure reporting, and the rule that the script only uses input/read bridge commands.
"""

import contextlib
import importlib.util
import json
import math
import os
import sys
import tempfile
import unittest
from unittest import mock


SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402


PLAYTEST = os.path.join(SCRIPTS, "playtests", "us120_vw_tiles.py")
TOKEN = "us120-offline-token"
INPUT_AND_READ_COMMANDS = {"ping", "state", "key", "screenshot", "wait", "wait_until"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us120_vw_tiles", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us120 = load_playtest()


class PondDipWorld(fb.FakeWorld):
    """Fake-only Z profile for testing the shallow traversal assertion."""

    def _s_player(self):
        state = super()._s_player()
        low_x, high_x = us120.SHALLOW_DIP_X
        state["location"][2] = (
            us120.SHALLOW_DIP_Z
            if low_x <= self.pos[0] <= high_x
            else us120.TIER0_Z
        )
        return state


def read_jsonl(path):
    with open(path, encoding="utf-8") as stream:
        return [json.loads(line) for line in stream if line.strip()]


class Us120FakeCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, faults=None, pond_dip=False):
        world_class = PondDipWorld if pond_dip else fb.FakeWorld
        world = world_class(
            map=us120.Us120Run.MAP,
            experience=us120.Us120Run.EXPERIENCE,
            start=(0.0, 0.0),
            arena_walls=False,
            obstacles=(),
            pickups=[],
            enemies=False,
        )
        server = fb.FakeBridgeServer(
            world, token=TOKEN, shot_dir=self.tmp, faults=faults
        ).start()
        self.addCleanup(server.stop)
        return server

    def compact_route(self):
        """Keep the fake run fast; flat fake Z is deliberately treated as both tiers."""
        patches = (
            mock.patch.object(
                us120,
                "GALLERY_VIEWPOINTS",
                ((0.0, 0.0), (40.0, 0.0), (80.0, 0.0), (120.0, 0.0)),
            ),
            mock.patch.object(
                us120,
                "RAMPS",
                {
                    "2t": {"low": (140.0, -20.0), "top": (180.0, -20.0)},
                    "3t": {"low": (200.0, 20.0), "top": (240.0, 20.0)},
                },
            ),
            mock.patch.object(us120, "CLIFF_START", (260.0, 0.0)),
            mock.patch.object(us120, "CLIFF_MAX_X", 10000.0),
            mock.patch.object(us120, "SHALLOW_START", (300.0, 0.0)),
            mock.patch.object(us120, "SHALLOW_END", (340.0, 0.0)),
            mock.patch.object(us120, "SHALLOW_APPROACH", ((300.0, 0.0),)),
            mock.patch.object(us120, "SHALLOW_DIP_X", (310.0, 330.0)),
            mock.patch.object(
                us120,
                "DEEP_BANKS",
                (
                    {
                        "label": "edge",
                        "pivot": (380.0, -20.0),
                        "start": (400.0, -20.0),
                        "keys": ("S",),
                        "direction": (-1.0, 0.0),
                        "face_progress": 0.0,
                        "route": ((400.0, -20.0),),
                    },
                    {
                        "label": "outer_corner",
                        "pivot": (380.0, 0.0),
                        "start": (400.0, -20.0),
                        "keys": ("S", "D"),
                        "direction": (-0.707107, 0.707107),
                        "face_progress": 0.0,
                        "route": ((400.0, -20.0),),
                    },
                    {
                        "label": "inner_corner",
                        "pivot": (380.0, 20.0),
                        "start": (400.0, 0.0),
                        "keys": ("S", "D"),
                        "direction": (-0.707107, 0.707107),
                        "face_progress": 100.0,
                        "route": ((400.0, 0.0),),
                    },
                ),
            ),
            mock.patch.object(us120, "DEEP_ENTRY_ROUTE", ((360.0, -40.0), (400.0, -40.0))),
            mock.patch.object(us120, "DEEP_ALLOWED_PAST_FACE", 10000.0),
            mock.patch.object(us120, "_near", lambda value, target, tolerance=None: True),
            mock.patch.object(us120, "BLOCK_HOLD_S", 0.02),
            mock.patch.object(us120, "JUMP_SAMPLE_S", 0.03),
        )
        return patches

    def play(self, server):
        with contextlib.ExitStack() as stack:
            for patcher in self.compact_route():
                stack.enter_context(patcher)
            code = us120.main(
                [
                    "--attach-port",
                    str(server.port),
                    "--token",
                    TOKEN,
                    "--run-dir",
                    self.run_dir,
                ]
            )
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as stream:
            return code, json.load(stream)

    @staticmethod
    def checks(report):
        return {check["name"]: check for check in report["checks"]}


class TestUs120Plan(unittest.TestCase):
    def test_plan_is_valid_and_maps_checks_to_the_story_acs(self):
        self.assertEqual(kit.plan_problems(us120.Us120Run), [])
        self.assertIs(us120.Run, us120.Us120Run)
        self.assertEqual(us120.Us120Run.STORY, "US-120")
        self.assertEqual(us120.Us120Run.MAP, kit.VW_TILE_TEST_MAP)
        self.assertEqual(
            [step.check for step in us120.PLAN],
            [
                "tile_gallery_visible",
                "ramp_2t_up",
                "ramp_2t_down",
                "ramp_3t_up",
                "ramp_3t_down",
                "cliff_walk_blocked",
                "cliff_jump_blocked",
                "shallow_pond_crosses",
                "deep_bank_blocks",
            ],
        )
        proved = {ac for step in us120.PLAN for ac in step.proves}
        self.assertEqual(
            proved, {"AC-120.1", "AC-120.4", "AC-120.5", "AC-120.6", "AC-120.7"}
        )

    def test_every_required_screenshot_has_a_nonempty_expectation(self):
        self.assertEqual(
            set(us120.SCREENSHOTS),
            {
                "gallery_1",
                "gallery_2",
                "gallery_3",
                "gallery_4",
                "ramp_2t_top",
                "ramp_3t_top",
                "cliff_rejection",
                "shallow_crossing",
                "deep_edge_rejection",
                "deep_outer_rejection",
                "deep_inner_rejection",
            },
        )
        self.assertTrue(all(us120.SCREENSHOTS.values()))

    def test_pond_checks_require_a_real_basin_and_radial_corner_probes(self):
        self.assertAlmostEqual(us120.SHALLOW_BED_Z, -38.0)
        self.assertAlmostEqual(us120.SHALLOW_WATER_Z, -8.0)
        self.assertAlmostEqual(us120.SHALLOW_DIP_Z, 54.0)
        self.assertAlmostEqual(us120.DEEP_BED_Z, -141.0)
        self.assertAlmostEqual(us120.DEEP_WATER_Z, -8.0)
        self.assertAlmostEqual(
            us120.DEEP_ALLOWED_PAST_FACE,
            us120.BANK_TOLERANCE_CM + us120.CAPSULE_RADIUS_CM,
        )
        by_label = {spec["label"]: spec for spec in us120.DEEP_BANKS}
        self.assertEqual(by_label["edge"]["keys"], ("S",))
        for label in ("outer_corner", "inner_corner"):
            spec = by_label[label]
            self.assertEqual(spec["keys"], ("S", "D"))
            self.assertAlmostEqual(math.hypot(*spec["direction"]), 1.0, places=5)
            self.assertLess(spec["direction"][0], 0.0)
            self.assertGreater(spec["direction"][1], 0.0)


class TestUs120AgainstFake(Us120FakeCase):
    def test_compact_fake_route_passes_and_captures_each_location_immediately(self):
        server = self.serve(pond_dip=True)
        code, report = self.play(server)
        self.assertEqual(code, 0, json.dumps(report["checks"], indent=2)[-5000:])
        checks = self.checks(report)
        for step in us120.PLAN:
            self.assertEqual(checks[step.check]["status"], "PASS", step.check)
        self.assertEqual(
            [shot["label"] for shot in report["screenshots"]],
            list(us120.SCREENSHOTS),
        )
        self.assertTrue(all(os.path.isfile(shot["path"]) for shot in report["screenshots"]))

        sent = [
            entry["msg"]["cmd"]
            for entry in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
            if entry["dir"] == "send"
        ]
        self.assertTrue(sent)
        self.assertLessEqual(set(sent), INPUT_AND_READ_COMMANDS)
        self.assertFalse([command for command in sent if command.startswith("debug_")])
        self.assertTrue([request for request in server.received if request.get("cmd") == "key"])

        shallow = checks["shallow_pond_crosses"]
        self.assertIn("min Z 54.0", shallow["detail"])
        self.assertIn("exit Z 92.0", shallow["detail"])

    def test_flat_floor_cannot_false_pass_the_shallow_pond(self):
        server = self.serve(pond_dip=False)
        code, report = self.play(server)
        self.assertEqual(code, 1)
        shallow = self.checks(report)["shallow_pond_crosses"]
        self.assertEqual(shallow["status"], "FAIL")
        self.assertIn("did not dip", shallow["detail"])

    def test_bridge_timeout_is_a_gameplay_failure_not_a_false_pass(self):
        server = self.serve(
            faults=fb.Faults(
                error_replies={"key": ("timeout", "injected movement timeout")}
            )
        )
        code, report = self.play(server)
        self.assertEqual(code, 1)
        failed = [
            check for check in report["checks"] if check["status"] == "FAIL"
        ]
        self.assertTrue(failed)
        self.assertTrue(all(check["kind"] == "gameplay" for check in failed))
        self.assertTrue(
            any("timeout" in check["detail"].lower() for check in failed),
            failed,
        )
        self.assertEqual(report["result"], "FAIL")


if __name__ == "__main__":
    unittest.main()
