# US-019: Spell Framework & Spell DataAsset

## Overview
Build the foundational spell system: a data-driven `UMordecaiSpellDataAsset`, a `UMordecaiGA_SpellBase` gameplay ability, SP cost/cooldown enforcement, casting phases with movement policies, interruption, attribute-based power scaling, skill rank scaling, upcasting, and Silenced immunity integration. This is the base class and data model that all individual spells (US-020 through US-023) will extend.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `ability_schema_v1.md` (canonical ability schema — costs, cooldowns, cast timing, targeting, scaling, delivery, effects)
- Design doc: `ability_system_v1.md` (ability types, GAS mapping, scaling formula: `AbilityPower = BaseAbilityPower + AttributeScaling + SkillScaling`)
- Design doc: `skill_sheet_v1.1.md` (magic skill rank descriptions — Fireball, Cone of Cold, Magic Missile, Stone Skin, Sleep, Blink, Illusion, Fire Ward, Float, Counter Spell)
- Design doc: `character_attributes_v1.md` (INT/WIS scaling for magic)
- Agent rules: `agent_rules_v2.md` (GAS for all abilities, data-driven, server-authoritative)
- Existing code: `UMordecaiAttributeSet` (SpellPoints, MaxSpellPoints attributes), `UMordecaiSkillComponent` (rank lookup), `UMordecaiAbilitySystemComponent`, `MordecaiGameplayTags.h/.cpp`
- Existing pattern: `UMordecaiGA_MeleeAttack` + `UMordecaiAttackProfileDataAsset` (analogous pattern for melee — spell framework mirrors this for spells)

---

## Acceptance Criteria
- [ ] AC-019.1: `UMordecaiSpellDataAsset` (UDataAsset subclass) exists in `Mordecai/Magic/` with UPROPERTY fields: SpellId (FName), DisplayName (FText), School (FGameplayTag), SpellPointCost (int32), CooldownDuration (float), CooldownTag (FGameplayTag), CastTime (float), WindupTime (float), RecoveryTime (float), bInterruptible (bool), MovementPolicy (enum: FreeMove/SlowWhileCasting/RootWhileCasting), TargetingType (enum: Self/MeleeArc/Cone/Circle/Line/Projectile/Dash/PersistentArea), Range (float), Radius (float), ArcDegrees (float), BasePower (float), ScalingStats (TArray of struct: Stat FGameplayAttribute + Coefficient float), DeliveryType (enum: InstantApply/SpawnProjectile/TraceHit/DashMovement/SpawnPersistentArea), UpcastCostMultiplier (float, default 2.0), UpcastPowerMultiplier (float, default 1.5)
- [ ] AC-019.2: `UMordecaiGA_SpellBase` (UGameplayAbility subclass) exists in `Mordecai/Magic/`, configured to read a `UMordecaiSpellDataAsset` pointer set on the ability CDO or passed via ability spec
- [ ] AC-019.3: SpellBase's `CanActivateAbility` checks that the caster's SpellPoints >= SpellPointCost (from the DataAsset). Returns false if insufficient SP
- [ ] AC-019.4: On `CommitAbility`, SpellBase deducts SpellPointCost from the caster's SpellPoints attribute via an instant GameplayEffect (SetByCaller magnitude)
- [ ] AC-019.5: On commit, SpellBase applies a cooldown GameplayEffect with Duration = CooldownDuration and grants the CooldownTag. While the cooldown tag is active, the spell cannot be reactivated
- [ ] AC-019.6: SpellBase executes casting phases in sequence: Windup (WindupTime) → Cast (CastTime) → Recovery (RecoveryTime). Each phase advances via timer. `OnSpellWindupComplete`, `OnSpellCast`, `OnSpellRecoveryComplete` are virtual functions subclasses override
- [ ] AC-019.7: During Windup and Cast phases, SpellBase applies the configured MovementPolicy: `FreeMove` applies no tags, `SlowWhileCasting` applies `Mordecai.State.CastingSlow` tag, `RootWhileCasting` applies `Mordecai.State.CastingRooted` tag. Tags are removed on Recovery or ability end
- [ ] AC-019.8: If `bInterruptible` is true and the caster receives damage (detected via `Mordecai.Event.DamageTaken` gameplay event or attribute change callback) during Windup or Cast phase, the spell is cancelled via `CancelAbility`. SP is still consumed (cost was committed). Cooldown is NOT applied on interrupt
- [ ] AC-019.9: SpellBase computes `SpellPower = BasePower × (1.0 + Σ(ScalingStat.Coefficient × EffectiveModForStat))` where EffectiveModForStat reads from `UMordecaiAttributeScaling` (existing attribute scaling system from US-010)
- [ ] AC-019.10: SpellBase integrates with `UMordecaiSkillComponent`: reads the spell's associated skill rank. Applies a rank power multiplier of `1.0 + (Rank × 0.05)` to SpellPower. At Rank 0 (untrained), multiplier is 1.0. At Rank 20 (max), multiplier is 2.0
- [ ] AC-019.11: SpellBase supports upcasting: when activated with an upcast flag (e.g., via gameplay event data or input tag), SP cost = SpellPointCost × UpcastCostMultiplier, and SpellPower is further multiplied by UpcastPowerMultiplier. CanActivateAbility checks the upcast cost if upcast is requested
- [ ] AC-019.12: New gameplay tags declared in `MordecaiGameplayTags.h/.cpp`: `Mordecai.Ability.Spell`, `Mordecai.State.Casting`, `Mordecai.State.CastingSlow`, `Mordecai.State.CastingRooted`, `Mordecai.Event.SpellCast`
- [ ] AC-019.13: SpellBase's `CanActivateAbility` returns false if the caster has the `Mordecai.Status.Silenced` tag (integration point with Epic 4 status effects US-017)

## Technical Notes
- **File location:** New subdirectory `Source/LyraGame/Mordecai/Magic/` for all spell-related code.
- **Pattern parallel:** This mirrors the `UMordecaiAttackProfileDataAsset` + `UMordecaiGA_MeleeAttack` pattern but for spells. Key differences: SP cost (not stamina), cooldowns, casting phases (not windup/active/recovery), movement policies, attribute scaling via INT/WIS.
- **SP deduction GE:** Create a minimal `UGameplayEffect` (instant, SetByCaller for SpellPoints attribute with negative magnitude). This is similar to how melee stamina costs work.
- **Cooldown GE:** Use GAS's built-in cooldown pattern: a Duration GE that grants the cooldown tag. `GetCooldownTimeRemaining` checks this.
- **Casting phases vs melee phases:** Melee has Windup/Active/Recovery. Spells have Windup/Cast/Recovery. The Cast phase is when the spell effect actually fires (e.g., projectile spawns). Subclasses override `OnSpellCast()` to do their specific delivery.
- **Rank scaling formula:** `1.0 + (Rank × 0.05)` means each rank gives +5% power. This is a baseline; individual spell rank milestones (Rank 5/10/15/20 unlocks) are implemented in each spell's subclass in future stories.
- **Upcasting:** Simple multiplier system. The input mechanism (how the player triggers upcast vs normal cast) is TODO(DECISION) — for now, support it as a bool flag on ability activation.
- **Interruptibility:** Only during Windup and Cast. Recovery cannot be interrupted (player is already committed). Interruption does NOT refund SP — casting has a risk/reward element.
- **Do NOT implement** individual spells, rank milestone unlocks, visual effects, or animations in this story. This is framework only.

## Tests Required
- [ ] `Mordecai.Spell.DataAssetFieldsExist` — SpellDataAsset has all required UPROPERTY fields (verifies AC-019.1)
- [ ] `Mordecai.Spell.SpellBaseBlocksActivationWithInsufficientSP` — CanActivateAbility returns false when SP < cost (verifies AC-019.3)
- [ ] `Mordecai.Spell.SpellBaseDeductsSPOnCommit` — SpellPoints reduced by SpellPointCost after commit (verifies AC-019.4)
- [ ] `Mordecai.Spell.SpellBaseAppliesCooldown` — Cooldown GE applied with correct tag and duration (verifies AC-019.5)
- [ ] `Mordecai.Spell.CooldownPreventsReactivation` — CanActivateAbility returns false while cooldown tag is active (verifies AC-019.5)
- [ ] `Mordecai.Spell.CastingPhasesProgress` — Windup → Cast → Recovery phases execute in order with correct timings (verifies AC-019.6)
- [ ] `Mordecai.Spell.SlowWhileCastingAppliesTag` — SlowWhileCasting policy applies Mordecai.State.CastingSlow tag during casting (verifies AC-019.7)
- [ ] `Mordecai.Spell.RootWhileCastingAppliesTag` — RootWhileCasting policy applies Mordecai.State.CastingRooted tag during casting (verifies AC-019.7)
- [ ] `Mordecai.Spell.InterruptibleSpellCancelledOnDamage` — Interruptible spell in Windup/Cast phase cancelled when caster takes damage; SP still consumed, no cooldown applied (verifies AC-019.8)
- [ ] `Mordecai.Spell.SpellPowerScalesWithAttributes` — SpellPower computed correctly with attribute scaling coefficients (verifies AC-019.9)
- [ ] `Mordecai.Spell.SpellPowerScalesWithSkillRank` — SpellPower multiplied by (1.0 + Rank × 0.05) based on SkillComponent rank (verifies AC-019.10)
- [ ] `Mordecai.Spell.UpcastIncreasedCostAndPower` — Upcast flag increases SP cost by UpcastCostMultiplier and SpellPower by UpcastPowerMultiplier (verifies AC-019.11)
- [ ] `Mordecai.Spell.SilencedBlocksSpellActivation` — CanActivateAbility returns false when caster has Mordecai.Status.Silenced tag (verifies AC-019.13)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-019]` prefix
