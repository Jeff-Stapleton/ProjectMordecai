"""Offline contract tests for capture_us099_marketing.py."""

import os
import sys
import tempfile
import unittest
from pathlib import Path

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

    def test_text_render_components_are_accepted_when_the_show_flag_is_off(self):
        state = {
            "game": {},
            "widgets": [{
                "class": "MordecaiCombatHUDWidget",
                "visible": True,
                "texts": ["HP", "SP", "10 / 10"],
            }],
            "actors": [{
                "name": "Pickup",
                "text_labels": [{"text": "Pickup", "visible": True}],
            }],
        }
        proof = capture._marketing_label_proof(state)
        self.assertTrue(proof["passed"])
        self.assertEqual(proof["text_label_violations"], [])
        self.assertEqual(len(proof["text_label_components_suppressed_by_show_flag"]), 1)
        self.assertIs(proof["text_render_show_flag"], False)

    def test_missing_visible_combat_hud_fails_label_proof(self):
        proof = capture._marketing_label_proof({
            "game": {},
            "widgets": [],
            "actors": [],
        })
        self.assertFalse(proof["passed"])
        self.assertFalse(proof["combat_hud_visible"])

    def test_source_sequence_rejects_compacted_or_missing_frame_numbers(self):
        with tempfile.TemporaryDirectory() as root:
            paths = []
            for name in ("frame_000001.png", "frame_000002.png", "frame_000004.png"):
                path = Path(root, name)
                path.touch()
                paths.append(str(path))
            problems = capture._validate_source_frame_sequence(paths)
        self.assertTrue(any("missing frame_000003.png" in problem for problem in problems))

    def test_marketing_frame_directory_argument_quotes_spaces(self):
        argument = capture._marketing_frame_dir_argument(
            r"C:\Users\Test User\Wall Fade\Frames")
        self.assertEqual(
            argument,
            '-MarketingFrameDir="C:\\Users\\Test User\\Wall Fade\\Frames"')

    def test_capture_refuses_a_nonempty_output_directory(self):
        with tempfile.TemporaryDirectory() as root:
            Path(root, "stale.png").touch()
            with self.assertRaisesRegex(RuntimeError, "must be empty"):
                capture.capture(root)

    def test_only_the_continuous_session_runs_in_benchmark_mode(self):
        still_args = capture._capture_extra_args()
        movie_args = capture._capture_extra_args(r"C:\Frames")
        self.assertNotIn("-BENCHMARK", still_args)
        self.assertFalse(any(arg.startswith("-FPS=") for arg in still_args))
        self.assertIn("-BENCHMARK", movie_args)
        self.assertIn("-FPS=30", movie_args)


if __name__ == "__main__":
    unittest.main()
