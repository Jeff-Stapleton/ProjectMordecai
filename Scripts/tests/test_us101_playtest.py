"""Tests for Scripts/playtests/us101_defenses.py against tests/fake_bridge.py.

DefenseWorld (below) extends CombatWorld (tests/test_us054_combat_playtest.py: CombatTestMap, the
TestAttacker at (2100, 0) following AMordecaiEnemyAIController -- Idle until the player comes
within 900 cm, then Approach, Attack within 150 cm; the hit lands 0.5 s into each 1.1 s swing,
then Recover for 1.5 s; the death at health 0 and the respawn) with the defenses as US-101 makes
them work:
  - LT held (>= 0.5) blocks: a hit takes health x 0.5, stamina 0.7 x damage, posture x 0.5; a hit
    within perfect_window_s (0.25 s) of raising the block drains no stamina (a perfect block).
  - A fresh RT press while LT is held parries: 20 stamina, a 0.5 s window with the guard down; a
    hit in it does nothing to the player, gives the player Riposte and the attacker Parried, and
    ends the parry without a whiff; a parry that meets no hit whiffs for 0.6 s.
  - LeftAlt dodges: 15 stamina, Mordecai.State.Dodging for 0.4 s (i-frames: a hit does nothing),
    then DodgeCooldown 0.3 s.  With W held the dodge goes +X and stops at the attacker's capsule
    (80 cm, centre to centre); with no move key it goes 110 cm backward (-X, N27).  Move keys do
    nothing while Dodging (no steering mid-dodge, AC-005.13).
  - SpaceBar jumps: movement_mode Falling for 0.7 s with Mordecai.State.Airborne; the JumpAvoidable
    BasicSlash skips an Airborne target.
  - Stamina never regenerates (N30); posture starts at 100 and never regenerates for the player.
Switches break it: defenses=False with airborne_tag=False is the N32 bug (every swing does the
full 15; the parry never receives a hit, so it whiffs; no Airborne tag); perfect_window_s narrows
the perfect-block window; dodge_ignores_input makes every dodge go backward, out of reach.
These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k us101
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
from tests.test_us054_combat_playtest import CombatWorld  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us101_defenses.py")
STORY_FILES = [os.path.join(os.path.dirname(SCRIPTS), "stories", d,
                            "US-101-recovery-defenses-against-enemy-melee.md")
               for d in ("in-progress", "done", "backlog")]
TOKEN = "us101-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "axis", "screenshot", "wait_until", "wait"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us101_defenses", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us101 = load_playtest()
LT, RT = us101.LT, us101.RT
PLAN_ORDER = ["walk_to_fight_spot", "undefended_hit", "block_mitigates", "perfect_block",
              "dodge_evades", "jump_evades", "parry_negates"]
DEFENSE_CHECKS = PLAN_ORDER[2:]
SHOTS = ["fight_spot", "undefended", "blocked", "perfect_block", "dodged", "jumped", "parried"]


class DefenseWorld(CombatWorld):
    """CombatTestMap with a swinging TestAttacker and the player's defenses (module docstring)."""

    CONTACT_CM = 80.0          # player + attacker capsule radii
    BACKWARD_DODGE_CM = 110.0  # N27
    PARRY_S, WHIFF_S, DODGE_S, DODGE_COOLDOWN_S = 0.5, 0.6, 0.4, 0.3
    PARRY_COST, DODGE_COST = 20.0, 15.0

    def __init__(self, *, defenses=True, airborne_tag=True, perfect_window_s=0.25,
                 dodge_ignores_input=False, jump_s=0.7, **kw):
        super().__init__(**kw)
        self.defenses, self.airborne_tag = defenses, airborne_tag
        self.perfect_window_s = float(perfect_window_s)
        self.dodge_ignores_input = dodge_ignores_input
        self.jump_s = float(jump_s)
        self.stamina = self.posture = 100.0
        self.prev_rt = 0.0
        self.blocking, self.block_since = False, None
        self.parry_active, self.parry_hit = False, False
        self.parry_until = self.whiff_until = -1.0
        self.riposte = self.attacker_parried = False
        self.dodge_until = self.dodge_cd_until = self.jump_until = -1.0
        self.defense_log = []      # (real time, kind, health, stamina, posture taken) per hit
        self.dodges = []           # (x before, x after)
        self.parries = self.jumps = 0

    def attacker(self):
        return next(e for e in self.enemies if self.is_attacker(e))

    # --- simulation -------------------------------------------------------------------------
    def tick(self, dt):
        with self.lock:
            if self.ready and not self.menu_open and not self.is_dead:
                self._pad(self.real_time())
        super().tick(dt)

    def _pad(self, now):
        held = self._held_union()
        lt, rt = abs(held.get(LT, 0.0)), abs(held.get(RT, 0.0))
        lt_down, rt_down, rt_was = lt >= 0.5, rt >= 0.5, self.prev_rt >= 0.5
        if self.parry_active and now >= self.parry_until:
            self.parry_active = False
            if not self.parry_hit:
                self.whiff_until = now + self.WHIFF_S
        if lt_down and rt_down and not rt_was and not self.parry_active and now >= self.whiff_until:
            self.parry_active, self.parry_hit = True, False
            self.parry_until = now + self.PARRY_S
            self.stamina = max(0.0, self.stamina - self.PARRY_COST)
            self.parries += 1
        guard = lt_down and not self.parry_active and now >= self.whiff_until
        if guard and not self.blocking:
            self.block_since = now
        self.blocking = guard
        self.prev_rt = rt

    def _move(self, dt):
        if self.real_time() < self.dodge_until:
            self.vel = [0.0, 0.0]   # AC-005.13: no steering mid-dodge (the launch is the move)
            return
        super()._move(dt)

    def _blocked(self, x, y):
        # The attacker's capsule stops the player (the base fake only has boxes)
        return super()._blocked(x, y) or any(
            math.hypot(x - e["location"][0], y - e["location"][1]) < self.CONTACT_CM
            for e in self.enemies if not e.get("is_dead"))

    def _on_key_pressed(self, key):
        if self.ready and not self.menu_open and not self.is_dead:
            now = self.real_time()
            if key == us101.DODGE_KEY:
                self._dodge(now)
                return
            if key == us101.JUMP_KEY:
                if now >= self.jump_until:
                    self.jump_until = now + self.jump_s
                    self.jumps += 1
                return
        super()._on_key_pressed(key)

    def _dodge(self, now):
        if now < self.dodge_cd_until:
            return
        self.stamina = max(0.0, self.stamina - self.DODGE_COST)
        self.dodge_until = now + self.DODGE_S
        self.dodge_cd_until = self.dodge_until + self.DODGE_COOLDOWN_S
        x0 = self.pos[0]
        if "W" in self._held_union() and not self.dodge_ignores_input:
            a = self.attacker()["location"]
            limit = a[0] - self.CONTACT_CM if abs(a[1] - self.pos[1]) < self.CONTACT_CM \
                and a[0] > self.pos[0] else math.inf
            self.pos[0] = min(self.pos[0] + self.BACKWARD_DODGE_CM, limit)
        else:
            self.pos[0] -= self.BACKWARD_DODGE_CM
        self.dodges.append((round(x0, 1), round(self.pos[0], 1)))

    def airborne_on(self, now):
        return self.airborne_tag and now < self.jump_until

    def _defense(self, now):
        if self.airborne_on(now):
            return "evaded"  # JumpAvoidable: the hit query skips an Airborne target
        if not self.defenses:
            return "none"    # N32: nothing consults the defenses
        if now < self.dodge_until:
            return "evaded"
        if self.parry_active:
            return "parried"
        if self.blocking:
            return "perfect" if now - self.block_since <= self.perfect_window_s else "blocked"
        return "none"

    def _hit_player(self, now):
        kind = self._defense(now)
        dmg = self.hit_damage
        if kind in ("evaded", "parried"):
            dh = ds = dp = 0.0
            if kind == "parried":
                self.riposte = self.attacker_parried = True
                self.parry_hit, self.parry_active, self.parry_until = True, False, now
        elif kind in ("perfect", "blocked"):
            dh, dp = dmg * 0.5, dmg * 0.3 * 0.5
            ds = 0.0 if kind == "perfect" else dmg * 0.7
        else:
            dh, ds, dp = dmg, 0.0, dmg * 0.3
        self.health = max(0.0, self.health - dh)
        self.stamina = max(0.0, self.stamina - ds)
        self.posture = max(0.0, self.posture - dp)
        self.hits.append((round(now, 2), self.health))
        self.defense_log.append((round(now, 3), kind, dh, round(ds, 3), round(dp, 3)))
        if self.health <= 0:
            self.deaths += 1
            self.kill_player()

    # --- state ------------------------------------------------------------------------------------
    def _s_player(self):
        d = super()._s_player()
        if not d.get("valid"):
            return d
        now = self.real_time()
        t = list(d["tags"])
        if self.blocking:
            t.append(us101.TAG_BLOCKING)
        if self.parry_active:
            t.append(us101.TAG_PARRYING)
        elif now < self.whiff_until:
            t.append(us101.TAG_WHIFF)
        if self.riposte:
            t.append(us101.TAG_RIPOSTE)
        if now < self.dodge_until:
            t.append(us101.TAG_DODGING)
        elif now < self.dodge_cd_until:
            t.append("Mordecai.State.DodgeCooldown")
        if self.airborne_on(now):
            t.append(us101.TAG_AIRBORNE)
        d["tags"] = t
        if not self.is_dead and now < self.jump_until:
            d["movement_mode"] = "Falling"
        d["attributes"].update(stamina=self.stamina, posture=self.posture)
        return d

    def _actors(self, radius, include_all):
        entries = super()._actors(radius, include_all)
        for a in entries:
            if a.get("kind") == "enemy" and self.is_attacker(a):
                a["tags"] = [us101.TAG_PARRIED] if self.attacker_parried else []
        return entries


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class Us101Case(unittest.TestCase):
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
        world = DefenseWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        code = us101.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)

    def md(self):
        with open(os.path.join(self.run_dir, "report.md"), encoding="utf-8") as f:
            return f.read()

    @staticmethod
    def by_name(rep):
        return {c["name"]: c for c in rep["checks"]}

    @staticmethod
    def statuses(rep, status):
        return [c["name"] for c in rep["checks"] if c["status"] == status]

    @staticmethod
    def acs(rep):
        return {(a["story"], a["id"]): a for a in rep["acs"]}

    def tail(self):
        return self.log.getvalue()[-5000:]


# ---------------------------------------------------------------------------
# Declarations
# ---------------------------------------------------------------------------
class TestUs101Plan(unittest.TestCase):
    def test_declarations(self):
        run = us101.Us101Run
        self.assertEqual(kit.plan_problems(run), [])
        self.assertIs(us101.Run, run)
        self.assertEqual(run.STORY, "US-101")
        self.assertEqual(run.RUN_SUFFIX, "us101")
        self.assertEqual(run.launch_map(), kit.COMBAT_TEST_MAP)
        self.assertIn("MAP", run.__dict__)
        self.assertEqual(run.expected_experience(), kit.DEFAULT_EXPERIENCE)
        self.assertEqual(run.FEATURES, ("block_parry", "dodge", "stamina", "enemies", "movement"))
        self.assertEqual(set(run.COVERS), {"US-003", "US-005", "US-006"})
        self.assertEqual(run.PENDING, {})

    def test_plan_order_and_proves(self):
        plan = {s.check: s for s in us101.PLAN}
        self.assertEqual([s.check for s in us101.PLAN], PLAN_ORDER)
        self.assertEqual(plan["walk_to_fight_spot"].proves, ())
        own = {name: [a for a in plan[name].proves if not a.startswith("US-")]
               for name in PLAN_ORDER}
        self.assertEqual(own, {"walk_to_fight_spot": [], "undefended_hit": ["AC-101.10"],
                               "block_mitigates": ["AC-101.11"], "perfect_block": ["AC-101.12"],
                               "dodge_evades": ["AC-101.13"], "jump_evades": ["AC-101.14"],
                               "parry_negates": ["AC-101.15"]})
        self.assertEqual(plan["block_mitigates"].proves[1:], ("US-006:AC-006.3", "US-006:AC-006.4"))
        self.assertEqual(plan["dodge_evades"].proves[1:], ("US-005:AC-005.3",))
        self.assertEqual(plan["jump_evades"].proves[1:], ("US-003:AC-003.7",))
        self.assertFalse([s.check for s in us101.PLAN if s.known_issue])
        self.assertTrue(all(s.kind == "gameplay" for s in us101.PLAN))

    def test_the_story_names_every_proved_ac(self):
        story = next((p for p in STORY_FILES if os.path.isfile(p)), None)
        if story is None:
            self.skipTest("US-101 story file moved")
        with open(story, encoding="utf-8") as f:
            text = f.read()
        for step in us101.PLAN:
            for ac in step.proves:
                if not ac.startswith("US-"):
                    self.assertIn(f"{ac}:", text, step.check)
            self.assertIn(f"`{step.check}`", text, step.check)

    def test_spots_and_budgets(self):
        self.assertEqual(us101.FIGHT_SPOT, fb.COMBAT_ATTACKER_FIGHT_SPOT)
        self.assertEqual(list(us101.ATTACKER_SPAWN), fb.TEST_ATTACKER_SPEC["location"][:2])
        # The fight spot is inside the reach even when walk_to stops SPOT_TOL off
        self.assertLess(math.dist(us101.FIGHT_SPOT, us101.ATTACKER_SPAWN) + us101.SPOT_TOL,
                        us101.REACH_CM)
        self.assertEqual((us101.BLOCKED_HEALTH, us101.BLOCKED_STAMINA, us101.FULL_POSTURE),
                         (7.5, 10.5, 4.5))
        # Stamina never regenerates (N30): the run never empties it (no guard break)
        left = (100.0 - us101.BLOCKED_STAMINA - us101.DODGE_STAMINA - us101.PARRY_STAMINA)
        self.assertGreater(left, us101.BLOCKED_STAMINA)
        # Each timed input lands inside its window around the nominal hit
        hit = us101.NOMINAL_HIT_S
        self.assertTrue(0 < hit - us101.PERFECT_BLOCK_DELAY_S < us101.PERFECT_WINDOW_S)
        self.assertTrue(us101.DODGE_DELAY_S < hit < us101.DODGE_DELAY_S + 0.4)
        self.assertTrue(us101.PARRY_DELAY_S < hit < us101.PARRY_DELAY_S + 0.5)
        self.assertTrue(us101.JUMP_DELAY_S < hit < us101.JUMP_DELAY_S + 0.6)

    def test_plan_prints_without_a_game(self):
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        self.addCleanup(kit.set_log_stream, previous)
        self.assertEqual(us101.main(["--plan"]), 0)
        rows = [line for line in out.getvalue().splitlines() if line.startswith("| ")]
        self.assertEqual(len(rows), 1 + len(PLAN_ORDER))
        for i, name in enumerate(PLAN_ORDER, 1):
            self.assertTrue(rows[i].startswith(f"| {i} | `{name}` |"), rows[i])


# ---------------------------------------------------------------------------
# Pure helpers and judges
# ---------------------------------------------------------------------------
class TestUs101Helpers(unittest.TestCase):
    NAME = "BP_MordecaiEnemy_TestAttacker_C_1"

    def st(self, ai, index=1, name=None):
        actors = [{"kind": "pawn", "name": "B_Hero_Mordecai_C_0", "ai_state": None}]
        actors += [{"kind": "enemy", "name": f"Other_{i}", "ai_state": "Idle"}
                   for i in range(index - 1)]
        actors.append({"kind": "enemy", "name": name or self.NAME, "label": "TestAttacker",
                       "class": "BP_MordecaiEnemy_TestAttacker_C", "ai_state": ai})
        return {"actors": actors}

    def test_attacker_conditions_find_it_at_any_index(self):
        attack = fb.Condition(us101.attacker_condition(self.NAME, True))
        other = fb.Condition(us101.attacker_condition(self.NAME, False))
        self.assertEqual(attack.roots, ["actors"])
        for index in (1, 2, 4):
            self.assertTrue(attack.evaluate(self.st("Attack", index))[0], index)
            self.assertFalse(other.evaluate(self.st("Attack", index))[0], index)
            for state in ("Recover", "Staggered", "Approach", "Idle"):
                self.assertFalse(attack.evaluate(self.st(state, index))[0], state)
                self.assertTrue(other.evaluate(self.st(state, index))[0], state)
        # Another enemy attacking, or the attacker out of the first ACTOR_SLOTS, never matches
        self.assertFalse(attack.evaluate(self.st("Attack", name="Someone_Else"))[0])
        self.assertFalse(attack.evaluate(self.st("Attack", index=us101.ACTOR_SLOTS))[0])
        self.assertFalse(other.evaluate({"actors": {"valid": False}})[0])
        self.assertLess(len(us101.attacker_condition(self.NAME, False)), fb.MAX_CONDITION_LENGTH)

    def test_find_attacker(self):
        st = {"actors": [{"kind": "pawn", "name": "Hero"},
                         {"kind": "enemy", "name": "Enemy_0", "label": "Enemy_0",
                          "class": "BP_MordecaiEnemy_TestAttacker_C"},
                         {"kind": "enemy", "name": "TestDummy", "label": "TestDummy",
                          "class": "BP_MordecaiEnemy_TestDummy_C"}]}
        self.assertEqual(us101.find_attacker(st)["name"], "Enemy_0")       # by class
        self.assertEqual(us101.find_attacker(st, "Enemy_0")["name"], "Enemy_0")
        st["actors"][2]["label"] = "TestAttacker"
        self.assertEqual(us101.find_attacker(st)["name"], "TestDummy")     # by label first
        self.assertEqual(us101.find_attacker(st, "Enemy_0")["name"], "Enemy_0")  # exact name wins
        self.assertIsNone(us101.find_attacker({"actors": {"valid": False}}))

    def test_shift_is_clamped(self):
        self.assertAlmostEqual(us101.shift_for(us101.NOMINAL_HIT_S + 0.05), 0.05)
        self.assertEqual(us101.shift_for(5.0), us101.MAX_SHIFT_S)
        self.assertEqual(us101.shift_for(0.0), -us101.MAX_SHIFT_S)

    def test_reach_at_hit(self):
        samples = [{"t": 0.30, "dist": 80.0}, {"t": 0.48, "dist": 90.0},
                   {"t": 0.55, "dist": 120.0}, {"t": 0.90, "dist": 400.0}]
        ok, text = us101.reach_at_hit(samples, 0.52)
        self.assertTrue(ok, text)
        self.assertIn("2 read(s) within 0.1s", text)
        self.assertIn("90..120 cm", text)
        ok, text = us101.reach_at_hit([{"t": 0.50, "dist": 210.0}], 0.52)
        self.assertFalse(ok, text)
        ok, text = us101.reach_at_hit([{"t": 0.20, "dist": 80.0}], 0.52)   # nearest read
        self.assertTrue(ok)
        self.assertIn("the read nearest", text)
        self.assertEqual(us101.reach_at_hit([{"t": 0.5, "dist": None}], 0.52)[0], None)

    @staticmethod
    def rec(health, stamina=0.0, posture=0.0, samples=(), **kw):
        before = {"health": 70.0, "stamina": 100.0, "posture": 90.0}
        after = {"health": 70.0 - health, "stamina": 100.0 - stamina, "posture": 90.0 - posture}
        rec = {"before": before, "after": after, "spent": us101.spent(before, after),
               "timeline": [], "samples": list(samples), "hit_s": 0.52, "expected_hit_s": 0.52,
               "after_tags": [], "after_mode": "Walking", "attacker_after_tags": []}
        rec.update(kw)
        return rec

    def test_judge_undefended_and_block(self):
        self.assertEqual(us101.judge_undefended(self.rec(15.0, posture=4.5))[0], "PASS")
        status, detail = us101.judge_undefended(self.rec(15.0, hit_s=None))
        self.assertEqual(status, "FAIL")
        self.assertIn("no health drop", detail)
        self.assertEqual(us101.judge_block(self.rec(7.5, 10.5, 2.25))[0], "PASS")
        status, detail = us101.judge_block(self.rec(15.0, posture=4.5))
        self.assertEqual(status, "FAIL")
        self.assertIn("full 15 landed through the block (N32)", detail)
        status, detail = us101.judge_block(self.rec(7.5, 0.0))
        self.assertIn("read as a perfect block", detail)

    def test_judge_perfect_reports_the_timing(self):
        timeline = [(f"{LT}=1 sent", 0.300), (f"{LT}=1 answered", 0.305)]
        status, detail = us101.judge_perfect(self.rec(7.5, 0.0, timeline=timeline))
        self.assertEqual(status, "PASS", detail)
        self.assertIn("LT went up 0.215s before the hit was seen", detail)
        self.assertIn(f"{LT}=1 sent +0.300s", detail)
        self.assertIn("hit seen +0.520s", detail)
        status, detail = us101.judge_perfect(self.rec(7.5, 10.5, timeline=timeline))
        self.assertEqual(status, "FAIL")
        self.assertIn("a normal block, not a perfect one", detail)

    def test_judge_dodge_needs_the_tag_and_the_reach(self):
        inside = [{"t": 0.5, "dist": 80.0, "tags": [us101.TAG_DODGING]}]
        self.assertEqual(us101.judge_dodge(self.rec(0.0, 15.0, samples=inside))[0], "PASS")
        out = [{"t": 0.5, "dist": 210.0, "tags": [us101.TAG_DODGING]}]
        status, detail = us101.judge_dodge(self.rec(0.0, 15.0, samples=out))
        self.assertEqual(status, "FAIL")
        self.assertIn("inconclusive: out of the 150 cm reach", detail)
        status, detail = us101.judge_dodge(self.rec(0.0, samples=[{"t": 0.5, "dist": 80.0}]))
        self.assertIn("no 'Mordecai.State.Dodging'", detail)
        status, detail = us101.judge_dodge(self.rec(15.0, samples=inside))
        self.assertIn("the full 15 landed through the dodge, N32", detail)

    def test_judge_jump(self):
        air = [{"t": 0.5, "dist": 100.0, "mode": "Falling", "tags": [us101.TAG_AIRBORNE]}]
        self.assertEqual(us101.judge_jump(self.rec(0.0, samples=air))[0], "PASS")
        status, detail = us101.judge_jump(self.rec(0.0, samples=air,
                                                   after_tags=[us101.TAG_AIRBORNE]))
        self.assertEqual(status, "FAIL")
        self.assertIn("stuck after the landing", detail)
        no_tag = [{"t": 0.5, "dist": 100.0, "mode": "Falling", "tags": []}]
        status, detail = us101.judge_jump(self.rec(15.0, samples=no_tag))
        self.assertIn("not in player.tags at any of 1 Falling read(s)", detail)
        status, detail = us101.judge_jump(self.rec(0.0, samples=[{"t": 0.5, "dist": 100.0,
                                                                  "mode": "Walking", "tags": []}]))
        self.assertIn("never read 'Falling'", detail)

    def test_judge_parry(self):
        s = [{"t": 0.5, "dist": 100.0, "tags": [us101.TAG_PARRYING], "attacker_tags": []}]
        ok = self.rec(0.0, 20.0, samples=s, after_tags=[us101.TAG_RIPOSTE],
                      attacker_after_tags=[us101.TAG_PARRIED])
        self.assertEqual(us101.judge_parry(ok)[0], "PASS")
        status, detail = us101.judge_parry(self.rec(15.0, 20.0, samples=s))
        self.assertEqual(status, "FAIL")
        self.assertIn("no 'Mordecai.State.Riposte' on the player", detail)
        self.assertIn("no 'Mordecai.State.Parried' on the TestAttacker", detail)


# ---------------------------------------------------------------------------
# Whole runs against the fake
# ---------------------------------------------------------------------------
class TestUs101AgainstTheFake(Us101Case):
    def test_everything_passes_when_the_defenses_work(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(self.statuses(rep, "FAIL"), [], self.tail())
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(code, 0)
        checks = self.by_name(rep)
        for name in PLAN_ORDER:
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertEqual(checks["map"]["status"], "PASS", checks["map"]["detail"])
        self.assertEqual([s["label"] for s in rep["screenshots"]], SHOTS)
        # Each defense met exactly one swing, in order; every other swing was undefended
        kinds = [k for _, k, *_ in world.defense_log if k != "none"]
        self.assertEqual(kinds, ["blocked", "perfect", "evaded", "evaded", "parried"])
        undefended = sum(1 for _, k, *_ in world.defense_log if k == "none")
        self.assertGreaterEqual(undefended, 2)   # the walk's swing and the control
        self.assertAlmostEqual(world.health, 100.0 - 15.0 * undefended - 7.5 * 2)
        self.assertAlmostEqual(world.stamina, 100.0 - 10.5 - 15.0 - 20.0)
        self.assertEqual(world.deaths, 0)
        self.assertEqual(world.jumps, 1)
        self.assertEqual(world.parries, 1)
        # The dodge went +X into the attacker and stopped at its capsule (inside the reach)
        self.assertEqual(len(world.dodges), 1)
        x0, x1 = world.dodges[0]
        self.assertGreaterEqual(x1, x0)
        self.assertAlmostEqual(x1, us101.ATTACKER_SPAWN[0] - DefenseWorld.CONTACT_CM, delta=1.0)
        # The details carry the numbers and the timing
        self.assertIn("health -15", checks["undefended_hit"]["detail"])
        self.assertIn("posture -4.5", checks["undefended_hit"]["detail"])
        self.assertIn("health -7.5", checks["block_mitigates"]["detail"])
        self.assertIn("stamina -10.5", checks["block_mitigates"]["detail"])
        self.assertRegex(checks["perfect_block"]["detail"],
                         r"LT went up 0\.\d+s before the hit was seen \(window 0\.25s\)")
        self.assertIn("stamina -0", checks["perfect_block"]["detail"])
        self.assertIn("'Mordecai.State.Dodging' seen: True", checks["dodge_evades"]["detail"])
        self.assertIn("with 'Mordecai.State.Airborne'", checks["jump_evades"]["detail"])
        self.assertIn("'Mordecai.State.Airborne' gone", checks["jump_evades"]["detail"])
        self.assertIn("'Mordecai.State.Riposte' on the player: True",
                      checks["parry_negates"]["detail"])
        self.assertIn("'Mordecai.State.ParryWhiff' seen: False", checks["parry_negates"]["detail"])
        # The measured hit times drove the later delays (about 0.5 s after the edge here)
        self.assertEqual(len(rep["hit_offsets_s"]), 3)
        self.assertTrue(all(0.45 <= t <= 0.65 for t in rep["hit_offsets_s"]), rep["hit_offsets_s"])
        self.assertEqual([s["check"] for s in rep["swings"]], PLAN_ORDER[1:])
        md = self.md()
        self.assertIn("## Swings", md)
        self.assertIn("| perfect_block |", md)
        # Attached run: diagnostic only, nothing accepted
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        self.assertFalse(any(a["accepted"] for a in rep["acs"]))
        acs = self.acs(rep)
        for n in range(10, 16):
            self.assertEqual(acs[("US-101", f"AC-101.{n}")]["status"], "PASS", n)
        for ac in (("US-006", "AC-006.3"), ("US-005", "AC-005.3"), ("US-003", "AC-003.7")):
            self.assertEqual(acs[ac]["status"], "PASS", ac)
        self.assert_real_input_only(server)

    def assert_real_input_only(self, server):
        sent = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual(set(sent), INPUT_AND_READ_CMDS)
        axes = [r for r in server.received if r.get("cmd") == "axis"]
        self.assertTrue(axes)
        self.assertLessEqual({k for r in axes for k in r["values"]}, {LT, RT})
        lt = [r["values"][LT] for r in axes if LT in r["values"] and "hold_s" not in r]
        self.assertEqual(lt[-1], 0.0, "LT is released at the end")
        # RT only as a timed press (the parry), never held across reads
        self.assertTrue(all("hold_s" in r for r in axes if RT in r["values"]))
        keys = [(r.get("action", "tap"), r.get("key") or tuple(r.get("keys") or ()))
                for r in server.received if r.get("cmd") == "key"]
        self.assertIn(("tap", us101.DODGE_KEY), keys)
        self.assertIn(("tap", us101.JUMP_KEY), keys)
        self.assertEqual(keys.count(("press", "W")), keys.count(("release", "W")))
        self.assertEqual(keys.count(("press", "W")), 1)
        # The edge waits poll every frame (poll_s 0) on the attacker's actors[] entry
        waits = [r for r in server.received if r.get("cmd") == "wait_until"]
        self.assertTrue(any("ai_state==\"Attack\"" in r["condition"] and r.get("poll_s") == 0.0
                            for r in waits))

    def test_every_defense_fails_with_the_n32_bug(self):
        # Issue #53 as filed: every swing does the full 15, whatever the player does; no Airborne
        world, server = self.serve(defenses=False, airborne_tag=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1, self.tail())
        checks = self.by_name(rep)
        self.assertEqual(checks["walk_to_fight_spot"]["status"], "PASS")
        self.assertEqual(checks["undefended_hit"]["status"], "PASS",
                         checks["undefended_hit"]["detail"])
        for name in DEFENSE_CHECKS:
            self.assertEqual(checks[name]["status"], "FAIL", f"{name}: {checks[name]['detail']}")
        self.assertEqual(sorted(rep["new_failures"]), sorted(DEFENSE_CHECKS))
        self.assertEqual(rep["known_failures"], [])
        self.assertIn("the full 15 landed through the block (N32)",
                      checks["block_mitigates"]["detail"])
        self.assertIn("the full 15 landed", checks["perfect_block"]["detail"])
        self.assertIn("the full 15 landed through the dodge, N32", checks["dodge_evades"]["detail"])
        self.assertIn("'Mordecai.State.Airborne' not in player.tags", checks["jump_evades"]["detail"])
        self.assertIn("no 'Mordecai.State.Riposte' on the player",
                      checks["parry_negates"]["detail"])
        # Every swing landed in full: the seventh one (the parry's) killed the player
        self.assertTrue(all(k == "none" for _, k, *_ in world.defense_log))
        self.assertEqual(world.deaths, 1)
        acs = self.acs(rep)
        self.assertEqual(acs[("US-101", "AC-101.10")]["status"], "PASS")
        for n in range(11, 16):
            self.assertEqual(acs[("US-101", f"AC-101.{n}")]["status"], "FAIL", n)

    def test_a_timing_miss_and_a_dodge_out_of_reach_are_not_passes(self):
        # A perfect-block window too narrow for the press, and a dodge that ignores W (backward,
        # 110 cm, out of the 150 cm reach: the swing misses, health is unchanged, yet it proves
        # nothing).  The jump then walks back to the fight spot first.
        world, server = self.serve(perfect_window_s=0.05, dodge_ignores_input=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1, self.tail())
        checks = self.by_name(rep)
        self.assertEqual(sorted(rep["new_failures"]), ["dodge_evades", "perfect_block"],
                         self.tail())
        perfect = checks["perfect_block"]["detail"]
        self.assertIn("a normal block, not a perfect one", perfect)
        self.assertRegex(perfect, r"LT went up 0\.\d+s before the hit was seen")
        self.assertIn(f"{LT}=1 sent +0.", perfect)
        dodge = checks["dodge_evades"]["detail"]
        self.assertIn("inconclusive: out of the 150 cm reach at the hit", dodge)
        self.assertIn("health -0", dodge)
        self.assertEqual(len(world.dodges), 1)
        self.assertLess(world.dodges[0][1], world.dodges[0][0])     # backward
        for name in ("undefended_hit", "block_mitigates", "jump_evades", "parry_negates"):
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertIn("walked back from", checks["jump_evades"]["detail"])


if __name__ == "__main__":
    unittest.main()
