#!/usr/bin/env python3
"""US-057 Magic Playtest -- Tier 4: the playable magic arena, played with REAL input

Casts the four loadout spells (Q Fireball, E Blink, R Stone Skin, One Restoration) in the
PACKAGED DevTestMap through the in-game agent bridge and checks what a player sees: the SP bar
text, the spell loadout, the Fireball projectile and its damage on a Frontliner, the Blink
teleport, the Stone Skin buff and its HUD indicator, the refusal of a spell the player cannot
afford, and the spell cooldowns (state and HUD).  Every action is a key tap (or walk_to, built
from W/A/S/D taps) injected into Slate; nothing calls game functions.  The run framework is
Scripts/playtest_kit.py (Specs/story_playtests_v1.md section 2); this file is the PLAN and the
steps.  --plan prints the Steps table.

SP plan (max SP 10, no regeneration): 10 -> Fireball 7 -> Blink 5 -> Stone Skin 1 -> Restoration
(cost 3) refused.  The loadout costs 12 per round, more than the pool (design question N6).

Route (DevTestMap; facing is pinned to +X, so every spell goes +X):
  - spawn (0, 0) -> (-150, -650) -> (-700, -650) -> (ex - 300, ey) of Frontliner_3 (its live
    location, about (-700, -500)), re-walked once when the enemy moved more than 40 cm in Y.
    Fireball is never cast +X from spawn: the projectile may die on the Health Potion's pickup
    trigger at (400, 0) (suspected bug).
  - Fireball at Frontliner_3 (300 cm, +X), then walk to (0, -800) on lane L1 (y = -800, x -750..
    +950 is clear) and Blink +X there.
The Fireball checks (cast, projectile, hit, cooldown state, cooldown HUD) come from ONE fast
poll of player, actors (radius 3000) and widgets right after the Q tap, so a short-lived
projectile or a 2 s cooldown cannot be missed between steps.

Usage (the kit's common options):
  py Scripts/playtests/us057_magic.py                fresh package: launch, play, quit
  py Scripts/playtests/us057_magic.py --plan         print the Steps table; no game
  py Scripts/playtests/us057_magic.py --allow-stale  STALE package: diagnostic only (recorded)
  py Scripts/playtests/us057_magic.py --attach-port N [--token T]   always diagnostic

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us057/ (report.md, report.json, game.log,
bridge.jsonl, bridge_NN.png).  Open every screenshot and say what is on screen: the damage
number near Frontliner_3's health bar (fireball_hit) is the only damage-number evidence.

Exit codes: 0 every check passed (WARN allowed), 1 a gameplay FAIL, 2 infrastructure.
"""

import math
import os
import re
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-057"
TITLE = "US-057 magic arena playtest (Tier 4, real input)"
RUN_SUFFIX = "us057"
TAP_S = 0.1
WALK_TOL = 60.0

ACTOR_RADIUS = 3000
POLL_S = 0.03            # the fast poll after the Fireball tap
HUD_TIMEOUT_S = 5.0
SP_TIMEOUT_S = 2.0       # SP drops within 2 s of a tap
PROJECTILE_WINDOW_S = 1.5
HIT_WINDOW_S = 2.5
COOLDOWN_WINDOW_S = 1.0
SP_HUD_TIMEOUT_S = 2.0
BLINK_TIMEOUT_S = 2.0
STONE_SKIN_TIMEOUT_S = 2.0
INDICATOR_TIMEOUT_S = 2.0
REFUSAL_WATCH_S = 1.5
EXPIRE_TIMEOUT_S = 12.0

# The loadout (DA_Spell_*, AC-057.1-4 and AC-057.6): slot, key, display name, SP cost.
SPELLS = (
    {"slot": 0, "key": "Q", "name": "Fireball", "cost": 3},
    {"slot": 1, "key": "E", "name": "Blink", "cost": 2},
    {"slot": 2, "key": "R", "name": "Stone Skin", "cost": 4},
    {"slot": 3, "key": "One", "name": "Restoration", "cost": 3},
)
FIREBALL, BLINK, STONE_SKIN, RESTORATION = SPELLS
SP_START = 10
SP_PLAN = (10, 7, 5, 1)  # after Fireball, Blink, Stone Skin; Restoration is refused at 1

TARGET_ENEMY = "Frontliner_3"     # spawns at (-400, -500)
SHOT_WAYPOINTS = ((-150.0, -650.0), (-700.0, -650.0))
SHOT_OFFSET_X = 300.0             # stand 300 cm -X of the enemy: the Fireball flies +X
REWALK_DY = 40.0
SHOT_TOL = 25.0                   # the shot position: tighter than WALK_TOL (|dy| <= 40)
LANE_POINT = (0.0, -800.0)        # lane L1: y = -800, x -750..+950 clear
BLINK_MIN_DX = 400.0              # Blink moves up to 500 along +X
BLINK_MAX_DY = 50.0

STONE_SKIN_TAG = "Mordecai.Status.StoneSkin"
RESTORATION_TAG = "Mordecai.Status.Restoration"
STONE_SKIN_TEXT = "StoneSkin"
SP_BAR = "SpellPointsBar"         # WBP_SpellPointsBar / MordecaiSpellPointsBarWidget
COOLDOWN_WIDGET = "SpellCooldown"  # WBP_SpellCooldownA/B / MordecaiSpellCooldownWidget
COOLDOWN_WIDGETS_EXPECTED = 4

KI_NO_COOLDOWNS = ("Known issues backlog (Specs/agent_playtest_plan_v1.md) 'Spells have no "
                   "cooldowns': no DA_Spell_* sets a CooldownTag (ApplySpellCooldown returns "
                   "early) and the HUD cooldown widgets are never bound")
KI_N8 = ("N8 (suspected NEW, Known issues backlog of Specs/agent_playtest_plan_v1.md): the HUD "
         "default layout (UMordecaiCombatHUDWidget) has 2 SpellCooldown widgets "
         "(SpellCooldownA/B), not 4")

SCREENSHOTS = {
    "fireball_hit": "the diorama arena, the player about 300 cm -X of Frontliner_3 (a red "
                    "cylinder under its red health bar) and an orange damage number near that "
                    "bar; the SP bar reads '7 / 10'. The ONLY damage-number evidence of the run",
    "after_blink": "the player on lane L1 (south edge, y = -800) about 500 cm further +X than "
                   "before the Blink; SP bar '5 / 10'",
    "stone_skin": "the Stone Skin buff indicator in the status bar (bottom-left): 'StoneSkin' and "
                  "a decimal countdown, green tint; SP bar '1 / 10'",
    "spell_hud": "the SP bar reading '1 / 10' after the refused Restoration, and the spell "
                 "cooldown widgets (top-left, under the SP bar)",
}
KNOWN_ISSUES = (
    "Spells have no cooldowns (Known issues backlog): fireball_cooldown_state and cooldown_hud "
    "are expected to FAIL.",
    "N8 (suspected NEW, to be filed in the Known issues backlog): the HUD default layout has 2 "
    "SpellCooldown widgets, not the 4 AC-057.9 asks for; cooldown_widgets is expected to FAIL.",
    "N6 (design question): max SP is 10 and SP does not regenerate, but one cast of each "
    "loadout spell costs 12 (3 + 2 + 4 + 3); this run spends 10 -> 7 -> 5 -> 1 and expects "
    "Restoration to be refused.",
)
NOTES = (
    "SP 10 vs a loadout cost of 12 with no SP regeneration: a fresh character cannot cast all "
    "four spells once (design question N6). The run spends 10 -> 7 -> 5 -> 1 and checks that "
    "Restoration (3 SP) is refused at 1 SP.",
    "Gamepad spell bindings (LB / RB / LB+RB / DPadUp, AC-057.5) are NOT exercised: only the "
    "keyboard keys Q / E / R / One are played.",
    "AC-057.7 / AC-057.8 / AC-057.11 (the fire Frontliner and the Burning indicator) and "
    "Restoration's heal (AC-057.4) are not covered: the Frontliners do not attack today, and "
    "the SP plan leaves no SP for Restoration.",
)
EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA

set_log_stream = kit.set_log_stream
lit = kit.lit
_dumps = kit.dumps
_section = kit.section
RE_NUMBER_TEXT = re.compile(r"^\s*\d+(?:\.\d+)?\s*s?\s*$")


def log(msg):
    kit.log(msg, STORY)


def _sp_text(sp, max_sp):
    return f"{int(round(sp))} / {int(round(max_sp))}"


PLAN = (
    Step("hud_visible", f"state widgets -> a {SP_BAR} widget renders {lit(_sp_text(10, 10))} "
         "(the SP bar, current / max)",
         proves=("AC-057.9", "AC-057.10", "US-055:AC-055.1", "US-055:AC-055.3")),
    Step("spell_loadout", "state player -> player.spells slots 0-3: keys Q/E/R/One, Fireball/"
         "Blink/Stone Skin/Restoration, granted, sp_cost 3/2/4/3",
         proves=("AC-057.1", "AC-057.2", "AC-057.3", "AC-057.4", "AC-057.6")),
    Step("cooldown_widgets", f"state widgets -> {COOLDOWN_WIDGETS_EXPECTED} {COOLDOWN_WIDGET} "
         "widgets on the HUD (one per spell)", proves=("AC-057.9",), known_issue=KI_N8),
    Step("walk_shot_line", ", ".join(f"walk_to({x:g}, {y:g})" for x, y in SHOT_WAYPOINTS)
         + ", walk_to(ex-"
         f"{SHOT_OFFSET_X:g}, ey) of {TARGET_ENEMY} (live location, actors radius "
         f"{ACTOR_RADIUS}) within {SHOT_TOL:g} cm; walk again if |player.y - ey| > {REWALK_DY:g}"),
    Step("fireball_cast", f"tap Q -> player.attributes.spell_points 10 -> 7 within "
         f"{SP_TIMEOUT_S:g}s (fast poll of player, actors, widgets)",
         proves=("AC-057.1", "AC-057.5", "AC-057.10", "US-055:AC-055.2")),
    Step("fireball_projectile", f"same poll -> an actors[] entry with kind==\"projectile\" "
         f"within {PROJECTILE_WINDOW_S:g}s of the tap", proves=("AC-057.1",)),
    Step("fireball_hits_enemy", f"same poll -> the nearest enemy's health drops below its "
         f"pre-cast value (200) within {HIT_WINDOW_S:g}s; detail reports the damage",
         proves=("AC-057.1", "AC-057.12")),
    Step("sp_hud_updates", f"state widgets -> the {SP_BAR} widget renders {lit(_sp_text(7, 10))}",
         proves=("AC-057.10", "US-055:AC-055.2", "US-055:AC-055.3")),
    Step("fireball_cooldown_state", f"same poll -> player.spells[0].cooldown_remaining_s>0 within "
         f"{COOLDOWN_WINDOW_S:g}s of the tap", proves=("AC-057.1",), known_issue=KI_NO_COOLDOWNS),
    Step("cooldown_hud", f"same poll -> a {COOLDOWN_WIDGET} widget renders a numeric remaining "
         "time", proves=("US-055:AC-055.8", "US-055:AC-055.9"), known_issue=KI_NO_COOLDOWNS),
    Step("walk_blink_lane", f"walk_to({LANE_POINT[0]:g}, {LANE_POINT[1]:g}) -> within {WALK_TOL:g} cm (lane L1)"),
    Step("blink", f"tap E -> player.location.x rises > {BLINK_MIN_DX:g} and spell_points 7 -> 5 "
         f"within {BLINK_TIMEOUT_S:g}s, |dy| < {BLINK_MAX_DY:g}",
         proves=("AC-057.2", "AC-057.5")),
    Step("stone_skin", f"tap R -> {STONE_SKIN_TAG} in player.tags and spell_points 5 -> 1 within "
         f"{STONE_SKIN_TIMEOUT_S:g}s", proves=("AC-057.3",)),
    Step("stone_skin_indicator", f"state widgets -> a widget renders {lit(STONE_SKIN_TEXT)} and a "
         f"numeric remaining duration within {INDICATOR_TIMEOUT_S:g}s",
         proves=("US-055:AC-055.4", "US-055:AC-055.5", "US-055:AC-055.6")),
    Step("restoration_refused_low_sp", f"tap One at 1 SP -> for {REFUSAL_WATCH_S:g}s "
         f"spell_points stays 1, no {RESTORATION_TAG}, player.spells[3].can_afford==false"),
    Step("stone_skin_expires", f"state player, widgets -> within {EXPIRE_TIMEOUT_S:g}s "
         f"{STONE_SKIN_TAG} leaves player.tags and no widget renders {lit(STONE_SKIN_TEXT)}",
         proves=("US-055:AC-055.6",)),
)


# ---------------------------------------------------------------------------
# State helpers
# ---------------------------------------------------------------------------
def _num(v):
    return float(v) if isinstance(v, (int, float)) and not isinstance(v, bool) else None


def _player(st):
    return _section(st, "player")


def _attrs(st):
    a = _player(st).get("attributes")
    return a if isinstance(a, dict) else {}


def _sp(st):
    return _num(_attrs(st).get("spell_points"))


def _max_sp(st):
    return _num(_attrs(st).get("max_spell_points"))


def _xy(st):
    loc = _player(st).get("location")
    if isinstance(loc, list) and len(loc) >= 2 and _num(loc[0]) is not None \
            and _num(loc[1]) is not None:
        return float(loc[0]), float(loc[1])
    return None


def _tags(st):
    tags = _player(st).get("tags")
    return [str(t) for t in tags] if isinstance(tags, list) else []


def _spells(st):
    spells = _player(st).get("spells")
    return [s for s in spells if isinstance(s, dict)] if isinstance(spells, list) else []


def _spell(st, slot):
    for s in _spells(st):
        if s.get("slot") == slot:
            return s
    return None


def _widgets(st):
    w = st.get("widgets") if isinstance(st, dict) else None
    return [x for x in w if isinstance(x, dict)] if isinstance(w, list) else []


def _texts(widget):
    t = widget.get("texts")
    return [str(x) for x in t] if isinstance(t, list) else []


def _matches(widget, fragment):
    f = fragment.lower()
    return f in str(widget.get("class") or "").lower() or f in str(widget.get("name") or "").lower()


def _sp_bars(st):
    return [w for w in _widgets(st) if _matches(w, SP_BAR)]


def _cooldown_widgets(st):
    return [w for w in _widgets(st) if _matches(w, COOLDOWN_WIDGET)]


def _numeric_texts(widget):
    return [t for t in _texts(widget) if RE_NUMBER_TEXT.match(t)]


def _stone_skin_widgets(st):
    """Widgets that render a text containing 'StoneSkin'."""
    return [w for w in _widgets(st) if any(STONE_SKIN_TEXT in t for t in _texts(w))]


def _stone_skin_indicator(st):
    """A widget rendering 'StoneSkin' AND a numeric remaining duration, or None."""
    for w in _stone_skin_widgets(st):
        if _numeric_texts(w):
            return w
    return None


def _widget_line(w):
    return f"{w.get('class')}/{w.get('name')} texts={_texts(w)}"


def _actors(st):
    a = st.get("actors") if isinstance(st, dict) else None
    return [x for x in a if isinstance(x, dict)] if isinstance(a, list) else []


def _enemies(st):
    return [a for a in _actors(st) if a.get("kind") == "enemy"]


def _projectiles(st):
    return [a for a in _actors(st) if a.get("kind") == "projectile"]


def _health(actor):
    h = actor.get("health") if isinstance(actor, dict) else None
    return _num(h.get("current")) if isinstance(h, dict) else None


def _loc(actor):
    loc = actor.get("location") if isinstance(actor, dict) else None
    if isinstance(loc, list) and len(loc) >= 2 and _num(loc[0]) is not None \
            and _num(loc[1]) is not None:
        return float(loc[0]), float(loc[1])
    return None


def _find_enemy(st, fragment):
    for e in _enemies(st):
        if fragment.lower() in str(e.get("label") or e.get("name") or "").lower():
            return e
    return None


def _fmt(v, nd=0):
    return "?" if v is None else f"{v:.{nd}f}"


# ---------------------------------------------------------------------------
# The run
# ---------------------------------------------------------------------------
class Us057Run(kit.PlaytestRun):
    STORY = STORY
    COVERS = ("US-055",)
    FEATURES = ("spells", "spell_hud", "projectiles_aim", "status_effects")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    WALK_TOL = WALK_TOL
    TAP_S = TAP_S
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.sp_log = []          # (step, SP before, SP after) as the run saw them
        self.fireball = None      # the fast-poll observations after the Q tap
        self.blink_result = None
        self.stone_skin_t0 = None
        self.stone_skin_ok = False
        self.loadout = None

    def steps(self):
        self.step_hud()
        self.step_fireball()
        self.step_blink()
        self.step_stone_skin()
        self.step_restoration()
        self.step_stone_skin_expires()

    # --- 1-3: HUD, loadout, cooldown widgets ----------------------------------------------
    def step_hud(self):
        want = _sp_text(SP_START, SP_START)

        def has_text(st):
            return any(want in _texts(w) for w in _sp_bars(st))

        def describe(st):
            bars = _sp_bars(st)
            hit = [w for w in bars if want in _texts(w)]
            if hit:
                return f"'{want}' rendered by {_widget_line(hit[0])}"
            return (f"expected '{want}' in a {SP_BAR} widget; {SP_BAR} widgets: "
                    f"{[_widget_line(w) for w in bars] or 'none'}; player SP "
                    f"{_fmt(_sp(st))} / {_fmt(_max_sp(st))}; widget classes "
                    f"{[w.get('class') for w in _widgets(st)]}")

        self.observe("hud_visible", ["widgets", "player"], has_text, describe,
                     timeout_s=HUD_TIMEOUT_S)

        def loadout(b):
            st = b.state(["player"])
            p = _player(st)
            if p.get("valid") is not True:
                return "FAIL", f"player section invalid: {p.get('reason')}", None
            self.loadout = _spells(st)
            problems, seen = [], []
            for want_s in SPELLS:
                s = _spell(st, want_s["slot"])
                if s is None:
                    problems.append(f"slot {want_s['slot']} missing")
                    continue
                seen.append(f"{s.get('slot')}:{s.get('key')} {s.get('display_name')!r} "
                            f"cost={s.get('sp_cost')} granted={s.get('granted')} "
                            f"cooldown_tag={s.get('cooldown_tag')!r}")
                if str(s.get("key") or "").lower() != want_s["key"].lower():
                    problems.append(f"slot {want_s['slot']} key {s.get('key')!r}, expected "
                                    f"{want_s['key']}")
                if s.get("display_name") != want_s["name"]:
                    problems.append(f"slot {want_s['slot']} is {s.get('display_name')!r}, "
                                    f"expected {want_s['name']!r}")
                if s.get("granted") is not True:
                    problems.append(f"{want_s['name']} granted={s.get('granted')}")
                if _num(s.get("sp_cost")) != float(want_s["cost"]):
                    problems.append(f"{want_s['name']} sp_cost={s.get('sp_cost')}, expected "
                                    f"{want_s['cost']}")
            text = "; ".join(seen) or "no spells"
            if problems:
                return "FAIL", f"{'; '.join(problems)} (loadout: {text})", self.loadout
            return "PASS", text, self.loadout

        self.measure("spell_loadout", loadout)

        def cooldown_widgets(b):
            st = b.state(["widgets"])
            ws = _cooldown_widgets(st)
            lines = [_widget_line(w) for w in ws]
            status = "PASS" if len(ws) == COOLDOWN_WIDGETS_EXPECTED else "FAIL"
            return (status, f"{len(ws)} {COOLDOWN_WIDGET} widget(s) on screen (expected "
                    f"{COOLDOWN_WIDGETS_EXPECTED}): {lines}", ws)

        self.measure("cooldown_widgets", cooldown_widgets)

    # --- 4-10: the Fireball -----------------------------------------------------------------
    def walk_shot_line(self):
        b = self.bridge
        walks = []
        for wx, wy in SHOT_WAYPOINTS:
            r = b.walk_to(wx, wy, tol=WALK_TOL)
            walks.append(f"({r['final'][0]:.0f}, {r['final'][1]:.0f})")
        detail = None
        for attempt_n in (1, 2):
            st = b.state(["player", "actors"], radius=ACTOR_RADIUS)
            enemy = _find_enemy(st, TARGET_ENEMY)
            if enemy is None:
                return ("FAIL", f"no enemy '{TARGET_ENEMY}' in actors (radius {ACTOR_RADIUS}); "
                        f"enemies: {[e.get('label') for e in _enemies(st)]}", None)
            if _loc(enemy) is None:
                return "FAIL", f"{enemy.get('label')} has no location: {_dumps(enemy, 400)}", None
            ex, ey = _loc(enemy)
            tx, ty = ex - SHOT_OFFSET_X, ey
            r = b.walk_to(tx, ty, tol=SHOT_TOL)
            walks.append(f"({r['final'][0]:.0f}, {r['final'][1]:.0f})")
            st = b.state(["player", "actors"], radius=ACTOR_RADIUS)
            enemy = _find_enemy(st, TARGET_ENEMY) or enemy
            ex2, ey2 = _loc(enemy) or (ex, ey)
            if _xy(st) is None:
                return "FAIL", f"no player location: {_player(st).get('reason')}", None
            px, py = _xy(st)
            dy = abs(py - ey2)
            detail = (f"walked {' -> '.join(walks)}; {enemy.get('label')} at ({ex2:.0f}, "
                      f"{ey2:.0f}), player ({px:.0f}, {py:.0f}), {ex2 - px:.0f} cm +X of the "
                      f"player, |dy| {dy:.0f}")
            if dy <= REWALK_DY and ex2 > px:
                return "PASS", detail + (" (re-walked once)" if attempt_n == 2 else ""), \
                    {"player": [px, py], "enemy": [ex2, ey2], "label": enemy.get("label")}
        return ("FAIL", f"{detail}: not on the enemy's line (|dy| > {REWALK_DY:g} or the enemy is "
                "not +X) after a re-walk", None)

    def step_fireball(self):
        walked, _ = self.attempt("walk_shot_line", self.walk_shot_line)
        obs = {"tapped": False}
        self.fireball = obs

        def cast():
            b = self.bridge
            st0 = b.state(["player", "actors"], radius=ACTOR_RADIUS)
            sp0, max0 = _sp(st0), _max_sp(st0)
            pos = _xy(st0) or (0.0, 0.0)
            enemies = _enemies(st0)
            target = min(enemies, key=lambda e: math.hypot(
                ((_loc(e) or pos)[0] - pos[0]), ((_loc(e) or pos)[1] - pos[1])), default=None)
            obs.update(sp0=sp0, max_sp=max0, target=(target or {}).get("label"),
                       target_hp0=_health(target) if target else None,
                       hp0={e.get("label"): _health(e) for e in enemies},
                       sp_values=[], sp_at=None, projectile_at=None, projectile=None,
                       hit_at=None, hit_hp=None, min_hp={}, cooldown_max=None,
                       cooldown_at=None, cooldown_hud_at=None, cooldown_hud=None,
                       cooldown_texts=[], polls=0)
            want_sp = None if sp0 is None else sp0 - FIREBALL["cost"]
            obs["want_sp"] = want_sp
            b.tap(FIREBALL["key"], hold_s=TAP_S)
            obs["tapped"] = True
            t0 = time.monotonic()
            deadline = t0 + max(SP_TIMEOUT_S, PROJECTILE_WINDOW_S, HIT_WINDOW_S)
            while True:
                st = b.state(["player", "actors", "widgets"], radius=ACTOR_RADIUS)
                t = time.monotonic() - t0
                obs["polls"] += 1
                sp = _sp(st)
                if not obs["sp_values"] or obs["sp_values"][-1] != sp:
                    obs["sp_values"].append(sp)
                if obs["sp_at"] is None and sp is not None and want_sp is not None \
                        and sp == want_sp:
                    obs["sp_at"] = round(t, 2)
                projs = _projectiles(st)
                if projs and obs["projectile_at"] is None:
                    obs["projectile_at"] = round(t, 2)
                    p = projs[0]
                    obs["projectile"] = {"label": p.get("label"), "class": p.get("class"),
                                         "location": p.get("location")}
                for e in _enemies(st):
                    hp = _health(e)
                    label = e.get("label")
                    if hp is not None:
                        prev = obs["min_hp"].get(label)
                        obs["min_hp"][label] = hp if prev is None else min(prev, hp)
                    if label == obs["target"] and hp is not None and obs["target_hp0"] is not None \
                            and hp < obs["target_hp0"] and obs["hit_at"] is None:
                        obs["hit_at"], obs["hit_hp"] = round(t, 2), hp
                fb = _spell(st, FIREBALL["slot"]) or {}
                cd = _num(fb.get("cooldown_remaining_s"))
                if cd is not None and (obs["cooldown_max"] is None or cd > obs["cooldown_max"]):
                    obs["cooldown_max"] = cd
                if cd is not None and cd > 0 and obs["cooldown_at"] is None:
                    obs["cooldown_at"] = round(t, 2)
                for w in _cooldown_widgets(st):
                    for text in _texts(w):
                        if text not in obs["cooldown_texts"]:
                            obs["cooldown_texts"].append(text)
                    nums = _numeric_texts(w)
                    if nums and obs["cooldown_hud_at"] is None:
                        obs["cooldown_hud_at"] = round(t, 2)
                        obs["cooldown_hud"] = _widget_line(w)
                done = (obs["sp_at"] is not None and obs["projectile_at"] is not None
                        and obs["hit_at"] is not None and t >= COOLDOWN_WINDOW_S)
                if done or time.monotonic() >= deadline:
                    break
                time.sleep(POLL_S)
            sps = " -> ".join(_fmt(v) for v in obs["sp_values"])
            if obs["sp_at"] is not None and obs["sp_at"] <= SP_TIMEOUT_S:
                self.sp_log.append(("Fireball", sp0, want_sp))
                return ("PASS", f"tapped Q; spell_points {_fmt(sp0)} -> {_fmt(want_sp)} after "
                        f"{obs['sp_at']}s (values seen: {sps}; {obs['polls']} polls)", obs)
            return ("FAIL", f"tapped Q; spell_points did not drop {_fmt(sp0)} -> {_fmt(want_sp)} "
                    f"within {SP_TIMEOUT_S:g}s (values seen: {sps})", obs)

        cast_ok, _ = self.attempt("fireball_cast", cast)
        tapped = obs.get("tapped")
        why = "not run: Q was not tapped"

        def projectile(_b):
            at = obs.get("projectile_at")
            if at is not None and at <= PROJECTILE_WINDOW_S:
                p = obs["projectile"]
                return ("PASS", f"projectile '{p['label']}' ({p['class']}) at {p['location']} "
                        f"{at}s after the tap", p)
            return ("FAIL", f"no actors[] entry with kind==\"projectile\" within "
                    f"{PROJECTILE_WINDOW_S:g}s of the tap" + (f" (first seen at {at}s)"
                                                            if at is not None else "")
                    + f" ({obs.get('polls')} polls, radius {ACTOR_RADIUS})", None)

        self.measure("fireball_projectile", projectile, needs=tapped, needs_why=why)

        def hit(_b):
            target, hp0 = obs.get("target"), obs.get("target_hp0")
            mins = {k: v for k, v in (obs.get("min_hp") or {}).items()}
            damaged = {k: f"{obs['hp0'].get(k)} -> {v}" for k, v in mins.items()
                       if obs["hp0"].get(k) is not None and v < obs["hp0"][k]}
            if target is None:
                return "FAIL", "no enemy in actors before the cast", None
            if obs.get("hit_at") is not None and obs["hit_at"] <= HIT_WINDOW_S:
                dmg = hp0 - obs["hit_hp"]
                low = mins.get(target, obs["hit_hp"])
                return ("PASS", f"{target} health {hp0:g} -> {obs['hit_hp']:g} after "
                        f"{obs['hit_at']}s: {dmg:g} damage (lowest seen {low:g}; Fireball "
                        f"BasePower 25 Fire)", {"target": target, "damage": dmg})
            return ("FAIL", f"{target} (nearest enemy) health stayed {_fmt(hp0)} for "
                    f"{HIT_WINDOW_S:g}s after the tap (enemies damaged: {damaged or 'none'}; "
                    f"projectile seen: {obs.get('projectile_at') is not None})", None)

        self.measure("fireball_hits_enemy", hit, needs=tapped and walked,
                     needs_why=why if not tapped else "not run: walk_shot_line failed (the "
                     "Fireball was not aimed at an enemy)")
        self.shot("fireball_hit", needs=tapped, needs_why="not taken: Q was not tapped")

        sp0, max_sp = obs.get("sp0"), obs.get("max_sp")
        want_text = _sp_text(obs["want_sp"], max_sp) if cast_ok and max_sp is not None else None

        def sp_hud_has(st):
            return any(want_text in _texts(w) for w in _sp_bars(st))

        def sp_hud_describe(st):
            bars = _sp_bars(st)
            hitw = [w for w in bars if want_text in _texts(w)]
            if hitw:
                return f"'{want_text}' rendered by {_widget_line(hitw[0])}"
            return (f"expected '{want_text}' in a {SP_BAR} widget; {SP_BAR} widgets: "
                    f"{[_widget_line(w) for w in bars] or 'none'}")

        self.observe("sp_hud_updates", ["widgets"], sp_hud_has, sp_hud_describe,
                     timeout_s=SP_HUD_TIMEOUT_S, needs=cast_ok,
                     needs_why="not run: fireball_cast failed (SP did not drop)")

        def cooldown_state(_b):
            at, mx = obs.get("cooldown_at"), obs.get("cooldown_max")
            fb = _spell({"player": {"spells": self.loadout or []}}, FIREBALL["slot"]) or {}
            if at is not None and at <= COOLDOWN_WINDOW_S:
                return ("PASS", f"player.spells[0].cooldown_remaining_s={mx:g} {at}s after the "
                        "tap", None)
            return ("FAIL", f"player.spells[0].cooldown_remaining_s stayed {_fmt(mx, 2)} for "
                    f"{COOLDOWN_WINDOW_S:g}s after the tap (cooldown_tag "
                    f"{fb.get('cooldown_tag')!r}; AC-057.1 asks CooldownDuration=2.0)", None)

        self.measure("fireball_cooldown_state", cooldown_state, needs=tapped, needs_why=why)

        def cooldown_hud(_b):
            if obs.get("cooldown_hud_at") is not None:
                return ("PASS", f"{obs['cooldown_hud']} {obs['cooldown_hud_at']}s after the tap",
                        None)
            return ("FAIL", f"no {COOLDOWN_WIDGET} widget rendered a numeric remaining time "
                    f"after the tap (texts seen: {obs.get('cooldown_texts') or 'none'})", None)

        self.measure("cooldown_hud", cooldown_hud, needs=tapped, needs_why=why)

    # --- 11-12: Blink -------------------------------------------------------------------------
    def step_blink(self):
        laned, _ = self.walk("walk_blink_lane", *LANE_POINT)

        def blink():
            b = self.bridge
            st0 = b.state(["player"])
            xy0, sp0 = _xy(st0), _sp(st0)
            if xy0 is None:
                return "FAIL", f"no player location: {_player(st0).get('reason')}", None
            x0, y0 = xy0
            want_sp = None if sp0 is None else sp0 - BLINK["cost"]
            b.tap(BLINK["key"], hold_s=TAP_S)
            seen = {}

            def moved(st):
                xy, sp = _xy(st), _sp(st)
                seen.update(xy=xy, sp=sp)
                return (xy is not None and xy[0] - x0 > BLINK_MIN_DX and sp is not None
                        and want_sp is not None and sp == want_sp)

            try:
                b.wait_for(moved, timeout_s=BLINK_TIMEOUT_S, sections=["player"], poll_s=0.05)
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                xy = seen.get("xy") or (x0, y0)
                return ("FAIL", f"tapped E at ({x0:.0f}, {y0:.0f}): within {BLINK_TIMEOUT_S:g}s x "
                        f"moved {xy[0] - x0:+.0f} (needs > {BLINK_MIN_DX:g}) and spell_points "
                        f"{_fmt(sp0)} -> {_fmt(seen.get('sp'))} (expected {_fmt(want_sp)})", None)
            x1, y1 = seen["xy"]
            dx, dy = x1 - x0, y1 - y0
            self.blink_result = {"from": [x0, y0], "to": [x1, y1], "dx": dx, "dy": dy}
            self.sp_log.append(("Blink", sp0, want_sp))
            text = (f"tapped E: ({x0:.0f}, {y0:.0f}) -> ({x1:.0f}, {y1:.0f}), dx {dx:+.0f}, dy "
                    f"{dy:+.0f}; spell_points {_fmt(sp0)} -> {_fmt(want_sp)}")
            if abs(dy) >= BLINK_MAX_DY:
                return "FAIL", f"{text}: |dy| >= {BLINK_MAX_DY:g} (Blink should go +X)", None
            return "PASS", text, self.blink_result

        tried = {"done": False}

        def blink_once():
            tried["done"] = True
            return blink()

        self.attempt("blink", blink_once, needs=laned,
                     needs_why="not run: walk_blink_lane failed (not on the clear lane)")
        self.shot("after_blink", needs=tried["done"], needs_why="not taken: E was not tapped")

    # --- 13-14: Stone Skin ----------------------------------------------------------------
    def step_stone_skin(self):
        st = self.read(["player"])
        sp0 = _sp(st)
        affordable = sp0 is not None and sp0 >= STONE_SKIN["cost"]

        def stone_skin():
            b = self.bridge
            want_sp = sp0 - STONE_SKIN["cost"]
            b.tap(STONE_SKIN["key"], hold_s=TAP_S)
            self.stone_skin_t0 = time.monotonic()
            seen = {}

            def on(st1):
                seen.update(tags=_tags(st1), sp=_sp(st1))
                return STONE_SKIN_TAG in seen["tags"] and seen["sp"] == want_sp

            try:
                st1 = b.wait_for(on, timeout_s=STONE_SKIN_TIMEOUT_S, sections=["player"],
                                 poll_s=0.05)
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                return ("FAIL", f"tapped R: within {STONE_SKIN_TIMEOUT_S:g}s player.tags="
                        f"{seen.get('tags')} (expected {STONE_SKIN_TAG}), spell_points "
                        f"{_fmt(sp0)} -> {_fmt(seen.get('sp'))} (expected {_fmt(want_sp)})", None)
            self.sp_log.append(("Stone Skin", sp0, want_sp))
            return ("PASS", f"tapped R: {STONE_SKIN_TAG} on the player, spell_points {_fmt(sp0)} "
                    f"-> {_fmt(want_sp)} after {time.monotonic() - self.stone_skin_t0:.2f}s "
                    f"(tags {_tags(st1)})", None)

        skin_ok, _ = self.attempt("stone_skin", stone_skin, needs=affordable,
                                  needs_why=f"not run: spell_points is {_fmt(sp0)}, Stone Skin "
                                  f"costs {STONE_SKIN['cost']} (the SP plan broke earlier)")
        self.stone_skin_ok = skin_ok

        def describe(st1):
            w = _stone_skin_indicator(st1)
            if w is not None:
                return (f"{_widget_line(w)}: '{STONE_SKIN_TEXT}' with remaining duration "
                        f"{_numeric_texts(w)[0]}")
            partial = _stone_skin_widgets(st1)
            if partial:
                return (f"'{STONE_SKIN_TEXT}' rendered without a numeric duration: "
                        f"{[_widget_line(x) for x in partial]}")
            return (f"no widget renders '{STONE_SKIN_TEXT}'; status widgets: "
                    f"{[_widget_line(x) for x in _widgets(st1) if _matches(x, 'Status')]}")

        self.observe("stone_skin_indicator", ["widgets"],
                     lambda st1: _stone_skin_indicator(st1) is not None, describe,
                     timeout_s=INDICATOR_TIMEOUT_S, needs=skin_ok,
                     needs_why="not run: stone_skin failed (no Stone Skin tag)")
        self.shot("stone_skin", needs=skin_ok, needs_why="not taken: Stone Skin was not cast")

    # --- 15: Restoration refused ------------------------------------------------------------
    def step_restoration(self):
        st = self.read(["player"])
        sp0 = _sp(st)
        low = sp0 is not None and sp0 < RESTORATION["cost"]

        def refused():
            b = self.bridge
            b.tap(RESTORATION["key"], hold_s=TAP_S)
            t0 = time.monotonic()
            sp_values, restoration_seen, active_seen = [], False, False
            st1 = {}
            while True:
                st1 = b.state(["player"])
                sp = _sp(st1)
                if not sp_values or sp_values[-1] != sp:
                    sp_values.append(sp)
                restoration_seen |= RESTORATION_TAG in _tags(st1)
                active_seen |= bool((_spell(st1, RESTORATION["slot"]) or {}).get("active"))
                if time.monotonic() - t0 >= REFUSAL_WATCH_S:
                    break
                time.sleep(0.1)
            spell = _spell(st1, RESTORATION["slot"]) or {}
            afford = spell.get("can_afford")
            problems = []
            if any(v != sp0 for v in sp_values):
                problems.append(f"spell_points changed: {' -> '.join(_fmt(v) for v in sp_values)}")
            if restoration_seen:
                problems.append(f"{RESTORATION_TAG} was applied")
            if afford is not False:
                problems.append(f"player.spells[3].can_afford={afford!r} (expected false)")
            text = (f"tapped One at {_fmt(sp0)} SP (Restoration costs {RESTORATION['cost']}); for "
                    f"{REFUSAL_WATCH_S:g}s spell_points {' -> '.join(_fmt(v) for v in sp_values)}, "
                    f"{RESTORATION_TAG} {'seen' if restoration_seen else 'never seen'}, "
                    f"spells[3].active {'seen' if active_seen else 'never true'}, "
                    f"can_afford={afford!r}")
            if problems:
                return "FAIL", f"{'; '.join(problems)} ({text})", None
            return "PASS", text, None

        self.attempt("restoration_refused_low_sp", refused, needs=low,
                     needs_why=f"not run: spell_points is {_fmt(sp0)}, enough for Restoration "
                     f"({RESTORATION['cost']}): the SP plan broke earlier")
        self.shot("spell_hud")

    # --- 16: Stone Skin expires -------------------------------------------------------------
    def step_stone_skin_expires(self):
        def gone(st):
            return STONE_SKIN_TAG not in _tags(st) and not _stone_skin_widgets(st)

        def describe(st):
            since = (f"{time.monotonic() - self.stone_skin_t0:.1f}s after the cast"
                     if self.stone_skin_t0 else "")
            left = [_widget_line(w) for w in _stone_skin_widgets(st)]
            if gone(st):
                return f"{STONE_SKIN_TAG} and the '{STONE_SKIN_TEXT}' indicator are gone {since}"
            return (f"{since}: tag {'still on' if STONE_SKIN_TAG in _tags(st) else 'gone'}, "
                    f"'{STONE_SKIN_TEXT}' widgets {left or 'gone'} (Duration=10.0, AC-057.3)")

        self.observe("stone_skin_expires", ["player", "widgets"], gone, describe,
                     timeout_s=EXPIRE_TIMEOUT_S, needs=self.stone_skin_ok,
                     needs_why="not run: stone_skin failed (no Stone Skin tag)")

    # --- report hooks ---------------------------------------------------------------------
    def report_extra(self):
        return {"spell_points": [{"spell": s, "before": a, "after": b} for s, a, b in self.sp_log],
                "fireball": self.fireball, "blink": self.blink_result,
                "loadout": self.loadout, "notes": list(NOTES)}

    def md_sections(self):
        lines = ["", "## Spell points", "",
                 f"Plan: {' -> '.join(str(v) for v in SP_PLAN)} (Restoration refused at 1).", ""]
        if self.sp_log:
            for s, a, b in self.sp_log:
                lines.append(f"- {s}: {_fmt(a)} -> {_fmt(b)}")
        else:
            lines.append("- no cast changed spell points")
        obs = self.fireball or {}
        if obs.get("tapped"):
            lines += ["", "## Fireball observations (fast poll after Q)", "",
                      f"- target {obs.get('target')} health before {_fmt(obs.get('target_hp0'))}, "
                      f"lowest seen {obs.get('min_hp')}",
                      f"- SP reached {_fmt(obs.get('want_sp'))} at {obs.get('sp_at')}s; "
                      f"projectile first seen at {obs.get('projectile_at')}s "
                      f"({obs.get('projectile')}); hit at {obs.get('hit_at')}s",
                      f"- spells[0].cooldown_remaining_s max {obs.get('cooldown_max')}; cooldown "
                      f"widget texts {obs.get('cooldown_texts')}; {obs.get('polls')} polls"]
        lines += ["", "## Notes", ""] + [f"- {n}" for n in NOTES]
        return lines


Run = Us057Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us057Run, argv)


def main(argv=None):
    return kit.main(Us057Run, argv)


if __name__ == "__main__":
    sys.exit(main())
