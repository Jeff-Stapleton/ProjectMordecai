"""Offline tests for packaged marketing-media verification."""

import os
import sys
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


if __name__ == "__main__":
    unittest.main()
