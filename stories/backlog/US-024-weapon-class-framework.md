# US-024: Weapon Class Framework

## Overview
Build the foundational weapon system: a data-driven `UMordecaiWeaponDataAsset` that defines weapon identity, stats, and attack profile references; a `UMordecaiWeaponInstance` runtime struct for player-owned weapon state; an equip/unequip system that grants weapon abilities and applies stat modifiers via GAS; and a weapon slot model (MainHand, OffHand). This is the base framework that all weapon types (US-025+) build on.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `item_schema_v2.md` (canonical item schema — ItemId, StatModifiers, GrantedAbilities, GrantedEffects, ProcRules, EquipSlot)
- Design doc: `item_system_v1.md` (item categories, rarity tiers, modifier types, affix system)
- Design doc: `attack_taxonomy_v1.md` (Section 2.1 — AttackProfile data model, weapon class profiles)
- Design doc: `player_attacks_agent_brief_v1.md` (weapon class attack profiles — Longsword, Rapier examples)
- Agent rules: `agent_rules_v2.md` (GAS for abilities/attributes, data-driven, unlimited carry inventory)
- Existing code: `UMordecaiAttackProfileDataAsset` (US-002), `UMordecaiGA_MeleeAttack` (US-004), `UMordecaiAttributeSet`, `UMordecaiAbilitySystemComponent`

---

## Acceptance Criteria

### Weapon DataAsset
- [ ] AC-024.1: `UMordecaiWeaponDataAsset` (UDataAsset subclass) exists in `Mordecai/Weapons/` with UPROPERTY fields:
  - `WeaponId` (FName) — unique identifier
  - `DisplayName` (FText) — human-readable name
  - `WeaponType` (EMordecaiWeaponType enum) — see AC-024.2
  - `EquipSlot` (EMordecaiEquipSlot enum: MainHand, OffHand, TwoHand)
  - `Rarity` (EMordecaiItemRarity enum: Common, Green, Blue, Purple, Red)
  - `BaseDamage` (float) — base weapon damage added to attacks
  - `AttackSpeedMultiplier` (float, default 1.0) — weapon speed modifier
  - `Range` (float) — weapon reach in cm
  - `PostureDamageBonus` (float, default 0.0) — flat bonus to posture damage
  - `LightAttackProfiles` (TArray<UMordecaiAttackProfileDataAsset*>) — ordered combo chain for light attacks
  - `HeavyAttackProfile` (UMordecaiAttackProfileDataAsset*) — single heavy/charged attack
  - `StatModifiers` (TArray<FMordecaiStatModifier>) — attribute modifiers applied while equipped
  - `GrantedAbilities` (TArray<TSubclassOf<UGameplayAbility>>) — abilities granted while equipped
  - `GrantedTags` (FGameplayTagContainer) — tags applied while equipped (e.g., `Weapon.Sword`, `Weapon.TwoHanded`)

- [ ] AC-024.2: `EMordecaiWeaponType` enum exists with values: Longsword, Greatsword, Shortsword, Dagger, Axe, Mace, Spear, Quarterstaff, Unarmed, Longbow, Shortbow, Crossbow, Throwable, Wand

- [ ] AC-024.3: `FMordecaiStatModifier` struct exists with fields: `Attribute` (FGameplayAttribute), `Operation` (EMordecaiModifierOp enum: Add, Multiply, PercentAdd), `Value` (float). This mirrors the item_schema_v2 StatModifiers section

### Weapon Instance
- [ ] AC-024.4: `FMordecaiWeaponInstance` struct exists with fields: `InstanceId` (FGuid), `WeaponDataAsset` (UMordecaiWeaponDataAsset*), `AffixRolls` (TArray<FName> — placeholder for future affix system), `IsEquipped` (bool). This is the runtime representation of a player-owned weapon

### Equip/Unequip System
- [ ] AC-024.5: `UMordecaiEquipmentComponent` (UActorComponent subclass) exists in `Mordecai/Weapons/`. Attached to the player character. Manages equipped weapon state
- [ ] AC-024.6: `UMordecaiEquipmentComponent::EquipWeapon(FMordecaiWeaponInstance& Weapon, EMordecaiEquipSlot Slot)` — equips the weapon to the specified slot. On equip:
  - Applies `StatModifiers` as a GameplayEffect to the owner's ASC
  - Grants `GrantedAbilities` via the ASC
  - Adds `GrantedTags` to the ASC
  - Sets the weapon's `LightAttackProfiles` as the active combo chain on the melee attack ability
  - Marks `Weapon.IsEquipped = true`
- [ ] AC-024.7: `UMordecaiEquipmentComponent::UnequipWeapon(EMordecaiEquipSlot Slot)` — unequips the weapon from the specified slot. On unequip:
  - Removes the stat modifier GE
  - Removes granted abilities
  - Removes granted tags
  - Resets melee attack to Unarmed defaults
  - Marks `Weapon.IsEquipped = false`
- [ ] AC-024.8: Equipping a TwoHand weapon clears both MainHand and OffHand slots (unequipping whatever was there). Equipping a MainHand weapon while a TwoHand is equipped unequips the TwoHand first
- [ ] AC-024.9: The EquipmentComponent tracks currently equipped weapons: `GetEquippedWeapon(EMordecaiEquipSlot)` returns the current weapon instance (or nullptr/empty)

### Melee Attack Integration
- [ ] AC-024.10: `UMordecaiGA_MeleeAttack` is extended to read its active attack profile chain from the EquipmentComponent's equipped MainHand weapon, instead of a hardcoded profile. When no weapon is equipped, fallback to Unarmed defaults
- [ ] AC-024.11: Weapon `BaseDamage` is added to the melee attack's damage calculation: `FinalDamage = (AttackProfile.BasePower + Weapon.BaseDamage) × existing multipliers`
- [ ] AC-024.12: Weapon `AttackSpeedMultiplier` modifies the Windup/Active/Recovery timings on attack profiles: `AdjustedTime = BaseTime / AttackSpeedMultiplier`. A weapon with 1.2 AttackSpeedMultiplier makes attacks 20% faster

### Tags
- [ ] AC-024.13: New gameplay tags declared: `Mordecai.Weapon.Type.Sword`, `Mordecai.Weapon.Type.Axe`, `Mordecai.Weapon.Type.Mace`, `Mordecai.Weapon.Type.Spear`, `Mordecai.Weapon.Type.Staff`, `Mordecai.Weapon.Type.Dagger`, `Mordecai.Weapon.Type.Bow`, `Mordecai.Weapon.Type.Crossbow`, `Mordecai.Weapon.Type.Wand`, `Mordecai.Weapon.Type.Unarmed`, `Mordecai.Weapon.Slot.MainHand`, `Mordecai.Weapon.Slot.OffHand`, `Mordecai.Weapon.Slot.TwoHand`

## Technical Notes
- **File location:** New subdirectory `Source/LyraGame/Mordecai/Weapons/` for all weapon-related code.
- **Pattern parallel:** This mirrors the spell framework pattern (DataAsset + ability integration). The WeaponDataAsset is analogous to SpellDataAsset, and the EquipmentComponent is analogous to how spells are granted.
- **Stat modifier GE:** On equip, create a runtime GE instance that applies all `StatModifiers` from the weapon. Use `ASC->ApplyGameplayEffectToSelf` with an infinite duration GE. Store the active GE handle so it can be removed on unequip.
- **Ability granting:** Use `ASC->GiveAbility()` for each ability in `GrantedAbilities`. Store handles for removal on unequip.
- **Unarmed fallback:** When no weapon is equipped, the EquipmentComponent provides a default Unarmed attack profile (weak punch, no special stats). This can be a static default or a UPROPERTY on the component.
- **Do NOT implement:** Individual weapon type definitions (US-025+), affix resolution, proc hooks, weapon visuals/meshes, or weapon switching animations. This story is framework only.
- **Replication:** Equipped weapon state should replicate (the EquipmentComponent should be a replicated component). The stat GE and granted abilities replicate naturally through GAS.

## Tests Required
- [ ] `Mordecai.Weapon.DataAssetFieldsExist` — WeaponDataAsset has all required UPROPERTY fields (verifies AC-024.1)
- [ ] `Mordecai.Weapon.EquipAppliesStatModifiers` — Equipping a weapon with StatModifiers applies the GE to the ASC (verifies AC-024.6)
- [ ] `Mordecai.Weapon.EquipGrantsAbilities` — Equipping a weapon with GrantedAbilities makes them available on the ASC (verifies AC-024.6)
- [ ] `Mordecai.Weapon.EquipGrantsTags` — Equipping a weapon adds GrantedTags to the ASC (verifies AC-024.6)
- [ ] `Mordecai.Weapon.UnequipRemovesAll` — Unequipping removes stat GE, abilities, and tags (verifies AC-024.7)
- [ ] `Mordecai.Weapon.TwoHandClearsBothSlots` — Equipping TwoHand unequips both MainHand and OffHand (verifies AC-024.8)
- [ ] `Mordecai.Weapon.MeleeUsesEquippedProfile` — Melee attack reads combo chain from equipped weapon (verifies AC-024.10)
- [ ] `Mordecai.Weapon.UnarmedFallbackWhenNoWeapon` — With no weapon equipped, melee uses Unarmed defaults (verifies AC-024.10)
- [ ] `Mordecai.Weapon.BaseDamageAddsToAttack` — Weapon BaseDamage increases melee hit damage (verifies AC-024.11)
- [ ] `Mordecai.Weapon.AttackSpeedModifiesTiming` — Weapon AttackSpeedMultiplier adjusts attack phase durations (verifies AC-024.12)
- [ ] `Mordecai.Weapon.GetEquippedWeaponReturnsCorrect` — GetEquippedWeapon returns the right weapon per slot (verifies AC-024.9)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-024]` prefix
