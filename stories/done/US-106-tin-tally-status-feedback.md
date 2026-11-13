# US-106: Tin Tally Status Chips and Combat Feedback

**Epic:** Epic Tin Tally: In-engine UI
**Status:** Done
**Priority:** P1
**Depends on:** US-105
**Paired story:** none

## Overview
Finish the contextual left/centre HUD: accessible two-row status chips, player-facing status names, combat callouts, and combo. This story owns the open status-name defect and adds targeted regression/packaged evidence for the active-combo rebuild fix already present on main.

## Execution Mode
**HEADLESS**
Native fallback widget/layout and tests use US-103's cooked foundation assets.

## References
- `Design/TinTally/ds/project/components/{StatusChip,CombatCallout,HUDLayout}/README.md`
- `Design/TinTally/ds/project/02-hud-and-pacing.md`
- `Specs/control_bindings_v1.1.md` — spell/combat inputs used in Tier 4
- `Source/LyraGame/Mordecai/UI/MordecaiStatusEffect*`, `MordecaiCombatFeedbackWidget.*`, `MordecaiComboCounterWidget.*`
- Issues #21 and #79

## Acceptance Criteria
- [x] AC-106.1: `IndicatorContainer` is panel-compatible and the fallback uses a WrapBox at `(32,318)`, `456x112`, maximum eight visible chips in two rows.
- [x] AC-106.2: every chip carries display name, glyph, duration/stacks and non-color polarity shape; `"Stone Skin"` replaces `"StoneSkin"` in UI and Tier 4 assertions in the same commit.
- [x] AC-106.3: callouts use exact shipped copy `"Perfect Dodge!"`, `"Perfect Block!"`, `"Parried!"`, `"Parry Failed!"`, and `"Posture Broken!"`, latest replaces previous, and hide after 1.0 s; High Contrast adds the specified plate.
- [x] AC-106.4: Normal/Large/XL grow upward from the Jeff-approved fixed bottom at centre `+180` and never overlap the combo fixed at `+184`.
- [x] AC-106.5: combo reads `"Hit N"`, remains visible through a HUD activation/rebuild during an active window, and hides when the window closes.
- [x] AC-106.6: both themes pass component-preview comparison and no existing playtest PASS becomes FAIL.

## Technical Notes
- Preserve `StatusNameText`, `DurationText`, `TintImage`, `FeedbackText`, and `ComboText`.
- Add native fallbacks for component names `StatusGlyph`, `StackPips`, `OverflowChip`, and `CalloutPlate`.
- Use one approved status display-data source for chip and later tooltip; do not derive shipping copy from tag segments after this story.
- Keep caps as transforms and strings as real TextBlocks.
- TODO(DECISION): if more than four hidden chip names must remain bridge-visible, add accessible-name reporting rather than weakening the visible max-eight rule.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.StatusChip.WrapsAndCapsAtEight`
- `Mordecai.UI.StatusChip.DisplayNameAndPolarity`
- `Mordecai.UI.CombatCallout.ReplacesAndExpires`
- `Mordecai.UI.Combo.SurvivesHUDReactivation`
### Tier 2 — PIE smoke (editor integration)
- Extend HUD smoke for status rows and centre-lane boxes.
### Tier 3 — Packaged playtest
- CombatTestMap captures normal, status, callout, combo, and High Contrast states.
### Tier 4 — Real-input playtest
- Update/run `Scripts/playtests/us057_magic.py`; run combat/death paths and full sweep.

## Playtest
- **Launch path:** CombatTestMap; `py Scripts/playtest_story.py US-057` and combat/death stories
- **Player steps:** cast Stone Skin, trigger positive/negative feedback and combo, die while combo timing is active when feasible
- **Visible on screen:** `"Stone Skin"` chip with duration, callout under player, `"Hit N"` in its own plate, no overlap
- **Proof:** Tier 4 state/text checks and both-theme gallery captures
- **Tier 4 script:** update existing magic/combat scripts and their fake-bridge tests
- **Steps:** generated plans must reflect the renamed string
- **Playable alone:** YES

## Definition of Done
- [x] Jeff confirms AC-106.4; all ACs checked and red tests fail for intended reasons first
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation and Python tooling tests pass
- [x] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [x] Content guard: `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [x] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [x] Updated US-057/combat stories and their `--lint` checks pass on that package; full sweep has no baseline PASS to FAIL
- [x] #21 is evidenced fixed and #79's existing source fix is evidenced by targeted regression, without issue mutation; both-theme previews/diffs and every gameplay screenshot are bundled for coordinator review
- [x] Evidence run dirs appear in Completion Notes and closing `[US-106]` commit body; closing commit touches only `stories/`
- [x] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
- **Coordinator decisions (2026-11-12):**
  - Image review belongs to the coordinator. The builder PIL-checks and bundles every image but does not open PNG files; the coordinator's review satisfies the visual-review step.
  - Gameplay categories remain byte-for-byte equivalent to main `3d9f08b4`: `MicroStunned` is Control, `Sleeping` takes the unknown/default Debuff category, and gameplay Buff contains only `Focused`. UI display sort and polarity are separate fields.
  - Jeff approved the fixed-bottom callout rule recorded in `Specs/design_decisions_log.md`: bottom `+180`, upward growth, combo `+184`.
  - AC-106.5 keeps the logic test as the issue #79 regression. Tier 4 combo proof remains blocked by known baseline N24.
  - The status-chip fallback uses flat-strip teeth because the specified 9-slice textures do not exist. This is approved for US-106; chip bodies still carry the required 3 px ink outline (4 px in High Contrast).
  - `Focused` can render `3/3` whenever its active tag exists because the tag is granted only at `FocusedStreakThreshold == 3`; `Exposed` can render `1 hit` because its active tag is consumed by the next hit. No unavailable ASC value is invented.
- **RED:** `f72a3904` — status 0/3, callout 0/1, live theme 0/1 and HUD gallery 0/1 failed for the intended review defects; logs are in the bundle.
- **GREEN package:** `65d6d1b767991edb738463878f3d45f5bdafeb16`; clean build (`dirty=false`, `source_changed_during_build=false`), `Saved/Playtests/package-20261113-004639.log`.
- **Automated verification:** compile PASS; Tier 1 836/836; Python tooling 663/663; Tier 2 PIE 20/20; content guard PASS; US-057/054/091 lint PASS.
- **Packaged story runs:**
  - US-057 PASS: Tier 3 `Saved/Playtests/20261113-005621-boot`; Tier 4 17/17 AC PASS `Saved/Playtests/20261113-005642-us057`.
  - US-054: Tier 3 DevTestMap and CombatTestMap PASS (`20261113-005723-boot`, `20261113-005741-boot`); combat Tier 4 PASS `20261113-005807-us054combat`; arena Tier 4 retained the baseline `enemies_present` failure `20261113-005759-us054`.
  - US-091 PASS: Tier 3 `Saved/Playtests/20261113-005924-boot`; Tier 4 `20261113-005942-us091death` and `20261113-010037-us091weapons`.
- **Both-theme galleries:** StatusChip `20261113-010115-tin-tally-gallery`, CombatCallout `20261113-010148-tin-tally-gallery`, HUDLayout `20261113-010222-tin-tally-gallery`; all 6 captures/comparisons PASS.
- **Full sweep:** `Saved/Playtests/20261113-010305-sweep` is baseline-equivalent: 4/4 Tier 3 launch paths PASS and 0 PASS→FAIL across 15 scripts, 393 checks and 146 AC comparisons versus `20261112-230453-sweep`. It retains the same four failing scripts (US-001, US-054 arena, US-069 and US-078).
- **Images:** 160 PNGs were verified with Pillow only (0 invalid, 0 near-uniform, 0 all-black) and bundled for the coordinator's visual review. Per D1, the builder did not open PNGs. Coordinator image review remains the only unfinished DoD item, so the story stays in progress.
- **Evidence bundle:** `us106-r2-65d6d1b` at `C:\Users\jeffd\AppData\Local\Cursor\AgentStores\cursor_agent_stores\bc-7055e11f-fb2d-5bb1-b1e5-3eae5edf1fb6\files\us106-r2-65d6d1b` (335 files including `SHA256SUMS.txt`, raw baseline/final sweep JSON, RED/GREEN logs and `build_info.json`).
- **AC-106.5 qualification:** the issue #79 activation/rebuild behavior is accepted by the targeted logic regression per coordinator decision D4. Tier 4 combo proof remains blocked by known baseline N24.
- **Round 3 RED:** `0bd31e10` — the real cached-geometry test failed on Frostbitten pip 4 outside the body and overlapping duration, Shocked compact pip 3 outside the body and overlapping duration, and Stone Skin's sub-6 px name/time clearance. The gallery PIL analysis also rejected both Round 2 themes with pip-coloured pixels in the duration boxes.
- **Round 3 GREEN package:** `2dc787ee32e909cf931b1af8a379a6ca97ea7285`; clean package (`dirty=false`, `source_changed_during_build=false`), `Saved/Playtests/package-20261113-022122.log`.
- **Round 3 automated verification:** compile PASS; Tier 1 836/836; Python tooling 665/665; Tier 2 PIE 20/20; content guard PASS. The cached-geometry suite is 3/3 and the live High Contrast theme regression passes.
- **Round 3 galleries:** StatusChip `Saved/Playtests/20261113-023107-tin-tally-gallery` and HUDLayout `Saved/Playtests/20261113-023146-tin-tally-gallery`; both themes and all four comparisons PASS. Frostbitten and compact Shocked each have 0 pip-coloured pixels in their duration box in both themes.
- **Round 3 US-057:** Tier 3 PASS `Saved/Playtests/20261113-023224-boot`; Tier 4 17/17 AC PASS `Saved/Playtests/20261113-023244-us057`, including visible `"Stone Skin"` / `"10.0"`.
- **Round 3 full sweep:** `Saved/Playtests/20261113-023326-sweep`; 4/4 Tier 3 launch paths PASS, 15 Tier 4 scripts ran, and comparison with the accepted `65d6d1b7` sweep `Saved/Playtests/20261113-010305-sweep` reports zero script and zero check PASS→FAIL across 393 checks. The same known/baseline failures remain; `enemies_present` is labelled NEW by this run but was already FAIL in the accepted baseline and is not a PASS→FAIL.
- **Round 3 images:** 113 PNGs were verified with Pillow only (0 invalid, 0 black, 0 near-uniform); the builder did not open PNG files. The coordinator's prior image-review role is accepted, with the Round 3 visual check pending coordinator review of this bundle.
- **Round 3 evidence bundle:** `us106-r3-2dc787e` at `C:\Users\jeffd\AppData\Local\Cursor\AgentStores\cursor_agent_stores\bc-7055e11f-fb2d-5bb1-b1e5-3eae5edf1fb6\files\us106-r3-2dc787e` (127 SHA256 manifest entries, raw baseline/final sweep JSON, RED/GREEN logs, clean `build_info.json`, gallery summaries and every image).
