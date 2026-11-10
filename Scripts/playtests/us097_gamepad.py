#!/usr/bin/env python3
"""US-097 gamepad movement and L3 sprint playtest (Tier 4, real input).

Runs on MovementTestMap. It pushes the left stick through the bridge's real axis input, then
clicks L3, pushes the stick again at sprint speed, clicks L3 a second time, and confirms the
same stick is back to walking speed. Nothing calls gameplay functions.
"""

import os
import sys

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-097"
TITLE = "US-097 valid gamepad key names (Tier 4, real input)"
LEFT_Y = "Gamepad_LeftY"
L3 = "Gamepad_LeftThumbstick"
MOVE_HOLD_S = 0.5
MOVE_MIN = 100.0
SPRINT_HOLD_S = 1.0
SPRINT_MIN = 700.0
WALK_BACK_RANGE = (400.0, 700.0)
SETTLE_TIMEOUT_S = 2.0
RUNTIME_IMC = "MordecaiIMC_Runtime"

SCREENSHOTS = {
    "start": "MovementTestMap's open grid floor from the fixed overhead camera, with the player "
             "at the centre and the Mordecai combat HUD visible",
    "after_gamepad": "the same enemy-free grid lane after left-stick movement and the L3 sprint; "
                     "the fixed camera has followed the player",
}

PLAN = (
    Step("spawn_ready", "state player -> a live Mordecai player at MovementTestMap's spawn"),
    Step("imc_active", f"state input -> {RUNTIME_IMC!r} is active"),
    Step("gamepad_move", f"axis {LEFT_Y}=1 for {MOVE_HOLD_S:g}s -> player.location x rises > "
         f"{MOVE_MIN:g} cm", proves=("AC-097.2",)),
    Step("sprint_l3", f"tap {L3}, axis {LEFT_Y}=1 for {SPRINT_HOLD_S:g}s -> dx > "
         f"{SPRINT_MIN:g} cm; tap L3 again, axis {LEFT_Y}=-1 for {SPRINT_HOLD_S:g}s -> walk "
         f"back {WALK_BACK_RANGE[0]:g}..{WALK_BACK_RANGE[1]:g} cm", proves=("AC-097.3",)),
)


class Us097Run(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("movement", "sprint")
    TITLE = TITLE
    RUN_SUFFIX = "us097"
    MAP = kit.MOVEMENT_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.measurements = []

    @staticmethod
    def _player(bridge):
        player = kit.section(bridge.state(["player"]), "player")
        location = player.get("location")
        if player.get("valid") is not True or not (
                isinstance(location, list) and len(location) >= 2):
            return None, player
        return (float(location[0]), float(location[1])), player

    @staticmethod
    def _settle(bridge):
        try:
            bridge.wait_until("player.speed<1", timeout_s=SETTLE_TIMEOUT_S)
            return None
        except ab.BridgeError as error:
            if error.code != "timeout":
                raise
            return f"player was still moving {SETTLE_TIMEOUT_S:g}s after release"

    def _axis_measure(self, value, seconds):
        start, raw = self._player(self.bridge)
        if start is None:
            return None, f"player state invalid before axis input: {raw}"
        response = self.bridge.axis({LEFT_Y: value}, hold_s=seconds)
        settle_error = self._settle(self.bridge)
        end, raw = self._player(self.bridge)
        if end is None:
            return None, f"player state invalid after axis input: {raw}"
        return {
            "start": list(start),
            "end": list(end),
            "dx": round(end[0] - start[0], 1),
            "dy": round(end[1] - start[1], 1),
            "held_s": response.get("held_s"),
            "settle_error": settle_error,
        }, None

    def steps(self):
        self.observe(
            "spawn_ready", ["player"],
            lambda state: (kit.section(state, "player").get("valid") is True
                           and kit.section(state, "player").get("is_dead") is False),
            lambda state: f"player={kit.section(state, 'player')}", timeout_s=3)
        self.observe(
            "imc_active", ["input"],
            lambda state: RUNTIME_IMC in (kit.section(state, "input").get("mapping_contexts") or []),
            lambda state: f"input.mapping_contexts="
                          f"{kit.section(state, 'input').get('mapping_contexts')}", timeout_s=3)
        self.shot("start")
        self.attempt("gamepad_move", self._gamepad_move)
        self.attempt("sprint_l3", self._sprint_l3)
        self.shot("after_gamepad")

    def _gamepad_move(self):
        measurement, error = self._axis_measure(1.0, MOVE_HOLD_S)
        if error:
            return "FAIL", error, None
        self.measurements.append({"check": "gamepad_move", **measurement})
        ok = measurement["dx"] > MOVE_MIN and not measurement["settle_error"]
        detail = (f"{LEFT_Y}=1 for {measurement['held_s']}s: "
                  f"{measurement['start']} -> {measurement['end']}, dx {measurement['dx']:.0f} "
                  f"cm (need > {MOVE_MIN:g})")
        if measurement["settle_error"]:
            detail += f"; {measurement['settle_error']}"
        return ("PASS" if ok else "FAIL"), detail, measurement

    def _sprint_l3(self):
        self.bridge.tap(L3, hold_s=self.TAP_S)
        forward, error = self._axis_measure(1.0, SPRINT_HOLD_S)
        self.bridge.tap(L3, hold_s=self.TAP_S)
        if error:
            return "FAIL", error, None
        backward, error = self._axis_measure(-1.0, SPRINT_HOLD_S)
        if error:
            return "FAIL", error, None

        forward_dx = forward["dx"]
        back_distance = -backward["dx"]
        data = {"forward": forward, "backward_after_toggle": backward}
        self.measurements.append({"check": "sprint_l3", **data})
        ok = (forward_dx > SPRINT_MIN
              and WALK_BACK_RANGE[0] <= back_distance <= WALK_BACK_RANGE[1]
              and not forward["settle_error"] and not backward["settle_error"])
        detail = (f"tapped L3, {LEFT_Y}=1 for {forward['held_s']}s: dx {forward_dx:.0f} cm "
                  f"(need > {SPRINT_MIN:g}); tapped L3 again, {LEFT_Y}=-1 for "
                  f"{backward['held_s']}s: {back_distance:.0f} cm back (need a walk in "
                  f"{WALK_BACK_RANGE[0]:g}..{WALK_BACK_RANGE[1]:g})")
        errors = [value for value in (forward["settle_error"], backward["settle_error"]) if value]
        if errors:
            detail += "; " + "; ".join(errors)
        return ("PASS" if ok else "FAIL"), detail, data

    def report_extra(self):
        return {"gamepad_measurements": self.measurements}


Run = Us097Run


def main(argv=None):
    return kit.main(Us097Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
