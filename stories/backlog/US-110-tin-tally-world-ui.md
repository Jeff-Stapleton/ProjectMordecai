# US-110: Tin Tally Enemy Tags, Damage Numbers, and Pickup Labels

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-103, US-104, US-105
**Paired story:** none

## Overview
Replace prototype world-facing labels and number pops with the Tin Tally family while preserving existing combat/inventory behavior. Enemy tags gain posture/tier readability, damage numbers move to a bounded UMG pool, and pickup labels replace the current TextRender presentation.

## Execution Mode
**HEADLESS**
Native widgets/components use already-cooked foundation assets; no designer WBP is required.

## References
- `Design/TinTally/ds/project/components/{EnemyTag,DamageNumber,PickupLabel}/README.md`
- `Design/TinTally/ds/project/02-hud-and-pacing.md` — world-space map
- Existing `MordecaiEnemyIndicator*`, `MordecaiDamagePopComponent.*`, pickup actors
- `Specs/control_bindings_v1.1.md` — Interact/combat inputs
- Issue #81

## Acceptance Criteria
- [ ] AC-110.1: EnemyTag keeps `NameText`, `HealthProgressBar`, `StatusIconContainer`, appears under documented aggro/tier rules at Z+120/150 wide, and shows posture from the first hit with redundant tier/status signals.
- [ ] AC-110.2: US-092's revived-enemy refresh remains covered by a targeted regression; tags never retain stale pre-death values.
- [ ] AC-110.3: a HUD-root canvas owns a preallocated pool of 20 `UMordecaiDamagePopWidget`s; no widget is constructed per hit and oldest entries recycle deterministically.
- [ ] AC-110.4: damage numbers show actual health taken, block/crit/heal/type rules, no Physical glyph, and Shadow/Radiant special outlines; projection/fan/lifetime match the card.
- [ ] AC-110.5: pickup label widget replaces player-facing `NameLabel`, remains camera-readable, changes far/near/focused state from real distance/focus, and agrees with the interact prompt.
- [ ] AC-110.6: the same commit updates bridge `actors[].text_labels` and US-079 to read the widget `NameText`; no duplicate `UTextRenderComponent` remains in shipping.
- [ ] AC-110.7: representative light/dark-floor captures pass in both themes and combat/inventory scripts have no regression.

## Technical Notes
- Many-instance widgets may use the manifest's SDF rounded-rect material if profiling requires it; colors still come from the theme.
- Keep pool ownership inside the one HUD root.
- Reuse interaction focus source; do not duplicate pickup eligibility.
- Pickup bindings are exactly `RarityBand`, `RaritySocket`, `UnknownMark`, `NameText`, `InteractKey`, and `FocusRing`; each has a native fallback.
- TODO(DECISION): confirm PickupLabel far threshold; use the card's example only in tests until approved.
- TODO(DECISION): Physical subtype glyph remains absent unless Jeff overturns D15.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.EnemyTag.ReviveRefreshesValues`
- `Mordecai.UI.DamageNumber.PoolCapAndFormatting`
- `Mordecai.UI.PickupLabel.FocusAndFacing`
### Tier 2 — PIE smoke (editor integration)
- Extend enemy/HUD/inventory smoke for indicator registration, pool ownership and label replacement.
### Tier 3 — Packaged playtest
- CombatTestMap and DevTestMap captures on representative floors.
### Tier 4 — Real-input playtest
- Combat/death/revive paths and US-079 pickup path; add exact visual-state checks where bridge data supports them.

## Playtest
- **Launch path:** CombatTestMap and DevTestMap
- **Player steps:** aggro/damage/kill/revive enemy; deal physical/typed/crit/blocked damage; approach/focus/pick up item
- **Visible on screen:** fresh enemy values/posture, bounded formatted pops, readable focused pickup label and matching prompt
- **Proof:** Tier 4 gameplay state plus both-theme gallery/full-frame captures
- **Tier 4 script:** extend existing combat/inventory scripts and fake-bridge tests
- **Steps:** generated/linted with the final checks
- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked; red tests fail for intended missing behavior first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] CombatTestMap and DevTestMap Tier 3 boots exit 0 — never `--allow-stale`
- [ ] Affected combat/inventory Tier 4 stories and `--lint` pass; full sweep has no baseline PASS to FAIL
- [ ] #81's existing US-092 fix remains passing and bridge migration is evidenced; both-theme/floor previews/diffs and every gameplay screenshot are opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-110]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
<Filled in by the implementing agent.>
