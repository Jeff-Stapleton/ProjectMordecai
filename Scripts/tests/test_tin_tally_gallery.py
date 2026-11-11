"""Offline tests for the US-103 packaged Tin Tally gallery harness."""

import os
import sys
import tempfile
import unittest
from unittest import mock

from PIL import Image

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import tin_tally_gallery as gallery  # noqa: E402

FIXTURES = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), "fixtures", "tin_tally_gallery"
)


class TempImageCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)

    def image(self, name, color=(20, 80, 70, 255), size=(64, 64)):
        path = os.path.join(self.tmp, name)
        Image.new("RGBA", size, color).save(path)
        return path

    @staticmethod
    def build(commit="a" * 40, dirty=False):
        return {
            "commit": commit,
            "dirty": dirty,
            "built_at": "2026-11-11T10:00:00-07:00",
            "config": "Development",
        }

    def metadata(self, theme="diorama", commit="a" * 40):
        return {
            "schema": 1,
            "component": "Plate",
            "theme": theme,
            "state": "sheet",
            "commit": commit,
            "package_built_at": "2026-11-11T10:00:00-07:00",
            "resolution": [1920, 1080],
            "crop_rect": [480, 120, 960, 720],
        }


class TestMetadata(TempImageCase):
    def test_metadata_records_capture_identity_and_package(self):
        metadata = gallery.make_capture_metadata(
            "Plate", "diorama", "sheet", self.build(), [480, 120, 960, 720]
        )
        self.assertIn("component", metadata)
        self.assertEqual(metadata["component"], "Plate")
        self.assertEqual(metadata["theme"], "diorama")
        self.assertEqual(metadata["state"], "sheet")
        self.assertEqual(metadata["commit"], "a" * 40)
        self.assertEqual(metadata["resolution"], [1920, 1080])
        self.assertEqual(metadata["crop_rect"], [480, 120, 960, 720])


class TestCaptureValidation(TempImageCase):
    def test_accepts_a_fresh_readable_capture_in_the_requested_theme(self):
        capture = self.image("capture.png")
        reasons = gallery.validate_capture(
            capture, self.metadata(), self.build(), "diorama"
        )
        self.assertEqual(reasons, [])

    def test_rejects_missing_capture(self):
        reasons = gallery.validate_capture(
            os.path.join(self.tmp, "missing.png"),
            self.metadata(),
            self.build(),
            "diorama",
        )
        self.assertTrue(any("missing" in reason.lower() for reason in reasons), reasons)

    def test_rejects_stale_commit_metadata(self):
        capture = self.image("capture.png")
        reasons = gallery.validate_capture(
            capture, self.metadata(commit="b" * 40), self.build(), "diorama"
        )
        self.assertTrue(any("stale" in reason.lower() for reason in reasons), reasons)

    def test_rejects_black_capture(self):
        capture = self.image("black.png", color=(0, 0, 0, 255))
        reasons = gallery.validate_capture(
            capture, self.metadata(), self.build(), "diorama"
        )
        self.assertTrue(any("black" in reason.lower() for reason in reasons), reasons)

    def test_rejects_wrong_theme_metadata(self):
        capture = self.image("capture.png")
        reasons = gallery.validate_capture(
            capture, self.metadata(theme="high-contrast"), self.build(), "diorama"
        )
        self.assertTrue(any("theme" in reason.lower() for reason in reasons), reasons)

    def test_rejects_wrong_resolution(self):
        capture = self.image("capture.png")
        metadata = self.metadata()
        metadata["resolution"] = [1280, 720]
        reasons = gallery.validate_capture(capture, metadata, self.build(), "diorama")
        self.assertTrue(any("1920" in reason for reason in reasons), reasons)


class TestImageComparison(TempImageCase):
    def test_identical_geometry_passes_and_writes_review_evidence(self):
        reference = self.image("reference.png", size=(96, 64))
        capture = self.image("capture.png", size=(96, 64))
        result = gallery.compare_images(reference, capture, self.tmp, "Plate.diorama")
        self.assertTrue(result["passed"], result)
        self.assertGreaterEqual(result["geometry_score"], 0.99)
        self.assertTrue(os.path.isfile(result["side_by_side"]))
        self.assertTrue(os.path.isfile(result["diff"]))

    def test_geometrically_divergent_capture_is_rejected(self):
        reference = self.image("reference.png", color=(255, 255, 255, 255), size=(96, 64))
        capture = self.image("capture.png", color=(0, 0, 0, 255), size=(96, 64))
        result = gallery.compare_images(reference, capture, self.tmp, "Plate.diorama")
        self.assertFalse(result["passed"])
        self.assertIn("geometr", result["reason"].lower())
        self.assertTrue(os.path.isfile(result["side_by_side"]))
        self.assertTrue(os.path.isfile(result["diff"]))

    def test_prior_stretched_keycap_capture_is_rejected(self):
        reference = os.path.join(FIXTURES, "keycap-reference-top.png")
        stretched = os.path.join(FIXTURES, "keycap-stretched.png")
        result = gallery.compare_images(reference, stretched, self.tmp, "Keycap.diorama")
        self.assertFalse(result["passed"], result)
        self.assertIn("geometr", result["reason"].lower())

    def test_comparison_reports_foreground_and_bounding_box_metrics(self):
        reference = os.path.join(FIXTURES, "keycap-reference-top.png")
        stretched = os.path.join(FIXTURES, "keycap-stretched.png")
        result = gallery.compare_images(reference, stretched, self.tmp, "Keycap.diorama")
        self.assertIn("foreground_iou", result)
        self.assertIn("bbox_edge_delta", result)
        self.assertIn("foreground_pixels", result)
        self.assertIn("reference_crop", result)
        self.assertTrue(os.path.isfile(result["reference_crop"]))


class TestGalleryOrchestration(TempImageCase):
    def test_gallery_run_compares_every_capture_to_its_reference(self):
        crop = self.image("keycap-diorama-crop.png", size=(960, 720))
        capture_result = {
            "component": "Keycap",
            "theme": "diorama",
            "state": "sheet",
            "passed": True,
            "reasons": [],
            "full": self.image("keycap-diorama-full.png", size=(1920, 1080)),
            "crop": crop,
            "metadata": os.path.join(self.tmp, "keycap-diorama.json"),
            "log": os.path.join(self.tmp, "keycap-diorama.log"),
        }
        comparison = {
            "passed": True,
            "reason": "foreground geometry is within tolerance",
            "foreground_iou": 1.0,
            "bbox_edge_delta": 0.0,
            "foreground_pixels": {"reference": 100, "capture": 100},
            "reference_crop": os.path.join(self.tmp, "reference.png"),
            "side_by_side": os.path.join(self.tmp, "side.png"),
            "diff": os.path.join(self.tmp, "diff.png"),
        }
        with (
            mock.patch.object(
                gallery.playtest_boot,
                "gate",
                return_value=({"build": self.build()}, None),
            ),
            mock.patch.object(gallery, "_capture_one", return_value=capture_result),
            mock.patch.object(
                gallery,
                "compare_images",
                return_value=comparison,
            ) as compare,
        ):
            result = gallery.main(
                [
                    "--component",
                    "Keycap",
                    "--theme",
                    "diorama",
                    "--run-dir",
                    self.tmp,
                ]
            )
        self.assertEqual(result, 0)
        compare.assert_called_once()
        summary = gallery._load_json(os.path.join(self.tmp, "summary.json"))
        self.assertEqual(summary["results"][0]["comparison"], comparison)


class TestSweepComparison(unittest.TestCase):
    def test_reports_script_and_individual_check_pass_to_fail(self):
        baseline = {
            "rows": [{"script": "Scripts/playtests/us001_movement.py", "result": "PASS"}],
            "tier4": [{
                "script": "Scripts/playtests/us001_movement.py",
                "checks": {"move": "PASS", "camera": "PASS"},
            }],
        }
        final = {
            "rows": [{"script": "Scripts/playtests/us001_movement.py", "result": "FAIL"}],
            "tier4": [{
                "script": "Scripts/playtests/us001_movement.py",
                "checks": {"move": "FAIL", "camera": "PASS"},
            }],
        }
        result = gallery.compare_sweeps(baseline, final)
        self.assertEqual(
            result["script_pass_to_fail"],
            ["Scripts/playtests/us001_movement.py"],
        )
        self.assertEqual(
            result["check_pass_to_fail"],
            ["Scripts/playtests/us001_movement.py:move"],
        )


if __name__ == "__main__":
    unittest.main()
