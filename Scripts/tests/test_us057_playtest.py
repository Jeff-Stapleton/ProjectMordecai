"""Tests for Scripts/playtests/us057_magic.py against tests/fake_bridge.py (FakeWorld).

MagicWorld below models CombatTestMap (fb.MAP_PRESETS: the TestDummy at (600, 600), the
TestAttacker at (2100, 0), both stationary) and adds what the shared fake lacks: Fireball
projectiles that fly +X and damage the enemies (200 HP), the Blink teleport, the Stone Skin /
Restoration status tags, the SP bar, the spell cooldown widgets and the status indicator
widgets, and optional cooldowns.
These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Us057
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

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us057_magic.py")
TOKEN = "us057-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "click", "screenshot", "wait_until", "wait", "quit"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us057_magic", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us057 = load_playtest()

STONE_SKIN_TAG = "Mordecai.Status.StoneSkin"
RESTORATION_TAG = "Mordecai.Status.Restoration"
COOLDOWN_S = {"Fireball": 2.0, "Blink": 4.0, "StoneSkin": 15.0, "Restoration": 10.0}
TAG_FOR = {"StoneSkin": STONE_SKIN_TAG, "Restoration": RESTORATION_TAG}
COOLDOWN_NAMES = "ABCDEFGH"


class MagicWorld(fb.FakeWorld):
    """FakeWorld plus the magic loop (the shared fake has the loadout and SP only).

    Options: cooldowns (spells set a cooldown, the state reports it and the cooldown widgets
    show it; False = today's game), cooldown_widgets (2 = today's HUD), projectiles (Fireball
    spawns one), projectile_speed, fireball_damage, cast_delay_s (Fireball's cast time before
    the projectile), blink_distance, stone_skin_s (None = never expires), restoration_s,
    sp_bar (the SpellPointsBar widget is on screen), indicators (status indicator widgets),
    restoration_ignores_cost (One applies Restoration without SP: a broken refusal).
    """

    def __init__(self, *, cooldowns=False, cooldown_widgets=2, projectiles=True,
                 projectile_speed=1000.0, fireball_damage=25.0, cast_delay_s=0.3,
                 blink_distance=500.0, stone_skin_s=2.5, restoration_s=5.0, sp_bar=True,
                 indicators=True, restoration_ignores_cost=False, enemy_hp=200.0, **kwargs):
        for name, value in fb.MAP_PRESETS[fb.COMBAT_TEST_MAP].items():
            kwargs.setdefault(name, value)
        super().__init__(**kwargs)
        self.cooldowns = cooldowns
        self.cooldown_widget_count = cooldown_widgets
        self.projectiles_on = projectiles
        self.projectile_speed = projectile_speed
        self.fireball_damage = fireball_damage
        self.cast_delay_s = cast_delay_s
        self.blink_distance = blink_distance
        self.stone_skin_s = stone_skin_s
        self.restoration_s = restoration_s
        self.sp_bar = sp_bar
        self.indicators = indicators
        self.restoration_ignores_cost = restoration_ignores_cost
        self.enemy_hp = {e["label"]: float(enemy_hp) for e in self.enemies}
        self.enemy_max_hp = float(enemy_hp)
        self.projectiles = []      # {label, pos [x, y], travelled}
        self.projectile_n = 0
        self.status = {}           # tag -> expiry (real time) or None (never)
        self.cooldown_until = {}   # spell_id -> real time
        self.cast_log = []         # (key, spell_id, player [x, y]) of every successful cast

    # --- casting ------------------------------------------------------------------------
    def _on_key_pressed(self, key):
        if key not in fb.SPELL_KEYS or not self.ready or self.menu_open or self.is_dead:
            return super()._on_key_pressed(key)
        spell_id = fb.SPELL_KEYS[key]
        cost = fb.SPELL_COSTS[spell_id]
        now = self.real_time()
        if self.cooldown_until.get(spell_id, 0.0) > now:
            return
        free = spell_id == "Restoration" and self.restoration_ignores_cost
        if self.spell_points < cost and not free:
            return
        if not free:
            self.spell_points -= cost
        self.casts.append(key)
        self.cast_log.append((key, spell_id, list(self.pos)))
        if self.cooldowns:
            self.cooldown_until[spell_id] = now + COOLDOWN_S[spell_id]
        if spell_id == "Fireball" and self.projectiles_on:
            self.after(self.cast_delay_s, lambda w: w._spawn_projectile())
        elif spell_id == "Blink":
            self._blink()
        elif spell_id in TAG_FOR:
            secs = self.stone_skin_s if spell_id == "StoneSkin" else self.restoration_s
            self.status[TAG_FOR[spell_id]] = None if secs is None else now + secs

    def _spawn_projectile(self):
        self.projectile_n += 1
        self.projectiles.append({"label": f"BP_MordecaiProjectile_Fireball_C_{self.projectile_n}",
                                 "pos": list(self.pos), "travelled": 0.0})

    def _blink(self):
        x, y = self.pos
        step = 10.0
        moved = 0.0
        while moved + step <= self.blink_distance and not self._blocked(x + step, y):
            x += step
            moved += step
        self.pos = [x, y]

    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            now = self.real_time()
            for tag, until in list(self.status.items()):
                if until is not None and now >= until:
                    del self.status[tag]
            for p in list(self.projectiles):
                d = self.projectile_speed * dt
                p["pos"][0] += d  # facing is pinned to +X
                p["travelled"] += d
                hit = None
                for e in self.enemies:
                    if math.hypot(e["location"][0] - p["pos"][0],
                                  e["location"][1] - p["pos"][1]) <= 60.0:
                        hit = e
                        break
                if hit is not None:
                    self.enemy_hp[hit["label"]] = max(
                        0.0, self.enemy_hp[hit["label"]] - self.fireball_damage)
                    self.projectiles.remove(p)
                elif p["travelled"] >= 1500.0 or self._blocked(*p["pos"]):
                    self.projectiles.remove(p)

    # --- state ------------------------------------------------------------------------------
    def _cooldown_left(self, spell_id):
        return max(0.0, self.cooldown_until.get(spell_id, 0.0) - self.real_time())

    def _s_player(self):
        d = super()._s_player()
        if d.get("valid"):
            d["tags"] = list(d["tags"]) + sorted(self.status)
            for s in d["spells"]:
                s["cooldown_remaining_s"] = round(self._cooldown_left(s["spell_id"]), 2)
                if self.cooldowns:
                    s["cooldown_tag"] = f"Mordecai.Cooldown.Spell.{s['spell_id']}"
        return d

    def _s_widgets(self):
        out = super()._s_widgets()
        if self.sp_bar:
            out.append({"class": "WBP_SpellPointsBar_C", "name": "SpellPointsBar",
                        "rect": [20, 100, 250, 16],
                        "texts": [f"{int(self.spell_points)} / {int(self.max_spell_points)}"],
                        "buttons": []})
        for i in range(self.cooldown_widget_count):
            slot = fb.SPELL_SLOTS[i] if i < len(fb.SPELL_SLOTS) else None
            left = self._cooldown_left(slot[1]) if slot and self.cooldowns else 0.0
            out.append({"class": f"WBP_SpellCooldown{COOLDOWN_NAMES[i]}_C",
                        "name": f"SpellCooldown{COOLDOWN_NAMES[i]}",
                        "rect": [20 + 130 * i, 124, 120, 16],
                        "texts": [f"{left:.1f}" if left > 0 else "Ready"], "buttons": []})
        out.append({"class": "MordecaiStatusEffectBarWidget", "name": "StatusEffectBar",
                    "rect": [20, 620, 400, 40], "texts": [], "buttons": []})
        if self.indicators:
            now = self.real_time()
            for tag, until in sorted(self.status.items()):
                left = 99.0 if until is None else max(0.0, until - now)
                out.append({"class": "MordecaiStatusEffectIndicatorWidget",
                            "name": f"Indicator_{tag.split('.')[-1]}", "rect": [20, 620, 80, 40],
                            "texts": [tag.split(".")[-1], f"{left:.1f}"], "buttons": []})
        return out

    def _actors(self, radius, include_all):
        entries = super()._actors(radius, include_all)
        for e in entries:
            if e["kind"] == "enemy":
                hp = self.enemy_hp.get(e["label"], self.enemy_max_hp)
                e["health"] = {"current": hp, "max": self.enemy_max_hp}
                e["is_dead"] = hp <= 0
        px, py = self.pos
        for p in self.projectiles:
            x, y = p["pos"]
            d = math.hypot(x - px, y - py)
            if d <= radius:
                entries.append({"label": p["label"], "name": p["label"],
                                "class": "BP_MordecaiProjectile_Fireball_C",
                                "native_class": "MordecaiProjectile", "kind": "projectile",
                                "location": [round(x, 2), round(y, 2), 100.0],
                                "distance": round(d, 1), "hidden": False,
                                "has_visible_mesh": True, "health": None, "is_dead": None,
                                "ai_state": None, "tags": [], "widgets": []})
        entries.sort(key=lambda e: e["distance"])
        return entries


class SteppingEnemyWorld(MagicWorld):
    """The TestDummy moves 120 cm +Y the first time the player stands near the shot spot
    (300, 600).  (The real dummy never moves; this exercises the re-walk.)"""

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.enemy_moved = False

    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            sx, sy = fb.COMBAT_DUMMY_SHOT_SPOT
            if not self.enemy_moved and math.hypot(self.pos[0] - sx, self.pos[1] - sy) < 80.0:
                for e in self.enemies:
                    if e["label"] == "TestDummy":
                        e["location"][1] += 120.0
                self.enemy_moved = True


# CombatTestMap's enemies under object names that do not say which is which: the target is the
# enemy nearest the TestDummy's placement (600, 600).
UNNAMED_ENEMIES = (dict(fb.TEST_DUMMY_SPEC, label="BP_MordecaiEnemy_C_1"),
                   dict(fb.TEST_ATTACKER_SPEC, label="BP_MordecaiEnemy_C_0"))


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class PlaytestCase(unittest.TestCase):
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
        world = MagicWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server, *extra):
        code = us057.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir, *extra])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)

    @staticmethod
    def by_name(rep):
        return {c["name"]: c for c in rep["checks"]}

    @staticmethod
    def statuses(rep, status):
        return [c["name"] for c in rep["checks"] if c["status"] == status]

    def md(self):
        with open(os.path.join(self.run_dir, "report.md"), encoding="utf-8") as f:
            return f.read()


TODAY_KNOWN = ["cooldown_widgets", "fireball_cooldown_state", "cooldown_hud"]
PLAN_CHECKS = [s.check for s in us057.PLAN]


class TestUs057PassPath(PlaytestCase):
    def test_a_game_with_cooldowns_and_four_widgets_passes_everything(self):
        world, server = self.serve(cooldowns=True, cooldown_widgets=4)
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "FAIL"), [])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        checks = self.by_name(rep)
        for name in PLAN_CHECKS:
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        # The SP plan 10 -> 7 -> 5 -> 1, Restoration refused; the game state agrees.
        self.assertEqual(world.casts, ["Q", "E", "R"])
        self.assertEqual(world.spell_points, 1.0)
        self.assertEqual([(s["spell"], s["before"], s["after"]) for s in rep["spell_points"]],
                         [("Fireball", 10.0, 7.0), ("Blink", 7.0, 5.0), ("Stone Skin", 5.0, 1.0)])
        self.assertEqual(world.enemy_hp["TestDummy"], 175.0)
        self.assertEqual(world.enemy_hp["TestAttacker"], 200.0)
        self.assertIn("25 damage", checks["fireball_hits_enemy"]["detail"])
        self.assertIn("TestDummy health 200 -> 175", checks["fireball_hits_enemy"]["detail"])
        self.assertEqual(checks["map"]["status"], "PASS", checks["map"]["detail"])
        self.assertIn(fb.COMBAT_TEST_MAP, checks["map"]["detail"])
        # The known issues all passed: each is flagged FIXED? (re-check the backlog).
        self.assertEqual(sorted(rep["fixed_candidates"]), sorted(TODAY_KNOWN))
        self.assertIn("PASS (FIXED?)", self.md())
        self.assertNotIn(RESTORATION_TAG, world.status)
        self.assertNotIn(STONE_SKIN_TAG, world.status)

    def test_fireball_is_cast_on_the_shot_line_never_from_spawn(self):
        world, server = self.serve()
        self.play(server)
        key, spell_id, (x, y) = world.cast_log[0]
        self.assertEqual((key, spell_id), ("Q", "Fireball"))
        # The shot spot: 300 cm -X of the TestDummy (600, 600).
        sx, sy = fb.COMBAT_DUMMY_SHOT_SPOT
        self.assertEqual((sx, sy), (300.0, 600.0))
        self.assertLess(abs(y - sy), us057.SHOT_TOL + 1)
        self.assertLess(abs(x - sx), us057.SHOT_TOL + 1)
        # Blink from the start of the Blink lane (0, -600), along +X.
        blink = world.cast_log[1]
        self.assertEqual(blink[1], "Blink")
        self.assertEqual(us057.LANE_POINT, fb.COMBAT_BLINK_LANE_START)
        bx, by = fb.COMBAT_BLINK_LANE_START
        self.assertLess(math.hypot(blink[2][0] - bx, blink[2][1] - by), us057.WALK_TOL + 1)
        # Every cast spot is out of the TestAttacker's 900 cm aggro range.
        ax, ay = fb.TEST_ATTACKER_SPEC["location"][:2]
        for _, _, (cx, cy) in world.cast_log:
            self.assertGreater(math.hypot(cx - ax, cy - ay), 900.0)

    def test_the_target_falls_back_to_the_enemy_nearest_the_dummy_placement(self):
        # Packaged labels are object names: no 'TestDummy' in them.  The enemy nearest (600, 600)
        # is the target, never the attacker.
        world, server = self.serve(enemies=UNNAMED_ENEMIES)
        code, rep = self.play(server)
        checks = self.by_name(rep)
        self.assertEqual(checks["walk_shot_line"]["status"], "PASS",
                         checks["walk_shot_line"]["detail"])
        self.assertIn("BP_MordecaiEnemy_C_1 at (600, 600)", checks["walk_shot_line"]["detail"])
        self.assertEqual(checks["fireball_hits_enemy"]["status"], "PASS")
        self.assertEqual(world.enemy_hp["BP_MordecaiEnemy_C_1"], 175.0)
        self.assertEqual(world.enemy_hp["BP_MordecaiEnemy_C_0"], 200.0)

    def test_rewalks_when_the_enemy_moved(self):
        # The TestDummy steps 120 cm +Y as soon as the player reaches the first shot position.
        world = SteppingEnemyWorld()
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        code, rep = self.play(server)
        checks = self.by_name(rep)
        self.assertTrue(world.enemy_moved)
        self.assertEqual(checks["walk_shot_line"]["status"], "PASS",
                         checks["walk_shot_line"]["detail"])
        self.assertIn("re-walked once", checks["walk_shot_line"]["detail"])
        self.assertEqual(checks["fireball_hits_enemy"]["status"], "PASS")


class TestUs057TodaysGame(PlaytestCase):
    def test_todays_game_fails_only_the_known_issues(self):
        _, server = self.serve()  # no cooldowns, 2 cooldown widgets: the packaged game today
        code, rep = self.play(server)
        self.assertEqual(code, 1, self.log.getvalue()[-4000:])
        self.assertEqual(sorted(self.statuses(rep, "FAIL")), sorted(TODAY_KNOWN))
        self.assertEqual(sorted(rep["known_failures"]), sorted(TODAY_KNOWN))
        self.assertEqual(rep["new_failures"], [])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        checks = self.by_name(rep)
        self.assertIn("2 SpellCooldown widget(s) on screen (expected 4)",
                      checks["cooldown_widgets"]["detail"])
        self.assertIn("cooldown_remaining_s stayed 0.00", checks["fireball_cooldown_state"]["detail"])
        self.assertIn("cooldown_tag ''", checks["fireball_cooldown_state"]["detail"])
        self.assertIn("Ready", checks["cooldown_hud"]["detail"])
        md = self.md()
        self.assertIn("FAIL (known)", md)
        self.assertNotIn("FAIL (NEW)", md)
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-057", "AC-057.1")]["status"], "FAIL")
        self.assertEqual(acs[("US-057", "AC-057.9")]["status"], "FAIL")
        self.assertEqual(acs[("US-055", "AC-055.8")]["status"], "FAIL")
        for ac in (("US-057", "AC-057.2"), ("US-057", "AC-057.3"), ("US-057", "AC-057.10"),
                   ("US-055", "AC-055.4"), ("US-055", "AC-055.6")):
            self.assertEqual(acs[ac]["status"], "PASS", ac)
        # An attached run is diagnostic: nothing is accepted.
        self.assertFalse(any(a["accepted"] for a in rep["acs"]))
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])


class TestUs057Failures(PlaytestCase):
    def fail_set(self, rep):
        return sorted(set(self.statuses(rep, "FAIL")) - set(TODAY_KNOWN))

    def test_no_projectile_and_no_damage_are_new_failures(self):
        _, server = self.serve(projectiles=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.fail_set(rep), ["fireball_hits_enemy", "fireball_projectile"])
        self.assertIn("fireball_projectile", rep["new_failures"])
        self.assertIn("fireball_hits_enemy", rep["new_failures"])
        checks = self.by_name(rep)
        self.assertIn('no actors[] entry with kind=="projectile"',
                      checks["fireball_projectile"]["detail"])
        self.assertIn("health stayed 200", checks["fireball_hits_enemy"]["detail"])
        # SP still dropped: the cast and the HUD checks pass.
        self.assertEqual(checks["fireball_cast"]["status"], "PASS")
        self.assertEqual(checks["sp_hud_updates"]["status"], "PASS")

    def test_a_projectile_that_does_no_damage_fails_the_hit_only(self):
        _, server = self.serve(fireball_damage=0.0)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.fail_set(rep), ["fireball_hits_enemy"])
        self.assertIn("projectile seen: True", self.by_name(rep)["fireball_hits_enemy"]["detail"])

    def test_a_missing_sp_bar_fails_the_hud_checks(self):
        _, server = self.serve(sp_bar=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.fail_set(rep), ["hud_visible", "sp_hud_updates"])
        detail = self.by_name(rep)["hud_visible"]["detail"]
        self.assertIn("expected '10 / 10' in a SpellPointsBar widget", detail)
        self.assertIn("player SP 10 / 10", detail)

    def test_a_blink_that_does_not_move_fails(self):
        world, server = self.serve(blink_distance=0.0)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.fail_set(rep), ["blink"])
        self.assertIn("x moved +0 (needs > 400)", self.by_name(rep)["blink"]["detail"])
        # SP was still spent, so the rest of the SP plan runs on.
        self.assertEqual(self.by_name(rep)["stone_skin"]["status"], "PASS")

    def test_restoration_cast_without_sp_fails_the_refusal(self):
        _, server = self.serve(restoration_ignores_cost=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.fail_set(rep), ["restoration_refused_low_sp"])
        self.assertIn("Mordecai.Status.Restoration was applied",
                      self.by_name(rep)["restoration_refused_low_sp"]["detail"])

    def test_a_missing_indicator_fails(self):
        _, server = self.serve(indicators=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.fail_set(rep), ["stone_skin_indicator"])
        self.assertIn("no widget renders 'StoneSkin'",
                      self.by_name(rep)["stone_skin_indicator"]["detail"])
        # With no indicator ever drawn, the expiry is judged on the tag alone (and passes).
        self.assertEqual(self.by_name(rep)["stone_skin_expires"]["status"], "PASS")

    def test_stone_skin_that_never_expires_fails(self):
        _, server = self.serve(stone_skin_s=None)
        with mock.patch.object(us057, "EXPIRE_TIMEOUT_S", 1.0):
            code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.fail_set(rep), ["stone_skin_expires"])
        self.assertIn("tag still on", self.by_name(rep)["stone_skin_expires"]["detail"])

    def test_no_enemies_fails_the_walk_and_skips_the_hit(self):
        world, server = self.serve(enemies=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["walk_shot_line"]["status"], "FAIL")
        self.assertIn("no enemy 'TestDummy'", checks["walk_shot_line"]["detail"])
        self.assertIn("no enemy near (600, 600)", checks["walk_shot_line"]["detail"])
        self.assertEqual(checks["fireball_hits_enemy"]["status"], "SKIP")
        self.assertIn("walk_shot_line failed", checks["fireball_hits_enemy"]["detail"])
        self.assertEqual(checks["fireball_cast"]["status"], "PASS")

    def test_a_broken_sp_plan_skips_what_depends_on_it(self):
        # Stone Skin costs nothing here: SP stays 5, so Restoration is affordable and its refusal
        # cannot be tested (SKIP), and the Stone Skin check fails on SP.
        _, server = self.serve()
        with mock.patch.dict(fb.SPELL_COSTS, {"StoneSkin": 0.0}):
            code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["stone_skin"]["status"], "FAIL")
        self.assertIn("(expected 1)", checks["stone_skin"]["detail"])
        self.assertEqual(checks["restoration_refused_low_sp"]["status"], "SKIP")
        self.assertIn("enough for Restoration", checks["restoration_refused_low_sp"]["detail"])
        self.assertEqual(checks["stone_skin_indicator"]["status"], "SKIP")
        self.assertEqual(checks["stone_skin_expires"]["status"], "SKIP")


class TestUs057Report(PlaytestCase):
    def test_report_fields_screenshots_and_audit_trail(self):
        _, server = self.serve()
        _, rep = self.play(server)
        self.assertEqual(rep["story"], "US-057")
        self.assertEqual(rep["covers"], ["US-055"])
        self.assertEqual(rep["features"], ["spells", "spell_hud", "projectiles_aim",
                                           "status_effects"])
        self.assertEqual([p["check"] for p in rep["plan"]], PLAN_CHECKS)
        self.assertEqual(rep["undeclared_checks"], [])
        self.assertEqual([s["label"] for s in rep["screenshots"]],
                         ["fireball_hit", "after_blink", "stone_skin", "spell_hud"])
        for s in rep["screenshots"]:
            self.assertTrue(ab.file_complete(s["path"]), s["path"])
            self.assertTrue(s["expect"])
        self.assertIn("orange damage number", rep["screenshots"][0]["expect"])
        self.assertEqual(len(rep["notes"]), len(us057.NOTES))
        self.assertTrue(any("N6" in n for n in rep["notes"]))
        self.assertTrue(any("Gamepad spell bindings" in n for n in rep["notes"]))
        self.assertEqual(rep["known_issues"], list(us057.KNOWN_ISSUES))
        self.assertIsNotNone(rep["fireball"]["projectile_at"])
        self.assertAlmostEqual(rep["blink"]["dx"], 500.0, delta=0.1)
        ac_ids = {(a["story"], a["id"]) for a in rep["acs"]}
        for ac in (("US-057", "AC-057.1"), ("US-057", "AC-057.12"), ("US-055", "AC-055.9")):
            self.assertIn(ac, ac_ids)
        md = self.md()
        for text in ("## Spell points", "## Fireball observations", "## Notes",
                     "10 -> 7 -> 5 -> 1", "## Acceptance criteria"):
            self.assertIn(text, md)
        # Real input only: input, state reads, waits and screenshots.
        sent = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual(set(sent), INPUT_AND_READ_CMDS)
        keys = [r.get("key") or r.get("keys") for r in server.received if r.get("cmd") == "key"]
        for k in ("Q", "E", "R", "One"):
            self.assertIn(k, keys)
        self.assertFalse(any(r.get("route") == "viewport" or r.get("unsafe")
                             for r in server.received if r.get("cmd") == "key"))

    def test_plan_prints_the_steps_table(self):
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        try:
            code = us057.main(["--plan"])
        finally:
            kit.set_log_stream(previous)
        self.assertEqual(code, 0)
        rows = [line for line in out.getvalue().splitlines() if line.startswith("| ")
                and not line.startswith("| #")]
        self.assertEqual(len(rows), len(PLAN_CHECKS))
        self.assertIn("`fireball_hits_enemy`", out.getvalue())
        self.assertIn("known issue", out.getvalue())
        self.assertEqual(kit.plan_problems(us057.Run), [])

    def test_ac_ids_exist_in_the_story_files(self):
        stories = os.path.join(os.path.dirname(SCRIPTS), "stories", "done")
        text = {}
        for story, name in (("US-057", "US-057-playable-magic-arena.md"),
                            ("US-055", "US-055-spell-hud-status-indicators.md")):
            with open(os.path.join(stories, name), encoding="utf-8") as f:
                text[story] = f.read()
        for step in us057.PLAN:
            for ac in step.proves:
                story, ac_id = kit.split_ac(ac, "US-057")
                self.assertIn(f"{ac_id}:", text[story], ac)


if __name__ == "__main__":
    unittest.main()
