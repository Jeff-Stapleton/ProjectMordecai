# Item Schema v2

This document defines the **canonical item schema** for Project Mordecai.

The schema must support:
- common gear
- magical items
- consumables
- materials / town resources
- upgrade keys
- granted effects
- tags
- proc hooks
- conditional rules
- visual identity
- identification state where applicable
- auto-storage behavior where applicable

The schema must align with the design rule that **inventory is player-friendly and effectively unlimited for standard play**.

---

# 1. Design Goals

The item system should:
- keep loot readable
- support curated magical items
- support believable common loot
- let players safely retain important progression materials
- avoid bag-slot friction as a source of difficulty
- clearly separate collectible loot from usable / identified loot when needed

---

# 2. Recommended Runtime Mapping

- item definition → Data Asset / Data Table row
- item instance → saveable runtime struct
- equip effects → gameplay effects / granted tags / abilities
- on-hit or conditional behavior → proc definitions resolved by shared systems
- material ledger entry → persistent resource count / town storage record

---

# 3. Canonical Item Definition Schema

## Identity

```yaml
ItemId: emberfang
DisplayName: Emberfang
ItemType: MagicalItem
Description: A named sword that kindles fire with every strike.
Icon: T_UI_Item_Emberfang
WorldMesh: SM_Sword_Emberfang
```

### Required fields
- `ItemId`
- `DisplayName`
- `ItemType`
- `Description`

### Recommended optional fields
- `ShortDescription`
- `Icon`
- `WorldMesh`
- `SortOrder`
- `LoreText`

---

## Classification

```yaml
ItemType: MagicalItem
Subtype: Sword
Rarity: Purple
EquipSlot: MainHand
Tags:
  - Item.Weapon
  - Item.Magical
  - Weapon.Sword
  - Damage.Fire
```

### Recommended item types
- `Weapon`
- `Armor`
- `Trinket`
- `Consumable`
- `Material`
- `TownResource`
- `MagicalItem`
- `QuestItem`
- `UpgradeKey`
- `CurrencyProxy`

### Explicitly removed from the default model
- `Bag`

Bag items are not part of the default launch inventory model.

---

## Storage Rules

```yaml
Storage:
  CarryModel: Unlimited
  AutoStoreOnPickup: false
  StorageDomain: PlayerInventory
  SortPriority: Signature
```

### Field meanings
- `CarryModel`
  - `Unlimited`
  - `SpecialRule`
- `AutoStoreOnPickup`
  - `true` if the item should route directly into persistent resource storage
- `StorageDomain`
  - `PlayerInventory`
  - `TownStorage`
  - `QuestState`
- `SortPriority`
  - `Low`
  - `Normal`
  - `High`
  - `Signature`
  - `Critical`

### Default expectations
- Materials usually use `AutoStoreOnPickup: true`
- Town resources usually use `AutoStoreOnPickup: true`
- Upgrade keys should be highly protected in storage and sorting
- Common gear and magical items usually remain visible in player inventory

---

## Identification Rules

```yaml
Identification:
  UsesIdentification: true
  DefaultState: Unidentified
  IdentificationService: MageTower
  ShowPartialInfoBeforeIdentify: true
```

### Field meanings
- `UsesIdentification`
  - whether the item participates in the appraisal system
- `DefaultState`
  - `Identified`
  - `Unidentified`
- `IdentificationService`
  - usually `MageTower`
- `ShowPartialInfoBeforeIdentify`
  - allows teaser text, rarity hinting, silhouette hints, etc.

### Default expectations
- Most common gear should use `UsesIdentification: false`
- Most curated magical items may use `UsesIdentification: true`
- Identification should gate usage clarity and sometimes usage permission, not storage

---

## Stack / Ownership Rules

```yaml
Stacking:
  Stackable: false
  MaxStackSize: 1

Ownership:
  BindType: None
  DropOnDeath: false
  Tradable: true
```

### Recommended bind types
- `None`
- `BindOnPickup`
- `BindOnEquip`
- `QuestBound`

---

## Equip Rules

```yaml
EquipRules:
  Equippable: true
  RequiredLevel: 10
  RequiredTags: []
  ForbiddenTags: []
  AllowedClasses: []
  RequiresIdentificationToEquip: true
```

Optional, but useful if you later add restrictions.

---

## Core Stats

```yaml
StatModifiers:
  - Stat: WeaponDamage
    Operation: Add
    Value: 18
  - Stat: CritChance
    Operation: Add
    Value: 0.05
  - Stat: FireDamageScalar
    Operation: Add
    Value: 0.12
```

### Recommended operations
- `Add`
- `Multiply`
- `Override`
- `PercentAdd`

---

## Granted Content

```yaml
GrantedAbilities:
  - AbilityId: emberfang_flame_arc

GrantedEffects:
  - EffectId: ge_emberfang_passive_fire_bonus

GrantedTags:
  - Weapon.FireImbued
```

This is one of the most important parts of item identity.

---

## Proc Hooks

```yaml
ProcRules:
  - ProcId: emberfang_burn_proc
    Trigger: OnHit
    Chance: 0.25
    InternalCooldown: 2.0
    Effects:
      - EffectId: ge_burning_buildup_medium
```

### Recommended triggers
- `OnHit`
- `OnCrit`
- `OnKill`
- `OnBlock`
- `OnPerfectBlock`
- `OnParry`
- `OnDodge`
- `OnAbilityUse`
- `OnStatusApplied`
- `OnLowHealth`
- `OnEnterCombat`

---

## Affixes

```yaml
Affixes:
  Prefixes:
    - flaming
  Suffixes:
    - of_cinders
```

### Rule
If using randomized or semi-randomized affixes, affixes should still be built from known modifier primitives.

---

## Durability / Special Rules

```yaml
Durability:
  UsesDurability: false
  MaxDurability: 0

SpecialRules:
  BossDropGuaranteed: false
  WorldPlaced: true
  ContainerPlaced: false
```

---

## Economy / Value

```yaml
Economy:
  SellValue: 250
  BuyValue: 0
  SalvageTableId: salvage_weapon_purple
```

---

## Presentation

```yaml
Presentation:
  RarityColor: Purple
  EquipFX: NS_Weapon_Embers
  InspectText: The blade glows with a restrained inner furnace.
```

---

# 4. Recommended Type Defaults

## 4.1 Material

```yaml
ItemType: Material
Storage:
  CarryModel: Unlimited
  AutoStoreOnPickup: true
  StorageDomain: TownStorage
  SortPriority: Normal
Identification:
  UsesIdentification: false
```

Use for monster parts, herbs, ore, hides, fibers, and similar progression inputs.

## 4.2 TownResource

```yaml
ItemType: TownResource
Storage:
  CarryModel: Unlimited
  AutoStoreOnPickup: true
  StorageDomain: TownStorage
  SortPriority: High
Identification:
  UsesIdentification: false
```

Use for building-focused resources and civic upgrade materials.

## 4.3 UpgradeKey

```yaml
ItemType: UpgradeKey
Storage:
  CarryModel: Unlimited
  AutoStoreOnPickup: false
  StorageDomain: PlayerInventory
  SortPriority: Critical
Identification:
  UsesIdentification: false
```

Use for unique progression unlockers such as major relics.

## 4.4 MagicalItem

```yaml
ItemType: MagicalItem
Storage:
  CarryModel: Unlimited
  AutoStoreOnPickup: false
  StorageDomain: PlayerInventory
  SortPriority: Signature
Identification:
  UsesIdentification: true
  DefaultState: Unidentified
  IdentificationService: MageTower
  ShowPartialInfoBeforeIdentify: true
EquipRules:
  RequiresIdentificationToEquip: true
```

Use for curated named gear and build-defining drops.

---

# 5. Item Instance Schema

An item definition describes the template. An instance tracks player-owned state.

```yaml
InstanceId: iteminst_000145
ItemId: emberfang
Quantity: 1
IdentificationState: Unidentified
AffixRolls:
  - flaming
  - of_cinders
DurabilityCurrent: 0
IsEquipped: false
OwnerCharacterId: char_001
```

### Required fields
- `InstanceId`
- `ItemId`

### Recommended instance fields
- `Quantity`
- `IdentificationState`
- `AffixRolls`
- `DurabilityCurrent`
- `IsEquipped`
- `OwnerCharacterId`

### Notes
- Materials that auto-store may not require normal player-inventory instances in the same way equipment does.
- Identifiable items should track identification state explicitly at instance level if identification is not purely definition-driven.
