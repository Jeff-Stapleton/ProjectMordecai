# US-112: Native Tin Tally Front-end Screens

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-103, US-104, US-109
**Paired story:** US-118 (EDITOR) — front-end experience, playlists, backdrop, and cooked wiring

## Overview
Build native fallback classes for Project Mordecai's TitleScreen, ExpeditionSelect, and final LoadingScreen. This is the code half; it is not player-visible until US-118 wires the saved front-end experience, playlists, backdrop, and package content.

## Execution Mode
**HEADLESS**
Native widget classes, fallback trees, filtering/actions, and automation tests only.

## References
- `Design/TinTally/ds/project/components/{TitleScreen,ExpeditionSelect,LoadingScreen}/README.md`
- `Design/TinTally/ds/project/04-open-decisions.md` — approved Title → Play → ExpeditionSelect flow
- `Specs/control_bindings_v1.1.md` — front-end navigation/confirm/back input
- `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`
- Existing `UMordecaiLoadingScreenWidget`, front-end experience and `DA_Mordecai_Arena`

## Acceptance Criteria
- [ ] AC-112.1: `UMordecaiTitleScreenWidget` builds a fallback with exact `"PROJECT MORDECAI"`, `"Everything counts. Nothing lasts."`, and `"Play"`, `"Options"`, `"Credits"`, `"Quit"` controls.
- [ ] AC-112.2: exact `UMordecaiExpeditionSelectScreen` and `UMordecaiExpeditionCard` build fallbacks with `"Choose an Expedition"`, `"EXPEDITION"`, `"Start"`, `"Move"`, and `"Back"`, list only Mordecai playlist definitions, and expose real Play/Start actions.
- [ ] AC-112.3: Back on Title opens US-109's safe-default `UMordecaiConfirmationDialog`; no native action routes to host/join/replay/add-player surfaces.
- [ ] AC-112.4: `UMordecaiLoadingScreenWidget` keeps `TitleText`/`StatusText`, exact `"PROJECT MORDECAI"`, `"Loading..."`, and `"Tip"` copy, opaque coverage, cooked foundation assets and Tin Tally throbber without Lyra content.
- [ ] AC-112.5: keyboard and gamepad focus/navigation work through CommonUI and all affected labels remain bridge-visible.
- [ ] AC-112.6: every required binding has a fallback; this code-only story boots without regression and records `NOT YET PLAYABLE — pending US-118`.

## Technical Notes
- Classes: `UMordecaiTitleScreenWidget`, `UMordecaiMenuTagButton`, `UMordecaiExpeditionSelectScreen`, `UMordecaiExpeditionCard`.
- Preserve loading `Background`, `TitleText`, `StatusText`, `TallyThrobber`, `TipBorder`, `TipKey`, `TipText`; expedition bindings are `HeaderText`, `CardRow`, `DevColumn`, `ActionBar`, `TileTitleText`, `TileSubTitleText`, `TileDescriptionText`, `TileIconImage`, `KickerText`, `StartHint`.
- US-118 owns real playlist/experience IDs and saved content wiring.
- Version stamp is Development-only; hide it until a version scheme is approved.
- Do not address front-end music (#65) or executable/company/icon naming (#66/#67/#69).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.FrontEnd.NoLyraReachablePresentation`
- `Mordecai.UI.FrontEnd.TitleActionsAndSafeQuit`
- existing loading-screen tests plus Tin Tally asset/copy checks
### Tier 2 — PIE smoke (editor integration)
- Native construction, focus and action smoke with test descriptors.
### Tier 3 — Packaged playtest
- MovementTestMap boot only; the front-end route is accepted by US-118.
### Tier 4 — Real-input playtest
- Full existing sweep; no new input AC is accepted before US-118.

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap`
- **Player steps:** existing movement path only; native screen actions are Tier 1/Tier 2 until US-118
- **Visible on screen:** existing packaged game still boots without widget/load errors
- **Proof:** Tier 3 boot and no-regression sweep; front-end proof belongs to US-118
- **Tier 4 script:** none added
- **Steps:** none
- **Playable alone:** NO — playable after US-118

## Definition of Done
- [ ] All ACs checked; red tests fail for intended missing native surfaces first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [ ] Full Tier 4 sweep on that package has no baseline PASS to FAIL; no front-end input AC is claimed before US-118
- [ ] Every gameplay screenshot is opened/described; code-only completion states `NOT YET PLAYABLE — pending US-118`
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-112]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
<Filled in by the implementing agent.>
