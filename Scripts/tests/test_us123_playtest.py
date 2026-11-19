"""Offline fake-bridge coverage for Scripts/playtests/us123_posture.py."""

import importlib.util
import io
import json
import os
import sys
import tempfile
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import agent_bridge as ab  # noqa: E402
import playtest_boot as pb  # noqa: E402
import playtest_kit as kit  # noqa: E402
from tests import fake_bridge as fb  # noqa: E402
from tests.test_us101_playtest import DefenseWorld  # noqa: E402

PLAYTEST = os.path.join(SCRIPTS, "playtests", "us123_posture.py")
TOKEN = "us123-token"


def load_playtest():
    spec = importlib.util.spec_from_file_location("us123_posture", PLAYTEST)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


us123 = load_playtest()


class PostureWorld(DefenseWorld):
    """Small gameplay double for the real-input route; it is not acceptance evidence."""

    def __init__(self, broken_runtime=True):
        super().__init__(windup_s=0.7, attack_s=0.9, cooldown_s=2.0,
                         click_damage=0.0, arena_walls=False)
        self.PARRY_S = 1.0
        self.health = self.max_health = 1000.0
        self.broken_runtime = broken_runtime
        self.sprinting = False
        self.last_spend = self.real_time()
        self.previous_stamina = self.stamina
        self.enemy_posture = 100.0
        self.enemy_max_posture = 100.0
        self.broken_until = -1.0
        self.down_until = -1.0
        self.stagger_damage = 0.0
        self.combo_index = 0
        self.rt_was = 0.0

    def attacker(self):
        return next(enemy for enemy in self.enemies if self.is_attacker(enemy))

    def tick(self, dt):
        super().tick(dt)
        with self.lock:
            now = self.real_time()
            held = self._held_union()
            moving = any(key in held for key in ("W", "A", "S", "D"))
            if self.sprinting and moving:
                self.stamina = max(-50.0, self.stamina - 12.0 * dt)
            elif now - self.last_spend >= 1.0:
                self.stamina = min(100.0, self.stamina + 15.0 * dt)
            if self.stamina < self.previous_stamina - 0.001:
                self.last_spend = now
            self.previous_stamina = self.stamina

            if self.down_until > 0 and now >= self.down_until:
                self.down_until = -1.0
                self.enemy_posture = self.enemy_max_posture
                self._set_ai(self.attacker(), "Idle", now)
            elif self.broken_until > 0 and now >= self.broken_until:
                self.broken_until = -1.0
                self.enemy_posture = self.enemy_max_posture
                self.stagger_damage = 0.0
                self._set_ai(self.attacker(), "Idle", now)

            rt = abs(float(held.get(us123.RT, 0.0)))
            lt = abs(float(held.get(us123.LT, 0.0)))
            if rt >= 0.5 and self.rt_was < 0.5 and lt < 0.5:
                self._start_player_attack()
            self.rt_was = rt

    def _attacker_tick(self, enemy, now):
        if (self.attacker_parried or now < self.broken_until or now < self.down_until):
            self._set_ai(enemy, "Staggered", now)
            return
        if enemy.get("ai_state") == "Staggered":
            self._set_ai(enemy, "Idle", now)
        super()._attacker_tick(enemy, now)

    def _hit_player(self, now):
        before = len(self.defense_log)
        super()._hit_player(now)
        if len(self.defense_log) > before and self.defense_log[-1][1] == "parried":
            self.last_spend = now
            self.enemy_posture = max(0.0, self.enemy_posture - 50.0)
            if self.enemy_posture <= 0.0 and self.broken_runtime:
                self.broken_until = now + 1.0
                self.stagger_damage = 0.0
                self._set_ai(self.attacker(), "Staggered", now)

    def _start_player_attack(self):
        now = self.real_time()
        combo = us123.TAG_COMBO in self.timed_tags
        self.combo_index = 2 if combo else 1
        stamina_multiplier = 1.25 if self.stamina > 75.0 else (1.0 if self.stamina > 25.0 else 0.75)
        base = 16.0 if self.combo_index == 1 else 18.0
        cost = 8.0 if self.combo_index == 1 else 10.0
        damage = base * stamina_multiplier
        if now < self.down_until:
            damage *= 2.0
        elif now < self.broken_until or (self.riposte and self.attacker_parried):
            damage *= 1.5
        self.stamina -= cost
        self.last_spend = now
        self.timed_tags[us123.TAG_ATTACKING] = now + 0.12

        def strike(world):
            enemy = world.attacker()
            enemy["health"] = max(0.0, enemy["health"] - damage)
            if world.riposte and world.attacker_parried:
                world.riposte = False
            if world.real_time() < world.broken_until:
                world.stagger_damage += damage
                if world.stagger_damage >= 50.0 and world.broken_runtime:
                    world.broken_until = -1.0
                    world.down_until = world.real_time() + 2.0
                    world._set_ai(enemy, "Staggered")
            world.timed_tags[us123.TAG_COMBO] = world.real_time() + 0.5

        self.after(0.03, strike)

    def _on_key_pressed(self, key):
        if key == us123.SPRINT and self.ready and not self.is_dead:
            self.sprinting = not self.sprinting
            return
        super()._on_key_pressed(key)

    def _s_player(self):
        value = super()._s_player()
        if value.get("valid"):
            tags = [tag for tag in value["tags"] if "Stamina.Tier." not in tag]
            if self.stamina > 75.0:
                tags.append(us123.TAG_GREEN)
            elif self.stamina > 25.0:
                tags.append(us123.TAG_YELLOW)
            else:
                tags.append("Mordecai.Stamina.Tier.Red")
            if self.sprinting:
                tags.append(us123.TAG_SPRINTING)
            value["tags"] = tags
            value["attributes"]["stamina"] = round(self.stamina, 3)
            value["weapon"] = {
                "id": "Longsword", "display_name": "Longsword",
                "type": "Longsword", "index": 0, "count": 1,
            }
        return value

    def _actors(self, radius, include_all):
        entries = super()._actors(radius, include_all)
        now = self.real_time()
        for actor in entries:
            if actor.get("kind") != "enemy" or us123.ATTACKER.lower() not in (
                    str(actor.get("label")) + str(actor.get("class"))).lower():
                continue
            actor["posture"] = round(self.enemy_posture, 3)
            actor["max_posture"] = self.enemy_max_posture
            tags = list(actor.get("tags") or [])
            if now < self.broken_until:
                tags.append(us123.TAG_BROKEN)
            if now < self.down_until:
                tags.append(us123.TAG_DOWN)
            actor["tags"] = sorted(set(tags))
        return entries


class Us123PlaytestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)
        self.log = io.StringIO()
        for module in (kit, ab, pb):
            previous = module.set_log_stream(self.log)
            self.addCleanup(module.set_log_stream, previous)
        patcher = mock.patch.object(ab, "SESSION_FILE", os.path.join(self.tmp, "session.json"))
        patcher.start()
        self.addCleanup(patcher.stop)
        self.run_dir = os.path.join(self.tmp, "run")

    def serve(self, **kwargs):
        world = PostureWorld(**kwargs)
        server = fb.FakeBridgeServer(world, token=TOKEN, shot_dir=self.tmp).start()
        self.addCleanup(server.stop)
        return world, server

    def play(self, server):
        passed_image = {
            "path": "fake.png", "width": 1280, "height": 720,
            "rgb_stddev": [10.0, 10.0, 10.0], "non_blank": True, "passed": True,
        }
        with mock.patch.object(us123, "_image_stats", return_value=passed_image):
            code = us123.main([
                "--attach-port", str(server.port), "--token", TOKEN,
                "--run-dir", self.run_dir,
            ])
        with open(os.path.join(self.run_dir, "report.json"), encoding="utf-8") as stream:
            return code, json.load(stream)

    @staticmethod
    def checks(report):
        return {check["name"]: check for check in report["checks"]}

    def test_plan_contract(self):
        self.assertEqual(kit.plan_problems(us123.Us123Run), [])
        self.assertIs(us123.Run, us123.Us123Run)
        self.assertEqual(us123.Us123Run.launch_map(), kit.COMBAT_TEST_MAP)
        self.assertEqual(
            [step.check for step in us123.PLAN],
            ["setup", "unbroken_control", "break_by_parry", "staggered_hit",
             "recovers", "knockdown", "image_verification"])
        self.assertEqual(
            set(us123.Us123Run.PENDING),
            {"AC-123.11"},
        )

    def test_helpers_read_enemy_posture(self):
        state = {"actors": [{
            "kind": "enemy", "label": "TestAttacker", "health": {"current": 150, "max": 200},
            "posture": 25, "max_posture": 100, "tags": [us123.TAG_BROKEN],
        }]}
        actor = us123.find_attacker(state)
        self.assertEqual(us123._health(actor), 150.0)
        self.assertEqual(us123._posture(actor), 25.0)
        self.assertEqual(us123._max_posture(actor), 100.0)

    def test_full_real_input_route_passes_against_fake_bridge(self):
        world, server = self.serve()
        code, report = self.play(server)
        checks = self.checks(report)
        self.assertEqual(code, 0, self.log.getvalue()[-5000:])
        self.assertEqual([name for name, value in checks.items() if value["status"] == "FAIL"], [])
        self.assertAlmostEqual(report["control_damage"], 16.0)
        self.assertAlmostEqual(report["stagger_l1"], 24.0)
        self.assertEqual(report["knockdown_route"], [24.0, 27.0])
        self.assertGreater(world.down_until, world.real_time())
        sent = [request.get("cmd") for request in server.received]
        self.assertLessEqual(
            set(sent), {
                "ping", "state", "key", "axis", "mouse", "screenshot", "wait", "wait_until",
            })
        self.assertTrue(any(request.get("cmd") == "axis" and us123.LT in request.get("values", {})
                            for request in server.received))
        self.assertTrue(any(request.get("cmd") == "axis" and us123.RT in request.get("values", {})
                            for request in server.received))

    def test_missing_runtime_fails_posture_acceptance(self):
        _, server = self.serve(broken_runtime=False)
        code, report = self.play(server)
        checks = self.checks(report)
        self.assertEqual(code, 1)
        self.assertEqual(checks["break_by_parry"]["status"], "FAIL")
        self.assertIn("missing 'Mordecai.State.PostureBroken'", checks["break_by_parry"]["detail"])
        self.assertIn("break_by_parry", report["new_failures"])


if __name__ == "__main__":
    unittest.main()
