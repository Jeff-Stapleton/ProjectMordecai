#!/usr/bin/env python3
"""US-054 Combat Playtest -- Tier 4: the arena combat loop on CombatTestMap, REAL input

The combat half of the US-054 arena loop (AC-054.13: attack -> take damage -> die -> respawn ->
enemies reset), with the death/respawn, status-effect, death screen and kill counter stories it
is built from (US-053, US-057, US-062, US-063).  It plays CombatTestMap (issue #46, T1): a
TestDummy at (600, 600) that never moves or aggroes, and a TestAttacker at (2100, 0) that holds
its spot and swings BasicSlash (15 Slash, 150 cm reach, one swing per ~2.6 s) at a player within
reach.  The attacker is 2100 cm from the PlayerStart, outside its 900 cm aggro range, so the
respawned player is never attacked and the arena reset can be read.  The HUD and the Frontliner
AI checks stay on DevTestMap in Scripts/playtests/us054_arena.py; US-054 owns both scripts.

It drives the PACKAGED game through the in-game agent bridge: every action is a key or mouse
event injected into Slate (key / click), every observation is a state read (state /
wait_until) or a screenshot.  Nothing calls game functions; bridge.jsonl in the run dir is the
audit trail.  The run framework is Scripts/playtest_kit.py (Specs/story_playtests_v1.md section
2); this file is the PLAN and the steps.

Steps (--plan prints them as the story's Steps table):
  1    Both targets present (actors[].kind == "enemy", radius 3000, found by label, else class
       name): alive, 200/200, at their spawns, Idle; the attacker out of aggro range.
  2-4  Walk to the melee spot (440, 600), 160 cm -X of the dummy: LMB (light attack, judged on
       the dummy) and LeftAlt (dodge).
  5-6  Walk via (440, 0) to the fight spot (2000, 0), 100 cm -X of the attacker: it hits the
       player.
  7-8  Tap One (Restoration: health rises, SP -3, the tag), then watch for Burning (WARN: the
       attacker swings BasicSlash).  Burning ends at once when the player dies.
  9-12 The death (seen by whichever step reads it first: no death goes unobserved), the death
       screen, no movement while dead, the respawn at the PlayerStart with full health.
  13-14 The arena reset, judged on the first read that shows the respawned player (both targets
       at their spawns, full health, alive, Idle, and the reset observed: the dummy healed or the
       attacker back to Idle), then movement after the respawn.
  15   The kill counter's text equals player.kills (the authoritative GameMode count).
A failed check SKIPs the checks that depend on it (needs=); independent checks still run.

Usage:
  py Scripts/playtests/us054_combat.py                    fresh package: launch, play, quit
  py Scripts/playtests/us054_combat.py --allow-stale      STALE package: diagnostic only
  py Scripts/playtests/us054_combat.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us054_combat.py --plan             print the Steps table; no game
  py Scripts/playtests/us054_combat.py --attach-port N [--token T]   (always diagnostic)

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us054combat/ (report.md, report.json, game.log,
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
TITLE = "US-054 combat loop playtest (Tier 4, real input)"
RUN_SUFFIX = "us054combat"  # us054_arena.py is "us054": two scripts of one story

# --- CombatTestMap ground truth (Scripts/us100_02_create_test_maps.py) --------------------------
SPAWN = (0.0, 0.0)                     # the one PlayerStart: the respawn point
DUMMY = "TestDummy"                    # BP_MordecaiEnemy_TestDummy: never moves, never aggroes
ATTACKER = "TestAttacker"              # BP_MordecaiEnemy_TestAttacker: holds its spot, swings
DUMMY_SPAWN = (600.0, 600.0)
ATTACKER_SPAWN = (2100.0, 0.0)
TARGETS = ((DUMMY, DUMMY_SPAWN), (ATTACKER, ATTACKER_SPAWN))
ENEMY_MAX_HEALTH = 200.0               # AC-054.2 (both targets' CDO DefaultMaxHealth)
ENEMY_RADIUS = 3000                    # actors radius: the attacker is 2100 cm from the spawn
SPAWN_TOL_CM = 100.0                   # "at its spawn"
AGGRO_RANGE_CM = 900.0                 # AMordecaiEnemyAIController::AggroRange
DUMMY_MELEE_SPOT = (440.0, 600.0)      # 160 cm -X of the dummy (facing is pinned to +X)
ATTACKER_VIA = (440.0, 0.0)            # the straight line to the fight spot passes the dummy
ATTACKER_FIGHT_SPOT = (2000.0, 0.0)    # 100 cm -X of the attacker: inside its 150 cm reach
                                       # even when walk_to stops SPOT_TOL off
SPOT_TOL = 25.0                        # the melee and fight spots: tighter than WALK_TOL
RESPAWN_RADIUS = 150.0
IDLE = "Idle"

# --- tags, widget classes, texts ----------------------------------------------------------------
TAG_DEAD = "Mordecai.State.Dead"
TAG_ATTACKING = "Mordecai.State.Attacking"
TAG_DODGE_SUBSTR = "dodg"              # Mordecai.State.Dodging (case-insensitive substring)
TAG_BURNING = "Mordecai.Status.Burning"
TAG_RESTORATION = "Mordecai.Status.Restoration"
RE_KILLS = re.compile(r"^\s*Kills:\s*(\d+)\s*$")
RE_COUNTDOWN = re.compile(r"\d")
YOU_DIED = "You Died"
BURNING_TEXT = "Burning"
RESTORATION_KEY = "One"
RESTORATION_SP = 3.0
ATTACK_POINT = (640, 360)              # the viewport centre: the game view, no widget

# --- timeouts (module-level so the offline tests can shorten them) ------------------------------
ATTACK_TIMEOUT_S = 1.0
DODGE_TIMEOUT_S = 0.5
DAMAGE_TIMEOUT_S = 20.0
RESTORATION_TIMEOUT_S = 6.0
BURNING_TIMEOUT_S = 25.0
DEATH_TIMEOUT_S = 60.0
DEATH_SCREEN_TIMEOUT_S = 1.5
DEAD_HOLD_S = 0.5
RESPAWN_DELAY_S = 3.0                  # AMordecaiGameMode::RespawnDelaySeconds (AC-053.4)
RESPAWN_MARGIN_S = 0.3                 # never hold W into the respawn (it would spoil `respawn`)
DEAD_MAX_MOVE_CM = 5.0
RESPAWN_TIMEOUT_S = 6.0
MOVE_HOLD_S = 0.4
MOVE_MIN_CM = 80.0
MOVE_SETTLE_S = 0.5                    # the packaged game hitches for a few hundred ms after a screenshot
MOVE_MIN_FPS = 20.0                    # a hold that ran under this frame rate was cut by a frame hitch
POLL_S = 0.1

SCREENSHOTS = {
    "start": "CombatTestMap from the fixed overhead camera: the player at the PlayerStart on the "
             "grey measuring-grid floor, the combat HUD top-left; the blue-grey TestDummy "
             "cylinder (600, 600) under its floating health bar may be in view up and to the "
             "right; the orange TestAttacker (2100, 0) is far up the screen, likely out of view",
    "near_dummy": "the player on the grid floor right next to the blue-grey TestDummy cylinder "
                  "(160 cm +X of the player, i.e. just up the screen) under its floating health "
                  "bar",
    "hit": "the player next to the orange TestAttacker cylinder (100 cm +X of the player, just "
           "up the screen) after its first hit: the player's health bar below full",
    "dead": "the player dead next to the orange TestAttacker (health bar empty); a 'You Died' "
            "death screen with a countdown if the death screen exists (known issue N2: it is "
            "never created)",
    "respawned": "the player back at the PlayerStart with a full health bar on the grid floor; "
                 "the TestAttacker back at (2100, 0), likely out of view",
}
KNOWN_ISSUES = (
    "Block and parry on the mouse (AC-054.6: RMB hold, RMB+LMB) are not tested: they need the "
    "right mouse button held across state reads, and the playtest client's click holds a button "
    "only inside one blocking command (hold_s), so nothing can be read while it is held (B2). "
    "The gamepad path (LT hold, LT+RT) is played by Scripts/playtests/us083_combat_input.py.",
    "The death screen is never created (Known issue N2); death_screen is expected to FAIL.",
    "The TestAttacker swings BasicSlash only (no FireSlash), so Burning never procs on this map: "
    "burning_on_hit is expected to WARN and AC-057.11 / AC-057.8 stay NOT RUN here.",
    "The run lands one light attack and kills no enemy, so kill_counter_text_matches can only "
    "compare 'Kills: 0' with player.kills == 0 (the kill path is US-086's).",
)
KI_DEATH_SCREEN = "Known issue N2: the death screen is never created"

PLAN = (
    Step("combat_targets_present", f"state player, actors (radius {ENEMY_RADIUS}) -> the "
         f"{DUMMY} and the {ATTACKER} (by label, else class name) alive, health "
         f"{ENEMY_MAX_HEALTH:g}/{ENEMY_MAX_HEALTH:g}, within {SPAWN_TOL_CM:g} cm of "
         f"({DUMMY_SPAWN[0]:g}, {DUMMY_SPAWN[1]:g}) and ({ATTACKER_SPAWN[0]:g}, "
         f"{ATTACKER_SPAWN[1]:g}), ai_state '{IDLE}'; the {ATTACKER} >= {AGGRO_RANGE_CM:g} cm "
         "from the player"),
    Step("walk_to_dummy", f"walk_to({DUMMY_MELEE_SPOT[0]:g}, {DUMMY_MELEE_SPOT[1]:g}) -> within "
         f"{SPOT_TOL:g} cm (the melee spot, 160 cm -X of the {DUMMY})"),
    Step("melee_lmb", f"click Left at {ATTACK_POINT} -> within {ATTACK_TIMEOUT_S:g}s "
         f"'{TAG_ATTACKING}' in player.tags or the {DUMMY}'s health below its value before the "
         "click", proves=("AC-054.6", "AC-054.5")),
    Step("dodge_lalt", f"tap LeftAlt -> within {DODGE_TIMEOUT_S:g}s a Dodging tag in player.tags "
         "or a displacement > 150 cm", proves=("AC-054.6", "AC-054.5")),
    Step("walk_to_attacker", f"walk_to({ATTACKER_VIA[0]:g}, {ATTACKER_VIA[1]:g}), walk_to("
         f"{ATTACKER_FIGHT_SPOT[0]:g}, {ATTACKER_FIGHT_SPOT[1]:g}) -> within 60 cm, then within "
         f"{SPOT_TOL:g} cm (the fight spot, 100 cm -X of the {ATTACKER})"),
    Step("attacker_damages_player", "stand at the fight spot -> player.attributes.health < "
         f"player.attributes.max_health within {DAMAGE_TIMEOUT_S:g}s"),
    Step("restoration_heals", f"tap {RESTORATION_KEY} -> within {RESTORATION_TIMEOUT_S:g}s "
         f"health rises, spell_points drops by {RESTORATION_SP:g} and '{TAG_RESTORATION}' is in "
         "player.tags", proves=("US-057:AC-057.4", "US-057:AC-057.5", "US-057:AC-057.12")),
    Step("burning_on_hit", f"keep standing -> within {BURNING_TIMEOUT_S:g}s '{TAG_BURNING}' in "
         f"player.tags and a widget text '{BURNING_TEXT}' (WARN if Burning never procs: the "
         f"{ATTACKER} swings BasicSlash; ends at once, WARN, when the player dies)",
         proves=("US-057:AC-057.11", "US-057:AC-057.8")),
    Step("player_dies", f"keep standing -> player.is_dead==true within {DEATH_TIMEOUT_S:g}s (or "
         f"already seen dead by an earlier step), '{TAG_DEAD}' in player.tags",
         proves=("US-053:AC-053.1",)),
    Step("death_screen", f"state widgets -> within {DEATH_SCREEN_TIMEOUT_S:g}s one widget with "
         f"'{YOU_DIED}', a countdown digit and 'Kills: N'", proves=("US-062:AC-062.1", "US-062:AC-062.2"),
         known_issue=KI_DEATH_SCREEN),
    Step("dead_no_move", f"while dead: hold W {DEAD_HOLD_S:g}s -> moved < {DEAD_MAX_MOVE_CM:g} cm",
         proves=("US-053:AC-053.2",)),
    Step("respawn", f"wait -> within {RESPAWN_TIMEOUT_S:g}s player.is_dead==false, health == "
         f"max_health, within {RESPAWN_RADIUS:g} cm of ({SPAWN[0]:g}, {SPAWN[1]:g}), no "
         f"'{TAG_DEAD}' tag", proves=("US-053:AC-053.4", "US-053:AC-053.5")),
    Step("arena_reset", "the first read that shows the respawned player (state actors) -> the "
         f"{DUMMY} and the {ATTACKER} alive, health == max, within {SPAWN_TOL_CM:g} cm of their "
         f"spawns, ai_state '{IDLE}' in that read; PASS only when that read shows the reset (the "
         f"{DUMMY} below max before the death and full at the respawn, or the {ATTACKER} out of "
         f"{IDLE} before the death and {IDLE} at the respawn), else WARN",
         proves=("US-053:AC-053.8", "US-053:AC-053.9")),
    Step("respawn_moves", f"wait {MOVE_SETTLE_S:g}s, hold W {MOVE_HOLD_S:g}s -> player x grows by "
         f">= {MOVE_MIN_CM:g} cm (measured once more if the hold ran under {MOVE_MIN_FPS:g} fps: a "
         "frame hitch)", proves=("US-053:AC-053.6",)),
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


def find_target(st, fragment):
    """The enemy whose label (or name) contains `fragment`, else the one whose class name does
    (case-insensitive); None when neither matches.  In a package the label is the actor's object
    name, so the class name (BP_MordecaiEnemy_TestDummy_C) is the fallback."""
    es = enemies(st)
    f = fragment.lower()
    for keys in (("label", "name"), ("class",)):
        for a in es:
            if any(f in str(a.get(k) or "").lower() for k in keys):
                return a
    return None


def fmt_xy(p):
    return f"({p[0]:.0f}, {p[1]:.0f})" if p else "(?)"


def describe_target(a, fragment):
    if a is None:
        return f"{fragment}: not in actors[]"
    cur, mx = enemy_health(a)
    return (f"{fragment} '{a.get('label')}' at {fmt_xy(xy(a.get('location')))} hp {cur}/{mx} "
            f"ai={a.get('ai_state')} dead={a.get('is_dead')}")


def describe_targets(st):
    return "; ".join(describe_target(find_target(st, f), f) for f, _ in TARGETS)


def is_dead(p):
    return p is not None and p.get("is_dead") is True


def reset_problems(st):
    """Why the targets are NOT reset ([] = both alive, full health, at their spawns, Idle).
    st: the first read that shows the respawned player (see arena_reset)."""
    es = enemies(st)
    problems = []
    if len(es) != len(TARGETS):
        problems.append(f"{len(es)} enemies (expected {len(TARGETS)})")
    for fragment, spawn in TARGETS:
        a = find_target(st, fragment)
        if a is None:
            problems.append(f"no {fragment} in actors[]")
            continue
        pos = xy(a.get("location"))
        d = dist(pos, spawn)
        cur, mx = enemy_health(a)
        if d > SPAWN_TOL_CM:
            problems.append(f"{fragment} at {fmt_xy(pos)} is {d:.0f} cm from its spawn "
                            f"{fmt_xy(spawn)}")
        if cur is None or mx is None or cur < mx or mx != ENEMY_MAX_HEALTH:
            problems.append(f"{fragment} health {cur}/{mx}")
        if a.get("is_dead") is not False:
            problems.append(f"{fragment} is_dead={a.get('is_dead')}")
        if a.get("ai_state") != IDLE:
            problems.append(f"{fragment}: AI not reset to {IDLE} at the respawn (ai_state "
                            f"{a.get('ai_state')})")
    return problems


def reset_evidence(pre_death, st):
    """What shows the reset happened (the spec rule): the dummy was below max health before the
    death and is at max in st, or the attacker had left Idle before the death and is Idle in st.
    st must be the first read that shows the respawned player, never a later one: without a
    reset the attacker leashes back to Idle by itself within seconds (see arena_reset).
    pre_death: {fragment: {"min_health", "max", "states"}} as the run recorded it."""
    out = []
    dummy, rec = find_target(st, DUMMY), pre_death.get(DUMMY) or {}
    low, mx_before = rec.get("min_health"), rec.get("max")
    if dummy is not None and low is not None and mx_before is not None and low < mx_before:
        cur, mx = enemy_health(dummy)
        if cur is not None and mx is not None and cur >= mx:
            out.append(f"the {DUMMY} was at {low:g}/{mx_before:g} before the death and is at "
                       f"{cur:g}/{mx:g} at the respawn")
    attacker, rec = find_target(st, ATTACKER), pre_death.get(ATTACKER) or {}
    states = rec.get("states") or []
    if attacker is not None and any(s not in (None, IDLE) for s in states) \
            and attacker.get("ai_state") == IDLE:
        out.append(f"the {ATTACKER} went {' -> '.join(str(s) for s in states)} before the death "
                   f"and is {IDLE} at the respawn")
    return out


def death_verdict(p_now, death):
    """(status, detail) of player_dies from the player read now and the first read any step made
    of the player dead (None: no step saw it).  The respawn comes 3 s after the death, so a death
    an earlier step saw counts even when the player has respawned since."""
    if is_dead(p_now):
        where = f" (first seen dead in {death['step']})" if death else ""
        if TAG_DEAD not in tags(p_now):
            return ("FAIL", f"player.is_dead==true{where} but '{TAG_DEAD}' is not in player.tags "
                    f"{tags(p_now)}")
        return ("PASS", f"player.is_dead==true{where} with '{TAG_DEAD}' (health "
                f"{attr(p_now, 'health')})")
    if death is not None:
        seen = (f"player.is_dead==true in {death['step']} (health {death.get('health')}, tags "
                f"{death.get('tags')})")
        if TAG_DEAD not in (death.get("tags") or []):
            return "FAIL", f"{seen}, but without '{TAG_DEAD}'"
        return ("PASS", f"{seen}; the player has respawned since (is_dead="
                f"{(p_now or {}).get('is_dead')})")
    return "FAIL", "player.is_dead is not true and no step saw the player dead"


class Us054CombatRun(kit.PlaytestRun):
    """One run of the playtest.  execute() returns the exit code."""

    STORY = STORY
    COVERS = ("US-053", "US-057", "US-062", "US-063")
    FEATURES = ("enemies", "enemy_ai", "melee_attack", "dodge", "death_respawn", "kill_counter",
                "status_effects")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.COMBAT_TEST_MAP
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.targets_start = {}      # fragment -> (x, y) read in combat_targets_present
        # What the reads before the death showed of each target (arena_reset's evidence): the
        # lowest health, the max health and every ai_state seen (once each, first-seen order)
        self.pre_death = {f: {"min_health": None, "max": None, "states": []} for f, _ in TARGETS}
        self.death = None            # the first read of the player dead: step, tags, health
        self.died_at = None          # time.monotonic() of that read
        self.enemies_at_death = None
        self.respawn_state = None
        self.dummy_hit = None        # (health before the click, lowest after it)

    # --- the run ------------------------------------------------------------------------------
    def steps(self):
        self.step_targets()
        self.step_dummy()
        damaged = self.step_attacker()
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

    def _note(self, st, step):
        """Every read until the death: the targets' lowest health and ai_states, and the FIRST
        read that shows the player dead (whichever step makes it), so no death goes unobserved
        and dead_no_move times its hold from the real death."""
        if self.death is not None:
            return
        for fragment, _ in TARGETS:
            a = find_target(st, fragment)
            if a is None:
                continue
            rec = self.pre_death[fragment]
            cur, mx = enemy_health(a)
            if cur is not None:
                rec["min_health"] = cur if rec["min_health"] is None else min(rec["min_health"],
                                                                              cur)
            if mx is not None:
                rec["max"] = mx
            s = a.get("ai_state")
            if s is not None and s not in rec["states"]:
                rec["states"].append(s)   # each state once, in the order first seen
        p = player(st)
        if is_dead(p):
            self.died_at = time.monotonic()
            self.death = {"step": step, "tags": tags(p), "health": attr(p, "health"),
                          "location": xy(p.get("location"))}
            self.enemies_at_death = describe_targets(st) if enemies(st) else None
            self.log(f"the player is dead (first seen in {step})")

    def describe_pre_death(self):
        parts = []
        for fragment, _ in TARGETS:
            rec = self.pre_death[fragment]
            parts.append(f"{fragment} lowest health {rec['min_health']}/{rec['max']}, ai_states "
                         f"{' -> '.join(str(s) for s in rec['states']) or 'not read'}")
        return "; ".join(parts)

    # --- 1: the targets --------------------------------------------------------------------
    def step_targets(self):
        def present(b):
            st = b.state(["player", "actors"], radius=ENEMY_RADIUS)
            self._note(st, "combat_targets_present")
            p = player(st)
            ppos = xy((p or {}).get("location"))
            problems = []
            for fragment, spawn in TARGETS:
                a = find_target(st, fragment)
                if a is None:
                    problems.append(f"no {fragment} in actors[] (radius {ENEMY_RADIUS}; labels "
                                    f"{[e.get('label') for e in enemies(st)]})")
                    continue
                pos = xy(a.get("location"))
                self.targets_start[fragment] = pos
                cur, mx = enemy_health(a)
                if a.get("is_dead") is not False:
                    problems.append(f"{fragment} is_dead={a.get('is_dead')}")
                if cur != ENEMY_MAX_HEALTH or mx != ENEMY_MAX_HEALTH:
                    problems.append(f"{fragment} health {cur}/{mx} (expected "
                                    f"{ENEMY_MAX_HEALTH:g}/{ENEMY_MAX_HEALTH:g})")
                if dist(pos, spawn) > SPAWN_TOL_CM:
                    problems.append(f"{fragment} at {fmt_xy(pos)} is {dist(pos, spawn):.0f} cm "
                                    f"from its spawn {fmt_xy(spawn)}")
                if a.get("ai_state") != IDLE:
                    problems.append(f"{fragment} ai_state {a.get('ai_state')} (expected {IDLE})")
                if fragment == ATTACKER and ppos is not None \
                        and dist(ppos, pos) < AGGRO_RANGE_CM:
                    problems.append(f"the {ATTACKER} is only {dist(ppos, pos):.0f} cm from the "
                                    f"player (aggro range {AGGRO_RANGE_CM:g})")
            if ppos is None:
                problems.append("no valid player location")
            detail = f"player at {fmt_xy(ppos)}; {describe_targets(st)}"
            return ("FAIL" if problems else "PASS",
                    ("; ".join(problems) + "; " + detail) if problems else detail, None)

        self.measure("combat_targets_present", present)
        self.shot("start")

    # --- 2-4: the dummy: walk, attack, dodge ----------------------------------------------
    def step_dummy(self):
        walked, _ = self.walk("walk_to_dummy", *DUMMY_MELEE_SPOT, tol=SPOT_TOL)
        self.shot("near_dummy")

        def melee(b):
            st0 = b.state(["player", "actors"], radius=ENEMY_RADIUS)
            self._note(st0, "melee_lmb")
            p0 = player(st0)
            if p0 is None:
                return "FAIL", "no valid player before the click", None
            if is_dead(p0):
                return ("FAIL", f"player dead before the click (tags {tags(p0)}); nothing was "
                        "clicked", None)
            # Damage is judged against the dummy's health BEFORE the click, never against the
            # AC's max (the N13 false PASS), and on the dummy only (found again by label/class).
            d0 = find_target(st0, DUMMY)
            h0 = enemy_health(d0)[0] if d0 is not None else None
            r = b.click(*ATTACK_POINT, button="Left")
            seen = {"tag": False, "tags": [], "dummy": None, "damaged": False}

            def hit(st):
                self._note(st, "melee_lmb")
                p = player(st)
                seen["tags"] = tags(p)
                seen["tag"] = seen["tag"] or TAG_ATTACKING in seen["tags"]
                d = find_target(st, DUMMY)
                seen["dummy"] = enemy_health(d) if d is not None else None
                cur = (seen["dummy"] or (None, None))[0]
                seen["damaged"] = seen["damaged"] or (cur is not None and h0 is not None
                                                      and cur < h0)
                # Keep reading until the damage shows: the tag alone passes at the deadline.
                return seen["damaged"]

            self._poll(["player", "actors"], hit, ATTACK_TIMEOUT_S)
            low = (seen["dummy"] or (None, None))[0]
            self.dummy_hit = (h0, low)
            what = (f"Left click at {ATTACK_POINT}, Slate hit {r.get('hit')}; player.tags "
                    f"{seen['tags']}, '{TAG_ATTACKING}' seen: {seen['tag']}; {DUMMY} health "
                    f"{h0} before the click, {seen['dummy']} after"
                    + ("" if walked else " (the walk to the dummy failed)"))
            if seen["tag"] or seen["damaged"]:
                return "PASS", what, None
            return ("FAIL", f"{what}: no '{TAG_ATTACKING}' tag and no {DUMMY} damage within "
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
                self._note(st, "dodge_lalt")
                p = player(st)
                pos = xy((p or {}).get("location"))
                if pos:
                    seen["moved"] = max(seen["moved"], dist(pos, pos0))
                seen["tags"] = tags(p)
                return (any(TAG_DODGE_SUBSTR in t.lower() for t in seen["tags"])
                        or seen["moved"] > 150.0)

            done, _ = self._poll(["player"], dodged, DODGE_TIMEOUT_S)
            what = f"tapped LeftAlt; player.tags {seen['tags']}, moved {seen['moved']:.0f} cm"
            if done:
                return "PASS", what, None
            return ("FAIL", f"{what}: no Dodging tag and no displacement > 150 cm within "
                    f"{DODGE_TIMEOUT_S:g}s", None)

        self.measure("dodge_lalt", dodge)

    # --- 5-8: the attacker: walk, take damage, Restoration, Burning ---------------------------
    def step_attacker(self):
        def walk(b):
            legs = ((ATTACKER_VIA, self.WALK_TOL), (ATTACKER_FIGHT_SPOT, SPOT_TOL))
            done = []
            for i, ((x, y), tol) in enumerate(legs, 1):
                try:
                    r = b.walk_to(x, y, tol=tol)
                except ab.BridgeError as e:
                    if e.kind != "gameplay":
                        raise
                    return ("FAIL", f"leg {i} walk_to({x:.0f}, {y:.0f}) failed: {e} (legs done: "
                            f"{done})", None)
                done.append(f"({r['final'][0]:.0f}, {r['final'][1]:.0f})")
            st = b.state(["player", "actors"], radius=ENEMY_RADIUS)
            self._note(st, "walk_to_attacker")
            a = find_target(st, ATTACKER)
            pos = xy((player(st) or {}).get("location"))
            gap = (f"{dist(pos, xy(a['location'])):.0f} cm from the {ATTACKER}"
                   if a is not None and pos is not None else f"no {ATTACKER} in actors[]")
            return "PASS", f"reached {' -> '.join(done)}; {gap}; {describe_targets(st)}", None

        walked, _ = self.measure("walk_to_attacker", walk)

        def damaged(b):
            seen = {}
            t0 = time.monotonic()

            def hurt(st):
                self._note(st, "attacker_damages_player")
                p = player(st)
                seen.update(h=attr(p, "health"), hmax=attr(p, "max_health"),
                            attacker=describe_target(find_target(st, ATTACKER), ATTACKER))
                return (seen["h"] is not None and seen["hmax"] is not None
                        and seen["h"] < seen["hmax"])

            done, _ = self._poll(["player", "actors"], hurt, DAMAGE_TIMEOUT_S)
            what = (f"player.attributes.health {seen.get('h')} / player.attributes.max_health "
                    f"{seen.get('hmax')}; {seen.get('attacker')}"
                    + ("" if walked else " (the walk to the attacker failed)"))
            if done:
                return "PASS", f"{what} after {time.monotonic() - t0:.1f}s", None
            return "FAIL", f"{what}: not below max within {DAMAGE_TIMEOUT_S:g}s", None

        ok, _ = self.measure("attacker_damages_player", damaged)
        self.shot("hit", needs=ok, needs_why="not taken: the player was never hit")
        why = "not run: the TestAttacker never damaged the player"

        # Restoration BEFORE Burning: the heal runs on a live player.
        def restoration(b):
            st0 = b.state(["player", "actors"], radius=ENEMY_RADIUS)
            self._note(st0, "restoration_heals")
            p0 = player(st0)
            if p0 is None or is_dead(p0):
                return ("WARN", "inconclusive: the player was dead (or had no pawn) before "
                        f"{RESTORATION_KEY} could be tapped", None)
            h0, hmax, sp0 = attr(p0, "health"), attr(p0, "max_health"), attr(p0, "spell_points")
            b.tap(RESTORATION_KEY, hold_s=self.TAP_S)
            seen = {"rose": False, "tag": False, "prev": h0, "sp": sp0, "died": False,
                    "healths": [h0], "alive_reads": 0}

            def watch(st):
                self._note(st, "restoration_heals")
                p = player(st)
                if p is None or is_dead(p):
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

            done, _ = self._poll(["player", "actors"], watch, RESTORATION_TIMEOUT_S)
            drop = (sp0 - seen["sp"]) if sp0 is not None and seen["sp"] is not None else None
            hs = seen["healths"]
            what = (f"tapped {RESTORATION_KEY} at health {h0}/{hmax}, SP {sp0}; SP now "
                    f"{seen['sp']} (drop {drop}), health samples {hs[:6]}...{hs[-3:]}, health "
                    f"rose between reads: {seen['rose']}, '{TAG_RESTORATION}' seen: {seen['tag']}")
            if done and not seen["died"]:
                return "PASS", what, None
            cast = seen["tag"] and drop is not None and abs(drop - RESTORATION_SP) < 0.01
            if seen["died"] and seen["alive_reads"] == 0:
                return ("WARN", "inconclusive: the player was dead at the first read after the "
                        f"tap (killed during it); {what}", None)
            if seen["died"] and cast and not seen["rose"]:
                # The spell was cast (SP and tag) but the attacker killed the player before a
                # rise could be read: the heal itself is unproven, not disproven.
                return "WARN", f"inconclusive: the player died during the heal; {what}", None
            return "FAIL", f"{what} (expected all three within {RESTORATION_TIMEOUT_S:g}s)", None

        self.measure("restoration_heals", restoration, needs=ok, needs_why=why)

        def burning(b):
            seen = {"tag": False, "text": False, "died": False, "both": False}

            def watch(st):
                self._note(st, "burning_on_hit")
                p = player(st)
                if p is None or is_dead(p):
                    seen["died"] = True   # stop at once: the death and respawn are next
                    return True
                tag = TAG_BURNING in tags(p)
                text = any(BURNING_TEXT.lower() in t.lower() for t in all_texts(st))
                seen["tag"] = seen["tag"] or tag
                seen["text"] = seen["text"] or text
                seen["both"] = seen["both"] or (tag and text)
                return seen["both"]

            t0 = time.monotonic()
            self._poll(["player", "widgets", "actors"], watch, BURNING_TIMEOUT_S)
            took = time.monotonic() - t0
            if seen["both"]:
                return ("PASS", f"'{TAG_BURNING}' in player.tags with a '{BURNING_TEXT}' "
                        f"indicator on screen after {took:.1f}s", None)
            if seen["tag"]:
                return ("FAIL", f"'{TAG_BURNING}' was on the player but no widget text "
                        f"'{BURNING_TEXT}' was on screen at the same time (indicator text seen "
                        f"at any time: {seen['text']})", None)
            end = (f"the player died first, after {took:.1f}s" if seen["died"] else
                   f"not within {BURNING_TIMEOUT_S:g}s")
            return ("WARN", f"Burning never procced ({end}; the {ATTACKER} swings BasicSlash, "
                    "which applies no Burning): AC-057.11 not exercised", None)

        self.measure("burning_on_hit", burning, needs=ok, needs_why=why)
        return ok

    # --- 9-11: death ---------------------------------------------------------------------
    def step_death(self, damaged):
        why = "not run: the TestAttacker never damaged the player"

        def dies(b):
            how = ""
            if self.death is None:
                try:
                    r = b.wait_until("player.is_dead==true", timeout_s=DEATH_TIMEOUT_S)
                except ab.BridgeError as e:
                    if e.code != "timeout":
                        raise
                    return ("FAIL", f"player.is_dead never became true within "
                            f"{DEATH_TIMEOUT_S:g}s; last values "
                            f"{kit.dumps((e.response or {}).get('values'))}", None)
                how = f" after {r.get('elapsed_s')}s of player_dies"
            st = b.state(["player", "actors"], radius=ENEMY_RADIUS)
            self._note(st, "player_dies")
            status, detail = death_verdict(player(st), self.death)
            return status, detail + how, None

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
            if not is_dead(p0):
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
            if not is_dead(p1):
                return ("WARN", "inconclusive: the player respawned during the hold", None)
            moved = dist(pos0, xy(p1.get("location")))
            return ("PASS" if moved < DEAD_MAX_MOVE_CM else "FAIL",
                    f"held W {DEAD_HOLD_S:g}s while dead: moved {moved:.1f} cm from "
                    f"{fmt_xy(pos0)} (limit {DEAD_MAX_MOVE_CM:g})", None)

        self.measure("dead_no_move", no_move, needs=died, needs_why=why)
        return died

    # --- 12-14: respawn, arena reset, movement --------------------------------------------
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
            # Judged on the first read that shows the respawned player, and on no later read.
            # ExecuteRespawn teleports the player and runs ResetArena in the same call, so that
            # read already shows the whole reset (location, health, is_dead and ai_state; the
            # targets never move or heal by themselves).  A later read could only hide a missing
            # reset: without one the TestAttacker keeps swinging at the dead player until the
            # respawn, then, the player now beyond its 1800 cm LeashRange, goes Recover -> Leash
            # -> Idle by itself within ~2.6 s, which would read as a reset.
            st = self.respawn_state
            problems = reset_problems(st)
            before = f"before the death: {self.describe_pre_death()}"
            at = f"at the respawn: {describe_targets(st)}"
            if problems:
                return "FAIL", f"not reset: {'; '.join(problems)}; {before}; {at}", None
            evidence = reset_evidence(self.pre_death, st)
            if not evidence:
                return ("WARN", f"both targets are at their spawns, full health, alive and "
                        f"{IDLE}, but no reset was observable (the {DUMMY} was never below max "
                        f"health and the {ATTACKER} never left {IDLE} before the death); "
                        f"{before}; {at}", None)
            return ("PASS", f"both targets back within {SPAWN_TOL_CM:g} cm of their spawns, full "
                    f"health, alive, {IDLE} at the respawn; reset observed: "
                    f"{'; '.join(evidence)}; {before}; {at}", None)

        self.measure("arena_reset", reset, needs=ok, needs_why=why)

        def moves(b):
            # The 'respawned' screenshot just before hitches the packaged game for a few hundred
            # ms.  The bridge times a hold in real time and lets the key up before the input of
            # the frame that follows a long frame, so a hitch inside the hold leaves the
            # character almost no simulated input (20261029-132452-us054combat: W held 0.464 s
            # over 3 frames moved it 3 cm).  So settle first, and measure once more when the
            # bridge reports the hold itself ran under MOVE_MIN_FPS: the game never saw that
            # hold.  Input that is really ignored after the respawn fails the clean hold too.
            notes = []
            for attempt in (1, 2):
                b.wait(MOVE_SETTLE_S)
                p0 = player(b.state(["player"]))
                if p0 is None:
                    return "FAIL", "no valid player after the respawn", None
                x0 = xy(p0.get("location"))[0]
                r = b.hold("W", MOVE_HOLD_S) or {}
                p1 = player(b.state(["player"])) or {}
                x1 = (xy(p1.get("location")) or (x0, 0))[0]
                frames, held_s = r.get("frames"), r.get("held_s")
                fps = frames / held_s if frames and held_s else None
                result = (f"held W {MOVE_HOLD_S:g}s ({held_s}s, {frames} frames): x {x0:.0f} -> "
                          f"{x1:.0f} (+{x1 - x0:.0f} cm, need >= {MOVE_MIN_CM:g})")
                if x1 - x0 >= MOVE_MIN_CM or fps is None or fps >= MOVE_MIN_FPS or attempt == 2:
                    break
                notes.append(f"attempt 1 not judged: {result}, a {fps:.0f} fps hold (a frame "
                             f"hitch), so it was measured again")
            return ("PASS" if x1 - x0 >= MOVE_MIN_CM else "FAIL", "; ".join(notes + [result]),
                    None)

        self.measure("respawn_moves", moves, needs=ok, needs_why=why)

    # --- 15: kill counter -------------------------------------------------------------------
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
        return {"targets_start": {f: list(p) if p else None
                                  for f, p in self.targets_start.items()},
                "pre_death": self.pre_death, "death": self.death,
                "enemies_at_death": self.enemies_at_death,
                "dummy_hit": list(self.dummy_hit) if self.dummy_hit else None}

    def md_sections(self):
        death = self.death
        start = {f: fmt_xy(p) for f, p in self.targets_start.items()}
        seen = (f"first seen in {death['step']} at {fmt_xy(death['location'])} (tags "
                f"{death['tags']})" if death else "not seen")
        return ["", "## Combat targets", "",
                f"- At the start: {start or 'not read'}",
                f"- The {DUMMY}'s health before the light attack and after it: "
                f"{self.dummy_hit or 'not read'}",
                f"- Before the death: {self.describe_pre_death()}",
                f"- The death: {seen}",
                f"- At the death: {self.enemies_at_death or 'not read'}"]


Run = Us054CombatRun  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us054CombatRun, argv)


def main(argv=None):
    return kit.main(Us054CombatRun, argv)


if __name__ == "__main__":
    sys.exit(main())
