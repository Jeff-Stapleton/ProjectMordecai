#!/usr/bin/env python3
"""Playtest Kit -- the run framework every Tier 4 (real-input) story playtest shares

The contract is Specs/story_playtests_v1.md section 2; the bridge protocol and the Python client
are Specs/agent_bridge_v1.md.  A story script (Scripts/playtests/usXXX_<name>.py) subclasses
PlaytestRun, declares its PLAN of Steps and plays them in steps() with the step helpers.  The
kit does the rest: the STALE gate, launch or attach, build freshness, bridge readiness, the
expected experience and map (MAP: DEFAULT_MAP = DevTestMap, MOVEMENT_TEST_MAP, COMBAT_TEST_MAP),
screenshots, crash and log checks, plan enforcement, per-AC acceptance, report.md / report.json
and the exit code.

Real input only: every helper drives the game through agent_bridge.Bridge's real-input methods
(key tap, axis, click, walk_to built from W/A/S/D taps) and reads it with state / wait_until /
screenshot.  Nothing here calls game functions, sends a debug_* command, a route:"viewport" key
or a denied debug key.

A story script:
  import playtest_kit as kit
  from playtest_kit import Step

  class Run(kit.PlaytestRun):
      STORY = "US-078"
      FEATURES = ("weapons",)
      PLAN = (Step("cycle_next", "tap Tab -> player.weapon.index==1", proves=("AC-078.4",)),)

      def steps(self):
          self.press("cycle_next", "Tab", then="player.weapon.index==1", timeout_s=2)
          self.shot("cycled")

  if __name__ == "__main__":
      raise SystemExit(kit.main(Run))

Usage (every story script takes the same options):
  py Scripts/playtests/usXXX_<name>.py                    fresh package: launch, play, quit
  py Scripts/playtests/usXXX_<name>.py --allow-stale      STALE package: diagnostic only (recorded)
  py Scripts/playtests/usXXX_<name>.py --keep-running     leave the game running afterwards
  py Scripts/playtests/usXXX_<name>.py --attach-port N [--token T]
        play a game that is already running; always diagnostic only, never acceptance evidence
  py Scripts/playtests/usXXX_<name>.py --plan             print the PLAN as the story's Steps
        table (markdown) and exit 0; no game is launched
Evidence: report.md's Evidence line reads ACCEPTANCE CANDIDATE only for a launched, fresh,
clean-tree package without --allow-stale that exited 0; every other run is DIAGNOSTIC ONLY, and
an AC is accepted only by a PASS in an ACCEPTANCE CANDIDATE run.

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-<suffix>/ with report.md, report.json, game.log,
bridge.jsonl and bridge_NN.png (suffix: RUN_SUFFIX, default "us" + the story's digits).  A STALE
build without --allow-stale stops before any run dir is created.

Exit codes:
  0 -- every check passed (WARN allowed); --plan printed the table
  1 -- a gameplay check FAILED (the game did not do what a player expects)
  2 -- infrastructure problem (STALE BUILD, launch failure, no connection, crash, a bad PLAN)
"""

import argparse
import dataclasses
import datetime
import json
import os
import re
import shutil
import sys
import time
import traceback

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402

DEFAULT_MAP = pb.DEFAULT_MAP
MOVEMENT_TEST_MAP = pb.MOVEMENT_TEST_MAP
COMBAT_TEST_MAP = pb.COMBAT_TEST_MAP
TEST_MAPS = pb.TEST_MAPS
DEFAULT_EXPERIENCE = pb.DEFAULT_EXPERIENCE
FRONTEND_EXPERIENCE = pb.FRONTEND_EXPERIENCE
EXIT_PASS, EXIT_FAIL, EXIT_INFRA = pb.EXIT_PASS, pb.EXIT_FAIL, pb.EXIT_INFRA

CONNECTION_LOST = frozenset({"disconnected", "no_response", "closed", "connect_failed", "crash",
                             "exited"})
# Checks the kit records itself (section 2.3): they need no Step.  screenshot_<label> too.
KIT_CHECKS = frozenset({"build_fresh", "launch", "attach", "bridge_ready", "experience", "map",
                        "no_crash", "content_cooked", "log_errors", "completed"})
SCREENSHOT_PREFIX = "screenshot_"
NOT_REACHED = "not reached"
STEP_KINDS = ("gameplay", "infra")
# Section 6.2: Playable Status is tracked per feature, in this vocabulary.
FEATURE_VOCABULARY = frozenset({
    "movement", "camera", "melee_attack", "combo", "dodge", "block_parry", "posture_stagger",
    "stamina", "sprint", "projectiles_aim", "attributes", "skills", "feats", "status_effects",
    "spells", "spell_hud", "weapons", "weapon_hud", "inventory", "pickups", "identification",
    "enemies", "enemy_ai", "combat_hud", "death_respawn", "kill_counter", "damage_numbers",
    "enemy_indicators", "combo_counter", "pause_menu", "character_sheet", "skill_tree_ui",
    "feat_ui", "front_end_menu"})
PLAN_HEADER = ("| # | Check | Player step (bridge calls) → expected | Proves |\n"
               "|---|---|---|---|")
AC_STATUSES = ("PASS", "FAIL", "NOT RUN", "PENDING")
PENDING_PREFIX = "bridge cannot drive"

RE_STORY = re.compile(r"^US-\d+$")
RE_CHECK = re.compile(r"^[A-Za-z][A-Za-z0-9_]*$")
RE_AC = re.compile(r"^(?:(US-\d+):)?(AC[A-Za-z0-9.\-]*[A-Za-z0-9])$")

_LOG_STREAM = None


def set_log_stream(stream):
    """Send the kit's (and every story script's) log lines to `stream` (None = sys.stdout).
    Returns the previous one."""
    global _LOG_STREAM
    previous = _LOG_STREAM
    _LOG_STREAM = stream
    return previous


def _out():
    return _LOG_STREAM if _LOG_STREAM is not None else sys.stdout


def log(msg, tag="playtest"):
    print(f"[{tag}] {msg}", file=_out(), flush=True)


# ---------------------------------------------------------------------------
# Small helpers (story scripts use them too)
# ---------------------------------------------------------------------------
def lit(text):
    """A wait_until string literal (double-quoted, \\ and \" escaped)."""
    return '"' + str(text).replace("\\", "\\\\").replace('"', '\\"') + '"'


def now_iso():
    return datetime.datetime.now().isoformat(timespec="seconds")


def dumps(obj, limit=None):
    text = json.dumps(obj, ensure_ascii=False, separators=(",", ":"), default=str)
    if limit and len(text) > limit:
        text = text[:limit] + "..."
    return text


def section(st, name):
    """state[name] when it is an object, else {} (an invalid or missing section)."""
    value = st.get(name) if isinstance(st, dict) else None
    return value if isinstance(value, dict) else {}


def keys_text(key_or_keys):
    if isinstance(key_or_keys, (list, tuple)):
        return "+".join(str(k) for k in key_or_keys)
    return str(key_or_keys)


def map_key(value):
    """A map path as the 'map' check compares it: any '.Object' suffix stripped, lower case.
    '/MordecaiCore/Game/Maps/DevTestMap.DevTestMap' -> '/mordecaicore/game/maps/devtestmap';
    None -> ''.  The bridge reports the package name (no suffix); cooked FNames may differ in
    case from the script's spelling."""
    s = str(value or "").strip()
    head, sep, tail = s.rpartition("/")
    return (head + sep + tail.split(".", 1)[0]).lower()


def split_ac(ac, story):
    """'US-077:AC-077.2' -> ('US-077', 'AC-077.2'); 'AC-078.4' -> (story, 'AC-078.4')."""
    m = RE_AC.match(str(ac))
    if not m:
        return story, str(ac)
    return (m.group(1) or story), m.group(2)


def qualified_ac(story, ac_id, own_story):
    """The id as a PLAN writes it: bare for the owning story, 'US-NNN:AC-...' otherwise."""
    return ac_id if story == own_story else f"{story}:{ac_id}"


# ---------------------------------------------------------------------------
# The PLAN
# ---------------------------------------------------------------------------
@dataclasses.dataclass(frozen=True)
class Step:
    """One check of a story's PLAN (Specs/story_playtests_v1.md section 2.3).

    check: the check name the script records (unique in the plan).  does: the player's action
    and the expected result, in bridge terms (the Steps table row).  proves: AC ids this check
    proves (a bare id is the owning story's; 'US-077:AC-077.2' another story's).  kind: gameplay
    or infra.  known_issue: set when the check is EXPECTED to FAIL today because of a filed bug
    (it never changes the status or the exit code; the report marks the FAIL 'known').
    """
    check: str
    does: str
    proves: tuple = ()
    kind: str = "gameplay"
    known_issue: str = None

    def __post_init__(self):
        proves = (self.proves,) if isinstance(self.proves, str) else tuple(self.proves or ())
        object.__setattr__(self, "proves", proves)

    def to_dict(self):
        return {"check": self.check, "does": self.does, "proves": list(self.proves),
                "kind": self.kind, "known_issue": self.known_issue}


def plan_problems(run_class):
    """Everything wrong with a PlaytestRun subclass's declarations (empty list = fine)."""
    problems = []
    story = getattr(run_class, "STORY", None)
    if not isinstance(story, str) or not RE_STORY.match(story):
        problems.append(f"STORY must be 'US-NNN', got {story!r}")
    covers = tuple(getattr(run_class, "COVERS", ()) or ())
    for c in covers:
        if not isinstance(c, str) or not RE_STORY.match(c):
            problems.append(f"COVERS entry {c!r} is not a story id 'US-NNN'")
    for f in getattr(run_class, "FEATURES", ()) or ():
        if f not in FEATURE_VOCABULARY:
            problems.append(f"FEATURES entry {f!r} is not in the feature vocabulary "
                            "(Specs/story_playtests_v1.md section 6.2)")
    plan = getattr(run_class, "PLAN", None)
    if not plan:
        problems.append("PLAN is empty: declare a Step for every check the script records")
        plan = ()
    seen = set()
    stories = {story, *covers}

    def check_ac(ac, where):
        m = RE_AC.match(str(ac))
        if not m:
            problems.append(f"{where}: {ac!r} is not an AC id (AC-078.4, AC3, US-077:AC-077.2)")
        elif m.group(1) and m.group(1) not in stories:
            problems.append(f"{where}: {ac} names {m.group(1)}, which is neither STORY nor in "
                            "COVERS")

    for i, step in enumerate(plan, 1):
        if not isinstance(step, Step):
            problems.append(f"PLAN[{i}] is not a Step: {step!r}")
            continue
        name = step.check
        if not isinstance(name, str) or not RE_CHECK.match(name):
            problems.append(f"PLAN[{i}]: bad check name {name!r} (letters, digits, _)")
        elif name in KIT_CHECKS or name.startswith(SCREENSHOT_PREFIX):
            problems.append(f"PLAN[{i}]: {name} is a check the kit records itself")
        elif name in seen:
            problems.append(f"PLAN[{i}]: check {name} is declared twice")
        seen.add(name)
        if not str(step.does or "").strip():
            problems.append(f"PLAN[{i}] ({name}): 'does' is empty")
        if step.kind not in STEP_KINDS:
            problems.append(f"PLAN[{i}] ({name}): kind must be gameplay or infra, got "
                            f"{step.kind!r}")
        for ac in step.proves:
            check_ac(ac, f"PLAN[{i}] ({name})")
    for ac, why in (getattr(run_class, "PENDING", None) or {}).items():
        check_ac(ac, "PENDING")
        if not str(why or "").startswith(PENDING_PREFIX):
            problems.append(f"PENDING[{ac}] must read '{PENDING_PREFIX}: <reason>', got {why!r}")
    return problems


def plan_table(run_class):
    """The PLAN as the story's Steps table (Specs/story_playtests_v1.md section 4)."""
    lines = [PLAN_HEADER]
    for i, step in enumerate(run_class.PLAN, 1):
        issue = re.sub(r"(?i)^known issue\s*:\s*", "", str(step.known_issue or ""))
        does = step.does + (f" _(known issue: {issue})_" if issue else "")
        proves = ", ".join(step.proves) or "-"
        lines.append(f"| {i} | `{step.check}` | {pb.md_cell(does)} | {pb.md_cell(proves)} |")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# The run
# ---------------------------------------------------------------------------
class PlaytestRun:
    """One run of a story playtest.  Subclass it, set the class attributes, write steps().

    execute() returns the exit code.  Class attributes (section 2.2): STORY, COVERS, FEATURES,
    TITLE, RUN_SUFFIX, MAP / EXPERIENCE / FRONTEND, SCREENSHOTS, KNOWN_ISSUES, PLAN, PENDING,
    WALK_TOL, TAP_S, and SCRIPT (the script's path for the report; default: the module file).
    """

    STORY = None
    COVERS = ()
    FEATURES = ()
    TITLE = None
    RUN_SUFFIX = None
    MAP = DEFAULT_MAP
    EXPERIENCE = DEFAULT_EXPERIENCE
    FRONTEND = False
    SCREENSHOTS = {}
    KNOWN_ISSUES = ()
    PLAN = ()
    PENDING = {}
    WALK_TOL = 60.0
    TAP_S = 0.1
    SCRIPT = None

    def __init__(self, args):
        self.args = args
        self.mode = "attach" if args.attach_port else "launch"
        self.title = self.TITLE or f"{self.STORY} playtest (Tier 4, real input)"
        self.run_suffix = self.RUN_SUFFIX or ("us" + re.sub(r"\D", "", str(self.STORY or "")))
        self.checks = []
        self.screens = []
        self.warnings = []
        self.bridge = None
        self.run_dir = None
        self.fresh = None
        self.started_at = now_iso()
        self.finished_at = None
        self.exe = None
        self.exe_argv = None
        self.ping = None
        self.focus_start = None
        self.focus_end = None
        self.state_start = None
        self.state_end = None
        self.abort = None      # a lost connection: everything after it is SKIP
        self.blocked = None    # a failed precondition (wrong experience): the rest is SKIP
        self.shot_n = 0
        self.shutdown_note = None
        self.game_log = None
        self.game_log_note = None
        self.log_summary = None
        self.missing_content = None
        self.exit_code = None
        self._plan = {s.check: s for s in self.PLAN if isinstance(s, Step)}

    # --- declarations -------------------------------------------------------------------
    @classmethod
    def expected_experience(cls):
        if cls.FRONTEND and cls.EXPERIENCE == DEFAULT_EXPERIENCE:
            return FRONTEND_EXPERIENCE
        return cls.EXPERIENCE

    @classmethod
    def launch_map(cls):
        return None if cls.FRONTEND else (cls.MAP or DEFAULT_MAP)

    @classmethod
    def script_path(cls):
        path = cls.SCRIPT or getattr(sys.modules.get(cls.__module__), "__file__", None)
        return os.path.abspath(path) if path else None

    def plan_step(self, name):
        return self._plan.get(name)

    # --- checks -------------------------------------------------------------------------
    def log(self, msg):
        log(msg, self.STORY or "playtest")

    def check(self, name, status, detail, kind="gameplay"):
        pb.add_check(self.checks, name, status, detail, kind)
        self.log(f"{status:<4} {name}: {detail}")
        return status in ("PASS", "WARN", "INFO")

    def skip(self, name, why, kind="gameplay"):
        self.check(name, "SKIP", why, kind)
        return False, None

    def _kind(self, name, kind):
        if kind is not None:
            return kind
        step = self.plan_step(name)
        return step.kind if step is not None else "gameplay"

    @staticmethod
    def err_detail(e):
        detail = str(e)
        resp = e.response if isinstance(e.response, dict) else None
        if resp is not None:
            if "values" in resp:
                detail += f"; last values: {dumps(resp.get('values'), 1500)}"
            elif not any(k in resp for k in ("state", "player", "pickups", "pause_menu")):
                text = dumps(resp, 600)
                if len(text) < 600:
                    detail += f"; {text}"
        return detail

    def alive(self):
        b = self.bridge
        return b is not None and b.conn is not None and not b.conn.closed and b.is_alive()

    def _lost(self, name, e):
        """Mark the connection lost (everything after it SKIPs) when `e` says so."""
        if e.kind == "infra" and (e.code in CONNECTION_LOST or not self.alive()):
            self.abort = f"not run: the bridge connection was lost in {name} ({e.code})"

    def attempt(self, name, fn, kind=None, needs=True, needs_why=None):
        """Run fn() -> (status, detail, data) as check `name`.  Returns (ok, data).

        A BridgeError is a FAIL: gameplay when the bridge says so (and the check is a gameplay
        check), infra otherwise; a lost connection SKIPs everything after it.  kind None = the
        PLAN step's kind (gameplay when the check is not in the plan).
        """
        kind = self._kind(name, kind)
        if self.abort or self.blocked:
            return self.skip(name, self.abort or self.blocked, kind)
        if not needs:
            return self.skip(name, needs_why or "not run: a step it depends on failed", kind)
        try:
            status, detail, data = fn()
        except ab.BridgeError as e:
            k = kind if e.kind == "gameplay" else "infra"
            self.check(name, "FAIL", self.err_detail(e), k)
            self._lost(name, e)
            return False, None
        ok = self.check(name, status, detail, kind)
        return ok, data

    # --- step helpers (section 2.5): one check each, 1:1 with bridge calls --------------
    def _wait_then(self, then, timeout_s):
        """wait_until `then` (if given).  Returns (ok, text, response)."""
        if not then:
            return True, "", None
        try:
            r = self.bridge.wait_until(then, timeout_s=timeout_s)
        except ab.BridgeError as e:
            if e.code != "timeout":
                raise
            values = (e.response or {}).get("values")
            return (False, f"but {then} was not true within {timeout_s:g}s; last values: "
                    f"{dumps(values, 1500)}", e.response)
        return True, f"{then} after {r.get('elapsed_s')}s", r

    def _input_then(self, what, resp, then, timeout_s):
        ok, text, waited = self._wait_then(then, timeout_s)
        detail = f"{what}; {text}" if text else what
        return ("PASS" if ok else "FAIL"), detail, {"input": resp, "then": waited}

    def walk(self, check, x, y, tol=None, timeout_s=None, needs=True, needs_why=None, kind=None):
        """walk_to(x, y): PASS within `tol` (WALK_TOL) of the target."""
        tol = self.WALK_TOL if tol is None else tol

        def fn():
            r = self.bridge.walk_to(x, y, tol=tol, timeout_s=timeout_s)
            fx, fy = r["final"]
            return ("PASS", f"reached ({fx:.0f}, {fy:.0f}), {r['dist']:.0f} cm from ({x:.0f}, "
                    f"{y:.0f}), {r['pulses']} pulses, {r.get('side_steps', 0)} side-steps", r)

        return self.attempt(check, fn, kind, needs, needs_why)

    def expect(self, check, condition, timeout_s=5, needs=True, needs_why=None, kind=None):
        """wait_until `condition`: PASS when it is true in time, FAIL with its last values."""
        def fn():
            try:
                r = self.bridge.wait_until(condition, timeout_s=timeout_s)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                values = (e.response or {}).get("values")
                return ("FAIL", f"{condition} not true within {timeout_s:g}s; last values: "
                        f"{dumps(values, 1500)}", e.response)
            return "PASS", f"{condition} after {r.get('elapsed_s')}s", r

        return self.attempt(check, fn, kind, needs, needs_why)

    def press(self, check, key_or_keys, then=None, timeout_s=3, hold_s=None, needs=True,
              needs_why=None, kind=None):
        """key tap (TAP_S), then wait_until `then`."""
        hold_s = self.TAP_S if hold_s is None else hold_s

        def fn():
            r = self.bridge.tap(key_or_keys, hold_s=hold_s)
            return self._input_then(f"tapped {keys_text(key_or_keys)}", r, then, timeout_s)

        return self.attempt(check, fn, kind, needs, needs_why)

    def hold(self, check, key_or_keys, seconds, then=None, timeout_s=3, needs=True,
             needs_why=None, kind=None):
        """key tap held for `seconds`, then wait_until `then`."""
        def fn():
            r = self.bridge.hold(key_or_keys, seconds)
            return self._input_then(f"held {keys_text(key_or_keys)} for {seconds:g}s", r, then,
                                    timeout_s)

        return self.attempt(check, fn, kind, needs, needs_why)

    def axis(self, check, values, hold_s, then=None, timeout_s=3, needs=True, needs_why=None,
             kind=None):
        """axis `values` held for hold_s, then wait_until `then`."""
        def fn():
            r = self.bridge.axis(values, hold_s=hold_s)
            shown = ", ".join(f"{k}={v:g}" for k, v in dict(values).items())
            return self._input_then(f"axis {shown} for {hold_s:g}s", r, then, timeout_s)

        return self.attempt(check, fn, kind, needs, needs_why)

    def click_button(self, check, label=None, button_id=None, then=None, timeout_s=3,
                     needs=True, needs_why=None, kind=None):
        """Aim at a visible button (state), click its center, then wait_until `then`."""
        def fn():
            r = self.bridge.click_button(label=label, button_id=button_id)
            target = r.get("target") or {}
            c = target.get("center") or [0, 0]
            what = (f"clicked '{target.get('label')}' at ({float(c[0]):.0f}, {float(c[1]):.0f}) "
                    f"({target.get('source')}); Slate hit {r.get('hit')}")
            return self._input_then(what, r, then, timeout_s)

        return self.attempt(check, fn, kind, needs, needs_why)

    def click(self, check, x, y, button="Left", then=None, timeout_s=3, hold_s=None, needs=True,
              needs_why=None, kind=None):
        """click at viewport pixel (x, y) (held hold_s, default one frame), then wait `then`."""
        def fn():
            r = self.bridge.click(x, y, button=button, hold_s=hold_s)
            return self._input_then(f"{button} click at ({x:.0f}, {y:.0f}); Slate hit "
                                    f"{r.get('hit')}", r, then, timeout_s)

        return self.attempt(check, fn, kind, needs, needs_why)

    def observe(self, check, sections, predicate, describe, timeout_s=3, radius=1500,
                needs=True, needs_why=None, kind=None):
        """Poll state(sections, radius) until predicate(state); describe(state) is the detail."""
        def text(st):
            return describe(st) if callable(describe) else str(describe)

        def fn():
            try:
                st = self.bridge.wait_for(predicate, timeout_s=timeout_s, sections=sections,
                                          radius=radius)
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                return "FAIL", f"not met within {timeout_s:g}s: {text(e.response or {})}", None
            return "PASS", text(st), st

        return self.attempt(check, fn, kind, needs, needs_why)

    def measure(self, check, fn, needs=True, needs_why=None, kind=None):
        """fn(bridge) -> (status, detail, data): the escape hatch for compound read-only checks."""
        return self.attempt(check, lambda: fn(self.bridge), kind, needs, needs_why)

    def shot(self, label, needs=True, needs_why=None):
        """screenshot -> bridge_NN.png; a near-uniform frame is a WARN (no visual evidence)."""
        name = f"{SCREENSHOT_PREFIX}{label}"

        def take():
            self.shot_n += 1
            path = os.path.join(self.run_dir, f"bridge_{self.shot_n:02d}.png")
            got = self.bridge.screenshot(path)
            entry = {"label": label, "path": got, "expect": self.SCREENSHOTS.get(label, "")}
            try:
                from PIL import Image as PILImage
                with PILImage.open(got) as img:
                    entry["size"] = list(img.size)
                    entry.update(pb.analyze_image(img))
            except ImportError:
                entry["analysis"] = "Pillow not installed"
            except Exception as e:
                entry["analysis"] = f"could not analyze: {e}"
            self.screens.append(entry)
            if entry.get("uniform"):
                return ("WARN", f"{got}: near-uniform{' black' if entry.get('black') else ''} "
                        f"frame (mean {entry.get('mean')}, stddev {entry.get('stddev')}): no visual "
                        "evidence", entry)
            return "PASS", got, entry

        return self.attempt(name, take, kind="infra", needs=needs, needs_why=needs_why)

    def read(self, sections=None, radius=1500):
        """A state dict for computing expectations; records no check ({} when there is none)."""
        if self.abort or self.blocked or self.bridge is None:
            return {}
        try:
            return self.bridge.state(sections, radius=radius)
        except ab.BridgeError as e:
            self.warnings.append(f"could not read state {sections}: {e}")
            self._lost("read", e)
            return {}

    def steps(self):
        """The story's player steps.  Subclasses implement it with the helpers above."""
        raise NotImplementedError("a PlaytestRun subclass must implement steps()")

    # --- the run ------------------------------------------------------------------------
    def execute(self):
        args = self.args
        if self.mode == "launch":
            self.exe_argv = json.loads(args.test_exe_argv) if args.test_exe_argv else None
            if self.exe_argv:
                self.exe = os.path.abspath(args.exe or self.exe_argv[0])
            else:
                self.exe = os.path.abspath(args.exe or pb.DEFAULT_EXE)
                if not os.path.isfile(self.exe):
                    self.log(f"ERROR: game exe not found: {self.exe} -- package first: "
                             "py Scripts/package_playtest.py")
                    return EXIT_INFRA
                self.exe = pb.resolve_exe(self.exe)
            if args.test_skip_freshness:
                self.fresh = {"fresh": None, "skipped": True, "reasons": [],
                              "warnings": ["staleness gate skipped (test launch)"],
                              "build": None, "head": None, "branch": None}
            else:
                self.fresh, code = pb.gate(self.exe, args.allow_stale)
                if code is not None:
                    self.log("STALE BUILD: refusing to run (no run dir written).  Re-package "
                             "with py Scripts/package_playtest.py; --allow-stale is diagnostic "
                             "only.")
                    return code
        self.run_dir = os.path.abspath(args.run_dir) if args.run_dir else pb.new_run_dir(
            self.run_suffix)
        os.makedirs(self.run_dir, exist_ok=True)
        self.log(f"Run dir: {self.run_dir}")
        try:
            # steps() always runs: after a failed launch, a wrong experience or a lost connection
            # its checks are SKIP, so the report still lists everything that was not tested.
            if self.start():
                self.record_start_state()
            self.steps()
        except KeyboardInterrupt:
            self.check("completed", "FAIL", "interrupted by the user", "infra")
        except Exception as e:  # report what we have instead of dying without a report
            traceback.print_exc()
            self.check("completed", "FAIL", f"playtest script error: {type(e).__name__}: {e}",
                       "infra")
        finally:
            self.finish()
        return self.exit_code

    def start(self):
        """Launch or attach, then build_fresh, bridge_ready, experience and map.  True when
        ready."""
        args = self.args
        expected = self.expected_experience()
        if self.mode == "launch":
            self._build_check()
            try:
                self.bridge = ab.Bridge.launch(
                    map=self.launch_map(), expect_experience=expected, frontend=self.FRONTEND,
                    allow_stale=args.allow_stale, detach=args.keep_running,
                    run_suffix=self.run_suffix, timeout=args.timeout, extra_args=args.extra_arg,
                    exe=self.exe, exe_argv=self.exe_argv, skip_freshness=args.test_skip_freshness,
                    run_dir=self.run_dir, ready_timeout=args.ready_timeout)
            except ab.BridgeError as e:
                self.game_log = os.path.join(self.run_dir, "game.log")
                if e.kind == "gameplay":
                    self.check("launch", "PASS", "the game started (it was shut down after the "
                               "failure below)", "infra")
                    self.check("experience", "FAIL", str(e), "gameplay")
                else:
                    self.check("launch", "FAIL", str(e), "infra")
                self.abort = f"not run: the launch failed ({e.code})"
                return False
            b = self.bridge
            self.game_log = b.game_log
            self.warnings.extend(b.warnings)
            if b.detached:
                lifetime = "detached (--keep-running)"
            else:
                lifetime = "kill-on-close job" if b.job else "NO kill-on-close job"
            self.check("launch", "PASS", f"pid {b.pid}, bridge {b.host}:{b.port}, {lifetime}",
                       "infra")
        else:
            try:
                self.bridge = ab.Bridge.attach(args.attach_port, token=args.token,
                                               run_dir=self.run_dir, wait_ready=True,
                                               timeout=args.ready_timeout)
            except ab.BridgeError as e:
                self.check("build_fresh", "SKIP", "no bridge to check", "infra")
                self.check("attach", "FAIL", str(e), "infra")
                self.abort = f"not run: could not attach ({e.code})"
                return False
            b = self.bridge
            self._attached_build_check(b.ping_info or {})
            self.check("attach", "PASS", f"bridge 127.0.0.1:{b.port}, pid {b.pid} (this script "
                       "did not launch the game; it is left running)", "infra")
        self.ping = dict(self.bridge.ping_info or {})
        ping = self.ping
        self.check("bridge_ready", "PASS" if ping.get("ready") else "FAIL",
                   f"ready={ping.get('ready')} (experience_loaded={ping.get('experience_loaded')}, "
                   f"loading_screen={ping.get('loading_screen')}, has_pawn={ping.get('has_pawn')}, "
                   f"map={ping.get('map')})", "infra")
        got = pb.experience_name(ping.get("experience"))
        if got != expected:
            self.check("experience", "FAIL", f"the bridge reports experience '{got or None}', "
                       f"expected {expected}", "gameplay")
            self.blocked = f"not run: wrong experience ({got or None})"
            return False
        self.check("experience", "PASS", f"{got} on {ping.get('map')}", "gameplay")
        if not self._map_check(ping):
            return False
        if not ping.get("ready"):
            self.abort = "not run: the bridge never became ready"
            return False
        return True

    def _map_check(self, ping):
        """The 'map' kit check: ping.map must be launch_map().  The test maps load the same
        experience as DevTestMap, so the experience check alone cannot tell a script on the wrong
        map.  A mismatch blocks the plan like a wrong experience (attach mode too).  A front-end
        run passes no map argument: recorded INFO, not compared.  True when the run may go on."""
        loaded = ping.get("map")
        expected = self.launch_map()
        if expected is None:
            self.check("map", "INFO", f"front-end run: the map is not compared (loaded "
                       f"{loaded or None})", "gameplay")
            return True
        if map_key(loaded) != map_key(expected):
            self.check("map", "FAIL", f"loaded {loaded or None}, expected {expected}", "gameplay")
            self.blocked = f"not run: wrong map ({loaded or None}, expected {expected})"
            return False
        self.check("map", "PASS", f"loaded {loaded}", "gameplay")
        return True

    def record_start_state(self):
        try:
            self.state_start = self.bridge.state(["game", "focus", "player", "input"])
            self.focus_start = self.state_start.get("focus")
        except ab.BridgeError as e:
            self.warnings.append(f"could not read the starting state: {e}")
            self._lost("the starting state read", e)

    def _build_check(self):
        f = self.fresh or {}
        build = f.get("build") or {}
        if f.get("skipped"):
            self.check("build_fresh", "WARN", "staleness gate skipped (test launch): not "
                       "evidence", "infra")
        elif f.get("fresh") and build.get("dirty"):
            self.check("build_fresh", "WARN", f"package matches HEAD and the working tree, but it "
                       f"was built from a DIRTY tree (commit {pb.short(build.get('commit'))}, "
                       f"{build.get('dirty_count')} uncommitted file(s)): diagnostic only, NOT "
                       "acceptance evidence -- commit, re-package, re-run", "infra")
        elif f.get("fresh"):
            self.check("build_fresh", "PASS", f"package matches HEAD and the working tree "
                       f"(commit {pb.short(build.get('commit'))})", "infra")
        else:
            self.check("build_fresh", "WARN", "STALE (allowed by --allow-stale: diagnostic only, "
                       "NOT acceptance evidence): " + "; ".join(str(r) for r in
                                                                 f.get("reasons") or []), "infra")

    def _attached_build_check(self, ping):
        pid = ping.get("pid")
        image = ab.process_image(pid) if pid else None
        info_path = pb.find_build_info(image) if image else None
        try:
            with pb.log_stream(_out()):
                self.fresh = pb.check_build_freshness(info_path)
        except Exception as e:  # git missing etc.
            self.fresh = {"fresh": False, "reasons": [f"freshness check failed: {e}"],
                          "warnings": [], "build": None, "head": None, "branch": None}
        self.fresh["attached_exe"] = image
        self.exe = image
        if self.fresh.get("fresh") and (self.fresh.get("build") or {}).get("dirty"):
            self.check("build_fresh", "WARN", f"the attached game ({image}) was packaged from a "
                       "DIRTY tree: diagnostic only, NOT acceptance evidence", "infra")
        elif self.fresh.get("fresh"):
            self.check("build_fresh", "PASS", f"the attached game ({image}) is a fresh package "
                       f"(commit {pb.short((self.fresh.get('build') or {}).get('commit'))})",
                       "infra")
        else:
            self.check("build_fresh", "WARN", f"attached game ({image or 'exe unknown'}): build "
                       "not verified fresh, so this run is diagnostic only: "
                       + "; ".join(str(r) for r in self.fresh.get("reasons") or []), "infra")

    # --- the end --------------------------------------------------------------------------
    def complete_plan(self):
        """Every PLAN check that was never recorded becomes SKIP 'not reached'."""
        recorded = {c["name"] for c in self.checks}
        for step in self.PLAN:
            if isinstance(step, Step) and step.check not in recorded:
                self.check(step.check, "SKIP", NOT_REACHED, step.kind)

    def finish(self):
        self.complete_plan()
        b = self.bridge
        if b is not None and not self.abort and self.alive():
            try:
                self.state_end = b.state(["game", "focus", "player", "inventory", "pause_menu"])
                self.focus_end = self.state_end.get("focus")
            except ab.BridgeError as e:
                self.warnings.append(f"could not read the final state: {e}")
        if b is not None:
            if self.mode == "launch":
                crash = b.crash_lines()
                alive = b.is_alive()
                if crash:
                    self.check("no_crash", "FAIL", f"crash marker in game.log: {crash[0]}", "infra")
                elif not alive:
                    code = b.proc.returncode if b.proc is not None else None
                    self.check("no_crash", "FAIL", f"the game exited on its own (code {code}) "
                               "before the playtest finished", "infra")
                else:
                    self.check("no_crash", "PASS", "no crash markers; the game was alive at the "
                               "end", "infra")
            else:
                try:
                    b.ping()
                    self.check("no_crash", "PASS", "the attached bridge still answers ping",
                               "infra")
                except ab.BridgeError as e:
                    self.check("no_crash", "FAIL", f"the attached bridge stopped answering: {e}",
                               "infra")
            self.shutdown()
        self.collect_log()
        self.finished_at = now_iso()
        self.exit_code = pb.exit_code_for(self.checks)
        try:
            self.write_reports()
        except OSError as e:
            self.log(f"ERROR: could not write the report: {e}")
            self.exit_code = EXIT_INFRA
        self.print_summary()

    def shutdown(self):
        b = self.bridge
        if self.mode == "attach":
            b.close()
            self.shutdown_note = "attached game left running (connection closed)"
        elif self.args.keep_running:
            b.close()  # detached launch: the game keeps running
            self.shutdown_note = (f"left running (--keep-running): pid {b.pid}; quit it with "
                                  f"py Scripts/agent_bridge.py --port {b.port} --token {b.token} "
                                  "quit")
        else:
            r = b.quit()
            self.shutdown_note = f"quit: exit code {r.get('exit_code')}; {r.get('shutdown')}"
            b.close()
        self.log(f"Shutdown: {self.shutdown_note}")

    def collect_log(self):
        path = self.game_log
        target = os.path.join(self.run_dir, "game.log")
        if self.mode == "attach":
            # agent_bridge.py launch writes a session file: snapshot that game's log if it is ours
            try:
                session = ab.load_session(ab.SESSION_FILE)
                src = os.path.join(session.get("run_dir") or "", "game.log")
                if int(session.get("port") or 0) == int(self.args.attach_port) and os.path.isfile(src):
                    shutil.copyfile(src, target)
                    path = target
                    self.game_log_note = f"snapshot of {src} (the attached game is still running)"
            except (ab.BridgeError, OSError, ValueError):
                pass
            if path is None:
                self.game_log_note = ("not available: attached to a game this script did not "
                                      "launch (no matching agent_bridge session)")
        if path and os.path.isfile(path):
            self.game_log = path
            self.log_summary = pb.summarize_log(path)
            self.missing_content = pb.find_missing_content(path)
            self.check("content_cooked", "FAIL" if self.missing_content else "PASS",
                       f"{len(self.missing_content)} /MordecaiCore package(s) requested but not in "
                       "the build" + (": " + ", ".join(self.missing_content[:8])
                                      if self.missing_content else ""), "gameplay")
            s = self.log_summary
            self.check("log_errors", "INFO", f"{s['errors']} error(s), {s['warnings']} "
                       "warning(s) in game.log (informational)", "gameplay")
        elif self.game_log_note is None:
            self.game_log_note = "game.log was not written"

    # --- evidence and acceptance ------------------------------------------------------------
    def evidence(self):
        reasons = []
        if self.mode == "attach":
            reasons.append("attached to a game this script did not launch (always diagnostic)")
        f = self.fresh or {}
        if f.get("skipped"):
            reasons.append("the staleness gate was skipped (test launch)")
        elif not f.get("fresh"):
            reasons.append("STALE or unverified build")
        elif (f.get("build") or {}).get("dirty"):
            reasons.append("the package was built from a dirty working tree")
        if self.args.allow_stale:
            # CLAUDE.md: never --allow-stale for acceptance evidence, even when the build is fresh
            reasons.append("run with --allow-stale" + (" (it does not apply with --attach-port)"
                                                       if self.mode == "attach" else ""))
        if reasons:
            return "DIAGNOSTIC ONLY -- not acceptance evidence: " + "; ".join(reasons)
        if self.exit_code == EXIT_PASS:
            return ("ACCEPTANCE CANDIDATE -- fresh package launched by this script; open every "
                    "screenshot and describe it before citing this run")
        return "FAILED on a fresh package -- the failures below are real findings"

    def is_candidate(self):
        """True only for a launched, fresh, clean-tree package, no --allow-stale, exit 0."""
        return self.evidence().startswith("ACCEPTANCE CANDIDATE")

    def undeclared_checks(self):
        """Recorded checks that are neither in PLAN nor kit checks (the script needs a Step)."""
        out = []
        for c in self.checks:
            name = c["name"]
            if (name not in self._plan and name not in KIT_CHECKS
                    and not name.startswith(SCREENSHOT_PREFIX) and name not in out):
                out.append(name)
        return out

    def _known(self, name):
        step = self.plan_step(name)
        return step.known_issue if step is not None else None

    def known_failures(self):
        return [c["name"] for c in self.checks if c["status"] == "FAIL" and self._known(c["name"])]

    def new_failures(self):
        return [c["name"] for c in self.checks
                if c["status"] == "FAIL" and not self._known(c["name"])]

    def fixed_candidates(self):
        return [c["name"] for c in self.checks if c["status"] == "PASS" and self._known(c["name"])]

    def acceptance(self):
        """report.json["acs"]: one entry per AC named in a Step's proves or in PENDING."""
        entries = {}

        def entry(story, ac_id):
            return entries.setdefault((story, ac_id), {
                "id": ac_id, "story": story, "checks": [], "status": None, "accepted": False,
                "known_issue": None, "pending": None})

        for step in self.PLAN:
            if not isinstance(step, Step):
                continue
            for ac in step.proves:
                e = entry(*split_ac(ac, self.STORY))
                if step.check not in e["checks"]:
                    e["checks"].append(step.check)
                if step.known_issue and not e["known_issue"]:
                    e["known_issue"] = step.known_issue
        for ac, why in (self.PENDING or {}).items():
            entry(*split_ac(ac, self.STORY))["pending"] = why
        candidate = self.is_candidate()
        for e in entries.values():
            recs = [c for c in self.checks if c["name"] in e["checks"]]
            statuses = [c["status"] for c in recs]
            recorded = {c["name"] for c in recs}
            if e["pending"]:
                status = "PENDING"
            elif "FAIL" in statuses:
                status = "FAIL"
            elif "SKIP" in statuses or any(n not in recorded for n in e["checks"]):
                status = "NOT RUN"
            elif "PASS" in statuses:
                status = "PASS"
            else:
                status = "NOT RUN"  # only WARN / INFO: nothing passed
            e["status"] = status
            # Never accepted by an infra check alone, and never by a diagnostic run.
            e["accepted"] = bool(status == "PASS" and candidate and any(
                c["status"] == "PASS" and c["kind"] == "gameplay" for c in recs))
        return list(entries.values())

    # --- reports --------------------------------------------------------------------------
    def report_extra(self):
        """Hook: a dict merged into report.json (story-specific sections)."""
        return {}

    def md_sections(self):
        """Hook: markdown lines inserted into report.md after the Checks and AC tables."""
        return []

    def report(self):
        f = self.fresh or {}
        counts = {s: sum(1 for c in self.checks if c["status"] == s)
                  for s in ("PASS", "WARN", "FAIL", "SKIP", "INFO")}
        b = self.bridge
        script = self.script_path()
        if script:
            try:
                script = os.path.relpath(script, pb.PROJECT_ROOT)
            except ValueError:  # another drive
                pass
        rep = {
            "story": self.STORY, "covers": list(self.COVERS), "features": list(self.FEATURES),
            "title": self.title, "script": script,
            "result": "PASS" if self.exit_code == EXIT_PASS else "FAIL",
            "exit_code": self.exit_code, "evidence": self.evidence(), "counts": counts,
            "run_dir": self.run_dir, "mode": self.mode, "started_at": self.started_at,
            "finished_at": self.finished_at, "exe": self.exe,
            "command_line": b.command_line if b is not None else None,
            "map": self.launch_map(), "frontend": bool(self.FRONTEND),
            "expected_experience": self.expected_experience(),
            "allow_stale": self.args.allow_stale, "attach_port": self.args.attach_port,
            "build": {"fresh": f.get("fresh"), "skipped": bool(f.get("skipped")),
                      "reasons": f.get("reasons") or [], "warnings": f.get("warnings") or [],
                      "build": f.get("build"), "head": f.get("head"), "branch": f.get("branch")},
            "ping": self.ping, "checks": self.checks,
            "plan": [s.to_dict() for s in self.PLAN if isinstance(s, Step)],
            "acs": self.acceptance(), "pending": dict(self.PENDING or {}),
            "undeclared_checks": self.undeclared_checks(),
            "known_failures": self.known_failures(), "new_failures": self.new_failures(),
            "fixed_candidates": self.fixed_candidates(),
            "focus_start": self.focus_start, "focus_end": self.focus_end,
            "state_start": self.state_start, "state_end": self.state_end,
            "screenshots": self.screens,
            "game_log": self.game_log, "game_log_note": self.game_log_note,
            "log": {k: v for k, v in (self.log_summary or {}).items() if k != "tail"} or None,
            "missing_content": self.missing_content, "known_issues": list(self.KNOWN_ISSUES),
            "shutdown": self.shutdown_note, "warnings": self.warnings,
            "transcript": os.path.join(self.run_dir, ab.TRANSCRIPT_NAME),
        }
        rep.update(self.report_extra() or {})
        return rep

    def _result_cell(self, ch):
        known = self._known(ch["name"])
        if ch["status"] == "FAIL":
            return "FAIL (known)" if known else "FAIL (NEW)"
        if ch["status"] == "PASS" and known:
            return "PASS (FIXED?)"
        return ch["status"]

    def write_reports(self):
        rep = self.report()
        with open(os.path.join(self.run_dir, "report.json"), "w", encoding="utf-8",
                  newline="\n") as fh:
            json.dump(rep, fh, indent=2, ensure_ascii=False, default=str)
            fh.write("\n")
        md = pb.md_cell
        f = rep["build"]
        build = f.get("build") or {}
        c = rep["counts"]
        lines = [
            f"# {self.title}: {rep['result']}",
            "",
            f"- Result: **{rep['result']}** (exit {rep['exit_code']}): {c['PASS']} PASS, "
            f"{c['WARN']} WARN, {c['FAIL']} FAIL, {c['SKIP']} SKIP",
            f"- Evidence: **{rep['evidence']}**",
            f"- Story: {rep['story']}" + (f" (also covers {', '.join(rep['covers'])})"
                                          if rep["covers"] else "")
            + (f"; features: {', '.join(rep['features'])}" if rep["features"] else ""),
            f"- Run dir: `{rep['run_dir']}`",
            f"- Mode: {rep['mode']}" + (f" (127.0.0.1:{rep['attach_port']})"
                                        if rep["mode"] == "attach" else
                                        " (this script launched the packaged game)"),
            f"- Started: {rep['started_at']}  Finished: {rep['finished_at']}",
            f"- Exe: `{rep['exe']}`",
            f"- Command: `{rep['command_line']}`" if rep["command_line"] else
            "- Command: n/a (this script did not launch the game)",
            f"- Map: {rep['map'] or '(front end)'}  Expected experience: "
            f"{rep['expected_experience']}",
            "",
            "## Build",
            "",
        ]
        if build:
            lines.append(f"- Built from commit `{pb.short(build.get('commit'))}` on "
                         f"`{build.get('branch')}`, dirty={build.get('dirty')}, "
                         f"built_at={build.get('built_at')}, config={build.get('config')}")
        if f.get("head"):
            lines.append(f"- Current HEAD `{pb.short(f.get('head'))}` on `{f.get('branch')}`")
        allow = " (run with --allow-stale)" if rep["allow_stale"] else ""
        if f.get("skipped"):
            lines.append("- Freshness: **not checked (test launch)**" + allow)
        elif f.get("fresh") and build.get("dirty"):
            lines.append("- Freshness: matches HEAD and the working tree, but **built from a DIRTY "
                         f"tree ({build.get('dirty_count')} uncommitted file(s)) -- diagnostic "
                         "only, not acceptance evidence**" + allow)
        elif f.get("fresh"):
            lines.append("- Freshness: OK (the package matches the code under test)" + allow)
        else:
            lines.append("- Freshness: **STALE BUILD / unverified -- not acceptance evidence**"
                         + allow)
            for r in f.get("reasons") or []:
                lines.append(f"  - {r}")
        for w in f.get("warnings") or []:
            lines.append(f"- Warning: {w}")
        if self.ping:
            pbuild = self.ping.get("build") if isinstance(self.ping.get("build"), dict) else None
            reported = (f"build commit `{pb.short(pbuild.get('commit'))}`" if pbuild
                        else "no build_info.json (ping.build is null)")
            lines.append(f"- The game reports {reported}, bridge_version "
                         f"{self.ping.get('bridge_version')}, pid {self.ping.get('pid')}")

        lines += ["", "## Checks", "",
                  "A FAIL is marked `known` when its PLAN step cites a filed bug (known_issue), "
                  "`NEW` otherwise; a PASS of a known issue is marked `FIXED?` (re-check the "
                  "backlog).", "", "| # | Check | Result | Kind | Detail |", "|---|---|---|---|---|"]
        for i, ch in enumerate(rep["checks"], 1):
            lines.append(f"| {i} | {md(ch['name'])} | {self._result_cell(ch)} | {ch['kind']} | "
                         f"{md(ch['detail'])} |")
        if rep["undeclared_checks"]:
            lines += ["", f"- WARN: undeclared check(s), not in PLAN: "
                          f"{', '.join(rep['undeclared_checks'])} -- fix the script (add a Step "
                          "for each, or rename the check)"]
        for step in self.PLAN:
            if isinstance(step, Step) and step.known_issue:
                lines.append(f"- Known issue on `{step.check}`: {step.known_issue}")

        lines += ["", "## Acceptance criteria", "",
                  "An AC is accepted only by a PASS in an ACCEPTANCE CANDIDATE run (see "
                  "Evidence); a diagnostic run accepts nothing, and PENDING ACs are never "
                  "claimed.", "", "| AC | Checks | Status | Accepted |", "|---|---|---|---|"]
        if rep["acs"]:
            for a in rep["acs"]:
                status = a["status"]
                if a["pending"]:
                    status += f" ({a['pending']})"
                elif a["known_issue"] and a["status"] == "FAIL":
                    status += f" (known: {a['known_issue']})"
                checks = ", ".join(a["checks"]) or "-"
                lines.append(f"| {md(qualified_ac(a['story'], a['id'], self.STORY))} | "
                             f"{md(checks)} | {md(status)} | "
                             f"{'yes' if a['accepted'] else 'no'} |")
        else:
            lines.append("| (no AC named in PLAN or PENDING) | - | - | no |")

        lines += self.md_sections() or []

        lines += ["", "## Environment (state.focus)", "",
                  "Records whether the run had a focused window, a locked desktop or the "
                  "virtual cursor.", "",
                  f"- Start: `{dumps(rep['focus_start'])}`",
                  f"- End: `{dumps(rep['focus_end'])}`"]

        lines += ["", "## Screenshots", "",
                  "Open every screenshot with the Read tool and state what is actually on "
                  "screen. A screenshot nobody looked at is not evidence.", ""]
        if rep["screenshots"]:
            for s in rep["screenshots"]:
                flag = ""
                if s.get("uniform"):
                    flag = (f"  **WARNING: near-uniform{' black' if s.get('black') else ''} "
                            f"frame (mean {s.get('mean')}, stddev {s.get('stddev')})**")
                lines.append(f"- `{s['path']}` ({s['label']}): should show {s['expect']}{flag}")
        else:
            lines.append("- (none captured)")

        lines += ["", "## Log", ""]
        if rep["log"]:
            s = rep["log"]
            lines.append(f"- game.log: `{rep['game_log']}` ({s.get('lines')} lines)"
                         + (f" -- {rep['game_log_note']}" if rep["game_log_note"] else ""))
            lines.append(f"- Errors: {s.get('errors')}  Warnings: {s.get('warnings')}")
            lines.append("- Missing /MordecaiCore content: "
                         + (", ".join(rep["missing_content"]) if rep["missing_content"]
                            else "none"))
            if s.get("top_errors"):
                lines += ["", "### Top errors", ""]
                for i, e in enumerate(s["top_errors"], 1):
                    lines.append(f"{i}. (x{e['count']}) `{e['line']}`")
        else:
            lines.append(f"- game.log: {rep['game_log_note'] or 'not available'}")
        lines.append(f"- Bridge transcript (every request and response): `{rep['transcript']}`")
        lines.append(f"- Shutdown: {rep['shutdown'] or 'n/a'}")

        if rep["known_issues"]:
            lines += ["", "## Known issues this run exercises", ""]
            lines += [f"- {k}" for k in rep["known_issues"]]
        if rep["warnings"]:
            lines += ["", "## Warnings", ""] + [f"- {w}" for w in rep["warnings"]]
        lines += ["", pb.AGENT_LINE, ""]
        with open(os.path.join(self.run_dir, "report.md"), "w", encoding="utf-8",
                  newline="\n") as fh:
            fh.write("\n".join(lines))

    def print_summary(self):
        out = _out()
        bar = "=" * 60
        result = "PASS" if self.exit_code == EXIT_PASS else "FAIL"
        print(bar, file=out)
        print(f"{self.title}: {result} (exit {self.exit_code})", file=out)
        print(bar, file=out)
        for c in self.checks:
            print(f"  {self._result_cell(c):<4} {c['name']}: {c['detail']}", file=out)
        undeclared = self.undeclared_checks()
        if undeclared:
            print(f"WARN: undeclared check(s), not in PLAN: {', '.join(undeclared)}", file=out)
        acs = self.acceptance()
        if acs:
            passed = sum(1 for a in acs if a["status"] == "PASS")
            accepted = sum(1 for a in acs if a["accepted"])
            print(f"Acceptance criteria: {passed}/{len(acs)} PASS, {accepted} accepted", file=out)
        print(f"Evidence: {self.evidence()}", file=out)
        print(f"Report: {os.path.join(self.run_dir, 'report.md')}", file=out)
        for s in self.screens:
            print(f"Screenshot: {s['path']}", file=out)
        print(pb.AGENT_LINE, file=out, flush=True)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def parse_args(run_class, argv=None):
    script = run_class.script_path()
    parser = argparse.ArgumentParser(
        prog=os.path.basename(script) if script else None,
        description=f"{run_class.STORY} playtest with real input (Tier 4). Exit 0 pass, 1 "
                    "gameplay FAIL, 2 infra.")
    parser.add_argument("--allow-stale", action="store_true",
                        help="run a STALE package (diagnostic only; recorded in the report)")
    parser.add_argument("--keep-running", action="store_true",
                        help="leave the launched game running afterwards (no kill-on-close job)")
    parser.add_argument("--attach-port", type=int, default=None,
                        help="play a game whose bridge already listens on this port")
    parser.add_argument("--token", default=None, help="expected ping token (with --attach-port)")
    parser.add_argument("--timeout", type=float, default=240,
                        help="experience load timeout in seconds (240)")
    parser.add_argument("--ready-timeout", type=float, default=ab.READY_TIMEOUT_S,
                        help=argparse.SUPPRESS)
    parser.add_argument("--exe", default=None, help="game binary (default: the packaged playtest)")
    parser.add_argument("--plan", action="store_true",
                        help="print the PLAN as the story's Steps table (markdown) and exit; no "
                             "game")
    parser.add_argument("--run-dir", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--extra-arg", action="append", default=[], help=argparse.SUPPRESS)
    # Test-only switches (tests/fake_game.py stands in for the game).
    parser.add_argument("--test-exe-argv", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-skip-freshness", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    if args.attach_port is not None and (args.allow_stale or args.keep_running):
        log("NOTE: --allow-stale / --keep-running do not apply with --attach-port (the attached "
            "game is checked, never launched, and always left running)",
            run_class.STORY or "playtest")
    return args


def main(run_class, argv=None):
    """Parse the common CLI and run `run_class` (a PlaytestRun subclass).  Returns the exit code."""
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = parse_args(run_class, argv)
    tag = run_class.STORY or "playtest"
    problems = plan_problems(run_class)
    if problems:
        for p in problems:
            log(f"ERROR: bad declaration: {p}", tag)
        log("Fix the script's PLAN / class attributes (Specs/story_playtests_v1.md section 2).",
            tag)
        return EXIT_INFRA
    if args.plan:
        if _LOG_STREAM is None:
            try:  # markdown is UTF-8 (the header's arrow), also through a pipe to a cp1252 parent
                sys.stdout.reconfigure(encoding="utf-8")
            except (AttributeError, ValueError):
                pass
        print(plan_table(run_class), file=_out(), flush=True)
        return EXIT_PASS
    return run_class(args).execute()
