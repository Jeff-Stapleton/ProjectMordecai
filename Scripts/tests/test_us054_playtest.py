"""Tests for Scripts/playtests/us054_arena.py against tests/fake_bridge.py.

ArenaWorld (below) adds what the shared FakeWorld lacks on DevTestMap: 200 HP Frontliners that
aggro, walk up to the player standing at the spawn and hit them, HUD bar widgets with rects and
enemy indicators.  The combat half of US-054 (walk, melee, dodge, death, respawn, arena reset)
is Scripts/playtests/us054_combat.py on CombatTestMap: tests/test_us054_combat_playtest.py.
These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k us054
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

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us054_arena.py")
TOKEN = "us054-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "click", "screenshot", "wait_until", "wait", "quit"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us054_arena", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us054 = load_playtest()

ARENA_SPAWNS = ((600.0, 0.0), (-400.0, 500.0), (-400.0, -500.0))
PLAN_ORDER = ["hud_visible", "hud_layout", "enemies_present", "enemies_visible",
              "enemy_health_bars", "enemy_indicators", "enemy_aggro", "enemy_approaches",
              "enemy_damages_player"]
# The DevTestMap checks keep the proves they had before the us054 split.
PROVES = {
    "hud_visible": ("AC-054.7", "AC-054.8", "US-063:AC-063.3", "US-063:AC-063.4"),
    "hud_layout": ("AC-054.7",),
    "enemies_present": ("AC-054.11", "AC-054.2", "US-050:AC-050.3"),
    "enemies_visible": ("AC-054.13",),
    "enemy_health_bars": ("AC-054.9", "US-052:AC-052.10"),
    "enemy_indicators": ("US-064:AC-064.2", "US-064:AC-064.10"),
    "enemy_aggro": ("US-051:AC-051.3",),
    "enemy_approaches": ("US-051:AC-051.4",),
    "enemy_damages_player": ("AC-054.13", "US-051:AC-051.5"),
}


class ArenaWorld(fb.FakeWorld):
    """The DevTestMap arena with fighting Frontliners.  Defaults model the game as it should be
    (every check passes); the options switch each behaviour off or break it.

    The Frontliners aggro aggro_s after the start, then (approach) walk toward the player at
    approach_speed until stop_cm from them, and (damage) hit a player within hit_range for
    hit_damage every hit_interval."""

    def __init__(self, *, visible_mesh=True, enemy_hp=200.0, aggro=True, aggro_s=0.3,
                 approach=True, approach_speed=400.0, stop_cm=150.0, damage=True,
                 hit_damage=5.0, hit_interval=0.3, hit_range=200.0, indicators=True,
                 enemy_bars=True, hud_layout_ok=True, kills_text=0, **kwargs):
        kwargs.setdefault("obstacles", fb.DEVTESTMAP_OBSTACLES)
        super().__init__(enemies=False, **kwargs)
        self.visible_mesh = visible_mesh
        self.enemy_hp = float(enemy_hp)
        self.aggro, self.aggro_s = aggro, aggro_s
        self.approach, self.approach_speed, self.stop_cm = approach, approach_speed, stop_cm
        self.damage, self.hit_damage, self.hit_interval = damage, hit_damage, hit_interval
        self.hit_range = hit_range
        self.indicators, self.enemy_bars = indicators, enemy_bars
        self.hud_layout_ok, self.kills_text = hud_layout_ok, kills_text
        self.aggroed = False
        self.hits = 0
        self.arena = [{"location": [s[0], s[1], 100.0], "health": self.enemy_hp,
                       "ai_state": "Idle", "next_hit": 0.0} for s in ARENA_SPAWNS]

    # --- simulation -------------------------------------------------------------------------
    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            now = self.real_time()
            if self.aggro and not self.aggroed and now >= self.aggro_s and self.ready:
                self.aggroed = True
                for e in self.arena:
                    e["ai_state"] = "Approach"
            if not self.aggroed or self.is_dead or not self.ready or self.menu_open:
                return
            for e in self.arena:
                dx, dy = self.pos[0] - e["location"][0], self.pos[1] - e["location"][1]
                d = math.hypot(dx, dy)
                if self.approach and d > self.stop_cm:
                    step = min(self.approach_speed * dt, d - self.stop_cm)
                    e["location"][0] += dx / d * step
                    e["location"][1] += dy / d * step
                    d -= step
                if self.damage and d <= self.hit_range and now >= e["next_hit"]:
                    e["next_hit"] = now + self.hit_interval
                    e["ai_state"] = "Attack"
                    self.hits += 1
                    self.health -= self.hit_damage
                    if self.health <= 0:
                        self.kill_player()
                        return

    # --- state --------------------------------------------------------------------------------
    def _s_widgets(self):
        out = super()._s_widgets()
        rows = ([[40, 20], [40, 44], [40, 64]] if self.hud_layout_ok
                else [[40, 64], [40, 44], [40, 20]])
        for (x, y), cls in zip(rows, ("MordecaiHealthBarWidget", "MordecaiStaminaBarWidget",
                                      "MordecaiPostureBarWidget")):
            out.append({"class": cls, "name": cls + "_0", "rect": [x, y, 260, 16], "texts": [],
                        "buttons": []})
        out.append({"class": "MordecaiSpellPointsBarWidget", "name": "SP", "rect": [40, 84, 260, 16],
                    "texts": [f"{self.spell_points:g} / {self.max_spell_points:g}"], "buttons": []})
        out.append({"class": "MordecaiKillCounterWidget", "name": "Kills",
                    "rect": [1140, 20, 120, 30], "texts": [f"Kills: {self.kills_text}"],
                    "buttons": []})
        if self.indicators and self.aggroed:
            for i, _ in enumerate(self.arena):
                out.append({"class": "MordecaiEnemyIndicatorWidget", "name": f"Ind_{i}",
                            "rect": [100 * i, 300, 80, 20], "texts": [], "buttons": []})
        return out

    def _actors(self, radius, include_all):
        entries = super()._actors(radius, include_all)
        px, py = self.pos
        for i, e in enumerate(self.arena):
            loc = e["location"]
            d = math.sqrt((loc[0] - px) ** 2 + (loc[1] - py) ** 2 + (loc[2] - fb.PAWN_Z) ** 2)
            if d > radius:
                continue
            entries.append({
                "label": f"BP_MordecaiEnemy_Frontliner_C_{i}", "name": f"Frontliner_{i}",
                "class": "BP_MordecaiEnemy_Frontliner_C", "native_class": "MordecaiEnemyCharacter",
                "kind": "enemy", "location": [round(v, 2) for v in loc], "distance": round(d, 1),
                "hidden": False, "has_visible_mesh": self.visible_mesh,
                "health": {"current": e["health"], "max": self.enemy_hp},
                "is_dead": False, "ai_state": e["ai_state"], "tags": [],
                "widgets": ([{"component": "HealthBarWidget", "class": "WBP_EnemyHealthBar_C",
                              "space": "Screen", "visible": True}] if self.enemy_bars else [])})
        entries.sort(key=lambda a: a["distance"])
        return entries


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class Us054Case(unittest.TestCase):
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
        # Short waits for the paths that are expected to time out.
        for name, value in (("APPROACH_TIMEOUT_S", 1.5), ("DAMAGE_TIMEOUT_S", 1.5),
                            ("INDICATOR_TIMEOUT_S", 1.5), ("AGGRO_TIMEOUT_S", 1.5),
                            ("HUD_TIMEOUT_S", 1.5)):
            p = mock.patch.object(us054, name, value)
            p.start()
            self.addCleanup(p.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, **world_kwargs):
        world = ArenaWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        code = us054.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)

    @staticmethod
    def by_name(rep):
        return {c["name"]: c for c in rep["checks"]}

    @staticmethod
    def statuses(rep, status):
        return [c["name"] for c in rep["checks"] if c["status"] == status]


class TestUs054Plan(Us054Case):
    def test_plan_is_valid_and_prints(self):
        self.assertEqual(kit.plan_problems(us054.Us054Run), [])
        self.assertIs(us054.Run, us054.Us054Run)
        self.assertEqual([s.check for s in us054.PLAN], PLAN_ORDER)
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        try:
            self.assertEqual(us054.main(["--plan"]), 0)
        finally:
            kit.set_log_stream(previous)
        text = out.getvalue()
        self.assertIn("| 1 | `hud_visible` |", text)
        self.assertIn("| 9 | `enemy_damages_player` |", text)
        self.assertNotIn("known issue", text)

    def test_proves_known_issues_and_declarations(self):
        plan = {s.check: s for s in us054.PLAN}
        for name, proves in PROVES.items():
            self.assertEqual(plan[name].proves, proves, name)
            # No DevTestMap check carries a known_issue (death_screen's N2 moved with it).
            self.assertIsNone(plan[name].known_issue, name)
        self.assertEqual(set(us054.Us054Run.COVERS),
                         {"US-050", "US-051", "US-052", "US-063", "US-064"})
        self.assertEqual(us054.Us054Run.STORY, "US-054")
        self.assertEqual(us054.Us054Run.RUN_SUFFIX, "us054")
        self.assertEqual(us054.Us054Run.launch_map(), kit.DEFAULT_MAP)
        # Declared, not inherited (AC-100.4): the Frontliner half stays on DevTestMap even if
        # the kit default ever changes.
        self.assertEqual(us054.Us054Run.__dict__.get("MAP"), kit.DEFAULT_MAP)
        self.assertEqual(us054.Us054Run.expected_experience(), kit.DEFAULT_EXPERIENCE)
        # The obsolete "enemies never damaged the player" note is gone (fixed since c7e4b8d).
        self.assertFalse(any("never damaged" in k for k in us054.KNOWN_ISSUES))
        self.assertIn("us054_combat.py", us054.__doc__)


class TestUs054Helpers(unittest.TestCase):
    def test_hud_widgets_exclude_enemy_bars(self):
        st = {"widgets": [{"class": "WBP_EnemyHealthBar_C"},
                          {"class": "MordecaiHealthBarWidget"},
                          {"class": "MordecaiStaminaBarWidget", "visible": False}]}
        self.assertEqual([w["class"] for w in us054.hud_widgets(st, "HealthBar")],
                         ["MordecaiHealthBarWidget"])
        self.assertEqual(us054.hud_widgets(st, "StaminaBar"), [])
        self.assertEqual(us054.widgets({"widgets": {"valid": False}}), [])

    def test_kill_counts(self):
        st = {"widgets": [{"class": "A", "texts": ["Kills: 3", "10 / 10"]},
                          {"class": "B", "texts": ["Kills:12 ", "Kills: x"]}]}
        self.assertEqual(us054.kill_counts(st), [3, 12])

    def test_displacement_is_from_the_nearest_spawn(self):
        self.assertEqual(us054.displacement({"location": [600, 0, 100]}), 0.0)
        self.assertAlmostEqual(us054.displacement({"location": [-400, -380, 100]}), 120.0)
        self.assertAlmostEqual(us054.displacement({"location": [480, 0, 100]}), 120.0)


class TestUs054AgainstTheFake(Us054Case):
    def test_everything_passes_when_the_arena_works(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(self.statuses(rep, "FAIL"), [], self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(code, 0)
        checks = self.by_name(rep)
        for name in PLAN_ORDER:
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertEqual(checks["map"]["status"], "PASS", checks["map"]["detail"])
        self.assertIn(fb.DEFAULT_MAP, checks["map"]["detail"])
        self.assertEqual(rep["fixed_candidates"], [])
        self.assertEqual([s["label"] for s in rep["screenshots"]], ["start", "hit"])
        # The Frontliners really walked up and hit the player standing at the spawn.
        self.assertGreater(world.hits, 0)
        self.assertGreater(rep["max_enemy_displacement"], us054.DISPLACED_CM)
        # Attached run: diagnostic only, nothing accepted.
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        self.assertFalse(any(a["accepted"] for a in rep["acs"]))
        # Audit trail: only state reads, waits and screenshots; the player only stands.
        sent = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual(set(sent), INPUT_AND_READ_CMDS)
        self.assertFalse([r for r in server.received if r.get("cmd") in ("key", "click")])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("## Enemies", md)
        self.assertEqual(len(rep["enemies_start"]), 3)
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-054", "AC-054.13")]["status"], "PASS")
        self.assertEqual(acs[("US-051", "AC-051.5")]["status"], "PASS")
        self.assertNotIn(("US-053", "AC-053.1"), acs)

    def test_invisible_enemies_fail_enemies_visible_as_new(self):
        # The game before issue #19: the Frontliners had no mesh.  Now that is a regression.
        _, server = self.serve(visible_mesh=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["enemies_visible"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("enemies_visible", rep["new_failures"])
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-054", "AC-054.13")]["status"], "FAIL")

    def test_no_enemy_damage_fails_and_skips_the_hit_screenshot(self):
        _, server = self.serve(damage=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["enemy_damages_player"]["status"], "FAIL")
        self.assertIn("player.attributes.health", checks["enemy_damages_player"]["detail"])
        self.assertEqual(rep["new_failures"], ["enemy_damages_player"])
        self.assertEqual(checks["screenshot_hit"]["status"], "SKIP")
        self.assertIn("never hit", checks["screenshot_hit"]["detail"])

    def test_idle_enemies_fail_aggro_approach_and_indicators(self):
        _, server = self.serve(aggro=False, approach=False, indicators=False, damage=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["enemy_aggro"]["status"], "FAIL")
        self.assertIn("ai=Idle", checks["enemy_aggro"]["detail"])
        self.assertEqual(checks["enemy_approaches"]["status"], "FAIL")
        self.assertEqual(checks["enemy_indicators"]["status"], "FAIL")
        self.assertIn("0 EnemyIndicator widget(s)", checks["enemy_indicators"]["detail"])

    def test_broken_hud_and_enemies_are_detected(self):
        _, server = self.serve(hud_layout_ok=False, enemy_hp=100.0, enemy_bars=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["hud_visible"]["status"], "PASS")
        self.assertEqual(checks["hud_layout"]["status"], "FAIL")
        self.assertIn("not top-left", checks["hud_layout"]["detail"])
        self.assertEqual(checks["enemies_present"]["status"], "FAIL")
        self.assertIn("health 100.0/100.0", checks["enemies_present"]["detail"])
        self.assertEqual(checks["enemy_health_bars"]["status"], "FAIL")
        for name in ("hud_layout", "enemies_present", "enemy_health_bars"):
            self.assertIn(name, rep["new_failures"])

    def test_a_wrong_kill_count_fails_hud_visible(self):
        _, server = self.serve(kills_text=2)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["hud_visible"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("'Kills: N' texts [0, 2]", check["detail"])


if __name__ == "__main__":
    unittest.main()
