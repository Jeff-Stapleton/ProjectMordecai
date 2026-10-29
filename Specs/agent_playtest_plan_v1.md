# Agent Playtest Plan v1: agents play the packaged game

**Status:** Phase 1 DONE (PR #2, branch `feature/playtest-phase1`). Phase 2 DONE on `feature/playtest-phase2`: acceptance 2.4 passed on a fresh package of `a40955b` on a locked machine (`Saved/Playtests/20261020-205856-us079`). Phase 3 IN PROGRESS on `feature/playtest-phase3` (kit, story runner, six feature scripts, exploratory agent; contract `Specs/story_playtests_v1.md`).
**Owner:** Jeff. **Last updated:** 2026-10-29 (T1 test maps, US-100, issue #46).

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

**Principle:** a feature is done only when it is playable in a fresh packaged build of the delivered commit, reached the way a player reaches it. The rules are in `CLAUDE.md` (Verification Ladder, Step 4c, Step 4d) and `Specs/agent_rules_v2.md` §3.

## Verification ladder

| Tier | What | Tool | Status |
|---|---|---|---|
| 1 | Headless logic tests | `py Scripts/run_tests.py` | Done |
| 2 | PIE editor integration | `py Scripts/run_pie_verify.py` | Done (fixed false passes) |
| 3 | Packaged boot + observation | `py Scripts/package_playtest.py` → `py Scripts/playtest_boot.py` | **Done (Phase 1)** |
| 4 | Packaged + **real input** + state queries | Agent bridge + MCP: `py Scripts/playtests/usXXX_<name>.py`, `py Scripts/agent_bridge.py`, MCP server `mordecai-play` | **Done (Phase 2)** |
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

**Phase 1 limit:** the agent can launch and look, but cannot act. Anything that needs a key press is recorded as `PENDING TIER 4 (real input)`. (Phase 2 retired that label; see 2.5.)

---

## Phase 2: agent bridge, so agents can act in the packaged game (DONE)

**Goal:** an agent playing the packaged build can press keys, move, read game state and take screenshots, through the same input path a player uses.

### Phase 2 delivered
Implemented on `feature/playtest-phase2`. Where the build differs from the plan in 2.1-2.4, `Specs/agent_bridge_v1.md` is the contract. The main differences: input goes in as Slate events (`FSlateApplication::Process*Event`), not `UGameViewportClient::InputKey`, and the viewport route is diagnostic only. The client picks a free port per launch. v1 has no `debug_*` commands.
- **Bridge (C++):** `UMordecaiAgentBridgeSubsystem` in `Source/LyraGame/Mordecai/Playtest/` (`MordecaiAgentBridgeSubsystem`, `...Server`, `...Protocol`, `...Predicate`, `...Input`, `...State`). It is created only in non-Shipping builds launched with `-AgentBridge`.
  - Transport: localhost-only TCP NDJSON, run on the game thread from an `FTSTicker`.
  - Commands: `ping`, `state`, `key`, `axis`, `mouse`, `click`, `screenshot`, `wait`, `wait_until`, `quit`. Every duration is real time.
  - It emulates a focused window, so it works on a locked or unfocused machine.
  - `LyraGame.Build.cs` gains `Sockets` and `Networking`. `AMordecaiPlayerController::GetCombatHUDWidget()` exposes the HUD to `state`.
  - Headless tests: `Mordecai.Playtest.AgentBridge.*` (`Source/LyraGame/Tests/Playtest/MordecaiAgentBridge{Protocol,Predicate,State}Tests.cpp`).
- **Protocol spec:** `Specs/agent_bridge_v1.md`.
- **Python client and CLI:** `Scripts/agent_bridge.py`. It provides:
  - `Bridge.launch()`, which reuses `playtest_boot`'s STALE gate, launch helper and kill-on-close job, then waits for `ping.ready`.
  - Closed-loop `walk_to`, `click_button`, `wait_until`, and a detached CLI session.
  - Exit codes 0/1/2.
- **MCP server:** `Scripts/mordecai_play_mcp.py` (`mordecai-play`, mcp 2.x SDK). It is registered project-scoped in `.mcp.json`. Dependencies are pinned in `Scripts/requirements.txt`.
- **Tier 4 playtest:** `Scripts/playtests/us079_inventory.py`, the 2.4 acceptance script.
- **Offline tests:** `Scripts/tests/fake_bridge.py` and `fake_game.py` (fakes of the bridge and the packaged game), the `Scripts/tests/test_*.py` suites, and the runner `py Scripts/run_python_tests.py`.
- **First real run:** `Saved/Playtests/20261020-144643-us079`. This was a diagnostic build (dirty, `--allow-stale`), so it is not acceptance evidence.
  - Result: 38 PASS, 1 WARN, 2 FAIL, 6 SKIP.
  - All six pickups passed with real input: `walk_to`, focus, prompt `[F] Pick up <name>`, then F to store the item.
  - FAIL 1: the inventory-tab check. The menu opened on the inventory tab, but the package reported the tab id as `Inventory` (FName case).
  - FAIL 2: Escape did not close the pause menu (game bug).
- **Acceptance (2.4): PASS** on a fresh package of `a40955b` (clean tree, Development), workstation locked (LogonUI running; desktop captures show the lock screen):
  - Tier 3: `Saved/Playtests/20261020-205821-boot` — PASS (B_MordecaiArenaExperience at 6.2 s, `content_cooked` PASS; `engine_01..03.png` show the arena, HUD, six pickups and three enemy bars).
  - Tier 4: `Saved/Playtests/20261020-205856-us079` — PASS, exit 0, every check PASS (Evidence: ACCEPTANCE CANDIDATE). All six pickups walked to and picked up with F, 4 items + 2 ledger resources, I opens the inventory tab with 4 rendered rows, a real click on Identify identifies the amulet (row reads "Mysterious Amulet"), Escape closes the menu. `bridge_01..04.png`: arena at start; inventory tab with "? Unidentified Purple Amulet" and its Identify button; the same tab with "Mysterious Amulet"; the arena after Escape.
  - Environment recorded by the bridge: Slate active, OS focus elsewhere (`app_has_focus=false`), in-memory cursor used for the click.
- **Docs (2.5):** done; see below.

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

**Done 2026-10-20.** The following files were updated:
- `CLAUDE.md`: new Step 4d, Definition of Done, Order, Step 6, What NOT To Do.
- `Specs/agent_rules_v2.md` §3: new `REAL-INPUT PLAYTEST — Tier 4 (Locked)` section.
- Skills: unreal-testing (Real-Input Playtest section), unreal-tribal-knowledge (new Tier 4 lessons), unreal-python-editor.
- `stories/PLANNER.md`: ladder and story template.
- `README.md`.

The rules now say:
- An AC that needs player input is accepted only by a passing Tier 4 run.
- A player-facing story with input-driven ACs ships `Scripts/playtests/usXXX_<name>.py`, which must exit 0 on the same fresh package before the story is done.
- `PENDING TIER 4 (real input)` is retired. The only remaining form is `PENDING TIER 4 (bridge cannot drive: <reason>)`, and an AC recorded that way is not claimed.

**Estimate:** 2-3 agent sessions (bridge + tests; client + walk_to; MCP + US-079 acceptance).

---

## Phase 3: per-story playtests and exploratory play

1. **Executable Playtest sections.** The story template's Playtest section becomes steps that map 1:1 to bridge calls. Each player-facing story ships `Scripts/playtests/usXXX_<name>.py`, run by `py Scripts/playtest_story.py US-XXX`.
2. **Playability-recovery epic.** Run Tier 3/4 against every story already marked done (US-001…US-079). Re-open or file bugs for anything not playable. Expected hits: melee/combat input paths, enemy AI and visibility (the unidentified red bars), status effects, skill tree and character sheet UIs.
3. **Exploratory agent.** An agent with the MCP tools free-plays for N minutes (move, fight, open menus, cast spells). It reports crashes, stuck states, errors in `game.log`, and screenshots that look wrong. It runs nightly after the planner.
4. **Pipeline wiring.** The worker runs Tier 1→4 for each story and cites run dirs in the closing commit. The planner tracks a "Playable status" line per feature.

### Phase 3 delivered (in progress, branch `feature/playtest-phase3`)
The contract is `Specs/story_playtests_v1.md`.
- **Kit:** `Scripts/playtest_kit.py`.
  - A `PlaytestRun` declares a `PLAN` of `Step`s, each naming the AC ids it proves and an optional `known_issue`.
  - Its step helpers map 1:1 to bridge calls.
  - The report records per-AC acceptance (only a launched, fresh, clean-tree run with exit 0 accepts) and marks every FAIL as NEW or known.
  - `us079_inventory.py` is rebased onto the kit.
- **Runner:** `Scripts/playtest_story.py`.
  - `US-XXX` runs Tier 3 and Tier 4 on the same package and writes a summary.
  - `--plan` and `--lint` keep the story's Playtest section executable.
  - `--all` is the recovery sweep: one Tier 3 boot per launch path, every Tier 4 script, and a per-feature Playable Status line for each feature.
  - `--list` shows the stories, their scripts and their latest runs.
- **Feature scripts (the recovery sweep):**
  - `us001_movement.py`: movement, camera, sprint, jump, facing, gamepad.
  - `us054_arena.py`: HUD, enemies, AI, melee, dodge, damage, death, respawn, reset, kill counter. Since US-100 (T1) it keeps only the HUD and enemy-AI checks on DevTestMap. Melee, dodge, damage, death, respawn, reset and the kill counter moved to `us054_combat.py` on CombatTestMap.
  - `us057_magic.py`: the four spells, SP, cooldowns, projectiles, status indicator.
  - `us069_pause_menu.py`: menu, tabs, tab contents, gamepad open and close.
  - `us078_weapons.py`: weapon cycling, weapon HUD, attacks.
  - `us079_inventory.py`: adds the empty state, filters, labels and loop time.
  - These cover US-001, 050-057, 062-069, 071, 077-079. The other done stories have no player input path and keep Tier 1 as their evidence.
- **Explorer:** `Scripts/explore_playtest.py` with `Scripts/explorer/charter.md`.
  - The `mordecai-play` MCP server gains `report_finding`, `session_info`, `--run-dir` and `--budget-s`.
  - The nightly step is `stories/agent-prompts/explorer-prompt.txt`. Nothing is scheduled yet.
- **Pipeline prompts:**
  - The worker runs `playtest_story.py US-XXX` and requires every input-driven AC to be accepted.
  - The planner reads the sweep and explorer reports.

---

## Known issues backlog (found by Phase 1 and Phase 2, not yet fixed)
- Three floating red bars around the player: unidentified (enemy health bars on invisible enemies?). **Answered by Phase 2** (see the Phase 2 list below): they are the health bars of three invisible Frontliner enemies.
- **Pickup name labels rendered rotated 90° (FIXED, issue #38).** Found by Phase 1 (`Saved/Playtests/20261018-223717-boot/engine_03.png`); also in the first Phase 3 sweep (`Saved/Playtests/20261022-184702-boot/engine_01.png`).
  - Cause: `AMordecaiItemPickup` gave its `NameLabel` (a `UTextRenderComponent`) a fixed relative rotation (45, 90, 0), commented "camera looks down from the south". The diorama camera has yaw 0: it looks along +X, pitched −50 (`MordecaiCameraMode_Diorama.cpp`). A TextRender's face is its local +X and its text runs along local −Y (engine `TextRenderComponent.cpp`), so the text ran along world +X: up the screen. Measured against that camera: facing 0.542, upright 0.000.
  - Fix: the label is a billboard. It uses absolute rotation (the actor's rotation never turns it), defaults to (50, 180, 0) (square to the default diorama camera), and every frame, in `TG_PostUpdateWork` after the camera update, `AMordecaiItemPickup::Tick` turns it to the local player's camera: X = the camera's backward vector, Z = its up (`ComputeNameLabelRotation`). It never ticks on a dedicated server.
  - Tier 1: `Mordecai.Items.ItemPickup.NameLabel*` (red before the fix). Tier 2: `verify_pie_inventory.py` phase 1 measures every label against the PIE camera. Tier 4: us079 `pickup_labels` requires `facing` and `upright` ≥ 0.99.
  - Verified on `92e7575` (fresh package): Tier 3 `Saved/Playtests/20261028-232530-boot` `engine_01.png`..`engine_03.png` show all six labels level, reading left to right (before: `Saved/Playtests/20261028-195434-boot/engine_02.png`). Tier 4 `pickup_labels` PASS, 6/6 at facing 1.000 and upright 1.000 (`Saved/Playtests/20261028-232550-us079`). The label text renders light gray and the far labels are faint on the light floor (the engine's default lit text material): readability beyond orientation, not filed yet.
- Steam socket config deprecation warnings print on screen (`DefaultEngine.ini` SocketSubsystemSteamIP keys).
- Loading screen is Lyra-branded.
- **Melee `AttackProfiles` CDO write in `AMordecaiPlayerController::GrantCombatAbilities` (FIXED, issue #40).** Same GC crash pattern as the spells; latent while `MeleeAttackProfiles` was empty.
  - Cause: the controller wrote its `MeleeAttackProfiles` into the native `UMordecaiGA_MeleeAttack` CDO, the disregard-for-GC fatal the spells hit (`e833b3e`). The write never reached the ability instance either: an instance of a native class copies only `Config` properties from its CDO.
  - Fixed: the controller stays the melee grant's SourceObject, and `UMordecaiGA_MeleeAttack::OnGiveAbility` -> `TakeAttackProfilesFromSpec` copies the profiles onto the instance, never the CDO. Tests `Mordecai.Combat.MeleeGrant.*` (5) and `Mordecai.Recovery.Combat.NoMeleeCDOWrite`. No Tier 4 check exercised it then: the game spawns the native `AMordecaiPlayerController`, whose `MeleeAttackProfiles` and `DefaultCombatAbilities` were empty (N10, fixed by US-083).
  - Verified on `9b53480`: Tier 1 682 passed / 0 failed (`Saved/TestLogs/tests-20261023-084222.log`); PIE 10/10. Tier 3 boot PASS on a fresh package (`Saved/Playtests/20261023-122030-boot`): B_MordecaiArenaExperience in 5.2 s, `content_cooked` PASS, no crash, no GC-verification error in `game.log`. Regression check with `us054_arena.py` on the same package (`20261023-122238-us054`, `20261023-122636-us054`): the same FAIL set as the base commit `32d3927` (`20261023-150847-us054`). The only check that differs between runs is `walk_to_frontliner3` (N14).
- `BP_CameraMode_Diorama.uasset` references the missing class `/Script/MordecaiCoreRuntime.MordecaiCameraMode_Diorama` (cook warning). Stale asset, so delete it or re-parent it.
- 23 path-string content loads in Mordecai C++. They are safe now that all of `/MordecaiCore` is cooked, but better as `UPROPERTY` references.
- `Scripts/Package.bat` uses `-allmaps`, which isn't a UAT flag. It cooks only `MapsToCook`.
- Commit `52735cd`'s subject starts with a UTF-8 BOM (cosmetic).

**Found by Phase 2** (by the bridge's `state` and the first Tier 4 run, `Saved/Playtests/20261020-144643-us079`; see also `Specs/agent_bridge_v1.md` §9). Each item is a game bug to fix or file. None of them may be worked around in a playtest.
- **The pause menu could not be closed with real input (FIXED in Phase 2).** After a real Escape the menu was still open 3 s later (check `menu_closes_escape`, `bridge_03.png`).
  - Cause: the widget uses CommonUI Menu input mode (`MordecaiPauseMenuWidget.cpp:38`), which blocks every unbound key, and the widget is not a back handler.
  - Fixed in Phase 2: `UMordecaiPauseMenuWidget` is a CommonUI back handler, so Escape closes the menu (tests `Mordecai.UI.PauseMenu.*`; real-input check `menu_closes_escape`). I still does not close it.
- **I does not close the pause menu either.** The Enhanced Input `Inventory` action never fires while the menu is open in Menu mode. It needs a CommonUI UI action binding.
- **Gamepad stick and L3 mappings use invalid FKey names.** `MordecaiHeroComponent.cpp:167,180,197` use `Gamepad_LeftStick2D`, `Gamepad_RightStick2D` and `Gamepad_LeftThumbstickButton`. The engine names are `Gamepad_Left2D`, `Gamepad_Right2D` and `Gamepad_LeftThumbstick`. Gamepad move, look and sprint are dead, silently.
- **Facing and aim are pinned to +X.** The diorama camera writes the control rotation every frame (`MordecaiCameraMode_Diorama.cpp:51`), so mouse and right-stick facing (`control_bindings_v1.1.md`) cannot work. Whether fixed facing is intended is Jeff's design call.
- **Sprint, jump, attacks, block and dodge are not bound at runtime (FIXED, US-083, issue #15).**
  - The only live mappings are the programmatic `MordecaiIMC_Runtime` (`MordecaiHeroComponent.cpp:76-209` before US-083). `IMC_Mordecai.uasset` is never added.
  - `AS_MordecaiCombat` is an empty ability set.
  - Sprint is mapped to LeftShift but never bound (`SetSprinting` has no caller).
  - Done stories (US-054, US-078) claim otherwise.
  - Fixed: `UMordecaiHeroComponent::BuildAndBindRuntimeInput` maps and binds them in `MordecaiIMC_Runtime` (triggers on the mappings, `EKeys::` names): LMB / RT released before `HeavyAttackHoldSeconds` (0.4 s, `TODO(DECISION)`) = light attack (Tap), held that long = heavy attack on release (HoldAndRelease), pressed while Block is triggered = parry (chord on Block + Pressed, bound on Triggered so only a fresh press parries, runtime `IA_Mordecai_Parry_Runtime`); RMB / LT held = block (Down); LeftAlt / Gamepad B = dodge (Pressed); Space / Gamepad A = `ACharacter::Jump` / `StopJumping`; LeftShift = sprint toggle. The combat bindings press `Mordecai.Input.*` tags on the Lyra ASC (`AbilityInputTagPressed` / `Released`, processed in `ALyraPlayerController::PostProcessInput`); the granted specs carry those tags. Block ends on release (`InputReleased`, never during a guard-break stagger), parry drops an active block and Block cannot activate while Parrying (AC-006.17/18), attacks are refused while Parrying or in a ParryWhiff, and no combat ability activates while `Mordecai.State.Dead`. `SetSprinting` rescales the current walk speed, so it composes with Block's speed multiplier. Tests `Mordecai.Recovery.Input.*` (3) and `Mordecai.Recovery.Combat.*` (8); PIE `Scripts/verify_pie_combat_input.py`. Block and parry on the gamepad (LT held, RT pressed while blocking) are played by `Scripts/playtests/us083_combat_input.py`; on the mouse (RMB hold, RMB+LMB) they stay `PENDING TIER 4 (bridge cannot drive: B2)` (AC-083.11).
  - Verified on `01abb8b`, on a fresh package. Sweep `Saved/Playtests/20261023-192402-sweep/summary.md`; story run `py Scripts/playtest_story.py US-083` -> `Saved/Playtests/20261023-193225-story-us083/summary.md`.
  - These real-input checks PASS on that package and FAILed in the `8ea853b` sweep:
    - `sprint_shift`: dx 747 cm in 1 s of W; after the second tap, S peaks at 600 cm/s, a walk. Run `20261023-192424-us001`, which also has `jump_space` (Falling at once).
    - `dodge_lalt`: run `20261023-192458-us054`.
    - `light_attack_lmb` and `heavy_attack_hold` (`Mordecai.Attack.Heavy`): run `20261023-192629-us078`.
  - `melee_lmb` was a false PASS in the `8ea853b` sweep: it accepted enemy health 100 < 200, and the player had no Attacking tag. Made honest by AC-083.9, it PASSes on `Mordecai.State.Attacking` (run `20261023-192458-us054`).
  - `Scripts/playtests/us083_combat_input.py`, gamepad half, passes all 10 player checks twice, exit 0 (`20261023-192719-us083`, `20261023-193244-us083`).
  - No check that PASSed in the `8ea853b` sweep FAILs.
  - Tier 1: 692 passed / 0 failed (`Saved/TestLogs/tests-20261023-183506.log`). PIE: 11/11.
  - On the fresh package of `ef69eb0`, the script with its keyboard/mouse half (`Saved/Playtests/20261024-110250-us083`) passed every input check: all 10 gamepad checks, plus LMB tap and hold, Left Alt and Space, including the first Left click after gamepad input. Its lane walk was body-blocked (N14), so it exited 1.
- **Spells have no cooldowns.**
  - No `DA_Spell_*` asset sets a `CooldownTag`, so `ApplySpellCooldown` returns early (`MordecaiGA_SpellBase.cpp:445`).
  - The HUD cooldown widgets are never bound (`UMordecaiSpellCooldownWidget::BindToCooldownTag` has no callers), so they always show "ready".
- **Weapon effects never reach the ASC.** `UMordecaiEquipmentComponent` looks for an ASC on the character (`MordecaiEquipmentComponent.cpp:55-56`), but the ASC lives on the PlayerState. Weapon `GrantedTags` and `StatModifiers` are never applied.
- **The three red bars are explained.** They are the health bars of three `BP_MordecaiEnemy_Frontliner` enemies that have no mesh, so the enemies are invisible. **FIXED** (issue #19, US-085, `f21525f`): each Frontliner has a dark red cylinder body the size of its capsule under its bar. `AMordecaiEnemyCharacter::BodyMesh` is a non-colliding slot; the Blueprint sets its mesh. On a fresh package:
  - Tier 3 `Saved/Playtests/20261028-085310-boot`: three red bodies under the three bars in `engine_01..03.png`, and 0 errors in `game.log` (before: three "no visible mesh").
  - Tier 4 `enemies_visible` PASS, 3/3 `mesh=True`, in `Saved/Playtests/20261028-085332-us054` and the sweep's `20261028-090901-us054`.
- **The inventory filter bar overlaps the "Stored Resources" header.** The category buttons (Materials, Quest) draw over the ledger heading (`Saved/Playtests/20261020-144643-us079/bridge_02.png`).
- **On-screen debug text is shown to players.** "MORDECAI: Programmatic IMC created and input bound successfully (including 4 spell actions)." appears in yellow under the HUD bars (`bridge_02.png`; message from `MordecaiHeroComponent.cpp:402`).
- **The CommonUI pool left inventory rows stale on a second menu open.** Confirmed by the first Phase 3 sweep (below). **FIXED in code** (issue #28, US-094, `a8aaa51`).
  - CommonUI pools the pause-menu widget. On close, the menu's Slate tree is released one core tick later, and the content's `NativeDestruct` unbinds the inventory tab.
  - A reopen reuses the same instance. `NativeOnInitialized` does not run again, and `RebuildContentArea` short-circuits on the unchanged content class, so the tab stayed unbound: stale rows, '--' after a filter click, and Identify silently failing.
  - us079 opens the menu twice (`empty_state`, then `menu_opens_inventory`), so it hits this on the second open.
  - Fix: `UMordecaiPauseMenuWidget::NativeOnActivated` rebinds the active tab to the current pawn on every activation, and `NativeOnDeactivated` unbinds it.
  - Tier 1: `Mordecai.Recovery.PauseMenu.RebindsContentOnReactivate`, `Mordecai.Recovery.Inventory.*` (red before the fix).
  - Tier 2: `verify_pie_inventory.py` reopens the pooled menu across `PIE_NEXT_PHASE` runs (red on the unfixed binary).
  - Tier 4 on a fresh package of `a8aaa51`: every pooled reopen drew exactly what the player held.
    - `menu_ledger_rows` PASS, with "Iron Ore  x1" and "Town Stone  x1" (`Saved/Playtests/20261028-195608-us079`).
    - `filter_armor_empty` PASS, and `menu_rows` drew ['Health Potion'] (`20261028-195731-us079`).
  - The swarm killed the player on the pickup route in all three runs, so the checks that need all 4 items had not run yet. Only on the `92e7575` package did they run: `Saved/Playtests/20261028-232550-us079` exited 0, because the player outran the swarm.
  - T1 does not change this. Jeff decided on 2026-10-29 that us079 and its six pickups stay on DevTestMap (`Specs/design_decisions_log.md`), so the inventory route still races the swarm. US-094 stays in progress until Jeff's call.

**Found by the Phase 3 source audit** (2026-10-22, read-only, while planning the per-story playtests; `Specs/story_playtests_v1.md`). The playtests cite these by id in `known_issue`. N7, N11 and N12 are suspected: the sweep confirms or clears them.
- **N1 The kill counter can never go up.** `UMordecaiKillCounterWidget` waits for `Mordecai.Event.EnemyKill` tag-count changes on the player ASC (`MordecaiKillCounterWidget.cpp:68`), but nothing ever grants that tag. `AMordecaiGameMode` counts kills elsewhere (`MordecaiGameMode.cpp:115-118`). Melee is bound since US-083, but no playtest kills an enemy yet (the kill path is US-086's), and at most 3 Fireballs (75 damage) fit the SP budget.
- **N2 The death screen is never created.** Nothing references `UMordecaiDeathScreenWidget` outside its own files.
- **N3 The weapon HUD draws nothing.** `UMordecaiEquippedWeaponWidget` is a native widget with no widget tree (`MordecaiCharacter.cpp:63,197-201`).
- **N4 The Character tab of the pause menu is blank.** `UMordecaiCharacterSheetWidget` builds no widget tree (`MordecaiCharacterSheetWidget.cpp:36-40`).
- **N5 The Skills and Feats tabs show "Coming Soon".** `MordecaiPauseMenuWidget.cpp:60-69` registers only Character and Inventory.
- **N6 The spell budget runs out (design question).** `MaxSpellPoints` is 10 (`MordecaiAttributeSet.cpp:14-15`), the loadout costs 12 (Fireball 3, Blink 2, Stone Skin 4, Restoration 3), nothing regenerates SP, and respawn does not restore it (`MordecaiCharacter.cpp:357-359`).
- **N7 (suspected) Enemies never hurt the player.** In run `20261020-205856-us079` the player spent about 60 s within aggro range at full health. Suspected causes: empty enemy attack profiles (`MordecaiGA_MeleeAttack.cpp:292-296`) or no navmesh movement. `us054_arena.py` checks `enemy_approaches` and `enemy_damages_player` decide.
- **N8 The HUD has 2 spell cooldown widgets, not 4.** See the default layout, `MordecaiCombatHUDWidget.cpp:81-92`.
- **N9 Previous weapon is BackSlash only.** Shift+Tab is not mapped, and Tab still fires Next (`MordecaiHeroComponent.cpp:227-232`). AC-078.7 specifies Shift+Tab.
- **N10 The player's combat abilities exist only on an unsaved CDO edit (FIXED, US-083, issue #15).** `Scripts/us054_10_configure_all.py:15-42` set `DefaultCombatAbilities` on the C++ CDO and never saved it (`MordecaiPlayerController.cpp:136-147`; both citations before US-083). Even in the editor session that ran it, the edit never reached a spawned controller: an instance of a native class copies only `Config` properties from its CDO (unreal-tribal-knowledge, issue #40). This is the root of "attacks are not bound".
  - Fixed: the `AMordecaiPlayerController` constructor sets `DefaultCombatAbilities` (native `StaticClass()` refs: `UMordecaiGA_MeleeAttack`, the new `UMordecaiGA_HeavyAttack`, Dodge, Block, Parry) and `CombatAbilityInputTags`; `GrantCombatAbilities` adds each spec's input tag. The heavy attack is the melee ability with `AttackSlot` Heavy: it swings the equipped weapon's `GetActiveHeavyAttackProfile()` (no unarmed heavy, `TODO(DECISION)`) and owns `Mordecai.Attack.Heavy`. The script no longer writes the controller CDO. Test `Mordecai.Recovery.Combat.AbilitiesGrantedFromNativeDefaults`.
  - Verified on `01abb8b` and `ef69eb0`: in the packaged game the player's light attack, heavy attack, dodge, block and parry activate on real input (the checks under "not bound" above). Tier 3 boot PASS on both packages (`Saved/Playtests/20261023-192402-boot`, `20261023-193225-boot`, `20261024-110226-boot`): B_MordecaiArenaExperience, `content_cooked` PASS, no crash, 0 errors in `game.log`.
- **N11 (suspected) Projectiles are destroyed by pickup trigger spheres.** Projectiles overlap WorldDynamic and die on the first overlap (`MordecaiProjectile.cpp:32-33,145-177`).
- **N12 (suspected) The facing arrow is hidden in game.** `UArrowComponent` is hidden in game by default (`MordecaiCharacter.cpp:66-70`), so AC-2.1.5's arrow may not be visible.

**Found by the first Phase 3 sweep** (2026-10-22, `py Scripts/playtest_story.py --all` on a fresh package of `8ea853b`, `Saved/Playtests/20261022-184702-sweep/summary.md`). Each item is a NEW failure on real input, confirmed in the screenshots.
- **N7 confirmed: enemies swarm the player but never hurt them.** **FIXED** (issue #12, US-084, `c7e4b8d`): on a fresh package `enemy_damages_player`, `player_dies` and `respawn` PASS (`Saved/Playtests/20261027-085212-us054`), and the Tier 3 boot shows a standing player killed (`Saved/Playtests/20261027-085120-boot`, `engine_02.png`).
  - From the first read, all three Frontliners sit next to the player at the spawn: around (−121, ±151) and (200, 0), `ai_state` Attack.
  - The player stays at 100/100 health for 20 s (`us054_arena.py` check `enemy_damages_player`).
  - The death, death-screen, respawn and arena-reset checks therefore cannot run.
  - Root cause (issue #12, US-084): not the attack profiles (the cooked Frontliners have them). The Mordecai experiences create no Lyra teams, so the player and every enemy have `FGenericTeamId::NoTeam`, and `UMordecaiHitDetectionSubsystem::AreActorsFriendly` compared the ids and filtered the player out of every enemy hit (and the enemies out of the player's). The `Mordecai.Team.Enemy` tag now decides whenever an enemy is involved. On a fresh package of `6c858f5`, `enemy_damages_player` PASSes (`Saved/Playtests/20261027-055456-us054`); that exposed N31.
  - Second cause, found on the `48f60b2` package in an exploratory session (`Saved/Playtests/20261027-080939-mcp`): standing at the spawn for 60 s, the player stayed at 100/100 with all three Frontliners in Attack and Rooted at ~195 cm. Their slash (`DA_EnemyAttack_BasicSlash` / `FireSlash`) reaches 150 cm (arc radius, centre to centre), but the AI enters Attack at `AttackRange` 200 (AC-054.2), so an enemy that stops between 150 and 200 cm swings at air forever (the sweep's "(−121, ±151) and (200, 0)" is that geometry). Walking to 80 cm of one got hit within 1.6 s. Fix: the AI's effective attack range is `min(AttackRange, the melee profile's reach)` (`AMordecaiEnemyAIController::GetEffectiveAttackRange`, `UMordecaiGA_MeleeAttack::GetAttackReach`).
- **N13 Enemies have 100 MaxHealth, not 200.** AC-054.2 specifies 200, but the cooked enemies report 100/100 (check `enemies_present`). The first guess here was that the BP edit was not saved, as with N10. That was wrong. **FIXED** (issue #18, US-085, `39500e2`): on a fresh package all three Frontliners read 200.0/200.0 (`enemies_present` detail in `Saved/Playtests/20261027-202640-us054` and the sweep's `Saved/Playtests/20261027-202849-us054`; `c7e4b8d` read 100.0/100.0). `enemies_present` still FAILs, but only on its spacing clause: the enemies have swarmed the spawn by the first read (226–261 cm apart; placement, US-085 TODO(DECISION) / T1).
  - Root cause (issue #18, US-085): the BP was fine. `BP_MordecaiEnemy_Frontliner` has saved `DefaultMaxHealth` 200 since US-054 (`f7fc80d`). But DevTestMap saved `DefaultMaxHealth` 100, the native default, on each of the three placed Frontliners: an instance override, in every committed version of the map since `f7fc80d`. `InitializeAbilitySystem` reads the instance's value, so the enemies spawned at 100/100.
  - Why nothing caught it: the stats are `EditDefaultsOnly`, so the Details panel hides them on placed actors, Python refuses to write them there ("cannot be edited on instances"), and the US-054 scripts only read the CDO.
  - How it happened: the placed actors never received either value `Scripts/us054_02_create_enemy_bp.py` wrote on the CDO. In `f7fc80d` DevTestMap also saved the native `AIControllerClass` (`AIController`) on the three Frontliners, next to `DefaultMaxHealth` 100. US-054 repaired the AI controller on the instances (`3f4d1b5`), which works because that property is EditAnywhere. `DefaultMaxHealth` could not be repaired that way and stayed. The values the later scripts wrote (`DefaultAbilities`, the health bar class, `EnemyAttackProfiles`) are not serialized on the instances. So do not trust a CDO write to reach actors that are already placed: read the instances.
  - Fix: `Scripts/us085_configure_frontliner.py` sets the CDO to the stale value and back. Python writes to a template also write every loaded instance that holds the template's value, so the three actors follow it back to 200, equal their archetype, and the saved DevTestMap no longer has `DefaultMaxHealth` in its name table. Guard: `ValidateContent` logs a `LogMordecaiEnemy` error when an enemy's `DefaultMaxHealth`, `DefaultMaxPosture` or `DefaultMoveSpeed` differs from its class's (Tier 1 `Mordecai.Recovery.Enemy.LogsInstanceStatOverride`). Tier 2: `Scripts/verify_pie_enemies.py`.
  - Verified on `39500e2` (fresh package, clean tree):
    - Tier 1: 703/0, incl. the new guard test, which was red before the guard.
    - Tier 2: PIE smoke 12/12; `verify_pie_enemies.py` 10/10, which was 4/10 (6 failed) before the map fix.
    - Tier 3: boot PASS (`Saved/Playtests/20261027-202616-boot`). `game.log` has no "overrides Default…" error, only the three "no visible mesh" errors.
    - Tier 4: see above.
    - Sweep `Saved/Playtests/20261027-202756-sweep`: same statuses as the `c7e4b8d` sweep in us054/us057/us069/us078/us079/us083. us001 varies with where the enemies stand (N14): `move_d` FAIL (blocked at 55 health), `walk_lane_reset` and `sprint_shift` PASS.
- **N14 The invisible enemies body-block the player.** They crowd the spawn and follow the player, so the player meets an invisible wall:
  - A 0.7 s hold of A moved the player only 93 cm (`us001_movement.py` `move_a`).
  - `walk_to(0, -650)` got stuck at (−78, −45) (`walk_lane_wp1`).
  - `us054_arena.py` `walk_to_frontliner3` fails intermittently on its third leg, where the player walks into the enemies (2026-10-23). Base commit `32d3927`: FAIL, then PASS (`Saved/Playtests/20261023-150847-us054`, `20261023-150936-us054`). The step carries no `known_issue`, so it reports NEW. When it fails, `melee_lmb` and `dodge_lalt` run without the player next to the enemy.
  - It is intermittent: whether a walk passes depends on where the enemies stand.
    - In the US-083 sweep (`01abb8b`, `Saved/Playtests/20261023-192402-sweep`), `move_a` (205 cm), `walk_lane_wp1` and `walk_to_frontliner3` all PASSed.
    - On the `ef69eb0` package, `us083_combat_input.py` lost its lane walk and sprint to it twice:
      - `Saved/Playtests/20261024-110250-us083`: after two backward dodges at the spawn, the player was boxed in at (-39, 0). S moved it 0 cm, A and D about 2 cm, and `walk_lane_wp1` timed out.
      - `20261024-115717-us083`: the walk passed, but `sprint_left_shift` covered 372 cm in 1 s of W and ended 21 cm up (z 92 -> 113: onto an enemy capsule). A state read afterwards had all three Frontliners 194-199 cm from the player, one on the lane line ahead.
      - On the `5cdadd5` package, the walk came first, from the spawn before any dodge, and the player was still stuck at once, at (-8, -7) (`Saved/Playtests/20261024-133951-us083`).
  - How the enemies move: they follow at 400 cm/s (a walk is 600) and stop at their 200 cm AttackRange, so a straight walk leaves them behind, but a backward dodge or a turn puts them in the way. Since `5cdadd5` the us083 script walks and sprints first, from the spawn, with a run-up along the lane. That was not enough: the enemies close in from the very start.
  - The proper fix is a test map without enemies (T1).
  - **Invisible part FIXED** (issue #19, US-085, `f21525f`): the enemies have visible bodies (see "The three red bars are explained"). The bodies never collide, and a dead enemy's body is hidden with its collision, so what the player sees is what blocks it.
  - **Decision (Jeff, 2026-10-28):** issue #19 is fixed by the mesh only. The Frontliners keep their placement inside AggroRange of PlayerStart; the swarm at the spawn, and the body-blocking and deaths it causes, are T1's to solve (issue #46). This settles US-085's placement TODO(DECISION).
  - On `f21525f` the two named us001 checks still depend on the swarm:
    - Story run `Saved/Playtests/20261028-085852-us001`: `move_a` PASS (207 cm). `walk_lane_wp1` FAIL: the player was killed at (152, -109), not stopped by an invisible wall.
    - Sweep run `20261028-090829-us001`: `walk_lane_wp1` PASS. `move_a` FAIL only on drift: 385 cm along -Y (an enemy shove; a walk covers about 240 cm), 37 cm across (limit 30).
  - **T1 in progress** (US-100, issue #46, 2026-10-29; evidence to come). The scripts that do not test the enemy AI leave DevTestMap:
    - us001, us083 and us069 run on `/MordecaiCore/Game/Maps/MovementTestMap`, which has no enemies. It keeps DevTestMap's frame (PlayerStart (0, 0), lane L1 at y = -800), so their waypoints and check names are unchanged.
    - The melee/dodge/death chain of us054, and us057 and us078, run on `/MordecaiCore/Game/Maps/CombatTestMap`: a TestDummy that never moves or attacks, and a TestAttacker that holds its spot more than 900 cm from every scripted spot except its fight spot.
    - DevTestMap keeps its Frontliners for the enemy-AI checks (`us054_arena.py`) and for us079.
    - This entry is closed only by a fresh-package sweep in which `move_a` and `walk_lane_wp1` PASS on MovementTestMap.
- **N15 Fireball spends SP but does nothing visible.**
  - SP drops 10 → 7.
  - No projectile appears in `actors` within 1.5 s.
  - The enemy straight ahead (+X, about 1 m) keeps 100 health (`us057_magic.py` checks `fireball_projectile` and `fireball_hits_enemy`; `bridge_01.png`).
  - It may be related to N11.
- **N16 The status-effect indicator shows "Status", not the status name.** Stone Skin's indicator reads "Status 10.0" bottom-left (`stone_skin_indicator`, `bridge_03.png`).
- **N17 There are no enemy indicators.** No `EnemyIndicator` widget is on screen with three enemies within aggro range (`us054_arena.py` `enemy_indicators`; US-064).
- **N18 LB/RB do not switch pause-menu tabs.** AC-069.4 specifies them (`us069_pause_menu.py` `shoulder_switches_tab`). Needs triage: possibly missing by design (no CommonUI tab-list action).
- **The CommonUI pool bug is CONFIRMED** (it was latent). **FIXED in code** (issue #28, US-094, `a8aaa51`); see "The CommonUI pool left inventory rows stale" above.
  - Open the inventory before any pickup ("Inventory is empty.", correct), close it, then pick up all six items: the second open still shows "Inventory is empty." and "No resources stored." (`us079_inventory.py` checks `menu_rows`, `menu_ledger_rows`, `identify_button`, `filter_*`; `bridge_03.png`).
  - A player who opens the inventory twice sees a stale, empty list.
- **A false PASS in that sweep, now fixed.** `us054_arena.py` `melee_lmb` passed only because it compared enemy health (100, N13) against the AC's 200. It now compares against the nearest enemy's health before the click, so a sweep before US-083 would show it FAIL (attacks were not bound). Since US-083 `melee_lmb` and `dodge_lalt` carry no `known_issue` and are expected to PASS: a FAIL there is NEW.
- **More on-screen debug text.** "MORDECAI: Weapon loadout initialized — 5 weapons available" and "MORDECAI: SetupPlayerInputComponent called…" appear alongside the IMC message and the Steam warnings.

**Found by the first exploratory-agent run** (2026-10-22, `py Scripts/explore_playtest.py --minutes 10` on the same fresh package of `8ea853b`, `Saved/Playtests/20261022-185207-explore/report.md`; 7.8 min, $3.07, 210 tool calls). The run independently re-found N7, N15, N14 and the stale inventory. The latter goes further: after several reopens the tab shows "--" placeholders, so the widget is unbound (`bridge_14.png`). New:
- **N19 (state evidence) The player's invulnerability sticks after Blink.** `Mordecai.State.Invulnerable` stayed on the player after Blink ended (Casting tag gone, `spells[1].active` false) and was still there 6+ s later. This is an i-frame tag that is never removed.
- **N20 Blink can spend SP and not move the player.** A Blink with an invisible enemy about 90 cm ahead spent 2 SP and moved 0 cm, with no feedback (`bridge_04.png`).
- **N21 Walls hide the player.** At the south-west corner the near arena wall hides everything but the top of the player's head, and the diorama camera has no cutaway or fade (`bridge_11.png`).
- **N22 (design question) Items have no actions.** Picked-up items have no Use or Equip action: the Health Potion cannot be used, and the Rusty Sword cannot be equipped or added to the weapon cycle.
- **The Settings tab also shows "Coming Soon"** (an extension of N5).

**Found while fixing US-083** (2026-10-23, issue #15: source research and the design review of binding the combat input).
- **N23 Left Shift crouched the player (FIXED, US-083).** `InputData_Mordecai` (the arena PawnData's `InputConfig`) tags `IA_Mordecai_Sprint` with `InputTag.Crouch` (`NativeInputActions[4]`), and `ULyraHeroComponent::InitializePlayerInput` binds that tag to `Input_Crouch` -> `ToggleCrouch` on Triggered, every frame the key is held. Every Left Shift press toggled crouch: walk speed capped at 300 cm/s and jumping refused while `bWantsToCrouch` (the `Status.Crouching` in the sweep's us001 run after the second LeftShift tap and in us078 after Shift+Tab). The sweep's `sprint_shift` and `jump_space` failures were the missing bindings, but the crouch would have broken both once they were bound. Fixed in C++: `UMordecaiHeroComponent::InitializePlayerInput` removes that binding after Lyra makes it (`RemoveNativeBindingsForTag`, test `Mordecai.Recovery.Input.LyraCrouchBindingRemoved`). The content entry itself is still wrong; deleting it is an EDITOR change. Verified on `01abb8b`: no state read in the sweep's Tier 4 runs (`Saved/Playtests/20261023-192402-sweep`) contains `Status.Crouching`. The `8ea853b` sweep had 12 such reads in us001 and 31 in us078.
- **N24 Light attacks never combo.** `UMordecaiGA_MeleeAttack::NotifyComboInput` has no caller, so a second click during Recovery does nothing (AC-004.9). Wiring it (an `InputPressed` override, which the Lyra input path now reaches) also needs a guard first: `GetActiveProfile` re-resolves the equipped weapon on every call, so a weapon switched mid-combo can leave `CurrentComboIndex` out of range; `OnWindupComplete` then sets a 0 s timer (which clears it) and the attack never ends (`State.Attacking` forever).
- **N25 Attack cancel windows are not wired.** `CanCancelIntoDodge` / `CanCancelIntoBlock` are never called: a dodge or block during an attack runs alongside it, and the attack can still land its hit (`MordecaiDodgeTests.cpp` says "the controller cancels the attack first"; nothing does). Seen on real input on `01abb8b`: in `dodge_lalt` (us054, `Saved/Playtests/20261023-192458-us054`), the Left Alt tap during the light attack left `Mordecai.State.Attacking` and `Mordecai.State.Dodging` on the player together.
- **N26 Spells ignore death and the parry whiff.** Spells activate through `TryActivateAbilityByClass` (`UMordecaiHeroComponent::ActivateSpellBySlot`), and `UMordecaiGA_SpellBase` is not blocked by `State.Dead` or `State.ParryWhiff` (AC-006.14 says no abilities during the whiff). The combat abilities are, since US-083.
- **N27 A dodge travels about 110 cm, not 400 (measured).** `UMordecaiGA_Dodge::ApplyDodgeMovement` launches the pawn at 1000 cm/s with no Z (`LaunchCharacter`): it lands within a few frames, then walking braking stops it. The design review while fixing US-083 estimated about 120 cm, against `DodgeDistanceCm` 400.
  - Measured on `01abb8b` (`dodge_lalt`, us054, `Saved/Playtests/20261023-192458-us054/bridge.jsonl`), with the light attack still running (N25):
    - the first read after the Left Alt tap: 98 cm, still moving at 358 cm/s (the displacement the check reports);
    - the next read: the player at rest about 111 cm from the start.
  - The check passes on the Dodging tag, so a dodge that is too short does not fail it.
  - From US-100 (T1) on, `us083_combat_input.py` records each dodge's displacement as a measurement on MovementTestMap, with no enemy in the way. The script records it without FAILing, so N27 does not block us083's exit 0. The measurements come with US-100's sweep (evidence to come).
- **N28 Sprint is client-local.** `AMordecaiCharacter::SetSprinting` writes `MaxWalkSpeed` on the machine that handled the input (no RPC, nothing replicated), so a remote client in networked play would be corrected on every sprinting move (and while blocking, Block's speed change runs on both sides). Standalone and a listen-server host are fine. `TODO(DECISION)` at `SetSprinting`: a CharacterMovementComponent saved-move flag read in `GetMaxSpeed`, with Block's slow-down moved there too.
- **N29 Unarmed attacks have no profiles in the game (latent).** With nothing equipped, the light attack reads `UMordecaiEquipmentComponent::UnarmedAttackProfiles` and the heavy attack gets no profile from `GetActiveHeavyAttackProfile()` (`MordecaiGA_MeleeAttack.cpp` `GetResolvedAttackProfiles`).
  - `UnarmedAttackProfiles` is never set: it has no native default, no asset sets it, and there is no `DA_AP_Unarmed_*`. The ability's own fallback `AttackProfiles` is empty too. So an unarmed player's light and heavy attacks both end at once.
  - Unreachable in the arena today: the weapon loadout always equips one of its 5 weapons, and nothing unequips the main hand.
  - The Tier 1 test `Mordecai.Recovery.Combat.MeleeUsesEquippedWeaponProfiles` injects a test profile. `Mordecai.Weapon.Blades.EquipSetsActiveProfiles` asserts the empty default.
  - `TODO(DECISION)`: unarmed content DataAssets (an EDITOR story) vs the code-built `UMordecaiWeaponProfileFactory::CreateUnarmed` (3 punches + a slam heavy, spec-tested by `Mordecai.Weapon.Unarmed.ProfilesMatchSpec`).
  - Split off US-083's AC-083.2 at Jeff's call (2026-10-24).
- **N30 The player's stamina never regenerates.** Nothing in game code calls `UMordecaiStaminaSystem`'s regen (`ComputeRegenAmount`, `IsRegenAllowed`; AC-008.10/11), and nothing calls its sprint drain (AC-008.14/15). Only enemy posture regenerates (`MordecaiEnemyCharacter.cpp`).
  - Combat now spends stamina (US-083 bound the input). In `Saved/Playtests/20261024-115717-us083/bridge.jsonl` it went 100 -> 80 -> 55 -> 30 -> 22 -> -3 -> -18 -> -26 -> -50 in about 15 s of blocks, parries, attacks and dodges.
  - It then stayed at -50 (the AC-008.1 floor) for the rest of the run: 53 reads over 10 s.
  - The HUD's stamina bar shows it empty (grey) from then on (`bridge_01.png`, `bridge_02.png` of that run).
  - A player who fights for 15 s is exhausted for good.
- **N31 The player never dies: Health sits at 0 (found by the issue #12 / US-084 playtest).** Hidden behind N7 until enemy hits landed. **FIXED** (`48f60b2`): `player_dies` and `respawn` PASS on fresh packages of `48f60b2` (`Saved/Playtests/20261027-080244-us054`) and `c7e4b8d` (`Saved/Playtests/20261027-085212-us054`).
  - On a fresh package of `6c858f5` (`Saved/Playtests/20261027-055456-us054`), `enemy_damages_player` PASSed and the player's Mordecai Health read 0.0/100 for 169 state reads, but `player_dies` FAILed: `is_dead` never true, no `Mordecai.State.Dead`, movement never stopped.
  - Root cause: `AMordecaiCharacter` bound its Health→death delegate only in `BeginPlay` and `PossessedBy`. For the local player the PlayerState ASC is attached to the pawn later, by `ULyraHeroComponent` from `SetupPlayerInputComponent` (it waits for the InputComponent), so `GetAbilitySystemComponent()` was null both times and the bind silently did nothing. Latent since US-053 (`0e29035`); its tests call `HandlePlayerDeath()` directly on a character without an ASC.
  - Fix (issue #12 branch): bind from `ALyraCharacter::OnAbilitySystemInitialized`, unbind from `OnAbilitySystemUninitialized`; death decided on authority. Tier 1: `Mordecai.Recovery.Player.*` (3).

**Found by the issue #12 sweep** (2026-10-27, `py Scripts/playtest_story.py --all` on a fresh package of `c7e4b8d`, `Saved/Playtests/20261027-085120-sweep/summary.md`; every NEW failure triaged against the `01abb8b` sweep `Saved/Playtests/20261023-192402-sweep`, no branch regression found).
- **DevTestMap now hits and kills the player, so T1 (enemy-free test maps) blocks every non-combat playtest.** With N7 and N31 fixed, a player standing at the spawn is killed in about 10 s (three Frontliners close to 150 cm and hit for 15). 14 checks that PASSed in the `01abb8b` sweep now FAIL only because the player was being hit, body-blocked (N14, now more frequent) or dead when they ran:
  - us001: `move_s`, `walk_speed_baseline`, `walk_lane_reset`, `walk_lane_reset2`, `jump_space` (and `sprint_shift`/`sprint_l3` SKIP).
  - us054: `walk_to_frontliner3`, `melee_lmb`, `dodge_lalt`, `arena_reset`, `respawn_moves`. `arena_reset` also requires the AC-054.2 200 max health (N13): the first read after the respawn had all three enemies back at their spawns at 100/100 (ResetArena works), and they re-aggroed within ~1.5 s because every spawn is inside AggroRange of PlayerStart. Since issue #18 (`39500e2`) the health clause holds (200.0/200.0 after the reset); it FAILs on positions only, from the re-aggro.
  - us057: `walk_blink_lane`. us078: `light_attack_lmb`. us083: `block_over_held_rt_no_parry`, `light_attack_rt_tap` (its `_idle()` treats a dead player as idle).
  - Remedy: T1 maps (movement lanes and a combat-dummy pocket without hostile AI), or out-of-aggro Frontliner placement (US-085 TODO(DECISION)). A death guard in the scripts would only hide the cause. **Decided (Jeff, 2026-10-28, issue #19): T1.** The placement stays.
  - **In progress: US-100** (issue #46, 2026-10-29; evidence to come). The 14 checks move to the test maps:
    - MovementTestMap: us001 `move_s`, `walk_speed_baseline`, `walk_lane_reset`, `walk_lane_reset2`, `jump_space`; us083 `block_over_held_rt_no_parry`, `light_attack_rt_tap`.
    - CombatTestMap, in the new `Scripts/playtests/us054_combat.py`: `walk_to_dummy` (the renamed `walk_to_frontliner3`), `melee_lmb`, `dodge_lalt`, `arena_reset`, `respawn_moves`.
    - CombatTestMap: us057 `walk_blink_lane`, us078 `light_attack_lmb`.
    - us083 and us078 now FAIL with an explicit "player dead" detail instead of treating a dead player as idle. That is not a death guard: the cause stays visible.
    - Acceptance: all 14 PASS on a fresh package, and no check that PASSed in the `f21525f` sweep, the `01abb8b` sweep or the `92e7575` us079 run (`Saved/Playtests/20261028-232550-us079`) FAILs.
    - `arena_reset` and `respawn_moves` were SKIP in the `01abb8b` sweep (no death); they PASSed in `Saved/Playtests/20261028-085332-us054` @ `f21525f`.
- **13 NEW failures are pre-existing and identical in the `01abb8b` sweep, but their steps carry no `known_issue`:** us054 `enemies_present` (N13; since issue #18 only its spacing clause fails, so it needs no N13 tag), `enemy_indicators` (N17); us057 `fireball_projectile`, `fireball_hits_enemy` (N15), `stone_skin_indicator` (N16); us069 `shoulder_switches_tab` (N18); us079 `menu_rows`, `unidentified_badge`, `menu_ledger_rows`, `identify_button`, `filter_weapons`, `filter_armor_empty`, `filter_all` (the CommonUI pool bug, US-094; fixed in code in `a8aaa51`, so they need no tag). Tag them in the scripts' PLANs in the owning stories. `us054_arena.py`'s KNOWN_ISSUES text "Suspected NEW (unverified): ... the enemies never damaged the player" is obsolete since `c7e4b8d`.
- **Issue #19 sweep** (2026-10-28, fresh package of `f21525f`, `Saved/Playtests/20261028-090808-sweep/summary.md`, triaged against the issue #18 sweep `Saved/Playtests/20261027-202756-sweep`): no regression from the enemy bodies.
  - us054 `enemies_visible` is FAIL (known) → PASS. Every other us054 failure is unchanged.
  - us057, us069, us078 and us079 have the same NEW and known failures as before.
  - us001 and us083 changed only in checks that depend on where the swarm stands or when it kills the player.
    - us083 `dodge_gamepad_b` FAILed because its B tap (09:11:27.268) reached a dead player: the first alive read is 0.2 s later (`20261028-091107-us083/bridge.jsonl`).
    - us083 `block_over_held_rt_no_parry` went FAIL → PASS.
- **N26 has real-input evidence now:** in `Saved/Playtests/20261027-085233-us057` the R tap cast Stone Skin while the player was dead (bridge.jsonl lines 220-245: SP 7 -> 3, `Mordecai.State.Dead` with `State.Casting` and then `Status.StoneSkin`).

**Found by the issue #46 source audit** (2026-10-29, read-only, while planning CombatTestMap's attacker for US-100).
- **N32 Block, parry, dodge i-frames and Blink invulnerability do not reduce enemy melee damage.**
  - `UMordecaiGA_MeleeAttack::PerformHitDetectionAndApplyDamage` calls `ApplyDamageToTarget` (`MordecaiGA_MeleeAttack.cpp`) for every hit. That function applies the health and posture damage as instant GEs straight to the target ASC, then the on-hit payloads and status effects. Nothing in it reads `Mordecai.State.Blocking`, `State.Dodging`, `State.Invulnerable` or `State.Parrying`.
  - The hit query filters only the attacker itself and friendly targets. Its airborne filter (`JumpAvoidable` via `UMordecaiHitDetectionSubsystem::IsTargetAirborne`) never fires in game: nothing in Source/, Config/ or content grants `Mordecai.State.Airborne` (only a test adds it; the jump is a bare `ACharacter::Jump`, which only gets Lyra's `Movement.Mode.Falling` tag). The arc test also ignores Z (`IsPointInArcSector` zeroes it), so a jumping player inside the reach is still hit.
  - `UMordecaiGA_Block::ReceiveBlockedHit` holds the block mitigation (`ComputeBlockedDamage`), the stamina drain and the perfect-block posture damage. Its only caller is a test (`MordecaiCombatInputAbilityTests.cpp`).
  - `UMordecaiAttributeSet` only clamps (`PreAttributeChange`); it has no damage-execution hook.
  - So a player who blocks, parries, dodges, Blinks or jumps through an enemy swing takes the full 15 damage. Only leaving the 150 cm reach avoids it. N19 (the Invulnerable tag sticking after Blink) therefore has no gameplay effect today.
  - Why the done stories missed it: US-005 AC-005.3 says the damage pipeline checks `Mordecai.State.Dodging`, but its test `Mordecai.Dodge.GrantsIFramesDuringDodge` only checks the tag and applies no damage. US-006's block tests (`Mordecai.Block.MitigatesDamage` and the perfect-block tests, AC-006.3/006.9/006.10) call `ComputeBlockedDamage` and the block functions directly, never an enemy hit.
  - The us083 block and parry checks are tag-only, so they stay valid. A playtest check that asserts mitigation (for example blocking the CombatTestMap attacker) would FAIL as a real bug.
  - Not fixed by US-100; it needs a recovery story. Its scope must include granting `Mordecai.State.Airborne` while the character is falling (for example from the movement-mode change), or the AC-003.7 jump-avoid rule stays dead against `DA_EnemyAttack_BasicSlash` (`JumpAvoidable` = true).

**Bridge gaps found while writing the Phase 3 playtests.** These are missing bridge state, not game bugs; each blocks an assertion.
- **B1 The bridge did not report pickup name labels (FIXED, issue #30).** `actors[].widgets` lists only `UWidgetComponent`s, but the pickup label is a `UTextRenderComponent`, so check `pickup_labels` in `us079_inventory.py` (AC-079.10) failed (known issue B1).
  - Fix: `MakeActorEntry` reports every `UTextRenderComponent` as `actors[].text_labels[]`: `component`, `text`, `visible`, and `facing` / `upright` against the local camera (`Specs/agent_bridge_v1.md` §4 `state`). `pickup_labels` reads it and lost its `known_issue`; it also requires each label to read its pickup's display name, square to the camera and left to right (issue #38).
  - Tier 1: `Mordecai.Playtest.AgentBridge.State.PickupEntries`, `.ActorsMeasureLabelsAgainstTheLocalCamera` (red before the fix).
  - Verified on `92e7575` (fresh package): `pickup_labels` FAIL (known) → PASS in `Saved/Playtests/20261028-232550-us079`: 6/6 labels visible, text = display name, facing/upright 1.000. That us079 run exited 0 (55/55, 15/15 ACs accepted): the player outran the swarm (T1) that time.
- **B2 Other missing state.** No camera transform (AC-1.2.1), no progress-bar fill or colour (HUD bars, stamina tiers, rarity colours), no equip slot on `player.weapon` (AC-078.15), no damage-pop events (US-065), no texts from world-space widget components (enemy bars). Mouse buttons cannot be held across reads by the playtest client, which blocks block and parry on the mouse (AC-083.11). The gamepad triggers can be held with `axis` (no `hold_s`), so `us083_combat_input.py` plays block and parry on LT/RT, and the heavy attack fires on release since US-083, so a held click checks it. The `wait_until` grammar has no "array contains", so tag checks need client-side predicates.
- **B3 A hold was timed in wall time, so a frame hitch cut it short (FIXED 2026-10-29, US-100).**
  - What happened: the packaged game hitches for a few hundred ms after a `screenshot`, and often during its first launches after a package. us001 runs first in a sweep, and 31-46% of its 0.4 s holds ran at under 30 fps, in every sweep since Phase 3, on DevTestMap as on MovementTestMap.
  - The bridge let a held input up once `hold_s` of wall time had passed, at the end of a frame. A long frame's delta is simulated by the frame after it, so a hitch inside a hold left the character almost no input. Evidence (ad62476 and ea0205f sweeps):
    - `respawn_moves`: W held 0.464 s over 3 frames moved 3 cm (`Saved/Playtests/20261029-132452-us054combat`).
    - us001 `move_w`: 0.496 s over 5 frames moved 9 cm (`Saved/Playtests/20261029-164423-us001`).
    - These are the random us001 `move_*` failures of the earlier sweeps too.
  - Fix: `FMordecaiAgentBridgeHoldClock` (`MordecaiAgentBridgeProtocol.h`).
    - `key` tap, `axis` and `click` holds go up at the frame boundary nearest to the point where the simulated frames with the input down add up to `hold_s`.
    - Each frame counts at most the world's `MaxUndilatedFrameTime` of 0.4 s, so a long hold stays down through a hitch and a quick tap goes up before the hitch is simulated.
    - Responses add `simulated_s`. See `Specs/agent_bridge_v1.md` §2 Hold time.
  - Tier 1: `Mordecai.Playtest.AgentBridge.HoldClock.*`. They were red first against a wall-time clock, which let the key up at the end of the long frame, and then against an uncapped clock without the nearest-boundary rule.
  - Found in the review of this fix:
    - A frame longer than 0.4 s counted in full, though the world simulates only 0.4 s of it.
    - A 0.05 s click held through a hitch would have read as a heavy attack.
  - Left as is: auto-repeat still runs on wall time. No playtest reads repeats.

**Playtest infrastructure (proposal).**
- **T1 Dedicated test maps per playtest (Jeff, 2026-10-24; issue #46). IN PROGRESS: US-100** (`stories/in-progress/US-100-recovery-playtest-test-maps.md`, branch `fix/issue-46-test-maps`, 2026-10-29). Evidence to come: it is delivered only when a fresh package boots both maps and the sweep passes the 14 checks listed under "Found by the issue #12 sweep" above.
  - **What US-100 delivers:**
    - `/MordecaiCore/Game/Maps/MovementTestMap`: no enemies, no pickups; 4000 × 4000 cm grid floor; PlayerStart (0, 0, 100); lane L1 at y = -800; a 30 cm walk-up step and a 60 cm jump step at +Y.
    - `/MordecaiCore/Game/Maps/CombatTestMap`: `TestDummy` (`BP_MordecaiEnemy_TestDummy`) at (600, 600), 200 HP; it never moves and never aggroes (AI BP with AggroRange 0). `TestAttacker` (`BP_MordecaiEnemy_TestAttacker`) at (2100, 0), 200 HP; it holds its spot (`DefaultMoveSpeed` 0) and swings BasicSlash about every 2.6 s within 150 cm. It is more than 900 cm from every scripted spot except its fight spot (2000, 0), 100 cm -X of it. The navmesh is built.
    - Both maps get the Mordecai experience as DevTestMap does: World Settings GameMode Override = `MordecaiGameMode`, no World Settings experience. Both are in `package_playtest.py` `DEFAULT_MAPS` and `+MapsToCook`. Built by `Scripts/us100_01_create_test_enemies.py` and `Scripts/us100_02_create_test_maps.py`.
  - **Each script declares its `MAP`, and the kit's `map` check FAILs a run whose loaded map differs:**
    - MovementTestMap: us001, us083, us069.
    - CombatTestMap: the new `us054_combat.py` (US-054's melee/dodge/death/respawn/reset chain), us057, us078.
    - DevTestMap: `us054_arena.py` (enemy AI) and us079 (Jeff, 2026-10-29: its six pickups stay there).
    - `playtest_story.py` supports a story with several scripts (marker-keyed Steps tables, a unique `RUN_SUFFIX`, a FAIL of an AC's check in any run beats a PASS).
  - Out of scope: crouch (Jeff, 2026-10-29: it is "Sneak (Toggle)", blocked US-034), and the inventory route (us079 stays on DevTestMap).

  The original proposal follows. Every Tier 4 script plays DevTestMap, whose Frontliners (invisible until issue #19) follow the player and body-block it (N14). Since 2026-10-28 (Jeff, issue #19) the swarm at the spawn is T1's to solve: the Frontliners keep their placement. Movement checks (walks, sprint, dodge distance) are therefore hostage to where the enemies stand, and an exit-0 run of a story's own script can fail on another story's bug. US-083 closed with AC-083.4..AC-083.8 passing on real input but in no exit-0 run: the us083 script's lane walk was body-blocked in all three tries, on two packages.
  - It now blocks the inventory slice too. On the fresh package of `a8aaa51` (issue #28), the swarm killed the player on us079's pickup route in 3 of 3 runs (`Saved/Playtests/20261028-195457-us079`, `-195608-us079`, `-195731-us079`), after 0, 2 and 1 pickups. The earlier packages had survived the route 3 of 3 times at 25 HP. So US-094's four-item checks, and US-079's own acceptance, waited on T1.
    - Jeff decided on 2026-10-29 that us079 stays on DevTestMap, so T1 does not unblock them. Their acceptance still races the swarm; closing them is Jeff's call. On `92e7575` the race was won once: `Saved/Playtests/20261028-232550-us079`, exit 0.
  - Proposal: small cooked maps per concern, each resolving the Mordecai experience: an enemy-free movement/sprint lane, a combat dummy that never moves, and the arena kept for the enemy stories. A script picks its map (`--map`, as `playtest_boot.py` does).
  - Needs: the maps (an EDITOR story), adding them to the package's maps-to-cook, and a `MAP` per playtest script in the kit.

## Open decisions (Jeff)
1. Worker timeouts (3600/4500s) vs packaging time (86 min first, 3-12 min after): raise them, or package in the background across cron runs?
2. Tier 3 for **every** story (current rule) or only player-facing ones?
3. HEADLESS stories may launch the editor for Tier 2: confirm.
4. `Package.bat`: switch to `-CookAll -CookMapsOnly`, or retire it?
5. ~~The cron prompts reference removed `stories/` folders~~ **Resolved 2026-10-19.** All Mordecai context moved from `~/.openclaw` into the repo: `CLAUDE.md`, `Specs/`, `stories/` (restored from git history), `.claude/skills/`, `Scripts/check_editor.py`, `Scripts/ue_remote_exec.py`. The superseded `AGENT_RULES.md` was not migrated. All four OpenClaw Mordecai cron jobs are disabled, so nothing is scheduled; the prompts are kept for reference in `stories/agent-prompts/`. Choosing a new scheduler is a later decision.
6. ~~Phase 2 transport/port and whether the MCP server should be registered globally or per-project.~~ **Resolved 2026-10-20** (Jeff may revisit).
   - **Transport:** localhost-only TCP on `127.0.0.1`, newline-delimited JSON (`Specs/agent_bridge_v1.md` §2-§3).
   - **Port:** the Python client picks a free `127.0.0.1` port for every launch and passes `-AgentBridgePort=<n>`. When the flag is omitted, the game defaults to 47777. The client also passes `-AgentBridgeToken=`, which `ping` echoes back, so the client knows it reached the process it launched.
   - **MCP registration:** project-scoped, in the committed `.mcp.json`, as `mordecai-play` (`py -X utf8 Scripts/mordecai_play_mcp.py`; tools `mcp__mordecai-play__*`).
     - Claude Code asks once to approve a project server.
     - non-interactive `claude -p` / SDK sessions may load it without asking, depending on their settings.
     - Per-user pre-approval can go in personal settings: `enabledMcpjsonServers: ["mordecai-play"]`, and allow `mcp__mordecai-play__*`.
   - **Setup:** install the SDK once with `py -m pip install -r Scripts/requirements.txt`.
7. ~~Crouch on the movement test map (issue #46)?~~ **Decided 2026-10-29 (Jeff): out of scope.**
   - Mordecai's crouch is the spec'd "Sneak (Toggle)" (Ctrl / R3, `Specs/control_bindings_v1.1.md`). The Sneaking skill is "crouched movement" (`Specs/skill_sheet_v1.1.md`), owned by US-034, which is blocked on placeholder skill values.
   - T1 builds no crouch geometry or check. No crouch control is bound today: N23's C++ fix removed Lyra's crouch binding, and no C++ maps or binds `IA_Mordecai_Sneak`.
   - Recorded in `Specs/design_decisions_log.md` (2026-10-29).
