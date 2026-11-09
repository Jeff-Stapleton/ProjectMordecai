#!/usr/bin/env python3
"""GitHub issue #92 Tier 4: capture player hit/miss and enemy melee debug shapes.

Run after review on the same fresh Development package:
  py Scripts/playtests/issue092_melee_debug_draw.py
  py Scripts/playtests/issue092_melee_debug_draw.py --draw-off

The first run's screenshots must visibly show the red miss, green hit plus dummy marker, and
enemy swing. The second run repeats the same real-input actions with no overlay. The script
checks the underlying hit/miss gameplay through bridge state; visual overlay acceptance requires
opening every screenshot. It intentionally uses ISSUE-92, not the already-assigned US-092.
"""

import math
import os
import re
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402

STORY = "ISSUE-92"
TITLE = "Issue #92 melee debug-draw playtest (Tier 4, real input)"
RUN_SUFFIX = "issue092"

DUMMY = "TestDummy"
ATTACKER = "TestAttacker"
DUMMY_SPOT = (440.0, 600.0)
ATTACKER_SPOT = (2000.0, 0.0)
ACTOR_RADIUS = 3000
SPOT_TOL = 25.0
ATTACK_POINT = (640, 360)
HIT_TIMEOUT_S = 2.0
ENEMY_TIMEOUT_S = 20.0
POLL_S = 0.05
ATTACK_TAG = "Mordecai.State.Attacking"
CVAR = "Mordecai.Debug.DrawMeleeHitShapes"
CVAR_LOG_TIMEOUT_S = 2.0
APPLIED_CVAR_RE = re.compile(
    rf"{re.escape(CVAR)}\s*=\s*\"(-?\d+)\"\s+LastSetBy:")

SCREENSHOTS = {
    "miss": "at the CombatTestMap PlayerStart immediately after an empty-space light swing; "
            "default-on run: a red 200 cm / 120 degree Longsword fan; CVar=0 run: no overlay",
    "hit": "beside TestDummy immediately after a damaging light swing; default-on run: a green "
           "Longsword fan and a small green dummy marker; CVar=0 run: no overlay",
    "enemy": "beside TestAttacker immediately after its melee hit; default-on run: its melee "
             "shape and player hit marker; CVar=0 run: no overlay",
}

def plan_for_mode(draw_off):
    """Only declare ACs this launch mode can exercise; visuals stay WARN pending human review."""
    config_proves = ("AC-2",) if draw_off else ()
    if draw_off:
        visual_proves = {
            "visual_empty_space_overlay": ("AC-8",),
            "visual_player_hit_overlay": ("AC-8",),
            "visual_enemy_overlay": ("AC-8",),
        }
        visual_expectation = "no melee debug overlay"
    else:
        visual_proves = {
            "visual_empty_space_overlay": ("AC-4", "AC-8"),
            "visual_player_hit_overlay": ("AC-1", "AC-4", "AC-8"),
            "visual_enemy_overlay": ("AC-5", "AC-8"),
        }
        visual_expectation = "the expected melee debug overlay"
    return (
        Step("draw_configuration", f"game log reports applied {CVAR}="
             f"{0 if draw_off else 1}", proves=config_proves, kind="infra"),
        Step("empty_space_swing", f"at the PlayerStart, click Left; '{ATTACK_TAG}' is observed "
             "and TestDummy remains out of range at unchanged health"),
        Step("visual_empty_space_overlay", f"review screenshot_miss for "
             f"{visual_expectation}", proves=visual_proves["visual_empty_space_overlay"]),
        Step("walk_to_dummy", f"walk_to{DUMMY_SPOT} -> within {SPOT_TOL:g} cm of the melee spot"),
        Step("player_hits_dummy", f"click Left -> {DUMMY} health decreases within "
             f"{HIT_TIMEOUT_S:g}s"),
        Step("visual_player_hit_overlay", f"review screenshot_hit for "
             f"{visual_expectation}", proves=visual_proves["visual_player_hit_overlay"]),
        Step("walk_to_attacker", f"walk_to{ATTACKER_SPOT} -> within {SPOT_TOL:g} cm of "
             "TestAttacker"),
        Step("enemy_hits_player", f"stand through an enemy swing -> player health decreases "
             f"within {ENEMY_TIMEOUT_S:g}s"),
        Step("visual_enemy_overlay", f"review screenshot_enemy for {visual_expectation}",
             proves=visual_proves["visual_enemy_overlay"]),
    )


PLAN = plan_for_mode(False)


def _num(value):
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def _xy(value):
    if isinstance(value, (list, tuple)) and len(value) >= 2:
        x, y = _num(value[0]), _num(value[1])
        return (x, y) if x is not None and y is not None else None
    return None


def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None


def _target(state, fragment):
    needle = fragment.lower()
    actors = (state or {}).get("actors")
    if not isinstance(actors, list):
        return None
    return next((actor for actor in actors if isinstance(actor, dict)
                 and actor.get("kind") == "enemy"
                 and needle in (str(actor.get("label")) + str(actor.get("class"))).lower()), None)


def _health(actor):
    health = (actor or {}).get("health")
    return _num(health.get("current")) if isinstance(health, dict) else None


def build_exec_arg(draw_off):
    """A single UE token: quote only ExecCmds' value so FParse still sees -ExecCmds=."""
    commands = f"{CVAR} 0,{CVAR}" if draw_off else CVAR
    return f"-ExecCmds={pb.quote_value(commands)}"


def read_applied_cvar(log_path):
    try:
        with open(log_path, encoding="utf-8", errors="replace") as stream:
            matches = APPLIED_CVAR_RE.findall(stream.read())
    except OSError:
        return None
    return int(matches[-1]) if matches else None


class Issue092Run(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("melee_attack", "enemies", "enemy_ai")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    MAP = kit.COMBAT_TEST_MAP
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    SCRIPT = os.path.abspath(__file__)

    def steps(self):
        configured = self.step_configuration()
        self.step_miss()
        self.step_hit()
        self.step_enemy()
        if configured is False:
            self.log("the applied CVar check failed; screenshots cannot establish the requested "
                     "on/off mode")

    def step_configuration(self):
        if self.mode == "attach":
            self.skip("draw_configuration",
                      "attached: this harness did not launch the game, so the applied CVar is "
                      "unchecked", "infra")
            return None

        def check(_bridge):
            expected = 0 if self.args.draw_off else 1
            log_path = self.game_log or os.path.join(self.run_dir, "game.log")
            deadline = time.monotonic() + CVAR_LOG_TIMEOUT_S
            applied = read_applied_cvar(log_path)
            while applied is None and time.monotonic() < deadline:
                time.sleep(POLL_S)
                applied = read_applied_cvar(log_path)
            mode = "OFF (--draw-off)" if self.args.draw_off else "default ON"
            detail = (f"{mode}; game log applied {CVAR}={applied}; expected {expected}; "
                      f"launch token {build_exec_arg(self.args.draw_off)}")
            return ("PASS" if applied == expected else "FAIL"), detail, applied

        ok, _ = self.measure("draw_configuration", check)
        return ok

    def visual_review(self, check, label, needs=True, needs_why=None):
        if not needs:
            return self.skip(check, needs_why or "not run: screenshot was not captured",
                             "gameplay")
        mode = "absence of every melee debug shape" if self.args.draw_off else \
            "the expected colored melee shape and visible hit marker"
        self.check(check, "WARN", f"requires screenshot review: open screenshot_{label} and "
                   f"confirm {mode}; not auto-accepted from gameplay state", "gameplay")

    def step_miss(self):
        def swing(bridge):
            before = bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player_pos = _xy((_player(before) or {}).get("location"))
            dummy = _target(before, DUMMY)
            dummy_pos = _xy((dummy or {}).get("location"))
            health_before = _health(dummy)
            if player_pos is None or dummy_pos is None or health_before is None:
                return "FAIL", "missing player or TestDummy state before empty swing", None
            distance = math.dist(player_pos, dummy_pos)
            if distance <= 200.0:
                return "FAIL", f"TestDummy unexpectedly only {distance:.1f} cm away", None
            response = bridge.click(*ATTACK_POINT, button="Left")
            seen_attacking = False
            tags = []
            deadline = time.monotonic() + 0.35
            while time.monotonic() < deadline:
                player = _player(bridge.state(["player"]))
                tags = list((player or {}).get("tags") or [])
                seen_attacking = seen_attacking or ATTACK_TAG in tags
                time.sleep(POLL_S)
            self.shot("miss")
            health_after = _health(_target(
                bridge.state(["actors"], radius=ACTOR_RADIUS), DUMMY))
            status = "PASS" if seen_attacking and health_after == health_before else "FAIL"
            return status, (f"clicked empty space with TestDummy {distance:.1f} cm away; "
                            f"health {health_before} -> {health_after}; Slate hit "
                            f"{response.get('hit')}; '{ATTACK_TAG}' seen: {seen_attacking}; "
                            f"last tags {tags}"), None

        ok, _ = self.measure("empty_space_swing", swing)
        self.visual_review("visual_empty_space_overlay", "miss", needs=ok,
                           needs_why="not run: empty-space swing was not observed")

    def step_hit(self):
        walked, _ = self.walk("walk_to_dummy", *DUMMY_SPOT, tol=SPOT_TOL)

        def hit(bridge):
            before = bridge.state(["actors"], radius=ACTOR_RADIUS)
            health_before = _health(_target(before, DUMMY))
            if health_before is None:
                return "FAIL", "no TestDummy health before player swing", None
            response = bridge.click(*ATTACK_POINT, button="Left")
            deadline = time.monotonic() + HIT_TIMEOUT_S
            health_after = health_before
            while time.monotonic() < deadline:
                health_after = _health(_target(
                    bridge.state(["actors"], radius=ACTOR_RADIUS), DUMMY))
                if health_after is not None and health_after < health_before:
                    self.shot("hit")
                    return ("PASS", f"TestDummy health {health_before} -> {health_after}; "
                            f"Slate hit {response.get('hit')}", None)
                time.sleep(POLL_S)
            return ("FAIL", f"TestDummy health stayed {health_after} for "
                    f"{HIT_TIMEOUT_S:g}s after click", None)

        ok, _ = self.measure("player_hits_dummy", hit, needs=walked,
                             needs_why="not run: walk_to_dummy failed")
        self.visual_review("visual_player_hit_overlay", "hit", needs=ok,
                           needs_why="not run: player hit screenshot was not captured")
        return ok

    def step_enemy(self):
        walked, _ = self.walk("walk_to_attacker", *ATTACKER_SPOT, tol=SPOT_TOL)

        def hit(bridge):
            before = _player(bridge.state(["player"]))
            health_before = _num(((before or {}).get("attributes") or {}).get("health"))
            if health_before is None:
                return "FAIL", "no player health before enemy swing", None
            deadline = time.monotonic() + ENEMY_TIMEOUT_S
            health_after = health_before
            while time.monotonic() < deadline:
                now = _player(bridge.state(["player"]))
                health_after = _num(((now or {}).get("attributes") or {}).get("health"))
                if health_after is not None and health_after < health_before:
                    self.shot("enemy")
                    return "PASS", f"player health {health_before} -> {health_after}", None
                time.sleep(POLL_S)
            return ("FAIL", f"player health stayed {health_after} for "
                    f"{ENEMY_TIMEOUT_S:g}s beside TestAttacker", None)

        ok, _ = self.measure("enemy_hits_player", hit, needs=walked,
                             needs_why="not run: walk_to_attacker failed")
        self.visual_review("visual_enemy_overlay", "enemy", needs=ok,
                           needs_why="not run: enemy hit screenshot was not captured")

    def md_sections(self):
        configuration_note = (
            f"- `{CVAR}` was checked from the running game's console-response log."
            if self.mode == "launch" else
            f"- Attached run: `{CVAR}` was not checked because this harness did not launch "
            "the game."
        )
        return ["", "## Issue #92 review notes", "",
                f"- Launch mode: {'off (--draw-off)' if self.args.draw_off else 'default on'}.",
                configuration_note,
                "- Runtime 0 -> 1 toggle acceptance is Tier 1/PIE evidence, not claimed by this "
                "packaged launch.",
                "- Overlay presence/absence remains NOT RUN until every listed screenshot is "
                "opened and reviewed."]


Run = Issue092Run


def parse_args(argv=None):
    values = list(sys.argv[1:] if argv is None else argv)
    draw_off = False
    filtered = []
    for value in values:
        if value == "--draw-off":
            draw_off = True
        else:
            filtered.append(value)
    args = kit.parse_args(Issue092Run, filtered)
    args.draw_off = draw_off
    if args.attach_port is not None and draw_off:
        kit.log("NOTE: --draw-off has no effect with --attach-port because this harness did not "
                "launch the attached game", STORY)
    if args.attach_port is None:
        args.extra_arg = list(args.extra_arg or []) + [build_exec_arg(draw_off)]
    return args


def main(argv=None):
    """Issue runs use ISSUE-N, so bypass only playtest_kit's US-NNN declaration-name lint."""
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    args = parse_args(argv)
    Issue092Run.PLAN = plan_for_mode(args.draw_off)
    if args.plan:
        try:
            sys.stdout.reconfigure(encoding="utf-8")
        except (AttributeError, ValueError):
            pass
        print(kit.plan_table(Issue092Run), flush=True)
        return kit.EXIT_PASS
    return Issue092Run(args).execute()


if __name__ == "__main__":
    sys.exit(main())
