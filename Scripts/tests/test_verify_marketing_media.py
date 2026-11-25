"""Offline tests for packaged marketing-media verification."""

import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

from PIL import Image, ImageDraw

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import verify_marketing_media as media  # noqa: E402


class MarketingMediaVerificationCase(unittest.TestCase):
    @staticmethod
    def game_like_frame():
        image = Image.new("RGB", (128, 72), (15, 52, 49))
        draw = ImageDraw.Draw(image)
        draw.rectangle((0, 52, 127, 71), fill=(23, 28, 30))
        draw.rectangle((8, 58, 48, 64), fill=(160, 38, 28))
        draw.rectangle((78, 58, 120, 64), fill=(40, 120, 185))
        draw.ellipse((52, 20, 76, 50), fill=(175, 128, 82))
        return image

    def test_identical_game_frames_resemble_each_other(self):
        frame = self.game_like_frame()
        self.assertGreater(media.histogram_similarity(frame, frame.copy()), 0.99)
        self.assertTrue(media.hud_region_non_uniform(frame))

    def test_wallpaper_like_frame_does_not_resemble_game_reference(self):
        game = self.game_like_frame()
        wallpaper = Image.new("RGB", game.size, (80, 155, 210))
        draw = ImageDraw.Draw(wallpaper)
        draw.rectangle((0, 40, 127, 71), fill=(225, 196, 126))
        self.assertLess(media.histogram_similarity(game, wallpaper), 0.55)
        self.assertFalse(media.hud_region_non_uniform(wallpaper))

    def test_identical_frames_are_static(self):
        frame = self.game_like_frame()
        self.assertEqual(media.frame_motion_score(frame, frame.copy()), 0.0)

    def test_changed_game_frames_have_motion(self):
        before = self.game_like_frame()
        after = before.copy()
        draw = ImageDraw.Draw(after)
        draw.ellipse((68, 20, 92, 50), fill=(220, 152, 70))
        self.assertGreater(media.frame_motion_score(before, after), 0.002)

    def test_temporal_continuity_rejects_a_duplicated_frame_slideshow(self):
        source = self.game_like_frame()
        slideshow = []
        continuous = []
        for index in range(60):
            slide = source.copy()
            ImageDraw.Draw(slide).ellipse(
                (20 + (index // 4) % 50, 20, 30 + (index // 4) % 50, 30),
                fill=(220, 152, 70))
            slideshow.append(slide)
            live = source.copy()
            ImageDraw.Draw(live).ellipse(
                (20 + index % 50, 20, 30 + index % 50, 30),
                fill=(220, 152, 70))
            continuous.append(live)
        self.assertFalse(media.temporal_continuity(slideshow)["passed"])
        self.assertTrue(media.temporal_continuity(continuous)["passed"])


@unittest.skipUnless(shutil.which("ffmpeg") and shutil.which("ffprobe"),
                     "ffmpeg and ffprobe are required")
class MarketingMediaNegativeClipCase(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.addCleanup(self.temp.cleanup)
        self.root = self.temp.name
        self.source = os.path.join(self.root, "source.png")
        frame = MarketingMediaVerificationCase.game_like_frame().resize((1920, 1080))
        frame.save(self.source)
        with open(self.source, "rb") as stream:
            digest = hashlib.sha256(stream.read()).hexdigest()
        self.manifest = os.path.join(self.root, "source-frames.json")
        with open(self.manifest, "w", encoding="utf-8") as stream:
            json.dump({
                "version": 1,
                "fps": 30,
                "frames": [{"index": 1, "file": "source.png", "sha256": digest}],
            }, stream)

    def encode_still(self, image, name):
        path = os.path.join(self.root, f"{name}.png")
        image.resize((1920, 1080)).save(path)
        video = os.path.join(self.root, f"{name}.mp4")
        subprocess.check_call([
            "ffmpeg", "-y", "-loglevel", "error", "-loop", "1", "-i", path,
            "-t", "6", "-r", "30", "-pix_fmt", "yuv420p", video,
        ])
        return video

    def verify_rejected(self, video):
        output = os.path.join(self.root, os.path.basename(video) + ".json")
        code = media.main([
            "--video", video,
            "--source-manifest", self.manifest,
            "--output", output,
        ])
        with open(output, encoding="utf-8") as stream:
            result = json.load(stream)
        self.assertEqual(code, 1)
        self.assertFalse(result["passed"])
        self.assertFalse(result["videos"][0]["passed"])

    def test_static_game_frame_is_rejected(self):
        self.verify_rejected(self.encode_still(
            MarketingMediaVerificationCase.game_like_frame(), "static"))

    def test_black_clip_is_rejected(self):
        self.verify_rejected(self.encode_still(
            Image.new("RGB", (128, 72), "black"), "black"))

    def test_lock_screen_like_photo_and_clock_is_rejected(self):
        image = Image.new("RGB", (128, 72), (55, 95, 135))
        draw = ImageDraw.Draw(image)
        for y in range(72):
            color = (55 + y, min(180, 95 + y), min(210, 135 + y))
            draw.line((0, y, 127, y), fill=color)
        draw.text((48, 18), "10:42", fill="white")
        draw.text((42, 31), "Sunday", fill="white")
        self.verify_rejected(self.encode_still(image, "lock-screen"))


if __name__ == "__main__":
    unittest.main()
