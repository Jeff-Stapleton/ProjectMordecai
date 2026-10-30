# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
UE 5.7 diorama-style ARPG built on Lyra. Fixed camera, twin-stick controls, GAS-based combat, Iris replication.

## Architecture

### Class Hierarchy (Mordecai extends Lyra)
All project-specific code lives in `Source/LyraGame/Mordecai/` and extends Lyra base classes:

- `AMordecaiCharacter` → `ALyraCharacter` — Twin-stick player character with sprint, facing arrow
- `AMordecaiPlayerController` → `ALyraPlayerController`
- `AMordecaiPlayerState` → `ALyraPlayerState`
- `AMordecaiGameMode` → `ALyraGameMode`
- `AMordecaiGameState` → `ALyraGameState`
- `UMordecaiAbilitySystemComponent` → `ULyraAbilitySystemComponent`
- `UMordecaiAttributeSet` → `ULyraAttributeSet` — Health, Stamina, Posture, SpellPoints + 9 primary stats (Str/Dex/End/Con/Res/Dis/Int/Wis/Cha)
- `UMordecaiCameraMode_Diorama` → `ULyraCameraMode` — Fixed overhead camera (no rotation)

### Combat System
- `UMordecaiHitDetectionSubsystem` (WorldSubsystem) — Shape queries (arc sector, capsule, circle) with target filtering, airborne rules, friendly fire
- `UMordecaiAttackProfileDataAsset` (DataAsset) — Data-driven attack definitions: timing, hit shapes, damage profiles, combo data, stamina costs
- `MordecaiCombatTypes.h` — Core enums (`EMordecaiAttackType`, `EMordecaiDamageType`, `EMordecaiInputSlot`, `EMordecaiHitShapeType`) and structs (`FMordecaiHitShapeParams`, `FMordecaiDamageProfile`, `FMordecaiProjectileSpec`)
- `MordecaiHitDetectionTypes.h` — `FMordecaiHitResult`, `EMordecaiTargetFilter`, trace channel definition

### Gameplay Tags
Native tags declared in `MordecaiGameplayTags.h/.cpp` under `MordecaiGameplayTags` namespace. Pattern: `Mordecai.State.*`, `Mordecai.Damage.*`.

### Plugin: MordecaiCore
`Plugins/GameFeatures/MordecaiCore/` — GameFeature plugin for content (maps, input actions, experiences). Runtime module at `Source/MordecaiCoreRuntime/`. Dependencies: GameplayAbilities, EnhancedInput, ModularGameplay.

## Critical References
Read these before starting any work:

### Core Rules & Design
- **Agent Rules (MUST READ FIRST):** `Specs/agent_rules_v2.md`
- **Agent Playtest Plan (phases, status, Phase 2/3 implementation plan, known-issues backlog):** `Specs/agent_playtest_plan_v1.md`
- **Agent Bridge (Tier 4 real-input protocol, Python client, MCP server, playtest scripts):** `Specs/agent_bridge_v1.md`
- **Game Design v2:** `Specs/game_design_v2.md`
- **UE5 Tech Stack:** `Specs/ue5_tech_stack_context.md`
- **Design Decisions Log:** `Specs/design_decisions_log.md`
- **New Spells Proposal:** `Specs/new_spells_proposal.md`

### Combat & Abilities
- **Attack Taxonomy:** `Specs/attack_taxonomy_v1.md`
- **Combat System:** `Specs/combat_system_v1.md`
- **Ability Schema:** `Specs/ability_schema_v1.md`
- **Ability System:** `Specs/ability_system_v1.md`
- **Damage Types:** `Specs/damage_types_v1.md`
- **Player Attacks:** `Specs/player_attacks_agent_brief_v1.md`

### Attributes & Status Effects
- **Character Attributes:** `Specs/character_attributes_v1.md`
- **Attribute Scaling:** `Specs/attribute_scaling_v1.md`
- **Stat Formulas:** `Specs/stat_formulas_v1.md`
- **Status Effects:** `Specs/status_effects_v1.md`
- **Status Effects (Launch Set):** `Specs/status_effects_launch_set_v1.md`

### Skills & Items
- **Skill Sheet v1.1:** `Specs/skill_sheet_v1.1.md`
- **Item Schema v2:** `Specs/item_schema_v2.md`
- **Item System:** `Specs/item_system_v1.md`

### Input & Controls
- **Control Bindings v1.1:** `Specs/control_bindings_v1.1.md`

### Enemies & AI
- **Enemy Archetypes:** `Specs/enemy_archetypes_v1.md`
- **Enemy AI Framework:** `Specs/enemy_ai_framework_v1.md`

### World & Village
- **World Design:** `Specs/world_design_v1.md`
- **Village System:** `Specs/village_system_v1.md`
- **Map Style Technical Design (cell rules, height tiers, combat pockets):** `Specs/map_style_technical_design_v1.md`
- **Agentic Map Pipeline (7-stage world-building production pipeline):** `Specs/agentic_map_pipeline_v1.md`. Both are required reading for any map/world story; see `agent_rules_v2.md` §8.

## UE5 Reference Skills
Read these when working on the relevant area — they contain patterns, pitfalls, and code templates:

- **UE5 C++ Patterns** (types, macros, delegates, smart pointers, replication):
  `.claude/skills/unreal-cpp/SKILL.md`
  - References: `actor-lifecycle.md`, `containers.md`, `delegates.md`, `replication.md`, `smart-pointers.md`, `string-types.md` (same directory `/references/`)

- **GAS Framework** (abilities, attributes, effects, gameplay cues, ability tasks):
  `.claude/skills/unreal-gas/SKILL.md`
  - References: `ability-tasks.md`, `attributes.md`, `common-patterns.md`, `gameplay-abilities.md`, `gameplay-effects.md` (same directory `/references/`)

- **Testing** (automation framework, spec/BDD tests, screenshot tests, running from CLI):
  `.claude/skills/unreal-testing/SKILL.md`
  - References: `automation-driver.md`, `automation-spec.md`, `fbx-testing.md`, `screenshot-comparison.md` (same directory `/references/`)

- **Python Editor Scripting** (editor automation, asset creation, level setup):
  `.claude/skills/unreal-python-editor/SKILL.md`
  - References: `actor-subsystem.md`, `asset-library.md`, `common-classes.md`, `custom-classes.md`, `level-blockout.md`, `level-subsystem.md`, `remote-execution.md` (same directory `/references/`)
  - Remote exec script: `Scripts/ue_remote_exec.py`

- **Tribal Knowledge** (hard-won lessons, gotchas, non-obvious fixes for Lyra/UE5/Mordecai):
  `.claude/skills/unreal-tribal-knowledge/SKILL.md`

**When to read which skill:**
- Writing ANY C++ → read `unreal-cpp` SKILL.md first
- Writing GAS code (abilities, attributes, effects) → also read `unreal-gas` SKILL.md
- Writing tests → read `unreal-testing` SKILL.md
- Creating editor assets via Python → read `unreal-python-editor` SKILL.md
- Debugging asset loading, experience, camera, or ability issues → read `unreal-tribal-knowledge` SKILL.md
- **After solving any non-obvious problem** → ADD the lesson to `unreal-tribal-knowledge` SKILL.md

## Project Paths
- **Project Root:** `C:\Users\jeffd\Documents\Gamedev\ProjectMordecai`
- **Engine:** `C:\Users\jeffd\Documents\Gamedev\UnrealEngine`
- **Solution:** `ProjectMordecai.sln`
- **MordecaiCore Plugin:** `Plugins\GameFeatures\MordecaiCore`

## Build & Test Commands (Headless — No GUI Editor Needed)
```powershell
# Compile (Development Editor, Win64)
& "C:\Users\jeffd\Documents\Gamedev\UnrealEngine\Engine\Build\BatchFiles\Build.bat" LyraEditor Win64 Development -Project="C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\ProjectMordecai.uproject" -WaitMutex -FromMsBuild

# Run ALL Mordecai tests (from the repo root)
py Scripts/run_tests.py

# Run a SINGLE test or test group (replace filter with test name prefix)
# Examples: "Mordecai.HitDetection" runs all hit detection tests
#           "Mordecai.HitDetection.ArcSectorHitsActorInArc" runs one test
py Scripts/run_tests.py Mordecai.HitDetection
```

These commands are fully headless. They do NOT require the GUI Unreal Editor to be running.

`run_tests.py` runs `UnrealEditor-Cmd -ExecCmds="Automation RunTests <filter>" -NullRHI ... -TestExit="Automation Test Queue Empty"` and judges pass/fail from the test log (`Saved\TestLogs\tests-<timestamp>.log`; `--log <path>` to choose). Exit 0 = at least one test ran and all passed; 1 = one or more tests failed; 2 = infrastructure (no tests matched, editor crashed, queue never finished). `Scripts\RunTests.bat [Filter]` wraps it. Never judge a raw `UnrealEditor-Cmd ... Automation RunTests` run by its exit code — it exits 0 even when tests fail.

Python tooling (playtest scripts, agent bridge client, MCP server) — one-time setup and its own offline tests:
```powershell
# Once per machine: the MCP SDK (mcp 2.x) and Pillow
py -m pip install -r Scripts/requirements.txt

# Offline unit tests of the Scripts/ tooling (fakes only; never launches the game or the editor)
py Scripts/run_python_tests.py            # -k walk_to = only matching tests, -v = one line per test
```
`run_python_tests.py`: exit 0 = at least one test ran and all passed; 1 = a test failed or errored; 2 = infrastructure (no tests found, a test module failed to import). Run it after changing `Scripts/agent_bridge.py`, `Scripts/mordecai_play_mcp.py`, `Scripts/playtest_boot.py` or `Scripts/playtests/`. It gates the Python tooling only — it is not Tier 1 and not playtest evidence.

Packaging and the packaged playtest (Tier 3) commands are in **Step 4c** below; the real-input playtest (Tier 4) commands are in **Step 4d**.

## Editor Launch (For EDITOR-Mode Stories Only)
Some stories require the GUI Unreal Editor running (creating DataAssets, Blueprints, level setup via Python).

### How to launch the editor:
```powershell
Start-Process "C:\Users\jeffd\Documents\Gamedev\UnrealEngine\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList "C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\ProjectMordecai.uproject"
```

### How to check if it's ready:
```powershell
py "Scripts/check_editor.py" --wait --timeout 300
```
This pings the Python remote execution endpoint (UDP 239.0.0.1:6766). Exit code 0 = ready.

### How to run Python scripts against the running editor:
```powershell
py "Scripts/ue_remote_exec.py" --file path\to\script.py
```

### When you need the editor:
- Check the story's **Execution Mode** field
- If `EDITOR`: launch the editor, wait for ready, then do your work
- If `HEADLESS`: do NOT launch the editor for development — compile and test from CLI. The one exception is the Tier 2 PIE smoke run (Step 4b): `run_pie_verify.py` launches the editor for it, and you close it afterwards (Step 7). Packaging, the packaged playtest (Step 4c) and the real-input playtest (Step 4d) do not use the editor.

---

## Workflow: TDD Pipeline (MANDATORY)

Every task follows this sequence. No exceptions.

### Verification Ladder
**Playable in the packaged build, or it isn't done.** Each tier proves something the tier below cannot. A lower tier NEVER substitutes for a higher tier.

1. **Tier 1 — Headless automation tests** (Step 4): logic. NullRHI, no rendering, no real content.
2. **Tier 2 — PIE smoke tests** (Step 4b): editor integration (spawn chain, components, widgets). Runs on in-editor memory, including unsaved assets and PIE-only settings.
3. **Tier 3 — Packaged playtest** (Step 4c): `py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`, then Read the screenshots. Proves a player launching the packaged game gets the Mordecai experience and the feature is on screen.
4. **Tier 4 — Real-input playtest** (Step 4d): the story's `py Scripts/playtests/usXXX_<name>.py` on the SAME fresh package as Tier 3, then Read the screenshots. The in-game agent bridge (`UMordecaiAgentBridgeSubsystem`, non-Shipping builds launched with `-AgentBridge`; protocol in `Specs/agent_bridge_v1.md`) injects OS-level key/axis/mouse/click events into Slate, so they take a player's path (Slate input preprocessors, focus, viewport, CommonUI, Enhanced Input), and returns state JSON + screenshots. `Scripts/agent_bridge.py` is its Python client and CLI; the `mordecai-play` MCP server (`Scripts/mordecai_play_mcp.py`, `.mcp.json`) lets an agent session play interactively. Proves a player can actually do it.

- **Definition of Done** for any story with player-facing behavior: Tier 3 on a FRESH package built from the commit you deliver shows the feature working, reached the way a player reaches it, and you have opened every screenshot and stated what is visibly on screen. If the story has ACs that need player input (keys, movement, mouse, menus), its Tier 4 playtest `Scripts/playtests/usXXX_<name>.py` also exits 0 on that same package, and you have opened every `bridge_NN.png`.
- **Order, every story:** Step 4 (Tier 1) → Step 4b (Tier 2) → commit and push code AND content (Step 5 rules) → Step 4c packages that commit and playtests it (Tier 3) → Step 4d runs the story's real-input playtest on that same package (Tier 4) → Step 6 closing commit records the evidence and moves the story.
- Content is real only when it is SAVED and COMMITTED (Step 5).
- An AC that needs player input is accepted ONLY by a passing Tier 4 run — never by Tier 3 screenshots, a Tier 2 `[LOGIC]` check, or any direct call. `PENDING TIER 4 (real input)` is retired. The one exception is an input the bridge cannot drive yet: record `PENDING TIER 4 (bridge cannot drive: <reason>)`; that AC stays unchecked and is not claimed, and the bridge gap is filed (Step 6).

### Step 1: Understand
- Read the story file in `stories/in-progress/`
- Read all referenced design docs
- Understand every acceptance criterion (AC)
- Read the story's **Playtest** section: launch path (front end or direct map + expected experience), the player's steps, what must be visible on screen, and which playtest proves each step (the Tier 3 `playtest_boot.py` run for what is on screen; the story's Tier 4 script `Scripts/playtests/usXXX_<name>.py` for every step that needs input). If a player-facing story has none, write one into the story (use `control_bindings_v1.1.md` for keys) before Step 2 and say so in your completion notes

### Step 2: Write Tests FIRST
- For every AC, write at least one automation test that verifies it
- Tests go in `Source/LyraGame/Tests/` organized by feature area
- Use UE Automation Test Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` or `BEGIN_DEFINE_SPEC` / `END_DEFINE_SPEC`)
- Tests MUST fail before implementation (red phase)
- Run tests to confirm they fail for the right reasons
- Player-facing story with ACs that need input: write its Tier 4 playtest `Scripts/playtests/usXXX_<name>.py` now too, modelled on `Scripts/playtests/us079_inventory.py` (one check per player step, every wait FAILs on timeout, screenshots of what the ACs promise). It runs in Step 4d; its logic can be exercised offline against `Scripts/tests/fake_bridge.py` (see `Scripts/tests/test_us079_playtest.py`)

### Step 3: Implement
- Write the minimum code to make tests pass
- Follow agent_rules_v2.md constraints at all times
- Use `TODO(DECISION)` for open items — never guess

### Step 4: Verify — Tier 1 (MANDATORY — Do Not Skip)
- Compile the full project (see Build & Test Commands above)
- Then run ALL Mordecai tests (not just new ones): `py Scripts/run_tests.py`
- Both must pass with zero errors (`run_tests.py` exit 0)
- If tests fail or build breaks, **fix before proceeding** — do not commit broken code
- Passing here proves logic only. Continue to Step 4b, Step 4c and Step 4d.

### Step 4b: PIE Smoke Test — Tier 2 (MANDATORY — Run Every Time)
Headless tests (NullRHI) cannot catch visual or integration issues (invisible widgets, broken spawn chains, missing Blueprint content). After headless tests pass, **always** run PIE smoke tests to verify the game works in-editor.

PIE smoke tests are **necessary, not sufficient**. PIE runs on in-editor memory (including unsaved assets) with PIE-only settings applied (e.g. `LyraDeveloperSettings` ExperienceOverride, which Lyra honors only in PIE). A PIE pass says nothing about what is saved, what is committed, or what a player gets in the packaged game — that is Step 4c.

**When to run PIE smoke tests:**
- **EVERY story, regardless of Execution Mode** — no exceptions.

**How to run:**
```powershell
# Full pipeline (launches editor if needed, starts PIE, runs all tests, stops PIE):
py Scripts/run_pie_verify.py

# If editor is already running:
py Scripts/run_pie_verify.py --no-launch

# Run specific tests only:
py Scripts/run_pie_verify.py --tests hud,spawn

# PIE another map: opens it first, reopens DevTestMap after PIE stops (default: the open map, DevTestMap).
# On a test map (MovementTestMap, CombatTestMap) the default tests are spawn,hud,test_maps.
# Git Bash rewrites /MordecaiCore/... arguments: run it from PowerShell, or prefix MSYS_NO_PATHCONV=1.
py Scripts/run_pie_verify.py --map /MordecaiCore/Game/Maps/CombatTestMap

# Or run individual smoke tests manually during PIE:
py "Scripts/ue_remote_exec.py" "exec(open(r'Scripts/verify_pie_hud.py').read())"
py "Scripts/ue_remote_exec.py" "exec(open(r'Scripts/verify_pie_spawn.py').read())"
```

**PIE mode:** Use `editor_request_begin_play()` (NOT `editor_play_simulate()` which starts spectator mode).

**If remote exec is unreliable:** Open the editor's Python console (`Window > Developer Tools > Output Log > Cmd dropdown > Python`) and paste the smoke test script content directly.

**Available smoke tests** (in `Scripts/`): `run_pie_verify.py` runs every `Scripts/verify_pie_<name>.py` it finds (`--tests <name>,...` selects). Examples:
- `verify_pie_hud.py` — Checks combat HUD widget tree: root visible, all child bars exist
- `verify_pie_spawn.py` — Checks spawn chain: controller, pawn, components, camera, ASC
- `verify_pie_test_maps.py` — Checks a test map (US-100): MordecaiGameMode, the pawn near (0, 0), no enemies on MovementTestMap; on CombatTestMap the TestDummy and TestAttacker Idle at 200/200 with MaxWalkSpeed 0, and a navmesh. SKIP lines only on any other map.

**Smoke tests must not fake the player:**
- DO assert on state a player would see: actors placed in the level, components present, widgets bound and visible, input actions mapped in the active IMC.
- DO tag every check that calls gameplay functions directly with `[LOGIC]` in its PASS/FAIL line — e.g. teleporting the pawn and calling `interaction.try_pickup_focused()`, or calling `subsystem.toggle_pause_menu_to_tab('inventory')` (both in `verify_pie_inventory.py`, which does not tag them yet — add the tag when you touch it). These are logic/diagnostic checks, not proof of playability.
- DO NOT cite a `[LOGIC]` check — or any Tier 2 result — as acceptance of a player-facing AC. Player-facing ACs are accepted at Tier 3 (Step 4c) for what is on screen, and only at Tier 4 (Step 4d, real input) for anything that needs input.

**Writing new smoke tests:**
When you implement a feature that has runtime/visual behavior, add a `Scripts/verify_pie_<feature>.py` smoke test. Follow the pattern in existing verify scripts: get PIE world, check invariants, print PASS/FAIL lines, raise RuntimeError on any failure. Follow the rules above.

### Step 4c: Packaged Playtest — Tier 3 (MANDATORY — Run Every Time)
The packaged game is what a player runs: no PIE, no `WITH_EDITOR` code, no `LyraDeveloperSettings`, and only content that is saved on disk and cooked. Run this for **every story**, regardless of Execution Mode, after Step 4b passes and the work is committed. For a story with no player-facing behavior it proves the delivered commit still packages and boots to the expected experience; for a player-facing story it must also show the feature. Packaging is slow (a first or `--clean` package can take an hour+; iterative cooks are faster) — that cost is accepted. Run it in the background and poll the log; do not let a tool timeout kill it.

**1. Commit first, then package that commit:**
- Step 4's `LyraEditor` compile must be current — the cook runs on the editor binaries.
- Save every dirty asset in the editor, then close the editor (Step 7 command). The cook reads from disk; unsaved editor memory does not ship, and a running editor with Live Coding blocks the package build.
- Commit and push code AND content following the Step 5 rules (`check_content_committed.py` exit 0). `git status` must be clean: `package_playtest.py` records the commit in `build_info.json`, and a build with `"dirty": true` is a diagnostic build, not delivery evidence.
- While the package builds and until the playtests are done (Step 4c and Step 4d), do not edit or commit anything. A new commit (HEAD != packaged commit), an uncommitted change under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject`, or an edit made while UAT was running makes `playtest_boot.py` report STALE BUILD. A fix means: commit → re-package → re-run.

**2. Package** (Development Win64 → `PackagedBuilds\Playtest\`):
```powershell
# Default: iterative cook of the front end + DevTestMap + MovementTestMap + CombatTestMap
py Scripts/package_playtest.py

# Full (non-iterative) cook — use when an iterative cook looks wrong
py Scripts/package_playtest.py --clean

# Cook every map (-CookAll -CookMapsOnly) in addition to the four default maps
py Scripts/package_playtest.py --all-maps

# Print the RunUAT BuildCookRun command only
py Scripts/package_playtest.py --dry-run
```
- Exit 0 = packaged. It first deletes the previous `build_info.json` and `PackagedBuilds\Playtest\Windows\`, then on success writes `PackagedBuilds\Playtest\build_info.json` (commit, branch, dirty, dirty_files, dirty_count, dirty_fingerprint, source_changed_during_build, built_at, config, maps, all_maps, game_exe, uat_log).
- Exit 1 = UAT reported success but the game binary is missing. Exit 2 = infrastructure (uproject/RunUAT missing, git error, previous package locked — e.g. the packaged game is still running). Any other non-zero = UAT failed or was interrupted (UAT's own exit code; 130 on Ctrl+C); the script prints the log tail and the `Error` lines. The full UAT log is `Saved\Playtests\package-<YYYYmmdd-HHMMSS>.log`. A package failure is a build failure — fix it before proceeding; `build_info.json` is not written, so `playtest_boot.py` reports STALE BUILD.
- Keep the default `--config Development`. Shipping strips dev features; DebugGame produces `LyraGame-Win64-DebugGame.exe`, which `playtest_boot.py` only runs via `--exe`.

**3. Boot playtest:**
```powershell
# Default: DevTestMap, expects B_MordecaiArenaExperience
py Scripts/playtest_boot.py

# The playtest test maps (US-100), also B_MordecaiArenaExperience: no enemies / a dummy + an attacker
py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap
py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap

# Real player entry point (L_LyraFrontEnd, no map arg), expects B_LyraFrontEnd_Experience
py Scripts/playtest_boot.py --frontend

# Another map / experience
py Scripts/playtest_boot.py --map <MapPath> --expect-experience <ExperienceName>

# More or later screenshots
py Scripts/playtest_boot.py --settle 10 --shots 5 --shot-interval 2.0
```
- Exit 0 = all checks passed.
- Exit 1 = gameplay failure: wrong experience (`B_LyraDefaultExperience` means the Mordecai experience did NOT load), crash, timeout, or `content_cooked` FAIL. That check fails when the game asked for a `/MordecaiCore` asset that is not in the build, usually content loaded from C++ by a path string; PIE can't catch it. A `loading_screen` FAIL means a loading screen showed that was not the Mordecai one (`UMordecaiLoadingScreenWidget`, issue #39). Fix the cause, then repeat Step 4 → 4b → 5 (commit) → 4c.
- Exit 2 = infrastructure: exe missing, **STALE BUILD**, window not found, git error. On STALE BUILD, commit if needed and re-run `package_playtest.py` — do not work around it.
- NEVER use `--allow-stale` for acceptance evidence. It is for diagnosis only, and the report records it.
- Use `--frontend` whenever the story touches the front end, menus, or playlists (how a player reaches a map).
- Each run writes `Saved\Playtests\<YYYYmmdd-HHMMSS>-boot\`: `report.md`, `report.json`, `game.log`, `engine_loading.png` (the loading screen when it first shows) and `engine_01.png`, ... (saved by the game itself from its own viewport, via `-PlaytestCapture` / `UMordecaiPlaytestCaptureSubsystem`) and `shot_01.png`, ... (desktop capture of the game window)
- `engine_NN.png` is the primary evidence: it works when the machine is locked or the window is covered, as it is on unattended runs. `shot_NN.png` is all black whenever the workstation is locked, so treat a black `shot_NN.png` with good `engine_NN.png` as expected. A missing or black `engine_NN.png` (`engine_screenshots` FAIL/WARN) means you have no visual evidence.

**4. Look at the screenshots (MANDATORY):**
- Read `report.md`, then open `engine_loading.png` and EVERY `engine_NN.png` (and any non-black `shot_NN.png`) with the Read tool.
- State what is actually on screen: the Mordecai diorama arena or a Lyra shooter HUD? The Mordecai HUD? The character? The content your story added (pickups, enemies, widgets)? A black/uniform-screenshot warning means you saw nothing — investigate; it is not a pass.
- Check the report's top errors. An error your change introduced (missing asset, failed load, `LogMordecai*` error) is a defect to fix, not a footnote.
- Compare against the story's **Playtest** section. Everything listed there that does not need player input must be visibly confirmed.
- Tier 3 sends no input. ACs that need keys/movement/mouse/menus are accepted only by the Step 4d real-input playtest — never claimed from a Tier 3 run.

**5. Cite the evidence:** the absolute run dir path, the PASS/FAIL per check, and a one-line description of each screenshot go in the story's completion notes and the body of the closing commit (Step 6). Report failures honestly — never paper over a FAIL, a warning, or a screenshot that does not show the feature.

### Step 4d: Real-Input Playtest — Tier 4
Tier 3 looks; Tier 4 plays. **An AC that needs player input (keys, movement, mouse, menus) is accepted ONLY by a passing Tier 4 run.** Every player-facing story with input-driven ACs ships a Tier 4 playtest `Scripts/playtests/usXXX_<name>.py` (written in Step 2, modelled on `Scripts/playtests/us079_inventory.py`), and it must exit 0 on the fresh package before the story moves to done. Tier 3 (Step 4c) stays mandatory for every story; Tier 4 does not replace it.

The in-game agent bridge (`UMordecaiAgentBridgeSubsystem`, `Source/LyraGame/Mordecai/Playtest/`; contract in `Specs/agent_bridge_v1.md`) exists only in non-Shipping builds launched with `-AgentBridge`, listening on `127.0.0.1` only. Its input commands — `key`, `axis`, `mouse`, `click` — inject OS-level events into Slate (`FSlateApplication::Process*Event`), so each one takes the path a player's keyboard, mouse or gamepad takes: Slate input preprocessors (CommonInput, CommonUI, Lyra's loading-screen input eater), user-0 focus, the game viewport, the CommonUI action router, the PlayerController and Enhanced Input. Slate hit-tests clicks. The bridge has no commands that call gameplay or UI functions.

**1. Same package as Step 4c:**
- Run it right after Step 4c, against the SAME fresh package. Do not re-package while HEAD is unchanged and nothing under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject` has changed.
- The script runs `playtest_boot.py`'s staleness gate first and refuses a STALE BUILD (exit 2) before it creates a run dir. Fix it the same way: commit → re-package → re-run Step 4c and Step 4d.
- Any new commit makes HEAD differ from the packaged commit, so the build goes STALE — including a commit that only fixes the playtest script. Commit the script with the story's code before packaging (Step 5); a later script fix means commit → re-package → re-run Step 4c and Step 4d.
- NEVER use `--allow-stale` for acceptance evidence. That report is headed `DIAGNOSTIC ONLY -- not acceptance evidence`.

**Story playtests (Phase 3, `Specs/story_playtests_v1.md`):**
- Every Tier 4 script is a `PlaytestRun` subclass on `Scripts/playtest_kit.py`.
  - It declares its `PLAN`: one `Step(check, does, proves=(AC ids), known_issue=...)` per player step.
  - Its step helpers (`walk`, `press`, `expect`, `click_button`, `observe`, `shot`, ...) map 1:1 to bridge calls.
  - Its report says, per AC, whether the run accepted it. Only a launched, fresh, clean-tree run with exit 0 accepts anything.
  - It launches its own `MAP`, and the kit's `map` check FAILs a run that loaded another map:
    - `kit.MOVEMENT_TEST_MAP` (no enemies): us001, us083, us069.
    - `kit.COMBAT_TEST_MAP` (a dummy and a fixed-cadence attacker): us054_combat, us057, us078.
    - `kit.DEFAULT_MAP` (DevTestMap, the Frontliner arena): us054_arena, us079.
  - Pick the map whose contents the story's checks need. A story whose checks need two maps owns two scripts, each with its own `RUN_SUFFIX` and its own Steps table (`Specs/story_playtests_v1.md` §3).
- `py Scripts/playtest_story.py US-XXX` runs the Tier 3 boot of each of the story's maps and its Tier 4 script(s) on the same package and writes `Saved\Playtests\<ts>-story-usXXX\summary.md`. Use it for Steps 4c and 4d.
- `py Scripts/playtest_story.py US-XXX --plan` prints the Steps table for the story's Playtest section.
- `py Scripts/playtest_story.py US-XXX --lint` checks that the story and its script agree.
- `py Scripts/playtest_story.py --all` is the Playability Recovery sweep over every script. It boots each launch path once at Tier 3 (DevTestMap, MovementTestMap, CombatTestMap). It reports per-feature Playable Status lines and marks each failure NEW or known.
- `known_issue` marks a check that is expected to FAIL because of a filed bug. The check still FAILs and still exits 1; the report just shows it as known instead of NEW.
- Exploratory play (not evidence): `py Scripts/explore_playtest.py --minutes 20` runs a headless Claude session with only the `mordecai-play` tools. It files findings and runs crash, stuck, log and blank-frame detectors, then writes `Saved\Playtests\<ts>-explore\report.md`.

**2. Run:**
```powershell
# Tier 3 + Tier 4 for one story, on the same package (preferred)
py Scripts/playtest_story.py US-XXX

# The story's real-input playtest alone: launches the package with -AgentBridge, plays it, quits
py Scripts/playtests/usXXX_<name>.py

# The first one: US-079 inventory (walk to the 6 pickups, F, I, click Identify, Escape)
py Scripts/playtests/us079_inventory.py

# Leave the game running afterwards (then close it: step 6 below)
py Scripts/playtests/us079_inventory.py --keep-running
```
- Exit 0 = every check passed (WARN allowed). Exit 1 = gameplay failure: the game did not do what a player expects (a key did nothing, a menu did not open or close, a `wait_until` timed out). Exit 2 = infrastructure: STALE BUILD, launch failure, no bridge connection, crash.
- Each run writes `Saved\Playtests\<YYYYmmdd-HHMMSS>-usXXX\`: `report.md` and `report.json` (every player step is a PASS/FAIL/SKIP/WARN check), `game.log`, `bridge.jsonl` (every request and response — the audit trail that only input and state commands were sent) and `bridge_NN.png` (the game's own frames, like `engine_NN.png`).
- It works on a locked machine or with the window unfocused. The bridge only emulates the environment of a focused window: Slate keeps accepting device input while the app is inactive, user-0 focus goes back to the game viewport only when nothing has focus, and mouse/click use an in-memory cursor. `report.md` records `state.focus` at the start and end, and every input response carries `env` (`focus_restored`, `virtual_cursor`, `app_active`), so the report says which environment the run had.

**3. Input only through the real-input commands:**
- Drive the game only with `key`, `axis`, `mouse` and `click` (client helpers `tap`, `press`/`release`, `hold`, `axis`, `mouse`, `click`, `click_button` and `walk_to` are built from them). Read with `state`, `wait`, `wait_until` and `screenshot`.
- Any future `debug_*` bridge command (its response carries `"debug": true`), a `key` with `route:"viewport"` (`"diagnostic": true`), remote exec, Lyra's RPC registry or a cheat/console command is never evidence — the same rule as Tier 2's `[LOGIC]` checks.
- A FAIL is a game bug to fix or file, never something to work around in the script. US-079's first run found that Escape cannot close the pause menu.

**4. Look at the screenshots (MANDATORY):** Read `report.md`, then open EVERY `bridge_NN.png` with the Read tool and state what is on screen: is the menu really open, the row really renamed, the pickup really gone? `report.md` lists what each screenshot should show; compare them. A near-uniform-frame WARN means you have no visual evidence for that step.

**5. Cite the evidence:** the absolute run dir path, the PASS/FAIL per check, and a one-line description of each `bridge_NN.png` go in the story's completion notes and the body of the closing commit (Step 6), next to the Step 4c evidence.

**6. Close the game:** the playtest scripts launch the game in a kill-on-close job, so it normally exits with them. After a playtest script's `--keep-running`, run the quit command it prints (`py Scripts/agent_bridge.py --port <port> --token <token> quit`) or the Step 7 `Stop-Process`; plain `py Scripts/agent_bridge.py quit` only closes a CLI `launch` session (it reads `Saved\Playtests\agent_bridge_session.json`, which only `launch` writes). A running game blocks the next `package_playtest.py`.

**Exploring and writing the script (not evidence by itself):**
```powershell
py Scripts/agent_bridge.py launch [--map M | --frontend]   # detached; STALE gate; writes Saved\Playtests\agent_bridge_session.json
py Scripts/agent_bridge.py ping
py Scripts/agent_bridge.py state --sections player,pickups,inventory
py Scripts/agent_bridge.py press F                         # a tap; --hold 0.1
py Scripts/agent_bridge.py walk-to 400 0                   # closed-loop WASD walk; --tol 50
py Scripts/agent_bridge.py click-button "Identify"
py Scripts/agent_bridge.py wait-until "inventory.count>=4" --timeout 10
py Scripts/agent_bridge.py screenshot
py Scripts/agent_bridge.py quit                            # ALWAYS: the detached game outlives the CLI
```
- Also: `axis Gamepad_LeftY=1 --hold 0.5`, `mouse X Y`, `click [X Y]`, `wait 1.5`. CLI exit 0 = ok, 1 = gameplay failure (bridge `timeout` or `bad_key`, `walk_to` stuck or mapping mismatch, wrong experience, button not on screen), 2 = infrastructure (STALE BUILD, launch failure, no connection, no or stale session). Run dir `Saved\Playtests\<YYYYmmdd-HHMMSS>-bridge\`.
- The client picks a free `127.0.0.1` port for every launch and passes `-AgentBridgePort=` (the game's default is 47777), plus a token that `ping` echoes back.
- **MCP server `mordecai-play`** (`Scripts/mordecai_play_mcp.py`, registered project-scoped in the committed `.mcp.json`; tools `mcp__mordecai-play__*`: `launch_game`, `get_state`, `press_key`, `hold_key`, `release_key`, `walk_to`, `click`, `click_button`, `screenshot`, `wait_until`, `quit_game`). Install the SDK once with `py -m pip install -r Scripts/requirements.txt`. An interactive Claude Code session asks once to approve the project server; Non-interactive sessions (`claude -p`, the Agent SDK) may load it without asking, depending on their settings; if the `mcp__mordecai-play__*` tools are missing, approve the server or pre-approve it in personal settings. It owns one game in a kill-on-close job and writes `Saved\Playtests\<YYYYmmdd-HHMMSS>-mcp\`. Call `quit_game` when done.
- CLI and MCP sessions are for exploring and for writing the playtest. Acceptance comes only from the story's `Scripts/playtests/usXXX_<name>.py` run on a fresh package.

### Step 5: Commit and Push (MANDATORY — before Step 4c packages)
- Only commit when build succeeds AND Tier 1 + Tier 2 tests pass
- Commit and push the work BEFORE Step 4c: the package must be built from the commit you deliver. Step 6 adds the closing commit with the playtest evidence afterwards.
- **Commit content with code.** Every `.uasset`/`.umap` (Git LFS) your story created or changed goes in the same branch/PR as the code and scripts that need it. Asset-creating editor scripts must save (`unreal.EditorAssetLibrary.save_asset(...)`, save the level) — content that exists only in editor memory, or on disk but uncommitted, does not exist.
- Run the content guard before AND after committing:
```powershell
py Scripts/check_content_committed.py
py Scripts/check_content_committed.py --base <ref> --json   # base defaults to origin/main
```
  - Before committing: it lists every saved-but-uncommitted asset — stage all of them.
  - After committing: it MUST exit 0 before you push and before Step 4c packages. Exit 1 = assets left uncommitted, or asset-writing `Scripts/*.py` changed on this branch with zero `.uasset`/`.umap` changes — run the script in the editor, save, commit the assets. Exit 2 = git error/bad ref — fix the ref; do not skip the check.
  - `[no-content]` exempts an asset-writing script from the scripts-without-assets check only if EVERY commit on the branch that touched that script carries the tag in its message and the script has no uncommitted changes. Use it only when the script change truly produces no asset change, and say why in that same message.
- If you change anything under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject` after packaging (e.g. a fix), commit it and re-run Step 4c and Step 4d — the earlier playtests no longer cover the delivered commit.
- Commit message format: `[US-XXX] Brief description of what was implemented`
- The playtest evidence (Step 4c and Step 4d run dirs, each result PASS or FAIL, and what the screenshots show) goes in the body of the closing commit made after the playtests (Step 6)
- Group related changes into logical commits (not one giant commit)
- **Push to remote after every commit**: `git push`
- **Never leave uncommitted work** — if you're done or stopping, commit and push what you have

### Step 6: Update Story (Only After the Packaged and Real-Input Playtests Pass)
- Check off completed ACs in the story file
- Add completion notes: Step 4c and Step 4d run dirs, PASS/FAIL per check, a one-line description of each screenshot (`engine_loading.png`, `engine_NN.png` and `bridge_NN.png`), and any AC recorded `PENDING TIER 4 (bridge cannot drive: <reason>)` (not claimed)
- Move story from `stories/in-progress/` to `stories/done/` ONLY when ALL of these hold:
  - All ACs pass at Tier 1 and Tier 2
  - `playtest_boot.py` exited 0 on a fresh package of the delivered commit (no STALE BUILD, no `--allow-stale`)
  - Story with ACs that need player input: its `Scripts/playtests/usXXX_<name>.py` exited 0 on that same fresh package (no STALE BUILD, no `--allow-stale`), and every input-driven AC the bridge can drive was accepted by that run. An AC recorded `PENDING TIER 4 (bridge cannot drive: <reason>)` stays unchecked and is not claimed; the story may still move to done, its Playable Status says the input is not yet accepted, and the bridge gap is added to the Known issues backlog in `Specs/agent_playtest_plan_v1.md`. If no input AC can be driven, no Tier 4 script is required.
  - You opened every screenshot (`engine_loading.png`, `engine_NN.png` and `bridge_NN.png`) and described what is on screen
  - `check_content_committed.py` exits 0
- If the packaged or real-input playtest failed or was not run, the story stays in `stories/in-progress/` — record the failure and the run dir in the story
- HEADLESS story whose feature needs content: name the paired EDITOR story in the completion notes and record `NOT YET PLAYABLE — pending US-XXX`. The feature — and the epic's playable milestone — is not met until both land and the packaged playtest shows it.
- **Closing commit:** commit and push the story update/move (`[US-XXX] ...`), with the Step 4c and Step 4d evidence in the commit body. It must touch only `stories/` — nothing under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject` (otherwise re-package and re-run Step 4c and Step 4d). A failed playtest gets a closing commit too: the story stays in `stories/in-progress/` with the failure and run dir recorded.

### Step 7: Close the Editor (MANDATORY)
- After all work is complete (committed and pushed), **close the Unreal Editor** if it is running
- Use the following command to gracefully shut it down:
```powershell
Stop-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
```
- This applies to **every story**, regardless of Execution Mode — if PIE smoke tests launched the editor, close it when done
- If the packaged game is still running — `playtest_boot.py --keep-running`, a playtest script's `--keep-running`, a detached `py Scripts/agent_bridge.py launch`, or an MCP `launch_game` without `quit_game` — close it: `py Scripts/agent_bridge.py quit` for a CLI session, otherwise `Stop-Process -Name "LyraGame" -ErrorAction SilentlyContinue`

---

## Test Standards

### Naming Convention
```cpp
// Test class: Mordecai.[Feature].[Behavior]
// Examples:
// Mordecai.Combat.DodgeGrantsIFrames
// Mordecai.Attributes.HealthClampedToMax
// Mordecai.Camera.FollowsCharacterWithoutRotating
```

### Test Organization
```
Source/LyraGame/Tests/
├── Attributes/
├── Camera/
├── Character/
├── Combat/
├── Foundation/
├── Input/
├── Level/
├── Playtest/      (agent bridge + playtest capture: Mordecai.Playtest.*)
└── UI/            (and the other feature folders: Items, HUD, Magic, ...)
```

### What Makes a Good Test
- Tests one behavior, named clearly
- Fails for the right reason before implementation
- Passes after implementation without modification
- Does not depend on editor state or specific assets (use mock/stub data)
- Runs headless (NullRHI compatible)

---

## Code Standards

### Naming
- Classes: `UMordecai*`, `AMordecai*`, `FMordecai*` prefix for project-specific classes
- Gameplay Tags: `Mordecai.Category.Subcategory.Name`
- Plugins: `MordecaiCore`, `MordecaiCombat`, etc.

### Architecture Rules
- GAS for all abilities, attributes, status effects
- Enhanced Input for all player input
- Data-driven where possible (DataAssets, DataTables)
- Server-authoritative with Iris replication

### Files
- New systems go in appropriately named subdirectories under `Source/LyraGame/Mordecai/`
- Keep header includes minimal
- One class per file (with reasonable exceptions for small helper types)

---

## Story Pipeline
Stories live in `stories/` and use a kanban flow:
- `stories/backlog/` — scoped and ready to pick up
- `stories/in-progress/` — currently being worked on
- `stories/done/` — completed: all ACs passing AND the Tier 3 packaged playtest of the delivered commit passed AND, for ACs that need player input, the story's Tier 4 real-input playtest passed on that same package (Step 6)
- `stories/PLAN.md` — epic breakdown and priority order
- Story template: section 7 of `stories/PLANNER.md` (`stories/TEMPLATE.md` no longer exists — do not recreate it). Every story has a **Playtest** section and a Definition of Done that includes the Tier 3 packaged playtest and, for input-driven ACs, the Tier 4 real-input playtest (`Scripts/playtests/usXXX_<name>.py`)

---

## What NOT To Do
- Never skip writing tests first
- Never commit with failing tests
- Never commit with build errors
- Never guess on open items (use TODO(DECISION))
- Never add BuildGraph, Horde, Gauntlet, or CI infrastructure — packaging and playtests stay local Python scripts
- Never gate gameplay setup on `World->IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings` (ExperienceOverride is PIE-only), or any editor-only state — every Mordecai map must resolve the Mordecai experience in a packaged build
- Never move a story to done on PIE-only evidence — Tier 1/Tier 2 passing is not done; the Step 4c packaged playtest must pass and its screenshots must be read, and so must the Step 4d real-input playtest for every AC that needs input
- Never commit an asset-creating editor script without the saved `.uasset`/`.umap` files it produces, in the same branch
- Never fake player input in acceptance checks (teleport + `try_pickup_focused()`, direct `toggle_pause_menu_to_tab()`, calling what an input action would call) — those are `[LOGIC]` checks only
- Never cite a `debug_*` bridge command, a `route:"viewport"` key, or any direct-call result as Tier 4 evidence — Tier 4 evidence is the bridge's real-input commands (key/axis/mouse/click) in the story's playtest run
- Never run Tier 4 acceptance on a STALE BUILD or with `--allow-stale`
- Never use `playtest_boot.py --allow-stale` for acceptance evidence, and never report a FAIL, a warning, or an unread screenshot as a pass
- Never implement camera rotation
- Never create status effect systems outside GAS
- Never implement bag-based inventory (use unlimited carry + town gating per agent_rules_v2)
