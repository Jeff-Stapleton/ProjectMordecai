# CLAUDE.md — Project Mordecai Development Rules

## Project Overview
UE 5.7 diorama-style ARPG built on Lyra. Fixed camera, twin-stick controls, GAS-based combat, Iris replication.

## Critical References
Read these before starting any work:

### Core Rules & Design
- **Agent Rules (MUST READ FIRST):** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\agent_rules_v2.md`
- **Game Design v2:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\game_design_v2.md`
- **UE5 Tech Stack:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\ue5_tech_stack_context.md`

### Combat & Abilities
- **Attack Taxonomy:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\attack_taxonomy_v1.md`
- **Combat System:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\combat_system_v1.md`
- **Ability Schema:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\ability_schema_v1.md`
- **Ability System:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\ability_system_v1.md`
- **Damage Types:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\damage_types_v1.md`
- **Player Attacks:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\player_attacks_agent_brief_v1.md`

### Attributes & Status Effects
- **Character Attributes:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\character_attributes_v1.md`
- **Attribute Scaling:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\attribute_scaling_v1.md`
- **Stat Formulas:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\stat_formulas_v1.md`
- **Status Effects:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\status_effects_v1.md`
- **Status Effects (Launch Set):** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\status_effects_launch_set_v1.md`

### Skills & Items
- **Skill Sheet v1.1:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\skill_sheet_v1.1.md`
- **Item Schema v2:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\item_schema_v2.md`
- **Item System:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\item_system_v1.md`

### Input & Controls
- **Control Bindings v1.1:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\control_bindings_v1.1.md`

### Enemies & AI
- **Enemy Archetypes:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\enemy_archetypes_v1.md`
- **Enemy AI Framework:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\enemy_ai_framework_v1.md`

### World & Village
- **World Design:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\world_design_v1.md`
- **Village System:** `C:\Users\jeffd\.openclaw\workspace\projects\mordecai\docs\village_system_v1.md`

## UE5 Reference Skills
Read these when working on the relevant area — they contain patterns, pitfalls, and code templates:

- **UE5 C++ Patterns** (types, macros, delegates, smart pointers, replication):
  `C:\Users\jeffd\.openclaw\workspace\skills\unreal-cpp\SKILL.md`
  - References: `actor-lifecycle.md`, `containers.md`, `delegates.md`, `replication.md`, `smart-pointers.md`, `string-types.md` (same directory `/references/`)

- **GAS Framework** (abilities, attributes, effects, gameplay cues, ability tasks):
  `C:\Users\jeffd\.openclaw\workspace\skills\unreal-gas\SKILL.md`
  - References: `ability-tasks.md`, `attributes.md`, `common-patterns.md`, `gameplay-abilities.md`, `gameplay-effects.md` (same directory `/references/`)

- **Testing** (automation framework, spec/BDD tests, screenshot tests, running from CLI):
  `C:\Users\jeffd\.openclaw\workspace\skills\unreal-testing\SKILL.md`
  - References: `automation-driver.md`, `automation-spec.md`, `fbx-testing.md`, `screenshot-comparison.md` (same directory `/references/`)

- **Python Editor Scripting** (editor automation, asset creation, level setup):
  `C:\Users\jeffd\.openclaw\workspace\skills\unreal-python-editor\SKILL.md`
  - References: `actor-subsystem.md`, `asset-library.md`, `common-classes.md`, `custom-classes.md`, `level-blockout.md`, `level-subsystem.md`, `remote-execution.md` (same directory `/references/`)
  - Remote exec script: `C:\Users\jeffd\.openclaw\workspace\skills\unreal-python-editor\scripts\ue_remote_exec.py`

**When to read which skill:**
- Writing ANY C++ → read `unreal-cpp` SKILL.md first
- Writing GAS code (abilities, attributes, effects) → also read `unreal-gas` SKILL.md
- Writing tests → read `unreal-testing` SKILL.md
- Creating editor assets via Python → read `unreal-python-editor` SKILL.md

## Project Paths
- **Project Root:** `C:\Users\jeffd\Documents\Gamedev\ProjectMordecai`
- **Engine:** `C:\Users\jeffd\Documents\Gamedev\UnrealEngine`
- **Solution:** `ProjectMordecai.sln`
- **MordecaiCore Plugin:** `Plugins\GameFeatures\MordecaiCore`

## Build Commands (Headless — No GUI Editor Needed)
```powershell
# Compile (Development Editor, Win64)
& "C:\Users\jeffd\Documents\Gamedev\UnrealEngine\Engine\Build\BatchFiles\Build.bat" ProjectMordecaiEditor Win64 Development -Project="C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\ProjectMordecai.uproject" -WaitMutex -FromMsBuild

# Run Automation Tests (headless — launches its own editor instance)
& "C:\Users\jeffd\Documents\Gamedev\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\ProjectMordecai.uproject" -ExecCmds="Automation RunTests Mordecai" -Unattended -NullRHI -NoSound -NoSplash -Log -LogCmds="LogAutomationTest Display" -TestExit="Automation Test Queue Empty"
```

These commands are fully headless. They do NOT require the GUI Unreal Editor to be running.

## Editor Launch (For EDITOR-Mode Stories Only)
Some stories require the GUI Unreal Editor running (creating DataAssets, Blueprints, level setup via Python).

### How to launch the editor:
```powershell
Start-Process "C:\Users\jeffd\Documents\Gamedev\UnrealEngine\Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList "C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\ProjectMordecai.uproject"
```

### How to check if it's ready:
```powershell
py "C:\Users\jeffd\.openclaw\workspace\projects\mordecai\scripts\check_editor.py" --wait --timeout 300
```
This pings the Python remote execution endpoint (UDP 239.0.0.1:6766). Exit code 0 = ready.

### How to run Python scripts against the running editor:
```powershell
py "C:\Users\jeffd\.openclaw\workspace\skills\unreal-python-editor\scripts\ue_remote_exec.py" --file path\to\script.py
```

### When you need the editor:
- Check the story's **Execution Mode** field
- If `EDITOR`: launch the editor, wait for ready, then do your work
- If `HEADLESS`: do NOT launch the editor — compile and test from CLI only

---

## Workflow: TDD Pipeline (MANDATORY)

Every task follows this sequence. No exceptions.

### Step 1: Understand
- Read the story file in `stories/in-progress/`
- Read all referenced design docs
- Understand every acceptance criterion (AC)

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

### Step 4: Verify (MANDATORY — Do Not Skip)
- Compile the full project first:
  ```powershell
  & "C:\Users\jeffd\Documents\Gamedev\UnrealEngine\Engine\Build\BatchFiles\Build.bat" ProjectMordecaiEditor Win64 Development -Project="C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\ProjectMordecai.uproject" -WaitMutex -FromMsBuild
  ```
- Then run ALL tests (not just new ones):
  ```powershell
  & "C:\Users\jeffd\Documents\Gamedev\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\ProjectMordecai.uproject" -ExecCmds="Automation RunTests Mordecai" -Unattended -NullRHI -NoSound -NoSplash -Log -LogCmds="LogAutomationTest Display" -TestExit="Automation Test Queue Empty"
  ```
- Both are **headless** — they do NOT need the GUI editor
- Both must pass with zero errors
- If tests fail or build breaks, **fix before proceeding** — do not commit broken code

### Step 5: Commit and Push (MANDATORY)
- Only commit when build succeeds AND tests pass
- Commit message format: `[US-XXX] Brief description of what was implemented`
- Group related changes into logical commits (not one giant commit)
- **Push to remote after every commit**: `git push`
- **Never leave uncommitted work** — if you're done or stopping, commit and push what you have

### Step 6: Update Story
- Check off completed ACs in the story file
- If all ACs are done, move story from `stories/in-progress/` to `stories/done/`
- Commit and push the story file move too

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
├── Combat/
│   ├── MordecaiDodgeTests.cpp
│   ├── MordecaiBlockTests.cpp
│   └── MordecaiPostureTests.cpp
├── Attributes/
│   └── MordecaiAttributeTests.cpp
├── Camera/
│   └── MordecaiCameraTests.cpp
├── Input/
│   └── MordecaiInputTests.cpp
└── Inventory/
    └── MordecaiInventoryTests.cpp
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

### Architecture
- GAS for all abilities, attributes, status effects
- Enhanced Input for all player input
- Data-driven where possible (DataAssets, DataTables)
- Server-authoritative with Iris replication

### Files
- New systems go in appropriately named subdirectories under `Source/LyraGame/`
- Keep header includes minimal
- One class per file (with reasonable exceptions for small helper types)

---

## Story Pipeline
Stories live in `stories/` and use a kanban flow:
- `stories/backlog/` — scoped and ready to pick up
- `stories/in-progress/` — currently being worked on
- `stories/done/` — completed with all ACs passing
- `stories/PLAN.md` — epic breakdown and priority order
- `stories/TEMPLATE.md` — story file template

---

## What NOT To Do
- Never skip writing tests first
- Never commit with failing tests
- Never commit with build errors
- Never guess on open items (use TODO(DECISION))
- Never add BuildGraph, Horde, or CI infrastructure
- Never implement camera rotation
- Never create status effect systems outside GAS
- Never implement bag-based inventory (use unlimited carry + town gating per agent_rules_v2)
