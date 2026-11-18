#!/usr/bin/env python3
"""US-122 stamina runtime — Tier 4 packaged playtest with real input.

The run reaches MovementTestMap's open lane, toggles sprint with Left Shift,
cycles WASD so it can drain the authoritative PlayerState stamina without
running into a boundary, proves an idle toggle auto-clears without draining,
checks low-tier sprint still reaches the legacy 1.5x speed, stops sprinting,
verifies the post-spend delay, and watches stamina refill to Green. Screenshots
are verified with Pillow and the machine-readable result is written beside the
report as image-verification.json.
"""

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

STORY = "US-122"
TITLE = "US-122 stamina runtime: sprint drain, tier tag, delay and refill"
SPRINT_KEY = "LeftShift"
BLOCK_AXIS = "Gamepad_LeftTriggerAxis"
TAG_SPRINTING = "Mordecai.State.Sprinting"
TAG_BLOCKING = "Mordecai.State.Blocking"
TAG_GREEN = "Mordecai.Stamina.Tier.Green"
TAG_RED = "Mordecai.Stamina.Tier.Red"
TAG_EXHAUSTED = "Mordecai.Stamina.Tier.Exhausted"
LOW_TAGS = (TAG_RED, TAG_EXHAUSTED)
LANE_WAYPOINTS = ((-650.0, 0.0), (-700.0, -650.0), (-300.0, -800.0))
PULSE_S = 0.5
DELAYED_MOVE_WAIT_S = 0.5
IDLE_CLEAR_WAIT_S = 1.35
STOP_TAP_HOLD_S = 0.01
STATIONARY_SPEED_MAX = 5.0
STATIONARY_WAIT_TIMEOUT_S = 2.0
LOW_STAMINA = 25.0
DELAY_OBSERVE_S = 0.75
REFILLED_STAMINA = 95.0
LOW_TIER_SPEED_HOLD_S = 0.5
LOW_TIER_SPRINT_SPEED_MIN = 850.0
LOW_TIER_SPRINT_SPEED_MAX = 950.0
SPEED_POLL_S = 0.03
MOVEMENT_CYCLE = ("W", "D", "S", "A")

SCREENSHOTS = {
    "drained": "MovementTestMap with the Mordecai HUD stamina bar visibly low and Red/Exhausted",
    "refilled": "the same packaged HUD after stamina refills, with a nearly full Green bar",
}

PLAN = (
    Step("walk_lane", "walk_to three open-floor waypoints ending at (-300, -800) on lane L1"),
    Step("idle_sprint_auto_clears",
         f"release movement and observe speed <= {STATIONARY_SPEED_MAX:g} cm/s, tap {SPRINT_KEY}, "
         f"wait {IDLE_CLEAR_WAIT_S:g}s -> stamina does not decrease and '{TAG_SPRINTING}' "
         "auto-clears", proves=("AC-122.11", "AC-122.12", "AC-122.17")),
    Step("block_rejects_sprint",
         f"hold {BLOCK_AXIS}=1 until '{TAG_BLOCKING}', tap {SPRINT_KEY}, release block -> "
         f"'{TAG_SPRINTING}' never appears and does not appear after block release"),
    Step("sprint_drains",
         f"tap {SPRINT_KEY}, wait {DELAYED_MOVE_WAIT_S:g}s, then hold W {PULSE_S:g}s -> "
         f"'{TAG_SPRINTING}' remains in player.tags and player.attributes.stamina decreases",
         proves=("AC-122.7",)),
    Step("low_tier_authoritative",
         f"cycle real WASD holds while sprint remains tagged until stamina <= {LOW_STAMINA:g}; "
         f"'{TAG_RED}' or '{TAG_EXHAUSTED}' is authoritative; capture drained HUD",
         proves=("AC-122.4", "AC-122.8", "AC-122.9", "AC-122.10")),
    Step("low_tier_sprint_speed",
         f"while low-tier sprint remains active, continue the held movement direction for "
         f"{LOW_TIER_SPEED_HOLD_S:g}s and sample player.speed -> "
         f"{LOW_TIER_SPRINT_SPEED_MIN:g}..{LOW_TIER_SPRINT_SPEED_MAX:g} cm/s "
         "(legacy 600 x 1.5 at every tier)", proves=("AC-122.13", "AC-122.17")),
    Step("regen_delay",
         f"tap {SPRINT_KEY} -> sprint tag removed; wait {DELAY_OBSERVE_S:g}s and stamina does "
         "not increase", proves=("AC-122.2", "AC-122.7")),
    Step("refills_green",
         f"wait without input until stamina >= {REFILLED_STAMINA:g} and '{TAG_GREEN}' is present; "
         "capture refilled HUD", proves=("AC-122.1", "AC-122.9", "AC-122.10")),
    Step("image_verification",
         "Pillow checks both captures are non-uniform and detects Red in the drained HUD and "
         "Green in the refilled HUD; write image-verification.json",
         proves=("AC-122.10",), kind="infra"),
)


def player_state(bridge):
    state = bridge.state(["player"])
    player = kit.section(state, "player")
    attrs = player.get("attributes")
    tags = player.get("tags")
    return (
        float(attrs.get("stamina")) if isinstance(attrs, dict)
        and isinstance(attrs.get("stamina"), (int, float)) else None,
        [str(tag) for tag in tags] if isinstance(tags, list) else [],
        player,
    )


def _near(pixel, target, tolerance=58.0):
    return math.sqrt(sum((float(pixel[i]) - target[i]) ** 2 for i in range(3))) <= tolerance


def analyze_hud_image(path, expected):
    from PIL import Image, ImageStat

    with Image.open(path) as source:
        image = source.convert("RGB")
        full_stddev = ImageStat.Stat(image).stddev
        # Combat HUD gauges occupy the upper-left 500x150 viewport region.
        hud = image.crop((0, 0, min(500, image.width), min(150, image.height)))
        pixels = list(hud.getdata())
        green = sum(_near(pixel, (76, 203, 110)) for pixel in pixels)
        red = sum(_near(pixel, (255, 90, 60)) for pixel in pixels)

    non_uniform = max(full_stddev) >= 3.0
    colour_count = red if expected == "red" else green
    return {
        "path": os.path.abspath(path),
        "expected": expected,
        "non_uniform": non_uniform,
        "rgb_stddev": [round(value, 3) for value in full_stddev],
        "hud_green_pixels": green,
        "hud_red_pixels": red,
        "passed": non_uniform and colour_count >= 20,
    }


class Us122Run(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("stamina", "sprint", "combat_hud")
    TITLE = TITLE
    MAP = kit.MOVEMENT_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.drained_path = None
        self.refilled_path = None
        self.stamina_samples = []
        self.sprint_direction = None

    def steps(self):
        reached, _ = self.attempt("walk_lane", self._walk_lane)
        idle, _ = self.attempt(
            "idle_sprint_auto_clears", self._idle_sprint_auto_clears,
            needs=reached, needs_why="not run: lane L1 was not reached")
        blocked, _ = self.attempt(
            "block_rejects_sprint", self._block_rejects_sprint,
            needs=idle, needs_why="not run: idle sprint did not auto-clear cleanly")
        sprinted, _ = self.attempt(
            "sprint_drains", self._start_sprint,
            needs=blocked, needs_why="not run: sprint refusal during block was not verified")
        low, _ = self.attempt(
            "low_tier_authoritative", self._drain_to_low,
            needs=sprinted, needs_why="not run: real sprint did not start and drain stamina")
        low_speed, _ = self.attempt(
            "low_tier_sprint_speed", self._low_tier_sprint_speed,
            needs=low, needs_why="not run: sprint did not reach a low authoritative tier")
        if low:
            self.shot("drained")
            self.drained_path = self.screens[-1]["path"] if self.screens else None
        stopped, _ = self.attempt(
            "regen_delay", self._stop_and_check_delay,
            needs=low_speed, needs_why="not run: low-tier sprint speed was not verified")
        refilled, _ = self.attempt(
            "refills_green", self._wait_for_refill,
            needs=stopped, needs_why="not run: sprint was not stopped cleanly")
        if refilled:
            self.shot("refilled")
            self.refilled_path = self.screens[-1]["path"] if self.screens else None
        self.attempt(
            "image_verification", self._verify_images, kind="infra",
            needs=bool(self.drained_path and self.refilled_path),
            needs_why="not run: both drained and refilled captures were not written")

    def _walk_lane(self):
        details = []
        for x, y in LANE_WAYPOINTS:
            result = self.bridge.walk_to(x, y, tol=60.0)
            details.append(
                f"({x:g},{y:g}), distance {result.get('dist')}, pulses {result.get('pulses')}")
        return "PASS", " -> ".join(details), {"waypoints": LANE_WAYPOINTS}

    def _sample(self, label):
        stamina, tags, _ = player_state(self.bridge)
        self.stamina_samples.append({
            "label": label,
            "stamina": stamina,
            "tags": tags,
            "at": round(time.monotonic(), 3),
        })
        return stamina, tags

    def _idle_sprint_auto_clears(self):
        deadline = time.monotonic() + STATIONARY_WAIT_TIMEOUT_S
        settled_speed = None
        while time.monotonic() < deadline:
            _, _, player = player_state(self.bridge)
            speed = player.get("speed")
            if isinstance(speed, (int, float)):
                settled_speed = float(speed)
                if settled_speed <= STATIONARY_SPEED_MAX:
                    break
            self.bridge.wait(SPEED_POLL_S)
        if settled_speed is None or settled_speed > STATIONARY_SPEED_MAX:
            return "FAIL", (
                f"pawn did not settle below {STATIONARY_SPEED_MAX:g} cm/s before the idle "
                f"toggle (last speed {settled_speed})"), {"speed": settled_speed}

        before, _ = self._sample("before_idle_toggle")
        if before is None:
            return "FAIL", "player.attributes.stamina is unavailable", None
        self.bridge.tap(SPRINT_KEY, hold_s=0.1)
        self.bridge.wait(IDLE_CLEAR_WAIT_S)
        after, tags = self._sample("after_idle_clear")
        problems = []
        if after is None or after < before - 0.1:
            problems.append(f"standing sprint drained stamina ({before} -> {after})")
        if TAG_SPRINTING in tags:
            problems.append(f"'{TAG_SPRINTING}' did not auto-clear (tags {tags})")
        detail = (
            f"settled at {settled_speed:g} cm/s, tap {SPRINT_KEY}, wait "
            f"{IDLE_CLEAR_WAIT_S:g}s: stamina {before} -> {after}; tags {tags}")
        return ("FAIL" if problems else "PASS"), (
            detail + (f" -- {'; '.join(problems)}" if problems else "")), {
                "settled_speed": settled_speed,
                "before": before,
                "after": after,
                "tags": tags,
            }

    def _block_rejects_sprint(self):
        self.bridge.axis({BLOCK_AXIS: 1.0})
        try:
            deadline = time.monotonic() + 1.0
            blocking_tags = []
            while time.monotonic() < deadline:
                _, blocking_tags, _ = player_state(self.bridge)
                if TAG_BLOCKING in blocking_tags:
                    break
                self.bridge.wait(SPEED_POLL_S)
            if TAG_BLOCKING not in blocking_tags:
                return "FAIL", (
                    f"held {BLOCK_AXIS}=1 but '{TAG_BLOCKING}' did not appear; "
                    f"tags {blocking_tags}"), {"tags": blocking_tags}

            self.bridge.tap(SPRINT_KEY, hold_s=0.1)
            self.bridge.wait(0.15)
            _, during_tags, _ = player_state(self.bridge)
            if TAG_SPRINTING in during_tags:
                return "FAIL", (
                    f"tap {SPRINT_KEY} while blocking started sprint; tags {during_tags}"), {
                        "during_tags": during_tags}
        finally:
            self.bridge.axis({BLOCK_AXIS: 0.0})

        deadline = time.monotonic() + 1.0
        after_tags = []
        while time.monotonic() < deadline:
            _, after_tags, _ = player_state(self.bridge)
            if TAG_BLOCKING not in after_tags:
                break
            self.bridge.wait(SPEED_POLL_S)
        self.bridge.wait(0.1)
        _, after_tags, _ = player_state(self.bridge)
        problems = []
        if TAG_BLOCKING in after_tags:
            problems.append(f"'{TAG_BLOCKING}' remained after release")
        if TAG_SPRINTING in after_tags:
            problems.append(f"'{TAG_SPRINTING}' appeared after block release")
        detail = (
            f"held {BLOCK_AXIS}=1, tapped {SPRINT_KEY}, released block; tags {after_tags}")
        return ("FAIL" if problems else "PASS"), (
            detail + (f" -- {'; '.join(problems)}" if problems else "")), {
                "after_tags": after_tags}

    def _start_sprint(self):
        before, _ = self._sample("before_sprint")
        if before is None:
            return "FAIL", "player.attributes.stamina is unavailable", None
        self.bridge.tap(SPRINT_KEY, hold_s=0.1)
        self.bridge.wait(DELAYED_MOVE_WAIT_S)
        waiting, waiting_tags = self._sample("after_delayed_move_wait")
        if TAG_SPRINTING not in waiting_tags:
            return "FAIL", (
                f"tap {SPRINT_KEY}, wait {DELAYED_MOVE_WAIT_S:g}s: sprint auto-cleared "
                f"before movement (tags {waiting_tags})"), {
                    "before": before, "waiting": waiting, "tags": waiting_tags}
        if waiting is None or waiting < before - 0.1:
            return "FAIL", (
                f"stationary sprint drained during the {DELAYED_MOVE_WAIT_S:g}s grace "
                f"({before} -> {waiting})"), {
                    "before": before, "waiting": waiting, "tags": waiting_tags}
        self.bridge.press("W")
        self.sprint_direction = "W"
        self.bridge.wait(PULSE_S)
        after, tags = self._sample("first_sprint_pulse")
        problems = []
        if TAG_SPRINTING not in tags:
            problems.append(f"'{TAG_SPRINTING}' absent (tags {tags})")
        if after is None or after >= before - 0.1:
            problems.append(f"stamina did not decrease ({before} -> {after})")
        detail = (
            f"tap {SPRINT_KEY}, wait {DELAYED_MOVE_WAIT_S:g}s at stamina {waiting}, "
            f"then hold W {PULSE_S:g}s: stamina {before} -> {after}; tags {tags}")
        return ("FAIL" if problems else "PASS"), (
            detail + (f" -- {'; '.join(problems)}" if problems else "")), {
                "before": before, "waiting": waiting, "after": after, "tags": tags}

    def _drain_to_low(self):
        deadline = time.monotonic() + 15.0
        if self.sprint_direction not in MOVEMENT_CYCLE:
            return "FAIL", "no movement key remains held from sprint_drains", None
        while time.monotonic() < deadline:
            stamina, tags = self._sample("draining")
            if stamina is not None and stamina <= LOW_STAMINA:
                if TAG_SPRINTING not in tags:
                    return "FAIL", f"stamina reached {stamina}, but sprint tag disappeared: {tags}", None
                low = [tag for tag in tags if tag in LOW_TAGS]
                if not low:
                    return "FAIL", f"stamina reached {stamina}, but no low tier tag: {tags}", None
                return "PASS", f"stamina {stamina}; sprint active; authoritative tier {low}", {
                    "stamina": stamina, "tags": tags}
            if TAG_SPRINTING not in tags:
                return "FAIL", f"sprint tag disappeared before low stamina (stamina {stamina}, tags {tags})", None
            self.bridge.wait(PULSE_S)
            cycle_index = MOVEMENT_CYCLE.index(self.sprint_direction)
            next_direction = MOVEMENT_CYCLE[(cycle_index + 1) % len(MOVEMENT_CYCLE)]
            # Press the next direction before releasing the old one so there is
            # no stationary gap that should legitimately auto-clear sprint.
            self.bridge.press(next_direction)
            self.bridge.release(self.sprint_direction)
            self.sprint_direction = next_direction
        stamina, tags = self._sample("drain_timeout")
        return "FAIL", f"stamina did not reach {LOW_STAMINA:g} in 15s ({stamina}, tags {tags})", None

    def _low_tier_sprint_speed(self):
        peak = 0.0
        if self.sprint_direction not in MOVEMENT_CYCLE:
            return "FAIL", "no movement key remains held at low stamina", None
        sampled_direction = self.sprint_direction
        deadline = time.monotonic() + LOW_TIER_SPEED_HOLD_S
        while time.monotonic() < deadline:
            _, tags, player = player_state(self.bridge)
            speed = player.get("speed")
            if isinstance(speed, (int, float)):
                peak = max(peak, float(speed))
            if TAG_SPRINTING not in tags:
                return "FAIL", (
                    f"sprint cleared during the low-tier speed sample (peak {peak}, tags "
                    f"{tags})"), None
            time.sleep(SPEED_POLL_S)
        stamina, tags = self._sample("low_tier_speed")
        passed = LOW_TIER_SPRINT_SPEED_MIN <= peak <= LOW_TIER_SPRINT_SPEED_MAX
        detail = (
            f"continued held {sampled_direction} {LOW_TIER_SPEED_HOLD_S:g}s at stamina "
            f"{stamina}: peak speed {peak:.1f} cm/s (expected {LOW_TIER_SPRINT_SPEED_MIN:g}.."
            f"{LOW_TIER_SPRINT_SPEED_MAX:g}); tags {tags}")
        return ("PASS" if passed else "FAIL"), detail, {
            "peak_speed": peak, "stamina": stamina, "tags": tags}

    def _stop_and_check_delay(self):
        before, _ = self._sample("before_stop")
        # The delay starts at the final stamina spend, not after this input
        # helper returns. Use a one-frame real tap so the full 0.75s
        # observation remains inside the specified 1.0s regeneration delay.
        self.bridge.tap(SPRINT_KEY, hold_s=STOP_TAP_HOLD_S)
        if self.sprint_direction:
            self.bridge.release(self.sprint_direction)
            self.sprint_direction = None
        stopped, tags, _ = player_state(self.bridge)
        if TAG_SPRINTING in tags:
            return "FAIL", f"tap {SPRINT_KEY}: sprint tag remained (tags {tags})", None
        self.bridge.wait(DELAY_OBSERVE_S)
        delayed, delayed_tags = self._sample("inside_regen_delay")
        if before is None or stopped is None or delayed is None:
            return "FAIL", "stamina unavailable while checking the delay", None
        if delayed > stopped + 0.1:
            return "FAIL", (
                f"stamina regenerated inside {DELAY_OBSERVE_S:g}s delay: "
                f"{stopped} -> {delayed}"), None
        return "PASS", (
            f"tap {SPRINT_KEY}: sprint tag removed; stamina {before} -> {stopped} -> "
            f"{delayed} during {DELAY_OBSERVE_S:g}s delay; tags {delayed_tags}"), {
                "before_stop": before, "after_stop": stopped, "after_delay": delayed}

    def _wait_for_refill(self):
        def ready(state):
            player = kit.section(state, "player")
            attrs = player.get("attributes")
            tags = player.get("tags")
            return (
                isinstance(attrs, dict)
                and isinstance(attrs.get("stamina"), (int, float))
                and float(attrs["stamina"]) >= REFILLED_STAMINA
                and isinstance(tags, list)
                and TAG_GREEN in tags
                and TAG_SPRINTING not in tags
            )

        try:
            state = self.bridge.wait_for(ready, timeout_s=10.0, sections=["player"], poll_s=0.1)
        except ab.BridgeError as error:
            if error.code != "wait_for_timeout":
                raise
            stamina, tags, _ = player_state(self.bridge)
            return "FAIL", f"did not refill Green in 10s (stamina {stamina}, tags {tags})", None
        player = kit.section(state, "player")
        stamina = float(player["attributes"]["stamina"])
        tags = [str(tag) for tag in player["tags"]]
        self.stamina_samples.append({"label": "refilled", "stamina": stamina, "tags": tags})
        return "PASS", f"stamina refilled to {stamina}; authoritative Green tag present", {
            "stamina": stamina, "tags": tags}

    def _verify_images(self):
        results = [
            analyze_hud_image(self.drained_path, "red"),
            analyze_hud_image(self.refilled_path, "green"),
        ]
        payload = {
            "story": STORY,
            "generated_by": os.path.abspath(__file__),
            "result": "PASS" if all(item["passed"] for item in results) else "FAIL",
            "images": results,
        }
        path = os.path.join(self.run_dir, "image-verification.json")
        with open(path, "w", encoding="utf-8") as stream:
            json.dump(payload, stream, indent=2)
            stream.write("\n")
        if payload["result"] != "PASS":
            return "FAIL", f"{path}: {results}", payload
        return "PASS", (
            f"{path}: drained red pixels {results[0]['hud_red_pixels']}; "
            f"refilled green pixels {results[1]['hud_green_pixels']}"), payload

    def report_extra(self):
        return {
            "stamina_samples": self.stamina_samples,
            "image_verification": os.path.join(self.run_dir, "image-verification.json"),
        }


Run = Us122Run


def main(argv=None):
    return kit.main(Us122Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
