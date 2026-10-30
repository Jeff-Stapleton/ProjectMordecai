"""Tests for Scripts/playtests/us054_combat.py against tests/fake_bridge.py.

CombatWorld (below) models CombatTestMap (fb.MAP_PRESETS[fb.COMBAT_TEST_MAP]: the TestDummy at
(600, 600), the TestAttacker at (2100, 0), the PlayerStart at (0, 0)) and adds what the shared
FakeWorld lacks: an LMB click hurts an enemy within the default-equipped Longsword's first light
attack (DA_AP_Longsword_L1: ArcSector 200 cm, 120 deg around +X, centre to centre); the
attacker follows AMordecaiEnemyAIController -- Idle until the player pawn comes within 900 cm,
then Approach, swinging (Attack, then Recover) at it within 150 cm, by default with the known
cadence (15 damage 0.5 s into each swing, one swing per ~2.6 s); the dead player stays its
target (it keeps swinging, without damage), and beyond its 1800 cm LeashRange it goes Leash,
then Idle; the death at health 0 and the respawn at (0, 0) 3 s later with ResetArena (both
targets back at their spawns, full health, Idle); Restoration on One, Burning procs, the death
screen, the attack and dodge tags.  These prove the playtest's logic and reporting only; they
say nothing about the real game.  Nothing here launches the real game.
Run: py Scripts/run_python_tests.py -k us054_combat
"""

import importlib.util
import io
import json
import math
import os
import sys
import tempfile
import time
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402

PLAYTESTS = os.path.join(SCRIPTS, "playtests")
TOKEN = "us054c-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "click", "screenshot", "wait_until", "wait", "quit"}


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(PLAYTESTS, name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us054c = load("us054_combat")

TAG_ATTACKING = "Mordecai.State.Attacking"
TAG_DODGING = "Mordecai.State.Dodging"
TAG_BURNING = "Mordecai.Status.Burning"
TAG_RESTORATION = "Mordecai.Status.Restoration"
# A fast attacker for the tests that do not time the cadence: 25 damage per 0.8 s swing.
FAST = dict(hit_damage=25.0, windup_s=0.2, attack_s=0.4, cooldown_s=0.4)
# A fast attacker whose cycle puts the respawn mid-swing.  The respawn comes 3 s after the killing
# hit, which lands windup_s into a swing: (0.2 + 3.0) mod (0.6 + 0.6) = 0.8 s into the 1.2 s
# cycle, i.e. 0.2 s into a Recover (the real cadence: 0.9 s into a 1.1 s Attack).  Without a
# reset the attacker is still Recover at the first read of the respawned player, then leashes to
# Idle by itself ~0.4 s later (the player is beyond LeashRange).  FAST's 0.8 s cycle ends exactly
# at the respawn, so a reset-less run would race that leash.
MID_SWING = dict(hit_damage=25.0, windup_s=0.2, attack_s=0.6, cooldown_s=0.6)
AI_NOT_RESET = r"TestAttacker: AI not reset to Idle at the respawn \(ai_state (Attack|Recover)\)"
PLAN_ORDER = ["combat_targets_present", "walk_to_dummy", "melee_lmb", "dodge_lalt",
              "walk_to_attacker", "attacker_damages_player", "restoration_heals", "burning_on_hit",
              "player_dies", "death_screen", "dead_no_move", "respawn", "death_screen_removed",
              "arena_reset", "respawn_moves", "kill_counter_text_matches"]
# The checks that moved from us054_arena.py keep their proves (Specs: the us054 split); US-086
# (issue #7) adds its death screen ACs to death_screen.
MOVED_PROVES = {
    "melee_lmb": ("AC-054.6", "AC-054.5"),
    "dodge_lalt": ("AC-054.6", "AC-054.5"),
    "restoration_heals": ("US-057:AC-057.4", "US-057:AC-057.5", "US-057:AC-057.12"),
    "burning_on_hit": ("US-057:AC-057.11", "US-057:AC-057.8"),
    "player_dies": ("US-053:AC-053.1",),
    "death_screen": ("US-062:AC-062.1", "US-062:AC-062.2", "US-086:AC-086.1", "US-086:AC-086.4"),
    "dead_no_move": ("US-053:AC-053.2",),
    "respawn": ("US-053:AC-053.4", "US-053:AC-053.5"),
    "arena_reset": ("US-053:AC-053.8", "US-053:AC-053.9"),
    "respawn_moves": ("US-053:AC-053.6",),
    "kill_counter_text_matches": ("US-063:AC-063.1",),
}


class CombatWorld(fb.FakeWorld):
    """CombatTestMap with a fighting TestAttacker.  Defaults model the game as it should be, with
    the known attacker cadence and BasicSlash (no Burning); the options change each behaviour.

    click_damage / click_damage_delay_s / melee_reach / melee_arc_deg: an LMB click damages every
    enemy within melee_reach and melee_arc_deg around the player's +X facing (centre to centre,
    as UMordecaiHitDetectionSubsystem::IsPointInArcSector), click_damage_delay_s later.  The
    defaults are the light attack the player really swings: InitializeWeaponLoadout equips the
    first starting weapon, the Longsword, whose first light attack (DA_AP_Longsword_L1) is an
    ArcSector of 200 cm / 120 deg (DA_PlayerAttack_LightSlash_1, 180 cm / 70 deg, is only the
    no-weapon fallback).  aggro_range, attack_reach, leash_range, hit_damage, windup_s, attack_s,
    cooldown_s: the attacker's AI, as AMordecaiEnemyAIController::UpdateStateMachine.  Its target
    is the player pawn, dead or alive (FindPlayerTarget): Idle -> Approach within aggro_range;
    Approach -> Leash beyond leash_range, -> Attack within attack_reach; the hit lands windup_s
    into the swing (no damage to a dead player), the swing lasts attack_s, then Recover for
    cooldown_s, after which Leash beyond leash_range, Attack within attack_reach, else Approach;
    Leash -> Idle on the next tick (it never moves, so it is always within 100 cm of its spawn).
    attacker_hits=False: it swings but never hits.  heal_per_s / restoration_s: Restoration on One
    (3 SP).  burn_chance: 1.0 every hit burns, 0 never (BasicSlash).  arena_reset: ResetArena on
    the respawn; reset_ai=False: that reset restores location, health and is_dead but leaves the
    AI state.  ai_reported=False: every enemy's ai_state reads 'Idle' (a broken report).
    moves_after_respawn=False: movement input stays ignored after the respawn (a broken
    HandlePlayerRespawn).  death_screen=False: no death screen (issue #7 before its fix);
    death_screen_stays=True: it is never removed after a respawn.  ai_log: (label, real time,
    ai_state) of every AI state change.
    """

    def __init__(self, *, click_damage=12.0, click_damage_delay_s=0.2, melee_reach=200.0,
                 melee_arc_deg=120.0, aggro_range=900.0, attack_reach=150.0, leash_range=1800.0,
                 hit_damage=15.0, windup_s=0.5, attack_s=1.1, cooldown_s=1.5, attacker_hits=True,
                 restoration=True, heal_per_s=8.0, restoration_s=5.0, burn_chance=0.0,
                 burn_indicator=True, attacks=True, attack_tag=True, death_screen=True,
                 death_screen_stays=False, dead_moves=False, arena_reset=True, reset_ai=True,
                 ai_reported=True, moves_after_respawn=True, kills_text=0, **kwargs):
        for name, value in fb.MAP_PRESETS[fb.COMBAT_TEST_MAP].items():
            kwargs.setdefault(name, value)
        kwargs.setdefault("respawn_s", us054c.RESPAWN_DELAY_S)  # the game's 3 s
        super().__init__(**kwargs)
        self.click_damage, self.click_damage_delay_s = float(click_damage), click_damage_delay_s
        self.melee_reach, self.melee_arc_deg = float(melee_reach), float(melee_arc_deg)
        self.aggro_range, self.attack_reach = float(aggro_range), float(attack_reach)
        self.leash_range = float(leash_range)
        self.hit_damage = float(hit_damage)
        self.windup_s, self.attack_s, self.cooldown_s = windup_s, attack_s, cooldown_s
        self.attacker_hits = attacker_hits
        self.restoration = restoration
        self.heal_per_s, self.restoration_s = heal_per_s, restoration_s
        self.burn_chance, self.burn_indicator = burn_chance, burn_indicator
        self.attacks, self.attack_tag = attacks, attack_tag
        self.death_screen, self.dead_moves = death_screen, dead_moves
        self.death_screen_stays = death_screen_stays
        self.arena_reset, self.reset_ai = arena_reset, reset_ai
        self.ai_reported, self.kills_text = ai_reported, kills_text
        self.moves_after_respawn = moves_after_respawn
        self.timed_tags = {}
        self.burn_rolls = 0
        self.hits = []             # (real time, health after) of every attacker hit
        self.deaths = 0
        self.respawns = []         # real time of every respawn
        self.ai_log = []
        self.spawns = {e["label"]: list(e["location"]) for e in self.enemies}
        for e in self.enemies:
            self._reset_enemy(e)

    def _reset_enemy(self, e, ai=True):
        e["location"] = list(self.spawns[e["label"]])
        e["health"] = float(e.get("max_health", 200.0))
        e["is_dead"] = False
        if ai:
            self._set_ai(e, "Idle")
            e["since"], e["swung"] = 0.0, False

    def _set_ai(self, e, state, now=None):
        if e.get("ai_state") != state:
            self.ai_log.append((e["label"], round(self.real_time() if now is None else now, 3),
                                state))
        e["ai_state"] = state

    @staticmethod
    def is_attacker(e):
        return "testattacker" in (str(e.get("class")) + str(e.get("label"))).lower()

    def enemy(self, fragment):
        f = fragment.lower()
        return next((e for e in self.enemies
                     if f in (str(e.get("class")) + str(e.get("label"))).lower()), None)

    # --- simulation -------------------------------------------------------------------------
    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            now = self.real_time()
            self.timed_tags = {t: until for t, until in self.timed_tags.items() if until > now}
            if not self.ready or self.menu_open:
                return
            if not self.is_dead and TAG_RESTORATION in self.timed_tags:
                self.health = min(self.max_health, self.health + self.heal_per_s * dt)
            for e in self.enemies:
                if self.is_attacker(e):
                    self._attacker_tick(e, now)

    def _attacker_tick(self, e, now):
        # The target is the player pawn, dead or alive (FindPlayerTarget): a dead player keeps
        # being swung at, without damage, until the respawn
        d = math.hypot(self.pos[0] - e["location"][0], self.pos[1] - e["location"][1])
        state = e["ai_state"]
        if state == "Idle":
            if d <= self.aggro_range:
                self._set_ai(e, "Approach", now)
        elif state == "Approach":
            if d > self.leash_range:
                self._set_ai(e, "Leash", now)
            elif d <= self.attack_reach:
                self._swing(e, now)
        elif state == "Attack":
            if not e["swung"] and now - e["since"] >= self.windup_s:
                e["swung"] = True
                if not self.is_dead and self.attacker_hits and d <= self.attack_reach:
                    self._hit_player(now)
            if now - e["since"] >= self.attack_s:
                self._set_ai(e, "Recover", now)
                e["since"] = now
        elif state == "Recover":
            if now - e["since"] >= self.cooldown_s:
                if d > self.leash_range:
                    self._set_ai(e, "Leash", now)
                elif d <= self.attack_reach:
                    self._swing(e, now)
                else:
                    self._set_ai(e, "Approach", now)
        elif state == "Leash":
            # Back to Idle within 100 cm of its spawn: always, it never moves
            spawn = self.spawns[e["label"]]
            if math.hypot(e["location"][0] - spawn[0], e["location"][1] - spawn[1]) < 100.0:
                self._set_ai(e, "Idle", now)

    def _swing(self, e, now):
        self._set_ai(e, "Attack", now)
        e["since"], e["swung"] = now, False

    def _hit_player(self, now):
        self.health -= self.hit_damage
        self.hits.append((round(now, 2), self.health))
        self.burn_rolls += 1
        if self.burn_chance >= 1.0 or (self.burn_chance > 0 and self.burn_rolls % 3 == 0):
            self.timed_tags[TAG_BURNING] = now + 3.0
        if self.health <= 0:
            self.deaths += 1
            self.kill_player()

    def _move(self, dt):
        if not self.moves_after_respawn and self.respawns and not self.is_dead:
            self.vel = [0.0, 0.0]
            return
        if self.dead_moves and self.is_dead:
            dead, self.dead_since = self.dead_since, None
            try:
                super()._move(dt)
            finally:
                self.dead_since = dead
            return
        super()._move(dt)

    def respawn(self):
        # ExecuteRespawn: the teleport, the player's restore and ResetArena in one call
        with self.lock:
            super().respawn()
            self.respawns.append(round(self.real_time(), 3))
            self.timed_tags = {}
            if self.arena_reset:
                for e in self.enemies:
                    self._reset_enemy(e, ai=self.reset_ai)

    def _on_key_pressed(self, key):
        if self.ready and not self.menu_open and not self.is_dead:
            now = self.real_time()
            if key == "One":
                if self.restoration and self.spell_points >= 3:
                    self.spell_points -= 3
                    self.timed_tags[TAG_RESTORATION] = now + self.restoration_s
                return
            if key == "LeftAlt":
                if self.attacks:
                    self.timed_tags[TAG_DODGING] = now + 0.4
                return
        super()._on_key_pressed(key)

    def mouse_down(self, conn_id, x, y, button):
        hit = super().mouse_down(conn_id, x, y, button)
        with self.lock:
            if (button == "Left" and self.attacks and self.game_input_allowed
                    and not self.is_dead):
                if self.attack_tag:
                    self.timed_tags[TAG_ATTACKING] = self.real_time() + 0.5
                if self.click_damage:
                    px, py = self.pos
                    half = math.radians(self.melee_arc_deg / 2.0)
                    struck = []
                    for e in self.enemies:
                        dx, dy = e["location"][0] - px, e["location"][1] - py
                        if math.hypot(dx, dy) <= self.melee_reach \
                                and abs(math.atan2(dy, dx)) <= half:
                            struck.append(e)
                    if struck:
                        self.after(self.click_damage_delay_s, lambda w: w._strike(struck))
        return hit

    def _strike(self, struck):
        for e in struck:
            e["health"] = max(0.0, e["health"] - self.click_damage)

    # --- state --------------------------------------------------------------------------------
    def _s_player(self):
        d = super()._s_player()
        if d.get("valid"):
            d["tags"] = list(d["tags"]) + sorted(self.timed_tags)
        return d

    def _s_widgets(self):
        out = super()._s_widgets()
        out.append({"class": "MordecaiKillCounterWidget", "name": "Kills",
                    "rect": [1140, 20, 120, 30], "texts": [f"Kills: {self.kills_text}"],
                    "buttons": []})
        if self.burn_indicator and TAG_BURNING in self.timed_tags:
            out.append({"class": "MordecaiStatusEffectBarWidget", "name": "Status",
                        "rect": [40, 104, 260, 20], "texts": ["Burning"], "buttons": []})
        stays = self.death_screen_stays and self.respawns
        if self.death_screen and (self.is_dead or stays):
            left = (max(0, math.ceil(self.respawn_s - (self.real_time() - self.dead_since)))
                    if self.is_dead else 0)
            # UMordecaiDeathScreenWidget: the countdown is a bare ceiling digit, empty at 0 (the
            # bridge drops empty texts)
            texts = ["You Died"] + ([str(left)] if left > 0 else []) + [f"Kills: {self.kills}"]
            out.append({"class": "MordecaiDeathScreenWidget", "name": "Death",
                        "rect": [0, 0, 1280, 720], "texts": texts, "buttons": []})
        return out

    def _actors(self, radius, include_all):
        entries = super()._actors(radius, include_all)
        if not self.ai_reported:
            for a in entries:
                if a["kind"] == "enemy":
                    a["ai_state"] = "Idle"
        return entries


def game_today(**kw):
    """The packaged game as expected on CombatTestMap: a BasicSlash attacker (no Burning),
    everything else working, the death screen included (issue #7); the fast attacker."""
    opts = dict(FAST)
    opts.update(kw)
    return opts


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class Us054CombatCase(unittest.TestCase):
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

    def shorten(self, **values):
        """Patch module-level timeouts for the paths that are expected to time out."""
        for name, value in values.items():
            p = mock.patch.object(us054c, name, value)
            p.start()
            self.addCleanup(p.stop)

    def serve(self, **world_kwargs):
        world = CombatWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        code = us054c.main(["--attach-port", str(server.port), "--token", TOKEN,
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


class TestUs054CombatPlan(unittest.TestCase):
    def test_plan_is_the_spec_plan(self):
        self.assertEqual(kit.plan_problems(us054c.Us054CombatRun), [])
        self.assertIs(us054c.Run, us054c.Us054CombatRun)
        self.assertEqual([s.check for s in us054c.PLAN], PLAN_ORDER)
        plan = {s.check: s for s in us054c.PLAN}
        for name, proves in MOVED_PROVES.items():
            self.assertEqual(plan[name].proves, proves, name)
        for name in ("combat_targets_present", "walk_to_dummy", "walk_to_attacker",
                     "attacker_damages_player"):
            self.assertEqual(plan[name].proves, (), name)
        self.assertEqual(plan["death_screen_removed"].proves, ("US-086:AC-086.1",))
        # N2 is fixed (issue #7): no check is expected to fail
        self.assertEqual({s.check for s in us054c.PLAN if s.known_issue}, set())
        self.assertEqual(set(us054c.Us054CombatRun.COVERS),
                         {"US-053", "US-057", "US-062", "US-063", "US-086"})

    def test_declarations(self):
        run = us054c.Us054CombatRun
        self.assertEqual(run.STORY, "US-054")
        self.assertEqual(run.RUN_SUFFIX, "us054combat")
        self.assertEqual(run.launch_map(), kit.COMBAT_TEST_MAP)
        self.assertEqual(run.expected_experience(), kit.DEFAULT_EXPERIENCE)
        # The DevTestMap half is the other US-054 script: a distinct run suffix.
        arena = load("us054_arena")
        self.assertEqual(arena.Us054Run.STORY, "US-054")
        self.assertNotEqual(arena.Us054Run.RUN_SUFFIX, run.RUN_SUFFIX)
        # The scripted spots agree with the fake map (Scripts/us100_02_create_test_maps.py).
        self.assertEqual(us054c.DUMMY_MELEE_SPOT, fb.COMBAT_DUMMY_MELEE_SPOT)
        self.assertEqual(us054c.ATTACKER_FIGHT_SPOT, fb.COMBAT_ATTACKER_FIGHT_SPOT)
        self.assertEqual(list(us054c.DUMMY_SPAWN), fb.TEST_DUMMY_SPEC["location"][:2])
        self.assertEqual(list(us054c.ATTACKER_SPAWN), fb.TEST_ATTACKER_SPEC["location"][:2])
        # Only the fight spot is inside the attacker's aggro range.
        for spot in (us054c.SPAWN, us054c.DUMMY_MELEE_SPOT, us054c.ATTACKER_VIA):
            self.assertGreater(math.dist(spot, us054c.ATTACKER_SPAWN), us054c.AGGRO_RANGE_CM)
        # The fight spot is 100 cm -X of the attacker: inside its 150 cm swing even when walk_to
        # stops SPOT_TOL off; the melee spot is inside the player's light swing (200 cm) likewise.
        self.assertEqual(us054c.ATTACKER_FIGHT_SPOT, (2000.0, 0.0))
        self.assertEqual(us054c.ATTACKER_SPAWN[0] - us054c.ATTACKER_FIGHT_SPOT[0], 100.0)
        self.assertLess(math.dist(us054c.ATTACKER_FIGHT_SPOT, us054c.ATTACKER_SPAWN)
                        + us054c.SPOT_TOL, 150.0)
        self.assertLess(math.dist(us054c.DUMMY_MELEE_SPOT, us054c.DUMMY_SPAWN) + us054c.SPOT_TOL,
                        CombatWorld().melee_reach)

    def test_plan_prints(self):
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        try:
            self.assertEqual(us054c.main(["--plan"]), 0)
        finally:
            kit.set_log_stream(previous)
        text = out.getvalue()
        self.assertIn("| 1 | `combat_targets_present` |", text)
        self.assertIn("| 13 | `death_screen_removed` |", text)
        self.assertIn("| 16 | `kill_counter_text_matches` |", text)
        self.assertNotIn("Known issue N2", text)


class TestUs054CombatHelpers(unittest.TestCase):
    @staticmethod
    def enemy(label, cls, x, y, hp=200.0, ai="Idle", dead=False):
        return {"kind": "enemy", "label": label, "name": label, "class": cls,
                "location": [x, y, 100.0], "health": {"current": hp, "max": 200.0},
                "is_dead": dead, "ai_state": ai}

    def test_find_target_by_label_then_class(self):
        st = {"actors": [self.enemy("Enemy_0", "BP_MordecaiEnemy_TestAttacker_C", 2100, 0),
                         self.enemy("TestDummy", "BP_Other_C", 600, 600),
                         self.enemy("Enemy_2", "BP_MordecaiEnemy_TestDummy_C", 0, 0)]}
        self.assertEqual(us054c.find_target(st, "TestDummy")["label"], "TestDummy")
        self.assertEqual(us054c.find_target(st, "TestAttacker")["label"], "Enemy_0")
        self.assertIsNone(us054c.find_target(st, "Frontliner"))
        self.assertIsNone(us054c.find_target({"actors": {"valid": False}}, "TestDummy"))

    def test_reset_problems(self):
        ok = {"actors": [self.enemy("TestDummy", "D", 610, 590),
                         self.enemy("TestAttacker", "A", 2100, 0)]}
        self.assertEqual(us054c.reset_problems(ok), [])
        bad = {"actors": [self.enemy("TestDummy", "D", 600, 600, hp=188.0),
                          self.enemy("TestAttacker", "A", 1900, 0, ai="Approach")]}
        problems = us054c.reset_problems(bad)
        self.assertTrue(any("TestDummy health 188.0/200.0" in p for p in problems), problems)
        self.assertTrue(any("200 cm from its spawn" in p for p in problems), problems)
        self.assertTrue(any("ai_state Approach" in p for p in problems), problems)
        self.assertTrue(any("1 enemies" in p for p in
                            us054c.reset_problems({"actors": ok["actors"][:1]})))

    def test_reset_evidence(self):
        now = {"actors": [self.enemy("TestDummy", "D", 600, 600),
                          self.enemy("TestAttacker", "A", 2100, 0)]}
        both = {"TestDummy": {"min_health": 188.0, "max": 200.0, "states": ["Idle"]},
                "TestAttacker": {"min_health": 200.0, "max": 200.0,
                                 "states": ["Idle", "Approach", "Attack", "Recover"]}}
        ev = us054c.reset_evidence(both, now)
        self.assertEqual(len(ev), 2, ev)
        self.assertIn("188/200 before the death", ev[0])
        self.assertIn("Idle -> Approach -> Attack -> Recover", ev[1])
        none = {"TestDummy": {"min_health": 200.0, "max": 200.0, "states": ["Idle"]},
                "TestAttacker": {"min_health": 200.0, "max": 200.0, "states": ["Idle"]}}
        self.assertEqual(us054c.reset_evidence(none, now), [])
        self.assertEqual(us054c.reset_evidence({}, now), [])

    def test_death_verdict(self):
        dead = {"valid": True, "is_dead": True, "tags": ["Mordecai.State.Dead"],
                "attributes": {"health": 0.0}}
        alive = {"valid": True, "is_dead": False, "tags": [], "attributes": {"health": 100.0}}
        seen = {"step": "burning_on_hit", "tags": ["Mordecai.State.Dead"], "health": 0.0}
        status, detail = us054c.death_verdict(dead, seen)
        self.assertEqual(status, "PASS")
        self.assertIn("first seen dead in burning_on_hit", detail)
        # Seen dead by the previous step, respawned since: the death is not lost.
        status, detail = us054c.death_verdict(alive, seen)
        self.assertEqual(status, "PASS", detail)
        self.assertIn("respawned since", detail)
        self.assertEqual(us054c.death_verdict(alive, dict(seen, tags=[]))[0], "FAIL")
        self.assertEqual(us054c.death_verdict(dict(dead, tags=[]), None)[0], "FAIL")
        self.assertEqual(us054c.death_verdict(alive, None),
                         ("FAIL", "player.is_dead is not true and no step saw the player dead"))


class TestCombatWorldAi(unittest.TestCase):
    """The fake's TestAttacker follows AMordecaiEnemyAIController: a fake that stopped targeting
    the dead player and never leashed hid arena_reset's false PASS.  A manual clock: no server."""

    @staticmethod
    def world(**kw):
        world = CombatWorld(**dict(MID_SWING, respawn_s=None, **kw))
        clock = [0.0]
        world.real_time = lambda: clock[0]

        def run_for(seconds, dt=1.0 / 60):
            end = clock[0] + seconds
            while clock[0] < end - 1e-9:
                clock[0] += dt
                world.tick(dt)

        return world, run_for

    @staticmethod
    def states(world, since=0.0):
        return [s for label, t, s in world.ai_log if label == "TestAttacker" and t >= since]

    def test_it_swings_at_the_dead_player_then_leashes_to_idle_after_the_respawn(self):
        world, run_for = self.world(arena_reset=False)
        attacker = world.enemy("TestAttacker")
        run_for(0.5)
        self.assertEqual(attacker["ai_state"], "Idle")       # the PlayerStart is 2100 cm away
        world.set_position(*us054c.ATTACKER_FIGHT_SPOT)
        run_for(0.5)
        self.assertEqual(self.states(world), ["Approach", "Attack"])
        self.assertEqual(len(world.hits), 1)                 # 0.2 s into the swing
        world.kill_player()
        t_dead, hits = world.real_time(), len(world.hits)
        run_for(3.0)
        after_death = self.states(world, t_dead)
        self.assertIn("Attack", after_death)                 # the dead pawn stays its target
        self.assertIn("Recover", after_death)
        self.assertNotIn("Leash", after_death)
        self.assertEqual(len(world.hits), hits, "no damage to a dead player")
        world.respawn()                                      # no ResetArena
        t_respawn = world.real_time()
        self.assertIn(attacker["ai_state"], ("Attack", "Recover"))
        run_for(world.attack_s + world.cooldown_s + 0.1)
        self.assertEqual(self.states(world, t_respawn)[-2:], ["Leash", "Idle"])
        self.assertEqual(attacker["location"][:2], [2100.0, 0.0])   # it never moved

    def test_the_reset_makes_it_idle_at_once_and_it_stays_idle(self):
        world, run_for = self.world()
        attacker = world.enemy("TestAttacker")
        world.set_position(*us054c.ATTACKER_FIGHT_SPOT)
        run_for(1.0)
        world.kill_player()
        run_for(1.0)
        world.respawn()                                      # ResetArena in the same call
        self.assertEqual(attacker["ai_state"], "Idle")
        run_for(3.0)
        self.assertEqual(attacker["ai_state"], "Idle")       # 2100 cm: out of aggro range


class TestUs054CombatAgainstTheFake(Us054CombatCase):
    def test_everything_passes_when_the_combat_loop_works(self):
        world, server = self.serve(**dict(FAST, burn_chance=1.0))
        code, rep = self.play(server)
        self.assertEqual(self.statuses(rep, "FAIL"), [], self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(code, 0)
        checks = self.by_name(rep)
        for name in PLAN_ORDER:
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertEqual(checks["map"]["status"], "PASS", checks["map"]["detail"])
        self.assertEqual(rep["fixed_candidates"], [])
        self.assertRegex(checks["death_screen"]["detail"], r"'You Died', '[123]', 'Kills: 0'")
        self.assertEqual([s["label"] for s in rep["screenshots"]],
                         ["start", "near_dummy", "hit", "dead", "respawned"])
        # The light attack is judged on the dummy, which was really hit.
        self.assertIn("TestDummy health 200.0 before the click, (188.0, 200.0) after",
                      checks["melee_lmb"]["detail"])
        self.assertIn("reset observed: the TestDummy was at 188/200 before the death",
                      checks["arena_reset"]["detail"])
        self.assertIn("the TestAttacker went Idle -> ", checks["arena_reset"]["detail"])
        self.assertIn("Attack", rep["pre_death"]["TestAttacker"]["states"])
        # The world went through the loop: one cast, one death, a reset arena.
        self.assertEqual(world.spell_points, 7.0)
        self.assertEqual(world.deaths, 1)
        self.assertFalse(world.is_dead)
        self.assertEqual(world.enemy("TestDummy")["health"], 200.0)
        self.assertEqual(world.enemy("TestAttacker")["ai_state"], "Idle")
        # Attached run: diagnostic only, nothing accepted.
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        self.assertFalse(any(a["accepted"] for a in rep["acs"]))
        # Audit trail: only input, state, waits and screenshots; real keys and a Left click.
        sent = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual(set(sent), INPUT_AND_READ_CMDS)
        keys = [r.get("key") or r.get("keys") for r in server.received if r.get("cmd") == "key"]
        for k in ("One", "LeftAlt", "W"):
            self.assertIn(k, keys)
        clicks = [r for r in server.received if r.get("cmd") == "click"]
        self.assertEqual([(c.get("x"), c.get("y"), c.get("button")) for c in clicks],
                         [(640, 360, "Left")])
        # Far actors are read with radius 3000, never with wait_until's fixed 1500.
        radii = {r.get("radius") for r in server.received if r.get("cmd") == "state"
                 and "actors" in (r.get("sections") or [])}
        self.assertEqual(radii, {us054c.ENEMY_RADIUS})
        md = self.md()
        self.assertIn("## Combat targets", md)
        self.assertEqual(set(rep["targets_start"]), {"TestDummy", "TestAttacker"})

    def test_the_game_as_expected_passes(self):
        # BasicSlash never burns: burning_on_hit watches until the death and ends at once with
        # a WARN; player_dies passes on the death burning_on_hit saw.
        _, server = self.serve(**game_today())
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-4000:])
        self.assertEqual(rep["known_failures"], [])
        self.assertEqual(rep["new_failures"], [], self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        checks = self.by_name(rep)
        self.assertEqual(checks["burning_on_hit"]["status"], "WARN")
        self.assertIn("the player died first", checks["burning_on_hit"]["detail"])
        self.assertIn("BasicSlash", checks["burning_on_hit"]["detail"])
        self.assertEqual(checks["player_dies"]["status"], "PASS", checks["player_dies"]["detail"])
        self.assertIn("first seen dead in burning_on_hit", checks["player_dies"]["detail"])
        self.assertNotIn("of player_dies", checks["player_dies"]["detail"])
        self.assertEqual(rep["death"]["step"], "burning_on_hit")
        # The death screen is seen on the first read, so the W hold has most of the 3 s window.
        self.assertEqual(checks["dead_no_move"]["status"], "PASS", checks["dead_no_move"]["detail"])
        for name in ("restoration_heals", "death_screen", "respawn", "death_screen_removed",
                     "arena_reset", "respawn_moves", "kill_counter_text_matches", "melee_lmb",
                     "dodge_lalt"):
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        acs = self.acs(rep)
        self.assertEqual(acs[("US-057", "AC-057.11")]["status"], "NOT RUN")
        for ac in (("US-053", "AC-053.1"), ("US-053", "AC-053.8"), ("US-053", "AC-053.9"),
                   ("US-054", "AC-054.6"), ("US-057", "AC-057.4"), ("US-062", "AC-062.1"),
                   ("US-063", "AC-063.1"), ("US-086", "AC-086.1"), ("US-086", "AC-086.4")):
            self.assertEqual(acs[ac]["status"], "PASS", ac)
        md = self.md()
        self.assertNotIn("FAIL (known)", md)
        self.assertNotIn("FAIL (NEW)", md)
        self.assertNotIn("Known issue N2", md)
        self.assertIn("Block and parry", md)

    def test_no_death_screen_fails_death_screen_as_new(self):
        # Issue #7 before its fix: the player dies and respawns, but no widget says 'You Died'
        _, server = self.serve(**game_today(death_screen=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["death_screen"]["status"], "FAIL")
        self.assertIn("not met within 1.5s", checks["death_screen"]["detail"])
        self.assertEqual(rep["new_failures"], ["death_screen"])
        self.assertEqual(rep["known_failures"], [])
        self.assertEqual(checks["death_screen_removed"]["status"], "SKIP")
        self.assertIn("never seen", checks["death_screen_removed"]["detail"])
        # Even after the full 1.5 s death-screen wait and the 'dead' screenshot there is time to
        # hold W before respawning.
        self.assertEqual(checks["dead_no_move"]["status"], "PASS", checks["dead_no_move"]["detail"])
        acs = self.acs(rep)
        for ac in (("US-062", "AC-062.1"), ("US-086", "AC-086.1"), ("US-086", "AC-086.4")):
            self.assertEqual(acs[ac]["status"], "FAIL", ac)

    def test_a_death_screen_left_after_the_respawn_fails(self):
        # The screen shows the same 'Kills: N' as the HUD, so kill_counter_text_matches cannot
        # tell it is still up: death_screen_removed does
        _, server = self.serve(**game_today(death_screen_stays=True))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["death_screen"]["status"], "PASS", checks["death_screen"]["detail"])
        self.assertEqual(checks["death_screen_removed"]["status"], "FAIL")
        self.assertIn("'You Died'", checks["death_screen_removed"]["detail"])
        self.assertEqual(checks["kill_counter_text_matches"]["status"], "PASS",
                         checks["kill_counter_text_matches"]["detail"])
        self.assertEqual(rep["new_failures"], ["death_screen_removed"])
        acs = self.acs(rep)
        self.assertEqual(acs[("US-086", "AC-086.1")]["status"], "FAIL")
        self.assertEqual(acs[("US-086", "AC-086.4")]["status"], "PASS")

    def test_the_known_cadence_fits_the_default_timeouts(self):
        # 15 damage per ~2.6 s with Restoration's heal: the death comes well inside the default
        # windows (no timeout is shortened here).
        world, server = self.serve()
        code, rep = self.play(server)
        checks = self.by_name(rep)
        self.assertEqual(rep["new_failures"], [], self.log.getvalue()[-4000:])
        self.assertEqual(code, 0)
        for name in ("attacker_damages_player", "restoration_heals", "player_dies", "respawn",
                     "arena_reset"):
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        gaps = [b[0] - a[0] for a, b in zip(world.hits, world.hits[1:])]
        self.assertTrue(gaps and all(2.4 <= g <= 2.9 for g in gaps), gaps)
        self.assertGreaterEqual(len(world.hits), 7)
        states = rep["pre_death"]["TestAttacker"]["states"]
        self.assertEqual(states[0], "Idle")
        self.assertIn("Attack", states)
        self.assertIn("Recover", states)

    def test_a_death_during_restoration_ends_burning_at_once(self):
        # A heal that never shows: restoration_heals watches until the attacker kills the player.
        _, server = self.serve(**game_today(heal_per_s=0.0))
        code, rep = self.play(server)
        checks = self.by_name(rep)
        self.assertEqual(checks["restoration_heals"]["status"], "WARN",
                         checks["restoration_heals"]["detail"])
        self.assertIn("died during the heal", checks["restoration_heals"]["detail"])
        self.assertEqual(checks["burning_on_hit"]["status"], "WARN")
        self.assertIn("the player died first", checks["burning_on_hit"]["detail"])
        self.assertEqual(checks["player_dies"]["status"], "PASS", checks["player_dies"]["detail"])
        self.assertIn("first seen dead in restoration_heals", checks["player_dies"]["detail"])
        self.assertEqual(checks["respawn"]["status"], "PASS")
        self.assertEqual(rep["new_failures"], [])

    def test_no_reset_fails_arena_reset(self):
        # Judged on the first read of the respawned player: the hurt dummy, and the attacker
        # still mid-swing at the dead player, are both seen unreset there
        _, server = self.serve(**game_today(**MID_SWING, arena_reset=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["arena_reset"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("not reset", check["detail"])
        self.assertIn("TestDummy health 188.0/200.0", check["detail"])
        self.assertRegex(check["detail"], AI_NOT_RESET)
        self.assertIn("at the respawn: ", check["detail"])
        self.assertIn("arena_reset", rep["new_failures"])
        self.assertEqual(self.acs(rep)[("US-053", "AC-053.8")]["status"], "FAIL")

    def test_no_reset_with_a_missed_dummy_is_not_a_pass(self):
        # The light attack misses the dummy (no health evidence) and nothing resets the arena.
        # The attacker swings on at the dead player, so it is mid-swing at the respawn, then
        # leashes back to Idle by itself (Recover -> Leash -> Idle): a later read would take that
        # for a reset of the AI.  Judged on the respawn read it FAILs, and AC-053.8/9 with it.
        world, server = self.serve(**game_today(**MID_SWING, arena_reset=False,
                                                click_damage=0.0))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["arena_reset"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertRegex(check["detail"], AI_NOT_RESET)
        self.assertNotIn("TestDummy health", check["detail"])   # the dummy was never hurt
        self.assertNotIn("reset observed", check["detail"])
        self.assertIn("arena_reset", rep["new_failures"])
        acs = self.acs(rep)
        for ac in (("US-053", "AC-053.8"), ("US-053", "AC-053.9")):
            self.assertEqual(acs[ac]["status"], "FAIL", ac)
        # The trap the respawn read avoids: the attacker does reach Idle on its own afterwards
        self.assertEqual(len(world.respawns), 1)
        self.assertEqual(self.attacker_states_after(world, world.respawns[0])[-2:],
                         ["Leash", "Idle"])

    def test_a_reset_that_leaves_the_ai_fails(self):
        # ResetArena restores location, health and is_dead (the dummy heals: health evidence) but
        # not the AI: the attacker is still mid-swing at the respawn, so arena_reset FAILs and
        # AC-053.9 (AI state reset) is not accepted
        world, server = self.serve(**game_today(**MID_SWING, reset_ai=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["arena_reset"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertRegex(check["detail"], AI_NOT_RESET)
        self.assertNotIn("TestDummy health", check["detail"])   # healed by the reset
        self.assertIn("TestDummy lowest health 188.0/200.0", check["detail"])
        self.assertEqual(rep["new_failures"], ["arena_reset"])
        acs = self.acs(rep)
        for ac in (("US-053", "AC-053.8"), ("US-053", "AC-053.9")):
            self.assertEqual(acs[ac]["status"], "FAIL", ac)
        self.assertEqual(world.enemy("TestDummy")["health"], 200.0)

    def attacker_states_after(self, world, since, timeout_s=4.0):
        """The TestAttacker's AI states from `since` on, once it is Idle again (or timeout_s)."""
        deadline = time.monotonic() + timeout_s
        while True:
            with world.lock:
                states = [s for label, t, s in world.ai_log
                          if label == "TestAttacker" and t >= since]
                idle = world.enemy("TestAttacker")["ai_state"] == "Idle"
            if idle or time.monotonic() >= deadline:
                return states
            time.sleep(0.05)

    def test_a_reset_with_no_observable_evidence_is_a_warn(self):
        # The dummy is never hurt and the attacker's ai_state always reads Idle: everything is
        # at its spawn after the respawn, but nothing shows a reset happened.
        _, server = self.serve(**game_today(click_damage=0.0, ai_reported=False))
        code, rep = self.play(server)
        check = self.by_name(rep)["arena_reset"]
        self.assertEqual(check["status"], "WARN", check["detail"])
        self.assertIn("no reset was observable", check["detail"])
        self.assertEqual(self.acs(rep)[("US-053", "AC-053.8")]["status"], "NOT RUN")
        self.assertEqual(rep["new_failures"], [])

    def test_no_attacker_damage_skips_the_death_chain(self):
        self.shorten(DAMAGE_TIMEOUT_S=1.5)
        _, server = self.serve(**game_today(attacker_hits=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["attacker_damages_player"]["status"], "FAIL")
        self.assertIn("player.attributes.health", checks["attacker_damages_player"]["detail"])
        self.assertIn("attacker_damages_player", rep["new_failures"])
        for name in ("restoration_heals", "burning_on_hit", "player_dies"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("never damaged", checks[name]["detail"])
        for name in ("death_screen", "dead_no_move", "respawn"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("did not die", checks[name]["detail"])
        for name in ("death_screen_removed", "arena_reset", "respawn_moves"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("did not respawn", checks[name]["detail"])
        for label in ("hit", "dead", "respawned"):
            self.assertEqual(checks[f"screenshot_{label}"]["status"], "SKIP", label)
        self.assertEqual(checks["kill_counter_text_matches"]["status"], "PASS")
        self.assertEqual(self.acs(rep)[("US-053", "AC-053.1")]["status"], "NOT RUN")

    def test_melee_damage_on_the_dummy_alone_passes(self):
        _, server = self.serve(**game_today(attack_tag=False))
        _, rep = self.play(server)
        check = self.by_name(rep)["melee_lmb"]
        self.assertEqual(check["status"], "PASS", check["detail"])
        self.assertIn("'Mordecai.State.Attacking' seen: False", check["detail"])
        self.assertIn("(188.0, 200.0) after", check["detail"])

    def test_unbound_attacks_fail_melee_and_dodge_as_new(self):
        _, server = self.serve(**game_today(attacks=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for name in ("melee_lmb", "dodge_lalt"):
            self.assertEqual(checks[name]["status"], "FAIL", name)
            self.assertIn(name, rep["new_failures"])
        self.assertIn("no 'Mordecai.State.Attacking' tag and no TestDummy damage",
                      checks["melee_lmb"]["detail"])
        # Nothing hurt the dummy, but the attacker left Idle: the reset is still observed.
        self.assertEqual(checks["arena_reset"]["status"], "PASS", checks["arena_reset"]["detail"])

    def test_targets_are_found_by_class_name_when_labels_are_object_names(self):
        specs = (dict(fb.TEST_DUMMY_SPEC, label="BP_MordecaiEnemy_C_1"),
                 dict(fb.TEST_ATTACKER_SPEC, label="BP_MordecaiEnemy_C_0"))
        _, server = self.serve(**dict(FAST, enemies=specs))
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-4000:])
        detail = self.by_name(rep)["combat_targets_present"]["detail"]
        self.assertIn("TestDummy 'BP_MordecaiEnemy_C_1' at (600, 600)", detail)
        self.assertIn("TestAttacker 'BP_MordecaiEnemy_C_0' at (2100, 0)", detail)

    def test_a_missing_or_misplaced_target_fails_the_first_check(self):
        # No attacker, and a dummy with the native 100 max health (an N13-style override).
        self.shorten(DAMAGE_TIMEOUT_S=1.5)
        _, server = self.serve(**game_today(enemies=(dict(fb.TEST_DUMMY_SPEC,
                                                          max_health=100.0),)))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        check = checks["combat_targets_present"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("no TestAttacker in actors[]", check["detail"])
        self.assertIn("TestDummy health 100.0/100.0 (expected 200/200)", check["detail"])
        self.assertIn("combat_targets_present", rep["new_failures"])
        self.assertEqual(checks["attacker_damages_player"]["status"], "FAIL")
        self.assertIn("TestAttacker: not in actors[]", checks["attacker_damages_player"]["detail"])

    def test_an_attacker_in_aggro_range_of_the_spawn_fails_the_first_check(self):
        self.shorten(DAMAGE_TIMEOUT_S=1.5)  # it stands far from the fight spot: no damage
        close = dict(fb.TEST_ATTACKER_SPEC, location=[800.0, -400.0, 100.0])
        _, server = self.serve(**game_today(enemies=(fb.TEST_DUMMY_SPEC, close)))
        code, rep = self.play(server)
        check = self.by_name(rep)["combat_targets_present"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("TestAttacker at (800, -400) is", check["detail"])
        self.assertIn("cm from the player (aggro range 900)", check["detail"])

    def test_broken_burning_death_and_kill_counter_are_detected(self):
        _, server = self.serve(**game_today(burn_chance=1.0, burn_indicator=False,
                                            dead_moves=True, kills_text=2))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["burning_on_hit"]["status"], "FAIL")
        self.assertIn("no widget text 'Burning'", checks["burning_on_hit"]["detail"])
        self.assertEqual(checks["dead_no_move"]["status"], "FAIL", checks["dead_no_move"]["detail"])
        self.assertEqual(checks["kill_counter_text_matches"]["status"], "FAIL")
        for name in ("burning_on_hit", "dead_no_move", "kill_counter_text_matches"):
            self.assertIn(name, rep["new_failures"])

    def test_movement_still_ignored_after_the_respawn_fails_respawn_moves(self):
        _, server = self.serve(**game_today(moves_after_respawn=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["respawn_moves"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("s simulated", check["detail"])
        self.assertIn("respawn_moves", rep["new_failures"])
        self.assertEqual(self.acs(rep)[("US-053", "AC-053.6")]["status"], "FAIL")

    def test_restoration_that_does_nothing_fails(self):
        _, server = self.serve(**game_today(restoration=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["restoration_heals"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("'Mordecai.Status.Restoration' seen: False", check["detail"])


if __name__ == "__main__":
    unittest.main()
