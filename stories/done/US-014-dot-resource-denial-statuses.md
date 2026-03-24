# US-014: DoT & Resource Denial Statuses (Burning, Bleeding, Poisoned)

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement the three status effects that deal damage over time or deny resource recovery: Burning (periodic fire damage + cast interruption), Bleeding (healing reduction + hit-refresh timer), and Poisoned (stamina regen suppression + movement reduction). These share the "pressure over time" design identity — the player must actively counterplay to escape them.

## References
- Design doc: `status_effects_launch_set_v1.md` — Burning, Bleeding, Poisoned rows (identity, mechanics, counterplay, sources, cleanses)
- Design doc: `damage_types_v1.md` — Fire→Burning, Physical→Bleeding, Poison→Poisoned relationships
- Design doc: `combat_system_v1.md` — Damage formula (StatusModifier slot)
- Design doc: `character_attributes_v1.md` — RES (Affliction Resist), CON (Health Regen)
- Agent rules: `agent_rules_v2.md` — GAS for all status effects; `TODO(DECISION)` for stacking policy
- Depends on: US-013 (Status Effect Framework)

---

## Acceptance Criteria

### Burning
- [x] AC-014.1: `UMordecaiGE_Burning` (subclass of `UMordecaiStatusEffectGameplayEffect`) exists. Applies `Mordecai.Status.Burning` tag. Duration: configurable `BurningDurationSec` (default placeholder: 5.0s).
- [x] AC-014.2: Burning deals periodic fire damage every tick. Period: configurable `BurningTickIntervalSec` (default: 1.0s). Damage per tick: configurable `BurningDamagePerTick` (default placeholder: 3.0). Damage type tag: `Mordecai.Damage.Fire`.
- [x] AC-014.3: Each Burning tick has a configurable chance to interrupt casting: `BurningInterruptChance` (default placeholder: 15%, i.e., 0.15). Implementation: on periodic execute, if target has `Mordecai.State.Casting` tag, roll against `BurningInterruptChance` and, if successful, send `Mordecai.Event.CastInterrupted` gameplay event. Actual cast interruption handling deferred to Epic 5 (Magic System).
- [x] AC-014.4: Re-applying Burning while already burning refreshes the duration (per framework default stacking).
- [x] AC-014.5: Burning is blocked if target has `Mordecai.Immunity.Burning` tag.

### Bleeding
- [x] AC-014.6: `UMordecaiGE_Bleeding` exists. Applies `Mordecai.Status.Bleeding` tag. Duration: configurable `BleedingDurationSec` (default placeholder: 6.0s).
- [x] AC-014.7: While Bleeding is active, healing received is reduced by a configurable percentage: `BleedingHealReductionPct` (default placeholder: 0.50, meaning 50% reduction). Implementation: apply an attribute modifier to a `HealingReceivedModifier` attribute (or use a GE magnitude modifier on incoming healing GEs). If `HealingReceivedModifier` attribute doesn't exist yet, create it on `UMordecaiAttributeSet`.
- [x] AC-014.8: Hit-refresh mechanic: when the bleeding target takes any damage (from any source), the Bleeding duration resets to its full value. Implementation: listen for `Mordecai.Event.DamageTaken` gameplay event while Bleeding is active; on receive, remove and re-apply the Bleeding GE.
- [x] AC-014.9: Clot mechanic: if the target avoids taking damage for a configurable `BleedingClotTimeSec` (default placeholder: 4.0s), Bleeding is removed early. Implementation: track time since last damage; if it exceeds `BleedingClotTimeSec`, remove the GE. This timer resets on every damage taken.
- [x] AC-014.10: Bleeding is blocked if target has `Mordecai.Immunity.Bleeding` tag.

### Poisoned
- [x] AC-014.11: `UMordecaiGE_Poisoned` exists. Applies `Mordecai.Status.Poisoned` tag. Duration: configurable `PoisonedDurationSec` (default placeholder: 8.0s).
- [x] AC-014.12: While Poisoned is active, stamina regen rate is reduced by a configurable percentage: `PoisonedStaminaRegenReductionPct` (default placeholder: 0.60, meaning 60% reduction). Implementation: apply a GE modifier to `StaminaRegenRate` attribute (or equivalent stamina regen modifier). If stamina regen attribute doesn't exist, add it to `UMordecaiAttributeSet`.
- [x] AC-014.13: While Poisoned is active, movement speed is reduced by a configurable percentage: `PoisonedMoveSpeedReductionPct` (default placeholder: 0.15, meaning 15% reduction). Implementation: apply a GE modifier to `MoveSpeed` attribute (or use Character Movement Component max walk speed modifier).
- [x] AC-014.14: Poisoned is blocked if target has `Mordecai.Immunity.Poisoned` tag.

## Technical Notes
- Place GE subclasses in `Source/LyraGame/Mordecai/StatusEffects/Effects/`
- Each status gets its own `.h/.cpp` pair: `MordecaiGE_Burning.h/.cpp`, etc.
- Burning periodic damage: use GAS periodic execution. The GE's `Period` field drives tick timing. On each period, apply an instant damage GE or use `UGameplayEffectExecutionCalculation` to deal damage.
- Bleeding hit-refresh: use an `AbilityTask` or listen for a gameplay event (`Mordecai.Event.DamageTaken`). Since Bleeding is a GE (not an ability), the hit-refresh logic may need a helper component or a custom GE execution that checks for the tag and re-applies. Consider using `UMordecaiStatusEffectComponent` to manage this listener.
- Bleeding clot timer: similar approach — track via component. On each damage event, reset timer. On timer expiry, remove Bleeding GE.
- Poisoned movement reduction: prefer modifying an attribute that the Character Movement Component reads, rather than directly setting CMC values. If no `MoveSpeed` attribute exists, add one to `UMordecaiAttributeSet` with a default of 1.0 (multiplier). CMC reads `MaxWalkSpeed * MoveSpeedMultiplier`.
- Healing reduction: may need a `HealingReceivedMultiplier` attribute on the attribute set. Healing GEs should read this multiplier. If it doesn't exist yet, add it.
- New attributes added here (`HealingReceivedMultiplier`, `StaminaRegenRate`, `MoveSpeedMultiplier`) should be initialized with neutral defaults (1.0 for multipliers, base regen rate for regen).
- All numeric values are placeholders. Use `UPROPERTY(EditDefaultsOnly)` so designers can tune via subclasses or CDO.

## Tests Required

### Burning Tests
- [x] `Mordecai.StatusEffect.Burning.AppliesTagAndDuration` — apply Burning GE, verify `Mordecai.Status.Burning` tag is active and removed after duration (AC-014.1)
- [x] `Mordecai.StatusEffect.Burning.DealsPeriodicDamage` — apply Burning, advance time by 3 ticks, verify health reduced by ~3x `BurningDamagePerTick` (AC-014.2)
- [x] `Mordecai.StatusEffect.Burning.InterruptsCastingOnChance` — apply Burning to target with `Mordecai.State.Casting` tag, force interrupt chance to 1.0 for test, verify `Mordecai.Event.CastInterrupted` event sent (AC-014.3)
- [x] `Mordecai.StatusEffect.Burning.RefreshesDurationOnReapply` — apply Burning, wait 2s, re-apply, verify duration resets (AC-014.4)
- [x] `Mordecai.StatusEffect.Burning.BlockedByImmunity` — add `Mordecai.Immunity.Burning` tag, apply Burning, verify not applied (AC-014.5)

### Bleeding Tests
- [x] `Mordecai.StatusEffect.Bleeding.ReducesHealingReceived` — apply Bleeding, apply a healing GE, verify healing amount reduced by `BleedingHealReductionPct` (AC-014.7)
- [x] `Mordecai.StatusEffect.Bleeding.HitRefreshesDuration` — apply Bleeding, wait 3s, deal damage to target, verify Bleeding duration reset (AC-014.8)
- [x] `Mordecai.StatusEffect.Bleeding.ClotsAfterNoDamage` — apply Bleeding, wait `BleedingClotTimeSec` without dealing damage, verify Bleeding removed early (AC-014.9)
- [x] `Mordecai.StatusEffect.Bleeding.BlockedByImmunity` — add immunity, apply, verify not applied (AC-014.10)

### Poisoned Tests
- [x] `Mordecai.StatusEffect.Poisoned.ReducesStaminaRegen` — apply Poisoned, verify stamina regen rate reduced by `PoisonedStaminaRegenReductionPct` (AC-014.12)
- [x] `Mordecai.StatusEffect.Poisoned.ReducesMoveSpeed` — apply Poisoned, verify movement speed reduced by `PoisonedMoveSpeedReductionPct` (AC-014.13)
- [x] `Mordecai.StatusEffect.Poisoned.BlockedByImmunity` — add immunity, apply, verify not applied (AC-014.14)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-014]` prefix
