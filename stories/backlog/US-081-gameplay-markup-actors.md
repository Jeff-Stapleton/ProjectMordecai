# US-081: Cell Framework — Gameplay Markup Actors & Cell Registry

## Overview
Build the **gameplay markup layer** — the queryable actors that are the *source of truth* for walkability, walls, ledges, climb edges, hazards, water, gates, spawns, and interactables in a cell (independent of art mesh). This is the second half of **Milestone 1 (Cell Framework)** from `map_style_technical_design_v1.md` §5, which calls the markup layer "the most important for agents."

The whole point of the markup layer is that it is **queryable by tools and tests** (`map_style_technical_design_v1.md` §5.1): an agent or validator must be able to ask "which cells contain explosive gates?", "which arenas have no spawn group?", "which exits are one-way drops?". This story delivers the markup actors plus a per-cell registry subsystem that answers those queries.

Depends on US-080 for the shared world enums (`EMordecaiGateRequirement`, `EMordecaiHeightTier`). The movement/exploration skills in US-034/035 will later read these markers (e.g., Climbing interacts with `PM_ClimbEdge`, Swimming with `PM_DeepWater`).

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `map_style_technical_design_v1.md` §4.2 (gameplay shape vs art mesh — markup is source of truth), §5.1 (required markup actors — the 19 `PM_*` types), §5.2 (markup must be queryable), §10.1 (source-of-truth hierarchy)
- Design doc: `agentic_map_pipeline_v1.md` §2 Stage D (gameplay markup), §6 (automation hooks: "report all gates by requirement type", "show missing markup overlays")
- Design doc: `agent_rules_v2.md` — naming `PM_<Type>_<Variant>`; markup is source of truth for collision/walkability, NOT art meshes
- Depends on: US-080 (world enums: `EMordecaiGateRequirement`, `EMordecaiHeightTier`)
- Pattern reference: existing Mordecai WorldSubsystem (`UMordecaiHitDetectionSubsystem`) for subsystem registration; `AMordecaiProjectile` for a simple replicated actor scaffold

---

## Acceptance Criteria

### Markup Type Vocabulary
- [ ] AC-081.1: `EMordecaiMarkupType` enum exists in `Source/LyraGame/Mordecai/World/MordecaiMarkupTypes.h` with one value per markup actor from `map_style_technical_design_v1.md` §5.1: `WalkableArea`, `Wall`, `Ledge`, `Drop`, `ClimbEdge`, `JumpGap`, `Hazard`, `TallGrass`, `ShallowWater`, `DeepWater`, `SecretHint`, `Breakable`, `Gate`, `Interact`, `SpawnGroup`, `PatrolPath`, `LootAnchor`, `CameraInterest`, `ReturnShortcut`. All 19 present, `UENUM(BlueprintType)`.

### Markup Actor Base
- [ ] AC-081.2: `AMordecaiGameplayMarker` actor exists in `Source/LyraGame/Mordecai/World/`. Properties: `EMordecaiMarkupType MarkupType` (`EditAnywhere, BlueprintReadOnly`), `FName OwningCellId` (which cell this marker belongs to), `FName MarkerId` (unique within the cell), `FGameplayTagContainer MarkerTags`. Root is a `USceneComponent`. The actor `bReplicates = false` by default (markup is authoring/validation data, not gameplay-replicated state) — note this is intentional in a comment.
- [ ] AC-081.3: `AMordecaiGameplayMarker::GetMarkupType()` returns the type. The base actor is concrete (placeable) and carries no per-type behavior — type-specific data lives in lightweight subclasses or in `MarkerTags`. At minimum, provide a `AMordecaiGateMarker` subclass adding `EMordecaiGateRequirement GateRequirement` (so "report all gates by requirement type" is answerable) and a `AMordecaiSpawnGroupMarker` subclass adding `int32 MaxConcurrent` and `FGameplayTag EnemyArchetypeTag`.
- [ ] AC-081.4: Markers expose a `bool IsTraversalMarker()` helper that returns true for the traversal-relevant types (`ClimbEdge`, `JumpGap`, `Drop`, `DeepWater`, `ShallowWater`, `Ledge`) — used by validators and (later) movement skills.

### Cell Markup Registry (WorldSubsystem)
- [ ] AC-081.5: `UMordecaiCellMarkupSubsystem` (a `UWorldSubsystem`) exists. Markers register themselves with it on `BeginPlay` and unregister on `EndPlay` (keyed by `OwningCellId`). If a marker has no `OwningCellId`, it registers under a `NAME_None` bucket and the subsystem logs a warning.
- [ ] AC-081.6: `GetMarkersInCell(FName CellId)` returns all registered markers for a cell. `GetMarkersOfType(FName CellId, EMordecaiMarkupType)` filters by type. `GetGatesByRequirement(EMordecaiGateRequirement)` returns every `AMordecaiGateMarker` across all cells matching that requirement (answers the §6 "report all gates by requirement type" query).
- [ ] AC-081.7: `CellHasMarkupType(FName CellId, EMordecaiMarkupType)` returns bool — the primitive behind validation queries like "which arenas have no spawn group?". `GetCellIdsMissingType(EMordecaiMarkupType, const TArray<FName>& CandidateCells)` returns the subset of candidate cells lacking that markup type.

### Robustness
- [ ] AC-081.8: Registering the same marker twice is idempotent (no duplicates). Querying an unknown cell returns an empty array, never null. Unregistering a marker not present is a safe no-op. Destroying a registered marker (EndPlay) removes it from all query results.

## Technical Notes
- Place code in `Source/LyraGame/Mordecai/World/`. Files: `MordecaiMarkupTypes.h`, `MordecaiGameplayMarker.h/.cpp`, `MordecaiGateMarker.h/.cpp`, `MordecaiSpawnGroupMarker.h/.cpp`, `MordecaiCellMarkupSubsystem.h/.cpp`.
- **Markup is authoring data, not replicated gameplay state.** Markers are spawned in the level (later, in EDITOR cell stories) and read by validators/tools. Keep them cheap — no Tick, no replication.
- **Headless testing:** Create a scratch `UWorld` (the existing combat/hit-detection tests already do this — follow `MordecaiHitDetectionTests` for the world-setup boilerplate), spawn markers via `SpawnActor`, drive `BeginPlay`/registration, and assert subsystem query results. All ACs are verifiable without rendering.
- **Subsystem lifecycle:** Markers register in `BeginPlay`. In tests that spawn actors into a minimal world, ensure `BeginPlay` is dispatched (or expose an explicit `RegisterMarker`/`UnregisterMarker` API the actor calls and tests can call directly — preferred, since it decouples the registration logic from actor lifecycle and is cleaner to unit-test).
- **Do not** create per-type subclasses for all 19 markup types — that's over-engineering. Only `Gate` and `SpawnGroup` carry enough type-specific data to warrant subclasses this story; the rest are differentiated by `MarkupType` + `MarkerTags`. Add more subclasses later only when a type needs structured fields.
- **No editor placement / level work** in this story. Placing markers in actual cells is EDITOR work, blocked on World Intent approval (see US-080 Blockers).

## Tests Required
- [ ] `Mordecai.World.Markup.AllNineteenTypesPresent` — `EMordecaiMarkupType` enumerates all 19 documented markup types (AC-081.1)
- [ ] `Mordecai.World.Markup.MarkerReportsTypeAndCell` — a spawned marker returns its `MarkupType` and `OwningCellId` (AC-081.2, AC-081.3)
- [ ] `Mordecai.World.Markup.GateMarkerCarriesRequirement` — `AMordecaiGateMarker` stores and returns its `GateRequirement` (AC-081.3)
- [ ] `Mordecai.World.Markup.IsTraversalMarker` — ClimbEdge/JumpGap/Drop/water/Ledge return true; Wall/Gate/Interact return false (AC-081.4)
- [ ] `Mordecai.World.Markup.RegisterAndQueryByCell` — markers registered under a cell are returned by `GetMarkersInCell`; a different cell's markers are excluded (AC-081.5, AC-081.6)
- [ ] `Mordecai.World.Markup.QueryByType` — `GetMarkersOfType` returns only matching-type markers in the cell (AC-081.6)
- [ ] `Mordecai.World.Markup.GatesByRequirementAcrossCells` — gate markers in different cells with the same requirement are all returned by `GetGatesByRequirement` (AC-081.6)
- [ ] `Mordecai.World.Markup.CellHasMarkupType` — true when present, false when absent (AC-081.7)
- [ ] `Mordecai.World.Markup.CellsMissingType` — `GetCellIdsMissingType(SpawnGroup, {A,B,C})` returns exactly the cells without a SpawnGroup marker (AC-081.7)
- [ ] `Mordecai.World.Markup.RegistrationIsIdempotent` — registering the same marker twice yields one entry (AC-081.8)
- [ ] `Mordecai.World.Markup.UnregisterRemovesFromQueries` — unregistering/destroying a marker removes it from all query results; unknown-cell query returns empty array (AC-081.8)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] All Mordecai tests still pass (no regressions)
- [ ] PIE smoke test passes (existing suite — confirm markup actors + subsystem compile into the build without runtime errors)
- [ ] Code committed and pushed with `[US-081]` prefix
