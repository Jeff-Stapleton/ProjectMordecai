#!/usr/bin/env python3
"""Exploratory Playtest -- a headless agent free-plays the PACKAGED game (Phase 3)

Runs a headless Claude Code session (claude -p) whose ONLY tools are the mordecai-play MCP tools
(Scripts/mordecai_play_mcp.py).  Under the charter Scripts/explorer/charter.md it plays the
packaged game for N minutes with real input -- moves everywhere, casts every spell, cycles
weapons, opens every menu and tab, combines inputs -- looks at every screenshot and files what a
player would call a bug with report_finding.  Afterwards automatic detectors scan the run dir
(crash markers, stuck walks, game.log errors, missing content, blank frames) and a report is
written.  The contract is Specs/story_playtests_v1.md section 5.

This is exploration, NOT acceptance: nothing it does accepts an AC (story acceptance comes from
Scripts/playtests/usXXX_*.py).  Findings are leads; confirm each one before filing it.

Usage:
  py Scripts/explore_playtest.py                       10 minutes on DevTestMap
  py Scripts/explore_playtest.py --minutes 20 --focus "spells and the pause menu"
  py Scripts/explore_playtest.py --frontend            start at the front end
  py Scripts/explore_playtest.py --allow-stale         a stale package (findings are against the
                                                       packaged commit; recorded)
  py Scripts/explore_playtest.py --dry-run             print the claude command and the prompt,
                                                       run nothing
  py Scripts/explore_playtest.py --attach-port N [--token T]
        diagnostic: the agent plays a bridge that is already running (agent_bridge.py launch, or
        Scripts/tests/fake_bridge.py); no STALE gate, always recorded as diagnostic
  Options: --model M, --max-budget-usd D (default 10), --charter FILE
  Tests only: --agent-cmd JSON (argv that replaces "claude"; the claude arguments are appended),
              --run-dir DIR, --test-skip-freshness, --test-exe-argv JSON, --test-wall-clock-s S

How the agent runs: claude -p --mcp-config <run_dir>/mcp_config.json --strict-mcp-config
--tools "" --allowedTools "mcp__mordecai-play,mcp__mordecai-play__*" --permission-mode dontAsk
--output-format stream-json --verbose --no-session-persistence --max-budget-usd D [--model M],
with the prompt on stdin (prompt.txt; no 32k command-line limit), in a fresh temp directory
outside the repo (so the project's CLAUDE.md, .mcp.json and settings do not load), in a
kill-on-close job.  Wall clock: minutes*60 + 300 s, then the process tree is killed (the MCP
server dies with it, and the game with the server).

Output: Saved/Playtests/<YYYYmmdd-HHMMSS>-explore/
  report.md, report.json        build, duration, agent exit and cost, tool calls, findings
                                (agent, then detectors), the agent's summary, every screenshot
  prompt.txt, mcp_config.json   what the agent was given
  agent.jsonl, agent.stderr.log the agent's stream-json transcript and stderr
  findings.jsonl                the agent's findings (report_finding)
  game.log, bridge.jsonl, bridge_NN.png   the game (a relaunch: launch_NN/ subdirs)

Exit codes:
  0 -- no crash or blocker finding, no new major finding, no detector crash
  1 -- any crash or blocker finding (agent or detector), or any new (not known) major finding
  2 -- infrastructure problem: no claude CLI, STALE BUILD without --allow-stale, the agent never
       launched (or attached to) the game
"""

import argparse
import collections
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import playtest_boot as pb  # noqa: E402

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
RUN_SUFFIX = "explore"
SERVER_NAME = "mordecai-play"
MCP_SERVER = os.path.join(SCRIPT_DIR, "mordecai_play_mcp.py")
CHARTER = os.path.join(SCRIPT_DIR, "explorer", "charter.md")
PLAN_DOC = os.path.join(PROJECT_ROOT, "Specs", "agent_playtest_plan_v1.md")
BRIDGE_DOC = os.path.join(PROJECT_ROOT, "Specs", "agent_bridge_v1.md")
KNOWN_ISSUES_HEADING = r"Known issues backlog"
BINDINGS_HEADING = r"5\.\s+Key names"
# Both forms: the server-level name and the wildcard (Claude Code accepts either).
ALLOWED_TOOLS = (f"mcp__{SERVER_NAME}", f"mcp__{SERVER_NAME}__*")
TOOL_PREFIX = f"mcp__{SERVER_NAME}__"

DEFAULT_MINUTES = 10.0
DEFAULT_MAX_BUDGET_USD = 10.0
WALL_SLACK_S = 300          # wall clock = minutes*60 + this (boot, final summary, quit)
CHILD_GRACE_S = 15          # after the agent exits, let the MCP server quit the game cleanly

PROMPT_NAME = "prompt.txt"
CONFIG_NAME = "mcp_config.json"
STREAM_NAME = "agent.jsonl"
STDERR_NAME = "agent.stderr.log"
FINDINGS_NAME = "findings.jsonl"
TRANSCRIPT_NAME = "bridge.jsonl"

SEVERITIES = ("crash", "blocker", "major", "minor", "cosmetic", "question")
STUCK_CODES = frozenset({"stuck", "walk_timeout"})
WAIT_CODES = frozenset({"timeout", "wait_for_timeout"})
LOST_CODES = frozenset({"disconnected", "no_response", "closed", "connect_failed"})
RE_SHOT = re.compile(r"^bridge_\d+\.png$", re.IGNORECASE)

EVIDENCE = ("EXPLORATION ONLY -- findings are leads to confirm, never acceptance evidence "
            "(story acceptance comes from Scripts/playtests/usXXX_*.py)")
AGENT_LINE = ("AGENT: open every screenshot behind a NEW finding with the Read tool and confirm "
              "it before filing it (Known issues backlog in Specs/agent_playtest_plan_v1.md, or a "
              "recovery story when it breaks a done feature).")

EXIT_PASS, EXIT_FAIL, EXIT_INFRA = 0, 1, 2


def log(msg):
    print(f"[Explore] {msg}", flush=True)


def _dumps(obj):
    return json.dumps(obj, ensure_ascii=False, separators=(",", ":"), default=str)


# ---------------------------------------------------------------------------
# The prompt
# ---------------------------------------------------------------------------
def extract_section(path, heading_re):
    """The markdown section whose heading matches heading_re, up to the next heading of the same
    or a higher level.  Returns (text, None) or (None, why)."""
    try:
        with open(path, "r", encoding="utf-8-sig", errors="replace") as f:
            lines = f.read().splitlines()
    except OSError as e:
        return None, f"cannot read {path}: {e}"
    pattern = re.compile(r"^(#{1,6})\s+" + heading_re)
    start = level = None
    for i, line in enumerate(lines):
        m = pattern.match(line)
        if m:
            start, level = i, len(m.group(1))
            break
    if start is None:
        return None, f"no heading matching {heading_re!r} in {path}"
    end = len(lines)
    for j in range(start + 1, len(lines)):
        m = re.match(r"^(#{1,6})\s", lines[j])
        if m and len(m.group(1)) <= level:
            end = j
            break
    return "\n".join(lines[start:end]).strip(), None


def build_prompt(charter, minutes, run_dir, *, focus=None, frontend=False, allow_stale=False,
                 attach_port=None, fresh=None, known_issues=None, bindings=None):
    """The whole prompt (it goes to claude on stdin)."""
    if attach_port is not None:
        start = ("The game is ALREADY RUNNING (a diagnostic attach to 127.0.0.1:"
                 f"{attach_port}): do NOT call launch_game. Start with get_state and screenshot.")
    else:
        call = "launch_game(" + ", ".join(
            (["frontend=true"] if frontend else []) + (["allow_stale=true"] if allow_stale
                                                       else [])) + ")"
        start = f"Start by calling {call}."
        if frontend:
            start += (" It boots the FRONT END (the main menu), not the arena: reach the game "
                      "the way a player does, with clicks on the menu buttons.")
        if allow_stale:
            build = (fresh or {}).get("build") or {}
            start += (" This package is STALE (it does not match the code in the repository): "
                      "your findings apply to the packaged commit "
                      f"{pb.short(build.get('commit'))}, and every result says 'STALE BUILD'.")
    lines = [
        charter.strip(),
        "",
        "# This session",
        "",
        f"- Play-time budget: {minutes:g} minutes. The mordecai-play server counts it from "
        + ("your first tool call" if attach_port is not None else "launch_game")
        + "; every tool result ends with 'time left: Ns' and session_info tells you the rest. "
        "When the budget is used up the input tools are refused: file your last findings, call "
        "quit_game and write your final summary.",
        f"- {start}",
        f"- Run dir: {run_dir} (findings.jsonl, game.log, bridge.jsonl and bridge_NN.png go "
        "there; you cannot read files, so judge from screenshots and get_state).",
        "- Your only tools are the mordecai-play tools (mcp__mordecai-play__*).",
    ]
    if focus:
        lines.append(f"- FOCUS for this session (spend most of your time here, then cover the "
                     f"rest of the checklist): {focus}")
    lines += ["", "# Known issues (mark a matching finding with known_issue = the entry's text; "
              "do not file it as new)", "",
              known_issues or "(the Known issues backlog could not be read)", "",
              "# Live key bindings (Specs/agent_bridge_v1.md section 5)", "",
              bindings or "(the key bindings section could not be read)", ""]
    return "\n".join(lines)


def mcp_config(run_dir, budget_s, *, attach_port=None, token=None, skip_freshness=False,
               exe_argv=None):
    """The --mcp-config JSON: only the mordecai-play server, with absolute paths."""
    args = ["-X", "utf8", MCP_SERVER, "--run-dir", os.path.abspath(run_dir),
            "--budget-s", f"{budget_s:g}"]
    if attach_port is not None:
        args += ["--attach", str(int(attach_port))]
        if token:
            args += ["--token", token]
    if skip_freshness:
        args.append("--test-skip-freshness")
    if exe_argv:
        args += ["--test-exe-argv", json.dumps(exe_argv)]
    return {"mcpServers": {SERVER_NAME: {"type": "stdio", "command": sys.executable,
                                         "args": args, "env": {"PYTHONUNBUFFERED": "1"}}}}


def agent_command(program, config_path, max_budget_usd, model=None):
    """argv: program (["claude.exe"] or the --agent-cmd list) plus the claude arguments.  The
    prompt is NOT on the command line: it goes to stdin."""
    cmd = list(program) + [
        "-p", "--mcp-config", os.path.abspath(config_path), "--strict-mcp-config",
        "--tools", "", "--allowedTools", ",".join(ALLOWED_TOOLS),
        "--permission-mode", "dontAsk", "--output-format", "stream-json", "--verbose",
        "--no-session-persistence", "--max-budget-usd", f"{max_budget_usd:g}"]
    if model:
        cmd += ["--model", model]
    return cmd


# ---------------------------------------------------------------------------
# Running the agent
# ---------------------------------------------------------------------------
def run_agent(cmd, prompt_path, stream_path, stderr_path, wall_s, cwd):
    """Run the agent in a kill-on-close job; kill the tree after wall_s seconds.

    Returns {exit_code, killed_by_wall_clock, interrupted, shutdown, job, duration_s, error}.
    """
    out = {"exit_code": None, "killed_by_wall_clock": False, "interrupted": False,
           "shutdown": None, "job": False, "duration_s": 0.0, "error": None}
    t0 = time.time()
    env = dict(os.environ, PYTHONUNBUFFERED="1")
    with open(prompt_path, "rb") as fin, open(stream_path, "wb") as fout, \
            open(stderr_path, "wb") as ferr:
        try:
            proc = subprocess.Popen(cmd, stdin=fin, stdout=fout, stderr=ferr, cwd=cwd, env=env)
        except OSError as e:
            out["error"] = f"could not start the agent: {e}"
            return out
        job = None
        if sys.platform == "win32":
            try:
                job = pb.KillOnCloseJob(proc.pid)
                if not job.ok:
                    log(f"NOTE: no kill-on-close job for the agent ({job.error}); relying on "
                        "taskkill /T")
                    job = None
            except Exception as e:
                log(f"NOTE: no kill-on-close job for the agent ({e})")
                job = None
        out["job"] = job is not None
        try:
            out["exit_code"] = proc.wait(timeout=wall_s)
        except subprocess.TimeoutExpired:
            out["killed_by_wall_clock"] = True
            log(f"Wall clock of {wall_s:.0f}s reached: killing the agent's process tree")
            out["shutdown"] = pb.kill_tree(proc, job)
            out["exit_code"] = proc.returncode
        except KeyboardInterrupt:
            out["interrupted"] = True
            log("Interrupted: killing the agent's process tree")
            out["shutdown"] = pb.kill_tree(proc, job)
            out["exit_code"] = proc.returncode
        finally:
            if job is not None:
                # The MCP server quits its game when claude closes its stdin; give it a moment,
                # then kill whatever is left in the job (server, game, crash reporter).
                end = time.time() + CHILD_GRACE_S
                while (job.active_processes() or 0) > 0 and time.time() < end:
                    time.sleep(0.25)
                left = job.terminate()
                if left:
                    note = f"killed {left} leftover process(es) in the agent's job"
                    out["shutdown"] = "; ".join(n for n in (out["shutdown"], note) if n)
                job.close()
    out["duration_s"] = round(time.time() - t0, 1)
    return out


def parse_stream(path):
    """Read the agent's stream-json transcript defensively.

    Returns {events, malformed, result (the last "result" event or None), cost_usd, num_turns,
    is_error, subtype, result_text, duration_ms, mcp_servers, mordecai_tools_seen, tool_calls,
    last_text}.  Missing fields are None; nothing here raises on odd input.
    """
    out = {"events": 0, "malformed": 0, "result": None, "cost_usd": None, "num_turns": None,
           "is_error": None, "subtype": None, "result_text": None, "duration_ms": None,
           "mcp_servers": None, "mordecai_tools_seen": [], "tool_calls": {}, "last_text": None}
    calls = collections.Counter()
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            raw_lines = f.read().splitlines()
    except OSError:
        return out
    for raw in raw_lines:
        if not raw.strip():
            continue
        try:
            ev = json.loads(raw)
        except ValueError:
            out["malformed"] += 1
            continue
        if not isinstance(ev, dict):
            out["malformed"] += 1
            continue
        out["events"] += 1
        kind = ev.get("type")
        if kind == "system" and ev.get("subtype") == "init":
            servers = ev.get("mcp_servers")
            if isinstance(servers, list):
                out["mcp_servers"] = [{"name": s.get("name"), "status": s.get("status")}
                                      for s in servers if isinstance(s, dict)]
            tools = ev.get("tools")
            if isinstance(tools, list):
                out["mordecai_tools_seen"] = sorted(
                    t for t in tools if isinstance(t, str) and t.startswith(TOOL_PREFIX))
        elif kind == "assistant":
            msg = ev.get("message") if isinstance(ev.get("message"), dict) else {}
            content = msg.get("content") if isinstance(msg.get("content"), list) else []
            for block in content:
                if not isinstance(block, dict):
                    continue
                if block.get("type") == "tool_use":
                    calls[str(block.get("name"))] += 1
                elif block.get("type") == "text" and str(block.get("text") or "").strip():
                    out["last_text"] = str(block.get("text"))
        elif kind == "result":
            out["result"] = ev
    out["tool_calls"] = dict(sorted(calls.items()))
    res = out["result"]
    if isinstance(res, dict):
        cost = res.get("total_cost_usd", res.get("cost_usd"))
        out["cost_usd"] = cost if isinstance(cost, (int, float)) else None
        turns = res.get("num_turns")
        out["num_turns"] = turns if isinstance(turns, int) else None
        out["is_error"] = res.get("is_error") if isinstance(res.get("is_error"), bool) else None
        out["subtype"] = res.get("subtype")
        text = res.get("result")
        out["result_text"] = text if isinstance(text, str) else None
        dur = res.get("duration_ms")
        out["duration_ms"] = dur if isinstance(dur, (int, float)) else None
    return out


# ---------------------------------------------------------------------------
# Reading the run dir
# ---------------------------------------------------------------------------
def game_dirs(run_dir):
    """The run dir and its relaunch subdirs (launch_NN/, written by the MCP server)."""
    dirs = [run_dir]
    try:
        dirs += [os.path.join(run_dir, n) for n in sorted(os.listdir(run_dir))
                 if n.startswith("launch_") and os.path.isdir(os.path.join(run_dir, n))]
    except OSError:
        pass
    return dirs


def read_jsonl(path):
    """(entries, unreadable line count); a missing file is ([], 0)."""
    entries, bad = [], 0
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                if not line.strip():
                    continue
                try:
                    obj = json.loads(line)
                except ValueError:
                    bad += 1
                    continue
                if isinstance(obj, dict):
                    entries.append(obj)
                else:
                    bad += 1
    except OSError:
        pass
    return entries, bad


def transcript_entries(run_dir):
    """[(dir, entry)] for every bridge.jsonl entry in the run dir and its relaunch subdirs."""
    out = []
    for d in game_dirs(run_dir):
        entries, _ = read_jsonl(os.path.join(d, TRANSCRIPT_NAME))
        out += [(d, e) for e in entries]
    return out


def notes_of(entries, name=None):
    return [(d, e["msg"]) for d, e in entries
            if e.get("dir") == "note" and isinstance(e.get("msg"), dict)
            and (name is None or e["msg"].get("note") == name)]


def played(entries):
    """True when the agent launched (or attached to) a game: the bridge client's launch/attach
    note, or any request sent to a bridge."""
    if any(m.get("note") in ("launch", "attach") for _, m in notes_of(entries)):
        return True
    return any(e.get("dir") == "send" for _, e in entries)


def screenshots(run_dir):
    shots = []
    for d in game_dirs(run_dir):
        try:
            names = sorted(n for n in os.listdir(d) if RE_SHOT.match(n))
        except OSError:
            continue
        shots += [os.path.join(d, n) for n in names]
    return shots


def load_agent_findings(run_dir):
    """The agent's findings.jsonl, normalized: severity lowercase (unknown -> question)."""
    entries, bad = read_jsonl(os.path.join(run_dir, FINDINGS_NAME))
    out = []
    for i, e in enumerate(entries, 1):
        f = dict(e)
        sev = str(f.get("severity") or "").strip().lower()
        f["severity"] = sev if sev in SEVERITIES else "question"
        f.setdefault("id", f"F{i:02d}")
        f["source"] = "agent"
        f["known_issue"] = f.get("known_issue") or None
        f["title"] = str(f.get("title") or "(no title)")
        out.append(f)
    return out, bad


# ---------------------------------------------------------------------------
# Detectors (they need no agent)
# ---------------------------------------------------------------------------
def _finding(detector, severity, title, details, evidence=None):
    return {"detector": detector, "severity": severity, "title": title, "details": details,
            "evidence": evidence or [], "known_issue": None, "source": "detector"}


def detect_crash(run_dir, entries):
    out = []
    for d in game_dirs(run_dir):
        game_log = os.path.join(d, "game.log")
        markers = []
        if os.path.isfile(game_log):
            try:
                with open(game_log, "r", encoding="utf-8-sig", errors="replace") as f:
                    for line in f:
                        if any(m in line for m in pb.CRASH_MARKERS):
                            markers.append(pb.strip_timestamp(line.rstrip("\r\n")))
            except OSError:
                pass
        exited = [m for dd, m in notes_of(entries, "game_exited") if dd == d]
        if not markers and not exited:
            continue
        parts = []
        if markers:
            parts.append(f"{len(markers)} crash marker(s) in game.log, first: {markers[0][:300]}")
        for m in exited:
            parts.append(f"the game exited while the agent was playing (exit code "
                         f"{m.get('exit_code')}" + (f", during {m['during']}" if m.get("during")
                                                    else "") + ")")
        out.append(_finding("crash", "crash", "The game crashed" if markers
                            else "The game exited while the agent was playing",
                            "; ".join(parts), [game_log] if os.path.isfile(game_log) else []))
    return out


def detect_stuck(run_dir, entries):
    errors = [m for _, m in notes_of(entries, "mcp_error")]
    walks, waits, lost = [], [], []
    if errors:
        for m in errors:
            code, text = m.get("code"), f"{m.get('tool')}: {m.get('message')}"
            if code in STUCK_CODES:
                walks.append(text)
            elif code in WAIT_CODES:
                waits.append(text)
            elif code in LOST_CODES:
                lost.append(text)
    else:
        # A transcript without the server's notes (an older server): read the raw responses.
        for _, e in entries:
            msg = e.get("msg") if isinstance(e.get("msg"), dict) else {}
            if e.get("dir") == "recv" and msg.get("ok") is False and msg.get("error") == "timeout":
                waits.append(str(msg.get("message") or "timeout"))
            blob = _dumps(msg) if msg else str(e.get("line") or "")
            if "stuck at (" in blob:
                walks.append(blob[:300])
    transcripts = [os.path.join(d, TRANSCRIPT_NAME) for d in game_dirs(run_dir)
                   if os.path.isfile(os.path.join(d, TRANSCRIPT_NAME))]
    out = []
    if walks:
        out.append(_finding("stuck", "minor", f"walk_to got stuck or timed out {len(walks)} "
                            "time(s)", "; ".join(walks[:5]) + " (check whether the agent walked "
                            "into a wall on purpose)", transcripts))
    if waits:
        out.append(_finding("stuck", "minor", f"wait_until timed out {len(waits)} time(s)",
                            "; ".join(waits[:5]), transcripts))
    if lost:
        out.append(_finding("stuck", "major", f"Lost the bridge connection {len(lost)} time(s)",
                            "; ".join(lost[:5]), transcripts))
    return out


def detect_log_errors(run_dir):
    out = []
    for d in game_dirs(run_dir):
        game_log = os.path.join(d, "game.log")
        if not os.path.isfile(game_log):
            continue
        s = pb.summarize_log(game_log)
        if not s["errors"]:
            continue
        mordecai = collections.Counter()
        try:
            with open(game_log, "r", encoding="utf-8-sig", errors="replace") as f:
                for raw in f:
                    line = pb.strip_timestamp(raw.rstrip("\r\n"))
                    m = pb.RE_SEVERITY.match(line)
                    if m and m.group(2) == "Error" and m.group(1).startswith("LogMordecai"):
                        mordecai[line[:300]] += 1
        except OSError:
            pass
        top = [f"(x{c}) {l}" for l, c in mordecai.most_common(5)]
        top += [f"(x{e['count']}) {e['line']}" for e in s["top_errors"]
                if not e["line"].startswith("LogMordecai")][:max(0, 8 - len(top))]
        out.append(_finding(
            "log_errors", "minor" if mordecai else "question",
            f"game.log: {s['errors']} error(s), {s['warnings']} warning(s)"
            + (f", {sum(mordecai.values())} from LogMordecai*" if mordecai else ""),
            "; ".join(top), [game_log]))
    return out


def detect_missing_content(run_dir):
    out = []
    for d in game_dirs(run_dir):
        game_log = os.path.join(d, "game.log")
        if not os.path.isfile(game_log):
            continue
        missing = pb.find_missing_content(game_log)
        if missing:
            out.append(_finding("missing_content", "major",
                                f"{len(missing)} /MordecaiCore package(s) requested but not in "
                                "the build", ", ".join(missing[:12]), [game_log]))
    return out


def detect_blank_frames(shots, run_dir):
    """(findings, per-shot analysis, note).  Needs Pillow; without it the detector is skipped."""
    try:
        from PIL import Image
    except ImportError:
        return [], [{"path": p} for p in shots], "skipped: Pillow is not installed"
    infos, blank = [], []
    for path in shots:
        try:
            with Image.open(path) as img:
                img.load()
                info = pb.analyze_image(img)
        except Exception as e:  # truncated or unreadable PNG
            infos.append({"path": path, "error": str(e)})
            blank.append(f"{os.path.basename(path)} unreadable ({e})")
            continue
        info["path"] = path
        infos.append(info)
        if info["uniform"]:
            blank.append(f"{os.path.relpath(path, run_dir)} near-uniform{' black' if info['black'] else ''} (mean {info['mean']}, "
                         f"stddev {info['stddev']})")
    findings = []
    if blank:
        findings.append(_finding("blank_frames", "minor",
                                 f"{len(blank)} of {len(shots)} screenshot(s) near-uniform or "
                                 "unreadable (no visual evidence for those steps)",
                                 "; ".join(blank[:8]),
                                 [i["path"] for i in infos if i.get("uniform") or i.get("error")]))
    return findings, infos, None


def run_detectors(run_dir):
    """Every detector over the run dir.  Returns (findings, detector status, screenshot info)."""
    entries = transcript_entries(run_dir)
    shots = screenshots(run_dir)
    status = {}
    findings = []
    for name, fn in (("crash", lambda: detect_crash(run_dir, entries)),
                     ("stuck", lambda: detect_stuck(run_dir, entries)),
                     ("log_errors", lambda: detect_log_errors(run_dir)),
                     ("missing_content", lambda: detect_missing_content(run_dir))):
        try:
            found = fn()
            status[name] = {"ran": True, "findings": len(found)}
            findings += found
        except Exception as e:  # a broken detector must not lose the report
            status[name] = {"ran": False, "error": f"{type(e).__name__}: {e}"}
    try:
        found, shot_info, note = detect_blank_frames(shots, run_dir)
        status["blank_frames"] = {"ran": note is None, "findings": len(found)}
        if note:
            status["blank_frames"]["note"] = note
        findings += found
    except Exception as e:
        shot_info = [{"path": p} for p in shots]
        status["blank_frames"] = {"ran": False, "error": f"{type(e).__name__}: {e}"}
    if not any(os.path.isfile(os.path.join(d, "game.log")) for d in game_dirs(run_dir)):
        for name in ("log_errors", "missing_content"):
            status[name]["note"] = "no game.log in the run dir (attached game, or never launched)"
    for i, f in enumerate(findings, 1):
        f["id"] = f"D{i:02d}"
    return findings, status, shot_info


# ---------------------------------------------------------------------------
# Verdict and reports
# ---------------------------------------------------------------------------
def is_known(finding):
    return bool(finding.get("known_issue"))


def verdict(agent_findings, detector_findings, did_play, infra_reason=None):
    """(exit code, reason)."""
    if infra_reason:
        return EXIT_INFRA, infra_reason
    if not did_play:
        return EXIT_INFRA, "the agent never launched (or attached to) the game"
    every = list(agent_findings) + list(detector_findings)
    severe = [f for f in every if f["severity"] in ("crash", "blocker")]
    new_major = [f for f in every if f["severity"] == "major" and not is_known(f)]
    if severe or new_major:
        parts = []
        if severe:
            parts.append(f"{len(severe)} crash/blocker finding(s): "
                         + ", ".join(f"{f['id']} {f['title']}" for f in severe[:5]))
        if new_major:
            parts.append(f"{len(new_major)} new major finding(s): "
                         + ", ".join(f"{f['id']} {f['title']}" for f in new_major[:5]))
        return EXIT_FAIL, "; ".join(parts)
    return EXIT_PASS, "no crash or blocker finding and no new major finding"


def _sev_rank(f):
    return SEVERITIES.index(f["severity"]) if f["severity"] in SEVERITIES else len(SEVERITIES)


def md_cell(text, limit=300):
    text = pb.md_cell("" if text is None else text)
    return text if len(text) <= limit else text[:limit] + "..."


def write_reports(run_dir, report):
    json_path = os.path.join(run_dir, "report.json")
    with open(json_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(report, f, indent=2, ensure_ascii=False, default=str)
        f.write("\n")

    agent = report["agent"]
    build = report["build"]
    fnd = report["findings"]
    lines = [f"# Exploratory playtest: {report['result']} (exit {report['exit_code']})", "",
             f"- Evidence: **{report['evidence']}**"]
    for reason in report["diagnostic"]:
        lines.append(f"- DIAGNOSTIC: {reason}")
    lines += [
        f"- Verdict: {report['exit_reason']}",
        f"- Run dir: `{report['run_dir']}`",
        f"- Started: {report['started_at']}  Finished: {report['finished_at']}  "
        f"Duration: {report['duration_s']}s",
        f"- Budget: {report['minutes']:g} min of play (server budget {report['budget_s']:g}s), "
        f"wall clock {report['wall_clock_s']:g}s",
        f"- Mode: {report['mode']}"
        + (f" (127.0.0.1:{report['attach_port']})" if report["mode"] == "attach" else
           f" ({'front end' if report['frontend'] else 'DevTestMap'})"),
        f"- Focus: {report['focus'] or '(none: the whole charter)'}",
        f"- The agent played: {'yes' if report['played'] else 'NO'}",
        "", "## Build", ""]
    if build.get("skipped"):
        lines.append(f"- Freshness: not checked ({build.get('skipped')})")
    else:
        lines.append(f"- Packaged commit `{pb.short(build.get('commit'))}`"
                     f"{' (built from a DIRTY tree)' if build.get('dirty') else ''}; HEAD "
                     f"`{pb.short(build.get('head'))}` on `{build.get('branch')}`")
        if build.get("fresh"):
            lines.append("- Freshness: FRESH (the package matches HEAD and the working tree)")
        else:
            lines.append("- Freshness: **STALE BUILD (allowed by --allow-stale): the findings "
                         f"apply to the packaged commit {pb.short(build.get('commit'))}, not to "
                         "HEAD**")
            for r in build.get("reasons") or []:
                lines.append(f"  - {r}")
    for w in build.get("warnings") or []:
        lines.append(f"- Warning: {w}")

    lines += ["", "## Agent", "",
              f"- Command: `{subprocess.list2cmdline(agent['command'])}` (prompt on stdin: "
              f"`{PROMPT_NAME}`)",
              f"- Exit code: {agent['exit_code']}"
              + ("  **killed: wall clock reached**" if agent["killed_by_wall_clock"] else "")
              + ("  **interrupted**" if agent["interrupted"] else ""),
              f"- Cost: {('$%.4f' % agent['cost_usd']) if agent['cost_usd'] is not None else 'n/a'}"
              f"  Turns: {agent['num_turns'] if agent['num_turns'] is not None else 'n/a'}"
              f"  Result: {agent['subtype'] or 'n/a'}"
              + ("  **is_error**" if agent["is_error"] else ""),
              "- MCP servers: " + (", ".join(f"{s['name']} ({s['status']})"
                                             for s in agent["mcp_servers"])
                                   if agent["mcp_servers"] else "n/a (no init event)"),
              f"- mordecai-play tools the agent saw: {len(agent['mordecai_tools_seen'])}"]
    if agent.get("error"):
        lines.append(f"- Error: {agent['error']}")
    if agent.get("shutdown"):
        lines.append(f"- Shutdown: {agent['shutdown']}")
    if agent["stream_malformed_lines"]:
        lines.append(f"- Stream: {agent['stream_malformed_lines']} malformed line(s)")
    lines += ["", "| Tool | Calls (agent stream) | Calls (server) |", "|---|---|---|"]
    names = sorted(set(agent["tool_calls"]) | {TOOL_PREFIX + n for n in agent["server_tool_calls"]})
    for name in names:
        short_name = name[len(TOOL_PREFIX):] if name.startswith(TOOL_PREFIX) else name
        lines.append(f"| {md_cell(name)} | {agent['tool_calls'].get(name, 0)} | "
                     f"{agent['server_tool_calls'].get(short_name, 0)} |")
    if not names:
        lines.append("| (none) | 0 | 0 |")
    if agent["stderr_tail"]:
        lines += ["", "Agent stderr (tail):", "", "```"] + agent["stderr_tail"] + ["```"]

    lines += ["", "## Findings", "",
              f"{len(fnd['agent'])} from the agent ({fnd['new']} new, {fnd['known']} known), "
              f"{len(fnd['detector'])} from the detectors. Severity counts: "
              + (", ".join(f"{k} {v}" for k, v in fnd["counts"].items()) or "none"), "",
              "| ID | Source | Severity | Known? | Title | Screenshot |", "|---|---|---|---|---|---|"]
    for f in fnd["agent"] + fnd["detector"]:
        known = "known" if is_known(f) else "NEW"
        shot = f.get("screenshot") or ", ".join(f.get("evidence") or []) or ""
        lines.append(f"| {f['id']} | {f['source']}{'/' + f['detector'] if f.get('detector') else ''}"
                     f" | {f['severity']} | {known} | {md_cell(f['title'], 150)} | "
                     f"{md_cell(shot, 200)} |")
    if not fnd["agent"] and not fnd["detector"]:
        lines.append("| - | - | - | - | (no findings) | - |")
    if fnd["unreadable_lines"]:
        lines.append(f"\nWARNING: {fnd['unreadable_lines']} unreadable line(s) in {FINDINGS_NAME}")
    for f in fnd["agent"] + fnd["detector"]:
        lines += ["", f"### {f['id']} ({f['severity']}, {'known' if is_known(f) else 'NEW'}): "
                  f"{pb.md_cell(f['title'])}", "", f"- Details: {f.get('details')}"]
        for key, label in (("repro", "Repro"), ("expected", "Expected"),
                           ("known_issue", "Known issue"), ("player_location", "Player location"),
                           ("map", "Map"), ("build_commit", "Build commit"),
                           ("screenshot", "Screenshot")):
            if f.get(key) not in (None, "", []):
                lines.append(f"- {label}: {f[key]}")
        if f.get("evidence"):
            lines.append("- Evidence: " + ", ".join(f"`{p}`" for p in f["evidence"]))

    lines += ["", "## Detectors", ""]
    for name, st in report["detectors"].items():
        lines.append(f"- {name}: " + ("ran" if st.get("ran") else "did NOT run")
                     + (f", {st['findings']} finding(s)" if "findings" in st else "")
                     + (f" ({st['note']})" if st.get("note") else "")
                     + (f" ERROR {st['error']}" if st.get("error") else ""))

    lines += ["", "## The agent's final summary", ""]
    lines.append(agent["result_text"] or agent.get("last_text") or "(none: no result event)")

    lines += ["", "## Screenshots (open them)", ""]
    if report["screenshots"]:
        for s in report["screenshots"]:
            flag = ""
            if s.get("uniform"):
                flag = (f"  **WARNING: near-uniform{' black' if s.get('black') else ''} frame "
                        f"(mean {s.get('mean')}, stddev {s.get('stddev')})**")
            elif s.get("error"):
                flag = f"  **unreadable: {s['error']}**"
            lines.append(f"- {s['path']}{flag}")
    else:
        lines.append("- (none)")

    lines += ["", "## Files", ""]
    for label, path in report["files"].items():
        if isinstance(path, list):
            for p in path:
                lines.append(f"- {label}: `{p}`")
        elif path:
            lines.append(f"- {label}: `{path}`")
    lines += ["", AGENT_LINE, ""]
    md_path = os.path.join(run_dir, "report.md")
    with open(md_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    return json_path, md_path


def _tail(path, n=20):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return [l.rstrip("\r\n")[:400] for l in collections.deque(f, maxlen=n)]
    except OSError:
        return []


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        prog="explore_playtest.py",
        description="Exploratory playtest: a headless Claude Code session free-plays the packaged "
                    "game through the mordecai-play MCP tools and files findings.")
    parser.add_argument("--minutes", type=float, default=DEFAULT_MINUTES,
                        help=f"play-time budget in minutes ({DEFAULT_MINUTES:g})")
    parser.add_argument("--focus", default=None, help="what to spend most of the session on")
    parser.add_argument("--frontend", action="store_true",
                        help="start at the front end (launch_game(frontend=true))")
    parser.add_argument("--allow-stale", action="store_true",
                        help="run a STALE package (diagnostic; findings apply to the packaged "
                             "commit; recorded)")
    parser.add_argument("--dry-run", action="store_true",
                        help="print the claude command and the prompt, run nothing")
    parser.add_argument("--model", default=None, help="claude --model")
    parser.add_argument("--max-budget-usd", type=float, default=DEFAULT_MAX_BUDGET_USD,
                        help=f"claude --max-budget-usd ({DEFAULT_MAX_BUDGET_USD:g})")
    parser.add_argument("--charter", default=CHARTER, help=f"charter file ({CHARTER})")
    parser.add_argument("--attach-port", type=int, default=None,
                        help="diagnostic: the agent plays the bridge already on this port")
    parser.add_argument("--token", default=None, help="expected ping token (with --attach-port)")
    # Test-only.
    parser.add_argument("--agent-cmd", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--run-dir", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-skip-freshness", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--test-exe-argv", default=None, help=argparse.SUPPRESS)
    parser.add_argument("--test-wall-clock-s", type=float, default=None, help=argparse.SUPPRESS)
    args = parser.parse_args(argv)
    if args.minutes <= 0:
        parser.error("--minutes must be > 0")
    if args.max_budget_usd <= 0:
        parser.error("--max-budget-usd must be > 0")
    if args.agent_cmd:
        try:
            agent_cmd = json.loads(args.agent_cmd)
        except ValueError as e:
            parser.error(f"--agent-cmd must be a JSON list of strings: {e}")
        if not (isinstance(agent_cmd, list) and agent_cmd
                and all(isinstance(a, str) for a in agent_cmd)):
            parser.error("--agent-cmd must be a JSON list of strings")
        args.agent_cmd = agent_cmd
    if args.test_exe_argv:
        args.test_exe_argv = json.loads(args.test_exe_argv)
    if args.attach_port is not None and (args.allow_stale or args.frontend):
        log("NOTE: --allow-stale / --frontend do not apply with --attach-port (the attached "
            "game is already running)")
    return args


def _gate_record(fresh, skipped=None):
    build = (fresh or {}).get("build") or {}
    return {"skipped": skipped, "commit": build.get("commit"),
            "head": (fresh or {}).get("head"), "branch": (fresh or {}).get("branch"),
            "fresh": (fresh or {}).get("fresh"), "stale": (fresh or {}).get("fresh") is False,
            "dirty": bool(build.get("dirty")), "built_at": build.get("built_at"),
            "reasons": list((fresh or {}).get("reasons") or []),
            "warnings": list((fresh or {}).get("warnings") or [])}


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        try:
            stream.reconfigure(errors="replace")
        except (AttributeError, ValueError):
            pass
    try:
        args = parse_args(argv)
    except SystemExit as e:
        return EXIT_INFRA if e.code else EXIT_PASS
    mode = "attach" if args.attach_port is not None else "launch"
    budget_s = args.minutes * 60.0
    wall_s = (args.test_wall_clock_s if args.test_wall_clock_s is not None
              else budget_s + WALL_SLACK_S)

    try:
        with open(args.charter, "r", encoding="utf-8-sig") as f:
            charter = f.read()
    except OSError as e:
        log(f"ERROR: cannot read the charter {args.charter}: {e}")
        return EXIT_INFRA
    known, why = extract_section(PLAN_DOC, KNOWN_ISSUES_HEADING)
    if known is None:
        log(f"WARNING: no Known issues section: {why}")
    bindings, why = extract_section(BRIDGE_DOC, BINDINGS_HEADING)
    if bindings is None:
        log(f"WARNING: no key bindings section: {why}")

    if args.agent_cmd:
        program = args.agent_cmd
    else:
        found = shutil.which("claude")
        program = [found or "claude"]

    if args.dry_run:
        stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        run_dir = os.path.abspath(args.run_dir) if args.run_dir else os.path.join(
            pb.PLAYTESTS_DIR, f"{stamp}-{RUN_SUFFIX}")
        config = mcp_config(run_dir, budget_s, attach_port=args.attach_port, token=args.token,
                            skip_freshness=args.test_skip_freshness, exe_argv=args.test_exe_argv)
        cmd = agent_command(program, os.path.join(run_dir, CONFIG_NAME), args.max_budget_usd,
                            args.model)
        prompt = build_prompt(charter, args.minutes, run_dir, focus=args.focus,
                              frontend=args.frontend and mode == "launch",
                              allow_stale=args.allow_stale and mode == "launch",
                              attach_port=args.attach_port, known_issues=known, bindings=bindings)
        print("DRY RUN: nothing was run, no run dir was created; the STALE gate and the claude "
              "CLI check were not run.")
        print(f"Run dir (would be): {run_dir}")
        print(f"Working directory: a fresh temp dir outside the repo; wall clock {wall_s:g}s")
        print(f"Command (prompt on stdin): {subprocess.list2cmdline(cmd)}")
        print(f"{CONFIG_NAME}:")
        print(json.dumps(config, indent=2))
        print("----- prompt -----")
        print(prompt)
        print("----- end of prompt -----")
        return EXIT_PASS

    # --- Step 1: STALE gate, claude CLI (refused before any run dir exists) ---
    fresh, skipped = None, None
    if mode == "attach":
        skipped = "attached: this harness did not launch the game"
    elif args.test_skip_freshness:
        skipped = "staleness gate skipped (test run)"
    else:
        fresh, code = pb.gate(pb.DEFAULT_EXE, args.allow_stale)
        if code is not None:
            log("STALE BUILD: refusing to run (no run dir written).  Re-package with "
                "py Scripts/package_playtest.py; --allow-stale is diagnostic only.")
            return code
    if not args.agent_cmd and not shutil.which("claude"):
        log("ERROR: the claude CLI is not on PATH (install Claude Code: "
            "https://docs.claude.com/en/docs/claude-code)")
        return EXIT_INFRA

    # --- Step 2: run dir, mcp_config.json, prompt.txt ---
    run_dir = os.path.abspath(args.run_dir) if args.run_dir else pb.new_run_dir(RUN_SUFFIX)
    os.makedirs(run_dir, exist_ok=True)
    log(f"Run dir: {run_dir}")
    config_path = os.path.join(run_dir, CONFIG_NAME)
    config = mcp_config(run_dir, budget_s, attach_port=args.attach_port, token=args.token,
                        skip_freshness=args.test_skip_freshness, exe_argv=args.test_exe_argv)
    with open(config_path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(config, f, indent=2)
        f.write("\n")
    prompt = build_prompt(charter, args.minutes, run_dir, focus=args.focus,
                          frontend=args.frontend and mode == "launch",
                          allow_stale=args.allow_stale and mode == "launch",
                          attach_port=args.attach_port, fresh=fresh, known_issues=known,
                          bindings=bindings)
    prompt_path = os.path.join(run_dir, PROMPT_NAME)
    with open(prompt_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(prompt)
    cmd = agent_command(program, config_path, args.max_budget_usd, args.model)

    # --- Step 3: the agent, in a temp dir outside the repo ---
    started_at = datetime.datetime.now().isoformat(timespec="seconds")
    t0 = time.time()
    stream_path = os.path.join(run_dir, STREAM_NAME)
    stderr_path = os.path.join(run_dir, STDERR_NAME)
    work_dir = tempfile.mkdtemp(prefix="mordecai-explore-")
    log(f"Agent: {subprocess.list2cmdline(cmd)} (cwd {work_dir}, wall clock {wall_s:g}s)")
    try:
        result = run_agent(cmd, prompt_path, stream_path, stderr_path, wall_s, work_dir)
    finally:
        shutil.rmtree(work_dir, ignore_errors=True)
    log(f"Agent exit code {result['exit_code']} after {result['duration_s']}s"
        + (" (killed: wall clock)" if result["killed_by_wall_clock"] else ""))

    # --- Step 4: detectors, findings, verdict ---
    stream = parse_stream(stream_path)
    entries = transcript_entries(run_dir)
    did_play = played(entries)
    agent_findings, unreadable = load_agent_findings(run_dir)
    agent_findings.sort(key=_sev_rank)
    detector_findings, detector_status, shot_info = run_detectors(run_dir)
    server_calls = collections.Counter(str(m.get("tool")) for _, m in notes_of(entries,
                                                                               "mcp_tool"))
    infra = None
    if result.get("error"):
        infra = result["error"]
    elif result["interrupted"]:
        infra = "interrupted by the user"
    exit_code, exit_reason = verdict(agent_findings, detector_findings, did_play, infra)

    diagnostic = []
    if mode == "attach":
        diagnostic.append(f"attached to 127.0.0.1:{args.attach_port}: this harness did not "
                          "launch the game or check its build")
    if fresh is not None and fresh.get("fresh") is False:
        diagnostic.append("STALE BUILD allowed by --allow-stale: findings apply to the packaged "
                          f"commit {pb.short(((fresh.get('build') or {}).get('commit')))}")
    if fresh is not None and ((fresh.get("build") or {}).get("dirty")):
        diagnostic.append("the package was built from a DIRTY tree")
    if args.test_skip_freshness and mode == "launch":
        diagnostic.append("staleness gate skipped (test run)")
    if args.agent_cmd:
        diagnostic.append("test agent command (--agent-cmd), not claude")

    every = agent_findings + detector_findings
    counts = collections.Counter(f["severity"] for f in every)
    report = {
        "tool": "explore_playtest",
        "result": {EXIT_PASS: "PASS", EXIT_FAIL: "FAIL"}.get(exit_code, "INFRA"),
        "exit_code": exit_code, "exit_reason": exit_reason, "evidence": EVIDENCE,
        "diagnostic": diagnostic, "run_dir": run_dir, "started_at": started_at,
        "finished_at": datetime.datetime.now().isoformat(timespec="seconds"),
        "duration_s": round(time.time() - t0, 1), "minutes": args.minutes, "budget_s": budget_s,
        "wall_clock_s": wall_s, "focus": args.focus, "mode": mode,
        "frontend": bool(args.frontend and mode == "launch"), "attach_port": args.attach_port,
        "allow_stale": bool(args.allow_stale), "played": did_play,
        "build": _gate_record(fresh, skipped),
        "agent": {"command": cmd, "cwd": "temp dir outside the repo (removed)",
                  "exit_code": result["exit_code"],
                  "killed_by_wall_clock": result["killed_by_wall_clock"],
                  "interrupted": result["interrupted"], "error": result.get("error"),
                  "shutdown": result["shutdown"], "job": result["job"],
                  "duration_s": result["duration_s"], "cost_usd": stream["cost_usd"],
                  "num_turns": stream["num_turns"], "is_error": stream["is_error"],
                  "subtype": stream["subtype"], "result_text": stream["result_text"],
                  "last_text": stream["last_text"], "duration_ms": stream["duration_ms"],
                  "mcp_servers": stream["mcp_servers"],
                  "mordecai_tools_seen": stream["mordecai_tools_seen"],
                  "tool_calls": stream["tool_calls"],
                  "server_tool_calls": dict(sorted(server_calls.items())),
                  "stream_events": stream["events"],
                  "stream_malformed_lines": stream["malformed"],
                  "stderr_tail": _tail(stderr_path)},
        "findings": {"agent": agent_findings, "detector": detector_findings,
                     "counts": {s: counts[s] for s in SEVERITIES if counts[s]},
                     "new": sum(1 for f in agent_findings if not is_known(f)),
                     "known": sum(1 for f in agent_findings if is_known(f)),
                     "unreadable_lines": unreadable},
        "detectors": detector_status,
        "screenshots": shot_info,
        "files": {"prompt": prompt_path, "mcp_config": config_path, "agent stream": stream_path,
                  "agent stderr": stderr_path,
                  "findings": os.path.join(run_dir, FINDINGS_NAME),
                  "bridge transcript": [os.path.join(d, TRANSCRIPT_NAME) for d in
                                        game_dirs(run_dir)
                                        if os.path.isfile(os.path.join(d, TRANSCRIPT_NAME))],
                  "game.log": [os.path.join(d, "game.log") for d in game_dirs(run_dir)
                               if os.path.isfile(os.path.join(d, "game.log"))]},
    }
    json_path, md_path = write_reports(run_dir, report)

    # --- Step 5: summary ---
    print("", flush=True)
    print("=" * 60)
    print(f"EXPLORATORY PLAYTEST: {report['result']} (exit {exit_code})")
    print("=" * 60)
    print(f"  {exit_reason}")
    print(f"  Evidence: {EVIDENCE}")
    for reason in diagnostic:
        print(f"  DIAGNOSTIC: {reason}")
    print(f"  Agent: exit {result['exit_code']}, cost "
          + (f"${stream['cost_usd']:.4f}" if stream["cost_usd"] is not None else "n/a")
          + f", {sum(stream['tool_calls'].values())} tool call(s)")
    for f in every:
        print(f"  {f['id']} [{f['severity']}{', known' if is_known(f) else ''}] {f['title']}")
    print(f"  Report: {md_path}")
    print(f"  {AGENT_LINE}", flush=True)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
