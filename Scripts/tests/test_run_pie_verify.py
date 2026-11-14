"""Offline contracts for Tier-2 smoke-test selection."""

import os
import sys
import unittest


SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import playtest_boot as pb  # noqa: E402
import run_pie_verify as rpv  # noqa: E402


class TestMapSpecificDefaults(unittest.TestCase):
    def defaults(self, map_path):
        self.assertTrue(
            hasattr(rpv, "default_smoke_tests"),
            "run_pie_verify.default_smoke_tests is not implemented",
        )
        return rpv.default_smoke_tests(map_path)

    def test_vw_tile_map_runs_its_content_smoke_by_default(self):
        self.assertEqual(
            self.defaults(pb.VW_TILE_TEST_MAP),
            ["spawn", "hud", "test_maps", "vw_tiles"],
        )

    def test_other_test_maps_keep_the_any_map_defaults(self):
        self.assertEqual(
            self.defaults(pb.MOVEMENT_TEST_MAP),
            ["spawn", "hud", "test_maps"],
        )


if __name__ == "__main__":
    unittest.main()
