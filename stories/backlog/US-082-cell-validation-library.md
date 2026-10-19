# US-082: Cell Framework — Validation Library & Report

## Overview
Build the **cell validation pass** — the deterministic checker that inspects a `UMordecaiCellMetadata` (US-080) plus its registered gameplay markup (US-081) and reports defects: missing metadata fields, broken exit connectivity, gate-requirement mismatches, missing landmarks, secrets without hints, combat cells without spawn groups, and more. This completes the testable core of **Milestone 1 (Cell Framework)** and is the foundation of the agentic review loop: per `agentic_map_pipeline_v1.md` §7, validators "produce deterministic feedback" so fix agents resolve "only flagged problems."

Per `agentic_map_pipeline_v1.md` §5, the validation rules are explicitly prioritized (Critical / High / Medium). This story implements that ruleset as pure C++ functions returning a structured report — no editor dependency, fully headless-testable by constructing cells with known defects.

Depends on US-080 (metadata + types) and US-081 (markup registry, so spawn-group / gate / secret-hint checks can query markers).

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `agentic_map_pipeline_v1.md` §5 (validation rules — Critical/High/Medium priority order), §7 (review loop), §6 (report generation)
- Design doc: `map_style_technical_design_v1.md` §10.4 (validation checklist automation — the canonical defect list), §13 (cell definition-of-done)
- Depends on: US-080 (`UMordecaiCellMetadata`, world enums), US-081 (`UMordecaiCellMarkupSubsystem`, markup queries)
- Pattern reference: existing static `UBlueprintFunctionLibrary` helpers in the codebase (e.g. `UMordecaiItemLibrary`) for the pure-function-returning-struct style

---

## Acceptance Criteria

### Report Data Model
- [ ] AC-082.1: `EMordecaiValidationSeverity` enum exists: `Critical`, `High`, `Medium`, `Info`. `FMordecaiValidationIssue` USTRUCT holds: `EMordecaiValidationSeverity Severity`, `FName CheckId` (stable id, e.g. `"ExitConnectivity"`), `FName CellId`, `FText Message` (human-readable). `FMordecaiCellValidationReport` USTRUCT holds `FName CellId`, `TArray<FMordecaiValidationIssue> Issues`, and helpers `bool HasCritical()`, `bool PassedClean()` (no Critical or High), `int32 CountBySeverity(EMordecaiValidationSeverity)`.

### Metadata Completeness (Critical/High)
- [ ] AC-082.2: `UMordecaiCellValidationLibrary::ValidateMetadata(const UMordecaiCellMetadata*)` flags as **Critical**: empty `CellId`, `BiomeId == None`, empty `HeightTiersUsed`, and zero `Exits` for non-`DeadEnd` cells. Returns a report (never asserts/crashes; a null asset yields a single Critical "NullCell" issue).
- [ ] AC-082.3: Flags as **High**: a `Gate`-type cell with an empty `GateRequirements` array; a `Secret`-type cell (or `bContainsSecret == true`) — these feed downstream markup checks (AC-082.6); a `LandmarkType` that is `NAME_None` on cell types that require a landmark (`Vista`, `Service`, `DungeonEntrance` — per §10.4 "missing landmark in required cell types").

### Exit Connectivity (Critical)
- [ ] AC-082.4: `ValidateConnectivity(const TArray<UMordecaiCellMetadata*>& Cells)` checks the cell graph: every `FMordecaiCellExit` whose `TargetCellId` is set must resolve to a cell present in the set (else **Critical** "ExitConnectivity"); and for non-`bOneWay` exits, the target cell must have a reciprocal exit pointing back (an exit whose `TargetCellId` equals this cell and whose `Direction` is the opposite, via `UMordecaiWorldLibrary::GetOpposite`) — missing reciprocal is **High** "OneWayMismatch". An exit with `TargetCellId == NAME_None` is treated as an intentional open boundary (no issue).
- [ ] AC-082.5: A gate on an exit (`GateRequirement != None`) must also appear in the owning cell's `GateRequirements` metadata array, and vice-versa — mismatch is **High** "GateRequirementMismatch" (per §10.4 "gate requirement mismatch").

### Markup-Backed Checks (require US-081 registry)
- [ ] AC-082.6: `ValidateMarkup(const UMordecaiCellMetadata*, UMordecaiCellMarkupSubsystem*)` flags: a cell with `bContainsCombat == true` but no `SpawnGroup` markup (**High** "CombatNoSpawnGroup"); a cell with `bContainsSecret == true` but no `SecretHint` markup (**High** "SecretNoHint", per §10.4 "secret cell with no hint marker"); a metadata `GateRequirements` entry with no corresponding `Gate` markup actor of that requirement in the cell (**High** "GateNoMarker"); a `ReturnShortcut`-declared exit (or `ShortcutReturn` cell type) with no `ReturnShortcut` markup (**Medium** "ShortcutNotInstantiated", per §10.4 "return shortcut declared but not instantiated").
- [ ] AC-082.7: When the markup subsystem is null, `ValidateMarkup` returns a single `Info` issue ("MarkupUnavailable") rather than crashing — so metadata-only validation still works in contexts without a world.

### Aggregate Entry Point
- [ ] AC-082.8: `ValidateCell(const UMordecaiCellMetadata*, UMordecaiCellMarkupSubsystem*)` runs metadata + markup checks for a single cell and returns a merged `FMordecaiCellValidationReport`. `ValidateWorld(const TArray<UMordecaiCellMetadata*>&, UMordecaiCellMarkupSubsystem*)` runs per-cell checks plus the cross-cell connectivity pass and returns one report per cell (`TArray<FMordecaiCellValidationReport>`). Issues are ordered by descending severity within each report.

## Technical Notes
- Place code in `Source/LyraGame/Mordecai/World/`. Files: `MordecaiValidationTypes.h`, `MordecaiCellValidationLibrary.h/.cpp` (a `UBlueprintFunctionLibrary`).
- **Pure functions over data.** No ticking, no editor module, no commandlet in this story. The commandlet/editor-utility wrapper that *runs* these checks across content and emits CSV/JSON (`agentic_map_pipeline_v1.md` §6) is a later EDITOR story — this story delivers the reusable check logic those tools will call.
- **Headless testing:** Construct `UMordecaiCellMetadata` objects via `NewObject`, populate fields to create specific defects, and assert the report contains the expected `CheckId` at the expected `Severity`. For markup-backed checks, reuse the US-081 scratch-world + marker-registration pattern.
- **Severity discipline:** Map each check to the priority in `agentic_map_pipeline_v1.md` §5 — connectivity/gate-tag/spawn-in-collision are Critical-tier; landmark/secret-hint/path-width are High; naming/dressing are Medium. Where §5 and §10.4 overlap, follow §5's priority labels (cited per-AC above).
- **Out of scope for this story** (require rendering or geometry the framework doesn't yet model, so defer with `TODO(DECISION)` notes): camera-readability checks ("hidden interactables from gameplay camera"), combat-pocket movement-width checks, overlapping-blocker geometry checks, blocked-spawn-inside-collision. List them as unimplemented checks in a header comment so a later story picks them up.
- Naming-convention validation (`Cell_<Abbrev>_<NN>`) is in scope as a **Medium** check if cheap (use `UMordecaiWorldLibrary::MakeCellId` to compare), but keep it optional — do not block on it.

## Tests Required
- [ ] `Mordecai.World.Validation.NullCellYieldsCritical` — `ValidateMetadata(nullptr)` returns one Critical "NullCell" issue, no crash (AC-082.2)
- [ ] `Mordecai.World.Validation.MissingMetadataFieldsCritical` — empty CellId / None biome / empty HeightTiers / zero exits on a non-DeadEnd cell each produce a Critical issue (AC-082.2)
- [ ] `Mordecai.World.Validation.GateCellWithoutRequirementHigh` — a Gate-type cell with empty GateRequirements flags High (AC-082.3)
- [ ] `Mordecai.World.Validation.MissingLandmarkHigh` — a Vista cell with `LandmarkType == NAME_None` flags High; a Path cell does not (AC-082.3)
- [ ] `Mordecai.World.Validation.BrokenExitLinkCritical` — an exit targeting a cell absent from the set flags Critical "ExitConnectivity" (AC-082.4)
- [ ] `Mordecai.World.Validation.OneWayMismatchHigh` — a two-way exit with no reciprocal exit on the target flags High "OneWayMismatch"; a properly reciprocated pair passes (AC-082.4)
- [ ] `Mordecai.World.Validation.GateRequirementMismatchHigh` — an exit gate not listed in the cell's GateRequirements (or vice-versa) flags High (AC-082.5)
- [ ] `Mordecai.World.Validation.CombatCellWithoutSpawnGroup` — `bContainsCombat` true + no SpawnGroup marker flags High "CombatNoSpawnGroup" (AC-082.6)
- [ ] `Mordecai.World.Validation.SecretCellWithoutHint` — `bContainsSecret` true + no SecretHint marker flags High "SecretNoHint" (AC-082.6)
- [ ] `Mordecai.World.Validation.GateDeclaredButNoMarker` — GateRequirements lists Explosives but no Explosives Gate marker exists in the cell flags High "GateNoMarker" (AC-082.6)
- [ ] `Mordecai.World.Validation.MarkupUnavailableIsInfoNotCrash` — `ValidateMarkup` with null subsystem returns one Info issue and does not crash (AC-082.7)
- [ ] `Mordecai.World.Validation.CleanCellPassesClean` — a fully-populated, well-connected cell with all required markup produces a report where `PassedClean()` is true (AC-082.1, AC-082.8)
- [ ] `Mordecai.World.Validation.ReportOrderedBySeverity` — a cell with mixed issues returns them ordered Critical→High→Medium→Info (AC-082.8)
- [ ] `Mordecai.World.Validation.ValidateWorldReturnsPerCellReports` — `ValidateWorld` over N cells returns N reports and runs the cross-cell connectivity pass (AC-082.8)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] All Mordecai tests still pass (no regressions)
- [ ] PIE smoke test passes (existing suite — confirm validation library compiles into the build without runtime errors)
- [ ] Code committed and pushed with `[US-082]` prefix
