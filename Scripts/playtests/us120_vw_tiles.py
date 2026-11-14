#!/usr/bin/env python3
"""US-120 Verdant Wilds tile fixture -- Tier 4 real-input traversal checks."""

import math
import json
import os
import pathlib
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

MANIFEST_PATH = pathlib.Path(SCRIPTS_DIR) / "us120_vw_source_manifest.json"
IMPORT_REPORT_PATH = (
    pathlib.Path(SCRIPTS_DIR).parent
    / "Saved"
    / "Playtests"
    / "us120-vw-import-report.json"
)
MANIFEST = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
FIXTURE = MANIFEST["test_fixture"]
LANE_BY_LABEL = {actor["label"]: actor for actor in FIXTURE["lanes"]}
DEEP_FIXTURE_BY_LABEL = {
    lane["label"]: lane for lane in FIXTURE["deep_lanes"]
}

SPAWN = (-1200.0, 0.0)
TIER0_Z = 92.0
TIER1_Z = 292.0
Z_TOL = 55.0
BLOCK_HOLD_S = 1.5
DEEP_PUSHES = 3
PROBE_START_TOL = 25.0
PROBE_SETTLE_S = 0.5
JUMP_SAMPLE_S = 2.0
GALLERY_VIEWPOINTS = (
    (-1200.0, 2000.0),
    (-400.0, 2000.0),
    (400.0, 2000.0),
    (1200.0, 2000.0),
)
RAMPS = {
    "2t": {
        "low": (-500.0, LANE_BY_LABEL["VW_Lane_Ramp2T"]["location_cm"][1]),
        "top": (250.0, LANE_BY_LABEL["VW_Lane_Ramp2T"]["location_cm"][1]),
    },
    "3t": {
        "low": (-700.0, LANE_BY_LABEL["VW_Lane_Ramp3T"]["location_cm"][1]),
        "top": (250.0, LANE_BY_LABEL["VW_Lane_Ramp3T"]["location_cm"][1]),
    },
}
CLIFF_START = (1100.0, 0.0)
CLIFF_FACE_X = LANE_BY_LABEL["VW_Lane_Cliff"]["location_cm"][0]
CLIFF_MIN_X = 1330.0
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
        "mesh": LANE_BY_LABEL["VW_Lane_DeepEdge"]["mesh"],
        "yaw_deg": LANE_BY_LABEL["VW_Lane_DeepEdge"]["yaw_deg"],
        "pivot": tuple(LANE_BY_LABEL["VW_Lane_DeepEdge"]["location_cm"][:2]),
        "start": (4070.0, -1000.0),
        "keys": ("S",),
        "direction": (-1.0, 0.0),
        "face_progress": 0.0,
        "minimum_contact_progress": None,
        "water_regions_xy_cm": DEEP_FIXTURE_BY_LABEL["edge"]["water_regions_xy_cm"],
        "route": tuple(
            tuple(point)
            for point in DEEP_FIXTURE_BY_LABEL["edge"]["clear_route_xy_cm"]
        ) + ((4070.0, -1000.0),),
    },
    {
        "label": "outer_corner",
        "mesh": LANE_BY_LABEL["VW_Lane_DeepOuterCorner"]["mesh"],
        "yaw_deg": LANE_BY_LABEL["VW_Lane_DeepOuterCorner"]["yaw_deg"],
        "pivot": tuple(LANE_BY_LABEL["VW_Lane_DeepOuterCorner"]["location_cm"][:2]),
        "start": (3970.0, -170.0),
        "keys": ("S", "D"),
        "direction": (-DIAGONAL, DIAGONAL),
        "face_progress": 0.0,
        "minimum_contact_progress": None,
        "water_regions_xy_cm": DEEP_FIXTURE_BY_LABEL["outer_corner"][
            "water_regions_xy_cm"
        ],
        "route": tuple(
            tuple(point)
            for point in DEEP_FIXTURE_BY_LABEL["outer_corner"]["clear_route_xy_cm"]
        ) + ((3970.0, -170.0),),
    },
    {
        "label": "inner_corner",
        "mesh": LANE_BY_LABEL["VW_Lane_DeepInnerCorner"]["mesh"],
        "yaw_deg": LANE_BY_LABEL["VW_Lane_DeepInnerCorner"]["yaw_deg"],
        "pivot": tuple(LANE_BY_LABEL["VW_Lane_DeepInnerCorner"]["location_cm"][:2]),
        "start": (3970.0, 830.0),
        "keys": ("S", "D"),
        "direction": (-DIAGONAL, DIAGONAL),
        "face_progress": 0.0,
        "minimum_contact_progress": None,
        "water_regions_xy_cm": DEEP_FIXTURE_BY_LABEL["inner_corner"][
            "water_regions_xy_cm"
        ],
        "derive_from_hulls": True,
        "route": tuple(
            tuple(point)
            for point in DEEP_FIXTURE_BY_LABEL["inner_corner"][
                "clear_route_xy_cm"
            ]
        ) + ((3970.0, 830.0),),
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
        proves=(),
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


def _blocking_hulls(measurement):
    shallow_max = MANIFEST["pond_collision_contracts"]["shallow_max_z_cm"]
    return [
        hull
        for hull in measurement["collision_hulls"]
        if hull["max"][2] > shallow_max
    ]


def _movement_keys(direction):
    keys = []
    if direction[0] > 0.25:
        keys.append("W")
    elif direction[0] < -0.25:
        keys.append("S")
    if direction[1] > 0.25:
        keys.append("D")
    elif direction[1] < -0.25:
        keys.append("A")
    return tuple(keys)


def derive_inner_corner_probe(lane, fixture, measurement):
    """Derive the radial probe from measured UE-space wall hulls and actor yaw."""
    hulls = _blocking_hulls(measurement)
    if not hulls:
        raise ab.BridgeError(
            f"{lane['mesh']} has no measured blocking-wall hulls",
            kind="infrastructure",
        )
    centres = [
        (
            (hull["min"][0] + hull["max"][0]) * 0.5,
            (hull["min"][1] + hull["max"][1]) * 0.5,
        )
        for hull in hulls
    ]
    local_x, local_y = centres[len(centres) // 2]
    local_length = math.hypot(local_x, local_y)
    if local_length <= 1.0:
        raise ab.BridgeError(
            f"{lane['mesh']} wall centroid does not define an arc radius",
            kind="infrastructure",
        )
    local_direction = (local_x / local_length, local_y / local_length)
    wall_radial_max = max(
        x * local_direction[0] + y * local_direction[1]
        for hull in hulls
        for x in (hull["min"][0], hull["max"][0])
        for y in (hull["min"][1], hull["max"][1])
    )
    radians = math.radians(float(lane["yaw_deg"]))
    cosine = math.cos(radians)
    sine = math.sin(radians)
    direction = (
        cosine * local_direction[0] - sine * local_direction[1],
        sine * local_direction[0] + cosine * local_direction[1],
    )
    pivot = tuple(float(value) for value in lane["location_cm"][:2])
    start_distance = wall_radial_max + CAPSULE_RADIUS_CM + 30.0
    start = (
        pivot[0] - direction[0] * start_distance,
        pivot[1] - direction[1] * start_distance,
    )
    route = tuple(
        tuple(float(value) for value in point)
        for point in fixture["clear_route_xy_cm"]
    ) + (start,)
    return {
        "start": start,
        "route": route,
        "direction": direction,
        "keys": _movement_keys(direction),
        "axis_values": {
            "Gamepad_LeftY": direction[0],
            "Gamepad_LeftX": direction[1],
        },
        "water_regions_xy_cm": fixture["water_regions_xy_cm"],
        "wall_radial_max_cm": wall_radial_max,
        "wall_hulls_local_cm": hulls,
        "derivation": (
            "centre of the middle measured UE-space blocking-wall hull, "
            f"rotated by lane yaw {float(lane['yaw_deg']):.1f} degrees"
        ),
    }


def _runtime_deep_banks():
    banks = []
    report = None
    for original in DEEP_BANKS:
        spec = dict(original)
        if spec.get("derive_from_hulls"):
            if report is None:
                report = _import_report()
            lane = LANE_BY_LABEL["VW_Lane_DeepInnerCorner"]
            fixture = DEEP_FIXTURE_BY_LABEL["inner_corner"]
            measurement = report["mesh_measurements"][spec["mesh"]]
            spec.update(derive_inner_corner_probe(lane, fixture, measurement))
        banks.append(spec)
    return tuple(banks)


def _inside_water_side(location, spec):
    x, y = location[:2]
    return any(
        region[0] < x < region[1] and region[2] < y < region[3]
        for region in spec.get("water_regions_xy_cm", ())
    )


def classify_deep_probe(
    *,
    max_progress,
    minimum_contact,
    max_allowed,
    minimum_z,
    entered_basin,
):
    near_land = minimum_z >= TIER0_Z - 20.0
    if minimum_contact <= max_progress <= max_allowed and near_land:
        return "BLOCKED"
    crossed = max_progress > max_allowed
    dropped = minimum_z < TIER0_Z - 60.0
    if crossed and (dropped or entered_basin):
        return "COLLISION DEFECT"
    return "FIXTURE ERROR"


def _import_report():
    if not IMPORT_REPORT_PATH.is_file():
        raise ab.BridgeError(
            f"US-120 import report is missing: {IMPORT_REPORT_PATH}",
            kind="infrastructure",
        )
    report = json.loads(IMPORT_REPORT_PATH.read_text(encoding="utf-8"))
    if report.get("result") != "PASS":
        raise ab.BridgeError("US-120 import report is not PASS", kind="infrastructure")
    return report


def _projected_hull_minimum(mesh_id, yaw_deg, direction, blocking_only):
    measurement = _import_report()["mesh_measurements"][mesh_id]
    hulls = measurement["collision_hulls"]
    if blocking_only:
        hulls = [
            hull for hull in hulls
            if hull["max"][2] > MANIFEST["pond_collision_contracts"]["shallow_max_z_cm"]
        ]
    if not hulls:
        raise ab.BridgeError(
            f"{mesh_id} has no measured contact hulls", kind="infrastructure"
        )
    radians = math.radians(yaw_deg)
    cosine = math.cos(radians)
    sine = math.sin(radians)
    projections = []
    for hull in hulls:
        for x in (hull["min"][0], hull["max"][0]):
            for y in (hull["min"][1], hull["max"][1]):
                world_x = cosine * x - sine * y
                world_y = sine * x + cosine * y
                projections.append(
                    world_x * direction[0] + world_y * direction[1]
                )
    return min(projections)


def measured_contact_minimum(spec):
    explicit = spec.get("minimum_contact_progress")
    if explicit is not None:
        return float(explicit)
    hull_minimum = _projected_hull_minimum(
        spec["mesh"], spec["yaw_deg"], spec["direction"], blocking_only=True
    )
    return hull_minimum - CAPSULE_RADIUS_CM - BANK_TOLERANCE_CM


def measured_cliff_minimum_x():
    actor = LANE_BY_LABEL["VW_Lane_Cliff"]
    hull_minimum = _projected_hull_minimum(
        actor["mesh"], actor["yaw_deg"], (1.0, 0.0), blocking_only=False
    )
    return (
        actor["location_cm"][0]
        + hull_minimum
        - CAPSULE_RADIUS_CM
        - BANK_TOLERANCE_CM
    )


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
            self.bridge.walk_to(*CLIFF_START, tol=PROBE_START_TOL, timeout_s=20.0)
            response = self.bridge.hold("W", BLOCK_HOLD_S)
            location = _player(self.bridge)
            minimum_x = measured_cliff_minimum_x()
            ok = (
                minimum_x <= location[0] <= CLIFF_MAX_X
                and location[2] < TIER1_Z - Z_TOL
            )
            detail = (
                f"held W {response.get('held_s')}s: location "
                f"({location[0]:.1f}, {location[1]:.1f}, {location[2]:.1f}); "
                f"need measured contact X {minimum_x:.1f}..{CLIFF_MAX_X:.0f} "
                "and below Tier 1"
            )
            return ("PASS" if ok else "FAIL"), detail, {
                "location": location,
                "minimum_contact_x": minimum_x,
            }

        walked, _ = self.attempt("cliff_walk_blocked", walk_blocked)

        def jump_blocked():
            bridge = self.bridge
            bridge.walk_to(*CLIFF_START, tol=PROBE_START_TOL, timeout_s=20.0)
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
            minimum_x = measured_cliff_minimum_x()
            ok = (
                minimum_x <= max_x <= CLIFF_MAX_X
                and TIER0_Z + 50.0 <= max_z < TIER1_Z - Z_TOL
            )
            detail = (
                f"real W+Space samples: max X {max_x:.1f} "
                f"(need contact {minimum_x:.1f}..{CLIFF_MAX_X:.0f}), "
                f"max Z {max_z:.1f} (need {TIER0_Z + 50.0:.0f}.."
                f"{TIER1_Z - Z_TOL:.0f})"
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
            banks = _runtime_deep_banks()
            probe_log = {
                "coordinate_space": "Unreal world centimetres; north=+X, east=+Y",
                "classification_contract": {
                    "blocked": "progress <= face + 55 cm and Z remains near land",
                    "collision_defect": (
                        "crosses face + 55 cm and either drops below land - 60 cm "
                        "or a sampled point enters the declared water-side basin"
                    ),
                    "fixture_error": "any other crossing or approach failure",
                },
                "probes": [],
            }
            self._walk_sequence(DEEP_ENTRY_ROUTE, timeout_s=25.0)
            for spec_index, spec in enumerate(banks):
                path_record = {
                    "label": spec["label"],
                    "mesh": spec.get("mesh", "offline-fake"),
                    "pivot": spec["pivot"],
                    "yaw_deg": spec.get("yaw_deg"),
                    "planned_route": spec["route"],
                    "start": spec["start"],
                    "direction": spec["direction"],
                    "keys": spec["keys"],
                    "axis_values": spec.get("axis_values"),
                    "face_progress": spec["face_progress"],
                    "water_regions_xy_cm": spec.get("water_regions_xy_cm", []),
                    "derivation": spec.get("derivation", "shared fixed lane contract"),
                    "wall_radial_max_cm": spec.get("wall_radial_max_cm"),
                    "wall_hulls_local_cm": spec.get("wall_hulls_local_cm", []),
                    "path_points": [],
                    "reapproaches": [],
                    "settle_s": PROBE_SETTLE_S,
                }
                probe_log["probes"].append(path_record)
                try:
                    self._walk_sequence(spec["route"][:-1], timeout_s=25.0)
                    self.bridge.walk_to(
                        *spec["start"], tol=PROBE_START_TOL, timeout_s=25.0
                    )
                    self.bridge.wait(PROBE_SETTLE_S)
                except ab.BridgeError as exc:
                    results.append({
                        "representative": spec["label"],
                        "classification": "FIXTURE ERROR",
                        "detail": f"approach to derived probe start failed: {exc}",
                    })
                    path_record["classification"] = "FIXTURE ERROR"
                    path_record["approach_error"] = str(exc)
                    break
                max_progress = -math.inf
                min_z = math.inf
                location = _player(self.bridge)
                max_allowed = spec["face_progress"] + DEEP_ALLOWED_PAST_FACE
                minimum_contact = measured_contact_minimum(spec)
                pushes = []
                entered_basin = False
                crossed = False
                reapproach_error = None
                for push_index in range(DEEP_PUSHES):
                    push_max = -math.inf
                    axis_values = spec.get("axis_values")
                    if axis_values:
                        self.bridge.axis(axis_values)
                    else:
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
                            inside_basin = _inside_water_side(location, spec)
                            max_progress = max(max_progress, progress)
                            push_max = max(push_max, progress)
                            min_z = min(min_z, location[2])
                            entered_basin = entered_basin or inside_basin
                            path_record["path_points"].append({
                                "push": push_index + 1,
                                "location": list(location),
                                "progress_cm": progress,
                                "inside_water_side": inside_basin,
                            })
                            if progress > max_allowed:
                                crossed = True
                                break
                            time.sleep(0.05)
                    finally:
                        if axis_values:
                            self.bridge.axis(
                                {axis: 0.0 for axis in axis_values}
                            )
                        else:
                            for key in reversed(spec["keys"]):
                                self.bridge.release(key)
                    pushes.append(push_max)
                    self.bridge.wait(0.1)
                    if crossed:
                        break
                    if push_index + 1 < DEEP_PUSHES:
                        try:
                            response = self.bridge.walk_to(
                                *spec["start"],
                                tol=PROBE_START_TOL,
                                timeout_s=25.0,
                            )
                            path_record["reapproaches"].append({
                                "after_push": push_index + 1,
                                "target": list(spec["start"]),
                                "final": response.get("final"),
                            })
                            self.bridge.wait(PROBE_SETTLE_S)
                        except ab.BridgeError as exc:
                            reapproach_error = str(exc)
                            path_record["reapproach_error"] = reapproach_error
                            break
                classification = classify_deep_probe(
                    max_progress=max_progress,
                    minimum_contact=minimum_contact,
                    max_allowed=max_allowed,
                    minimum_z=min_z,
                    entered_basin=entered_basin,
                )
                if reapproach_error is not None:
                    classification = "FIXTURE ERROR"
                results.append({
                    "representative": spec["label"],
                    "keys": spec["keys"],
                    "axis_values": spec.get("axis_values"),
                    "location": location,
                    "max_progress": max_progress,
                    "minimum_contact": minimum_contact,
                    "max_allowed": max_allowed,
                    "minimum_z": min_z,
                    "pushes": pushes,
                    "entered_basin": entered_basin,
                    "reapproach_error": reapproach_error,
                    "classification": classification,
                })
                path_record.update({
                    "classification": classification,
                    "max_progress_cm": max_progress,
                    "minimum_contact_cm": minimum_contact,
                    "max_allowed_cm": max_allowed,
                    "minimum_z_cm": min_z,
                    "entered_basin": entered_basin,
                })
                if classification == "BLOCKED":
                    self.shot(
                        f"deep_{spec['label'].replace('_corner', '')}_rejection"
                    )
                if classification != "BLOCKED":
                    break
                if spec_index + 1 < len(banks):
                    self.bridge.walk_to(
                        DEEP_SAFE_X,
                        spec["start"][1],
                        tol=self.WALK_TOL,
                        timeout_s=25.0,
                    )
            path_file = pathlib.Path(self.run_dir) / "deep_probe_paths.json"
            path_file.write_text(
                json.dumps(probe_log, indent=2) + "\n",
                encoding="utf-8",
            )
            bad = [
                result for result in results
                if result["classification"] != "BLOCKED"
            ]
            details = []
            for result in results:
                if "detail" in result:
                    details.append(
                        f"{result['representative']} {result['classification']}: "
                        f"{result['detail']}"
                    )
                    continue
                details.append(
                    f"{result['representative']} progress/Z "
                    f"{result['max_progress']:.1f}/{result['minimum_z']:.1f} "
                    f"(contact {result['minimum_contact']:.1f}.."
                    f"{result['max_allowed']:.1f}; {len(result['pushes'])} pushes; "
                    f"inside_basin={result['entered_basin']}) "
                    f"[{result['classification']}]"
                )
            detail = "; ".join(details)
            return ("FAIL" if bad else "PASS"), (
                detail + "; each probe must remain within face + "
                f"{DEEP_ALLOWED_PAST_FACE:.0f} cm and near land Z; "
                f"path log {path_file}"
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
