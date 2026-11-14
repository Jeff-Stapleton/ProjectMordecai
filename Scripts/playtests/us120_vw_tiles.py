#!/usr/bin/env python3
"""US-120 Verdant Wilds tile fixture -- Tier 4 real-input traversal checks."""

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

SPAWN = (-1200.0, 0.0)
TIER0_Z = 92.0
TIER1_Z = 292.0
Z_TOL = 55.0
BLOCK_HOLD_S = 1.5
JUMP_SAMPLE_S = 2.0
GALLERY_VIEWPOINTS = (
    (-1200.0, 2000.0),
    (-400.0, 2000.0),
    (400.0, 2000.0),
    (1200.0, 2000.0),
)
RAMPS = {
    "2t": {"low": (-500.0, -600.0), "top": (250.0, -600.0)},
    "3t": {"low": (-700.0, 600.0), "top": (250.0, 600.0)},
}
CLIFF_START = (1100.0, 0.0)
CLIFF_FACE_X = 1400.0
CLIFF_MAX_X = 1370.0
SHALLOW_START = (1900.0, 0.0)
SHALLOW_END = (3100.0, 0.0)
SHALLOW_APPROACH = ((1500.0, -500.0), (1900.0, -500.0), SHALLOW_START)
SHALLOW_BED_Z = -38.0
SHALLOW_WATER_Z = -8.0
SHALLOW_DIP_Z = 54.0
SHALLOW_DIP_TOL = 22.0
SHALLOW_DIP_X = (2250.0, 2750.0)
SHALLOW_CROSS_TIMEOUT_S = 15.0
DEEP_BED_Z = -141.0
DEEP_WATER_Z = -8.0
CAPSULE_RADIUS_CM = 40.0
BANK_TOLERANCE_CM = 15.0
DEEP_ALLOWED_PAST_FACE = CAPSULE_RADIUS_CM + BANK_TOLERANCE_CM
DIAGONAL = math.sqrt(0.5)
DEEP_BANKS = (
    {
        "label": "edge",
        "pivot": (3800.0, -1000.0),
        "start": (4070.0, -1000.0),
        "keys": ("S",),
        "direction": (-1.0, 0.0),
        "face_progress": 0.0,
        "route": ((4200.0, -1400.0), (4200.0, -1000.0), (4070.0, -1000.0)),
    },
    {
        "label": "outer_corner",
        "pivot": (3800.0, 0.0),
        "start": (3970.0, -170.0),
        "keys": ("S", "D"),
        "direction": (-DIAGONAL, DIAGONAL),
        "face_progress": 0.0,
        "route": ((4200.0, -1000.0), (4200.0, -300.0), (3970.0, -170.0)),
    },
    {
        "label": "inner_corner",
        "pivot": (3800.0, 1000.0),
        "start": (3970.0, 830.0),
        "keys": ("S", "D"),
        "direction": (-DIAGONAL, DIAGONAL),
        # The concave bank's authored quarter-circle face is 100 cm into the
        # local -X/-Y water quadrant along this radial probe.
        "face_progress": 100.0,
        "route": ((4200.0, -300.0), (4200.0, 700.0), (3970.0, 830.0)),
    },
)
DEEP_SAFE_X = 4200.0
DEEP_ENTRY_ROUTE = ((3400.0, -1400.0), (DEEP_SAFE_X, -1400.0))

SCREENSHOTS = {
    "gallery_1": "the first labelled Verdant Wilds gallery group, with approved meshes and felt materials",
    "gallery_2": "the second labelled Verdant Wilds gallery group",
    "gallery_3": "the third labelled Verdant Wilds gallery group, including fenced pit pieces",
    "gallery_4": "the fourth labelled Verdant Wilds gallery group, including both ramps",
    "ramp_2t_top": "the player standing on the 200 cm Tier 1 platform above Ramp 2T",
    "ramp_3t_top": "the player standing on the 200 cm Tier 1 platform above Ramp 3T",
    "cliff_rejection": "the player below and on the near side of the 200 cm straight cliff after walking and jumping",
    "shallow_crossing": "the player wading in the visible shallow pond below land height, with bank, bed and blue water surface visible",
    "deep_edge_rejection": "the player on the near/land side of the deep pond Edge bank",
    "deep_outer_rejection": "the player on the near/land side of the deep pond OuterCorner bank",
    "deep_inner_rejection": "the player on the near/land side of the deep pond InnerCorner bank",
}

PLAN = (
    Step(
        "tile_gallery_visible",
        "walk through four catalogue viewpoints and capture all 17 labelled ready tiles",
        proves=("AC-120.1", "AC-120.4", "AC-120.5"),
    ),
    Step(
        "ramp_2t_up",
        "walk_to the Ramp 2T base, then its top without Space -> player Z rises about 200 cm",
        proves=("AC-120.6",),
    ),
    Step(
        "ramp_2t_down",
        "walk_to the Ramp 2T base from its top -> player returns to Tier 0 without sticking",
        proves=("AC-120.6",),
    ),
    Step(
        "ramp_3t_up",
        "walk_to the Ramp 3T base, then its top without Space -> player Z rises about 200 cm",
        proves=("AC-120.6",),
    ),
    Step(
        "ramp_3t_down",
        "walk_to the Ramp 3T base from its top -> player returns to Tier 0 without sticking",
        proves=("AC-120.6",),
    ),
    Step(
        "cliff_walk_blocked",
        "walk_to the cliff approach and hold W into the 200 cm face -> remain below/on the near side",
        proves=("AC-120.7",),
    ),
    Step(
        "cliff_jump_blocked",
        "hold W and tap Space into the cliff -> sampled player state never reaches Tier 1",
        proves=("AC-120.7",),
    ),
    Step(
        "shallow_pond_crosses",
        "hold W from land through the real shallow basin -> Z dips toward bed+capsule, then exits on far land",
        proves=("AC-120.7",),
    ),
    Step(
        "deep_bank_blocks",
        "approach deep Edge and both corners perpendicular to each authored face using real keys -> remain on land within bank tolerance",
        proves=("AC-120.7",),
    ),
)


def _player(bridge):
    state = bridge.state(["player"]).get("player") or {}
    location = state.get("location")
    if state.get("valid") is not True or not isinstance(location, list) or len(location) < 3:
        raise ab.BridgeError("invalid player state", kind="gameplay")
    return tuple(float(value) for value in location[:3])


def _near(value, target, tolerance=Z_TOL):
    return abs(value - target) <= tolerance


class Us120Run(kit.PlaytestRun):
    STORY = "US-120"
    FEATURES = ("movement",)
    TITLE = "US-120 Verdant Wilds v3 terrain tiles (Tier 4, real input)"
    MAP = kit.VW_TILE_TEST_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)
    WALK_TOL = 55.0

    def steps(self):
        self.step_gallery()
        self.step_ramp("2t")
        self.step_ramp("3t")
        self.step_cliff()
        self.step_shallow()
        self.step_deep()

    def _walk_sequence(self, points, timeout_s=20.0):
        details = []
        for x, y in points:
            response = self.bridge.walk_to(x, y, tol=self.WALK_TOL, timeout_s=timeout_s)
            final = response.get("final", [None, None])
            details.append(
                f"({x:.0f},{y:.0f}) reached at ({final[0]},{final[1]})"
            )
        return details

    def step_gallery(self):
        def visit():
            details = []
            for index, point in enumerate(GALLERY_VIEWPOINTS, 1):
                details.extend(self._walk_sequence((point,), timeout_s=25.0))
                self.shot(f"gallery_{index}")
            return "PASS", "; ".join(details), {"viewpoints": GALLERY_VIEWPOINTS}

        self.attempt("tile_gallery_visible", visit)

    def step_ramp(self, ramp):
        spec = RAMPS[ramp]
        up_name = f"ramp_{ramp}_up"
        down_name = f"ramp_{ramp}_down"

        def ascend():
            details = self._walk_sequence((spec["low"], spec["top"]), timeout_s=20.0)
            location = _player(self.bridge)
            ok = _near(location[2], TIER1_Z)
            detail = (
                f"{'; '.join(details)}; top Z {location[2]:.1f} "
                f"(need {TIER1_Z:.0f} +/- {Z_TOL:.0f}, no Space sent)"
            )
            return ("PASS" if ok else "FAIL"), detail, {"location": location}

        up, _ = self.attempt(up_name, ascend)
        self.shot(
            f"ramp_{ramp}_top",
            needs=up,
            needs_why=f"not captured: {up_name} failed",
        )

        def descend():
            response = self.bridge.walk_to(
                *spec["low"], tol=self.WALK_TOL, timeout_s=20.0
            )
            location = _player(self.bridge)
            ok = _near(location[2], TIER0_Z)
            detail = (
                f"base reached at {response.get('final')}; Z {location[2]:.1f} "
                f"(need {TIER0_Z:.0f} +/- {Z_TOL:.0f})"
            )
            return ("PASS" if ok else "FAIL"), detail, {"location": location}

        self.attempt(
            down_name,
            descend,
            needs=up,
            needs_why=f"not run: {up_name} did not reach Tier 1",
        )

    def step_cliff(self):
        def walk_blocked():
            self.bridge.walk_to(
                *CLIFF_START, tol=self.WALK_TOL, timeout_s=20.0
            )
            response = self.bridge.hold("W", BLOCK_HOLD_S)
            location = _player(self.bridge)
            ok = location[0] <= CLIFF_MAX_X and location[2] < TIER1_Z - Z_TOL
            detail = (
                f"held W {response.get('held_s')}s: location "
                f"({location[0]:.1f}, {location[1]:.1f}, {location[2]:.1f}); "
                f"need X <= {CLIFF_MAX_X:.0f} and below Tier 1"
            )
            return ("PASS" if ok else "FAIL"), detail, {"location": location}

        walked, _ = self.attempt("cliff_walk_blocked", walk_blocked)

        def jump_blocked():
            bridge = self.bridge
            bridge.walk_to(*CLIFF_START, tol=self.WALK_TOL, timeout_s=20.0)
            max_z = -math.inf
            max_x = -math.inf
            bridge.press("W")
            try:
                bridge.tap("SpaceBar", hold_s=self.TAP_S)
                deadline = time.monotonic() + JUMP_SAMPLE_S
                while time.monotonic() < deadline:
                    x, _, z = _player(bridge)
                    max_x = max(max_x, x)
                    max_z = max(max_z, z)
                    time.sleep(0.05)
            finally:
                bridge.release("W")
            ok = max_x <= CLIFF_MAX_X and max_z < TIER1_Z - Z_TOL
            detail = (
                f"real W+Space samples: max X {max_x:.1f} (need <= {CLIFF_MAX_X:.0f}), "
                f"max Z {max_z:.1f} (need < {TIER1_Z - Z_TOL:.0f})"
            )
            return ("PASS" if ok else "FAIL"), detail, {"max_x": max_x, "max_z": max_z}

        jumped, _ = self.attempt(
            "cliff_jump_blocked",
            jump_blocked,
            needs=walked,
            needs_why="not run: the walking cliff probe did not remain on the near side",
        )
        self.shot(
            "cliff_rejection",
            needs=walked and jumped,
            needs_why="not captured: a cliff rejection check failed",
        )

    def step_shallow(self):
        def cross():
            self._walk_sequence(SHALLOW_APPROACH, timeout_s=25.0)
            bridge = self.bridge
            min_z = math.inf
            min_location = None
            captured = False
            deadline = time.monotonic() + SHALLOW_CROSS_TIMEOUT_S
            bridge.press("W")
            try:
                while time.monotonic() < deadline:
                    location = _player(bridge)
                    if location[2] < min_z:
                        min_z = location[2]
                        min_location = location
                    if (
                        not captured
                        and abs(location[2] - SHALLOW_DIP_Z) <= SHALLOW_DIP_TOL
                    ):
                        self.shot("shallow_crossing")
                        captured = True
                    if location[0] >= SHALLOW_END[0] - 5.0:
                        break
                    time.sleep(0.05)
            finally:
                bridge.release("W")
            location = _player(bridge)
            reached_far_land = location[0] >= SHALLOW_END[0] - 5.0
            dipped = abs(min_z - SHALLOW_DIP_Z) <= SHALLOW_DIP_TOL
            exited = _near(location[2], TIER0_Z, tolerance=20.0)
            ok = reached_far_land and dipped and exited and captured
            if not dipped:
                outcome = (
                    f"did not dip toward bed+capsule Z {SHALLOW_DIP_Z:.0f}; "
                    f"min Z {min_z:.1f}"
                )
            elif not exited:
                outcome = f"did not exit onto land; exit Z {location[2]:.1f}"
            elif not reached_far_land:
                outcome = f"did not reach far land; exit X {location[0]:.1f}"
            else:
                outcome = "real basin dip and far-land exit observed"
            detail = (
                f"{outcome}; min Z {min_z:.1f} at {min_location}; "
                f"exit Z {location[2]:.1f} at "
                f"({location[0]:.1f}, {location[1]:.1f})"
            )
            return ("PASS" if ok else "FAIL"), detail, {
                "minimum_location": min_location,
                "exit_location": location,
                "bed_z": SHALLOW_BED_Z,
                "water_z": SHALLOW_WATER_Z,
            }

        self.attempt("shallow_pond_crosses", cross)

    def step_deep(self):
        results = []

        def probe_all():
            self._walk_sequence(DEEP_ENTRY_ROUTE, timeout_s=25.0)
            for spec in DEEP_BANKS:
                self._walk_sequence(spec["route"], timeout_s=25.0)
                max_progress = -math.inf
                min_z = math.inf
                location = _player(self.bridge)
                for key in spec["keys"]:
                    self.bridge.press(key)
                try:
                    deadline = time.monotonic() + BLOCK_HOLD_S
                    while time.monotonic() < deadline:
                        location = _player(self.bridge)
                        offset_x = location[0] - spec["pivot"][0]
                        offset_y = location[1] - spec["pivot"][1]
                        progress = (
                            offset_x * spec["direction"][0]
                            + offset_y * spec["direction"][1]
                        )
                        max_progress = max(max_progress, progress)
                        min_z = min(min_z, location[2])
                        time.sleep(0.05)
                finally:
                    for key in reversed(spec["keys"]):
                        self.bridge.release(key)
                max_allowed = spec["face_progress"] + DEEP_ALLOWED_PAST_FACE
                blocked = (
                    max_progress <= max_allowed
                    and min_z >= TIER0_Z - 20.0
                )
                results.append({
                    "representative": spec["label"],
                    "keys": spec["keys"],
                    "location": location,
                    "max_progress": max_progress,
                    "max_allowed": max_allowed,
                    "minimum_z": min_z,
                    "blocked": blocked,
                })
                self.shot(
                    f"deep_{spec['label'].replace('_corner', '')}_rejection"
                )
                self.bridge.walk_to(
                    DEEP_SAFE_X,
                    spec["start"][1],
                    tol=self.WALK_TOL,
                    timeout_s=25.0,
                )
            bad = [result for result in results if not result["blocked"]]
            detail = "; ".join(
                f"{result['representative']} progress/Z "
                f"{result['max_progress']:.1f}/{result['minimum_z']:.1f} "
                f"(limit {result['max_allowed']:.1f}) "
                f"({'blocked' if result['blocked'] else 'CROSSED'})"
                for result in results
            )
            return ("FAIL" if bad else "PASS"), (
                detail + "; each probe must remain within face + "
                f"{DEEP_ALLOWED_PAST_FACE:.0f} cm and near land Z"
            ), {
                "representatives": results,
                "bed_z": DEEP_BED_Z,
                "water_z": DEEP_WATER_Z,
            }

        self.attempt("deep_bank_blocks", probe_all)


Run = Us120Run


def main(argv=None):
    return kit.main(Us120Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
