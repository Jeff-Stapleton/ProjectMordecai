"""Source reproducibility and licensing checks for US-103 foundation assets."""

import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]
CREATE_SCRIPT = ROOT / "Scripts" / "us103_create_foundation_assets.py"
FONT_ROOT = ROOT / "Design" / "TinTally" / "ds" / "project" / "fonts"
STATIC_ROOT = FONT_ROOT / "static"
LICENSE_ROOT = ROOT / "Content" / "Legal" / "TinTallyFonts"

STATIC_FONTS = (
    "Rowdies-Regular.ttf",
    "Rowdies-Bold.ttf",
    "AtkinsonHyperlegibleNext-Medium.ttf",
    "AtkinsonHyperlegibleNext-SemiBold.ttf",
    "AtkinsonHyperlegibleNext-Bold.ttf",
    "AtkinsonHyperlegibleNext-ExtraBold.ttf",
    "AtkinsonHyperlegibleMono-SemiBold.ttf",
    "AtkinsonHyperlegibleMono-Bold.ttf",
    "AtkinsonHyperlegibleMono-ExtraBold.ttf",
)

FAMILIES = {
    "Rowdies": (
        "Copyright 2018 The Rowdies Project Authors",
        "https://github.com/magictype/rowdies",
    ),
    "AtkinsonHyperlegibleNext": (
        "Copyright 2020-2024 The Atkinson Hyperlegible Next Project Authors",
        "https://github.com/googlefonts/atkinson-hyperlegible-next",
    ),
    "AtkinsonHyperlegibleMono": (
        "Copyright 2020-2024 The Atkinson Hyperlegible Mono Project Authors",
        "https://github.com/googlefonts/atkinson-hyperlegible-next-mono",
    ),
}


class TestFoundationFontSources(unittest.TestCase):
    def test_asset_creation_imports_only_committed_static_ttf_sources(self):
        script = CREATE_SCRIPT.read_text(encoding="utf-8")
        self.assertNotIn('os.environ["TEMP"]', script)
        self.assertIn('FONT_SOURCE = ROOT / "Design"', script)
        for filename in STATIC_FONTS:
            source = STATIC_ROOT / filename
            self.assertTrue(source.is_file(), source)
            self.assertGreater(source.stat().st_size, 10_000, source)

    def test_each_font_family_commits_full_ofl_and_copyright_notice(self):
        for family, (copyright_line, url) in FAMILIES.items():
            with self.subTest(family=family):
                ofl = LICENSE_ROOT / family / "OFL.txt"
                notice = LICENSE_ROOT / family / "NOTICE.txt"
                self.assertTrue(ofl.is_file(), ofl)
                self.assertTrue(notice.is_file(), notice)
                license_text = ofl.read_text(encoding="utf-8")
                notice_text = notice.read_text(encoding="utf-8")
                self.assertIn("SIL OPEN FONT LICENSE Version 1.1 - 26 February 2007", license_text)
                self.assertIn("PERMISSION & CONDITIONS", license_text)
                self.assertIn("TERMINATION", license_text)
                self.assertIn(copyright_line, notice_text)
                self.assertIn(url, notice_text)
                self.assertIn("SIL Open Font License 1.1", notice_text)


if __name__ == "__main__":
    unittest.main()
