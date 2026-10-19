# Agent Playtest Plan v1: agents play the packaged game

**Status:** Phase 1 DONE (PR #2, branch `feature/playtest-phase1`). Phase 2 NEXT. Phase 3 after that.
**Owner:** Jeff. **Last updated:** 2026-10-19.

## Problem

Months of stories were marked done, but almost none of it reached real gameplay. Agents verified features in the editor (headless tests and PIE) and never in the game a player runs. On 2026-10-18 the first packaged playtest found that **every** one of these passed in PIE and failed for players:

| Packaged game | Root cause | Fixed by |
|---|---|---|
| Loaded Lyra's shooter experience on DevTestMap | Arena experience was set only under `IsPlayInEditor()` | `389835b` |
| 0 weapons, 0 spells, no HUD, no pickup prompts, spell/pause keys unbound | C++ loads 16 assets by path string, so the cooker never packaged them | `17d19c9` (`DirectoriesToAlwaysCook=/MordecaiCore`) |
| Crash about 6s after load once spells were cooked | Spell data was written into ability CDOs (disregard-for-GC pool) | `e833b3e` |
| US-079 arena had nothing to pick up | Assets were never saved or committed | `8eda213` |
| No menu path to the arena | No playlist asset | `ec76bf3` |
| Failing tests reported as passing | `-TestExit` exits 0 on failure; PIE tests passed by default | `668cf31` |

**Principle:** a feature is done only when it is playable in a fresh packaged build of the delivered commit, reached the way a player reaches it. The rules are in `docs/CLAUDE.md` (Verification Ladder, Step 4c) and `docs/agent_rules_v2.md` §3.

## Verification ladder

| Tier | What | Tool | Status |
|---|---|---|---|
| 1 | Headless logic tests | `py Scripts/run_tests.py` | Done |
| 2 | PIE editor integration | `py Scripts/run_pie_verify.py` | Done (fixed false passes) |
| 3 | Packaged boot + observation | `py Scripts/package_playtest.py` → `py Scripts/playtest_boot.py` | **Done (Phase 1)** |
| 4 | Packaged + **real input** + state queries | Agent bridge + MCP | **Phase 2** |
| 5 | Per-story scripted playtests + exploratory play | Playtest scripts + agent | **Phase 3** |

A lower tier never substitutes for a higher one.

---

## Phase 1: make the build real (DONE)

Delivered on `feature/playtest-phase1` (PR #2, 8 commits):
- `package_playtest.py`: Development Win64 package into `PackagedBuilds/Playtest/`, iterative cook (first run 86 min, later runs 3-12 min). Writes `build_info.json` (commit plus dirty-tree fingerprint).
- `playtest_boot.py`:
  - Gates: refuses stale builds (STALE BUILD, exit 2).
  - Checks: the expected experience, crashes, timeouts, and any `/MordecaiCore` asset missing from the build (`content_cooked`).
  - Evidence: writes `Saved/Playtests/<run>/report.md`, `engine_NN.png` (captured by the game itself) and `shot_NN.png` (desktop).
- `UMordecaiPlaytestCaptureSubsystem` (non-Shipping, `-PlaytestCapture`): the game screenshots its own viewport, so capture works on a locked machine (desktop capture is black there).
- `check_content_committed.py`: content must be saved and committed.
- `run_tests.py`: pass/fail judged from the log.
- `create_arena_playlist.py` + `DA_Mordecai_Arena`: front-end path to the arena.
- Rules and skills rewritten: CLAUDE.md, agent_rules_v2, AGENT_RULES, PLANNER (+ story template with a Playtest section), cron prompts, unreal-testing / tribal-knowledge / python-editor skills.

**Phase 1 limit:** the agent can launch and look, but cannot act. Anything that needs a key press is recorded as `PENDING TIER 4 (real input)`.

---

## Phase 2: agent bridge, so agents can act in the packaged game (NEXT)

**Goal:** an agent playing the packaged build can press keys, move, read game state and take screenshots, through the same input path a player uses.

### 2.1 In-game bridge: `UMordecaiAgentBridgeSubsystem` (C++)
- `UGameInstanceSubsystem` in `Source/LyraGame/Mordecai/Playtest/`. Compiled out of Shipping. Created only with `-AgentBridge` (optional `-AgentBridgePort=47777`).
- **Transport:** localhost-only TCP (bind 127.0.0.1), newline-delimited JSON request/response. Use `FTcpListener` (Networking/Sockets modules; add to `LyraGame.Build.cs` if missing). Requests are queued and executed on the game thread from an `FTSTicker`.
- **Commands (v1):**
  - `ping` → build commit, map, experience, game time.
  - `state` → pawn location/rotation/velocity; HP, stamina, posture and SP (from the attribute set); owned gameplay tags; current weapon; spell slots and cooldowns; focused pickup and prompt text; inventory items and ledger; pause menu open/tab; visible Mordecai widgets with their text; actors within `radius` (class, label, location, distance, health if any). The `radius` query answers questions like "what are the red bars?".
  - `key {name, action: press|release|tap, hold_s}` → **real input**: inject through `UGameViewportClient::InputKey(FInputKeyEventArgs)` on the game viewport, so `PlayerInput` → Enhanced Input mapping contexts → abilities run exactly as for a keyboard. Fallback: `FSlateApplication::ProcessKeyDownEvent` after `SetAllUserFocusToGameViewport()`. Must work unfocused/locked (verify both).
  - `axis {name, value, hold_s}` for gamepad sticks (twin-stick aim/move).
  - `mouse {x, y}` / `click {button}`.
  - `screenshot {path}` (reuses the Phase 1 capture path).
  - `wait {seconds}` and `wait_until {condition, timeout_s}` (condition is a small predicate over `state`, e.g. `inventory.count>=4`).
  - `quit`.
- **Never** call gameplay functions directly as a stand-in for input. Gameplay-calling commands are allowed only as clearly labelled `debug_*` commands, and are never acceptance evidence.
- **TDD:** headless tests for JSON command parsing, key-name to `FKey` mapping, the `wait_until` predicate parser, and state serialization from stub data. The socket and injection are proven at Tier 4 by the acceptance playtest (2.4).

### 2.2 Python client: `Scripts/agent_bridge.py`
- `Bridge` class: `launch(map, ...)` (reuses `playtest_boot` launch, staleness gate and job-object cleanup; adds `-AgentBridge`), `connect()`, `state()`, `press(key)`, `hold(key, s)`, `walk_to(x, y, tol)`, `screenshot()`, `quit()`.
- `walk_to`: closed loop. Read the pawn position, choose WASD from the fixed diorama camera axes (the camera never rotates, per agent_rules §1; determine the W/A/S/D to world-axis mapping once from `control_bindings_v1.1.md` + the IMC and assert it), hold in short pulses until within tolerance, and give up after a timeout with a "stuck at (x, y)" report.
- CLI for agents: `py Scripts/agent_bridge.py state`, `... press F`, `... walk-to 400 0`.

### 2.3 MCP server: `Scripts/mordecai_play_mcp.py`
- stdio MCP server (Python `mcp` package; add to a `Scripts/requirements.txt`) exposing tools: `launch_game`, `get_state`, `press_key`, `hold_key`, `walk_to`, `screenshot` (returns the image), `wait_until`, `quit_game`.
- Register it in Claude Code's MCP config so any agent session can "play" interactively.

### 2.4 Phase 2 acceptance (must pass on a fresh package, on a locked machine)
`Scripts/playtests/us079_inventory.py`, a real-input version of `verify_pie_inventory.py`:
1. Launch DevTestMap. Assert the Mordecai experience and the HUD visible (`state`).
2. For each of the 6 pickups: `walk_to` it, assert the prompt reads `[F] Pick up <name>`, `press F`, assert the pickup is gone.
3. Assert 4 inventory items and 2 ledger resources.
4. `press I`. Assert the pause menu is open on the inventory tab with 4 rows. Screenshot.
5. Identify the amulet via the UI path (input), assert the identified name. Screenshot.
6. `press Escape`, assert the menu closes.

Report to `Saved/Playtests/<run>/` like Tier 3. Any step failing is a real bug to file.

### 2.5 Docs
Update the CLAUDE.md Step 4c (+ a new Tier 4 step), agent_rules_v2 §3 and the unreal-testing skill. The `PENDING TIER 4` label is then retired for stories with a Tier 4 playtest.

**Estimate:** 2-3 agent sessions (bridge + tests; client + walk_to; MCP + US-079 acceptance).

---

## Phase 3: per-story playtests and exploratory play

1. **Executable Playtest sections.** The story template's Playtest section becomes steps that map 1:1 to bridge calls. Each player-facing story ships `Scripts/playtests/usXXX_<name>.py`, run by `py Scripts/playtest_story.py US-XXX`.
2. **Playability-recovery epic.** Run Tier 3/4 against every story already marked done (US-001…US-079). Re-open or file bugs for anything not playable. Expected hits: melee/combat input paths, enemy AI and visibility (the unidentified red bars), status effects, skill tree and character sheet UIs.
3. **Exploratory agent.** An agent with the MCP tools free-plays for N minutes (move, fight, open menus, cast spells). It reports crashes, stuck states, errors in `game.log`, and screenshots that look wrong. It runs nightly after the planner.
4. **Pipeline wiring.** The worker runs Tier 1→4 for each story and cites run dirs in the closing commit. The planner tracks a "Playable status" line per feature.

---

## Known issues backlog (found by Phase 1, not yet fixed)
- Three floating red bars around the player: unidentified (enemy health bars on invisible enemies?). Phase 2 `state` will answer.
- Pickup name labels render rotated 90° (hard to read).
- Steam socket config deprecation warnings print on screen (`DefaultEngine.ini` SocketSubsystemSteamIP keys).
- Loading screen is Lyra-branded.
- Melee `AttackProfiles` CDO write in `AMordecaiPlayerController::GrantCombatAbilities`: same GC crash pattern as the spells; latent while `MeleeAttackProfiles` is empty.
- `BP_CameraMode_Diorama.uasset` references the missing class `/Script/MordecaiCoreRuntime.MordecaiCameraMode_Diorama` (cook warning). Stale asset, so delete it or re-parent it.
- 23 path-string content loads in Mordecai C++. They are safe now that all of `/MordecaiCore` is cooked, but better as `UPROPERTY` references.
- `Scripts/Package.bat` uses `-allmaps`, which isn't a UAT flag. It cooks only `MapsToCook`.
- Commit `52735cd`'s subject starts with a UTF-8 BOM (cosmetic).

## Open decisions (Jeff)
1. Worker timeouts (3600/4500s) vs packaging time (86 min first, 3-12 min after): raise them, or package in the background across cron runs?
2. Tier 3 for **every** story (current rule) or only player-facing ones?
3. HEADLESS stories may launch the editor for Tier 2: confirm.
4. `Package.bat`: switch to `-CookAll -CookMapsOnly`, or retire it?
5. ~~The cron prompts reference removed `stories/` folders~~ **Resolved 2026-10-19.** All Mordecai context moved from `~/.openclaw` into the repo: `CLAUDE.md`, `Specs/`, `stories/` (restored from git history), `.claude/skills/`, `Scripts/check_editor.py`, `Scripts/ue_remote_exec.py`. The superseded `AGENT_RULES.md` was not migrated. All four OpenClaw Mordecai cron jobs are disabled, so nothing is scheduled; the prompts are kept for reference in `stories/agent-prompts/`. Choosing a new scheduler is a later decision.
6. Phase 2 transport/port and whether the MCP server should be registered globally or per-project.
