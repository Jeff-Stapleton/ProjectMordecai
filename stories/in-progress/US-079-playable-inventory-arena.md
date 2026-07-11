# US-079: Playable Inventory Arena Integration

## Overview
Wire the inventory system (US-031/032/033) and inventory widget (US-071) into the playable arena so Jeff can pick up test items, see them populate the pause-menu Inventory tab, trigger auto-store for materials, and identify unidentified magical items. This is the EDITOR half of the Epic 7 integration milestone — per the Playability-First Rule, after three HEADLESS inventory stories we need a playable prototype to prove the system feels right end-to-end.

Scope includes: pickup actor class (C++ scaffold, BP placement), a small set of test item DataAssets covering every major routing case, arena loot placement, pause-menu tab wiring, and a PIE smoke test that exercises pickup → auto-store → open inventory → identify.

## Execution Mode
**Mode:** EDITOR

## References
- Design doc: `item_schema_v2.md` Section 4 — recommended type defaults (Material, TownResource, UpgradeKey, MagicalItem)
- Design doc: `agent_rules_v2.md` Section 2 — unlimited carry, auto-store for materials, town gating for magical items
- Design doc: `game_design_v2.md` Section 8 — clean UI, diorama pickup flow
- Existing code: `UMordecaiInventoryComponent::PickupItem`, `UMordecaiResourceLedger`, `UMordecaiIdentificationService`, `UMordecaiPauseMenuSubsystem::OpenPauseMenu`, `UMordecaiInventoryWidget` (US-071)
- Pattern reference: US-057 (Playable Magic Arena) and US-078 (Playable Weapon Arena) — the EDITOR playable-integration template
- Depends on: US-071 (Inventory Widget) — blocks on US-071 landing first

---

## Acceptance Criteria

### Pickup Actor (C++ Scaffold)
- [ ] AC-079.1: `AMordecaiItemPickup` actor exists in `Source/LyraGame/Mordecai/Items/` with: a root `USceneComponent`, a `USphereComponent` trigger (configurable radius, default 120cm), and an optional `UStaticMeshComponent` for in-world display. `EditAnywhere` properties: `TObjectPtr<UMordecaiItemDefinition> ItemDefinition`, `int32 Quantity = 1`, `bool bDestroyOnPickup = true`.
- [ ] AC-079.2: On `BeginOverlap` with a `AMordecaiCharacter`, the actor resolves the character's `UMordecaiInventoryComponent` and calls `PickupItem(ItemDefinition, Quantity)`. If `bDestroyOnPickup`, the actor destroys itself after pickup. Null-safety: no-op if ItemDefinition is null, no-op if inventory component is missing.
- [ ] AC-079.3: Server-authoritative: pickup logic only runs on the server. The actor is replicated (`bReplicates = true`), and destruction propagates. Tests may verify via an automation spec that a simulated overlap on a standalone world calls `PickupItem` exactly once.

### Test Item DataAssets
- [ ] AC-079.4: Create `UMordecaiItemDefinition` assets under `Plugins/GameFeatures/MordecaiCore/Content/Items/TestPickups/`. At minimum the following 6 assets:
  - `DA_TestItem_HealthPotion` — Consumable, Common, Stackable, MaxStackSize=10, no identification
  - `DA_TestItem_IronOre` — Material, Common, AutoStoreOnPickup=true, StorageDomain=TownStorage
  - `DA_TestItem_TownStone` — TownResource, Common, AutoStoreOnPickup=true
  - `DA_TestItem_UpgradeKey` — UpgradeKey, Rare, SortPriority=Critical
  - `DA_TestItem_MysteriousAmulet` — MagicalItem, Epic, UsesIdentification=true, DefaultIdentificationState=Unidentified, RequiresIdentificationToEquip=true
  - `DA_TestItem_RustySword` — Weapon, Common, UsesIdentification=false
- [ ] AC-079.5: Each DataAsset has a valid `ItemId` (FName matching filename suffix), `DisplayName`, `Description`, and correct defaults per `item_schema_v2.md` Section 4. Create via Python editor scripting (`ue_remote_exec.py`) so the story is reproducible — check the script into `Scripts/create_test_pickups.py`.

### Arena Wiring
- [ ] AC-079.6: Place a minimum of 6 `AMordecaiItemPickup` instances in the existing test arena level — one for each test DataAsset. Positions are reachable by the player-spawn, arranged in a visible loop so Jeff can run through and collect them all in under 30 seconds.
- [ ] AC-079.7: The player character's `UMordecaiInventoryComponent` and `UMordecaiResourceLedger` are correctly attached and replicated (already true per US-031; this AC verifies the arena actually spawns them). Verified by a PIE smoke test that reads both components off the possessed pawn.

### Pause Menu Tab Wiring
- [ ] AC-079.8: The Pause Menu's "Inventory" tab (registered via `UMordecaiPauseMenuWidget::RegisterTab("inventory", ...)`) resolves to `UMordecaiInventoryWidget` (US-071). Wiring happens in the pause menu subsystem or a bootstrap BP. When the widget is created, it receives the local player's inventory + ledger via `BindToInventory`.
- [ ] AC-079.9: Opening the pause menu with no items shows the empty-state placeholder. Opening after picking up items shows the flat list. The ledger panel shows all auto-stored resources. Verified by PIE smoke test.

### Blueprint / Visual Polish (Minimum Viable)
- [ ] AC-079.10: `BP_MordecaiItemPickup` Blueprint subclass exists, exposing a particle/mesh preview plus a text widget showing the item's `DisplayName` in-world (so Jeff can tell pickups apart at a glance). Text uses `UMordecaiItemLibrary::GetDisplayName` so unidentified pickups show partial info.
- [ ] AC-079.11: The Inventory widget tab has a minimal Blueprint polish pass: category filter buttons are visible and clickable, rarity colors apply to row backgrounds/borders, unidentified items show a "?" badge, and the identify button appears on unidentified rows. Blueprint polish stops at "playable and readable" — full visual polish is deferred.

### PIE Smoke Test
- [ ] AC-079.12: `Scripts/verify_pie_inventory.py` exists and exercises the full flow:
  1. Enter PIE
  2. Teleport pawn to each pickup, simulate overlap (or wait for natural overlap), verify pickup actor destroys
  3. After all pickups collected, assert inventory component has the expected non-autostored instances (HealthPotion, UpgradeKey, MysteriousAmulet, RustySword — 4 entries)
  4. Assert resource ledger has IronOre=1, TownStone=1
  5. Open the pause menu via the subsystem
  6. Verify the Inventory tab's widget is active and `GetVisibleRowCount()` reflects the expected count
  7. Call `TryIdentify` on the MysteriousAmulet instance; assert it succeeds and the row rebuilds as identified
  8. Print PASS/FAIL lines; raise `RuntimeError` on any assertion failure
- [ ] AC-079.13: `Scripts/run_pie_verify.py --tests inventory` invokes the new script. The pipeline (launch editor → enter PIE → run verify → exit PIE → close editor) succeeds end-to-end.

## Technical Notes
- **Pickup actor placement:** Use Python editor scripting (`unreal.EditorLevelLibrary.spawn_actor_from_class`) to place pickups at fixed offsets from the arena spawn point. Coordinates live in `Scripts/place_test_pickups.py` so the arena is rebuildable.
- **DataAsset creation:** Follow the pattern from US-057 (spell DataAssets) and US-078 (weapon DataAssets). The Python script loads `UMordecaiItemDefinition` class, creates assets in the target folder, sets UPROPERTIES, saves.
- **Pause menu tab registration:** The existing pattern from US-066/067/068 registers tab content during `UMordecaiPauseMenuSubsystem` construction or during character setup. For this story, register `UMordecaiInventoryWidget` as the "inventory" tab content class and pass the bound inventory + ledger references after widget creation.
- **Arena level:** Reuse the existing test arena from US-054/078 — do not create a new level. Add pickups to that map. If the arena map name has drifted, use whatever PIE-launches today.
- **Input:** Pickup uses overlap, not a button press. The player just walks over items. This matches the auto-pickup rhythm implied by unlimited-carry.
- **Replication:** `AMordecaiItemPickup` is server-authoritative. The overlap event is server-handled; the pickup actor is replicated so clients see it destroy.
- **Identification test:** The PIE smoke test's identify call exercises the `UMordecaiIdentificationService` subsystem as wired in a real GameInstance — this is the first time that service runs outside automation tests.
- **Don't build town/Mage-Tower UI** — out of scope. The identify button on the inventory row is sufficient for this milestone. Full town-service UI is a future story.

## Tests Required
- [ ] `Mordecai.Items.ItemPickup.OverlapCallsPickupItem` — Simulated overlap on a standalone world calls `UMordecaiInventoryComponent::PickupItem` exactly once with the configured definition and quantity (AC-079.2)
- [ ] `Mordecai.Items.ItemPickup.DestroysAfterPickup` — With `bDestroyOnPickup=true`, actor is destroyed after pickup fires (AC-079.2)
- [ ] `Mordecai.Items.ItemPickup.NullDefinitionNoOp` — Pickup with null ItemDefinition does not crash and does not destroy itself (AC-079.2)
- [ ] `Mordecai.Items.ItemPickup.ClientOverlapDoesNotPickup` — Overlap on a non-authority actor does not call PickupItem (server-authority check) (AC-079.3)

(Plus PIE smoke test AC-079.12 — runs as Python verification, not a C++ automation test.)

---

## Definition of Done
- [ ] All C++ automation tests written and passing
- [ ] `AMordecaiItemPickup` C++ class compiles with zero errors
- [ ] All 6 test DataAssets created and saved; `Scripts/create_test_pickups.py` reproduces them
- [ ] 6 pickups placed in arena; `Scripts/place_test_pickups.py` reproduces the layout
- [ ] Pause Menu "Inventory" tab wired to `UMordecaiInventoryWidget`
- [ ] BP_MordecaiItemPickup created with in-world label
- [ ] Inventory widget has minimum viable polish (filters, rarity colors, identify button)
- [ ] `Scripts/verify_pie_inventory.py` exists and passes
- [ ] `py Scripts/run_pie_verify.py --tests inventory` runs end-to-end clean
- [ ] Code committed and pushed with `[US-079]` prefix
- [ ] Editor closed after verification (`Stop-Process -Name "UnrealEditor"`)
