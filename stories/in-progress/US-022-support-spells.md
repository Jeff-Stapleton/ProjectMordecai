# US-022: Support Spells (Bless, Restoration)

## Overview
Implement two support spells using the Spell Framework from US-019. Bless (self-buff increasing damage multipliers) and Restoration (heal-over-time with configurable heal amount). Each spell implements Rank 1 base behavior only; rank milestone unlocks (Cleanse, Sanctuary, etc.) are deferred to future stories.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `ability_schema_v1.md` (delivery type: InstantApply; self-targeted; duration GE pattern)
- Design doc: `new_spells_proposal.md` (Restoration: Rank 1 = heal moderate HP over time; Bless not in new_spells but referenced in PLAN.md)
- Design doc: `skill_sheet_v1.1.md` (no explicit Bless entry — Bless is a standard support buff concept from game_design_v2.md)
- Design doc: `character_attributes_v1.md` (PhysicalDamageMultiplier, MagicDamageMultiplier attributes exist in UMordecaiAttributeSet from US-010)
- Agent rules: `agent_rules_v2.md` (GAS for all buffs/effects)
- Existing code: `UMordecaiGA_SpellBase`, `UMordecaiSpellDataAsset` (US-019), `UMordecaiAttributeSet` (PhysicalDamageMultiplier, MagicDamageMultiplier, Health, MaxHealth)

---

## Acceptance Criteria
- [ ] AC-022.1: `UMordecaiGA_Bless` extends `UMordecaiGA_SpellBase`. Delivery: InstantApply self-targeted. On cast, applies a duration GE that increases `PhysicalDamageMultiplier` and `MagicDamageMultiplier` by a configurable BlessBonus amount (read from BasePower in SpellDataAsset as a percentage, e.g., BasePower=15 means +0.15 additive to multipliers)
- [ ] AC-022.2: Bless GE lasts for a configurable Duration (from SpellDataAsset). Applies tag `Mordecai.Status.Blessed`. When duration expires, the GE and tag are removed and multipliers revert
- [ ] AC-022.3: Bless cannot stack with itself — reapplying Bless while active refreshes the duration but does not double the bonus
- [ ] AC-022.4: `UMordecaiGA_Restoration` extends `UMordecaiGA_SpellBase`. Delivery: InstantApply self-targeted. On cast, applies a periodic HoT (Heal over Time) GE that restores health over Duration seconds
- [ ] AC-022.5: Restoration HoT GE ticks once per second. Total heal = BasePower from SpellDataAsset (scaled by SpellPower). Each tick heals TotalHeal / Duration. Health cannot exceed MaxHealth
- [ ] AC-022.6: Restoration HoT GE applies tag `Mordecai.Status.Restoration`. Removed when duration expires. Reapplying refreshes duration and recalculates heal amount
- [ ] AC-022.7: Both spells declare ability tags: `Mordecai.Ability.Spell.Bless`, `Mordecai.Ability.Spell.Restoration`
- [ ] AC-022.8: New gameplay tags declared: `Mordecai.Status.Blessed`, `Mordecai.Status.Restoration`

## Technical Notes
- **Bless multiplier:** The existing `PhysicalDamageMultiplier` and `MagicDamageMultiplier` attributes (from US-010) default to 1.0. Bless adds to these via an Additive GE modifier. When the GE expires, GAS automatically reverts the attribute.
- **Bless non-stacking:** Use GAS's built-in stacking policy. Set the GE's stacking type to `AggregateBySource` with `StackCount = 1` and `StackDurationRefreshPolicy = RefreshOnSuccessfulApplication`. This means reapplying refreshes duration without doubling the effect.
- **Restoration HoT:** Use a Duration GE with `Period = 1.0` and `ExecutePeriodicEffectOnApplication = false`. Each period tick applies a health modification of `(SpellPower / Duration)`. Use `FScalableFloat` or SetByCaller for the magnitude.
- **Health clamping:** The existing `UMordecaiAttributeSet::PostGameplayEffectExecute` should already clamp Health to [0, MaxHealth]. If not, ensure it does.
- **Small story:** This is intentionally a smaller story (2 spells, 8 ACs) since both are simple self-targeted buff/HoT patterns. Should be completable in a single short session.
- **Bless design note:** Bless is not in skill_sheet_v1.1 as an explicit skill entry. It's referenced in game_design_v2.md and PLAN.md. For now, it's a standalone spell not tied to a specific skill. TODO(DECISION): Should Bless be tied to a "Divine Magic" or "Support" skill for rank scaling? Defaulting to: Bless uses the spell framework's generic rank scaling.

## Tests Required
- [ ] `Mordecai.Spell.Bless.IncreasesPhysicalDamageMultiplier` — PhysicalDamageMultiplier increases by BlessBonus while active (verifies AC-022.1)
- [ ] `Mordecai.Spell.Bless.IncreasesMagicDamageMultiplier` — MagicDamageMultiplier increases by BlessBonus while active (verifies AC-022.1)
- [ ] `Mordecai.Spell.Bless.ExpiresAfterDuration` — GE, tag, and multiplier bonus removed after Duration (verifies AC-022.2)
- [ ] `Mordecai.Spell.Bless.DoesNotStackWithSelf` — Reapplying refreshes duration without doubling bonus (verifies AC-022.3)
- [ ] `Mordecai.Spell.Restoration.HealsOverTime` — Health increases each tick by TotalHeal/Duration (verifies AC-022.4, AC-022.5)
- [ ] `Mordecai.Spell.Restoration.HealDoesNotExceedMaxHealth` — Health clamped at MaxHealth (verifies AC-022.5)
- [ ] `Mordecai.Spell.Restoration.ExpiresAfterDuration` — HoT GE and tag removed after Duration (verifies AC-022.6)
- [ ] `Mordecai.Spell.Restoration.RefreshResetsDuration` — Reapplying refreshes duration (verifies AC-022.6)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-022]` prefix
