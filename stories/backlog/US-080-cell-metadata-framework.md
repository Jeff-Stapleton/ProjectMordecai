# US-080: Cell Framework — Metadata Asset & Core World Types

## Overview
Build the C++ data foundation for the authored-cell world model: the cell metadata asset type plus the enums and structs that describe a cell's identity, exits, height tiers, and gate requirements. This is **Milestone 1 (Cell Framework)** from `map_style_technical_design_v1.md` §12 — the metadata-asset half. Per `agent_rules_v2.md`, the cell metadata asset (`DA_Cell_*`) is the **source of truth** and **must exist before geometry**.

This story implements the *already-locked* data model only. It does **not** introduce new gate types, new biomes, new height-tier rules, or connection-socket standards — those changes require human approval (see Blockers). It encodes the vocabulary that the design docs have already fixed.

Per the dependency chain, this is the first Epic 8 story. US-081 (markup actors) and US-082 (validation library) build on these types, and the movement/exploration skills (US-034/035) ultimately interact with the markup these types describe.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `map_style_technical_design_v1.md` §3 (map cell model), §3.2 (room-graph node types), §4.4 (height tiers), §5.2 (cell metadata schema), §11 (folder/naming), §12 (Milestone 1)
- Design doc: `agentic_map_pipeline_v1.md` §3 (cell templates), §8 (human-approval gates)
- Design doc: `world_design_v1.md` — 6 biomes + final area, biome-to-gate mapping
- Design doc: `agent_rules_v2.md` — locked map rules: cells are authored chunks, metadata-first, markup is source of truth, height tiers mandatory, naming `DA_Cell_<BiomeAbbrev>_<Number>`
- Pattern reference: `UMordecaiItemDefinition` (`Source/LyraGame/Mordecai/Items/MordecaiItemDefinition.h`) — `UDataAsset` subclass with `EditDefaultsOnly, BlueprintReadOnly` UPROPERTYs and `Mordecai|...` category convention; `UMordecaiSkillDataAsset` — `GetPrimaryAssetId()` pattern for cooked/streamed assets

---

## Acceptance Criteria

### Core Enums
- [ ] AC-080.1: `EMordecaiBiomeId` enum exists in `Source/LyraGame/Mordecai/World/MordecaiWorldTypes.h` with values matching `world_design_v1.md`: `VerdantWilds`, `SearingExpanse`, `MireOfEchoes`, `CrystalCaverns`, `StormPeaks`, `WitheredKingdom`, plus `None = 0` as the unset default. Each carries a stable `UMETA(DisplayName=...)`.
- [ ] AC-080.2: `EMordecaiCellType` enum exists with the room-graph node types from `map_style_technical_design_v1.md` §3.2: `Path`, `Arena`, `Gate`, `Vista`, `Secret`, `Service`, `DungeonEntrance`, `ShortcutReturn`, `Transition`. Default is `Path`.
- [ ] AC-080.3: `EMordecaiHeightTier` enum exists encoding the locked tier vocabulary (§4.4) with explicit underlying values: `Ground = 0`, `Ledge = 1`, `WallTop = 2`, `Lower = -1`. The enum's underlying type is `int8` so the negative value is representable.
- [ ] AC-080.4: `EMordecaiGateRequirement` enum encodes the gate types already documented in `world_design_v1.md` / `map_style_technical_design_v1.md` §3.3: `None = 0`, `Explosives`, `MagneticDevice`, `Invisibility`, `PhaseTalisman`, `Glider`, `Lockpick`, `Climb`, `Swim`. This enum enumerates *documented* gates only — adding a new value is a human-approval change (see Blockers).

### Exit / Connection Struct
- [ ] AC-080.5: `FMordecaiCellExit` USTRUCT exists with: `FName ExitSocketId` (named connection socket), `EMordecaiCellExitDirection Direction` (enum: `North`, `East`, `South`, `West`, `Up`, `Down`), `FName TargetCellId` (the cell this exit links to; `NAME_None` if unlinked), `EMordecaiGateRequirement GateRequirement` (gate that blocks this exit, `None` if open), and `bool bOneWay` (true for one-way drops/shortcuts). All fields `EditAnywhere, BlueprintReadWrite`.
- [ ] AC-080.6: `EMordecaiCellExitDirection` enum exists with the 6 values above. A static helper `UMordecaiWorldLibrary::GetOpposite(EMordecaiCellExitDirection)` returns the opposing direction (North↔South, East↔West, Up↔Down) — used later by connectivity validation.

### Cell Metadata Asset
- [ ] AC-080.7: `UMordecaiCellMetadata` (`UDataAsset` subclass, `BlueprintType`) exists in `Source/LyraGame/Mordecai/World/`. It implements every field in the locked schema (`map_style_technical_design_v1.md` §5.2): `FName CellId`, `EMordecaiBiomeId BiomeId`, `EMordecaiCellType CellType`, `FGameplayTagContainer ThemeTags`, `TArray<EMordecaiHeightTier> HeightTiersUsed`, `TArray<FMordecaiCellExit> Exits`, `EMordecaiPrimaryPathRole PrimaryPathRole`, `TArray<EMordecaiGateRequirement> GateRequirements`, `bool bContainsSecret`, `bool bContainsCombat`, `bool bContainsPuzzle`, `int32 EncounterBudget`, `FName LandmarkType`, `int32 RevisitValue`, `TArray<FName> Dependencies`, `TArray<FName> TestChecklist`.
- [ ] AC-080.8: `EMordecaiPrimaryPathRole` enum exists with at least: `PrimaryRoute`, `SideRoute`, `Shortcut`, `DeadEnd`, `Hub`. Default `PrimaryRoute`.
- [ ] AC-080.9: `UMordecaiCellMetadata` overrides `GetPrimaryAssetId()` to return a stable `FPrimaryAssetId` of type `"MordecaiCell"` keyed on `CellId` (mirrors `UMordecaiSkillDataAsset`).
- [ ] AC-080.10: `UMordecaiCellMetadata::GetExitBySocket(FName SocketId)` returns a `const FMordecaiCellExit*` (nullptr if not found). `GetExitsRequiringGate(EMordecaiGateRequirement)` returns a `TArray<FMordecaiCellExit>` filtered to that gate.

### Naming Helper
- [ ] AC-080.11: `UMordecaiWorldLibrary::GetBiomeAbbreviation(EMordecaiBiomeId)` returns the canonical abbreviation used in asset names (`VerdantWilds`→`"VW"`, `SearingExpanse`→`"SE"`, `MireOfEchoes`→`"ME"`, `CrystalCaverns`→`"CC"`, `StormPeaks`→`"SP"`, `WitheredKingdom`→`"WK"`). `MakeCellId(EMordecaiBiomeId, int32 Number)` returns `FName` of the form `Cell_<Abbrev>_<NN>` (zero-padded to 2 digits, e.g. `Cell_VW_01`) per `map_style_technical_design_v1.md` §11.

## Technical Notes
- Place all code in a new `Source/LyraGame/Mordecai/World/` subdirectory. Types header: `MordecaiWorldTypes.h`. Asset: `MordecaiCellMetadata.h/.cpp`. Static helpers: `MordecaiWorldLibrary.h/.cpp` (a `UBlueprintFunctionLibrary`).
- This is **pure data + pure functions** — no actors, no ticking, no world dependency. All ACs are verifiable headlessly by constructing a `UMordecaiCellMetadata` via `NewObject`, setting fields, and asserting helper output / round-trip.
- Keep all enums `UENUM(BlueprintType)` so DataAssets and later Blueprint validators can use them. `EMordecaiHeightTier` needs `: int8` to represent `Lower = -1`.
- Do **not** add gate types, biomes, height tiers, or exit directions beyond those enumerated above — those are human-approval changes per `agentic_map_pipeline_v1.md` §8. If a needed value is missing, file a `TODO(DECISION)` rather than inventing one.
- `LandmarkType` is left as `FName` (not an enum) intentionally — the landmark kit vocabulary (`map_style_technical_design_v1.md` §6.2) is still expanding; a free-form name avoids premature locking. Note this with a `TODO(DECISION)` comment.
- No editor/asset creation in this story. The `DA_Cell_*` *content assets* and the blockout template cell are EDITOR work (future story, blocked on World Intent approval — see Blockers).

## Tests Required
- [ ] `Mordecai.World.CellTypes.HeightTierValues` — `EMordecaiHeightTier` casts to the locked ints (Ground=0, Ledge=1, WallTop=2, Lower=-1) (AC-080.3)
- [ ] `Mordecai.World.CellTypes.GateRequirementCoversDocumentedGates` — all documented gates (Explosives, MagneticDevice, Invisibility, PhaseTalisman, Glider, Lockpick, Climb, Swim) are present and `None=0` (AC-080.4)
- [ ] `Mordecai.World.CellTypes.OppositeDirection` — GetOpposite maps each of the 6 directions to its opposite (AC-080.6)
- [ ] `Mordecai.World.CellMetadata.HasAllSchemaFields` — a constructed metadata asset exposes and round-trips every §5.2 field (set then read back) (AC-080.7)
- [ ] `Mordecai.World.CellMetadata.PrimaryAssetIdFromCellId` — `GetPrimaryAssetId()` is type `MordecaiCell` and keyed on `CellId` (AC-080.9)
- [ ] `Mordecai.World.CellMetadata.GetExitBySocket` — returns the matching exit and nullptr for an unknown socket (AC-080.10)
- [ ] `Mordecai.World.CellMetadata.GetExitsRequiringGate` — filters exits to a given gate requirement; empty array when none match (AC-080.10)
- [ ] `Mordecai.World.CellMetadata.MakeCellIdFormatsName` — `MakeCellId(VerdantWilds, 1)` returns `Cell_VW_01`; `MakeCellId(StormPeaks, 12)` returns `Cell_SP_12` (AC-080.11)
- [ ] `Mordecai.World.CellMetadata.BiomeAbbreviationsAreCanonical` — all 6 biomes return their documented 2-letter abbreviations (AC-080.11)
- [ ] `Mordecai.World.CellMetadata.DefaultsAreNeutral` — a freshly constructed asset has CellType=Path, BiomeId=None, PrimaryPathRole=PrimaryRoute, empty exits/tiers (AC-080.2, AC-080.7, AC-080.8)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] All Mordecai tests still pass (no regressions)
- [ ] PIE smoke test passes (existing suite — confirm new types compile into the build without runtime errors)
- [ ] Code committed and pushed with `[US-080]` prefix
