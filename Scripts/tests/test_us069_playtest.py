"""Tests for Scripts/playtests/us069_pause_menu.py against tests/fake_bridge.py.

These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Us069
"""

import importlib.util
import io
import json
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

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us069_pause_menu.py")
TOKEN = "us069-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "click", "screenshot", "wait_until", "wait", "quit"}
TAB_IDS = [t for t, _ in fb.PAUSE_TABS]


def load_playtest():
    spec = importlib.util.spec_from_file_location("us069_pause_menu", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us069 = load_playtest()


def read_jsonl(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


class PauseMenuWorld(fb.FakeWorld):
    """FakeWorld plus the pause menu's tab contents and its gamepad bindings.

    Defaults model today's game (ground truth of the Phase 3 audit): the Character tab's
    UMordecaiCharacterSheetWidget is blank (N4), Skills and Feats show the "Coming Soon"
    placeholder (N5), Settings shows the placeholder, LB/RB do nothing, B
    (Gamepad_FaceButton_Right) closes the menu (CommonUI DefaultBack), Start
    (Gamepad_Special_Right) toggles it, and I does not close it.
    """

    def __init__(self, *, character_sheet=False, skills_tab=False, feats_tab=False,
                 shoulder_switches=False, gamepad_back_closes=True, start_toggles=True,
                 menu_blocks_input=True, **kwargs):
        super().__init__(**kwargs)
        self.character_sheet = character_sheet
        self.skills_tab = skills_tab
        self.feats_tab = feats_tab
        self.shoulder_switches = shoulder_switches
        self.gamepad_back_closes = gamepad_back_closes
        self.start_toggles = start_toggles
        self.menu_blocks_input = menu_blocks_input

    @property
    def game_input_allowed(self):
        return self.ready and (not self.menu_open or not self.menu_blocks_input)

    def _on_key_pressed(self, key):
        if not self.ready:
            return
        if self.menu_open:
            if key == "Gamepad_FaceButton_Right" and self.gamepad_back_closes:
                self.close_menu()
                return
            if key == "Gamepad_Special_Right" and self.start_toggles:
                self.close_menu()
                return
            if key in ("Gamepad_LeftShoulder", "Gamepad_RightShoulder") and self.shoulder_switches:
                step = -1 if key == "Gamepad_LeftShoulder" else 1
                i = TAB_IDS.index(self.active_tab)
                self.active_tab = TAB_IDS[(i + step) % len(TAB_IDS)]
                return
            if not self.menu_blocks_input and key in ("Q", "E", "R", "One"):
                if self.spell_points >= fb.SPELL_COST:  # the input leaks to gameplay (a bug)
                    self.spell_points -= fb.SPELL_COST
                    self.casts.append(key)
                return
        elif key == "Gamepad_Special_Right" and self.start_toggles:
            self.open_menu(self.last_tab or "character")
            return
        super()._on_key_pressed(key)

    def _content(self):
        tab = self.active_tab
        placeholder = {"class": "MordecaiPauseMenuPlaceholderWidget", "name": "Placeholder_0",
                       "rect": [160, 150, 960, 520], "texts": ["Coming Soon"], "buttons": []}
        if tab == "character":
            texts = []
            if self.character_sheet:
                texts = ["Physical", "STR", "10", "+0.0%", "DEX", "10", "+0.0%", "END", "10",
                         "+0.0%", "Resilience", "CON", "10", "+0.0%", "RES", "10", "+0.0%",
                         "DIS", "10", "+0.0%", "Magical", "INT", "10", "+0.0%", "WIS", "10",
                         "+0.0%", "CHA", "10", "+0.0%", "Health", "100 / 100", "Stamina",
                         "100 / 100", "Spell Points",
                         f"{self.spell_points:.0f} / {self.max_spell_points:.0f}", "Posture",
                         "0 / 100", "Physical Damage: +0.0%"]
            return {"class": "MordecaiCharacterSheetWidget", "name": "CharacterSheet_0",
                    "rect": [160, 150, 960, 520], "texts": texts, "buttons": []}
        if tab == "skills" and self.skills_tab:
            return {"class": "MordecaiSkillTreeWidget", "name": "SkillTree_0",
                    "rect": [160, 150, 960, 520],
                    "texts": ["Weapon", "Defense & Armor", "Movement & Exploration",
                              "Utility & Interaction", "Magic", "Skill Points: 0", "Swords",
                              "0 / 20"], "buttons": []}
        if tab == "feats" and self.feats_tab:
            return {"class": "MordecaiFeatDisplayWidget", "name": "FeatDisplay_0",
                    "rect": [160, 150, 960, 520],
                    "texts": ["Feats: 0 unlocked - 0 Common, 0 Rare, 0 Legendary"],
                    "buttons": []}
        if tab in ("skills", "feats", "settings"):
            return placeholder
        return None

    def _s_widgets(self):
        out = super()._s_widgets()
        if self.menu_painted:
            content = self._content()
            if content is not None:
                out.append(content)
        return out


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

    def serve(self, faults=None, **world_kwargs):
        world_kwargs.setdefault("obstacles", fb.DEVTESTMAP_OBSTACLES)
        world = PauseMenuWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp, faults=faults).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server, *extra):
        code = us069.main(["--attach-port", str(server.port), "--token", TOKEN,
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


KNOWN_FAILS = ["character_tab_content", "skills_tab_content", "feats_tab_content", "i_closes"]


class TestUs069TodaysGame(PlaytestCase):
    def test_todays_game_fails_exactly_the_known_issues_and_the_shoulder_finding(self):
        world, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 1, self.log.getvalue()[-3000:])
        self.assertEqual(sorted(self.statuses(rep, "FAIL")),
                         sorted(KNOWN_FAILS + ["shoulder_switches_tab"]))
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        self.assertEqual(sorted(rep["known_failures"]), sorted(KNOWN_FAILS))
        self.assertEqual(rep["new_failures"], ["shoulder_switches_tab"])
        self.assertEqual(rep["undeclared_checks"], [])
        checks = self.by_name(rep)
        for name in ("hud_visible", "escape_opens", "menu_input_mode", "tabs_listed",
                     "game_input_blocked", "click_skills_tab", "click_feats_tab",
                     "settings_placeholder", "escape_closes", "reopen_restores_tab",
                     "gamepad_back_closes", "start_button_opens", "escape_closes_again",
                     "i_opens_inventory", "final_close"):
            self.assertEqual(checks[name]["status"], "PASS", f"{name}: {checks[name]['detail']}")
        self.assertIn("NEW finding", checks["shoulder_switches_tab"]["detail"])
        self.assertIn("AC-069.4", checks["shoulder_switches_tab"]["detail"])
        self.assertIn("Coming Soon", checks["skills_tab_content"]["detail"])
        self.assertIn("Coming Soon", checks["feats_tab_content"]["detail"])
        self.assertIn("missing attributes", checks["character_tab_content"]["detail"])
        self.assertIn("moved 0.0 cm", checks["game_input_blocked"]["detail"])
        # LB did nothing, so the menu reopened on the tab it was closed on: settings.
        self.assertEqual(rep["tab_before_close"], "settings")
        self.assertIn('"settings"', checks["reopen_restores_tab"]["detail"])
        self.assertFalse(world.menu_open, "final_close must leave the menu closed")
        self.assertEqual(world.casts, [])
        self.assertEqual([s["label"] for s in rep["screenshots"]],
                         ["menu_character", "menu_skills", "menu_feats", "menu_closed"])
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])

    def test_the_acs_in_the_report(self):
        _, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-069", "AC-069.4")]["status"], "FAIL")
        self.assertEqual(acs[("US-069", "AC-069.4")]["checks"],
                         ["click_skills_tab", "click_feats_tab", "shoulder_switches_tab"])
        for key in (("US-069", "AC-069.1"), ("US-069", "AC-069.2"), ("US-069", "AC-069.3"),
                    ("US-069", "AC-069.6"), ("US-069", "AC-069.7"), ("US-069", "AC-069.10"),
                    ("US-069", "AC-069.11")):
            self.assertEqual(acs[key]["status"], "PASS", key)
        for key in (("US-066", "AC-066.3"), ("US-067", "AC-067.4"), ("US-068", "AC-068.8")):
            self.assertEqual(acs[key]["status"], "FAIL", key)
            self.assertTrue(acs[key]["known_issue"], key)
        self.assertFalse([a for a in rep["acs"] if a["accepted"]],
                         "an attached run never accepts an AC")
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("| FAIL (NEW) |", md)
        self.assertIn("| FAIL (known) |", md)
        self.assertIn("## Pause menu tabs", md)

    def test_only_real_input_and_reads_are_sent(self):
        _, server = self.serve()
        self.play(server)
        sent = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertLessEqual(set(sent), INPUT_AND_READ_CMDS)
        keys = [r.get("key") for r in server.received if r.get("cmd") == "key"]
        for k in ("Escape", "W", "Q", "Gamepad_LeftShoulder", "Gamepad_FaceButton_Right",
                  "Gamepad_Special_Right", "I"):
            self.assertIn(k, keys)
        self.assertFalse([r for r in server.received if r.get("route") == "viewport"])
        self.assertFalse([r for r in server.received if r.get("unsafe")])
        self.assertEqual(len([r for r in server.received if r.get("cmd") == "click"]), 3)


class TestUs069FixedGame(PlaytestCase):
    def test_a_fixed_game_passes_and_flags_the_known_issues_as_fixed(self):
        world, server = self.serve(character_sheet=True, skills_tab=True, feats_tab=True,
                                   shoulder_switches=True, i_closes_menu=True)
        code, rep = self.play(server)
        self.assertEqual(code, 0, json.dumps([c for c in rep["checks"]
                                              if c["status"] in ("FAIL", "SKIP")]))
        self.assertEqual(self.statuses(rep, "FAIL"), [])
        self.assertEqual(self.statuses(rep, "SKIP"), ["final_close"])
        self.assertEqual(sorted(rep["fixed_candidates"]), sorted(KNOWN_FAILS))
        # LB went settings -> inventory; Escape closed on inventory and reopened there.
        self.assertEqual(rep["tab_before_close"], "inventory")
        self.assertEqual(self.by_name(rep)["reopen_restores_tab"]["status"], "PASS")
        self.assertFalse(world.menu_open)


class TestUs069Failures(PlaytestCase):
    def test_escape_regression_fails_close_and_skips_the_reopen(self):
        world, server = self.serve(escape_closes_menu=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["escape_closes"]["status"], "FAIL")
        self.assertEqual(checks["screenshot_menu_closed"]["status"], "SKIP")
        self.assertEqual(checks["reopen_restores_tab"]["status"], "SKIP")
        # B still closes the menu that Escape left open, and Start reopens it.
        self.assertEqual(checks["gamepad_back_closes"]["status"], "PASS")
        self.assertEqual(checks["start_button_opens"]["status"], "PASS")
        self.assertEqual(checks["escape_closes_again"]["status"], "FAIL")
        self.assertIn("escape_closes", rep["new_failures"])

    def test_game_input_leaking_through_the_menu_fails(self):
        world, server = self.serve(menu_blocks_input=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["game_input_blocked"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("spell_points 10.0 -> 8.0", check["detail"])
        self.assertEqual(world.casts, ["Q"])

    def test_a_dead_start_button_fails_and_skips_what_needs_the_menu(self):
        world, server = self.serve(start_toggles=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["start_button_opens"]["status"], "FAIL")
        self.assertEqual(checks["escape_closes_again"]["status"], "SKIP")
        self.assertEqual(checks["i_opens_inventory"]["status"], "PASS")

    def test_a_menu_that_does_not_open_skips_the_menu_steps(self):
        _, server = self.serve()
        with mock.patch.object(PauseMenuWorld, "open_menu", lambda self, tab="character": None):
            code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["escape_opens"]["status"], "FAIL")
        for name in ("menu_input_mode", "tabs_listed", "game_input_blocked",
                     "character_tab_content", "click_skills_tab", "skills_tab_content",
                     "settings_placeholder", "shoulder_switches_tab", "escape_closes",
                     "reopen_restores_tab", "screenshot_menu_character"):
            self.assertEqual(checks[name]["status"], "SKIP", name)

    def test_a_wrong_experience_blocks_the_rest(self):
        _, server = self.serve(experience="B_LyraDefaultExperience")
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["experience"])
        self.assertEqual(self.by_name(rep)["final_close"]["status"], "SKIP")

    def test_a_lost_connection_is_infra(self):
        _, server = self.serve(faults=fb.Faults(drop_on_cmd="click"))
        code, rep = self.play(server)
        self.assertEqual(code, 1)  # the known content FAILs before it are gameplay
        checks = self.by_name(rep)
        self.assertEqual(checks["click_skills_tab"]["status"], "FAIL")
        self.assertEqual(checks["click_skills_tab"]["kind"], "infra")
        self.assertIn("connection was lost", checks["escape_closes"]["detail"])


class TestUs069Plan(unittest.TestCase):
    STORIES = {"US-069": "US-069-pause-menu-framework.md",
               "US-066": "US-066-character-sheet-widget.md",
               "US-067": "US-067-skill-tree-display.md",
               "US-068": "US-068-feat-display.md"}

    def story_ac_ids(self, story):
        path = os.path.join(os.path.dirname(SCRIPTS), "stories", "done", self.STORIES[story])
        if not os.path.isfile(path):
            self.skipTest(f"{path} moved")
        with open(path, encoding="utf-8") as f:
            return set(re.findall(r"^- \[[ xX]\] \**(AC[-\w.]*\w)", f.read(), re.M))

    def test_the_plan_is_valid_and_proves_real_acs(self):
        self.assertEqual(kit.plan_problems(us069.Us069Run), [])
        self.assertIs(us069.Run, us069.Us069Run)
        self.assertEqual(us069.Us069Run.STORY, "US-069")
        self.assertEqual(us069.Us069Run.COVERS, ("US-066", "US-067", "US-068"))
        for step in us069.PLAN:
            for ac in step.proves:
                story, ac_id = kit.split_ac(ac, "US-069")
                self.assertIn(ac_id, self.story_ac_ids(story), f"{step.check}: {ac}")
        names = [s.check for s in us069.PLAN]
        self.assertEqual(len(names), 20)
        known = {s.check for s in us069.PLAN if s.known_issue}
        self.assertEqual(known, set(KNOWN_FAILS))
        self.assertIsNone(us069.Us069Run(us069.parse_args([])).plan_step(
            "shoulder_switches_tab").known_issue)

    def test_plan_prints_the_steps_table(self):
        out = io.StringIO()
        previous = kit.set_log_stream(out)
        self.addCleanup(kit.set_log_stream, previous)
        self.assertEqual(us069.main(["--plan"]), 0)
        rows = [line for line in out.getvalue().splitlines() if line.startswith("| ")]
        self.assertEqual(len(rows), 1 + len(us069.PLAN))
        self.assertTrue(any(r.startswith("| 14 | `reopen_restores_tab` |")
                            and r.endswith("| AC-069.11 |") for r in rows))

    def test_expected_tabs_match_the_fake_and_the_story(self):
        self.assertEqual(tuple(us069.EXPECTED_TABS), tuple(fb.PAUSE_TABS))


if __name__ == "__main__":
    unittest.main()
