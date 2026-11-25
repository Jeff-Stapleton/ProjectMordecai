"""Offline contract tests for capture_us099_marketing.py."""

import os
import sys
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import capture_us099_marketing as capture  # noqa: E402


def still(name, fade, x=0.0):
    return {
        "file": name,
        "width": 1920,
        "height": 1080,
        "west_wall": {"name": capture.WALL_NAME, "fade": fade},
        "player": [x, 0.0, 92.0],
    }


class CaptureUs099MarketingTests(unittest.TestCase):
    def test_four_required_phases_pass_the_still_contract(self):
        records = [
            still("approach.png", 1.0),
            still("mid.png", 0.5, -920.0),
            still("faded.png", 0.0, -935.0),
            still("restored.png", 1.0),
        ]
        self.assertEqual(capture._validate_stills(records), [])

    def test_non_transitional_mid_frame_is_rejected(self):
        records = [
            still("approach.png", 1.0),
            still("mid.png", 0.0, -935.0),
            still("faded.png", 0.0, -935.0),
            still("restored.png", 1.0),
        ]
        self.assertTrue(any("mid-fade" in problem
                            for problem in capture._validate_stills(records)))

    def test_wall_lookup_is_case_insensitive(self):
        state = {"camera": {"occluders": [{"name": "arena_wall_w", "fade": 0.25}]}}
        self.assertEqual(capture._occluder(state)["fade"], 0.25)


if __name__ == "__main__":
    unittest.main()
