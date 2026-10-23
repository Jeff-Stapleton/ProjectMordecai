"""Unit tests for the helpers extracted from Scripts/playtest_boot.py (no real game needed).

Run: py Scripts/run_python_tests.py
"""

import contextlib
import io
import json
import os
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if SCRIPTS not in sys.path:
    sys.path.insert(0, SCRIPTS)

import playtest_boot as pb  # noqa: E402

FAKE_GAME = os.path.join(SCRIPTS, "tests", "fake_game.py")
IDENTIFIED = ("[2026.10.19-04.37.20:486][  0]LogLyraExperience: Identified experience "
              "LyraExperienceDefinition:B_MordecaiArenaExperience (Source: OptionsString)")
LOADED = ("[2026.10.19-04.37.20:498][  0]LogLyraExperience: EXPERIENCE: OnExperienceLoadComplete"
          "(CurrentExperience = LyraExperienceDefinition:B_MordecaiArenaExperience, Server)")
WRONG = ("[2026.10.19-04.37.20:486][  0]LogLyraExperience: Identified experience "
         "LyraExperienceDefinition:B_LyraDefaultExperience (Source: Default)")


class FakeProc:
    """Stands in for subprocess.Popen: poll() returns `code` once exit_after seconds passed."""

    def __init__(self, exit_after=None, code=0):
        self.t0 = time.time()
        self.exit_after = exit_after
        self.code = code
        self.returncode = None
        self.pid = 4242

    def poll(self):
        if self.exit_after is not None and time.time() - self.t0 >= self.exit_after:
            self.returncode = self.code
        return self.returncode


def write_lines(path, lines, mode="w"):
    with open(path, mode, encoding="utf-8", newline="\n") as f:
        for line in lines:
            f.write(line + "\n")


@contextlib.contextmanager
def captured_log():
    out = io.StringIO()
    with pb.log_stream(out):
        yield out


class TempDirCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
        self.tmp = self._tmp.name
        self.addCleanup(self._tmp.cleanup)


class TestImportAndHelp(unittest.TestCase):
    def test_import_has_no_side_effects(self):
        before = sorted(os.listdir(pb.PLAYTESTS_DIR)) if os.path.isdir(pb.PLAYTESTS_DIR) else None
        r = subprocess.run([sys.executable, "-c",
                            f"import sys; sys.path.insert(0, {SCRIPTS!r}); import playtest_boot"],
                           capture_output=True, text=True, timeout=60)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(r.stdout, "")
        self.assertEqual(r.stderr, "")
        after = sorted(os.listdir(pb.PLAYTESTS_DIR)) if os.path.isdir(pb.PLAYTESTS_DIR) else None
        self.assertEqual(before, after, "importing playtest_boot created run dirs")

    def test_help_works(self):
        r = subprocess.run([sys.executable, os.path.join(SCRIPTS, "playtest_boot.py"), "--help"],
                           capture_output=True, text=True, timeout=60)
        self.assertEqual(r.returncode, 0, r.stderr)
        for flag in ("--exe", "--map", "--frontend", "--expect-experience", "--timeout",
                     "--settle", "--shots", "--shot-interval", "--allow-stale",
                     "--fail-on-errors", "--keep-running"):
            self.assertIn(flag, r.stdout)


class TestBuildGameArgs(unittest.TestCase):
    @staticmethod
    def original(map_arg, game_log, run_dir, shots, settle, interval):
        """The inline expression main() used before the refactor, verbatim."""
        quote_value = pb.quote_value
        RES_X, RES_Y = pb.RES_X, pb.RES_Y
        game_args = ([map_arg] if map_arg else []) + [
            "-windowed", f"-ResX={RES_X}", f"-ResY={RES_Y}", "-log",
            f"-abslog={quote_value(game_log)}", "-nosplash", "-nosound",
        ]
        if shots > 0:
            game_args += [
                "-PlaytestCapture", f"-PlaytestShotDir={quote_value(run_dir)}",
                f"-PlaytestShots={shots}", f"-PlaytestShotDelay={settle:g}",
                f"-PlaytestShotInterval={interval:g}",
            ]
        return game_args

    def test_matches_the_original_inline_arguments(self):
        cases = [
            (pb.DEFAULT_MAP, r"C:\P\Saved\Playtests\x-boot\game.log", r"C:\P\x-boot", 3, 6.0, 2.0),
            (None, r"C:\a b\game.log", r"C:\a b", 3, 0.0, 1.5),
            ("/Game/Foo", "C:/x/game.log", "C:/x", 0, 6.0, 2.0),
            (pb.DEFAULT_MAP, "C:/x/game.log", "C:/x", 5, 10.0, 2.5),
        ]
        for map_arg, game_log, run_dir, shots, settle, interval in cases:
            with self.subTest(map=map_arg, shots=shots):
                self.assertEqual(
                    pb.build_game_args(map_arg, game_log, shot_dir=run_dir, shots=shots,
                                       settle=settle, shot_interval=interval),
                    self.original(map_arg, game_log, run_dir, shots, settle, interval))

    def test_exact_default_tokens(self):
        self.assertEqual(pb.build_game_args("/M", "C:/r/game.log"),
                         ["/M", "-windowed", "-ResX=1280", "-ResY=720", "-log",
                          "-abslog=C:/r/game.log", "-nosplash", "-nosound"])

    def test_capture_tokens_and_quoting(self):
        args = pb.build_game_args(None, "C:/a b/game.log", shot_dir="C:/a b", shots=3, settle=6,
                                  shot_interval=2.0)
        self.assertEqual(args[0], "-windowed")  # front end: no map token
        self.assertIn('-abslog="C:/a b/game.log"', args)
        self.assertEqual(args[-5:], ["-PlaytestCapture", '-PlaytestShotDir="C:/a b"',
                                     "-PlaytestShots=3", "-PlaytestShotDelay=6",
                                     "-PlaytestShotInterval=2"])

    def test_shot_dir_defaults_to_the_log_folder(self):
        args = pb.build_game_args("/M", os.path.join("C:\\r", "game.log"), shots=1)
        self.assertIn(f"-PlaytestShotDir={os.path.abspath('C:/r')}", args)

    def test_extra_tokens_come_last(self):
        args = pb.build_game_args("/M", "C:/r/game.log", extra=["-AgentBridge", "-AgentBridgePort=5"])
        self.assertEqual(args[-2:], ["-AgentBridge", "-AgentBridgePort=5"])

    def test_build_command_line(self):
        self.assertEqual(pb.build_command_line(r"C:\a b\LyraGame.exe", ["/M", "-log"]),
                         '"C:\\a b\\LyraGame.exe" /M -log')
        self.assertEqual(pb.build_command_line(r"C:\x\Game.exe", ["-abslog=\"C:/a b/g.log\""]),
                         'C:\\x\\Game.exe -abslog="C:/a b/g.log"')
        self.assertEqual(pb.build_command_line("ignored.exe", ["-x"],
                                               exe_argv=[r"C:\py\python.exe", r"C:\t\fake.py"]),
                         r"C:\py\python.exe C:\t\fake.py -x")


class TestNewRunDir(TempDirCase):
    def test_default_suffix_is_boot(self):
        d = pb.new_run_dir(base=self.tmp)
        self.assertTrue(os.path.isdir(d))
        self.assertRegex(os.path.basename(d), r"^\d{8}-\d{6}-boot$")

    def test_custom_suffix_and_collisions(self):
        with mock.patch.object(pb.datetime, "datetime") as dt:
            dt.now.return_value.strftime.return_value = "20261020-120000"
            first = pb.new_run_dir("us079", base=self.tmp)
            second = pb.new_run_dir("us079", base=self.tmp)
            third = pb.new_run_dir("us079", base=self.tmp)
        self.assertEqual([os.path.basename(p) for p in (first, second, third)],
                         ["20261020-120000-us079", "20261020-120000-us079-2",
                          "20261020-120000-us079-3"])

    def test_base_defaults_to_playtests_dir(self):
        with mock.patch.object(pb, "PLAYTESTS_DIR", os.path.join(self.tmp, "Playtests")):
            d = pb.new_run_dir("bridge")
        self.assertEqual(os.path.dirname(d), os.path.join(self.tmp, "Playtests"))
        self.assertTrue(d.endswith("-bridge"))


class TestLogStream(unittest.TestCase):
    def test_set_log_stream_returns_previous_and_redirects(self):
        out = io.StringIO()
        previous = pb.set_log_stream(out)
        try:
            pb.log("hello")
        finally:
            self.assertIs(pb.set_log_stream(previous), out)
        self.assertEqual(out.getvalue(), "[Boot-Playtest] hello\n")

    def test_default_is_stdout(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            pb.log("to stdout")
        self.assertIn("to stdout", out.getvalue())

    def test_banner_and_scan_follow_the_stream(self):
        stdout = io.StringIO()
        with captured_log() as out, contextlib.redirect_stdout(stdout):
            pb.print_stale_banner({"reasons": ["build commit a != HEAD b"]}, allowed=False)
            pb.BootState("B_MordecaiArenaExperience").scan([IDENTIFIED, "Fatal error: boom"], 1.0)
            pb.resolve_exe(os.path.abspath(__file__))
        text = out.getvalue()
        self.assertIn("STALE BUILD", text)
        self.assertIn("build commit a != HEAD b", text)
        self.assertIn("Identified experience", text)
        self.assertIn("CRASH MARKER", text)
        self.assertEqual(stdout.getvalue(), "")

    def test_log_stream_context_restores(self):
        out = io.StringIO()
        before = pb._LOG_STREAM
        with pb.log_stream(out):
            self.assertIs(pb._LOG_STREAM, out)
        self.assertIs(pb._LOG_STREAM, before)


class TestWaitForExperience(TempDirCase):
    def wait(self, lines, proc=None, timeout=5.0, grace=pb.EVIDENCE_GRACE_S, late=None):
        path = os.path.join(self.tmp, "game.log")
        if lines is not None:
            write_lines(path, ["\ufeffLog file open"] + lines)
        if late:
            delay, more = late
            threading.Timer(delay, write_lines, (path, more, "a")).start()
        state = pb.BootState("B_MordecaiArenaExperience")
        tail = pb.LogTail(path)
        self.addCleanup(tail.close)
        t0 = time.time()
        with captured_log():
            outcome = pb.wait_for_experience(proc or FakeProc(), tail, state, timeout, t0,
                                             poll_s=0.05, evidence_grace_s=grace)
        return outcome, state, time.time() - t0

    def test_loaded(self):
        outcome, state, _ = self.wait([IDENTIFIED, LOADED])
        self.assertEqual(outcome, "loaded")
        self.assertEqual(state.load_complete["name"], "B_MordecaiArenaExperience")
        self.assertEqual(state.first_identified["source"], "OptionsString")

    def test_loaded_when_the_log_arrives_late(self):
        outcome, _, elapsed = self.wait(None, late=(0.3, [IDENTIFIED, LOADED]))
        self.assertEqual(outcome, "loaded")
        self.assertGreaterEqual(elapsed, 0.25)

    def test_crash(self):
        outcome, state, _ = self.wait([IDENTIFIED, "[x][  1]LogWindows: Error: Fatal error: boom"])
        self.assertEqual(outcome, "crash")
        self.assertTrue(state.crash_lines)

    def test_no_experience(self):
        outcome, state, _ = self.wait(["LogLyraExperience: Error: Failed to identify experience"])
        self.assertEqual(outcome, "no_experience")
        self.assertIsNotNone(state.failed_identify_line)

    def test_exited_flushes_the_last_lines(self):
        outcome, state, _ = self.wait([IDENTIFIED], proc=FakeProc(exit_after=0.2, code=3))
        self.assertEqual(outcome, "exited")
        self.assertEqual(len(state.identified), 1)

    def test_timeout(self):
        outcome, _, elapsed = self.wait([], timeout=0.4)
        self.assertEqual(outcome, "timeout")
        self.assertGreaterEqual(elapsed, 0.35)
        self.assertLess(elapsed, 3.0)

    def test_known_failure_shortens_the_deadline(self):
        outcome, state, elapsed = self.wait([WRONG], timeout=60, grace=0.3)
        self.assertEqual(outcome, "timeout")
        self.assertLess(elapsed, 5.0)
        self.assertIn("B_LyraDefaultExperience", state.experience_failure())

    def test_wait_after_crash_returns_when_the_game_exits(self):
        path = os.path.join(self.tmp, "game.log")
        write_lines(path, ["x"])
        tail = pb.LogTail(path)
        self.addCleanup(tail.close)
        t0 = time.time()
        with captured_log():
            pb.wait_after_crash(FakeProc(exit_after=0.2), tail, pb.BootState("B_X"), t0,
                                grace_s=10, poll_s=0.05)
        self.assertLess(time.time() - t0, 3.0)


class TestGate(unittest.TestCase):
    EXE = os.path.abspath(r"C:\Fake\Win64\LyraGame.exe")

    def fresh(self, **kw):
        base = {"build_info_path": "C:/b.json", "fresh": True, "reasons": [], "warnings": [],
                "git_error": None, "build": {"commit": "abc", "game_exe": self.EXE},
                "head": "abc1234567890", "branch": "main", "current_dirty_files": []}
        base.update(kw)
        return base

    def run_gate(self, fresh, allow):
        with mock.patch.object(pb, "find_build_info", return_value="C:/b.json"), \
                mock.patch.object(pb, "check_build_freshness", return_value=fresh), \
                captured_log() as out:
            result = pb.gate(self.EXE, allow)
        return result, out.getvalue()

    def test_fresh(self):
        (fresh, code), text = self.run_gate(self.fresh(), False)
        self.assertIsNone(code)
        self.assertIn("Build is fresh: commit abc1234567", text)

    def test_stale_refused(self):
        (fresh, code), text = self.run_gate(self.fresh(fresh=False, reasons=["commit a != b"]),
                                            False)
        self.assertEqual(code, pb.EXIT_INFRA)
        self.assertIn("STALE BUILD", text)
        self.assertIn("Re-package", text)

    def test_stale_allowed(self):
        (fresh, code), text = self.run_gate(self.fresh(fresh=False, reasons=["commit a != b"]),
                                            True)
        self.assertIsNone(code)
        self.assertIn("continuing: --allow-stale", text)

    def test_git_error(self):
        err = self.fresh(fresh=False, git_error="git missing", reasons=["git error: git missing"])
        (_, code), text = self.run_gate(dict(err), False)
        self.assertEqual(code, pb.EXIT_INFRA)
        self.assertIn("ERROR: cannot verify build freshness: git missing", text)
        (_, code), text = self.run_gate(dict(err), True)
        self.assertIsNone(code)
        self.assertIn("WARNING: cannot verify build freshness", text)

    def test_other_exe_warns(self):
        (fresh, code), text = self.run_gate(
            self.fresh(build={"commit": "abc", "game_exe": r"C:\Other\LyraGame.exe"}), False)
        self.assertIsNone(code)
        self.assertTrue(any("build_info.json describes" in w for w in fresh["warnings"]))
        self.assertIn("WARNING: running", text)


SLEEPER = "import sys, time\ntime.sleep(float(sys.argv[1]))\n"


@unittest.skipUnless(sys.platform == "win32", "Windows job objects")
class TestLaunchGame(TempDirCase):
    def sleeper(self):
        path = os.path.join(self.tmp, "sleeper.py")
        with open(path, "w", encoding="utf-8") as f:
            f.write(SLEEPER)
        return [sys.executable, path]

    def kill_later(self, proc):
        self.addCleanup(lambda: proc.poll() is None and pb.kill_tree(proc))

    def test_job_kills_the_game_when_closed(self):
        with captured_log():
            proc, job, command_line = pb.launch_game("unused.exe", ["30"], exe_argv=self.sleeper())
        self.kill_later(proc)
        self.assertTrue(command_line.endswith("sleeper.py 30"))
        self.assertIsNotNone(job)
        self.assertTrue(job.ok)
        self.assertIsNone(proc.poll())
        self.assertEqual(job.active_processes(), 1)
        job.close()
        proc.wait(timeout=10)
        self.assertIsNotNone(proc.returncode)

    def test_keep_running_has_no_job(self):
        proc, job, _ = pb.launch_game("unused.exe", ["30"], keep_running=True,
                                      exe_argv=self.sleeper())
        self.kill_later(proc)
        self.assertIsNone(job)
        self.assertIsNone(proc.poll())
        note = pb.kill_tree(proc)
        self.assertIn("terminated", note)

    def test_detached_launch_runs(self):
        proc, job, _ = pb.launch_game("unused.exe", ["30"], keep_running=True, detached=True,
                                      exe_argv=self.sleeper())
        self.kill_later(proc)
        self.assertIsNone(job)
        time.sleep(0.2)
        self.assertIsNone(proc.poll())
        pb.kill_tree(proc)

    def test_start_failure_raises_oserror(self):
        with self.assertRaises(OSError):
            pb.launch_game(os.path.join(self.tmp, "missing.exe"), ["-x"])


class TestWriteReports(TempDirCase):
    def report(self, engine, desktop):
        return {
            "result": "PASS", "exit_code": 0, "started_at": "s", "finished_at": "f",
            "run_dir": self.tmp, "exe": "C:/x.exe", "command_line": "x", "map": pb.DEFAULT_MAP,
            "build": {"build": None, "head": "abc", "branch": "b", "fresh": True, "reasons": [],
                      "warnings": []},
            "checks": [{"name": "launch", "status": "PASS", "detail": "pid 1", "kind": "infra"}],
            "experience": {"expected": "B_X", "identified": None, "load_complete": None,
                           "load_time_s": None, "all_identified": []},
            "fatal_lines": [], "fail_on_errors": False, "warnings": [],
            "log": {"path": "g.log", "lines": 1, "errors": 0, "warnings": 0, "top_errors": [],
                    "tail": []},
            "screenshots": desktop, "engine_screenshots": engine,
        }

    def test_report_md_lists_engine_screenshots(self):
        engine = [{"path": os.path.join(self.tmp, "engine_01.png"), "uniform": False},
                  {"path": os.path.join(self.tmp, "engine_02.png"), "uniform": True,
                   "black": True, "mean": 1.0, "stddev": 0.1}]
        desktop = [{"path": os.path.join(self.tmp, "shot_01.png"), "uniform": False,
                    "occluded": False}]
        _, md_path = pb.write_reports(self.tmp, self.report(engine, desktop))
        with open(md_path, encoding="utf-8") as f:
            md = f.read()
        self.assertIn("## Engine screenshots", md)
        self.assertIn(engine[0]["path"], md)
        self.assertIn("near-uniform black frame", md)
        self.assertIn("## Screenshots", md)
        self.assertIn(desktop[0]["path"], md)
        self.assertLess(md.index("## Engine screenshots"), md.index("## Screenshots"))
        with open(os.path.join(self.tmp, "report.json"), encoding="utf-8") as f:
            self.assertEqual(len(json.load(f)["engine_screenshots"]), 2)

    def test_no_engine_screenshots(self):
        _, md_path = pb.write_reports(self.tmp, self.report([], []))
        with open(md_path, encoding="utf-8") as f:
            md = f.read()
        self.assertEqual(md.count("- (none captured)"), 2)


@unittest.skipUnless(sys.platform == "win32", "Windows .bat wrapper and job objects")
class TestMainEndToEnd(TempDirCase):
    """playtest_boot.main() boots tests/fake_game.py (through a .bat, as if it were an exe)."""

    def test_main_passes_with_the_fake_game(self):
        bat = os.path.join(self.tmp, "FakeGame.bat")
        with open(bat, "w", encoding="ascii") as f:
            f.write(f'@"{sys.executable}" "{FAKE_GAME}" %*\n')
        playtests = os.path.join(self.tmp, "Playtests")
        out = io.StringIO()
        argv = ["playtest_boot.py", "--exe", bat, "--allow-stale", "--shots", "0",
                "--timeout", "60"]
        with mock.patch.object(pb, "PLAYTESTS_DIR", playtests), \
                mock.patch.object(sys, "argv", argv), pb.log_stream(out), \
                contextlib.redirect_stdout(out):
            code = pb.main()
        text = out.getvalue()
        self.assertEqual(code, pb.EXIT_PASS, text)
        runs = os.listdir(playtests)
        self.assertEqual(len(runs), 1)
        self.assertTrue(runs[0].endswith("-boot"))
        run_dir = os.path.join(playtests, runs[0])
        with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as f:
            report = json.load(f)
        status = {c["name"]: c["status"] for c in report["checks"]}
        self.assertEqual(status["build_fresh"], "WARN")  # no build_info.json next to the .bat
        self.assertEqual(status["launch"], "PASS")
        self.assertEqual(status["experience_identified"], "PASS")
        self.assertEqual(status["experience_loaded"], "PASS")
        self.assertEqual(status["no_crash"], "PASS")
        self.assertEqual(status["content_cooked"], "PASS")
        self.assertEqual(status["game_window"], "SKIP")
        self.assertEqual(report["experience"]["outcome"], "loaded")
        self.assertIn("-abslog=", report["command_line"])
        self.assertNotIn("-PlaytestCapture", report["command_line"])
        with open(os.path.join(run_dir, "report.md"), encoding="utf-8") as f:
            md = f.read()
        self.assertIn("# Boot playtest: PASS", md)
        self.assertIn("## Engine screenshots", md)
        self.assertIn("STALE BUILD (continuing: --allow-stale)", text)
        self.assertIn("BOOT PLAYTEST: PASS", text)

    def test_run_dir_option_writes_the_run_there(self):
        bat = os.path.join(self.tmp, "FakeGame.bat")
        with open(bat, "w", encoding="ascii") as f:
            f.write(f'@"{sys.executable}" "{FAKE_GAME}" %*\n')
        playtests = os.path.join(self.tmp, "Playtests")
        run_dir = os.path.join(self.tmp, "chosen dir", "x-boot")
        out = io.StringIO()
        argv = ["playtest_boot.py", "--exe", bat, "--allow-stale", "--shots", "0",
                "--timeout", "60", "--run-dir", run_dir]
        with mock.patch.object(pb, "PLAYTESTS_DIR", playtests), \
                mock.patch.object(sys, "argv", argv), pb.log_stream(out), \
                contextlib.redirect_stdout(out):
            code = pb.main()
        self.assertEqual(code, pb.EXIT_PASS, out.getvalue())
        self.assertFalse(os.path.exists(playtests), "no timestamped run dir is created")
        with open(os.path.join(run_dir, "report.json"), encoding="utf-8") as f:
            report = json.load(f)
        self.assertEqual(report["run_dir"], os.path.abspath(run_dir))
        self.assertTrue(os.path.isfile(os.path.join(run_dir, "report.md")))
        self.assertIn(f"Run dir: {os.path.abspath(run_dir)}", out.getvalue())


class TestRunDirOptionIsHidden(unittest.TestCase):
    def test_not_in_help(self):
        r = subprocess.run([sys.executable, os.path.join(SCRIPTS, "playtest_boot.py"), "--help"],
                           capture_output=True, text=True, timeout=60)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertNotIn("--run-dir", r.stdout)


if __name__ == "__main__":
    unittest.main()
