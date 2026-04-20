# US-033: Unidentified Items & Identification Service

## Overview
Implement the identification layer from `item_schema_v2.md` Section "Identification Rules" — unidentified items are carried normally but gate *clarity* and optionally *equipping*. Adds `UsesIdentification` / `DefaultState` / `RequiresIdentificationToEquip` fields to the item definition, per-instance `IdentificationState` on `FMordecaiItemInstance`, partial-info display helpers, and a headless `UMordecaiIdentificationService` that flips state (the actual Mage Tower NPC/UI is a future town story — this story delivers the data plumbing so town content can call `Service::Identify(Instance)`).

**Depends on US-032** (item definition) and **US-031** (item instance struct and inventory component).

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `Specs/item_schema_v2.md` — "Identification Rules" subsection (fields, defaults) and Section 4.4 (MagicalItem default uses identification)
- Design doc: `Specs/item_system_v1.md` (magical-item flow)
- Agent rules: `Specs/agent_rules_v2.md` Section 2 — "Surface unidentified magical items clearly in UI and tooltips" (the DATA APIs for this are delivered here; UI is later)
- Existing code (from US-031): `FMordecaiItemInstance`, `UMordecaiInventoryComponent`
- Existing code (from US-032): `UMordecaiItemDefinition`, `UMordecaiItemLibrary`

---

## Acceptance Criteria

### Identification Enum & Definition Fields
- [ ] AC-033.1: `EMordecaiIdentificationState` enum (in `Mordecai/Items/MordecaiItemTypes.h` beside the other item enums) with values: `Identified`, `Unidentified`. BlueprintType.
- [ ] AC-033.2: `UMordecaiItemDefinition` gains the following identification UPROPERTY fields (default to identified gear — matches item_schema defaults):
  - `UsesIdentification` (bool, default false)
  - `DefaultIdentificationState` (EMordecaiIdentificationState, default Identified)
  - `RequiresIdentificationToEquip` (bool, default false)
  - `ShowPartialInfoBeforeIdentify` (bool, default true)
  - `IdentificationService` (FName, default `"MageTower"` — placeholder for future multi-service support)

### Instance Field
- [ ] AC-033.3: `FMordecaiItemInstance` gains an `IdentificationState` field (EMordecaiIdentificationState, default `Identified`). Serialized / replicated with the rest of the instance.
- [ ] AC-033.4: When `UMordecaiInventoryComponent::AddItem` (or `PickupItem` when routed to inventory) creates a new instance for a definition with `UsesIdentification=true`, the instance's `IdentificationState` is set from `Def->DefaultIdentificationState` (e.g., MagicalItem with DefaultState=Unidentified will create an Unidentified instance on pickup). For definitions with `UsesIdentification=false`, the instance is always `Identified` regardless of `DefaultIdentificationState`.
- [ ] AC-033.5: Instance helper `FMordecaiItemInstance::IsIdentified() const -> bool` returns true iff `IdentificationState == Identified`.

### Display / Partial Info API
- [ ] AC-033.6: `UMordecaiItemLibrary::GetDisplayName(const FMordecaiItemInstance& Instance) -> FText` — if instance is Identified, returns `Def->DisplayName`. If Unidentified AND `Def->ShowPartialInfoBeforeIdentify=true`, returns a partial-info label of the form `"Unidentified <RarityName> <SubtypeOrTypeName>"` (e.g., `"Unidentified Purple Sword"` using Subtype when set, else a human-readable form of `ItemType`). If Unidentified AND `ShowPartialInfoBeforeIdentify=false`, returns the literal `"Unknown Item"`.
- [ ] AC-033.7: `UMordecaiItemLibrary::GetDescription(const FMordecaiItemInstance& Instance) -> FText` — if Identified, returns `Def->Description`. If Unidentified AND `ShowPartialInfoBeforeIdentify=true`, returns `Def->ShortDescription` if set, otherwise an empty FText. If Unidentified AND `ShowPartialInfoBeforeIdentify=false`, returns empty FText.
- [ ] AC-033.8: `UMordecaiItemLibrary::GetVisibleTags(const FMordecaiItemInstance& Instance) -> FGameplayTagContainer` — if Identified, returns `Def->Tags`. If Unidentified, returns only tags matching `Mordecai.Item.Rarity.*` and `Mordecai.Item.Type.*` (hides combat/damage/affix tags that would leak identity).

### Equip Gating
- [ ] AC-033.9: `UMordecaiItemLibrary::CanEquipInstance(const FMordecaiItemInstance& Instance) -> bool` — returns false if `Def->UsesIdentification && Def->RequiresIdentificationToEquip && !Instance.IsIdentified()`. Returns false if `!Def->IsEquippable()`. Otherwise true.
- [ ] AC-033.10: Equipment-side enforcement: `UMordecaiEquipmentComponent::EquipWeapon` (from US-024) gains a pre-check that rejects (returns false, logs a warning) if the weapon's backing item instance is unidentified AND the definition requires identification to equip. **Scope note:** only added when a weapon's `FMordecaiWeaponInstance` is linked to a `FMordecaiItemInstance`. Because the current weapon framework is separate from the inventory instance system, the hook for this story is: expose a static helper `UMordecaiEquipmentComponent::IsInstanceEquippable(const FMordecaiItemInstance&)` that defers to `UMordecaiItemLibrary::CanEquipInstance`. Tying weapons to inventory instances is a future story; we verify only the helper and library gating here.

### Identification Service
- [ ] AC-033.11: `UMordecaiIdentificationService` (UGameInstanceSubsystem) exists in `Mordecai/Items/`. Runs alongside the GameInstance; globally accessible.
- [ ] AC-033.12: `UMordecaiIdentificationService::IdentifyInstance(UMordecaiInventoryComponent* Inventory, const FGuid& InstanceId, FName /*ServiceName*/ = "MageTower") -> bool` — looks up the instance in the inventory; if found and Unidentified, sets state to Identified, notifies the inventory (which broadcasts `OnInventoryChanged`), and returns true. Returns false if not found or already identified.
- [ ] AC-033.13: Service fires `FOnMordecaiItemIdentified` multicast delegate (signature: `UMordecaiInventoryComponent*, FGuid InstanceId`) when an identification succeeds.
- [ ] AC-033.14: `UMordecaiInventoryComponent::SetInstanceIdentificationState(const FGuid& InstanceId, EMordecaiIdentificationState NewState) -> bool` — mutation helper that the service uses; returns true on success. Fires `OnInventoryChanged` with delta=0 to signal a state change to listeners.

### Gameplay Tags
- [ ] AC-033.15: Native tags declared in `MordecaiGameplayTags`:
  - `Mordecai.Item.Identification.Identified`
  - `Mordecai.Item.Identification.Unidentified`
- [ ] AC-033.16: `UMordecaiItemLibrary::GetIdentificationTag(EMordecaiIdentificationState) -> FGameplayTag` returns the matching tag.

## Technical Notes
- **File location:** All new code in `Source/LyraGame/Mordecai/Items/` alongside US-031/US-032.
- **No UI in this story.** Widgets consuming partial-info APIs are covered by US-071 (Inventory UI). This story delivers the functions; future UI calls them.
- **No Mage Tower NPC.** The service is callable headlessly — future town stories (Epic 9) add the actual Mage Tower interaction that invokes `IdentifyInstance`.
- **No cost model.** Identification is free and instant in this story. A future design decision adds gold/time/reagent cost — mark in `UMordecaiIdentificationService.h` with `// TODO(DECISION): Identification cost model unspecified; currently free.`
- **Batch identify:** Out of scope. Identifying N items is N calls for now.
- **Tag-hiding rule (AC-033.8):** Simple filter — keep any tag under `Mordecai.Item.Rarity` or `Mordecai.Item.Type` roots; drop everything else. Tag container utilities in `GameplayTagContainer.h` handle this (iterate `FGameplayTagContainer::GetGameplayTagArray()` and match by parent tag).
- **Do NOT implement:**
  - Mage Tower NPC, identification UI, cost/currency consumption
  - Tie-in from world pickup actors (we test service directly)
  - Identification scrolls or one-shot identify consumables
  - Weapon instance ↔ item instance linkage refactor (big migration; separate story)

## Tests Required
- [ ] `Mordecai.Item.IdentificationStateEnumExists` — EMordecaiIdentificationState has Identified/Unidentified values (AC-033.1)
- [ ] `Mordecai.Item.DefinitionIdentificationFieldsDefault` — defaults match spec (UsesIdentification=false, DefaultState=Identified, RequiresIdToEquip=false, ShowPartial=true) (AC-033.2)
- [ ] `Mordecai.Item.InstanceUsesDefinitionDefaultState` — instance created from a def with DefaultState=Unidentified has IdentificationState=Unidentified (AC-033.4)
- [ ] `Mordecai.Item.InstanceAlwaysIdentifiedWhenUsesIdentificationFalse` — even if DefaultState=Unidentified, UsesIdentification=false forces Identified (AC-033.4)
- [ ] `Mordecai.Item.GetDisplayNameShowsPartialWhenUnidentified` — unidentified Purple sword returns partial-info label, not DisplayName (AC-033.6)
- [ ] `Mordecai.Item.GetDisplayNameShowsFullWhenIdentified` — after identification, GetDisplayName returns DisplayName (AC-033.6)
- [ ] `Mordecai.Item.GetDisplayNameHidesEverythingWhenShowPartialFalse` — ShowPartialInfoBeforeIdentify=false + Unidentified returns "Unknown Item" (AC-033.6)
- [ ] `Mordecai.Item.GetDescriptionRespectsPartialInfoFlag` — partial info controls description visibility (AC-033.7)
- [ ] `Mordecai.Item.GetVisibleTagsFiltersRevealingTags` — unidentified instance's visible tags contain Rarity/Type roots and exclude Damage.* tags (AC-033.8)
- [ ] `Mordecai.Item.CanEquipBlockedByUnidentified` — RequiresIdToEquip=true + Unidentified → CanEquipInstance returns false (AC-033.9)
- [ ] `Mordecai.Item.CanEquipAllowedAfterIdentification` — same instance after Identify returns true (AC-033.9)
- [ ] `Mordecai.Item.CanEquipFalseForNonEquippableType` — Material instance returns false even when Identified (AC-033.9)
- [ ] `Mordecai.Item.ServiceIdentifiesInstance` — IdentifyInstance flips state to Identified and returns true (AC-033.12)
- [ ] `Mordecai.Item.ServiceRejectsAlreadyIdentified` — second IdentifyInstance call returns false (AC-033.12)
- [ ] `Mordecai.Item.ServiceFiresDelegateOnSuccess` — OnItemIdentified broadcasts with matching InstanceId (AC-033.13)
- [ ] `Mordecai.Item.InventoryDelegateFiresOnIdentificationStateChange` — SetInstanceIdentificationState fires OnInventoryChanged with delta=0 (AC-033.14)
- [ ] `Mordecai.Item.GetIdentificationTagReturnsMatchingTag` — both enum values map to declared tags (AC-033.16)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] PIE smoke test passes (`py Scripts/run_pie_verify.py`)
- [ ] Code committed and pushed with `[US-033]` prefix
