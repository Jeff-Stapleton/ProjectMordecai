# US-103: Tin Tally Foundation Assets and Visual Acceptance

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-102
**Paired story:** US-102 (HEADLESS) — native theme and primitive contract

## Overview
Import and cook the complete Tin Tally foundation, then add the packaged-build visual gallery used by every later UI story. The player receives the two themes, final typography, pattern fill, glyph/tally art, focus treatment, and a repeatable comparison against each component preview.

## Execution Mode
**EDITOR**
The story creates and configures font, material, texture, style, DataAsset, and gallery assets and saves them through Unreal Editor.

## References
- `Design/TinTally/ds/project/03-building-in-unreal.md` — canonical asset names and construction
- `Design/TinTally/ds/project/tokens.json` — type styles and colors
- `Design/TinTally/ds/project/components/{Cover,Glyphs,Keycap,Plate,Tag,Tally}/README.md`
- `Design/TinTally/tools/render.py` — canonical preview renderer/checker

## Acceptance Criteria
- [ ] AC-103.1: saved, committed, cooked `DA_UITheme_Diorama` and `DA_UITheme_HighContrast` contain the reviewed token values converted from sRGB.
- [ ] AC-103.2: static Rowdies and Atkinson TTFs, `F_Tally_Display`, `F_Tally_UI`, `F_Tally_Mono`, and every `TS_Tally_<style>` load in a package; the three OFL notices are retained for Credits.
- [ ] AC-103.3: saved/cooked RoundedBox styles, focus brush, `M_UI_PatternFill`, `T_UI_Hatch16`, 117 single-ink glyph masks, tally atlas, and badge silhouettes match the manifest and use no retired asset names.
- [ ] AC-103.4: a Development-only packaged visual gallery captures a named component/state at 1920x1080 in either theme and writes full-frame/crop images plus commit/theme/state metadata.
- [ ] AC-103.5: the comparison rejects missing, stale, black, wrong-theme, or geometrically divergent images and produces side-by-side/diff evidence; Plate, Tag, Keycap, Tally, and Glyphs pass human review in both themes.
- [ ] AC-103.6: all created assets are saved and committed; `py Scripts/check_content_committed.py` passes.

## Technical Notes
- Import static TTFs; do not import the WOFF2 previews or variable fonts.
- Raster glyphs, if used, are 64 px white masks with UI texture group and UserInterface2D compression.
- `M_UI_PatternFill` tiles from screen pixel position, not widget UVs.
- The gallery uses real cooked runtime classes/assets, deterministic fixture data, fixed exposure, no live animation, and never replaces Tier 3/Tier 4 gameplay evidence.
- Store generated acceptance evidence under `Saved/`, not in git.
- Asset-creation scripts must be idempotent, save every package, and be committed with their resulting assets.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.Foundation.AssetPathsAndTypes`
- `Mordecai.UI.Foundation.FontStylesMatchTokens`
- `Mordecai.UI.Foundation.NoRetiredAssetNames`
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_tin_tally_gallery.py` loads every foundation asset and toggles themes.
### Tier 3 — Packaged playtest
- Package the gallery with the default maps, capture Foundation components in both themes, and boot MovementTestMap.
### Tier 4 — Real-input playtest
- Full existing sweep; no baseline PASS becomes FAIL.

## Playtest
- **Launch path:** MovementTestMap plus the packaged visual-gallery launch route; full sweep on the same package
- **Player steps:** use the gallery's test-only theme/component controls, then run normal movement
- **Visible on screen:** readable static fonts, hard-edged primitives, ink|focus|ink ring, pattern fill, glyphs, tally frames in Diorama and High Contrast
- **Proof:** gallery comparison report plus Tier 3 boot and Tier 4 regression
- **Tier 4 script:** add a Development-only gallery script if its controls require input; otherwise full existing sweep
- **Steps:** component/theme selection and screenshot capture must be listed by the implemented script
- **Playable alone:** YES with US-102

## Definition of Done
- [ ] All ACs checked off
- [ ] Red tests/checks recorded before asset creation
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks are labeled LOGIC
- [ ] Content: every `.uasset`/`.umap` is saved/committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` and the packaged gallery route exit 0 — never `--allow-stale`
- [ ] Any gallery input script and the full Tier 4 sweep pass on the same package; its `playtest_story.py --lint` exits 0
- [ ] Foundation comparisons pass in both themes; every `engine_NN.png`, `bridge_NN.png`, preview, crop, and diff is opened and described
- [ ] `py -I Design/TinTally/tools/render.py check` reports `problems: 0`
- [ ] Evidence run dirs appear in Completion Notes and the closing `[US-103]` commit body; the closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
<Filled in by the implementing agent.>
