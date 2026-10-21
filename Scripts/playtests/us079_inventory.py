#!/usr/bin/env python3
"""US-079 Inventory Playtest -- Tier 4: the inventory vertical slice, played with REAL input

The real-input version of Scripts/verify_pie_inventory.py (Specs/agent_playtest_plan_v1.md
section 2.4, Specs/agent_bridge_v1.md section 8).  It drives the PACKAGED game through the
in-game agent bridge (Scripts/agent_bridge.py).  Every action is a key or mouse event injected
into Slate, the path a player's keyboard and mouse take; nothing calls game functions, and
bridge.jsonl in the run dir is the audit trail of every request.

Steps (every step is a check: PASS, FAIL, SKIP or WARN):
  1. Launch DevTestMap (STALE gate first).  Expect B_MordecaiArenaExperience, the bridge ready,
     and the combat HUD on screen with its '[I] Inventory' hint.  Record state.focus, screenshot.
  2. The 6 pickups, route Potion (400,0) -> IronOre (300,300) -> TownStone (0,420) ->
     UpgradeKey (-300,300) -> Amulet (-400,0) -> waypoint (-250,-150) -> RustySword (-300,-300).
     For each: walk_to it, wait until it is the focused pickup, assert the HUD prompt renders
     '[F] Pick up <focused display name>' and is visible, tap F, wait until it leaves the world
     (a valid pickups section, never an invalid one) AND inventory.count or ledger_count goes
     up (both counts are recorded in the check).
  3. Assert 4 inventory items and 2 ledger resources, by id and by name.
  4. Tap I: the pause menu opens on the inventory tab with 4 rendered rows, 2 ledger rows and an
     Identify button on the amulet row.  Screenshot.
  5. Click the amulet's Identify button (a Slate click at its center): its row reads
     'Mysterious Amulet' and the item is identified.  Screenshot.
  6. Tap Escape: the pause menu closes within 3 s (UMordecaiPauseMenuWidget is a CommonUI back
     handler, AC-069.10).  Screenshot.  A FAIL here is a REGRESSION of that fix
     (Specs/agent_bridge_v1.md section 9), not a known issue.
A failed check SKIPs the checks that depend on it; independent checks still run.  A lost bridge
connection SKIPs everything after it.

Usage:
  py Scripts/playtests/us079_inventory.py                    fresh package: launch, play, quit
  py Scripts/playtests/us079_inventory.py --allow-stale      STALE package: diagnostic only (recorded)
  py Scripts/playtests/us079_inventory.py --keep-running     leave the game running afterwards
  py Scripts/playtests/us079_inventory.py --attach-port N [--token T]
        play a game that is already running (py Scripts/agent_bridge.py launch, or
        Scripts/tests/fake_bridge.py); it is left running.  An attached run is ALWAYS
        diagnostic only, never acceptance evidence, even on a fresh package: this script did
        not launch the game, so its state may not be pristine.  The build is still checked
        through the process's exe and recorded.  Acceptance needs this script to launch a fresh
        package built from a clean, committed tree itself (no --attach-port, no --allow-stale).
Evidence: report.md's Evidence line reads ACCEPTANCE CANDIDATE only for a launched, fresh,
clean-tree package without --allow-stale that exited 0; every other run is DIAGNOSTIC ONLY.

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-us079/ with report.md, report.json, game.log,
bridge.jsonl and bridge_NN.png.  Open every screenshot with the Read tool and state what is on
screen.  A STALE build without --allow-stale stops before any run dir is created, as
playtest_boot.py does.

Exit codes:
  0 -- every check passed (WARN allowed)
  1 -- a gameplay check FAILED (the game did not do what a player expects)
  2 -- infrastructure problem (STALE BUILD, launch failure, no connection, crash)
"""

import argparse
import datetime
import json
import os
import shutil
import sys
import time
import traceback

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:  # sys.path[0] is Scripts/playtests
    sys.path.insert(0, SCRIPTS_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402

STORY = "US-079"
TITLE = "US-079 inventory playtest (Tier 4, real input)"
RUN_SUFFIX = "us079"
MAP = pb.DEFAULT_MAP
EXPERIENCE = pb.DEFAULT_EXPERIENCE
HUD_HINT = "[I] Inventory"
PROMPT_PREFIX = "[F] Pick up "
WALK_TOL = 60.0
TAP_S = 0.1

HUD_TIMEOUT_S = 5.0
FOCUS_TIMEOUT_S = 3.0
PROMPT_TIMEOUT_S = 2.0
PICKUP_TIMEOUT_S = 3.0
MENU_TIMEOUT_S = 5.0
ROWS_TIMEOUT_S = 3.0
BUTTON_TIMEOUT_S = 2.0
IDENTIFY_TIMEOUT_S = 3.0
ESCAPE_TIMEOUT_S = 3.0

# The six DevTestMap pickups (DevTestMap.umap / place_test_pickups.py), in route order.  The
# waypoint keeps the walk to the sword clear of Box_02 (X -450..-350, Y -350..-250).
ROUTE = (
    {"name": "HealthPotion", "label": "TestPickup_HealthPotion", "target": (400.0, 0.0),
     "item_id": "TestItem_HealthPotion", "display_name": "Health Potion"},
    {"name": "IronOre", "label": "TestPickup_IronOre", "target": (300.0, 300.0),
     "item_id": "TestItem_IronOre", "display_name": "Iron Ore"},
    {"name": "TownStone", "label": "TestPickup_TownStone", "target": (0.0, 420.0),
     "item_id": "TestItem_TownStone", "display_name": "Town Stone"},
    {"name": "UpgradeKey", "label": "TestPickup_UpgradeKey", "target": (-300.0, 300.0),
     "item_id": "TestItem_UpgradeKey", "display_name": "Upgrade Key"},
    {"name": "MysteriousAmulet", "label": "TestPickup_MysteriousAmulet", "target": (-400.0, 0.0),
     "item_id": "TestItem_MysteriousAmulet", "display_name": "Unidentified Purple Amulet"},
    {"name": "RustySword", "label": "TestPickup_RustySword", "target": (-300.0, -300.0),
     "item_id": "TestItem_RustySword", "display_name": "Rusty Sword",
     "waypoints": ((-250.0, -150.0),)},
)
# After all six: 4 inventory instances (item_id -> display name) and 2 ledger entries.
EXPECTED_ITEMS = {"TestItem_UpgradeKey": "Upgrade Key", "TestItem_RustySword": "Rusty Sword",
                  "TestItem_MysteriousAmulet": "Unidentified Purple Amulet",
                  "TestItem_HealthPotion": "Health Potion"}
EXPECTED_LEDGER = {"TestItem_IronOre": 1, "TestItem_TownStone": 1}
AMULET_ITEM_ID = "TestItem_MysteriousAmulet"
AMULET_IDENTIFIED_NAME = "Mysterious Amulet"
IDENTIFY_LABEL = "Identify"

SCREENSHOTS = {
    "start": "the Mordecai diorama arena (fixed overhead camera), the player at the centre, the "
             "combat HUD (bars top-left, 'Kills: 0' top-right, '[I] Inventory' bottom-centre) "
             "and the six coloured pickups",
    "menu": "the pause menu ('Paused', tab bar) on the Inventory tab: 4 item rows (Upgrade Key, "
            "Rusty Sword, '? Unidentified Purple Amulet' with an Identify button, Health "
            "Potion) and 'Stored Resources' with Iron Ore x1 and Town Stone x1",
    "identified": "the same inventory tab with the amulet row now reading 'Mysterious Amulet' "
                  "and no Identify button",
    "after_escape": "the arena with the HUD (pause menu closed)",
}
KNOWN_ISSUES = (
    "I does not close the pause menu (no UI action binding); Escape closes it via the CommonUI "
    "back handler.",
)
CONNECTION_LOST = frozenset({"disconnected", "no_response", "closed", "connect_failed", "crash",
                             "exited"})
EXIT_PASS, EXIT_FAIL, EXIT_INFRA = pb.EXIT_PASS, pb.EXIT_FAIL, pb.EXIT_INFRA

_LOG_STREAM = None


def set_log_stream(stream):
    """Send this script's log lines to `stream` (None = sys.stdout).  Returns the previous one."""
    global _LOG_STREAM
    previous = _LOG_STREAM
    _LOG_STREAM = stream
    return previous


def log(msg):
    print(f"[US-079] {msg}", file=_LOG_STREAM if _LOG_STREAM is not None else sys.stdout,
          flush=True)


def lit(text):
    """A wait_until string literal (double-quoted, \\ and \" escaped)."""
    return '"' + str(text).replace("\\", "\\\\").replace('"', '\\"') + '"'


def _now():
    return datetime.datetime.now().isoformat(timespec="seconds")


def _dumps(obj, limit=None):
    text = json.dumps(obj, ensure_ascii=False, separators=(",", ":"), default=str)
    if limit and len(text) > limit:
        text = text[:limit] + "..."
    return text


def _norm_id(value):
    return str(value or "").strip().strip("{}").replace("-", "").lower()


def _world_labels(st):
    """Labels in state.pickups.world.  Callers must check pickups.valid first: an invalid
    section (no world / no pawn) has no 'world' list, which is NOT "the pickup left"."""
    world = _section(st, "pickups").get("world")
    return [p.get("label") for p in (world if isinstance(world, list) else [])
            if isinstance(p, dict)]


def _stored_counts(inv):
    """(inventory.count, inventory.ledger_count) of an inventory section; None if not a number."""
    def num(v):
        return v if isinstance(v, (int, float)) and not isinstance(v, bool) else None
    return num(inv.get("count")), num(inv.get("ledger_count"))


def _grew(n0, l0, n, l):
    """True if the item count or the ledger entry count went up."""
    return (n0 is not None and n is not None and n > n0) or \
        (l0 is not None and l is not None and l > l0)


def _fmt(v):
    return "?" if v is None else v


def _section(st, name):
    value = st.get(name) if isinstance(st, dict) else None
    return value if isinstance(value, dict) else {}


class Us079Run:
    """One run of the playtest.  execute() returns the exit code."""

    def __init__(self, args):
        self.args = args
        self.mode = "attach" if args.attach_port else "launch"
        self.checks = []
        self.screens = []
        self.pickups = []
        self.warnings = []
        self.bridge = None
        self.run_dir = None
        self.fresh = None
        self.started_at = _now()
        self.finished_at = None
        self.exe = None
        self.exe_argv = None
        self.ping = None
        self.focus_start = None
        self.focus_end = None
        self.state_start = None
        self.state_end = None
        self.inventory = None
        self.amulet_instance = None
        self.menu_open_inventory = False
        self.menu_any_open = False
        self.identify_center = None
        self.identify_clicked = False
        self.abort = None      # a lost connection: everything after it is SKIP
        self.blocked = None    # a failed precondition (wrong experience): the rest is SKIP
        self.shot_n = 0
        self.shutdown_note = None
        self.game_log = None
        self.game_log_note = None
        self.log_summary = None
        self.missing_content = None
        self.exit_code = None

    # --- checks -------------------------------------------------------------------------
    def check(self, name, status, detail, kind="gameplay"):
        pb.add_check(self.checks, name, status, detail, kind)
        log(f"{status:<4} {name}: {detail}")
        return status in ("PASS", "WARN", "INFO")

    def skip(self, name, why, kind="gameplay"):
        self.check(name, "SKIP", why, kind)
        return False, None

    @staticmethod
    def err_detail(e):
        detail = str(e)
        resp = e.response if isinstance(e.response, dict) else None
        if resp is not None:
            if "values" in resp:
                detail += f"; last values: {_dumps(resp.get('values'), 1500)}"
            elif not any(k in resp for k in ("state", "player", "pickups", "pause_menu")):
                text = _dumps(resp, 600)
                if len(text) < 600:
                    detail += f"; {text}"
        return detail

    def alive(self):
        b = self.bridge
        return b is not None and b.conn is not None and not b.conn.closed and b.is_alive()

    def attempt(self, name, fn, kind="gameplay", needs=True, needs_why=None):
        """Run fn() -> (status, detail, data) as check `name`.  Returns (ok, data).

        A BridgeError is a FAIL: gameplay when the bridge says so (and the check is a gameplay
        check), infra otherwise; a lost connection SKIPs everything after it.
        """
        if self.abort or self.blocked:
            return self.skip(name, self.abort or self.blocked, kind)
        if not needs:
            return self.skip(name, needs_why or "not run: a step it depends on failed", kind)
        try:
            status, detail, data = fn()
        except ab.BridgeError as e:
            k = kind if e.kind == "gameplay" else "infra"
            self.check(name, "FAIL", self.err_detail(e), k)
            if e.kind == "infra" and (e.code in CONNECTION_LOST or not self.alive()):
                self.abort = f"not run: the bridge connection was lost in {name} ({e.code})"
            return False, None
        ok = self.check(name, status, detail, kind)
        return ok, data

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
                    log(f"ERROR: game exe not found: {self.exe} -- package first: "
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
                    log("STALE BUILD: refusing to run (no run dir written).  Re-package with "
                        "py Scripts/package_playtest.py; --allow-stale is diagnostic only.")
                    return code
        self.run_dir = os.path.abspath(args.run_dir) if args.run_dir else pb.new_run_dir(
            RUN_SUFFIX)
        os.makedirs(self.run_dir, exist_ok=True)
        log(f"Run dir: {self.run_dir}")
        try:
            # Every step runs: after a failed launch or a lost connection its checks are SKIP,
            # so the report still lists everything that was not tested.
            self.start()
            self.step_boot()
            self.step_pickups()
            self.step_inventory()
            self.step_menu()
            self.step_identify()
            self.step_escape()
        except KeyboardInterrupt:
            self.check("completed", "FAIL", "interrupted by the user", "infra")
        except Exception as e:  # report what we have instead of dying without a report
            traceback.print_exc()
            self.check("completed", "FAIL", f"playtest script error: {type(e).__name__}: {e}",
                       "infra")
        finally:
            self.finish()
        return self.exit_code

    # --- step 1: launch / attach --------------------------------------------------------
    def start(self):
        args = self.args
        if self.mode == "launch":
            self._build_check()
            try:
                self.bridge = ab.Bridge.launch(
                    map=MAP, expect_experience=EXPERIENCE, allow_stale=args.allow_stale,
                    detach=args.keep_running, run_suffix=RUN_SUFFIX, timeout=args.timeout,
                    extra_args=args.extra_arg, exe=self.exe, exe_argv=self.exe_argv,
                    skip_freshness=args.test_skip_freshness, run_dir=self.run_dir,
                    ready_timeout=args.ready_timeout)
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
        if got != EXPERIENCE:
            self.check("experience", "FAIL", f"the bridge reports experience '{got or None}', "
                       f"expected {EXPERIENCE}", "gameplay")
            self.blocked = f"not run: wrong experience ({got or None})"
            return False
        self.check("experience", "PASS", f"{got} on {ping.get('map')}", "gameplay")
        if not ping.get("ready"):
            self.abort = "not run: the bridge never became ready"
            return False
        return True

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
            with pb.log_stream(_LOG_STREAM or sys.stdout):
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

    # --- step 1 (continued): HUD, focus, first screenshot ---------------------------------
    def step_boot(self):
        def hud():
            b = self.bridge
            cond = (f"pickups.prompt.hint_visible==true && "
                    f"pickups.prompt.hint_text=={lit(HUD_HINT)}")
            try:
                b.wait_until(cond, timeout_s=HUD_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                return ("FAIL", f"the '{HUD_HINT}' hint is not visible within {HUD_TIMEOUT_S:g}s"
                        f"; last values: {_dumps((e.response or {}).get('values'), 800)}", None)
            st = b.state(["widgets"])
            widgets = st.get("widgets")
            widgets = widgets if isinstance(widgets, list) else []
            classes = [str(w.get("class")) for w in widgets if isinstance(w, dict)]
            hud_classes = [c for c in classes if "combathud" in c.lower()]
            if not hud_classes:
                return ("FAIL", f"'{HUD_HINT}' is visible, but no combat HUD widget is in "
                        f"state.widgets (visible Mordecai widgets: {classes})", None)
            return ("PASS", f"{hud_classes[0]} on screen with '{HUD_HINT}' visible "
                    f"(visible Mordecai widgets: {', '.join(classes)})", None)

        self.attempt("hud_visible", hud)
        if not (self.abort or self.blocked):
            try:
                self.state_start = self.bridge.state(["game", "focus", "player", "input"])
                self.focus_start = self.state_start.get("focus")
            except ab.BridgeError as e:
                self.warnings.append(f"could not read the starting state: {e}")
        self.shot("start")

    # --- step 2: the six pickups --------------------------------------------------------
    def step_pickups(self):
        for stop in ROUTE:
            self.do_pickup(stop)

    def _walk(self, x, y):
        r = self.bridge.walk_to(x, y, tol=WALK_TOL)
        fx, fy = r["final"]
        return ("PASS", f"reached ({fx:.0f}, {fy:.0f}), {r['dist']:.0f} cm from ({x:.0f}, "
                f"{y:.0f}), {r['pulses']} pulses, {r.get('side_steps', 0)} side-steps", r)

    def do_pickup(self, stop):
        name, label = stop["name"], stop["label"]
        rec = {"name": name, "label": label, "target": list(stop["target"]), "walk": None,
               "focused": None, "prompt": None, "picked": None}
        self.pickups.append(rec)
        for i, (wx, wy) in enumerate(stop.get("waypoints") or ()):
            self.attempt(f"walk_{name}_waypoint{i + 1}", lambda: self._walk(wx, wy))
        tx, ty = stop["target"]
        walked, result = self.attempt(f"walk_{name}", lambda: self._walk(tx, ty))
        rec["walk"] = result
        why = f"not run: walk_{name} failed"

        def focus():
            b = self.bridge
            try:
                r = b.wait_until(f"pickups.focused.label=={lit(label)}",
                                 timeout_s=FOCUS_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                focused = ((e.response or {}).get("values") or {}).get("pickups.focused.label")
                return ("FAIL", f"{label} is not the focused pickup {FOCUS_TIMEOUT_S:g}s after "
                        f"the walk (focused: {focused})", None)
            st = b.state(["pickups"])
            f = _section(st, "pickups").get("focused") or {}
            if f.get("label") != label:
                return "FAIL", f"focus moved away again (focused: {f.get('label')})", None
            return ("PASS", f"{label} focused ('{f.get('display_name')}', "
                    f"{float(f.get('distance') or 0):.0f} cm) after {r.get('elapsed_s')}s", f)

        focused_ok, focused = self.attempt(f"focus_{name}", focus, needs=walked, needs_why=why)
        rec["focused"] = focused
        why = f"not run: focus_{name} failed"

        def prompt():
            display = (focused or {}).get("display_name") or ""
            expected = PROMPT_PREFIX + display
            try:
                self.bridge.wait_until(f"pickups.prompt.visible==true && "
                                       f"pickups.prompt.text=={lit(expected)}",
                                       timeout_s=PROMPT_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                v = (e.response or {}).get("values") or {}
                return ("FAIL", f"expected the visible prompt '{expected}'; rendered "
                        f"text={v.get('pickups.prompt.text')!r}, visible="
                        f"{v.get('pickups.prompt.visible')}", None)
            note = ("" if display == stop["display_name"] else
                    f" (note: the item asset's expected name is '{stop['display_name']}')")
            return "PASS", f"'{expected}' rendered and visible{note}", expected

        _, rec["prompt"] = self.attempt(f"prompt_{name}", prompt, needs=focused_ok,
                                        needs_why=why)

        def pick():
            b = self.bridge
            before = _section(b.state(["inventory"]), "inventory")
            if before.get("valid") is not True:
                return ("FAIL", f"inventory section invalid before F ({before.get('reason')}): "
                        "a pickup could not be confirmed, so F was not tapped", False)
            n0, l0 = _stored_counts(before)
            b.tap("F", hold_s=TAP_S)
            t0 = time.monotonic()

            def settled(st):
                # Stop early on an invalid section (no pawn: nothing can be judged); otherwise
                # wait until the pickup left pickups.world AND something was stored.
                p, inv = _section(st, "pickups"), _section(st, "inventory")
                if p.get("valid") is not True or inv.get("valid") is not True:
                    return True
                n, l = _stored_counts(inv)
                return label not in _world_labels(st) and _grew(n0, l0, n, l)

            try:
                st = b.wait_for(settled, timeout_s=PICKUP_TIMEOUT_S,
                                sections=["pickups", "inventory"])
                timed_out = False
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                st, timed_out = e.response or {}, True
            p, inv = _section(st, "pickups"), _section(st, "inventory")
            n, l = _stored_counts(inv)
            counts = (f"inventory.count {_fmt(n0)} -> {_fmt(n)}, inventory.ledger_count "
                      f"{_fmt(l0)} -> {_fmt(l)}")
            if p.get("valid") is not True:
                return ("FAIL", f"pickups section invalid after F ({p.get('reason')}): no evidence "
                        f"{label} was picked up; {counts}", False)
            if inv.get("valid") is not True:
                return ("FAIL", f"inventory section invalid after F ({inv.get('reason')}): the "
                        f"pickup cannot be confirmed; {counts}", False)
            in_world = label in _world_labels(st)
            stored = _grew(n0, l0, n, l)
            if timed_out or in_world or not stored:
                where = (f"{label} is still in pickups.world" if in_world else
                         f"{label} left pickups.world but nothing was stored")
                return ("FAIL", f"{where} {PICKUP_TIMEOUT_S:g}s after F (focused: "
                        f"{(p.get('focused') or {}).get('label')}, prompt: "
                        f"{(p.get('prompt') or {}).get('text')!r}); {counts}", False)
            return ("PASS", f"tapped F; {label} left the world and was stored after "
                    f"{time.monotonic() - t0:.2f}s; {counts}", True)

        _, rec["picked"] = self.attempt(f"pickup_{name}", pick, needs=focused_ok, needs_why=why)

    # --- step 3: inventory and ledger ---------------------------------------------------
    def step_inventory(self):
        def count():
            st = self.bridge.state(["inventory"])
            inv = _section(st, "inventory")
            self.inventory = inv
            if not inv.get("valid"):
                return "FAIL", f"inventory section invalid: {inv.get('reason')}", None
            items = [i for i in inv.get("items") or [] if isinstance(i, dict)]
            for it in items:
                if it.get("item_id") == AMULET_ITEM_ID:
                    self.amulet_instance = it.get("instance_id")
            names = ", ".join(str(i.get("display_name")) for i in items)
            n = inv.get("count")
            return ("PASS" if n == len(EXPECTED_ITEMS) else "FAIL",
                    f"inventory.count={n} (expected {len(EXPECTED_ITEMS)}): {names or 'empty'}",
                    inv)

        self.attempt("inventory_count", count)
        have = isinstance(self.inventory, dict) and bool(self.inventory.get("valid"))
        why = "not run: the inventory could not be read"

        def items():
            got = {}
            for it in self.inventory.get("items") or []:
                if isinstance(it, dict):
                    got.setdefault(it.get("item_id"), []).append(it.get("display_name"))
            problems = []
            for item_id, name in EXPECTED_ITEMS.items():
                names = got.get(item_id)
                if not names:
                    problems.append(f"{item_id} missing")
                elif len(names) != 1:
                    problems.append(f"{item_id} x{len(names)}")
                elif names[0] != name:
                    problems.append(f"{item_id} is named '{names[0]}', expected '{name}'")
            extra = sorted(str(k) for k in got if k not in EXPECTED_ITEMS)
            if extra:
                problems.append("unexpected: " + ", ".join(extra))
            if problems:
                return "FAIL", "; ".join(problems), None
            return ("PASS", "; ".join(f"{k}='{v}'" for k, v in EXPECTED_ITEMS.items()), None)

        def ledger_count():
            n = self.inventory.get("ledger_count")
            return ("PASS" if n == len(EXPECTED_LEDGER) else "FAIL",
                    f"inventory.ledger_count={n} (expected {len(EXPECTED_LEDGER)})", None)

        def ledger_entries():
            got = {e.get("item_id"): e.get("count") for e in self.inventory.get("ledger") or []
                   if isinstance(e, dict)}
            if got != EXPECTED_LEDGER:
                return "FAIL", f"ledger {got}, expected {EXPECTED_LEDGER}", None
            names = ", ".join(f"{e.get('display_name')} x{e.get('count')}"
                              for e in self.inventory.get("ledger") or [])
            return "PASS", names, None

        self.attempt("inventory_items", items, needs=have, needs_why=why)
        self.attempt("ledger_count", ledger_count, needs=have, needs_why=why)
        self.attempt("ledger_entries", ledger_entries, needs=have, needs_why=why)

    # --- step 4: I opens the inventory tab --------------------------------------------------
    def _amulet_row(self, st):
        tab = _section(_section(st, "pause_menu"), "inventory_tab")
        rows = [r for r in tab.get("rows") or [] if isinstance(r, dict)]
        if self.amulet_instance:
            want = _norm_id(self.amulet_instance)
            for r in rows:
                if _norm_id(r.get("instance_id")) == want:
                    return r
        for r in rows:  # no inventory read: the unidentified amulet row
            if "amulet" in str(r.get("text") or "").lower() and r.get("unidentified"):
                return r
        return None

    @staticmethod
    def _button_ok(button, viewport):
        if not isinstance(button, dict) or button.get("label") != IDENTIFY_LABEL:
            return False
        c = button.get("center")
        if not (isinstance(c, list) and len(c) >= 2 and all(isinstance(v, (int, float))
                                                           for v in c[:2])):
            return False
        w, h = viewport
        return 0 <= c[0] <= w and 0 <= c[1] <= h and button.get("enabled") is True

    def step_menu(self):
        def open_menu():
            b = self.bridge
            b.tap("I", hold_s=TAP_S)
            try:
                r = b.wait_until('pause_menu.open==true && pause_menu.active_tab=="inventory"',
                                 timeout_s=MENU_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                v = (e.response or {}).get("values") or {}
                self.menu_any_open = v.get("pause_menu.open") is True
                return ("FAIL", f"after I: pause_menu.open={v.get('pause_menu.open')}, "
                        f"active_tab={v.get('pause_menu.active_tab')!r} (expected open on "
                        f"'inventory' within {MENU_TIMEOUT_S:g}s)", None)
            self.menu_any_open = True
            self.menu_open_inventory = True
            return "PASS", f"pause menu open on the inventory tab {r.get('elapsed_s')}s after I", None

        self.attempt("menu_opens_inventory", open_menu)
        why = "not run: the inventory tab did not open"

        def rows():
            b = self.bridge
            timed_out = False
            try:
                b.wait_until("len(pause_menu.inventory_tab.rows)==4", timeout_s=ROWS_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                timed_out = True
            tab = _section(_section(b.state(["pause_menu"]), "pause_menu"), "inventory_tab")
            texts = [str(r.get("text")) for r in tab.get("rows") or [] if isinstance(r, dict)]
            status = "FAIL" if timed_out or len(texts) != 4 else "PASS"
            return status, f"{len(texts)} rendered rows (expected 4): {texts}", tab

        _, tab = self.attempt("menu_rows", rows, needs=self.menu_open_inventory, needs_why=why)

        def ledger_rows():
            texts = [str(t) for t in (tab or {}).get("ledger_rows") or []]
            return ("PASS" if len(texts) == len(EXPECTED_LEDGER) else "FAIL",
                    f"{len(texts)} ledger rows (expected {len(EXPECTED_LEDGER)}): {texts}", None)

        self.attempt("menu_ledger_rows", ledger_rows, needs=tab is not None,
                     needs_why=why if not self.menu_open_inventory
                     else "not run: the rendered rows could not be read")

        def button():
            b = self.bridge
            seen = {}

            def ready(st):
                vp = _section(st, "game").get("viewport_size") or [1280, 720]
                row = self._amulet_row(st)
                seen["row"] = row
                return row is not None and self._button_ok(row.get("identify_button"), vp)

            try:
                st = b.wait_for(ready, timeout_s=BUTTON_TIMEOUT_S, sections=["pause_menu", "game"])
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                row = seen.get("row")
                if row is None:
                    return "FAIL", "no amulet row in the rendered inventory rows", None
                return ("FAIL", f"the amulet row '{row.get('text')}' has no usable Identify "
                        f"button: {_dumps(row.get('identify_button'))}", None)
            row = self._amulet_row(st)
            center = row["identify_button"]["center"]
            self.identify_center = (float(center[0]), float(center[1]))
            return ("PASS", f"'{IDENTIFY_LABEL}' on row '{row.get('text')}' at viewport "
                    f"({center[0]:.0f}, {center[1]:.0f}), enabled", row)

        self.attempt("identify_button", button, needs=self.menu_open_inventory, needs_why=why)
        self.shot("menu", needs=self.menu_any_open or self.menu_open_inventory,
                  needs_why="not taken: the pause menu did not open")

    # --- step 5: identify the amulet through the UI ----------------------------------------
    def step_identify(self):
        def click():
            x, y = self.identify_center
            r = self.bridge.click(x, y, button="Left")
            self.identify_clicked = True
            return "PASS", f"Left click at ({x:.0f}, {y:.0f}); Slate hit {r.get('hit')}", r

        self.attempt("identify_click", click, needs=self.identify_center is not None,
                     needs_why="not run: no Identify button to click")

        def identified():
            b = self.bridge
            seen = {}

            def done(st):
                row = self._amulet_row_any(st)
                item = self._amulet_item(st)
                seen.update(row=row, item=item)
                return (row is not None and row.get("text") == AMULET_IDENTIFIED_NAME
                        and not row.get("identify_button") and item is not None
                        and item.get("identified") is True
                        and item.get("display_name") == AMULET_IDENTIFIED_NAME)

            try:
                b.wait_for(done, timeout_s=IDENTIFY_TIMEOUT_S, sections=["pause_menu", "inventory"])
            except ab.BridgeError as e:
                if e.code != "wait_for_timeout":
                    raise
                row, item = seen.get("row") or {}, seen.get("item") or {}
                return ("FAIL", f"{IDENTIFY_TIMEOUT_S:g}s after the click: row text "
                        f"{row.get('text')!r}, Identify button {_dumps(row.get('identify_button'))}"
                        f", item identified={item.get('identified')}, display_name="
                        f"{item.get('display_name')!r} (expected '{AMULET_IDENTIFIED_NAME}')", None)
            return ("PASS", f"the amulet row reads '{AMULET_IDENTIFIED_NAME}' with no Identify "
                    "button and inventory reports it identified", None)

        self.attempt("amulet_identified", identified, needs=self.identify_clicked,
                     needs_why="not run: the Identify button was not clicked")
        self.shot("identified", needs=self.identify_clicked,
                  needs_why="not taken: the Identify button was not clicked")

    def _amulet_row_any(self, st):
        """The amulet's row by instance id (its text changes when it is identified)."""
        tab = _section(_section(st, "pause_menu"), "inventory_tab")
        rows = [r for r in tab.get("rows") or [] if isinstance(r, dict)]
        if self.amulet_instance:
            want = _norm_id(self.amulet_instance)
            for r in rows:
                if _norm_id(r.get("instance_id")) == want:
                    return r
        for r in rows:
            if "amulet" in str(r.get("text") or "").lower():
                return r
        return None

    def _amulet_item(self, st):
        for it in _section(st, "inventory").get("items") or []:
            if isinstance(it, dict) and it.get("item_id") == AMULET_ITEM_ID:
                return it
        return None

    # --- step 6: Escape closes the menu -------------------------------------------------
    def step_escape(self):
        def escape():
            b = self.bridge
            b.tap("Escape", hold_s=TAP_S)
            try:
                r = b.wait_until("pause_menu.open==false", timeout_s=ESCAPE_TIMEOUT_S)
            except ab.BridgeError as e:
                if e.code != "timeout":
                    raise
                st = b.state(["pause_menu", "focus", "input"])
                pm, focus = _section(st, "pause_menu"), _section(st, "focus")
                return ("FAIL", f"the pause menu is still open {ESCAPE_TIMEOUT_S:g}s after a real "
                        f"Escape press (active_tab={pm.get('active_tab')!r}, commonui_input_mode="
                        f"{focus.get('commonui_input_mode')!r}, game_input_allowed="
                        f"{focus.get('game_input_allowed')}). REGRESSION of the back-handler fix "
                        "(AC-069.10), not a known issue: Escape closes the menu through CommonUI's "
                        "DefaultBack action because UMordecaiPauseMenuWidget sets bIsBackHandler "
                        "(Specs/agent_bridge_v1.md section 9) -- check that the pushed widget is "
                        "still a back handler and run the Mordecai.UI.PauseMenu.* tests, then "
                        "file it as a regression", None)
            return "PASS", f"the pause menu closed {r.get('elapsed_s')}s after Escape", None

        self.attempt("menu_closes_escape", escape, needs=self.menu_any_open,
                     needs_why="not run: the pause menu never opened")
        self.shot("after_escape", needs=self.menu_any_open,
                  needs_why="not taken: the pause menu never opened")

    # --- screenshots ----------------------------------------------------------------------
    def shot(self, label, needs=True, needs_why=None):
        name = f"screenshot_{label}"

        def take():
            self.shot_n += 1
            path = os.path.join(self.run_dir, f"bridge_{self.shot_n:02d}.png")
            got = self.bridge.screenshot(path)
            entry = {"label": label, "path": got, "expect": SCREENSHOTS.get(label, "")}
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

        self.attempt(name, take, kind="infra", needs=needs, needs_why=needs_why)

    # --- the end --------------------------------------------------------------------------
    def finish(self):
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
        self.finished_at = _now()
        self.exit_code = pb.exit_code_for(self.checks)
        try:
            self.write_reports()
        except OSError as e:
            log(f"ERROR: could not write the report: {e}")
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
        log(f"Shutdown: {self.shutdown_note}")

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

    # --- reports --------------------------------------------------------------------------
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

    def report(self):
        f = self.fresh or {}
        counts = {s: sum(1 for c in self.checks if c["status"] == s)
                  for s in ("PASS", "WARN", "FAIL", "SKIP", "INFO")}
        b = self.bridge
        return {
            "story": STORY, "title": TITLE, "script": os.path.relpath(os.path.abspath(__file__),
                                                                       pb.PROJECT_ROOT),
            "result": "PASS" if self.exit_code == EXIT_PASS else "FAIL",
            "exit_code": self.exit_code, "evidence": self.evidence(), "counts": counts,
            "run_dir": self.run_dir, "mode": self.mode, "started_at": self.started_at,
            "finished_at": self.finished_at, "exe": self.exe,
            "command_line": b.command_line if b is not None else None,
            "map": MAP, "expected_experience": EXPERIENCE, "allow_stale": self.args.allow_stale,
            "attach_port": self.args.attach_port,
            "build": {"fresh": f.get("fresh"), "skipped": bool(f.get("skipped")),
                      "reasons": f.get("reasons") or [], "warnings": f.get("warnings") or [],
                      "build": f.get("build"), "head": f.get("head"), "branch": f.get("branch")},
            "ping": self.ping, "checks": self.checks, "pickups": self.pickups,
            "inventory": self.inventory, "focus_start": self.focus_start,
            "focus_end": self.focus_end, "state_start": self.state_start,
            "state_end": self.state_end, "screenshots": self.screens,
            "game_log": self.game_log, "game_log_note": self.game_log_note,
            "log": {k: v for k, v in (self.log_summary or {}).items() if k != "tail"} or None,
            "missing_content": self.missing_content, "known_issues": list(KNOWN_ISSUES),
            "shutdown": self.shutdown_note, "warnings": self.warnings,
            "transcript": os.path.join(self.run_dir, ab.TRANSCRIPT_NAME),
        }

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
            f"# {TITLE}: {rep['result']}",
            "",
            f"- Result: **{rep['result']}** (exit {rep['exit_code']}): {c['PASS']} PASS, "
            f"{c['WARN']} WARN, {c['FAIL']} FAIL, {c['SKIP']} SKIP",
            f"- Evidence: **{rep['evidence']}**",
            f"- Run dir: `{rep['run_dir']}`",
            f"- Mode: {rep['mode']}" + (f" (127.0.0.1:{rep['attach_port']})"
                                        if rep["mode"] == "attach" else
                                        " (this script launched the packaged game)"),
            f"- Started: {rep['started_at']}  Finished: {rep['finished_at']}",
            f"- Exe: `{rep['exe']}`",
            f"- Command: `{rep['command_line']}`" if rep["command_line"] else
            "- Command: n/a (this script did not launch the game)",
            f"- Map: {MAP}  Expected experience: {EXPERIENCE}",
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

        lines += ["", "## Checks", "", "| # | Check | Result | Kind | Detail |",
                  "|---|---|---|---|---|"]
        for i, ch in enumerate(rep["checks"], 1):
            lines.append(f"| {i} | {md(ch['name'])} | {ch['status']} | {ch['kind']} | "
                         f"{md(ch['detail'])} |")

        lines += ["", "## Pickups", "", "| Pickup | Target | Walk | Focused | Prompt | Picked up |",
                  "|---|---|---|---|---|---|"]
        for p in rep["pickups"]:
            walk = p.get("walk") or {}
            final = walk.get("final")
            walk_text = (f"({final[0]:.0f}, {final[1]:.0f}), {walk.get('pulses')} pulses"
                         if final else "-")
            focused = (p.get("focused") or {}).get("display_name") or "-"
            lines.append(f"| {p['label']} | ({p['target'][0]:.0f}, {p['target'][1]:.0f}) | "
                         f"{md(walk_text)} | {md(focused)} | {md(p.get('prompt') or '-')} | "
                         f"{'yes' if p.get('picked') else 'no'} |")

        inv = rep.get("inventory") or {}
        lines += ["", "## Inventory after the pickups", ""]
        if inv.get("valid"):
            for it in inv.get("items") or []:
                lines.append(f"- {it.get('item_id')}: '{it.get('display_name')}' "
                             f"(identified={it.get('identified')}, x{it.get('quantity')})")
            for e in inv.get("ledger") or []:
                lines.append(f"- ledger {e.get('item_id')}: '{e.get('display_name')}' "
                             f"x{e.get('count')}")
        else:
            lines.append(f"- not read ({inv.get('reason') or 'no inventory state'})")

        lines += ["", "## Environment (state.focus)", "",
                  "Records whether the run had a focused window, a locked desktop or the "
                  "virtual cursor.", "",
                  f"- Start: `{_dumps(rep['focus_start'])}`",
                  f"- End: `{_dumps(rep['focus_end'])}`"]

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

        lines += ["", "## Known issues this run exercises", ""]
        lines += [f"- {k}" for k in rep["known_issues"]]
        if rep["warnings"]:
            lines += ["", "## Warnings", ""] + [f"- {w}" for w in rep["warnings"]]
        lines += ["", pb.AGENT_LINE, ""]
        with open(os.path.join(self.run_dir, "report.md"), "w", encoding="utf-8",
                  newline="\n") as fh:
            fh.write("\n".join(lines))

    def print_summary(self):
        out = _LOG_STREAM if _LOG_STREAM is not None else sys.stdout
        bar = "=" * 60
        result = "PASS" if self.exit_code == EXIT_PASS else "FAIL"
        print(bar, file=out)
        print(f"{TITLE}: {result} (exit {self.exit_code})", file=out)
        print(bar, file=out)
        for c in self.checks:
            print(f"  {c['status']:<4} {c['name']}: {c['detail']}", file=out)
        print(f"Evidence: {self.evidence()}", file=out)
        print(f"Report: {os.path.join(self.run_dir, 'report.md')}", file=out)
        for s in self.screens:
            print(f"Screenshot: {s['path']}", file=out)
        print(pb.AGENT_LINE, file=out, flush=True)


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        prog="us079_inventory.py",
        description="US-079 inventory playtest with real input (Tier 4). Exit 0 pass, 1 gameplay "
                    "FAIL, 2 infra.")
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
    parser.add_argument("--run-dir", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--extra-arg", action="append", default=[], help=argparse.SUPPRESS)
    # Test-only switches (tests/fake_game.py stands in for the game).
    parser.add_argument("--test-exe-argv", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-skip-freshness", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    if args.attach_port is not None and (args.allow_stale or args.keep_running):
        log("NOTE: --allow-stale / --keep-running do not apply with --attach-port (the attached "
            "game is checked, never launched, and always left running)")
    return args


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = parse_args(argv)
    return Us079Run(args).execute()


if __name__ == "__main__":
    sys.exit(main())
