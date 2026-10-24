"""Tests for Scripts/playtests/us078_weapons.py against tests/fake_bridge.py.

A FakeWorld subclass stands in for the weapon arena: five weapons in the game's order, Tab /
Gamepad_FaceButton_Top next, BackSlash previous, and switches for the known issues (Shift+Tab,
the weapon HUD) and for the attacks (bound since US-083: a short click is a light attack, a
hold of at least 0.4 s a heavy attack on release).  These prove the playtest's logic and reporting only; they say
nothing about the real game.  Nothing here launches the real game.
Run: py Scripts/run_python_tests.py -k Us078
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

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us078_weapons.py")
TOKEN = "us078-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "axis", "mouse", "click", "screenshot",
                       "wait_until", "wait"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us078_weapons", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us078 = load_playtest()

GAME_WEAPONS = (("DA_Weapon_Longsword", "Longsword", "Longsword"),
                ("DA_Weapon_Dagger", "Dagger", "Dagger"),
                ("DA_Weapon_Greatsword", "Greatsword", "Greatsword"),
                ("DA_Weapon_Spear", "Spear", "Spear"),
                ("DA_Weapon_Mace", "Mace", "Mace"))
KNOWN_FAILS = ["weapon_hud_text", "cycle_prev_shift_tab"]
ATTACK_CHECKS = ["light_attack_lmb", "heavy_attack_hold"]
HEAVY_HOLD_S = 0.4   # UMordecaiHeroComponent::HeavyAttackHoldSeconds


class WeaponWorld(fb.FakeWorld):
    """The fake DevTestMap with the US-078 weapon list.

    shift_tab_prev: Shift+Tab cycles back (False = today's N9: Tab still fires Next).
    hud_shows_weapon: a visible widget shows the weapon name (False = today's N3).
    attacks_bound: a Left click starts an attack on its release, Mordecai.State.Attacking for
    attack_s; a press held HEAVY_HOLD_S or longer is a heavy attack, which also owns
    Mordecai.Attack.Heavy (False = the game before US-083: attacks are not bound).
    heavy_as_light: a held press starts a light attack (a heavy input routed to the light one).
    next_works: Tab / Y cycle at all.  weapons: [(id, display_name, type)];
    stuck_attacking: the tag never clears.
    """

    def __init__(self, *, shift_tab_prev=True, hud_shows_weapon=True, attacks_bound=True,
                 next_works=True, weapons=GAME_WEAPONS, attack_s=0.6, stuck_attacking=False,
                 heavy_as_light=False, **kwargs):
        super().__init__(**kwargs)
        self.shift_tab_prev = shift_tab_prev
        self.hud_shows_weapon = hud_shows_weapon
        self.attacks_bound = attacks_bound
        self.next_works = next_works
        self.weapons = tuple(weapons)
        self.attack_s = attack_s
        self.stuck_attacking = stuck_attacking
        self.heavy_as_light = heavy_as_light
        self.attack_until = None
        self.attack_heavy = False
        self.left_down_at = None
        self.attacks = []

    def _cycle(self, delta):
        self.weapon_index = (self.weapon_index + delta) % len(self.weapons)

    def _on_key_pressed(self, key):
        if self.ready and not self.menu_open and not self.is_dead:
            if key == "Tab":
                if self.shift_tab_prev and "LeftShift" in self._held_union():
                    self._cycle(-1)
                elif self.next_works:
                    self._cycle(+1)
                return
            if key == "Gamepad_FaceButton_Top":
                if self.next_works:
                    self._cycle(+1)
                return
            if key == "BackSlash":
                self._cycle(-1)
                return
        super()._on_key_pressed(key)

    def mouse_down(self, conn_id, x, y, button):
        with self.lock:
            hit = super().mouse_down(conn_id, x, y, button)
            if button == "Left":
                self.left_down_at = self.real_time()
            return hit

    def mouse_up(self, conn_id, x, y, button):
        with self.lock:
            hit = super().mouse_up(conn_id, x, y, button)
            if self.attacks_bound and button == "Left" and hit == "SViewport":
                held = self.real_time() - (self.left_down_at or self.real_time())
                self.attack_heavy = held >= HEAVY_HOLD_S and not self.heavy_as_light
                self.attack_until = self.real_time() + self.attack_s
                self.attacks.append(("heavy" if self.attack_heavy else "light",
                                     round(self.real_time(), 2)))
            return hit

    def attacking(self):
        return self.stuck_attacking or (self.attack_until is not None
                                        and self.real_time() < self.attack_until)

    def _s_player(self):
        # The base fake knows two weapons; let it build the section, then replace the weapon.
        index = self.weapon_index
        self.weapon_index = 0
        try:
            out = super()._s_player()
        finally:
            self.weapon_index = index
        if out.get("valid"):
            wid, name, wtype = self.weapons[self.weapon_index]
            out["weapon"] = {"id": wid, "display_name": name, "type": wtype,
                             "index": self.weapon_index, "count": len(self.weapons)}
            if self.attacking():
                out["tags"] = list(out["tags"]) + ["Mordecai.State.Attacking"]
                if self.attack_heavy and not self.stuck_attacking:
                    out["tags"].append("Mordecai.Attack.Heavy")
        return out

    def _s_widgets(self):
        out = super()._s_widgets()
        if self.hud_shows_weapon and self.has_pawn:
            _, name, wtype = self.weapons[self.weapon_index]
            out.append({"class": "MordecaiEquippedWeaponWidget", "name": "WeaponDisplay",
                        "rect": [1000, 640, 240, 60], "texts": [name, wtype], "buttons": []})
        return out


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class Us078Case(unittest.TestCase):
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
        world = WeaponWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        code = us078.main(["--attach-port", str(server.port), "--token", TOKEN,
                           "--run-dir", self.run_dir])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as f:
            return code, json.load(f)

    def report_md(self):
        with open(os.path.join(self.run_dir, "report.md"), encoding="utf-8") as f:
            return f.read()

    def sent(self):
        return [e["msg"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]

    @staticmethod
    def by_name(rep):
        return {c["name"]: c for c in rep["checks"]}

    @staticmethod
    def statuses(rep, status):
        return [c["name"] for c in rep["checks"] if c["status"] == status]

    @staticmethod
    def acs(rep):
        return {kit.qualified_ac(a["story"], a["id"], "US-078"): a for a in rep["acs"]}


class TestUs078Pass(Us078Case):
    def test_passes_when_every_weapon_feature_works(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        self.assertEqual(self.statuses(rep, "FAIL"), [])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(rep["undeclared_checks"], [])
        checks = self.by_name(rep)
        for step in us078.PLAN:
            self.assertEqual(checks[step.check]["status"], "PASS",
                             f"{step.check}: {checks[step.check]['detail']}")
        self.assertIn("index 0/5 'Longsword'", checks["starting_weapon"]["detail"])
        self.assertIn("'Longsword' is on screen in MordecaiEquippedWeaponWidget",
                      checks["weapon_hud_text"]["detail"])
        for k, name in enumerate(us078.WEAPONS[1:], 1):
            self.assertIn(f"-> {k} '{name}'", checks[f"cycle_next_{k}"]["detail"])
        self.assertIn("tapped Tab on 4 'Mace' -> 0 'Longsword'", checks["cycle_wrap"]["detail"])
        self.assertIn("tapped BackSlash on 0 'Longsword' -> 4 'Mace'",
                      checks["cycle_prev_backslash"]["detail"])
        self.assertIn("tapped LeftShift+Tab on 4 'Mace' -> 3 'Spear'",
                      checks["cycle_prev_shift_tab"]["detail"])
        self.assertIn("tapped Gamepad_FaceButton_Top on 3 'Spear' -> 4 'Mace'",
                      checks["cycle_gamepad_y"]["detail"])
        self.assertIn("-> 0 'Longsword'", checks["weapon_hud_updates"]["detail"])
        self.assertIn("the HUD shows 'Longsword'", checks["weapon_hud_updates"]["detail"])
        self.assertIn("Slate hit SViewport", checks["light_attack_lmb"]["detail"])
        self.assertIn("Mordecai.State.Attacking without Mordecai.Attack.Heavy after",
                      checks["light_attack_lmb"]["detail"])
        self.assertIn("held 0.8s", checks["heavy_attack_hold"]["detail"])
        self.assertIn("Mordecai.Attack.Heavy after", checks["heavy_attack_hold"]["detail"])
        self.assertEqual([kind for kind, _ in world.attacks], ["light", "heavy"])
        # Every known issue passed here: each is flagged as a fixed candidate.
        self.assertEqual(sorted(rep["fixed_candidates"]),
                         sorted(KNOWN_FAILS + ["weapon_hud_updates"]))
        # Screenshots: start and cycled_mace.
        self.assertEqual([s["label"] for s in rep["screenshots"]], ["start", "cycled_mace"])
        # Every AC passed but nothing is accepted on an attached (diagnostic) run.
        acs = self.acs(rep)
        for ac_id, a in acs.items():
            if ac_id == "AC-078.15":
                self.assertEqual(a["status"], "PENDING")
            else:
                self.assertEqual(a["status"], "PASS", ac_id)
            self.assertFalse(a["accepted"], ac_id)
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])

    def test_real_input_only(self):
        _, server = self.serve()
        code, _ = self.play(server)
        self.assertEqual(code, 0)
        sent = self.sent()
        self.assertLessEqual({m["cmd"] for m in sent}, INPUT_AND_READ_CMDS)
        self.assertFalse([m for m in sent if m.get("route") == "viewport" or m.get("unsafe")
                          or str(m.get("cmd", "")).startswith("debug")])
        keys = [m.get("keys") or m.get("key") for m in sent if m["cmd"] == "key"]
        self.assertEqual(keys, ["Tab", "Tab", "Tab", "Tab", "Tab", "BackSlash",
                                ["LeftShift", "Tab"], "Gamepad_FaceButton_Top", "Tab"])
        clicks = [m for m in sent if m["cmd"] == "click"]
        self.assertEqual([(c["x"], c["y"], c["button"], c.get("hold_s")) for c in clicks],
                         [(640, 360, "Left", None), (640, 360, "Left", 0.8)])


class TestUs078Failures(Us078Case):
    def test_todays_known_issues_fail_as_known(self):
        _, server = self.serve(shift_tab_prev=False, hud_shows_weapon=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(sorted(self.statuses(rep, "FAIL")), sorted(KNOWN_FAILS))
        self.assertEqual(sorted(rep["known_failures"]), sorted(KNOWN_FAILS))
        self.assertEqual(rep["new_failures"], [])
        checks = self.by_name(rep)
        self.assertIn("no visible widget text equals the weapon name 'Longsword'",
                      checks["weapon_hud_text"]["detail"])
        # N9: Shift+Tab fires Next, so the Mace wraps to the Longsword.
        self.assertIn("expected index 3 'Spear'", checks["cycle_prev_shift_tab"]["detail"])
        self.assertIn("the weapon is index 0/5 'Longsword'",
                      checks["cycle_prev_shift_tab"]["detail"])
        # The gamepad check expects the next weapon from wherever it really is.
        self.assertEqual(checks["cycle_gamepad_y"]["status"], "PASS")
        self.assertIn("on 0 'Longsword' -> 1 'Dagger'", checks["cycle_gamepad_y"]["detail"])
        self.assertEqual(checks["weapon_hud_updates"]["status"], "SKIP")
        self.assertIn("weapon_hud_text failed", checks["weapon_hud_updates"]["detail"])
        for name in ATTACK_CHECKS:
            self.assertEqual(checks[name]["status"], "PASS", name)
        acs = self.acs(rep)
        self.assertEqual(acs["AC-078.7"]["status"], "FAIL")
        self.assertIn("N9", acs["AC-078.7"]["known_issue"])
        self.assertEqual(acs["AC-078.11"]["status"], "FAIL")
        self.assertEqual(acs["AC-078.13"]["status"], "PASS")
        self.assertEqual(acs["AC-078.14"]["status"], "NOT RUN")  # weapon_hud_updates is SKIPped
        self.assertEqual(acs["US-077:AC-077.8"]["status"], "NOT RUN")
        for ac_id in ("AC-078.6", "AC-078.8", "AC-078.9", "US-077:AC-077.3",
                      "US-077:AC-077.4", "US-077:AC-077.5"):
            self.assertEqual(acs[ac_id]["status"], "PASS", ac_id)
        md = self.report_md()
        self.assertIn("FAIL (known)", md)
        self.assertNotIn("FAIL (NEW)", md)

    def test_unbound_attacks_fail_new(self):
        # The game before US-083
        _, server = self.serve(shift_tab_prev=False, hud_shows_weapon=False, attacks_bound=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(sorted(rep["new_failures"]), sorted(ATTACK_CHECKS))
        checks = self.by_name(rep)
        self.assertIn("no Mordecai.State.Attacking without Mordecai.Attack.Heavy in player.tags "
                      "within 1s", checks["light_attack_lmb"]["detail"])
        self.assertIn("no Mordecai.Attack.Heavy in player.tags within 0.5s",
                      checks["heavy_attack_hold"]["detail"])
        self.assertEqual(self.acs(rep)["AC-078.13"]["status"], "FAIL")

    def test_a_hold_that_starts_a_light_attack_fails_the_heavy_check(self):
        # A heavy input routed to the light attack still shows Mordecai.State.Attacking; only
        # the heavy attack's own tag passes heavy_attack_hold.
        world, server = self.serve(heavy_as_light=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["light_attack_lmb"]["status"], "PASS")
        self.assertEqual(checks["heavy_attack_hold"]["status"], "FAIL")
        self.assertIn("no Mordecai.Attack.Heavy", checks["heavy_attack_hold"]["detail"])
        self.assertEqual(rep["new_failures"], ["heavy_attack_hold"])
        self.assertEqual([kind for kind, _ in world.attacks], ["light", "light"])

    def test_a_wrong_weapon_name_is_a_new_fail(self):
        weapons = list(GAME_WEAPONS)
        weapons[1] = ("DA_Weapon_Dagger", "Rusty Knife", "Dagger")
        _, server = self.serve(weapons=weapons)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(rep["new_failures"], ["cycle_next_1"])
        detail = self.by_name(rep)["cycle_next_1"]["detail"]
        self.assertIn("display_name is 'Rusty Knife', expected 'Dagger'", detail)
        self.assertEqual(self.by_name(rep)["cycle_next_2"]["status"], "PASS")

    def test_cycling_that_does_nothing_fails_and_skips_what_depends_on_it(self):
        _, server = self.serve(next_works=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for k in range(1, 5):
            self.assertEqual(checks[f"cycle_next_{k}"]["status"], "FAIL", k)
            self.assertIn("the weapon is index 0/5 'Longsword'",
                          checks[f"cycle_next_{k}"]["detail"])
        self.assertEqual(checks["screenshot_cycled_mace"]["status"], "SKIP")
        self.assertEqual(checks["cycle_wrap"]["status"], "SKIP")
        self.assertIn("needs the weapon at index 4", checks["cycle_wrap"]["detail"])
        self.assertEqual(checks["cycle_prev_backslash"]["status"], "PASS")
        self.assertEqual(checks["cycle_gamepad_y"]["status"], "FAIL")
        self.assertIn("cycle_next_1", rep["new_failures"])
        self.assertIn("cycle_gamepad_y", rep["new_failures"])

    def test_a_wrong_starting_weapon_list_fails(self):
        _, server = self.serve(weapons=GAME_WEAPONS[:2])
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["starting_weapon"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("index 0/2 'Longsword'", check["detail"])
        self.assertIn("starting_weapon", rep["new_failures"])

    def test_an_attack_tag_that_never_clears_cannot_pass_an_attack(self):
        world, server = self.serve(stuck_attacking=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for name in ("light_attack_lmb", "heavy_attack_hold"):
            self.assertEqual(checks[name]["status"], "FAIL", name)
            self.assertIn("already in player.tags", checks[name]["detail"])
        self.assertFalse([m for m in self.sent() if m["cmd"] == "click"])
        self.assertEqual(world.attacks, [])


class TestUs078Report(Us078Case):
    def test_report_fields(self):
        _, server = self.serve(shift_tab_prev=False, hud_shows_weapon=False,
                               attacks_bound=False)
        _, rep = self.play(server)
        self.assertEqual(rep["story"], "US-078")
        self.assertEqual(rep["covers"], ["US-077"])
        self.assertEqual(rep["features"], ["weapons", "weapon_hud", "melee_attack"])
        self.assertEqual(rep["title"], us078.TITLE)
        self.assertEqual([p["check"] for p in rep["plan"]], [s.check for s in us078.PLAN])
        self.assertEqual(len(rep["plan"]), 13)
        self.assertEqual(rep["pending"], us078.PENDING)
        self.assertEqual(rep["known_issues"], list(us078.KNOWN_ISSUES))
        self.assertEqual(rep["script"].replace("\\", "/"), "Scripts/playtests/us078_weapons.py")
        cycles = {c["check"]: c for c in rep["cycles"]}
        self.assertEqual(cycles["cycle_prev_shift_tab"]["keys"], "LeftShift+Tab")
        self.assertEqual((cycles["cycle_prev_shift_tab"]["from"],
                          cycles["cycle_prev_shift_tab"]["to"]), (4, 0))
        self.assertFalse(cycles["cycle_prev_shift_tab"]["passed"])
        self.assertEqual(cycles["cycle_next_4"]["name"], "Mace")
        ids = {kit.qualified_ac(a["story"], a["id"], "US-078") for a in rep["acs"]}
        self.assertEqual(ids, {"AC-078.6", "AC-078.7", "AC-078.8", "AC-078.9", "AC-078.10",
                               "AC-078.11", "AC-078.12", "AC-078.13", "AC-078.14", "AC-078.15",
                               "US-077:AC-077.1", "US-077:AC-077.3", "US-077:AC-077.4",
                               "US-077:AC-077.5", "US-077:AC-077.6", "US-077:AC-077.7",
                               "US-077:AC-077.8"})
        md = self.report_md()
        self.assertIn("## Weapon cycling", md)
        self.assertIn("| cycle_prev_shift_tab | LeftShift+Tab | 4 | 0 | Longsword | no |", md)
        self.assertIn("| AC-078.15 | - | PENDING (bridge cannot drive: player.weapon has no "
                      "equip slot", md)
        self.assertIn("Known issue on `cycle_prev_shift_tab`: N9", md)


class TestUs078Plan(unittest.TestCase):
    def test_the_plan_is_valid(self):
        self.assertEqual(kit.plan_problems(us078.Us078Run), [])
        self.assertIs(us078.Run, us078.Us078Run)
        self.assertEqual(us078.Us078Run.expected_experience(), kit.DEFAULT_EXPERIENCE)
        self.assertEqual(us078.Us078Run.launch_map(), kit.DEFAULT_MAP)

    def test_known_issues_cite_the_backlog(self):
        known = {s.check: s.known_issue for s in us078.PLAN if s.known_issue}
        self.assertEqual(sorted(known), sorted(KNOWN_FAILS + ["weapon_hud_updates"]))
        for check, issue in known.items():
            self.assertIn("Specs/agent_playtest_plan_v1.md", issue, check)
        self.assertTrue(known["weapon_hud_text"].startswith("N3 "))
        self.assertTrue(known["cycle_prev_shift_tab"].startswith("N9 "))
        for name in ATTACK_CHECKS:
            self.assertNotIn(name, known, "attacks are bound since US-083")

    def test_plan_prints_without_a_game(self):
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        self.addCleanup(kit.set_log_stream, previous)
        with mock.patch.object(ab.Bridge, "launch") as launch, \
                mock.patch.object(ab.Bridge, "attach") as attach, \
                mock.patch.object(pb, "gate") as gate:
            code = us078.main(["--plan"])
        self.assertEqual(code, 0)
        launch.assert_not_called()
        attach.assert_not_called()
        gate.assert_not_called()
        lines = out.getvalue().strip().splitlines()
        self.assertEqual(len(lines), 2 + 13)
        self.assertTrue(lines[2].startswith("| 1 | `starting_weapon` |"))
        self.assertIn("US-077:AC-077.5", lines[2])
        self.assertTrue(lines[-1].startswith("| 13 | `heavy_attack_hold` |"))


if __name__ == "__main__":
    unittest.main()
