"""Offline tests for the US-103 packaged Tin Tally gallery harness."""

import os
import json
import sys
import tempfile
import unittest
from unittest import mock

from PIL import Image, ImageStat

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

    def image(self, name, color=(22, 54, 52, 255), size=(1920, 1080)):
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
            "build_stamp": "test-build",
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
        capture = self.image("capture.png", size=(1280, 720))
        metadata = self.metadata()
        metadata["resolution"] = [1280, 720]
        reasons = gallery.validate_capture(capture, metadata, self.build(), "diorama")
        self.assertTrue(any("1920" in reason for reason in reasons), reasons)

    def test_rejects_physical_image_size_that_disagrees_with_required_capture(self):
        capture = self.image("capture.png", size=(1280, 720))
        reasons = gallery.validate_capture(
            capture, self.metadata(), self.build(), "diorama"
        )
        self.assertTrue(any("image size" in reason.lower() for reason in reasons), reasons)

    def test_rejects_sheet_pixel_that_does_not_match_resolved_theme_plate(self):
        capture = self.image("wrong-theme-pixels.png", color=(255, 0, 255, 255))
        reasons = gallery.validate_capture(
            capture, self.metadata(), self.build(), "diorama"
        )
        self.assertTrue(any("plate" in reason.lower() for reason in reasons), reasons)

    def test_hud_layout_is_not_exempt_from_resolved_theme_plate_validation(self):
        capture = self.image("wrong-hud-theme-pixels.png", color=(255, 0, 255, 255))
        metadata = self.metadata()
        metadata["component"] = "HUDLayout"
        metadata["crop_rect"] = [0, 0, 1920, 1080]
        reasons = gallery.validate_capture(
            capture,
            metadata,
            self.build(),
            "diorama",
            expected_component="HUDLayout",
        )
        self.assertTrue(any("plate" in reason.lower() for reason in reasons), reasons)

    def test_hud_layout_validates_the_real_gauge_plate_theme_probe(self):
        for theme, plate in (
            ("diorama", (22, 54, 52, 255)),
            ("high-contrast", (0, 0, 0, 255)),
        ):
            with self.subTest(theme=theme):
                capture = self.image(
                    f"hud-{theme}.png",
                    color=(230, 227, 218, 255),
                )
                with Image.open(capture) as source:
                    pixels = source.copy()
                for y in range(102, 113):
                    for x in range(472, 483):
                        pixels.putpixel((x, y), plate)
                pixels.save(capture)
                metadata = self.metadata(theme=theme)
                metadata["component"] = "HUDLayout"
                metadata["crop_rect"] = [0, 0, 1920, 1080]
                reasons = gallery.validate_capture(
                    capture,
                    metadata,
                    self.build(),
                    theme,
                    expected_component="HUDLayout",
                )
                self.assertEqual(reasons, [])

    def test_rejects_missing_game_build_stamp(self):
        capture = self.image("capture.png")
        metadata = self.metadata()
        metadata.pop("build_stamp")
        reasons = gallery.validate_capture(capture, metadata, self.build(), "diorama")
        self.assertTrue(any("build stamp" in reason.lower() for reason in reasons), reasons)


class TestImageComparison(TempImageCase):
    def status_chip_fixture(self, name, with_intrusion):
        path = os.path.join(self.tmp, name)
        image = Image.new("RGB", (960, 720), (180, 90, 160))
        plate = gallery._theme_token_rgb("plate", "diorama")
        frost = gallery._theme_token_rgb("dmg-frost", "diorama")
        lightning = gallery._theme_token_rgb("dmg-lightning", "diorama")
        for origin, width in (((498, 178), 250), ((498, 260), 166)):
            for y in range(origin[1], origin[1] + 36):
                for x in range(origin[0], origin[0] + width):
                    image.putpixel((x, y), plate)
        if with_intrusion:
            image.putpixel((498 + 250 - 8 - 20, 178 + 18), frost)
            image.putpixel((498 + 166 - 8 - 20, 260 + 18), lightning)
        image.save(path)
        return path

    def test_status_chip_analysis_rejects_pip_pixels_in_duration_boxes(self):
        result = gallery.analyze_status_chip_pip_intrusion(
            self.status_chip_fixture("intruding.png", True), "diorama"
        )
        self.assertFalse(result["passed"], result)
        self.assertTrue(all(not fixture["passed"] for fixture in result["fixtures"]))

    def test_status_chip_analysis_accepts_clean_duration_boxes(self):
        result = gallery.analyze_status_chip_pip_intrusion(
            self.status_chip_fixture("clean.png", False), "diorama"
        )
        self.assertTrue(result["passed"], result)

    @staticmethod
    def hatch_region_metrics(path):
        with Image.open(path) as source:
            region = source.convert("L").crop((28, 66, 74, 290))
        mean_luminance = ImageStat.Stat(region).mean[0]
        pixels = region.load()
        scores = {}
        for lag in range(12, 21):
            difference = sum(
                abs(pixels[x, y] - pixels[x - lag, y])
                for y in range(region.height)
                for x in range(lag, region.width)
            )
            scores[lag] = difference / ((region.width - lag) * region.height)
        return mean_luminance, min(scores, key=scores.get)

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

    def test_round_five_plate_hatch_captures_are_rejected(self):
        for theme in ("diorama", "high-contrast"):
            with self.subTest(theme=theme):
                reference = os.path.join(FIXTURES, f"plate-reference-{theme}.png")
                round_five = os.path.join(FIXTURES, f"plate-round5-{theme}.png")
                reference_luminance, _ = self.hatch_region_metrics(reference)
                capture_luminance, capture_period = self.hatch_region_metrics(round_five)
                self.assertGreater(
                    abs(reference_luminance - capture_luminance),
                    40.0,
                    "fixture must retain the round-five dark-floor regression",
                )
                self.assertNotIn(
                    capture_period,
                    range(15, 18),
                    "fixture must retain the round-five oversized stripe period",
                )
                result = gallery.compare_images(
                    reference, round_five, self.tmp, f"Plate.{theme}"
                )
                self.assertFalse(result["passed"], result)
                self.assertIn("hatch", result["reason"].lower())

    def test_round_two_tally_capture_is_rejected_for_text_and_edge_divergence(self):
        reference = os.path.join(FIXTURES, "tally-reference-top.png")
        round_two = os.path.join(FIXTURES, "tally-round2.png")
        result = gallery.compare_images(reference, round_two, self.tmp, "Tally.diorama")
        self.assertFalse(result["passed"], result)
        self.assertIn("text", result["reason"].lower())

    def test_round_three_tally_capture_is_rejected_by_final_detail_regions(self):
        reference = os.path.join(FIXTURES, "tally-reference-top.png")
        round_three = os.path.join(FIXTURES, "tally-round3.png")
        result = gallery.compare_images(reference, round_three, self.tmp, "Tally.diorama")
        self.assertIn("localized_regions", result)
        self.assertEqual(
            set(result["localized_regions"]),
            {"captions", "rarity", "seal_throbber", "anatomy"},
        )
        self.assertFalse(result["passed"], result)
        self.assertIn("localized", result["reason"].lower())

    def test_round_three_keycap_capture_is_rejected_by_final_detail_regions(self):
        reference = os.path.join(FIXTURES, "keycap-reference-top.png")
        round_three = os.path.join(FIXTURES, "keycap-round3.png")
        result = gallery.compare_images(reference, round_three, self.tmp, "Keycap.diorama")
        self.assertIn("localized_regions", result)
        self.assertEqual(
            set(result["localized_regions"]),
            {"wide_keys", "mouse", "gamepad_spacing", "state_headers"},
        )
        self.assertFalse(result["passed"], result)
        self.assertIn("localized", result["reason"].lower())

    def test_round_three_glyph_capture_is_rejected_by_final_detail_regions(self):
        reference = os.path.join(FIXTURES, "glyphs-reference-top.png")
        round_three = os.path.join(FIXTURES, "glyphs-round3.png")
        result = gallery.compare_images(reference, round_three, self.tmp, "Glyphs.diorama")
        self.assertIn("localized_regions", result)
        self.assertEqual(
            set(result["localized_regions"]),
            {"tile_chips", "labels"},
        )
        self.assertFalse(result["passed"], result)
        self.assertIn("localized", result["reason"].lower())

    def test_comparison_reports_text_region_and_downsampled_edge_metrics(self):
        reference = os.path.join(FIXTURES, "tally-reference-top.png")
        round_two = os.path.join(FIXTURES, "tally-round2.png")
        result = gallery.compare_images(reference, round_two, self.tmp, "Tally.diorama")
        self.assertIn("edge_grid_similarity", result)
        self.assertIn("text_row_similarity", result)
        self.assertIn("edge_grid", result["thresholds"])
        self.assertIn("text_row", result["thresholds"])

    def test_comparison_reports_foreground_and_bounding_box_metrics(self):
        reference = os.path.join(FIXTURES, "keycap-reference-top.png")
        stretched = os.path.join(FIXTURES, "keycap-stretched.png")
        result = gallery.compare_images(reference, stretched, self.tmp, "Keycap.diorama")
        self.assertIn("foreground_iou", result)
        self.assertIn("bbox_edge_delta", result)
        self.assertIn("foreground_pixels", result)
        self.assertIn("reference_crop", result)
        self.assertTrue(os.path.isfile(result["reference_crop"]))

    def test_hud_sheets_use_the_global_text_and_edge_floors(self):
        for component in ("bar", "spellslot", "loadoutplate"):
            for theme in gallery.THEMES:
                with self.subTest(component=component, theme=theme):
                    thresholds = gallery._comparison_thresholds(component, theme)
                    self.assertEqual(
                        thresholds["edge_grid"],
                        gallery.MIN_EDGE_GRID_SIMILARITY,
                    )
                    self.assertEqual(
                        thresholds["text_row"],
                        gallery.MIN_TEXT_ROW_SIMILARITY,
                    )


class TestGalleryOrchestration(TempImageCase):
    class FakeProcess:
        def __init__(self, polls=None):
            self.polls = list(polls or [None])
            self.returncode = None

        def poll(self):
            if len(self.polls) > 1:
                return self.polls.pop(0)
            return self.polls[0]

        def terminate(self):
            self.returncode = 0
            self.polls = [0]

        def wait(self, timeout=None):
            return self.returncode or 0

        def kill(self):
            self.terminate()

    def fake_launch(self, raw_metadata, image_size=(1920, 1080), color=(22, 54, 52, 255)):
        def launch(_exe, game_args):
            output_arg = next(
                value for value in game_args
                if value.startswith('-TinTallyGalleryOutput="')
            )
            output = output_arg.split('"', 1)[1].rsplit('"', 1)[0].replace("/", os.sep)
            Image.new("RGBA", image_size, color).save(output)
            with open(os.path.splitext(output)[0] + ".json", "w", encoding="utf-8") as target:
                json.dump(raw_metadata, target)
            return self.FakeProcess(), None, None
        return launch

    def test_capture_rejects_raw_wrong_theme_before_any_client_metadata_merge(self):
        raw = self.metadata(theme="high-contrast")
        with mock.patch.object(
            gallery.playtest_boot,
            "launch_game",
            side_effect=self.fake_launch(raw),
        ):
            result = gallery._capture_one(
                "game.exe", self.tmp, self.build(), "Plate", "diorama", "sheet"
            )
        self.assertFalse(result["passed"], result)
        self.assertTrue(any("theme" in reason.lower() for reason in result["reasons"]))

    def test_capture_rejects_raw_wrong_resolution_and_physical_size(self):
        raw = self.metadata()
        raw["resolution"] = [1280, 720]
        with mock.patch.object(
            gallery.playtest_boot,
            "launch_game",
            side_effect=self.fake_launch(raw, image_size=(1280, 720)),
        ):
            result = gallery._capture_one(
                "game.exe", self.tmp, self.build(), "Plate", "diorama", "sheet"
            )
        self.assertFalse(result["passed"], result)
        self.assertTrue(any("1920" in reason for reason in result["reasons"]))

    def test_capture_deletes_stale_targets_before_launch(self):
        stale_png = os.path.join(self.tmp, "plate-diorama-full.png")
        stale_json = os.path.splitext(stale_png)[0] + ".json"
        self.image("plate-diorama-full.png")
        with open(stale_json, "w", encoding="utf-8") as target:
            json.dump(self.metadata(), target)
        process = self.FakeProcess([None, 0])
        with mock.patch.object(
            gallery.playtest_boot,
            "launch_game",
            return_value=(process, None, None),
        ):
            result = gallery._capture_one(
                "game.exe", self.tmp, self.build(), "Plate", "diorama", "sheet"
            )
        self.assertFalse(result["passed"], result)
        self.assertFalse(os.path.exists(stale_png))
        self.assertFalse(os.path.exists(stale_json))

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

    def test_comparison_failures_are_stored_separately_from_capture_reasons(self):
        crop = self.image("keycap-diorama-crop.png", size=(960, 720))
        capture_result = {
            "component": "Keycap",
            "theme": "diorama",
            "state": "sheet",
            "passed": True,
            "reasons": [],
            "full": self.image("keycap-diorama-full.png"),
            "crop": crop,
            "metadata": os.path.join(self.tmp, "keycap-diorama.json"),
            "log": os.path.join(self.tmp, "keycap-diorama.log"),
        }
        comparison = {
            "passed": False,
            "reason": "localized region diverged",
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
            mock.patch.object(gallery, "compare_images", return_value=comparison),
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
        self.assertEqual(result, 1)
        summary = gallery._load_json(os.path.join(self.tmp, "summary.json"))
        row = summary["results"][0]
        self.assertEqual(row["reasons"], [])
        self.assertEqual(row["compare_failures"], ["localized region diverged"])


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
