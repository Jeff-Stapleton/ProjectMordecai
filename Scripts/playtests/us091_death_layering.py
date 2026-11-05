#!/usr/bin/env python3
"""US-091 death-layering playtest -- the proven US-054 combat path, scoped to AC-091.C.

The base run supplies the real-input route to death and respawn. This owner run keeps every
prerequisite check but attributes only the two weapon-HUD layering observations to US-091, so an
unrelated story's acceptance criteria cannot determine US-091's result.
"""

from dataclasses import replace
import os
import sys

PLAYTEST_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(PLAYTEST_DIR)
for path in (SCRIPTS_DIR, PLAYTEST_DIR):
    if path not in sys.path:
        sys.path.insert(0, path)

import playtest_kit as kit  # noqa: E402
import us054_combat as base  # noqa: E402


def _us091_step(step):
    proves = ("AC-091.C",) if step.check in {
        "weapon_hud_hidden_under_death",
        "weapon_hud_restored_after_respawn",
    } else ()
    return replace(step, proves=proves, known_issue=None)


class Us091DeathLayeringRun(base.Us054CombatRun):
    STORY = "US-091"
    COVERS = ()
    FEATURES = ("weapon_hud",)
    TITLE = "US-091 weapon HUD death-layering playtest (Tier 4, real input)"
    RUN_SUFFIX = "us091death"
    MAP = kit.COMBAT_TEST_MAP
    PLAN = tuple(_us091_step(step) for step in base.PLAN)
    SCRIPT = os.path.abspath(__file__)


Run = Us091DeathLayeringRun


def parse_args(argv=None):
    return kit.parse_args(Us091DeathLayeringRun, argv)


def main(argv=None):
    return kit.main(Us091DeathLayeringRun, argv)


if __name__ == "__main__":
    sys.exit(main())
