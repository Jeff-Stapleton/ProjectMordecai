# US-008: Stamina Tier System

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement the tiered stamina resource system. Stamina is consumed by attacks, dodging, blocking, sprinting, and parrying. Unlike binary stamina systems, low stamina reduces effectiveness but NEVER fully prevents actions — the player can always act, just at diminished capacity. Stamina tiers (Green/Yellow/Red) are tracked via gameplay tags and apply effectiveness modifiers to combat actions. This supports the "skill-first" pillar: stamina management is a skill expression, not a hard gate.

## References
- Design doc: `combat_system_v1.md` — Stamina as tiered resource, consumed by attacks/dodge/block/sprint
- Design doc: `character_attributes_v1.md` — END: Stamina +4.0%/pt, StaminaRegen +2.0%/pt; STR: Stamina scaling; DEX: Stamina scaling
- Design doc: `stat_formulas_v1.md` — `Stamina = BaseStamina + (STR × 2) + (DEX × 1)`, `StaminaRegen = Base + (END × 2.0%)`
- Design doc: `control_bindings_v1.1.md` — "Dodge: Always possible; effectiveness depends on stamina tier"
- Design doc: `game_design_v2.md` — Stamina management as skill expression; never blocks actions
- Agent rules: `agent_rules_v2.md` — GAS for all attributes; UE 5.7

---

## Acceptance Criteria

### Stamina Attributes
- [ ] AC-008.1: `Stamina` and `MaxStamina` attributes exist in `UMordecaiAttributeSet` (should already exist from US-001; verify clamping allows negative values — Stamina can go below 0)
- [ ] AC-008.2: MaxStamina initialized with placeholder default (100.0). Attribute scaling from STR/DEX/END is `TODO(DECISION)` — stubbed as base value until Epic 3

### Stamina Tiers
- [ ] AC-008.3: Three stamina tiers defined: **Green** (Stamina > 66% of Max), **Yellow** (33%–66%), **Red** (< 33%). Thresholds are configurable (`GreenThresholdPercent`, `YellowThresholdPercent`, defaults: 0.66, 0.33)
- [ ] AC-008.4: Current tier tracked via mutually exclusive gameplay tags: `Mordecai.Stamina.Tier.Green`, `Mordecai.Stamina.Tier.Yellow`, `Mordecai.Stamina.Tier.Red`
- [ ] AC-008.5: Tier tags update immediately when Stamina changes (via `PostGameplayEffectExecute` or attribute change delegate)
- [ ] AC-008.6: A fourth state `Mordecai.Stamina.Tier.Exhausted` is applied when Stamina ≤ 0 (can overlap with Red)

### Effectiveness Modifiers
- [ ] AC-008.7: Each tier has configurable effectiveness multipliers applied to combat actions:
  - `GreenMultiplier` = 1.25 (25% bonus at full stamina — reward for stamina management)
  - `YellowMultiplier` = 1.0 (normal effectiveness — this is the "standard" range for most of the bar)
  - `RedMultiplier` = 0.75 (25% penalty when low)
  - `ExhaustedMultiplier` = 0.5 (50% penalty when depleted)
- [ ] AC-008.8: Effectiveness multipliers affect: dodge distance, attack damage output, block stability. Implemented as a queryable function `GetStaminaTierMultiplier()` that other systems (US-005, US-006, US-004) can call.
- [ ] AC-008.9: Actions are NEVER blocked regardless of stamina level — even at 0 or negative stamina, abilities activate (just at reduced effectiveness)

### Stamina Regeneration
- [ ] AC-008.10: Stamina regenerates passively at a configurable base rate (`StaminaRegenRate`, units per second, default placeholder: 15.0)
- [ ] AC-008.11: Stamina regeneration is paused for a configurable delay after any stamina consumption (`StaminaRegenDelayMs`, default placeholder: 1000ms)
- [ ] AC-008.12: Regen delay timer resets on each new stamina consumption (if player keeps spending, regen stays paused)
- [ ] AC-008.13: Stamina regeneration rate scales with END attribute (`TODO(DECISION)` — stubbed as base rate only until Epic 3; interface exists for `END × 2.0%` modifier)

### Sprint Stamina Drain
- [ ] AC-008.14: While sprinting (`Mordecai.State.Sprinting` tag active), stamina drains at a configurable rate per second (`SprintStaminaDrainRate`, default placeholder: 10.0/sec)
- [ ] AC-008.15: Sprint drain applies continuously while sprinting, not as a lump cost
- [ ] AC-008.16: Sprint does NOT stop when stamina reaches 0 — sprint continues at reduced speed (per tier multiplier), stamina can go negative

### Stamina Consumption Events
- [ ] AC-008.17: Stamina consumption is handled via Gameplay Effects with `SetByCaller` magnitude tagged `Mordecai.Data.StaminaCost`
- [ ] AC-008.18: Each consuming system (attack, dodge, block, parry, sprint) uses this standardized GE to deduct stamina, ensuring consistent regen delay triggering

## Technical Notes
- Place in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- **Tier tracking**: Implement in `UMordecaiAttributeSet::PostGameplayEffectExecute` — after any Stamina change, recalculate tier and update tags on the owning ASC. Use `AddLooseGameplayTag` / `RemoveLooseGameplayTag` for tier tags.
- **Effectiveness query**: Create a static utility function `UMordecaiStaminaHelper::GetTierMultiplier(UAbilitySystemComponent* ASC)` that reads the current tier tag and returns the multiplier. Other abilities call this when calculating output.
- **Regen**: Use an infinite-duration periodic GE (`GE_MordecaiStaminaRegen`) that ticks every 0.1s (or similar), adding `StaminaRegenRate × TickInterval` to Stamina. The regen GE checks a "last consumption time" before applying. Alternatively, use `FTimerManager` on a component.
- **Regen delay**: Track via a transient gameplay tag `Mordecai.State.StaminaRegenPaused` applied by a short-duration GE whenever stamina is consumed. While this tag is active, regen ticks are skipped. Each consumption re-applies (refreshes) the tag.
- **Sprint drain**: Apply a periodic GE while `Mordecai.State.Sprinting` is active. The sprint ability (from US-001) should apply this.
- **Negative stamina**: Modify the `Stamina` clamping in `PreAttributeChange` to allow values below 0 (clamp to some floor like -50 to prevent infinite debt). MaxStamina still clamps above 0.
- **Exhausted state**: When Stamina ≤ 0, apply `Mordecai.Stamina.Tier.Exhausted` in addition to Red tier tag (both active simultaneously).
- Do NOT implement attribute scaling formulas — those are Epic 3. Just expose configurable base values.

## Tests Required
- [ ] `Mordecai.Stamina.GreenTierAboveThreshold` — set Stamina to 80/100, verify Green tier tag active (AC-008.3, AC-008.4)
- [ ] `Mordecai.Stamina.YellowTierInMiddleRange` — set Stamina to 50/100, verify Yellow tier tag active (AC-008.3, AC-008.4)
- [ ] `Mordecai.Stamina.RedTierBelowThreshold` — set Stamina to 20/100, verify Red tier tag active (AC-008.3, AC-008.4)
- [ ] `Mordecai.Stamina.ExhaustedAtZero` — set Stamina to 0, verify Exhausted tag active (AC-008.6)
- [ ] `Mordecai.Stamina.TierUpdatesOnChange` — start at Green, consume stamina to Yellow range, verify tag switches from Green to Yellow (AC-008.5)
- [ ] `Mordecai.Stamina.TierMultiplierReturnsCorrectValue` — at Green return 1.25, at Yellow return 1.0, at Red return 0.75, at Exhausted return 0.5 (AC-008.7, AC-008.8)
- [ ] `Mordecai.Stamina.ActionsNotBlockedAtZero` — set Stamina to 0, activate dodge ability, verify it activates (AC-008.9)
- [ ] `Mordecai.Stamina.RegenerationOccursOverTime` — consume some stamina, wait past regen delay, verify stamina increases (AC-008.10)
- [ ] `Mordecai.Stamina.RegenPausedDuringDelay` — consume stamina, immediately check, verify no regen during delay window (AC-008.11)
- [ ] `Mordecai.Stamina.RegenDelayResetsOnConsumption` — consume stamina, wait half the delay, consume again, verify delay restarted (AC-008.12)
- [ ] `Mordecai.Stamina.SprintDrainsPerSecond` — activate sprint, wait 1 second, verify stamina reduced by SprintStaminaDrainRate (AC-008.14, AC-008.15)
- [ ] `Mordecai.Stamina.SprintContinuesBelowZero` — sprint with low stamina, verify sprint doesn't stop at 0 (AC-008.16)
- [ ] `Mordecai.Stamina.CanGoNegative` — consume stamina past 0, verify negative value (AC-008.1)
- [ ] `Mordecai.Stamina.StandardizedConsumptionGE` — apply stamina cost GE with SetByCaller, verify stamina reduced and regen delay triggered (AC-008.17, AC-008.18)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-008]` prefix
