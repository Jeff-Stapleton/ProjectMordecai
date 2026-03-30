# US-059: Perception & Mental Debuffs (Blinded, Fear, Cursed)

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement three debuff statuses that degrade perception and mental composure: Blinded (reduces accuracy and detection), Fear (amplifies stamina penalties and reduces guard stability), and Cursed (reduces spell regen and healing, disables some buffs). These are "soft degradation" debuffs — they worsen stats and effectiveness rather than categorically blocking actions.

Split from original US-017 scope. Silenced and Rooted are in US-017.

## References
- Design doc: `status_effects_launch_set_v1.md` — Blinded ("Reads become unreliable"), Fear ("Composure collapse"), Cursed ("Resource rules broken")
- Design doc: `game_design_v2.md` — Section 4 (Defense mechanics, Stamina tiers), Section 2 (Skill-first combat)
- Design doc: `character_attributes_v1.md` — WIS (Magic Dmg scaling), CHA (Cast Speed), RES (Affliction Resist)
- Agent rules: `agent_rules_v2.md` — GAS for all status effects; `TODO(DECISION)` for stacking policy
- Depends on: US-013 (Status Effect Framework), US-017 (shares attribute definitions)

---

## Acceptance Criteria

### Blinded
- [ ] AC-059.1: `UMordecaiGE_Blinded` exists. Applies `Mordecai.Status.Blinded` tag. Duration: configurable `BlindedDurationSec` (default placeholder: 4.0s).
- [ ] AC-059.2: While Blinded, ranged accuracy is reduced. Implementation: modify `RangedAccuracyMultiplier` attribute (add to AttributeSet if needed, default 1.0) by `BlindedAccuracyReductionPct` (default placeholder: 0.40, meaning 40% reduction). US-009 (Projectile/Aim Assist) reads this.
- [ ] AC-059.3: While Blinded, aim assist effectiveness is reduced. Implementation: modify `AimAssistMultiplier` attribute (add to AttributeSet if needed, default 1.0) by `BlindedAimAssistReductionPct` (default placeholder: 0.50, meaning 50% reduction). US-009 reads this.
- [ ] AC-059.4: While Blinded, stealth detection range is reduced. Implementation: modify `StealthDetectionMultiplier` attribute (add to AttributeSet if needed, default 1.0) by `BlindedDetectionReductionPct` (default placeholder: 0.60, meaning 60% reduction). Stealth system reads this when implemented.
- [ ] AC-059.5: Perfect dodge grants brief clarity: when `Mordecai.State.PerfectDodge` triggers while Blinded, temporarily suppress Blinded modifiers for `BlindedClarityWindowMs` (default placeholder: 1000ms). Interface only — wiring deferred.
- [ ] AC-059.6: Blinded is blocked if target has `Mordecai.Immunity.Blinded` tag.

### Fear
- [ ] AC-059.7: `UMordecaiGE_Fear` exists. Applies `Mordecai.Status.Fear` tag. Duration: configurable `FearDurationSec` (default placeholder: 5.0s).
- [ ] AC-059.8: While Fear is active, stamina tier penalties are amplified by a configurable multiplier: `FearStaminaTierPenaltyMultiplier` (default placeholder: 1.50, meaning 50% worse tier penalties). Implementation: modify `StaminaTierPenaltyMultiplier` attribute (add to AttributeSet if needed, default 1.0). US-008 (Stamina Tiers) reads this.
- [ ] AC-059.9: While Fear is active, guard stability is reduced by a configurable percentage: `FearGuardStabilityReductionPct` (default placeholder: 0.25, meaning 25% reduction). Implementation: modify `BlockStabilityMultiplier` attribute (same as Corroded uses in US-016).
- [ ] AC-059.10: While Fear is active, ranged aim has slight drift: `FearAimDriftPct` (default placeholder: 0.15). Implementation: modify `RangedAccuracyMultiplier` (stacks with Blinded if both active).
- [ ] AC-059.11: Resolve mechanic: `Mordecai.State.PerfectBlock` or `Mordecai.State.PerfectParry` while Fear is active grants `Mordecai.Status.Resolved` which cancels Fear immediately. Register `Mordecai.Status.Resolved` tag. Interface only.
- [ ] AC-059.12: Fear is blocked if target has `Mordecai.Immunity.Fear` tag.

### Cursed
- [ ] AC-059.13: `UMordecaiGE_Cursed` exists. Applies `Mordecai.Status.Cursed` tag. Duration: configurable `CursedDurationSec` (default placeholder: 10.0s).
- [ ] AC-059.14: While Cursed, spell point regen is reduced by a configurable percentage: `CursedSpellRegenReductionPct` (default placeholder: 0.50, meaning 50% reduction). Implementation: modify `SpellPointRegenMultiplier` attribute (from US-017).
- [ ] AC-059.15: While Cursed, healing received is reduced by a configurable percentage: `CursedHealReductionPct` (default placeholder: 0.30, meaning 30% reduction). Implementation: modify `HealingReceivedMultiplier` attribute (same as Bleeding uses from US-014; effects stack if both active).
- [ ] AC-059.16: While Cursed, certain buffs are disabled. Implementation: add `Mordecai.Status.Cursed` as a removal tag for buff GEs that should be suppressed. `TODO(DECISION)` — which buffs are disabled by Cursed. For now, document that buff GEs should check for `Mordecai.Status.Cursed` in their `OngoingTagRequirements` if they're curse-suppressible.
- [ ] AC-059.17: Cursed is blocked if target has `Mordecai.Immunity.Cursed` tag.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/StatusEffects/Effects/`
- New attributes needed on `UMordecaiAttributeSet`:
  - `RangedAccuracyMultiplier` (default 1.0) — read by projectile/aim system
  - `AimAssistMultiplier` (default 1.0) — read by aim assist subsystem
  - `StealthDetectionMultiplier` (default 1.0) — read by stealth system (future)
  - `StaminaTierPenaltyMultiplier` (default 1.0) — read by stamina tier system
- Reuses attributes from earlier stories: `HealingReceivedMultiplier` (US-014), `BlockStabilityMultiplier` (US-016), `SpellPointRegenMultiplier` (US-017). Multiple statuses modifying the same attribute is expected and should compose correctly.
- Register new tags: `Mordecai.Status.Resolved`.
- These debuffs are formulaic GE + attribute modifier patterns. The coding agent should follow the established pattern from US-014/016/017.

## Tests Required

### Blinded Tests
- [ ] `Mordecai.StatusEffect.Blinded.ReducesAccuracy` — apply Blinded, verify `RangedAccuracyMultiplier` reduced (AC-059.2)
- [ ] `Mordecai.StatusEffect.Blinded.ReducesAimAssist` — apply Blinded, verify `AimAssistMultiplier` reduced (AC-059.3)
- [ ] `Mordecai.StatusEffect.Blinded.ReducesDetection` — apply Blinded, verify `StealthDetectionMultiplier` reduced (AC-059.4)
- [ ] `Mordecai.StatusEffect.Blinded.BlockedByImmunity` — add immunity, apply, verify not applied (AC-059.6)

### Fear Tests
- [ ] `Mordecai.StatusEffect.Fear.AmplifiesStaminaPenalties` — apply Fear, verify `StaminaTierPenaltyMultiplier` increased (AC-059.8)
- [ ] `Mordecai.StatusEffect.Fear.ReducesGuardStability` — apply Fear, verify `BlockStabilityMultiplier` reduced (AC-059.9)
- [ ] `Mordecai.StatusEffect.Fear.AppliesAimDrift` — apply Fear, verify `RangedAccuracyMultiplier` reduced (AC-059.10)
- [ ] `Mordecai.StatusEffect.Fear.BlockedByImmunity` — add immunity, apply, verify not applied (AC-059.12)

### Cursed Tests
- [ ] `Mordecai.StatusEffect.Cursed.ReducesSpellRegen` — apply Cursed, verify `SpellPointRegenMultiplier` reduced by expected amount (AC-059.14)
- [ ] `Mordecai.StatusEffect.Cursed.ReducesHealing` — apply Cursed, verify `HealingReceivedMultiplier` reduced (AC-059.15)
- [ ] `Mordecai.StatusEffect.Cursed.BlockedByImmunity` — add immunity, apply, verify not applied (AC-059.17)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-059]` prefix
