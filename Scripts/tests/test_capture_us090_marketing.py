"""Offline contract tests for the in-engine US-090 marketing capture."""

import importlib.util
import os
import unittest


SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAPTURE = os.path.join(SCRIPTS, "capture_us090_marketing.py")


def load_capture():
    spec = importlib.util.spec_from_file_location("capture_us090_marketing", CAPTURE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


capture = load_capture()


def good_records():
    return [
        {"index": 1, "elapsed_s": 0.0, "event": "full", "spell_points": 10.0,
         "max_spell_points": 10.0, "is_dead": False, "width": 1920, "height": 1080},
        {"index": 2, "elapsed_s": 0.4, "event": "spend", "spell_points": 7.0,
         "max_spell_points": 10.0, "is_dead": False, "width": 1920, "height": 1080},
        {"index": 3, "elapsed_s": 1.4, "event": "regen", "spell_points": 7.25,
         "max_spell_points": 10.0, "is_dead": False, "width": 1920, "height": 1080},
        {"index": 4, "elapsed_s": 2.4, "event": "regen", "spell_points": 7.5,
         "max_spell_points": 10.0, "is_dead": False, "width": 1920, "height": 1080},
        {"index": 5, "elapsed_s": 5.0, "event": "dead", "spell_points": 8.0,
         "max_spell_points": 10.0, "is_dead": True, "width": 1920, "height": 1080},
        {"index": 6, "elapsed_s": 8.0, "event": "respawn", "spell_points": 10.0,
         "max_spell_points": 10.0, "is_dead": False, "width": 1920, "height": 1080},
    ]


class TestUs090MarketingCaptureContract(unittest.TestCase):
    def test_capture_is_1080p_real_time_and_has_no_benchmark_or_static_fill(self):
        self.assertEqual(capture.OUTPUT_SIZE, (1920, 1080))
        with open(CAPTURE, encoding="utf-8") as stream:
            source = stream.read()
        self.assertIn("r.SetRes 1920x1080w", source)
        self.assertNotIn("-BENCHMARK", source)
        self.assertNotIn('"full_hold"', source)

    def test_contract_requires_spend_real_regen_death_and_respawn_refill(self):
        self.assertEqual(capture.capture_contract_problems(good_records()), [])
        for removed_event in ("spend", "regen", "dead", "respawn"):
            records = [
                record for record in good_records()
                if record["event"] != removed_event
            ]
            with self.subTest(removed_event=removed_event):
                self.assertTrue(capture.capture_contract_problems(records))

    def test_contract_rejects_a_static_tail_after_respawn(self):
        records = good_records()
        records.append(dict(
            records[-1], index=7, elapsed_s=9.0, event="live"))
        problems = capture.capture_contract_problems(records)
        self.assertTrue(any("static tail" in problem for problem in problems), problems)

    def test_contract_requires_the_initial_full_budget(self):
        records = good_records()
        records[0]["spell_points"] = 9.0
        records[1]["spell_points"] = 6.0
        problems = capture.capture_contract_problems(records)
        self.assertTrue(any("initial SP is not full" in problem for problem in problems), problems)

    def test_encoder_durations_preserve_real_elapsed_time_at_30_fps(self):
        durations = capture.frame_durations(good_records())
        self.assertEqual(len(durations), len(good_records()))
        self.assertAlmostEqual(durations[0], 0.4)
        self.assertAlmostEqual(durations[1], 1.0)
        self.assertAlmostEqual(sum(durations), 8.0 + 1.0 / capture.FPS)
        self.assertNotEqual(durations[:-1], [1.0 / capture.FPS] * (len(durations) - 1))
        fast = good_records()[:2]
        fast[1]["elapsed_s"] = 0.01
        self.assertAlmostEqual(capture.frame_durations(fast)[0], 0.01)

    def test_stills_are_full_spend_dead_and_respawn(self):
        self.assertEqual(capture.still_indices(good_records()), [1, 2, 5, 6])


if __name__ == "__main__":
    unittest.main()
