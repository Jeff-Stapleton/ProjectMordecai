"""Tests for Scripts/playtests/us083_combat_input.py against tests/fake_bridge.py.

PadWorld (below) adds the combat the shared FakeWorld lacks.  Gamepad: LT held blocks, a fresh RT
press past 0.5 while blocking parries (0.5 s Parrying, then 0.6 s ParryWhiff, the guard down), a
RT release is a light attack (held < 0.4 s) or a heavy one, B dodges and A jumps.  Keyboard and
mouse: a Left click released before 0.4 s is a light attack, one held longer a heavy attack (on
the release), LeftAlt dodges, SpaceBar jumps (Falling) and LeftShift toggles a sprint (walk 600
cm/s, sprint 900 cm/s).  An attack input while an attack runs is refused (State.Attacking blocks
the melee ability).  A dodge moves the player 140 cm backward (-X), reports Falling for part of
its active phase (its launch) and leaves a DodgeCooldown tag for 0.3 s.  The world is
MovementTestMap (fb.MAP_PRESETS[fb.MOVEMENT_TEST_MAP]: open floor, boundary walls at +-2000, the
jump step at +Y, no pickups, no enemies), the map the script launches.  Switches break each
rule.  These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k us083
"""

import importlib.util
import io
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

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us083_combat_input.py")
STORY_FILES = [os.path.join(os.path.dirname(SCRIPTS), "stories", d,
                            "US-083-recovery-combat-abilities-and-input.md")
               for d in ("in-progress", "done", "backlog")]
TOKEN = "us083-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "axis", "click", "screenshot", "wait_until",
                       "wait"}
LANE_CHECKS = ["walk_lane_wp1", "walk_lane_wp2", "walk_lane_start", "walk_lane_run_up",
               "sprint_left_shift"]
KB_MOUSE_CHECKS = ["light_attack_lmb_tap", "heavy_attack_lmb_hold", "dodge_left_alt",
                   "jump_space_bar"]
DODGE_CHECKS = ["dodge_gamepad_b", "dodge_left_alt"]
# A wall across the whole floor between the spawn and lane L1's first waypoint
LANE_BLOCKER = (-470.0, -440.0, -2000.0, 2000.0)


def load_playtest():
    spec = importlib.util.spec_from_file_location("us083_combat_input", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us083 = load_playtest()
LT, RT = us083.LT, us083.RT
PLAN_CHECKS = [s.check for s in us083.PLAN]


class PadWorld(fb.FakeWorld):
    """MovementTestMap with the US-083 combat bindings.  Defaults model the game as it should be;
    the switches break one rule each.

    Gamepad -- parry_on_partial: a RT pull below 0.5 while blocking parries.
    parry_on_block_over_rt: raising the block over a held RT parries.  guard_through_parry: the
    parry leaves the guard up.  attack_out_of_parry: the RT release during a parry attacks.
    no_guard_back: the guard stays down after a parry.  heavy_as_light: a long RT hold is a light
    attack.  block, attacks, dodge, jump: False = that gamepad input does nothing.

    Keyboard and mouse -- lmb, alt_dodge, space_jump, shift_sprint: False = the Left click /
    LeftAlt / SpaceBar / LeftShift does nothing.  lmb_heavy_as_light: a long Left click is a light
    attack.  lmb_tap_as_heavy: a short Left click is a heavy attack.  sticky_sprint: a second
    LeftShift tap does not end the sprint.

    Timing -- attack_s: how long an attack runs (ATTACK_S).  dodge_cooldown_s: how long the
    DodgeCooldown tag stays after a dodge (DODGE_COOLDOWN_S).

    attacks_seen: (input, "light" | "heavy") per attack, in order.  attacks_refused: the input of
    each attack refused because another one was running."""

    PARRY_S, WHIFF_S, ATTACK_S, HEAVY_HOLD_S, DODGE_S, JUMP_S = 0.5, 0.6, 0.6, 0.4, 0.4, 0.6
    DODGE_COOLDOWN_S = 0.3
    DODGE_CM = 140.0               # backward (-X); the game's B dodge + A jump reached x=-144
    # Falling for this part of the dodge's active phase (LaunchCharacter).  It starts after the
    # first read, so a read can see Dodging and Walking before the launch's Falling: a jump judged
    # then would take that Falling for a jump.  The dodge step's displacement measurement waits the
    # Dodging tag out, so the jump step normally starts after it; _on_the_ground's own Dodging
    # guard covers a dodge step that returned early (test_the_jump_waits_out_the_dodging_tag_* and
    # the *_even_when_the_dodge_step_did_not_wait tests).  The window lies inside DODGE_S.
    DODGE_FALLING_S = (0.2, 0.35)
    SPRINT_MULT = 1.5

    def __init__(self, *, block=True, attacks=True, dodge=True, jump=True,
                 parry_on_partial=False, parry_on_block_over_rt=False, guard_through_parry=False,
                 attack_out_of_parry=False, no_guard_back=False, heavy_as_light=False,
                 lmb=True, alt_dodge=True, space_jump=True, shift_sprint=True,
                 lmb_heavy_as_light=False, lmb_tap_as_heavy=False, sticky_sprint=False,
                 attack_s=None, dodge_cooldown_s=None, **kw):
        for key, value in fb.MAP_PRESETS[fb.MOVEMENT_TEST_MAP].items():
            kw.setdefault(key, value)
        super().__init__(**kw)
        self.block, self.attacks, self.dodge, self.jump = block, attacks, dodge, jump
        self.parry_on_partial = parry_on_partial
        self.parry_on_block_over_rt = parry_on_block_over_rt
        self.guard_through_parry = guard_through_parry
        self.attack_out_of_parry = attack_out_of_parry
        self.no_guard_back = no_guard_back
        self.heavy_as_light = heavy_as_light
        self.lmb, self.alt_dodge = lmb, alt_dodge
        self.space_jump, self.shift_sprint = space_jump, shift_sprint
        self.lmb_heavy_as_light = lmb_heavy_as_light
        self.lmb_tap_as_heavy = lmb_tap_as_heavy
        self.sticky_sprint = sticky_sprint
        self.attack_s = self.ATTACK_S if attack_s is None else float(attack_s)
        self.dodge_cooldown_s = (self.DODGE_COOLDOWN_S if dodge_cooldown_s is None
                                 else float(dodge_cooldown_s))
        self.base_speed = self.speed
        self.sprinting = False
        self.prev_lt = self.prev_rt = 0.0
        self.rt_down_at = None
        self.lmb_down_at = None
        self.blocking = False
        self.parried_once = False
        self.parry_until = self.whiff_until = -1.0
        self.attack_until = self.dodge_until = self.jump_until = -1.0
        self.dodge_at = -10.0
        self.attack_heavy = False
        self.attacks_seen = []
        self.attacks_refused = []

    def _attack(self, source, heavy):
        now = self.real_time()
        if now < self.attack_until:   # ActivationBlockedTags: State.Attacking
            self.attacks_refused.append(source)
            return
        self.attack_heavy = heavy
        self.attack_until = now + self.attack_s
        self.attacks_seen.append((source, "heavy" if heavy else "light"))

    def _dodge_back(self):
        """The dodge's displacement: no move input, so backward (-X, the facing is +X), DODGE_CM in
        10 cm steps that stop at a block."""
        for _ in range(int(self.DODGE_CM // 10)):
            if self._blocked(self.pos[0] - 10.0, self.pos[1]):
                break
            self.pos[0] -= 10.0

    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            if not self.ready or self.is_dead:
                return
            now = self.real_time()
            held = self._held_union()
            lt, rt = abs(held.get(LT, 0.0)), abs(held.get(RT, 0.0))
            in_parry = now < self.whiff_until
            lt_down, lt_was = lt >= 0.5, self.prev_lt >= 0.5
            rt_down, rt_was = rt >= 0.5, self.prev_rt >= 0.5
            # Parry: a fresh RT press while the block is up (or a broken rule)
            if self.block and lt_down and not in_parry and (
                    (rt_down and not rt_was)
                    or (self.parry_on_partial and 0.0 < rt < 0.5)
                    or (self.parry_on_block_over_rt and rt_down and not lt_was)):
                self.parry_until, self.whiff_until = now + self.PARRY_S, now + self.PARRY_S + self.WHIFF_S
                self.parried_once = True
                in_parry = True
            guard_ok = not (self.no_guard_back and self.parried_once and not in_parry)
            self.blocking = (self.block and lt_down and guard_ok
                             and (not in_parry or self.guard_through_parry))
            # Attacks fire on the RT release: light if held < HEAVY_HOLD_S, else heavy
            if rt_down and not rt_was:
                self.rt_down_at = now
            if rt_was and not rt_down and self.rt_down_at is not None:
                held_s = now - self.rt_down_at
                self.rt_down_at = None
                if self.attacks and (not in_parry or self.attack_out_of_parry):
                    self._attack("RT", held_s >= self.HEAVY_HOLD_S and not self.heavy_as_light)
            self.prev_lt, self.prev_rt = lt, rt

    def _on_key_pressed(self, key):
        super()._on_key_pressed(key)
        if not self.ready or self.menu_open or self.is_dead:
            return
        now = self.real_time()
        if (key == us083.DODGE_KEY and self.dodge) or (key == us083.KB_DODGE_KEY
                                                       and self.alt_dodge):
            if now >= self.dodge_until + self.dodge_cooldown_s:   # Dodging / DodgeCooldown block it
                self.dodge_at, self.dodge_until = now, now + self.DODGE_S
                self._dodge_back()
        elif (key == us083.JUMP_KEY and self.jump) or (key == us083.KB_JUMP_KEY
                                                       and self.space_jump):
            self.jump_until = now + self.JUMP_S
        elif key == us083.SPRINT_KEY and self.shift_sprint:
            self.sprinting = True if self.sticky_sprint else not self.sprinting
            self.speed = self.base_speed * (self.SPRINT_MULT if self.sprinting else 1.0)

    # Left click: light attack when released before HEAVY_HOLD_S, else a heavy one on the release
    def mouse_down(self, conn_id, x, y, button):
        with self.lock:
            hit = super().mouse_down(conn_id, x, y, button)
            if button == "Left":
                self.lmb_down_at = self.real_time()
            return hit

    def mouse_up(self, conn_id, x, y, button):
        with self.lock:
            hit = super().mouse_up(conn_id, x, y, button)
            if (button == "Left" and hit == "SViewport" and self.lmb and self.ready
                    and not self.is_dead and self.lmb_down_at is not None):
                held_s = self.real_time() - self.lmb_down_at
                self._attack("LMB", self.lmb_tap_as_heavy if held_s < self.HEAVY_HOLD_S
                             else not self.lmb_heavy_as_light)
            self.lmb_down_at = None
            return hit

    def _s_player(self):
        out = super()._s_player()
        if not out.get("valid"):
            return out
        now = self.real_time()
        tags = list(out.get("tags") or [])
        if self.blocking:
            tags.append(us083.TAG_BLOCKING)
        if now < self.parry_until:
            tags.append(us083.TAG_PARRYING)
        elif now < self.whiff_until:
            tags.append(us083.TAG_WHIFF)
        if now < self.attack_until:
            tags.append(us083.TAG_ATTACKING)
            if self.attack_heavy:
                tags.append(us083.TAG_HEAVY)
        if now < self.dodge_until:
            tags.append(us083.TAG_DODGING)
        elif now < self.dodge_until + self.dodge_cooldown_s:
            tags.append("Mordecai.State.DodgeCooldown")
        out["tags"] = tags
        fall0, fall1 = self.DODGE_FALLING_S
        if now < self.jump_until or fall0 <= now - self.dodge_at < fall1:
            out["movement_mode"] = "Falling"
        return out


class DiesAfterTheBlockWorld(PadWorld):
    """The player dies the moment LT is first released (block_release_lt) and never respawns:
    every later step finds a dead player."""

    def __init__(self, **kw):
        kw.setdefault("respawn_s", None)
        super().__init__(**kw)

    def tick(self, dt):
        with self.lock:
            was_blocking = self.prev_lt >= 0.5
            super().tick(dt)
            if was_blocking and self.prev_lt < 0.5:
                self.kill_player()


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class Us083Case(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        for module in (kit, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        patcher = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, world_class=None, **world_kwargs):
        world = (world_class or PadWorld)(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        code = us083.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)

    @staticmethod
    def by_name(rep):
        return {c["name"]: c for c in rep["checks"]}

    @staticmethod
    def statuses(rep, status):
        return [c["name"] for c in rep["checks"] if c["status"] == status]


class TestUs083Plan(unittest.TestCase):
    def test_plan_is_valid_and_proves_story_acs(self):
        self.assertEqual(kit.plan_problems(us083.Us083Run), [])
        self.assertIs(us083.Run, us083.Us083Run)
        self.assertEqual(us083.Us083Run.STORY, "US-083")
        self.assertEqual(us083.Us083Run.COVERS, ())
        story = next((p for p in STORY_FILES if os.path.isfile(p)), None)
        if story is None:
            self.skipTest("US-083 story file moved")
        with open(story, encoding="utf-8") as f:
            text = f.read()
        for step in us083.PLAN:
            for ac in step.proves:
                self.assertIn(f"{ac}:", text, step.check)
        for ac in us083.PENDING:
            self.assertIn(f"{ac}:", text)
        self.assertFalse([s.check for s in us083.PLAN if s.known_issue])

    def test_plan_prints_without_a_game(self):
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        self.addCleanup(kit.set_log_stream, previous)
        self.assertEqual(us083.main(["--plan"]), 0)
        rows = [line for line in out.getvalue().splitlines() if line.startswith("| ")]
        self.assertEqual(len(rows), 1 + len(us083.PLAN))
        # The lane walk and the sprint first (before any dodge), then the gamepad (10 steps), then
        # the keyboard and mouse
        self.assertTrue(rows[1].startswith("| 1 | `walk_lane_wp1` |"))
        self.assertEqual(PLAN_CHECKS[:5], LANE_CHECKS)
        self.assertEqual(PLAN_CHECKS[15:], KB_MOUSE_CHECKS)
        self.assertEqual(len(PLAN_CHECKS), 19)
        self.assertTrue(rows[6].startswith("| 6 | `block_lt_hold` |"))
        self.assertTrue(rows[16].startswith("| 16 | `light_attack_lmb_tap` |"))
        self.assertTrue(rows[-1].startswith("| 19 | `jump_space_bar` |"))

    def test_the_script_launches_the_movement_test_map(self):
        self.assertEqual(us083.Us083Run.launch_map(), kit.MOVEMENT_TEST_MAP)
        self.assertEqual(kit.MOVEMENT_TEST_MAP, fb.MOVEMENT_TEST_MAP)
        self.assertEqual(us083.Us083Run.expected_experience(), kit.DEFAULT_EXPERIENCE)
        self.assertNotIn("N14", " ".join(us083.KNOWN_ISSUES))
        self.assertNotIn("Frontliner", us083.__doc__)

    def test_the_route_and_the_sprint_lane_are_clear_on_the_movement_map(self):
        # The waypoints are DevTestMap's (the check names are kept); on MovementTestMap they and
        # the sprint along lane L1 must be open floor, far from the walls and the jump step
        boxes = ([b for _, b in fb.MOVEMENT_TEST_MAP_WALLS]
                 + list(fb.MOVEMENT_TEST_MAP_OBSTACLES))

        def clearance(px, py):
            return min(math.hypot(px - min(max(px, x0), x1), py - min(max(py, y0), y1))
                       for x0, x1, y0, y1 in boxes)

        points = ([(0.0, 0.0)] + [(x, y) for _, x, y, _ in us083.LANE_WAYPOINTS]
                  + [us083.LANE_START, us083.RUN_UP])
        for px, py in points:
            self.assertGreater(clearance(px, py), 4 * fb.CAPSULE_RADIUS, f"({px:.0f}, {py:.0f})")
        self.assertEqual(us083.LANE_START[1], fb.MOVEMENT_TEST_MAP_LANE_Y)
        self.assertEqual(us083.RUN_UP[1], fb.MOVEMENT_TEST_MAP_LANE_Y)
        # A long sprint from the run-up point (900 cm/s for 1.1 s plus its stop) stays on lane
        # L1.  After the shortest passing sprint, the S hold (600 cm/s for 1.1 s plus its stop)
        # and two full 400 cm backward dodges (N27 fixed) stay on open floor along y=-800
        far = us083.RUN_UP[0] + 900.0 * 1.1 + 300.0
        near = (us083.RUN_UP[0] + us083.SPRINT_MIN - (600.0 * 1.1 + 100.0)
                - 2 * us083.DODGE_DISTANCE_CM)
        self.assertLessEqual(far, 1600.0, "past the end of lane L1")
        x = near
        while x <= far:
            self.assertGreater(clearance(x, fb.MOVEMENT_TEST_MAP_LANE_Y), 4 * fb.CAPSULE_RADIUS,
                               f"y=-800 at x={x:.0f}")
            x += 50.0

    def test_keyboard_and_mouse_steps_prove_ac_083_4_to_8(self):
        proves = {s.check: s.proves for s in us083.PLAN}
        for check, ac in (("light_attack_lmb_tap", "AC-083.4"), ("heavy_attack_lmb_hold",
                                                                 "AC-083.5"),
                          ("dodge_left_alt", "AC-083.6"), ("jump_space_bar", "AC-083.7"),
                          ("sprint_left_shift", "AC-083.8")):
            self.assertEqual(proves[check], (ac, "AC-083.3"), check)
        for check in ("walk_lane_wp1", "walk_lane_wp2", "walk_lane_start", "walk_lane_run_up"):
            self.assertEqual(proves[check], (), check)
        self.assertIn("sprint", us083.Us083Run.FEATURES)


class TestUs083AgainstTheFake(Us083Case):
    def test_everything_passes_when_the_bindings_work(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(self.statuses(rep, "FAIL"), [], self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(code, 0)
        self.assertEqual(rep["undeclared_checks"], [])
        checks = self.by_name(rep)
        for name in PLAN_CHECKS + ["experience", "map"]:
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertIn(fb.MOVEMENT_TEST_MAP, checks["map"]["detail"])
        self.assertIn(us083.TAG_PARRYING, checks["parry_rt_while_blocking"]["detail"])
        self.assertIn("no attack at any of", checks["guard_returns_after_parry"]["detail"])
        self.assertIn("Slate hit SViewport", checks["light_attack_lmb_tap"]["detail"])
        self.assertIn("held 0.8s", checks["heavy_attack_lmb_hold"]["detail"])
        self.assertIn("the dodge over, Walking; tapped SpaceBar: Falling",
                      checks["jump_space_bar"]["detail"])
        self.assertIn("the dodge over, Walking; tapped Gamepad_FaceButton_Bottom: Falling",
                      checks["jump_gamepad_a"]["detail"])
        self.assertIn("need > 700", checks["sprint_left_shift"]["detail"])
        self.assertIn("then held S 1s: peak speed 600 cm/s", checks["sprint_left_shift"]["detail"])
        self.assertNotIn(" -- ", checks["sprint_left_shift"]["detail"])
        self.assertFalse(world.sprinting, "the sprint was toggled back")
        # The block-over-RT step ends with a heavy attack on the RT release; nothing else attacks
        # out of turn: RT light (tap), RT heavy (hold), then LMB light (click), LMB heavy (held)
        self.assertEqual(world.attacks_seen, [("RT", "heavy"), ("RT", "light"), ("RT", "heavy"),
                                              ("LMB", "light"), ("LMB", "heavy")])
        self.assertEqual(world.attacks_refused, [])
        # Real input only, and the triggers are released at the end
        entries = read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
        sent = [e["msg"] for e in entries if e["dir"] == "send"]
        self.assertLessEqual({m["cmd"] for m in sent}, INPUT_AND_READ_CMDS)
        self.assertFalse([m for m in sent if str(m.get("cmd", "")).startswith("debug")
                          or m.get("route") == "viewport" or m.get("unsafe")])
        self.assertEqual(world._held_union(), {})
        axes = [m["values"] for m in sent if m["cmd"] == "axis"]
        self.assertIn({LT: 1.0}, axes)
        self.assertIn({RT: us083.PARTIAL_RT}, axes)
        clicks = [(m.get("x"), m.get("y"), m.get("button"), m.get("hold_s"))
                  for m in sent if m["cmd"] == "click"]
        self.assertEqual(clicks, [(640, 360, "Left", None), (640, 360, "Left", 0.8)])
        keys = [m.get("key") or tuple(m.get("keys") or ()) for m in sent if m["cmd"] == "key"]
        for k in ("Gamepad_FaceButton_Right", "Gamepad_FaceButton_Bottom", "LeftAlt", "SpaceBar",
                  "W", "S"):
            self.assertIn(k, keys)
        self.assertEqual(keys.count("LeftShift"), 2, "the sprint toggle is tapped back")
        # The trigger axes are all released before the first click
        first_click = next(i for i, m in enumerate(sent) if m["cmd"] == "click")
        held = {}
        for m in sent[:first_click]:
            if m["cmd"] == "axis" and m.get("hold_s") is None:
                held.update(m["values"])
        self.assertEqual({k: v for k, v in held.items() if v}, {})
        # The lane walk comes first, from the spawn and before any dodge; its first pulse leads
        # west along y=0 (S); then along the lane (+X) to the sprint start
        walks = [i for i, e in enumerate(entries)
                 if e["dir"] == "note" and e["msg"].get("note") == "walk_to"]
        self.assertEqual([entries[i]["msg"]["to"] for i in walks],
                         [[-650.0, 0.0], [-700.0, -650.0], [-700.0, -800.0], [-300.0, -800.0]])
        start = entries[walks[0]]["msg"]["from"]
        self.assertLess(abs(start[0]), 30.0, "the spawn")
        self.assertLess(abs(start[1]), 30.0)
        first_dodge = next(i for i, e in enumerate(entries)
                           if e["dir"] == "send" and e["msg"]["cmd"] == "key"
                           and e["msg"].get("key") in (us083.DODGE_KEY, us083.KB_DODGE_KEY))
        self.assertLess(walks[-1], first_dodge, "every walk comes before the first dodge")
        first_pulse = next(e["msg"] for e in entries[walks[0]:]
                           if e["dir"] == "send" and e["msg"]["cmd"] == "key")
        self.assertEqual(first_pulse.get("keys") or [first_pulse.get("key")], ["S"])
        self.assertEqual([s["label"] for s in rep["screenshots"]], ["lane_start", "blocking"])
        movement = {m["check"]: m for m in rep["movement"]}
        self.assertGreater(movement["sprint_left_shift"]["dx"], 700)
        self.assertGreater(movement["sprint_left_shift"]["start"][0], -360.0,
                           "the sprint starts at the run-up point, not the lane start")
        self.assertLess(movement["sprint_left_shift_toggle_back"]["dx"], -400)
        self.assertEqual(movement["sprint_left_shift_toggle_back"]["peak_speed"], 600.0)
        # Each dodge's displacement is measured (N27), not judged: the fake dodges 140 cm -X
        self.assertEqual([m["check"] for m in rep["movement"]],
                         ["sprint_left_shift", "sprint_left_shift_toggle_back"] + DODGE_CHECKS)
        for name in DODGE_CHECKS:
            m = movement[name]
            self.assertEqual(m["kind"], "dodge", name)
            self.assertAlmostEqual(m["distance"], PadWorld.DODGE_CM, delta=1.0, msg=name)
            self.assertAlmostEqual(m["dx"], -PadWorld.DODGE_CM, delta=1.0, msg=name)
            self.assertAlmostEqual(m["dy"], 0.0, delta=1.0, msg=name)
            self.assertEqual(m["expected_cm"], us083.DODGE_DISTANCE_CM)
            self.assertIsNotNone(m["settle_s"], name)
            self.assertIn("displacement 140 cm", checks[name]["detail"])
            self.assertIn("N27: DodgeDistanceCm 400; measured, not judged", checks[name]["detail"])
        acs = {a["id"]: a for a in rep["acs"]}
        for ac in ("AC-083.3", "AC-083.4", "AC-083.5", "AC-083.6", "AC-083.7", "AC-083.8",
                   "AC-083.10"):
            self.assertEqual(acs[ac]["status"], "PASS", ac)
        self.assertEqual(acs["AC-083.11"]["status"], "PENDING")
        self.assertFalse(any(a["accepted"] for a in rep["acs"]), "an attached run accepts nothing")
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("## Sprint measurements", md)
        self.assertIn("## Dodge measurements (N27, not judged)", md)
        self.assertIn("| dodge_left_alt | tap LeftAlt (dodge, no move key) |", md)
        self.assertLess(md.index("## Sprint measurements"), md.index("## Dodge measurements"))

    def test_no_block_fails_and_skips_what_needs_it(self):
        _, server = self.serve(block=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["block_lt_hold"]["status"], "FAIL")
        self.assertIn("no 'Mordecai.State.Blocking'", checks["block_lt_hold"]["detail"])
        for name in ("parry_rt_while_blocking", "block_release_lt"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
        self.assertEqual(checks["guard_returns_after_parry"]["status"], "SKIP")
        self.assertEqual(checks["light_attack_rt_tap"]["status"], "PASS")

    def test_a_partial_trigger_pull_that_parries_fails(self):
        _, server = self.serve(parry_on_partial=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["partial_rt_no_parry"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("a trigger pulled below 0.5 must not parry", check["detail"])

    def test_a_block_raised_over_a_held_trigger_that_parries_fails(self):
        # The Started-binding bug: the chord alone made the parry fire
        _, server = self.serve(parry_on_block_over_rt=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["block_over_held_rt_no_parry"]["status"], "FAIL")
        self.assertIn("raising the block over a held attack button must not parry",
                      checks["block_over_held_rt_no_parry"]["detail"])
        self.assertEqual(checks["parry_rt_while_blocking"]["status"], "PASS")

    def test_a_parry_that_keeps_the_guard_fails(self):
        _, server = self.serve(guard_through_parry=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["parry_rt_while_blocking"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("the parry did not drop the guard", check["detail"])

    def test_an_attack_out_of_the_parry_fails(self):
        _, server = self.serve(attack_out_of_parry=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["guard_returns_after_parry"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("an attack fired during the parry", check["detail"])

    def test_a_guard_that_never_comes_back_fails(self):
        _, server = self.serve(no_guard_back=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["guard_returns_after_parry"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("the guard did not come back", check["detail"])

    def test_a_long_hold_that_starts_a_light_attack_fails_the_heavy_check(self):
        _, server = self.serve(heavy_as_light=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["light_attack_rt_tap"]["status"], "PASS")
        self.assertEqual(checks["heavy_attack_rt_hold"]["status"], "FAIL")
        self.assertIn("no 'Mordecai.Attack.Heavy'", checks["heavy_attack_rt_hold"]["detail"])

    def test_dead_buttons_fail(self):
        _, server = self.serve(attacks=False, dodge=False, jump=False, lmb=False,
                               alt_dodge=False, space_jump=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for name in ("light_attack_rt_tap", "heavy_attack_rt_hold", "dodge_gamepad_b",
                     "jump_gamepad_a", "light_attack_lmb_tap", "heavy_attack_lmb_hold",
                     "dodge_left_alt", "jump_space_bar"):
            self.assertEqual(checks[name]["status"], "FAIL", name)
        for name in ("jump_gamepad_a", "jump_space_bar"):
            self.assertIn("player.movement_mode stayed", checks[name]["detail"])
        self.assertIn("no 'Mordecai.State.Attacking' without 'Mordecai.Attack.Heavy'",
                      checks["light_attack_lmb_tap"]["detail"])
        self.assertIn("tapped LeftAlt: no Dodging tag", checks["dodge_left_alt"]["detail"])
        # LeftShift and the walk are independent of the dead buttons
        for name in ("walk_lane_wp1", "walk_lane_wp2", "walk_lane_start", "sprint_left_shift"):
            self.assertEqual(checks[name]["status"], "PASS", name)

    def test_dead_keyboard_and_mouse_keys_fail_only_their_checks(self):
        world, server = self.serve(lmb=False, alt_dodge=False, space_jump=False,
                                   shift_sprint=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"),
                         ["sprint_left_shift", "light_attack_lmb_tap", "heavy_attack_lmb_hold",
                          "dodge_left_alt", "jump_space_bar"])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual([s for s, _ in world.attacks_seen], ["RT", "RT", "RT"])
        acs = {a["id"]: a for a in rep["acs"]}
        for ac in ("AC-083.4", "AC-083.5", "AC-083.6", "AC-083.7", "AC-083.8", "AC-083.3"):
            self.assertEqual(acs[ac]["status"], "FAIL", ac)
        self.assertEqual(acs["AC-083.10"]["status"], "PASS")

    def test_an_lmb_hold_that_starts_a_light_attack_fails_the_lmb_heavy_check(self):
        world, server = self.serve(lmb_heavy_as_light=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["heavy_attack_lmb_hold"])
        checks = self.by_name(rep)
        self.assertEqual(checks["light_attack_lmb_tap"]["status"], "PASS")
        self.assertEqual(checks["heavy_attack_rt_hold"]["status"], "PASS")
        self.assertIn("no 'Mordecai.Attack.Heavy'", checks["heavy_attack_lmb_hold"]["detail"])
        self.assertEqual(world.attacks_seen[-2:], [("LMB", "light"), ("LMB", "light")])
        acs = {a["id"]: a for a in rep["acs"]}
        self.assertEqual(acs["AC-083.5"]["status"], "FAIL")
        self.assertEqual(acs["AC-083.4"]["status"], "PASS")

    def test_a_short_lmb_click_that_starts_a_heavy_attack_fails_the_lmb_light_check(self):
        world, server = self.serve(lmb_tap_as_heavy=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["light_attack_lmb_tap"])
        checks = self.by_name(rep)
        self.assertIn("no 'Mordecai.State.Attacking' without 'Mordecai.Attack.Heavy'",
                      checks["light_attack_lmb_tap"]["detail"])
        self.assertEqual(checks["heavy_attack_lmb_hold"]["status"], "PASS")
        self.assertEqual(world.attacks_seen[-2:], [("LMB", "heavy"), ("LMB", "heavy")])
        acs = {a["id"]: a for a in rep["acs"]}
        self.assertEqual(acs["AC-083.4"]["status"], "FAIL")
        self.assertEqual(acs["AC-083.5"]["status"], "PASS")

    def test_each_attack_waits_for_the_previous_one_to_end(self):
        # A running attack refuses the next attack input (State.Attacking blocks the melee
        # ability).  With 1.2 s attacks, an attack step that did not wait for idle would press
        # into the previous attack (the block-over-RT step ends with a heavy one) and be refused
        world, server = self.serve(attack_s=1.2)
        code, rep = self.play(server)
        self.assertEqual(self.statuses(rep, "FAIL"), [], self.log.getvalue()[-4000:])
        self.assertEqual(code, 0)
        self.assertEqual(world.attacks_refused, [])
        self.assertEqual(world.attacks_seen, [("RT", "heavy"), ("RT", "light"), ("RT", "heavy"),
                                              ("LMB", "light"), ("LMB", "heavy")])

    def test_a_dead_space_bar_does_not_pass_on_the_dodges_falling(self):
        # The LeftAlt dodge reports Falling after a first Walking read (DODGE_FALLING_S): the
        # dodge step waits for the dodge to end (Dodging gone) before the jump step, so a dead
        # SpaceBar FAILs instead of passing on the dodge's launch
        _, server = self.serve(space_jump=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["jump_space_bar"])
        checks = self.by_name(rep)
        self.assertEqual(checks["dodge_left_alt"]["status"], "PASS")
        self.assertIn("the dodge over, Walking; tapped SpaceBar: player.movement_mode stayed",
                      checks["jump_space_bar"]["detail"])
        self.assertEqual({a["id"]: a for a in rep["acs"]}["AC-083.7"]["status"], "FAIL")

    def test_a_dead_a_button_does_not_pass_on_the_dodges_falling(self):
        # The same for the gamepad: the B dodge also reports Falling after a first Walking read;
        # the dodge step waits for that dodge to end before jump_gamepad_a, so a dead A FAILs
        # instead of passing on its launch
        _, server = self.serve(jump=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["jump_gamepad_a"])
        checks = self.by_name(rep)
        self.assertEqual(checks["dodge_gamepad_b"]["status"], "PASS")
        self.assertIn("the dodge over, Walking; tapped Gamepad_FaceButton_Bottom: "
                      "player.movement_mode stayed", checks["jump_gamepad_a"]["detail"])
        self.assertEqual(checks["jump_space_bar"]["status"], "PASS")

    def test_the_jump_waits_out_the_dodging_tag_not_only_walking(self):
        # _on_the_ground on its own, with no dodge step before it: right after the B tap a read
        # shows Dodging and Walking (the launch's Falling comes later, DODGE_FALLING_S), so it
        # must keep waiting until the Dodging tag is gone, not return on that Walking
        world, server = self.serve()
        os.makedirs(self.run_dir, exist_ok=True)
        run = us083.Us083Run(us083.parse_args(["--attach-port", str(server.port), "--token",
                                               TOKEN, "--run-dir", self.run_dir]))
        run.bridge = ab.Bridge.attach(server.port, token=TOKEN, run_dir=self.run_dir,
                                      wait_ready=True, timeout=10)
        self.addCleanup(run.bridge.close)
        run.bridge.tap(us083.DODGE_KEY, hold_s=run.TAP_S)
        self.assertLess(world.real_time(), world.dodge_until, "the tap outlasted the dodge")
        why, what = run._on_the_ground()
        self.assertIsNone(why, why)
        self.assertEqual(what, "the dodge over, Walking")
        self.assertGreaterEqual(world.real_time(), world.dodge_until)

    def test_a_dead_space_bar_fails_even_when_the_dodge_step_did_not_wait(self):
        # _dodge_displacement returns at once when there is no position before the tap: the dodge
        # step then ends on its first Dodging read, and jump_space_bar's own wait (no Dodging tag,
        # Walking) must keep a dead SpaceBar from passing on the LeftAlt dodge's launch Falling
        _, server = self.serve(space_jump=False)
        with mock.patch.object(us083.Us083Run, "_dodge_displacement",
                               lambda self, check, key, p0: ""):
            code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["jump_space_bar"])
        self.assertIn("the dodge over, Walking; tapped SpaceBar: player.movement_mode stayed",
                      self.by_name(rep)["jump_space_bar"]["detail"])

    def test_a_dead_a_button_fails_even_when_the_dodge_step_did_not_wait(self):
        # The same for the gamepad: jump_gamepad_a's own wait keeps a dead A from passing on the
        # B dodge's launch Falling
        _, server = self.serve(jump=False)
        with mock.patch.object(us083.Us083Run, "_dodge_displacement",
                               lambda self, check, key, p0: ""):
            code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["jump_gamepad_a"])
        self.assertIn("the dodge over, Walking; tapped Gamepad_FaceButton_Bottom: "
                      "player.movement_mode stayed", self.by_name(rep)["jump_gamepad_a"]["detail"])

    def test_a_dead_left_alt_does_not_pass_on_a_leftover_dodge_cooldown(self):
        # The B dodge's DodgeCooldown lasts into the LeftAlt step here (3 s): dodge_left_alt waits
        # for every dodge tag to go, so a dead LeftAlt FAILs instead of passing on that tag
        _, server = self.serve(alt_dodge=False, dodge_cooldown_s=3.0)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["dodge_left_alt"])
        self.assertIn("tapped LeftAlt: no Dodging tag",
                      self.by_name(rep)["dodge_left_alt"]["detail"])
        self.assertEqual({a["id"]: a for a in rep["acs"]}["AC-083.6"]["status"], "FAIL")

    def test_a_left_shift_that_does_nothing_fails_the_sprint(self):
        world, server = self.serve(shift_sprint=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["sprint_left_shift"])
        check = self.by_name(rep)["sprint_left_shift"]
        self.assertIn("need > 700", check["detail"])
        self.assertIn("peak speed 600 cm/s", check["detail"])
        self.assertNotIn(" -- ", check["detail"])
        self.assertFalse(world.sprinting)
        movement = {m["check"]: m for m in rep["movement"]}
        self.assertLess(movement["sprint_left_shift"]["dx"], 700)
        self.assertEqual({a["id"]: a for a in rep["acs"]}["AC-083.8"]["status"], "FAIL")

    def test_a_sprint_the_second_tap_does_not_end_fails(self):
        world, server = self.serve(sticky_sprint=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["sprint_left_shift"])
        check = self.by_name(rep)["sprint_left_shift"]
        self.assertIn("the second tap did not end the sprint", check["detail"])
        self.assertIn("peak speed 900 cm/s", check["detail"])
        self.assertTrue(world.sprinting)
        movement = {m["check"]: m for m in rep["movement"]}
        self.assertGreater(movement["sprint_left_shift"]["dx"], 700)
        self.assertTrue(movement["sprint_left_shift"]["sprint_left_on"])

    def test_a_blocked_lane_walk_fails_and_skips_the_sprint(self):
        _, server = self.serve(obstacles=fb.MOVEMENT_TEST_MAP_OBSTACLES + (LANE_BLOCKER,))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["walk_lane_wp1"])
        checks = self.by_name(rep)
        self.assertIn("walk_lane_wp1", rep["new_failures"])
        for name in ("walk_lane_wp2", "walk_lane_start", "screenshot_lane_start",
                     "walk_lane_run_up", "sprint_left_shift"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
        self.assertIn("previous waypoint was not reached", checks["walk_lane_wp2"]["detail"])
        self.assertIn("not at the lane start", checks["walk_lane_run_up"]["detail"])
        self.assertIn("not at the run-up point", checks["sprint_left_shift"]["detail"])
        for name in ("light_attack_lmb_tap", "heavy_attack_lmb_hold", "dodge_left_alt",
                     "jump_space_bar"):
            self.assertEqual(checks[name]["status"], "PASS", name)
        acs = {a["id"]: a for a in rep["acs"]}
        self.assertEqual(acs["AC-083.8"]["status"], "NOT RUN")
        self.assertEqual(acs["AC-083.3"]["status"], "NOT RUN")
        self.assertEqual(acs["AC-083.7"]["status"], "PASS")
        # No sprint was measured; the dodges still were
        self.assertEqual([m["check"] for m in rep["movement"]], DODGE_CHECKS)

    def test_the_movement_map_has_nothing_in_the_way(self):
        # MovementTestMap has no enemies and no pickups (DevTestMap's Frontliners stood on this
        # route and body-blocked it, N14): the walk to the sprint start needs no side-step
        world, server = self.serve()
        self.assertEqual(world.map, fb.MOVEMENT_TEST_MAP)
        self.assertEqual(world.enemies, [])
        self.assertEqual(world.world_pickups, [])
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-4000:])
        checks = self.by_name(rep)
        for name in LANE_CHECKS[:4]:
            self.assertRegex(checks[name]["detail"], r"(?<!\d)0 side-steps", name)

    def test_a_dead_player_is_not_idle(self):
        # The player dies when LT is released (block_release_lt) and stays dead: every later step
        # FAILs saying the player is dead, and no attack, dodge or jump input is sent to it
        world, server = self.serve(DiesAfterTheBlockWorld)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertTrue(world.is_dead)
        checks = self.by_name(rep)
        dead_checks = ["partial_rt_no_parry", "block_over_held_rt_no_parry", "light_attack_rt_tap",
                       "heavy_attack_rt_hold", "dodge_gamepad_b", "jump_gamepad_a",
                       "light_attack_lmb_tap", "heavy_attack_lmb_hold", "dodge_left_alt",
                       "jump_space_bar"]
        self.assertEqual(self.statuses(rep, "FAIL"), dead_checks)
        for name in dead_checks:
            self.assertIn("the player is dead", checks[name]["detail"], name)
            self.assertIn(us083.TAG_DEAD, checks[name]["detail"], name)
        for name in LANE_CHECKS + ["block_lt_hold", "parry_rt_while_blocking",
                                   "guard_returns_after_parry", "block_release_lt"]:
            self.assertEqual(checks[name]["status"], "PASS", name)
        self.assertEqual(world.attacks_seen, [])
        sent = [e["msg"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertFalse([m for m in sent if m["cmd"] == "click"], "no click on a dead player")
        keys = [m.get("key") for m in sent if m["cmd"] == "key"]
        for k in (us083.DODGE_KEY, us083.JUMP_KEY, us083.KB_DODGE_KEY, us083.KB_JUMP_KEY):
            self.assertNotIn(k, keys)
        # No dodge was played, so none was measured
        self.assertEqual([m["check"] for m in rep["movement"]],
                         ["sprint_left_shift", "sprint_left_shift_toggle_back"])


if __name__ == "__main__":
    unittest.main()
