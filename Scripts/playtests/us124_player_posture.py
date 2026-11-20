#!/usr/bin/env python3
"""US-124 player posture break, input lock, vulnerability and knockdown."""

import json
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

STORY = "US-124"
TITLE = "US-124 player posture break, input lock and vulnerability"
PLAYER_POSTURE_TEST_MAP = kit.PLAYER_POSTURE_TEST_MAP
FIGHT_SPOT = (-100.0, 0.0)
FIGHT_TOLERANCE = 40.0
RADIUS = 2000
TAG_BROKEN = "Mordecai.State.PostureBroken"
TAG_DOWN = "Mordecai.State.KnockedDown"
LOCKED_MOVEMENT_MODE = "NULL"  # UCharacterMovementComponent::GetMovementName for MOVE_None
CONTROL_DAMAGE = 20.0
BROKEN_DAMAGE = 30.0
DOWN_DAMAGE = 10.0
TOL = 0.6

SCREENSHOTS = {
    "broken": "the player visibly posture-broken with the posture HUD in its break state",
    "knocked_down": "the player knocked down after two 1.5x bonus hits cross 40 health",
    "recovered": "the player recovered with full posture and movement restored",
}
PENDING = {
    "AC-124.6": "bridge cannot drive: no two-client MultiplayerTestMap harness exists",
}

PLAN = (
    Step("setup",
         "walk from the safe PlayerStart to the dedicated multi-attacker fixture; read the "
         "player's full posture and the five TestAttacker-derived actors"),
    Step("unbroken_control",
         "the first real enemy swing lands before posture breaks -> exactly 20 health taken",
         proves=("AC-124.2",)),
    Step("break_and_lock",
         "the posture-breaker swing reaches posture 0 -> PostureBroken, movement mode NULL "
         "(MOVE_None), W input cannot move the player; capture the break HUD",
         proves=("AC-124.1", "AC-124.4", "AC-124.5")),
    Step("broken_bonus",
         "the next real enemy swing during PostureBroken takes 30 health, exactly 1.5x the "
         "20-health control, while the player remains broken",
         proves=("AC-124.2", "AC-124.5")),
    Step("knockdown",
         "a second broken-window enemy hit raises cumulative health taken to 60 -> "
         "KnockedDown replaces PostureBroken; capture the outcome",
         proves=("AC-124.2",)),
    Step("down_bonus",
         "the next real enemy swing while KnockedDown takes 10 health, 2x its 5 base damage",
         proves=("AC-124.2",)),
    Step("recovery",
         "after two seconds KnockedDown clears, posture=max, movement is Walking, and W moves "
         "the player again; capture recovery",
         proves=("AC-124.1", "AC-124.4", "AC-124.5")),
    Step("image_verification",
         "Pillow verifies all evidence frames are 16:9, at least 1280x720 and non-blank; "
         "write image-verification.json",
         kind="infra"),
)


def _num(value):
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None


def _attrs(player):
    value = (player or {}).get("attributes")
    return value if isinstance(value, dict) else {}


def _tags(player):
    return [str(tag) for tag in ((player or {}).get("tags") or [])]


def _health(player):
    return _num(_attrs(player).get("health"))


def _posture(player):
    return _num(_attrs(player).get("posture"))


def _max_posture(player):
    return _num(_attrs(player).get("max_posture"))


def _location(player):
    value = (player or {}).get("location")
    return [float(value[0]), float(value[1])] if isinstance(value, list) and len(value) >= 2 else None


def _fixture_actors(state):
    actors = (state or {}).get("actors")
    if not isinstance(actors, list):
        return []
    return [
        actor for actor in actors
        if isinstance(actor, dict)
        and actor.get("kind") == "enemy"
        and "PostureTestAttacker" in str(actor.get("label") or actor.get("name") or "")
    ]


def _image_stats(path):
    from PIL import Image, ImageStat

    with Image.open(path) as source:
        image = source.convert("RGB")
        width, height = image.size
        stddev = ImageStat.Stat(image).stddev
    return {
        "path": os.path.abspath(path),
        "width": width,
        "height": height,
        "rgb_stddev": [round(value, 3) for value in stddev],
        "non_blank": max(stddev) >= 3.0,
        "passed": (
            width >= 1280 and height >= 720
            and abs((width / height) - (16 / 9)) <= 0.01
            and max(stddev) >= 3.0
        ),
    }


class Us124Run(kit.PlaytestRun):
    STORY = STORY
    TITLE = TITLE
    FEATURES = ("posture_stagger", "movement", "combat_hud")
    MAP = PLAYER_POSTURE_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    PENDING = PENDING
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.start_health = None
        self.control_health = None
        self.break_health = None
        self.first_bonus_health = None
        self.down_health = None
        self.damage_route = []
        self.image_paths = []

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

    def _wait_state(self, predicate, timeout_s, description, poll_s=0.02):
        deadline = time.monotonic() + timeout_s
        last = None
        while time.monotonic() < deadline:
            last = self._read()
            if predicate(last):
                return last
            self.bridge.wait(poll_s)
        raise RuntimeError(
            f"{description} not seen in {timeout_s:g}s; last={kit.dumps(last, 1400)}")

    def steps(self):
        setup, _ = self.attempt("setup", self._guarded(self._setup))
        control, _ = self.attempt(
            "unbroken_control", self._guarded(self._unbroken_control), needs=setup,
            needs_why="not run: the posture fixture was not reached")
        broken, _ = self.attempt(
            "break_and_lock", self._guarded(self._break_and_lock), needs=control,
            needs_why="not run: the unbroken control hit did not pass")
        if broken:
            self.shot("broken")
            self.image_paths.append(self.screens[-1]["path"])
        bonus, _ = self.attempt(
            "broken_bonus", self._guarded(self._broken_bonus), needs=broken,
            needs_why="not run: PostureBroken was not observed")
        down, _ = self.attempt(
            "knockdown", self._guarded(self._knockdown), needs=bonus,
            needs_why="not run: the first broken-window bonus hit did not pass")
        if down:
            self.shot("knocked_down")
            self.image_paths.append(self.screens[-1]["path"])
        down_bonus, _ = self.attempt(
            "down_bonus", self._guarded(self._down_bonus), needs=down,
            needs_why="not run: KnockedDown was not observed")
        recovered, _ = self.attempt(
            "recovery", self._guarded(self._recovery), needs=down_bonus,
            needs_why="not run: the 2x knocked-down hit did not pass")
        if recovered:
            self.shot("recovered")
            self.image_paths.append(self.screens[-1]["path"])
        self.attempt(
            "image_verification", self._verify_images, kind="infra",
            needs=len(self.image_paths) == len(SCREENSHOTS),
            needs_why="not run: all three posture evidence screenshots were not captured")

    def _setup(self):
        state = self._read()
        player = _player(state)
        self.start_health = _health(player)
        posture, maximum = _posture(player), _max_posture(player)
        if self.start_health is None or posture is None or maximum is None:
            return "FAIL", f"player attributes unavailable: {player}", None
        if abs(posture - maximum) > 0.1:
            return "FAIL", f"player posture starts {posture}/{maximum}, expected full", None
        result = self.bridge.walk_to(*FIGHT_SPOT, tol=FIGHT_TOLERANCE)
        state = self._read()
        actors = _fixture_actors(state)
        if len(actors) != 5:
            return "FAIL", (
                f"expected five PostureTestAttacker fixture actors, found {len(actors)}: "
                f"{[actor.get('label') for actor in actors]}"), None
        return "PASS", (
            f"walked to {result.get('final')}; player health {self.start_health:g}, "
            f"posture {posture:g}/{maximum:g}; five fixture attackers present"), {
                "actors": [actor.get("label") for actor in actors]}

    def _unbroken_control(self):
        state = self._wait_state(
            lambda value: (
                _health(_player(value)) is not None
                and _health(_player(value)) <= self.start_health - CONTROL_DAMAGE + TOL
            ), 2.0, "unbroken control hit")
        player = _player(state)
        self.control_health = _health(player)
        damage = self.start_health - self.control_health
        problems = []
        if abs(damage - CONTROL_DAMAGE) > TOL:
            problems.append(f"damage {damage:g}, expected {CONTROL_DAMAGE:g}")
        if TAG_BROKEN in _tags(player) or TAG_DOWN in _tags(player):
            problems.append(f"vulnerability started before the control was read: {_tags(player)}")
        self.damage_route.append(damage)
        return ("FAIL" if problems else "PASS"), (
            f"unbroken enemy hit took {damage:g} health; tags {_tags(player)}"
            + (f" -- {'; '.join(problems)}" if problems else "")), {"damage": damage}

    def _break_and_lock(self):
        state = self._wait_state(
            lambda value: TAG_BROKEN in _tags(_player(value)),
            2.0, f"missing '{TAG_BROKEN}'")
        player = _player(state)
        self.break_health = _health(player)
        before = _location(player)
        if before is None:
            return "FAIL", f"player location unavailable at break: {player}", None
        self.bridge.tap("W", hold_s=0.10)
        after_state = self._read()
        after_player = _player(after_state)
        after = _location(after_player)
        moved = math.dist(before, after) if after is not None else 9999.0
        problems = []
        if TAG_BROKEN not in _tags(after_player):
            problems.append(f"missing '{TAG_BROKEN}' after lockout input")
        if after_player.get("movement_mode") != LOCKED_MOVEMENT_MODE:
            problems.append(
                f"movement_mode is {after_player.get('movement_mode')}, "
                f"expected {LOCKED_MOVEMENT_MODE} (MOVE_None)")
        if moved > 5.0:
            problems.append(f"W moved the broken player {moved:.1f} cm")
        return ("FAIL" if problems else "PASS"), (
            f"PostureBroken at health {self.break_health:g}; W moved {moved:.1f} cm; "
            f"movement_mode {after_player.get('movement_mode')}"
            + (f" -- {'; '.join(problems)}" if problems else "")), {
                "health": self.break_health, "moved_cm": moved, "tags": _tags(after_player)}

    def _broken_bonus(self):
        state = self._wait_state(
            lambda value: (
                _health(_player(value)) is not None
                and _health(_player(value)) <= self.break_health - BROKEN_DAMAGE + TOL
            ), 1.0, "1.5x broken-window bonus hit")
        player = _player(state)
        self.first_bonus_health = _health(player)
        damage = self.break_health - self.first_bonus_health
        ratio = damage / CONTROL_DAMAGE
        problems = []
        if abs(damage - BROKEN_DAMAGE) > TOL:
            problems.append(f"damage {damage:g}, expected {BROKEN_DAMAGE:g}")
        if abs(ratio - 1.5) > 0.05:
            problems.append(f"ratio {ratio:.3f}, expected 1.5")
        if TAG_BROKEN not in _tags(player):
            problems.append(f"missing '{TAG_BROKEN}' after the first bonus hit")
        self.damage_route.append(damage)
        return ("FAIL" if problems else "PASS"), (
            f"control {CONTROL_DAMAGE:g} -> broken {damage:g} ({ratio:.2f}x); tags {_tags(player)}"
            + (f" -- {'; '.join(problems)}" if problems else "")), {
                "damage": damage, "ratio": ratio}

    def _knockdown(self):
        state = self._wait_state(
            lambda value: TAG_DOWN in _tags(_player(value)),
            1.0, f"missing '{TAG_DOWN}' after 40+ broken-window health damage")
        player = _player(state)
        self.down_health = _health(player)
        second_bonus = self.first_bonus_health - self.down_health
        problems = []
        if abs(second_bonus - BROKEN_DAMAGE) > TOL:
            problems.append(f"second broken hit {second_bonus:g}, expected {BROKEN_DAMAGE:g}")
        if TAG_BROKEN in _tags(player):
            problems.append("PostureBroken was not replaced")
        if player.get("movement_mode") != LOCKED_MOVEMENT_MODE:
            problems.append(
                f"movement_mode is {player.get('movement_mode')}, "
                f"expected {LOCKED_MOVEMENT_MODE} (MOVE_None)")
        self.damage_route.append(second_bonus)
        return ("FAIL" if problems else "PASS"), (
            f"broken-window damage {self.damage_route[-2]:g}+{second_bonus:g}=60; "
            f"tags {_tags(player)}; movement_mode {player.get('movement_mode')}"
            + (f" -- {'; '.join(problems)}" if problems else "")), {
                "damage": second_bonus, "tags": _tags(player)}

    def _down_bonus(self):
        state = self._wait_state(
            lambda value: (
                _health(_player(value)) is not None
                and _health(_player(value)) <= self.down_health - DOWN_DAMAGE + TOL
            ), 1.0, "2x knocked-down bonus hit")
        player = _player(state)
        damage = self.down_health - _health(player)
        problems = []
        if abs(damage - DOWN_DAMAGE) > TOL:
            problems.append(f"damage {damage:g}, expected {DOWN_DAMAGE:g}")
        if TAG_DOWN not in _tags(player):
            problems.append(f"missing '{TAG_DOWN}' after the 2x hit")
        self.damage_route.append(damage)
        return ("FAIL" if problems else "PASS"), (
            f"5 base damage became {damage:g} while down; tags {_tags(player)}"
            + (f" -- {'; '.join(problems)}" if problems else "")), {"damage": damage}

    def _recovery(self):
        state = self._wait_state(
            lambda value: (
                TAG_BROKEN not in _tags(_player(value))
                and TAG_DOWN not in _tags(_player(value))
                and _posture(_player(value)) is not None
                and abs(_posture(_player(value)) - _max_posture(_player(value))) <= 0.1
                and (_player(value) or {}).get("movement_mode") == "Walking"
            ), 3.0, "knockdown recovery with full posture and Walking")
        before = _location(_player(state))
        self.bridge.tap("W", hold_s=0.20)
        after_state = self._read()
        after = _location(_player(after_state))
        moved = math.dist(before, after) if before is not None and after is not None else 0.0
        if moved < 50.0:
            return "FAIL", f"W moved the recovered player only {moved:.1f} cm", None
        player = _player(after_state)
        return "PASS", (
            f"tags cleared; posture {_posture(player):g}/{_max_posture(player):g}; "
            f"movement {player.get('movement_mode')}; W moved {moved:.1f} cm"), {
                "moved_cm": moved}

    def _verify_images(self):
        results = [_image_stats(path) for path in self.image_paths]
        payload = {
            "story": STORY,
            "result": "PASS" if results and all(item["passed"] for item in results) else "FAIL",
            "images": results,
        }
        path = os.path.join(self.run_dir, "image-verification.json")
        with open(path, "w", encoding="utf-8") as stream:
            json.dump(payload, stream, indent=2)
            stream.write("\n")
        return payload["result"], f"{path}: {results}", payload

    def report_extra(self):
        return {
            "damage_route": self.damage_route,
            "player_posture_test_map": PLAYER_POSTURE_TEST_MAP,
            "image_verification": os.path.join(self.run_dir, "image-verification.json"),
        }


Run = Us124Run


def main(argv=None):
    return kit.main(Us124Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
