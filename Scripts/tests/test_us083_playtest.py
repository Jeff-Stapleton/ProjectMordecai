"""Tests for Scripts/playtests/us083_combat_input.py against tests/fake_bridge.py.

PadWorld (below) adds the gamepad combat the shared FakeWorld lacks: LT held blocks, a fresh RT
press past 0.5 while blocking parries (0.5 s Parrying, then 0.6 s ParryWhiff, the guard down), a
RT release is a light attack (held < 0.4 s) or a heavy one, B dodges and A jumps.  Switches break
each rule.  These prove the playtest's logic and reporting only; they say nothing about the real
game.  Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Us083
"""

import importlib.util
import io
import json
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
INPUT_AND_READ_CMDS = {"ping", "state", "key", "axis", "screenshot", "wait_until", "wait"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us083_combat_input", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us083 = load_playtest()
LT, RT = us083.LT, us083.RT
PLAN_CHECKS = [s.check for s in us083.PLAN]


class PadWorld(fb.FakeWorld):
    """The arena with the US-083 gamepad combat bindings.  Defaults model the game as it should
    be; the switches break one rule each.

    parry_on_partial: a RT pull below 0.5 while blocking parries.  parry_on_block_over_rt: raising
    the block over a held RT parries.  guard_through_parry: the parry leaves the guard up.
    attack_out_of_parry: the RT release during a parry attacks.  no_guard_back: the guard stays
    down after a parry.  heavy_as_light: a long RT hold is a light attack.  block, attacks, dodge,
    jump: False = that input does nothing."""

    PARRY_S, WHIFF_S, ATTACK_S, HEAVY_HOLD_S, DODGE_S, JUMP_S = 0.5, 0.6, 0.6, 0.4, 0.4, 0.6

    def __init__(self, *, block=True, attacks=True, dodge=True, jump=True,
                 parry_on_partial=False, parry_on_block_over_rt=False, guard_through_parry=False,
                 attack_out_of_parry=False, no_guard_back=False, heavy_as_light=False, **kw):
        super().__init__(**kw)
        self.block, self.attacks, self.dodge, self.jump = block, attacks, dodge, jump
        self.parry_on_partial = parry_on_partial
        self.parry_on_block_over_rt = parry_on_block_over_rt
        self.guard_through_parry = guard_through_parry
        self.attack_out_of_parry = attack_out_of_parry
        self.no_guard_back = no_guard_back
        self.heavy_as_light = heavy_as_light
        self.prev_lt = self.prev_rt = 0.0
        self.rt_down_at = None
        self.blocking = False
        self.parried_once = False
        self.parry_until = self.whiff_until = -1.0
        self.attack_until = self.dodge_until = self.jump_until = -1.0
        self.attack_heavy = False
        self.attacks_seen = []

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
                    self.attack_heavy = held_s >= self.HEAVY_HOLD_S and not self.heavy_as_light
                    self.attack_until = now + self.ATTACK_S
                    self.attacks_seen.append("heavy" if self.attack_heavy else "light")
            self.prev_lt, self.prev_rt = lt, rt

    def _on_key_pressed(self, key):
        super()._on_key_pressed(key)
        if not self.ready or self.menu_open or self.is_dead:
            return
        now = self.real_time()
        if key == us083.DODGE_KEY and self.dodge:
            self.dodge_until = now + self.DODGE_S
        elif key == us083.JUMP_KEY and self.jump:
            self.jump_until = now + self.JUMP_S

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
            tags.append("Mordecai.State.Dodging")
        out["tags"] = tags
        if now < self.jump_until:
            out["movement_mode"] = "Falling"
        return out


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

    def serve(self, **world_kwargs):
        world = PadWorld(**world_kwargs)
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
        self.assertTrue(rows[1].startswith("| 1 | `block_lt_hold` |"))


class TestUs083AgainstTheFake(Us083Case):
    def test_everything_passes_when_the_bindings_work(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(self.statuses(rep, "FAIL"), [], self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(code, 0)
        checks = self.by_name(rep)
        for name in PLAN_CHECKS:
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertIn(us083.TAG_PARRYING, checks["parry_rt_while_blocking"]["detail"])
        self.assertIn("no attack at any of", checks["guard_returns_after_parry"]["detail"])
        # The block-over-RT step ends with a heavy attack on the RT release; nothing else attacks
        # out of turn: light (tap), then heavy (hold)
        self.assertEqual(world.attacks_seen[-2:], ["light", "heavy"])
        # Real input only, and the triggers are released at the end
        sent = [e["msg"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual({m["cmd"] for m in sent}, INPUT_AND_READ_CMDS)
        self.assertFalse([m for m in sent if str(m.get("cmd", "")).startswith("debug")
                          or m.get("route") == "viewport" or m.get("unsafe")])
        self.assertEqual(world._held_union(), {})
        axes = [m["values"] for m in sent if m["cmd"] == "axis"]
        self.assertIn({LT: 1.0}, axes)
        self.assertIn({RT: us083.PARTIAL_RT}, axes)
        self.assertEqual([s["label"] for s in rep["screenshots"]], ["blocking"])
        acs = {a["id"]: a for a in rep["acs"]}
        self.assertEqual(acs["AC-083.10"]["status"], "PASS")
        self.assertEqual(acs["AC-083.3"]["status"], "PASS")
        self.assertEqual(acs["AC-083.11"]["status"], "PENDING")
        self.assertFalse(any(a["accepted"] for a in rep["acs"]), "an attached run accepts nothing")

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
        _, server = self.serve(attacks=False, dodge=False, jump=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for name in ("light_attack_rt_tap", "heavy_attack_rt_hold", "dodge_gamepad_b",
                     "jump_gamepad_a"):
            self.assertEqual(checks[name]["status"], "FAIL", name)
        self.assertIn("player.movement_mode stayed", checks["jump_gamepad_a"]["detail"])


if __name__ == "__main__":
    unittest.main()
