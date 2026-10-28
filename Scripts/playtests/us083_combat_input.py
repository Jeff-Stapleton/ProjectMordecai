#!/usr/bin/env python3
"""US-083 Combat Input Playtest -- Tier 4: block, parry, attacks, dodge, jump and sprint on the
GAMEPAD and on the KEYBOARD AND MOUSE, played with REAL input

US-083 binds the player's combat input (MordecaiIMC_Runtime).  Block and parry need a button held
across state reads: the playtest client cannot hold a mouse button that way (B2), but it can hold
a gamepad trigger (`axis` without hold_s keeps the value until the next `axis` for that key).  So
this script plays block (LT held) and parry (RT pressed while LT blocks) for real, plus the
parry's input rules (a fresh RT press only: a partly pulled RT, or a block raised over a held RT,
must not parry), and the gamepad attack, dodge and jump keys.

Then it plays the keyboard and mouse inputs (AC-083.4..AC-083.8): LMB tap and hold, LeftAlt,
SpaceBar, and the LeftShift sprint toggle in lane L1.  The owning stories' scripts play these too
(us078 light_attack_lmb / heavy_attack_hold, us054 melee_lmb / dodge_lalt, us001 jump_space /
sprint_shift), but those scripts exit 1 on other stories' known failures, and only an exit-0 run
accepts an AC (CLAUDE.md Step 4d / 6), so the story's own script plays them as well.

Drives the PACKAGED game through the in-game agent bridge (Scripts/agent_bridge.py): every action
is an axis, key or click event injected into Slate (axis / key / click; walk_to is W/A/S/D taps),
every observation a state read, a wait or a screenshot.  Nothing calls game functions;
bridge.jsonl in the run dir is the audit trail.  The run framework is Scripts/playtest_kit.py
(Specs/story_playtests_v1.md section 2).

Steps (every step is a check; --plan prints them as the story's Steps table):
  1-4   First, from the spawn and before any dodge: walk to lane L1 (west along y=0 between the
        enemies' spawn points to (-650, 0), south to us001's (-700, -650) and the lane start
        (-700, -800); screenshot), then run up along the lane (+X) to (-300, -800).
  5     Sprint: tap LeftShift, hold W 1.0 s: > 700 cm.  Tap LeftShift again, hold S 1.0 s while
        reading player.speed: the peak must be a walk (500..750 cm/s), so the second tap ended the
        sprint.
  6-9   LT held: Blocking.  RT pressed while blocking: a parry (Parrying / ParryWhiff / Riposte)
        that drops the guard and does not attack.  LT still held: the guard comes back once the
        parry and its whiff end, with no attack in between.  LT released: Blocking gone.
  10-11 LT held and RT pulled to 0.3 (below the 0.5 actuation): no parry, the guard stays.  RT
        held first, then LT: a block, no parry.
  12-15 RT tapped 0.1 s: a light attack (Attacking, no Mordecai.Attack.Heavy).  RT held 0.8 s: a
        heavy attack on the release (Mordecai.Attack.Heavy).  Gamepad B: a dodge.  Once that dodge
        is over (no Dodging tag, Walking), Gamepad A: a jump (Falling).  Both triggers are released
        before the keyboard and mouse half.
  16-19 Left click at the viewport centre (640, 360), the bridge's default click: a light attack.
        Left click held 0.8 s: a heavy attack on the release.  LeftAlt: a dodge.  Once that dodge
        is over (no Dodging tag, Walking), SpaceBar: a jump (Falling).
Why the walk and the sprint come first: the arena's enemies (N14) follow the player at
400 cm/s and stop about 200 cm from it (their AttackRange).  A player walking straight leaves them
behind, but walks into any that stand ahead.  At the spawn, before any dodge, they stand at their
spawn points (196, 0) and (-123, +/-153), clear of the route; the run-up along the lane leaves the
ones that followed behind the player before its 1 s of W.  When the walk came after the combat
steps, a backward dodge had pushed the player into one (boxed in at the spawn), or they had cut
the corner onto the sprint line (Saved/Playtests/20261024-110250-us083, -115717-us083).
Every attack and dodge starts from idle (no attack, block or parry tag left).  The combat steps
read tags only, so they work wherever the sprint left the player; nothing needs an enemy.  A failed
check SKIPs the checks that depend on it; independent checks still run.

Usage:
  py Scripts/playtests/us083_combat_input.py                    fresh package: launch, play, quit
  py Scripts/playtests/us083_combat_input.py --allow-stale      STALE package: diagnostic only
  py Scripts/playtests/us083_combat_input.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us083_combat_input.py --plan             print the Steps table; no game
  py Scripts/playtests/us083_combat_input.py --attach-port N [--token T]   diagnostic only
  py Scripts/playtest_story.py US-083                           Tier 3 boot + this script

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us083/ (report.md, report.json, game.log, bridge.jsonl,
bridge_NN.png).  Open every screenshot with the Read tool and state what is on screen.

Exit codes: 0 every check passed (WARN allowed); 1 a gameplay check FAILED; 2 infrastructure.
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

STORY = "US-083"
TITLE = "US-083 combat input playtest: gamepad, keyboard and mouse (Tier 4, real input)"
TAP_S = 0.1

LT = "Gamepad_LeftTriggerAxis"     # block (held)
RT = "Gamepad_RightTriggerAxis"    # light (tap) / heavy (hold) attack; parry while blocking
DODGE_KEY = "Gamepad_FaceButton_Right"   # B
JUMP_KEY = "Gamepad_FaceButton_Bottom"   # A
TAG_BLOCKING = "Mordecai.State.Blocking"
TAG_PARRYING = "Mordecai.State.Parrying"
TAG_WHIFF = "Mordecai.State.ParryWhiff"
TAG_RIPOSTE = "Mordecai.State.Riposte"
PARRY_TAGS = (TAG_PARRYING, TAG_WHIFF, TAG_RIPOSTE)
TAG_ATTACKING = "Mordecai.State.Attacking"
TAG_HEAVY = "Mordecai.Attack.Heavy"
TAG_DODGE_SUBSTR = "dodg"          # Mordecai.State.Dodging, then DodgeCooldown
TAG_DODGING = "Mordecai.State.Dodging"   # the dodge's active phase (its launch)

# --- keyboard and mouse (AC-083.4..AC-083.8) ------------------------------------------------
CLICK_POINT = (640, 360)           # the viewport centre: the arena, no widget (as us078)
LMB_HEAVY_S = 0.8                  # > HeavyAttackHoldSeconds (0.4)
KB_DODGE_KEY = "LeftAlt"
KB_JUMP_KEY = "SpaceBar"
SPRINT_KEY = "LeftShift"
# Lane L1 (y=-800, x -750..+950 is clear).  The walk starts at the spawn (0, 0), before any dodge:
# the Frontliners still stand at their spawn points (196, 0) and (-123, +/-153), within their
# 200 cm attack range, so the route leaves between the two western ones (west along y=0, 153 cm
# from each), then goes south, west of Box_02, to us001's lane start, 100 cm off Cover_01.  They
# follow at 400 cm/s (a walk is 600) and cut the corner of the turn into the lane, so the player
# runs up along the lane (+X) before the sprint: anything that followed is then behind it.
LANE_WAYPOINTS = (("walk_lane_wp1", -650.0, 0.0, "west along y=0, between the enemies' spawn "
                   "points"),
                  ("walk_lane_wp2", -700.0, -650.0, "south, west of Box_02, to us001's waypoint"))
LANE_START = (-700.0, -800.0)
RUN_UP = (-300.0, -800.0)          # the sprint covers ~750 cm + its stop from here: lane ends +950
SETTLE_TIMEOUT_S = 2.0             # wait for the player to stop before measuring
SPRINT_HOLD_S = 1.0
SPRINT_MIN = 700.0                 # cm: more than any walk can cover in 1.0 s
SPRINT_SPEED_MIN = 750.0           # cm/s: above a walk (600), below a sprint (900)
WALK_SPEED_MIN = 500.0             # cm/s: a free walk reaches 600
SPEED_POLL_S = 0.05

PARTIAL_RT = 0.3                   # below the 0.5 actuation threshold of the parry's Pressed
RT_LEAD_S = 0.15                   # RT down this long before LT goes up
RT_TAP_S = 0.1
RT_HEAVY_S = 0.8                   # > HeavyAttackHoldSeconds (0.4)
POLL_S = 0.05
GUARD_TIMEOUT_S = 1.0
PARRY_TIMEOUT_S = 0.5
GUARD_BACK_TIMEOUT_S = 2.5         # parry window 0.5 s + whiff 0.6 s, with margin
NO_PARRY_WATCH_S = 0.6
IDLE_TIMEOUT_S = 3.0               # a Longsword heavy attack lasts 1.25 s
LIGHT_TIMEOUT_S = 1.0
HEAVY_TIMEOUT_S = 0.5
DODGE_TIMEOUT_S = 0.5
JUMP_TIMEOUT_S = 1.0

SCREENSHOTS = {
    "blocking": "the player in the diorama arena with LT held (blocking); there is no block "
                "animation or guard visual, so the block itself is checked through player.tags",
    "lane_start": "the same fixed camera angle with the player at the lane L1 start (-700, -800), "
                  "near the south-west corner of the arena, and the combat HUD; the sprint "
                  "starts here",
}
KNOWN_ISSUES = (
    "Block and parry on the mouse (RMB hold, RMB+LMB, AC-083.11) are not played: the playtest "
    "client holds a mouse button only inside one blocking click (B2). This script plays the "
    "same actions on the gamepad triggers.",
    "The arena's enemies spawn inside aggro range of the player, follow it and can body-block it "
    "(N14, Specs/agent_playtest_plan_v1.md Known issues backlog; the swarm is left to the T1 "
    "test maps, issue #46): an enemy in the way can stop the "
    "walk to lane L1 (walk_lane_*), which then SKIPs the sprint, or shorten the sprint itself. "
    "No step expects that: the walk and the sprint come first, from the spawn before any dodge, "
    "between the enemies' spawn points, and the run-up along the lane leaves the ones that "
    "followed behind the player before the sprint.",
)

PLAN = (
    # --- first, from the spawn before any dodge: lane L1 and the LeftShift sprint (AC-083.8) ---
    *[Step(check, f"walk_to({x:g}, {y:g}) -> within 60 cm (waypoint to lane L1: {why})")
      for check, x, y, why in LANE_WAYPOINTS],
    Step("walk_lane_start", f"walk_to({LANE_START[0]:g}, {LANE_START[1]:g}) -> within 60 cm "
         "(lane L1 start)"),
    Step("walk_lane_run_up", f"walk_to({RUN_UP[0]:g}, {RUN_UP[1]:g}) -> within 60 cm (along lane "
         "L1, +X: the enemies that followed the player end up behind it)"),
    Step("sprint_left_shift", f"at the run-up point: tap {SPRINT_KEY}, hold W {SPRINT_HOLD_S:g}s "
         f"-> dx > {SPRINT_MIN:g} cm; tap {SPRINT_KEY} again, hold S {SPRINT_HOLD_S:g}s reading "
         f"player.speed every {SPEED_POLL_S:g}s -> back to a walk (peak {WALK_SPEED_MIN:g}.."
         f"{SPRINT_SPEED_MIN:g} cm/s)", proves=("AC-083.8", "AC-083.3")),
    # --- the gamepad ---
    Step("block_lt_hold", f"axis {LT}=1 (held) -> '{TAG_BLOCKING}' in player.tags within "
         f"{GUARD_TIMEOUT_S:g}s", proves=("AC-083.10", "AC-083.3")),
    Step("parry_rt_while_blocking", f"LT held, axis {RT}=1 -> within {PARRY_TIMEOUT_S:g}s a parry "
         f"tag ({', '.join(PARRY_TAGS)}) with no '{TAG_BLOCKING}' and no '{TAG_ATTACKING}'; then "
         "RT=0", proves=("AC-083.10", "AC-083.3")),
    Step("guard_returns_after_parry", f"LT still held -> '{TAG_BLOCKING}' again without a parry "
         f"tag within {GUARD_BACK_TIMEOUT_S:g}s, and no '{TAG_ATTACKING}' at any read meanwhile",
         proves=("AC-083.10",)),
    Step("block_release_lt", f"axis {LT}=0 -> '{TAG_BLOCKING}' gone within {GUARD_TIMEOUT_S:g}s",
         proves=("AC-083.10",)),
    Step("partial_rt_no_parry", f"LT held (Blocking), axis {RT}={PARTIAL_RT:g} for "
         f"{NO_PARRY_WATCH_S:g}s -> no parry tag and '{TAG_BLOCKING}' at every read; then both 0",
         proves=("AC-083.3",)),
    Step("block_over_held_rt_no_parry", f"axis {RT}=1, {RT_LEAD_S:g}s later {LT}=1 -> "
         f"'{TAG_BLOCKING}' within {GUARD_TIMEOUT_S:g}s and no parry tag for {NO_PARRY_WATCH_S:g}s "
         "more; then both 0", proves=("AC-083.3",)),
    Step("light_attack_rt_tap", f"idle -> axis {RT}=1 hold_s {RT_TAP_S:g} -> '{TAG_ATTACKING}' "
         f"without '{TAG_HEAVY}' within {LIGHT_TIMEOUT_S:g}s", proves=("AC-083.3",)),
    Step("heavy_attack_rt_hold", f"idle -> axis {RT}=1 hold_s {RT_HEAVY_S:g} -> '{TAG_HEAVY}' "
         f"in player.tags within {HEAVY_TIMEOUT_S:g}s of the release", proves=("AC-083.3",)),
    Step("dodge_gamepad_b", f"tap {DODGE_KEY} (B) -> a Dodging tag in player.tags within "
         f"{DODGE_TIMEOUT_S:g}s", proves=("AC-083.3",)),
    Step("jump_gamepad_a", f"wait until the dodge is over (no '{TAG_DODGING}') and "
         f"player.movement_mode==\"Walking\", tap {JUMP_KEY} (A) -> wait_until "
         f"player.movement_mode==\"Falling\" within {JUMP_TIMEOUT_S:g}s", proves=("AC-083.3",)),
    # --- keyboard and mouse: AC-083.4..AC-083.7, with the LeftShift sprint above for AC-083.8
    # (the owning stories' scripts exit 1 on other stories' failures, so this script plays these
    # inputs too) ---
    Step("light_attack_lmb_tap", f"idle -> click Left at ({CLICK_POINT[0]}, {CLICK_POINT[1]}) "
         f"(the bridge's default click) -> '{TAG_ATTACKING}' without '{TAG_HEAVY}' within "
         f"{LIGHT_TIMEOUT_S:g}s", proves=("AC-083.4", "AC-083.3")),
    Step("heavy_attack_lmb_hold", f"idle -> click Left at ({CLICK_POINT[0]}, {CLICK_POINT[1]}) "
         f"held {LMB_HEAVY_S:g}s -> '{TAG_HEAVY}' in player.tags within {HEAVY_TIMEOUT_S:g}s of "
         "the release", proves=("AC-083.5", "AC-083.3")),
    Step("dodge_left_alt", f"idle (and no Dodging / DodgeCooldown tag left) -> tap {KB_DODGE_KEY} "
         f"-> a Dodging tag in player.tags within {DODGE_TIMEOUT_S:g}s",
         proves=("AC-083.6", "AC-083.3")),
    Step("jump_space_bar", f"wait until the dodge is over (no '{TAG_DODGING}') and "
         f"player.movement_mode==\"Walking\", tap {KB_JUMP_KEY} -> wait_until "
         f"player.movement_mode==\"Falling\" within {JUMP_TIMEOUT_S:g}s",
         proves=("AC-083.7", "AC-083.3")),
)
PENDING = {"AC-083.11": "bridge cannot drive: the playtest client cannot hold a mouse button "
                        "across state reads (B2), so RMB block and RMB+LMB parry are not played"}

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA
set_log_stream = kit.set_log_stream


def _tags(st):
    tags = kit.section(st, "player").get("tags")
    return [str(t) for t in tags] if isinstance(tags, list) else []


def _parried(tags):
    return any(t in tags for t in PARRY_TAGS)


def _dodging(tags):
    return any(TAG_DODGE_SUBSTR in t.lower() for t in tags)


def _idle(tags):
    return TAG_ATTACKING not in tags and TAG_BLOCKING not in tags and not _parried(tags)


def _idle_no_dodge(tags):
    return _idle(tags) and not _dodging(tags)


class Us083Run(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("block_parry", "melee_attack", "dodge", "movement", "sprint")
    TITLE = TITLE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    PENDING = PENDING
    TAP_S = TAP_S
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.measurements = []   # the sprint and its toggle-back (report_extra / md_sections)

    def steps(self):
        # The walk and the sprint first, before any dodge has pushed the player into an enemy
        # (module docstring)
        self.step_lane_and_sprint()
        self.step_block_and_parry()
        self.step_parry_needs_a_fresh_press()
        self.step_attacks_dodge_jump()
        self.step_keyboard_mouse()

    # --- helpers ------------------------------------------------------------------------------
    def _poll(self, seconds, until=None):
        """Read player.tags every POLL_S for up to `seconds`, stopping early once until(tags) is
        true.  Returns (the tags that matched or None, every tag list read)."""
        seen = []
        deadline = time.monotonic() + seconds
        while True:
            tags = _tags(self.bridge.state(["player"]))
            seen.append(tags)
            if until is not None and until(tags):
                return tags, seen
            if time.monotonic() >= deadline:
                return None, seen
            time.sleep(POLL_S)

    def _release_triggers(self):
        """Both triggers back to 0 (a held axis persists until released or the connection ends)."""
        try:
            self.bridge.axis({LT: 0.0, RT: 0.0})
        except ab.BridgeError as e:
            self.warnings.append(f"could not release the triggers: {e}")
            self._lost("trigger release", e)

    def _wait_idle(self, idle=_idle):
        """Wait until no attack, block or parry tag is left (idle=_idle_no_dodge: no dodge tag
        either).  Returns None or why not."""
        got, seen = self._poll(IDLE_TIMEOUT_S, idle)
        if got is None:
            return (f"the player is not idle {IDLE_TIMEOUT_S:g}s after the previous step (tags "
                    f"{seen[-1]}), so this step cannot be told apart from it")
        return None

    # --- reading the player (as us001_movement.py) ---------------------------------------
    @staticmethod
    def _player(b):
        p = kit.section(b.state(["player"]), "player")
        loc = p.get("location")
        if p.get("valid") is not True or not (isinstance(loc, list) and len(loc) >= 2):
            return None, p
        return (float(loc[0]), float(loc[1])), p

    @staticmethod
    def _settle(b, timeout_s):
        """wait_until the player stops.  Returns the seconds it took, or None on a timeout."""
        try:
            r = b.wait_until("player.speed<1", timeout_s=timeout_s)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            return None
        return float(r.get("elapsed_s") or 0.0)

    def _record(self, check, what, p0, p1, extra=None):
        rec = {"check": check, "input": what, "start": list(p0) if p0 else None,
               "end": list(p1) if p1 else None,
               "dx": round(p1[0] - p0[0], 1) if p0 and p1 else None,
               "dy": round(p1[1] - p0[1], 1) if p0 and p1 else None}
        rec.update(extra or {})
        self.measurements.append(rec)
        return rec

    def _hold_and_measure(self, key, seconds, before=None, after=None):
        """Read the position, [before()], hold `key` for `seconds`, wait for the player to stop,
        read again, [after()].  Returns ((p0, p1, held_s, stop_s), None) or (None, why)."""
        b = self.bridge
        p0, raw = self._player(b)
        if p0 is None:
            return None, f"player section invalid before the input ({raw.get('reason')})"
        if before:
            before()
        r = b.hold(key, seconds)
        stop_s = self._settle(b, SETTLE_TIMEOUT_S)
        p1, raw = self._player(b)
        if after:
            after()
        if p1 is None:
            return None, f"player section invalid after the input ({raw.get('reason')})"
        return (p0, p1, r.get("held_s"), stop_s), None

    def _hold_watching_speed(self, key, seconds):
        """Press `key`, read player.speed every SPEED_POLL_S for `seconds`, release it, wait for
        the player to stop.  Returns ((p0, p1, peak_speed), None) or (None, why)."""
        b = self.bridge
        p0, raw = self._player(b)
        if p0 is None:
            return None, f"player section invalid before {key} ({raw.get('reason')})"
        peak = 0.0
        b.press(key)
        try:
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                speed = kit.section(b.state(["player"]), "player").get("speed")
                if isinstance(speed, (int, float)):
                    peak = max(peak, float(speed))
                time.sleep(SPEED_POLL_S)
        finally:
            b.release(key)
        self._settle(b, SETTLE_TIMEOUT_S)
        p1, raw = self._player(b)
        if p1 is None:
            return None, f"player section invalid after {key} ({raw.get('reason')})"
        return (p0, p1, peak), None

    # --- 6-9: block and parry ------------------------------------------------------------
    def step_block_and_parry(self):
        def block():
            self.bridge.axis({LT: 1.0})
            got, seen = self._poll(GUARD_TIMEOUT_S, lambda t: TAG_BLOCKING in t)
            if got is None:
                return ("FAIL", f"held {LT}=1: no '{TAG_BLOCKING}' in player.tags within "
                        f"{GUARD_TIMEOUT_S:g}s (tags {seen[-1]})", None)
            return "PASS", f"held {LT}=1: '{TAG_BLOCKING}' (tags {got})", None

        blocked, _ = self.attempt("block_lt_hold", block)
        self.shot("blocking", needs=blocked, needs_why="not taken: LT did not block")
        why = "not run: LT did not block"

        def parry():
            self.bridge.axis({RT: 1.0})
            try:
                got, seen = self._poll(PARRY_TIMEOUT_S, _parried)
            finally:
                self.bridge.axis({RT: 0.0})
            if got is None:
                return ("FAIL", f"LT held, {RT}=1: no parry tag ({', '.join(PARRY_TAGS)}) within "
                        f"{PARRY_TIMEOUT_S:g}s (tags {seen[-1]})", None)
            detail = (f"LT held, {RT}=1: {', '.join(t for t in got if t in PARRY_TAGS)} "
                      f"(tags {got})")
            problems = []
            if TAG_BLOCKING in got:
                problems.append(f"'{TAG_BLOCKING}' is still on: the parry did not drop the guard")
            if TAG_ATTACKING in got:
                problems.append(f"'{TAG_ATTACKING}' is on: the RT press also attacked")
            if problems:
                return "FAIL", f"{detail} -- {'; '.join(problems)}", None
            return "PASS", detail, None

        parried, _ = self.attempt("parry_rt_while_blocking", parry, needs=blocked, needs_why=why)

        def guard_back():
            t0 = time.monotonic()
            got, seen = self._poll(GUARD_BACK_TIMEOUT_S,
                                   lambda t: TAG_BLOCKING in t and not _parried(t))
            attacked = [t for t in seen if TAG_ATTACKING in t]
            if attacked:
                return ("FAIL", f"LT still held: an attack fired during the parry (tags "
                        f"{attacked[0]}); the RT release must not attack out of a parry", None)
            if got is None:
                return ("FAIL", f"LT still held: the guard did not come back within "
                        f"{GUARD_BACK_TIMEOUT_S:g}s (tags {seen[-1]})", None)
            return ("PASS", f"LT still held: '{TAG_BLOCKING}' again after "
                    f"{time.monotonic() - t0:.2f}s, once the parry and its whiff ended; no "
                    f"attack at any of {len(seen)} reads (tags {got})", None)

        self.attempt("guard_returns_after_parry", guard_back, needs=parried,
                     needs_why="not run: no parry started")

        def release():
            self.bridge.axis({LT: 0.0})
            got, seen = self._poll(GUARD_TIMEOUT_S, lambda t: TAG_BLOCKING not in t)
            if got is None:
                return ("FAIL", f"{LT}=0: '{TAG_BLOCKING}' still in player.tags "
                        f"{GUARD_TIMEOUT_S:g}s after the release (tags {seen[-1]})", None)
            return "PASS", f"{LT}=0: '{TAG_BLOCKING}' gone (tags {got})", None

        self.attempt("block_release_lt", release, needs=blocked, needs_why=why)
        self._release_triggers()

    # --- 10-11: only a fresh RT press parries ---------------------------------------------------
    def step_parry_needs_a_fresh_press(self):
        def partial():
            not_idle = self._wait_idle()
            if not_idle:
                return "FAIL", not_idle, None
            self.bridge.axis({LT: 1.0})
            try:
                got, seen = self._poll(GUARD_TIMEOUT_S, lambda t: TAG_BLOCKING in t)
                if got is None:
                    return ("FAIL", f"held {LT}=1: no '{TAG_BLOCKING}' within "
                            f"{GUARD_TIMEOUT_S:g}s (tags {seen[-1]}), so the RT pull was not "
                            "tried", None)
                self.bridge.axis({RT: PARTIAL_RT})
                _, seen = self._poll(NO_PARRY_WATCH_S)
            finally:
                self._release_triggers()
            parried = [t for t in seen if _parried(t)]
            if parried:
                return ("FAIL", f"LT held, {RT}={PARTIAL_RT:g}: a parry started (tags "
                        f"{parried[0]}); a trigger pulled below 0.5 must not parry", None)
            dropped = [t for t in seen if TAG_BLOCKING not in t]
            if dropped:
                return ("FAIL", f"LT held, {RT}={PARTIAL_RT:g}: the guard dropped (tags "
                        f"{dropped[0]})", None)
            return ("PASS", f"LT held, {RT}={PARTIAL_RT:g} for {NO_PARRY_WATCH_S:g}s: no parry, "
                    f"'{TAG_BLOCKING}' at all {len(seen)} reads", None)

        self.attempt("partial_rt_no_parry", partial)

        def over_held():
            not_idle = self._wait_idle()
            if not_idle:
                return "FAIL", not_idle, None
            self.bridge.axis({RT: 1.0})
            try:
                time.sleep(RT_LEAD_S)
                self.bridge.axis({LT: 1.0})
                got, seen = self._poll(GUARD_TIMEOUT_S, lambda t: TAG_BLOCKING in t)
                _, more = self._poll(NO_PARRY_WATCH_S)
            finally:
                # RT was held past the heavy hold time: its release is a heavy attack (harmless)
                self._release_triggers()
            parried = [t for t in seen + more if _parried(t)]
            if parried:
                return ("FAIL", f"{RT} held, then {LT}=1: a parry started (tags {parried[0]}); "
                        "raising the block over a held attack button must not parry", None)
            if got is None:
                return ("FAIL", f"{RT} held, then {LT}=1: no '{TAG_BLOCKING}' within "
                        f"{GUARD_TIMEOUT_S:g}s (tags {seen[-1]})", None)
            return ("PASS", f"{RT} held, then {LT}=1: '{TAG_BLOCKING}' and no parry at "
                    f"{len(seen) + len(more)} reads", None)

        self.attempt("block_over_held_rt_no_parry", over_held)

    # --- shared: attacks, dodges and jumps -------------------------------------------------
    def _attack(self, check, send, what, timeout_s, heavy):
        """From idle, send() the input; PASS when the attack's tag shows: the heavy attack's
        Mordecai.Attack.Heavy, or for a light attack Mordecai.State.Attacking without it.
        what(response) describes the input for the detail."""
        def started(tags):
            return TAG_HEAVY in tags if heavy else (TAG_ATTACKING in tags and TAG_HEAVY not in tags)

        want = f"'{TAG_HEAVY}'" if heavy else f"'{TAG_ATTACKING}' without '{TAG_HEAVY}'"

        def fn():
            not_idle = self._wait_idle()
            if not_idle:
                return "FAIL", not_idle, None
            label = what(send())
            t0 = time.monotonic()  # the input call returns after the release
            got, seen = self._poll(timeout_s, started)
            if got is None:
                return ("FAIL", f"idle; {label}: no {want} in player.tags within {timeout_s:g}s "
                        f"(tags {seen[-1]})", None)
            return ("PASS", f"idle; {label}: {want} after {time.monotonic() - t0:.2f}s (tags "
                    f"{got})", None)

        return self.attempt(check, fn)

    def _rt_attack(self, check, hold_s, timeout_s, heavy):
        return self._attack(check, lambda: self.bridge.axis({RT: 1.0}, hold_s=hold_s),
                            lambda r: f"{RT}=1 held {hold_s:g}s", timeout_s, heavy)

    def _lmb_attack(self, check, hold_s, timeout_s, heavy):
        x, y = CLICK_POINT

        def what(r):
            return (f"Left click at ({x}, {y})" + (f" held {hold_s:g}s" if hold_s is not None
                                                     else "") + f"; Slate hit {r.get('hit')}")

        return self._attack(check, lambda: self.bridge.click(x, y, button="Left", hold_s=hold_s),
                            what, timeout_s, heavy)

    def _dodge(self, check, key, idle):
        def fn():
            not_idle = self._wait_idle(idle)
            if not_idle:
                return "FAIL", not_idle, None
            self.bridge.tap(key, hold_s=self.TAP_S)
            got, seen = self._poll(DODGE_TIMEOUT_S, _dodging)
            if got is None:
                return ("FAIL", f"tapped {key}: no Dodging tag within {DODGE_TIMEOUT_S:g}s "
                        f"(tags {seen[-1]})", None)
            return "PASS", f"tapped {key}: tags {got}", None

        return self.attempt(check, fn)

    def _on_the_ground(self):
        """Before a jump: wait until the player is Walking and the dodge's active phase, whose
        launch is Falling too, has ended (a read can show Dodging and Walking just before the
        launch).  Returns (None, what) or (why not, None)."""
        def grounded(st):
            p = kit.section(st, "player")
            return p.get("movement_mode") == "Walking" and TAG_DODGING not in _tags(st)

        try:
            self.bridge.wait_for(grounded, timeout_s=IDLE_TIMEOUT_S, sections=["player"],
                                 poll_s=POLL_S)
        except ab.BridgeError as e:
            if e.code != "wait_for_timeout":
                raise
            p = kit.section(e.response or {}, "player")
            return (f"the dodge is not over or the player is not Walking {IDLE_TIMEOUT_S:g}s "
                    f"after it (movement_mode {p.get('movement_mode')!r}, tags "
                    f"{_tags(e.response or {})}), so a jump cannot be told apart from the "
                    "dodge's launch", None)
        return None, "the dodge over, Walking"

    def _jump(self, check, key):
        def fn():
            why, before = self._on_the_ground()
            if why:
                return "FAIL", why, None
            self.bridge.tap(key, hold_s=self.TAP_S)
            try:
                r = self.bridge.wait_until('player.movement_mode=="Falling"',
                                           timeout_s=JUMP_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                return ("FAIL", f"{before}; tapped {key}: player.movement_mode stayed "
                        f"{(e.response or {}).get('values')} for {JUMP_TIMEOUT_S:g}s", None)
            return "PASS", f"{before}; tapped {key}: Falling after {r.get('elapsed_s')}s", None

        return self.attempt(check, fn)

    # --- 12-15: gamepad attacks, dodge, jump ---------------------------------------------------
    def step_attacks_dodge_jump(self):
        self._rt_attack("light_attack_rt_tap", RT_TAP_S, LIGHT_TIMEOUT_S, heavy=False)
        self._rt_attack("heavy_attack_rt_hold", RT_HEAVY_S, HEAVY_TIMEOUT_S, heavy=True)
        self._dodge("dodge_gamepad_b", DODGE_KEY, _idle)
        # The dodge launches the player (Falling for a few frames): only judge the jump from the
        # ground, once the dodge is over
        self._jump("jump_gamepad_a", JUMP_KEY)

    # --- 16-19: keyboard and mouse attacks, dodge, jump (AC-083.4..AC-083.7) ---------------
    def step_keyboard_mouse(self):
        # Both triggers are 0 here: the gamepad steps release them (axis hold_s, _release_triggers)
        self._lmb_attack("light_attack_lmb_tap", None, LIGHT_TIMEOUT_S, heavy=False)
        self._lmb_attack("heavy_attack_lmb_hold", LMB_HEAVY_S, HEAVY_TIMEOUT_S, heavy=True)
        # No dodge tag left either: a DodgeCooldown from the B dodge would pass the check alone
        self._dodge("dodge_left_alt", KB_DODGE_KEY, _idle_no_dodge)
        self._jump("jump_space_bar", KB_JUMP_KEY)

    # --- 1-5: lane L1 and the LeftShift sprint (AC-083.8), first -------------------------
    def _sprint(self, check):
        """Tap LeftShift, hold W and measure; tap LeftShift again, then hold S as long and require
        a walk's speed back (the second tap ended the sprint).  As us001_movement.py sprint_shift
        with verify_toggle_back=True."""
        def fn():
            b = self.bridge
            got, err = self._hold_and_measure(
                "W", SPRINT_HOLD_S, before=lambda: b.tap(SPRINT_KEY, hold_s=self.TAP_S),
                after=lambda: b.tap(SPRINT_KEY, hold_s=self.TAP_S))  # toggle back
            if got is None:
                return "FAIL", err, None
            p0, p1, held_s, _ = got
            dx = p1[0] - p0[0]
            rec = self._record(check, f"tap {SPRINT_KEY}, hold W {SPRINT_HOLD_S:g}s", p0, p1,
                               {"held_s": held_s})
            detail = (f"tapped {SPRINT_KEY}, held W {held_s}s: dx {dx:.0f} cm (need > "
                      f"{SPRINT_MIN:g}); tapped {SPRINT_KEY} again to toggle back")
            sprinted = dx > SPRINT_MIN

            # The toggle-back is judged by speed, not distance: a speed reading cannot be shortened
            # by a frame hitch or an enemy in the way, and a sprint (900 cm/s) and a walk
            # (600 cm/s) are far apart.
            back, err = self._hold_watching_speed("S", SPRINT_HOLD_S)
            if back is None:
                return "FAIL", f"{detail}; toggle-back not measured: {err}", rec
            q0, q1, peak = back
            self._record(f"{check}_toggle_back", f"hold S {SPRINT_HOLD_S:g}s after the second "
                         f"{SPRINT_KEY} tap", q0, q1, {"peak_speed": round(peak, 1)})
            rec["sprint_left_on"] = peak > SPRINT_SPEED_MIN
            detail += (f"; then held S {SPRINT_HOLD_S:g}s: peak speed {peak:.0f} cm/s, "
                       f"{q0[0] - q1[0]:.0f} cm back (need a walk: {WALK_SPEED_MIN:g}..."
                       f"{SPRINT_SPEED_MIN:g} cm/s)")
            if rec["sprint_left_on"]:
                detail += (" -- the second tap did not end the sprint" if sprinted else
                           " -- the first tap did not start the sprint but the second one did "
                           "(toggle out of phase)")
                return "FAIL", detail, rec
            if peak < WALK_SPEED_MIN:
                detail += (" -- the player never reached walking speed on the way back (blocked? "
                           "N14), so the toggle-back is unverified")
                return "FAIL", detail, rec
            return ("PASS" if sprinted else "FAIL"), detail, rec

        return fn

    def step_lane_and_sprint(self):
        ok = True
        for check, x, y, _ in LANE_WAYPOINTS:
            ok, _ = self.walk(check, x, y, needs=ok, needs_why="not run: the previous waypoint "
                              "was not reached")
        lane, _ = self.walk("walk_lane_start", *LANE_START, needs=ok,
                            needs_why="not run: a lane waypoint was not reached")
        self.shot("lane_start", needs=lane, needs_why="not taken: the player is not at the lane "
                                                      "start")
        run_up, _ = self.walk("walk_lane_run_up", *RUN_UP, needs=lane,
                              needs_why="not run: the player is not at the lane start")
        self.attempt("sprint_left_shift", self._sprint("sprint_left_shift"), needs=run_up,
                     needs_why="not run: the player is not at the run-up point")

    # --- report hooks ------------------------------------------------------------------------
    def report_extra(self):
        return {"movement": self.measurements}

    def md_sections(self):
        md = kit.pb.md_cell
        lines = ["", "## Sprint measurements", "",
                 "| Check | Input | Start | End | dx | Peak speed |", "|---|---|---|---|---|---|"]

        def pt(p):
            return f"({p[0]:.0f}, {p[1]:.0f})" if p else "-"

        for m in self.measurements:
            lines.append(f"| {m['check']} | {md(m['input'])} | {pt(m['start'])} | "
                         f"{pt(m['end'])} | {m['dx']} | {m.get('peak_speed', '-')} |")
        if not self.measurements:
            lines.append("| (none) | - | - | - | - | - |")
        return lines


Run = Us083Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us083Run, argv)


def main(argv=None):
    return kit.main(Us083Run, argv)


if __name__ == "__main__":
    sys.exit(main())
