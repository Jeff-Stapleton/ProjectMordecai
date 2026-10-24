# Agent Playtest Plan v1: agents play the packaged game

**Status:** Phase 1 DONE (PR #2, branch `feature/playtest-phase1`). Phase 2 DONE on `feature/playtest-phase2`: acceptance 2.4 passed on a fresh package of `a40955b` on a locked machine (`Saved/Playtests/20261020-205856-us079`). Phase 3 IN PROGRESS on `feature/playtest-phase3` (kit, story runner, six feature scripts, exploratory agent; contract `Specs/story_playtests_v1.md`).
**Owner:** Jeff. **Last updated:** 2026-10-20.

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
  - `us054_arena.py`: HUD, enemies, AI, melee, dodge, damage, death, respawn, reset, kill counter.
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
- Pickup name labels render rotated 90° (hard to read).
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
- **Spells have no cooldowns.**
  - No `DA_Spell_*` asset sets a `CooldownTag`, so `ApplySpellCooldown` returns early (`MordecaiGA_SpellBase.cpp:445`).
  - The HUD cooldown widgets are never bound (`UMordecaiSpellCooldownWidget::BindToCooldownTag` has no callers), so they always show "ready".
- **Weapon effects never reach the ASC.** `UMordecaiEquipmentComponent` looks for an ASC on the character (`MordecaiEquipmentComponent.cpp:55-56`), but the ASC lives on the PlayerState. Weapon `GrantedTags` and `StatModifiers` are never applied.
- **The three red bars are explained.** They are the health bars of three `BP_MordecaiEnemy_Frontliner` enemies that have no mesh, so the enemies are invisible.
- **The inventory filter bar overlaps the "Stored Resources" header.** The category buttons (Materials, Quest) draw over the ledger heading (`Saved/Playtests/20261020-144643-us079/bridge_02.png`).
- **On-screen debug text is shown to players.** "MORDECAI: Programmatic IMC created and input bound successfully (including 4 spell actions)." appears in yellow under the HUD bars (`bridge_02.png`; message from `MordecaiHeroComponent.cpp:402`).
- **The CommonUI pool may leave inventory rows stale on a second menu open (latent).**
  - CommonUI pools the pause-menu widget.
  - On a second open, `RebuildContentArea` can short-circuit because the content class is unchanged, so the rows of the unbound inventory widget go stale and Identify silently fails.
  - The us079 flow opens the menu only once, so it does not hit this.

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
- **N11 (suspected) Projectiles are destroyed by pickup trigger spheres.** Projectiles overlap WorldDynamic and die on the first overlap (`MordecaiProjectile.cpp:32-33,145-177`).
- **N12 (suspected) The facing arrow is hidden in game.** `UArrowComponent` is hidden in game by default (`MordecaiCharacter.cpp:66-70`), so AC-2.1.5's arrow may not be visible.

**Found by the first Phase 3 sweep** (2026-10-22, `py Scripts/playtest_story.py --all` on a fresh package of `8ea853b`, `Saved/Playtests/20261022-184702-sweep/summary.md`). Each item is a NEW failure on real input, confirmed in the screenshots.
- **N7 confirmed: enemies swarm the player but never hurt them.**
  - From the first read, all three Frontliners sit next to the player at the spawn: around (−121, ±151) and (200, 0), `ai_state` Attack.
  - The player stays at 100/100 health for 20 s (`us054_arena.py` check `enemy_damages_player`).
  - The death, death-screen, respawn and arena-reset checks therefore cannot run.
- **N13 Enemies have 100 MaxHealth, not 200.** AC-054.2 specifies 200, but the cooked enemies report 100/100 (check `enemies_present`). The likely cause is that `DefaultMaxHealth` is still at its C++ default because the BP edit was not saved, as with N10.
- **N14 The invisible enemies body-block the player.** They crowd the spawn and follow the player, so the player meets an invisible wall:
  - A 0.7 s hold of A moved the player only 93 cm (`us001_movement.py` `move_a`).
  - `walk_to(0, -650)` got stuck at (−78, −45) (`walk_lane_wp1`).
  - `us054_arena.py` `walk_to_frontliner3` fails intermittently on its third leg, where the player walks into the enemies (2026-10-23). Base commit `32d3927`: FAIL, then PASS (`Saved/Playtests/20261023-150847-us054`, `20261023-150936-us054`). The step carries no `known_issue`, so it reports NEW. When it fails, `melee_lmb` and `dodge_lalt` run without the player next to the enemy.
- **N15 Fireball spends SP but does nothing visible.**
  - SP drops 10 → 7.
  - No projectile appears in `actors` within 1.5 s.
  - The enemy straight ahead (+X, about 1 m) keeps 100 health (`us057_magic.py` checks `fireball_projectile` and `fireball_hits_enemy`; `bridge_01.png`).
  - It may be related to N11.
- **N16 The status-effect indicator shows "Status", not the status name.** Stone Skin's indicator reads "Status 10.0" bottom-left (`stone_skin_indicator`, `bridge_03.png`).
- **N17 There are no enemy indicators.** No `EnemyIndicator` widget is on screen with three enemies within aggro range (`us054_arena.py` `enemy_indicators`; US-064).
- **N18 LB/RB do not switch pause-menu tabs.** AC-069.4 specifies them (`us069_pause_menu.py` `shoulder_switches_tab`). Needs triage: possibly missing by design (no CommonUI tab-list action).
- **The CommonUI pool bug is CONFIRMED** (it was latent).
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
- **N23 Left Shift crouched the player (FIXED, US-083).** `InputData_Mordecai` (the arena PawnData's `InputConfig`) tags `IA_Mordecai_Sprint` with `InputTag.Crouch` (`NativeInputActions[4]`), and `ULyraHeroComponent::InitializePlayerInput` binds that tag to `Input_Crouch` -> `ToggleCrouch` on Triggered, every frame the key is held. Every Left Shift press toggled crouch: walk speed capped at 300 cm/s and jumping refused while `bWantsToCrouch` (the `Status.Crouching` in the sweep's us001 run after the second LeftShift tap and in us078 after Shift+Tab). The sweep's `sprint_shift` and `jump_space` failures were the missing bindings, but the crouch would have broken both once they were bound. Fixed in C++: `UMordecaiHeroComponent::InitializePlayerInput` removes that binding after Lyra makes it (`RemoveNativeBindingsForTag`, test `Mordecai.Recovery.Input.LyraCrouchBindingRemoved`). The content entry itself is still wrong; deleting it is an EDITOR change.
- **N24 Light attacks never combo.** `UMordecaiGA_MeleeAttack::NotifyComboInput` has no caller, so a second click during Recovery does nothing (AC-004.9). Wiring it (an `InputPressed` override, which the Lyra input path now reaches) also needs a guard first: `GetActiveProfile` re-resolves the equipped weapon on every call, so a weapon switched mid-combo can leave `CurrentComboIndex` out of range; `OnWindupComplete` then sets a 0 s timer (which clears it) and the attack never ends (`State.Attacking` forever).
- **N25 Attack cancel windows are not wired.** `CanCancelIntoDodge` / `CanCancelIntoBlock` are never called: a dodge or block during an attack runs alongside it, and the attack can still land its hit (`MordecaiDodgeTests.cpp` says "the controller cancels the attack first"; nothing does).
- **N26 Spells ignore death and the parry whiff.** Spells activate through `TryActivateAbilityByClass` (`UMordecaiHeroComponent::ActivateSpellBySlot`), and `UMordecaiGA_SpellBase` is not blocked by `State.Dead` or `State.ParryWhiff` (AC-006.14 says no abilities during the whiff). The combat abilities are, since US-083.
- **N27 (suspected) A dodge travels about 120 cm, not 400.** `UMordecaiGA_Dodge::ApplyDodgeMovement` launches the pawn at 1000 cm/s with no Z (`LaunchCharacter`): it lands within a few frames, then walking braking stops it, so it covers about 120 cm, not `DodgeDistanceCm` 400 (design-review estimate while fixing US-083). `dodge_lalt` (us054) passes on the Dodging tag and prints the displacement; its US-083 run gives the measured distance.
- **N28 Sprint is client-local.** `AMordecaiCharacter::SetSprinting` writes `MaxWalkSpeed` on the machine that handled the input (no RPC, nothing replicated), so a remote client in networked play would be corrected on every sprinting move (and while blocking, Block's speed change runs on both sides). Standalone and a listen-server host are fine. `TODO(DECISION)` at `SetSprinting`: a CharacterMovementComponent saved-move flag read in `GetMaxSpeed`, with Block's slow-down moved there too.

**Bridge gaps found while writing the Phase 3 playtests.** These are missing bridge state, not game bugs; each blocks an assertion.
- **B1 The bridge does not report pickup name labels.** `actors[].widgets` lists only `UWidgetComponent`s, but the pickup label is a `UTextRenderComponent` (`MordecaiItemPickup.cpp:36`). Check `pickup_labels` in `us079_inventory.py` (AC-079.10) fails on that. The fix is to report TextRender components (text, visibility) in `MakeActorEntry`.
- **B2 Other missing state.** No camera transform (AC-1.2.1), no progress-bar fill or colour (HUD bars, stamina tiers, rarity colours), no equip slot on `player.weapon` (AC-078.15), no damage-pop events (US-065), no texts from world-space widget components (enemy bars). Mouse buttons cannot be held across reads by the playtest client, which blocks block and parry on the mouse (AC-083.11). The gamepad triggers can be held with `axis` (no `hold_s`), so `us083_combat_input.py` plays block and parry on LT/RT, and the heavy attack fires on release since US-083, so a held click checks it. The `wait_until` grammar has no "array contains", so tag checks need client-side predicates.

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
