# US-071: Inventory UI — Flat List, Filter & Ledger Widget

## Overview
Build the C++ inventory widget that displays the player's flat item list and auto-stored resource ledger. This is the "Inventory" tab content for the Pause Menu (US-069). The widget reads from the player's `UMordecaiInventoryComponent` and sibling `UMordecaiResourceLedger`, responds to their change delegates, supports category filtering, and exposes an "Identify" action that calls `UMordecaiIdentificationService` for unidentified items.

Per the Playability-First Rule, this is the HEADLESS half of the Epic 7 integration milestone. US-079 (EDITOR) follows to wire test pickups into the arena and verify the experience in PIE.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `item_schema_v2.md` — item categories, rarity tiers, identification rules, sort priority buckets
- Design doc: `item_system_v1.md` — rarity colors: Common, Green, Blue, Purple, Red/Gold
- Design doc: `agent_rules_v2.md` Section 2 — unlimited carry, town gating, auto-store for materials
- Design doc: `game_design_v2.md` Section 8 — clean UI principle
- Existing code: `UMordecaiInventoryComponent` (`OnInventoryChanged`, `GetSortedItems`, `GetItemsByType`), `UMordecaiResourceLedger` (`OnResourceChanged`, `GetAllResources`), `UMordecaiItemLibrary::GetDisplayName/GetDescription/GetVisibleTags/CanEquipInstance`, `UMordecaiIdentificationService::IdentifyInstance`
- Pattern reference: `UMordecaiCharacterSheetWidget` (US-066), `UMordecaiSkillTreeWidget` (US-067), `UMordecaiFeatDisplayWidget` (US-068) — ASC-bound widgets registered via `UMordecaiPauseMenuWidget::RegisterTab`
- Depends on: US-069 (Pause Menu), US-031 (InventoryComponent + ResourceLedger), US-032 (ItemDefinition), US-033 (Identification Service)

---

## Acceptance Criteria

### Widget Structure
- [ ] AC-071.1: `UMordecaiInventoryWidget` (UUserWidget) exists in `Source/LyraGame/Mordecai/UI/`. It is the content widget for the Pause Menu "Inventory" tab. Registers itself via `UMordecaiPauseMenuWidget::RegisterTab("inventory", ...)` (registration wired in the owning subsystem or the character setup — the widget itself exposes the class).
- [ ] AC-071.2: The widget has a `BindToInventory(UMordecaiInventoryComponent* Inventory, UMordecaiResourceLedger* Ledger)` method. Subscribes to `OnInventoryChanged` and `OnResourceChanged`. When either fires, the widget rebuilds its displayed rows (no polling). If either pointer is null, the widget displays "--" placeholders and skips binding for that source.
- [ ] AC-071.3: On destruction or when `Unbind()` is called, all delegate handles are cleared (no dangling bindings).

### Inventory List Display
- [ ] AC-071.4: Displays all instances returned by `UMordecaiInventoryComponent::GetSortedItems()` as a vertical list of rows. Each row exposes: icon (soft-pointer resolved), display name (via `UMordecaiItemLibrary::GetDisplayName`), quantity (shown only when `IsStackable()` and Quantity > 1), rarity color (mapped from `UMordecaiItemDefinition::Rarity`), identification badge (if `UsesIdentification` and not yet identified).
- [ ] AC-071.5: Row display name uses `UMordecaiItemLibrary::GetDisplayName(Instance)` so unidentified items automatically render partial-info or "Unknown Item" per the library rules. Row tooltip/description uses `UMordecaiItemLibrary::GetDescription(Instance)`.
- [ ] AC-071.6: Rarity color is resolved from a C++ color map keyed on `EMordecaiItemRarity`: Common→Gray, Uncommon/Green→Green, Rare/Blue→Blue, Epic/Purple→Purple, Legendary/Red→Red, Mythic/Gold→Gold. Exact FLinearColor values are declared as static constexpr in the widget header.

### Category Filtering
- [ ] AC-071.7: Widget exposes a filter state `EMordecaiInventoryFilter` with values: `All`, `Weapons`, `Armor`, `Trinkets`, `Consumables`, `Materials`, `Quest`, `Magical`. `SetFilter(EMordecaiInventoryFilter)` changes the filter. Default is `All`.
- [ ] AC-071.8: When the filter is not `All`, the displayed list is restricted to items whose `UMordecaiItemDefinition::ItemType` matches the filter. `Materials` filter includes both `Material` and `TownResource`. `Magical` filter includes `MagicalItem` and `UpgradeKey`. `Quest` filter includes `QuestItem`. Filter changes do not re-fetch from the component — the widget filters its cached row set.
- [ ] AC-071.9: Filter changing triggers `OnFilterChanged` (BlueprintAssignable, `FName NewFilterId`). Tests assert the delegate fires exactly once per call and that `GetVisibleRowCount()` reflects the filter.

### Resource Ledger Panel
- [ ] AC-071.10: A separate ledger section displays entries from `UMordecaiResourceLedger::GetAllResources()`. Each entry shows: display name (from `Entry.Def->DisplayName`, or `Entry.ItemId.ToString()` if Def is null), count (int32). Sorted alphabetically by display name for readability. Updates on `OnResourceChanged`.
- [ ] AC-071.11: If the ledger is empty, the panel displays a single "No resources stored." placeholder FText. If the ledger is unbound (null), the panel displays "--".

### Identify Action
- [ ] AC-071.12: For rows whose instance is unidentified (`Instance.IdentificationState == Unidentified`) AND whose definition `UsesIdentification`, the widget exposes a `TryIdentify(const FGuid& InstanceId)` method. Calling it locates the owning `GameInstance` via `GetGameInstance()`, retrieves `UMordecaiIdentificationService`, and calls `IdentifyInstance(Inventory, InstanceId)`. Returns the bool result from the service.
- [ ] AC-071.13: On successful identification, the inventory component fires `OnInventoryChanged` (via `SetInstanceIdentificationState` → delta=0), which triggers the widget to re-render. The row now displays the identified name/description/tags. No extra manual refresh call is required.
- [ ] AC-071.14: `TryIdentify` is a no-op (returns false) when called on an already-identified instance, a non-existent InstanceId, or an instance whose definition does not use identification. Does not crash when the service subsystem is unavailable.

### Empty State
- [ ] AC-071.15: When the inventory component has zero instances and the ledger has zero entries, the widget shows a single "Inventory is empty." message. When the filter excludes all current items but the inventory is not itself empty, shows "No items match this filter." instead.

## Technical Notes
- Place all code in `Source/LyraGame/Mordecai/UI/`.
- **Row model:** Build a lightweight `FMordecaiInventoryRowModel` USTRUCT holding `FGuid InstanceId`, `FText DisplayName`, `FText Description`, `int32 Quantity`, `EMordecaiItemType Type`, `EMordecaiItemRarity Rarity`, `bool bIsUnidentified`, `TSoftObjectPtr<UTexture2D> Icon`. The widget caches a `TArray<FMordecaiInventoryRowModel>` computed from the component; filtering operates on this cache.
- **Rebuild pattern:** On any change delegate fire, clear the cache, call `GetSortedItems()`, rebuild the row model array, then reapply the current filter. Single pass — no incremental diffing. This matches the pattern used by US-067 (Skill Tree) and US-068 (Feat Display).
- **Rarity color:** Declare `static const TMap<EMordecaiItemRarity, FLinearColor> RarityColors` (or a free function returning FLinearColor). Keep the color palette in one place so US-079 Blueprint polish can override if desired.
- **Pause menu registration:** The widget is a content class passed to `UMordecaiPauseMenuWidget::RegisterTab`. Default registration wiring (who calls RegisterTab) belongs in the pause menu subsystem or the character; US-071 tests register the widget manually as part of their Arrange.
- **Binding source:** In normal play, the widget is bound to the local player's `UMordecaiInventoryComponent` and `UMordecaiResourceLedger`. Test fixtures can create these components directly on a scratch actor. No ASC dependency for this widget.
- **NullRHI testing:** Widget creation, component binding, row model rebuild, filter switching, and identify call dispatch can all be verified headlessly by inspecting widget state — no visual rendering required. Follow the test patterns in `MordecaiCharacterSheetWidgetTests.cpp` / `MordecaiSkillTreeWidgetTests.cpp`.
- **Dropping items:** OUT OF SCOPE. Drop/trash actions are a later story. This widget is read + identify only.
- **Equipping items:** OUT OF SCOPE. Equip routing through the inventory UI is deferred (Weapons have their own cycling UI via US-077). This widget can display `IsEquipped` state but exposes no equip action.

## Tests Required
- [ ] `Mordecai.UI.Inventory.BindsAndUnbinds` — Binding to a component subscribes `OnInventoryChanged`; unbinding clears the handle and no updates propagate (AC-071.2, AC-071.3)
- [ ] `Mordecai.UI.Inventory.GracefulWithoutBinding` — Widget with null inventory and ledger shows "--" placeholders and does not crash on filter changes (AC-071.2, AC-071.11)
- [ ] `Mordecai.UI.Inventory.DisplaysFlatList` — Adding N items to the component produces N rows in the widget in `GetSortedItems()` order (AC-071.4)
- [ ] `Mordecai.UI.Inventory.RowUsesLibraryDisplayName` — An unidentified item's row display name matches `UMordecaiItemLibrary::GetDisplayName(Instance)` (partial-info form) (AC-071.5)
- [ ] `Mordecai.UI.Inventory.RarityColorMap` — All 6 rarities map to distinct FLinearColors; Common is Gray (AC-071.6)
- [ ] `Mordecai.UI.Inventory.FilterRestrictsList` — Setting filter to `Weapons` shows only Weapon-type rows; `Materials` filter includes both Material and TownResource (AC-071.7, AC-071.8)
- [ ] `Mordecai.UI.Inventory.FilterFiresDelegate` — SetFilter fires `OnFilterChanged` exactly once per call with the new filter id (AC-071.9)
- [ ] `Mordecai.UI.Inventory.LedgerPanelShowsResources` — Adding auto-stored items populates the ledger section with correct counts, sorted by display name (AC-071.10)
- [ ] `Mordecai.UI.Inventory.LedgerEmptyStateMessage` — Empty ledger shows "No resources stored." placeholder (AC-071.11)
- [ ] `Mordecai.UI.Inventory.IdentifyActionFlipsState` — TryIdentify on a valid unidentified instance returns true, flips state to Identified, row rebuilds with identified name (AC-071.12, AC-071.13)
- [ ] `Mordecai.UI.Inventory.IdentifyNoopOnIdentified` — TryIdentify on an already-identified instance returns false and does not modify state (AC-071.14)
- [ ] `Mordecai.UI.Inventory.IdentifyNoopOnMissingInstance` — TryIdentify with an unknown FGuid returns false and does not crash (AC-071.14)
- [ ] `Mordecai.UI.Inventory.IdentifyWithoutServiceSubsystem` — Widget whose GameInstance has no IdentificationService registered returns false from TryIdentify without crashing (AC-071.14)
- [ ] `Mordecai.UI.Inventory.EmptyInventoryShowsPlaceholder` — Zero items + zero ledger entries shows "Inventory is empty." (AC-071.15)
- [ ] `Mordecai.UI.Inventory.FilterHidesAllShowsAltPlaceholder` — Non-empty inventory filtered to a category with zero matches shows "No items match this filter." (AC-071.15)
- [ ] `Mordecai.UI.Inventory.RebuildsOnInventoryChanged` — Adding an item after binding triggers a rebuild and the new row appears; removing triggers a rebuild and the row disappears (AC-071.2, AC-071.4)
- [ ] `Mordecai.UI.Inventory.RebuildsOnResourceChanged` — Auto-storing a material after binding triggers the ledger panel to update its count (AC-071.2, AC-071.10)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] PIE smoke test passes (headless widget creation via `run_pie_verify.py` — confirm the widget class compiles into the pause menu tab slot without runtime errors)
- [ ] Code committed and pushed with `[US-071]` prefix
