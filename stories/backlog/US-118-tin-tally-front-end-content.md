# US-118: Tin Tally Front-end Content and Backdrop

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-112
**Paired story:** US-112 (HEADLESS) — native front-end screens

## Overview
Wire US-112's native screens into the saved front-end experience, replace the Lyra lobby with the Tin Tally toy-island backdrop, and remove sample shooter/party content from the shipped package. The player receives the complete Title → ExpeditionSelect → LoadingScreen → Mordecai arena path.

## Execution Mode
**EDITOR**
The story changes/saves the front-end experience, playlist/card/icon and backdrop/map content.

## References
- `Design/TinTally/ds/project/components/{TitleScreen,ExpeditionSelect,LoadingScreen}/README.md`
- `Design/TinTally/ds/project/04-open-decisions.md` — approved front-end flow
- `Specs/control_bindings_v1.1.md` — navigation/confirm/back input
- Existing `L_LyraFrontEnd`, `B_LyraFrontEnd_Experience`, `DA_Mordecai_Arena`

## Acceptance Criteria
- [ ] AC-118.1: the saved front-end experience uses `UMordecaiTitleScreenWidget`; title copy is exactly `"PROJECT MORDECAI"`, `"Everything counts. Nothing lasts."`, `"Play"`, `"Options"`, `"Credits"`, and `"Quit"`; Play pushes `UMordecaiExpeditionSelectScreen`.
- [ ] AC-118.2: the front-end map shows the reviewed toy-island/display-case backdrop with no Lyra sci-fi lobby, character, logo, UE roundel, machine/online identity, build stamp or ADD PLAYER.
- [ ] AC-118.3: Lyra sample shooter/party modes are removed from the shipped package, not merely hidden from the card list; no host/join/replay route remains reachable.
- [ ] AC-118.4: real keyboard/gamepad input selects `"Play"` and the real playlist fields `"Mordecai Arena"`, `"Dev test arena"`, and `"Project Mordecai diorama arena (DevTestMap)"`; the screen uses `"Choose an Expedition"`, `"EXPEDITION"`, `"Start"`, `"Move"`, and `"Back"`; loading shows exact `"PROJECT MORDECAI"`, `"Loading..."`, and `"Tip"` before `B_MordecaiArenaExperience`.
- [ ] AC-118.5: Back on Title opens the safe-focused Mordecai quit dialog; cancelling returns focus to the correct tag.
- [ ] AC-118.6: TitleScreen, ExpeditionSelect, LoadingScreen and backdrop pass packaged visual review in both themes; every asset is saved/committed and content guard passes.

## Technical Notes
- Preserve authoritative playlist/map/experience IDs and cooker scan paths.
- Asset scripts must be idempotent and save every changed package.
- #68's global `B_LyraDefaultExperience` fallback is outside this story; do not claim it.
- Audio #65 and executable/company/icon #66/#67/#69 remain outside scope.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.FrontEnd.SavedMainScreenAndPlaylist`
- `Mordecai.UI.FrontEnd.SampleModesNotShipped`
- existing loading-screen tests
### Tier 2 — PIE smoke (editor integration)
- Front-end map/backdrop, main class, playlist resolution, focus and loading widget.
### Tier 3 — Packaged playtest
- `py Scripts/playtest_boot.py --frontend` plus loading and arena captures.
### Tier 4 — Real-input playtest
- New `Scripts/playtests/us118_front_end.py` navigates Title → Play → Mordecai Arena → arena; a second path opens/cancels Quit.

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --frontend`; Tier 4 `py Scripts/playtest_story.py US-118`
- **Player steps:** navigate to Play; select arena; observe loading/arena; relaunch; Back; cancel Quit
- **Visible on screen:** Mordecai title/backdrop/expedition/loading only, then the Mordecai arena/HUD
- **Proof:** Tier 3 front-end boot, Tier 4 experience and focus checks, both-theme preview comparisons
- **Tier 4 script:** `Scripts/playtests/us118_front_end.py` with fake-bridge tests
- **Steps:** paste generated `--plan` table and require `--lint` exit 0
- **Playable alone:** YES with US-112

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: every `.uasset`/`.umap` this story creates or changes is saved and committed in this branch; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --frontend` exits 0 — never `--allow-stale`
- [ ] Input-driven ACs: `py Scripts/playtest_story.py US-118` exits 0 on that same package; `py Scripts/playtest_story.py US-118 --lint` exits 0; every input-driven AC is accepted
- [ ] Every `engine_NN.png` and `bridge_NN.png` opened; Completion Notes state what is actually on screen
- [ ] Evidence run dirs are in Completion Notes and the closing `[US-118]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
<Filled in by the implementing agent.>
