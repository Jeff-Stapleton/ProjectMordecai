"""Offline contract tests for Scripts/capture_us096_marketing.py."""

import os
import sys
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import capture_us096_marketing as capture  # noqa: E402


def records(sequence=capture.EXPECTED_SEQUENCE, duration=6.5):
    count = len(sequence)
    return [
        {
            "index": index + 1,
            "weapon": weapon,
            "elapsed_s": duration * index / (count - 1),
            "width": 1920,
            "height": 1080,
            "shift_tab": index >= 3,
            "sprinting_before": False,
            "sprinting_after": False,
        }
        for index, weapon in enumerate(sequence)
    ]


class TestUs096MarketingContract(unittest.TestCase):
    def test_valid_forward_and_backward_capture_passes(self):
        sample = records()
        proofs = [{"phase": "start", "passed": True}, {"phase": "end", "passed": True}]
        self.assertEqual(capture.weapon_sequence(sample),
                         list(capture.EXPECTED_SEQUENCE))
        self.assertEqual(capture.still_indices(sample), [1, 2, 3, 4, 5])
        self.assertEqual(capture.capture_contract_problems(sample, proofs), [])

    def test_static_weapon_footage_fails(self):
        sample = records(("Longsword",) * 5)
        problems = capture.capture_contract_problems(
            sample, [{"phase": "start", "passed": True}])
        self.assertTrue(any("weapon sequence" in problem for problem in problems))
        self.assertTrue(any("still count" in problem for problem in problems))

    def test_shift_tab_sprint_change_fails(self):
        sample = records()
        sample[3]["sprinting_after"] = True
        problems = capture.capture_contract_problems(
            sample, [{"phase": "start", "passed": True}])
        self.assertTrue(any("changed sprint state" in problem for problem in problems))

    def test_wrong_resolution_and_label_overlay_fail(self):
        sample = records()
        sample[0]["width"] = 1280
        problems = capture.capture_contract_problems(
            sample, [{"phase": "start", "passed": False}])
        self.assertTrue(any("not 1920x1080" in problem for problem in problems))
        self.assertTrue(any("label proof failed" in problem for problem in problems))

    def test_clip_duration_must_be_real_time_five_to_twenty_seconds(self):
        too_short = records(duration=4.9)
        too_long = records(duration=20.1)
        proof = [{"phase": "start", "passed": True}]
        self.assertTrue(any("outside 5-20s" in problem
                            for problem in capture.capture_contract_problems(too_short, proof)))
        self.assertTrue(any("outside 5-20s" in problem
                            for problem in capture.capture_contract_problems(too_long, proof)))


if __name__ == "__main__":
    unittest.main()
