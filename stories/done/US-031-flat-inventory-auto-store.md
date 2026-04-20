# US-031: Flat Inventory & Auto-Store System

## Overview
Build the core inventory runtime for Mordecai: `UMordecaiInventoryComponent` (unlimited-carry, flat list of item instances) + `UMordecaiResourceLedger` (auto-stored Material/TownResource counts keyed by ItemId). Pickups are routed by `AutoStoreOnPickup` on the item definition: resources auto-route to the ledger; gear/consumables/quest items land in the inventory. No bag slots, no capacity limits — pure data layer. This story delivers the APIs that future UI (US-071) and town systems (Epic 9) bind to.

**Depends on US-032** (item definition + category enums).

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `Specs/item_schema_v2.md` (Storage Rules section; Section 4 type defaults for Material/TownResource/UpgradeKey/MagicalItem)
- Design doc: `Specs/item_system_v1.md`
- Agent rules: `Specs/agent_rules_v2.md` Section 2 (Locked: unlimited carry, auto-store, DO/DO NOT lists)
- Existing code: US-032 `UMordecaiItemDefinition`, `UMordecaiItemLibrary::CompareSortPriority`
- Existing code: `Mordecai/Weapons/MordecaiEquipmentComponent.h` (player-component pattern)
- Lyra reference: `Source/LyraGame/Inventory/LyraInventoryManagerComponent.h` — do NOT subclass; use as reference for replicated add/remove patterns

---

## Acceptance Criteria

### Instance Struct
- [x] AC-031.1: `FMordecaiItemInstance` USTRUCT exists in `Mordecai/Items/MordecaiItemInstance.h` with fields:
  - `InstanceId` (FGuid) — unique per pickup
  - `ItemDefinition` (TObjectPtr<UMordecaiItemDefinition>)
  - `Quantity` (int32, default 1)
  - `AffixRolls` (TArray<FName>, default empty — placeholder for future affix system)
  - `IsEquipped` (bool, default false)
  - `IsValid() const` helper returning `ItemDefinition != nullptr && Quantity > 0`
- [x] AC-031.2: `FMordecaiItemInstance` supports NetSerialize (implement `NetSerialize` or use FFastArraySerializerItem if wrapping in a FastArray container). Replication over network is required per agent rules (server-authoritative).

### Inventory Component
- [x] AC-031.3: `UMordecaiInventoryComponent` (UActorComponent) exists in `Mordecai/Items/`. Replicated component. Attached to the Mordecai character as part of its default component set.
- [x] AC-031.4: `UMordecaiInventoryComponent::AddItem(UMordecaiItemDefinition* Def, int32 Quantity = 1) -> FGuid` — adds an item. Routing rules:
  - If `Def->IsAutoStored()` → forwards to the owner's `UMordecaiResourceLedger` and returns an invalid FGuid; the inventory list is NOT modified
  - Else if `Def->IsStackable()` AND an existing non-full stack of the same ItemDefinition exists → merges into that stack up to `MaxStackSize`; creates additional stack instance(s) for overflow; returns the InstanceId of the first modified stack
  - Else → creates a new `FMordecaiItemInstance` and appends it; returns its InstanceId
- [x] AC-031.5: `UMordecaiInventoryComponent::RemoveItem(const FGuid& InstanceId, int32 Quantity = 1) -> bool` — decrements quantity; removes the instance entirely when quantity reaches zero. Returns true on success, false if instance not found or not enough quantity. Does NOT touch the ledger.
- [x] AC-031.6: `UMordecaiInventoryComponent::ConsumeByDefinition(UMordecaiItemDefinition* Def, int32 Quantity) -> bool` — removes `Quantity` total across any instances of `Def`, preferring lowest-quantity stacks first. Returns false if the total across all stacks is < Quantity (and makes NO modifications in that case — atomic).
- [x] AC-031.7: `UMordecaiInventoryComponent::GetAllItems() const -> const TArray<FMordecaiItemInstance>&` — returns all instances.
- [x] AC-031.8: `UMordecaiInventoryComponent::GetItemsByType(EMordecaiItemType Type) const -> TArray<FMordecaiItemInstance>` — returns instances filtered by type.
- [x] AC-031.9: `UMordecaiInventoryComponent::GetSortedItems() const -> TArray<FMordecaiItemInstance>` — returns all items sorted using `UMordecaiItemLibrary::CompareSortPriority`.
- [x] AC-031.10: `UMordecaiInventoryComponent::FindInstance(const FGuid& InstanceId) const -> const FMordecaiItemInstance*` — lookup by InstanceId.
- [x] AC-031.11: `UMordecaiInventoryComponent::GetTotalQuantityOfDefinition(UMordecaiItemDefinition* Def) const -> int32` — summed across all stacks.

### Resource Ledger
- [x] AC-031.12: `UMordecaiResourceLedger` (UActorComponent) exists in `Mordecai/Items/`. Replicated. Attached to the Mordecai character alongside the inventory component. Holds `TMap<FName /*ItemId*/, int32 /*Count*/>`. **Note:** in the launch scope this lives on the character for simplicity; a future story can migrate it to the PlayerState or a dedicated subsystem when town persistence lands (out of scope).
- [x] AC-031.13: `UMordecaiResourceLedger::AddResource(UMordecaiItemDefinition* Def, int32 Quantity)` — increments the count for `Def->ItemId`; rejects non-auto-stored items (logs a warning, returns early).
- [x] AC-031.14: `UMordecaiResourceLedger::GetResourceCount(FName ItemId) const -> int32` — returns the count (0 if missing).
- [x] AC-031.15: `UMordecaiResourceLedger::ConsumeResource(FName ItemId, int32 Quantity) -> bool` — atomically decrements; returns false (no change) if count < Quantity. Used later by crafting.
- [x] AC-031.16: `UMordecaiResourceLedger::GetAllResources() const -> TArray<FMordecaiResourceEntry>` where `FMordecaiResourceEntry { FName ItemId; int32 Count; TObjectPtr<UMordecaiItemDefinition> Def; }`. Used for town-storage UI in the future.

### Events
- [x] AC-031.17: `UMordecaiInventoryComponent` declares `FOnMordecaiInventoryChanged` multicast delegate (signature: `const FGuid& InstanceId, int32 QuantityDelta`). Fires on AddItem (when routed to inventory, not ledger) and RemoveItem.
- [x] AC-031.18: `UMordecaiResourceLedger` declares `FOnMordecaiResourceChanged` multicast delegate (signature: `FName ItemId, int32 NewCount`). Fires on AddResource and ConsumeResource.

### Pickup Routing
- [x] AC-031.19: Single entry point `UMordecaiInventoryComponent::PickupItem(UMordecaiItemDefinition* Def, int32 Quantity)` that encapsulates the routing decision: calls `ResourceLedger->AddResource(Def, Quantity)` if `Def->IsAutoStored()`, else `this->AddItem(Def, Quantity)`. This is the canonical pickup API for loot drops to call — keeps the routing decision in one place.
- [x] AC-031.20: There is **no carry limit** (no MaxItems, no weight). `AddItem` and `AddResource` never reject based on capacity. This enforces the "unlimited carry" locked rule.

### Character Wiring
- [x] AC-031.21: `AMordecaiCharacter` constructor adds both `UMordecaiInventoryComponent` and `UMordecaiResourceLedger` as default subobjects. They are accessible via `GetInventoryComponent()` and `GetResourceLedger()` accessors on the character.

## Technical Notes
- **File location:** `Source/LyraGame/Mordecai/Items/` — same directory as US-032 types.
- **FastArray:** For `FMordecaiItemInstance` replication efficiency, wrap the inventory list in an `FFastArraySerializer` (mirror the pattern in `LyraInventoryList` / `FLyraInventoryList` in `LyraInventoryManagerComponent.h`). A planner decision: adopt FastArray; the weapon equipment component replicates simpler state and doesn't need it, but inventory churn warrants it.
- **Atomicity in ConsumeByDefinition (AC-031.6):** Compute the summed total first; return false immediately if < requested; only then mutate. Same rule for `ConsumeResource` (AC-031.15).
- **Do NOT implement:**
  - Inventory UI widgets (US-071, later)
  - Equipping flow (weapon equip already lives in `UMordecaiEquipmentComponent`; tying inventory → equip is a future story)
  - Loot drop / world pickup actors (we test via direct `PickupItem` calls)
  - Identification (US-033)
  - Save/load persistence (future)
  - Town-bank mirroring / cross-character sharing (the ledger is per-character for launch)
- **Replication note for headless tests:** Automation tests run single-process. Verify replication intent via UPROPERTY `Replicated` specifiers and `GetLifetimeReplicatedProps` coverage — do not simulate network in tests.
- **TODO(DECISION) to flag:** The ledger currently lives on the character. When town persistence lands, this likely migrates to PlayerState or a GameInstance subsystem. Mark the class header comment with `// TODO(DECISION): Migrate to PlayerState when town persistence ships.`

## Tests Required
- [x] `Mordecai.Inventory.AddNonAutoStoredCreatesInstance` — adding a Weapon (non-autostored) appends to inventory, ledger untouched (AC-031.4)
- [x] `Mordecai.Inventory.AddAutoStoredRoutesToLedger` — adding a Material (autostored) increments ledger, inventory untouched (AC-031.4, 031.19)
- [x] `Mordecai.Inventory.AddStackableMergesIntoExistingStack` — two adds of same Consumable def (Stackable, MaxStackSize=10) produces one instance with Quantity=2 (AC-031.4)
- [x] `Mordecai.Inventory.AddStackableOverflowsWhenAtMax` — adding 5 more to a stack of 10 with MaxStackSize=10 creates a new stack with Quantity=5 (AC-031.4)
- [x] `Mordecai.Inventory.RemoveItemDecrementsQuantity` — RemoveItem with Quantity=1 on a stack of 3 leaves Quantity=2 (AC-031.5)
- [x] `Mordecai.Inventory.RemoveItemDeletesInstanceAtZero` — RemoveItem that zeroes Quantity removes the instance entirely (AC-031.5)
- [x] `Mordecai.Inventory.ConsumeByDefinitionIsAtomic` — requesting more than total quantity returns false and leaves inventory unchanged (AC-031.6)
- [x] `Mordecai.Inventory.ConsumeByDefinitionPrefersLowestStack` — consuming 3 from two stacks (5 and 2) removes the 2-stack first (AC-031.6)
- [x] `Mordecai.Inventory.GetItemsByTypeFiltersCorrectly` — adding one Weapon and one Consumable, filter by Weapon returns only the weapon (AC-031.8)
- [x] `Mordecai.Inventory.GetSortedItemsOrdersByPriority` — Critical SortPriority appears before Normal in sorted output (AC-031.9)
- [x] `Mordecai.Inventory.NoCapacityLimit` — 1000 distinct non-stackable items can be added without rejection (AC-031.20)
- [x] `Mordecai.Inventory.ChangedDelegateFiresOnAdd` — binding to OnInventoryChanged fires with +Quantity delta on AddItem (AC-031.17)
- [x] `Mordecai.Inventory.ChangedDelegateFiresOnRemove` — delegate fires with -Quantity delta on RemoveItem (AC-031.17)
- [x] `Mordecai.Ledger.AddResourceIncrementsCount` — AddResource raises GetResourceCount by Quantity (AC-031.13, 031.14)
- [x] `Mordecai.Ledger.AddResourceRejectsNonAutoStored` — AddResource with a Weapon definition logs warning and does not change count (AC-031.13)
- [x] `Mordecai.Ledger.ConsumeResourceIsAtomic` — consuming more than available returns false and does not change count (AC-031.15)
- [x] `Mordecai.Ledger.ChangedDelegateFiresOnAdd` — OnResourceChanged fires with new total on AddResource (AC-031.18)
- [x] `Mordecai.Character.HasInventoryAndLedgerComponents` — AMordecaiCharacter exposes non-null InventoryComponent and ResourceLedger accessors (AC-031.21)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [ ] PIE smoke test passes (`py Scripts/run_pie_verify.py`) _(skipped: pure data-layer C++ with no runtime/visual behavior; CDO-level component wiring verified via `Mordecai.Character.HasInventoryAndLedgerComponents` automation test)_
- [x] Code committed and pushed with `[US-031]` prefix
