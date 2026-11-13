# US-106: Tin Tally Status Chips and Combat Feedback

**Epic:** Epic Tin Tally: In-engine UI
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
- [ ] AC-106.1: `IndicatorContainer` is panel-compatible and the fallback uses a WrapBox at `(32,318)`, `456x112`, maximum eight visible chips in two rows.
- [ ] AC-106.2: every chip carries display name, glyph, duration/stacks and non-color polarity shape; `"Stone Skin"` replaces `"StoneSkin"` in UI and Tier 4 assertions in the same commit.
- [ ] AC-106.3: callouts use exact shipped copy `"Perfect Dodge!"`, `"Perfect Block!"`, `"Parried!"`, `"Parry Failed!"`, and `"Posture Broken!"`, latest replaces previous, and hide after 1.0 s; High Contrast adds the specified plate.
- [ ] AC-106.4: Normal/Large/XL grow upward from the Jeff-approved fixed bottom at centre `+180` and never overlap the combo fixed at `+184`.
- [ ] AC-106.5: combo reads `"Hit N"`, remains visible through a HUD activation/rebuild during an active window, and hides when the window closes.
- [ ] AC-106.6: both themes pass component-preview comparison and no existing playtest PASS becomes FAIL.

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
- [ ] Jeff confirms AC-106.4; all ACs checked and red tests fail for intended reasons first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and Python tooling tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] Updated US-057/combat stories and their `--lint` checks pass on that package; full sweep has no baseline PASS to FAIL
- [ ] #21 is evidenced fixed and #79's existing source fix is evidenced by targeted regression, without issue mutation; both-theme previews/diffs and every gameplay screenshot are bundled for coordinator review
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-106]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
- **Coordinator decisions (2026-11-12):**
  - Image review belongs to the coordinator. The builder PIL-checks and bundles every image but does not open PNG files; the coordinator's review satisfies the visual-review step.
  - Gameplay categories remain byte-for-byte equivalent to main `3d9f08b4`: `MicroStunned` is Control, `Sleeping` takes the unknown/default Debuff category, and gameplay Buff contains only `Focused`. UI display sort and polarity are separate fields.
  - Jeff approved the fixed-bottom callout rule recorded in `Specs/design_decisions_log.md`: bottom `+180`, upward growth, combo `+184`.
  - AC-106.5 keeps the logic test as the issue #79 regression. Tier 4 combo proof remains blocked by known baseline N24.
  - The status-chip fallback uses flat-strip teeth because the specified 9-slice textures do not exist. This is approved for US-106; chip bodies still carry the required 3 px ink outline (4 px in High Contrast).
  - `Focused` can render `3/3` whenever its active tag exists because the tag is granted only at `FocusedStreakThreshold == 3`; `Exposed` can render `1 hit` because its active tag is consumed by the next hit. No unavailable ASC value is invented.
