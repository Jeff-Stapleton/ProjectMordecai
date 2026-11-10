# US-102: Tin Tally Theme Runtime and Shared Primitives

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** none
**Paired story:** US-103 (EDITOR) — cooked foundation assets and visual-acceptance gallery

## Overview
Create the native runtime contract for Diorama and High Contrast, shared RoundedBox construction, and the tally-mark widget. This is the code half of the Tin Tally foundation; it deliberately creates no assets and is not player-visible until US-103 lands.

## Execution Mode
**HEADLESS**
Native data types, theme selection, shared widget construction, and automation tests only.

## References
- `Design/TinTally/ds/BUILD.md` — D1, D5, D6, D13-D17, D21-D27
- `Design/TinTally/ds/project/03-building-in-unreal.md` — Theme, Asset manifest, Fonts
- `Design/TinTally/ds/project/tokens.json` — exact tokens and two themes
- Existing code: `Source/LyraGame/Mordecai/UI/`

## Acceptance Criteria
- [ ] AC-102.1: `UMordecaiUIThemeDataAsset` represents every runtime color token, including aliases and alpha, and conversion from an sRGB hex uses `FLinearColor::FromSRGBColor`.
- [ ] AC-102.2: the local UI theme service defaults to Diorama, swaps to High Contrast at runtime, and notifies live widgets without reconstructing gameplay UI.
- [ ] AC-102.3: shared plate, pill, tag, well, keycap, and focus builders expose the exact RoundedBox layers/radii/strokes from the manifest and use no plate textures.
- [ ] AC-102.4: `UMordecaiTallyWidget` renders `Count` from 0 through `Max`, clamps invalid input, and supports the six 0-5 bundle frames without using the tally motif outside D1's allowed jobs.
- [ ] AC-102.5: a packaged Development build still boots the Mordecai HUD with no regression; this code-only half is recorded `NOT YET PLAYABLE — pending US-103`.

## Technical Notes
- Add native files under `Source/LyraGame/Mordecai/UI/TinTally/`.
- Keep theme fields data-driven and designer-readable; do not make `tokens.json` a runtime dependency.
- Theme selection must not depend on PIE or editor-only state.
- US-103 creates `DA_UITheme_Diorama`, `DA_UITheme_HighContrast`, fonts, styles, materials, and images.
- TODO(DECISION): none for this code contract; device-specific keycap content belongs to US-104.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.Theme.SRGBConversion` — AC-102.1
- `Mordecai.UI.Theme.RuntimeSwapNotifiesWidgets` — AC-102.2
- `Mordecai.UI.Primitives.RoundedBoxManifest` — AC-102.3
- `Mordecai.UI.TallyWidget.CountAndClamp` — AC-102.4
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_tin_tally_theme.py` — theme service exists and resolves a theme; no direct gameplay calls.
### Tier 3 — Packaged playtest
- Package and boot MovementTestMap; inspect all boot screenshots for the existing HUD and no load errors.
### Tier 4 — Real-input playtest
- Run the full existing sweep; no baseline PASS becomes FAIL.

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap`; Tier 4 full sweep on the same package
- **Player steps:** run the existing movement/HUD path; no new player control is introduced
- **Visible on screen:** the existing Mordecai arena and HUD remain visible; no missing-theme error or blank root
- **Proof:** Tier 3 boot plus sweep comparison; visual acceptance begins in US-103
- **Tier 4 script:** existing sweep; no new input-driven AC
- **Steps:** none added
- **Playable alone:** NO — playable after US-103

## Definition of Done
- [ ] All ACs checked off
- [ ] Red tests fail for the intended missing contract before implementation
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks are labeled LOGIC
- [ ] Content: every changed asset is saved/committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [ ] Full Tier 4 regression sweep on the same package has no baseline PASS to FAIL
- [ ] Every `engine_NN.png`/`bridge_NN.png` is opened; Completion Notes describe what is actually visible
- [ ] Evidence run dirs appear in Completion Notes and the closing `[US-102]` commit body; the closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
<Filled in by the implementing agent.>
