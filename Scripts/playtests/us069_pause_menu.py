#!/usr/bin/env python3
"""US-069 Pause Menu Playtest -- Tier 4: the pause menu and its tabs, played with REAL input

Drives the PACKAGED game through the in-game agent bridge (Scripts/agent_bridge.py) with key
taps, gamepad buttons and Slate clicks only; nothing calls game functions, and bridge.jsonl in
the run dir is the audit trail of every request.  The run framework (launch, STALE gate, checks,
plan enforcement, AC acceptance, reports, exit codes) is Scripts/playtest_kit.py
(Specs/story_playtests_v1.md section 2); this file is the PLAN and the steps.

Covers US-069 (pause menu framework), the three tab contents that plug into it: US-066
(Character sheet), US-067 (Skill tree) and US-068 (Feats), and US-095 (recovery: LB/RB switch
tabs and I closes the menu, issues #23 and #32).  About 60 s of play:
  1.  The combat HUD is on screen.
  2.  Escape opens the menu on the Character tab and pauses the game; Menu input mode with the
      mouse cursor, the title 'Paused', the five tabs as tab buttons; W and Q do nothing.
  3.  The Character tab shows the character sheet (known issue N4: it is blank).
  4.  Clicking the Skills, Feats and Settings tab buttons switches the tab; Skills and Feats
      show their content (known issue N5: "Coming Soon"), Settings shows "Coming Soon".
  5.  LB (Gamepad_LeftShoulder) selects the previous tab (Settings -> Inventory) and RB
      (Gamepad_RightShoulder) the next (Inventory -> Settings), wrapping (AC-069.4, AC-095.4).
  6.  Escape closes and resumes; Escape reopens on the last tab; B (Gamepad_FaceButton_Right)
      closes; Start (Gamepad_Special_Right) opens; Escape closes.
  7.  I opens the menu on Inventory; I closes it (AC-095.2); I reopens it, and I held for 1.2 s
      (key repeats included) keeps it open until the key-up, which closes it for good (AC-095.5);
      Escape closes it if it is still open.
A failed check SKIPs the checks that depend on it (every dependency reads the menu's real state
first); a lost bridge connection SKIPs everything after it.

The map is MovementTestMap (/MordecaiCore/Game/Maps/MovementTestMap, B_MordecaiArenaExperience):
an open grid floor with the PlayerStart at (0, 0), no enemies and no pickups.  The script uses
no coordinates; on this map nothing can attack or kill the player while the menu is closed (a
death disables the player's input and could fail a step for the wrong reason).  The
'[I] Inventory' hint is part of the combat HUD on every arena-experience map.

Usage:
  py Scripts/playtests/us069_pause_menu.py                 fresh package: launch, play, quit
  py Scripts/playtests/us069_pause_menu.py --allow-stale   STALE package: diagnostic only
  py Scripts/playtests/us069_pause_menu.py --keep-running  leave the game running afterwards
  py Scripts/playtests/us069_pause_menu.py --plan          print the Steps table; no game
  py Scripts/playtests/us069_pause_menu.py --attach-port N [--token T]   diagnostic only

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us069/ with report.md, report.json, game.log,
bridge.jsonl and bridge_NN.png.  Open every screenshot with the Read tool and state what is on
screen.

Exit codes: 0 every check passed (WARN allowed); 1 a gameplay check FAILED; 2 infrastructure.
"""

import os
import re
import sys

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-069"
TITLE = "US-069 pause menu playtest (Tier 4, real input)"
RUN_SUFFIX = "us069"
TAP_S = 0.1

HUD_HINT = "[I] Inventory"
MENU_TITLE = "Paused"
PLACEHOLDER_TEXT = "Coming Soon"
# AC-069.2: id -> display name, in tab-bar order.
EXPECTED_TABS = (("character", "Character"), ("skills", "Skills"), ("feats", "Feats"),
                 ("inventory", "Inventory"), ("settings", "Settings"))
TAB_IDS = tuple(i for i, _ in EXPECTED_TABS)
DEFAULT_TAB = "character"
# AC-066.3: the 9 primary attributes (short id, display name).
PRIMARY_ATTRIBUTES = (("STR", "Strength"), ("DEX", "Dexterity"), ("END", "Endurance"),
                      ("CON", "Constitution"), ("RES", "Resistance"), ("DIS", "Discipline"),
                      ("INT", "Intelligence"), ("WIS", "Wisdom"), ("CHA", "Charisma"))
# AC-066.4: default group labels (FMordecaiCharacterSheetData, MordecaiCharacterSheetTypes.cpp).
ATTRIBUTE_GROUPS = ("Physical", "Resilience", "Magical")
# AC-066.5: "CurrentValue / MaxValue" (FormatResource: "%d / %d").
RESOURCE_RE = re.compile(r"^\s*-?\d+\s*/\s*\d+\s*$")
SKILL_CATEGORY = "Weapon"     # AC-067.3: the first EMordecaiSkillCategory section
SKILL_MAX_RANK = "/ 20"       # AC-067.4: rank / max rank 20
FEATS_SUMMARY_PREFIX = "Feats:"  # AC-068.8: "Feats: N unlocked -- ..."

HUD_TIMEOUT_S = 5.0
OPEN_TIMEOUT_S = 3.0
CLOSE_TIMEOUT_S = 3.0
TAB_TIMEOUT_S = 3.0
CONTENT_TIMEOUT_S = 3.0
SHOULDER_TIMEOUT_S = 2.0
I_HOLD_S = 1.2        # past the 0.5 s keyboard repeat delay: the bridge sends repeats like the OS
REOPEN_WATCH_S = 1.0  # after the key-up: long enough for a reopen to show
BLOCKED_SETTLE_S = 1.0
BLOCKED_W_HOLD_S = 0.3
MAX_DRIFT_CM = 5.0

KNOWN_N4 = ("N4 The Character tab of the pause menu is blank: UMordecaiCharacterSheetWidget "
            "builds no widget tree (Known issues backlog, Specs/agent_playtest_plan_v1.md)")
KNOWN_N5 = ("N5 The Skills and Feats tabs show \"Coming Soon\": MordecaiPauseMenuWidget registers "
            "only Character and Inventory (Known issues backlog, Specs/agent_playtest_plan_v1.md)")
KNOWN_ISSUES = (KNOWN_N4, KNOWN_N5)

SCREENSHOTS = {
    "menu_character": "the pause menu ('Paused', tab bar Character/Skills/Feats/Inventory/"
                      "Settings) on the Character tab: the 9 primary attributes in Physical / "
                      "Resilience / Magical groups and the Health / Stamina / Spell Points / "
                      "Posture 'x / y' lines (known issue N4: the content area is blank)",
    "menu_skills": "the pause menu on the Skills tab: skill categories (Weapon, ...) and skill "
                   "rows with 'rank / 20' (known issue N5: 'Coming Soon')",
    "menu_feats": "the pause menu on the Feats tab: the 'Feats: N unlocked' summary header "
                  "(known issue N5: 'Coming Soon')",
    "menu_lb": "the pause menu after LB on the Settings tab: the Inventory tab highlighted in the "
               "tab bar and the inventory list ('Inventory is empty.') in the content area",
    "menu_rb": "the pause menu after RB on the Inventory tab: the Settings tab highlighted in the "
               "tab bar and 'Coming Soon' in the content area",
    "menu_closed": "MovementTestMap's open grid floor with the player and the combat HUD, no "
                   "pause menu (closed by Escape)",
    "menu_closed_by_i": "MovementTestMap's open grid floor with the player and the combat HUD, no "
                        "pause menu (closed by I)",
}
EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA

set_log_stream = kit.set_log_stream
lit = kit.lit
_dumps = kit.dumps
_section = kit.section

PLAN = (
    Step("hud_visible", f"wait_until pickups.prompt.hint_visible==true && "
         f"pickups.prompt.hint_text=={lit(HUD_HINT)} -> a combat HUD widget in state.widgets"),
    Step("escape_opens", 'tap Escape -> wait_until pause_menu.open==true && '
         f'pause_menu.active_tab=={lit(DEFAULT_TAB)} && game.paused==true',
         proves=("AC-069.6", "AC-069.7")),
    Step("menu_input_mode", 'state focus, widgets -> focus.commonui_input_mode=="Menu" && '
         f'focus.mouse_cursor_shown==true and the pause menu widget shows {lit(MENU_TITLE)}',
         proves=("AC-069.1",)),
    Step("tabs_listed", "state pause_menu, widgets -> pause_menu.tabs is exactly "
         + ", ".join(f"{i}/{n}" for i, n in EXPECTED_TABS)
         + " and 5 enabled tab buttons with those ids are on screen", proves=("AC-069.2",)),
    Step("game_input_blocked", f"tap W ({BLOCKED_W_HOLD_S:g}s), tap Q, wait "
         f"{BLOCKED_SETTLE_S:g}s -> player.location moved < {MAX_DRIFT_CM:g} cm and "
         "spell_points unchanged, menu still open", proves=("AC-069.1",)),
    Step("character_tab_content", "state pause_menu, widgets -> on the character tab a visible "
         "MordecaiCharacterSheetWidget with all 9 primary attributes (STR..CHA), the groups "
         "Physical/Resilience/Magical and an 'x / y' resource line",
         proves=("US-066:AC-066.1", "US-066:AC-066.3", "US-066:AC-066.4", "US-066:AC-066.5"),
         known_issue=KNOWN_N4),
    Step("click_skills_tab", 'click_button(button_id="skills") -> wait_until '
         'pause_menu.active_tab=="skills"', proves=("AC-069.4",)),
    Step("skills_tab_content", f"state pause_menu, widgets -> a visible "
         f"MordecaiSkillTreeWidget with texts including {lit(SKILL_CATEGORY)} and "
         f"{lit(SKILL_MAX_RANK)}",
         proves=("US-067:AC-067.1", "US-067:AC-067.3", "US-067:AC-067.4"), known_issue=KNOWN_N5),
    Step("click_feats_tab", 'click_button(button_id="feats") -> wait_until '
         'pause_menu.active_tab=="feats"', proves=("AC-069.4",)),
    Step("feats_tab_content", f"state pause_menu, widgets -> a visible MordecaiFeatDisplayWidget "
         f"with a text starting {lit(FEATS_SUMMARY_PREFIX)}",
         proves=("US-068:AC-068.1", "US-068:AC-068.8"), known_issue=KNOWN_N5),
    Step("settings_placeholder", 'click_button(button_id="settings") -> active_tab=="settings" '
         f"and a visible widget shows {lit(PLACEHOLDER_TEXT)}", proves=("AC-069.3",)),
    Step("shoulder_switches_tab", "tap Gamepad_LeftShoulder -> wait_until pause_menu.open==true "
         "&& pause_menu.active_tab == the previous tab in tab-bar order, wrapping "
         "(settings -> inventory)", proves=("AC-069.4", "US-095:AC-095.4")),
    Step("shoulder_rb_switches_tab", "tap Gamepad_RightShoulder -> wait_until "
         "pause_menu.open==true && pause_menu.active_tab == the next tab in tab-bar order, "
         "wrapping (inventory -> settings)", proves=("AC-069.4", "US-095:AC-095.4")),
    Step("escape_closes", "tap Escape -> wait_until pause_menu.open==false && game.paused==false",
         proves=("AC-069.10", "AC-069.6")),
    Step("reopen_restores_tab", "tap Escape -> wait_until pause_menu.open==true && "
         "pause_menu.active_tab == the tab active before escape_closes", proves=("AC-069.11",)),
    Step("gamepad_back_closes", "tap Gamepad_FaceButton_Right -> wait_until "
         "pause_menu.open==false && game.paused==false", proves=("AC-069.10",)),
    Step("start_button_opens", "tap Gamepad_Special_Right -> wait_until pause_menu.open==true "
         "&& game.paused==true", proves=("AC-069.7",)),
    Step("escape_closes_again", "tap Escape -> wait_until pause_menu.open==false"),
    Step("i_opens_inventory", 'tap I -> wait_until pause_menu.open==true && '
         'pause_menu.active_tab=="inventory"', proves=("US-095:AC-095.2",)),
    Step("i_closes", f"tap I (the menu closes on its key-up) -> wait_until pause_menu.open==false "
         f"&& game.paused==false within {CLOSE_TIMEOUT_S:g}s", proves=("US-095:AC-095.2",)),
    Step("i_held_closes_on_key_up", f"tap I -> wait_until pause_menu.open==true; press I, wait "
         f"{I_HOLD_S:g}s (key repeats from 0.5s) -> pause_menu.open==true while held; release I "
         f"-> wait_until pause_menu.open==false && game.paused==false; wait {REOPEN_WATCH_S:g}s "
         "-> still closed (a held I never reopens the menu)", proves=("US-095:AC-095.5",)),
    Step("final_close", "if the menu is still open: tap Escape -> wait_until "
         "pause_menu.open==false (runs even when i_closes or i_held_closes_on_key_up failed)"),
)


# ---------------------------------------------------------------------------
# State helpers
# ---------------------------------------------------------------------------
def _widgets(st):
    w = st.get("widgets") if isinstance(st, dict) else None
    return [x for x in w if isinstance(x, dict)] if isinstance(w, list) else []


def _texts(widget):
    t = widget.get("texts")
    return [str(x) for x in t] if isinstance(t, list) else []


def _widgets_of(st, fragment):
    frag = fragment.lower()
    return [w for w in _widgets(st) if frag in str(w.get("class") or "").lower()]


def _all_texts(st):
    return [t for w in _widgets(st) for t in _texts(w)]


def _describe_widgets(st, limit=600):
    parts = []
    for w in _widgets(st):
        texts = _texts(w)
        parts.append(f"{w.get('class')}{texts[:12]}")
    return _dumps(parts, limit) if parts else "no visible Mordecai widgets"


def _menu(st):
    return _section(st, "pause_menu")


def _has_word(texts, word):
    pat = re.compile(r"\b" + re.escape(word) + r"\b", re.IGNORECASE)
    return any(pat.search(t) for t in texts)


def _has_label(texts, label):
    """A text that IS the label (a group heading), not one that merely contains it."""
    want = label.casefold()
    return any(t.strip().casefold() == want for t in texts)


def _num(v):
    return v if isinstance(v, (int, float)) and not isinstance(v, bool) else None


def _neighbour_tab(tab, step):
    """The tab `step` places from `tab` in tab-bar order, wrapping; None for an unknown tab."""
    return TAB_IDS[(TAB_IDS.index(tab) + step) % len(TAB_IDS)] if tab in TAB_IDS else None


class Us069Run(kit.PlaytestRun):
    """One run of the playtest.  execute() returns the exit code."""

    STORY = STORY
    COVERS = ("US-066", "US-067", "US-068", "US-095")
    FEATURES = ("pause_menu", "character_sheet", "skill_tree_ui", "feat_ui")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.MOVEMENT_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    TAP_S = TAP_S
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.tab_before_close = None
        self.tab_log = []   # (check, active_tab) after each tab step, for the report

    # --- small readers ----------------------------------------------------------------------
    def menu_state(self):
        """(open, active_tab) from a fresh state read; (None, None) when it cannot be read."""
        pm = _menu(self.read(["pause_menu"]))
        if pm.get("valid") is not True:
            return None, None
        return pm.get("open"), pm.get("active_tab")

    def is_open(self):
        return self.menu_state()[0] is True

    def is_closed(self):
        return self.menu_state()[0] is False

    # --- the steps ----------------------------------------------------------------------------
    def steps(self):
        self.step_hud()
        self.step_open()
        self.step_tabs()
        self.step_shoulder()
        self.step_close_reopen()
        self.step_gamepad()
        self.step_inventory_key()

    # 1
    def step_hud(self):
        def hud(b):
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
            huds = [str(w.get("class")) for w in _widgets_of(st, "combathud")]
            if not huds:
                return ("FAIL", f"'{HUD_HINT}' is visible, but no combat HUD widget is in "
                        f"state.widgets ({_describe_widgets(st)})", None)
            return "PASS", f"{huds[0]} on screen with '{HUD_HINT}' visible", None

        self.measure("hud_visible", hud)

    # 2-6
    def step_open(self):
        self.press("escape_opens", "Escape",
                   then=f"pause_menu.open==true && pause_menu.active_tab=={lit(DEFAULT_TAB)} "
                        "&& game.paused==true", timeout_s=OPEN_TIMEOUT_S)
        opened = self.is_open()
        why = "not run: the pause menu is not open"

        def input_mode(st):
            f, menus = _section(st, "focus"), _widgets_of(st, "pausemenu")
            titled = any(MENU_TITLE in _texts(w) for w in menus)
            return (f.get("commonui_input_mode") == "Menu" and f.get("mouse_cursor_shown") is True
                    and titled)

        def describe_mode(st):
            f = _section(st, "focus")
            titles = [t for w in _widgets_of(st, "pausemenu") for t in _texts(w)]
            return (f"commonui_input_mode={f.get('commonui_input_mode')!r}, mouse_cursor_shown="
                    f"{f.get('mouse_cursor_shown')}, game_input_allowed="
                    f"{f.get('game_input_allowed')}; pause menu widget texts: {titles[:8]} "
                    f"(expected '{MENU_TITLE}')")

        self.observe("menu_input_mode", ["focus", "widgets"], input_mode, describe_mode,
                      timeout_s=TAB_TIMEOUT_S, needs=opened, needs_why=why)

        expected_ids = [i for i, _ in EXPECTED_TABS]

        def tab_buttons(st):
            vw, vh = _section(st, "game").get("viewport_size") or [1280, 720]
            out = {}
            for w in _widgets(st):
                for btn in w.get("buttons") or []:
                    if not isinstance(btn, dict):
                        continue
                    bid = str(btn.get("id") or "").lower()
                    c = btn.get("center")
                    on_screen = (isinstance(c, list) and len(c) >= 2
                                 and all(_num(v) is not None for v in c[:2])
                                 and 0 <= c[0] <= vw and 0 <= c[1] <= vh)
                    if bid in expected_ids and on_screen and btn.get("enabled") is True:
                        out.setdefault(bid, btn)
            return out

        def tabs_ok(st):
            tabs = [(t.get("id"), t.get("name")) for t in _menu(st).get("tabs") or []
                    if isinstance(t, dict)]
            if tabs != list(EXPECTED_TABS):
                return False
            buttons = tab_buttons(st)
            return all(i in buttons and str(buttons[i].get("label") or "").casefold()
                       == n.casefold() for i, n in EXPECTED_TABS)

        def describe_tabs(st):
            tabs = [(t.get("id"), t.get("name")) for t in _menu(st).get("tabs") or []
                    if isinstance(t, dict)]
            buttons = tab_buttons(st)
            shown = {i: buttons[i].get("label") for i in buttons}
            return (f"pause_menu.tabs={tabs}; visible enabled tab buttons {len(buttons)}/5: "
                    f"{shown} (expected {list(EXPECTED_TABS)})")

        self.observe("tabs_listed", ["pause_menu", "widgets", "game"], tabs_ok, describe_tabs,
                     timeout_s=TAB_TIMEOUT_S, needs=opened, needs_why=why)

        def blocked(b):
            before = b.state(["player", "pause_menu"])
            p0 = _section(before, "player")
            if p0.get("valid") is not True:
                return "FAIL", f"player section invalid before the keys ({p0.get('reason')})", None
            loc0 = p0.get("location") or [0, 0, 0]
            sp0 = (p0.get("attributes") or {}).get("spell_points")
            b.tap("W", hold_s=BLOCKED_W_HOLD_S)
            b.tap("Q", hold_s=TAP_S)
            b.wait(BLOCKED_SETTLE_S)
            after = b.state(["player", "pause_menu", "focus"])
            p1, pm = _section(after, "player"), _menu(after)
            if p1.get("valid") is not True:
                return "FAIL", f"player section invalid after the keys ({p1.get('reason')})", None
            loc1 = p1.get("location") or [0, 0, 0]
            sp1 = (p1.get("attributes") or {}).get("spell_points")
            drift = ((float(loc1[0]) - float(loc0[0])) ** 2
                     + (float(loc1[1]) - float(loc0[1])) ** 2) ** 0.5
            detail = (f"after W ({BLOCKED_W_HOLD_S:g}s) and Q in the open menu: moved "
                      f"{drift:.1f} cm (max {MAX_DRIFT_CM:g}), spell_points {sp0} -> {sp1}, "
                      f"menu open={pm.get('open')}, commonui_input_mode="
                      f"{_section(after, 'focus').get('commonui_input_mode')!r}")
            if pm.get("open") is not True:
                return "FAIL", detail + "; the menu closed, so nothing was proven", None
            ok = drift < MAX_DRIFT_CM and sp0 == sp1
            return ("PASS" if ok else "FAIL"), detail, None

        self.measure("game_input_blocked", blocked, needs=opened, needs_why=why)

        def sheet_ok(st):
            if _menu(st).get("active_tab") != "character":
                return False
            for w in _widgets_of(st, "charactersheet"):
                texts = _texts(w)
                if (all(_has_word(texts, s) or _has_word(texts, n) for s, n in PRIMARY_ATTRIBUTES)
                        and all(_has_label(texts, g) for g in ATTRIBUTE_GROUPS)
                        and any(RESOURCE_RE.match(t) for t in texts)):
                    return True
            return False

        def describe_sheet(st):
            sheets = _widgets_of(st, "charactersheet")
            if not sheets:
                return (f"active_tab={_menu(st).get('active_tab')!r}; no visible "
                        f"CharacterSheet widget; visible widgets: {_describe_widgets(st)}")
            texts = [t for w in sheets for t in _texts(w)]
            missing = [s for s, n in PRIMARY_ATTRIBUTES
                       if not (_has_word(texts, s) or _has_word(texts, n))]
            groups = [g for g in ATTRIBUTE_GROUPS if not _has_label(texts, g)]
            res = [t for t in texts if RESOURCE_RE.match(t)]
            return (f"{sheets[0].get('class')} visible with {len(texts)} text(s); missing "
                    f"attributes {missing}, missing groups {groups}, resource lines {res[:4]}; "
                    f"texts {_dumps(texts[:16], 400)}")

        self.observe("character_tab_content", ["pause_menu", "widgets"], sheet_ok,
                     describe_sheet, timeout_s=CONTENT_TIMEOUT_S, needs=opened, needs_why=why)
        self.shot("menu_character", needs=opened, needs_why="not taken: the menu is not open")

    # 7-11
    def _tab_content(self, check, tab, fragment, ok_texts, wants, shot_label):
        """Observe the content widget of `tab`: a visible widget whose class contains
        `fragment` and whose texts satisfy ok_texts(texts)."""
        def ok(st):
            if _menu(st).get("active_tab") != tab:
                return False
            return any(ok_texts(_texts(w)) for w in _widgets_of(st, fragment))

        def describe(st):
            found = _widgets_of(st, fragment)
            placeholder = PLACEHOLDER_TEXT in _all_texts(st)
            head = (f"active_tab={_menu(st).get('active_tab')!r}; "
                    + (f"{found[0].get('class')} texts {_dumps(_texts(found[0])[:16], 400)}"
                       if found else f"no visible {fragment} widget"))
            if placeholder:
                head += f"; the content area shows '{PLACEHOLDER_TEXT}'"
            return f"{head} (expected {wants}); visible widgets: {_describe_widgets(st)}"

        on_tab = self.menu_state() == (True, tab)
        self.observe(check, ["pause_menu", "widgets"], ok, describe,
                     timeout_s=CONTENT_TIMEOUT_S, needs=on_tab,
                     needs_why=f"not run: the menu is not open on the {tab} tab")
        self.shot(shot_label, needs=self.is_open(), needs_why="not taken: the menu is not open")

    def _click_tab(self, check, tab):
        ok, _ = self.click_button(check, button_id=tab,
                                  then=f"pause_menu.open==true && pause_menu.active_tab=={lit(tab)}",
                                  timeout_s=TAB_TIMEOUT_S, needs=self.is_open(),
                                  needs_why="not run: the pause menu is not open")
        self.tab_log.append((check, self.menu_state()[1]))
        return ok

    def step_tabs(self):
        self._click_tab("click_skills_tab", "skills")
        self._tab_content(
            "skills_tab_content", "skills", "skilltree",
            lambda texts: (any(SKILL_CATEGORY in t for t in texts)
                           and any(SKILL_MAX_RANK in t for t in texts)),
            f"texts including '{SKILL_CATEGORY}' and '{SKILL_MAX_RANK}'", "menu_skills")
        self._click_tab("click_feats_tab", "feats")
        self._tab_content(
            "feats_tab_content", "feats", "featdisplay",
            lambda texts: any(t.strip().startswith(FEATS_SUMMARY_PREFIX) for t in texts),
            f"a text starting '{FEATS_SUMMARY_PREFIX}'", "menu_feats")

        def settings(b):
            r = b.click_button(button_id="settings")
            target = r.get("target") or {}
            c = target.get("center") or [0, 0]

            def shown(st):
                return (_menu(st).get("active_tab") == "settings"
                        and PLACEHOLDER_TEXT in _all_texts(st))

            what = (f"clicked '{target.get('label')}' at ({float(c[0]):.0f}, {float(c[1]):.0f}); "
                    f"Slate hit {r.get('hit')}")
            try:
                st = b.wait_for(shown, timeout_s=TAB_TIMEOUT_S, sections=["pause_menu", "widgets"])
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                st = e.response or {}
                return ("FAIL", f"{what}; within {TAB_TIMEOUT_S:g}s active_tab="
                        f"{_menu(st).get('active_tab')!r} and no visible '{PLACEHOLDER_TEXT}' "
                        f"(visible widgets: {_describe_widgets(st)})", None)
            holders = [str(w.get("class")) for w in _widgets(st)
                       if PLACEHOLDER_TEXT in _texts(w)]
            return ("PASS", f"{what}; settings tab active, '{PLACEHOLDER_TEXT}' shown by "
                    f"{holders}", None)

        self.measure("settings_placeholder", settings, needs=self.is_open(),
                     needs_why="not run: the pause menu is not open")
        self.tab_log.append(("settings_placeholder", self.menu_state()[1]))

    # 12-13
    def _shoulder(self, check, key, step, shot_label):
        """Tap a shoulder button; the menu must stay open on the neighbouring tab (wrapping).
        The expected tab is computed from a fresh read, so RB still checks the right tab when
        LB failed."""
        is_open, before = self.menu_state()
        want = _neighbour_tab(before, step)
        ok, _ = self.press(check, key,
                           then=f"pause_menu.open==true && pause_menu.active_tab=={lit(want)}",
                           timeout_s=SHOULDER_TIMEOUT_S, needs=is_open is True and want is not None,
                           needs_why="not run: the pause menu is not open on a known tab")
        self.tab_log.append((check, self.menu_state()[1]))
        self.shot(shot_label, needs=ok, needs_why=f"not taken: {check} did not switch the tab")

    def step_shoulder(self):
        self._shoulder("shoulder_switches_tab", "Gamepad_LeftShoulder", -1, "menu_lb")
        self._shoulder("shoulder_rb_switches_tab", "Gamepad_RightShoulder", 1, "menu_rb")

    # 14-15
    def step_close_reopen(self):
        is_open, self.tab_before_close = self.menu_state()
        closed, _ = self.press("escape_closes", "Escape",
                               then="pause_menu.open==false && game.paused==false",
                               timeout_s=CLOSE_TIMEOUT_S, needs=is_open is True,
                               needs_why="not run: the pause menu is not open")
        self.shot("menu_closed", needs=closed, needs_why="not taken: Escape did not close the menu")
        tab = self.tab_before_close
        self.press("reopen_restores_tab", "Escape",
                   then=f"pause_menu.open==true && pause_menu.active_tab=={lit(tab)}",
                   timeout_s=OPEN_TIMEOUT_S, needs=closed and bool(tab) and self.is_closed(),
                   needs_why="not run: escape_closes did not close the menu")

    # 16-18
    def step_gamepad(self):
        self.press("gamepad_back_closes", "Gamepad_FaceButton_Right",
                   then="pause_menu.open==false && game.paused==false",
                   timeout_s=CLOSE_TIMEOUT_S, needs=self.is_open(),
                   needs_why="not run: the pause menu is not open")
        self.press("start_button_opens", "Gamepad_Special_Right",
                   then="pause_menu.open==true && game.paused==true", timeout_s=OPEN_TIMEOUT_S,
                   needs=self.is_closed(), needs_why="not run: the pause menu is not closed")
        self.press("escape_closes_again", "Escape", then="pause_menu.open==false",
                   timeout_s=CLOSE_TIMEOUT_S, needs=self.is_open(),
                   needs_why="not run: the pause menu is not open")

    # 19-22
    def step_inventory_key(self):
        self.press("i_opens_inventory", "I",
                   then='pause_menu.open==true && pause_menu.active_tab=="inventory"',
                   timeout_s=OPEN_TIMEOUT_S, needs=self.is_closed(),
                   needs_why="not run: the pause menu is not closed")
        closed, _ = self.press("i_closes", "I", then="pause_menu.open==false && game.paused==false",
                               timeout_s=CLOSE_TIMEOUT_S, needs=self.is_open(),
                               needs_why="not run: the pause menu is not open")
        self.shot("menu_closed_by_i", needs=closed, needs_why="not taken: I did not close the menu")
        self.measure("i_held_closes_on_key_up", self._held_i, needs=self.is_closed(),
                     needs_why="not run: the pause menu is not closed")
        self.press("final_close", "Escape", then="pause_menu.open==false",
                   timeout_s=CLOSE_TIMEOUT_S, needs=self.is_open(),
                   needs_why="not run: the pause menu is already closed")

    def _held_i(self, b):
        """I held in the open menu: open while down (repeats included), closed by the key-up, and
        still closed after it (a repeat that reached the game would reopen it)."""
        b.tap("I", hold_s=TAP_S)
        try:
            b.wait_until("pause_menu.open==true", timeout_s=OPEN_TIMEOUT_S)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            return "FAIL", f"tapped I; the menu did not open within {OPEN_TIMEOUT_S:g}s", None
        released = False
        b.press("I")
        try:
            b.wait(I_HOLD_S)
            held = _menu(b.state(["pause_menu"]))
            b.release("I")
            released = True
        finally:
            if not released:
                try:
                    b.release("I")
                except ab.BridgeError:
                    pass
        what = (f"pressed I in the open menu and held it {I_HOLD_S:g}s: pause_menu.open="
                f"{held.get('open')}")
        if held.get("open") is not True:
            return "FAIL", what + " while held (it must stay open until the key-up)", None
        try:
            r = b.wait_until("pause_menu.open==false && game.paused==false",
                             timeout_s=CLOSE_TIMEOUT_S)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            v = (e.response or {}).get("values") or {}
            return ("FAIL", f"{what}; released: the menu did not close within {CLOSE_TIMEOUT_S:g}s "
                    f"(open={v.get('pause_menu.open')}, paused={v.get('game.paused')})", None)
        what += f"; released: closed after {r.get('elapsed_s')}s"
        b.wait(REOPEN_WATCH_S)
        after = _menu(b.state(["pause_menu"]))
        if after.get("open") is not False:
            return ("FAIL", f"{what}; {REOPEN_WATCH_S:g}s later pause_menu.open={after.get('open')} "
                    "(the held key reopened it)", None)
        return "PASS", f"{what}; still closed {REOPEN_WATCH_S:g}s later", None

    # --- report hooks ---------------------------------------------------------------------
    def report_extra(self):
        return {"tab_log": [{"check": c, "active_tab": t} for c, t in self.tab_log],
                "tab_before_close": self.tab_before_close}

    def md_sections(self):
        lines = ["", "## Pause menu tabs", "", "| After check | active_tab |", "|---|---|"]
        for c, t in self.tab_log:
            lines.append(f"| {c} | {t!r} |")
        lines.append(f"- Tab active before escape_closes (reopen must restore it): "
                     f"{self.tab_before_close!r}")
        return lines


Run = Us069Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us069Run, argv)


def main(argv=None):
    return kit.main(Us069Run, argv)


if __name__ == "__main__":
    sys.exit(main())
