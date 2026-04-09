# US-016: Combat Modifier Debuffs (Weakened, Brittle, Exposed, Corroded)

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement four debuff statuses that modify combat effectiveness: Weakened (reduces outgoing posture damage), Brittle (increases incoming posture damage), Exposed (next hit deals bonus damage), and Corroded (reduces block stability and armor efficiency). These are modifier-focused — they change the math of combat rather than dealing direct damage or restricting actions.

## References
- Design doc: `status_effects_launch_set_v1.md` — Weakened ("You can't break them"), Brittle ("You break easily"), Exposed ("Next mistake hurts more"), Corroded ("Gear advantage denied")
- Design doc: `combat_system_v1.md` — Posture system, Defense formula (FinalDamage = IncomingDamage × (1 − Mitigation))
- Design doc: `damage_types_v1.md` — Corrosive→Corroded relationship
- Agent rules: `agent_rules_v2.md` — GAS for all status effects
- Depends on: US-013 (Status Effect Framework), interfaces with US-007 (Posture), US-006 (Block)

---

## Acceptance Criteria

### Weakened
- [ ] AC-016.1: `UMordecaiGE_Weakened` exists. Applies `Mordecai.Status.Weakened` tag. Duration: configurable `WeakenedDurationSec` (default placeholder: 8.0s).
- [ ] AC-016.2: While Weakened, outgoing posture damage is reduced by a configurable percentage: `WeakenedPostureDamageReductionPct` (default placeholder: 0.40, meaning 40% reduction). Implementation: modify `OutgoingPostureDamageMultiplier` attribute (add to AttributeSet if needed, default 1.0). US-007 (Posture) reads this when calculating posture damage dealt.
- [ ] AC-016.3: Heavy/charged attacks partially bypass Weakened penalty: when the attack has `Mordecai.Attack.Heavy` tag, the Weakened reduction is halved. Implementation: expose a `WeakenedHeavyBypassPct` (default placeholder: 0.50, meaning heavy attacks only suffer 50% of the Weakened penalty). Actual bypass calculation happens in the posture damage pipeline.
- [ ] AC-016.4: Weakened is blocked if target has `Mordecai.Immunity.Weakened` tag.

### Brittle
- [ ] AC-016.5: `UMordecaiGE_Brittle` exists. Applies `Mordecai.Status.Brittle` tag. Duration: configurable `BrittleDurationSec` (default placeholder: 7.0s).
- [ ] AC-016.6: While Brittle, incoming posture damage is increased by a configurable percentage: `BrittlePostureDamageIncreasePct` (default placeholder: 0.35, meaning 35% more posture damage taken). Implementation: modify `IncomingPostureDamageMultiplier` attribute (add to AttributeSet if needed, default 1.0). US-007 reads this.
- [ ] AC-016.7: While Brittle, posture break threshold is lowered (stagger happens sooner). Implementation: reduce effective `MaxPosture` by `BrittleMaxPostureReductionPct` (default placeholder: 0.20, meaning 20% lower posture threshold). Applied as a modifier to `MaxPosture` attribute.
- [ ] AC-016.8: Perfect block halves incoming posture damage while Brittle: when `Mordecai.State.PerfectBlock` is active and target is Brittle, the `BrittlePostureDamageIncreasePct` is halved. Interface only — wiring deferred to US-006 integration.
- [ ] AC-016.9: Brittle is blocked if target has `Mordecai.Immunity.Brittle` tag.

### Exposed
- [ ] AC-016.10: `UMordecaiGE_Exposed` exists. Applies `Mordecai.Status.Exposed` tag. Duration: configurable `ExposedDurationSec` (default placeholder: 5.0s). Also expires on the next hit received (whichever comes first).
- [ ] AC-016.11: While Exposed, the next incoming hit deals bonus damage: `ExposedBonusDamagePct` (default placeholder: 0.30, meaning 30% more damage). Implementation: modify `IncomingDamageMultiplier` attribute (add to AttributeSet if needed, default 1.0).
- [ ] AC-016.12: Exposed is consumed on the first hit: after the bonus damage is applied, the Exposed GE is removed. Only one hit benefits from the bonus.
- [ ] AC-016.13: Perfect block removes Exposed without consuming the hit (no bonus damage if perfectly blocked). Interface for `Mordecai.State.PerfectBlock` → remove Exposed.
- [ ] AC-016.14: Exposed is blocked if target has `Mordecai.Immunity.Exposed` tag.

### Corroded
- [ ] AC-016.15: `UMordecaiGE_Corroded` exists. Applies `Mordecai.Status.Corroded` tag. Duration: configurable `CorrodedDurationSec` (default placeholder: 10.0s).
- [ ] AC-016.16: While Corroded, block stability is reduced by a configurable percentage: `CorrodedBlockStabilityReductionPct` (default placeholder: 0.30, meaning 30% less block stability). Implementation: modify `BlockStabilityMultiplier` attribute (add to AttributeSet if needed, default 1.0). US-006 reads this.
- [ ] AC-016.17: While Corroded, armor efficiency (damage mitigation) is reduced by a configurable percentage: `CorrodedArmorReductionPct` (default placeholder: 0.25, meaning 25% less effective armor). Implementation: modify `ArmorEfficiencyMultiplier` attribute (add to AttributeSet if needed, default 1.0). The damage pipeline reads this.
- [ ] AC-016.18: While Corroded, Sharpen/Repair buffs are disabled. Implementation: add `Mordecai.Status.Corroded` as a blocking tag for Sharpen and Repair buff GEs (those GEs don't exist yet — add tag requirement documentation for when they're created in Epic 6).
- [ ] AC-016.19: Corroded is blocked if target has `Mordecai.Immunity.Corroded` tag.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/StatusEffects/Effects/`
- New attributes needed on `UMordecaiAttributeSet`:
  - `OutgoingPostureDamageMultiplier` (default 1.0) — read by posture damage calculation
  - `IncomingPostureDamageMultiplier` (default 1.0) — read by posture damage calculation
  - `IncomingDamageMultiplier` (default 1.0) — read by damage execution calculation
  - `BlockStabilityMultiplier` (default 1.0) — read by block system
  - `ArmorEfficiencyMultiplier` (default 1.0) — read by damage mitigation calculation
- All are multiplicative modifiers. GEs apply them as `Additive` operations on the base 1.0 (e.g., Weakened adds -0.40 to `OutgoingPostureDamageMultiplier`). Or use `Multiply` with 0.60. Choose whichever composes better with multiple simultaneous debuffs.
- Exposed's "consume on hit" mechanic: listen for damage event on the component. When Exposed is active and damage is received, remove Exposed GE after the damage calculation has used the multiplier. Timing matters — the bonus must apply to the hit that removes it.
- Heavy attack bypass for Weakened: the posture damage pipeline (US-007) should check for `Mordecai.Attack.Heavy` tag on the damage source and apply the bypass. For now, just ensure the attribute and tag interface exist.
- Register `Mordecai.Attack.Heavy` tag if not already registered.

## Tests Required

### Weakened Tests
- [ ] `Mordecai.StatusEffect.Weakened.AppliesTagAndModifier` — apply Weakened, verify tag active and `OutgoingPostureDamageMultiplier` reduced (AC-016.1, AC-016.2)
- [ ] `Mordecai.StatusEffect.Weakened.ReducesOutgoingPostureDamage` — apply Weakened, simulate posture damage output, verify reduced by expected percentage (AC-016.2)
- [ ] `Mordecai.StatusEffect.Weakened.HeavyBypassesPartially` — verify `WeakenedHeavyBypassPct` configurable exists and defaults correctly (AC-016.3)
- [ ] `Mordecai.StatusEffect.Weakened.BlockedByImmunity` — add immunity, apply, verify not applied (AC-016.4)

### Brittle Tests
- [ ] `Mordecai.StatusEffect.Brittle.AppliesTagAndModifier` — apply Brittle, verify tag active and `IncomingPostureDamageMultiplier` increased (AC-016.5, AC-016.6)
- [ ] `Mordecai.StatusEffect.Brittle.LowersMaxPosture` — apply Brittle, verify `MaxPosture` reduced by expected percentage (AC-016.7)
- [ ] `Mordecai.StatusEffect.Brittle.BlockedByImmunity` — add immunity, apply, verify not applied (AC-016.9)

### Exposed Tests
- [ ] `Mordecai.StatusEffect.Exposed.AppliesTagAndBonusDamage` — apply Exposed, verify tag active and `IncomingDamageMultiplier` increased (AC-016.10, AC-016.11)
- [ ] `Mordecai.StatusEffect.Exposed.ConsumedOnFirstHit` — apply Exposed, deal damage, verify bonus applied AND Exposed removed after (AC-016.12)
- [ ] `Mordecai.StatusEffect.Exposed.ExpiresOnDurationIfNoHit` — apply Exposed, wait for duration, verify removed without hit (AC-016.10)
- [ ] `Mordecai.StatusEffect.Exposed.BlockedByImmunity` — add immunity, apply, verify not applied (AC-016.14)

### Corroded Tests
- [ ] `Mordecai.StatusEffect.Corroded.ReducesBlockStability` — apply Corroded, verify `BlockStabilityMultiplier` reduced (AC-016.16)
- [ ] `Mordecai.StatusEffect.Corroded.ReducesArmorEfficiency` — apply Corroded, verify `ArmorEfficiencyMultiplier` reduced (AC-016.17)
- [ ] `Mordecai.StatusEffect.Corroded.BlockedByImmunity` — add immunity, apply, verify not applied (AC-016.19)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-016]` prefix
