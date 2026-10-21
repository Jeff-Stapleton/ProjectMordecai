Project Mordecai
===

Overview
----
This is a for of the Unreal Lyra Source code. We are adapting it as the starting point for Project Mordecai

Content Information
----
When downloading Lyra Source code from git, the content folders are not included.

To make use of the Lyra source code, you will need to download the content from the Unreal Marketplace through the Epic Games Launcher.

For more information please visit the [Unreal Engine Lyra documentation](https://docs.unrealengine.com/5.0/en-US/lyra-sample-game-in-unreal-engine/)

Once installed, copy the "Content" folder and any other folders named "Content" from the Marketplace project you downloaded to the project in your solution, and you should be good to go.

Project Docs & Agent Context
----
Everything agents and humans need lives in this repo (moved from `~/.openclaw` on 2026-10-19):

- `CLAUDE.md`: the agent rulebook (TDD pipeline, verification ladder, packaged playtest). Claude Code loads it automatically.
- `Specs/`: game design docs, `agent_rules_v2.md` (authoritative constraints, read first), `design_decisions_log.md`, `agent_playtest_plan_v1.md` (agent-playtesting phases and backlog).
- `stories/`: kanban story pipeline (`backlog/`, `in-progress/`, `done/`), `PLAN.md` (epics), `PLANNER.md` (planner instructions and the story template), `PLANNER_LOG.md`, `agent-prompts/` (nightly planner and worker prompts, currently not scheduled).
- `.claude/skills/`: Unreal skills Claude Code discovers automatically (`unreal-cpp`, `unreal-gas`, `unreal-python-editor`, `unreal-testing`, `unreal-tribal-knowledge`).
- `Scripts/check_editor.py`, `Scripts/ue_remote_exec.py`: editor readiness check and Python remote execution against a running editor.

Building, Testing & Playtesting
----
All tooling is local (Windows, UE 5.7 source build at `C:\Users\jeffd\Documents\Gamedev\UnrealEngine`); run it from the repo root. Python is `py` (3.12). There is no CI, BuildGraph, Horde, or Gauntlet pipeline.

One-time Python setup (the MCP SDK for `mordecai_play_mcp.py`, and Pillow): `py -m pip install -r Scripts/requirements.txt`. `agent_bridge.py` and `Scripts/playtests/*.py` otherwise need only the standard library.

**Done means a packaged playtest.** A story with player-facing behavior is done only when a playtest on a FRESH packaged build (built from the commit being delivered) shows the feature working, reached the way a player reaches it, and the screenshots have been looked at. Anything that needs player input (keys, movement, mouse, menus) is accepted only by a real-input playtest on that same build. Each tier is required; a lower tier never substitutes for a higher one:

1. Headless automation tests (logic): `Scripts\RunTests.bat`
2. PIE smoke tests (editor integration): `py Scripts/run_pie_verify.py`
3. Packaged playtest: `py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`, then review the screenshots
4. Real-input playtest: the story's `py Scripts/playtests/usXXX_<name>.py` on the same package, then review the screenshots. It drives the game through the in-game agent bridge (non-Shipping builds launched with `-AgentBridge`), which injects real key, mouse and gamepad events through Slate and Enhanced Input; agents can also play through the `mordecai-play` MCP server. Protocol: `Specs/agent_bridge_v1.md`.

Scripts:

- `Scripts\Build.bat` - builds the editor target (`LyraEditor Win64 Development`).
- `Scripts\RunTests.bat [Filter]` / `py Scripts/run_tests.py [Filter]` - runs headless automation tests via `UnrealEditor-Cmd -NullRHI`; the filter defaults to `Mordecai` (e.g. `Scripts\RunTests.bat Mordecai.GameMode`). Pass/fail is judged from the test log, because `UnrealEditor-Cmd` exits 0 even when tests fail. Exit 0 = all passed, 1 = a test failed, 2 = no tests matched / crash / queue never finished.
- `py Scripts/run_pie_verify.py [--no-launch] [--tests hud,spawn]` - launches the editor if needed, starts PIE and runs the `Scripts/verify_pie_*.py` smoke tests. These run against in-editor state and call gameplay functions directly, so they prove editor integration, not that a player can use the feature.
- `py Scripts/package_playtest.py [--clean] [--all-maps] [--config Development/DebugGame/Shipping] [--dry-run]` - fresh Development Win64 package (RunUAT BuildCookRun; iterative cook unless `--clean`; cooks `L_LyraFrontEnd` + `DevTestMap`, plus every other map with `--all-maps`) into `PackagedBuilds\Playtest\`. Writes `PackagedBuilds\Playtest\build_info.json` (commit, branch, dirty state) and logs to `Saved\Playtests\package-<timestamp>.log`. Commit first, then package. The first package can take an hour or more.
- `py Scripts/playtest_boot.py [--frontend] [--map <map>] [--expect-experience <name>] [--allow-stale]` - launches the packaged game (`PackagedBuilds\Playtest\Windows\ProjectMordecai\Binaries\Win64\LyraGame.exe`; the top-level `Windows\LyraGame.exe` is only a bootstrapper), refuses a STALE BUILD that does not match the current commit, waits for the expected experience (default `B_MordecaiArenaExperience` on `DevTestMap`; `--frontend` boots the real menu and expects `B_LyraFrontEnd_Experience`), takes screenshots and writes `Saved\Playtests\<timestamp>-boot\report.md`. The game saves `engine_NN.png` itself from its own viewport (`-PlaytestCapture`, non-Shipping builds), which works when the machine is locked. The desktop captures `shot_NN.png` are black in that case. Exit 0 = pass, 1 = gameplay failure (wrong experience, crash, timeout), 2 = infrastructure (missing exe, stale build, no window).
- `py Scripts/playtests/usXXX_<name>.py [--keep-running] [--allow-stale]` - a story's real-input playtest (Tier 4); the first is `Scripts/playtests/us079_inventory.py` (walk to the six DevTestMap pickups, press F, open the inventory with I, click Identify, press Escape). Refuses a STALE BUILD like `playtest_boot.py`, launches the packaged game with the agent bridge, plays it with real input, and writes `Saved\Playtests\<timestamp>-usXXX\report.md` with `bridge_NN.png` screenshots, `game.log` and `bridge.jsonl` (every request sent). Works on a locked machine. Exit 0 = pass, 1 = gameplay failure, 2 = infrastructure (stale build, launch failure, no connection, crash). `--allow-stale` runs are diagnostic only.
- `py Scripts/agent_bridge.py launch|ping|state|press|axis|mouse|click|click-button|walk-to|screenshot|wait|wait-until|quit` - Python client and CLI for the in-game agent bridge, for exploring the packaged game and writing playtests. `launch` starts the game detached (refusing a STALE BUILD) and later commands reuse its session; always finish with `quit`. Prints JSON; exit 0 = ok, 1 = gameplay failure, 2 = infrastructure.
- `Scripts/mordecai_play_mcp.py` - the `mordecai-play` MCP server (stdio), registered for this project in `.mcp.json`, so Claude Code sessions can play the packaged game (`launch_game`, `get_state`, `press_key`, `walk_to`, `click_button`, `screenshot`, `quit_game`, ...). Claude Code asks once to approve it. Needs the one-time `pip install` above.
- `py Scripts/run_python_tests.py [-k NAME] [-v]` - offline unit tests for the Python playtest tooling, against fakes of the game and bridge (`Scripts/tests/`); never launches the game or the editor. Exit 0 = all passed, 1 = a test failed, 2 = no tests found / import error.
- `py Scripts/check_content_committed.py [--base REF] [--json]` - fails if any `.uasset`/`.umap` is modified or untracked, or if asset-writing scripts under `Scripts/` changed on the branch (vs `origin/main`) with no asset changes. Must pass before a story is done.
- `Scripts\Package.bat [Config]` - distribution package into `PackagedBuilds\` plus a timestamped zip. Its `-allmaps` switch is not a UAT option and is silently ignored, so it cooks the `MapsToCook` list in `Config/DefaultGame.ini` (front end + DevTestMap), not every map. It does not write `build_info.json`, so it is not playtest evidence; use `package_playtest.py` for that.

Rules:

- Content is real only when it is saved and committed. Editor scripts that create assets must save them, and the resulting `.uasset`/`.umap` files (Git LFS) are committed in the same branch/PR as the code.
- No PIE-only paths: gameplay setup must not depend on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings` or other editor-only state. Every Mordecai map must load its Mordecai experience in a packaged build (if `playtest_boot.py` reports `B_LyraDefaultExperience`, it did not).
- Evidence: cite the playtest run dirs (`Saved\Playtests\<run>\report.md` plus screenshots, for the packaged and the real-input playtest) in the story's completion notes and the body of the closing commit made after the playtests. `Saved/` and `PackagedBuilds/` are gitignored. Report failures honestly.
- Real input only: a real-input playtest drives the game only with key, mouse and gamepad events through the bridge; nothing that calls game functions directly counts as evidence.


Additional Information
----
See the [Unreal Engine README](../../../README.md) at the root of the repository for [Licensing](../../../README.md#licensing) and [Contributing](../../../README.md#contributions) information.