# US-017: Control & Mental Debuffs (Silenced, Rooted, Blinded, Fear, Cursed)

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement five debuff statuses that restrict player actions or impose mental/psychological pressure: Silenced (blocks spellcasting), Rooted (blocks movement), Blinded (reduces accuracy and detection), Fear (amplifies stamina penalties and reduces guard stability), and Cursed (reduces spell regen and healing, disables some buffs). These range from simple tag-based blocks to multi-modifier debuffs.

## References
- Design doc: `status_effects_launch_set_v1.md` — Silenced ("Magic is shut off"), Rooted ("Position solved or punished"), Blinded ("Reads become unreliable"), Fear ("Composure collapse"), Cursed ("Resource rules broken")
- Design doc: `game_design_v2.md` — Section 4 (Defense mechanics, Stamina tiers), Section 2 (Skill-first combat)
- Design doc: `character_attributes_v1.md` — WIS (Magic Dmg scaling), CHA (Cast Speed), RES (Affliction Resist)
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

### Blinded
- [ ] AC-017.12: `UMordecaiGE_Blinded` exists. Applies `Mordecai.Status.Blinded` tag. Duration: configurable `BlindedDurationSec` (default placeholder: 4.0s).
- [ ] AC-017.13: While Blinded, ranged accuracy is reduced. Implementation: modify `RangedAccuracyMultiplier` attribute (add to AttributeSet if needed, default 1.0) by `BlindedAccuracyReductionPct` (default placeholder: 0.40, meaning 40% reduction). US-009 (Projectile/Aim Assist) reads this.
- [ ] AC-017.14: While Blinded, aim assist effectiveness is reduced. Implementation: modify `AimAssistMultiplier` attribute (add to AttributeSet if needed, default 1.0) by `BlindedAimAssistReductionPct` (default placeholder: 0.50, meaning 50% reduction). US-009 reads this.
- [ ] AC-017.15: While Blinded, stealth detection range is reduced. Implementation: modify `StealthDetectionMultiplier` attribute (add to AttributeSet if needed, default 1.0) by `BlindedDetectionReductionPct` (default placeholder: 0.60, meaning 60% reduction). Stealth system reads this when implemented.
- [ ] AC-017.16: Perfect dodge grants brief clarity: when `Mordecai.State.PerfectDodge` triggers while Blinded, temporarily suppress Blinded modifiers for `BlindedClarityWindowMs` (default placeholder: 1000ms). Interface only — wiring deferred.
- [ ] AC-017.17: Blinded is blocked if target has `Mordecai.Immunity.Blinded` tag.

### Fear
- [ ] AC-017.18: `UMordecaiGE_Fear` exists. Applies `Mordecai.Status.Fear` tag. Duration: configurable `FearDurationSec` (default placeholder: 5.0s).
- [ ] AC-017.19: While Fear is active, stamina tier penalties are amplified by a configurable multiplier: `FearStaminaTierPenaltyMultiplier` (default placeholder: 1.50, meaning 50% worse tier penalties). Implementation: modify `StaminaTierPenaltyMultiplier` attribute (add to AttributeSet if needed, default 1.0). US-008 (Stamina Tiers) reads this.
- [ ] AC-017.20: While Fear is active, guard stability is reduced by a configurable percentage: `FearGuardStabilityReductionPct` (default placeholder: 0.25, meaning 25% reduction). Implementation: modify `BlockStabilityMultiplier` attribute (same as Corroded uses).
- [ ] AC-017.21: While Fear is active, ranged aim has slight drift: `FearAimDriftPct` (default placeholder: 0.15). Implementation: modify `RangedAccuracyMultiplier` (stacks with Blinded if both active).
- [ ] AC-017.22: Resolve mechanic: `Mordecai.State.PerfectBlock` or `Mordecai.State.PerfectParry` while Fear is active grants `Mordecai.Status.Resolved` which cancels Fear immediately. Register `Mordecai.Status.Resolved` tag. Interface only.
- [ ] AC-017.23: Fear is blocked if target has `Mordecai.Immunity.Fear` tag.

### Cursed
- [ ] AC-017.24: `UMordecaiGE_Cursed` exists. Applies `Mordecai.Status.Cursed` tag. Duration: configurable `CursedDurationSec` (default placeholder: 10.0s).
- [ ] AC-017.25: While Cursed, spell point regen is reduced by a configurable percentage: `CursedSpellRegenReductionPct` (default placeholder: 0.50, meaning 50% reduction). Implementation: modify `SpellPointRegenMultiplier` attribute.
- [ ] AC-017.26: While Cursed, healing received is reduced by a configurable percentage: `CursedHealReductionPct` (default placeholder: 0.30, meaning 30% reduction). Implementation: modify `HealingReceivedMultiplier` attribute (same as Bleeding uses; effects stack if both active).
- [ ] AC-017.27: While Cursed, certain buffs are disabled. Implementation: add `Mordecai.Status.Cursed` as a removal tag for buff GEs that should be suppressed. `TODO(DECISION)` — which buffs are disabled by Cursed. For now, document that buff GEs should check for `Mordecai.Status.Cursed` in their `OngoingTagRequirements` if they're curse-suppressible.
- [ ] AC-017.28: Cursed is blocked if target has `Mordecai.Immunity.Cursed` tag.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/StatusEffects/Effects/`
- New attributes needed on `UMordecaiAttributeSet`:
  - `SpellPointRegenMultiplier` (default 1.0) — read by SP regen system
  - `RangedAccuracyMultiplier` (default 1.0) — read by projectile/aim system
  - `AimAssistMultiplier` (default 1.0) — read by aim assist subsystem
  - `StealthDetectionMultiplier` (default 1.0) — read by stealth system
  - `StaminaTierPenaltyMultiplier` (default 1.0) — read by stamina tier system
- Some attributes were already introduced by earlier stories (`HealingReceivedMultiplier` from US-014, `MoveSpeedMultiplier` from US-014, `BlockStabilityMultiplier` from US-016). Reuse these — multiple statuses modifying the same attribute is expected and should compose correctly.
- Silenced blocking spells: use GAS tag requirements. The Silenced tag being present blocks ability activation for abilities that have it in their `ActivationBlockedTags`. This is standard GAS pattern.
- Rooted blocking movement: prefer a tag-based check in the movement component over zeroing the speed attribute, as zeroing speed has side effects (animation, physics). Check for `Mordecai.Status.Rooted` in the movement tick and skip movement input processing.
- Register new tags: `Mordecai.Ability.Category.Spell`, `Mordecai.Ability.Category.Magical`, `Mordecai.Event.BreakFree`, `Mordecai.Status.Resolved`.

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

### Blinded Tests
- [ ] `Mordecai.StatusEffect.Blinded.ReducesAccuracy` — apply Blinded, verify `RangedAccuracyMultiplier` reduced (AC-017.13)
- [ ] `Mordecai.StatusEffect.Blinded.ReducesAimAssist` — apply Blinded, verify `AimAssistMultiplier` reduced (AC-017.14)
- [ ] `Mordecai.StatusEffect.Blinded.ReducesDetection` — apply Blinded, verify `StealthDetectionMultiplier` reduced (AC-017.15)
- [ ] `Mordecai.StatusEffect.Blinded.BlockedByImmunity` — add immunity, apply, verify not applied (AC-017.17)

### Fear Tests
- [ ] `Mordecai.StatusEffect.Fear.AmplifiesStaminaPenalties` — apply Fear, verify `StaminaTierPenaltyMultiplier` increased (AC-017.19)
- [ ] `Mordecai.StatusEffect.Fear.ReducesGuardStability` — apply Fear, verify `BlockStabilityMultiplier` reduced (AC-017.20)
- [ ] `Mordecai.StatusEffect.Fear.AppliesAimDrift` — apply Fear, verify `RangedAccuracyMultiplier` reduced (AC-017.21)
- [ ] `Mordecai.StatusEffect.Fear.BlockedByImmunity` — add immunity, apply, verify not applied (AC-017.23)

### Cursed Tests
- [ ] `Mordecai.StatusEffect.Cursed.ReducesSpellRegen` — apply Cursed, verify `SpellPointRegenMultiplier` reduced by expected amount (AC-017.25)
- [ ] `Mordecai.StatusEffect.Cursed.ReducesHealing` — apply Cursed, verify `HealingReceivedMultiplier` reduced (AC-017.26)
- [ ] `Mordecai.StatusEffect.Cursed.BlockedByImmunity` — add immunity, apply, verify not applied (AC-017.28)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-017]` prefix
