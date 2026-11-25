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
SPRINT_KEY = "LeftShift"
F07_CORNER = (-935.0, 935.0)
F07_ROUTE = ((-935.0, 0.0), F07_CORNER)
RETURN_ROUTE = ((-935.0, 0.0), (0.0, 0.0))
CORNER_TOL = 20.0
WALL_NAME = "Arena_Wall_W"
FADED_MAX = 0.0001
RESTORED_MIN = 0.999
FADE_TIMEOUT_S = 0.5

SCREENSHOTS = {
    "f07_corner": "the living player fully visible at DevTestMap's original F07 +Y corner beside the "
                 "200 cm near (-X) arena wall, with only the camera-blocking wall dither-faded "
                 "and the combat HUD still visible",
    "restored": "the living player back at the arena centre with the west wall fully restored "
                "and visible in the gameplay camera frame, with the combat HUD still visible",
}

PLAN = (
    Step("walk_f07_corner", f"tap {SPRINT_KEY}, walk_to(-935, 0), then walk_to("
         f"{F07_CORNER[0]:g}, {F07_CORNER[1]:g}) with real WASD -> "
         f"within {CORNER_TOL:g} cm, x <= -915, and player.is_dead==false",
         proves=("AC-099.3",)),
    Step("wall_fades_at_corner", f"observe camera.occluders[{WALL_NAME}].fade <= "
         f"{FADED_MAX:g} within {FADE_TIMEOUT_S:g}s, then screenshot f07_corner",
         proves=("AC-099.2", "AC-099.3")),
    Step("walk_back_to_centre", "walk_to(-935, 0), then walk_to(0, 0) with real WASD",
         proves=("AC-099.3",)),
    Step("wall_restores_clear", f"observe camera.occluders[{WALL_NAME}].fade >= "
         f"{RESTORED_MIN:g} within {FADE_TIMEOUT_S:g}s, then screenshot restored",
         proves=("AC-099.2", "AC-099.3")),
)


def _occluder(state, name=WALL_NAME):
    camera = state.get("camera") if isinstance(state, dict) else None
    occluders = camera.get("occluders") if isinstance(camera, dict) else None
    if not isinstance(occluders, list):
        return None
    wanted = name.casefold()
    return next(
        (entry for entry in occluders
         if isinstance(entry, dict) and str(entry.get("name", "")).casefold() == wanted),
        None,
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
            route_results = []
            try:
                route_results.append(
                    {"sprint": self.bridge.tap(SPRINT_KEY)}
                )
            except ab.BridgeError as error:
                return ("FAIL", f"tap {SPRINT_KEY} before the corner route failed: {error}",
                        {"route": route_results, "error": error.response})

            for target in F07_ROUTE:
                try:
                    route_results.append(
                        self.bridge.walk_to(*target, tol=CORNER_TOL)
                    )
                except ab.BridgeError as error:
                    detail = f"walk_to {target} on the original F07 route failed: {error}"
                    if error.code == "player_dead":
                        detail += "; the Frontliners killed the player before the required shot"
                    return "FAIL", detail, {"route": route_results, "error": error.response}

            player = (self.bridge.state(["player"]).get("player") or {})
            location = player.get("location") or []
            if len(location) < 2:
                return "FAIL", f"player location missing after walk: {player}", route_results
            distance = math.hypot(location[0] - F07_CORNER[0], location[1] - F07_CORNER[1])
            if player.get("is_dead") is not False:
                return ("FAIL", "the player died before the original-F07-corner screenshot "
                        f"(location {location})", route_results)
            if distance > CORNER_TOL:
                return ("FAIL", f"player stopped {distance:.1f} cm from {F07_CORNER}, need <= "
                        f"{CORNER_TOL:g}", route_results)
            if float(location[0]) > -915.0:
                return ("FAIL", f"player x={float(location[0]):.1f}; must be <= -915 at the "
                        f"original F07 near-wall location", route_results)
            return ("PASS", f"real WASD reached ({location[0]:.1f}, {location[1]:.1f}), "
                    f"{distance:.1f} cm from {F07_CORNER}; x<=-915 and player alive",
                    {"route": route_results})

        reached, _ = self.attempt("walk_f07_corner", walk_corner)

        def wall_fades():
            try:
                state = self.bridge.wait_for(
                    lambda value: (
                        _occluder(value) is not None
                        and float(_occluder(value).get("fade", 1.0)) <= FADED_MAX
                    ),
                    timeout_s=FADE_TIMEOUT_S,
                    sections=["camera", "player"],
                    poll_s=0.05,
                )
            except ab.BridgeError as error:
                wall = _occluder(error.response or {})
                return ("FAIL", f"{WALL_NAME} did not reach fade <= {FADED_MAX:g} within "
                        f"{FADE_TIMEOUT_S:g}s; last={wall}", error.response)
            wall = _occluder(state)
            return ("PASS", f"{WALL_NAME} fade={float(wall['fade']):.3f} within "
                    f"{FADE_TIMEOUT_S:g}s", wall)

        faded, _ = self.attempt(
            "wall_fades_at_corner", wall_fades, needs=reached,
            needs_why="not checked: the player did not reach x<=-915 alive")
        self.shot("f07_corner", needs=faded,
                  needs_why="not taken: the west wall did not fully fade at the corner")

        def walk_centre():
            route_results = []
            for target in RETURN_ROUTE:
                try:
                    route_results.append(self.bridge.walk_to(*target, tol=CORNER_TOL))
                except ab.BridgeError as error:
                    return ("FAIL", f"walk_to {target} on the return route failed: {error}",
                            {"route": route_results, "error": error.response})
            player = (self.bridge.state(["player"]).get("player") or {})
            location = player.get("location") or []
            distance = math.hypot(float(location[0]), float(location[1])) if len(location) >= 2 else math.inf
            if distance > CORNER_TOL:
                return ("FAIL", f"player stopped {distance:.1f} cm from (0, 0), need <= "
                        f"{CORNER_TOL:g}", {"route": route_results, "player": player})
            return ("PASS", f"real WASD returned to ({location[0]:.1f}, {location[1]:.1f}), "
                    f"{distance:.1f} cm from (0, 0)", {"route": route_results})

        cleared, _ = self.attempt(
            "walk_back_to_centre", walk_centre, needs=faded,
            needs_why="not attempted: the wall did not fade at the corner")

        def wall_restores():
            try:
                state = self.bridge.wait_for(
                    lambda value: (
                        _occluder(value) is not None
                        and float(_occluder(value).get("fade", 0.0)) >= RESTORED_MIN
                    ),
                    timeout_s=FADE_TIMEOUT_S,
                    sections=["camera", "player"],
                    poll_s=0.05,
                )
            except ab.BridgeError as error:
                wall = _occluder(error.response or {})
                return ("FAIL", f"{WALL_NAME} did not restore to fade >= {RESTORED_MIN:g} "
                        f"within {FADE_TIMEOUT_S:g}s; last={wall}", error.response)
            wall = _occluder(state)
            return ("PASS", f"{WALL_NAME} fade={float(wall['fade']):.3f} within "
                    f"{FADE_TIMEOUT_S:g}s after leaving the wall", wall)

        restored, _ = self.attempt(
            "wall_restores_clear", wall_restores, needs=cleared,
            needs_why="not checked: the player did not return to the arena centre")
        self.shot("restored", needs=restored,
                  needs_why="not taken: the west wall did not fully restore")


Run = Us099Run


def main(argv=None):
    return kit.main(Us099Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
