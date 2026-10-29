"""Tests for Scripts/package_playtest.py's cook list (offline: nothing is packaged, UAT never runs).

The package must cook every map a playtest launches: the front end, DevTestMap and the playtest
test maps (issue #46: MovementTestMap, CombatTestMap).  DEFAULT_MAPS is that list.  It must match
the +MapsToCook lines in Config/DefaultGame.ini (the editor's Package Project and UAT's fallback),
reach UAT's -map= argument with and without --all-maps, and contain the map of every
Scripts/playtests/*.py script.  /MordecaiCore is also always-cooked, so an iterative local package
can hide a map missing from the list: these tests enforce it instead.
Run: py Scripts/run_python_tests.py --pattern test_package_playtest.py
"""

import contextlib
import io
import os
import re
import sys
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)
sys.dont_write_bytecode = True  # importing package_playtest must not leave Scripts/__pycache__

import package_playtest as pp  # noqa: E402
import playtest_boot as pb  # noqa: E402
import playtest_story as ps  # noqa: E402

DEFAULT_GAME_INI = os.path.join(pp.PROJECT_ROOT, "Config", "DefaultGame.ini")
FRONT_END = "/Game/System/FrontEnd/Maps/L_LyraFrontEnd"
RE_MAPS_TO_COOK = re.compile(r'^([+\-.!]?)MapsToCook=\(FilePath="([^"]+)"\)\s*$')


def maps_to_cook():
    """[(op, path)] of every MapsToCook line in Config/DefaultGame.ini, in file order."""
    with open(DEFAULT_GAME_INI, encoding="utf-8-sig") as f:
        return [(m.group(1), m.group(2)) for m in
                (RE_MAPS_TO_COOK.match(line.strip()) for line in f) if m]


def map_arg(cmd):
    """The maps of UAT's -map= argument."""
    args = [a for a in cmd if a.startswith("-map=")]
    assert len(args) == 1, cmd
    return args[0][len("-map="):].split("+")


class TestDefaultMaps(unittest.TestCase):
    def test_the_default_cook_list(self):
        self.assertEqual(pp.DEFAULT_MAPS, [FRONT_END, pb.DEFAULT_MAP, pb.MOVEMENT_TEST_MAP,
                                           pb.COMBAT_TEST_MAP])
        for m in pb.TEST_MAPS:
            self.assertIn(m, pp.DEFAULT_MAPS)
        self.assertEqual(len(pp.DEFAULT_MAPS), len(set(pp.DEFAULT_MAPS)))
        # Map primary assets are scanned (AlwaysCook) under /Game/Maps and /MordecaiCore/Game/Maps
        for m in pp.DEFAULT_MAPS[1:]:
            self.assertTrue(m.startswith("/MordecaiCore/Game/Maps/"), m)
            self.assertNotIn(".", m.rsplit("/", 1)[-1], f"{m}: a package path, no .Object suffix")

    def test_default_game_ini_cooks_the_same_maps(self):
        lines = maps_to_cook()
        self.assertTrue(lines, f"no MapsToCook lines in {DEFAULT_GAME_INI}")
        self.assertEqual({op for op, _ in lines}, {"+"}, lines)  # only additions
        paths = [p for _, p in lines]
        self.assertEqual(len(paths), len(set(paths)), paths)
        self.assertEqual(set(paths), set(pp.DEFAULT_MAPS))

    def test_the_uat_command_names_every_default_map(self):
        for clean in (False, True):
            with self.subTest(clean=clean):
                cmd = pp.build_uat_command("Development", list(pp.DEFAULT_MAPS), clean)
                self.assertEqual(map_arg(cmd), pp.DEFAULT_MAPS)
                self.assertNotIn("-CookAll", cmd)
                self.assertEqual("-iterate" in cmd, not clean)
                self.assertIn("-clientconfig=Development", cmd)

    def test_all_maps_still_names_every_default_map(self):
        cmd = pp.build_uat_command("Development", [pp.ALL_MAPS_MARKER], False)
        self.assertEqual(map_arg(cmd), pp.DEFAULT_MAPS)
        for a in pp.ALL_MAPS_UAT_ARGS:
            self.assertIn(a, cmd)
        for m in (pb.MOVEMENT_TEST_MAP, pb.COMBAT_TEST_MAP):
            self.assertIn(m, map_arg(cmd))

    def test_every_playtest_script_map_is_cooked(self):
        scripts, errors = ps.discover()
        self.assertEqual(errors, [], "a playtest script does not import, so its map is unknown")
        self.assertTrue(scripts)
        for s in scripts:
            with self.subTest(script=s.name):
                if s.frontend:
                    self.assertIsNone(s.map)  # the front end: GameDefaultMap, always cooked
                else:
                    self.assertIn(s.map, pp.DEFAULT_MAPS,
                                  f"{s.name} launches {s.map}, which the package does not cook "
                                  "(add it to package_playtest.DEFAULT_MAPS and +MapsToCook)")

    def test_the_text_names_the_test_maps(self):
        for name in ("MovementTestMap", "CombatTestMap"):
            self.assertIn(name, pp.__doc__)
        out = io.StringIO()
        with mock.patch.object(sys, "argv", ["package_playtest.py", "--help"]), \
                contextlib.redirect_stdout(out), self.assertRaises(SystemExit) as cm:
            pp.main()
        self.assertEqual(cm.exception.code, 0)
        help_text = " ".join(out.getvalue().split())
        self.assertIn("front end, DevTestMap, MovementTestMap, CombatTestMap", help_text)

    def test_dry_run_prints_the_maps_and_the_command(self):
        for extra, all_maps in (([], False), (["--all-maps"], True)):
            with self.subTest(all_maps=all_maps):
                out = io.StringIO()
                with mock.patch.object(sys, "argv", ["package_playtest.py", "--dry-run", *extra]), \
                        mock.patch.object(pp, "capture_git_state", return_value=None), \
                        contextlib.redirect_stdout(out), self.assertRaises(SystemExit) as cm:
                    pp.main()
                self.assertEqual(cm.exception.code, 0)
                text = out.getvalue()
                self.assertIn("-map=" + "+".join(pp.DEFAULT_MAPS), text)
                self.assertIn("Maps: " + ("ALL (-CookAll -CookMapsOnly) + " if all_maps else "")
                              + "+".join(pp.DEFAULT_MAPS), text)
                self.assertEqual("-CookAll" in text, all_maps)


if __name__ == "__main__":
    unittest.main()
