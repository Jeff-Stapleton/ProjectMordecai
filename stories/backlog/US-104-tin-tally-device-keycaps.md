# US-104: Tin Tally Device-aware Keycaps (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-103, US-097, TODO(DECISION) device glyph and input policy
**Paired story:** none

## Overview
Replace bracketed prompt text with CommonInput-driven Tin Tally keycap assets across HUD, menus, and front end. This is a decision-blocked stub: no implementation may begin until Jeff chooses the CommonUI integration, pad-symbol policy, spell model, and gamepad Inventory hint.

## Execution Mode
**EDITOR**
The story creates CommonInput controller data and keycap image/style assets for existing `UCommonActionWidget`/action-bar paths; it does not add a native widget class. If the chosen policy requires native code, create a paired HEADLESS story before implementation.

## References
- `Design/TinTally/ds/project/components/Keycap/README.md`
- `Design/TinTally/ds/project/03-building-in-unreal.md` — Playtest safety
- `Design/TinTally/ds/project/04-open-decisions.md` — Device-aware glyphs, Quick-item keys
- `Specs/control_bindings_v1.1.md` — shipped input assignments and conflicts
- Existing story: `stories/backlog/US-097-recovery-gamepad-key-names.md`

## Acceptance Criteria
- [ ] AC-104.1: TODO(DECISION) entries for CommonUI integration, pad symbols, direct spells versus Skill A/B, and gamepad Inventory hint are answered and logged before implementation.
- [ ] AC-104.2: keyboard/mouse, Xbox, and PlayStation controller data render one coherent device family per screen; last-input switching updates all visible prompts.
- [ ] AC-104.3: square, round, pill, mouse, and D-pad caps match the component states in both themes; unbound is dashed and never presented as usable.
- [ ] AC-104.4: before keycaps, strings remain `"[F] Pick up {item}"` and `"[I] Inventory"`; after keycaps, labels are `"Pick up {item}"` and `"Inventory"`. The same commit updates Tier 4 constants to validate the separate keycap and label.
- [ ] AC-104.5: assets are saved/committed/cooked and the content guard passes.

## Technical Notes
- Prefer CommonInput controller data; do not manually inspect hardware each frame.
- Keep all labels as real text for bridge/accessibility inspection.
- BLOCKED: use no placeholder button mapping and do not treat US-097's valid-key fix as the spell-layout decision.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.Keycap.DeviceFamilyAndState`
- `Mordecai.UI.Keycap.UnboundNeverAdvertised`
### Tier 2 — PIE smoke (editor integration)
- Verify controller data loads and switching input type updates every registered keycap.
### Tier 3 — Packaged playtest
- Capture Keycap and InteractPrompt state sheets in both themes and all three device families.
### Tier 4 — Real-input playtest
- Update and run US-079, US-069, and front-end scripts with real keyboard and gamepad input.

## Playtest
- **Launch path:** MovementTestMap, DevTestMap inventory path, and front end on one fresh package
- **Player steps:** use keyboard, then gamepad; focus a pickup; open Inventory and pause
- **Visible on screen:** matching device caps lead labels; no mixed device families; no duplicated bracketed key text
- **Proof:** new Tier 4 keycap checks plus both-theme gallery comparisons
- **Tier 4 script:** defined after decisions; existing affected scripts must be updated in the same commit
- **Steps:** BLOCKED pending input policy
- **Playable alone:** YES after blockers are resolved

## Definition of Done
- [ ] Blockers resolved and logged
- [ ] All ACs checked off
- [ ] Red tests/checks fail for the intended missing behavior before implementation
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks are labeled LOGIC
- [ ] Content: every created/changed asset is saved/committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] Required MovementTestMap/DevTestMap/front-end Tier 3 boots exit 0 — never `--allow-stale`
- [ ] Affected Tier 4 stories and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] Every theme/device preview, diff, `engine_NN.png`, and `bridge_NN.png` is opened and described
- [ ] Evidence run dirs appear in Completion Notes and the closing `[US-104]` commit body; the closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on the decisions named above.
