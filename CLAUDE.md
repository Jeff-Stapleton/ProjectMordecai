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

Packaging and the packaged playtest (Tier 3) commands are in **Step 4c** below.

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
- If `HEADLESS`: do NOT launch the editor for development — compile and test from CLI. The one exception is the Tier 2 PIE smoke run (Step 4b): `run_pie_verify.py` launches the editor for it, and you close it afterwards (Step 7). Packaging and the packaged playtest (Step 4c) do not use the editor.

---

## Workflow: TDD Pipeline (MANDATORY)

Every task follows this sequence. No exceptions.

### Verification Ladder
**Playable in the packaged build, or it isn't done.** Each tier proves something the tier below cannot. A lower tier NEVER substitutes for a higher tier.

1. **Tier 1 — Headless automation tests** (Step 4): logic. NullRHI, no rendering, no real content.
2. **Tier 2 — PIE smoke tests** (Step 4b): editor integration (spawn chain, components, widgets). Runs on in-editor memory, including unsaved assets and PIE-only settings.
3. **Tier 3 — Packaged playtest** (Step 4c): `py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`, then Read the screenshots. Proves a player launching the packaged game gets the Mordecai experience and the feature is on screen.
4. **Tier 4 — Real-input playtest** (Phase 2 — planned, NOT available yet): an in-game agent bridge compiled into non-Shipping builds that injects real key/mouse input at the Slate/viewport level (so Enhanced Input mappings are exercised), returns state JSON + screenshots, and is exposed as an MCP server; plus per-story real-input playtest scripts.

- **Definition of Done** for any story with player-facing behavior: Tier 3 on a FRESH package built from the commit you deliver shows the feature working, reached the way a player reaches it, and you have opened every screenshot and stated what is visibly on screen.
- **Order, every story:** Step 4 (Tier 1) → Step 4b (Tier 2) → commit and push code AND content (Step 5 rules) → Step 4c packages that commit and playtests it (Tier 3) → Step 6 closing commit records the evidence and moves the story.
- Content is real only when it is SAVED and COMMITTED (Step 5).
- Until Tier 4 exists, confirm everything Tier 3 can see; ACs that need player input are recorded as `PENDING TIER 4 (real input)` — never claimed as playtested.

### Step 1: Understand
- Read the story file in `stories/in-progress/`
- Read all referenced design docs
- Understand every acceptance criterion (AC)
- Read the story's **Playtest** section: launch path (front end or direct map + expected experience), the player's steps, what must be visible on screen, and which playtest script/run proves it. If a player-facing story has none, write one into the story (use `control_bindings_v1.1.md` for keys) before Step 2 and say so in your completion notes

### Step 2: Write Tests FIRST
- For every AC, write at least one automation test that verifies it
- Tests go in `Source/LyraGame/Tests/` organized by feature area
- Use UE Automation Test Framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` or `BEGIN_DEFINE_SPEC` / `END_DEFINE_SPEC`)
- Tests MUST fail before implementation (red phase)
- Run tests to confirm they fail for the right reasons

### Step 3: Implement
- Write the minimum code to make tests pass
- Follow agent_rules_v2.md constraints at all times
- Use `TODO(DECISION)` for open items — never guess

### Step 4: Verify — Tier 1 (MANDATORY — Do Not Skip)
- Compile the full project (see Build & Test Commands above)
- Then run ALL Mordecai tests (not just new ones): `py Scripts/run_tests.py`
- Both must pass with zero errors (`run_tests.py` exit 0)
- If tests fail or build breaks, **fix before proceeding** — do not commit broken code
- Passing here proves logic only. Continue to Step 4b and Step 4c.

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

# Or run individual smoke tests manually during PIE:
py "Scripts/ue_remote_exec.py" "exec(open(r'Scripts/verify_pie_hud.py').read())"
py "Scripts/ue_remote_exec.py" "exec(open(r'Scripts/verify_pie_spawn.py').read())"
```

**PIE mode:** Use `editor_request_begin_play()` (NOT `editor_play_simulate()` which starts spectator mode).

**If remote exec is unreliable:** Open the editor's Python console (`Window > Developer Tools > Output Log > Cmd dropdown > Python`) and paste the smoke test script content directly.

**Available smoke tests** (in `Scripts/`): `run_pie_verify.py` runs every `Scripts/verify_pie_<name>.py` it finds (`--tests <name>,...` selects). Examples:
- `verify_pie_hud.py` — Checks combat HUD widget tree: root visible, all child bars exist
- `verify_pie_spawn.py` — Checks spawn chain: controller, pawn, components, camera, ASC

**Smoke tests must not fake the player:**
- DO assert on state a player would see: actors placed in the level, components present, widgets bound and visible, input actions mapped in the active IMC.
- DO tag every check that calls gameplay functions directly with `[LOGIC]` in its PASS/FAIL line — e.g. teleporting the pawn and calling `interaction.try_pickup_focused()`, or calling `subsystem.toggle_pause_menu_to_tab('inventory')` (both in `verify_pie_inventory.py`, which does not tag them yet — add the tag when you touch it). These are logic/diagnostic checks, not proof of playability.
- DO NOT cite a `[LOGIC]` check — or any Tier 2 result — as acceptance of a player-facing AC. Player-facing ACs are accepted at Tier 3 (Step 4c) now, and with real input at Tier 4 when it exists.

**Writing new smoke tests:**
When you implement a feature that has runtime/visual behavior, add a `Scripts/verify_pie_<feature>.py` smoke test. Follow the pattern in existing verify scripts: get PIE world, check invariants, print PASS/FAIL lines, raise RuntimeError on any failure. Follow the rules above.

### Step 4c: Packaged Playtest — Tier 3 (MANDATORY — Run Every Time)
The packaged game is what a player runs: no PIE, no `WITH_EDITOR` code, no `LyraDeveloperSettings`, and only content that is saved on disk and cooked. Run this for **every story**, regardless of Execution Mode, after Step 4b passes and the work is committed. For a story with no player-facing behavior it proves the delivered commit still packages and boots to the expected experience; for a player-facing story it must also show the feature. Packaging is slow (a first or `--clean` package can take an hour+; iterative cooks are faster) — that cost is accepted. Run it in the background and poll the log; do not let a tool timeout kill it.

**1. Commit first, then package that commit:**
- Step 4's `LyraEditor` compile must be current — the cook runs on the editor binaries.
- Save every dirty asset in the editor, then close the editor (Step 7 command). The cook reads from disk; unsaved editor memory does not ship, and a running editor with Live Coding blocks the package build.
- Commit and push code AND content following the Step 5 rules (`check_content_committed.py` exit 0). `git status` must be clean: `package_playtest.py` records the commit in `build_info.json`, and a build with `"dirty": true` is a diagnostic build, not delivery evidence.
- While the package builds and until the playtest is done, do not edit or commit anything. A new commit (HEAD != packaged commit), an uncommitted change under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject`, or an edit made while UAT was running makes `playtest_boot.py` report STALE BUILD. A fix means: commit → re-package → re-run.

**2. Package** (Development Win64 → `PackagedBuilds\Playtest\`):
```powershell
# Default: iterative cook of the front end + DevTestMap
py Scripts/package_playtest.py

# Full (non-iterative) cook — use when an iterative cook looks wrong
py Scripts/package_playtest.py --clean

# Cook every map (-CookAll -CookMapsOnly) in addition to front end + DevTestMap
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

# Real player entry point (L_LyraFrontEnd, no map arg), expects B_LyraFrontEnd_Experience
py Scripts/playtest_boot.py --frontend

# Another map / experience
py Scripts/playtest_boot.py --map <MapPath> --expect-experience <ExperienceName>

# More or later screenshots
py Scripts/playtest_boot.py --settle 10 --shots 5 --shot-interval 2.0
```
- Exit 0 = all checks passed.
- Exit 1 = gameplay failure: wrong experience (`B_LyraDefaultExperience` means the Mordecai experience did NOT load), crash, timeout, or `content_cooked` FAIL. That check fails when the game asked for a `/MordecaiCore` asset that is not in the build, usually content loaded from C++ by a path string; PIE can't catch it. Fix the cause, then repeat Step 4 → 4b → 5 (commit) → 4c.
- Exit 2 = infrastructure: exe missing, **STALE BUILD**, window not found, git error. On STALE BUILD, commit if needed and re-run `package_playtest.py` — do not work around it.
- NEVER use `--allow-stale` for acceptance evidence. It is for diagnosis only, and the report records it.
- Use `--frontend` whenever the story touches the front end, menus, or playlists (how a player reaches a map).
- Each run writes `Saved\Playtests\<YYYYmmdd-HHMMSS>-boot\`: `report.md`, `report.json`, `game.log`, `engine_01.png`, ... (saved by the game itself from its own viewport, via `-PlaytestCapture` / `UMordecaiPlaytestCaptureSubsystem`) and `shot_01.png`, ... (desktop capture of the game window)
- `engine_NN.png` is the primary evidence: it works when the machine is locked or the window is covered, as it is on unattended runs. `shot_NN.png` is all black whenever the workstation is locked, so treat a black `shot_NN.png` with good `engine_NN.png` as expected. A missing or black `engine_NN.png` (`engine_screenshots` FAIL/WARN) means you have no visual evidence.

**4. Look at the screenshots (MANDATORY):**
- Read `report.md`, then open EVERY `engine_NN.png` (and any non-black `shot_NN.png`) with the Read tool.
- State what is actually on screen: the Mordecai diorama arena or a Lyra shooter HUD? The Mordecai HUD? The character? The content your story added (pickups, enemies, widgets)? A black/uniform-screenshot warning means you saw nothing — investigate; it is not a pass.
- Check the report's top errors. An error your change introduced (missing asset, failed load, `LogMordecai*` error) is a defect to fix, not a footnote.
- Compare against the story's **Playtest** section. Everything listed there that does not need player input must be visibly confirmed.
- Phase 1 sends no input. ACs that need keys/movement/mouse are recorded as `PENDING TIER 4 (real input)` — never claimed as playtested.

**5. Cite the evidence:** the absolute run dir path, the PASS/FAIL per check, and a one-line description of each screenshot go in the story's completion notes and the body of the closing commit (Step 6). Report failures honestly — never paper over a FAIL, a warning, or a screenshot that does not show the feature.

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
- If you change anything under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject` after packaging (e.g. a fix), commit it and re-run Step 4c — the earlier playtest no longer covers the delivered commit.
- Commit message format: `[US-XXX] Brief description of what was implemented`
- The playtest evidence (Step 4c run dir, its result PASS or FAIL, and what the screenshots show) goes in the body of the closing commit made after the playtest (Step 6)
- Group related changes into logical commits (not one giant commit)
- **Push to remote after every commit**: `git push`
- **Never leave uncommitted work** — if you're done or stopping, commit and push what you have

### Step 6: Update Story (Only After the Packaged Playtest Passes)
- Check off completed ACs in the story file
- Add completion notes: Step 4c run dir, PASS/FAIL per check, a one-line description of each screenshot, and any ACs marked `PENDING TIER 4 (real input)`
- Move story from `stories/in-progress/` to `stories/done/` ONLY when ALL of these hold:
  - All ACs pass at Tier 1 and Tier 2
  - `playtest_boot.py` exited 0 on a fresh package of the delivered commit (no STALE BUILD, no `--allow-stale`)
  - You opened every screenshot and described what is on screen
  - `check_content_committed.py` exits 0
- If the packaged playtest failed or was not run, the story stays in `stories/in-progress/` — record the failure and the run dir in the story
- HEADLESS story whose feature needs content: name the paired EDITOR story in the completion notes and record `NOT YET PLAYABLE — pending US-XXX`. The feature — and the epic's playable milestone — is not met until both land and the packaged playtest shows it.
- **Closing commit:** commit and push the story update/move (`[US-XXX] ...`), with the Step 4c evidence in the commit body. It must touch only `stories/` — nothing under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject` (otherwise re-package and re-run Step 4c). A failed playtest gets a closing commit too: the story stays in `stories/in-progress/` with the failure and run dir recorded.

### Step 7: Close the Editor (MANDATORY)
- After all work is complete (committed and pushed), **close the Unreal Editor** if it is running
- Use the following command to gracefully shut it down:
```powershell
Stop-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue
```
- This applies to **every story**, regardless of Execution Mode — if PIE smoke tests launched the editor, close it when done
- If you ran `playtest_boot.py --keep-running`, close the packaged game too: `Stop-Process -Name "LyraGame" -ErrorAction SilentlyContinue`

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
└── Level/
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
- `stories/done/` — completed: all ACs passing AND the Tier 3 packaged playtest of the delivered commit passed (Step 6)
- `stories/PLAN.md` — epic breakdown and priority order
- Story template: section 7 of `stories/PLANNER.md` (`stories/TEMPLATE.md` no longer exists — do not recreate it). Every story has a **Playtest** section and a Definition of Done that includes the Tier 3 packaged playtest

---

## What NOT To Do
- Never skip writing tests first
- Never commit with failing tests
- Never commit with build errors
- Never guess on open items (use TODO(DECISION))
- Never add BuildGraph, Horde, Gauntlet, or CI infrastructure — packaging and playtests stay local Python scripts
- Never gate gameplay setup on `World->IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings` (ExperienceOverride is PIE-only), or any editor-only state — every Mordecai map must resolve the Mordecai experience in a packaged build
- Never move a story to done on PIE-only evidence — Tier 1/Tier 2 passing is not done; the Step 4c packaged playtest must pass and its screenshots must be read
- Never commit an asset-creating editor script without the saved `.uasset`/`.umap` files it produces, in the same branch
- Never fake player input in acceptance checks (teleport + `try_pickup_focused()`, direct `toggle_pause_menu_to_tab()`, calling what an input action would call) — those are `[LOGIC]` checks only
- Never use `playtest_boot.py --allow-stale` for acceptance evidence, and never report a FAIL, a warning, or an unread screenshot as a pass
- Never implement camera rotation
- Never create status effect systems outside GAS
- Never implement bag-based inventory (use unlimited carry + town gating per agent_rules_v2)
