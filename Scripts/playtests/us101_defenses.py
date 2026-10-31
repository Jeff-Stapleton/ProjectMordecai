#!/usr/bin/env python3
"""US-101 Defenses Playtest -- Tier 4: block, perfect block, dodge, jump and parry against the
CombatTestMap TestAttacker's swing, played with REAL input

US-101 (GitHub issue #53, known issue N32) routes every enemy melee hit through the target's
defenses.  This script stands the player next to the CombatTestMap TestAttacker (2100, 0), which
holds its spot and swings DA_EnemyAttack_BasicSlash (ArcSector 150 cm / 90 deg, windup 500 ms,
15 Slash, PostureDamageScalar 0.3, JumpAvoidable) about every 2.6 s, and meets one swing with each
defense.  It drives the PACKAGED game through the in-game agent bridge: every action is an axis or
key event injected into Slate (axis LT / RT, key W / LeftAlt / SpaceBar; walk_to is W/A/S/D taps),
every observation a state read, a wait_until or a screenshot.  Nothing calls game functions;
bridge.jsonl in the run dir is the audit trail.  The run framework is Scripts/playtest_kit.py
(Specs/story_playtests_v1.md section 2).

The swing and how the script meets it
  - AMordecaiEnemyAIController: in Attack the AI activates the swing on its next tick; the hit is
    applied once, when the 500 ms windup ends (about edge + 0.52 s); after the 1.1 s ability it
    goes Recover for 1500 ms (AttackCooldownMs), then Attack again.  The bridge does not expose the
    attack phase, only actors[].ai_state, so every check syncs on the attacker's not-Attack ->
    Attack edge: a server-side wait_until with poll_s 0 (one evaluation per frame) on its actors[]
    entry, found by its exact name at any of the first ACTOR_SLOTS indices (actors[] is sorted by
    distance and actors[0] is the player pawn).
  - Per swing: wait_until the attacker is out of Attack; read player + actors (the "before"
    values, in a read that shows it out of Attack, so the previous swing's hit is in them); wait
    for the edge; the timed input; wait_until it leaves Attack again (the swing is over: its hit is
    in); read again (the "after" values).  A swing that started during a step's set-up is waited
    out and the next one is used, so each check's before/after brackets exactly one swing.
  - Timed inputs are sent DELAY seconds after the edge was seen.  The control swing (and the
    blocked ones) measure when the hit shows (wait_until player health drops, poll_s 0); the later
    delays are shifted by (median measured hit time - NOMINAL_HIT_S), at most MAX_SHIFT_S, so a
    slower frame rate moves the presses with the hit.  Each check's detail and the report's Swings
    table list the edge, the input and the hit times.

Steps (--plan prints them as the story's Steps table):
  1  Walk (WASD) to the fight spot (2000, 0), 100 cm -X of the attacker (its first swing usually
     lands during the walk: not measured).
  2  Control: stand through one swing: health -15, posture -4.5.
  3  Block: LT held from Recover through the swing: health -7.5 (not -15), stamina -10.5.
  4  Perfect block: LT raised ~0.30 s after the edge (~0.2 s before the hit, inside the 250 ms
     window): health -7.5, stamina unchanged.
  5  Dodge: W held from ~0.15 s, LeftAlt tapped ~0.25 s after the edge (the dodge goes +X into the
     attacker and stays inside its reach; a backward dodge leaves it, N27): no health lost,
     Mordecai.State.Dodging seen, the player within 150 cm of the attacker around the hit.
  6  Jump: SpaceBar tapped ~0.15 s after the edge: no health lost, Mordecai.State.Airborne in
     player.tags while movement_mode is Falling, and gone after the landing.
  7  Parry, last (Riposte and Parried are never removed today, N33): LT held from Recover, RT
     pressed ~0.25 s after the edge: no health lost, Mordecai.State.Riposte on the player,
     Mordecai.State.Parried on the attacker.
  A screenshot after the walk and after each swing shows the HUD bars.  Steps 5-7 need the control
  to have passed (a defended swing that does no damage proves nothing if swings never land here),
  and steps 5-7 FAIL when the player was out of the 150 cm reach at the hit.
Budget: health 100 -> 85 (the walk's swing) -> 70 -> 62.5 -> 55, then unchanged; each failed
defense costs 15 more.  Stamina never regenerates (N30): 100 - 10.5 - 0 - 15 - 20 stays above 0, so
the guard never breaks.

Usage:
  py Scripts/playtests/us101_defenses.py                    fresh package: launch, play, quit
  py Scripts/playtests/us101_defenses.py --allow-stale      STALE package: diagnostic only
  py Scripts/playtests/us101_defenses.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us101_defenses.py --plan             print the Steps table; no game
  py Scripts/playtests/us101_defenses.py --attach-port N [--token T]   diagnostic only
  py Scripts/playtest_story.py US-101                       Tier 3 boot + this script

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us101/ (report.md, report.json, game.log, bridge.jsonl,
bridge_NN.png).  Open every screenshot with the Read tool and state what is on screen.

Exit codes: 0 every check passed (WARN allowed); 1 a gameplay check FAILED; 2 infrastructure.
"""

import math
import os
import statistics
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-101"
TITLE = "US-101 defenses against enemy melee playtest (Tier 4, real input)"
RUN_SUFFIX = "us101"

# --- CombatTestMap ground truth (Scripts/us100_02_create_test_maps.py) --------------------------
ATTACKER = "TestAttacker"              # BP_MordecaiEnemy_TestAttacker: holds its spot, swings
ATTACKER_SPAWN = (2100.0, 0.0)
FIGHT_SPOT = (2000.0, 0.0)             # 100 cm -X of the attacker: inside its 150 cm reach
SPOT_TOL = 25.0                        # walk_to tolerance at the fight spot
RETURN_TOL = 40.0                      # walk back to the fight spot when further than this
REACH_CM = 150.0                       # BasicSlash ArcSector radius (2D, centre to centre)
ENEMY_RADIUS = 3000                    # state reads (wait_until gathers actors within 1500)
ACTOR_SLOTS = 5                        # actors[] indices the edge conditions look at
ATTACK = "Attack"

# --- the swing and the defenses (code defaults; AC-101.1..AC-101.5) ------------------------------
SLASH_DAMAGE = 15.0                    # DA_EnemyAttack_BasicSlash BasePower
POSTURE_SCALAR = 0.3                   # PostureDamageScalar: 4.5 posture per hit
MITIGATION = 0.5                       # UMordecaiGA_Block::Mitigation
STABILITY = 0.3                        # UMordecaiGA_Block::Stability
PERFECT_WINDOW_S = 0.25                # UMordecaiGA_Block::PerfectBlockWindowMs
DODGE_STAMINA = 15.0                   # UMordecaiGA_Dodge::StaminaCost
PARRY_STAMINA = 20.0                   # UMordecaiGA_Parry::ParryStaminaCost
TOL = 0.5                              # every health / stamina / posture expectation is +-0.5
BLOCKED_HEALTH = SLASH_DAMAGE * (1.0 - MITIGATION)       # 7.5
BLOCKED_STAMINA = SLASH_DAMAGE * (1.0 - STABILITY)       # 10.5
FULL_POSTURE = SLASH_DAMAGE * POSTURE_SCALAR             # 4.5

# --- input ----------------------------------------------------------------------------------------
LT = "Gamepad_LeftTriggerAxis"         # block (held); with a fresh RT press: parry
RT = "Gamepad_RightTriggerAxis"
FORWARD_KEY = "W"                      # +X: toward the attacker
DODGE_KEY = "LeftAlt"
JUMP_KEY = "SpaceBar"
TAP_S = 0.1
RT_TAP_S = 0.1

# --- tags -----------------------------------------------------------------------------------------
TAG_BLOCKING = "Mordecai.State.Blocking"
TAG_PARRYING = "Mordecai.State.Parrying"
TAG_WHIFF = "Mordecai.State.ParryWhiff"
TAG_RIPOSTE = "Mordecai.State.Riposte"
TAG_PARRIED = "Mordecai.State.Parried"
TAG_DODGING = "Mordecai.State.Dodging"
TAG_AIRBORNE = "Mordecai.State.Airborne"
TAG_DEAD = "Mordecai.State.Dead"
DEFENSE_TAGS = (TAG_BLOCKING, TAG_PARRYING, TAG_DODGING, TAG_AIRBORNE)
STATE_PREFIX = "Mordecai.State."

# --- timing: seconds after the edge was seen (module-level so the offline tests can change them) --
NOMINAL_HIT_S = 0.52                   # windup 0.5 s + the AI tick that activates the swing
PERFECT_BLOCK_DELAY_S = 0.30           # LT ~0.2 s before the hit: inside the 0.25 s window
DODGE_DELAY_S = 0.25                   # i-frames 0.4 s from the press cover the hit
FORWARD_LEAD_S = 0.10                  # W goes down this long before LeftAlt (the dodge direction)
JUMP_DELAY_S = 0.15                    # ~0.7 s in the air from here covers the hit
PARRY_DELAY_S = 0.25                   # the 0.5 s parry window from here covers the hit
MAX_SHIFT_S = 0.15                     # the measured hit time moves the delays at most this much
OBSERVE_AFTER_HIT_S = 0.25             # dodge / jump / parry: read state until hit + this
REACH_WINDOW_S = 0.10                  # reads within this of the expected hit judge the reach
EDGE_POLL_S = 0.0                      # wait_until poll_s: one evaluation per frame
SAMPLE_POLL_S = 0.03
NOT_ATTACK_TIMEOUT_S = 3.0             # a swing lasts 1.1 s
EDGE_TIMEOUT_S = 6.0                   # Recover 1.5 s (a posture-broken attacker: its stagger)
HIT_TIMEOUT_S = 1.5                    # after the edge
GUARD_TIMEOUT_S = 1.0                  # LT -> Mordecai.State.Blocking
PRE_EDGE_TRIES = 3

SCREENSHOTS = {
    "fight_spot": "CombatTestMap's grey grid floor from the fixed overhead camera: the player at "
                  "(2000, 0) right next to the red/orange TestAttacker cylinder (100 cm +X, just up "
                  "the screen) under its floating health bar; the combat HUD top-left (health "
                  "probably already below full: the attacker's first swing lands during the walk)",
    "undefended": "the player next to the TestAttacker after the control swing: the HUD health bar "
                  "15 lower than before it, stamina still full",
    "blocked": "after the blocked swing: health 7.5 lower, the stamina bar visibly down (10.5)",
    "perfect_block": "after the perfect block: health 7.5 lower, stamina unchanged since the "
                     "previous screenshot; 'Perfect Block!' feedback may flash",
    "dodged": "after the dodge into the attacker: the player still pressed against (or beside) "
              "the TestAttacker, health unchanged since the previous screenshot, stamina lower (15)",
    "jumped": "after the jump: the player back on the floor next to the TestAttacker, health "
              "unchanged since the previous screenshot",
    "parried": "after the parry: health unchanged, stamina lower (20); 'Parry' feedback may flash; "
               "the TestAttacker may be staggered",
}
KNOWN_ISSUES = (
    "Riposte and Parried are loose tags that nothing removes (N33): parry_negates runs last, and "
    "it FAILs if either tag is already there before its swing.",
    "A dodge travels about 110 cm (N27); with no move input it goes backward (-X), out of the "
    "150 cm reach, which would make dodge_evades prove nothing.  So W is held into the dodge "
    "(+X, into the attacker), and the check FAILs when the player was out of reach at the hit.",
    "Stamina never regenerates (N30): the stamina budget is 100 - 10.5 (block) - 0 (perfect "
    "block) - 15 (dodge) - 20 (parry).",
    "Block and parry on the mouse (RMB hold, RMB+LMB) are not played: the playtest client holds a "
    "mouse button only inside one blocking click (B2).  LT / RT are the same Block / Parry "
    "actions.",
)

PLAN = (
    Step("walk_to_fight_spot", f"walk_to({FIGHT_SPOT[0]:g}, {FIGHT_SPOT[1]:g}) -> within "
         f"{SPOT_TOL:g} cm (100 cm -X of the {ATTACKER} at ({ATTACKER_SPAWN[0]:g}, "
         f"{ATTACKER_SPAWN[1]:g})); state actors (radius {ENEMY_RADIUS}) -> the {ATTACKER} (by "
         f"label, else class) alive, within its {REACH_CM:g} cm reach"),
    Step("undefended_hit", f"no input: wait_until the {ATTACKER}'s actors[] ai_state leaves "
         f"'{ATTACK}', read player, wait_until it is '{ATTACK}' (the edge, poll_s 0), wait_until "
         f"player.attributes.health drops; after the swing -> health -{SLASH_DAMAGE:g} and posture "
         f"-{FULL_POSTURE:g} (+-{TOL:g}): the control, the swing lands at this spot",
         proves=("AC-101.10",)),
    Step("block_mitigates", f"axis {LT}=1 held from the attacker's Recover ('{TAG_BLOCKING}' seen "
         f"before the edge) through its swing -> health -{BLOCKED_HEALTH:g}, not "
         f"-{SLASH_DAMAGE:g}, and stamina -{BLOCKED_STAMINA:g} (+-{TOL:g}); then {LT}=0",
         proves=("AC-101.11", "US-006:AC-006.3", "US-006:AC-006.4")),
    Step("perfect_block", f"{LT} up at the edge; ~{PERFECT_BLOCK_DELAY_S:g}s after it (shifted by "
         f"the measured hit time) axis {LT}=1, wait_until health drops -> health "
         f"-{BLOCKED_HEALTH:g} and stamina unchanged (+-{TOL:g}); the edge, LT and hit times are in "
         "the detail", proves=("AC-101.12",)),
    Step("dodge_evades", f"press {FORWARD_KEY} ~{DODGE_DELAY_S - FORWARD_LEAD_S:g}s and tap "
         f"{DODGE_KEY} ~{DODGE_DELAY_S:g}s after the edge (a dodge +X, into the attacker), release "
         f"{FORWARD_KEY}; poll state -> '{TAG_DODGING}' seen, health unchanged (+-{TOL:g}), and "
         f"the player within {REACH_CM:g} cm of the attacker at the reads around the expected hit "
         "(needs the control)", proves=("AC-101.13", "US-005:AC-005.3")),
    Step("jump_evades", f"tap {JUMP_KEY} ~{JUMP_DELAY_S:g}s after the edge; poll state -> "
         f"'{TAG_AIRBORNE}' in player.tags at a read with player.movement_mode==\"Falling\", "
         f"health unchanged (+-{TOL:g}), within reach around the expected hit, and no "
         f"'{TAG_AIRBORNE}' once Walking again (needs the control)",
         proves=("AC-101.14", "US-003:AC-003.7")),
    Step("parry_negates", f"axis {LT}=1 held from Recover ('{TAG_BLOCKING}' seen), axis {RT}=1 "
         f"for {RT_TAP_S:g}s ~{PARRY_DELAY_S:g}s after the edge (the parry chord) -> health "
         f"unchanged (+-{TOL:g}), '{TAG_RIPOSTE}' in player.tags and '{TAG_PARRIED}' in the "
         f"attacker's actors[] tags; then {LT}=0 (last: N33; needs the control)",
         proves=("AC-101.15",)),
)

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA
set_log_stream = kit.set_log_stream


class SwingError(Exception):
    """A swing could not be played or read as planned (a wait timed out, a precondition failed):
    the check FAILs with this message."""


# ---------------------------------------------------------------------------
# State helpers (pure: the tests call them too)
# ---------------------------------------------------------------------------
def _num(v):
    return float(v) if isinstance(v, (int, float)) and not isinstance(v, bool) else None


def player(st):
    p = (st or {}).get("player")
    return p if isinstance(p, dict) and p.get("valid") is True else None


def tags(entry):
    return [str(t) for t in ((entry or {}).get("tags") or []) if t is not None]


def state_tags(entry):
    """Only the Mordecai.State.* tags (what the report keeps per read)."""
    return [t for t in tags(entry) if t.startswith(STATE_PREFIX)]


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


def fmt_xy(p):
    return f"({p[0]:.0f}, {p[1]:.0f})" if p else "(?)"


def enemies(st):
    v = (st or {}).get("actors")
    return [a for a in v if isinstance(a, dict) and a.get("kind") == "enemy"] \
        if isinstance(v, list) else []


def find_attacker(st, name=None):
    """The attacker's actors[] entry: by its exact object name when known, else the enemy whose
    label (or name) contains 'TestAttacker', else the one whose class name does."""
    es = enemies(st)
    if name:
        for a in es:
            if a.get("name") == name:
                return a
    f = ATTACKER.lower()
    for keys in (("label", "name"), ("class",)):
        for a in es:
            if any(f in str(a.get(k) or "").lower() for k in keys):
                return a
    return None


def attacker_condition(name, attacking):
    """A wait_until condition on the attacker's ai_state, at whichever of the first ACTOR_SLOTS
    actors[] indices holds it (the array is sorted by distance; [0] is the player pawn).
    attacking=True: it is in Attack; False: it is in any other state."""
    who = kit.lit(name)
    parts = []
    for i in range(ACTOR_SLOTS):
        a = f"actors[{i}]"
        if attacking:
            parts.append(f'({a}.name=={who} && {a}.ai_state=="{ATTACK}")')
        else:
            parts.append(f'({a}.name=={who} && {a}.ai_state!="{ATTACK}" && {a}.ai_state!=null)')
    return " || ".join(parts)


def vitals(p):
    return {k: attr(p, k) for k in ("health", "max_health", "stamina", "max_stamina", "posture",
                                    "max_posture")}


def spent(before, after):
    """How much of health / stamina / posture the swing took (before - after; None if unread)."""
    out = {}
    for k in ("health", "stamina", "posture"):
        a, b = (before or {}).get(k), (after or {}).get(k)
        out[k] = round(a - b, 3) if a is not None and b is not None else None
    return out


def near(value, target, tol=TOL):
    return value is not None and abs(value - target) <= tol


def shift_for(expected_hit_s):
    """How far to move the timed inputs: the measured hit time minus the nominal one, clamped."""
    return max(-MAX_SHIFT_S, min(MAX_SHIFT_S, float(expected_hit_s) - NOMINAL_HIT_S))


def fmt_t(t):
    return "?" if t is None else f"+{t:.3f}s"


def timeline_text(rec):
    parts = ["edge +0.000s"] + [f"{label} {fmt_t(t)}" for label, t in rec.get("timeline") or []]
    hit = rec.get("hit_s")
    parts.append(f"hit seen {fmt_t(hit)}" if hit is not None else
                 f"expected hit {fmt_t(rec.get('expected_hit_s'))}")
    return "; ".join(parts)


def spent_text(rec):
    s = rec.get("spent") or {}

    def one(k):
        v = s.get(k)
        return f"{k} -{v:g}" if v is not None else f"{k} ?"

    b, a = rec.get("before") or {}, rec.get("after") or {}
    return (f"{one('health')} ({b.get('health')} -> {a.get('health')}), {one('stamina')} "
            f"({b.get('stamina')} -> {a.get('stamina')}), {one('posture')} ({b.get('posture')} -> "
            f"{a.get('posture')})")


def reach_at_hit(samples, hit_s, window=REACH_WINDOW_S, reach=REACH_CM):
    """(in reach: True / False / None, text) from the reads within `window` of the expected hit
    (the nearest read when none is that close).  None: no read had both positions."""
    known = [s for s in samples or () if s.get("dist") is not None]
    if not known:
        return None, "no read had the player's and the attacker's positions"
    close = [s for s in known if abs(s["t"] - hit_s) <= window]
    which = f"{len(close)} read(s) within {window:g}s of the expected hit {fmt_t(hit_s)}"
    if not close:
        close = [min(known, key=lambda s: abs(s["t"] - hit_s))]
        which = f"the read nearest the expected hit {fmt_t(hit_s)} (at {fmt_t(close[0]['t'])})"
    lo, hi = min(s["dist"] for s in close), max(s["dist"] for s in close)
    return hi <= reach, (f"{which}: player {lo:.0f}..{hi:.0f} cm from the attacker (reach "
                         f"{reach:g})")


def seen(samples, tag, where="tags"):
    return any(tag in (s.get(where) or []) for s in samples or ())


def _died(rec):
    return rec.get("dead_after") is True


def _base(rec):
    return f"{spent_text(rec)}; {timeline_text(rec)}"


# ---------------------------------------------------------------------------
# Judges (pure): (status, detail) from one swing's record
# ---------------------------------------------------------------------------
def judge_undefended(rec):
    s = rec.get("spent") or {}
    detail = f"stood through the swing: {_base(rec)}"
    if rec.get("hit_s") is None:
        return "FAIL", f"{detail} -- no health drop within {HIT_TIMEOUT_S:g}s of the edge"
    if near(s.get("health"), SLASH_DAMAGE) and near(s.get("posture"), FULL_POSTURE):
        return "PASS", detail
    return ("FAIL", f"{detail} -- expected health -{SLASH_DAMAGE:g} and posture "
            f"-{FULL_POSTURE:g} (+-{TOL:g})" + (" (the player died)" if _died(rec) else ""))


def judge_block(rec):
    s = rec.get("spent") or {}
    h, st = s.get("health"), s.get("stamina")
    detail = f"{LT} held from Recover (blocking): {_base(rec)}"
    if near(h, BLOCKED_HEALTH) and near(st, BLOCKED_STAMINA):
        return "PASS", detail
    why = []
    if near(h, SLASH_DAMAGE):
        why.append(f"the full {SLASH_DAMAGE:g} landed through the block (N32)")
    elif not near(h, BLOCKED_HEALTH):
        why.append(f"health -{h} (expected -{BLOCKED_HEALTH:g})")
    if near(st, 0.0) and near(h, BLOCKED_HEALTH):
        why.append("no stamina drained: read as a perfect block, though LT went up well before "
                   f"the {PERFECT_WINDOW_S:g}s window")
    elif not near(st, BLOCKED_STAMINA):
        why.append(f"stamina -{st} (expected -{BLOCKED_STAMINA:g})")
    if _died(rec):
        why.append("the player died")
    return "FAIL", f"{detail} -- {'; '.join(why)}"


def judge_perfect(rec):
    s = rec.get("spent") or {}
    h, st = s.get("health"), s.get("stamina")
    lt = dict(rec.get("timeline") or []).get(f"{LT}=1 answered")
    hit = rec.get("hit_s")
    lead = (f"LT went up {hit - lt:.3f}s before the hit was seen (window {PERFECT_WINDOW_S:g}s)"
            if lt is not None and hit is not None else "LT-to-hit lead not measured")
    detail = f"{lead}: {_base(rec)}"
    if near(h, BLOCKED_HEALTH) and near(st, 0.0):
        return "PASS", detail
    why = []
    if hit is None:
        why.append(f"no health drop within {HIT_TIMEOUT_S:g}s of the edge")
    if near(h, SLASH_DAMAGE):
        why.append(f"the full {SLASH_DAMAGE:g} landed (N32, or LT too late to block)")
    elif not near(h, BLOCKED_HEALTH):
        why.append(f"health -{h} (expected -{BLOCKED_HEALTH:g})")
    if near(st, BLOCKED_STAMINA):
        why.append(f"stamina -{st}: a normal block, not a perfect one (a timing miss: see the "
                   "lead)")
    elif not near(st, 0.0):
        why.append(f"stamina -{st} (expected 0)")
    if _died(rec):
        why.append("the player died")
    return "FAIL", f"{detail} -- {'; '.join(why)}"


def _evade_common(rec, what):
    """Shared by dodge / jump / parry: (problems, reach text)."""
    s = rec.get("spent") or {}
    problems = []
    if not near(s.get("health"), 0.0):
        problems.append(f"health -{s.get('health')} (expected unchanged"
                        + (f": the full {SLASH_DAMAGE:g} landed through the {what}, N32"
                           if near(s.get("health"), SLASH_DAMAGE) else "") + ")")
    in_reach, reach = reach_at_hit(rec.get("samples"), rec.get("expected_hit_s") or NOMINAL_HIT_S)
    if in_reach is None:
        problems.append(f"inconclusive: {reach}, so the reach at the hit is unknown")
    elif not in_reach:
        problems.append(f"inconclusive: out of the {REACH_CM:g} cm reach at the hit ({reach}): "
                        "a swing that cannot reach proves nothing")
    if _died(rec):
        problems.append("the player died")
    return problems, reach


def judge_dodge(rec):
    problems, reach = _evade_common(rec, "dodge")
    dodged = seen(rec.get("samples"), TAG_DODGING)
    if not dodged:
        problems.append(f"no '{TAG_DODGING}' in player.tags at any read after {DODGE_KEY}")
    detail = (f"{FORWARD_KEY} + {DODGE_KEY}: '{TAG_DODGING}' seen: {dodged}; {reach}; "
              f"{_base(rec)}")
    return ("FAIL", f"{detail} -- {'; '.join(problems)}") if problems else ("PASS", detail)


def judge_jump(rec):
    problems, reach = _evade_common(rec, "jump")
    samples = rec.get("samples") or []
    falling = [s for s in samples if s.get("mode") == "Falling"]
    airborne = [s for s in falling if TAG_AIRBORNE in (s.get("tags") or [])]
    if not falling:
        problems.append(f"movement_mode never read 'Falling' after {JUMP_KEY}")
    elif not airborne:
        problems.append(f"'{TAG_AIRBORNE}' not in player.tags at any of {len(falling)} Falling "
                        "read(s)")
    post_mode, post_tags = rec.get("after_mode"), rec.get("after_tags") or []
    landed = "not read"
    if post_mode == "Walking":
        stuck = TAG_AIRBORNE in post_tags
        landed = f"Walking after the swing, '{TAG_AIRBORNE}' {'still on' if stuck else 'gone'}"
        if stuck:
            problems.append(f"'{TAG_AIRBORNE}' stuck after the landing (movement_mode Walking)")
    elif post_mode is not None:
        landed = f"movement_mode {post_mode} after the swing (landing not judged)"
    hit_s = rec.get("expected_hit_s") or NOMINAL_HIT_S
    at_hit = min(samples, key=lambda s: abs(s["t"] - hit_s)) if samples else {}
    detail = (f"{JUMP_KEY}: {len(falling)} Falling read(s), {len(airborne)} with "
              f"'{TAG_AIRBORNE}'; at the read nearest the expected hit "
              f"({fmt_t(at_hit.get('t'))}) movement_mode {at_hit.get('mode')}; {landed}; "
              f"{reach}; {_base(rec)}")
    return ("FAIL", f"{detail} -- {'; '.join(problems)}") if problems else ("PASS", detail)


def judge_parry(rec):
    problems, reach = _evade_common(rec, "parry")
    samples = rec.get("samples") or []
    riposte = TAG_RIPOSTE in (rec.get("after_tags") or []) or seen(samples, TAG_RIPOSTE)
    parried = (TAG_PARRIED in (rec.get("attacker_after_tags") or [])
               or seen(samples, TAG_PARRIED, "attacker_tags"))
    parrying = seen(samples, TAG_PARRYING)
    whiff = seen(samples, TAG_WHIFF) or TAG_WHIFF in (rec.get("after_tags") or [])
    if not riposte:
        problems.append(f"no '{TAG_RIPOSTE}' on the player")
    if not parried:
        problems.append(f"no '{TAG_PARRIED}' on the {ATTACKER}")
    detail = (f"{LT} held, {RT} pressed: '{TAG_PARRYING}' seen: {parrying}, '{TAG_RIPOSTE}' on "
              f"the player: {riposte}, '{TAG_PARRIED}' on the {ATTACKER}: {parried}, "
              f"'{TAG_WHIFF}' seen: {whiff}; {reach}; {_base(rec)}")
    return ("FAIL", f"{detail} -- {'; '.join(problems)}") if problems else ("PASS", detail)


# ---------------------------------------------------------------------------
# The run
# ---------------------------------------------------------------------------
class Us101Run(kit.PlaytestRun):
    STORY = STORY
    COVERS = ("US-003", "US-005", "US-006")
    FEATURES = ("block_parry", "dodge", "stamina", "enemies", "movement")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.COMBAT_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    TAP_S = TAP_S
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.attacker_name = None
        self.hit_offsets = []      # seconds from the edge to the health drop, per measured swing
        self.swings = []           # one record per check that met a swing (report_extra)
        self.held_axes = set()
        self.held_keys = set()

    # --- the run ------------------------------------------------------------------------------
    def steps(self):
        try:
            walked, _ = self.measure("walk_to_fight_spot", self.step_walk)
            self.shot("fight_spot", needs=walked, needs_why="not taken: the player is not at the "
                                                            "fight spot")
            why = (f"not run: the player did not reach the fight spot next to the {ATTACKER}")
            control, _ = self.attempt("undefended_hit", self._guarded(self.step_undefended),
                                      needs=walked, needs_why=why)
            self._shot_after("undefended", walked)
            self.attempt("block_mitigates", self._guarded(self.step_block), needs=walked,
                         needs_why=why)
            self._shot_after("blocked", walked)
            self.attempt("perfect_block", self._guarded(self.step_perfect_block), needs=walked,
                         needs_why=why)
            self._shot_after("perfect_block", walked)
            why_control = why if not walked else (
                f"not run: the undefended control swing did not take {SLASH_DAMAGE:g} health at "
                "the fight spot, so a defended swing that does no damage would prove nothing")
            self.attempt("dodge_evades", self._guarded(self.step_dodge), needs=control,
                         needs_why=why_control)
            self._shot_after("dodged", control)
            self.attempt("jump_evades", self._guarded(self.step_jump), needs=control,
                         needs_why=why_control)
            self._shot_after("jumped", control)
            # Last: Riposte and Parried are never removed (N33)
            self.attempt("parry_negates", self._guarded(self.step_parry), needs=control,
                         needs_why=why_control)
            self._shot_after("parried", control)
        finally:
            self._release_held()

    def _shot_after(self, label, ran):
        self.shot(label, needs=ran, needs_why="not taken: the check before it did not run")

    def _guarded(self, fn):
        """fn() -> (status, detail, data), with a SwingError a FAIL and every held input released
        afterwards (a held LT or W would leak into the next check)."""
        def run():
            try:
                return fn()
            except SwingError as e:
                return "FAIL", str(e), None
            finally:
                self._release_held()
        return run

    # --- input (real input only: axis / key through the bridge) ---------------------------------
    def _axis(self, key, value):
        self.bridge.axis({key: value})
        if value:
            self.held_axes.add(key)
        else:
            self.held_axes.discard(key)

    def _press(self, key):
        self.bridge.press(key)
        self.held_keys.add(key)

    def _release(self, key):
        self.bridge.release(key)
        self.held_keys.discard(key)

    def _release_held(self):
        if self.bridge is None or self.abort:
            return
        try:
            if self.held_axes:
                self.bridge.axis({k: 0.0 for k in sorted(self.held_axes)})
                self.held_axes.clear()
            for key in sorted(self.held_keys):
                self.bridge.release(key)
            self.held_keys.clear()
        except ab.BridgeError as e:
            self.warnings.append(f"could not release the held input: {e}")
            self._lost("input release", e)

    def _raise_guard(self):
        """axis LT=1, then poll player.tags until Mordecai.State.Blocking (else SwingError)."""
        self._axis(LT, 1.0)
        deadline = time.monotonic() + GUARD_TIMEOUT_S
        while True:
            got = tags(player(self.bridge.state(["player"])))
            if TAG_BLOCKING in got:
                return f"{LT}=1: '{TAG_BLOCKING}'"
            if time.monotonic() >= deadline:
                raise SwingError(f"held {LT}=1: no '{TAG_BLOCKING}' in player.tags within "
                                 f"{GUARD_TIMEOUT_S:g}s (tags {got})")
            time.sleep(SAMPLE_POLL_S)

    # --- reading ---------------------------------------------------------------------------------
    def _read(self):
        return self.bridge.state(["player", "actors"], radius=ENEMY_RADIUS)

    def _wait(self, condition, timeout_s, what):
        try:
            return self.bridge.wait_until(condition, timeout_s=timeout_s, poll_s=EDGE_POLL_S)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            values = (e.response or {}).get("values")
            raise SwingError(f"{what}: not seen within {timeout_s:g}s (last values "
                             f"{kit.dumps(values, 800)})") from e

    def _cond(self, attacking):
        return attacker_condition(self.attacker_name, attacking)

    def expected_hit(self):
        return statistics.median(self.hit_offsets) if self.hit_offsets else NOMINAL_HIT_S

    @staticmethod
    def _sleep_until(t):
        left = t - time.monotonic()
        if left > 0:
            time.sleep(left)

    def _sample(self, t_edge, t_end):
        """Read player + actors until t_end: one compact record per read."""
        out = []
        while True:
            st = self._read()
            t = time.monotonic() - t_edge
            p = player(st)
            a = find_attacker(st, self.attacker_name)
            pos, apos = xy((p or {}).get("location")), xy((a or {}).get("location"))
            out.append({"t": round(t, 3), "mode": (p or {}).get("movement_mode"),
                        "tags": state_tags(p), "health": attr(p, "health"),
                        "dist": round(dist(pos, apos), 1) if pos and apos else None,
                        "attacker_tags": state_tags(a), "ai": (a or {}).get("ai_state")})
            if time.monotonic() >= t_end:
                return out
            time.sleep(SAMPLE_POLL_S)

    def _ensure_at_spot(self):
        """Walk back to the fight spot when a previous step moved the player off it."""
        p = player(self.bridge.state(["player"]))
        pos = xy((p or {}).get("location"))
        if pos is None or dist(pos, FIGHT_SPOT) <= RETURN_TOL:
            return ""
        try:
            r = self.bridge.walk_to(*FIGHT_SPOT, tol=SPOT_TOL)
        except ab.BridgeError as e:
            if e.kind != "gameplay":
                raise
            raise SwingError(f"the player was at {fmt_xy(pos)}, and walking back to the fight "
                             f"spot failed: {e}") from e
        return (f"walked back from {fmt_xy(pos)} to ({r['final'][0]:.0f}, {r['final'][1]:.0f}) "
                "first; ")

    # --- one swing -------------------------------------------------------------------------------
    def _pre_edge(self):
        """wait_until the attacker is out of Attack, then read player + actors.  The read must show
        it out of Attack too (else it is waited out again): every hit before it is in its values."""
        for _ in range(PRE_EDGE_TRIES):
            self._wait(self._cond(False), NOT_ATTACK_TIMEOUT_S,
                       f"the {ATTACKER} out of '{ATTACK}' (its swing over)")
            st = self._read()
            a = find_attacker(st, self.attacker_name)
            if a is None:
                raise SwingError(f"no {ATTACKER} '{self.attacker_name}' in actors[] (radius "
                                 f"{ENEMY_RADIUS})")
            if a.get("ai_state") != ATTACK:
                return st, a
        raise SwingError(f"the {ATTACKER} was back in '{ATTACK}' at each of {PRE_EDGE_TRIES} "
                         "reads after its swing ended")

    def _swing(self, check, act, precondition=None):
        """Meet one swing: the before read, the edge, act(t_edge, rec), the swing's end, the after
        read.  Returns the record (also kept in self.swings)."""
        rec = {"check": check, "timeline": [], "samples": [], "hit_s": None,
               "expected_hit_s": round(self.expected_hit(), 3)}
        self.swings.append(rec)
        st0, a0 = self._pre_edge()
        p0 = player(st0)
        pos0, apos0 = xy((p0 or {}).get("location")), xy(a0.get("location"))
        rec["before"] = vitals(p0)
        rec["before_tags"] = state_tags(p0)
        rec["attacker_before"] = {"ai_state": a0.get("ai_state"), "tags": state_tags(a0)}
        rec["dist_before"] = round(dist(pos0, apos0), 1) if pos0 and apos0 else None
        if p0 is None or p0.get("is_dead") is True or TAG_DEAD in tags(p0):
            raise SwingError(f"the player is dead or has no pawn before the swing "
                             f"({(p0 or {}).get('is_dead')}, tags {tags(p0)})")
        if rec["dist_before"] is None or rec["dist_before"] > REACH_CM:
            raise SwingError(f"the player at {fmt_xy(pos0)} is {rec['dist_before']} cm from the "
                             f"{ATTACKER} at {fmt_xy(apos0)} before the swing: out of its "
                             f"{REACH_CM:g} cm reach")
        h0 = rec["before"]["health"]
        if h0 is None or h0 <= 1.0:
            raise SwingError(f"player health {h0} before the swing")
        problem = precondition(p0, a0) if precondition else None
        if problem:
            raise SwingError(f"before the swing: {problem}")
        r = self._wait(self._cond(True), EDGE_TIMEOUT_S,
                       f"the {ATTACKER}'s next swing (ai_state '{ATTACK}')")
        t_edge = time.monotonic()
        rec["edge_wait_s"] = r.get("elapsed_s")
        act(t_edge, rec)
        self._wait(self._cond(False), NOT_ATTACK_TIMEOUT_S, f"the end of the {ATTACKER}'s swing")
        rec["swing_s"] = round(time.monotonic() - t_edge, 3)
        st1 = self._read()
        p1 = player(st1)
        a1 = find_attacker(st1, self.attacker_name)
        rec["after"] = vitals(p1)
        rec["after_tags"] = state_tags(p1)
        rec["after_mode"] = (p1 or {}).get("movement_mode")
        rec["attacker_after_tags"] = state_tags(a1)
        rec["dead_after"] = bool(p1 is None or p1.get("is_dead") is True or TAG_DEAD in tags(p1))
        rec["spent"] = spent(rec["before"], rec["after"])
        return rec

    def _mark(self, rec, label, t_edge):
        rec["timeline"].append((label, round(time.monotonic() - t_edge, 3)))

    def _watch_hit(self, t_edge, rec):
        """wait_until the player's health drops (poll_s 0): the hit time, for the later delays."""
        h0 = rec["before"]["health"]
        left = max(0.05, HIT_TIMEOUT_S - (time.monotonic() - t_edge))
        try:
            self.bridge.wait_until(f"player.attributes.health<{h0 - 1.0:.3f}", timeout_s=left,
                                   poll_s=EDGE_POLL_S)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            return
        rec["hit_s"] = round(time.monotonic() - t_edge, 3)
        self.hit_offsets.append(rec["hit_s"])

    def _at(self, t_edge, rec, delay):
        """Sleep until `delay` (shifted by the measured hit time) after the edge."""
        shift = shift_for(rec["expected_hit_s"])
        rec["shift_s"] = round(shift, 3)
        self._sleep_until(t_edge + delay + shift)

    # --- 1: the fight spot -------------------------------------------------------------------------
    def step_walk(self, b):
        try:
            r = b.walk_to(*FIGHT_SPOT, tol=SPOT_TOL)
        except ab.BridgeError as e:
            if e.kind != "gameplay":
                raise
            return "FAIL", f"walk_to({FIGHT_SPOT[0]:g}, {FIGHT_SPOT[1]:g}) failed: {e}", None
        st = b.state(["player", "actors"], radius=ENEMY_RADIUS)
        p, a = player(st), find_attacker(st)
        pos = xy((p or {}).get("location"))
        where = (f"reached ({r['final'][0]:.0f}, {r['final'][1]:.0f}) in {r['pulses']} pulses, "
                 f"{r.get('side_steps', 0)} side-steps")
        if a is None:
            return ("FAIL", f"{where}; no {ATTACKER} in actors[] (radius {ENEMY_RADIUS}; enemies "
                    f"{[e.get('label') for e in enemies(st)]})", None)
        self.attacker_name = a.get("name") or a.get("label")
        apos = xy(a.get("location"))
        d = dist(pos, apos) if pos and apos else None
        h = a.get("health") if isinstance(a.get("health"), dict) else {}
        what = (f"{where}; the {ATTACKER} '{self.attacker_name}' at {fmt_xy(apos)}, "
                f"{'?' if d is None else f'{d:.0f}'} cm away, ai_state {a.get('ai_state')}, health "
                f"{h.get('current')}/{h.get('max')}; player health {attr(p, 'health')}, stamina "
                f"{attr(p, 'stamina')}, posture {attr(p, 'posture')}")
        problems = []
        if a.get("is_dead") is not False:
            problems.append(f"the {ATTACKER} is_dead={a.get('is_dead')}")
        if d is None or d > REACH_CM:
            problems.append(f"the player is out of its {REACH_CM:g} cm reach")
        if problems:
            return "FAIL", f"{what} -- {'; '.join(problems)}", None
        return "PASS", what, None

    # --- 2: the control ----------------------------------------------------------------------------
    def step_undefended(self):
        note = self._ensure_at_spot()

        def pre(p0, a0):
            up = [t for t in tags(p0) if t in DEFENSE_TAGS]
            return f"the player already has {up} before the control swing" if up else None

        rec = self._swing("undefended_hit", self._watch_hit, pre)
        status, detail = judge_undefended(rec)
        return status, note + detail, None

    # --- 3: block ----------------------------------------------------------------------------------
    def step_block(self):
        note = self._ensure_at_spot()
        note += self._raise_guard() + " before the swing; "

        def pre(p0, a0):
            return (None if TAG_BLOCKING in tags(p0) else
                    f"'{TAG_BLOCKING}' is gone (tags {tags(p0)}): the guard dropped")

        rec = self._swing("block_mitigates", self._watch_hit, pre)
        status, detail = judge_block(rec)
        return status, note + detail, None

    # --- 4: perfect block ---------------------------------------------------------------------------
    def step_perfect_block(self):
        note = self._ensure_at_spot()

        def pre(p0, a0):
            return (f"'{TAG_BLOCKING}' is still up from the previous step (tags {tags(p0)})"
                    if TAG_BLOCKING in tags(p0) else None)

        def act(t_edge, rec):
            self._at(t_edge, rec, PERFECT_BLOCK_DELAY_S)
            self._mark(rec, f"{LT}=1 sent", t_edge)
            self._axis(LT, 1.0)
            self._mark(rec, f"{LT}=1 answered", t_edge)
            self._watch_hit(t_edge, rec)

        rec = self._swing("perfect_block", act, pre)
        status, detail = judge_perfect(rec)
        return status, note + detail, None

    # --- 5: dodge ------------------------------------------------------------------------------------
    def step_dodge(self):
        note = self._ensure_at_spot()

        def pre(p0, a0):
            up = [t for t in tags(p0) if t.startswith("Mordecai.State.Dodg") or t in DEFENSE_TAGS]
            return f"a dodge or defense tag is already on the player: {up}" if up else None

        def act(t_edge, rec):
            self._at(t_edge, rec, DODGE_DELAY_S - FORWARD_LEAD_S)
            self._mark(rec, f"{FORWARD_KEY} down", t_edge)
            self._press(FORWARD_KEY)
            self._sleep_until(t_edge + DODGE_DELAY_S + rec["shift_s"])
            self._mark(rec, f"{DODGE_KEY} tap sent", t_edge)
            self.bridge.tap(DODGE_KEY, hold_s=self.TAP_S)
            self._mark(rec, f"{DODGE_KEY} released", t_edge)
            self._release(FORWARD_KEY)   # the direction is locked at activation (AC-005.13)
            self._mark(rec, f"{FORWARD_KEY} up", t_edge)
            rec["samples"] = self._sample(t_edge, t_edge + rec["expected_hit_s"]
                                          + OBSERVE_AFTER_HIT_S)

        rec = self._swing("dodge_evades", act, pre)
        status, detail = judge_dodge(rec)
        return status, note + detail, None

    # --- 6: jump --------------------------------------------------------------------------------------
    def step_jump(self):
        note = self._ensure_at_spot()

        def pre(p0, a0):
            if p0.get("movement_mode") != "Walking":
                return f"movement_mode {p0.get('movement_mode')!r}, not Walking"
            if TAG_AIRBORNE in tags(p0):
                return f"'{TAG_AIRBORNE}' is on the grounded (Walking) player"
            return None

        def act(t_edge, rec):
            self._at(t_edge, rec, JUMP_DELAY_S)
            self._mark(rec, f"{JUMP_KEY} tap sent", t_edge)
            self.bridge.tap(JUMP_KEY, hold_s=self.TAP_S)
            self._mark(rec, f"{JUMP_KEY} released", t_edge)
            rec["samples"] = self._sample(t_edge, t_edge + rec["expected_hit_s"]
                                          + OBSERVE_AFTER_HIT_S)

        rec = self._swing("jump_evades", act, pre)
        status, detail = judge_jump(rec)
        return status, note + detail, None

    # --- 7: parry (last: N33) ---------------------------------------------------------------------------
    def step_parry(self):
        note = self._ensure_at_spot()
        note += self._raise_guard() + " before the swing; "

        def pre(p0, a0):
            problems = []
            if TAG_BLOCKING not in tags(p0):
                problems.append(f"'{TAG_BLOCKING}' is gone (tags {tags(p0)})")
            if TAG_RIPOSTE in tags(p0):
                problems.append(f"'{TAG_RIPOSTE}' is already on the player (N33)")
            if TAG_PARRIED in tags(a0):
                problems.append(f"'{TAG_PARRIED}' is already on the {ATTACKER} (N33)")
            return "; ".join(problems) or None

        def act(t_edge, rec):
            self._at(t_edge, rec, PARRY_DELAY_S)
            self._mark(rec, f"{RT}=1 sent", t_edge)
            self.bridge.axis({RT: 1.0}, hold_s=RT_TAP_S)
            self._mark(rec, f"{RT} released", t_edge)
            rec["samples"] = self._sample(t_edge, t_edge + rec["expected_hit_s"]
                                          + OBSERVE_AFTER_HIT_S)

        rec = self._swing("parry_negates", act, pre)
        status, detail = judge_parry(rec)
        return status, note + detail, None

    # --- report hooks -------------------------------------------------------------------------------
    def report_extra(self):
        return {"attacker": self.attacker_name, "hit_offsets_s": self.hit_offsets,
                "swings": self.swings}

    def md_sections(self):
        md = kit.pb.md_cell
        lines = ["", "## Swings", "",
                 f"Times are seconds after the {ATTACKER}'s not-Attack -> Attack edge was seen "
                 "(client clock). Spent = before - after, read with the attacker out of Attack "
                 "before the edge and after its swing ended.", "",
                 f"- Measured hit times (edge to the health drop): "
                 f"{', '.join(fmt_t(t) for t in self.hit_offsets) or 'none'}; nominal "
                 f"{fmt_t(NOMINAL_HIT_S)}",
                 "", "| Check | Edge wait | Inputs and hit | Spent | Reach at the hit | Swing |",
                 "|---|---|---|---|---|---|"]
        for rec in self.swings:
            reach = "-"
            if rec.get("samples"):
                _, reach = reach_at_hit(rec["samples"], rec.get("expected_hit_s") or NOMINAL_HIT_S)
            lines.append(f"| {rec['check']} | {md(str(rec.get('edge_wait_s')))} | "
                         f"{md(timeline_text(rec))} | {md(spent_text(rec))} | {md(reach)} | "
                         f"{md(fmt_t(rec.get('swing_s')))} |")
        if not self.swings:
            lines.append("| (none) | - | - | - | - | - |")
        return lines


Run = Us101Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us101Run, argv)


def main(argv=None):
    return kit.main(Us101Run, argv)


if __name__ == "__main__":
    sys.exit(main())
