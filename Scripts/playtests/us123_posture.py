#!/usr/bin/env python3
"""US-123 enemy posture runtime — packaged CombatTestMap, real input only."""

import json
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

STORY = "US-123"
TITLE = "US-123 enemy posture break, critical window and knockdown"
ATTACKER = "TestAttacker"
FIGHT_SPOT = (2000.0, 0.0)
PIN_SPOT = (1850.0, 0.0)
SAFE_SPOT = (900.0, -600.0)
RADIUS = 3000
SPOT_TOL = 25.0
LT = "Gamepad_LeftTriggerAxis"
RT = "Gamepad_RightTriggerAxis"
SPRINT = "LeftShift"
TAG_PARRIED = "Mordecai.State.Parried"
TAG_RIPOSTE = "Mordecai.State.Riposte"
TAG_BROKEN = "Mordecai.State.PostureBroken"
TAG_DOWN = "Mordecai.State.KnockedDown"
TAG_YELLOW = "Mordecai.Stamina.Tier.Yellow"
TAG_GREEN = "Mordecai.Stamina.Tier.Green"
TAG_SPRINTING = "Mordecai.State.Sprinting"
TAG_BLOCKING = "Mordecai.State.Blocking"
TAG_ATTACKING = "Mordecai.State.Attacking"
TAG_COMBO = "Mordecai.State.ComboWindow"
# State polling can observe the Attack edge a few frames late. Start the
# 0.5-second parry window promptly enough to cover the fixed-cadence hit.
PARRY_DELAY_S = 0.1
STATE_POLL_S = 0.03
BASE_L1_YELLOW = 16.0
STAGGERED_L1_YELLOW = 24.0
STAGGERED_L2_YELLOW = 27.0
TOL = 0.6

SCREENSHOTS = {
    "staggered_critical": "the staggered TestAttacker beside the player with the critical 24 damage number",
    "knocked_down": "the TestAttacker in Staggered AI state with Mordecai.State.KnockedDown",
    "knocked_down_detail": "a second clean frame of the knocked-down TestAttacker during its two-second window",
}

PLAN = (
    Step("setup",
         "walk to the TestAttacker, pin the equipped Longsword and drain with real sprint input "
         "until the authoritative stamina tier is Yellow"),
    Step("unbroken_control",
         "tap RT once at Yellow stamina against the unbroken TestAttacker -> exactly 16 health taken"),
    Step("break_by_parry",
         "hold LT and press RT during each of two TestAttacker swings -> posture reaches 0; "
         "PostureBroken and Parried tags overlap and ai_state is Staggered",
         proves=("AC-123.10",)),
    Step("staggered_hit",
         "release LT and tap RT inside the break/riposte window -> 24 health (1.5 x the 16 "
         "control), Riposte consumed; capture the critical number",
         proves=("AC-123.3", "AC-123.7", "AC-123.10")),
    Step("recovers",
         "wait past 1 second -> PostureBroken/Parried gone, posture=max and the attacker swings again",
         proves=("AC-123.1", "AC-123.6", "AC-123.10")),
    Step("knockdown",
         "move out, refill, return, pin Longsword+Yellow, parry twice, assert the resulting "
         "authoritative tier remains Yellow, then RT L1->L2 inside the break: 24+27=51 "
         "actual health reaches the 50 threshold and KnockedDown replaces PostureBroken "
         "for about 2 seconds; capture it",
         proves=("AC-123.4", "AC-123.11")),
    Step("image_verification",
         "Pillow verifies all three evidence/marketing frames are 1280x720 and non-blank; "
         "write image-verification.json",
         kind="infra"),
)


def _num(value):
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def _tags(entry):
    return [str(tag) for tag in ((entry or {}).get("tags") or [])]


def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None


def _enemies(state):
    value = (state or {}).get("actors")
    return [actor for actor in value if isinstance(actor, dict) and actor.get("kind") == "enemy"] \
        if isinstance(value, list) else []


def find_attacker(state):
    for actor in _enemies(state):
        if ATTACKER.lower() in str(actor.get("label") or actor.get("name") or "").lower():
            return actor
    for actor in _enemies(state):
        if ATTACKER.lower() in str(actor.get("class") or "").lower():
            return actor
    return None


def _health(actor):
    health = (actor or {}).get("health")
    return _num(health.get("current")) if isinstance(health, dict) else None


def _posture(actor):
    return _num((actor or {}).get("posture"))


def _max_posture(actor):
    return _num((actor or {}).get("max_posture"))


def _stamina(player):
    attrs = (player or {}).get("attributes")
    return _num(attrs.get("stamina")) if isinstance(attrs, dict) else None


def _image_stats(path):
    from PIL import Image, ImageStat

    with Image.open(path) as source:
        image = source.convert("RGB")
        stddev = ImageStat.Stat(image).stddev
        width, height = image.size
    return {
        "path": os.path.abspath(path),
        "width": width,
        "height": height,
        "rgb_stddev": [round(value, 3) for value in stddev],
        "non_blank": max(stddev) >= 3.0,
        "passed": (width, height) == (1280, 720) and max(stddev) >= 3.0,
    }


class Us123Run(kit.PlaytestRun):
    STORY = STORY
    TITLE = TITLE
    FEATURES = ("block_parry", "stamina", "enemies", "movement")
    MAP = kit.COMBAT_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.attacker_name = None
        self.control_damage = None
        self.stagger_l1 = None
        self.knockdown_route = []
        self.image_paths = []

    def steps(self):
        setup, _ = self.attempt("setup", self._guarded(self._setup))
        control, _ = self.attempt(
            "unbroken_control", self._guarded(self._unbroken_control), needs=setup,
            needs_why="not run: Longsword and Yellow stamina were not pinned")
        broken, _ = self.attempt(
            "break_by_parry", self._guarded(self._break_by_parry), needs=control,
            needs_why="not run: the unbroken control hit did not pass")
        staggered, _ = self.attempt(
            "staggered_hit", self._guarded(self._staggered_hit), needs=broken,
            needs_why="not run: two real parries did not break posture")
        if staggered:
            self.shot("staggered_critical")
            self.image_paths.append(self.screens[-1]["path"])
        recovered, _ = self.attempt(
            "recovers", self._guarded(self._recovers), needs=staggered,
            needs_why="not run: the staggered critical hit did not pass")
        down, _ = self.attempt(
            "knockdown", self._guarded(self._knockdown), needs=recovered,
            needs_why="not run: the first break did not recover")
        if down:
            self.shot("knocked_down")
            self.image_paths.append(self.screens[-1]["path"])
            self.shot("knocked_down_detail")
            self.image_paths.append(self.screens[-1]["path"])
        self.attempt(
            "image_verification", self._verify_images, kind="infra",
            needs=len(self.image_paths) == 3,
            needs_why="not run: all three posture evidence screenshots were not captured")

    @staticmethod
    def _guarded(fn):
        def run():
            try:
                return fn()
            except RuntimeError as error:
                return "FAIL", str(error), None
            except ab.BridgeError as error:
                if error.kind != "gameplay":
                    raise
                return "FAIL", str(error), error.response
        return run

    def _read(self):
        return self.bridge.state(["player", "actors"], radius=RADIUS)

    def _wait_state(self, predicate, timeout_s, description):
        deadline = time.monotonic() + timeout_s
        last = None
        while time.monotonic() < deadline:
            last = self._read()
            if predicate(last):
                return last
            self.bridge.wait(STATE_POLL_S)
        raise RuntimeError(f"{description} not seen in {timeout_s:g}s; last={kit.dumps(last, 1200)}")

    def _wait_attacker_ai(self, attacking, timeout_s, description):
        name = kit.lit(self.attacker_name)
        expected = "Attack"
        parts = []
        for index in range(12):
            actor = f"actors[{index}]"
            op = "==" if attacking else "!="
            parts.append(
                f"({actor}.name=={name} && {actor}.ai_state{op}\"{expected}\" "
                f"&& {actor}.ai_state!=null)")
        try:
            return self.bridge.wait_until(
                " || ".join(parts), timeout_s=timeout_s, poll_s=0.0)
        except ab.BridgeError as error:
            if error.code != "timeout":
                raise
            raise RuntimeError(f"{description} not seen in {timeout_s:g}s") from error

    def _attacker(self, state=None):
        actor = find_attacker(state or self._read())
        if actor is None:
            raise RuntimeError(f"{ATTACKER} is absent from actors[]")
        return actor

    def _aim(self):
        kit.aim_world_positive_x(self.bridge)

    def _walk_fight(self):
        result = self.bridge.walk_to(*FIGHT_SPOT, tol=SPOT_TOL)
        self._aim()
        state = self._read()
        actor = self._attacker(state)
        self.attacker_name = actor.get("name")
        return result, state

    def _pin_yellow(self, target=75.0, center=FIGHT_SPOT):
        # Sprint short closed-loop W/S legs. Keeping this one-dimensional avoids
        # camera-relative lateral correction near the attacker while every
        # mutation still comes from real key input.
        deadline = time.monotonic() + 20.0
        pinned = None
        at_fight_spot = False
        try:
            while time.monotonic() < deadline:
                leg_x = center[0] if at_fight_spot else center[0] - 50.0
                state = self._read()
                player = _player(state)
                location = (player or {}).get("location") or []
                if len(location) < 2:
                    raise RuntimeError(f"player location unavailable: {player}")
                if abs(float(location[0]) - leg_x) > SPOT_TOL:
                    if TAG_SPRINTING not in _tags(player):
                        self.bridge.tap(SPRINT, hold_s=0.1)
                    key = "W" if float(location[0]) < leg_x else "S"
                    hold = min(0.08, max(0.03, abs(float(location[0]) - leg_x) / 600.0))
                    self.bridge.tap(key, hold_s=hold)
                    continue
                at_fight_spot = not at_fight_spot
                stamina = _stamina(player)
                tags = _tags(player)
                if stamina is not None and stamina <= target + 0.5 and TAG_YELLOW in tags:
                    pinned = (stamina, tags)
                    break
                if TAG_SPRINTING not in tags:
                    self.bridge.tap(SPRINT, hold_s=0.1)
        finally:
            if TAG_SPRINTING in _tags(_player(self._read())):
                self.bridge.tap(SPRINT, hold_s=0.01)
        if pinned is None:
            raise RuntimeError(f"real sprint did not reach Yellow stamina near {target:g}")
        self._wait_state(
            lambda value: TAG_SPRINTING not in _tags(_player(value)),
            1.0, "sprint toggle off before fight-spot correction")
        state = self._read()
        for _ in range(8):
            location = (_player(state) or {}).get("location") or []
            if len(location) >= 2 and abs(float(location[0]) - center[0]) <= SPOT_TOL:
                break
            key = "W" if not location or float(location[0]) < center[0] else "S"
            self.bridge.tap(key, hold_s=0.05)
            state = self._read()
        return _stamina(_player(state)), _tags(_player(state))

    def _weapon_and_tier(self):
        state = self._read()
        player = _player(state)
        weapon = (player or {}).get("weapon")
        name = str((weapon or {}).get("display_name") or (weapon or {}).get("id") or "")
        stamina = _stamina(player)
        tags = _tags(player)
        problems = []
        if name.lower() != "longsword":
            problems.append(f"weapon is {name!r}, expected Longsword")
        if stamina is None or not 25.0 < stamina <= 75.0:
            problems.append(f"authoritative stamina {stamina} is outside Yellow: {tags}")
        if problems:
            raise RuntimeError("; ".join(problems))
        return stamina, name, tags

    def _pin_yellow_at_fight(self, target=75.0):
        last = None
        for _ in range(3):
            last = self._pin_yellow(target)
            # Opposing drain pulses keep the player at the fight spot. Any
            # extra wait or aim pulse here would let stamina regenerate across
            # the 75-point Green/Yellow boundary before attack activation.
            state = self._read()
            player = _player(state)
            if TAG_YELLOW in _tags(player):
                stamina, weapon, _ = self._weapon_and_tier()
                return stamina, weapon, last[1]
        raise RuntimeError(
            f"could not keep Yellow stamina while correcting to the fight spot; last {last}")

    def _wait_attack_edge(self):
        self._wait_attacker_ai(False, 4.0, "attacker outside Attack")
        return self._wait_attacker_ai(True, 6.0, "attacker Attack edge")

    def _parry_once(self):
        self._wait_attacker_ai(False, 4.0, "attacker outside Attack before raising guard")
        before = self._attacker()
        posture_before = _posture(before)
        self.bridge.axis({LT: 1.0})
        try:
            self._wait_state(
                lambda state: TAG_BLOCKING in _tags(_player(state)),
                1.0, "blocking guard before parry")
            self._wait_attacker_ai(True, 6.0, "attacker Attack edge")
            self.bridge.wait(PARRY_DELAY_S)
            self.bridge.axis({RT: 1.0}, hold_s=0.1)
            state = self._wait_state(
                lambda value: (
                    TAG_PARRIED in _tags(find_attacker(value))
                    and (_posture(find_attacker(value)) or 0.0) < (posture_before or 0.0)
                ) or TAG_BROKEN in _tags(find_attacker(value)),
                1.0, "parried posture change")
            return state
        finally:
            self.bridge.axis({LT: 0.0})

    def _wait_parried_expiry(self):
        return self._wait_state(
            lambda state: TAG_PARRIED not in _tags(find_attacker(state)),
            2.0, "Parried expiry")

    def _tap_light_and_damage(self, combo=False):
        before = self._attacker()
        health_before = _health(before)
        self.bridge.axis({RT: 1.0}, hold_s=0.1)
        state = self._wait_state(
            lambda value: (_health(find_attacker(value)) or health_before) < health_before - 0.1,
            1.2, "light attack health change")
        taken = health_before - _health(find_attacker(state))
        if combo:
            return taken, state
        self._wait_state(
            lambda value: TAG_ATTACKING not in _tags(_player(value)),
            1.5, "light attack completion")
        return taken, state

    def _setup(self):
        result, state = self._walk_fight()
        actor = self._attacker(state)
        if _posture(actor) is None or _max_posture(actor) is None:
            return "FAIL", "bridge actor posture/max_posture are unavailable", None
        # Stay just below Green so two immediate parries plus L1 keep L2 Yellow.
        pinned_stamina, weapon, tags = self._pin_yellow_at_fight()
        return "PASS", (
            f"fight spot {result.get('final')}; bridge posture {_posture(actor)}/"
            f"{_max_posture(actor)}; pinned {weapon} and "
            f"'{TAG_YELLOW}' at stamina {pinned_stamina}"), {
                "weapon": weapon, "stamina": pinned_stamina, "tags": tags}

    def _unbroken_control(self):
        taken, _ = self._tap_light_and_damage()
        self.control_damage = taken
        ok = abs(taken - BASE_L1_YELLOW) <= TOL
        return ("PASS" if ok else "FAIL"), (
            f"Longsword L1 at Yellow took {taken:g} health from an unbroken target "
            f"(expected {BASE_L1_YELLOW:g} +/- {TOL:g})"), {"damage": taken}

    def _break_by_parry(self):
        first = self._parry_once()
        first_actor = self._attacker(first)
        first_posture = _posture(first_actor)
        # Raise guard while Parried is still active so the first post-stagger
        # swing cannot race ahead and give posture time to regenerate.
        second = self._parry_once()
        actor = self._attacker(second)
        tags = _tags(actor)
        posture = _posture(actor)
        problems = []
        if first_posture is None or first_posture <= 0:
            problems.append(f"first parry posture was {first_posture}, expected above zero")
        if posture is None or posture > 0.1:
            problems.append(f"posture is {posture}, expected 0")
        for tag in (TAG_BROKEN, TAG_PARRIED):
            if tag not in tags:
                problems.append(f"missing '{tag}' in {tags}")
        if actor.get("ai_state") != "Staggered":
            problems.append(f"ai_state is {actor.get('ai_state')}, expected Staggered")
        detail = (
            f"two real LT+RT parries: posture {first_posture} -> {posture}; "
            f"ai_state {actor.get('ai_state')}; tags {tags}")
        return ("FAIL" if problems else "PASS"), (
            detail + (f" -- {'; '.join(problems)}" if problems else "")), {
                "first_posture": first_posture, "posture": posture, "tags": tags}

    def _staggered_hit(self):
        state0 = self._read()
        player_tags = _tags(_player(state0))
        if TAG_RIPOSTE not in player_tags:
            return "FAIL", f"'{TAG_RIPOSTE}' absent before punish: {player_tags}", None
        taken, state = self._tap_light_and_damage(combo=True)
        self.stagger_l1 = taken
        player_tags = _tags(_player(state))
        actor = self._attacker(state)
        ratio = taken / self.control_damage if self.control_damage else 0.0
        problems = []
        if abs(taken - STAGGERED_L1_YELLOW) > TOL:
            problems.append(f"damage {taken:g}, expected {STAGGERED_L1_YELLOW:g}")
        if abs(ratio - 1.5) > 0.05:
            problems.append(f"ratio {ratio:.3f}, expected 1.5")
        if TAG_RIPOSTE in player_tags:
            problems.append("Riposte was not consumed")
        if TAG_BROKEN not in _tags(actor):
            problems.append("PostureBroken expired before the hit was observed")
        return ("FAIL" if problems else "PASS"), (
            f"unbroken {self.control_damage:g} -> stagger/riposte {taken:g} ({ratio:.2f}x); "
            f"Riposte consumed; enemy tags {_tags(actor)}"
            + (f" -- {'; '.join(problems)}" if problems else "")), {
                "control": self.control_damage, "damage": taken, "ratio": ratio}

    def _recovers(self):
        state = self._wait_state(
            lambda value: (
                TAG_BROKEN not in _tags(find_attacker(value))
                and TAG_PARRIED not in _tags(find_attacker(value))
                and _posture(find_attacker(value)) is not None
                and abs(_posture(find_attacker(value)) - _max_posture(find_attacker(value))) <= 0.1
            ), 2.5, "posture-break recovery")
        actor = self._attacker(state)
        swung = self._wait_state(
            lambda value: (find_attacker(value) or {}).get("ai_state") == "Attack",
            6.0, "post-recovery swing")
        actor2 = self._attacker(swung)
        return "PASS", (
            f"tags expired; posture {_posture(actor)}/{_max_posture(actor)}; "
            f"attacker resumed ai_state {actor2.get('ai_state')}"), None

    def _prepare_second_route(self):
        self.bridge.walk_to(*SAFE_SPOT, tol=40.0)
        self._wait_state(
            lambda state: (
                _stamina(_player(state)) is not None
                and _stamina(_player(state)) >= 95.0
                and TAG_GREEN in _tags(_player(state))
            ), 10.0, "full stamina away from attacker")
        self.bridge.walk_to(*PIN_SPOT, tol=SPOT_TOL)
        self._aim()
        self._pin_yellow(center=PIN_SPOT)
        self._walk_fight()
        stamina, weapon, _ = self._weapon_and_tier()
        return stamina, weapon

    def _knockdown(self):
        pinned_stamina, weapon = self._prepare_second_route()
        first = self._parry_once()
        # As above, arm the second parry before the first stagger expires.
        second = self._parry_once()
        if TAG_BROKEN not in _tags(self._attacker(second)):
            return "FAIL", "second route did not enter PostureBroken", None
        stamina, pinned_weapon, stamina_tags = self._weapon_and_tier()
        if pinned_weapon != weapon:
            return "FAIL", f"weapon changed from {weapon} to {pinned_weapon}", None

        l1, _ = self._tap_light_and_damage(combo=True)
        self._wait_state(
            lambda state: TAG_COMBO in _tags(_player(state)),
            0.9, "L1 combo window")
        l2, state = self._tap_light_and_damage(combo=True)
        self.knockdown_route = [l1, l2]
        actor = self._attacker(state)
        tags = _tags(actor)
        problems = []
        if abs(l1 - STAGGERED_L1_YELLOW) > TOL:
            problems.append(f"L1 {l1:g}, expected {STAGGERED_L1_YELLOW:g}")
        if abs(l2 - STAGGERED_L2_YELLOW) > TOL:
            problems.append(f"L2 {l2:g}, expected {STAGGERED_L2_YELLOW:g}")
        if TAG_DOWN not in tags:
            problems.append(f"missing '{TAG_DOWN}' in {tags}")
        if TAG_BROKEN in tags:
            problems.append("PostureBroken was not replaced")
        if actor.get("ai_state") != "Staggered":
            problems.append(f"ai_state {actor.get('ai_state')}, expected Staggered")
        detail = (
            f"pinned {weapon} at Yellow stamina {pinned_stamina:g}, post-parry Yellow stamina "
            f"{stamina:g} (runtime tags {stamina_tags}); L1 {l1:g} + L2 {l2:g} = "
            f"{l1 + l2:g} actual health; tags {tags}; ai_state {actor.get('ai_state')}")
        return ("FAIL" if problems else "PASS"), (
            detail + (f" -- {'; '.join(problems)}" if problems else "")), {
                "weapon": weapon, "stamina": stamina, "damage": [l1, l2], "tags": tags}

    def _verify_images(self):
        results = [_image_stats(path) for path in self.image_paths]
        payload = {
            "story": STORY,
            "result": "PASS" if len(results) == 3 and all(item["passed"] for item in results)
            else "FAIL",
            "images": results,
        }
        path = os.path.join(self.run_dir, "image-verification.json")
        with open(path, "w", encoding="utf-8") as stream:
            json.dump(payload, stream, indent=2)
            stream.write("\n")
        status = payload["result"]
        return status, f"{path}: {results}", payload

    def report_extra(self):
        return {
            "control_damage": self.control_damage,
            "stagger_l1": self.stagger_l1,
            "knockdown_route": self.knockdown_route,
            "image_verification": os.path.join(self.run_dir, "image-verification.json"),
        }


Run = Us123Run


def main(argv=None):
    return kit.main(Us123Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
