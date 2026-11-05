#!/usr/bin/env python3
"""US-091 weapon HUD playtest -- starting text and four real Tab updates, scoped to AC-091.D.

This owner run reuses US-078's tested state matchers and cycle helper, but stops after the
US-091 path. The unrelated known Shift+Tab bug and attack checks remain owned by US-078.
"""

import os
import sys

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
for path in (SCRIPTS_DIR, PLAYTEST_DIR):
    if path not in sys.path:
        sys.path.insert(0, path)

import playtest_kit as kit  # noqa: E402
from playtest_kit import Step  # noqa: E402
import us078_weapons as base  # noqa: E402


PLAN = (
    Step("starting_weapon", "state player -> index 0 of 5 and display_name 'Longsword'",
         proves=("AC-091.D",)),
    Step("weapon_hud_text", "state player, widgets -> visible MordecaiEquippedWeaponWidget text "
         "equals the starting weapon display_name", proves=("AC-091.D",)),
    Step("cycle_next_1", "tap Tab -> Dagger is equipped"),
    Step("cycle_next_2", "tap Tab -> Greatsword is equipped"),
    Step("cycle_next_3", "tap Tab -> Spear is equipped"),
    Step("weapon_hud_updates", "tap Tab a fourth time -> Mace is equipped and visible "
         "MordecaiEquippedWeaponWidget text equals 'Mace'", proves=("AC-091.D",)),
)


class Us091WeaponHudRun(base.Us078Run):
    STORY = "US-091"
    COVERS = ()
    FEATURES = ("weapon_hud",)
    TITLE = "US-091 weapon HUD text/update playtest (Tier 4, real input)"
    RUN_SUFFIX = "us091weapons"
    MAP = kit.COMBAT_TEST_MAP
    PLAN = PLAN
    KNOWN_ISSUES = ()
    PENDING = {}
    SCRIPT = os.path.abspath(__file__)

    def steps(self):
        self.step_start()
        for index in range(1, base.MACE_INDEX):
            self.cycle(f"cycle_next_{index}", base.NEXT_KEY, +1, require_from=index - 1)
        mace_ok, _ = self.cycle(
            "weapon_hud_updates",
            base.NEXT_KEY,
            +1,
            require_from=base.MACE_INDEX - 1,
            hud=True,
            needs=self.hud_ok,
            needs_why="not run: weapon_hud_text failed (no starting weapon name on screen)",
        )
        self.shot("cycled_mace", needs=mace_ok,
                  needs_why="not taken: the fourth Tab did not update the plate to Mace")


Run = Us091WeaponHudRun


def parse_args(argv=None):
    return kit.parse_args(Us091WeaponHudRun, argv)


def main(argv=None):
    return kit.main(Us091WeaponHudRun, argv)


if __name__ == "__main__":
    sys.exit(main())
