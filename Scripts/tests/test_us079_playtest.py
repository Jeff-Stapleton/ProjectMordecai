"""Tests for Scripts/playtests/us079_inventory.py against tests/fake_bridge.py and fake_game.py.

These prove the playtest's logic and reporting only; they say nothing about the real game.
Nothing here launches the real game.  Run: py Scripts/run_python_tests.py -k Us079
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
from tests import fake_bridge as fb  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us079_inventory.py")
FAKE_GAME = os.path.join(SCRIPTS, "tests", "fake_game.py")
TOKEN = "us079-token"
INPUT_AND_READ_CMDS = {"ping", "state", "key", "click", "screenshot", "wait_until", "wait", "quit"}


def load_playtest():
    spec = importlib.util.spec_from_file_location("us079_inventory", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us079 = load_playtest()

# UMordecaiInventoryWidget::FilterMatchesType (MordecaiInventoryWidget.cpp).
FILTER_TYPES = {"All": None, "Weapons": {"Weapon"}, "Armor": {"Armor"}, "Trinkets": {"Trinket"},
                "Consumables": {"Consumable"}, "Materials": {"Material", "TownResource"},
                "Quest": {"QuestItem"}, "Magical": {"MagicalItem", "UpgradeKey"}}


def make_realistic_world(base):
    """A FakeWorld subclass whose inventory tab matches the C++ more closely than the shared fake.

    - rows are filtered by the active filter (FilterMatchesType); the shared fake ignores it.
    - list_placeholder / ledger_placeholder are the rendered string or null (the C++ contract,
      GetListPlaceholderText / GetLedgerPlaceholderText), not the shared fake's dict.
    Options (keyword, JSON-friendly for fake_game.py): escape_budget (None = Escape always closes
    the menu as configured; N = only the first N Escapes close it, then the fix "regresses"),
    filters_work (False = filter clicks change nothing on screen), labels_visible (False = the
    pickups' name labels are hidden), label_facing / label_upright (what the bridge measures
    for every pickup label; 0.542 / 0.0 is the old fixed rotation, issue #38; None = no camera),
    labels_reported (False = no actors[].text_labels, a bridge without the issue #30 fix).
    """

    class RealisticWorld(base):
        def __init__(self, *, escape_budget=None, filters_work=True, labels_visible=True,
                     label_facing=1.0, label_upright=1.0, labels_reported=True, **kw):
            super().__init__(**kw)
            self.escape_budget = escape_budget
            self.filters_work = bool(filters_work)
            self.labels_visible = bool(labels_visible)
            self.label_facing = label_facing
            self.label_upright = label_upright
            self.labels_reported = bool(labels_reported)

        def _on_key_pressed(self, key):
            if key == "Escape" and self.menu_open and self.escape_budget is not None:
                if self.escape_budget <= 0:
                    return  # the back handler stopped working
                self.escape_budget -= 1
            super()._on_key_pressed(key)

        def _visible(self, it):
            types = FILTER_TYPES.get(self.inventory_filter) if self.filters_work else None
            return types is None or it["type"] in types

        def _s_pause_menu(self):
            pm = super()._s_pause_menu()
            tab = pm.get("inventory_tab")
            if not isinstance(tab, dict):
                return pm
            visible_ids = {it["instance_id"] for it in self.sorted_items() if self._visible(it)}
            tab["rows"] = [r for r in tab["rows"] if r["instance_id"] in visible_ids]
            if not self.inventory and not self.ledger:
                tab["list_placeholder"] = "Inventory is empty."
            elif self.inventory and not tab["rows"]:
                tab["list_placeholder"] = "No items match this filter."
            else:
                tab["list_placeholder"] = None
            tab["ledger_placeholder"] = None if self.ledger else "No resources stored."
            return pm

        def _actors(self, radius, include_all):
            entries = super()._actors(radius, include_all)
            for e in entries:
                if e["kind"] != "pickup":
                    continue
                if not self.labels_reported:
                    del e["text_labels"]
                    continue
                e["text_labels"] = [dict(lbl, visible=lbl["visible"] and self.labels_visible,
                                         facing=self.label_facing, upright=self.label_upright)
                                    for lbl in e["text_labels"]]
            return entries

    return RealisticWorld


RealisticWorld = make_realistic_world(fb.FakeWorld)

# fake_game.py builds fake_bridge.FakeWorld; this wrapper swaps in the realistic subclass.
FAKE_GAME_WRAPPER = """\
import sys
sys.path[0:0] = [{tests!r}, {scripts!r}]
import fake_bridge
from tests.test_us079_playtest import make_realistic_world
fake_bridge.FakeWorld = make_realistic_world(fake_bridge.FakeWorld)
import fake_game
sys.exit(fake_game.main())
"""


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
        for module in (us079, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        # Never read the real agent_bridge session file.
        patcher = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, faults=None, **world_kwargs):
        world_kwargs.setdefault("obstacles", fb.DEVTESTMAP_OBSTACLES)
        world = RealisticWorld(**world_kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp, faults=faults).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server, *extra):
        code = us079.main(["--attach-port", str(server.port), "--token", TOKEN,
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


class TestUs079AgainstTheFake(PlaytestCase):
    def test_passes_when_escape_closes_the_menu(self):
        world, server = self.serve(escape_closes_menu=True)
        code, rep = self.play(server)
        self.assertEqual(code, 0, self.log.getvalue()[-3000:])
        self.assertEqual(self.statuses(rep, "FAIL"), [])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        checks = self.by_name(rep)
        for name in ("attach", "experience", "hud_visible", "pickup_labels", "empty_state",
                     "pickup_loop_time", "inventory_count", "inventory_items",
                     "ledger_count", "ledger_entries", "menu_opens_inventory", "menu_rows",
                     "unidentified_badge", "menu_ledger_rows", "identify_button",
                     "filter_weapons", "filter_armor_empty", "filter_all", "identify_click",
                     "amulet_identified", "menu_closes_escape", "no_crash"):
            self.assertEqual(checks[name]["status"], "PASS", name)
        self.assertIn("'Inventory is empty.' shown", checks["empty_state"]["detail"])
        self.assertIn("the menu closed", checks["empty_state"]["detail"])
        self.assertIn("6 pickup name label(s) visible, each reading its display name square to "
                      "the camera", checks["pickup_labels"]["detail"])
        self.assertIn("TestPickup_MysteriousAmulet='Unidentified Purple Amulet' (facing 1.000, "
                      "upright 1.000)", checks["pickup_labels"]["detail"])
        self.assertIn("to the confirmed 6th pickup (limit 30s", checks["pickup_loop_time"]["detail"])
        self.assertIn("'? Unidentified Purple Amulet'", checks["unidentified_badge"]["detail"])
        self.assertIn("clicked 'Weapons'", checks["filter_weapons"]["detail"])
        self.assertIn("clicked 'Armor'", checks["filter_armor_empty"]["detail"])
        self.assertIn("clicked 'All'", checks["filter_all"]["detail"])
        # The checks run in the player's order: the empty menu first, the filters before Identify.
        order = [c["name"] for c in rep["checks"]]
        self.assertLess(order.index("empty_state"), order.index("walk_HealthPotion"))
        self.assertLess(order.index("pickup_RustySword"), order.index("pickup_loop_time"))
        self.assertLess(order.index("menu_rows"), order.index("unidentified_badge"))
        self.assertLess(order.index("filter_all"), order.index("identify_click"))
        self.assertTrue(all(p["picked_s"] is not None for p in rep["pickups"]))
        self.assertEqual(world.inventory_filter, "All")
        for stop in us079.ROUTE:
            for prefix in ("walk", "focus", "prompt", "pickup"):
                self.assertEqual(checks[f"{prefix}_{stop['name']}"]["status"], "PASS",
                                 f"{prefix}_{stop['name']}")
        self.assertEqual(checks["walk_RustySword_waypoint1"]["status"], "PASS")
        # Each pickup is confirmed by what was stored, and both counts are in the detail.
        self.assertIn("inventory.count 0 -> 1, inventory.ledger_count 0 -> 0",
                      checks["pickup_HealthPotion"]["detail"])
        self.assertIn("inventory.count 1 -> 1, inventory.ledger_count 0 -> 1",
                      checks["pickup_IronOre"]["detail"])
        # The game state the fake ended in: all six picked, the amulet identified, menu closed.
        self.assertEqual(sorted(world.picked), sorted(s["label"] for s in us079.ROUTE))
        self.assertEqual(len(world.inventory), 4)
        amulet = [i for i in world.inventory if i["item_id"] == us079.AMULET_ITEM_ID][0]
        self.assertTrue(amulet["identified"])
        self.assertFalse(world.menu_open)
        # Attached to a game it did not launch, and the fake is no fresh package: diagnostic.
        self.assertEqual(checks["build_fresh"]["status"], "WARN")
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        self.assertEqual(rep["result"], "PASS")

    def test_run_dir_has_the_report_screenshots_and_transcript(self):
        _, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 0)
        names = sorted(os.listdir(self.run_dir))
        # empty_state added the "empty" screenshot (bridge_02.png) before the pickups.
        for n in ("report.md", "report.json", "bridge.jsonl", "bridge_01.png", "bridge_02.png",
                  "bridge_03.png", "bridge_04.png", "bridge_05.png"):
            self.assertIn(n, names)
        self.assertEqual([s["label"] for s in rep["screenshots"]],
                         ["start", "empty", "menu", "identified", "after_escape"])
        for s in rep["screenshots"]:
            self.assertTrue(ab.file_complete(s["path"]), s["path"])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("Open every screenshot with the Read tool", md)
        self.assertIn(pb.AGENT_LINE, md)
        for s in rep["screenshots"]:
            self.assertIn(s["path"], md)
        self.assertIn("## Environment (state.focus)", md)
        self.assertIsInstance(rep["focus_start"], dict)
        self.assertIsInstance(rep["focus_end"], dict)
        self.assertIn("commonui_input_mode", rep["focus_start"])
        # The audit trail: only input commands, state reads, waits and screenshots.
        sent = [e["msg"]["cmd"] for e in read_jsonl(os.path.join(self.run_dir, "bridge.jsonl"))
                if e["dir"] == "send"]
        self.assertTrue(sent)
        self.assertLessEqual(set(sent), INPUT_AND_READ_CMDS)
        self.assertFalse([c for c in sent if c.startswith("debug_")])
        keys = [r.get("key") or r.get("keys") for r in server.received if r.get("cmd") == "key"]
        self.assertIn("F", keys)
        self.assertIn("I", keys)
        self.assertIn("Escape", keys)
        self.assertEqual(keys.count("I"), 2)       # empty_state, then step 4
        self.assertEqual(keys.count("Escape"), 2)  # empty_state, then step 6
        # The filters were clicked through Slate at their buttons, never set directly.
        clicks = [(r.get("x"), r.get("y")) for r in server.received if r.get("cmd") == "click"]
        self.assertEqual(len(clicks), 4)  # Weapons, Armor, All, Identify

    def test_escape_regression_fails_step_6_and_nothing_else(self):
        # The game's Escape fix (back handler) regressed after the first close: a FAIL that
        # names the regression.  (escape_budget=1: empty_state's Escape still closes the menu,
        # so step 6 is the one that meets the regression.)
        world, server = self.serve(escape_budget=1)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(self.statuses(rep, "FAIL"), ["menu_closes_escape"])
        self.assertEqual(self.statuses(rep, "SKIP"), [])
        detail = self.by_name(rep)["menu_closes_escape"]["detail"]
        self.assertIn("still open", detail)
        self.assertIn("REGRESSION of the back-handler fix (AC-069.10), not a known issue", detail)
        self.assertIn("bIsBackHandler", detail)
        self.assertIn("Mordecai.UI.PauseMenu.*", detail)
        self.assertNotIn("Known issue", detail)
        self.assertTrue(world.menu_open)
        # The screenshot after Escape is still taken: it shows the menu stuck open.
        self.assertIn("after_escape", [s["label"] for s in rep["screenshots"]])
        self.assertEqual(rep["result"], "FAIL")

    def test_escape_regression_from_the_start_fails_empty_state_and_blocks_the_walks(self):
        # Escape never closes the menu: empty_state meets it first, names the regression, and
        # the walks are not attempted (the open menu blocks game input).
        world, server = self.serve(escape_closes_menu=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["empty_state"]["status"], "FAIL")
        detail = checks["empty_state"]["detail"]
        self.assertIn("still open", detail)
        self.assertIn("REGRESSION of the back-handler fix (AC-069.10), not a known issue", detail)
        self.assertIn("'Inventory is empty.' shown", detail)
        for stop in us079.ROUTE:
            self.assertEqual(checks[f"walk_{stop['name']}"]["status"], "SKIP")
            self.assertIn("pause menu stayed open", checks[f"walk_{stop['name']}"]["detail"])
            self.assertEqual(checks[f"pickup_{stop['name']}"]["status"], "SKIP")
        self.assertEqual(checks["pickup_loop_time"]["status"], "SKIP")
        self.assertEqual(checks["menu_closes_escape"]["status"], "FAIL")
        self.assertEqual(world.picked, [])
        self.assertTrue(world.menu_open)

    def test_the_empty_state_never_taps_escape_on_a_closed_menu(self):
        # I does nothing before the first pickup (the menu never opens): empty_state FAILs and
        # does NOT tap Escape, which would open the pause menu on a closed one.
        world, server = self.serve()
        original = world._on_key_pressed
        presses = {"I": 0}

        def first_i_ignored(key):
            if key == "I" and not world.menu_open:
                presses["I"] += 1
                if presses["I"] == 1:
                    return
            original(key)

        world._on_key_pressed = first_i_ignored
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["empty_state"]["status"], "FAIL")
        self.assertIn("pause_menu.open=False", checks["empty_state"]["detail"])
        self.assertIn("Escape not tapped", checks["empty_state"]["detail"])
        self.assertNotIn("empty", [s["label"] for s in rep["screenshots"]])
        keys = [r.get("key") or r.get("keys") for r in server.received if r.get("cmd") == "key"]
        self.assertEqual(keys.index("Escape"), len(keys) - 1, "only step 6 taps Escape")
        self.assertEqual(self.statuses(rep, "FAIL"), ["empty_state"])

    def test_filters_that_do_not_filter_fail(self):
        # The filter buttons are clicked but the list does not change: Weapons and Armor FAIL
        # with the rows the player still sees; All (4 rows) and Identify still pass.
        world, server = self.serve(filters_work=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(self.statuses(rep, "FAIL"), ["filter_weapons", "filter_armor_empty"])
        self.assertIn("clicked 'Weapons'", checks["filter_weapons"]["detail"])
        self.assertIn("was not true within 3s", checks["filter_weapons"]["detail"])
        self.assertIn('"len(pause_menu.inventory_tab.rows)":4', checks["filter_weapons"]["detail"])
        self.assertIn("clicked 'Armor'", checks["filter_armor_empty"]["detail"])
        self.assertIn('"pause_menu.inventory_tab.list_placeholder":null',
                      checks["filter_armor_empty"]["detail"])
        for name in ("filter_all", "identify_click", "amulet_identified", "menu_closes_escape"):
            self.assertEqual(checks[name]["status"], "PASS", name)
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-071", "AC-071.8")]["status"], "FAIL")
        self.assertEqual(acs[("US-071", "AC-071.15")]["status"], "FAIL")
        self.assertEqual(acs[("US-079", "AC-079.11")]["status"], "FAIL")

    def test_a_filter_button_that_is_not_on_screen_fails(self):
        # No Armor button (the filter bar lost it): a gameplay FAIL naming the visible buttons.
        world, server = self.serve()
        original = world.buttons
        world.buttons = lambda: [b for b in original() if b["id"] != "armor"]
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["filter_armor_empty"]
        self.assertEqual(check["status"], "FAIL")
        self.assertEqual(check["kind"], "gameplay")
        self.assertIn("no visible button with id 'armor'", check["detail"])
        self.assertEqual(self.statuses(rep, "FAIL"), ["filter_armor_empty"])

    def test_hidden_pickup_labels_fail(self):
        world, server = self.serve(labels_visible=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["pickup_labels"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("6 of 6 pickup name label(s) are not readable", check["detail"])
        self.assertIn("TestPickup_HealthPotion: its label 'Health Potion' (NameLabel) is hidden",
                      check["detail"])
        self.assertEqual(self.statuses(rep, "FAIL"), ["pickup_labels"])

    def test_labels_turned_90_degrees_fail_as_new(self):
        # Issue #38: the old fixed rotation (45, 90, 0) under the diorama camera.
        world, server = self.serve(label_facing=0.542, label_upright=0.0)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["pickup_labels"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("6 of 6 pickup name label(s) are not readable", check["detail"])
        self.assertIn("TestPickup_IronOre: its label 'Iron Ore': facing 0.542 < 0.99 (not square "
                      "to the camera), upright 0.000 < 0.99 (not reading left to right)",
                      check["detail"])
        self.assertEqual(self.statuses(rep, "FAIL"), ["pickup_labels"])
        self.assertEqual(rep["new_failures"], ["pickup_labels"])
        self.assertEqual(rep["known_failures"], [])
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-079", "AC-079.10")]["status"], "FAIL")

    def test_a_bridge_without_text_labels_fails(self):
        world, server = self.serve(labels_reported=False)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["pickup_labels"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("the bridge reports no actors[].text_labels", check["detail"])
        self.assertEqual(self.statuses(rep, "FAIL"), ["pickup_labels"])

    def test_a_slow_pickup_loop_fails(self):
        with mock.patch.object(us079, "PICKUP_LOOP_LIMIT_S", 0.01):
            _, server = self.serve()
            code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["pickup_loop_time"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("(limit 0.01s", check["detail"])
        self.assertEqual(self.statuses(rep, "FAIL"), ["pickup_loop_time"])

    def test_known_issues_and_the_after_escape_expectation_match_the_fixed_game(self):
        _, server = self.serve()
        code, rep = self.play(server)
        self.assertEqual(code, 0)
        self.assertEqual(rep["known_issues"], [
            "I does not close the pause menu (no UI action binding); Escape closes it via the "
            "CommonUI back handler."])
        after = [s for s in rep["screenshots"] if s["label"] == "after_escape"][0]
        self.assertEqual(after["expect"], "the arena with the HUD (pause menu closed)")
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertNotIn("not a back handler", md)
        self.assertNotIn("predicted", md.lower())

    def test_a_missing_pickup_fails(self):
        labels = [s["label"] for s in fb.PICKUP_SPECS if s["label"] != "TestPickup_RustySword"]
        world, server = self.serve(pickups=labels)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["walk_RustySword"]["status"], "PASS")
        self.assertEqual(checks["focus_RustySword"]["status"], "FAIL")
        self.assertEqual(checks["prompt_RustySword"]["status"], "SKIP")
        self.assertEqual(checks["pickup_RustySword"]["status"], "SKIP")
        self.assertEqual(checks["inventory_count"]["status"], "FAIL")
        self.assertIn("inventory.count=3", checks["inventory_count"]["detail"])
        self.assertEqual(checks["inventory_items"]["status"], "FAIL")
        self.assertIn("TestItem_RustySword missing", checks["inventory_items"]["detail"])
        self.assertEqual(checks["menu_rows"]["status"], "FAIL")
        # The loop time needs all six pickups; the filters see the missing sword.
        self.assertEqual(checks["pickup_loop_time"]["status"], "SKIP")
        self.assertIn("not every pickup was confirmed", checks["pickup_loop_time"]["detail"])
        self.assertEqual(checks["filter_weapons"]["status"], "FAIL")
        self.assertEqual(checks["filter_all"]["status"], "FAIL")
        # Independent steps still ran and passed.
        for name in ("pickup_labels", "empty_state", "ledger_entries", "unidentified_badge",
                     "identify_button", "filter_armor_empty", "amulet_identified",
                     "menu_closes_escape"):
            self.assertEqual(checks[name]["status"], "PASS", name)

    def fast_pickup_timeout(self):
        patcher = mock.patch.object(us079, "PICKUP_TIMEOUT_S", 0.5)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_an_invalid_pickups_section_after_f_is_a_fail_not_a_pickup(self):
        # The pawn is lost the frame F goes down: pickups (and inventory) turn valid:false and
        # have no 'world' list.  That is no evidence of a pickup (review finding 13).
        self.fast_pickup_timeout()
        world, server = self.serve(lose_pawn_on_f=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        check = self.by_name(rep)["pickup_HealthPotion"]
        self.assertEqual(check["status"], "FAIL")
        self.assertIn("pickups section invalid after F (no pawn yet)", check["detail"])
        self.assertIn("inventory.count 0 -> ?, inventory.ledger_count 0 -> ?", check["detail"])
        self.assertFalse(rep["pickups"][0]["picked"])
        self.assertEqual(world.picked, [])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        row = [line for line in md.splitlines() if line.startswith("| TestPickup_HealthPotion |")]
        self.assertTrue(row and row[0].rstrip().endswith("| no |"), row)

    def test_a_pickup_that_stores_nothing_fails(self):
        self.fast_pickup_timeout()
        world, server = self.serve(pickup_adds_nothing=True)
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        for stop in us079.ROUTE:
            check = checks[f"pickup_{stop['name']}"]
            self.assertEqual(check["status"], "FAIL", stop["name"])
            self.assertIn("left pickups.world but nothing was stored", check["detail"])
        self.assertIn("inventory.count 0 -> 0, inventory.ledger_count 0 -> 0",
                      checks["pickup_HealthPotion"]["detail"])
        self.assertEqual(sorted(world.picked), sorted(s["label"] for s in us079.ROUTE))

    def test_a_wrong_experience_blocks_the_rest(self):
        _, server = self.serve(experience="B_LyraDefaultExperience")
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        checks = self.by_name(rep)
        self.assertEqual(checks["experience"]["status"], "FAIL")
        self.assertIn("B_LyraDefaultExperience", checks["experience"]["detail"])
        self.assertEqual(checks["walk_HealthPotion"]["status"], "SKIP")
        self.assertEqual(checks["menu_closes_escape"]["status"], "SKIP")
        self.assertEqual(self.statuses(rep, "FAIL"), ["experience"])

    def test_a_lost_connection_is_infra_and_skips_the_rest(self):
        # The first click is now filter_weapons (the filters come before Identify).
        _, server = self.serve(faults=fb.Faults(drop_on_cmd="click"))
        code, rep = self.play(server)
        self.assertEqual(code, 2)
        checks = self.by_name(rep)
        self.assertEqual(checks["filter_weapons"]["status"], "FAIL")
        self.assertEqual(checks["filter_weapons"]["kind"], "infra")
        for name in ("filter_armor_empty", "filter_all", "identify_click", "amulet_identified",
                     "screenshot_identified", "menu_closes_escape", "screenshot_after_escape"):
            self.assertEqual(checks[name]["status"], "SKIP", name)
            self.assertIn("connection was lost", checks[name]["detail"])
        self.assertEqual(checks["menu_opens_inventory"]["status"], "PASS")

    def test_no_bridge_to_attach_is_infra(self):
        code = us079.main(["--attach-port", str(ab.free_port()), "--run-dir", self.run_dir])
        self.assertEqual(code, 2)
        rep = self.report()
        checks = self.by_name(rep)
        self.assertEqual(checks["attach"]["status"], "FAIL")
        self.assertEqual(checks["walk_HealthPotion"]["status"], "SKIP")
        self.assertEqual(self.statuses(rep, "FAIL"), ["attach"])

    def test_attach_snapshots_the_session_game_log(self):
        _, server = self.serve(experience="B_LyraDefaultExperience")  # a quick run
        session_run = os.path.join(self.tmp, "session_run")
        os.makedirs(session_run)
        with open(os.path.join(session_run, "game.log"), "w", encoding="utf-8") as f:
            f.write("LogTemp: Warning: SkipPackage: /MordecaiCore/Missing/Thing\n"
                    "LogTemp: Error: something broke\n")
        ab.write_session(ab.SESSION_FILE, {"pid": os.getpid(), "port": server.port,
                                           "run_dir": session_run})
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertTrue(os.path.isfile(os.path.join(self.run_dir, "game.log")))
        checks = self.by_name(rep)
        self.assertEqual(checks["content_cooked"]["status"], "FAIL")
        self.assertEqual(rep["missing_content"], ["/MordecaiCore/Missing/Thing"])
        self.assertIn("snapshot", rep["game_log_note"])


class TestUs079Launch(PlaytestCase):
    def fake_game_argv(self):
        """fake_game.py, run with the realistic inventory tab (make_realistic_world)."""
        wrapper = os.path.join(self.tmp, "fake_game_realistic.py")
        with open(wrapper, "w", encoding="utf-8") as f:
            f.write(FAKE_GAME_WRAPPER.format(tests=os.path.dirname(FAKE_GAME), scripts=SCRIPTS))
        return json.dumps([sys.executable, wrapper])

    def test_launch_mode_with_the_fake_game(self):
        world_json = os.path.join(self.tmp, "world.json")
        with open(world_json, "w", encoding="utf-8") as f:
            json.dump({"obstacles": [list(b) for b in fb.DEVTESTMAP_OBSTACLES]}, f)
        code = us079.main(["--test-exe-argv", self.fake_game_argv(),
                           "--test-skip-freshness", "--run-dir", self.run_dir,
                           f"--extra-arg=-FakeWorldJson={pb.quote_value(world_json)}"])
        rep = self.report()
        self.assertEqual(code, 0, json.dumps([c for c in rep["checks"]
                                              if c["status"] in ("FAIL", "SKIP")]))
        checks = self.by_name(rep)
        self.assertEqual(checks["launch"]["status"], "PASS")
        self.assertIn("kill-on-close job", checks["launch"]["detail"])
        self.assertEqual(checks["build_fresh"]["status"], "WARN")
        self.assertEqual(checks["no_crash"]["status"], "PASS")
        self.assertEqual(checks["content_cooked"]["status"], "PASS")
        self.assertTrue(os.path.isfile(os.path.join(self.run_dir, "game.log")))
        self.assertIn("exit code 0", rep["shutdown"])
        self.assertIn("-AgentBridge", rep["command_line"])
        self.assertIn("DIAGNOSTIC ONLY", rep["evidence"])
        pid = rep["ping"]["pid"]
        self.assertFalse(ab.pid_alive(pid), "the fake game should have quit")

    def test_a_fresh_package_built_from_a_dirty_tree_is_diagnostic_only(self):
        # playtest_boot's gate calls a dirty build fresh when its dirty files still match.
        world_json = os.path.join(self.tmp, "world.json")
        with open(world_json, "w", encoding="utf-8") as f:
            json.dump({"obstacles": [list(b) for b in fb.DEVTESTMAP_OBSTACLES]}, f)
        dirty = {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
                 "build": {"commit": "abc1234def5678", "branch": "x", "dirty": True,
                           "dirty_count": 3},
                 "head": "abc1234def5678", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(dirty, None)) as gate:
            code = us079.main(["--test-exe-argv", self.fake_game_argv(),
                               "--run-dir", self.run_dir,
                               f"--extra-arg=-FakeWorldJson={pb.quote_value(world_json)}"])
        rep = self.report()
        self.assertEqual(code, 0, json.dumps([c for c in rep["checks"]
                                              if c["status"] in ("FAIL", "SKIP")]))
        self.assertTrue(gate.called)
        self.assertFalse(gate.call_args[0][1], "allow_stale must be False by default")
        check = self.by_name(rep)["build_fresh"]
        self.assertEqual(check["status"], "WARN")
        self.assertIn("DIRTY tree", check["detail"])
        self.assertIn("NOT acceptance evidence", check["detail"])
        self.assertTrue(rep["evidence"].startswith("DIAGNOSTIC ONLY"), rep["evidence"])
        self.assertIn("built from a dirty working tree", rep["evidence"])
        self.assertNotIn("ACCEPTANCE CANDIDATE", rep["evidence"])
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("built from a DIRTY tree (3 uncommitted file(s))", md)
        self.assertNotIn("- Freshness: OK", md)
        self.assertNotIn("--allow-stale", md)

    def test_stale_build_stops_before_a_run_dir(self):
        exe = os.path.join(self.tmp, "LyraGame.exe")
        with open(exe, "wb") as f:
            f.write(b"not a game")
        stale = {"fresh": False, "reasons": ["build commit aaa != HEAD bbb"], "warnings": [],
                 "git_error": None, "build": None, "head": "bbb", "branch": "x"}
        with mock.patch.object(pb, "gate", return_value=(stale, pb.EXIT_INFRA)) as gate, \
                mock.patch.object(ab.Bridge, "launch") as launch:
            code = us079.main(["--exe", exe, "--run-dir", self.run_dir])
        self.assertEqual(code, 2)
        gate.assert_called_once()
        self.assertFalse(gate.call_args[0][1], "allow_stale must be False by default")
        launch.assert_not_called()
        self.assertFalse(os.path.exists(self.run_dir))
        self.assertIn("STALE BUILD", self.log.getvalue())

    def test_missing_exe_is_infra(self):
        code = us079.main(["--exe", os.path.join(self.tmp, "nope.exe"), "--run-dir",
                           self.run_dir])
        self.assertEqual(code, 2)
        self.assertFalse(os.path.exists(self.run_dir))


class TestUs079Evidence(PlaytestCase):
    """evidence() and report.md's Freshness / Evidence lines, without a game."""

    CLEAN = {"fresh": True, "reasons": [], "warnings": [], "git_error": None,
             "build": {"commit": "abc1234def5678", "branch": "x", "dirty": False,
                       "dirty_count": 0}, "head": "abc1234def5678", "branch": "x"}
    STALE = {"fresh": False, "reasons": ["build commit aaa != HEAD bbb"], "warnings": [],
             "git_error": None, "build": None, "head": "bbb", "branch": "x"}

    def written(self, argv, fresh):
        run = us079.Us079Run(us079.parse_args(argv + ["--run-dir", self.run_dir]))
        run.fresh = fresh
        run.run_dir = self.run_dir
        os.makedirs(self.run_dir, exist_ok=True)
        run.exit_code = us079.EXIT_PASS
        run.finished_at = "2026-10-20T00:00:00"
        run.write_reports()
        with open(os.path.join(self.run_dir, "report.md"), encoding="utf-8") as f:
            lines = f.read().splitlines()
        line = {k: [x for x in lines if x.startswith(f"- {k}:")][0]
                for k in ("Evidence", "Freshness")}
        return run.evidence(), line

    def test_allow_stale_is_recorded_and_diagnostic_even_on_a_fresh_clean_build(self):
        evidence, line = self.written(["--allow-stale"], dict(self.CLEAN))
        self.assertTrue(evidence.startswith("DIAGNOSTIC ONLY"), evidence)
        self.assertIn("--allow-stale", line["Evidence"])
        self.assertIn("OK (the package matches the code under test) (run with --allow-stale)",
                      line["Freshness"])

    def test_a_fresh_clean_launch_without_allow_stale_is_a_candidate(self):
        evidence, line = self.written([], dict(self.CLEAN))
        self.assertTrue(evidence.startswith("ACCEPTANCE CANDIDATE"), evidence)
        self.assertNotIn("--allow-stale", line["Evidence"] + line["Freshness"])

    def test_a_stale_build_with_allow_stale(self):
        evidence, line = self.written(["--allow-stale"], dict(self.STALE))
        self.assertIn("STALE or unverified build", evidence)
        self.assertIn("run with --allow-stale", line["Evidence"])
        self.assertIn("STALE BUILD", line["Freshness"])
        self.assertIn("(run with --allow-stale)", line["Freshness"])

    def test_attach_mode_is_always_diagnostic(self):
        evidence, _ = self.written(["--attach-port", "1"], dict(self.CLEAN))
        self.assertTrue(evidence.startswith("DIAGNOSTIC ONLY"), evidence)
        self.assertIn("did not launch", evidence)
        self.assertIn("An attached run is ALWAYS diagnostic only, never acceptance evidence",
                      " ".join(us079.__doc__.split()))


class TestUs079PlanAndAcceptance(PlaytestCase):
    """The PLAN (playtest_kit) and the report's Phase 3 fields."""

    STORIES = {"US-079": "in-progress/US-079-playable-inventory-arena.md",
               "US-071": "in-progress/US-071-inventory-widget.md",
               "US-069": "done/US-069-pause-menu-framework.md"}

    def story_ac_ids(self, story):
        path = os.path.join(os.path.dirname(SCRIPTS), "stories", self.STORIES[story])
        if not os.path.isfile(path):
            self.skipTest(f"{path} moved")
        with open(path, encoding="utf-8") as f:
            return set(re.findall(r"^- \[[ xX]\] \**(AC[-\w.]*\w)", f.read(), re.M))

    def test_the_plan_is_valid_and_proves_real_acs(self):
        import playtest_kit as kit
        self.assertEqual(kit.plan_problems(us079.Us079Run), [])
        self.assertIs(us079.Run, us079.Us079Run)
        self.assertEqual(us079.Us079Run.STORY, "US-079")
        self.assertEqual(us079.Us079Run.COVERS, ("US-071", "US-069"))
        self.assertEqual(us079.Us079Run.FEATURES,
                         ("inventory", "pickups", "identification", "pause_menu"))
        for step in us079.PLAN:
            for ac in step.proves:
                story, ac_id = kit.split_ac(ac, "US-079")
                self.assertIn(ac_id, self.story_ac_ids(story), f"{step.check}: {ac}")
        names = [s.check for s in us079.PLAN]
        for stop in us079.ROUTE:
            for prefix in ("walk", "focus", "prompt", "pickup"):
                self.assertIn(f"{prefix}_{stop['name']}", names)
        self.assertIn("walk_RustySword_waypoint1", names)
        self.assertLess(names.index("walk_RustySword_waypoint1"), names.index("walk_RustySword"))

    def test_plan_prints_the_steps_table(self):
        out = io.StringIO()
        previous = us079.set_log_stream(out)
        self.addCleanup(us079.set_log_stream, previous)
        self.assertEqual(us079.main(["--plan"]), 0)
        rows = [line for line in out.getvalue().splitlines() if line.startswith("| ")]
        self.assertEqual(len(rows), 1 + len(us079.PLAN))
        # 37 -> 44: pickup_labels, empty_state, pickup_loop_time, unidentified_badge and the
        # three filter checks come before it.
        self.assertIn("| 44 | `menu_closes_escape` | tap Escape -> wait_until "
                      "pause_menu.open==false | US-069:AC-069.10 |", rows)

    def test_the_report_has_the_plan_and_the_acs(self):
        _, server = self.serve(escape_budget=1)  # empty_state's Escape works, step 6's does not
        code, rep = self.play(server)
        self.assertEqual(code, 1)
        self.assertEqual(rep["story"], "US-079")
        self.assertEqual(rep["covers"], ["US-071", "US-069"])
        self.assertEqual(rep["features"], ["inventory", "pickups", "identification",
                                           "pause_menu"])
        self.assertEqual([p["check"] for p in rep["plan"]], [s.check for s in us079.PLAN])
        self.assertEqual(rep["undeclared_checks"], [])
        self.assertEqual(rep["new_failures"], ["menu_closes_escape"])
        self.assertEqual(rep["known_failures"], [])
        # pickup_labels lost its known issue (issues #30, #38): a PASS there is no FIXED? candidate.
        self.assertEqual(rep["fixed_candidates"], [])
        acs = {(a["story"], a["id"]): a for a in rep["acs"]}
        self.assertEqual(acs[("US-069", "AC-069.10")]["status"], "FAIL")
        self.assertEqual(acs[("US-069", "AC-069.10")]["checks"], ["menu_closes_escape"])
        for key in (("US-079", "AC-079.2"), ("US-079", "AC-079.6"), ("US-079", "AC-079.9"),
                    ("US-079", "AC-079.10"), ("US-079", "AC-079.11"), ("US-071", "AC-071.7"),
                    ("US-071", "AC-071.8"), ("US-071", "AC-071.13"), ("US-071", "AC-071.15")):
            self.assertEqual(acs[key]["status"], "PASS", key)
        # 7 walks + pickup_loop_time
        self.assertEqual(len(acs[("US-079", "AC-079.6")]["checks"]), 8)
        self.assertIn("pickup_loop_time", acs[("US-079", "AC-079.6")]["checks"])
        self.assertEqual(acs[("US-079", "AC-079.9")]["checks"],
                         ["empty_state", "menu_rows", "menu_ledger_rows"])
        self.assertEqual(acs[("US-071", "AC-071.15")]["checks"],
                         ["empty_state", "filter_armor_empty"])
        self.assertFalse([a for a in rep["acs"] if a["accepted"]],
                         "an attached run never accepts an AC")
        md = open(os.path.join(self.run_dir, "report.md"), encoding="utf-8").read()
        self.assertIn("## Acceptance criteria", md)
        self.assertIn("| US-069:AC-069.10 | menu_closes_escape | FAIL | no |", md)
        self.assertIn("| FAIL (NEW) |", md)
        self.assertLess(md.index("## Acceptance criteria"), md.index("## Pickups"))
        self.assertLess(md.index("## Inventory after the pickups"),
                        md.index("## Environment (state.focus)"))


class TestUs079Helpers(unittest.TestCase):
    def test_lit_quotes_and_escapes(self):
        self.assertEqual(us079.lit("[F] Pick up X"), '"[F] Pick up X"')
        self.assertEqual(us079.lit('a"b\\c'), '"a\\"b\\\\c"')
        text = 'say "hi" \\ ok'  # one backslash, two double quotes
        cond = fb.Condition("pickups.prompt.text==" + us079.lit(text))
        ok, values = cond.evaluate({"pickups": {"prompt": {"text": text}}})
        self.assertTrue(ok, values)

    def test_route_matches_the_devtestmap_pickups(self):
        specs = {s["label"]: s for s in fb.PICKUP_SPECS}
        self.assertEqual(len(us079.ROUTE), 6)
        for stop in us079.ROUTE:
            spec = specs[stop["label"]]
            self.assertEqual(list(stop["target"]), spec["location"][:2])
            self.assertEqual(stop["item_id"], spec["item_id"])
            self.assertEqual(stop["display_name"], spec["display_name"])
        inventory = {s["item_id"] for s in fb.PICKUP_SPECS if s["route"] == "inventory"}
        ledger = {s["item_id"] for s in fb.PICKUP_SPECS if s["route"] == "ledger"}
        self.assertEqual(set(us079.EXPECTED_ITEMS), inventory)
        self.assertEqual(set(us079.EXPECTED_LEDGER), ledger)

    def test_waypoint_avoids_box_02(self):
        # Box_02 grown by the capsule radius: X -490..-310, Y -390..-210.
        (wx, wy), = [s for s in us079.ROUTE if s["name"] == "RustySword"][0]["waypoints"]
        self.assertFalse(-490 <= wx <= -310 and -390 <= wy <= -210)


class TestUs079PickupLabel(unittest.TestCase):
    """read_pickup_label: what makes an actors[].text_labels[] entry a readable name label."""

    @staticmethod
    def actor(**label):
        entry = {"component": "NameLabel", "text": "Iron Ore", "visible": True, "facing": 1.0,
                 "upright": 1.0}
        entry.update(label)
        return {"label": "TestPickup_IronOre", "kind": "pickup", "text_labels": [entry]}

    def problem(self, actor, expected="Iron Ore"):
        lbl, problem = us079.read_pickup_label(actor, expected)
        self.assertTrue((lbl is None) != (problem is None), (lbl, problem))
        return problem

    def test_a_square_upright_label_reading_the_display_name_is_readable(self):
        lbl, problem = us079.read_pickup_label(self.actor(), "Iron Ore")
        self.assertIsNone(problem)
        self.assertEqual(lbl["component"], "NameLabel")

    def test_the_thresholds_are_inclusive(self):
        self.assertIsNone(self.problem(self.actor(facing=0.99, upright=0.99)))
        self.assertIn("facing 0.989 < 0.99", self.problem(self.actor(facing=0.989)))
        self.assertIn("upright 0.989 < 0.99", self.problem(self.actor(upright=0.989)))

    def test_a_label_seen_from_behind_or_upside_down_is_not_readable(self):
        self.assertIn("facing -1.000", self.problem(self.actor(facing=-1.0)))
        self.assertIn("upright -1.000", self.problem(self.actor(upright=-1.0)))

    def test_the_text_must_be_the_display_name(self):
        problem = self.problem(self.actor(text="Mysterious Amulet"), "Unidentified Purple Amulet")
        self.assertIn("no label reads 'Unidentified Purple Amulet'", problem)
        self.assertIn("'Mysterious Amulet'", problem)
        # A pickup outside the route only needs some text
        self.assertIsNone(self.problem(self.actor(text="Anything"), None))
        self.assertIn("no label reads any text", self.problem(self.actor(text="  "), None))

    def test_a_label_without_a_camera_measurement_is_not_readable(self):
        problem = self.problem(self.actor(facing=None, upright=None))
        self.assertIn("facing=None, upright=None (no local camera to measure against)", problem)
        self.assertIn("no local camera", self.problem(self.actor(facing=True)))

    def test_hidden_missing_and_unreported_labels(self):
        self.assertIn("is hidden", self.problem(self.actor(visible=False)))
        self.assertEqual(self.problem({"label": "X", "text_labels": []}), "no text label")
        self.assertIn("the bridge reports no actors[].text_labels", self.problem({"label": "X"}))

    def test_a_readable_label_among_others_is_found(self):
        actor = self.actor()
        actor["text_labels"].insert(0, {"component": "Other", "text": "Iron Ore",
                                        "visible": False, "facing": 1.0, "upright": 1.0})
        lbl, problem = us079.read_pickup_label(actor, "Iron Ore")
        self.assertIsNone(problem)
        self.assertEqual(lbl["component"], "NameLabel")


if __name__ == "__main__":
    unittest.main()
