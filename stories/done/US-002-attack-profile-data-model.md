# US-002: Attack Profile Data Model & Damage Types

## Execution Mode
**Mode:** HEADLESS

## Overview
Create the data-driven foundation for the entire combat system. All attacks (melee, projectile, AoE) are described by an `AttackProfile` data asset so designers can tune without code changes. This story also establishes the damage type taxonomy and gameplay tags that every subsequent combat story depends on.

## References
- Design doc: `attack_taxonomy_v1.md` — Section 2 (Required Data Model)
- Design doc: `player_attacks_agent_brief_v1.md` — Section 2 (Required Data Model)
- Design doc: `damage_types_v1.md` — Sections 2-5 (Taxonomy, Status Relationships, Resistance Schema, Delivery Metadata)
- Design doc: `combat_system_v1.md` — Damage Formula baseline
- Agent rules: `agent_rules_v2.md` — GAS for all abilities/attributes/effects; data-driven preferred; UE 5.7

---

## Acceptance Criteria
- [x] AC-002.1: `EMordecaiAttackType` enum exists with values: `MeleeSweep`, `MeleeThrust`, `MeleeSlam`, `Projectile`, `AoE`
- [x] AC-002.2: `EMordecaiInputSlot` enum exists with values: `Light`, `Heavy`, `SkillA`, `SkillB`, `Throw`
- [x] AC-002.3: `EMordecaiRootedMode` enum exists with values: `None`, `Windup`, `Active`, `Full`
- [x] AC-002.4: `EMordecaiHitShapeType` enum exists with values: `ArcSector`, `Capsule`, `Box`, `Circle`
- [x] AC-002.5: `EMordecaiDamageType` enum exists with values: `Physical`, `Slash`, `Pierce`, `Blunt`, `Fire`, `Frost`, `Lightning`, `Poison`, `Corrosive`, `Arcane`, `Shadow`, `Radiant`
- [x] AC-002.6: `FMordecaiHitShapeParams` struct exists with fields for each shape variant (radius, angle, startAngleOffset for arc; length, width for capsule/box; radius for circle)
- [x] AC-002.7: `FMordecaiProjectileSpec` struct exists with fields: `Speed`, `GravityDrop`, `Lifetime`, `PierceCount`, `RicochetCount`, `RicochetRange`, `RicochetAngleLimit`, `OnHitAoERadius`, `OnHitAoEDamageScalar`
- [x] AC-002.8: `FMordecaiDamageProfile` struct exists with fields: `DamageType`, `Delivery` (tag), `BasePower`, `CanCrit`, `AppliesPostureDamage`, `BuildsStatus` (tag), `StatusMagnitude`
- [x] AC-002.9: `UMordecaiAttackProfileDataAsset` (UDataAsset subclass) exists with all fields from attack_taxonomy_v1.md Section 2.1: `AttackType`, `InputSlot`, `WindupTimeMs`, `ActiveTimeMs`, `RecoveryTimeMs`, `RootedDuring`, `HitShapeType`, `HitShapeParams`, `HitsAirborne`, `JumpAvoidable`, `DamageProfile`, `PostureDamageScalar`, `StaminaCost`, `ComboIndex`, `CancelableIntoDodge`, `CancelableIntoBlock`
- [x] AC-002.10: `OnHitPayload` and `OnUsePayload` fields exist on AttackProfile as arrays of Gameplay Effect class references
- [x] AC-002.11: Native gameplay tags registered for damage types: `Mordecai.Damage.Physical`, `Mordecai.Damage.Slash`, `Mordecai.Damage.Pierce`, `Mordecai.Damage.Blunt`, `Mordecai.Damage.Fire`, `Mordecai.Damage.Frost`, `Mordecai.Damage.Lightning`, `Mordecai.Damage.Poison`, `Mordecai.Damage.Corrosive`, `Mordecai.Damage.Arcane`, `Mordecai.Damage.Shadow`, `Mordecai.Damage.Radiant`
- [x] AC-002.12: All UPROPERTY fields are `EditAnywhere, BlueprintReadOnly` with appropriate categories for editor organization

## Technical Notes
- Place all types in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- Use `UDataAsset` subclass (not DataTable) for AttackProfile — allows per-asset editing and Blueprint referencing
- Gameplay tags should use `UE_DEFINE_GAMEPLAY_TAG_COMMENT` in a dedicated `MordecaiGameplayTags.h/.cpp`
- `FMordecaiHitShapeParams` should use a union-like approach (all fields present, relevant ones depend on `HitShapeType`) — keep it simple, avoid polymorphic structs
- `FMordecaiDamageProfile` is embedded in AttackProfile, not a separate asset
- `OnHitPayload` / `OnUsePayload`: use `TArray<TSubclassOf<UGameplayEffect>>` for now; can be extended to include spawned actors later
- Do NOT implement any runtime behavior (hit detection, damage application) — that's US-003 and US-004
- Slash/Pierce/Blunt are subtypes of Physical per damage_types_v1.md; the tag hierarchy (`Mordecai.Damage.Physical.Slash`) should reflect this

## Tests Required
- [x] `Mordecai.Combat.AttackTypeEnumHasAllValues` — verifies AC-002.1
- [x] `Mordecai.Combat.InputSlotEnumHasAllValues` — verifies AC-002.2
- [x] `Mordecai.Combat.RootedModeEnumHasAllValues` — verifies AC-002.3
- [x] `Mordecai.Combat.HitShapeTypeEnumHasAllValues` — verifies AC-002.4
- [x] `Mordecai.Combat.DamageTypeEnumHasAllValues` — verifies AC-002.5
- [x] `Mordecai.Combat.AttackProfileDataAssetHasAllFields` — verifies AC-002.9, AC-002.10, AC-002.12
- [x] `Mordecai.Combat.ProjectileSpecHasAllFields` — verifies AC-002.7
- [x] `Mordecai.Combat.DamageProfileHasAllFields` — verifies AC-002.8
- [x] `Mordecai.Combat.DamageGameplayTagsRegistered` — verifies AC-002.11
- [x] `Mordecai.Combat.AttackProfileSerializesCorrectly` — create a profile in code, set all fields, verify values persist through NewObject

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] `Scripts/Verify.bat` passes (tests + build)
- [x] Code committed and pushed with `[US-002]` prefix
