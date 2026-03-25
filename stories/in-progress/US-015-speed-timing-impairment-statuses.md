# US-015: Speed & Timing Impairment Statuses (Frostbitten, Shocked)

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement the two status effects that disrupt movement speed and action timing: Frostbitten (progressive slow + freeze at high stacks) and Shocked (micro-stun risk on repeated hits + increased block stamina cost). Both have stacking mechanics that escalate pressure — they punish players who stay in dangerous zones without counterplaying.

## References
- Design doc: `status_effects_launch_set_v1.md` — Frostbitten row ("Sluggish body, slower reactions"), Shocked row ("Timing disruption")
- Design doc: `damage_types_v1.md` — Frost→Frostbitten, Lightning→Shocked relationships; Frost synergy vs Drenched targets
- Design doc: `combat_system_v1.md` — Block stamina cost mechanics
- Design doc: `character_attributes_v1.md` — END (Stamina), RES (Affliction Resist)
- Agent rules: `agent_rules_v2.md` — GAS for all status effects; `TODO(DECISION)` for stacking policy
- Depends on: US-013 (Status Effect Framework)

---

## Acceptance Criteria

### Frostbitten
- [ ] AC-015.1: `UMordecaiGE_Frostbitten` exists. Applies `Mordecai.Status.Frostbitten` tag. Duration: configurable `FrostbittenDurationSec` (default placeholder: 6.0s).
- [ ] AC-015.2: While Frostbitten, movement speed is reduced by a configurable percentage: `FrostbittenMoveSpeedReductionPct` (default placeholder: 0.25, meaning 25% reduction). Uses `MoveSpeedMultiplier` attribute from US-014.
- [ ] AC-015.3: While Frostbitten, attack speed is reduced by a configurable percentage: `FrostbittenAttackSpeedReductionPct` (default placeholder: 0.20, meaning 20% reduction). Implementation: apply modifier to `AttackSpeedMultiplier` attribute on AttributeSet. If it doesn't exist, add it (default 1.0).
- [ ] AC-015.4: While Frostbitten, dodge recovery time is increased by a configurable percentage: `FrostbittenDodgeRecoveryIncreasePct` (default placeholder: 0.30, meaning 30% longer recovery). Implementation: apply modifier to `DodgeRecoveryMultiplier` attribute. If it doesn't exist, add it (default 1.0). US-005 (Dodge) reads this multiplier.
- [ ] AC-015.5: Frost stack mechanic: Frostbitten uses stack count (not just duration refresh). Each application adds 1 stack, up to configurable `FrostbittenMaxStacks` (default placeholder: 5). At max stacks, apply `Mordecai.Status.Frozen` tag for a configurable `FrozenDurationSec` (default placeholder: 2.0s), then remove all Frostbitten stacks.
- [ ] AC-015.6: `Mordecai.Status.Frozen` gameplay tag registered. While Frozen, the character cannot move, attack, block, or dodge (full incapacitation). Frozen ends after `FrozenDurationSec`.
- [ ] AC-015.7: Perfect dodge breaks Frostbitten stacks: when `Mordecai.State.PerfectDodge` triggers while Frostbitten, remove 1 stack (or all stacks — `TODO(DECISION)`). Interface only — actual trigger wiring deferred to when US-005 is integrated.
- [ ] AC-015.8: Frostbitten is blocked if target has `Mordecai.Immunity.Frostbitten` tag.

### Shocked
- [ ] AC-015.9: `UMordecaiGE_Shocked` exists. Applies `Mordecai.Status.Shocked` tag. Duration: configurable `ShockedDurationSec` (default placeholder: 5.0s).
- [ ] AC-015.10: Shocked uses stack count. Each application adds 1 stack, up to configurable `ShockedMaxStacks` (default placeholder: 3). More stacks = higher micro-stun chance (see AC-015.11).
- [ ] AC-015.11: Micro-stun mechanic: when a Shocked target takes a hit, roll against `ShockedMicroStunChancePerStack` (default placeholder: 0.15 per stack). On success, apply `Mordecai.Status.MicroStunned` tag for `ShockedMicroStunDurationMs` (default placeholder: 300ms). MicroStunned briefly interrupts the current action (cancels active ability, if any). Register `Mordecai.Status.MicroStunned` tag.
- [ ] AC-015.12: While Shocked, block stamina cost is increased by a configurable multiplier per stack: `ShockedBlockStaminaCostMultiplierPerStack` (default placeholder: 0.20, meaning +20% per stack). Implementation: modify `BlockStaminaCostMultiplier` attribute. If it doesn't exist, add it (default 1.0). US-006 (Block/Parry) reads this.
- [ ] AC-015.13: Shocked can interrupt long casts (>1s): each tick or on-hit while Shocked, if target has `Mordecai.State.Casting` tag, roll `ShockedCastInterruptChance` (default placeholder: 0.25 per stack). On success, send `Mordecai.Event.CastInterrupted`. Actual handling deferred to Epic 5.
- [ ] AC-015.14: Perfect block clears 1 Shocked stack: interface for `Mordecai.State.PerfectBlock` → remove 1 stack. Wiring deferred to US-006 integration.
- [ ] AC-015.15: Shocked is blocked if target has `Mordecai.Immunity.Shocked` tag.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/StatusEffects/Effects/`
- Frostbitten stacking: use GAS `StackLimitCount` = `FrostbittenMaxStacks`. On reaching max, trigger freeze via a custom execution or listener on the `UMordecaiStatusEffectComponent`.
- Frozen state: can be a separate GE (`UMordecaiGE_Frozen`) that applies `Mordecai.Status.Frozen` tag. Frozen blocks abilities via tag requirements on all gameplay abilities (add `Mordecai.Status.Frozen` as a blocking tag).
- MicroStunned: very short duration GE. Cancels current ability via `ASC->CancelAllAbilities()` or targeted cancel. Keep it simple — the tag alone blocks new activations for its brief duration.
- Shocked stacking: use GAS stack count. The micro-stun chance scales with stack count: `EffectiveChance = BaseChance * StackCount`.
- New attributes: `AttackSpeedMultiplier`, `DodgeRecoveryMultiplier`, `BlockStaminaCostMultiplier` — all multipliers defaulting to 1.0. Add to `UMordecaiAttributeSet`. These are read by their respective systems (US-004, US-005, US-006) when those are implemented.
- Register additional tags: `Mordecai.Status.Frozen`, `Mordecai.Status.MicroStunned`, `Mordecai.Immunity.Frozen`.
- All values are tuning placeholders. Expose via `UPROPERTY(EditDefaultsOnly)`.

## Tests Required

### Frostbitten Tests
- [ ] `Mordecai.StatusEffect.Frostbitten.AppliesTagAndReducesSpeed` — apply Frostbitten, verify tag active and `MoveSpeedMultiplier` reduced (AC-015.1, AC-015.2)
- [ ] `Mordecai.StatusEffect.Frostbitten.ReducesAttackSpeed` — apply Frostbitten, verify `AttackSpeedMultiplier` reduced (AC-015.3)
- [ ] `Mordecai.StatusEffect.Frostbitten.IncreaseDodgeRecovery` — apply Frostbitten, verify `DodgeRecoveryMultiplier` increased (AC-015.4)
- [ ] `Mordecai.StatusEffect.Frostbitten.StacksUpToMax` — apply Frostbitten 5 times, verify stack count reaches `FrostbittenMaxStacks` (AC-015.5)
- [ ] `Mordecai.StatusEffect.Frostbitten.FreezeAtMaxStacks` — apply Frostbitten `MaxStacks` times, verify `Mordecai.Status.Frozen` tag applied and Frostbitten stacks cleared (AC-015.5, AC-015.6)
- [ ] `Mordecai.StatusEffect.Frostbitten.FrozenBlocksAllActions` — apply Frozen, attempt to activate a test ability, verify blocked (AC-015.6)
- [ ] `Mordecai.StatusEffect.Frostbitten.FrozenAutoExpires` — apply Frozen, verify tag removed after `FrozenDurationSec` (AC-015.6)
- [ ] `Mordecai.StatusEffect.Frostbitten.BlockedByImmunity` — add immunity, apply, verify not applied (AC-015.8)

### Shocked Tests
- [ ] `Mordecai.StatusEffect.Shocked.AppliesTagAndStacks` — apply Shocked twice, verify tag active and stack count is 2 (AC-015.9, AC-015.10)
- [ ] `Mordecai.StatusEffect.Shocked.MicroStunOnHit` — apply Shocked (force stun chance to 1.0), deal damage, verify `Mordecai.Status.MicroStunned` applied briefly (AC-015.11)
- [ ] `Mordecai.StatusEffect.Shocked.IncreasesBlockStaminaCost` — apply Shocked (2 stacks), verify `BlockStaminaCostMultiplier` increased by 2x per-stack rate (AC-015.12)
- [ ] `Mordecai.StatusEffect.Shocked.InterruptsCasting` — apply Shocked to casting target (force chance to 1.0), verify cast interrupted event sent (AC-015.13)
- [ ] `Mordecai.StatusEffect.Shocked.BlockedByImmunity` — add immunity, apply, verify not applied (AC-015.15)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-015]` prefix
