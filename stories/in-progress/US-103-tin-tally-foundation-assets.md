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
- Interpretation calls (design-doc faithful):
  - AC-103.2's "static Rowdies and Atkinson TTFs" means the nine faces
    needed by `tokens.json`: Rowdies 400/700, Atkinson Hyperlegible Next
    500/600/700/800, and Atkinson Hyperlegible Mono 600/700/800.
  - Unreal asset identifiers cannot retain the token hyphens safely across
    generated-class paths, so `TS_Tally_<style>` maps `-` to `_`
    (`TS_Tally_tally_display`, `TS_Tally_ui_body`, and so on) while metadata
    retains the exact design token.
  - AC-103.4 uses a command-line Development-only gallery overlay on
    MovementTestMap rather than an input-controlled screen. It therefore adds
    no new Tier 4 input script; the full existing sweep is the Tier 4 gate.
  - The gallery crop uses a deterministic 960-design-pixel region centred in
    the required 1920x1080 frame. Browser/Slate font raster fringe may affect
    the similarity score, but dimensions, theme identity, hard-edge geometry,
    visibility, and exact metadata are never masked.
  - Coordinator correction: each 960x720 gallery crop represents the top
    960x720 design-pixel viewport of its component preview, without stretching
    either image. The previews are taller than the required capture region;
    this viewport is the common region that contains each component's anatomy
    and its first representative variants.
- RED scaffold: `Scripts/tin_tally_gallery.py` intentionally returns
  not-implemented results so its offline tests fail at assertions rather than
  import/compile time. It is replaced in GREEN without weakening the tests.
- RED evidence (2026-11-11):
  - `py Scripts/run_tests.py Mordecai.UI.Foundation` compiled and ran four
    tests: 0 passed, 4 failed at assertions. Missing cooked assets/styles,
    glyph count 0/117, absent two-layer tally widgets, stale SizeBox width,
    and mismatched-spec acceptance all failed as intended. Log:
    `Saved/TestLogs/tests-20261111-104659.log`.
  - `py Scripts/run_python_tests.py -k tin_tally_gallery -v` ran 10 tests:
    0 passed, 10 failed at assertions, 0 errors.
  - To avoid compile-RED while making mismatched-spec rejection directly
    testable, RED changes `FMordecaiUIPrimitiveBuilder::Apply` to return a
    scaffold boolean while retaining its old behavior (`true` for any
    non-null overlay). GREEN must add validation before returning `false`.
- Coordinator correction RED (2026-11-11):
  - Retained the rejected packaged Keycap crop and the Chrome-rendered
    top-viewport reference as regression fixtures.
  - `py Scripts/run_python_tests.py -k tin_tally_gallery -v` ran 13 tests:
    11 passed and 2 failed at assertions. The old comparator directly rejects
    the stretched fixture, but the failures prove it does not report foreground
    IoU/bounding-box metrics and the gallery workflow never calls it or records
    comparison evidence.
  - The visual PASS claims below are superseded by coordinator review:
    Plate, Tag, Keycap, and Tally failed AC-103.5 in both themes; Glyphs were
    plausible but still lacked required reference comparison artifacts.
- Delivery commits:
  - Story start: `876b739`.
  - RED (tests unchanged during GREEN): `0204c54`.
  - GREEN foundation/runtime/gallery: `737a339`.
  - Packaged-gallery UI capture correction found by PIL review: `5f5bdaa`.
- Verification (final source and content):
  - Compile: PASS.
  - Targeted `Mordecai.UI`: 115 passed, 0 failed
    (`Saved/TestLogs/tests-20261111-124250.log`).
  - Full Tier 1: 821 passed, 0 failed
    (`Saved/TestLogs/tests-20261111-124406.log`).
  - Full Python tooling: 638 passed, 0 failed, 0 errors, 0 skipped.
  - Tier 2: 20 smoke scripts passed, including
    `verify_pie_tin_tally_gallery.py`.
  - `py -I Design/TinTally/tools/render.py check`: `problems: 0`.
  - `py Scripts/check_content_committed.py`: PASS; 161 foundation assets
    accompany the idempotent creation script.
- Exact clean Development package:
  - Package commit `5f5bdaa6700873171fdce0f872595541e241ce0b`;
    `dirty=false`, `source_changed_during_build=false`.
  - MovementTestMap boot PASS:
    `Saved/Playtests/20261111-125529-boot`. Every boot PNG was checked by
    Pillow: 7/7 readable, correctly sized, varied, and non-black; the report
    records the Mordecai loading screen and arena experience with no missing
    cooked content.
  - Packaged gallery PASS for Plate, Tag, Keycap, Tally, and all 117 Glyphs
    in both themes: `Saved/Playtests/20261111-125612-tin-tally-gallery`.
    Pillow checked all 20 full/crop PNGs: full frames are 1920x1080, crops are
    960x720, none are black, every crop has non-background geometry, and
    Diorama teal/cream distributions differ from High Contrast black/white.
    PNGs were not opened in this agent per coordinator instruction; human
    visual sign-off remains for coordinator review from the evidence bundle.
- Packaged regression:
  - Baseline `559e28f95f`: `Saved/Playtests/20261111-115929-sweep`.
  - Final `5f5bdaa670`: `Saved/Playtests/20261111-125805-sweep`.
  - Both sweeps retain the same five pre-existing failing scripts
    (`us001`, `us054_arena`, `us057`, `us069`, `us078`); comparison reports
    zero script-level and zero individual-check PASS-to-FAIL transitions.
- Evidence bundle (292 files, SHA-256 manifest verified):
  `C:\Users\jeffd\AppData\Local\Cursor\AgentStores\cursor_agent_stores\bc-4d6a1be8-b372-5492-abbe-334810677171\files\issue-102-us103-5f5bdaa`.
- Coordinator-correction delivery commits:
  - Visual-comparison RED and retained rejected fixture: `6515e36`.
  - Design-size cooked gallery fixtures and foreground/bbox comparison:
    `3e8ff5a`.
  - Rowdies/Atkinson composite-font wiring and style token weights:
    `025f20a`.
  - Text-tint diagnostic correction: `4332e34`.
  - Stable `UTextBlock` use of cooked `TS_Tally_*` CDO font and line-height:
    `655a2e3`.
- Coordinator-correction verification at `655a2e335c`:
  - Compile: PASS.
  - Targeted `Mordecai.UI`: 115 passed, 0 failed.
  - Full Tier 1: 821 passed, 0 failed.
  - Full Python tooling: 641 passed, 0 failed, including 13 gallery
    comparator tests and rejection of the retained stretched Keycap capture.
  - Tier 2: 20 smoke scripts passed.
  - `py -I Design/TinTally/tools/render.py check`: `problems: 0`.
  - Cursor config sync check and content guard: PASS.
- Corrected exact clean Development package:
  - Package commit `655a2e335c2f7a3aacec6d21d40f9748be673219`;
    `dirty=false`, `source_changed_during_build=false`, package log
    `Saved/Playtests/package-20261111-164554.log`.
  - MovementTestMap boot PASS:
    `Saved/Playtests/20261111-165639-boot`; all 10 functional checks passed.
    Pillow verified every frame as readable, correctly sized, varied, and
    non-black; images were not opened through the file-read tool per
    coordinator instruction.
  - Corrected gallery PASS:
    `Saved/Playtests/20261111-165712-tin-tally-gallery`. The gallery writes
    same-size 960x720 reference crops, 1920x720 side-by-sides, and 960x720
    absolute diffs under `compare/`; it never resizes either input.
  - Foreground IoU / normalized maximum bbox-edge delta:
    Plate Diorama `0.2897 / 0.0111`, Plate High Contrast
    `0.2536 / 0.0250`; Tag Diorama `0.5655 / 0.0194`, Tag High Contrast
    `0.5036 / 0.0194`; Keycap Diorama `0.1952 / 0.0031`, Keycap High
    Contrast `0.1739 / 0.0056`; Tally Diorama `0.1296 / 0.0031`, Tally
    High Contrast `0.0987 / 0.0056`; Glyphs Diorama `0.1556 / 0.0194`,
    Glyphs High Contrast `0.4597 / 0.0194`. The documented gates are IoU
    `>= 0.08` and bbox delta `<= 0.20`.
  - Pillow region inspection
    (`scripted-visual-inspection.json`) found foreground pixels in every
    expected text/shape region in both themes: Plate anatomy, labels, inset
    well, and pattern; Tag anatomy, labels, gauges, and exhausted variant;
    Keycap keyboard/wide/mouse/gamepad/shoulder/trigger/stick/D-pad rows;
    Tally use titles, frames/numbers, enlarged anatomy, and rank rows; and
    Glyph heading/cells. The cooked fixture source sets deterministic
    Rowdies/Atkinson text in all sheets.
  - Dynamic `FKey`/device mapping remains owned by US-104. US-103's
    Development-only fixture statically composes the required shapes from
    cooked primitives and styles, so no preview element is omitted.
- Corrected packaged regression:
  - Baseline `559e28f95f`: 15 scripts, 10 PASS / 5 FAIL.
  - Final `655a2e335c`: `Saved/Playtests/20261111-165915-sweep`, 15 scripts,
    10 PASS / 5 FAIL.
  - Both retain the same failures (`us001`, `us054_arena`, `us057`, `us069`,
    `us078`); zero script-level and zero individual-check PASS-to-FAIL
    transitions.
- Corrected evidence bundle (381 files, all 381 SHA-256 hashes independently
  re-read and verified):
  `C:\Users\jeffd\AppData\Local\Cursor\AgentStores\cursor_agent_stores\bc-4d6a1be8-b372-5492-abbe-334810677171\files\issue-102-us103-655a2e3`.
- Story intentionally remains in progress with ACs unchecked for coordinator
  image review, closure, and merge.
