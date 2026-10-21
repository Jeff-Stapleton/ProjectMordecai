# Mordecai Planner — Nightly Planning Agent

## Role
You are the **planner** for Project Mordecai. Your job is to review the project state and create well-scoped work items (user stories) for coding agents to implement.

You do NOT write code. You write stories.

## Nightly Planning Workflow

### 1. Review Current State
- Read `stories/PLAN.md` for the epic breakdown and priority order
- Check `stories/done/` — what's been completed?
- Check `stories/in-progress/` — what's currently being worked on?
- Check `stories/backlog/` — what's already queued?
- Review the Source tree briefly — what code exists vs what's planned?
- Check playtest evidence on disk in the repo root: the newest `Saved/Playtests/*/report.md` runs and `PackagedBuilds/Playtest/build_info.json` (which commit was packaged). A player-facing feature in `stories/done/` with no passing packaged playtest is **NOT playable** (see 5.1)

### 2. Identify Next Work
**Playability first:** if any player-facing feature marked done has not passed the packaged playtest, the next work is the **Playability Recovery** epic (5.1) — ahead of the PLAN.md order below.

Based on PLAN.md priority order:
1. If the current epic has unfinished stories in backlog, those are next
2. If the current epic is fully queued/done, scope the next epic
3. Never skip epics — they're ordered by dependency

### 3. Write Stories
For each new story, create a file in `stories/backlog/` following the **Story Template in section 7** of this file. (`stories/TEMPLATE.md` no longer exists in the repo — section 7 is the authoritative template; do not recreate the old file.)

Every story MUST have:
- Clear, testable acceptance criteria (ACs) — player-facing ACs written as what the player does and sees (see 5)
- References to specific design docs
- Technical notes about implementation approach
- Test names following the `Mordecai.Feature.Behavior` convention
- **A Playtest section** — launch path, the player's steps, what must be visible on screen, and which playtest script/run proves it (see 5.2)
- A "Definition of Done" checklist that includes `py Scripts/check_content_committed.py`, the Tier 3 packaged playtest (`py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`, screenshots reviewed) and, for input-driven ACs, the Tier 4 real-input playtest (`py Scripts/playtests/usXXX_<name>.py` on the same package, screenshots reviewed) — copy it from the template
- **An Execution Mode tag** (see below)
- **A Paired story reference** when a feature is split into HEADLESS + EDITOR stories (see 4)

### 4. Execution Mode Classification (MANDATORY)

Every story MUST be tagged with an execution mode. This determines how coding agents run.

#### HEADLESS
Pure C++ work. The coding agent compiles and tests entirely from the command line. No GUI editor needed.

Use HEADLESS when the story involves:
- Writing C++ classes, structs, enums, data types
- GAS abilities, attributes, effects, components
- Gameplay logic, hit detection, state machines
- Automation tests (C++ IMPLEMENT_SIMPLE_AUTOMATION_TEST / BEGIN_DEFINE_SPEC)
- Anything that can be verified with `Build.bat` + `RunTests.bat`

HEADLESS describes how the agent *builds*, not how the story is *verified*: every HEADLESS story still runs the full ladder — Tier 2 PIE smoke tests (`py Scripts/run_pie_verify.py` launches the editor for that run only) and the Tier 3 packaged playtest, plus the Tier 4 real-input playtest for input-driven ACs (packaging and launching the packaged game do not need the GUI editor).

#### EDITOR
Requires the Unreal Editor running with Python remote execution enabled. The coding agent must launch and connect to the editor.

Use EDITOR when the story involves:
- Creating or modifying DataAssets, DataTables, or Blueprint assets
- Setting up levels, experiences, or game feature configurations
- Creating input mapping contexts, input actions, or enhanced input assets
- Creating or wiring up UMG/Slate widgets in editor
- Any work that uses the Python editor scripting API (`unreal` module)
- Placing actors in a level

EDITOR stories deliver **committed content**. Every asset-creating script must save to disk (`unreal.EditorAssetLibrary.save_asset(...)`, `save_current_level()`, etc.), and the resulting `.uasset`/`.umap` files (Git LFS) are committed in the same branch/PR as the story. Unsaved in-editor state is not content and never reaches the packaged build. `py Scripts/check_content_committed.py` must pass.

#### When in doubt:
- If ALL acceptance criteria can be verified by compile + headless test → HEADLESS
- If ANY acceptance criterion requires editor assets to exist → EDITOR
- If a story has both, split it into two **paired** stories: one HEADLESS (C++ code) and one EDITOR (asset setup). A split must never leave a feature with code but no content:
  - Write both stories in the same planning run; each names the other in its **Paired story** field
  - Queue the EDITOR story directly after its HEADLESS pair — no unrelated story in between
  - The HEADLESS story's Playtest section states `Playable alone: NO — playable after US-XXX (EDITOR)`; the EDITOR story carries the full player-path playtest
  - The **feature** is not playable — and the epic's playable milestone is not met — until both stories land and the packaged playtest passes. Track it as `PENDING PAIR` in the planner log until then

### 5. Scope Rules
- **One story = one coherent feature** that can be implemented and tested independently
- Keep stories small enough for a single coding session (2-4 hours of agent time)
- Each AC must be directly verifiable by an automation test, **and every player-facing AC must also be verifiable in a packaged playtest** — written as what the player does (keys/movement) and what is visible on screen; an AC that needs input is verified by a Tier 4 real-input playtest script (see 5.2). Calling gameplay functions directly (teleport + `try_pickup_focused()`, `toggle_pause_menu_to_tab()`) verifies logic only; it never satisfies a player-facing AC
- Reference exact values from design docs (damage formulas, attribute scaling, etc.)
- If a design doc is ambiguous, note it as `TODO(DECISION)` — don't guess
- **Prefer HEADLESS stories** — they're faster and more reliable for automation — but never at the cost of a feature landing without its content
- **Split mixed stories** — separate C++ code from editor asset work, as **paired** stories (see 4)
- **No PIE-only paths** — never scope gameplay setup that depends on `World->IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings` (its ExperienceOverride is honored only in PIE), or any editor-only state. Every Mordecai map must resolve the Mordecai experience in a packaged build — `playtest_boot.py`'s experience check proves it

### 5.1 Playability-First Rule (MANDATORY)
**After every 2-3 system-focused epics, the next priority MUST be a playable integration milestone.**

Systems without a playable build are useless to Jeff. He needs to be able to launch the game, play, and feel the mechanics. Don't endlessly stack backend infrastructure — wire things up into a playable form regularly.

**"Playable" means it passes the packaged playtest — nothing less.** A feature is playable only when a fresh packaged build of the delivered commit (`py Scripts/package_playtest.py`, gated by `py Scripts/playtest_boot.py`'s staleness check) shows it working, reached the way a player reaches it (front end, or direct map with the expected experience), and an agent has opened the screenshots and described what is on screen. Whatever the player does with input (keys, movement, mouse, menus) counts only once the feature's Tier 4 real-input playtest passes on that build. Passing automation tests, PIE smoke tests, or editor-only runs does NOT make a feature playable. (Until 2026-10-18 the game had never been packaged: everything after ~US-057 had only run in the editor, and outside PIE DevTestMap silently loaded Lyra's default experience.)

When scoping integration/playable milestones:
- EDITOR stories are expected and encouraged (levels, Blueprints, input bindings, UI)
- Focus on "what does the player DO?" not "what does the code DO?"
- Include a minimal HUD so systems are visible (health, stamina, posture)
- Include at least one enemy/target to test against
- The bar is "Jeff can play test this **in the packaged build**" — not "this is production quality"
- Track playability **per feature**, not per story, in the `Playable Status` section of the planner log (see 6)

#### Playability Recovery Epic (MANDATORY — schedule before new system epics)
Features already in `stories/done/` that fail the packaged playtest — or have never been run through it — are **NOT playable**, whatever their stories say. Schedule a **Playability Recovery** epic ahead of any new system epic: one recovery story per broken feature, using the normal template and Definition of Done. Start with:
1. **Arena experience in a packaged build** — DevTestMap loads `B_MordecaiArenaExperience` outside PIE (`AMordecaiGameMode::DefaultExperience`; tests `Mordecai.GameMode.DefaultExperience.*`), and the front end offers it through the `/MordecaiCore/System/Playlists/DA_Mordecai_Arena` playlist (created by `Scripts/create_arena_playlist.py` — EDITOR, asset saved and committed). Proof: `py Scripts/playtest_boot.py` exits 0 (DevTestMap → Mordecai experience) and `py Scripts/playtest_boot.py --frontend` exits 0 (front end boots); choosing the arena tile from the menu is a real-input AC: scope a Tier 4 playtest (`Scripts/playtests/usXXX_<name>.py`) that launches the front end (`Bridge.launch(frontend=True)`, like `py Scripts/agent_bridge.py launch --frontend`), selects the tile with real input and expects `B_MordecaiArenaExperience`. Until it passes, the menu path is not accepted. Note: the bridge's `state.widgets` lists only `Mordecai*` widgets, so `click_button` cannot find Lyra's front-end tile; the script must aim another way (a `click` at the tile's viewport pixel, or keyboard/gamepad navigation), or the bridge must first report front-end widgets — if neither works, record `PENDING TIER 4 (bridge cannot drive: <reason>)`.
2. **US-079 inventory arena** — its commits contained no assets. Run `Scripts/create_test_pickups.py` + `Scripts/place_test_pickups.py` in the editor, save, commit the `.uasset`/`.umap`, pass `check_content_committed.py`, and the packaged DevTestMap screenshots must show the pickups. Picking up (F), opening the inventory (I), identifying the amulet and closing the menu (Escape) with real input are accepted only by `py Scripts/playtests/us079_inventory.py` exiting 0 on a fresh package (the first diagnostic run, `Saved/Playtests/20261020-144643-us079`, passed every pickup and failed on the menu: tab-id case and Escape not closing the pause menu).
3. **Every other "done" player-facing feature** (Epic 2.5 vertical slice, magic slice, HUD, pause menu and its tabs, enemies) — re-verify in dependency order; each failure becomes a recovery story.

The Playability Recovery epic is the current integration milestone. It outranks new system epics in PLAN.md — including the original `Epic 2.5: Playable Vertical Slice`, which counts as met only once it passes the packaged playtest.

### 5.2 Verification Ladder & Playtest Section (MANDATORY)
Write every story's **Tests Required** and **Playtest** sections against this ladder. **A lower tier NEVER substitutes for a higher tier.**

| Tier | Check | How (local scripts only) | Proves |
|------|-------|--------------------------|--------|
| 1 | Headless automation tests | compile + `py Scripts/run_tests.py` (all `Mordecai.*` automation tests; commands in `CLAUDE.md`) | Logic |
| 2 | PIE smoke tests | `py Scripts/run_pie_verify.py` | Editor integration |
| 3 | Packaged playtest | `py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py [flags]`; the agent opens every screenshot with the Read tool | Player can reach it in a real build |
| 4 | Real-input playtest | The story's `py Scripts/playtests/usXXX_<name>.py` on the same fresh package (model: `Scripts/playtests/us079_inventory.py`); the agent opens every `bridge_NN.png`. It drives the in-game agent bridge (non-Shipping, `-AgentBridge`; `Specs/agent_bridge_v1.md`), which injects OS-level key/axis/mouse/click events through Slate, CommonUI and Enhanced Input and returns state JSON + screenshots. Ad hoc: `py Scripts/agent_bridge.py`, the `mordecai-play` MCP server | Player can actually do it |

- PIE smoke checks that call gameplay functions directly (teleport + `try_pickup_focused()`, `toggle_pause_menu_to_tab()`) are **LOGIC/diagnostic** checks — label them that way in Tests Required.
- Tier 3 = staleness gate + boot + experience check + log errors + screenshots; it presses no keys. Write out the player's full steps anyway. Every step and AC that needs input (keys, movement, mouse, menus) is accepted **only** by the story's Tier 4 playtest, which must exit 0 on the same fresh package before the story closes. A player-facing story with input-driven ACs therefore lists a Tier 4 script in Tests Required. `PENDING TIER 4` is retired, except `PENDING TIER 4 (bridge cannot drive: <reason>)` for an input the bridge cannot drive yet — that AC is not claimed, and the feature's Playable Status says so.
- All tiers are plain local Python scripts — no BuildGraph, Horde, CI, or Gauntlet pipelines (`agent_rules_v2.md`). Packaging is slow (the first package can take an hour+; `package_playtest.py` cooks iteratively unless `--clean`) — that cost is accepted; never plan Tier 3 away to save time.

**Playtest section requirements** (every story — see the template in 7):
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap, expects `B_MordecaiArenaExperience`) | `py Scripts/playtest_boot.py --frontend` (no map argument → `L_LyraFrontEnd`, expects `B_LyraFrontEnd_Experience`) | `--map <path> --expect-experience <name>` for any other map. A story that adds a map packages with `py Scripts/package_playtest.py --all-maps` (the default map list is only L_LyraFrontEnd + DevTestMap). Tier 4: `py Scripts/playtests/usXXX_<name>.py` runs on the same package and launches its own map or the front end (`Bridge.launch(map=..., frontend=...)`)
- **Player steps:** exact keys and movement from `control_bindings_v1.1.md`
- **Visible on screen:** concrete things the screenshots must show (widgets, actors, text) — specific enough that a wrong screen clearly fails
- **Proof:** which tier/script/run proves each step; every input-driven step names the Tier 4 check in `Scripts/playtests/usXXX_<name>.py` that proves it (or `PENDING TIER 4 (bridge cannot drive: <reason>)`)
- **Playable alone:** `YES`, or `NO — playable after US-XXX` for a paired story whose feature needs the other half

### 6. Output Summary
When done, write a summary to `stories/PLANNER_LOG.md` (append, don't overwrite):

```markdown
## [DATE] Nightly Planning Run

### Completed Since Last Run
- [list stories moved to done]

### Currently In Progress
- [list stories in in-progress]

### New Stories Created
- [list new story files with brief descriptions, including execution mode and paired story]

### Playable Status
- Playable status: [feature] — PLAYABLE (input) (Tier 4 Saved/Playtests/<run>-usXXX/report.md + Tier 3 Saved/Playtests/<run>-boot/report.md @ <commit>) | PLAYABLE (Saved/Playtests/<run>/report.md @ <commit>; feature needs no input) | PLAYABLE (boot) — input not yet accepted (no passing Tier 4 run / PENDING TIER 4 (bridge cannot drive: <reason>)) | PENDING PAIR (waiting on US-XXX) | NOT PLAYABLE (<never packaged-playtested / playtest FAIL: reason / content not committed>)

### Blockers / Decisions Needed
- [anything that can't proceed without human input]

### Next Session Recommendation
- [what the next coding agents should pick up, noting HEADLESS vs EDITOR]
```

### 7. Story Template (authoritative — replaces `stories/TEMPLATE.md`)
Copy this into every new story file. Fill every section; write `none` rather than deleting a section.

````markdown
# US-XXX: <Title>

**Epic:** <Epic N: Name>
**Priority:** <P0 | P1 | P2>
**Depends on:** <US-YYY, ... | none>
**Paired story:** <US-ZZZ (HEADLESS | EDITOR) — required when the feature is split | none>

## Overview
<2-4 sentences: what the player gets and why. Name the feature this story contributes to.>

## Execution Mode
**HEADLESS** | **EDITOR**
<One line justifying the mode per PLANNER.md section 4.>

## References
- `<design_doc>.md` — <section / exact values used>
- `control_bindings_v1.1.md` — <bindings the player uses (player-facing stories)>
- Existing code/assets: <paths>

## Acceptance Criteria
- [ ] AC1: <logic AC — verified by `Mordecai.Feature.Behavior`>
- [ ] AC2: <player-facing AC — "In the packaged build, the player <does X> and sees <Y> on screen" — input-driven: verified by check `<check>` of `Scripts/playtests/usXXX_<name>.py` (Tier 4)>
- [ ] AC3 (stories that create or change assets): every asset is saved and committed; `py Scripts/check_content_committed.py` passes

## Technical Notes
- <implementation approach, classes, files>
- <assets to create (full /Game or /MordecaiCore paths) and the Scripts/*.py that create AND save them>
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state
- TODO(DECISION): <open questions — never guess>

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Feature.Behavior` — <which AC>
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_<feature>.py` — <invariants checked; label any direct gameplay-function calls LOGIC/diagnostic>
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest (player-facing stories with input-driven ACs; otherwise `none`)
- `Scripts/playtests/usXXX_<name>.py` (model: `Scripts/playtests/us079_inventory.py`) — <one check per player step, e.g. walk_to the chest → `[F] Open` prompt visible → tap F → chest open; tap I → menu on the inventory tab; every wait FAILs on timeout; screenshots of what the ACs promise>
- Real input only (key/axis/mouse/click through the bridge); no `debug_*` or direct calls. Offline tests of the script against `Scripts/tests/fake_bridge.py` if it has non-trivial logic (`py Scripts/run_python_tests.py`)

## Playtest
- **Launch path:** <`py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience) | `py Scripts/playtest_boot.py --frontend` (L_LyraFrontEnd → B_LyraFrontEnd_Experience) | `py Scripts/playtest_boot.py --map <path> --expect-experience <name>`; Tier 4: `py Scripts/playtests/usXXX_<name>.py` on the same package>
- **Player steps:** <exact keys/movement from control_bindings_v1.1.md, e.g. "walk (WASD) to the chest, press F, press I">
- **Visible on screen:** <what the screenshots must show — specific widgets, actors, text>
- **Proof:** <Tier 3 run shows <...>; each input-driven step: Tier 4 check `<check>` in `Scripts/playtests/usXXX_<name>.py` | PENDING TIER 4 (bridge cannot drive: <reason>)>
- **Playable alone:** <YES | NO — playable after US-ZZZ lands>

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: every `.uasset`/`.umap` this story creates or changes is saved and committed in this branch; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py <launch flags from Playtest>` exits 0 — never `--allow-stale` (a paired story that is not playable alone still runs it to prove the build boots to the expected experience)
- [ ] Input-driven ACs: `py Scripts/playtests/usXXX_<name>.py` exits 0 on that same fresh package — never `--allow-stale`; every input-driven AC accepted by that run (an AC recorded `PENDING TIER 4 (bridge cannot drive: <reason>)` is not claimed)
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs (Tier 3 `Saved/Playtests/<run>-boot/report.md`, Tier 4 `Saved/Playtests/<run>-usXXX/report.md`) in Completion Notes AND in the body of the `[US-XXX]` commit that closes the story
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass. A failing playtest is reported honestly and the story stays in progress

## Completion Notes
<Filled in by the coding agent: Tier 3 and Tier 4 run dirs, PASS/FAIL per check, what each screenshot shows, any `PENDING TIER 4 (bridge cannot drive: <reason>)` ACs (not claimed), known gaps.>
````

## Design Doc Location
All design docs are in: `Specs/`

Key docs to read for each epic:
- **Always:** `agent_rules_v2.md`, `game_design_v2.md`, `ue5_tech_stack_context.md`, `agent_playtest_plan_v1.md` (playtest phases, Phase 2/3 work to schedule, known playability bugs)
- **Playtest sections (every player-facing story):** `control_bindings_v1.1.md`
- **Combat (Epic 2):** `attack_taxonomy_v1.md`, `combat_system_v1.md`, `damage_types_v1.md`, `player_attacks_agent_brief_v1.md`
- **Attributes (Epic 3):** `character_attributes_v1.md`, `attribute_scaling_v1.md`, `stat_formulas_v1.md`
- **Status Effects (Epic 4):** `status_effects_v1.md`, `status_effects_launch_set_v1.md`
- **Magic (Epic 5):** `ability_schema_v1.md`, `ability_system_v1.md`, `skill_sheet_v1.1.md`
- **Weapons (Epic 6):** `item_schema_v2.md`, `item_system_v1.md`
- **Inventory (Epic 7):** `item_schema_v2.md`, `agent_rules_v2.md` (unlimited carry model)
- **Playable Slice (Epic 2.5):** `game_design_v2.md` (diorama camera, controls), `combat_system_v1.md`, `enemy_archetypes_v1.md`, `enemy_ai_framework_v1.md`
- **Enemies (Epics 8+):** `enemy_archetypes_v1.md`, `enemy_ai_framework_v1.md`
- **World (Epic 8):** `world_design_v1.md`
- **Town (Epic 9):** `village_system_v1.md`
