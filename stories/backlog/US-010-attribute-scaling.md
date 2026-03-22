# US-010: Effective Mod Formula & Core Attribute Scaling

## Overview
Implement the Effective Mod diminishing returns formula and wire all 9 primary attributes to their primary and secondary derived stat effects via GAS. This is the mathematical backbone of character progression — every primary stat must meaningfully influence combat through percentage-based scaling with soft caps.

Currently, `UMordecaiAttributeSet` declares all 9 primary attributes (default 10.0) and core resources (default 100.0) but performs no scaling. This story adds the scaling infrastructure so that changing a primary attribute automatically recalculates all affected derived stats.

## Execution Mode
**Mode:** HEADLESS

## References
- **Primary (authoritative):** `character_attributes_v1.md` — primary_stats table, secondary_stats table, scaling_rules section
- **Secondary:** `attribute_scaling_v1.md` — extended combat-specific modifiers (NOT in scope for this story — see Technical Notes)
- **Legacy/potentially superseded:** `stat_formulas_v1.md` — simpler linear formulas (see TODO(DECISION) below)
- **Agent rules:** `agent_rules_v2.md` §6 (GAS for all attributes), §0 (UE 5.7)

---

## Acceptance Criteria

### Effective Mod Formula
- [ ] AC-010.1: `CalculateEffectiveMod(Mod)` returns the correct diminishing-returns value for Band 1 (Mods 1–5 at 100%): `CalculateEffectiveMod(5) == 5.0`
- [ ] AC-010.2: `CalculateEffectiveMod(Mod)` returns the correct value for Band 2 (Mods 6–10 at 70%): `CalculateEffectiveMod(10) == 8.5`
- [ ] AC-010.3: `CalculateEffectiveMod(Mod)` returns the correct value across Band 3 (Mods 11–15 at 45%): `CalculateEffectiveMod(12) == 9.4`
- [ ] AC-010.4: `CalculateEffectiveMod(Mod)` returns the correct value spanning all 4 bands (Mods 16+ at 25%): `CalculateEffectiveMod(20) == 12.0`
- [ ] AC-010.5: `CalculateEffectiveMod(0)` returns 0.0 (no negative scaling for zero-stat edge case)

### Primary Stat Effects (from character_attributes_v1.md primary_stats table)
Each primary attribute applies a percentage bonus to one primary derived stat, calculated as `ScalingPercent × EffectiveMod(RawAttributeValue)`:

- [ ] AC-010.6: STR scales `PhysicalDamageMultiplier` at 3.0% per Effective Mod point
- [ ] AC-010.7: DEX scales `AttackSpeedMultiplier` at 1.5% per Effective Mod point
- [ ] AC-010.8: END scales `MaxStamina` at 4.0% per Effective Mod point (percentage of base)
- [ ] AC-010.9: CON scales `MaxHealth` at 6.0% per Effective Mod point (percentage of base)
- [ ] AC-010.10: RES scales `AfflictionResistMultiplier` at 3.0% per Effective Mod point
- [ ] AC-010.11: DIS scales `MaxPosture` at 4.0% per Effective Mod point (percentage of base)
- [ ] AC-010.12: INT scales `MaxSpellPoints` at 4.0% per Effective Mod point (percentage of base)
- [ ] AC-010.13: WIS scales `MagicDamageMultiplier` at 3.0% per Effective Mod point
- [ ] AC-010.14: CHA scales `CastSpeedMultiplier` at 1.5% per Effective Mod point

### Secondary Stat Effects (from character_attributes_v1.md secondary_stats table)
Each primary attribute also applies a secondary percentage bonus:

- [ ] AC-010.15: STR scales `ArmorPenetrationMultiplier` at 1.5% per Effective Mod point
- [ ] AC-010.16: DEX scales `PhysicalCritChance` at 0.75% per Effective Mod point
- [ ] AC-010.17: END scales `StaminaRegenMultiplier` at 2.0% per Effective Mod point
- [ ] AC-010.18: CON scales `HealthRegenMultiplier` at 1.5% per Effective Mod point
- [ ] AC-010.19: RES scales `AfflictionRecoveryMultiplier` at 1.5% per Effective Mod point
- [ ] AC-010.20: DIS scales `PostureRecoveryMultiplier` at 2.0% per Effective Mod point
- [ ] AC-010.21: INT scales `SpellPointsRegenMultiplier` at 2.0% per Effective Mod point
- [ ] AC-010.22: WIS scales `ResistancePenetrationMultiplier` at 1.25% per Effective Mod point
- [ ] AC-010.23: CHA scales `MagicCritChance` at 0.75% per Effective Mod point

### GAS Integration
- [ ] AC-010.24: When a primary attribute changes (e.g., STR modified by a GE), all derived stats that depend on it automatically recalculate
- [ ] AC-010.25: Multiplier attributes default to 1.0 (neutral) at base stat values; additive chance attributes (crit) default to 0.0
- [ ] AC-010.26: Example end-to-end: setting STR to 12 results in `PhysicalDamageMultiplier == 1.0 + (0.03 × 9.4) == 1.282` and `ArmorPenetrationMultiplier == 1.0 + (0.015 × 9.4) == 1.141`

## Technical Notes

### Architecture
- **`CalculateEffectiveMod(float RawMod)`**: Pure static function in a utility class (e.g., `UMordecaiAttributeScaling` or `FMordecaiAttributeHelpers`). Formula:
  ```
  EffectiveMod = min(Mod,5) + max(min(Mod-5,5),0)*0.7 + max(min(Mod-10,5),0)*0.45 + max(Mod-15,0)*0.25
  ```
- **New derived attributes on `UMordecaiAttributeSet`**: Add `PhysicalDamageMultiplier`, `AttackSpeedMultiplier`, `AfflictionResistMultiplier`, `MagicDamageMultiplier`, `CastSpeedMultiplier`, `ArmorPenetrationMultiplier`, `PhysicalCritChance`, `StaminaRegenMultiplier`, `HealthRegenMultiplier`, `AfflictionRecoveryMultiplier`, `PostureRecoveryMultiplier`, `SpellPointsRegenMultiplier`, `ResistancePenetrationMultiplier`, `MagicCritChance`. Multipliers default 1.0; additive chances default 0.0.
- **Recalculation mechanism**: Use GAS-native approach — either custom `UGameplayModMagnitudeCalculation` (MMC) subclasses that read the primary attribute and apply Effective Mod scaling, OR override `PostAttributeChange` / `PostGameplayEffectExecute` on the attribute set to recalculate derived stats when a primary attribute changes. The MMC approach is preferred for GAS correctness.
- **Max resource scaling**: For `MaxHealth`, `MaxStamina`, `MaxPosture`, `MaxSpellPoints` — the percentage applies to a base value. Use `BaseMaxHealth` (etc.) as the unscaled base, and the attribute's current Max = Base × (1 + ScalingPercent × EffectiveMod). Base values remain at current defaults (100.0 for resources) until level progression is implemented.

### What's NOT in scope
- **attribute_scaling_v1.md combat modifiers** (posture damage, block stability, parry forgiveness, cast stability, elemental DR, status thresholds): These interact with specific combat subsystems (some not yet built) and will be wired in as those systems mature. This story covers only the character_attributes_v1.md primary/secondary tables.
- **Resistance formula** (`DamageTaken = IncomingDamage × (1 - Resistance)` from stat_formulas_v1.md): This is a damage pipeline concern, not attribute scaling. It will be integrated when damage calculation is unified.
- **Level-based base stat growth**: Base values for Health/Stamina/SP/Posture are fixed at 100.0 until a leveling system exists.

### TODO(DECISION): stat_formulas_v1.md reconciliation
`stat_formulas_v1.md` defines linear base formulas (e.g., `Health = BaseHealth + CON × 10`). `character_attributes_v1.md` defines percentage-based Effective Mod scaling (e.g., CON → 6.0% × EffMod for Health). These appear to be two different scaling layers that potentially conflict. Additionally, stat_formulas lists only 5 primary attributes while character_attributes lists 9.

**Default for this story**: Implement character_attributes_v1.md (Effective Mod percentage system) as the PRIMARY scaling mechanism. Treat stat_formulas_v1.md linear formulas as potentially superseded. If both are intended to coexist (linear base + percentage modifier), this needs explicit design clarification before implementation.

### TODO(DECISION): Mod input value
The scaling_rules section uses "Mod" in the formula but doesn't explicitly define whether Mod = raw attribute value (e.g., STR 12 → Mod 12) or a derived modifier (e.g., STR 12 → Mod 2 as offset from base 10). The examples ("Example STR 12" → "Effective Mod = 9.4") strongly suggest **Mod = raw attribute value**. Default to this interpretation.

## Tests Required
- [ ] `Mordecai.AttributeScaling.EffectiveModBand1` — verifies AC-010.1
- [ ] `Mordecai.AttributeScaling.EffectiveModBand2` — verifies AC-010.2
- [ ] `Mordecai.AttributeScaling.EffectiveModBand3` — verifies AC-010.3
- [ ] `Mordecai.AttributeScaling.EffectiveModAllBands` — verifies AC-010.4
- [ ] `Mordecai.AttributeScaling.EffectiveModZero` — verifies AC-010.5
- [ ] `Mordecai.AttributeScaling.STRScalesPhysicalDamage` — verifies AC-010.6
- [ ] `Mordecai.AttributeScaling.DEXScalesAttackSpeed` — verifies AC-010.7
- [ ] `Mordecai.AttributeScaling.ENDScalesMaxStamina` — verifies AC-010.8
- [ ] `Mordecai.AttributeScaling.CONScalesMaxHealth` — verifies AC-010.9
- [ ] `Mordecai.AttributeScaling.RESScalesAfflictionResist` — verifies AC-010.10
- [ ] `Mordecai.AttributeScaling.DISScalesMaxPosture` — verifies AC-010.11
- [ ] `Mordecai.AttributeScaling.INTScalesMaxSpellPoints` — verifies AC-010.12
- [ ] `Mordecai.AttributeScaling.WISScalesMagicDamage` — verifies AC-010.13
- [ ] `Mordecai.AttributeScaling.CHAScalesCastSpeed` — verifies AC-010.14
- [ ] `Mordecai.AttributeScaling.STRScalesArmorPenetration` — verifies AC-010.15
- [ ] `Mordecai.AttributeScaling.DEXScalesPhysicalCritChance` — verifies AC-010.16
- [ ] `Mordecai.AttributeScaling.ENDScalesStaminaRegen` — verifies AC-010.17
- [ ] `Mordecai.AttributeScaling.CONScalesHealthRegen` — verifies AC-010.18
- [ ] `Mordecai.AttributeScaling.RESScalesAfflictionRecovery` — verifies AC-010.19
- [ ] `Mordecai.AttributeScaling.DISScalesPostureRecovery` — verifies AC-010.20
- [ ] `Mordecai.AttributeScaling.INTScalesSpellPointsRegen` — verifies AC-010.21
- [ ] `Mordecai.AttributeScaling.WISScalesResistancePenetration` — verifies AC-010.22
- [ ] `Mordecai.AttributeScaling.CHAScalesMagicCritChance` — verifies AC-010.23
- [ ] `Mordecai.AttributeScaling.DerivedStatsRecalcOnPrimaryChange` — verifies AC-010.24
- [ ] `Mordecai.AttributeScaling.DefaultMultiplierValues` — verifies AC-010.25
- [ ] `Mordecai.AttributeScaling.EndToEndSTR12Example` — verifies AC-010.26

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-010]` prefix
