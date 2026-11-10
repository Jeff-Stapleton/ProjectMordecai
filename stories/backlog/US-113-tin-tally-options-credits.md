# US-113: Tin Tally Options and Credits (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-108, US-118, TODO(DECISION) settings and localization policy
**Paired story:** none

## Overview
Replace Lyra Options/Credits and connect a real pause Settings tab. This is blocked until Jeff chooses persistence/save ownership, UI scaling, rebinding scope, retained rows/language policy, credit content, and version scheme.

## Execution Mode
**HEADLESS**
After decisions, native activatable widgets can use existing settings services and US-103 assets; split an EDITOR pair if new settings assets are required.

## References
- `Design/TinTally/ds/project/components/{OptionsScreen,Credits}/README.md`
- `Design/TinTally/ds/project/04-open-decisions.md` — Settings tab, device glyphs
- `Specs/control_bindings_v1.1.md` — menu/settings navigation input
- Issues #10, #62, #63, #64

## Acceptance Criteria
- [ ] AC-113.1: Jeff approves settings persistence, UI scale implementation, rebinding scope, retained Lyra rows/language behavior, credits, and version policy before work begins.
- [ ] AC-113.2: Options provides the reviewed Gameplay/Display/Audio/Controls/Accessibility sections and exact names/values; unavailable functions are visibly unavailable, not fake controls.
- [ ] AC-113.3: High Contrast, UI scale, HUD margin, motion and Tally settings apply as specified and persist across restart through an approved save slot.
- [ ] AC-113.4: pause Settings opens the same sections instead of `"Coming Soon"`; front-end Options uses the same model.
- [ ] AC-113.5: Credits contains approved names and all three font OFL notices, with no Lyra logo/copy/localized leakage.
- [ ] AC-113.6: keyboard/gamepad navigation, apply/cancel/back and both-theme comparisons pass in a packaged build.

## Technical Notes
- Proposed classes: `UMordecaiOptionsScreen`, `UMordecaiCreditsScreen`.
- Reuse CommonUI settings infrastructure only where it exposes Mordecai actions truthfully.
- BLOCKED: no synthetic language/rebinding rows and no unapproved credit names.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- Settings model/persistence/theme swap, options route, credits license/no-Lyra tests.
### Tier 2 — PIE smoke (editor integration)
- Front-end and pause routes resolve the same widget/model.
### Tier 3 — Packaged playtest
- Front-end and pause Options, restart persistence, Credits.
### Tier 4 — Real-input playtest
- Navigate/edit/apply/cancel/restart with keyboard and gamepad.

## Playtest
- **Launch path:** front end and MovementTestMap pause menu
- **Player steps:** open Options, change each approved accessibility setting, apply/restart, open Credits, open pause Settings
- **Visible on screen:** persisted values, live theme/scale, approved credits/licenses, no Lyra text
- **Proof:** new Tier 4 scripts and both-theme comparisons
- **Tier 4 script:** BLOCKED pending decisions
- **Steps:** BLOCKED pending decisions
- **Playable alone:** YES after blockers

## Definition of Done
- [ ] Decisions logged and any required pair added; all ACs checked; red tests fail first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content: every changed asset is saved/committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] Front-end and MovementTestMap pause Tier 3 routes exit 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-113` and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] #10/#62/#63/#64 scopes are evidenced only when complete; both-theme previews/diffs and every gameplay screenshot are opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-113]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on the decisions named above.
