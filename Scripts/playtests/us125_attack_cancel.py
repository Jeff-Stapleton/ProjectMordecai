#!/usr/bin/env python3
"""US-125 MovementTestMap Tier 4: committed rejection, Recovery cancel, and dodge distance."""

import math
import os
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-125"
TITLE = "US-125 attack cancel and tier-scaled dodge"
RUN_SUFFIX = "us125-attack-cancel"
RT = "Gamepad_RightTriggerAxis"
LT = "Gamepad_LeftTriggerAxis"
TAG_ATTACK = "Mordecai.State.Attacking"
TAG_COMMITTED = "Mordecai.State.AttackCommitted"
TAG_COMBO = "Mordecai.State.ComboWindow"
TAG_DODGE = "Mordecai.State.Dodging"
TAG_BLOCK = "Mordecai.State.Blocking"
TAG_COOLDOWN = "Mordecai.State.DodgeCooldown"
TAP = 0.1

SCREENSHOTS = {
    "lane": "the player on MovementTestMap's unobstructed L1 lane with the combat HUD",
    "dodge_cancel": "the player after a full-length dodge cancel out of light-attack Recovery",
    "block_cancel": "the player guarding after a fresh Block press canceled light-attack Recovery",
}

PLAN = (
    Step("setup_yellow", "walk to lane L1 and spend five real light attacks to reach about 60 stamina"),
    Step("recovery_dodge_cancel",
         "tap RT, wait for ComboWindow, tap LeftAlt -> Attacking/ComboWindow clear before Dodging; "
         "settled displacement is 400 +/-40 cm at Yellow",
         proves=("AC1", "AC3", "AC7")),
    Step("committed_dodge_rejected",
         "tap RT and immediately tap LeftAlt during AttackCommitted -> attack remains and no dodge",
         proves=("AC2",)),
    Step("held_block_needs_fresh_press",
         "hold LT during AttackCommitted and through Recovery -> Block never activates; release LT",
         proves=("AC2",)),
    Step("recovery_block_cancel",
         "on a new light attack wait for ComboWindow, freshly hold LT -> attack ends before Block; "
         "release LT",
         proves=("AC1", "AC7")),
)


def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None


def _tags(state):
    return [str(tag) for tag in ((_player(state) or {}).get("tags") or [])]


def _loc(state):
    value = (_player(state) or {}).get("location")
    return (float(value[0]), float(value[1])) if isinstance(value, list) and len(value) >= 2 else None


def _stamina(state):
    value = ((_player(state) or {}).get("attributes") or {}).get("stamina")
    return float(value) if isinstance(value, (int, float)) else None


class Us125AttackCancelRun(kit.PlaytestRun):
    STORY = STORY
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    FEATURES = ("melee_attack", "dodge", "block_parry", "stamina", "movement")
    MAP = kit.MOVEMENT_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    TAP_S = TAP
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.measurements = []

    def _state(self):
        return self.bridge.state(["player"])

    def _wait(self, predicate, timeout, description):
        try:
            return self.bridge.wait_for(predicate, timeout_s=timeout, sections=["player"], poll_s=0.02)
        except ab.BridgeError as exc:
            raise RuntimeError(f"{description} not seen within {timeout:g}s: {exc}") from exc

    def _wait_tag(self, tag, present=True, timeout=2.0):
        return self._wait(lambda state: (tag in _tags(state)) == present, timeout,
                          f"{tag} {'present' if present else 'absent'}")

    def _idle(self):
        self._wait(lambda state: TAG_ATTACK not in _tags(state)
                   and TAG_BLOCK not in _tags(state)
                   and TAG_DODGE not in _tags(state)
                   and TAG_COOLDOWN not in _tags(state), 4.0, "combat idle")

    def _tap_attack(self):
        self.bridge.axis({RT: 1.0}, hold_s=TAP)

    def steps(self):
        setup, _ = self.attempt("setup_yellow", self.step_setup)
        self.shot("lane", needs=setup, needs_why="not taken: setup failed")
        dodge, _ = self.attempt("recovery_dodge_cancel", self.step_dodge_cancel,
                                needs=setup, needs_why="not run: Yellow setup failed")
        self.shot("dodge_cancel", needs=dodge, needs_why="not taken: dodge cancel failed")
        self.attempt("committed_dodge_rejected", self.step_committed_dodge,
                     needs=dodge, needs_why="not run: dodge cancel failed")
        held, _ = self.attempt("held_block_needs_fresh_press", self.step_held_block,
                               needs=dodge, needs_why="not run: dodge cancel failed")
        block, _ = self.attempt("recovery_block_cancel", self.step_block_cancel,
                                needs=held, needs_why="not run: held-block rule failed")
        self.shot("block_cancel", needs=block, needs_why="not taken: block cancel failed")

    def step_setup(self):
        try:
            result = self.bridge.walk_to(-300.0, -800.0, tol=60.0)
            for index in range(5):
                self._idle()
                self._tap_attack()
                self._wait_tag(TAG_ATTACK)
                self._wait_tag(TAG_ATTACK, False, 3.0)
            state = self._state()
            stamina = _stamina(state)
            ok = stamina is not None and 45.0 <= stamina <= 65.0
            detail = f"reached {result.get('final')}; stamina after five real attacks {stamina}"
            return ("PASS" if ok else "FAIL"), detail, {"stamina": stamina}
        except (ab.BridgeError, RuntimeError) as exc:
            return "FAIL", str(exc), None

    def step_dodge_cancel(self):
        try:
            self._idle()
            self._tap_attack()
            self._wait_tag(TAG_COMBO)
            before = self._state()
            p0 = _loc(before)
            stamina = _stamina(before)
            response = self.bridge.tap("LeftAlt", hold_s=TAP)
            active = self._state()
            tags_after_input = _tags(active)
            activated = TAG_DODGE in tags_after_input or TAG_COOLDOWN in tags_after_input
            overlap = TAG_ATTACK in tags_after_input or TAG_COMBO in tags_after_input
            self._wait_tag(TAG_DODGE, False, 2.0)
            self._wait(lambda state: float((_player(state) or {}).get("speed") or 999) < 1.0,
                       2.0, "dodge settle")
            p1 = _loc(self._state())
            distance = math.dist(p0, p1) if p0 and p1 else None
            self.measurements.append({"check": "recovery_dodge_cancel", "start": p0, "end": p1,
                                      "distance_cm": distance, "stamina_at_dodge": stamina})
            problems = []
            if not activated:
                problems.append(f"Dodge/DodgeCooldown not observed after input: {tags_after_input}")
            if overlap:
                problems.append(f"attack remained after Dodge input: {tags_after_input}")
            if distance is None or not 360.0 <= distance <= 440.0:
                problems.append(f"settled distance {distance}, expected 360..440 cm")
            detail = (f"Recovery at stamina {stamina}; Dodge input held "
                      f"{response.get('held_s')}s; post-input tags {tags_after_input}; "
                      f"settled {distance} cm from {p0} to {p1}")
            return ("FAIL", detail + " -- " + "; ".join(problems), None) if problems \
                else ("PASS", detail, {"distance_cm": distance})
        except (ab.BridgeError, RuntimeError) as exc:
            return "FAIL", str(exc), None

    def step_committed_dodge(self):
        try:
            self._idle()
            self._tap_attack()
            self._wait_tag(TAG_COMMITTED)
            self.bridge.tap("LeftAlt", hold_s=TAP)
            time.sleep(0.08)
            tags = _tags(self._state())
            ok = TAG_ATTACK in tags and TAG_DODGE not in tags
            self._wait_tag(TAG_ATTACK, False, 3.0)
            return ("PASS" if ok else "FAIL"), f"tags after committed dodge press: {tags}", None
        except (ab.BridgeError, RuntimeError) as exc:
            return "FAIL", str(exc), None

    def step_held_block(self):
        try:
            self._idle()
            self._tap_attack()
            self._wait_tag(TAG_COMMITTED)
            self.bridge.axis({LT: 1.0})
            seen_block = False
            deadline = time.monotonic() + 0.8
            last = []
            while time.monotonic() < deadline:
                last = _tags(self._state())
                seen_block |= TAG_BLOCK in last
                if TAG_ATTACK not in last:
                    break
                time.sleep(0.02)
            self.bridge.axis({LT: 0.0})
            self._wait_tag(TAG_ATTACK, False, 3.0)
            return ("FAIL" if seen_block else "PASS"), (
                f"held LT from committed phase through Recovery; Blocking seen={seen_block}; "
                f"last tags {last}"), None
        except (ab.BridgeError, RuntimeError) as exc:
            try:
                self.bridge.axis({LT: 0.0})
            except Exception:
                pass
            return "FAIL", str(exc), None

    def step_block_cancel(self):
        try:
            self._idle()
            self._tap_attack()
            self._wait_tag(TAG_COMBO)
            self.bridge.axis({LT: 1.0})
            state = self._wait_tag(TAG_BLOCK)
            tags = _tags(state)
            overlap = TAG_ATTACK in tags or TAG_COMBO in tags
            self.bridge.axis({LT: 0.0})
            self._wait_tag(TAG_BLOCK, False)
            detail = f"fresh LT in Recovery produced tags {tags}"
            return ("FAIL", detail + " -- attack/block overlap", None) if overlap \
                else ("PASS", detail, None)
        except (ab.BridgeError, RuntimeError) as exc:
            try:
                self.bridge.axis({LT: 0.0})
            except Exception:
                pass
            return "FAIL", str(exc), None

    def report_extra(self):
        return {"movement": self.measurements}


Run = Us125AttackCancelRun


def main(argv=None):
    return kit.main(Us125AttackCancelRun, argv)


if __name__ == "__main__":
    sys.exit(main())
