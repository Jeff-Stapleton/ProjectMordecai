#!/usr/bin/env python3
"""US-001 Movement Playtest -- Tier 4: twin-stick movement, the fixed camera and sprint/jump,
played with REAL input

Drives the PACKAGED game through the in-game agent bridge (Scripts/agent_bridge.py).  Every
action is a key, axis or mouse event injected into Slate, the path a player's keyboard, mouse
and gamepad take; nothing calls game functions, and bridge.jsonl in the run dir is the audit
trail of every request.  The run framework (launch, STALE gate, checks, plan enforcement, AC
acceptance, reports, exit codes) is Scripts/playtest_kit.py (Specs/story_playtests_v1.md
section 2); this file is the PLAN and the steps.

Steps (every step is a check: PASS, FAIL, SKIP or WARN; --plan prints them as a table):
  1. Boot DevTestMap: the combat HUD with its Health / Stamina bars and the SP text '10 / 10'
     is on screen; the player is a live MordecaiCharacter at the spawn (0, 0); the runtime
     input mapping context is active.  Screenshot.
  2. WASD from the spawn, A -> W -> D -> S (a square that ends back at the spawn and keeps
     more than 120 cm from every pickup): each key held 0.4 s moves the player > 100 cm the
     right way along its world axis (W=+X, S=-X, D=+Y, A=-Y) and < 30 cm along the other.
  3. The camera: control_rotation stays [-50, 0, 0] after the moves, and while the right
     stick is held and the mouse moves around the screen.
  4. Walk to the clear lane L1 (y=-800) via (0,-650) and (-700,-650).  Screenshot.  Hold W
     1.0 s: the walk covers 400..700 cm and the player stops within 0.75 s of the release.
  5. Sprint: tap LeftShift (then, from the lane start again, the L3 button) and hold W 1.0 s:
     > 700 cm.  Each toggle is tapped again afterwards so a working sprint does not leak into
     later steps; after the LeftShift one, S held 1.0 s must reach only walking speed (the
     second tap ended the sprint; a sprint left on SKIPs the L3 step).  L3 is expected to FAIL
     today (invalid FKey name, US-097).
  6. Jump: tap SpaceBar -> player.movement_mode=="Falling" within 1 s.
  7. Facing: the mouse at (1100, 360) (right of the player: +Y) turns the player to yaw ~90;
     the right stick held left (Gamepad_RightX=-1: -Y) turns it to yaw ~-90.  Screenshot.
     Expected FAIL today (facing is pinned to +X; the stick names are invalid).
  8. Gamepad move: Gamepad_LeftY=1 for 0.5 s moves the player > 100 cm along +X.  Expected
     FAIL today (invalid stick names).
A failed check SKIPs the checks that depend on it; independent checks still run.  A lost bridge
connection SKIPs everything after it.  Each check names the US-001 ACs it proves (PLAN below);
report.json["acs"] says which were accepted.

Usage:
  py Scripts/playtests/us001_movement.py                    fresh package: launch, play, quit
  py Scripts/playtests/us001_movement.py --allow-stale      STALE package: diagnostic only
  py Scripts/playtests/us001_movement.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us001_movement.py --plan             print the Steps table; no game
  py Scripts/playtests/us001_movement.py --attach-port N [--token T]
        play a game that is already running; always diagnostic only, never acceptance evidence
Evidence: report.md's Evidence line reads ACCEPTANCE CANDIDATE only for a launched, fresh,
clean-tree package without --allow-stale that exited 0; every other run is DIAGNOSTIC ONLY.

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us001/ with report.md, report.json, game.log,
bridge.jsonl and bridge_NN.png.  Open every screenshot with the Read tool and state what is on
screen.

Exit codes:
  0 -- every check passed (WARN allowed)
  1 -- a gameplay check FAILED (today: the known L3 sprint / facing / gamepad failures)
  2 -- infrastructure problem (STALE BUILD, launch failure, no connection, crash)
"""

import math
import os
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "US-001"
TITLE = "US-001 movement and camera playtest (Tier 4, real input)"
RUN_SUFFIX = "us001"

# --- what the game should show ------------------------------------------------------------
SP_TEXT = "10 / 10"                         # the SP bar text at full SP (SP=10)
HUD_CLASS_PARTS = ("combathud", "healthbar", "staminabar")   # widget class substrings
RUNTIME_IMC = "MordecaiIMC_Runtime"
PAWN_NATIVE_CLASS = "MordecaiCharacter"
SPAWN = (0.0, 0.0)
SPAWN_TOL = 100.0
CAMERA_PITCH = -50.0
CAMERA_TOL = 0.5                            # degrees

# --- movement ------------------------------------------------------------------------------
# A -> W -> D -> S: a square from the spawn that returns to it.  0.4 s at 600 cm/s is ~240 cm,
# so the corners are (0,-240), (240,-240), (240,0): every one more than 120 cm (the pickup
# focus range) from every pickup and clear of every obstacle.
MOVE_HOLD_S = 0.4
MOVE_MIN = 100.0                            # cm along the key's axis
CROSS_MAX = 30.0                            # cm along the other axis
MOVES = (  # (check, key, axis index 0=X 1=Y, sign, world direction)
    ("move_a", "A", 1, -1, "-Y"),
    ("move_w", "W", 0, +1, "+X"),
    ("move_d", "D", 1, +1, "+Y"),
    ("move_s", "S", 0, -1, "-X"),
)
SETTLE_TIMEOUT_S = 2.0                      # wait for the player to stop before measuring

# Lane L1 (y=-800, x -750..+950 is clear).  The waypoints go around Box_02 and the sword
# pickup (-300,-300) and keep 100 cm off Cover_01 (x -1000..-800, y -715..-685).
LANE_WAYPOINTS = (("walk_lane_wp1", 0.0, -650.0), ("walk_lane_wp2", -700.0, -650.0))
LANE_START = (-700.0, -800.0)
BASELINE_HOLD_S = 1.0
BASELINE_RANGE = (400.0, 700.0)             # cm in 1.0 s at WalkSpeed 600 with accel/decel
STOP_TIMEOUT_S = 0.75                       # responsive deceleration after the release
SPRINT_HOLD_S = 1.0
SPRINT_MIN = 700.0                          # more than any walk can cover in 1.0 s
SPRINT_SPEED_MIN = 750.0                    # cm/s: above a walk (600), below a sprint (900)
WALK_SPEED_MIN = 500.0                      # cm/s: a free walk reaches 600
SPEED_POLL_S = 0.05
SPRINT_KEY = "LeftShift"
SPRINT_PAD_KEY = "Gamepad_LeftThumbstick"   # L3
JUMP_KEY = "SpaceBar"
JUMP_TIMEOUT_S = 1.0
FACING_MOUSE = (1100.0, 360.0)              # right of the player (screen right = +Y): yaw 90
FACING_MOUSE_YAW = 90.0
FACING_STICK = {"Gamepad_RightX": -1.0}     # stick left = -Y: yaw -90
FACING_STICK_YAW = -90.0
FACING_TOL = 30.0
FACING_TIMEOUT_S = 1.0
CAMERA_PROBE_STICK = {"Gamepad_RightX": 1.0, "Gamepad_RightY": 1.0}
CAMERA_PROBE_MOUSE = ((200.0, 100.0), (1100.0, 620.0), (640.0, 360.0))
CAMERA_PROBE_SETTLE_S = 0.15
GAMEPAD_MOVE = {"Gamepad_LeftY": 1.0}       # stick up = screen up = +X
GAMEPAD_HOLD_S = 0.5
GAMEPAD_MIN = 100.0
GAMEPAD_TIMEOUT_S = 1.0

KI_FACING = ("Known issue: facing and aim are pinned to +X by the diorama camera "
             "(Specs/agent_playtest_plan_v1.md)")
KI_STICKS = ("Known issue: gamepad stick and L3 mappings use invalid FKey names "
             "(Specs/agent_playtest_plan_v1.md)")
KNOWN_ISSUES = (KI_FACING, KI_STICKS)

SCREENSHOTS = {
    "start": "the Mordecai diorama arena (fixed overhead camera, pitch -50, looking +X), the "
             "player at the centre, the combat HUD (Health / Stamina / Posture / SP bars "
             "top-left with '10 / 10', 'Kills: 0' top-right)",
    "after_moves": "the same camera angle (no rotation) with the player at the lane start "
                   "(-700, -800), near the south-west corner: the camera followed, so the "
                   "player is still near the screen centre and the arena has shifted",
    "facing_arrow": "the player after the mouse and right-stick facing attempts: is a facing "
                    "arrow visible on the player, and which way does the player face? (the "
                    "UArrowComponent is probably hidden in game; facing is pinned to +X today)",
}

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA
set_log_stream = kit.set_log_stream
lit = kit.lit
_dumps = kit.dumps
_section = kit.section


def _yaw_cond(target, tol):
    return f"player.rotation[1]>{target - tol:g} && player.rotation[1]<{target + tol:g}"


CAMERA_COND = (f"player.control_rotation[0]>{CAMERA_PITCH - CAMERA_TOL:g} && "
               f"player.control_rotation[0]<{CAMERA_PITCH + CAMERA_TOL:g} && "
               f"player.control_rotation[1]>{-CAMERA_TOL:g} && "
               f"player.control_rotation[1]<{CAMERA_TOL:g}")

PLAN = (
    Step("hud_visible", f"state widgets -> a CombatHUD, a HealthBar and a StaminaBar widget on "
         f"screen and the text {lit(SP_TEXT)} (SP bar)", proves=("AC-2.2.5",)),
    Step("spawn_state", f"state player, actors -> player.valid && !player.is_dead, within "
         f"{SPAWN_TOL:g} cm of ({SPAWN[0]:g}, {SPAWN[1]:g}), pawn native class "
         f"{PAWN_NATIVE_CLASS}", proves=("AC-2.3.3",)),
    Step("imc_active", f"state input -> {lit(RUNTIME_IMC)} in input.mapping_contexts",
         proves=("AC-1.3.2",)),
    *[Step(check, f"hold {key} {MOVE_HOLD_S:g}s -> player.location moves > {MOVE_MIN:g} cm "
           f"along {d}, < {CROSS_MAX:g} cm across", proves=("AC-2.1.1", "AC-1.3.2"))
      for check, key, _, _, d in MOVES],
    Step("camera_fixed", f"wait_until player.control_rotation == [{CAMERA_PITCH:g}, 0, 0] "
         f"(+-{CAMERA_TOL:g}) after the moves", proves=("AC-1.2.2",)),
    Step("no_camera_rotation_input", "axis Gamepad_RightX=1, Gamepad_RightY=1 held; mouse to "
         "(200,100), (1100,620), (640,360) -> player.control_rotation stays "
         f"[{CAMERA_PITCH:g}, 0, 0] at every sample", proves=("AC-1.2.4", "AC-1.2.2")),
    *[Step(check, f"walk_to({x:g}, {y:g}) -> within 60 cm (waypoint to lane L1)")
      for check, x, y in LANE_WAYPOINTS],
    Step("walk_lane_start", f"walk_to({LANE_START[0]:g}, {LANE_START[1]:g}) -> within 60 cm "
         "(lane L1 start)"),
    Step("walk_speed_baseline", f"hold W {BASELINE_HOLD_S:g}s -> dx in "
         f"[{BASELINE_RANGE[0]:g}, {BASELINE_RANGE[1]:g}] cm and wait_until player.speed<1 "
         f"within {STOP_TIMEOUT_S:g}s of the release", proves=("AC-2.1.6",)),
    Step("walk_lane_reset", f"walk_to({LANE_START[0]:g}, {LANE_START[1]:g}) -> within 60 cm"),
    Step("sprint_shift", f"tap {SPRINT_KEY}, hold W {SPRINT_HOLD_S:g}s -> dx > {SPRINT_MIN:g} "
         f"cm; tap {SPRINT_KEY} again, hold S {SPRINT_HOLD_S:g}s -> back to a walk (peak "
         f"player.speed {WALK_SPEED_MIN:g}..{SPRINT_SPEED_MIN:g} cm/s)",
         proves=("AC-1.3.3", "AC-2.1.3")),
    Step("walk_lane_reset2", f"walk_to({LANE_START[0]:g}, {LANE_START[1]:g}) -> within 60 cm"),
    Step("sprint_l3", f"tap {SPRINT_PAD_KEY} (L3), hold W {SPRINT_HOLD_S:g}s -> dx > "
         f"{SPRINT_MIN:g} cm (then tap L3 again)", proves=("AC-1.3.3", "AC-2.1.3"),
         known_issue=KI_STICKS),
    Step("jump_space", f"tap {JUMP_KEY} -> wait_until player.movement_mode==\"Falling\" "
         f"within {JUMP_TIMEOUT_S:g}s", proves=("AC-2.1.4",)),
    Step("facing_mouse", f"mouse {FACING_MOUSE[0]:g} {FACING_MOUSE[1]:g} (right of the "
         f"player, +Y) -> wait_until player.rotation yaw {FACING_MOUSE_YAW:g} "
         f"(+-{FACING_TOL:g}) within {FACING_TIMEOUT_S:g}s", proves=("AC-2.1.2",),
         known_issue=KI_FACING),
    Step("facing_stick", f"axis Gamepad_RightX=-1 held (left, -Y) -> wait_until "
         f"player.rotation yaw {FACING_STICK_YAW:g} (+-{FACING_TOL:g}) within "
         f"{FACING_TIMEOUT_S:g}s, then release", proves=("AC-2.1.2",),
         known_issue=f"{KI_FACING}; {KI_STICKS}"),
    Step("gamepad_move", f"axis Gamepad_LeftY=1 for {GAMEPAD_HOLD_S:g}s -> player.location "
         f"x rises > {GAMEPAD_MIN:g} cm", proves=("AC-2.1.1", "AC-1.3.2"),
         known_issue=KI_STICKS),
)

PENDING = {
    "AC-1.2.1": "bridge cannot drive: state has no camera transform (screenshot after_moves "
                "shows whether the camera followed)",
    "AC-2.1.5": "bridge cannot drive: state does not report the pawn's capsule or arrow "
                "component visibility (screenshot facing_arrow)",
}


class Us001Run(kit.PlaytestRun):
    """One run of the playtest.  execute() returns the exit code."""

    STORY = STORY
    FEATURES = ("movement", "camera", "sprint")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.DEFAULT_MAP
    EXPERIENCE = kit.DEFAULT_EXPERIENCE
    SCREENSHOTS = SCREENSHOTS
    KNOWN_ISSUES = KNOWN_ISSUES
    PLAN = PLAN
    PENDING = PENDING
    SCRIPT = os.path.abspath(__file__)

    def __init__(self, args):
        super().__init__(args)
        self.measurements = []   # one entry per measured movement (report_extra / md_sections)

    def steps(self):
        self.step_boot()
        self.step_moves()
        self.step_camera()
        lane = self.step_lane()
        self.step_sprint(lane)
        self.step_jump()
        self.step_facing()
        self.step_gamepad_move()

    # --- reading the player ----------------------------------------------------------------
    @staticmethod
    def _player(b):
        p = _section(b.state(["player"]), "player")
        loc = p.get("location")
        if p.get("valid") is not True or not (isinstance(loc, list) and len(loc) >= 2):
            return None, p
        return (float(loc[0]), float(loc[1])), p

    @staticmethod
    def _settle(b, timeout_s):
        """wait_until the player stops.  Returns the seconds it took, or None on a timeout."""
        try:
            r = b.wait_until("player.speed<1", timeout_s=timeout_s)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            return None
        return float(r.get("elapsed_s") or 0.0)

    def _record(self, check, what, p0, p1, extra=None):
        rec = {"check": check, "input": what, "start": list(p0) if p0 else None,
               "end": list(p1) if p1 else None,
               "dx": round(p1[0] - p0[0], 1) if p0 and p1 else None,
               "dy": round(p1[1] - p0[1], 1) if p0 and p1 else None}
        rec.update(extra or {})
        self.measurements.append(rec)
        return rec

    def _hold_and_measure(self, key, seconds, before=None, after=None):
        """Read the position, [before()], hold `key` for `seconds`, wait for the player to stop,
        read again, [after()].  Returns ((p0, p1, held_s, stop_s), None) or (None, why)."""
        b = self.bridge
        p0, raw = self._player(b)
        if p0 is None:
            return None, f"player section invalid before the input ({raw.get('reason')})"
        if before:
            before()
        r = b.hold(key, seconds)
        stop_s = self._settle(b, SETTLE_TIMEOUT_S)
        p1, raw = self._player(b)
        if after:
            after()
        if p1 is None:
            return None, f"player section invalid after the input ({raw.get('reason')})"
        return (p0, p1, r.get("held_s"), stop_s), None

    # --- step 1: boot --------------------------------------------------------------------
    def step_boot(self):
        def hud_ok(st):
            widgets = st.get("widgets") if isinstance(st.get("widgets"), list) else []
            classes = " ".join(str(w.get("class")) for w in widgets if isinstance(w, dict)).lower()
            texts = [str(t) for w in widgets if isinstance(w, dict)
                     for t in (w.get("texts") or [])]
            return all(part in classes for part in HUD_CLASS_PARTS) and SP_TEXT in texts

        def hud_text(st):
            widgets = st.get("widgets") if isinstance(st.get("widgets"), list) else []
            classes = [str(w.get("class")) for w in widgets if isinstance(w, dict)]
            low = " ".join(classes).lower()
            missing = [p for p in HUD_CLASS_PARTS if p not in low]
            texts = [str(t) for w in widgets if isinstance(w, dict)
                     for t in (w.get("texts") or [])]
            sp = "present" if SP_TEXT in texts else "MISSING"
            return (f"widget classes {classes}; missing class parts {missing or 'none'}; "
                    f"SP text {lit(SP_TEXT)} {sp} (texts: {_dumps(texts, 400)})")

        self.observe("hud_visible", ["widgets"], hud_ok, hud_text, timeout_s=5)

        def spawn_ok(st):
            p = _section(st, "player")
            loc = p.get("location") or [None, None]
            if p.get("valid") is not True or p.get("is_dead") is not False:
                return False
            if not all(isinstance(v, (int, float)) for v in loc[:2]):
                return False
            if math.hypot(loc[0] - SPAWN[0], loc[1] - SPAWN[1]) > SPAWN_TOL:
                return False
            return _pawn_native(st).lower() == PAWN_NATIVE_CLASS.lower()

        def spawn_text(st):
            p = _section(st, "player")
            return (f"player valid={p.get('valid')} is_dead={p.get('is_dead')} class="
                    f"{p.get('class')} location={p.get('location')} (spawn within "
                    f"{SPAWN_TOL:g} of {SPAWN}); pawn native class {_pawn_native(st) or None!r} "
                    f"(expected {PAWN_NATIVE_CLASS})" + (f"; reason: {p.get('reason')}"
                                                         if p.get("reason") else ""))

        self.observe("spawn_state", ["player", "actors"], spawn_ok, spawn_text, timeout_s=3,
                     radius=300)

        def imc_ok(st):
            return RUNTIME_IMC in (_section(st, "input").get("mapping_contexts") or [])

        self.observe("imc_active", ["input"], imc_ok,
                     lambda st: f"input.mapping_contexts="
                                f"{_section(st, 'input').get('mapping_contexts')}", timeout_s=3)
        self.shot("start")

    # --- step 2: WASD ----------------------------------------------------------------------
    def step_moves(self):
        for check, key, axis, sign, direction in MOVES:
            def fn(check=check, key=key, axis=axis, sign=sign, direction=direction):
                got, err = self._hold_and_measure(key, MOVE_HOLD_S)
                if got is None:
                    return "FAIL", err, None
                p0, p1, held_s, stop_s = got
                along = (p1[axis] - p0[axis]) * sign
                across = abs(p1[1 - axis] - p0[1 - axis])
                rec = self._record(check, f"hold {key} {MOVE_HOLD_S:g}s", p0, p1,
                                   {"along": round(along, 1), "across": round(across, 1),
                                    "held_s": held_s, "stop_s": stop_s})
                ok = along > MOVE_MIN and across < CROSS_MAX
                detail = (f"held {key} {held_s}s: ({p0[0]:.0f}, {p0[1]:.0f}) -> ({p1[0]:.0f}, "
                          f"{p1[1]:.0f}); {along:.0f} cm along {direction} (need > {MOVE_MIN:g}), "
                          f"{across:.0f} cm across (need < {CROSS_MAX:g})"
                          + ("" if stop_s is not None else
                             f"; still moving {SETTLE_TIMEOUT_S:g}s after the release"))
                return ("PASS" if ok else "FAIL"), detail, rec

            self.attempt(check, fn)

    # --- step 3: the camera --------------------------------------------------------------
    def step_camera(self):
        self.expect("camera_fixed", CAMERA_COND, timeout_s=1)

        def probe():
            b = self.bridge
            samples = []
            b.axis(CAMERA_PROBE_STICK)
            try:
                for x, y in CAMERA_PROBE_MOUSE:
                    b.mouse(x, y)
                    b.wait(CAMERA_PROBE_SETTLE_S)
                    p = _section(b.state(["player"]), "player")
                    samples.append({"mouse": [x, y], "control_rotation":
                                    p.get("control_rotation"), "rotation": p.get("rotation")})
            finally:
                self._release_axes(CAMERA_PROBE_STICK)
            bad = [s for s in samples if not _camera_fixed(s["control_rotation"])]
            shown = "; ".join(f"mouse ({s['mouse'][0]:g}, {s['mouse'][1]:g}): control_rotation "
                              f"{s['control_rotation']}" for s in samples)
            if bad:
                return ("FAIL", f"the camera rotated with right stick + mouse input: {shown}",
                        samples)
            return ("PASS", f"right stick held at (1, 1) and the mouse moved: control_rotation "
                    f"stayed [{CAMERA_PITCH:g}, 0, 0] at all {len(samples)} samples ({shown})",
                    samples)

        self.attempt("no_camera_rotation_input", probe)

    def _release_axes(self, values):
        try:
            self.bridge.axis({k: 0.0 for k in values})
        except ab.BridgeError as e:
            self.warnings.append(f"could not release {list(values)}: {e}")
            self._lost("axis release", e)

    # --- step 4: lane L1 and the walk-speed baseline ----------------------------------------
    def step_lane(self):
        ok = True
        for check, x, y in LANE_WAYPOINTS:
            ok, _ = self.walk(check, x, y, needs=ok, needs_why="not run: the previous waypoint "
                              "was not reached")
        lane, _ = self.walk("walk_lane_start", *LANE_START, needs=ok,
                            needs_why="not run: a lane waypoint was not reached")
        self.shot("after_moves")
        why = "not run: the player is not at the lane start"

        def baseline():
            b = self.bridge
            p0, raw = self._player(b)
            if p0 is None:
                return "FAIL", f"player section invalid ({raw.get('reason')})", None
            r = b.hold("W", BASELINE_HOLD_S)
            stop_s = self._settle(b, STOP_TIMEOUT_S)
            if stop_s is None:
                stop_s = self._settle(b, SETTLE_TIMEOUT_S)
                stopped_late = True
            else:
                stopped_late = False
            p1, raw = self._player(b)
            if p1 is None:
                return "FAIL", f"player section invalid after W ({raw.get('reason')})", None
            dx = p1[0] - p0[0]
            rec = self._record("walk_speed_baseline", f"hold W {BASELINE_HOLD_S:g}s", p0, p1,
                               {"held_s": r.get("held_s"), "stop_s": stop_s,
                                "stopped_late": stopped_late})
            lo, hi = BASELINE_RANGE
            problems = []
            if not lo <= dx <= hi:
                problems.append(f"dx {dx:.0f} cm is outside [{lo:g}, {hi:g}]")
            if stopped_late:
                problems.append(f"still moving {STOP_TIMEOUT_S:g}s after the release (slow "
                                "deceleration)")
            detail = (f"held W {r.get('held_s')}s: ({p0[0]:.0f}, {p0[1]:.0f}) -> ({p1[0]:.0f}, "
                      f"{p1[1]:.0f}), dx {dx:.0f} cm; stopped "
                      + (f"{stop_s:.2f}s after the release" if stop_s is not None and
                         not stopped_late else "late")
                      + (f" -- {'; '.join(problems)}" if problems else ""))
            return ("FAIL" if problems else "PASS"), detail, rec

        self.attempt("walk_speed_baseline", baseline, needs=lane, needs_why=why)
        reset, _ = self.walk("walk_lane_reset", *LANE_START)
        return reset

    # --- step 5: sprint ------------------------------------------------------------------
    def _sprint(self, check, toggle_key, label, verify_toggle_back=False):
        """Tap the toggle, hold W and measure; tap the toggle again.  With verify_toggle_back,
        then hold S as long and require a walk's distance back (the second tap ended the sprint)."""
        def fn():
            b = self.bridge
            got, err = self._hold_and_measure(
                "W", SPRINT_HOLD_S, before=lambda: b.tap(toggle_key, hold_s=self.TAP_S),
                after=lambda: b.tap(toggle_key, hold_s=self.TAP_S))  # toggle back
            if got is None:
                return "FAIL", err, None
            p0, p1, held_s, _ = got
            dx = p1[0] - p0[0]
            rec = self._record(check, f"tap {toggle_key}, hold W {SPRINT_HOLD_S:g}s", p0, p1,
                               {"held_s": held_s})
            walk = next((m["dx"] for m in self.measurements
                         if m["check"] == "walk_speed_baseline"), None)
            vs = f" (walk baseline {walk:.0f} cm)" if walk is not None else ""
            detail = (f"tapped {label}, held W {held_s}s: dx {dx:.0f} cm (need > "
                      f"{SPRINT_MIN:g}){vs}; tapped {label} again to toggle back")
            sprinted = dx > SPRINT_MIN
            if not verify_toggle_back:
                return ("PASS" if sprinted else "FAIL"), detail, rec

            # The toggle-back is judged by speed, not distance: a speed reading cannot be shortened
            # by a frame hitch or an enemy in the way, and a sprint (900 cm/s) and a walk
            # (600 cm/s) are far apart.
            back, err = self._hold_watching_speed("S", SPRINT_HOLD_S)
            if back is None:
                return "FAIL", f"{detail}; toggle-back not measured: {err}", rec
            q0, q1, peak = back
            self._record(f"{check}_toggle_back", f"hold S {SPRINT_HOLD_S:g}s after the second "
                         f"{toggle_key} tap", q0, q1, {"peak_speed": round(peak, 1)})
            rec["sprint_left_on"] = peak > SPRINT_SPEED_MIN
            detail += (f"; then held S {SPRINT_HOLD_S:g}s: peak speed {peak:.0f} cm/s, "
                       f"{q0[0] - q1[0]:.0f} cm back (need a walk: {WALK_SPEED_MIN:g}..."
                       f"{SPRINT_SPEED_MIN:g} cm/s)")
            if rec["sprint_left_on"]:
                detail += (" -- the second tap did not end the sprint" if sprinted else
                           " -- the first tap did not start the sprint but the second one did "
                           "(toggle out of phase)")
                return "FAIL", detail, rec
            if peak < WALK_SPEED_MIN:
                detail += (" -- the player never reached walking speed on the way back (blocked? "
                           "N14), so the toggle-back is unverified")
                return "FAIL", detail, rec
            return ("PASS" if sprinted else "FAIL"), detail, rec

        return fn

    def _hold_watching_speed(self, key, seconds):
        """Press `key`, read player.speed every SPEED_POLL_S for `seconds`, release it, wait for
        the player to stop.  Returns ((p0, p1, peak_speed), None) or (None, why)."""
        b = self.bridge
        p0, raw = self._player(b)
        if p0 is None:
            return None, f"player section invalid before {key} ({raw.get('reason')})"
        peak = 0.0
        b.press(key)
        try:
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline:
                speed = _section(b.state(["player"]), "player").get("speed")
                if isinstance(speed, (int, float)):
                    peak = max(peak, float(speed))
                time.sleep(SPEED_POLL_S)
        finally:
            b.release(key)
        self._settle(b, SETTLE_TIMEOUT_S)
        p1, raw = self._player(b)
        if p1 is None:
            return None, f"player section invalid after {key} ({raw.get('reason')})"
        return (p0, p1, peak), None

    def step_sprint(self, lane):
        why = "not run: the player is not at the lane start"
        _, shift = self.attempt("sprint_shift", self._sprint("sprint_shift", SPRINT_KEY,
                                                             SPRINT_KEY, verify_toggle_back=True),
                                needs=lane, needs_why=why)
        reset, _ = self.walk("walk_lane_reset2", *LANE_START)
        # A LeftShift sprint left on would make the L3 measurement pass on its own
        leaked = bool((shift or {}).get("sprint_left_on"))
        self.attempt("sprint_l3", self._sprint("sprint_l3", SPRINT_PAD_KEY,
                                               f"{SPRINT_PAD_KEY} (L3)"),
                     needs=reset and not leaked,
                     needs_why=(why if not reset else "not run: the LeftShift sprint is still on "
                                "after sprint_shift, so an L3 sprint cannot be told apart from it"))

    # --- step 6: jump ----------------------------------------------------------------------
    def step_jump(self):
        self.press("jump_space", JUMP_KEY, then='player.movement_mode=="Falling"',
                   timeout_s=JUMP_TIMEOUT_S)

    # --- step 7: facing --------------------------------------------------------------------
    def _wait_yaw(self, target, what, resp):
        cond = _yaw_cond(target, FACING_TOL)
        try:
            r = self.bridge.wait_until(cond, timeout_s=FACING_TIMEOUT_S)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            values = (e.response or {}).get("values") or {}
            return ("FAIL", f"{what}; player.rotation yaw stayed {values.get('player.rotation[1]')}"
                    f" (expected {target:g} +-{FACING_TOL:g} within {FACING_TIMEOUT_S:g}s)", resp)
        return "PASS", f"{what}; yaw {target:g} +-{FACING_TOL:g} after {r.get('elapsed_s')}s", resp

    def step_facing(self):
        def mouse():
            x, y = FACING_MOUSE
            r = self.bridge.mouse(x, y)
            return self._wait_yaw(FACING_MOUSE_YAW, f"mouse moved to ({x:g}, {y:g})", r)

        self.attempt("facing_mouse", mouse)

        def stick():
            b = self.bridge
            r = b.axis(FACING_STICK)
            try:
                return self._wait_yaw(FACING_STICK_YAW, "Gamepad_RightX=-1 held", r)
            finally:
                self._release_axes(FACING_STICK)

        self.attempt("facing_stick", stick)
        self.shot("facing_arrow")

    # --- step 8: gamepad move ----------------------------------------------------------------
    def step_gamepad_move(self):
        def fn():
            b = self.bridge
            p0, raw = self._player(b)
            if p0 is None:
                return "FAIL", f"player section invalid ({raw.get('reason')})", None
            r = b.axis(GAMEPAD_MOVE, hold_s=GAMEPAD_HOLD_S)
            self._settle(b, GAMEPAD_TIMEOUT_S)
            p1, raw = self._player(b)
            if p1 is None:
                return "FAIL", f"player section invalid after the stick ({raw.get('reason')})", r
            dx = p1[0] - p0[0]
            rec = self._record("gamepad_move", f"axis Gamepad_LeftY=1 {GAMEPAD_HOLD_S:g}s",
                               p0, p1, {"held_s": r.get("held_s")})
            detail = (f"Gamepad_LeftY=1 for {r.get('held_s')}s: ({p0[0]:.0f}, {p0[1]:.0f}) -> "
                      f"({p1[0]:.0f}, {p1[1]:.0f}), dx {dx:.0f} cm (need > {GAMEPAD_MIN:g})")
            return ("PASS" if dx > GAMEPAD_MIN else "FAIL"), detail, rec

        self.attempt("gamepad_move", fn)

    # --- report hooks ------------------------------------------------------------------------
    def report_extra(self):
        return {"movement": self.measurements}

    def md_sections(self):
        md = kit.pb.md_cell
        lines = ["", "## Movement measurements", "",
                 "| Check | Input | Start | End | dx | dy |", "|---|---|---|---|---|---|"]
        for m in self.measurements:
            def pt(p):
                return f"({p[0]:.0f}, {p[1]:.0f})" if p else "-"
            lines.append(f"| {m['check']} | {md(m['input'])} | {pt(m['start'])} | "
                         f"{pt(m['end'])} | {m['dx']} | {m['dy']} |")
        if not self.measurements:
            lines.append("| (none) | - | - | - | - | - |")
        return lines


Run = Us001Run  # the story runner (Scripts/playtest_story.py) reads a script's Run class


def _pawn_native(st):
    """native_class of the pawn in state.actors ('' when there is none)."""
    actors = st.get("actors") if isinstance(st, dict) else None
    for a in actors if isinstance(actors, list) else []:
        if isinstance(a, dict) and a.get("kind") == "pawn":
            return str(a.get("native_class") or "")
    return ""


def _camera_fixed(rot):
    if not (isinstance(rot, list) and len(rot) >= 2
            and all(isinstance(v, (int, float)) for v in rot[:2])):
        return False
    return abs(rot[0] - CAMERA_PITCH) <= CAMERA_TOL and abs(rot[1]) <= CAMERA_TOL


def parse_args(argv=None):
    return kit.parse_args(Us001Run, argv)


def main(argv=None):
    return kit.main(Us001Run, argv)


if __name__ == "__main__":
    sys.exit(main())
