#!/usr/bin/env python3
"""US-001 Movement Playtest -- Tier 4: twin-stick movement, the fixed camera and sprint/jump,
played with REAL input

Drives the PACKAGED game through the in-game agent bridge (Scripts/agent_bridge.py).  Every
action is a key, axis or mouse event injected into Slate, the path a player's keyboard, mouse
and gamepad take; nothing calls game functions, and bridge.jsonl in the run dir is the audit
trail of every request.  The run framework (launch, STALE gate, checks, plan enforcement, AC
acceptance, reports, exit codes) is Scripts/playtest_kit.py (Specs/story_playtests_v1.md
section 2); this file is the PLAN and the steps.

The map is MovementTestMap (/MordecaiCore/Game/Maps/MovementTestMap, B_MordecaiArenaExperience):
an open 4000 x 4000 cm floor with a measuring grid, 100 cm boundary walls at +-2000, the
PlayerStart at (0, 0) facing +X, lane L1 (y=-800, x -1000..+1600: plain floor) and two steps at
+Y (x 600..1600, y 800..1200), off every route here.  No enemies and no pickups, so nothing
shoves, blocks or kills the player mid-measurement.

Steps (every step is a check: PASS, FAIL, SKIP or WARN; --plan prints them as a table):
  1. Boot MovementTestMap: the combat HUD with its Health / Stamina bars and the SP text
     '10 / 10' is on screen; the player is a live MordecaiCharacter at the spawn (0, 0); the
     runtime input mapping context is active; the green FacingArrow is registered, visible to
     every player and points along the pawn's world forward vector.  Screenshot.
  2. WASD from the spawn, A -> W -> D -> S (a square on the open floor that ends back at the
     spawn): each key held 0.4 s moves the player > 100 cm the right way along its world axis
     (W=+X, S=-X, D=+Y, A=-Y) and < 30 cm along the other.
  3. The camera: control_rotation stays [-50, 0, 0] after the moves, and while the right
     stick is held and the mouse moves around the screen.
  4. Walk to lane L1 (y=-800) via (0,-650) and (-700,-650).  Screenshot.  Hold W 1.0 s: the
     walk covers 400..700 cm and the player stops within 0.75 s of the release.
  5. Sprint: tap LeftShift (then, from the lane start again, the L3 button) and hold W 1.0 s:
     > 700 cm.  A toggle-off tap is sent only while the Sprinting tag is present; after the
     LeftShift one, S held 1.0 s must reach only walking speed.  Separately, move while
     sprinting, stop, verify the tag auto-clears about 0.25 s after speed drops below 60 cm/s,
     and verify the next Shift starts a new sprint.  A sprint left on SKIPs the L3 step.
  6. Jump: tap SpaceBar -> player.movement_mode=="Falling" within 1 s.
  7. Facing and attacks: the physical mouse at (1100, 360) (right of the player: +Y) turns the
     player to yaw ~90 while the OS cursor stays hidden and the HUD draws the one reticle.  LMB
     starts Hit 1; another LMB in its combo window chains Hit 2.  While that aim is held, W still
     moves along camera-relative +X without replacing yaw; screenshot the off-axis
     aim/movement result.  The right stick held left (Gamepad_RightX=-1: -Y) then turns the
     player to yaw ~-90 and releasing it preserves that facing.  Screenshot.
  8. Gamepad move: Gamepad_LeftY=1 for 0.5 s moves the player > 100 cm along +X.  Expected
     to PASS.
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
import ctypes
from ctypes import wintypes

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
# so the corners are (0,-240), (240,-240), (240,0): open floor, far from the walls and steps.
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

# Lane L1 (y=-800, x -1000..+1600 is plain floor).  The two waypoints are the route the
# DevTestMap runs used (the check names are cited by other stories, e.g. AC-085.4); on the open
# floor they are plain positioning.  From the lane start a 1 s sprint, its stop, the L3 sprint and
# the gamepad move all stay well inside the lane (east wall at x=2000).
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
SPRINT_TAG = "Mordecai.State.Sprinting"
SPRINT_AUTO_CLEAR_MOVE_S = 0.35
SPRINT_STOP_SPEED = 60.0                    # runtime threshold: 10% of 600 cm/s walk speed
SPRINT_AUTO_CLEAR_EXPECTED_S = 0.25
SPRINT_AUTO_CLEAR_RANGE = (0.15, 0.60)      # bridge polling/frame tolerance around 0.25 s
SPRINT_AUTO_CLEAR_TIMEOUT_S = 1.25
SPRINT_RESTART_TIMEOUT_S = 0.5
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
ARROW_LENGTH = 80.0
ARROW_SIZE = 1.5

KNOWN_ISSUES = ()

SCREENSHOTS = {
    "start": "MovementTestMap from the fixed overhead camera (pitch -50, looking +X): the open "
             "grid floor, the player at the centre, no enemies or pickups, the combat HUD "
             "(Health / Stamina / Posture / SP bars top-left with '10 / 10', 'Kills: 0' "
             "top-right)",
    "after_moves": "the same camera angle (no rotation) with the player at the lane start "
                   "(-700, -800): the camera followed, so the player is still near the screen "
                   "centre and the floor grid has shifted under it",
    "physical_mouse_off_axis": "the player after W moved camera-relative +X while the real OS "
                               "cursor remained hidden and off-axis at screen-right/world +Y: "
                               "the single visible HUD reticle and green facing arrow point "
                               "toward +Y and the fixed camera angle is unchanged",
    "facing_arrow": "the player after the right-stick facing attempt: the existing green facing "
                    "arrow is visibly present and points along the pawn's world -Y direction",
}

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = kit.EXIT_PASS, kit.EXIT_FAIL, kit.EXIT_INFRA
set_log_stream = kit.set_log_stream
lit = kit.lit
_dumps = kit.dumps
_section = kit.section


class Win32PhysicalMouse:
    """Drive the real Windows cursor and LMB for packaged mouse-aim acceptance.

    The optional fallbacks exist only for attached offline fake-bridge tests. A launched
    acceptance run always requires the game HWND and uses SetCursorPos / SendInput.
    """

    def __init__(self, pid, fallback_move=None, fallback_click=None):
        self.pid = int(pid or 0)
        self.fallback_move = fallback_move
        self.fallback_click = fallback_click
        self.is_physical = fallback_move is None and fallback_click is None

    def _game_window(self):
        if sys.platform != "win32":
            raise RuntimeError("physical mouse acceptance requires Windows")
        user32 = ctypes.windll.user32
        matches = []

        @ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
        def visit(hwnd, _):
            window_pid = wintypes.DWORD()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(window_pid))
            if window_pid.value == self.pid and user32.IsWindowVisible(hwnd):
                matches.append(hwnd)
                return False
            return True

        user32.EnumWindows(visit, 0)
        if not matches:
            raise RuntimeError(f"no visible game window belongs to pid {self.pid}")
        return matches[0]

    def move_client(self, x, y):
        if self.fallback_move:
            response = self.fallback_move(x, y)
            return {"physical": False, "client": [x, y], "fallback": response}

        user32 = ctypes.windll.user32
        hwnd = self._game_window()
        point = wintypes.POINT(int(x), int(y))
        if not user32.ClientToScreen(hwnd, ctypes.byref(point)):
            raise ctypes.WinError()
        user32.ShowWindow(hwnd, 5)
        # Keep the packaged window exposed so SetCursorPos reaches its real HWND even when the
        # self-hosted worker's foreground lock does not allow this process to activate it.
        user32.SetWindowPos.argtypes = (
            wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
            ctypes.c_int, ctypes.c_int, wintypes.UINT)
        user32.SetWindowPos.restype = wintypes.BOOL
        if not user32.SetWindowPos(
                hwnd, ctypes.c_void_p(-1), 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0040):
            raise ctypes.WinError()
        foreground_ok = user32.GetForegroundWindow() == hwnd
        if not user32.SetCursorPos(point.x, point.y):
            raise ctypes.WinError()
        return {"physical": True, "hwnd": int(hwnd), "client": [x, y],
                "screen": [point.x, point.y], "foreground": foreground_ok}

    def click_left(self, x, y):
        if self.fallback_click:
            response = self.fallback_click(x, y)
            return {"physical": False, "client": [x, y], "fallback": response}

        moved = self.move_client(x, y)

        class MOUSEINPUT(ctypes.Structure):
            _fields_ = (("dx", wintypes.LONG), ("dy", wintypes.LONG),
                        ("mouseData", wintypes.DWORD), ("dwFlags", wintypes.DWORD),
                        ("time", wintypes.DWORD), ("dwExtraInfo", ctypes.c_size_t))

        class INPUTUNION(ctypes.Union):
            _fields_ = (("mi", MOUSEINPUT),)

        class INPUT(ctypes.Structure):
            _anonymous_ = ("u",)
            _fields_ = (("type", wintypes.DWORD), ("u", INPUTUNION))

        inputs = (INPUT * 2)(
            INPUT(type=0, mi=MOUSEINPUT(dwFlags=0x0002)),
            INPUT(type=0, mi=MOUSEINPUT(dwFlags=0x0004)),
        )

        sent = ctypes.windll.user32.SendInput(
            len(inputs), ctypes.byref(inputs), ctypes.sizeof(INPUT))
        if sent != len(inputs):
            raise ctypes.WinError()
        moved["sent_inputs"] = int(sent)
        return moved


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
    Step("facing_arrow", "state player.facing_arrow -> hidden_in_game=false, owner hidden=false, "
         "registered with a scene proxy, recently rendered, owner visibility unrestricted, "
         f"length={ARROW_LENGTH:g}, size={ARROW_SIZE:g}, and world forward aligned with the "
         "pawn forward vector", proves=("AC-2.1.5",)),
    *[Step(check, f"hold {key} {MOVE_HOLD_S:g}s (retry once if a frame hitch yields a short "
           f"correct-direction move) -> player.location moves > {MOVE_MIN:g} cm along {d}, "
           f"< {CROSS_MAX:g} cm across", proves=("AC-2.1.1", "AC-1.3.2"))
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
         f"cm; only while {SPRINT_TAG} is present, tap {SPRINT_KEY} again, then hold S "
         f"{SPRINT_HOLD_S:g}s -> back to a walk (peak player.speed {WALK_SPEED_MIN:g}.."
         f"{SPRINT_SPEED_MIN:g} cm/s)",
         proves=("AC-1.3.3", "AC-2.1.3")),
    Step("sprint_auto_clear_restart", f"tap {SPRINT_KEY}, hold W "
         f"{SPRINT_AUTO_CLEAR_MOVE_S:g}s, release and observe speed <= {SPRINT_STOP_SPEED:g} "
         f"cm/s -> {SPRINT_TAG} auto-clears about {SPRINT_AUTO_CLEAR_EXPECTED_S:g}s later; "
         f"tap {SPRINT_KEY} -> the tag returns (then state-aware cleanup tap)",
         proves=("AC-1.3.3", "AC-2.1.3")),
    Step("walk_lane_reset2", f"walk_to({LANE_START[0]:g}, {LANE_START[1]:g}) -> within 60 cm"),
    Step("sprint_l3", f"tap {SPRINT_PAD_KEY} (L3), hold W {SPRINT_HOLD_S:g}s -> dx > "
         f"{SPRINT_MIN:g} cm (then tap L3 again)", proves=("AC-1.3.3", "AC-2.1.3")),
    Step("jump_space", f"tap {JUMP_KEY} -> wait_until player.movement_mode==\"Falling\" "
         f"within {JUMP_TIMEOUT_S:g}s", proves=("AC-2.1.4",)),
    Step("facing_mouse_physical", f"Win32 SetCursorPos to client ({FACING_MOUSE[0]:g}, "
         f"{FACING_MOUSE[1]:g}) (right of the player, +Y; no bridge mouse command) -> "
         f"wait_until player.rotation yaw {FACING_MOUSE_YAW:g} (+-{FACING_TOL:g}) within "
         f"{FACING_TIMEOUT_S:g}s", proves=("AC-2.1.2",)),
    Step("single_cursor_mouse", "after physical mouse movement -> focus.input_type is "
         "MouseAndKeyboard and focus.mouse_cursor_shown=false; the screenshot-visible HUD "
         "reticle is the one gameplay cursor", proves=("AC-2.1.2",)),
    Step("attack_click_physical", "with the physical OS cursor still at the aim point, bridge "
         "click LMB through Slate -> Mordecai.State.Attacking appears in player.tags",
         proves=("AC-2.1.2",)),
    Step("combo_hit2", "wait for Mordecai.State.ComboWindow after Hit 1, click LMB through "
         "Slate again -> stamina is consumed again and Mordecai.State.AttackCommitted appears "
         "for Hit 2 without Mordecai.Attack.Heavy", proves=("US-004:AC-004.14",)),
    Step("movement_keeps_aim", f"with mouse aim still +Y, hold W {MOVE_HOLD_S:g}s -> "
         f"player.location moves > {MOVE_MIN:g} cm along camera-relative +X and < "
         f"{CROSS_MAX:g} cm across while yaw stays {FACING_MOUSE_YAW:g} "
         f"(+-{FACING_TOL:g})", proves=("AC-2.1.1", "AC-2.1.2", "AC-1.3.2")),
    Step("facing_stick", f"axis Gamepad_RightX=-1 held (left, -Y) -> wait_until "
         f"player.rotation yaw {FACING_STICK_YAW:g} (+-{FACING_TOL:g}) within "
         f"{FACING_TIMEOUT_S:g}s, then release and retain that yaw", proves=("AC-2.1.2",)),
    Step("cursor_hidden_gamepad", "after right-stick input -> focus.input_type is Gamepad and "
         "focus.mouse_cursor_shown=false", proves=("AC-2.1.2",)),
    Step("gamepad_move", f"axis Gamepad_LeftY=1 for {GAMEPAD_HOLD_S:g}s -> player.location "
         f"x rises > {GAMEPAD_MIN:g} cm", proves=("AC-2.1.1", "AC-1.3.2")),
)

PENDING = {
    "AC-1.2.1": "bridge cannot drive: state has no camera transform (screenshot after_moves "
                "shows whether the camera followed)",
}


class Us001Run(kit.PlaytestRun):
    """One run of the playtest.  execute() returns the exit code."""

    STORY = STORY
    COVERS = ("US-004",)
    FEATURES = ("movement", "camera", "sprint")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.MOVEMENT_TEST_MAP
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
        read again, [after()]. Returns ((p0, p1, held_s, stop_s, response), None) or
        (None, why)."""
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
        return (p0, p1, r.get("held_s"), stop_s, r), None

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

        def arrow_ok(st):
            arrow = _section(st, "player").get("facing_arrow")
            return (isinstance(arrow, dict)
                    and arrow.get("hidden_in_game") is False
                    and arrow.get("owner_hidden") is False
                    and arrow.get("registered") is True
                    and arrow.get("scene_proxy_present") is True
                    and arrow.get("was_recently_rendered") is True
                    and arrow.get("only_owner_see") is False
                    and arrow.get("owner_no_see") is False
                    and isinstance(arrow.get("arrow_length"), (int, float))
                    and isinstance(arrow.get("arrow_size"), (int, float))
                    and math.isclose(arrow.get("arrow_length", -1), ARROW_LENGTH, abs_tol=0.01)
                    and math.isclose(arrow.get("arrow_size", -1), ARROW_SIZE, abs_tol=0.01)
                    and isinstance(arrow.get("forward_alignment"), (int, float))
                    and arrow["forward_alignment"] > 0.999)

        def arrow_text(st):
            arrow = _section(st, "player").get("facing_arrow")
            if not isinstance(arrow, dict):
                return f"player.facing_arrow is {arrow!r} (runtime arrow state is missing)"
            return ("player.facing_arrow hidden_in_game="
                    f"{arrow.get('hidden_in_game')} owner_hidden={arrow.get('owner_hidden')} "
                    f"registered={arrow.get('registered')} scene_proxy_present="
                    f"{arrow.get('scene_proxy_present')} was_recently_rendered="
                    f"{arrow.get('was_recently_rendered')} only_owner_see="
                    f"{arrow.get('only_owner_see')} owner_no_see={arrow.get('owner_no_see')} "
                    f"arrow_length={arrow.get('arrow_length')} arrow_size={arrow.get('arrow_size')} "
                    f"forward={arrow.get('forward')} pawn_forward={arrow.get('pawn_forward')} "
                    f"alignment={arrow.get('forward_alignment')} (need rendered=true, "
                    f"length={ARROW_LENGTH:g}, size={ARROW_SIZE:g}, alignment > 0.999)")

        self.observe("facing_arrow", ["player"], arrow_ok, arrow_text, timeout_s=3)
        self.shot("start")

    # --- step 2: WASD ----------------------------------------------------------------------
    def step_moves(self):
        for check, key, axis, sign, direction in MOVES:
            def fn(check=check, key=key, axis=axis, sign=sign, direction=direction):
                got, err = self._hold_and_measure(key, MOVE_HOLD_S)
                if got is None:
                    return "FAIL", err, None
                p0, p1, held_s, stop_s, response = got
                along = (p1[axis] - p0[axis]) * sign
                across = abs(p1[1 - axis] - p0[1 - axis])
                attempts = 1
                # The packaged game can advance only a fraction of the requested hold while its
                # first post-launch frames hitch. Retry one real hold when direction is correct
                # but displacement is short; wrong-axis mappings still fail without a retry.
                simulated_s = response.get("simulated_s")
                frame_short = (isinstance(simulated_s, (int, float))
                               and simulated_s < MOVE_HOLD_S * 0.8)
                if 0 <= along <= MOVE_MIN and across < CROSS_MAX and frame_short:
                    retry, retry_err = self._hold_and_measure(key, MOVE_HOLD_S)
                    if retry is None:
                        return "FAIL", retry_err, None
                    _, p1, retry_held_s, retry_stop_s, retry_response = retry
                    held_s = round(held_s + retry_held_s, 3)
                    stop_s = retry_stop_s
                    attempts = 2
                    along = (p1[axis] - p0[axis]) * sign
                    across = abs(p1[1 - axis] - p0[1 - axis])
                rec = self._record(check, f"hold {key} {MOVE_HOLD_S:g}s x{attempts}", p0, p1,
                                   {"along": round(along, 1), "across": round(across, 1),
                                    "held_s": held_s, "stop_s": stop_s,
                                    "attempts": attempts,
                                    "simulated_s": [response.get("simulated_s")]
                                    + ([retry_response.get("simulated_s")]
                                       if attempts == 2 else [])})
                ok = along > MOVE_MIN and across < CROSS_MAX
                detail = (f"held {key} {held_s}s over {attempts} attempt(s): "
                          f"({p0[0]:.0f}, {p0[1]:.0f}) -> ({p1[0]:.0f}, "
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
        """Tap the toggle, hold W and measure, then toggle off only while Sprinting is present."""
        def fn():
            b = self.bridge
            if self._settle(b, SETTLE_TIMEOUT_S) is None:
                return "FAIL", "player did not stop before the sprint toggle", None
            p0, raw = self._player(b)
            if p0 is None:
                return "FAIL", f"player section invalid before the input ({raw.get('reason')})", None
            b.tap(toggle_key, hold_s=self.TAP_S)
            response = b.hold("W", SPRINT_HOLD_S)
            _, post_hold = self._player(b)
            tags = [str(tag) for tag in post_hold.get("tags", [])
                    ] if isinstance(post_hold.get("tags"), list) else []
            toggle_off_sent = SPRINT_TAG in tags
            if toggle_off_sent:
                b.tap(toggle_key, hold_s=self.TAP_S)
            stop_s = self._settle(b, SETTLE_TIMEOUT_S)
            p1, raw = self._player(b)
            if p1 is None:
                return "FAIL", f"player section invalid after the input ({raw.get('reason')})", None
            held_s = response.get("held_s")
            dx = p1[0] - p0[0]
            rec = self._record(check, f"tap {toggle_key}, hold W {SPRINT_HOLD_S:g}s", p0, p1,
                               {"held_s": held_s, "stop_s": stop_s,
                                "post_hold_tags": tags,
                                "toggle_off_sent": toggle_off_sent})
            walk = next((m["dx"] for m in self.measurements
                         if m["check"] == "walk_speed_baseline"), None)
            vs = f" (walk baseline {walk:.0f} cm)" if walk is not None else ""
            detail = (f"tapped {label}, held W {held_s}s: dx {dx:.0f} cm (need > "
                      f"{SPRINT_MIN:g}){vs}")
            sprinted = dx > SPRINT_MIN
            if not toggle_off_sent:
                if sprinted:
                    detail += f"; {SPRINT_TAG} auto-cleared before the toggle-off tap"
                else:
                    detail += (f"; {SPRINT_TAG} was absent before the toggle-off tap "
                               "(the first tap did not start sprint or it cleared too early)")
                detail += (f" (tags {tags}); no second {label} tap was sent, so sprint was not "
                           "accidentally restarted")
                return "FAIL", detail, rec
            detail += f"; {SPRINT_TAG} was present, so tapped {label} again to toggle back"
            if not verify_toggle_back:
                return ("PASS" if sprinted else "FAIL"), detail, rec

            # The toggle-back is judged by speed, not distance: a speed reading cannot be shortened
            # by a frame hitch or anything in the way, and a sprint (900 cm/s) and a walk
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
                detail += (" -- the player never reached walking speed on the way back "
                           "(blocked?), so the toggle-back is unverified")
                return "FAIL", detail, rec
            return ("PASS" if sprinted else "FAIL"), detail, rec

        return fn

    def _sprint_auto_clear_restart(self):
        """Prove moved sprint's stationary auto-clear and that the next Shift starts anew."""
        b = self.bridge

        def sample():
            state = b.state(["player", "game"])
            player = _section(state, "player")
            game = _section(state, "game")
            tags = [str(tag) for tag in player.get("tags", [])
                    ] if isinstance(player.get("tags"), list) else []
            speed = player.get("speed")
            world_time = game.get("world_time_s")
            return tags, (float(speed) if isinstance(speed, (int, float)) else None), (
                float(world_time) if isinstance(world_time, (int, float)) else None)

        tags, _, _ = sample()
        if SPRINT_TAG in tags:
            return ("FAIL", f"{SPRINT_TAG} was already present before the auto-clear check; "
                    "refusing to send an ambiguous Shift tap", {"initial_tags": tags})

        b.tap(SPRINT_KEY, hold_s=self.TAP_S)
        tags, _, _ = sample()
        if SPRINT_TAG not in tags:
            return ("FAIL", f"tap {SPRINT_KEY} did not start sprint (tags {tags})",
                    {"initial_tags": tags})
        # Keep W down until a separate release reply. A timed key tap releases in the game
        # before its reply reaches Python, so a hitch can consume the entire 0.25 s clear window.
        b.press("W")
        try:
            b.wait(SPRINT_AUTO_CLEAR_MOVE_S)
        finally:
            b.release("W")

        try:
            threshold = b.wait_until(f"player.speed<={SPRINT_STOP_SPEED:g}",
                                     timeout_s=SPRINT_AUTO_CLEAR_TIMEOUT_S,
                                     poll_s=SPEED_POLL_S)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            return ("FAIL", f"player.speed did not reach <= {SPRINT_STOP_SPEED:g} cm/s within "
                    f"{SPRINT_AUTO_CLEAR_TIMEOUT_S:g}s after releasing W",
                    {"values": (e.response or {}).get("values")})

        tags, speed, threshold_at = sample()
        samples = [{"world_time_s": threshold_at, "speed": speed, "tags": tags}]
        if SPRINT_TAG not in tags:
            return ("FAIL", f"{SPRINT_TAG} cleared before the bridge could observe it after "
                    f"player.speed reached <= {SPRINT_STOP_SPEED:g} cm/s",
                    {"threshold": threshold, "samples": samples})
        if threshold_at is None:
            return "FAIL", "game.world_time_s is unavailable at the stop threshold", {
                "threshold": threshold, "samples": samples}

        cleared_at = None
        deadline = time.monotonic() + SPRINT_AUTO_CLEAR_TIMEOUT_S
        while time.monotonic() < deadline:
            b.wait(SPEED_POLL_S)
            tags, speed, world_time = sample()
            samples.append({"world_time_s": world_time, "speed": speed, "tags": tags})
            if SPRINT_TAG not in tags:
                cleared_at = world_time
                break

        if cleared_at is None:
            return ("FAIL", f"{SPRINT_TAG} did not auto-clear within "
                    f"{SPRINT_AUTO_CLEAR_TIMEOUT_S:g}s after releasing W", {"samples": samples})
        clear_s = cleared_at - threshold_at
        lo, hi = SPRINT_AUTO_CLEAR_RANGE
        if not lo <= clear_s <= hi:
            return ("FAIL", f"{SPRINT_TAG} cleared {clear_s:.3f}s after speed reached <= "
                    f"{SPRINT_STOP_SPEED:g} cm/s (expected about "
                    f"{SPRINT_AUTO_CLEAR_EXPECTED_S:g}s, accepted {lo:g}..{hi:g}s)",
                    {"clear_s": clear_s, "samples": samples})

        b.tap(SPRINT_KEY, hold_s=self.TAP_S)
        restarted = False
        restart_deadline = time.monotonic() + SPRINT_RESTART_TIMEOUT_S
        restart_tags = []
        while time.monotonic() < restart_deadline:
            restart_tags, _, _ = sample()
            if SPRINT_TAG in restart_tags:
                restarted = True
                break
            b.wait(SPEED_POLL_S)
        if not restarted:
            return ("FAIL", f"after auto-clear, the next {SPRINT_KEY} tap did not start a new "
                    f"sprint (tags {restart_tags})", {"clear_s": clear_s, "samples": samples})

        # State-aware cleanup: this tap is sent only after the state above proves sprint is active.
        b.tap(SPRINT_KEY, hold_s=self.TAP_S)
        detail = (f"held W {SPRINT_AUTO_CLEAR_MOVE_S:g}s; after speed reached <= "
                  f"{SPRINT_STOP_SPEED:g} cm/s, {SPRINT_TAG} auto-cleared in {clear_s:.3f}s "
                  f"(expected about {SPRINT_AUTO_CLEAR_EXPECTED_S:g}s); the next "
                  f"{SPRINT_KEY} tap restored the tag; sent one state-aware cleanup tap")
        return "PASS", detail, {"clear_s": round(clear_s, 3), "samples": samples,
                                "restart_tags": restart_tags}

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
        self.attempt("sprint_auto_clear_restart", self._sprint_auto_clear_restart,
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

    def _physical_mouse(self):
        if self.mode == "attach":
            # Offline fake tests and exploratory attaches are diagnostic-only. Acceptance launches
            # the package and therefore always takes the Win32 path with no Slate fallback.
            return Win32PhysicalMouse(
                self.bridge.pid,
                fallback_move=self.bridge.mouse,
                fallback_click=self.bridge.click)
        return Win32PhysicalMouse(self.bridge.pid)

    def step_facing(self):
        def mouse():
            x, y = FACING_MOUSE
            r = self._physical_mouse().move_client(x, y)
            return self._wait_yaw(
                FACING_MOUSE_YAW,
                f"physical OS cursor moved to game client ({x:g}, {y:g})",
                r)

        mouse_ok, _ = self.attempt("facing_mouse_physical", mouse)

        def single_cursor():
            if self.mode == "attach":
                return ("PASS", "diagnostic attach cannot observe the packaged CommonUI cursor "
                        "policy; launched acceptance requires the OS cursor hidden", None)
            try:
                r = self.bridge.wait_until(
                    'focus.input_type=="MouseAndKeyboard" && focus.mouse_cursor_shown==false',
                    timeout_s=1.0)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                focus = _section(self.bridge.state(["focus"]), "focus")
                return ("FAIL", f"physical mouse input did not hide the OS gameplay cursor: "
                        f"focus={focus}", focus)
            return ("PASS", "physical mouse is last-used, the OS cursor is hidden, and the HUD "
                    "reticle is the one gameplay cursor", r)

        self.attempt("single_cursor_mouse", single_cursor, needs=mouse_ok,
                     needs_why="not run: physical mouse aim did not establish +Y facing")

        def attack_click():
            x, y = FACING_MOUSE
            response = self.bridge.click(x, y, button="Left")
            if self.mode == "attach":
                return ("PASS", "diagnostic attached fake accepted the Slate-routed click; "
                        "launched acceptance waits for State.Attacking", response)
            seen = []
            deadline = time.monotonic() + 0.6
            while time.monotonic() < deadline:
                tags = _section(self.bridge.state(["player"]), "player").get("tags") or []
                seen = list(tags)
                if "Mordecai.State.Attacking" in tags:
                    return ("PASS", "physical OS cursor remained at the off-axis aim point; "
                            "the Slate-routed LMB click reached Enhanced Input and started a "
                            "melee attack", {"input": response, "tags": seen})
                time.sleep(0.02)
            return ("FAIL", "LMB click with the physical cursor at the aim point did not produce "
                    f"Mordecai.State.Attacking; last player.tags={seen}; input={response}",
                    {"input": response, "tags": seen})

        self.attempt("attack_click_physical", attack_click, needs=mouse_ok,
                     needs_why="not run: physical mouse aim did not establish +Y facing")

        def combo_hit2():
            if self.mode == "attach":
                response = self.bridge.click(*FACING_MOUSE, button="Left")
                return ("PASS", "diagnostic attached fake accepted the second Slate-routed "
                        "click; launched acceptance observes ComboWindow, stamina and "
                        "AttackCommitted", response)

            b = self.bridge
            combo_deadline = time.monotonic() + 2.0
            combo_tags = []
            stamina_before = None
            while time.monotonic() < combo_deadline:
                player = _section(b.state(["player"]), "player")
                combo_tags = list(player.get("tags") or [])
                if "Mordecai.State.ComboWindow" in combo_tags:
                    stamina_before = (player.get("attributes") or {}).get("stamina")
                    break
                time.sleep(0.02)
            if stamina_before is None:
                return ("FAIL", "Hit 1 never opened Mordecai.State.ComboWindow; "
                        f"last player.tags={combo_tags}", {"tags": combo_tags})

            response = b.click(*FACING_MOUSE, button="Left")
            hit2_deadline = time.monotonic() + 2.0
            seen_committed = False
            last_tags = combo_tags
            stamina_after = stamina_before
            while time.monotonic() < hit2_deadline:
                player = _section(b.state(["player"]), "player")
                last_tags = list(player.get("tags") or [])
                stamina_after = (player.get("attributes") or {}).get("stamina")
                seen_committed = seen_committed or (
                    "Mordecai.State.AttackCommitted" in last_tags)
                spent_again = (
                    isinstance(stamina_before, (int, float))
                    and isinstance(stamina_after, (int, float))
                    and stamina_after < stamina_before)
                if (seen_committed and spent_again
                        and "Mordecai.Attack.Heavy" not in last_tags):
                    return ("PASS", "the second LMB queued in ComboWindow chained light Hit 2: "
                            f"stamina {stamina_before:g} -> {stamina_after:g}, "
                            "AttackCommitted seen, no Heavy tag",
                            {"input": response, "tags": last_tags,
                             "stamina_before": stamina_before,
                             "stamina_after": stamina_after})
                time.sleep(0.02)
            return ("FAIL", "the second LMB did not prove light Hit 2: "
                    f"stamina {stamina_before} -> {stamina_after}, "
                    f"AttackCommitted seen={seen_committed}, last tags={last_tags}",
                    {"input": response, "tags": last_tags,
                     "stamina_before": stamina_before,
                     "stamina_after": stamina_after})

        self.attempt("combo_hit2", combo_hit2, needs=mouse_ok,
                     needs_why="not run: physical mouse aim did not establish +Y facing")

        def movement_keeps_aim():
            b = self.bridge
            try:
                b.wait_until('player.movement_mode=="Walking"', timeout_s=2.0)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                return "FAIL", "player did not return to movable recovery before the aimed move", None
            recovery_deadline = time.monotonic() + 2.0
            while time.monotonic() < recovery_deadline:
                tags = _section(b.state(["player"]), "player").get("tags") or []
                if "Mordecai.State.Attacking" not in tags:
                    break
                time.sleep(0.02)
            else:
                return "FAIL", "State.Attacking did not end before the aimed move", None
            got, err = self._hold_and_measure("W", MOVE_HOLD_S)
            if got is None:
                return "FAIL", err, None
            p0, p1, held_s, stop_s, _ = got
            dx, dy = p1[0] - p0[0], p1[1] - p0[1]
            yaw = _section(b.state(["player"]), "player").get("rotation")
            yaw_value = yaw[1] if isinstance(yaw, list) and len(yaw) > 1 else None
            yaw_ok = (isinstance(yaw_value, (int, float))
                      and abs(_angle_delta(yaw_value, FACING_MOUSE_YAW)) < FACING_TOL)
            rec = self._record(
                "movement_keeps_aim", f"hold W {MOVE_HOLD_S:g}s while aimed +Y", p0, p1,
                {"held_s": held_s, "stop_s": stop_s, "yaw": yaw_value})
            ok = dx > MOVE_MIN and abs(dy) < CROSS_MAX and yaw_ok
            detail = (f"held W {held_s}s while mouse aim stayed at +Y: "
                      f"({p0[0]:.0f}, {p0[1]:.0f}) -> ({p1[0]:.0f}, {p1[1]:.0f}); "
                      f"{dx:.0f} cm along +X (need > {MOVE_MIN:g}), {abs(dy):.0f} cm across "
                      f"(need < {CROSS_MAX:g}), yaw {yaw_value} (need {FACING_MOUSE_YAW:g} "
                      f"+-{FACING_TOL:g})")
            return ("PASS" if ok else "FAIL"), detail, rec

        self.attempt("movement_keeps_aim", movement_keeps_aim, needs=mouse_ok,
                     needs_why="not run: mouse aim did not establish +Y facing")
        self.shot("physical_mouse_off_axis")

        def stick():
            b = self.bridge
            r = b.axis(FACING_STICK)
            try:
                result = self._wait_yaw(FACING_STICK_YAW, "Gamepad_RightX=-1 held", r)
            finally:
                self._release_axes(FACING_STICK)
            if result[0] != "PASS":
                return result
            yaw = _section(b.state(["player"]), "player").get("rotation")
            yaw_value = yaw[1] if isinstance(yaw, list) and len(yaw) > 1 else None
            if not (isinstance(yaw_value, (int, float))
                    and abs(_angle_delta(yaw_value, FACING_STICK_YAW)) < FACING_TOL):
                return ("FAIL", f"right stick reached -Y, but release changed yaw to {yaw_value} "
                        f"(expected {FACING_STICK_YAW:g} +-{FACING_TOL:g})", r)
            return ("PASS", f"{result[1]}; release preserved yaw {yaw_value}", r)

        self.attempt("facing_stick", stick)

        def cursor_hidden():
            try:
                r = self.bridge.wait_until(
                    'focus.input_type=="Gamepad" && focus.mouse_cursor_shown==false',
                    timeout_s=1.0)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                focus = _section(self.bridge.state(["focus"]), "focus")
                return ("FAIL", f"right-stick input did not hide the mouse cursor: focus={focus}",
                        focus)
            return "PASS", "gamepad is the last-used input and the gameplay cursor is hidden", r

        self.attempt("cursor_hidden_gamepad", cursor_hidden)
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


def _angle_delta(actual, expected):
    """Smallest signed angular difference in degrees."""
    return (float(actual) - float(expected) + 180.0) % 360.0 - 180.0


def parse_args(argv=None):
    return kit.parse_args(Us001Run, argv)


def main(argv=None):
    return kit.main(Us001Run, argv)


if __name__ == "__main__":
    sys.exit(main())
