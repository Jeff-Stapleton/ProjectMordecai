import os
import unittest


ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPELL_COOLDOWN_CPP = os.path.join(
    ROOT,
    "Source",
    "LyraGame",
    "Mordecai",
    "UI",
    "MordecaiSpellCooldownWidget.cpp",
)


class US105HUDSourceContractTests(unittest.TestCase):
    def test_gallery_preview_reuses_cached_ftext(self):
        with open(SPELL_COOLDOWN_CPP, "r", encoding="utf-8") as handle:
            source = handle.read()
        self.assertIn(
            "PreviewCooldownText",
            source,
            "the deterministic gallery countdown must cache its FText",
        )
        self.assertIn(
            "CooldownText->GetText().IdenticalTo(PreviewCooldownText)",
            source,
            "ticks must skip SetText when the cached preview copy is unchanged",
        )


if __name__ == "__main__":
    unittest.main()
