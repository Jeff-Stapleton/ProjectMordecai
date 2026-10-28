#!/usr/bin/env python3
"""US-054 Arena Playtest -- Tier 4: the playable arena loop, played with REAL input

The arena loop of US-054 (AC-054.13: launch -> HUD -> enemies -> attack -> take damage -> die ->
respawn -> enemies reset), with the HUD, enemy AI, death/respawn, kill counter, enemy indicator
and status-effect stories it is built from (US-050, US-051, US-052, US-053, US-057, US-062,
US-063, US-064).  It drives the PACKAGED game through the in-game agent bridge: every action is a
key or mouse event injected into Slate (key / click), every observation is a state read
(state / wait_until) or a screenshot.  Nothing calls game functions; bridge.jsonl in the run dir
is the audit trail.  The run framework is Scripts/playtest_kit.py (Specs/story_playtests_v1.md
section 2); this file is the PLAN and the steps.

Steps (--plan prints them as the story's Steps table):
  1-2  The combat HUD: health, stamina, posture, SP bars and the kill counter on screen, reading
       "Kills: 0", laid out top-left (health, then stamina below it, then posture below that).
  3-8  The three Frontliners (actors[].kind == "enemy", read with radius 3000, never by label):
       200/200 HP, spread out, visible meshes, floating health bars, enemy indicators, aggro
       (ai_state leaves Idle within 3 s) and approach (one leaves its spawn within 6 s).
  9    Walk next to the Frontliner nearest (-400, -500) by a route clear of the pickups.
  10-11 LMB (light attack) and LeftAlt (dodge).
  12-14 Stand there: the enemies damage the player, Burning may proc (40%: WARN if never seen),
       tap One (Restoration): health rises, SP drops by 3, the Restoration tag is on.
  15-18 Keep standing until the player dies: the death screen, no movement while dead, the
       respawn at the PlayerStart with full health.
  19-20 The arena reset (enemies back at their spawns, full health, alive) and movement after
       the respawn.  (The arena reset is checked BEFORE moving: the enemies re-aggro at once.)
  21   The kill counter's text equals player.kills (the authoritative GameMode count).
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
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-054"
TITLE = "US-054 playable arena playtest (Tier 4, real input)"
RUN_SUFFIX = "us054"

# --- the arena (DevTestMap ground truth) ------------------------------------------------------
SPAWN = (0.0, 0.0)
# BP_MordecaiEnemy_Frontliner placements: the reset target.  Enemies are matched to these by
# position (nearest spawn), never by label.
FRONTLINER_SPAWNS = ((600.0, 0.0), (-400.0, 500.0), (-400.0, -500.0))
TARGET_SPAWN = (-400.0, -500.0)        # the Frontliner this run walks to
ENEMY_MAX_HEALTH = 200.0               # AC-054.2
ENEMY_RADIUS = 3000                    # actors radius: the whole arena
MIN_ENEMY_SPACING = 300.0              # "varied positions" (AC-054.11)
DISPLACED_CM = 100.0                   # an enemy this far from every spawn has moved
RESPAWN_RADIUS = 150.0
ATTACK_STANDOFF = 160.0                # stop this far in front (-X) of the enemy

# --- tags, widget classes, texts ----------------------------------------------------------------
TAG_DEAD = "Mordecai.State.Dead"
TAG_ATTACKING = "Mordecai.State.Attacking"
TAG_DODGE_SUBSTR = "dodg"              # Mordecai.State.Dodging (case-insensitive substring)
TAG_BURNING = "Mordecai.Status.Burning"
TAG_RESTORATION = "Mordecai.Status.Restoration"
HUD_BARS = ("HealthBar", "StaminaBar", "PostureBar", "SpellPointsBar", "KillCounter")
ENEMY_BAR_CLASS = "EnemyHealthBar"
INDICATOR_CLASS = "EnemyIndicator"
RE_KILLS = re.compile(r"^\s*Kills:\s*(\d+)\s*$")
RE_COUNTDOWN = re.compile(r"\d")
YOU_DIED = "You Died"
BURNING_TEXT = "Burning"
RESTORATION_KEY = "One"
RESTORATION_SP = 3.0

# --- timeouts (module-level so the offline tests can shorten them) ------------------------------
HUD_TIMEOUT_S = 5.0
INDICATOR_TIMEOUT_S = 3.0
AGGRO_TIMEOUT_S = 3.0
APPROACH_TIMEOUT_S = 6.0
ATTACK_TIMEOUT_S = 1.0
DODGE_TIMEOUT_S = 0.5
DAMAGE_TIMEOUT_S = 20.0
BURNING_TIMEOUT_S = 25.0
RESTORATION_TIMEOUT_S = 6.0
DEATH_TIMEOUT_S = 60.0
DEATH_SCREEN_TIMEOUT_S = 1.5
DEAD_HOLD_S = 0.5
RESPAWN_DELAY_S = 3.0                  # AMordecaiGameMode::RespawnDelaySeconds (AC-053.4)
RESPAWN_MARGIN_S = 0.3                 # never hold W into the respawn (it would spoil `respawn`)
DEAD_MAX_MOVE_CM = 5.0
RESPAWN_TIMEOUT_S = 6.0
RESET_TIMEOUT_S = 2.0
MOVE_HOLD_S = 0.4
MOVE_MIN_CM = 80.0
POLL_S = 0.1

SCREENSHOTS = {
    "start": "the Mordecai diorama arena from the fixed overhead camera, the player at the centre, "
             "the combat HUD (health, stamina, posture and SP bars top-left, 'Kills: 0') and "
             "three floating enemy health bars with NO enemy bodies under them (invisible "
             "Frontliners, known issue)",
    "near_enemy": "the player in the lower-left part of the arena, next to the floating health "
                  "bar of the Frontliner placed at (-400, -500)",
    "hit": "the player after the first enemy hit: the health bar below full",
    "dead": "the player dead (health bar empty); a 'You Died' death screen with a countdown if "
            "the death screen exists (known issue N2: it is never created)",
    "respawned": "the player back at the arena centre with a full health bar, the three enemy "
                 "health bars back at their spawn points",
}
KNOWN_ISSUES = (
    "Block and parry on the mouse (AC-054.6: RMB hold, RMB+LMB) are not tested: they need the "
    "right mouse button held across state reads, and the playtest client's click holds a button "
    "only inside one blocking command (hold_s), so nothing can be read while it is held (B2). "
    "The gamepad path (LT hold, LT+RT) is played by Scripts/playtests/us083_combat_input.py.",
    "The Frontliners have no mesh (invisible enemies, Known issues backlog: 'The three red bars "
    "are explained'); enemies_visible is expected to FAIL.",
    "The death screen is never created (Known issue N2); death_screen is expected to FAIL.",
    "The run lands one light attack and kills no enemy, so kill_counter_text_matches can only "
    "compare 'Kills: 0' with player.kills == 0 (the kill path is US-086's).",
    "Suspected NEW (unverified): in a 60 s us079 run the enemies never damaged the player; "
    "enemy_approaches and enemy_damages_player carry no known_issue.",
)
KI_INVISIBLE = ("Known issues backlog, 'The three red bars are explained': the three "
                "Frontliners have no mesh, so the enemies are invisible")
KI_DEATH_SCREEN = "Known issue N2: the death screen is never created"

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
         proves=("AC-054.13",), known_issue=KI_INVISIBLE),
    Step("enemy_health_bars", f"state actors -> every enemy has a visible widget whose class "
         f"contains {ENEMY_BAR_CLASS}", proves=("AC-054.9", "US-052:AC-052.10")),
    Step("enemy_indicators", f"state widgets -> one {INDICATOR_CLASS} widget per enemy in aggro "
         f"range within {INDICATOR_TIMEOUT_S:g}s",
         proves=("US-064:AC-064.2", "US-064:AC-064.10")),
    Step("enemy_aggro", f"stand at the spawn -> every enemy ai_state != 'Idle' within "
         f"{AGGRO_TIMEOUT_S:g}s", proves=("US-051:AC-051.3",)),
    Step("enemy_approaches", f"stand -> some enemy > {DISPLACED_CM:g} cm from every spawn within "
         f"{APPROACH_TIMEOUT_S:g}s", proves=("US-051:AC-051.4",)),
    Step("walk_to_frontliner3", "walk_to(-150, -650), walk_to(ex-300, -650), walk_to(ex-160, ey) "
         "where (ex, ey) is the live enemy nearest (-400, -500) -> within 60 cm of each"),
    Step("melee_lmb", f"click Left at (640, 360) -> within {ATTACK_TIMEOUT_S:g}s "
         f"'{TAG_ATTACKING}' in player.tags or the nearest enemy's health below its value "
         "before the click",
         proves=("AC-054.6", "AC-054.5")),
    Step("dodge_lalt", f"tap LeftAlt -> within {DODGE_TIMEOUT_S:g}s a Dodging tag in player.tags "
         "or a displacement > 150 cm", proves=("AC-054.6", "AC-054.5")),
    Step("enemy_damages_player", "stand next to the enemy -> wait_until player.attributes.health "
         f"< player.attributes.max_health within {DAMAGE_TIMEOUT_S:g}s",
         proves=("AC-054.13", "US-051:AC-051.5")),
    Step("burning_on_hit", f"keep standing -> within {BURNING_TIMEOUT_S:g}s '{TAG_BURNING}' in "
         f"player.tags and a widget text '{BURNING_TEXT}' (WARN if Burning never procs: 40%)",
         proves=("US-057:AC-057.11", "US-057:AC-057.8")),
    Step("restoration_heals", f"tap {RESTORATION_KEY} -> within {RESTORATION_TIMEOUT_S:g}s "
         f"health rises, spell_points drops by {RESTORATION_SP:g} and '{TAG_RESTORATION}' is in "
         "player.tags", proves=("US-057:AC-057.4", "US-057:AC-057.5", "US-057:AC-057.12")),
    Step("player_dies", f"keep standing -> wait_until player.is_dead==true within "
         f"{DEATH_TIMEOUT_S:g}s, '{TAG_DEAD}' in player.tags", proves=("US-053:AC-053.1",)),
    Step("death_screen", f"state widgets -> within {DEATH_SCREEN_TIMEOUT_S:g}s one widget with "
         f"'{YOU_DIED}', a countdown digit and 'Kills: N'",proves=("US-062:AC-062.1", "US-062:AC-062.2"),
         known_issue=KI_DEATH_SCREEN),
    Step("dead_no_move", f"while dead: hold W {DEAD_HOLD_S:g}s -> moved < {DEAD_MAX_MOVE_CM:g} cm",
         proves=("US-053:AC-053.2",)),
    Step("respawn", f"wait -> within {RESPAWN_TIMEOUT_S:g}s player.is_dead==false, health == "
         f"max_health, within {RESPAWN_RADIUS:g} cm of (0, 0), no '{TAG_DEAD}' tag",
         proves=("US-053:AC-053.4", "US-053:AC-053.5")),
    Step("arena_reset", f"state actors at the respawn -> every enemy within {DISPLACED_CM:g} cm "
         "of a distinct spawn, health == max, not dead (WARN if no enemy had moved before the "
         "death: nothing to reset)", proves=("US-053:AC-053.8", "US-053:AC-053.9")),
    Step("respawn_moves", f"hold W {MOVE_HOLD_S:g}s -> player x grows by >= {MOVE_MIN_CM:g} cm",
         proves=("US-053:AC-053.6",)),
    Step("kill_counter_text_matches", "state widgets, player -> the 'Kills: N' text equals "
         "player.kills", proves=("US-063:AC-063.1",)),
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


def player(st):
    p = (st or {}).get("player")
    return p if isinstance(p, dict) and p.get("valid") is True else None


def tags(p):
    return [str(t) for t in ((p or {}).get("tags") or [])]


def attr(p, name):
    return _num(((p or {}).get("attributes") or {}).get(name))


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


def enemy_nearest(st, point):
    es = enemies(st)
    return min(es, key=lambda a: dist(xy(a.get("location")), point)) if es else None


def same_enemy(st, name, point):
    """The enemy called `name` (actors[].name); without a name, the one nearest `point`.  None
    when the named enemy is not in the state (another enemy never stands in for it)."""
    if name:
        return next((a for a in enemies(st) if a.get("name") == name), None)
    return enemy_nearest(st, point)


def fmt_xy(p):
    return f"({p[0]:.0f}, {p[1]:.0f})" if p else "(?)"


def describe_enemies(es):
    parts = []
    for a in es:
        cur, mx = enemy_health(a)
        parts.append(f"{fmt_xy(xy(a.get('location')))} hp {cur}/{mx} ai={a.get('ai_state')} "
                     f"dead={a.get('is_dead')} mesh={a.get('has_visible_mesh')}")
    return "; ".join(parts) if parts else "no enemy in actors[]"


def reset_problems(st):
    """Why the enemies are NOT reset ([] = every enemy at a distinct spawn, full, alive)."""
    es = enemies(st)
    problems = []
    if len(es) != len(FRONTLINER_SPAWNS):
        problems.append(f"{len(es)} enemies (expected {len(FRONTLINER_SPAWNS)})")
    used = set()
    for a in es:
        pos = xy(a.get("location"))
        i = nearest_spawn(pos)
        d = dist(pos, FRONTLINER_SPAWNS[i])
        cur, mx = enemy_health(a)
        if d > DISPLACED_CM:
            problems.append(f"enemy at {fmt_xy(pos)} is {d:.0f} cm from the nearest spawn")
        elif i in used:
            problems.append(f"two enemies at spawn {fmt_xy(FRONTLINER_SPAWNS[i])}")
        used.add(i)
        if cur is None or mx is None or cur < mx or mx != ENEMY_MAX_HEALTH:
            problems.append(f"enemy at {fmt_xy(pos)} health {cur}/{mx}")
        if a.get("is_dead") is not False:
            problems.append(f"enemy at {fmt_xy(pos)} is_dead={a.get('is_dead')}")
    return problems


class Us054Run(kit.PlaytestRun):
    """One run of the playtest.  execute() returns the exit code."""

    STORY = STORY
    COVERS = ("US-050", "US-051", "US-052", "US-053", "US-057", "US-062", "US-063", "US-064")
    FEATURES = ("enemies", "enemy_ai", "combat_hud", "melee_attack", "dodge", "death_respawn",
                "kill_counter", "enemy_indicators", "status_effects")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.enemies_start = []      # [(x, y)] read in enemies_present
        self.enemies_at_death = []   # [(x, y)] read right after the death
        self.max_displacement = 0.0  # the largest enemy displacement seen before the death
        self.respawn_state = None
        self.died_at = None          # time.monotonic() when player_dies saw the death

    # --- the run ------------------------------------------------------------------------------
    def steps(self):
        self.step_hud()
        self.step_enemies()
        walked = self.step_walk()
        self.step_attacks(walked)
        damaged = self.step_damage()
        died = self.step_death(damaged)
        self.step_respawn(died)
        self.step_kill_counter()

    def _poll(self, sections, fn, timeout_s, radius=ENEMY_RADIUS):
        """Read state every POLL_S until fn(state) returns True or timeout_s passes.  Returns
        (done, last_state).  fn may record what it saw (it is called on every read)."""
        deadline = time.monotonic() + timeout_s
        while True:
            st = self.bridge.state(sections, radius=radius)
            if fn(st):
                return True, st
            if time.monotonic() >= deadline:
                return False, st
            time.sleep(POLL_S)

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

        ok, st = self.observe("hud_visible", ["widgets"], have_all, describe,
                              timeout_s=HUD_TIMEOUT_S)

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

    # --- 9: walk to the Frontliner --------------------------------------------------------
    def step_walk(self):
        def walk(b):
            target = enemy_nearest(b.state(["actors"], radius=ENEMY_RADIUS), TARGET_SPAWN)
            if target is None:
                return "FAIL", "no enemy in actors[] to walk to", None
            ex, ey = xy(target["location"])
            legs = ((-150.0, -650.0), (ex - 300.0, -650.0), (ex - ATTACK_STANDOFF, ey))
            done = []
            for i, (x, y) in enumerate(legs, 1):
                try:
                    r = b.walk_to(x, y, tol=self.WALK_TOL)
                except ab.BridgeError as e:
                    if e.kind != "gameplay":
                        raise
                    return ("FAIL", f"leg {i} walk_to({x:.0f}, {y:.0f}) failed: {e} (enemy at "
                            f"({ex:.0f}, {ey:.0f}); legs done: {done})", None)
                done.append(f"({r['final'][0]:.0f}, {r['final'][1]:.0f})")
            return ("PASS", f"enemy nearest (-400, -500) at ({ex:.0f}, {ey:.0f}); reached "
                    f"{' -> '.join(done)}", {"enemy": [ex, ey]})

        ok, _ = self.measure("walk_to_frontliner3", walk)
        self.shot("near_enemy")
        return ok

    # --- 10-11: attack and dodge --------------------------------------------------------------
    def step_attacks(self, walked):
        def melee(b):
            st0 = b.state(["player", "actors"], radius=ENEMY_RADIUS)
            p0 = player(st0)
            if p0 is None:
                return "FAIL", "no valid player before the click", None
            pos0 = xy(p0.get("location"))
            # Damage is judged against the nearest enemy's health BEFORE the click, never against
            # the AC's max: in the 8ea853b sweep the cooked enemies had 100 max health (N13, issue
            # #18), which passed a "< 200" check with no attack at all.
            # The same enemy is read after the click (by name), so another, already damaged enemy
            # that becomes the nearest cannot pass the check.
            e0 = enemy_nearest(st0, pos0)
            name0 = e0.get("name") if e0 else None
            h0 = (enemy_health(e0) or (None, None))[0] if e0 else None
            r = b.click(640, 360, button="Left")
            seen = {}

            def hit(st):
                p = player(st)
                e = same_enemy(st, name0, xy((p or {}).get("location")) or pos0)
                seen["tags"] = tags(p)
                seen["enemy"] = enemy_health(e) if e else None
                cur = (seen["enemy"] or (None, None))[0]
                return TAG_ATTACKING in seen["tags"] or (cur is not None and h0 is not None
                                                         and cur < h0)

            done, _ = self._poll(["player", "actors"], hit, ATTACK_TIMEOUT_S)
            what = (f"Left click at (640, 360), Slate hit {r.get('hit')}; player.tags "
                    f"{seen.get('tags')}, nearest enemy health {seen.get('enemy')}"
                    + ("" if walked else " (the walk to the enemy failed)"))
            if done:
                return "PASS", what, None
            return ("FAIL", f"{what}: no '{TAG_ATTACKING}' tag and no enemy damage within "
                    f"{ATTACK_TIMEOUT_S:g}s", None)

        self.measure("melee_lmb", melee)

        def dodge(b):
            p0 = player(b.state(["player"]))
            if p0 is None:
                return "FAIL", "no valid player before LeftAlt", None
            pos0 = xy(p0.get("location"))
            b.tap("LeftAlt", hold_s=self.TAP_S)
            seen = {"moved": 0.0, "tags": []}

            def dodged(st):
                p = player(st)
                pos = xy((p or {}).get("location"))
                if pos:
                    seen["moved"] = max(seen["moved"], dist(pos, pos0))
                seen["tags"] = tags(p)
                return (any(TAG_DODGE_SUBSTR in t.lower() for t in seen["tags"])
                        or seen["moved"] > 150.0)

            done, _ = self._poll(["player"], dodged, DODGE_TIMEOUT_S)
            what = (f"tapped LeftAlt; player.tags {seen['tags']}, moved {seen['moved']:.0f} cm")
            if done:
                return "PASS", what, None
            return ("FAIL", f"{what}: no Dodging tag and no displacement > 150 cm within "
                    f"{DODGE_TIMEOUT_S:g}s", None)

        self.measure("dodge_lalt", dodge)

    # --- 12-14: take damage, Burning, Restoration ---------------------------------------
    def step_damage(self):
        ok, _ = self.expect("enemy_damages_player",
                            "player.attributes.health<player.attributes.max_health",
                            timeout_s=DAMAGE_TIMEOUT_S)
        self.shot("hit", needs=ok, needs_why="not taken: the player was never hit")
        why = "not run: the enemies never damaged the player"

        def burning(b):
            seen = {"tag": False, "text": False, "died": False, "both": False}

            def watch(st):
                p = player(st)
                if p is None or p.get("is_dead") is True:
                    seen["died"] = True
                    return True
                tag = TAG_BURNING in tags(p)
                text = any(BURNING_TEXT.lower() in t.lower() for t in all_texts(st))
                seen["tag"] = seen["tag"] or tag
                seen["text"] = seen["text"] or text
                seen["both"] = seen["both"] or (tag and text)
                return seen["both"]

            t0 = time.monotonic()
            self._poll(["player", "widgets"], watch, BURNING_TIMEOUT_S)
            took = time.monotonic() - t0
            if seen["both"]:
                return ("PASS", f"'{TAG_BURNING}' in player.tags with a '{BURNING_TEXT}' "
                        f"indicator on screen after {took:.1f}s", None)
            if seen["tag"]:
                return ("FAIL", f"'{TAG_BURNING}' was on the player but no widget text "
                        f"'{BURNING_TEXT}' was on screen at the same time (indicator text seen "
                        f"at any time: {seen['text']})", None)
            end = ("the player died first" if seen["died"] else
                   f"not within {BURNING_TIMEOUT_S:g}s")
            return ("WARN", f"Burning never procced ({end}; 40% chance per hit from the "
                    "FireSlash Frontliner): AC-057.11 not exercised", None)

        self.measure("burning_on_hit", burning, needs=ok, needs_why=why)

        def restoration(b):
            p0 = player(b.state(["player"]))
            if p0 is None or p0.get("is_dead") is True:
                return ("WARN", "inconclusive: the player was dead (or had no pawn) before "
                        f"{RESTORATION_KEY} could be tapped", None)
            h0, hmax, sp0 = attr(p0, "health"), attr(p0, "max_health"), attr(p0, "spell_points")
            b.tap(RESTORATION_KEY, hold_s=self.TAP_S)
            seen = {"rose": False, "tag": False, "prev": h0, "sp": sp0, "died": False,
                    "healths": [h0], "alive_reads": 0}

            def watch(st):
                p = player(st)
                if p is None or p.get("is_dead") is True:
                    seen["died"] = True
                    return True
                seen["alive_reads"] += 1
                h = attr(p, "health")
                if h is not None and seen["prev"] is not None and h > seen["prev"] + 1e-3:
                    seen["rose"] = True
                if h is not None:
                    seen["prev"] = h
                    seen["healths"].append(h)
                seen["sp"] = attr(p, "spell_points")
                seen["tag"] = seen["tag"] or TAG_RESTORATION in tags(p)
                sp_ok = (sp0 is not None and seen["sp"] is not None
                         and abs((sp0 - seen["sp"]) - RESTORATION_SP) < 0.01)
                return seen["rose"] and seen["tag"] and sp_ok

            done, _ = self._poll(["player"], watch, RESTORATION_TIMEOUT_S)
            drop = (sp0 - seen["sp"]) if sp0 is not None and seen["sp"] is not None else None
            hs = seen["healths"]
            what = (f"tapped {RESTORATION_KEY} at health {h0}/{hmax}, SP {sp0}; SP now "
                    f"{seen['sp']} (drop {drop}), health samples {hs[:6]}...{hs[-3:]}, health "
                    f"rose between reads: {seen['rose']}, '{TAG_RESTORATION}' seen: {seen['tag']}")
            if done and not seen["died"]:
                return "PASS", what, None
            cast = (seen["tag"] and drop is not None
                    and abs(drop - RESTORATION_SP) < 0.01)
            if seen["died"] and seen["alive_reads"] == 0:
                return ("WARN", "inconclusive: the player was dead at the first read after the "
                        f"tap (killed during it); {what}", None)
            if seen["died"] and cast and not seen["rose"]:
                # The spell was cast (SP and tag) but the enemies killed the player before a
                # rise could be read: the heal itself is unproven, not disproven.
                return "WARN", f"inconclusive: the player died during the heal; {what}", None
            return "FAIL", f"{what} (expected all three within {RESTORATION_TIMEOUT_S:g}s)", None

        self.measure("restoration_heals", restoration, needs=ok, needs_why=why)
        return ok

    # --- 15-18: death and respawn --------------------------------------------------------
    def step_death(self, damaged):
        why = "not run: the enemies never damaged the player"

        def dies(b):
            try:
                r = b.wait_until("player.is_dead==true", timeout_s=DEATH_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                return ("FAIL", f"player.is_dead never became true within {DEATH_TIMEOUT_S:g}s; "
                        f"last values {kit.dumps((e.response or {}).get('values'))}", None)
            self.died_at = time.monotonic()
            st = b.state(["player", "actors"], radius=ENEMY_RADIUS)
            self.enemies_at_death = [xy(a["location"]) for a in enemies(st)]
            self._note_displacement(st)
            p = player(st) or {}
            if TAG_DEAD not in tags(p):
                return ("FAIL", f"player.is_dead==true after {r.get('elapsed_s')}s but "
                        f"'{TAG_DEAD}' is not in player.tags {tags(p)}", None)
            return ("PASS", f"player.is_dead==true after {r.get('elapsed_s')}s with '{TAG_DEAD}' "
                    f"(health {attr(p, 'health')})", None)

        died, _ = self.measure("player_dies", dies, needs=damaged, needs_why=why)
        self.shot("dead", needs=died, needs_why="not taken: the player did not die")
        why = "not run: the player did not die"

        def screen(st):
            # The death screen widget itself: 'You Died', a countdown and the session kill
            # count (AC-062.2).  Digits elsewhere (the HUD's SP text) do not count.
            for w in widgets(st):
                texts = [str(t) for t in (w.get("texts") or [])]
                if not widget_visible(w) or not any(YOU_DIED.lower() in t.lower() for t in texts):
                    continue
                countdown = any(RE_COUNTDOWN.search(t) and not RE_KILLS.match(t)
                                for t in texts)
                if countdown and any(RE_KILLS.match(t) for t in texts):
                    return True
            return False

        self.observe("death_screen", ["widgets"], screen,
                     lambda st: (f"texts on screen: {all_texts(st)} (widget classes "
                                 f"{[w.get('class') for w in widgets(st)]})"),
                     timeout_s=DEATH_SCREEN_TIMEOUT_S, needs=died, needs_why=why)

        def no_move(b):
            p0 = player(b.state(["player"]))
            if p0 is None or p0.get("is_dead") is not True:
                return ("WARN", "inconclusive: the player had already respawned before W could "
                        "be held", None)
            pos0 = xy(p0.get("location"))
            since = time.monotonic() - (self.died_at or time.monotonic())
            if since + DEAD_HOLD_S > RESPAWN_DELAY_S - RESPAWN_MARGIN_S:
                return ("WARN", f"inconclusive: {since:.1f}s after the death there is no time "
                        f"to hold W {DEAD_HOLD_S:g}s before the {RESPAWN_DELAY_S:g}s respawn",
                        None)
            b.hold("W", DEAD_HOLD_S)
            p1 = player(b.state(["player"]))
            if p1 is None or p1.get("is_dead") is not True:
                return ("WARN", "inconclusive: the player respawned during the hold", None)
            moved = dist(pos0, xy(p1.get("location")))
            return ("PASS" if moved < DEAD_MAX_MOVE_CM else "FAIL",
                    f"held W {DEAD_HOLD_S:g}s while dead: moved {moved:.1f} cm from "
                    f"{fmt_xy(pos0)} (limit {DEAD_MAX_MOVE_CM:g})", None)

        self.measure("dead_no_move", no_move, needs=died, needs_why=why)
        return died

    def step_respawn(self, died):
        why = "not run: the player did not die"

        def respawned(st):
            p = player(st)
            if p is None:
                return False
            pos, h, hmax = xy(p.get("location")), attr(p, "health"), attr(p, "max_health")
            return (p.get("is_dead") is False and h is not None and hmax is not None
                    and h >= hmax - 1e-3 and pos is not None
                    and dist(pos, SPAWN) <= RESPAWN_RADIUS and TAG_DEAD not in tags(p))

        def describe(st):
            p = player(st) or {}
            return (f"is_dead={p.get('is_dead')}, health {attr(p, 'health')}/"
                    f"{attr(p, 'max_health')}, location {fmt_xy(xy(p.get('location')))}, "
                    f"tags {tags(p)}")

        ok, st = self.observe("respawn", ["player", "actors"], respawned, describe,
                              timeout_s=RESPAWN_TIMEOUT_S, radius=ENEMY_RADIUS, needs=died,
                              needs_why=why)
        self.respawn_state = st
        self.shot("respawned", needs=ok, needs_why="not taken: the player did not respawn")
        why = "not run: the player did not respawn"

        def reset(b):
            moved = [p for p in self.enemies_at_death
                     if dist(p, FRONTLINER_SPAWNS[nearest_spawn(p)]) > DISPLACED_CM]
            before = (f"enemies at the death {[fmt_xy(p) for p in self.enemies_at_death]}, max "
                      f"distance from a spawn before the death {self.max_displacement:.0f} cm")
            problems = reset_problems(self.respawn_state)
            if problems:
                done, last = self._poll(["actors"], lambda s: not reset_problems(s),
                                        RESET_TIMEOUT_S)
                problems = reset_problems(last)
                now = describe_enemies(enemies(last))
            else:
                now = describe_enemies(enemies(self.respawn_state))
            if problems:
                return "FAIL", f"not reset: {'; '.join(problems)}; {before}; now {now}", None
            if not moved and self.max_displacement <= DISPLACED_CM:
                return ("WARN", f"every enemy is at its spawn with full health, but no enemy had "
                        f"left its spawn before the death, so a reset was not observable; "
                        f"{before}; now {now}", None)
            return ("PASS", f"every enemy back within {DISPLACED_CM:g} cm of its spawn, full "
                    f"health, alive; {before}; now {now}", None)

        self.measure("arena_reset", reset, needs=ok, needs_why=why)

        def moves(b):
            p0 = player(b.state(["player"]))
            if p0 is None:
                return "FAIL", "no valid player after the respawn", None
            x0 = xy(p0.get("location"))[0]
            b.hold("W", MOVE_HOLD_S)
            p1 = player(b.state(["player"])) or {}
            x1 = (xy(p1.get("location")) or (x0, 0))[0]
            return ("PASS" if x1 - x0 >= MOVE_MIN_CM else "FAIL",
                    f"held W {MOVE_HOLD_S:g}s: x {x0:.0f} -> {x1:.0f} (+{x1 - x0:.0f} cm, need "
                    f">= {MOVE_MIN_CM:g})", None)

        self.measure("respawn_moves", moves, needs=ok, needs_why=why)

    # --- 21: kill counter -------------------------------------------------------------------
    def step_kill_counter(self):
        def counter(b):
            st = b.state(["widgets", "player"])
            p = player(st)
            kills = (p or {}).get("kills")
            shown = kill_counts(st)
            note = ("; note: the run kills no enemy (one light attack), so this only compares the "
                    "starting count")
            if p is None:
                return "FAIL", f"no valid player section; 'Kills: N' texts {shown}", None
            # The HUD root may repeat its children's texts: one distinct value is expected.
            values = sorted(set(shown))
            if len(values) != 1:
                return ("FAIL", f"'Kills: N' texts on screen {shown} (expected one value); "
                        f"player.kills={kills}{note}", None)
            return ("PASS" if values[0] == kills else "FAIL",
                    f"the counter reads 'Kills: {values[0]}', player.kills={kills}{note}", None)

        self.measure("kill_counter_text_matches", counter)

    # --- report hooks -------------------------------------------------------------------------
    def report_extra(self):
        return {"enemies_start": self.enemies_start, "enemies_at_death": self.enemies_at_death,
                "max_enemy_displacement": round(self.max_displacement, 1)}

    def md_sections(self):
        return ["", "## Enemies", "",
                f"- At the start: {[fmt_xy(p) for p in self.enemies_start] or 'not read'}",
                f"- At the player's death: "
                f"{[fmt_xy(p) for p in self.enemies_at_death] or 'not read'}",
                f"- Largest distance from a spawn seen before the death: "
                f"{self.max_displacement:.0f} cm"]


Run = Us054Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us054Run, argv)


def main(argv=None):
    return kit.main(Us054Run, argv)


if __name__ == "__main__":
    sys.exit(main())
