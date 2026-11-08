#!/usr/bin/env python3
"""US-087 Blink recovery -- dedicated Tier 4 owner for issue #25 / N20.

On CombatTestMap this script retains the passive TestDummy, walks to 80-130 cm west of it through
real WASD, then presses and releases E through Slate. It continuously audits the same player and
target while independently checking the exact 2 SP cost, >300 cm +X motion within two seconds,
and an Invulnerable tag that is first observed granted and then absent within 1.5 seconds.
Game-owned pre/post frames are supplemental; sampled state is the motion proof.
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
TITLE = "US-087 Blink recovery playtest (Tier 4, real input)"
RUN_SUFFIX = "us087-blink"

TARGET_LABEL = "TestDummy"
TARGET_CLASS = "BP_MordecaiEnemy_TestDummy_C"
TARGET_SPAWN = (600.0, 600.0)
ACTOR_RADIUS = 3000
APPROACH_DISTANCE = 100.0
APPROACH_TOL = 25.0
MIN_GAP = 80.0
MAX_GAP = 130.0
LINE_TOL = 30.0
HEIGHT_TOL = 20.0
TARGET_MOVE_TOL = 10.0
BASELINE_WATCH_S = 0.2
MOVE_WINDOW_S = 2.0
INVULN_END_WINDOW_S = 1.5
POLL_S = 0.03
SP_BEFORE = 10.0
SP_COST = 2.0
MIN_X_MOVEMENT = 300.0
MAX_Y_MOVEMENT = 35.0
MAX_Z_MOVEMENT = 10.0
INVULN_TAG = "Mordecai.State.Invulnerable"
STONE_SKIN_TAG = "Mordecai.Status.StoneSkin"
KNOWN_N20 = "N20 / issue #25: Blink spends SP but does not move when a Pawn is about 1 m ahead"

SCREENSHOTS = {
    "pre_blink": "CombatTestMap immediately before E: the live full-health passive TestDummy is "
                 "about 1 m directly ahead on +X; player SP is 10/10",
    "post_blink": "CombatTestMap after the timed audit: supplemental post-cast frame; sampled "
                  "player state, not pixels, proves motion and the 10-to-8 SP change",
}

PLAN = (
    Step("blink_preconditions", f"state player,actors -> retain exact live/full-health stationary "
         f"{TARGET_LABEL}; real-WASD walk_to about {APPROACH_DISTANCE:g} cm -X; require a "
         f"{MIN_GAP:g}-{MAX_GAP:g} cm +X gap, low Y/Z offset, yaw 0, live full-health stationary "
         f"player, exact SP {SP_BEFORE:g}, granted slot-1 E/Blink cost {SP_COST:g}, and neither "
         "Stone Skin nor Invulnerable; capture a supplemental pre-cast frame"),
    Step("blink_past_enemy", f"press E through Slate and release it in finally; continuously sample "
         f"the retained player/target -> SP {SP_BEFORE:g}->{SP_BEFORE - SP_COST:g} and player +X "
         f"movement > {MIN_X_MOVEMENT:g} cm within {MOVE_WINDOW_S:g}s with low Y/Z drift",
         proves=("AC-087.8",), known_issue=KNOWN_N20),
    Step("blink_integrity", "through the full timed audit the exact player stays live, the exact "
         "passive target is not replaced/moved/damaged/killed, and Stone Skin never appears",
         proves=("AC-087.8",)),
    Step("blink_invulnerability_ends", f"continuous player.tags sampling must first observe "
         f"{INVULN_TAG} granted, then observe it absent within {INVULN_END_WINDOW_S:g}s of E; "
         "never granted does not pass; capture a supplemental post-cast frame",
         proves=("AC-087.9",)),
)


def _num(value):
    return float(value) if isinstance(value, (int, float)) and not isinstance(value, bool) else None


def _player(state):
    value = (state or {}).get("player")
    return value if isinstance(value, dict) and value.get("valid") is True else None


def _actors(state):
    value = (state or {}).get("actors")
    return [a for a in value if isinstance(a, dict)] if isinstance(value, list) else []


def _xyz(value):
    loc = value.get("location") if isinstance(value, dict) else None
    if not isinstance(loc, list) or len(loc) < 3 or any(_num(v) is None for v in loc[:3]):
        return None
    return tuple(float(v) for v in loc[:3])


def _health(value):
    health = value.get("health") if isinstance(value, dict) else None
    if not isinstance(health, dict):
        return None, None
    return _num(health.get("current")), _num(health.get("max"))


def _player_health(value):
    attrs = value.get("attributes") if isinstance(value, dict) else None
    if not isinstance(attrs, dict):
        return None, None
    return _num(attrs.get("health")), _num(attrs.get("max_health"))


def _distance3(a, b):
    return math.sqrt(sum((a[i] - b[i]) ** 2 for i in range(3)))


def _yaw(value):
    rotation = value if isinstance(value, list) and len(value) >= 2 else None
    return _num(rotation[1]) if rotation else None


def _yaw_error(value):
    return abs((float(value) + 180.0) % 360.0 - 180.0)


def _tags(player):
    values = (player or {}).get("tags")
    return {str(v) for v in values} if isinstance(values, list) else set()


def _spell_points(player):
    attrs = (player or {}).get("attributes")
    return _num(attrs.get("spell_points")) if isinstance(attrs, dict) else None


class Us087BlinkRun(kit.PlaytestRun):
    STORY = STORY
    FEATURES = ("spells", "status_effects")
    TITLE = TITLE
    RUN_SUFFIX = RUN_SUFFIX
    SCREENSHOTS = SCREENSHOTS
    PLAN = PLAN
    MAP = kit.COMBAT_TEST_MAP
    SCRIPT = os.path.abspath(__file__)
    WALK_TOL = APPROACH_TOL

    def __init__(self, args):
        super().__init__(args)
        self.player_name = None
        self.target_name = None
        self.target_label = None
        self.target_location = None
        self.target_health = None
        self.target_max_health = None
        self.cast_origin = None
        self.preconditions_ok = False
        self.observation = {}

    def steps(self):
        self.prepare_blink()
        self.cast_and_observe()

    def _select_target(self, state):
        enemies = [a for a in _actors(state) if a.get("kind") == "enemy"]
        exact = [a for a in enemies if a.get("label") == TARGET_LABEL
                 and a.get("class") == TARGET_CLASS]
        candidates = exact or [a for a in enemies
                               if TARGET_LABEL.lower() in str(a.get("label") or "").lower()]
        if not candidates:
            return None
        return min(candidates, key=lambda a: math.hypot(
            (_xyz(a) or (1e9, 1e9, 0.0))[0] - TARGET_SPAWN[0],
            (_xyz(a) or (1e9, 1e9, 0.0))[1] - TARGET_SPAWN[1]))

    def _exact_target(self, state):
        return next((a for a in _actors(state) if a.get("name") == self.target_name), None)

    def _replacement(self, state):
        for actor in _actors(state):
            if actor.get("kind") != "enemy" or actor.get("name") == self.target_name:
                continue
            pos = _xyz(actor)
            if (self.target_label and actor.get("label") == self.target_label) or (
                    pos is not None and self.target_location is not None
                    and _distance3(pos, self.target_location) <= TARGET_MOVE_TOL):
                return actor
        return None

    @staticmethod
    def _blink_slot(player):
        spells = player.get("spells") if isinstance(player, dict) else None
        if not isinstance(spells, list):
            return None
        return next((spell for spell in spells if isinstance(spell, dict)
                     and spell.get("slot") == 1), None)

    def prepare_blink(self):
        def prepare():
            state0 = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player0 = _player(state0)
            target0 = self._select_target(state0)
            problems = []
            if player0 is None or player0.get("is_dead") is True:
                problems.append("player is missing or dead")
            if target0 is None:
                problems.append(f"no exact passive {TARGET_LABEL}/{TARGET_CLASS}")
            if problems:
                return "FAIL", "; ".join(problems), None

            self.player_name = str(player0.get("name") or "")
            self.target_name = str(target0.get("name") or "")
            self.target_label = str(target0.get("label") or "")
            self.target_location = _xyz(target0)
            self.target_health, self.target_max_health = _health(target0)
            if not self.player_name:
                problems.append("player has no exact name")
            if not self.target_name or self.target_location is None:
                problems.append("target has no exact name/location")
            if target0.get("class") != TARGET_CLASS or target0.get("ai_state") != "Idle":
                problems.append(f"target is not passive {TARGET_CLASS}/Idle")
            if target0.get("is_dead") is True or self.target_health is None \
                    or self.target_max_health is None or self.target_health <= 0:
                problems.append("target is not live with readable health")
            elif abs(self.target_health - self.target_max_health) > 0.01:
                problems.append(f"target is not full: {self.target_health:g}/"
                                f"{self.target_max_health:g}")
            if problems:
                return "FAIL", "; ".join(problems), None

            self.bridge.wait(BASELINE_WATCH_S)
            state1 = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            target1 = self._exact_target(state1)
            location1 = _xyz(target1)
            health1, max1 = _health(target1)
            if target1 is None or location1 is None:
                return "FAIL", f"retained target {self.target_name!r} disappeared", None
            if _distance3(location1, self.target_location) > TARGET_MOVE_TOL:
                return "FAIL", f"target moved during baseline: {self.target_location} -> {location1}", None
            if health1 != self.target_health or max1 != self.target_max_health \
                    or target1.get("is_dead") is True:
                return "FAIL", f"target changed during baseline: {health1}/{max1}", None

            approach = (location1[0] - APPROACH_DISTANCE, location1[1])
            walked = self.bridge.walk_to(approach[0], approach[1], tol=APPROACH_TOL)
            state2 = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            player2 = _player(state2)
            target2 = self._exact_target(state2)
            replacement = self._replacement(state2)
            player_pos = _xyz(player2)
            target_pos = _xyz(target2)
            hp2, max2 = _health(target2)
            if player2 is None or target2 is None or player_pos is None or target_pos is None:
                problems.append("player or exact target disappeared after walk")
            else:
                dx = target_pos[0] - player_pos[0]
                dy = target_pos[1] - player_pos[1]
                dz = target_pos[2] - player_pos[2]
                if not (MIN_GAP <= dx <= MAX_GAP):
                    problems.append(f"target gap dx={dx:.1f}, expected {MIN_GAP:g}-{MAX_GAP:g}")
                if abs(dy) > LINE_TOL or abs(dz) > HEIGHT_TOL:
                    problems.append(f"target offset dy={dy:.1f}, dz={dz:.1f}")
            if replacement is not None:
                problems.append(f"target replaced by {replacement.get('name')!r}")
            if player2 and (player2.get("is_dead") is True):
                problems.append("player died before Blink")
            player_hp, player_max = _player_health(player2)
            speed = _num((player2 or {}).get("speed"))
            if player_hp is None or player_max is None or player_hp <= 0 \
                    or abs(player_hp - player_max) > 0.01:
                problems.append(f"player is not live/full: {player_hp}/{player_max}")
            if speed is None or speed > 1.0:
                problems.append(f"player is not stationary: speed={speed}")
            actor_yaw = _yaw((player2 or {}).get("rotation"))
            control_yaw = _yaw((player2 or {}).get("control_rotation"))
            if actor_yaw is None or _yaw_error(actor_yaw) > 2.0:
                problems.append(f"player actor yaw is not +X/0: {actor_yaw}")
            if control_yaw is None or _yaw_error(control_yaw) > 2.0:
                problems.append(f"control yaw is not +X/0: {control_yaw}")
            sp = _spell_points(player2)
            if sp is None or abs(sp - SP_BEFORE) > 0.01:
                problems.append(f"spell_points is {sp}, expected exactly {SP_BEFORE:g}")
            slot = self._blink_slot(player2)
            if slot is None or slot.get("key") != "E" or slot.get("spell_id") != "Blink" \
                    or slot.get("display_name") != "Blink" or slot.get("granted") is not True \
                    or abs((_num(slot.get("sp_cost")) or -999.0) - SP_COST) > 0.01:
                problems.append(f"slot 1 is not granted E/Blink cost {SP_COST:g}: {slot}")
            contaminated = sorted(_tags(player2) & {INVULN_TAG, STONE_SKIN_TAG})
            if contaminated:
                problems.append(f"pre-cast tag contamination: {contaminated}")
            if target2 and (_distance3(target_pos, self.target_location) > TARGET_MOVE_TOL
                            or hp2 != self.target_health or max2 != self.target_max_health
                            or target2.get("is_dead") is True):
                problems.append(f"target changed before Blink: {hp2}/{max2} at {target_pos}")
            if problems:
                return "FAIL", "; ".join(problems), walked

            self.target_location = target_pos
            self.cast_origin = player_pos
            self.preconditions_ok = True
            screenshot = self.shot("pre_blink")
            return ("PASS", f"retained {self.target_label}/{self.target_name} live, full "
                    f"{hp2:g}/{max2:g}, stationary; player {self.player_name} full "
                    f"{player_hp:g}/{player_max:g}, stationary at {player_pos}; gap "
                    f"dx={dx:.1f}, dy={dy:.1f}, dz={dz:.1f}; yaw actor/control "
                    f"{actor_yaw:g}/{control_yaw:g}; SP {sp:g}; {screenshot}", walked)

        self.attempt("blink_preconditions", prepare)

    def cast_and_observe(self):
        if not self.preconditions_ok:
            why = "not run: blink_preconditions failed; no E input sent"
            for check in ("blink_past_enemy", "blink_integrity",
                          "blink_invulnerability_ends"):
                self.attempt(check, lambda: ("PASS", "", None), needs=False, needs_why=why)
            return

        before = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
        player_before = _player(before)
        sp0 = _spell_points(player_before)
        wall0 = time.monotonic()
        input_response = None
        release_response = None
        try:
            input_response = self.bridge.press("E")
        finally:
            release_response = self.bridge.release("E")

        result = {
            "player_name": self.player_name,
            "target_name": self.target_name,
            "target_label": self.target_label,
            "origin": list(self.cast_origin),
            "target_location": list(self.target_location),
            "target_health": self.target_health,
            "target_max_health": self.target_max_health,
            "sp_before": sp0,
            "sp_values": [],
            "input": input_response,
            "release": release_response,
            "movement_at_wall_s": None,
            "invulnerable_granted_at_wall_s": None,
            "invulnerable_cleared_at_wall_s": None,
            "integrity_failures": [],
            "samples": [],
            "polls": 0,
        }
        end_s = max(MOVE_WINDOW_S, INVULN_END_WINDOW_S)
        final_player = player_before

        while time.monotonic() - wall0 <= end_s:
            state = self.bridge.state(["player", "actors"], radius=ACTOR_RADIUS)
            elapsed = time.monotonic() - wall0
            result["polls"] += 1
            player = _player(state)
            exact = self._exact_target(state)
            replacement = self._replacement(state)
            if player is None:
                result["integrity_failures"].append("player disappeared")
            else:
                final_player = player
                position = _xyz(player)
                sp = _spell_points(player)
                tags = _tags(player)
                if not result["sp_values"] or result["sp_values"][-1] != sp:
                    result["sp_values"].append(sp)
                if player.get("name") != self.player_name:
                    result["integrity_failures"].append(
                        f"player replaced {self.player_name!r} -> {player.get('name')!r}")
                if player.get("is_dead") is True:
                    result["integrity_failures"].append("player died")
                if STONE_SKIN_TAG in tags:
                    result["integrity_failures"].append("Stone Skin appeared")
                if INVULN_TAG in tags and result["invulnerable_granted_at_wall_s"] is None:
                    result["invulnerable_granted_at_wall_s"] = elapsed
                if result["invulnerable_granted_at_wall_s"] is not None \
                        and INVULN_TAG not in tags \
                        and result["invulnerable_cleared_at_wall_s"] is None:
                    result["invulnerable_cleared_at_wall_s"] = elapsed
                if position is None:
                    result["integrity_failures"].append("player location became unreadable")
                else:
                    dx = position[0] - self.cast_origin[0]
                    dy = position[1] - self.cast_origin[1]
                    dz = position[2] - self.cast_origin[2]
                    if result["movement_at_wall_s"] is None and dx > MIN_X_MOVEMENT \
                            and abs(dy) <= MAX_Y_MOVEMENT and abs(dz) <= MAX_Z_MOVEMENT:
                        result["movement_at_wall_s"] = elapsed
                    result["samples"].append({
                        "wall_s": round(elapsed, 4), "location": list(position),
                        "delta": [dx, dy, dz], "spell_points": sp,
                        "invulnerable": INVULN_TAG in tags,
                    })

            if exact is None:
                result["integrity_failures"].append(f"target {self.target_name!r} disappeared")
                if replacement is not None:
                    result["integrity_failures"].append(
                        f"target replaced by {replacement.get('name')!r}")
            else:
                target_pos = _xyz(exact)
                health, max_health = _health(exact)
                if target_pos is None or _distance3(target_pos, self.target_location) > TARGET_MOVE_TOL:
                    result["integrity_failures"].append(
                        f"target moved {self.target_location} -> {target_pos}")
                if health != self.target_health or max_health != self.target_max_health:
                    result["integrity_failures"].append(
                        f"target health changed {self.target_health}/{self.target_max_health} "
                        f"-> {health}/{max_health}")
                if exact.get("is_dead") is True:
                    result["integrity_failures"].append("target died")
            time.sleep(POLL_S)

        result["wall_elapsed_s"] = time.monotonic() - wall0
        final_position = _xyz(final_player)
        result["final_location"] = list(final_position) if final_position else None
        result["sp_after"] = _spell_points(final_player)
        result["final_tags"] = sorted(_tags(final_player))
        result["integrity_failures"] = list(dict.fromkeys(result["integrity_failures"]))
        self.observation = result
        post_shot = self.shot("post_blink")
        result["post_screenshot"] = post_shot

        def movement_result():
            final = final_position
            dx = final[0] - self.cast_origin[0] if final else None
            dy = final[1] - self.cast_origin[1] if final else None
            dz = final[2] - self.cast_origin[2] if final else None
            sp_after = result["sp_after"]
            failures = []
            if sp0 is None or abs(sp0 - SP_BEFORE) > 0.01 or sp_after is None \
                    or abs(sp_after - (SP_BEFORE - SP_COST)) > 0.01:
                failures.append(f"spell_points {sp0} -> {sp_after}, expected "
                                f"{SP_BEFORE:g}->{SP_BEFORE - SP_COST:g}")
            if result["movement_at_wall_s"] is None:
                failures.append(f"no sampled +X movement > {MIN_X_MOVEMENT:g} cm with low Y/Z "
                                f"within {MOVE_WINDOW_S:g}s")
            if dx is None or dx <= MIN_X_MOVEMENT:
                failures.append(f"final +X movement {dx}, expected > {MIN_X_MOVEMENT:g} cm")
            if dy is None or abs(dy) > MAX_Y_MOVEMENT or dz is None or abs(dz) > MAX_Z_MOVEMENT:
                failures.append(f"final off-axis movement dy={dy}, dz={dz}")
            if result["integrity_failures"]:
                failures.append("integrity invalidated: " + "; ".join(result["integrity_failures"]))
            detail = (f"real Slate E press/release; SP {sp0}->{sp_after}; delta "
                      f"({dx},{dy},{dz}); first qualifying movement "
                      f"{result['movement_at_wall_s']} s; {result['polls']} polls")
            if failures:
                detail += "; " + "; ".join(failures)
            return ("PASS" if not failures else "FAIL"), detail, result["samples"]

        def integrity_result():
            failures = result["integrity_failures"]
            detail = ("exact player stayed live and exact passive target stayed live, stationary, "
                      "full-health and unchanged; no Stone Skin"
                      if not failures else "; ".join(failures))
            return ("PASS" if not failures else "FAIL"), detail, failures

        def invulnerability_result():
            granted = result["invulnerable_granted_at_wall_s"]
            cleared = result["invulnerable_cleared_at_wall_s"]
            ok = granted is not None and cleared is not None and cleared <= INVULN_END_WINDOW_S
            if granted is None:
                detail = f"{INVULN_TAG} was never observed granted after E"
            elif cleared is None:
                detail = f"{INVULN_TAG} was granted at {granted:.3f}s but never observed absent"
            elif cleared > INVULN_END_WINDOW_S:
                detail = f"{INVULN_TAG} cleared at {cleared:.3f}s, after {INVULN_END_WINDOW_S:g}s"
            else:
                detail = f"{INVULN_TAG} observed granted at {granted:.3f}s and absent at " \
                         f"{cleared:.3f}s; {post_shot}"
            return ("PASS" if ok else "FAIL"), detail, {
                "granted_at_wall_s": granted, "cleared_at_wall_s": cleared,
                "deadline_s": INVULN_END_WINDOW_S,
            }

        self.attempt("blink_past_enemy", movement_result)
        self.attempt("blink_integrity", integrity_result)
        self.attempt("blink_invulnerability_ends", invulnerability_result)

    def report_extra(self):
        return {
            "blink": self.observation,
            "motion_proof": "Continuous sampled player.location; screenshots are supplemental.",
            "dedicated_owner_note": "This script owns only US-087 Blink AC-087.8/.9.",
        }

    def md_sections(self):
        obs = self.observation or {}
        return ["", "## Blink timing and retained actors", "",
                f"- Exact player: `{obs.get('player_name')}`; origin {obs.get('origin')}; "
                f"final {obs.get('final_location')}",
                f"- Exact target: `{obs.get('target_name')}` (label `{obs.get('target_label')}`), "
                f"{obs.get('target_health')}/{obs.get('target_max_health')} at "
                f"{obs.get('target_location')}",
                f"- SP: {obs.get('sp_before')} -> {obs.get('sp_after')}; qualifying movement at "
                f"{obs.get('movement_at_wall_s')} s",
                f"- Invulnerable: granted at {obs.get('invulnerable_granted_at_wall_s')} s; "
                f"absent at {obs.get('invulnerable_cleared_at_wall_s')} s",
                f"- Audit: {obs.get('polls')} polls over {obs.get('wall_elapsed_s')} s; "
                f"integrity failures {obs.get('integrity_failures')}",
                "- Motion proof is continuous sampled state; pre/post screenshots are supplemental."]


Run = Us087BlinkRun


def main(argv=None):
    return kit.main(Us087BlinkRun, argv)


if __name__ == "__main__":
    raise SystemExit(main())
