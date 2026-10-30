#!/usr/bin/env python3
"""US-078 Weapons Playtest -- Tier 4: the weapon arena (US-078, covering US-077), REAL input

Drives the PACKAGED game through the in-game agent bridge (Scripts/agent_bridge.py).  Every
action is a key or mouse event injected into Slate, the path a player's keyboard, mouse and
gamepad take; nothing calls game functions, and bridge.jsonl in the run dir is the audit trail.
The run framework is Scripts/playtest_kit.py (Specs/story_playtests_v1.md section 2); this file
is the PLAN and the steps.

The run plays on CombatTestMap (issue #46, T1): the player stands at the PlayerStart (0, 0),
more than 900 cm from the TestAttacker (out of its aggro range) and away from the TestDummy, so
nothing hits the player while the weapons cycle and the attacks are judged.  On DevTestMap the
Frontliners killed the player before the attacks (light_attack_lmb clicked a dead player).

Steps (every step is a check; --plan prints them as the story's Steps table):
  1. Boot CombatTestMap: the player starts with the Longsword equipped (player.weapon.index 0 of
     count 5).
  2. The weapon HUD shows the equipped weapon's name (a visible widget text equals
     player.weapon.display_name).  Screenshot 'start'.
  3-6. Tap Tab four times: Dagger, Greatsword, Spear, Mace (index and display_name).  Screenshot
     'cycled_mace'.
  7. Tap Tab: wraps to the Longsword.
  8. Tap BackSlash: wraps back to the Mace.
  9. Tap Shift+Tab (the story's binding): previous weapon, the Spear.
  10. Tap Gamepad_FaceButton_Top (Y): next weapon.
  11. Tap Tab: the weapon HUD text follows the new weapon.
  12. Left click on the game view: a light attack starts (Mordecai.State.Attacking in
     player.tags, no Mordecai.Attack.Heavy).
  13. Left click held 0.8 s: a heavy attack starts on the release (Mordecai.Attack.Heavy, which
     only the heavy attack owns, in player.tags).
Each attack is judged from idle: a dead player before the click is an explicit FAIL ("player dead
before the click"), never mistaken for idle, and nothing is clicked.
Each cycle step reads player.weapon.index first and expects the next/previous one (mod 5) with
the matching display name, so one failure does not cascade into wrong expectations.  Weapon
order (the AvailableWeapons list): Longsword, Dagger, Greatsword, Spear, Mace; names compare
case-insensitively.  Known issues (Specs/agent_playtest_plan_v1.md, Known issues backlog): N3
(the weapon HUD draws nothing) and N9 (Shift+Tab is not mapped; Tab still fires Next).

Usage (the kit's common options):
  py Scripts/playtests/us078_weapons.py                    fresh package: launch, play, quit
  py Scripts/playtests/us078_weapons.py --allow-stale      STALE package: diagnostic only
  py Scripts/playtests/us078_weapons.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us078_weapons.py --plan             print the Steps table; no game
  py Scripts/playtests/us078_weapons.py --attach-port N [--token T]   diagnostic only

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us078/ with report.md, report.json, game.log,
bridge.jsonl and bridge_NN.png.  Open every screenshot with the Read tool and state what is on
screen.

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

STORY = "US-078"
TITLE = "US-078 weapon arena playtest (Tier 4, real input)"
TAP_S = 0.1

# The AvailableWeapons order (DA_Weapon_* added on possession, Longsword auto-equipped).
WEAPONS = ("Longsword", "Dagger", "Greatsword", "Spear", "Mace")
WEAPON_COUNT = len(WEAPONS)
MACE_INDEX = WEAPONS.index("Mace")

NEXT_KEY = "Tab"
PREV_KEY = "BackSlash"
PREV_STORY_KEYS = ("LeftShift", "Tab")   # AC-078.7: Shift+Tab
GAMEPAD_NEXT_KEY = "Gamepad_FaceButton_Top"  # Y
ATTACK_POINT = (640, 360)                # the viewport centre: the game view, no widget
ATTACKING_TAG = "Mordecai.State.Attacking"
DEAD_TAG = "Mordecai.State.Dead"
HEAVY_TAG = "Mordecai.Attack.Heavy"         # owned by the heavy attack only (US-083)
HEAVY_HOLD_S = 0.8

START_TIMEOUT_S = 5.0
HUD_TIMEOUT_S = 3.0
CYCLE_TIMEOUT_S = 2.0
LIGHT_TIMEOUT_S = 1.0
HEAVY_TIMEOUT_S = 0.5
ATTACK_CLEAR_S = 2.0

N3 = ("N3 The weapon HUD draws nothing: UMordecaiEquippedWeaponWidget has no widget tree "
      "(Specs/agent_playtest_plan_v1.md, Known issues backlog)")
N9 = ("N9 Previous weapon is BackSlash only: Shift+Tab is not mapped and Tab still fires Next "
      "(Specs/agent_playtest_plan_v1.md, Known issues backlog)")

SCREENSHOTS = {
    "start": "CombatTestMap from the fixed overhead camera: the player at the PlayerStart on the "
             "grid floor, the combat HUD top-left (the blue-grey TestDummy cylinder, 600 cm +X "
             "and +Y, may be in view up and to the right); the weapon HUD should read "
             "'Longsword' (known N3: no weapon name anywhere on screen today)",
    "cycled_mace": "the same view after four Tabs, the player still at the PlayerStart; the "
                   "weapon HUD should read 'Mace' (known N3: no weapon name anywhere on screen "
                   "today)",
}
KNOWN_ISSUES = (N3, N9)

PLAN = (
    Step("starting_weapon", f"state player -> player.weapon.index==0 && "
         f"player.weapon.count=={WEAPON_COUNT} && display_name 'Longsword' (case-insensitive)",
         proves=("AC-078.9", "AC-078.10", "US-077:AC-077.1", "US-077:AC-077.5",
                 "US-077:AC-077.6")),
    Step("weapon_hud_text", "state player, widgets -> a visible Mordecai widget text equals "
         "player.weapon.display_name", proves=("AC-078.11", "AC-078.12", "US-077:AC-077.7"),
         known_issue=N3),
    *[Step(f"cycle_next_{k}", f"tap {NEXT_KEY} -> wait_until player.weapon.index=="
           f"{k} (from {k - 1}), display_name '{WEAPONS[k]}'",
           proves=(("AC-078.6", "AC-078.8", "AC-078.14", "US-077:AC-077.3") if k == 1 else
                   ("AC-078.6", "AC-078.14", "US-077:AC-077.3")))
      for k in range(1, WEAPON_COUNT)],
    Step("cycle_wrap", f"tap {NEXT_KEY} on the last weapon -> wait_until player.weapon.index==0, "
         "display_name 'Longsword'", proves=("AC-078.6", "US-077:AC-077.3")),
    Step("cycle_prev_backslash", f"tap {PREV_KEY} on the first weapon -> wait_until "
         f"player.weapon.index=={MACE_INDEX}, display_name 'Mace'",
         proves=("AC-078.8", "US-077:AC-077.4")),
    Step("cycle_prev_shift_tab", f"tap {'+'.join(PREV_STORY_KEYS)} on the Mace -> wait_until "
         f"player.weapon.index=={MACE_INDEX - 1}, display_name 'Spear'", proves=("AC-078.7",),
         known_issue=N9),
    Step("cycle_gamepad_y", f"tap {GAMEPAD_NEXT_KEY} (Y) -> wait_until player.weapon.index=="
         f"(index+1)%{WEAPON_COUNT} with the matching display_name", proves=("AC-078.6",)),
    Step("weapon_hud_updates", f"tap {NEXT_KEY} -> the new weapon is equipped and a visible "
         "widget text equals its display_name within 3 s (needs weapon_hud_text)",
         proves=("AC-078.12", "AC-078.14", "US-077:AC-077.8"), known_issue=N3),
    Step("light_attack_lmb", f"alive, no {ATTACKING_TAG} -> click Left at {ATTACK_POINT} -> "
         f"{ATTACKING_TAG} without {HEAVY_TAG} in player.tags within {LIGHT_TIMEOUT_S:g} s",
         proves=("AC-078.14",)),
    Step("heavy_attack_hold", f"alive, no {ATTACKING_TAG} -> click Left at {ATTACK_POINT} held "
         f"{HEAVY_HOLD_S:g} s -> {HEAVY_TAG} in player.tags within {HEAVY_TIMEOUT_S:g} s",
         proves=("AC-078.13",)),
)
PENDING = {"AC-078.15": "bridge cannot drive: player.weapon has no equip slot (TwoHand "
                        "clearing both hand slots is not observable)"}

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA
set_log_stream = kit.set_log_stream


def _weapon(st):
    """state.player.weapon ({} when the player section or the weapon is missing)."""
    player = kit.section(st, "player")
    w = player.get("weapon") if player.get("valid") is True else None
    return w if isinstance(w, dict) else {}


def _index(w):
    v = w.get("index")
    return v if isinstance(v, int) and not isinstance(v, bool) else None


def _norm(text):
    return str(text or "").strip().lower()


def _weapon_text(w):
    if not w:
        return "no player.weapon"
    return (f"index {w.get('index')}/{w.get('count')} '{w.get('display_name')}' "
            f"({w.get('type')}, {w.get('id')})")


def _widget_texts(st):
    """[(widget class, text)] for every text of every visible Mordecai widget."""
    widgets = st.get("widgets") if isinstance(st, dict) else None
    out = []
    for w in widgets if isinstance(widgets, list) else []:
        if isinstance(w, dict):
            for t in w.get("texts") or []:
                out.append((str(w.get("class")), str(t)))
    return out


def _hud_shows(st, name):
    """The widget class whose text equals `name` (case-insensitive), or None."""
    if not _norm(name):
        return None
    for cls, text in _widget_texts(st):
        if _norm(text) == _norm(name):
            return cls
    return None


def _tags(st):
    tags = kit.section(st, "player").get("tags")
    return [str(t) for t in tags] if isinstance(tags, list) else []


def _dead(st):
    """The player is dead (player.is_dead, or the Dead tag): never 'idle'."""
    player = kit.section(st, "player")
    return player.get("is_dead") is True or DEAD_TAG in _tags(st)


class Us078Run(kit.PlaytestRun):
    STORY = STORY
    COVERS = ("US-077",)
    FEATURES = ("weapons", "weapon_hud", "melee_attack")
    TITLE = TITLE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    PENDING = PENDING
    TAP_S = TAP_S
    MAP = kit.COMBAT_TEST_MAP
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.cycles = []       # one record per cycle step: check, keys, from, to, name
        self.hud_ok = False

    def steps(self):
        self.step_start()
        self.step_cycling()
        self.step_attacks()

    # --- 1-2: the starting weapon and its HUD -------------------------------------------
    def step_start(self):
        def started(st):
            w = _weapon(st)
            return (_index(w) == 0 and w.get("count") == WEAPON_COUNT
                    and _norm(w.get("display_name")) == _norm(WEAPONS[0]))

        self.observe("starting_weapon", ["player"], started,
                     lambda st: f"player.weapon: {_weapon_text(_weapon(st))} (expected index 0/"
                                f"{WEAPON_COUNT} '{WEAPONS[0]}')", timeout_s=START_TIMEOUT_S)

        def hud_shown(st):
            return _hud_shows(st, _weapon(st).get("display_name")) is not None

        def hud_text(st):
            name = _weapon(st).get("display_name")
            cls = _hud_shows(st, name)
            if cls:
                return f"'{name}' is on screen in {cls}"
            texts = [f"{c}: {t!r}" for c, t in _widget_texts(st)]
            return (f"no visible widget text equals the weapon name {name!r}; visible texts: "
                    f"{kit.dumps(texts, 800)}")

        self.hud_ok, _ = self.observe("weapon_hud_text", ["player", "widgets"], hud_shown,
                                      hud_text, timeout_s=HUD_TIMEOUT_S)
        self.shot("start")

    # --- 3-11: cycling ---------------------------------------------------------------------
    def cycle(self, check, keys, delta, require_from=None, hud=False, needs=True,
              needs_why=None):
        """Tap `keys`, expect player.weapon.index to move by `delta` (mod WEAPON_COUNT) with the
        matching display name; hud=True also expects a widget text to show the new name."""
        rec = {"check": check, "keys": kit.keys_text(keys), "from": None, "to": None,
               "name": None, "passed": False}

        def fn():
            b = self.bridge
            before = _weapon(b.state(["player"]))
            cur = _index(before)
            rec["from"] = cur
            if cur is None:
                return ("FAIL", f"player.weapon.index is not readable before the tap "
                        f"({_weapon_text(before)}), so nothing was pressed", None)
            if require_from is not None and cur != require_from:
                return ("SKIP", f"not run: needs the weapon at index {require_from} "
                        f"('{WEAPONS[require_from]}'), it is at {cur}", None)
            expected = (cur + delta) % WEAPON_COUNT
            want = WEAPONS[expected]
            b.tap(list(keys) if isinstance(keys, tuple) else keys, hold_s=self.TAP_S)
            what = f"tapped {kit.keys_text(keys)} on {cur} '{before.get('display_name')}'"
            try:
                r = b.wait_until(f"player.weapon.index=={expected}", timeout_s=CYCLE_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                after = _weapon(b.state(["player"]))
                rec["to"], rec["name"] = _index(after), after.get("display_name")
                return ("FAIL", f"{what}; expected index {expected} '{want}' within "
                        f"{CYCLE_TIMEOUT_S:g}s, the weapon is {_weapon_text(after)}", after)
            st = b.state(["player", "widgets"] if hud else ["player"])
            after = _weapon(st)
            rec["to"], rec["name"] = _index(after), after.get("display_name")
            if _norm(after.get("display_name")) != _norm(want):
                return ("FAIL", f"{what}; index {expected} after {r.get('elapsed_s')}s, but its "
                        f"display_name is {after.get('display_name')!r}, expected '{want}'",
                        after)
            detail = (f"{what} -> {expected} '{after.get('display_name')}' "
                      f"({after.get('type')}) after {r.get('elapsed_s')}s")
            if hud:
                try:
                    st = b.wait_for(lambda s: _hud_shows(s, want) is not None,
                                    timeout_s=HUD_TIMEOUT_S, sections=["widgets"])
                except ab.BridgeError as e:
                    if e.code != "wait_for_timeout":
                        raise
                    texts = [f"{c}: {t!r}" for c, t in _widget_texts(e.response or {})]
                    return ("FAIL", f"{detail}; but no visible widget text equals '{want}' "
                            f"within {HUD_TIMEOUT_S:g}s; visible texts: {kit.dumps(texts, 800)}",
                            after)
                detail += f"; the HUD shows '{want}' ({_hud_shows(st, want)})"
            rec["passed"] = True
            return "PASS", detail, after

        self.cycles.append(rec)
        return self.attempt(check, fn, needs=needs, needs_why=needs_why)

    def step_cycling(self):
        mace_ok = False
        for k in range(1, WEAPON_COUNT):
            ok, _ = self.cycle(f"cycle_next_{k}", NEXT_KEY, +1)
            if k == MACE_INDEX:
                mace_ok = ok
        self.shot("cycled_mace", needs=mace_ok,
                  needs_why="not taken: cycling to the Mace failed")
        self.cycle("cycle_wrap", NEXT_KEY, +1, require_from=WEAPON_COUNT - 1)
        self.cycle("cycle_prev_backslash", PREV_KEY, -1, require_from=0)
        self.cycle("cycle_prev_shift_tab", PREV_STORY_KEYS, -1)
        self.cycle("cycle_gamepad_y", GAMEPAD_NEXT_KEY, +1)
        self.cycle("weapon_hud_updates", NEXT_KEY, +1, hud=True, needs=self.hud_ok,
                   needs_why="not run: weapon_hud_text failed (no weapon name on screen to "
                             "follow)")

    # --- 12-13: attacks ---------------------------------------------------------------------
    def attack(self, check, hold_s, timeout_s, heavy):
        """Click (held hold_s) from idle; PASS when the attack's tag shows: the heavy attack's
        Mordecai.Attack.Heavy, or for a light attack Mordecai.State.Attacking without it."""
        x, y = ATTACK_POINT

        def started(tags):
            if heavy:
                return HEAVY_TAG in tags
            return ATTACKING_TAG in tags and HEAVY_TAG not in tags

        want = HEAVY_TAG if heavy else f"{ATTACKING_TAG} without {HEAVY_TAG}"

        def fn():
            b = self.bridge
            # An attack is only judged from idle: an earlier attack's tag must be gone first.  A
            # dead player is not idle (no Attacking tag either): it ends the wait at once and is
            # an explicit FAIL, so the click never goes to a dead player.
            try:
                st0 = b.wait_for(lambda s: _dead(s) or ATTACKING_TAG not in _tags(s),
                                 timeout_s=ATTACK_CLEAR_S, sections=["player"])
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                return ("FAIL", f"{ATTACKING_TAG} is already in player.tags and stays there "
                        f"{ATTACK_CLEAR_S:g}s, so a new attack cannot be told apart; nothing was "
                        f"clicked (tags {_tags(e.response or {})})", None)
            if _dead(st0):
                p = kit.section(st0, "player")
                health = kit.section(p, "attributes").get("health")
                return ("FAIL", f"player dead before the click (player.is_dead="
                        f"{p.get('is_dead')}, health {health}, tags {_tags(st0)}); nothing was "
                        "clicked", None)
            r = b.click(x, y, button="Left", hold_s=hold_s)
            what = (f"idle; Left click at ({x}, {y})"
                    + (f" held {hold_s:g}s" if hold_s is not None else "")
                    + f"; Slate hit {r.get('hit')}")
            t0 = time.monotonic()
            try:
                st = b.wait_for(lambda s: started(_tags(s)), timeout_s=timeout_s,
                                sections=["player"], poll_s=0.05)
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                return ("FAIL", f"{what}; no {want} in player.tags within {timeout_s:g}s (tags "
                        f"{_tags(e.response or {})})", None)
            return ("PASS", f"{what}; {want} after {time.monotonic() - t0:.2f}s (tags "
                    f"{_tags(st)})", None)

        return self.attempt(check, fn)

    def step_attacks(self):
        self.attack("light_attack_lmb", None, LIGHT_TIMEOUT_S, heavy=False)
        self.attack("heavy_attack_hold", HEAVY_HOLD_S, HEAVY_TIMEOUT_S, heavy=True)

    # --- report hooks ---------------------------------------------------------------------
    def report_extra(self):
        return {"cycles": self.cycles}

    def md_sections(self):
        md = kit.pb.md_cell
        lines = ["", "## Weapon cycling", "", "| Check | Keys | From | To | Weapon | Passed |",
                 "|---|---|---|---|---|---|"]
        for c in self.cycles:
            lines.append(f"| {c['check']} | {md(c['keys'])} | {c['from']} | {c['to']} | "
                         f"{md(c['name'] or '-')} | {'yes' if c['passed'] else 'no'} |")
        return lines


Run = Us078Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us078Run, argv)


def main(argv=None):
    return kit.main(Us078Run, argv)


if __name__ == "__main__":
    sys.exit(main())
