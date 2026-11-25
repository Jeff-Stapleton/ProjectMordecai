#!/usr/bin/env python3
"""US-099 Tier 4: walk behind DevTestMap's near wall with real WASD input."""

import math
import os
import sys

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

set_log_stream = kit.set_log_stream

STORY = "US-099"
TITLE = "US-099 near-wall visibility playtest (Tier 4, real input)"
RUN_SUFFIX = "us099"
SW_CORNER = (-930.0, -930.0)
CORNER_TOL = 60.0

SCREENSHOTS = {
    "sw_corner": "the living player fully visible at DevTestMap's south-west corner beside the "
                 "200 cm near (-X) arena wall, with only the camera-blocking wall dither-faded "
                 "and the combat HUD still visible",
}

PLAN = (
    Step("walk_sw_corner", f"walk_to({SW_CORNER[0]:g}, {SW_CORNER[1]:g}) with real WASD -> "
         f"within {CORNER_TOL:g} cm and player.is_dead==false, then immediately screenshot "
         "sw_corner", proves=("AC-099.3",)),
)


class Us099Run(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("camera", "movement")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.DEFAULT_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = ()
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def steps(self):
        def walk_corner():
            try:
                result = self.bridge.walk_to(*SW_CORNER, tol=CORNER_TOL)
            except ab.BridgeError as error:
                detail = f"walk_to south-west corner failed: {error}"
                if error.code == "player_dead":
                    detail += "; the Frontliners killed the player before the required shot"
                return "FAIL", detail, error.response

            player = (self.bridge.state(["player"]).get("player") or {})
            location = player.get("location") or []
            if len(location) < 2:
                return "FAIL", f"player location missing after walk: {player}", result
            distance = math.hypot(location[0] - SW_CORNER[0], location[1] - SW_CORNER[1])
            if player.get("is_dead") is not False:
                return ("FAIL", "the player died before the south-west-corner screenshot "
                        f"(location {location})", result)
            if distance > CORNER_TOL:
                return ("FAIL", f"player stopped {distance:.1f} cm from {SW_CORNER}, need <= "
                        f"{CORNER_TOL:g}", result)
            return ("PASS", f"real WASD reached ({location[0]:.1f}, {location[1]:.1f}), "
                    f"{distance:.1f} cm from {SW_CORNER}; player alive; capturing immediately",
                    result)

        reached, _ = self.attempt("walk_sw_corner", walk_corner)
        self.shot("sw_corner", needs=reached,
                  needs_why="not taken: the player did not reach the corner alive")


Run = Us099Run


def main(argv=None):
    return kit.main(Us099Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
