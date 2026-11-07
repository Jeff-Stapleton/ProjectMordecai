#!/usr/bin/env python3
"""US-087 Fireball recovery -- dedicated Tier 4 owner for issue #20.

On CombatTestMap this script selects the passive TestDummy, retains its exact actors[].name,
walks about 1400 cm west of it through real WASD input, and taps Q through Slate. It independently
checks that the cast spends SP, a visible projectile is sampled within 1.5 seconds, and that same
live stationary target loses health within 2.5 seconds. Screenshots are taken immediately when
the visible projectile and damage are first observed. After the timed window, real WASD approaches
the retained target for a supplemental reduced-health-bar screenshot. This owner intentionally
covers only AC-087.4/.5; future Blink recovery needs a separate owner or explicit consolidation.
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

STORY = "US-087"
TITLE = "US-087 Fireball recovery playtest (Tier 4, real input)"
RUN_SUFFIX = "us087-fireball"

TARGET_LABEL = "TestDummy"
TARGET_SPAWN = (600.0, 600.0)
ACTOR_RADIUS = 3000
SHOT_DISTANCE = 1400.0
VISUAL_DISTANCE = 300.0
SHOT_TOL = 70.0
LINE_TOL = 50.0
TARGET_MOVE_TOL = 10.0
BASELINE_WATCH_S = 0.2
PROJECTILE_WINDOW_S = 1.5
DAMAGE_WINDOW_S = 2.5
POLL_S = 0.03
SP_COST = 3.0
TAP_S = 0.1

SCREENSHOTS = {
    "projectile_visible": "CombatTestMap immediately after Q: the player about 1400 cm west of "
                          "the passive TestDummy, with the asset-backed Fireball sphere visibly "
                          "in flight on the +X shot line; SP is reduced by 3",
    "same_target_damaged": "CombatTestMap immediately when the retained TestDummy loses health: "
                           "an immediate post-hit timing frame; the target may be off-camera, so "
                           "the continuous exact-name/health audit is the acceptance evidence",
    "reduced_health_visual": "CombatTestMap after the timed checks: the player has approached by "
                             "real WASD and the retained TestDummy's reduced health bar is visible",
}

PLAN = (
    Step("target_preconditions", f"state player,actors -> resolve the live {TARGET_LABEL} by "
         f"label/name or nearest ({TARGET_SPAWN[0]:g},{TARGET_SPAWN[1]:g}), retain exact name; "
         "require full health, no death, and stationary over a baseline watch; then walk_to about "
         f"{SHOT_DISTANCE:g} cm -X and validate a +X shot line inside 1500 range"),
    Step("fireball_cast", f"tap Q through Slate -> spell_points decreases by {SP_COST:g}; clock "
         "starts before the tap and reports wall/game timing", proves=("AC-087.4", "AC-087.5")),
    Step("projectile_visible", f"continuous state polling -> actors[] kind=='projectile' with "
         f"has_visible_mesh==true within {PROJECTILE_WINDOW_S:g}s; independently screenshot it",
         proves=("AC-087.4",)),
    Step("target_integrity", "through the full observation window the retained exact target name "
         "does not disappear or get replaced, move materially, reset/increase health, and the "
         "player stays alive", proves=("AC-087.5",)),
    Step("same_target_damaged", f"continuous polling -> the retained target's health decreases "
         f"within {DAMAGE_WINDOW_S:g}s without a reset/replacement; capture an immediate timing frame",
         proves=("AC-087.5",)),
    Step("reduced_health_visual", f"after the {DAMAGE_WINDOW_S:g}s timing window, walk_to with real "
         f"WASD to {VISUAL_DISTANCE:g} cm west of the retained target; revalidate exact name, "
         "reduced/current and max health, stationary position, no replacement, and live player; "
         "capture the reduced health bar", proves=()),
)


def _num(value):
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None


def _actors(state):
    value = (state or {}).get("actors")
    return [a for a in value if isinstance(a, dict)] if isinstance(value, list) else []


def _xy(actor):
    loc = (actor or {}).get("location")
    if isinstance(loc, list) and len(loc) >= 2 and _num(loc[0]) is not None and _num(loc[1]) is not None:
        return float(loc[0]), float(loc[1])
    return None


def _health(actor):
    health = (actor or {}).get("health")
    if not isinstance(health, dict):
        return None, None
    return _num(health.get("current")), _num(health.get("max"))


def _distance(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def _game_clocks(state):
    game = (state or {}).get("game")
    game = game if isinstance(game, dict) else {}
    return _num(game.get("real_time_s")), _num(game.get("world_time_s"))


class Us087FireballRun(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("spells", "projectiles_aim")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    MAP = kit.COMBAT_TEST_MAP
    SCRIPT = os.path.abspath(__file__)
    WALK_TOL = SHOT_TOL
    TAP_S = TAP_S

    def __init__(self, args):
        super().__init__(args)
        self.target_name = None
        self.target_label = None
        self.target_location = None
        self.target_health = None
        self.target_max_health = None
        self.preconditions_ok = False
        self.observation = {}

    def steps(self):
        self.prepare_target_and_shot()
        self.cast_and_observe()
        self.capture_reduced_health_visual()

    def _select_target(self, state):
        enemies = [a for a in _actors(state) if a.get("kind") == "enemy"]
        labelled = [a for a in enemies
                    if TARGET_LABEL.lower() in str(a.get("label") or "").lower()
                    or TARGET_LABEL.lower() in str(a.get("name") or "").lower()]
        candidates = labelled or [a for a in enemies if _xy(a) is not None]
        if not candidates:
            return None
        return min(candidates, key=lambda a: _distance(_xy(a), TARGET_SPAWN))

    def _exact_target(self, state):
        return next((a for a in _actors(state) if a.get("name") == self.target_name), None)

    def _replacement(self, state):
        for actor in _actors(state):
            if actor.get("kind") != "enemy" or actor.get("name") == self.target_name:
                continue
            if self.target_label and actor.get("label") == self.target_label:
                return actor
            pos = _xy(actor)
            if pos is not None and self.target_location is not None \
                    and _distance(pos, self.target_location) <= TARGET_MOVE_TOL:
                return actor
        return None

    def prepare_target_and_shot(self):
        def prepare():
            state0 = self.bridge.state(["game", "player", "actors"], radius=ACTOR_RADIUS)
            player0 = _player(state0)
            target0 = self._select_target(state0)
            problems = []
            if player0 is None or player0.get("is_dead") is True:
                problems.append("player is missing or dead")
            if target0 is None:
                problems.append(f"no enemy resolves as {TARGET_LABEL} or near {TARGET_SPAWN}")
            if problems:
                return "FAIL", "; ".join(problems), None

            self.target_name = str(target0.get("name") or "")
            self.target_label = str(target0.get("label") or "")
            self.target_location = _xy(target0)
            self.target_health, self.target_max_health = _health(target0)
            if not self.target_name:
                problems.append("selected target has no actors[].name")
            if target0.get("is_dead") is True or self.target_health is None \
                    or self.target_max_health is None or self.target_health <= 0:
                problems.append("selected target is not live with readable health")
            elif abs(self.target_health - self.target_max_health) > 0.01:
                problems.append(f"selected target is not full: {self.target_health:g}/"
                                f"{self.target_max_health:g}")
            if self.target_location is None:
                problems.append("selected target has no readable location")
            if problems:
                return "FAIL", "; ".join(problems), None

            self.bridge.wait(BASELINE_WATCH_S)
            state1 = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            target1 = self._exact_target(state1)
            if target1 is None:
                return "FAIL", f"retained target {self.target_name!r} disappeared during baseline", None
            location1 = _xy(target1)
            health1, max1 = _health(target1)
            if location1 is None or _distance(location1, self.target_location) > TARGET_MOVE_TOL:
                return "FAIL", f"target moved during baseline: {self.target_location} -> {location1}", None
            if health1 != self.target_health or max1 != self.target_max_health:
                return "FAIL", f"target health changed during baseline: {self.target_health}/" \
                               f"{self.target_max_health} -> {health1}/{max1}", None

            shot = (location1[0] - SHOT_DISTANCE, location1[1])
            walked = self.bridge.walk_to(shot[0], shot[1], tol=SHOT_TOL)
            state2 = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player2 = _player(state2)
            target2 = self._exact_target(state2)
            if player2 is None or target2 is None:
                return "FAIL", "player or exact target disappeared after walk", walked
            player_pos = _xy(player2)
            target_pos = _xy(target2)
            hp2, max2 = _health(target2)
            if player2.get("is_dead") is True:
                problems.append("player died before the cast")
            if target2.get("is_dead") is True or hp2 != self.target_health or max2 != self.target_max_health:
                problems.append(f"target no longer live/full before cast: {hp2}/{max2}")
            if player_pos is None or target_pos is None:
                problems.append("shot line has unreadable positions")
                dx, dy = 0.0, 0.0
            else:
                dx, dy = target_pos[0] - player_pos[0], target_pos[1] - player_pos[1]
                if not (SHOT_DISTANCE - SHOT_TOL <= dx <= SHOT_DISTANCE + SHOT_TOL):
                    problems.append(f"shot distance dx={dx:.0f}, expected {SHOT_DISTANCE:g}+/-{SHOT_TOL:g}")
                if abs(dy) > LINE_TOL:
                    problems.append(f"target is {dy:.0f} cm off the +X shot line")
                if dx >= 1500.0:
                    problems.append(f"target centre is outside Fireball range: {dx:.0f} cm")
            if problems:
                return "FAIL", "; ".join(problems), walked
            self.target_location = target_pos
            self.preconditions_ok = True
            return ("PASS", f"retained {self.target_label}/{self.target_name} live, full "
                    f"{hp2:g}/{max2:g}, stationary; walked to ({player_pos[0]:.0f},"
                    f"{player_pos[1]:.0f}); shot line dx={dx:.0f}, dy={dy:.0f}", walked)

        self.attempt("target_preconditions", prepare)

    def cast_and_observe(self):
        if not self.preconditions_ok:
            why = "not run: target_preconditions failed; no Q input sent"
            for check in ("fireball_cast", "projectile_visible", "target_integrity",
                          "same_target_damaged"):
                self.attempt(check, lambda: ("PASS", "", None), needs=False, needs_why=why)
            return

        before = self.bridge.state(["game", "player", "actors"], radius=ACTOR_RADIUS)
        player_before = _player(before)
        sp0 = _num(((player_before or {}).get("attributes") or {}).get("spell_points"))
        wall0 = time.monotonic()
        game_real0, world0 = _game_clocks(before)
        input_response = self.bridge.tap("Q", hold_s=TAP_S)

        result = {
            "target_name": self.target_name,
            "target_label": self.target_label,
            "health_before": self.target_health,
            "max_health": self.target_max_health,
            "sp_before": sp0,
            "input": input_response,
            "projectile_at_wall_s": None,
            "projectile_at_game_s": None,
            "damage_at_wall_s": None,
            "damage_at_game_s": None,
            "world_elapsed_s": None,
            "projectile_seen": False,
            "invisible_projectile_seen": False,
            "health_values": [],
            "integrity_failures": [],
            "polls": 0,
        }
        previous_health = self.target_health
        sp_after = sp0
        projectile_shot = False
        damage_shot = False

        while time.monotonic() - wall0 <= DAMAGE_WINDOW_S:
            state = self.bridge.state(["game", "player", "actors"], radius=ACTOR_RADIUS)
            result["polls"] += 1
            elapsed = time.monotonic() - wall0
            game_real, world_time = _game_clocks(state)
            player = _player(state)
            sp = _num(((player or {}).get("attributes") or {}).get("spell_points"))
            if sp is not None:
                sp_after = sp
            if player is None or player.get("is_dead") is True:
                result["integrity_failures"].append("player died or disappeared")

            exact = self._exact_target(state)
            replacement = self._replacement(state)
            if exact is None:
                result["integrity_failures"].append(f"target {self.target_name!r} disappeared")
                if replacement is not None:
                    result["integrity_failures"].append(
                        f"target replaced by {replacement.get('name')!r}")
            else:
                pos = _xy(exact)
                hp, max_hp = _health(exact)
                if pos is None or _distance(pos, self.target_location) > TARGET_MOVE_TOL:
                    result["integrity_failures"].append(
                        f"target moved {self.target_location} -> {pos}")
                if max_hp != self.target_max_health:
                    result["integrity_failures"].append(
                        f"target max health changed {self.target_max_health} -> {max_hp}")
                if hp is None:
                    result["integrity_failures"].append("target health became unreadable")
                else:
                    if not result["health_values"] or result["health_values"][-1] != hp:
                        result["health_values"].append(hp)
                    if previous_health is not None and hp > previous_health + 0.01:
                        result["integrity_failures"].append(
                            f"target health increased/reset {previous_health:g} -> {hp:g}")
                    previous_health = hp
                    if hp < self.target_health and result["damage_at_wall_s"] is None:
                        result["damage_at_wall_s"] = elapsed
                        result["damage_at_game_s"] = (
                            game_real - game_real0 if game_real is not None and game_real0 is not None
                            else None)
                        if not damage_shot:
                            self.shot("same_target_damaged")
                            damage_shot = True
                if exact.get("is_dead") is True:
                    result["integrity_failures"].append("target died during single Fireball check")

            for actor in _actors(state):
                if actor.get("kind") != "projectile":
                    continue
                result["projectile_seen"] = True
                if actor.get("has_visible_mesh") is True:
                    if result["projectile_at_wall_s"] is None:
                        result["projectile_at_wall_s"] = elapsed
                        result["projectile_at_game_s"] = (
                            game_real - game_real0 if game_real is not None and game_real0 is not None
                            else None)
                        result["projectile"] = {
                            "name": actor.get("name"), "class": actor.get("class"),
                            "location": actor.get("location"),
                            "has_visible_mesh": actor.get("has_visible_mesh"),
                        }
                        if not projectile_shot:
                            self.shot("projectile_visible")
                            projectile_shot = True
                    break
                result["invisible_projectile_seen"] = True

            if world_time is not None and world0 is not None:
                result["world_elapsed_s"] = world_time - world0
            time.sleep(POLL_S)

        result["sp_after"] = sp_after
        result["wall_elapsed_s"] = time.monotonic() - wall0
        result["integrity_failures"] = list(dict.fromkeys(result["integrity_failures"]))
        self.observation = result

        def cast_result():
            ok = sp0 is not None and sp_after is not None and abs((sp0 - sp_after) - SP_COST) <= 0.01
            detail = (f"real Slate Q tap; spell_points {sp0} -> {sp_after}; "
                      f"input held wall={input_response.get('held_s')}s, "
                      f"simulated={input_response.get('simulated_s')}s; observation "
                      f"wall={result['wall_elapsed_s']:.3f}s, world={result['world_elapsed_s']}s")
            return ("PASS" if ok else "FAIL"), detail, input_response

        def projectile_result():
            at = result["projectile_at_wall_s"]
            ok = at is not None and at <= PROJECTILE_WINDOW_S
            if ok:
                detail = (f"visible {result.get('projectile')} sampled at wall {at:.3f}s, "
                          f"game real delta {result['projectile_at_game_s']}s")
            elif result["invisible_projectile_seen"]:
                detail = f"projectile actor seen, but has_visible_mesh was never true in {PROJECTILE_WINDOW_S:g}s"
            elif result["projectile_seen"]:
                detail = f"projectile actor seen, but no visible sample within {PROJECTILE_WINDOW_S:g}s"
            else:
                detail = f"no actors[] projectile within {PROJECTILE_WINDOW_S:g}s"
            return ("PASS" if ok else "FAIL"), detail, result.get("projectile")

        def integrity_result():
            failures = result["integrity_failures"]
            detail = ("same exact target remained live, stationary and monotonic; player remained alive"
                      if not failures else "; ".join(failures))
            return ("PASS" if not failures else "FAIL"), detail, failures

        def damage_result():
            at = result["damage_at_wall_s"]
            failures = result["integrity_failures"]
            ok = at is not None and at <= DAMAGE_WINDOW_S and not failures
            if at is None:
                detail = (f"{self.target_name} health stayed {self.target_health:g}; values "
                          f"{result['health_values'] or [self.target_health]}")
            else:
                detail = (f"{self.target_name} health {self.target_health:g} -> "
                          f"{min(result['health_values']):g} at wall {at:.3f}s, game real delta "
                          f"{result['damage_at_game_s']}s")
                if failures:
                    detail += f"; invalidated by {'; '.join(failures)}"
            return ("PASS" if ok else "FAIL"), detail, result["health_values"]

        self.attempt("fireball_cast", cast_result)
        self.attempt("projectile_visible", projectile_result)
        self.attempt("target_integrity", integrity_result)
        self.attempt("same_target_damaged", damage_result)

    def capture_reduced_health_visual(self):
        result = self.observation or {}
        if result.get("damage_at_wall_s") is None or result.get("integrity_failures"):
            self.attempt("reduced_health_visual", lambda: ("PASS", "", None), needs=False,
                         needs_why="not run: timely valid same-target damage was not established")
            return

        def approach_and_capture():
            reduced_health = min(result.get("health_values") or [self.target_health])
            approach = (self.target_location[0] - VISUAL_DISTANCE, self.target_location[1])
            walked = self.bridge.walk_to(approach[0], approach[1], tol=SHOT_TOL)
            state = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player = _player(state)
            exact = self._exact_target(state)
            replacement = self._replacement(state)
            problems = []
            if player is None or player.get("is_dead") is True:
                problems.append("player died or disappeared during visual approach")
            if exact is None:
                problems.append(f"retained target {self.target_name!r} disappeared")
            else:
                position = _xy(exact)
                health, max_health = _health(exact)
                if position is None or _distance(position, self.target_location) > TARGET_MOVE_TOL:
                    problems.append(f"target moved {self.target_location} -> {position}")
                if health != reduced_health:
                    problems.append(f"target health changed after timed hit: {reduced_health} -> {health}")
                if max_health != self.target_max_health:
                    problems.append(
                        f"target max health changed {self.target_max_health} -> {max_health}")
                if exact.get("is_dead") is True:
                    problems.append("target died before supplemental visual")
            if replacement is not None:
                problems.append(f"target replaced by {replacement.get('name')!r}")
            if problems:
                return "FAIL", "; ".join(problems), walked
            screenshot = self.shot("reduced_health_visual")
            return ("PASS", f"retained {self.target_name} still {reduced_health:g}/"
                    f"{self.target_max_health:g} at {self.target_location}; player approached "
                    f"to ({approach[0]:.0f},{approach[1]:.0f}) by real input; {screenshot}", walked)

        self.attempt("reduced_health_visual", approach_and_capture)

    def report_extra(self):
        return {"fireball": self.observation,
                "dedicated_owner_note": ("This script owns US-087 for issue #20's Fireball slice; "
                                         "future Blink acceptance needs separate ownership or "
                                         "explicit consolidation.")}

    def md_sections(self):
        obs = self.observation or {}
        return ["", "## Fireball timing and retained target", "",
                f"- Exact target: `{obs.get('target_name')}` (label `{obs.get('target_label')}`), "
                f"baseline {obs.get('health_before')}/{obs.get('max_health')}",
                f"- Visible projectile: wall {obs.get('projectile_at_wall_s')} s; game real delta "
                f"{obs.get('projectile_at_game_s')} s",
                f"- Same-target damage: wall {obs.get('damage_at_wall_s')} s; game real delta "
                f"{obs.get('damage_at_game_s')} s; health values {obs.get('health_values')}",
                f"- Observation: wall {obs.get('wall_elapsed_s')} s; world delta "
                f"{obs.get('world_elapsed_s')} s; {obs.get('polls')} polls",
                "- Dedicated owner: Fireball AC-087.4/.5 only; Blink remains outside this script."]


Run = Us087FireballRun


def main(argv=None):
    return kit.main(Us087FireballRun, argv)


if __name__ == "__main__":
    raise SystemExit(main())
