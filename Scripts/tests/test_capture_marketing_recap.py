"""Offline contract tests for the clean marketing recapture orchestration."""

import os
import sys
import tempfile
import unittest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import capture_marketing_recap as recap  # noqa: E402


class MarketingRecapTests(unittest.TestCase):
    def test_capture_reuses_fixed_30_fps_path_without_freezing_ai(self):
        with tempfile.TemporaryDirectory() as root:
            args = recap._capture_args(root)
        self.assertIn("-MordecaiMarketingCapture", args)
        self.assertIn("-MarketingFrameCapture", args)
        self.assertIn("-BENCHMARK", args)
        self.assertIn("-FPS=30", args)
        self.assertNotIn("-MordecaiMarketingFreezeAI", args)

    def test_recapture_uses_devtest_map_and_1080p(self):
        self.assertEqual(recap.MAP, recap.kit.DEFAULT_MAP)
        self.assertEqual(recap.OUTPUT_SIZE, (1920, 1080))
        self.assertEqual(recap.MIN_FRAMES, 150)
        self.assertEqual(recap.MAX_FRAMES, 600)

    def test_weapon_plate_requires_visible_matching_widget_text(self):
        state = {
            "widgets": [{
                "class": "MordecaiEquippedWeaponWidget",
                "visible": True,
                "texts": ["Greatsword"],
            }],
        }
        self.assertTrue(recap._visible_weapon_plate(state, "Greatsword"))
        self.assertFalse(recap._visible_weapon_plate(state, "Dagger"))


if __name__ == "__main__":
    unittest.main()
