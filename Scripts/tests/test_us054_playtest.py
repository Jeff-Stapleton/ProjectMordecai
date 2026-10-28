"""Tests for Scripts/playtests/us054_arena.py against tests/fake_bridge.py.

ArenaWorld (below) adds what the shared FakeWorld lacks: 200 HP Frontliners that aggro, step
toward the player and hit them, Burning procs, Restoration on One, HUD bar widgets with rects,
enemy indicators, an optional death screen, attacks/dodge tags and the arena reset on respawn.
These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Us054
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


class ArenaWorld(fb.FakeWorld):
    """The DevTestMap arena with fighting Frontliners.  Defaults model the game as it should be
    (every check passes); the options switch each behaviour off or break it."""

    def __init__(self, *, visible_mesh=True, enemy_hp=200.0, enemy_start_hp=None, aggro=True,
                 aggro_s=0.3, approach=True, lunge_cm=120.0, damage=True, hit_damage=5.0,
                 hit_interval=0.3, hit_range=200.0, burn_chance=1.0, burn_indicator=True,
                 restoration=True, attacks=True, attack_tag=True, click_damage=0.0,
                 death_screen=True, dead_moves=False, arena_reset=True, indicators=True,
                 enemy_bars=True, hud_layout_ok=True, kills_text=0, **kwargs):
        kwargs.setdefault("obstacles", fb.DEVTESTMAP_OBSTACLES)
        kwargs.setdefault("respawn_s", us054.RESPAWN_DELAY_S)  # the game's 3 s
        super().__init__(enemies=False, **kwargs)
        self.visible_mesh = visible_mesh
        self.enemy_hp = float(enemy_hp)
        # Current health at the start, when an enemy is already hurt (max stays enemy_hp)
        self.enemy_start_hp = float(enemy_hp if enemy_start_hp is None else enemy_start_hp)
        self.attack_tag, self.click_damage = attack_tag, float(click_damage)
        self.aggro, self.aggro_s = aggro, aggro_s
        self.approach, self.lunge_cm = approach, lunge_cm
        self.damage, self.hit_damage, self.hit_interval = damage, hit_damage, hit_interval
        self.hit_range = hit_range
        self.burn_chance, self.burn_indicator = burn_chance, burn_indicator
        self.restoration, self.attacks = restoration, attacks
        self.death_screen, self.dead_moves = death_screen, dead_moves
        self.arena_reset, self.indicators, self.enemy_bars = arena_reset, indicators, enemy_bars
        self.hud_layout_ok, self.kills_text = hud_layout_ok, kills_text
        self.timed_tags = {}
        self.aggroed = False
        self.burn_rolls = 0
        self.arena = [self._fresh_enemy(s) for s in ARENA_SPAWNS]

    def _fresh_enemy(self, spawn):
        return {"location": [spawn[0], spawn[1], 100.0], "health": self.enemy_start_hp,
                "ai_state": "Idle", "next_hit": 0.0}

    # --- simulation -------------------------------------------------------------------------
    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            now = self.real_time()
            self.timed_tags = {t: e for t, e in self.timed_tags.items() if e > now}
            if self.aggro and not self.aggroed and now >= self.aggro_s and self.ready:
                self.aggroed = True
                for e in self.arena:
                    e["ai_state"] = "Approach"
                    if self.approach:
                        dx, dy = self.pos[0] - e["location"][0], self.pos[1] - e["location"][1]
                        d = math.hypot(dx, dy) or 1.0
                        e["location"][0] += dx / d * self.lunge_cm
                        e["location"][1] += dy / d * self.lunge_cm
            if self.is_dead or not self.ready or self.menu_open:
                return
            if "Mordecai.Status.Restoration" in self.timed_tags:
                self.health = min(self.max_health, self.health + 8.0 * dt)
            if not self.damage:
                return
            for e in self.arena:
                d = math.hypot(self.pos[0] - e["location"][0], self.pos[1] - e["location"][1])
                if d <= self.hit_range and now >= e["next_hit"]:
                    e["next_hit"] = now + self.hit_interval
                    e["ai_state"] = "Attack"
                    self.health -= self.hit_damage
                    self.burn_rolls += 1
                    if self.burn_chance >= 1.0 or (self.burn_chance > 0
                                                   and self.burn_rolls % 3 == 0):
                        self.timed_tags["Mordecai.Status.Burning"] = now + 3.0
                    if self.health <= 0:
                        self.kill_player()
                        return

    def _move(self, dt):
        if self.dead_moves and self.is_dead:
            dead, self.dead_since = self.dead_since, None
            try:
                super()._move(dt)
            finally:
                self.dead_since = dead
            return
        super()._move(dt)

    def respawn(self):
        with self.lock:
            super().respawn()
            self.timed_tags = {}
            if self.arena_reset:
                self.arena = [self._fresh_enemy(s) for s in ARENA_SPAWNS]

    def _on_key_pressed(self, key):
        if self.ready and not self.menu_open and not self.is_dead:
            now = self.real_time()
            if key == "One":
                if self.restoration and self.spell_points >= 3:
                    self.spell_points -= 3
                    self.timed_tags["Mordecai.Status.Restoration"] = now + 5.0
                return
            if key == "LeftAlt":
                if self.attacks:
                    self.timed_tags["Mordecai.State.Dodging"] = now + 0.4
                return
        super()._on_key_pressed(key)

    def mouse_down(self, conn_id, x, y, button):
        hit = super().mouse_down(conn_id, x, y, button)
        with self.lock:
            if (button == "Left" and self.attacks and self.game_input_allowed
                    and not self.is_dead):
                if self.attack_tag:
                    self.timed_tags["Mordecai.State.Attacking"] = self.real_time() + 0.5
                if self.click_damage and self.arena:
                    px, py = self.pos
                    target = min(self.arena, key=lambda e: math.hypot(
                        e["location"][0] - px, e["location"][1] - py))
                    target["health"] -= self.click_damage
        return hit

    # --- state --------------------------------------------------------------------------------
    def _s_player(self):
        d = super()._s_player()
        if d.get("valid"):
            d["tags"] = list(d["tags"]) + sorted(self.timed_tags)
        return d

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
        if self.burn_indicator and "Mordecai.Status.Burning" in self.timed_tags:
            out.append({"class": "MordecaiStatusEffectBarWidget", "name": "Status",
                        "rect": [40, 104, 260, 20], "texts": ["Burning"], "buttons": []})
        if self.death_screen and self.is_dead:
            left = max(0, math.ceil(self.respawn_s - (self.real_time() - self.dead_since)))
            out.append({"class": "MordecaiDeathScreenWidget", "name": "Death",
                        "rect": [0, 0, 1280, 720],
                        "texts": ["You Died", f"Respawning in {left}", f"Kills: {self.kills}"],
                        "buttons": []})
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


def game_today(**kw):
    """The packaged game as the brief describes it: no death screen (known issue N2),
    everything else working (attacks and dodge since US-083, enemy bodies since issue #19)."""
    opts = dict(death_screen=False)
    opts.update(kw)
    return opts


def game_before_us083(**kw):
    """The game before US-083: LMB and LeftAlt did nothing."""
    return game_today(attacks=False, **kw)


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
                            ("DEATH_TIMEOUT_S", 20.0), ("BURNING_TIMEOUT_S", 2.0),
                            ("RESTORATION_TIMEOUT_S", 3.0), ("RESET_TIMEOUT_S", 0.5),
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

    def plan_names(self):
        return [s.check for s in us054.PLAN]


class TestUs054Plan(Us054Case):
    def test_plan_is_valid_and_prints(self):
        self.assertEqual(kit.plan_problems(us054.Us054Run), [])
        self.assertIs(us054.Run, us054.Us054Run)
        self.assertEqual(len(us054.PLAN), 21)
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        try:
            self.assertEqual(us054.main(["--plan"]), 0)
        finally:
            kit.set_log_stream(previous)
        text = out.getvalue()
        self.assertIn("| 1 | `hud_visible` |", text)
        self.assertIn("| 21 | `kill_counter_text_matches` |", text)
        self.assertIn("Known issue N2: the death screen is never created", text)

    def test_ac_ids_are_qualified_and_known_issues_are_set(self):
        plan = {s.check: s for s in us054.PLAN}
        self.assertEqual(plan["death_screen"].known_issue,
                         "Known issue N2: the death screen is never created")
        # Suspected NEW, unverified: no known_issue on the enemy-damage checks.  Attacks and
        # dodge are bound since US-083; the Frontliners have a body since issue #19.
        for name in ("enemy_approaches", "enemy_damages_player", "kill_counter_text_matches",
                     "melee_lmb", "dodge_lalt", "enemies_visible"):
            self.assertIsNone(plan[name].known_issue, name)
        for s in us054.PLAN:
            for ac in s.proves:
                self.assertTrue(ac.startswith("AC-054.") or ac.startswith("US-0"), ac)
        self.assertEqual(set(us054.Us054Run.COVERS),
                         {"US-050", "US-051", "US-052", "US-053", "US-057", "US-062", "US-063",
                          "US-064"})


class TestUs054Helpers(unittest.TestCase):
    def test_hud_widgets_exclude_enemy_bars(self):
        st = {"widgets": [{"class": "WBP_EnemyHealthBar_C"},
                          {"class": "MordecaiHealthBarWidget"},
                          {"class": "MordecaiStaminaBarWidget", "visible": False}]}
        self.assertEqual([w["class"] for w in us054.hud_widgets(st, "HealthBar")],
                         ["MordecaiHealthBarWidget"])
        self.assertEqual(us054.hud_widgets(st, "StaminaBar"), [])
        self.assertEqual(us054.widgets({"widgets": {"valid": False}}), [])

    def test_same_enemy_follows_the_named_enemy(self):
        def enemy(name, x, hp):
            return {"kind": "enemy", "name": name, "location": [x, 0, 100],
                    "health": {"current": hp, "max": 200.0}}
        # Frontliner_0 was nearest at the click; now a hurt Frontliner_1 is nearer.
        st = {"actors": [enemy("Frontliner_0", 300, 200.0), enemy("Frontliner_1", 50, 150.0)]}
        e = us054.same_enemy(st, "Frontliner_0", (0, 0))
        self.assertEqual(e["name"], "Frontliner_0")
        self.assertEqual(us054.enemy_health(e), (200.0, 200.0))
        self.assertIsNone(us054.same_enemy(st, "Frontliner_2", (0, 0)))
        self.assertEqual(us054.same_enemy(st, None, (0, 0))["name"], "Frontliner_1")

    def test_kill_counts(self):
        st = {"widgets": [{"class": "A", "texts": ["Kills: 3", "10 / 10"]},
                          {"class": "B", "texts": ["Kills:12 ", "Kills: x"]}]}
        self.assertEqual(us054.kill_counts(st), [3, 12])

    def test_reset_problems(self):
        def enemy(x, y, hp=200.0, dead=False):
            return {"kind": "enemy", "location": [x, y, 100], "health": {"current": hp,
                                                                         "max": 200.0},
                    "is_dead": dead}
        ok = {"actors": [enemy(600, 0), enemy(-400, 500), enemy(-380, -520)]}
        self.assertEqual(us054.reset_problems(ok), [])
        bad = {"actors": [enemy(600, 0), enemy(-400, 500), enemy(-200, -300, hp=150)]}
        problems = us054.reset_problems(bad)
        self.assertTrue(any("from the nearest spawn" in p for p in problems), problems)
        self.assertTrue(any("health 150" in p for p in problems), problems)
        dup = {"actors": [enemy(600, 0), enemy(610, 0), enemy(-400, 500)]}
        self.assertTrue(any("two enemies" in p for p in us054.reset_problems(dup)))
        self.assertTrue(any("2 enemies" in p for p in
                            us054.reset_problems({"actors": ok["actors"][:2]})))


class TestUs054AgainstTheFake(Us054Case):
    def test_everything_passes_when_the_arena_works(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(self.statuses(rep, "FAIL"), [], self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(code, 0)
        checks = self.by_name(rep)
        for name in self.plan_names():
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        # The known-issue checks that passed are flagged as possibly fixed.
        self.assertEqual(rep["fixed_candidates"], ["death_screen"])
        self.assertEqual([s["label"] for s in rep["screenshots"]],
                         ["start", "near_enemy", "hit", "dead", "respawned"])
        self.assertIn("the counter reads 'Kills: 0', player.kills=0",
                      checks["kill_counter_text_matches"]["detail"])
        self.assertIn("the run kills no enemy", checks["kill_counter_text_matches"]["detail"])
        # The world really went through the loop.
        self.assertEqual(world.spell_points, 7.0)
        self.assertFalse(world.is_dead)
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
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("## Enemies", md)
        self.assertEqual(len(rep["enemies_start"]), 3)

    def test_the_game_as_today_fails_only_the_known_issues(self):
        _, server = self.serve(**game_today())
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(rep["known_failures"], ["death_screen"])
        self.assertEqual(rep["new_failures"], [], self.log.getvalue()[-4000:])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-054", "AC-054.13")]["status"], "PASS")
        self.assertEqual(acs[("US-062", "AC-062.1")]["status"], "FAIL")
        self.assertEqual(acs[("US-053", "AC-053.4")]["status"], "PASS")
        # Even after the full 1.5 s death-screen wait there is time to hold W before respawning.
        self.assertEqual(self.by_name(rep)["dead_no_move"]["status"], "PASS")
        self.assertEqual(acs[("US-063", "AC-063.1")]["status"], "PASS")
        for name in ("melee_lmb", "dodge_lalt"):
            self.assertEqual(self.by_name(rep)[name]["status"], "PASS", name)
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("FAIL (known)", md)
        self.assertNotIn("FAIL (NEW)", md)
        self.assertIn("Block and parry", md)

    def test_invisible_enemies_fail_enemies_visible_as_new(self):
        # The game before issue #19: the Frontliners had no mesh.  Now that is a regression.
        _, server = self.serve(**game_today(visible_mesh=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["enemies_visible"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("enemies_visible", rep["new_failures"])
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-054", "AC-054.13")]["status"], "FAIL")

    def test_the_game_before_us083_fails_melee_and_dodge_as_new(self):
        _, server = self.serve(**game_before_us083())
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for name in ("melee_lmb", "dodge_lalt"):
            self.assertEqual(checks[name]["status"], "FAIL", name)
            self.assertIn(name, rep["new_failures"])
        self.assertIn("no 'Mordecai.State.Attacking' tag and no enemy damage",
                      checks["melee_lmb"]["detail"])
        self.assertIn("no Dodging tag and no displacement > 150 cm", checks["dodge_lalt"]["detail"])

    # --- melee_lmb judges damage against the health read before the click (US-083 AC-083.9) ---
    def test_melee_does_not_pass_on_enemies_below_200_health(self):
        # The 8ea853b sweep's false PASS: the cooked enemies had 100/100 health (N13, issue #18),
        # which the old check read as "below 200" without any attack happening.
        _, server = self.serve(**game_before_us083(enemy_hp=100.0))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["melee_lmb"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("nearest enemy health (100.0, 100.0)", check["detail"])
        self.assertIn("melee_lmb", rep["new_failures"])

    def test_melee_compares_with_the_health_before_the_click(self):
        # An enemy that is already hurt (150/200) and takes no damage from the click: the old
        # comparison with 200 would PASS it.
        _, server = self.serve(attacks=False, enemy_start_hp=150.0)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["melee_lmb"]
        self.assertEqual(check["status"], "FAIL", check["detail"])
        self.assertIn("nearest enemy health (150.0, 200.0)", check["detail"])

    def test_melee_damage_alone_passes(self):
        # No Attacking tag is seen, but the clicked enemy loses health: the attack landed.
        _, server = self.serve(attack_tag=False, click_damage=10.0)
        code, rep = self.play(server)
        check = self.by_name(rep)["melee_lmb"]
        self.assertEqual(check["status"], "PASS", check["detail"])
        self.assertNotIn("Mordecai.State.Attacking", check["detail"])

    def test_no_enemy_damage_skips_the_death_loop(self):
        _, server = self.serve(**game_today(damage=False))
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["enemy_damages_player"]["status"], "FAIL")
        self.assertIn("player.attributes.health", checks["enemy_damages_player"]["detail"])
        self.assertIn("enemy_damages_player", rep["new_failures"])
        for name in ("burning_on_hit", "restoration_heals", "player_dies"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("never damaged", checks[name]["detail"])
        for name in ("death_screen", "dead_no_move", "respawn"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("did not die", checks[name]["detail"])
        for name in ("arena_reset", "respawn_moves"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("did not respawn", checks[name]["detail"])
        for label in ("hit", "dead", "respawned"):
            self.assertEqual(checks[f"screenshot_{label}"]["status"], "SKIP", label)
        self.assertEqual(checks["kill_counter_text_matches"]["status"], "PASS")
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-053", "AC-053.1")]["status"], "NOT RUN")

    def test_idle_enemies_fail_aggro_and_approach_and_reset_is_a_warn(self):
        _, server = self.serve(aggro=False, approach=False, indicators=False, damage=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["enemy_aggro"]["status"], "FAIL")
        self.assertIn("ai=Idle", checks["enemy_aggro"]["detail"])
        self.assertEqual(checks["enemy_approaches"]["status"], "FAIL")
        self.assertEqual(checks["enemy_indicators"]["status"], "FAIL")
        self.assertIn("0 EnemyIndicator widget(s)", checks["enemy_indicators"]["detail"])

    def test_broken_hud_enemies_and_death_are_detected(self):
        _, server = self.serve(hud_layout_ok=False, enemy_hp=100.0, enemy_bars=False,
                               dead_moves=True, arena_reset=False, burn_indicator=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["hud_visible"]["status"], "PASS")
        self.assertEqual(checks["hud_layout"]["status"], "FAIL")
        self.assertIn("not top-left", checks["hud_layout"]["detail"])
        self.assertEqual(checks["enemies_present"]["status"], "FAIL")
        self.assertIn("health 100.0/100.0", checks["enemies_present"]["detail"])
        self.assertEqual(checks["enemy_health_bars"]["status"], "FAIL")
        self.assertEqual(checks["burning_on_hit"]["status"], "FAIL")
        self.assertIn("no widget text 'Burning'", checks["burning_on_hit"]["detail"])
        self.assertEqual(checks["dead_no_move"]["status"], "FAIL",
                         checks["dead_no_move"]["detail"])
        self.assertEqual(checks["respawn"]["status"], "PASS")
        self.assertEqual(checks["arena_reset"]["status"], "FAIL")
        self.assertIn("not reset", checks["arena_reset"]["detail"])
        for name in ("hud_layout", "enemies_present", "enemy_health_bars", "burning_on_hit",
                     "dead_no_move", "arena_reset"):
            self.assertIn(name, rep["new_failures"])

    def test_burning_that_never_procs_is_a_warn_and_a_wrong_kill_count_fails(self):
        _, server = self.serve(burn_chance=0.0, kills_text=2)
        code, rep = self.play(server)
        checks = self.by_name(rep)
        self.assertEqual(checks["burning_on_hit"]["status"], "WARN")
        self.assertIn("never procced", checks["burning_on_hit"]["detail"])
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-057", "AC-057.11")]["status"], "NOT RUN")
        self.assertEqual(checks["kill_counter_text_matches"]["status"], "FAIL")
        self.assertEqual(checks["hud_visible"]["status"], "FAIL")
        self.assertEqual(code, 1)

    def test_dead_no_move_never_holds_w_into_the_respawn(self):
        # Too little time left before the respawn: WARN, and the respawn check is not spoiled.
        with mock.patch.object(us054, "RESPAWN_DELAY_S", 0.5):
            _, server = self.serve(respawn_s=1.0)
            code, rep = self.play(server)
        checks = self.by_name(rep)
        self.assertEqual(checks["dead_no_move"]["status"], "WARN")
        self.assertIn("no time to hold W", checks["dead_no_move"]["detail"])
        self.assertEqual(checks["respawn"]["status"], "PASS", checks["respawn"]["detail"])
        held = [r for r in server.received if r.get("cmd") == "key" and r.get("key") == "W"
                and r.get("hold_s") == us054.DEAD_HOLD_S]
        self.assertEqual(held, [])

    def test_restoration_that_does_nothing_fails(self):
        _, server = self.serve(restoration=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        detail = self.by_name(rep)["restoration_heals"]["detail"]
        self.assertEqual(self.by_name(rep)["restoration_heals"]["status"], "FAIL", detail)
        self.assertIn("'Mordecai.Status.Restoration' seen: False", detail)


if __name__ == "__main__":
    unittest.main()
