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
CLIFF_MAX_X = 1330.0
SHALLOW_START = (1900.0, 0.0)
SHALLOW_END = (3100.0, 0.0)
DEEP_BANKS = (
    ("edge", (4100.0, -800.0)),
    ("outer_corner", (4100.0, 0.0)),
    ("inner_corner", (4100.0, 800.0)),
)
DEEP_FACE_X = 3800.0
DEEP_MIN_X = 3870.0

SCREENSHOTS = {
    "gallery_1": "the first labelled Verdant Wilds gallery group, with approved meshes and felt materials",
    "gallery_2": "the second labelled Verdant Wilds gallery group",
    "gallery_3": "the third labelled Verdant Wilds gallery group, including fenced pit pieces",
    "gallery_4": "the fourth labelled Verdant Wilds gallery group, including both ramps",
    "ramp_2t_top": "the player standing on the 200 cm Tier 1 platform above Ramp 2T",
    "ramp_3t_top": "the player standing on the 200 cm Tier 1 platform above Ramp 3T",
    "cliff_rejection": "the player below and on the near side of the 200 cm straight cliff after walking and jumping",
    "shallow_crossing": "the player on the far side of the shallow pond lane",
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
        "walk_to the far side of the shallow pond lane -> reaches the far waypoint",
        proves=("AC-120.7",),
    ),
    Step(
        "deep_bank_blocks",
        "hold S downhill from the land side into deep Edge, OuterCorner and InnerCorner banks -> each remains on land",
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
            self.bridge.walk_to(
                *SHALLOW_START, tol=self.WALK_TOL, timeout_s=25.0
            )
            response = self.bridge.walk_to(
                *SHALLOW_END, tol=self.WALK_TOL, timeout_s=30.0
            )
            location = _player(self.bridge)
            ok = math.dist(location[:2], SHALLOW_END) <= self.WALK_TOL + 5.0
            detail = (
                f"far waypoint response {response.get('final')}; final "
                f"({location[0]:.1f}, {location[1]:.1f}, {location[2]:.1f})"
            )
            return ("PASS" if ok else "FAIL"), detail, {"location": location}

        crossed, _ = self.attempt("shallow_pond_crosses", cross)
        self.shot(
            "shallow_crossing",
            needs=crossed,
            needs_why="not captured: the shallow lane was not crossed",
        )

    def step_deep(self):
        results = []

        def probe_all():
            for label, start in DEEP_BANKS:
                self.bridge.walk_to(
                    *start, tol=self.WALK_TOL, timeout_s=25.0
                )
                response = self.bridge.hold("S", BLOCK_HOLD_S)
                location = _player(self.bridge)
                blocked = location[0] >= DEEP_MIN_X
                results.append({
                    "representative": label,
                    "held_s": response.get("held_s"),
                    "location": location,
                    "blocked": blocked,
                })
                self.shot(f"deep_{label.replace('_corner', '')}_rejection")
            bad = [result for result in results if not result["blocked"]]
            detail = "; ".join(
                f"{result['representative']} X {result['location'][0]:.1f} "
                f"({'blocked' if result['blocked'] else 'CROSSED'})"
                for result in results
            )
            return ("FAIL" if bad else "PASS"), (
                detail + f"; every X must remain >= {DEEP_MIN_X:.0f}"
            ), {"representatives": results}

        self.attempt("deep_bank_blocks", probe_all)


Run = Us120Run


def main(argv=None):
    return kit.main(Us120Run, argv)


if __name__ == "__main__":
    raise SystemExit(main())
