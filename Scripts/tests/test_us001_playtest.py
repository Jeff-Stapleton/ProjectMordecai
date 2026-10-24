"""Tests for Scripts/playtests/us001_movement.py against tests/fake_bridge.py.

These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Us001
"""

import importlib.util
import io
import json
import math
import os
import re
import sys
import tempfile
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us001_movement.py")
STORY_FILE = os.path.join(os.path.dirname(SCRIPTS), "stories", "done",
                          "US-001-project-foundation.md")
TOKEN = "us001-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "axis", "mouse", "click", "screenshot",
                       "wait_until", "wait", "quit"}
KNOWN_FAILS = ["sprint_l3", "facing_mouse", "facing_stick", "gamepad_move"]
FIXED_BY_US083 = ["sprint_shift", "jump_space"]


def load_playtest():
    spec = importlib.util.spec_from_file_location("us001_movement", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us001 = load_playtest()


# ---------------------------------------------------------------------------
# Fake worlds
# ---------------------------------------------------------------------------
class UnboundWorld(fb.FakeWorld):
    """The shared fake plus the combat HUD's bar widgets the real game shows (the shared fake
    lists only WBP_CombatHUD_C and the pickup prompt).  Everything else is the game before
    US-083: sprint/jump unbound, facing pinned to +X, gamepad sticks dead."""

    def _s_widgets(self):
        out = super()._s_widgets()
        for cls in ("MordecaiHealthBarWidget", "MordecaiStaminaBarWidget",
                    "MordecaiPostureBarWidget", "MordecaiSpellPointsBarWidget"):
            out.append({"class": cls, "name": cls + "_0", "rect": [20, 20, 300, 24],
                        "texts": [], "buttons": []})
        return out


class HudWorld(UnboundWorld):
    """Today's game (US-083): LeftShift toggles sprint (x1.5) and SpaceBar jumps
    (movement_mode Falling for 0.6 s).  The L3 key name is still invalid (US-097), facing is
    pinned to +X and the gamepad sticks are dead."""

    SPRINT_MULT = 1.5
    JUMP_S = 0.6
    SPRINT_KEYS = ("LeftShift",)
    STICKY_SPRINT = False   # True: a second tap does not end the sprint
    DROP_FIRST_SPRINT_TAP = False  # True: the first sprint tap is lost

    def __init__(self, **kw):
        super().__init__(**kw)
        self.base_speed = self.speed
        self.sprinting = False
        self.jump_until = -1.0
        self.sprint_taps = 0

    def _on_key_pressed(self, key):
        super()._on_key_pressed(key)
        if not self.ready or self.menu_open or self.is_dead:
            return
        if key in self.SPRINT_KEYS:
            self.sprint_taps += 1
            if self.DROP_FIRST_SPRINT_TAP and self.sprint_taps == 1:
                return
            self.sprinting = True if self.STICKY_SPRINT else not self.sprinting
            self.speed = self.base_speed * (self.SPRINT_MULT if self.sprinting else 1.0)
        elif key == "SpaceBar":
            self.jump_until = self.real_time() + self.JUMP_S

    def _s_player(self):
        out = super()._s_player()
        if out.get("valid") and self.real_time() < self.jump_until:
            out["movement_mode"] = "Falling"
        return out


class StickySprintWorld(HudWorld):
    """LeftShift starts the sprint, but tapping it again does not end it."""

    STICKY_SPRINT = True


class DroppedSprintTapWorld(HudWorld):
    """The first LeftShift tap is lost, so the second one starts the sprint."""

    DROP_FIRST_SPRINT_TAP = True


class FixedWorld(HudWorld):
    """A game with every known issue fixed: Shift / L3 toggle sprint, SpaceBar jumps, the mouse
    and the right stick turn the pawn, and the left stick moves it."""

    SPRINT_KEYS = ("LeftShift", "Gamepad_LeftThumbstick")

    def __init__(self, **kw):
        kw.setdefault("stick_moves", True)
        super().__init__(**kw)
        self.yaw = 0.0

    def move_cursor(self, x, y):
        with self.lock:
            super().move_cursor(x, y)
            wx, wy = (fb.VIEWPORT_H / 2.0) - y, x - (fb.VIEWPORT_W / 2.0)  # up=+X, right=+Y
            if math.hypot(wx, wy) > 1:
                self.yaw = math.degrees(math.atan2(wy, wx))

    def tick(self, dt):
        with self.lock:
            held = self._held_union()
            rx, ry = held.get("Gamepad_RightX", 0.0), held.get("Gamepad_RightY", 0.0)
            if math.hypot(rx, ry) > 0.25:
                self.yaw = math.degrees(math.atan2(rx, ry))
        super().tick(dt)

    def _s_player(self):
        out = super()._s_player()
        if out.get("valid"):
            out["rotation"] = [0.0, round(self.yaw, 2), 0.0]
        return out


class MouseTurnsCameraWorld(HudWorld):
    """A broken camera: the mouse writes the control rotation's yaw."""

    def move_cursor(self, x, y):
        with self.lock:
            super().move_cursor(x, y)
            self.control_yaw = (x - fb.VIEWPORT_W / 2.0) / 10.0


class NoBrakeWorld(HudWorld):
    """Movement with poor braking: after the keys are let go the pawn keeps creeping at 5 % of
    its walk speed for 1.5 s (player.speed stays above 1)."""

    SLIDE_S = 1.5
    SLIDE_FACTOR = 0.05

    def __init__(self, **kw):
        super().__init__(**kw)
        self.slide_until = -1.0
        self.slide_vel = (0.0, 0.0)

    def _move(self, dt):
        was = list(self.vel)
        super()._move(dt)
        now = self.real_time()
        walking = math.hypot(*was) > 0.5 * self.speed  # a key was held last tick (not a slide)
        if self.vel == [0.0, 0.0] and walking:
            self.slide_until = now + self.SLIDE_S
            self.slide_vel = (was[0] * self.SLIDE_FACTOR, was[1] * self.SLIDE_FACTOR)
        if self.vel == [0.0, 0.0] and now < self.slide_until:
            x, y = self.pos[0] + self.slide_vel[0] * dt, self.pos[1] + self.slide_vel[1] * dt
            if not self._blocked(x, y):
                self.pos = [x, y]
                self.vel = list(self.slide_vel)


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class PlaytestCase(unittest.TestCase):
    """Temp dir, captured logs, and a fake bridge per run."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        for module in (us001, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        patcher = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, world_class=HudWorld, faults=None, **world_kwargs):
        world_kwargs.setdefault("obstacles", fb.DEVTESTMAP_OBSTACLES)
        world = world_class(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp, faults=faults).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server, *extra):
        code = us001.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir, *extra])
        return code, self.report()

    def report(self):
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return json.load(f)

    @staticmethod
    def by_name(report):
        return {c["name"]: c for c in report["checks"]}

    @staticmethod
    def statuses(report, status):
        return [c["name"] for c in report["checks"] if c["status"] == status]

    @staticmethod
    def acs(report):
        return {a["id"]: a for a in report["acs"]}


# ---------------------------------------------------------------------------
# Runs against the fake
# ---------------------------------------------------------------------------
class TestUs001TodaysGame(PlaytestCase):
    """HudWorld behaves like today's game: what works PASSes (sprint and jump since US-083),
    the known issues FAIL as known."""

    def test_works_passes_and_the_known_issues_fail_as_known(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 1, self.log.getvalue()[-3000:])
        checks = self.by_name(rep)
        for name in ("attach", "experience", "hud_visible", "spawn_state", "imc_active",
                     "move_a", "move_w", "move_d", "move_s", "camera_fixed",
                     "no_camera_rotation_input", "walk_lane_wp1", "walk_lane_wp2",
                     "walk_lane_start", "walk_speed_baseline", "walk_lane_reset",
                     "sprint_shift", "walk_lane_reset2", "jump_space", "no_crash"):
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertEqual(self.statuses(rep, "FAIL"), KNOWN_FAILS)
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(rep["known_failures"], KNOWN_FAILS)
        self.assertEqual(rep["new_failures"], [])
        self.assertEqual(rep["fixed_candidates"], [])
        self.assertEqual(rep["undeclared_checks"], [])
        # The sprint and its toggle-back were both measured.
        self.assertIn("need > 700", checks["sprint_shift"]["detail"])
        self.assertIn("then held S 1s: peak speed 600 cm/s", checks["sprint_shift"]["detail"])
        self.assertNotIn(" -- ", checks["sprint_shift"]["detail"])
        self.assertFalse(world.sprinting, "the sprint was toggled back")
        # What the details say about each known failure.
        self.assertIn("need > 700", checks["sprint_l3"]["detail"])
        self.assertIn("yaw stayed 0", checks["facing_mouse"]["detail"])
        self.assertIn("yaw stayed 0", checks["facing_stick"]["detail"])
        self.assertIn("dx 0 cm", checks["gamepad_move"]["detail"])
        # The square of moves came back to the spawn and touched no pickup.
        self.assertEqual(world.picked, [])
        self.assertEqual(len(world.world_pickups), 6)
        # Nothing is left held: the axis holds were released.
        self.assertEqual(world._held_union(), {})

    def test_acceptance_criteria(self):
        _, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        acs = self.acs(rep)
        self.assertEqual({a["story"] for a in rep["acs"]}, {"US-001"})
        for ac in ("AC-2.2.5", "AC-2.3.3", "AC-1.2.2", "AC-1.2.4", "AC-2.1.6"):
            self.assertEqual(acs[ac]["status"], "PASS", ac)
        # WASD works, but the gamepad half of these ACs does not.
        for ac in ("AC-2.1.1", "AC-1.3.2"):
            self.assertEqual(acs[ac]["status"], "FAIL", ac)
            self.assertIn("gamepad_move", acs[ac]["checks"])
            self.assertIn("move_w", acs[ac]["checks"])
            self.assertEqual(acs[ac]["known_issue"], us001.KI_STICKS)
        self.assertEqual(acs["AC-1.3.2"]["checks"][0], "imc_active")
        # Sprint needs both keys; L3 still fails (US-097).  Facing is still pinned.
        for ac in ("AC-1.3.3", "AC-2.1.3", "AC-2.1.2"):
            self.assertEqual(acs[ac]["status"], "FAIL", ac)
            self.assertTrue(acs[ac]["known_issue"], ac)
        self.assertEqual(acs["AC-1.3.3"]["checks"], ["sprint_shift", "sprint_l3"])
        self.assertEqual(acs["AC-2.1.4"]["status"], "PASS")
        self.assertEqual(acs["AC-1.2.1"]["status"], "PENDING")
        self.assertIn("no camera transform", acs["AC-1.2.1"]["pending"])
        self.assertEqual(acs["AC-2.1.5"]["status"], "PENDING")
        self.assertFalse([a for a in rep["acs"] if a["accepted"]],
                         "an attached run never accepts an AC")
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("| AC-2.1.4 | jump_space | PASS", md)
        self.assertIn("FAIL (known: Known issue: facing", md)
        self.assertIn("| FAIL (known) |", md)
        self.assertNotIn("FAIL (NEW)", md)
        self.assertIn("## Movement measurements", md)
        self.assertLess(md.index("## Acceptance criteria"), md.index("## Movement measurements"))

    def test_run_dir_screenshots_and_the_audit_trail(self):
        _, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual([s["label"] for s in rep["screenshots"]],
                         ["start", "after_moves", "facing_arrow"])
        for s in rep["screenshots"]:
            self.assertTrue(ab.file_complete(s["path"]), s["path"])
            self.assertEqual(s["expect"], us001.SCREENSHOTS[s["label"]])
        sent = [e["msg"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        cmds = {m["cmd"] for m in sent}
        self.assertLessEqual(cmds, INPUT_AND_READ_CMDS)
        self.assertFalse([c for c in cmds if c.startswith("debug_")])
        self.assertFalse([m for m in sent if m.get("route") == "viewport"])
        self.assertFalse([m for m in sent if m.get("unsafe")])
        keys = [m.get("key") or tuple(m.get("keys") or ()) for m in sent if m["cmd"] == "key"]
        for k in ("A", "W", "D", "S", "LeftShift", "Gamepad_LeftThumbstick", "SpaceBar"):
            self.assertIn(k, keys)
        self.assertEqual(keys.count("LeftShift"), 2, "the sprint toggle is tapped back")
        axes = [m["values"] for m in sent if m["cmd"] == "axis"]
        self.assertIn({"Gamepad_LeftY": 1.0}, axes)
        self.assertIn({"Gamepad_RightX": -1.0}, axes)
        self.assertIn({"Gamepad_RightX": 0.0}, axes)
        mice = [(m["x"], m["y"]) for m in sent if m["cmd"] == "mouse"]
        self.assertIn(us001.FACING_MOUSE, mice)
        movement = {m["check"]: m for m in rep["movement"]}
        self.assertLess(movement["move_a"]["dy"], -100)
        self.assertGreater(movement["move_w"]["dx"], 100)
        self.assertGreater(movement["walk_speed_baseline"]["dx"], 400)
        self.assertGreater(movement["sprint_shift"]["dx"], 700)
        self.assertLess(movement["sprint_shift_toggle_back"]["dx"], -400)


class TestUs001BeforeUs083(PlaytestCase):
    def test_unbound_sprint_and_jump_fail_new(self):
        _, server = self.serve(UnboundWorld)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for name in FIXED_BY_US083:
            self.assertEqual(checks[name]["status"], "FAIL", name)
        self.assertEqual(rep["new_failures"], FIXED_BY_US083)
        self.assertIn("need > 700", checks["sprint_shift"]["detail"])
        self.assertIn("player.movement_mode", checks["jump_space"]["detail"])
        self.assertIn('"Walking"', checks["jump_space"]["detail"])

    def test_a_sprint_the_second_tap_does_not_end_fails(self):
        world, server = self.serve(StickySprintWorld)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["sprint_shift"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("the second tap did not end the sprint", check["detail"])
        self.assertIn("sprint_shift", rep["new_failures"])
        self.assertEqual(self.acs(rep)["AC-1.3.3"]["status"], "FAIL")
        self.assertTrue(world.sprinting)
        # The sprint left on cannot make the dead L3 key look fixed
        l3 = self.by_name(rep)["sprint_l3"]
        self.assertEqual(l3["status"], "SKIP", l3["detail"])
        self.assertIn("LeftShift sprint is still on", l3["detail"])
        self.assertEqual(rep["fixed_candidates"], [])

    def test_a_lost_first_sprint_tap_is_reported_out_of_phase(self):
        world, server = self.serve(DroppedSprintTapWorld)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["sprint_shift"]["status"], "FAIL")
        self.assertIn("toggle out of phase", checks["sprint_shift"]["detail"])
        self.assertEqual(checks["sprint_l3"]["status"], "SKIP")
        self.assertEqual(rep["fixed_candidates"], [])
        self.assertTrue(world.sprinting)


class TestUs001FixedGame(PlaytestCase):
    def test_a_fixed_game_passes_everything_and_flags_fixed_candidates(self):
        world, server = self.serve(FixedWorld)
        code, rep = self.play(server)
        fails = [c for c in rep["checks"] if c["status"] in ("FAIL", "SKIP")]
        self.assertEqual(code, 0, json.dumps(fails))
        self.assertEqual(rep["fixed_candidates"], KNOWN_FAILS)
        self.assertEqual(rep["known_failures"], [])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("PASS (FIXED?)", md)
        acs = self.acs(rep)
        for ac in ("AC-2.1.1", "AC-1.3.2", "AC-1.3.3", "AC-2.1.3", "AC-2.1.4", "AC-2.1.2"):
            self.assertEqual(acs[ac]["status"], "PASS", ac)
        self.assertFalse(world.sprinting, "both sprint toggles were tapped back")
        self.assertEqual(world._held_union(), {})


class TestUs001Failures(PlaytestCase):
    def test_the_shared_fake_has_no_bars_so_the_hud_check_fails_new(self):
        _, server = self.serve(fb.FakeWorld)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["hud_visible"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("missing class parts ['healthbar', 'staminabar']", check["detail"])
        self.assertIn('"10 / 10" present', check["detail"])
        self.assertIn("hud_visible", rep["new_failures"])

    def test_wrong_key_mapping_fails_the_moves(self):
        swapped = {"W": (-1.0, 0.0), "S": (1.0, 0.0), "D": (0.0, 1.0), "A": (0.0, -1.0)}
        _, server = self.serve(mapping=swapped)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["move_w"]["status"], "FAIL")
        self.assertIn("along +X", checks["move_w"]["detail"])
        self.assertEqual(checks["move_s"]["status"], "FAIL")
        self.assertEqual(checks["move_a"]["status"], "PASS")
        self.assertEqual(checks["move_d"]["status"], "PASS")
        self.assertIn("move_w", rep["new_failures"])
        self.assertEqual(self.acs(rep)["AC-2.1.1"]["status"], "FAIL")

    def test_a_slow_walk_fails_the_moves_and_the_baseline(self):
        _, server = self.serve(speed=200.0)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for name in ("move_a", "move_w", "move_d", "move_s", "walk_speed_baseline"):
            self.assertEqual(checks[name]["status"], "FAIL", name)
        self.assertIn("outside [400, 700]", checks["walk_speed_baseline"]["detail"])
        self.assertEqual(self.acs(rep)["AC-2.1.6"]["status"], "FAIL")

    def test_no_braking_fails_the_baseline(self):
        _, server = self.serve(NoBrakeWorld)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["walk_speed_baseline"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("slow deceleration", check["detail"])

    def test_a_rotated_camera_fails_camera_fixed(self):
        _, server = self.serve(control_yaw=30.0)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["camera_fixed"]["status"], "FAIL")
        self.assertIn("player.control_rotation[1]", checks["camera_fixed"]["detail"])
        self.assertEqual(checks["no_camera_rotation_input"]["status"], "FAIL")
        self.assertEqual(self.acs(rep)["AC-1.2.2"]["status"], "FAIL")

    def test_mouse_driven_camera_rotation_fails_the_probe(self):
        _, server = self.serve(MouseTurnsCameraWorld)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["camera_fixed"]["status"], "PASS")
        self.assertEqual(checks["no_camera_rotation_input"]["status"], "FAIL")
        self.assertIn("the camera rotated", checks["no_camera_rotation_input"]["detail"])
        self.assertEqual(self.acs(rep)["AC-1.2.4"]["status"], "FAIL")

    def test_a_spawn_away_from_the_centre_fails_spawn_state(self):
        _, server = self.serve(start=(200.0, -200.0))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["spawn_state"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("location=[200.0, -200.0, 92.0]", check["detail"])

    def test_a_wrong_experience_blocks_the_rest(self):
        _, server = self.serve(experience="B_LyraDefaultExperience")
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["experience"])
        checks = self.by_name(rep)
        for name in ("hud_visible", "move_w", "sprint_shift", "gamepad_move"):
            self.assertEqual(checks[name]["status"], "SKIP", name)

    def test_a_lost_connection_is_infra_and_skips_the_rest(self):
        _, server = self.serve(faults=fb.Faults(drop_on_cmd="mouse"))
        code, rep = self.play(server)
        self.assertEqual(code, 2)
        checks = self.by_name(rep)
        self.assertEqual(checks["no_camera_rotation_input"]["status"], "FAIL")
        self.assertEqual(checks["no_camera_rotation_input"]["kind"], "infra")
        for name in ("walk_lane_start", "walk_speed_baseline", "jump_space", "gamepad_move"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("connection was lost", checks[name]["detail"])
        self.assertEqual(checks["camera_fixed"]["status"], "PASS")


# ---------------------------------------------------------------------------
# Offline: the PLAN, the AC ids, the route
# ---------------------------------------------------------------------------
class TestUs001Plan(unittest.TestCase):
    def story_ac_ids(self):
        if not os.path.isfile(STORY_FILE):
            self.skipTest(f"{STORY_FILE} moved")
        with open(STORY_FILE, encoding="utf-8") as f:
            return set(re.findall(r"^- \[[ xX]\] \**(AC[-\w.]*\w)", f.read(), re.M))

    def test_the_plan_is_valid_and_proves_real_acs(self):
        self.assertEqual(kit.plan_problems(us001.Us001Run), [])
        self.assertIs(us001.Run, us001.Us001Run)
        self.assertEqual(us001.Us001Run.STORY, "US-001")
        self.assertEqual(us001.Us001Run.COVERS, ())
        self.assertEqual(us001.Us001Run.FEATURES, ("movement", "camera", "sprint"))
        ids = self.story_ac_ids()
        for step in us001.PLAN:
            for ac in step.proves:
                self.assertIn(ac, ids, step.check)
        for ac in us001.PENDING:
            self.assertIn(ac, ids)
        known = [s.check for s in us001.PLAN if s.known_issue]
        self.assertEqual(known, KNOWN_FAILS)
        for s in us001.PLAN:
            if s.known_issue:
                self.assertIn("Specs/agent_playtest_plan_v1.md", s.known_issue)

    def test_plan_prints_the_steps_table(self):
        out = io.StringIO()
        previous = us001.set_log_stream(out)
        self.addCleanup(us001.set_log_stream, previous)
        self.assertEqual(us001.main(["--plan"]), 0)
        rows = [line for line in out.getvalue().splitlines() if line.startswith("| ")]
        self.assertEqual(len(rows), 1 + len(us001.PLAN))
        self.assertTrue(any(r.startswith("| 18 | `jump_space` |") for r in rows))

    def test_the_route_stays_clear_of_pickups_and_obstacles(self):
        step = us001.MOVE_HOLD_S * fb.WALK_SPEED * 1.1   # a little long, as a real hold can be
        pos, corners = (0.0, 0.0), []
        dirs = {"A": (0, -1), "W": (1, 0), "D": (0, 1), "S": (-1, 0)}
        for _, key, _, _, _ in us001.MOVES:
            dx, dy = dirs[key]
            pos = (pos[0] + dx * step, pos[1] + dy * step)
            corners.append(pos)
        points = corners + [(x, y) for _, x, y in us001.LANE_WAYPOINTS] + [us001.LANE_START]
        for px, py in points:
            for spec in fb.PICKUP_SPECS:
                lx, ly = spec["location"][:2]
                self.assertGreater(math.hypot(px - lx, py - ly), fb.FOCUS_RANGE,
                                   f"({px:.0f}, {py:.0f}) is in range of {spec['label']}")
            for x0, x1, y0, y1 in fb.DEVTESTMAP_OBSTACLES:
                cx, cy = min(max(px, x0), x1), min(max(py, y0), y1)
                self.assertGreater(math.hypot(px - cx, py - cy), fb.CAPSULE_RADIUS)


if __name__ == "__main__":
    unittest.main()
