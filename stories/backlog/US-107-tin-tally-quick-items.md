# US-107: Tin Tally Quick Items (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P2
**Depends on:** US-104, US-105, TODO(DECISION) quick-item behavior and bindings
**Paired story:** none

## Overview
Add the Throw plus Item A-D HUD row and bind it to real inventory/use behavior. This is a blocked stub because count semantics, assignment, and keyboard/gamepad conflicts are unresolved; the implementation must not invent controls.

## Execution Mode
**HEADLESS**
After decisions, the native widget can use existing foundation assets and input/inventory systems.

## References
- `Design/TinTally/ds/project/components/QuickItems/README.md`
- `Design/TinTally/ds/project/components/HUDLayout/README.md`
- `Specs/control_bindings_v1.1.md` — Throw and Item A-D
- `Design/TinTally/ds/project/04-open-decisions.md` — potion cap, key conflicts

## Acceptance Criteria
- [ ] AC-107.1: Jeff decides potion cap/count semantics, assignment flow, and non-conflicting keyboard/gamepad bindings before implementation.
- [ ] AC-107.2: `QuickItems` is a HUD-root child at bottom-right `(-32,-132)`, `336x56`, with Throw plus Item A-D in fixed positions.
- [ ] AC-107.3: empty slots are Hidden, stocked slots show real glyph/count, and unavailable controls use dashed unbound caps rather than a false binding.
- [ ] AC-107.4: real input consumes/throws the selected item, updates inventory and count once, and persists across pause/death as decided.
- [ ] AC-107.5: both themes/device families match QuickItems previews and all input-driven checks pass on a packaged build.

## Technical Notes
- Proposed class: `UMordecaiQuickItemsWidget`, optional name `QuickItems`.
- Do not reuse key `1` or D-pad Up while they cast Restoration; do not assign Stone Skin a pad key here.
- If Jeff retains unbound slots, acceptance may ship dashed caps but must not claim item use for those slots.
- TODO(DECISION): all blockers in AC-107.1.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.QuickItems.InventoryProjection`
- `Mordecai.UI.QuickItems.UnboundState`
### Tier 2 — PIE smoke (editor integration)
- HUD child/map-box and inventory binding smoke.
### Tier 3 — Packaged playtest
- Stocked/depleted/unbound captures in both themes.
### Tier 4 — Real-input playtest
- New script drives every approved keyboard/gamepad item action and verifies inventory/HUD deltas.

## Playtest
- **Launch path:** DevTestMap with deterministic quickslot fixtures
- **Player steps:** assign if in scope, use Item A-D, throw, pause/reopen
- **Visible on screen:** correct fixed row, keycaps, glyphs and counts; depleted state updates immediately
- **Proof:** new Tier 4 checks plus visual comparisons
- **Tier 4 script:** BLOCKED pending bindings
- **Steps:** BLOCKED pending behavior
- **Playable alone:** YES after decisions

## Definition of Done
- [ ] Decisions logged; all ACs checked; red tests fail for intended reasons first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tooling tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] `py Scripts/playtest_boot.py` for the chosen DevTestMap fixture exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-107` and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] Both-theme/device comparisons and every `engine_NN.png`/`bridge_NN.png` are opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-107]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on potion cap, assignment, and input decisions.
