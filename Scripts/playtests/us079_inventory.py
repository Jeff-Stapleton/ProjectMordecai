#!/usr/bin/env python3
"""US-079 Inventory Playtest -- Tier 4: the inventory vertical slice, played with REAL input

The real-input version of Scripts/verify_pie_inventory.py (Specs/agent_playtest_plan_v1.md
section 2.4, Specs/agent_bridge_v1.md section 8).  It drives the PACKAGED game through the
in-game agent bridge (Scripts/agent_bridge.py).  Every action is a key or mouse event injected
into Slate, the path a player's keyboard and mouse take; nothing calls game functions, and
bridge.jsonl in the run dir is the audit trail of every request.  The run framework (launch,
STALE gate, checks, plan enforcement, AC acceptance, reports, exit codes) is Scripts/
playtest_kit.py (Specs/story_playtests_v1.md section 2); this file is the PLAN and the steps.

Steps (every step is a check: PASS, FAIL, SKIP or WARN; --plan prints them as a table):
  1. Launch DevTestMap (STALE gate first).  Expect B_MordecaiArenaExperience, the bridge ready,
     and the combat HUD on screen with its '[I] Inventory' hint.  Record state.focus, screenshot.
     Every pickup actor (state.actors kind "pickup") has a visible widget component (its
     in-world name label, AC-079.10).
  1b. Before any pickup, tap I: the pause menu opens with the inventory tab's 'Inventory is
     empty.' placeholder (screenshot), then tap Escape: it closes.  Step 4 then reopens the
     menu CommonUI pooled here, so its tab must show the current items (issue #28, US-094).
  2. The 6 pickups, route Potion (400,0) -> IronOre (300,300) -> TownStone (0,420) ->
     UpgradeKey (-300,300) -> Amulet (-400,0) -> waypoint (-250,-150) -> RustySword (-300,-300).
     For each: walk_to it, wait until it is the focused pickup, assert the HUD prompt renders
     '[F] Pick up <focused display name>' and is visible, tap F, wait until it leaves the world
     (a valid pickups section, never an invalid one) AND inventory.count or ledger_count goes
     up (both counts are recorded in the check).  The loop, from the start of the first walk
     to the confirmed 6th pickup, takes under 30 s (AC-079.6).
  3. Assert 4 inventory items and 2 ledger resources, by id and by name.
  4. Tap I: the pause menu opens on the inventory tab with 4 rendered rows (the amulet's with
     the '?' badge), 2 ledger rows and an Identify button on the amulet row.  Screenshot.  Click
     the Weapons filter (1 row: Rusty Sword), the Armor filter ('No items match this filter.')
     and the All filter (4 rows again).
  5. Click the amulet's Identify button (a Slate click at its center, re-read after the filter
     clicks): its row reads 'Mysterious Amulet' and the item is identified.  Screenshot.
  6. Tap Escape: the pause menu closes within 3 s (UMordecaiPauseMenuWidget is a CommonUI back
     handler, AC-069.10).  Screenshot.  A FAIL here is a REGRESSION of that fix
     (Specs/agent_bridge_v1.md section 9), not a known issue.
Story text vs the game: AC-079.2 and AC-079.3 say the pickup happens on BeginOverlap, but the
game picks up the focused pickup when the player presses F (the '[F] Pick up' prompt), so the
pickup_* checks tap F.  The story text needs an update; this script plays what the game does.
A failed check SKIPs the checks that depend on it; independent checks still run.  A lost bridge
connection SKIPs everything after it.  Each check names the ACs it proves (PLAN below:
US-079, US-071 and US-069); report.json["acs"] says which were accepted.

Usage:
  py Scripts/playtests/us079_inventory.py                    fresh package: launch, play, quit
  py Scripts/playtests/us079_inventory.py --allow-stale      STALE package: diagnostic only (recorded)
  py Scripts/playtests/us079_inventory.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us079_inventory.py --plan             print the Steps table; no game
  py Scripts/playtests/us079_inventory.py --attach-port N [--token T]
        play a game that is already running (py Scripts/agent_bridge.py launch, or
        Scripts/tests/fake_bridge.py); it is left running.  An attached run is ALWAYS
        diagnostic only, never acceptance evidence, even on a fresh package: this script did
        not launch the game, so its state may not be pristine.  The build is still checked
        through the process's exe and recorded.  Acceptance needs this script to launch a fresh
        package built from a clean, committed tree itself (no --attach-port, no --allow-stale).
Evidence: report.md's Evidence line reads ACCEPTANCE CANDIDATE only for a launched, fresh,
clean-tree package without --allow-stale that exited 0; every other run is DIAGNOSTIC ONLY.

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us079/ with report.md, report.json, game.log,
bridge.jsonl and bridge_NN.png.  Open every screenshot with the Read tool and state what is on
screen.  A STALE build without --allow-stale stops before any run dir is created, as
playtest_boot.py does.

Exit codes:
  0 -- every check passed (WARN allowed)
  1 -- a gameplay check FAILED (the game did not do what a player expects)
  2 -- infrastructure problem (STALE BUILD, launch failure, no connection, crash)
"""

import os
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-079"
TITLE = "US-079 inventory playtest (Tier 4, real input)"
RUN_SUFFIX = "us079"
MAP = kit.DEFAULT_MAP
EXPERIENCE = kit.DEFAULT_EXPERIENCE
HUD_HINT = "[I] Inventory"
PROMPT_PREFIX = "[F] Pick up "
WALK_TOL = 60.0
TAP_S = 0.1

HUD_TIMEOUT_S = 5.0
FOCUS_TIMEOUT_S = 3.0
PROMPT_TIMEOUT_S = 2.0
PICKUP_TIMEOUT_S = 3.0
MENU_TIMEOUT_S = 5.0
ROWS_TIMEOUT_S = 3.0
BUTTON_TIMEOUT_S = 2.0
IDENTIFY_TIMEOUT_S = 3.0
ESCAPE_TIMEOUT_S = 3.0
FILTER_TIMEOUT_S = 3.0
PICKUP_LOOP_LIMIT_S = 30.0  # AC-079.6: "collect them all in under 30 seconds"

# UMordecaiInventoryWidget::GetListPlaceholderText (MordecaiInventoryWidget.cpp), AC-071.15.
EMPTY_PLACEHOLDER = "Inventory is empty."
FILTERED_PLACEHOLDER = "No items match this filter."
# Filter button ids (FilterToName; the bridge emits tab-button ids lowercase).
FILTER_WEAPONS, FILTER_ARMOR, FILTER_ALL = "weapons", "armor", "all"
WEAPON_ROW_TEXT = "Rusty Sword"  # the only Weapon among the four items
UNIDENTIFIED_BADGE = "?"         # RefreshVisuals prefixes "? " to unidentified rows

# The six DevTestMap pickups (DevTestMap.umap / place_test_pickups.py), in route order.  The
# waypoint keeps the walk to the sword clear of Box_02 (X -450..-350, Y -350..-250).
ROUTE = (
    {"name": "HealthPotion", "label": "TestPickup_HealthPotion", "target": (400.0, 0.0),
     "item_id": "TestItem_HealthPotion", "display_name": "Health Potion"},
    {"name": "IronOre", "label": "TestPickup_IronOre", "target": (300.0, 300.0),
     "item_id": "TestItem_IronOre", "display_name": "Iron Ore"},
    {"name": "TownStone", "label": "TestPickup_TownStone", "target": (0.0, 420.0),
     "item_id": "TestItem_TownStone", "display_name": "Town Stone"},
    {"name": "UpgradeKey", "label": "TestPickup_UpgradeKey", "target": (-300.0, 300.0),
     "item_id": "TestItem_UpgradeKey", "display_name": "Upgrade Key"},
    {"name": "MysteriousAmulet", "label": "TestPickup_MysteriousAmulet", "target": (-400.0, 0.0),
     "item_id": "TestItem_MysteriousAmulet", "display_name": "Unidentified Purple Amulet"},
    {"name": "RustySword", "label": "TestPickup_RustySword", "target": (-300.0, -300.0),
     "item_id": "TestItem_RustySword", "display_name": "Rusty Sword",
     "waypoints": ((-250.0, -150.0),)},
)
# After all six: 4 inventory instances (item_id -> display name) and 2 ledger entries.
EXPECTED_ITEMS = {"TestItem_UpgradeKey": "Upgrade Key", "TestItem_RustySword": "Rusty Sword",
                  "TestItem_MysteriousAmulet": "Unidentified Purple Amulet",
                  "TestItem_HealthPotion": "Health Potion"}
EXPECTED_LEDGER = {"TestItem_IronOre": 1, "TestItem_TownStone": 1}
AMULET_ITEM_ID = "TestItem_MysteriousAmulet"
AMULET_IDENTIFIED_NAME = "Mysterious Amulet"
IDENTIFY_LABEL = "Identify"

SCREENSHOTS = {
    "start": "the Mordecai diorama arena (fixed overhead camera), the player at the centre, the "
             "combat HUD (bars top-left, 'Kills: 0' top-right, '[I] Inventory' bottom-centre) "
             "and the six coloured pickups",
    "empty": "the pause menu ('Paused', tab bar) on the Inventory tab before any pickup: no item "
             "rows, 'Inventory is empty.' under the filter bar and 'No resources stored.' under "
             "'Stored Resources'",
    "menu":"the pause menu ('Paused', tab bar) on the Inventory tab: 4 item rows (Upgrade Key, "
            "Rusty Sword, '? Unidentified Purple Amulet' with an Identify button, Health "
            "Potion) and 'Stored Resources' with Iron Ore x1 and Town Stone x1",
    "identified": "the same inventory tab with the amulet row now reading 'Mysterious Amulet' "
                  "and no Identify button",
    "after_escape": "the arena with the HUD (pause menu closed)",
}
KNOWN_ISSUES = (
    "I does not close the pause menu (no UI action binding); Escape closes it via the CommonUI "
    "back handler.",
)
EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA

# Logging goes through the kit (prefix "[US-079]"); tests capture it with set_log_stream.
set_log_stream = kit.set_log_stream
lit = kit.lit
_dumps = kit.dumps
_section = kit.section


def log(msg):
    kit.log(msg, STORY)


def _pickup_steps(stop):
    """The PLAN steps of one pickup: its waypoints, walk, focus, prompt, pickup."""
    name, label = stop["name"], stop["label"]
    tx, ty = stop["target"]
    steps = [Step(f"walk_{name}_waypoint{i + 1}",
                  f"walk_to({wx:g}, {wy:g}) -> within {WALK_TOL:g} cm (waypoint, clear of Box_02)",
                  proves=("AC-079.6",))
             for i, (wx, wy) in enumerate(stop.get("waypoints") or ())]
    steps += [
        Step(f"walk_{name}", f"walk_to({tx:g}, {ty:g}) -> within {WALK_TOL:g} cm",
             proves=("AC-079.6",)),
        Step(f"focus_{name}", f"wait_until pickups.focused.label=={lit(label)}"),
        Step(f"prompt_{name}", f"wait_until pickups.prompt.visible==true && pickups.prompt.text"
             f"=={lit(PROMPT_PREFIX + stop['display_name'])} (the focused display name)"),
        Step(f"pickup_{name}", f"tap F -> wait until {label} leaves pickups.world and "
             "inventory.count or inventory.ledger_count grows", proves=("AC-079.2",)),
    ]
    return steps


PLAN = (
    Step("hud_visible", f"wait_until pickups.prompt.hint_visible==true && "
         f"pickups.prompt.hint_text=={lit(HUD_HINT)} -> a combat HUD widget in state.widgets"),
    Step("pickup_labels", 'state actors -> every actors[] with kind=="pickup" has a widget '
         "component with actors[].widgets[].visible==true (its in-world name label)",
         proves=("AC-079.10",),
         known_issue="bridge gap B1: actors[].widgets lists only UWidgetComponents, but the "
                     "pickup name label is a UTextRenderComponent (MordecaiItemPickup.cpp:36); "
                     "the 'start' screenshot is the evidence until the bridge reports it"),
    Step("empty_state", "before any pickup: tap I -> wait_until pause_menu.open==true && "
         f"pause_menu.inventory_tab.list_placeholder=={lit(EMPTY_PLACEHOLDER)}; screenshot; "
         "tap Escape -> wait_until pause_menu.open==false",
         proves=("AC-079.9", "US-071:AC-071.15")),
    *[step for stop in ROUTE for step in _pickup_steps(stop)],
    Step("pickup_loop_time", "the start of the first walk to the confirmed 6th pickup takes "
         f"< {PICKUP_LOOP_LIMIT_S:g} s", proves=("AC-079.6",)),
    Step("inventory_count", f"state inventory -> inventory.count=={len(EXPECTED_ITEMS)}",
         proves=("AC-079.2", "AC-079.7")),
    Step("inventory_items", "state inventory -> Health Potion, Upgrade Key, Unidentified Purple "
         "Amulet and Rusty Sword, once each, by item id and display name",
         proves=("AC-079.2", "AC-079.7")),
    Step("ledger_count", f"state inventory -> inventory.ledger_count=={len(EXPECTED_LEDGER)}",
         proves=("AC-079.7",)),
    Step("ledger_entries", "state inventory -> inventory.ledger is Iron Ore x1 and Town Stone x1 "
         "(auto-stored)", proves=("AC-079.7",)),
    Step("menu_opens_inventory", 'tap I -> wait_until pause_menu.open==true && '
         'pause_menu.active_tab=="inventory"', proves=("AC-079.8",)),
    Step("menu_rows", "wait_until len(pause_menu.inventory_tab.rows)==4 (rendered item rows)",
         proves=("AC-079.9", "US-071:AC-071.4")),
    Step("unidentified_badge", f"state pause_menu -> the amulet row's text starts with "
         f"{lit(UNIDENTIFIED_BADGE)} and unidentified==true",
         proves=("AC-079.11", "US-071:AC-071.4")),
    Step("menu_ledger_rows", f"state pause_menu -> {len(EXPECTED_LEDGER)} rendered "
         "pause_menu.inventory_tab.ledger_rows", proves=("AC-079.9", "US-071:AC-071.10")),
    Step("identify_button", f"state pause_menu -> the amulet row has an enabled "
         f"'{IDENTIFY_LABEL}' button on screen", proves=("AC-079.11",)),
    Step("filter_weapons", f"click_button(button_id={lit(FILTER_WEAPONS)}) -> wait_until "
         f"len(pause_menu.inventory_tab.rows)==1 && pause_menu.inventory_tab.rows[0].text=="
         f"{lit(WEAPON_ROW_TEXT)}",
         proves=("AC-079.11", "US-071:AC-071.7", "US-071:AC-071.8")),
    Step("filter_armor_empty", f"click_button(button_id={lit(FILTER_ARMOR)}) -> wait_until "
         f"pause_menu.inventory_tab.list_placeholder=={lit(FILTERED_PLACEHOLDER)} && "
         "len(pause_menu.inventory_tab.rows)==0", proves=("US-071:AC-071.8", "US-071:AC-071.15")),
    Step("filter_all", f"click_button(button_id={lit(FILTER_ALL)}) -> wait_until "
         f"len(pause_menu.inventory_tab.rows)=={len(EXPECTED_ITEMS)}", proves=("US-071:AC-071.7",)),
    Step("identify_click", f"re-read the amulet row's '{IDENTIFY_LABEL}' button (rows rebuild on "
         "a filter click), click Left at its center"),
    Step("amulet_identified", f"state pause_menu, inventory -> the amulet row reads "
         f"'{AMULET_IDENTIFIED_NAME}' with no Identify button and the item is identified",
         proves=("US-071:AC-071.12", "US-071:AC-071.13")),
    Step("menu_closes_escape", "tap Escape -> wait_until pause_menu.open==false",
         proves=("US-069:AC-069.10",)),
)


class Us079Run(kit.PlaytestRun):
    """One run of the playtest.  execute() returns the exit code."""

    STORY = STORY
    COVERS = ("US-071", "US-069")
    FEATURES = ("inventory", "pickups", "identification", "pause_menu")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = MAP
    EXPERIENCE = EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    WALK_TOL = WALK_TOL
    TAP_S = TAP_S
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.pickups = []
        self.inventory = None
        self.amulet_instance = None
        self.menu_open_inventory = False
        self.menu_any_open = False
        self.identify_center = None
        self.identify_clicked = False
        self.menu_stuck = False   # the pause menu stayed open after empty_state's Escape
        self.loop_t0 = None       # monotonic start of the first pickup walk

    def steps(self):
        self.step_boot()
        self.step_empty_state()
        self.step_pickups()
        self.step_inventory()
        self.step_menu()
        self.step_identify()
        self.step_escape()

    # --- step 1 (continued): HUD, first screenshot ------------------------------------------
    def step_boot(self):
        def hud():
            b = self.bridge
            cond = (f"pickups.prompt.hint_visible==true && "
                    f"pickups.prompt.hint_text=={lit(HUD_HINT)}")
            try:
                b.wait_until(cond, timeout_s=HUD_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                return ("FAIL", f"the '{HUD_HINT}' hint is not visible within {HUD_TIMEOUT_S:g}s"
                        f"; last values: {_dumps((e.response or {}).get('values'), 800)}", None)
            st = b.state(["widgets"])
            widgets = st.get("widgets")
            widgets = widgets if isinstance(widgets, list) else []
            classes = [str(w.get("class")) for w in widgets if isinstance(w, dict)]
            hud_classes = [c for c in classes if "combathud" in c.lower()]
            if not hud_classes:
                return ("FAIL", f"'{HUD_HINT}' is visible, but no combat HUD widget is in "
                        f"state.widgets (visible Mordecai widgets: {classes})", None)
            return ("PASS", f"{hud_classes[0]} on screen with '{HUD_HINT}' visible "
                    f"(visible Mordecai widgets: {', '.join(classes)})", None)

        self.attempt("hud_visible", hud)
        self.shot("start")

        def labels():
            st = self.bridge.state(["actors"])
            actors = st.get("actors")
            if not isinstance(actors, list):
                return ("FAIL", f"state.actors is not a list: {_dumps(actors, 400)}", None)
            pickups = [a for a in actors if isinstance(a, dict) and a.get("kind") == "pickup"]
            if not pickups:
                return ("FAIL", "no actor with kind 'pickup' in state.actors (radius 1500): "
                        "nothing to check", None)
            bad, good = [], []
            for a in pickups:
                widgets = [w for w in a.get("widgets") or [] if isinstance(w, dict)]
                shown = [w for w in widgets if w.get("visible") is True]
                if shown:
                    good.append(f"{a.get('label')}={shown[0].get('component')}"
                                f"({shown[0].get('class')})")
                else:
                    bad.append(f"{a.get('label')} (widgets: {_dumps(widgets, 300)})")
            seen = {a.get("label") for a in pickups}
            absent = [s["label"] for s in ROUTE if s["label"] not in seen]
            note = f"; not in state.actors: {', '.join(absent)}" if absent else ""
            if bad:
                return ("FAIL", f"{len(bad)} of {len(pickups)} pickup actor(s) have no visible "
                        f"widget component: {'; '.join(bad)}{note}", None)
            return ("PASS", f"{len(pickups)} pickup actor(s), each with a visible widget: "
                    f"{', '.join(good)}{note}", None)

        self.attempt("pickup_labels", labels)

    # --- step 1b: the empty inventory, before any pickup ------------------------------------
    def step_empty_state(self):
        def empty():
            b = self.bridge
            b.tap("I", hold_s=TAP_S)
            cond = (f"pause_menu.open==true && pause_menu.inventory_tab.list_placeholder=="
                    f"{lit(EMPTY_PLACEHOLDER)}")
            problems = []
            try:
                r = b.wait_until(cond, timeout_s=MENU_TIMEOUT_S)
                parts = [f"'{EMPTY_PLACEHOLDER}' shown {r.get('elapsed_s')}s after I"]
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                v = (e.response or {}).get("values") or {}
                parts = []
                problems.append(f"after I: pause_menu.open={v.get('pause_menu.open')}, "
                                "list_placeholder="
                                f"{v.get('pause_menu.inventory_tab.list_placeholder')!r} "
                                f"(expected the menu open with '{EMPTY_PLACEHOLDER}' within "
                                f"{MENU_TIMEOUT_S:g}s)")
                if v.get("pause_menu.open") is not True:
                    # Escape on a closed menu would OPEN the pause menu: never tap it here.
                    return ("FAIL", problems[0] + "; Escape not tapped (the menu is not open)",
                            None)
            self.shot("empty")
            b.tap("Escape", hold_s=TAP_S)
            try:
                r = b.wait_until("pause_menu.open==false", timeout_s=ESCAPE_TIMEOUT_S)
                parts.append(f"the menu closed {r.get('elapsed_s')}s after Escape")
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                self.menu_stuck = True
                problems.append(f"the pause menu is still open {ESCAPE_TIMEOUT_S:g}s after a "
                                "real Escape press: REGRESSION of the back-handler fix "
                                "(AC-069.10), not a known issue -- run the "
                                "Mordecai.UI.PauseMenu.* tests")
            if problems:
                return "FAIL", "; ".join(problems + parts), None
            return "PASS", "; ".join(parts), None

        self.attempt("empty_state", empty)

    # --- step 2: the six pickups --------------------------------------------------------
    def step_pickups(self):
        self.loop_t0 = time.monotonic()
        for stop in ROUTE:
            self.do_pickup(stop)

        def loop_time():
            last = max(p["picked_s"] for p in self.pickups)
            splits = ", ".join(f"{p['name']} {p['picked_s']:.1f}s" for p in self.pickups)
            ok = last < PICKUP_LOOP_LIMIT_S
            return (("PASS" if ok else "FAIL"),
                    f"{last:.1f}s from the start of the first walk to the confirmed 6th pickup "
                    f"(limit {PICKUP_LOOP_LIMIT_S:g}s; includes the focus and prompt checks); "
                    f"pickups confirmed at: {splits}", last)

        all_picked = len(self.pickups) == len(ROUTE) and all(
            p.get("picked") and p.get("picked_s") is not None for p in self.pickups)
        self.attempt("pickup_loop_time", loop_time, needs=all_picked,
                     needs_why="not run: not every pickup was confirmed")

    def do_pickup(self, stop):
        name, label = stop["name"], stop["label"]
        rec = {"name": name, "label": label, "target": list(stop["target"]), "walk": None,
               "focused": None, "prompt": None, "picked": None, "picked_s": None}
        self.pickups.append(rec)
        stuck_why = ("not run: the pause menu stayed open after empty_state's Escape (game input "
                     "is blocked)")
        for i, (wx, wy) in enumerate(stop.get("waypoints") or ()):
            self.walk(f"walk_{name}_waypoint{i + 1}", wx, wy, needs=not self.menu_stuck,
                      needs_why=stuck_why)
        tx, ty = stop["target"]
        walked, result = self.walk(f"walk_{name}", tx, ty, needs=not self.menu_stuck,
                                   needs_why=stuck_why)
        rec["walk"] = result
        why = f"not run: walk_{name} failed"

        def focus():
            b = self.bridge
            try:
                r = b.wait_until(f"pickups.focused.label=={lit(label)}",
                                 timeout_s=FOCUS_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                focused = ((e.response or {}).get("values") or {}).get("pickups.focused.label")
                return ("FAIL", f"{label} is not the focused pickup {FOCUS_TIMEOUT_S:g}s after "
                        f"the walk (focused: {focused})", None)
            st = b.state(["pickups"])
            f = _section(st, "pickups").get("focused") or {}
            if f.get("label") != label:
                return "FAIL", f"focus moved away again (focused: {f.get('label')})", None
            return ("PASS", f"{label} focused ('{f.get('display_name')}', "
                    f"{float(f.get('distance') or 0):.0f} cm) after {r.get('elapsed_s')}s", f)

        focused_ok, focused = self.attempt(f"focus_{name}", focus, needs=walked, needs_why=why)
        rec["focused"] = focused
        why = f"not run: focus_{name} failed"

        def prompt():
            display = (focused or {}).get("display_name") or ""
            expected = PROMPT_PREFIX + display
            try:
                self.bridge.wait_until(f"pickups.prompt.visible==true && "
                                       f"pickups.prompt.text=={lit(expected)}",
                                       timeout_s=PROMPT_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                v = (e.response or {}).get("values") or {}
                return ("FAIL", f"expected the visible prompt '{expected}'; rendered "
                        f"text={v.get('pickups.prompt.text')!r}, visible="
                        f"{v.get('pickups.prompt.visible')}", None)
            note = ("" if display == stop["display_name"] else
                    f" (note: the item asset's expected name is '{stop['display_name']}')")
            return "PASS", f"'{expected}' rendered and visible{note}", expected

        _, rec["prompt"] = self.attempt(f"prompt_{name}", prompt, needs=focused_ok,
                                        needs_why=why)

        def pick():
            b = self.bridge
            before = _section(b.state(["inventory"]), "inventory")
            if before.get("valid") is not True:
                return ("FAIL", f"inventory section invalid before F ({before.get('reason')}): "
                        "a pickup could not be confirmed, so F was not tapped", False)
            n0, l0 = _stored_counts(before)
            b.tap("F", hold_s=TAP_S)
            t0 = time.monotonic()

            def settled(st):
                # Stop early on an invalid section (no pawn: nothing can be judged); otherwise
                # wait until the pickup left pickups.world AND something was stored.
                p, inv = _section(st, "pickups"), _section(st, "inventory")
                if p.get("valid") is not True or inv.get("valid") is not True:
                    return True
                n, l = _stored_counts(inv)
                return label not in _world_labels(st) and _grew(n0, l0, n, l)

            try:
                st = b.wait_for(settled, timeout_s=PICKUP_TIMEOUT_S,
                                sections=["pickups", "inventory"])
                timed_out = False
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                st, timed_out = e.response or {}, True
            p, inv = _section(st, "pickups"), _section(st, "inventory")
            n, l = _stored_counts(inv)
            counts = (f"inventory.count {_fmt(n0)} -> {_fmt(n)}, inventory.ledger_count "
                      f"{_fmt(l0)} -> {_fmt(l)}")
            if p.get("valid") is not True:
                return ("FAIL", f"pickups section invalid after F ({p.get('reason')}): no evidence "
                        f"{label} was picked up; {counts}", False)
            if inv.get("valid") is not True:
                return ("FAIL", f"inventory section invalid after F ({inv.get('reason')}): the "
                        f"pickup cannot be confirmed; {counts}", False)
            in_world = label in _world_labels(st)
            stored = _grew(n0, l0, n, l)
            if timed_out or in_world or not stored:
                where = (f"{label} is still in pickups.world" if in_world else
                         f"{label} left pickups.world but nothing was stored")
                return ("FAIL", f"{where} {PICKUP_TIMEOUT_S:g}s after F (focused: "
                        f"{(p.get('focused') or {}).get('label')}, prompt: "
                        f"{(p.get('prompt') or {}).get('text')!r}); {counts}", False)
            confirmed = time.monotonic()
            if self.loop_t0 is not None:
                rec["picked_s"] = round(confirmed - self.loop_t0, 2)
            return ("PASS", f"tapped F; {label} left the world and was stored after "
                    f"{confirmed - t0:.2f}s; {counts}", True)

        _, rec["picked"] = self.attempt(f"pickup_{name}", pick, needs=focused_ok, needs_why=why)

    # --- step 3: inventory and ledger ---------------------------------------------------
    def step_inventory(self):
        def count():
            st = self.bridge.state(["inventory"])
            inv = _section(st, "inventory")
            self.inventory = inv
            if not inv.get("valid"):
                return "FAIL", f"inventory section invalid: {inv.get('reason')}", None
            items = [i for i in inv.get("items") or [] if isinstance(i, dict)]
            for it in items:
                if it.get("item_id") == AMULET_ITEM_ID:
                    self.amulet_instance = it.get("instance_id")
            names = ", ".join(str(i.get("display_name")) for i in items)
            n = inv.get("count")
            return ("PASS" if n == len(EXPECTED_ITEMS) else "FAIL",
                    f"inventory.count={n} (expected {len(EXPECTED_ITEMS)}): {names or 'empty'}",
                    inv)

        self.attempt("inventory_count", count)
        have = isinstance(self.inventory, dict) and bool(self.inventory.get("valid"))
        why = "not run: the inventory could not be read"

        def items():
            got = {}
            for it in self.inventory.get("items") or []:
                if isinstance(it, dict):
                    got.setdefault(it.get("item_id"), []).append(it.get("display_name"))
            problems = []
            for item_id, name in EXPECTED_ITEMS.items():
                names = got.get(item_id)
                if not names:
                    problems.append(f"{item_id} missing")
                elif len(names) != 1:
                    problems.append(f"{item_id} x{len(names)}")
                elif names[0] != name:
                    problems.append(f"{item_id} is named '{names[0]}', expected '{name}'")
            extra = sorted(str(k) for k in got if k not in EXPECTED_ITEMS)
            if extra:
                problems.append("unexpected: " + ", ".join(extra))
            if problems:
                return "FAIL", "; ".join(problems), None
            return ("PASS", "; ".join(f"{k}='{v}'" for k, v in EXPECTED_ITEMS.items()), None)

        def ledger_count():
            n = self.inventory.get("ledger_count")
            return ("PASS" if n == len(EXPECTED_LEDGER) else "FAIL",
                    f"inventory.ledger_count={n} (expected {len(EXPECTED_LEDGER)})", None)

        def ledger_entries():
            got = {e.get("item_id"): e.get("count") for e in self.inventory.get("ledger") or []
                   if isinstance(e, dict)}
            if got != EXPECTED_LEDGER:
                return "FAIL", f"ledger {got}, expected {EXPECTED_LEDGER}", None
            names = ", ".join(f"{e.get('display_name')} x{e.get('count')}"
                              for e in self.inventory.get("ledger") or [])
            return "PASS", names, None

        self.attempt("inventory_items", items, needs=have, needs_why=why)
        self.attempt("ledger_count", ledger_count, needs=have, needs_why=why)
        self.attempt("ledger_entries", ledger_entries, needs=have, needs_why=why)

    # --- step 4: I opens the inventory tab --------------------------------------------------
    def _amulet_row(self, st):
        tab = _section(_section(st, "pause_menu"), "inventory_tab")
        rows = [r for r in tab.get("rows") or [] if isinstance(r, dict)]
        if self.amulet_instance:
            want = _norm_id(self.amulet_instance)
            for r in rows:
                if _norm_id(r.get("instance_id")) == want:
                    return r
        for r in rows:  # no inventory read: the unidentified amulet row
            if "amulet" in str(r.get("text") or "").lower() and r.get("unidentified"):
                return r
        return None

    @staticmethod
    def _button_ok(button, viewport):
        if not isinstance(button, dict) or button.get("label") != IDENTIFY_LABEL:
            return False
        c = button.get("center")
        if not (isinstance(c, list) and len(c) >= 2 and all(isinstance(v, (int, float))
                                                           for v in c[:2])):
            return False
        w, h = viewport
        return 0 <= c[0] <= w and 0 <= c[1] <= h and button.get("enabled") is True

    def step_menu(self):
        def open_menu():
            b = self.bridge
            b.tap("I", hold_s=TAP_S)
            try:
                r = b.wait_until('pause_menu.open==true && pause_menu.active_tab=="inventory"',
                                 timeout_s=MENU_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                v = (e.response or {}).get("values") or {}
                self.menu_any_open = v.get("pause_menu.open") is True
                return ("FAIL", f"after I: pause_menu.open={v.get('pause_menu.open')}, "
                        f"active_tab={v.get('pause_menu.active_tab')!r} (expected open on "
                        f"'inventory' within {MENU_TIMEOUT_S:g}s)", None)
            self.menu_any_open = True
            self.menu_open_inventory = True
            return "PASS", f"pause menu open on the inventory tab {r.get('elapsed_s')}s after I", None

        self.attempt("menu_opens_inventory", open_menu)
        why = "not run: the inventory tab did not open"

        def rows():
            b = self.bridge
            timed_out = False
            try:
                b.wait_until("len(pause_menu.inventory_tab.rows)==4", timeout_s=ROWS_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                timed_out = True
            tab = _section(_section(b.state(["pause_menu"]), "pause_menu"), "inventory_tab")
            texts = [str(r.get("text")) for r in tab.get("rows") or [] if isinstance(r, dict)]
            status = "FAIL" if timed_out or len(texts) != 4 else "PASS"
            return status, f"{len(texts)} rendered rows (expected 4): {texts}", tab

        _, tab = self.attempt("menu_rows", rows, needs=self.menu_open_inventory, needs_why=why)

        def badge():
            st = self.bridge.state(["pause_menu"])
            row = self._amulet_row_any(st)
            if row is None:
                texts = [str(r.get("text")) for r in _section(_section(st, "pause_menu"),
                                                             "inventory_tab").get("rows") or []
                         if isinstance(r, dict)]
                return "FAIL", f"no amulet row in the rendered rows: {texts}", None
            text = str(row.get("text") or "")
            ok = text.startswith(UNIDENTIFIED_BADGE) and row.get("unidentified") is True
            return (("PASS" if ok else "FAIL"),
                    f"amulet row text {text!r} (expected to start with '{UNIDENTIFIED_BADGE}'), "
                    f"unidentified={row.get('unidentified')}", row)

        self.attempt("unidentified_badge", badge, needs=self.menu_open_inventory, needs_why=why)

        def ledger_rows():
            texts = [str(t) for t in (tab or {}).get("ledger_rows") or []]
            return ("PASS" if len(texts) == len(EXPECTED_LEDGER) else "FAIL",
                    f"{len(texts)} ledger rows (expected {len(EXPECTED_LEDGER)}): {texts}", None)

        self.attempt("menu_ledger_rows", ledger_rows, needs=tab is not None,
                     needs_why=why if not self.menu_open_inventory
                     else "not run: the rendered rows could not be read")

        def button():
            b = self.bridge
            seen = {}

            def ready(st):
                vp = _section(st, "game").get("viewport_size") or [1280, 720]
                row = self._amulet_row(st)
                seen["row"] = row
                return row is not None and self._button_ok(row.get("identify_button"), vp)

            try:
                st = b.wait_for(ready, timeout_s=BUTTON_TIMEOUT_S, sections=["pause_menu", "game"])
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                row = seen.get("row")
                if row is None:
                    return "FAIL", "no amulet row in the rendered inventory rows", None
                return ("FAIL", f"the amulet row '{row.get('text')}' has no usable Identify "
                        f"button: {_dumps(row.get('identify_button'))}", None)
            row = self._amulet_row(st)
            center = row["identify_button"]["center"]
            self.identify_center = (float(center[0]), float(center[1]))
            return ("PASS", f"'{IDENTIFY_LABEL}' on row '{row.get('text')}' at viewport "
                    f"({center[0]:.0f}, {center[1]:.0f}), enabled", row)

        self.attempt("identify_button", button, needs=self.menu_open_inventory, needs_why=why)
        self.shot("menu", needs=self.menu_any_open or self.menu_open_inventory,
                  needs_why="not taken: the pause menu did not open")

        # The category filters (real clicks on the filter bar's buttons, aimed from state).
        rows_path = "pause_menu.inventory_tab.rows"
        self.click_button("filter_weapons", button_id=FILTER_WEAPONS,
                          then=f"len({rows_path})==1 && {rows_path}[0].text=={lit(WEAPON_ROW_TEXT)}",
                          timeout_s=FILTER_TIMEOUT_S, needs=self.menu_open_inventory,
                          needs_why=why)
        self.click_button("filter_armor_empty", button_id=FILTER_ARMOR,
                          then=(f"pause_menu.inventory_tab.list_placeholder=="
                                f"{lit(FILTERED_PLACEHOLDER)} && len({rows_path})==0"),
                          timeout_s=FILTER_TIMEOUT_S, needs=self.menu_open_inventory,
                          needs_why=why)
        self.click_button("filter_all", button_id=FILTER_ALL,
                          then=f"len({rows_path})=={len(EXPECTED_ITEMS)}",
                          timeout_s=FILTER_TIMEOUT_S, needs=self.menu_open_inventory,
                          needs_why=why)

    # --- step 5: identify the amulet through the UI ----------------------------------------
    def step_identify(self):
        def click():
            # The filter clicks rebuilt the rows: aim at the button where it is NOW.
            b = self.bridge
            seen = {}

            def ready(st):
                vp = _section(st, "game").get("viewport_size") or [1280, 720]
                row = self._amulet_row(st)
                seen["row"] = row
                return row is not None and self._button_ok(row.get("identify_button"), vp)

            try:
                st = b.wait_for(ready, timeout_s=BUTTON_TIMEOUT_S, sections=["pause_menu", "game"])
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                row = seen.get("row")
                return ("FAIL", "the amulet's Identify button is no longer on screen after the "
                        "filter clicks: " + ("no amulet row in the rendered rows" if row is None
                                             else f"row '{row.get('text')}', button "
                                             f"{_dumps(row.get('identify_button'))}"), None)
            c = self._amulet_row(st)["identify_button"]["center"]
            x, y = float(c[0]), float(c[1])
            moved = ""
            ox, oy = self.identify_center
            if abs(x - ox) > 0.5 or abs(y - oy) > 0.5:
                moved = f" (moved from ({ox:.0f}, {oy:.0f}) since identify_button)"
            r = b.click(x, y, button="Left")
            self.identify_clicked = True
            return ("PASS", f"Left click at ({x:.0f}, {y:.0f}){moved}; Slate hit {r.get('hit')}",
                    r)

        self.attempt("identify_click", click, needs=self.identify_center is not None,
                     needs_why="not run: no Identify button to click")

        def identified():
            b = self.bridge
            seen = {}

            def done(st):
                row = self._amulet_row_any(st)
                item = self._amulet_item(st)
                seen.update(row=row, item=item)
                return (row is not None and row.get("text") == AMULET_IDENTIFIED_NAME
                        and not row.get("identify_button") and item is not None
                        and item.get("identified") is True
                        and item.get("display_name") == AMULET_IDENTIFIED_NAME)

            try:
                b.wait_for(done, timeout_s=IDENTIFY_TIMEOUT_S, sections=["pause_menu", "inventory"])
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                row, item = seen.get("row") or {}, seen.get("item") or {}
                return ("FAIL", f"{IDENTIFY_TIMEOUT_S:g}s after the click: row text "
                        f"{row.get('text')!r}, Identify button {_dumps(row.get('identify_button'))}"
                        f", item identified={item.get('identified')}, display_name="
                        f"{item.get('display_name')!r} (expected '{AMULET_IDENTIFIED_NAME}')", None)
            return ("PASS", f"the amulet row reads '{AMULET_IDENTIFIED_NAME}' with no Identify "
                    "button and inventory reports it identified", None)

        self.attempt("amulet_identified", identified, needs=self.identify_clicked,
                     needs_why="not run: the Identify button was not clicked")
        self.shot("identified", needs=self.identify_clicked,
                  needs_why="not taken: the Identify button was not clicked")

    def _amulet_row_any(self, st):
        """The amulet's row by instance id (its text changes when it is identified)."""
        tab = _section(_section(st, "pause_menu"), "inventory_tab")
        rows = [r for r in tab.get("rows") or [] if isinstance(r, dict)]
        if self.amulet_instance:
            want = _norm_id(self.amulet_instance)
            for r in rows:
                if _norm_id(r.get("instance_id")) == want:
                    return r
        for r in rows:
            if "amulet" in str(r.get("text") or "").lower():
                return r
        return None

    def _amulet_item(self, st):
        for it in _section(st, "inventory").get("items") or []:
            if isinstance(it, dict) and it.get("item_id") == AMULET_ITEM_ID:
                return it
        return None

    # --- step 6: Escape closes the menu -------------------------------------------------
    def step_escape(self):
        def escape():
            b = self.bridge
            b.tap("Escape", hold_s=TAP_S)
            try:
                r = b.wait_until("pause_menu.open==false", timeout_s=ESCAPE_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                st = b.state(["pause_menu", "focus", "input"])
                pm, focus = _section(st, "pause_menu"), _section(st, "focus")
                return ("FAIL", f"the pause menu is still open {ESCAPE_TIMEOUT_S:g}s after a real "
                        f"Escape press (active_tab={pm.get('active_tab')!r}, commonui_input_mode="
                        f"{focus.get('commonui_input_mode')!r}, game_input_allowed="
                        f"{focus.get('game_input_allowed')}). REGRESSION of the back-handler fix "
                        "(AC-069.10), not a known issue: Escape closes the menu through CommonUI's "
                        "DefaultBack action because UMordecaiPauseMenuWidget sets bIsBackHandler "
                        "(Specs/agent_bridge_v1.md section 9) -- check that the pushed widget is "
                        "still a back handler and run the Mordecai.UI.PauseMenu.* tests, then "
                        "file it as a regression", None)
            return "PASS", f"the pause menu closed {r.get('elapsed_s')}s after Escape", None

        self.attempt("menu_closes_escape", escape, needs=self.menu_any_open,
                     needs_why="not run: the pause menu never opened")
        self.shot("after_escape", needs=self.menu_any_open,
                  needs_why="not taken: the pause menu never opened")

    # --- report hooks ---------------------------------------------------------------------
    def report_extra(self):
        return {"pickups": self.pickups, "inventory": self.inventory}

    def md_sections(self):
        md = kit.pb.md_cell
        lines = ["", "## Pickups", "", "| Pickup | Target | Walk | Focused | Prompt | Picked up |",
                 "|---|---|---|---|---|---|"]
        for p in self.pickups:
            walk = p.get("walk") or {}
            final = walk.get("final")
            walk_text = (f"({final[0]:.0f}, {final[1]:.0f}), {walk.get('pulses')} pulses"
                         if final else "-")
            focused = (p.get("focused") or {}).get("display_name") or "-"
            lines.append(f"| {p['label']} | ({p['target'][0]:.0f}, {p['target'][1]:.0f}) | "
                         f"{md(walk_text)} | {md(focused)} | {md(p.get('prompt') or '-')} | "
                         f"{'yes' if p.get('picked') else 'no'} |")

        inv = self.inventory or {}
        lines += ["", "## Inventory after the pickups", ""]
        if inv.get("valid"):
            for it in inv.get("items") or []:
                lines.append(f"- {it.get('item_id')}: '{it.get('display_name')}' "
                             f"(identified={it.get('identified')}, x{it.get('quantity')})")
            for e in inv.get("ledger") or []:
                lines.append(f"- ledger {e.get('item_id')}: '{e.get('display_name')}' "
                             f"x{e.get('count')}")
        else:
            lines.append(f"- not read ({inv.get('reason') or 'no inventory state'})")
        return lines


Run = Us079Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def _norm_id(value):
    return str(value or "").strip().strip("{}").replace("-", "").lower()


def _world_labels(st):
    """Labels in state.pickups.world.  Callers must check pickups.valid first: an invalid
    section (no world / no pawn) has no 'world' list, which is NOT "the pickup left"."""
    world = _section(st, "pickups").get("world")
    return [p.get("label") for p in (world if isinstance(world, list) else [])
            if isinstance(p, dict)]


def _stored_counts(inv):
    """(inventory.count, inventory.ledger_count) of an inventory section; None if not a number."""
    def num(v):
        return v if isinstance(v, (int, float)) and not isinstance(v, bool) else None
    return num(inv.get("count")), num(inv.get("ledger_count"))


def _grew(n0, l0, n, l):
    """True if the item count or the ledger entry count went up."""
    return (n0 is not None and n is not None and n > n0) or \
        (l0 is not None and l is not None and l > l0)


def _fmt(v):
    return "?" if v is None else v


def parse_args(argv=None):
    return kit.parse_args(Us079Run, argv)


def main(argv=None):
    return kit.main(Us079Run, argv)


if __name__ == "__main__":
    sys.exit(main())
