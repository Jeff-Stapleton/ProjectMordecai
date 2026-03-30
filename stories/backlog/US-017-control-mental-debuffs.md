# US-017: Action-Restricting Debuffs (Silenced, Rooted)

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement two debuff statuses that restrict player actions: Silenced (blocks spellcasting and pauses SP regen) and Rooted (blocks movement but allows combat actions). These are the "hard restriction" debuffs — they categorically disable a capability rather than degrading stats.

Split from original US-017 scope. Blinded, Fear, and Cursed moved to US-059.

## References
- Design doc: `status_effects_launch_set_v1.md` — Silenced ("Magic is shut off"), Rooted ("Position solved or punished")
- Design doc: `character_attributes_v1.md` — RES (Affliction Resist)
- Agent rules: `agent_rules_v2.md` — GAS for all status effects; `TODO(DECISION)` for stacking policy
- Depends on: US-013 (Status Effect Framework)

---

## Acceptance Criteria

### Silenced
- [ ] AC-017.1: `UMordecaiGE_Silenced` exists. Applies `Mordecai.Status.Silenced` tag. Duration: configurable `SilencedDurationSec` (default placeholder: 5.0s).
- [ ] AC-017.2: While Silenced, spell abilities cannot be activated. Implementation: all spell abilities should have `Mordecai.Status.Silenced` as a blocking tag in their `ActivationBlockedTags`. Since spell abilities don't exist yet (Epic 5), register tag `Mordecai.Ability.Category.Spell` and document that all spell abilities must include `Mordecai.Status.Silenced` in their blocked tags.
- [ ] AC-017.3: While Silenced, spell point regen is paused. Implementation: set `SpellPointRegenMultiplier` to 0. Add `SpellPointRegenMultiplier` attribute if needed (default 1.0). `TODO(DECISION)` — whether SP regen pause is mandatory or optional per design doc note.
- [ ] AC-017.4: While Silenced, weapon skills are still usable unless they have the `Mordecai.Ability.Category.Magical` tag. Non-magical weapon abilities remain unaffected.
- [ ] AC-017.5: Silenced is blocked if target has `Mordecai.Immunity.Silenced` tag.

### Rooted
- [ ] AC-017.6: `UMordecaiGE_Rooted` exists. Applies `Mordecai.Status.Rooted` tag. Duration: configurable `RootedDurationSec` (default placeholder: 3.0s).
- [ ] AC-017.7: While Rooted, character movement is blocked. Implementation: set `MoveSpeedMultiplier` to 0 (or add `Mordecai.Status.Rooted` as a movement-blocking tag that the CMC checks). The character can still rotate/face but cannot translate.
- [ ] AC-017.8: While Rooted, the character CAN still attack, block, cast spells, and use abilities. Only movement (translation) is prevented.
- [ ] AC-017.9: Break-free action: a heavy stamina cost action (`RootedBreakFreeStaminaCost`, default placeholder: 25.0) can remove Rooted early. Implementation: register `Mordecai.Event.BreakFree` gameplay event. When received while Rooted, consume stamina and remove Root GE. The actual break-free input binding is deferred.
- [ ] AC-017.10: Root duration refreshes if the character remains in a root zone. Implementation: re-applying Rooted refreshes duration per framework stacking default.
- [ ] AC-017.11: Rooted is blocked if target has `Mordecai.Immunity.Rooted` tag.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/StatusEffects/Effects/`
- New attributes needed on `UMordecaiAttributeSet`:
  - `SpellPointRegenMultiplier` (default 1.0) — read by SP regen system
- Reuses `MoveSpeedMultiplier` from US-014.
- Silenced blocking spells: use GAS tag requirements. The Silenced tag being present blocks ability activation for abilities that have it in their `ActivationBlockedTags`. Standard GAS pattern.
- Rooted blocking movement: prefer a tag-based check in the movement component over zeroing the speed attribute, as zeroing speed has side effects (animation, physics). Check for `Mordecai.Status.Rooted` in the movement tick and skip movement input processing.
- Register new tags: `Mordecai.Ability.Category.Spell`, `Mordecai.Ability.Category.Magical`, `Mordecai.Event.BreakFree`.

## Tests Required

### Silenced Tests
- [ ] `Mordecai.StatusEffect.Silenced.AppliesTag` — apply Silenced, verify tag active (AC-017.1)
- [ ] `Mordecai.StatusEffect.Silenced.PausesSpellRegen` — apply Silenced, verify `SpellPointRegenMultiplier` is 0 (AC-017.3)
- [ ] `Mordecai.StatusEffect.Silenced.BlockedByImmunity` — add immunity, apply, verify not applied (AC-017.5)

### Rooted Tests
- [ ] `Mordecai.StatusEffect.Rooted.AppliesTag` — apply Rooted, verify tag active (AC-017.6)
- [ ] `Mordecai.StatusEffect.Rooted.BlocksMovement` — apply Rooted, verify movement is prevented (check MoveSpeedMultiplier or tag presence) (AC-017.7)
- [ ] `Mordecai.StatusEffect.Rooted.AllowsAttackAndBlock` — apply Rooted, attempt to activate a non-movement ability, verify it works (AC-017.8)
- [ ] `Mordecai.StatusEffect.Rooted.BreakFreeConsumesStamina` — send BreakFree event while Rooted, verify Root removed and stamina reduced (AC-017.9)
- [ ] `Mordecai.StatusEffect.Rooted.RefreshesDurationOnReapply` — apply Rooted, wait, re-apply, verify duration refreshed (AC-017.10)
- [ ] `Mordecai.StatusEffect.Rooted.BlockedByImmunity` — add immunity, apply, verify not applied (AC-017.11)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-017]` prefix
