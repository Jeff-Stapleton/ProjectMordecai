#!/usr/bin/env python3
"""US-125 CombatTestMap Tier 4: perfect-dodge reward and weapon-swap combo safety."""

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
TITLE = "US-125 perfect dodge and combo weapon-swap safety"
RUN_SUFFIX = "us125-perfect-dodge-combo"
ATTACKER = "TestAttacker"
FIGHT_SPOT = (2000.0, 0.0)
SWAP_SPOT = (1200.0, -700.0)
RADIUS = 3000
TAG_ATTACK = "Mordecai.State.Attacking"
TAG_COMBO = "Mordecai.State.ComboWindow"
TAG_DODGE = "Mordecai.State.Dodging"
TAG_PERFECT = "Mordecai.State.PerfectDodge"
RT = "Gamepad_RightTriggerAxis"
TAP = 0.1

SCREENSHOTS = {
    "perfect_dodge": "the player beside TestAttacker immediately after a real perfect evasion; "
                     "health is unchanged and the Perfect Dodge callout may be visible",
    "weapon_swap": "the combat HUD showing the shorter replacement weapon after a mid-combo swap",
    "fresh_attack": "the replacement weapon attacking normally after the swapped chain ended",
}

PLAN = (
    Step("setup", "walk within 100 cm of CombatTestMap's fixed-cadence TestAttacker"),
    Step("perfect_dodge",
         "sync to the TestAttacker attack edge and tap LeftAlt about 0.40 s later -> hit evaded "
         "inside the 150 ms perfect window, health unchanged, net dodge stamina cost 5, and "
         "PerfectDodge success tag observed",
         proves=("AC4", "AC7")),
    Step("equip_dagger",
         "press Tab with real input until the weapon state reports Dagger"),
    Step("swap_during_combo",
         "tap RT, queue the next light step during ComboWindow, then press Tab -> weapon changes, "
         "queued continuation is cleared, and Attacking cannot stick",
         proves=("AC5", "AC7")),
    Step("fresh_new_weapon_attack",
         "tap RT after the swapped chain ended -> the new weapon starts and finishes a fresh attack",
         proves=("AC5",)),
)


def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None


def _tags(state):
    return [str(tag) for tag in ((_player(state) or {}).get("tags") or [])]


def _attrs(state):
    value = (_player(state) or {}).get("attributes")
    return value if isinstance(value, dict) else {}


def _weapon(state):
    value = (_player(state) or {}).get("weapon")
    return value if isinstance(value, dict) else {}


def _actors(state):
    value = (state or {}).get("actors")
    return value if isinstance(value, list) else []


def _attacker(state):
    for actor in _actors(state):
        if isinstance(actor, dict) and ATTACKER.lower() in str(
                actor.get("label") or actor.get("name") or actor.get("class") or "").lower():
            return actor
    return None


def _combat_values(state):
    return {
        str(actor.get("name") or actor.get("label")): (
            actor.get("health"), actor.get("posture"))
        for actor in _actors(state) if isinstance(actor, dict)
    }


def _texts(state):
    return [str(text) for widget in ((state or {}).get("widgets") or [])
            if isinstance(widget, dict) and widget.get("visible", True) is not False
            for text in (widget.get("texts") or [])]


def perfect_result_problems(health0, health1, stamina0, stamina1,
                            seen_dodge, seen_perfect, reward_count=1):
    problems = []
    if not seen_dodge:
        problems.append("Dodging was never observed")
    if not seen_perfect:
        problems.append("successful PerfectDodge feedback was never observed")
    if abs(health1 - health0) > 0.5:
        problems.append(f"health changed {health0}->{health1}")
    if not 4.0 <= stamina0 - stamina1 <= 6.0:
        problems.append(f"stamina delta {stamina0 - stamina1:g}, expected net cost 5")
    if reward_count != 1:
        problems.append(f"perfect-dodge refund count {reward_count}, expected exactly 1")
    return problems


class Us125PerfectDodgeComboRun(kit.PlaytestRun):
    STORY = STORY
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    FEATURES = ("dodge", "stamina", "melee_attack", "weapons", "weapon_hud")
    MAP = kit.COMBAT_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    TAP_S = TAP
    SCRIPT = os.path.abspath(__file__)

    def _read(self):
        return self.bridge.state(["player", "actors"], radius=RADIUS)

    def _wait(self, predicate, timeout, description, sections=("player",)):
        try:
            return self.bridge.wait_for(predicate, timeout_s=timeout, sections=list(sections),
                                        radius=RADIUS, poll_s=0.01)
        except ab.BridgeError as exc:
            raise RuntimeError(f"{description} not seen within {timeout:g}s: {exc}") from exc

    def _wait_tag(self, tag, present=True, timeout=3.0):
        return self._wait(lambda state: (tag in _tags(state)) == present, timeout,
                          f"{tag} {'present' if present else 'absent'}")

    def _idle(self):
        self._wait(lambda state: TAG_ATTACK not in _tags(state)
                   and TAG_DODGE not in _tags(state), 4.0, "combat idle")

    def steps(self):
        setup, _ = self.attempt("setup", self.step_setup)
        perfect, _ = self.attempt("perfect_dodge", self.step_perfect,
                                  needs=setup, needs_why="not run: setup failed")
        self.shot("perfect_dodge", needs=perfect, needs_why="not taken: perfect dodge failed")
        dagger, _ = self.attempt("equip_dagger", self.step_equip_dagger,
                                 needs=perfect, needs_why="not run: perfect dodge failed")
        swapped, _ = self.attempt("swap_during_combo", self.step_swap,
                                  needs=dagger, needs_why="not run: Dagger was not equipped")
        self.shot("weapon_swap", needs=swapped, needs_why="not taken: swap failed")
        fresh, _ = self.attempt("fresh_new_weapon_attack", self.step_fresh,
                                needs=swapped, needs_why="not run: swapped attack stuck")
        self.shot("fresh_attack", needs=fresh, needs_why="not taken: fresh attack failed")

    def step_setup(self):
        try:
            result = self.bridge.walk_to(*FIGHT_SPOT, tol=25.0)
            state = self._read()
            actor = _attacker(state)
            ok = actor is not None and actor.get("is_dead") is False
            return ("PASS" if ok else "FAIL"), (
                f"reached {result.get('final')}; attacker "
                f"{(actor or {}).get('label') or (actor or {}).get('name')} "
                f"ai={(actor or {}).get('ai_state')}"), None
        except ab.BridgeError as exc:
            return "FAIL", str(exc), None

    def step_perfect(self):
        try:
            # Bracket a fresh attack. The TestAttacker hit lands about 0.52 s after Attack starts;
            # a 0.40 s press puts the hit about 120 ms into the 150 ms perfect window.
            self._wait(lambda state: (_attacker(state) or {}).get("ai_state") != "Attack",
                       4.0, "attacker recovery", ("player", "actors"))
            before = self._read()
            health0 = float(_attrs(before).get("health"))
            stamina0 = float(_attrs(before).get("stamina"))
            self._wait(lambda state: (_attacker(state) or {}).get("ai_state") == "Attack",
                       6.0, "attacker attack edge", ("player", "actors"))
            time.sleep(0.40)
            self.bridge.press("W")
            try:
                self.bridge.tap("LeftAlt", hold_s=TAP)
            finally:
                self.bridge.release("W")

            seen_perfect = False
            seen_dodge = False
            deadline = time.monotonic() + 0.45
            latest = self._read()
            while time.monotonic() < deadline:
                latest = self._read()
                seen_dodge |= TAG_DODGE in _tags(latest)
                seen_perfect |= TAG_PERFECT in _tags(latest)
                time.sleep(0.01)
            health1 = float(_attrs(latest).get("health"))
            stamina1 = float(_attrs(latest).get("stamina"))
            # A 15 cost and one 10 refund is net -5. Small regeneration can begin only later.
            problems = perfect_result_problems(
                health0, health1, stamina0, stamina1, seen_dodge, seen_perfect)
            detail = (f"health {health0}->{health1}; stamina {stamina0}->{stamina1}; "
                      f"Dodging seen={seen_dodge}; PerfectDodge success seen={seen_perfect}")
            return ("FAIL", detail + " -- " + "; ".join(problems), None) if problems \
                else ("PASS", detail, None)
        except (ab.BridgeError, RuntimeError, TypeError, ValueError) as exc:
            return "FAIL", str(exc), None

    def step_equip_dagger(self):
        try:
            for presses in range(8):
                state = self.bridge.state(["player"])
                weapon = _weapon(state)
                if "dagger" in (str(weapon.get("id")) + str(weapon.get("display_name"))).lower():
                    return "PASS", f"Dagger equipped after {presses} Tab press(es): {weapon}", None
                self.bridge.tap("Tab", hold_s=TAP)
                time.sleep(0.2)
            return "FAIL", f"Dagger not found after cycling: {_weapon(self.bridge.state(['player']))}", None
        except ab.BridgeError as exc:
            return "FAIL", str(exc), None

    def step_swap(self):
        try:
            self._idle()
            moved = self.bridge.walk_to(*SWAP_SPOT, tol=40.0)
            before = self._read()
            combat_before = _combat_values(before)
            old_weapon = _weapon(self.bridge.state(["player"]))
            self.bridge.axis({RT: 1.0}, hold_s=TAP)
            self._wait_tag(TAG_COMBO)
            self.bridge.axis({RT: 1.0}, hold_s=TAP)  # queue continuation
            self.bridge.tap("Tab", hold_s=TAP)
            changed = self._wait(lambda state: _weapon(state).get("index") != old_weapon.get("index"),
                                 2.0, "weapon change")
            new_weapon = _weapon(changed)
            started = time.monotonic()
            final = self._wait_tag(TAG_ATTACK, False, 2.0)
            clear_s = time.monotonic() - started
            tags = _tags(final)
            combat_after = _combat_values(self._read())
            problems = []
            if TAG_ATTACK in tags or TAG_COMBO in tags:
                problems.append(f"attack state stuck: {tags}")
            if combat_after != combat_before:
                problems.append(
                    f"out-of-reach attack changed health/posture: "
                    f"{combat_before} -> {combat_after}")
            detail = (f"moved out of reach to {moved.get('final')}; weapon "
                      f"{old_weapon} -> {new_weapon}; attack cleared in {clear_s:.3f}s; "
                      f"health/posture {combat_before} -> {combat_after}; final tags {tags}")
            return ("FAIL", detail + " -- " + "; ".join(problems), None) if problems \
                else ("PASS", detail, None)
        except (ab.BridgeError, RuntimeError) as exc:
            return "FAIL", str(exc), None

    def step_fresh(self):
        try:
            self._idle()
            weapon = _weapon(self.bridge.state(["player"]))
            self.bridge.axis({RT: 1.0}, hold_s=TAP)
            self._wait_tag(TAG_ATTACK)
            step_one = self._wait(
                lambda state: "Hit 1" in _texts(state), 1.5,
                "new weapon combo step 1 HUD", ("player", "widgets"))
            self._wait_tag(TAG_ATTACK, False, 4.0)
            return "PASS", (
                f"fresh attack completed with {weapon}; visible combo text "
                f"{_texts(step_one)} confirms step 1"), None
        except (ab.BridgeError, RuntimeError) as exc:
            return "FAIL", str(exc), None


Run = Us125PerfectDodgeComboRun


def main(argv=None):
    return kit.main(Us125PerfectDodgeComboRun, argv)


if __name__ == "__main__":
    sys.exit(main())
