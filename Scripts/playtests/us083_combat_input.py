#!/usr/bin/env python3
"""US-083 Combat Input Playtest -- Tier 4: block, parry, attacks, dodge and jump on the GAMEPAD,
played with REAL input

US-083 binds the player's combat input (MordecaiIMC_Runtime).  The mouse and keyboard halves are
played by the owning stories' scripts (us001 sprint/jump, us054 LMB/LeftAlt, us078 LMB tap/hold).
Block and parry need a button held across state reads: the playtest client cannot hold a mouse
button that way (B2), but it can hold a gamepad trigger (`axis` without hold_s keeps the value
until the next `axis` for that key).  So this script plays block (LT held) and parry (RT pressed
while LT blocks) for real, plus the parry's input rules (a fresh RT press only: a partly pulled RT,
or a block raised over a held RT, must not parry), and the gamepad attack, dodge and jump keys.

Drives the PACKAGED game through the in-game agent bridge (Scripts/agent_bridge.py): every action
is an axis or key event injected into Slate (axis / key), every observation a state read or a
screenshot.  Nothing calls game functions; bridge.jsonl in the run dir is the audit trail.  The
run framework is Scripts/playtest_kit.py (Specs/story_playtests_v1.md section 2).

Steps (every step is a check; --plan prints them as the story's Steps table):
  1-4  LT held: Blocking.  RT pressed while blocking: a parry (Parrying / ParryWhiff / Riposte)
       that drops the guard and does not attack.  LT still held: the guard comes back once the
       parry and its whiff end, with no attack in between.  LT released: Blocking gone.
  5-6  LT held and RT pulled to 0.3 (below the 0.5 actuation): no parry, the guard stays.  RT held
       first, then LT: a block, no parry.
  7-10 RT tapped 0.1 s: a light attack (Attacking, no Mordecai.Attack.Heavy).  RT held 0.8 s: a
       heavy attack on the release (Mordecai.Attack.Heavy).  Gamepad B: a dodge.  Gamepad A: a jump.
The player stays at the spawn; nothing needs an enemy.  A failed check SKIPs the checks that
depend on it; independent checks still run.

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
TITLE = "US-083 gamepad combat input playtest (Tier 4, real input)"
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
}
KNOWN_ISSUES = (
    "Block and parry on the mouse (RMB hold, RMB+LMB, AC-083.11) are not played: the playtest "
    "client holds a mouse button only inside one blocking click (B2). This script plays the "
    "same actions on the gamepad triggers.",
)

PLAN = (
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
    Step("jump_gamepad_a", f"wait_until player.movement_mode==\"Walking\", tap {JUMP_KEY} (A) -> "
         f"wait_until player.movement_mode==\"Falling\" within {JUMP_TIMEOUT_S:g}s",
         proves=("AC-083.3",)),
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


class Us083Run(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("block_parry", "melee_attack", "dodge", "movement")
    TITLE = TITLE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    PENDING = PENDING
    TAP_S = TAP_S
    SCRIPT = os.path.abspath(__file__)

    def steps(self):
        self.step_block_and_parry()
        self.step_parry_needs_a_fresh_press()
        self.step_attacks_dodge_jump()

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

    def _wait_idle(self):
        """Wait until no attack, block or parry tag is left.  Returns None or why not."""
        got, seen = self._poll(IDLE_TIMEOUT_S, _idle)
        if got is None:
            return (f"the player is not idle {IDLE_TIMEOUT_S:g}s after the previous step (tags "
                    f"{seen[-1]}), so this step cannot be told apart from it")
        return None

    # --- 1-4: block and parry ------------------------------------------------------------
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

    # --- 5-6: only a fresh RT press parries ---------------------------------------------------
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

    # --- 7-10: gamepad attacks, dodge, jump ---------------------------------------------------
    def _rt_attack(self, check, hold_s, timeout_s, heavy):
        def started(tags):
            return TAG_HEAVY in tags if heavy else (TAG_ATTACKING in tags and TAG_HEAVY not in tags)

        want = f"'{TAG_HEAVY}'" if heavy else f"'{TAG_ATTACKING}' without '{TAG_HEAVY}'"

        def fn():
            not_idle = self._wait_idle()
            if not_idle:
                return "FAIL", not_idle, None
            self.bridge.axis({RT: 1.0}, hold_s=hold_s)
            t0 = time.monotonic()
            got, seen = self._poll(timeout_s, started)
            if got is None:
                return ("FAIL", f"idle; {RT}=1 held {hold_s:g}s: no {want} in player.tags within "
                        f"{timeout_s:g}s (tags {seen[-1]})", None)
            return ("PASS", f"idle; {RT}=1 held {hold_s:g}s: {want} after "
                    f"{time.monotonic() - t0:.2f}s (tags {got})", None)

        return self.attempt(check, fn)

    def step_attacks_dodge_jump(self):
        self._rt_attack("light_attack_rt_tap", RT_TAP_S, LIGHT_TIMEOUT_S, heavy=False)
        self._rt_attack("heavy_attack_rt_hold", RT_HEAVY_S, HEAVY_TIMEOUT_S, heavy=True)

        def dodge():
            not_idle = self._wait_idle()
            if not_idle:
                return "FAIL", not_idle, None
            self.bridge.tap(DODGE_KEY, hold_s=self.TAP_S)
            got, seen = self._poll(DODGE_TIMEOUT_S, _dodging)
            if got is None:
                return ("FAIL", f"tapped {DODGE_KEY}: no Dodging tag within {DODGE_TIMEOUT_S:g}s "
                        f"(tags {seen[-1]})", None)
            return "PASS", f"tapped {DODGE_KEY}: tags {got}", None

        self.attempt("dodge_gamepad_b", dodge)

        def jump():
            # The dodge launches the player (Falling for a few frames): only judge the jump from
            # the ground
            try:
                self.bridge.wait_until('player.movement_mode=="Walking"', timeout_s=IDLE_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                return ("FAIL", f"the player is not Walking {IDLE_TIMEOUT_S:g}s after the dodge "
                        f"(values {(e.response or {}).get('values')}), so a jump cannot be told "
                        "apart", None)
            self.bridge.tap(JUMP_KEY, hold_s=self.TAP_S)
            try:
                r = self.bridge.wait_until('player.movement_mode=="Falling"',
                                           timeout_s=JUMP_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                return ("FAIL", f"Walking; tapped {JUMP_KEY}: player.movement_mode stayed "
                        f"{(e.response or {}).get('values')} for {JUMP_TIMEOUT_S:g}s", None)
            return ("PASS", f"Walking; tapped {JUMP_KEY}: Falling after {r.get('elapsed_s')}s",
                    None)

        self.attempt("jump_gamepad_a", jump)


Run = Us083Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def parse_args(argv=None):
    return kit.parse_args(Us083Run, argv)


def main(argv=None):
    return kit.main(Us083Run, argv)


if __name__ == "__main__":
    sys.exit(main())
