# US-032: Item Definition & Categories

## Overview
Foundation story for Epic 7 (Inventory). Introduce the Mordecai item definition DataAsset that mirrors the canonical schema from `item_schema_v2.md` — categories, rarity, storage rules, sort priority, stack rules. This story is prerequisite for US-031 (inventory component) and US-033 (identification). No inventory runtime yet — just the data model and category helpers.

This story parallels the weapon system pattern (US-024 `UMordecaiWeaponDataAsset`): a monolithic `UDataAsset`-derived class with all fields, not Lyra's fragment composition. The WeaponDataAsset will be refactored later to be derivable from or co-existing with `UMordecaiItemDefinition`; this story does NOT do that refactor (deferred to a later story so Epic 7 can proceed without blocking on weapon rework).

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `Specs/item_schema_v2.md` (canonical item schema — ItemType, Storage, Stacking, Presentation fields)
- Design doc: `Specs/item_system_v1.md` (rarity tiers, modifier types)
- Agent rules: `Specs/agent_rules_v2.md` Section 2 (unlimited carry, auto-store defaults, DO/DO NOT lists)
- Existing code: `Mordecai/Weapons/MordecaiWeaponTypes.h` (EMordecaiItemRarity, EMordecaiModifierOp already exist — REUSE, do not duplicate)
- Existing code: `Mordecai/Weapons/MordecaiWeaponDataAsset.h` (pattern for DataAsset-based item)

---

## Acceptance Criteria

### Enums
- [ ] AC-032.1: `EMordecaiItemType` enum exists in `Mordecai/Items/MordecaiItemTypes.h` with values (in this order): `Weapon`, `Armor`, `Trinket`, `Consumable`, `Material`, `TownResource`, `UpgradeKey`, `QuestItem`, `MagicalItem`, `CurrencyProxy`. BlueprintType.
- [ ] AC-032.2: `EMordecaiCarryModel` enum exists with values: `Unlimited`, `SpecialRule`. BlueprintType.
- [ ] AC-032.3: `EMordecaiStorageDomain` enum exists with values: `PlayerInventory`, `TownStorage`, `QuestState`. BlueprintType.
- [ ] AC-032.4: `EMordecaiSortPriority` enum exists with values (in this order): `Low`, `Normal`, `High`, `Signature`, `Critical`. BlueprintType. Higher-priority values sort first.
- [ ] AC-032.5: `EMordecaiBindType` enum exists with values: `None`, `BindOnPickup`, `BindOnEquip`, `QuestBound`. BlueprintType.

### Item Definition DataAsset
- [ ] AC-032.6: `UMordecaiItemDefinition` (UDataAsset subclass) exists in `Mordecai/Items/` with identity UPROPERTY fields:
  - `ItemId` (FName) — unique identifier
  - `DisplayName` (FText)
  - `Description` (FText)
  - `ShortDescription` (FText, optional)
  - `Icon` (TSoftObjectPtr<UTexture2D>, optional)
- [ ] AC-032.7: Classification UPROPERTY fields on `UMordecaiItemDefinition`:
  - `ItemType` (EMordecaiItemType)
  - `Subtype` (FName, optional — e.g., "Sword", "Herb")
  - `Rarity` (EMordecaiItemRarity, default Common)
  - `Tags` (FGameplayTagContainer)
- [ ] AC-032.8: Storage UPROPERTY fields on `UMordecaiItemDefinition`:
  - `CarryModel` (EMordecaiCarryModel, default Unlimited)
  - `AutoStoreOnPickup` (bool, default false)
  - `StorageDomain` (EMordecaiStorageDomain, default PlayerInventory)
  - `SortPriority` (EMordecaiSortPriority, default Normal)
- [ ] AC-032.9: Stack UPROPERTY fields on `UMordecaiItemDefinition`:
  - `Stackable` (bool, default false)
  - `MaxStackSize` (int32, default 1; only meaningful when Stackable is true)
- [ ] AC-032.10: Ownership UPROPERTY fields on `UMordecaiItemDefinition`:
  - `BindType` (EMordecaiBindType, default None)
  - `DropOnDeath` (bool, default false)
  - `Tradable` (bool, default true)

### Category Helpers
- [ ] AC-032.11: `UMordecaiItemDefinition::IsAutoStored() const` returns true iff `AutoStoreOnPickup` is true. Used by US-031 to route pickups.
- [ ] AC-032.12: `UMordecaiItemDefinition::IsEquippable() const` returns true iff ItemType is `Weapon`, `Armor`, `Trinket`, or `MagicalItem`.
- [ ] AC-032.13: `UMordecaiItemDefinition::IsStackable() const` returns true iff `Stackable` is true AND `MaxStackSize > 1`.
- [ ] AC-032.14: Static/free function `UMordecaiItemLibrary::CompareSortPriority(const UMordecaiItemDefinition* A, const UMordecaiItemDefinition* B) -> int32` — returns negative if A sorts before B, positive if B before A, zero if equal. Order: higher SortPriority first (Critical → Signature → High → Normal → Low); on tie, compare `ItemType` order (Weapon/Armor/Trinket first, then MagicalItem, UpgradeKey, QuestItem, Consumable, Material/TownResource last); on tie, compare ItemId lexicographically.
- [ ] AC-032.15: Static helper `UMordecaiItemLibrary::GetTypeDefaultAutoStore(EMordecaiItemType Type) -> bool` returns true for `Material` and `TownResource`, false for all others. (Per item_schema_v2 Section 4 defaults.) Used by editor-time default population if a definition is created without explicitly setting `AutoStoreOnPickup`; runtime honors the set value.

### Gameplay Tags
- [ ] AC-032.16: Native gameplay tags declared in `MordecaiGameplayTags.h/.cpp`:
  - `Mordecai.Item.Type.Weapon`, `Mordecai.Item.Type.Armor`, `Mordecai.Item.Type.Trinket`, `Mordecai.Item.Type.Consumable`, `Mordecai.Item.Type.Material`, `Mordecai.Item.Type.TownResource`, `Mordecai.Item.Type.UpgradeKey`, `Mordecai.Item.Type.QuestItem`, `Mordecai.Item.Type.MagicalItem`
  - `Mordecai.Item.Rarity.Common`, `Mordecai.Item.Rarity.Green`, `Mordecai.Item.Rarity.Blue`, `Mordecai.Item.Rarity.Purple`, `Mordecai.Item.Rarity.Red`
- [ ] AC-032.17: `UMordecaiItemLibrary::GetTypeTag(EMordecaiItemType)` and `UMordecaiItemLibrary::GetRarityTag(EMordecaiItemRarity)` return the matching native tags from AC-032.16.

## Technical Notes
- **File location:** New subdirectory `Source/LyraGame/Mordecai/Items/`. Expected files:
  - `MordecaiItemTypes.h` (all enums)
  - `MordecaiItemDefinition.h/.cpp` (`UMordecaiItemDefinition`)
  - `MordecaiItemLibrary.h/.cpp` (static helper UBlueprintFunctionLibrary with `CompareSortPriority`, `GetTypeDefaultAutoStore`, tag accessors)
- **Do NOT duplicate existing types.** `EMordecaiItemRarity` and `EMordecaiModifierOp` already live in `Mordecai/Weapons/MordecaiWeaponTypes.h`. This story `#include`s them. The new enums in AC-032.1-5 are additions, not replacements.
- **No instance class yet.** This story is the *definition* schema only. Instance tracking (FMordecaiItemInstance or similar) lands in US-031 alongside the inventory component.
- **No fields from later stories.** Do NOT add `UsesIdentification`, `IdentificationState`, `RequiresIdentificationToEquip` — those are US-033. Do NOT add `StatModifiers`, `GrantedAbilities`, `ProcRules` — those are scoped under the weapon/armor concrete subclasses in later stories (out of scope here).
- **Relationship to UMordecaiWeaponDataAsset:** Existing WeaponDataAsset stays untouched in this story. A later cleanup story may reparent `UMordecaiWeaponDataAsset` to inherit from `UMordecaiItemDefinition`; that's explicitly NOT in scope here.
- **Test harness:** Use an in-memory `UMordecaiItemDefinition` (NewObject, fill fields) — no .uasset file needed. Follow the pattern in `Tests/Weapons/MordecaiWeaponTests.cpp`.

## Tests Required
- [ ] `Mordecai.Item.ItemTypeEnumHasExpectedValues` — verifies all 10 EMordecaiItemType values exist in order (AC-032.1)
- [ ] `Mordecai.Item.StorageEnumsDefined` — verifies EMordecaiCarryModel, EMordecaiStorageDomain, EMordecaiSortPriority, EMordecaiBindType have expected values (AC-032.2–5)
- [ ] `Mordecai.Item.DefinitionIdentityFields` — UMordecaiItemDefinition exposes ItemId/DisplayName/Description/Icon as UPROPERTY (AC-032.6)
- [ ] `Mordecai.Item.DefinitionClassificationFields` — ItemType/Subtype/Rarity/Tags exist with correct defaults (AC-032.7)
- [ ] `Mordecai.Item.DefinitionStorageDefaults` — CarryModel=Unlimited, AutoStoreOnPickup=false, StorageDomain=PlayerInventory, SortPriority=Normal (AC-032.8)
- [ ] `Mordecai.Item.DefinitionStackDefaults` — Stackable=false, MaxStackSize=1 (AC-032.9)
- [ ] `Mordecai.Item.IsAutoStoredTrueWhenFlagSet` — flipping AutoStoreOnPickup drives IsAutoStored() (AC-032.11)
- [ ] `Mordecai.Item.IsEquippableMatchesType` — IsEquippable() returns true for Weapon/Armor/Trinket/MagicalItem, false for Material/TownResource/UpgradeKey/QuestItem/Consumable (AC-032.12)
- [ ] `Mordecai.Item.IsStackableRequiresBothFlags` — Stackable=true with MaxStackSize=1 → IsStackable() returns false; Stackable=true with MaxStackSize=99 → true (AC-032.13)
- [ ] `Mordecai.Item.CompareSortPriorityOrdersByPriorityThenType` — Critical sorts before Normal; on tie, Weapon sorts before Material; on tie, ItemId breaks it (AC-032.14)
- [ ] `Mordecai.Item.GetTypeDefaultAutoStoreMaterialsTrue` — Material and TownResource return true; all others return false (AC-032.15)
- [ ] `Mordecai.Item.GetTypeTagReturnsMatchingNativeTag` — each EMordecaiItemType maps to its declared native tag (AC-032.17)
- [ ] `Mordecai.Item.GetRarityTagReturnsMatchingNativeTag` — each EMordecaiItemRarity maps to its declared native tag (AC-032.17)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] PIE smoke test passes (`py Scripts/run_pie_verify.py`)
- [ ] Code committed and pushed with `[US-032]` prefix
