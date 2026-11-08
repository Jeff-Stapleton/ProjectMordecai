#!/usr/bin/env python3
"""US-087 issue #16: real-input Fireball crossing a real DevTestMap pickup."""
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
RUN_SUFFIX = "us087-pickup-lane"
PICKUP_LABEL = "TestPickup_MysteriousAmulet"
ACTOR_RADIUS = 2200
APPROACH_DISTANCE = 500.0
WALK_TOL = 35.0
PICKUP_RADIUS, PROJECTILE_RADIUS, TEST_MARGIN = 120.0, 30.0, 25.0
ENVELOPE_RADIUS = PICKUP_RADIUS + PROJECTILE_RADIUS
POLL_S = 0.02
CROSSING_WINDOW_S = 1.5
TAP_S = 0.1
SCREENSHOTS = {"pickup_lane_crossed": "DevTestMap after real Q: pickup remains and the same "
               "Fireball cleared its trigger; continuous state, not the still, proves trajectory"}
PLAN = (
    Step("pickup_lane_preconditions", f"retain native {PICKUP_LABEL}; require live/full player, "
         f"yaw 0, Fireball cost 3 at 10 SP; real-WASD walk_to pickup.x-{APPROACH_DISTANCE:g}, "
         f"same y, within {WALK_TOL:g} cm; require an enemy-free +X lane"),
    Step("fireball_cast", "tap Q through Slate -> verified Fireball cost 3, SP 10 -> 7"),
    Step("pickup_trigger_pass_through", f"retain one exact visible projectile without a gap from "
         f"x < pickup.x-{ENVELOPE_RADIUS:g} through "
         f"x > pickup.x+{ENVELOPE_RADIUS + TEST_MARGIN:g}; a sampled 3D segment intersects the "
         "pickup envelope; capture a game-owned frame"),
    Step("pickup_unchanged", "retained pickup remains present and unchanged; no F input"),
    Step("lane_integrity", "player remains alive/unchanged; no enemy enters the guarded segment"),
)
def _num(value):
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None
def _loc(actor):
    value = (actor or {}).get("location")
    if isinstance(value, list) and len(value) >= 3 and all(_num(v) is not None for v in value[:3]):
        return tuple(float(v) for v in value[:3])
    return None
def _actors(state):
    value = (state or {}).get("actors")
    return [a for a in value if isinstance(a, dict)] if isinstance(value, list) else []
def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None
def _segment_distance(point, start, end):
    delta = tuple(end[i] - start[i] for i in range(3))
    length2 = sum(v * v for v in delta)
    t = 0.0 if length2 == 0 else max(0.0, min(
        1.0, sum((point[i] - start[i]) * delta[i] for i in range(3)) / length2))
    closest = tuple(start[i] + t * delta[i] for i in range(3))
    return math.sqrt(sum((point[i] - closest[i]) ** 2 for i in range(3)))
class Us087PickupLaneRun(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("spells", "projectiles_aim", "pickups")
    TITLE = "US-087 pickup-lane Fireball characterization (Tier 4, real input)"
    RUN_SUFFIX = RUN_SUFFIX
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    MAP = kit.DEFAULT_MAP
    SCRIPT = os.path.abspath(__file__)
    WALK_TOL = WALK_TOL
    TAP_S = TAP_S

    def __init__(self, args):
        super().__init__(args)
        self.pickup_name = self.pickup_location = self.player_name = None
        self.player_health = self.player_max_health = self.spell_cost = None
        self.preconditions_ok = False
    def steps(self):
        self.prepare_lane()
        self.cast_and_track()
    def _exact(self, state, name):
        return next((a for a in _actors(state) if a.get("name") == name), None)
    def _pickup(self, state):
        return next((a for a in _actors(state)
                     if a.get("kind") == "pickup"
                     and (a.get("label") == PICKUP_LABEL or a.get("name") == PICKUP_LABEL)), None)
    def _lane_enemies(self, state, start, end):
        return [a.get("name") or a.get("label") for a in _actors(state)
                if a.get("kind") == "enemy" and _loc(a) is not None
                and _segment_distance(_loc(a), start, end) <= ENVELOPE_RADIUS]
    def prepare_lane(self):
        def prepare():
            state0 = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player0, pickup0 = _player(state0), self._pickup(state0)
            problems = []
            if player0 is None or player0.get("is_dead") is True:
                problems.append("player missing or dead")
            if pickup0 is None:
                problems.append(f"no retained pickup {PICKUP_LABEL}")
            if problems:
                return "FAIL", "; ".join(problems), None
            self.pickup_name = str(pickup0.get("name") or "")
            self.pickup_location = _loc(pickup0)
            self.player_name = str(player0.get("name") or "")
            attrs = player0.get("attributes") or {}
            self.player_health = _num(attrs.get("health"))
            self.player_max_health = _num(attrs.get("max_health"))
            rotation = player0.get("control_rotation") or []
            spells = player0.get("spells") or []
            fireball = next((s for s in spells if s.get("slot") == 0
                             and s.get("display_name") == "Fireball"), {})
            self.spell_cost = _num(fireball.get("sp_cost"))
            sp = _num(attrs.get("spell_points"))
            max_sp = _num(attrs.get("max_spell_points"))
            if not self.pickup_name or self.pickup_location is None:
                problems.append("pickup has no exact name/location")
            if pickup0.get("native_class") != "MordecaiItemPickup":
                problems.append(f"pickup native_class={pickup0.get('native_class')!r}")
            if not self.player_name or self.player_health != self.player_max_health:
                problems.append(f"player not retained/full: {self.player_health}/{self.player_max_health}")
            if len(rotation) < 2 or abs(float(rotation[1])) > 0.01:
                problems.append(f"control yaw is not zero: {rotation}")
            if self.spell_cost != 3.0 or sp != 10.0 or max_sp != 10.0:
                problems.append(f"Fireball/SP contract cost={self.spell_cost}, SP={sp}/{max_sp}")
            if problems:
                return "FAIL", "; ".join(problems), None
            target = (self.pickup_location[0] - APPROACH_DISTANCE, self.pickup_location[1])
            walked = self.bridge.walk_to(*target, tol=WALK_TOL)
            state1 = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player1 = _player(state1)
            pickup1 = self._exact(state1, self.pickup_name)
            player_loc = _loc(player1)
            attrs1 = (player1 or {}).get("attributes") or {}
            health1, max1 = _num(attrs1.get("health")), _num(attrs1.get("max_health"))
            if player1 is None or player1.get("name") != self.player_name or player1.get("is_dead"):
                problems.append("retained player missing/dead after walk")
            if pickup1 is None or _loc(pickup1) != self.pickup_location:
                problems.append("retained pickup missing/moved after walk")
            if health1 != self.player_health or max1 != self.player_max_health:
                problems.append(f"player health changed on walk: {health1}/{max1}")
            if player_loc is None or math.dist(player_loc[:2], target) > WALK_TOL:
                problems.append(f"walk ended at {player_loc}, expected {target}")
            if player_loc:
                far = (self.pickup_location[0] + ENVELOPE_RADIUS + TEST_MARGIN,
                       self.pickup_location[1], self.pickup_location[2])
                enemies = self._lane_enemies(state1, player_loc, far)
                if enemies:
                    problems.append(f"enemy interference in pickup lane: {enemies}")
            if problems:
                return "FAIL", "; ".join(problems), walked
            self.preconditions_ok = True
            return ("PASS", f"retained {self.pickup_name} at {self.pickup_location}; player "
                    f"{self.player_name} at {player_loc}, health {health1}/{max1}, yaw 0; "
                    f"Fireball cost {self.spell_cost:g}, SP {sp:g}/{max_sp:g}; lane clear", walked)
        self.attempt("pickup_lane_preconditions", prepare)
    def cast_and_track(self):
        if not self.preconditions_ok:
            for check in ("fireball_cast", "pickup_trigger_pass_through", "pickup_unchanged",
                          "lane_integrity"):
                self.attempt(check, lambda: ("PASS", "", None), needs=False,
                             needs_why="not run: pickup_lane_preconditions failed; no Q sent")
            return
        before = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
        player0 = _player(before)
        sp0 = _num(((player0 or {}).get("attributes") or {}).get("spell_points"))
        self.bridge.tap("Q", hold_s=TAP_S)
        start = time.monotonic()
        near_x = self.pickup_location[0] - ENVELOPE_RADIUS
        far_x = self.pickup_location[0] + ENVELOPE_RADIUS + TEST_MARGIN
        projectile_name, previous = None, None
        samples, failures = [], []
        pickup_present, far_clear, intersects = True, False, False
        sp_after = sp0
        while time.monotonic() - start <= CROSSING_WINDOW_S and not far_clear:
            state = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player = _player(state)
            attrs = (player or {}).get("attributes") or {}
            if _num(attrs.get("spell_points")) is not None:
                sp_after = _num(attrs.get("spell_points"))
            if self._exact(state, self.pickup_name) is None:
                pickup_present = False
            health, max_health = _num(attrs.get("health")), _num(attrs.get("max_health"))
            if (player is None or player.get("name") != self.player_name or player.get("is_dead")
                    or health != self.player_health or max_health != self.player_max_health):
                failures.append(f"player integrity changed: {health}/{max_health}")
            player_loc = _loc(player)
            far = (far_x, self.pickup_location[1], self.pickup_location[2])
            if player_loc:
                enemies = self._lane_enemies(state, player_loc, far)
                if enemies:
                    failures.append(f"enemy interference: {enemies}")

            projectiles = [a for a in _actors(state) if a.get("kind") == "projectile"
                           and a.get("has_visible_mesh") is True and _loc(a) is not None]
            if projectile_name is None:
                candidate = next((a for a in projectiles
                                  if _loc(a)[0] < near_x
                                  and abs(_loc(a)[1] - self.pickup_location[1]) <= ENVELOPE_RADIUS),
                                 None)
                if candidate:
                    projectile_name, previous = str(candidate.get("name") or ""), _loc(candidate)
                    samples.append(previous)
            else:
                exact = next((a for a in projectiles if a.get("name") == projectile_name), None)
                if exact is None:
                    failures.append(f"projectile identity gap after retaining {projectile_name!r}")
                    break
                current = _loc(exact)
                samples.append(current)
                intersects |= _segment_distance(self.pickup_location, previous, current) \
                    <= ENVELOPE_RADIUS
                previous = current
                far_clear = current[0] > far_x
            if not far_clear:
                time.sleep(POLL_S)
        if far_clear:
            self.shot("pickup_lane_crossed")
        final = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
        pickup_present &= self._exact(final, self.pickup_name) is not None
        self.attempt("fireball_cast", lambda: (
            "PASS" if sp0 == 10.0 and sp_after == sp0 - self.spell_cost else "FAIL",
            f"real Slate Q; verified cost {self.spell_cost:g}; SP {sp0} -> {sp_after}", None))
        self.attempt("pickup_trigger_pass_through", lambda: (
            "PASS" if projectile_name and far_clear and intersects and not failures else "FAIL",
            f"same projectile {projectile_name!r}; {len(samples)} contiguous samples; "
            f"near<{near_x:g}, far>{far_x:g}, cleared={far_clear}, 3D intersection={intersects}; "
            f"failures={list(dict.fromkeys(failures)) or 'none'}", samples))
        self.attempt("pickup_unchanged", lambda: (
            "PASS" if pickup_present else "FAIL",
            f"retained pickup {self.pickup_name!r} "
            f"{'remained present' if pickup_present else 'disappeared'}; no F sent", None))
        self.attempt("lane_integrity", lambda: (
            "PASS" if not failures else "FAIL",
            "retained player stayed alive at unchanged health and lane stayed enemy-free"
            if not failures else "; ".join(dict.fromkeys(failures)), failures))
Run = Us087PickupLaneRun
def main(argv=None):
    return kit.main(Us087PickupLaneRun, argv)
if __name__ == "__main__":
    raise SystemExit(main())
