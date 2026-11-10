# US-117: Tin Tally World Map (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P2
**Depends on:** missing US-073, TODO(DECISION) world-map form and authoritative data
**Paired story:** none

## Overview
Implement the final WorldMap surface after US-073 and authoritative world/discovery/travel data exist. This stub does not choose a map topology or alter the world graph.

## Execution Mode
**EDITOR**
The eventual story creates only WorldMap UI assets and its saved menu route; authoritative map/world data must arrive from US-073/world work. Split a native view-model story if required after US-073 exists.

## References
- `Design/TinTally/ds/project/components/WorldMap/README.md`
- `Design/TinTally/ds/project/04-open-decisions.md` — map form
- `Specs/world_design_v1.md`
- `Specs/agent_rules_v2.md` §8 — human approval for world-graph changes
- `Specs/map_style_technical_design_v1.md`
- `Specs/agentic_map_pipeline_v1.md`
- `Specs/control_bindings_v1.1.md` — menu/map input
- Missing referenced story: US-073

## Acceptance Criteria
- [ ] AC-117.1: US-073 exists and Jeff chooses tin tile board, screen grid, or room graph plus discovery/travel/current-location rules before implementation.
- [ ] AC-117.2: `UMordecaiWorldMapWidget` consumes authoritative world data and does not encode or mutate biome progression in UI code.
- [ ] AC-117.3: current/discovered/undiscovered/locked/selected/travel states use shape/text/icon redundancy, focus and both-theme contrast.
- [ ] AC-117.4: keyboard/gamepad navigation, open/back and approved travel operate through CommonUI with real input.
- [ ] AC-117.5: the map is reachable from the approved menu/town route in a cooked package and all data/assets are committed.
- [ ] AC-117.6: full-board and state-sheet captures pass against the selected preview in both themes.

## Technical Notes
- Recommended review default is the drawn tin tile board, bound to data.
- No new biome graph, gate, connection, or progression rule may be introduced by this UI story.
- Do not change height tiers, connection sockets, combat-pocket rules, or progression sequence without Jeff's approval.
- TODO(DECISION): all AC-117.1 items.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- Data-to-view-state, selection/focus and travel-command tests.
### Tier 2 — PIE smoke (editor integration)
- Saved route/data asset resolution.
### Tier 3 — Packaged playtest
- Cooked world map with representative authoritative data in both themes.
### Tier 4 — Real-input playtest
- Open, navigate, inspect, cancel and approved travel path.

## Playtest
- **Launch path:** approved map/menu route
- **Player steps:** open map, navigate every state, cancel, reopen and travel if approved
- **Visible on screen:** accurate current/discovery/lock states and destination result
- **Proof:** new Tier 4 script plus both-theme comparison
- **Tier 4 script:** BLOCKED pending US-073/data
- **Steps:** BLOCKED pending US-073/data
- **Playable alone:** YES after blockers

## Definition of Done
- [ ] US-073/decisions/data exist and pair added if needed; all ACs checked; red tests fail first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content: every UI asset is saved/committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then the exact clean commit packages with the approved map route
- [ ] World-map Tier 3 route exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-117` and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] Both-theme previews/diffs and every `engine_NN.png`/`bridge_NN.png` are opened/described
- [ ] No world graph/gate/height/socket/progression change; evidence run dirs appear in Completion Notes and closing `[US-117]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on US-073, map form, and authoritative world data.
