#!/usr/bin/env python3
"""GitHub issue #92 Tier 4: capture player hit/miss and enemy melee debug shapes.

Run after review on the same fresh Development package:
  py Scripts/playtests/issue092_melee_debug_draw.py
  py Scripts/playtests/issue092_melee_debug_draw.py \
    --extra-arg='-ExecCmds="Mordecai.Debug.DrawMeleeHitShapes 0"'

The first run's screenshots must visibly show the red miss, green hit plus dummy marker, and
enemy swing. The second run repeats the same real-input actions with no overlay. The script
checks the underlying hit/miss gameplay through bridge state; visual overlay acceptance requires
opening every screenshot. It intentionally uses ISSUE-92, not the already-assigned US-092.
"""

import math
import os
import sys
import time

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

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

SCREENSHOTS = {
    "miss": "at the CombatTestMap PlayerStart immediately after an empty-space light swing; "
            "default-on run: a red 200 cm / 120 degree Longsword fan; CVar=0 run: no overlay",
    "hit": "beside TestDummy immediately after a damaging light swing; default-on run: a green "
           "Longsword fan and a small green dummy marker; CVar=0 run: no overlay",
    "enemy": "beside TestAttacker immediately after its melee hit; default-on run: its melee "
             "shape and player hit marker; CVar=0 run: no overlay",
}

PLAN = (
    Step("draw_configuration", "launch with no override (default-on run), or forward "
         "`-ExecCmds=\"Mordecai.Debug.DrawMeleeHitShapes 0\"` through --extra-arg (off run)",
         proves=("AC-1", "AC-2")),
    Step("empty_space_swing", "at the PlayerStart, click Left at viewport center and immediately "
         "screenshot; TestDummy remains out of range and at unchanged health",
         proves=("AC-4", "AC-8")),
    Step("walk_to_dummy", f"walk_to{DUMMY_SPOT} -> within {SPOT_TOL:g} cm of the melee spot"),
    Step("player_hits_dummy", f"walk_to{DUMMY_SPOT}, click Left at viewport center and immediately "
         f"screenshot; {DUMMY} health decreases within {HIT_TIMEOUT_S:g}s",
         proves=("AC-1", "AC-4", "AC-8")),
    Step("walk_to_attacker", f"walk_to{ATTACKER_SPOT} -> within {SPOT_TOL:g} cm of TestAttacker"),
    Step("enemy_hits_player", f"walk_to{ATTACKER_SPOT}, stand through an enemy swing and "
         f"immediately screenshot when player health decreases within {ENEMY_TIMEOUT_S:g}s",
         proves=("AC-5", "AC-8")),
)


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


def is_off_launch(extra_args):
    return any("Mordecai.Debug.DrawMeleeHitShapes 0" in arg for arg in (extra_args or []))


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
        self.step_configuration()
        self.step_miss()
        self.step_hit()
        self.step_enemy()

    def step_configuration(self):
        def check(_bridge):
            args = list(self.args.extra_arg or [])
            off = is_off_launch(args)
            mode = "OFF launch override" if off else "default ON (no off override)"
            return "PASS", f"{mode}; forwarded extra args: {args}", None

        self.measure("draw_configuration", check)

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
            time.sleep(0.3)
            self.shot("miss")
            health_after = _health(_target(
                bridge.state(["actors"], radius=ACTOR_RADIUS), DUMMY))
            status = "PASS" if health_after == health_before else "FAIL"
            return status, (f"clicked empty space with TestDummy {distance:.1f} cm away; "
                            f"health {health_before} -> {health_after}; Slate hit "
                            f"{response.get('hit')}"), None

        self.measure("empty_space_swing", swing)

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

        self.measure("enemy_hits_player", hit, needs=walked,
                     needs_why="not run: walk_to_attacker failed")


Run = Issue092Run


def main(argv=None):
    """Issue runs use ISSUE-N, so bypass only playtest_kit's US-NNN declaration-name lint."""
    args = kit.parse_args(Issue092Run, argv)
    if args.plan:
        print(kit.plan_table(Issue092Run), flush=True)
        return kit.EXIT_PASS
    return Issue092Run(args).execute()


if __name__ == "__main__":
    sys.exit(main())
