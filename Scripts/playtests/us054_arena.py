#!/usr/bin/env python3
"""US-054 Arena Playtest -- Tier 4: the DevTestMap HUD and Frontliner AI, played with REAL input

The DevTestMap half of the US-054 arena loop (AC-054.13): the combat HUD and the three
Frontliners -- present, visible, with health bars and enemy indicators, aggroing, approaching
and damaging the player -- with the HUD, enemy, enemy AI, kill counter and enemy indicator
stories it is built from (US-050, US-051, US-052, US-063, US-064).  It drives the PACKAGED game
through the in-game agent bridge: every observation is a state read (state / wait_until) or a
screenshot, and the player only stands at the PlayerStart.  Nothing calls game functions;
bridge.jsonl in the run dir is the audit trail.  The run framework is Scripts/playtest_kit.py
(Specs/story_playtests_v1.md section 2); this file is the PLAN and the steps.

The rest of the loop -- walk, melee, dodge, take damage, Restoration, Burning, die, the death
screen, respawn, the arena reset and the kill counter -- moved to Scripts/playtests/
us054_combat.py on CombatTestMap (issue #46, T1).  On DevTestMap every Frontliner spawn is
inside AggroRange of the PlayerStart: the swarm killed the player on the walk to a Frontliner and
re-aggroed onto the respawned player before the arena reset could be read.  US-054 owns both
scripts; each has its own Steps table in the story's Playtest section.

Steps (--plan prints them as the story's Steps table):
  1-2  The combat HUD: health, stamina, posture, SP bars and the kill counter on screen, reading
       "Kills: 0", laid out top-left (health, then stamina below it, then posture below that).
  3-8  The three Frontliners (actors[].kind == "enemy", read with radius 3000, never by label):
       200/200 HP, spread out, visible meshes, floating health bars, enemy indicators, aggro
       (ai_state leaves Idle within 3 s) and approach (one leaves its spawn within 6 s).
  9    Stand at the PlayerStart: the Frontliners damage the player (health below max).
A failed check SKIPs the checks that depend on it (needs=); independent checks still run.

Usage:
  py Scripts/playtests/us054_arena.py                    fresh package: launch, play, quit
  py Scripts/playtests/us054_arena.py --allow-stale      STALE package: diagnostic only
  py Scripts/playtests/us054_arena.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us054_arena.py --plan             print the Steps table; no game
  py Scripts/playtests/us054_arena.py --attach-port N [--token T]   (always diagnostic)

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us054/ (report.md, report.json, game.log,
bridge.jsonl, bridge_NN.png).  Open every screenshot with the Read tool.

Exit codes: 0 every check passed (WARN allowed); 1 a gameplay check FAILED; 2 infrastructure.
"""

import math
import os
import re
import sys

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-054"
TITLE = "US-054 arena enemies playtest (Tier 4, real input)"
RUN_SUFFIX = "us054"  # us054_combat.py is "us054combat": two scripts of one story

# --- the arena (DevTestMap ground truth) ------------------------------------------------------
# BP_MordecaiEnemy_Frontliner placements.  Enemies are matched to these by position (nearest
# spawn), never by label.
FRONTLINER_SPAWNS = ((600.0, 0.0), (-400.0, 500.0), (-400.0, -500.0))
ENEMY_MAX_HEALTH = 200.0               # AC-054.2
ENEMY_RADIUS = 3000                    # actors radius: the whole arena
MIN_ENEMY_SPACING = 300.0              # "varied positions" (AC-054.11)
DISPLACED_CM = 100.0                   # an enemy this far from every spawn has moved

# --- widget classes, texts ----------------------------------------------------------------------
HUD_BARS = ("HealthBar", "StaminaBar", "PostureBar", "SpellPointsBar", "KillCounter")
ENEMY_BAR_CLASS = "EnemyHealthBar"
INDICATOR_CLASS = "EnemyIndicator"
RE_KILLS = re.compile(r"^\s*Kills:\s*(\d+)\s*$")

# --- timeouts (module-level so the offline tests can shorten them) ------------------------------
HUD_TIMEOUT_S = 5.0
INDICATOR_TIMEOUT_S = 3.0
AGGRO_TIMEOUT_S = 3.0
APPROACH_TIMEOUT_S = 6.0
DAMAGE_TIMEOUT_S = 20.0

SCREENSHOTS = {
    "start": "the Mordecai diorama arena from the fixed overhead camera, the player at the centre, "
             "the combat HUD (health, stamina, posture and SP bars top-left, 'Kills: 0') and "
             "the three Frontliners: a red cylinder body under each floating enemy health bar",
    "hit": "the player at the arena centre after the first Frontliner hit: the health bar below "
           "full, Frontliners (red cylinders) next to the player",
}
KNOWN_ISSUES = ()

PLAN = (
    Step("hud_visible", "state widgets -> HealthBar, StaminaBar, PostureBar, SpellPointsBar and "
         "KillCounter widgets on screen, a text 'Kills: 0'",
         proves=("AC-054.7", "AC-054.8", "US-063:AC-063.3", "US-063:AC-063.4")),
    Step("hud_layout", "state widgets rects -> HealthBar top-left (x<300, y<60), StaminaBar "
         "below it, PostureBar below that", proves=("AC-054.7",)),
    Step("enemies_present", f"state actors (radius {ENEMY_RADIUS}) -> 3 kind=='enemy', health "
         f"{{{ENEMY_MAX_HEALTH:g}, {ENEMY_MAX_HEALTH:g}}}, pairwise > {MIN_ENEMY_SPACING:g} cm apart",
         proves=("AC-054.11", "AC-054.2", "US-050:AC-050.3")),
    Step("enemies_visible", "state actors -> every enemy has_visible_mesh==true",
         proves=("AC-054.13",)),
    Step("enemy_health_bars", f"state actors -> every enemy has a visible widget whose class "
         f"contains {ENEMY_BAR_CLASS}", proves=("AC-054.9", "US-052:AC-052.10")),
    Step("enemy_indicators", f"state widgets -> one {INDICATOR_CLASS} widget per enemy in aggro "
         f"range within {INDICATOR_TIMEOUT_S:g}s",
         proves=("US-064:AC-064.2", "US-064:AC-064.10")),
    Step("enemy_aggro", f"stand at the spawn -> every enemy ai_state != 'Idle' within "
         f"{AGGRO_TIMEOUT_S:g}s", proves=("US-051:AC-051.3",)),
    Step("enemy_approaches", f"stand -> some enemy > {DISPLACED_CM:g} cm from every spawn within "
         f"{APPROACH_TIMEOUT_S:g}s", proves=("US-051:AC-051.4",)),
    Step("enemy_damages_player", "stand at the spawn -> wait_until player.attributes.health "
         f"< player.attributes.max_health within {DAMAGE_TIMEOUT_S:g}s",
         proves=("AC-054.13", "US-051:AC-051.5")),
)


# ---------------------------------------------------------------------------
# State helpers (pure: the tests call them too)
# ---------------------------------------------------------------------------
def _list(v):
    return [x for x in v if isinstance(x, dict)] if isinstance(v, list) else []


def _num(v):
    return float(v) if isinstance(v, (int, float)) and not isinstance(v, bool) else None


def widgets(st):
    """state.widgets as a list of dicts (an invalid section is an object: [])."""
    return _list((st or {}).get("widgets"))


def widget_visible(w):
    """state.widgets lists only visible widgets; honour an explicit visible:false anyway."""
    return w.get("visible", True) is not False


def hud_widgets(st, name):
    """Visible player-HUD widgets whose class contains `name` (case-insensitive); enemy widgets
    (EnemyHealthBar contains HealthBar) are excluded."""
    n = name.lower()
    return [w for w in widgets(st) if widget_visible(w)
            and n in str(w.get("class") or "").lower()
            and "enemy" not in str(w.get("class") or "").lower()]


def all_texts(st):
    out = []
    for w in widgets(st):
        if widget_visible(w):
            out += [str(t) for t in (w.get("texts") or []) if t is not None]
    return out


def kill_counts(st):
    """Every N of a 'Kills: N' text on screen."""
    out = []
    for t in all_texts(st):
        m = RE_KILLS.match(t)
        if m:
            out.append(int(m.group(1)))
    return out


def xy(loc):
    if isinstance(loc, (list, tuple)) and len(loc) >= 2:
        x, y = _num(loc[0]), _num(loc[1])
        if x is not None and y is not None:
            return x, y
    return None


def dist(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def enemies(st):
    """actors[] entries with kind == 'enemy' that have a location."""
    return [a for a in _list((st or {}).get("actors"))
            if a.get("kind") == "enemy" and xy(a.get("location")) is not None]


def enemy_health(a):
    h = a.get("health") if isinstance(a.get("health"), dict) else {}
    return _num(h.get("current")), _num(h.get("max"))


def nearest_spawn(pos):
    return min(range(len(FRONTLINER_SPAWNS)), key=lambda i: dist(pos, FRONTLINER_SPAWNS[i]))


def displacement(a):
    """Distance from the enemy to the nearest Frontliner spawn."""
    pos = xy(a.get("location"))
    return dist(pos, FRONTLINER_SPAWNS[nearest_spawn(pos)])


def fmt_xy(p):
    return f"({p[0]:.0f}, {p[1]:.0f})" if p else "(?)"


def describe_enemies(es):
    parts = []
    for a in es:
        cur, mx = enemy_health(a)
        parts.append(f"{fmt_xy(xy(a.get('location')))} hp {cur}/{mx} ai={a.get('ai_state')} "
                     f"dead={a.get('is_dead')} mesh={a.get('has_visible_mesh')}")
    return "; ".join(parts) if parts else "no enemy in actors[]"


class Us054Run(kit.PlaytestRun):
    """One run of the playtest.  execute() returns the exit code."""

    STORY = STORY
    COVERS = ("US-050", "US-051", "US-052", "US-063", "US-064")
    FEATURES = ("enemies", "enemy_ai", "combat_hud", "kill_counter", "enemy_indicators")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.DEFAULT_MAP  # DevTestMap: the Frontliner AI half of US-054
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.enemies_start = []      # [(x, y)] read in enemies_present
        self.max_displacement = 0.0  # the largest enemy displacement seen

    # --- the run ------------------------------------------------------------------------------
    def steps(self):
        self.step_hud()
        self.step_enemies()
        self.step_damage()

    def _note_displacement(self, st):
        for a in enemies(st):
            self.max_displacement = max(self.max_displacement, displacement(a))

    # --- 1-2: the HUD ----------------------------------------------------------------------
    def step_hud(self):
        def have_all(st):
            return all(hud_widgets(st, n) for n in HUD_BARS) and set(kill_counts(st)) == {0}

        def describe(st):
            found = {n: [w.get("class") for w in hud_widgets(st, n)] for n in HUD_BARS}
            missing = [n for n, ws in found.items() if not ws]
            return (f"HUD widgets {found}; missing {missing or 'none'}; 'Kills: N' texts "
                    f"{kill_counts(st)} (visible classes: "
                    f"{[w.get('class') for w in widgets(st)]})")

        self.observe("hud_visible", ["widgets"], have_all, describe, timeout_s=HUD_TIMEOUT_S)

        def layout(b):
            s = b.state(["widgets"])
            rects = {}
            for n in ("HealthBar", "StaminaBar", "PostureBar"):
                ws = [w for w in hud_widgets(s, n) if isinstance(w.get("rect"), list)
                      and len(w["rect"]) >= 4]
                if not ws:
                    return "FAIL", f"no {n} widget with a rect on screen", None
                rects[n] = min(ws, key=lambda w: w["rect"][1])["rect"]
            h, s_, p = rects["HealthBar"], rects["StaminaBar"], rects["PostureBar"]
            problems = []
            if not (h[0] < 300 and h[1] < 60):
                problems.append(f"HealthBar at ({h[0]}, {h[1]}) is not top-left (x<300, y<60)")
            if not s_[1] > h[1]:
                problems.append(f"StaminaBar y={s_[1]} is not below HealthBar y={h[1]}")
            if not p[1] > s_[1]:
                problems.append(f"PostureBar y={p[1]} is not below StaminaBar y={s_[1]}")
            shown = ", ".join(f"{n} {r}" for n, r in rects.items())
            return ("FAIL" if problems else "PASS",
                    ("; ".join(problems) + f" ({shown})") if problems else shown, rects)

        self.measure("hud_layout", layout)

    # --- 3-8: the enemies ------------------------------------------------------------------
    def step_enemies(self):
        def present(b):
            st = b.state(["actors"], radius=ENEMY_RADIUS)
            es = enemies(st)
            self.enemies_start = [xy(a["location"]) for a in es]
            problems = []
            if len(es) != 3:
                problems.append(f"{len(es)} enemies (expected 3)")
            for a in es:
                cur, mx = enemy_health(a)
                if cur != ENEMY_MAX_HEALTH or mx != ENEMY_MAX_HEALTH:
                    problems.append(f"enemy at {fmt_xy(xy(a['location']))} health {cur}/{mx} "
                                    f"(expected {ENEMY_MAX_HEALTH:g}/{ENEMY_MAX_HEALTH:g})")
            pts = self.enemies_start
            for i in range(len(pts)):
                for j in range(i + 1, len(pts)):
                    if dist(pts[i], pts[j]) <= MIN_ENEMY_SPACING:
                        problems.append(f"enemies at {fmt_xy(pts[i])} and {fmt_xy(pts[j])} are "
                                        f"only {dist(pts[i], pts[j]):.0f} cm apart")
            detail = (f"spawn locations {[fmt_xy(p) for p in pts]}; {describe_enemies(es)}")
            return ("FAIL" if problems else "PASS",
                    ("; ".join(problems) + "; " + detail) if problems else detail, es)

        self.measure("enemies_present", present)
        have = bool(self.enemies_start)
        why = "not run: no enemy in actors[]"

        def visible(b):
            es = enemies(b.state(["actors"], radius=ENEMY_RADIUS))
            bad = [a for a in es if a.get("has_visible_mesh") is not True]
            return ("FAIL" if bad or not es else "PASS",
                    f"{len(es) - len(bad)}/{len(es)} enemies have a visible mesh; "
                    f"{describe_enemies(es)}", None)

        self.measure("enemies_visible", visible, needs=have, needs_why=why)

        def bars(b):
            es = enemies(b.state(["actors"], radius=ENEMY_RADIUS))
            bad = []
            for a in es:
                ws = [w for w in _list(a.get("widgets"))
                      if ENEMY_BAR_CLASS.lower() in str(w.get("class") or "").lower()
                      and w.get("visible") is True]
                if not ws:
                    bad.append(f"{fmt_xy(xy(a['location']))} widgets "
                               f"{[(w.get('class'), w.get('visible')) for w in _list(a.get('widgets'))]}")
            if bad or not es:
                return "FAIL", f"no visible {ENEMY_BAR_CLASS} on: {bad or 'no enemies'}", None
            return "PASS", f"all {len(es)} enemies have a visible {ENEMY_BAR_CLASS} widget", None

        self.measure("enemy_health_bars", bars, needs=have, needs_why=why)

        want = max(1, len(self.enemies_start))

        def indicators(st):
            return len([w for w in widgets(st)
                        if INDICATOR_CLASS.lower() in str(w.get("class") or "").lower()
                        and widget_visible(w)]) >= want

        def ind_detail(st):
            got = [w.get("class") for w in widgets(st)
                   if INDICATOR_CLASS.lower() in str(w.get("class") or "").lower()]
            return (f"{len(got)} {INDICATOR_CLASS} widget(s) on screen (expected >= {want}, one "
                    f"per enemy in aggro range): {got}")

        self.observe("enemy_indicators", ["widgets"], indicators, ind_detail,
                     timeout_s=INDICATOR_TIMEOUT_S, needs=have, needs_why=why)

        def aggro(st):
            es = enemies(st)
            self._note_displacement(st)
            return bool(es) and all(a.get("ai_state") not in (None, "Idle") for a in es)

        self.observe("enemy_aggro", ["actors"], aggro,
                     lambda st: f"ai_state per enemy: {describe_enemies(enemies(st))}",
                     timeout_s=AGGRO_TIMEOUT_S, radius=ENEMY_RADIUS, needs=have, needs_why=why)

        def approached(st):
            self._note_displacement(st)
            return any(displacement(a) > DISPLACED_CM for a in enemies(st))

        self.observe("enemy_approaches", ["actors"], approached,
                     lambda st: ("max distance from a spawn: "
                                 f"{max([displacement(a) for a in enemies(st)] or [0]):.0f} cm; "
                                 f"{describe_enemies(enemies(st))}"),
                     timeout_s=APPROACH_TIMEOUT_S, radius=ENEMY_RADIUS, needs=have,
                     needs_why=why)
        self.shot("start")

    # --- 9: the Frontliners damage the player ------------------------------------------------
    def step_damage(self):
        ok, _ = self.expect("enemy_damages_player",
                            "player.attributes.health<player.attributes.max_health",
                            timeout_s=DAMAGE_TIMEOUT_S)
        self.shot("hit", needs=ok, needs_why="not taken: the player was never hit")
        return ok

    # --- report hooks -------------------------------------------------------------------------
    def report_extra(self):
        return {"enemies_start": self.enemies_start,
                "max_enemy_displacement": round(self.max_displacement, 1)}

    def md_sections(self):
        return ["", "## Enemies", "",
                f"- At the start: {[fmt_xy(p) for p in self.enemies_start] or 'not read'}",
                f"- Largest distance from a spawn seen: {self.max_displacement:.0f} cm"]


Run = Us054Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us054Run, argv)


def main(argv=None):
    return kit.main(Us054Run, argv)


if __name__ == "__main__":
    sys.exit(main())
