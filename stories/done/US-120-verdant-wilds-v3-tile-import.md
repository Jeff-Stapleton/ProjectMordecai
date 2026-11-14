# US-120: Import and Prove the Verdant Wilds v3 Terrain Tile Kit

**Epic:** Epic 8: World & Exploration
**Priority:** P1
**Depends on:** US-100
**Paired story:** none

## Overview
Import the approved Verdant Wilds v3 terrain kit into MordecaiCore as deterministic, cooked Unreal content. Add an isolated packaged test map that proves all 17 ready tiles load and render, and that their ramps, cliffs, shallow pond banks, and deep pond banks behave correctly under real player input.

## Execution Mode
**EDITOR**
This story imports static meshes and textures, creates materials and material instances, and creates/saves a test map. C++ is acceptance-test code only, following the EDITOR-plus-content-test precedent of US-100 (`stories/done/US-100-recovery-playtest-test-maps.md:10-13`).

## References
- `catalog/catalog.json` in the private B2 bucket `mordecai-assets` (17 entries with `status: ready`, `biomes/verdant_wilds/tiles/v3/` plus `_shared/textures/`): ids, files with sha1/sha256, `bounds_cm`, `pivot`, `footprint_tiles`, `collision.count`, `materials`, `texture_set`, `units`
- `Specs/agent_rules_v2.md:120-228` — verification ladder, cooked content, committed content
- `Specs/agent_rules_v2.md:280-348` — world source-of-truth hierarchy and naming/folder rules
- `Specs/map_style_technical_design_v1.md:59-70,114-136,235-275` — collision readability, height tiers, gameplay shape versus art
- `Specs/agentic_map_pipeline_v1.md:60-121,165-205` — blockout, validation, local automation
- `Specs/control_bindings_v1.1.md:8-23` — WASD and Space
- Existing import precedent: `Scripts/us103_create_foundation_assets.py:100-115,300-426`
- Existing test-map precedent: `Scripts/us100_02_create_test_maps.py:1-24`
- Cook-list contract: `Scripts/package_playtest.py:104-119`; `Scripts/tests/test_package_playtest.py:47-94`

## Acceptance Criteria
- [x] AC-120.1: Exactly the 17 catalog entries whose status is `ready` are imported as `UStaticMesh` assets under `/MordecaiCore/World/Terrain/VerdantWilds/Meshes`; no non-ready or unexpected catalog mesh is imported. Verified by `Mordecai.World.Terrain.VerdantWilds.AllReadyMeshesLoad` and the committed source manifest.
- [x] AC-120.2: Every mesh imports at catalog scale and orientation with its catalog pivot and bounds: imported bounds match manifest bounds within 1 cm, actors require scale `(1,1,1)`, normal tile origins occupy the declared bottom-centre grid point, and ramp origins follow the declared cliff-row/grid-line contract. Verified by `Mordecai.World.Terrain.VerdantWilds.ImportContract`.
- [x] AC-120.3: Every mesh has Nanite enabled and its catalog UCX simple collision preserved; complex-as-simple is forbidden and hull counts match the catalog. Every convex collision vertex stays inside the manifest gameplay footprint. Pond footprints include the adjacent water tile/basin: edge XY is X -100..100 and Y -200..200 (plus 15 cm tolerance), corner XY is within the 2x2 tiles around the grid vertex, and pond Z is -200..+15 except intentional deep-bank blocking walls may reach Z 250. A deep blocking wall may not intrude more than 15 cm into the land side. Visual overhangs beyond 15 cm remain reported art defects. Verified by `Mordecai.World.Terrain.VerdantWilds.NaniteAndSimpleCollision` and `.CollisionInsideFootprint`.
- [x] AC-120.4: The ready shared textures, one saved master material, and all manifest-required `MI_VW_*` material instances exist at the approved terrain paths. D/DA maps use sRGB; N/ORM/H maps are linear with their intended compression; every mesh slot resolves by exact catalog slot name. Verified by `Mordecai.World.Terrain.VerdantWilds.MaterialsAndTexturesLoad`.
- [x] AC-120.5: `/MordecaiCore/Game/Maps/L_VW_TileTest` is a saved non-world-graph test fixture using `AMordecaiGameMode`. It contains all 17 meshes as labelled, unit-scale actors on the 200 cm grid and dedicated lanes for Ramp 2T, Ramp 3T, a 200 cm cliff, shallow pond crossing, and deep pond blocking. No supplemental collision volume may mask the collision being tested. Verified by `Mordecai.World.Terrain.VerdantWilds.TileTestMapContract` and `Scripts/verify_pie_vw_tiles.py`.
- [x] AC-120.6: In a fresh packaged build, real WASD input walks the player from Tier 0 to Tier 1 over both ramps and back down; each successful ascent raises the player approximately 200 cm without jumping or becoming stuck. Verified by Tier 4 checks `ramp_2t_up`, `ramp_2t_down`, `ramp_3t_up`, and `ramp_3t_down` in `Scripts/playtests/us120_vw_tiles.py`.
- [x] AC-120.7: In that same package, walking into and jumping toward the 200 cm cliff never places the player on Tier 1; the shallow pond lane is traversable end to end; and repeated walking into each deep-bank representative does not cross its bank. Verified by Tier 4 checks `cliff_walk_blocked`, `cliff_jump_blocked`, `shallow_pond_crosses`, and `deep_bank_blocks`.
- [x] AC-120.8: The new map is in `Config/DefaultGame.ini` `+MapsToCook` and `Scripts/package_playtest.py` `DEFAULT_MAPS`; `py Scripts/playtest_story.py US-120` passes Tier 3 and Tier 4 on one fresh clean package, every screenshot is checked by script (PIL/analyze_image, never the file-read tool) and bundled for coordinator review, and the full `py Scripts/playtest_story.py --all` sweep has zero baseline PASS→FAIL.
- [x] AC-120.9: Raw FBX and texture source files and credentials are absent from Git; the committed manifest contains the logical B2 keys, ready status, hashes, bounds, footprints, hull counts, slots, and texture references needed for deterministic reimport. Imported `.uasset`/`.umap` files are saved and committed through Git LFS, and `py Scripts/check_content_committed.py` exits 0.
- [x] AC-120.10: Completion Notes record the pre/post packaged `.pak` byte totals, cooked terrain-folder size, mesh/texture/material counts, and delta. No size threshold is inferred without a coordinator decision.

## Technical Notes
- Canonical paths:
  - `/MordecaiCore/World/Terrain/Shared/Materials/M_Terrain_Master`
  - `/MordecaiCore/World/Terrain/VerdantWilds/Meshes/SM_*`
  - `/MordecaiCore/World/Terrain/VerdantWilds/Materials/MI_*`
  - `/MordecaiCore/World/Terrain/VerdantWilds/Textures/T_*`
  - `/MordecaiCore/Game/Maps/L_VW_TileTest`
- Keep B2 downloads and credentials outside Git under `%TEMP%`. The Unreal script receives a staged source directory and a committed allowlist manifest; it must never print tokens.
- Run the importer unattended through `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=<absolute us120 script> -unattended -nop4 -nosplash`. Confirm the exact UE 5.7 API/property names against Jeff's source build before GREEN.
- FBX import: static mesh, unit scale 1.0, scene/unit conversion enabled, combine off, material/texture import off, automatic collision off, named UCX retained, one-convex-hull-per-UCX. Enable Nanite explicitly and read back every setting.
- Import D/DA textures as sRGB and N/ORM/H as linear. Assign MI instances by exact material-slot name and fail on unknown or missing slots.
- The importer is idempotent, saves every package, prints PASS/FAIL plus a final result, and fails if touched packages remain dirty. Existing save/import requirements are documented at `.claude/skills/unreal-python-editor/SKILL.md:30-72`.
- `L_VW_TileTest` is a test fixture, not a world cell: it does not alter the biome graph, progression, gate vocabulary, or production cell pipeline. The US-100 fixture exemption is at `stories/done/US-100-recovery-playtest-test-maps.md:182-186`.
- Use plain blockout support beneath the samples; production flat-ground painting/landscape is out of scope.
- Do not change character movement values to make a tile pass. The capsule and movement constants are pinned by `Source/LyraGame/Character/LyraCharacter.cpp:39-43` and `Source/LyraGame/Mordecai/MordecaiCharacter.cpp:100-117`.
- Deep-bank tests must exercise the imported UCX wall, not an added blocking volume.
- Pit pieces are imported and displayed but are fenced off from the player route. Last-safe-ground pit recovery does not exist; current respawn returns to PlayerStart (`Source/LyraGame/Mordecai/MordecaiGameMode.cpp:87-116`).
- No PIE-only paths: map experience resolution must work in the package.

### Decisions (coordinator, 2026-11-13)
- D1 Raw-source retention: B2 `mordecai-assets` stays authoritative. Git stores the ready-only manifest (copied from the B2 catalog with its hashes), the importer, and the imported Unreal assets. No raw FBX/PNG source in Git.
- D2 Material contract: one opaque Default Lit master for US-120 with BaseColor, Normal, ORM, optional Height, tint, roughness and normal-strength parameters; pond water is opaque stylized felt for this story. No RVT or world-aligned texturing in this story. The final look belongs to Art Bot and is a later story.
- D3 Footprints: derive from the catalog's `footprint_tiles`, `pivot`, `run_tiles`/`width_cm` and `bounds_cm`, recorded in the committed manifest as integer tile extents plus pivot-grid offset. For 1x1 cliff and pit edge pieces, the local origin is the midpoint of the authored grid edge between tiles, the body/high-or-land side is local +Y, and the collision footprint is X -100..100, Y 0..200, with Z from the piece. Their corners use the high/land-side tile quadrant(s) implied by their bounds. Ramps use the catalog ramp pivot and `run_tiles`/`width_cm`. Measure every piece's visual overhang into local -Y; overhang beyond 15 cm is a reported per-piece art defect, not a reason to stop or edit the mesh, and the sub-check remains a known issue tied to the measured value. If any other ready entry cannot be resolved unambiguously from catalog data, stop that AC and report it rather than guessing.
- D3 pond amendment (coordinator, 2026-11-13 18:46 MT): the catalog's pond bank and bed intentionally extend into the adjacent water tile; that basin is not a lip, and deep pieces intentionally carry an above-capsule-height blocking-wall UCX. Pond edge collision therefore allows X -100..100 plus 15 cm tolerance, Y -200..200, and Z down to -200; pond corners allow X/Y -200..200. Shallow hulls may not rise above +15 except for the land body. Deep blocking walls may rise to Z 250 but may not intrude more than 15 cm into the land side (local Y >15 for edges; the land quadrant for corners), preventing along-bank snags. Record every pond hull's measured extents in the import report. Shallow traversal and deep-bank rejection remain Tier 4 behavioral gates.
- D3 fixture orientation: the camera yaw is 0, so screen-up/north is Unreal +X and screen-right/east is Unreal +Y. Lay out `L_VW_TileTest` lanes in that camera-space convention and report the imported FBX face/downhill local axis for one straight cliff and the 2T ramp.
- D3 fixture correction (coordinator review, 2026-11-14): the original flat fixture floor covered the pond basins and invalidated the first shallow/deep behavioral result. The corrected map cuts four 3x3 floor holes, uses a collision bed at Z -38 for shallow and Z -141 for deep, adds non-colliding water at Z -8, and provides land support only on each authored bank's land side. Shallow acceptance requires a real Z dip and far-bank exit; deep Edge and corners are approached perpendicular to the face (corners along the arc radius) and must remain within 15 cm plus capsule radius past that face. The earlier curved-bank defect claim is withdrawn because it was produced by walking on the covering floor.
- D4 Package size: measure and report only (AC-120.10). No threshold in this story.
- D5 Pit tiles are imported as geometry only; last-safe-ground pit recovery is NOT part of US-120 and no AC may claim it.

## Tests Required
### RED / source-manifest checks
- `Scripts/tests/test_us120_source_manifest.py`
  - exact 17 ready mesh IDs
  - no non-ready entry
  - unique B2 key and checksum
  - complete expected bounds, footprint, hull count, slots, and texture references
- `Scripts/tests/test_package_playtest.py`
  - `L_VW_TileTest` is in both explicit cook lists and is the map used by `us120_vw_tiles.py`
- `Scripts/tests/test_us120_playtest.py`
  - PLAN/check mapping, timeout failures, and screenshot declarations against `Scripts/tests/fake_bridge.py`; fake physics is not acceptance

### Tier 1 — Automation
- `Mordecai.World.Terrain.VerdantWilds.AllReadyMeshesLoad` — AC-120.1
- `Mordecai.World.Terrain.VerdantWilds.ImportContract` — AC-120.2
- `Mordecai.World.Terrain.VerdantWilds.NaniteAndSimpleCollision` — AC-120.3
- `Mordecai.World.Terrain.VerdantWilds.CollisionInsideFootprint` — AC-120.3
- `Mordecai.World.Terrain.VerdantWilds.MaterialsAndTexturesLoad` — AC-120.4
- `Mordecai.World.Terrain.VerdantWilds.TileTestMapContract` — AC-120.5
- Run all `Mordecai.*` tests with `py Scripts/run_tests.py`.

### Tier 2 — PIE smoke
- `py Scripts/run_pie_verify.py --map /MordecaiCore/Game/Maps/L_VW_TileTest --tests spawn,hud,vw_tiles`
- `Scripts/verify_pie_vw_tiles.py` checks map/game mode, all 17 labelled actors, exact mesh assignments, unit scale, 200 cm placement, Nanite, simple collision, material slots, and absence of supplemental blockers. It reads state only and is not player-input acceptance.

### Tier 3 — Packaged playtest
- See Playtest.

### Tier 4 — Real-input playtest
- `Scripts/playtests/us120_vw_tiles.py`, a `PlaytestRun` on `L_VW_TileTest`
- Real bridge key/axis input only; no teleport, console, `debug_*`, direct gameplay calls, or route override
- Checks: `ramp_2t_up`, `ramp_2t_down`, `ramp_3t_up`, `ramp_3t_down`, `cliff_walk_blocked`, `cliff_jump_blocked`, `shallow_pond_crosses`, `deep_bank_blocks`
- Screenshot the complete tile gallery, both ramp tops, cliff rejection, shallow crossing, and deep-bank rejection

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/L_VW_TileTest --expect-experience B_MordecaiArenaExperience`; Tier 4 runs through `py Scripts/playtest_story.py US-120` on the same package.
- **Player steps:** use WASD to visit the labelled tile groups; walk up and down both ramps without Space; hold movement against the cliff; press Space while moving toward the cliff; walk across the shallow pond lane; hold movement into each deep-bank representative.
- **Visible on screen:** all 17 approved meshes appear with their intended Verdant Wilds materials; both ramp ascents show the player on the 200 cm upper tier; cliff attempts leave the player below; the shallow lane shows exposed bank/bed/water while the player wades below land height and exits; each deep pond shows its bank/bed/water while the player remains on the land side.
- **Proof:** Tier 3 proves the map/assets/materials are cooked and visible. Tier 4 proves player traversal and blocking through real input. Tier 1 and Tier 2 prove exact asset/map contracts but do not replace Tier 4.
- **Tier 4 script:** `Scripts/playtests/us120_vw_tiles.py`; run `py Scripts/playtest_story.py US-120`; `--lint` must exit 0.
- **Steps:**

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `tile_gallery_visible` | walk through the catalogue viewpoints and capture each tile group | AC-120.1, AC-120.4, AC-120.5 |
| 2 | `ramp_2t_up` | walk from the 2T base to its top without Space → Z rises about 200 cm | AC-120.6 |
| 3 | `ramp_2t_down` | reverse across the 2T ramp → returns to Tier 0 without sticking | AC-120.6 |
| 4 | `ramp_3t_up` | walk from the 3T base to its top without Space → Z rises about 200 cm | AC-120.6 |
| 5 | `ramp_3t_down` | reverse across the 3T ramp → returns to Tier 0 without sticking | AC-120.6 |
| 6 | `cliff_walk_blocked` | hold movement into the 200 cm cliff → remains below and on the near side | AC-120.7 |
| 7 | `cliff_jump_blocked` | press Space while moving into the cliff → never reaches Tier 1 | AC-120.7 |
| 8 | `shallow_pond_crosses` | hold W through the real shallow basin → Z dips toward bed-plus-capsule height, then returns to land height past the far bank | AC-120.7 |
| 9 | `deep_bank_blocks` | approach deep Edge perpendicular to its face and both corners along the arc radius → never advances more than face + 15 cm + capsule radius and remains near land Z | AC-120.7 |

- **Playable alone:** YES

## Scope Fence
### In
- The 17 ready v3 meshes
- Ready shared textures referenced by those meshes
- One master terrain material and required MI instances
- Deterministic importer and source manifest
- `L_VW_TileTest`
- Tier 1, Python, Tier 2, Tier 3, Tier 4, and sweep coverage
- Package-size measurement

### Out
- Props, foliage, scatter, or any catalog item still in review
- Production flat-ground painting or landscape
- Production map cells, biome graph, encounters, gates, and progression
- Pit last-safe-ground recovery and HP penalty; file as a separate gameplay follow-up
- Manual LODs
- Water simulation, swimming, buoyancy, or translucent-water rendering
- Changes to global capsule, jump, step-height, slope, speed, or camera tuning

## Budget
- Text/code: approximately 8 new and 4 modified files, 2,200–2,800 lines:
  - manifest, importer, map creator, C++ content tests, PIE verifier, Tier 4 script, two offline test files
  - small cook-list/config/playtest constant changes
- Binary content:
  - 17 `UStaticMesh` assets
  - manifest-ready texture set
  - 1 master material plus required `MI_VW_*` instances
  - 1 `.umap`
- Keep commits focused: RED tests/scaffold, GREEN imported content/scripts, then story-only evidence closure.

## Definition of Done
- [x] Decisions D1-D5 applied
- [x] Tests are written first and fail for the expected missing-content/cook-list reasons
- [x] All ACs checked off
- [x] Tier 1: project compiles and all `Mordecai.*` tests pass
- [x] Python tooling tests pass
- [x] Tier 2 PIE smoke passes on `L_VW_TileTest`
- [x] Every imported `.uasset`/`.umap` is saved and committed; no raw source/token is committed; `py Scripts/check_content_committed.py` exits 0
- [x] Work is committed before packaging; `py Scripts/package_playtest.py` exits 0 on that exact clean commit
- [x] `py Scripts/playtest_story.py US-120` exits 0 without `--allow-stale`
- [x] Every Tier 3 and Tier 4 screenshot is checked by script (PIL/analyze_image only; never open PNGs with the file-read tool) against the Playtest expectations and placed in the evidence bundle for coordinator visual review
- [x] Full `py Scripts/playtest_story.py --all` sweep has zero baseline PASS→FAIL
- [x] Completion Notes record run dirs, scripted screenshot checks, imported counts, and package-size deltas
- [x] Closing commit touches only `stories/`; story moves to done only after all evidence passes

## Completion Notes
### Result and package
- Story complete. Playable status: movement — PLAYABLE (input) (Tier 4
  `Saved/Playtests/20261114-023851-us120/report.md` + Tier 3
  `Saved/Playtests/20261114-023828-boot/report.md` at package commit
  `903337a0ea2ec04ba28fa91862b1983a469fdc30`).
- RED commits: `a2659daa` (original missing-content/cook-list contract) and
  `36a40349` (r2 real-pond fixture, shallow-Z, and radial-corner contracts).
- Import report: `Saved/Playtests/us120-vw-import-report.json`, result PASS; staged-source
  verification checked SHA-1 and SHA-256 for all 51 allowlisted files.
- Imported and committed through Git LFS: 17 static meshes, 34 textures, one
  `M_Terrain_Master`, four `MI_VW_*` terrain instances, `L_VW_TileTest`, and the
  fixture-only blue water material.
- Imported facing is local `-Y` for both `SM_VW_FeltCliff_Straight_01`'s face and
  `SM_VW_FeltCliff_Ramp_2T_01`'s downhill direction. Actors and mesh build scale are
  `(1,1,1)`. Screen north/east are Unreal `+X`/`+Y`.
- Fresh package metadata: `PackagedBuilds/Playtest/build_info.json`; commit
  `903337a0ea`, `dirty=false`, `source_changed_during_build=false`.

### Verification
- Tier 1 build PASS; `py Scripts/run_tests.py`: **849/849 PASS**, including
  `AllReadyMeshesLoad`, `ImportContract`, `NaniteAndSimpleCollision`,
  `CollisionInsideFootprint`, `MaterialsAndTexturesLoad`, and `TileTestMapContract`.
- Python tooling: **685/685 PASS**. Tier 2 PIE on `L_VW_TileTest`: `spawn`
  **69/69**, `hud` **12/12**, and `vw_tiles` **293/293 PASS**.
- Content guard PASS after commit; Git LFS hydration check printed no pointer-only
  entries. No raw source or B2 authorization string was committed or copied to evidence.
- Tier 3 focused run `Saved/Playtests/20261114-023828-boot`: PASS on the fresh
  package (correct map/experience, 3/3 engine frames, content cooked, zero log errors).
- Tier 4 focused run `Saved/Playtests/20261114-023851-us120`: **27 PASS, 0 FAIL**.
  `tile_gallery_visible`, `ramp_2t_up`, `ramp_2t_down`, `ramp_3t_up`,
  `ramp_3t_down`, `cliff_walk_blocked`, `cliff_jump_blocked`, and
  `shallow_pond_crosses` all PASS. Shallow state dipped from land Z 92.2 to
  **Z 54.1** over the Z -38 bed and exited at **Z 92.0**. `deep_bank_blocks` PASS:
  Edge progress -16.1 cm (limit 55), OuterCorner -59.5 (limit 55), and InnerCorner
  29.7 (arc-face limit 155), all near land Z.
- Final full sweep: `Saved/Playtests/20261114-024027-sweep`. Overall FAIL remains
  because baseline-existing US-001 facing, US-054 enemy-spacing, and US-078
  previous-weapon checks still fail. Comparison with the reused baseline
  `Saved/Playtests/20261113-163428-sweep` at `5bce1a16` found **0 script, 0 check,
  and 0 Tier-3 PASS→FAIL transitions** across 15 shared scripts, 395 shared checks,
  and four shared Tier-3 launches. US-120 itself passes in the sweep.

### Scripted image review and evidence
- Per the issue constraint, no PNG was opened with the file-read tool. PIL inspected
  **83 baseline + 101 final-sweep + 18 focused = 202 images** for dimensions,
  entropy, channel variation, near-black coverage, edges, and blue/teal water pixels:
  **202 non-uniform PASS, 0 blank/uniform FAIL**. Audited bridge state/checks prove
  traversal; originals are bundled for coordinator visual review.
- Focused Tier 3: `engine_loading.png` is a valid non-uniform loading frame and the
  report identifies `MordecaiLoadingScreenWidget`; `engine_01..03.png` are valid
  non-uniform `L_VW_TileTest` game frames. Desktop `shot_01..03.png` retain the
  report's overlap warnings and are not used as primary evidence.
- Focused Tier 4: `bridge_01..04.png` are valid gallery frames;
  `bridge_05.png`/`bridge_06.png` correspond to the passing 2T/3T top checks;
  `bridge_07.png` to cliff rejection; `bridge_08.png` captures the Z-54.1 shallow
  wade with a scripted teal-water fraction of 7.17%; and `bridge_09..11.png` capture
  passing deep Edge/Outer/Inner rejection with teal-water fractions 13.47%, 15.73%,
  and 10.43%. Thus the pond captures contain visible water regions rather than the
  old floor-covered crown slivers.
- Evidence bundle:
  `C:\Users\jeffd\AppData\Local\Cursor\AgentStores\cursor_agent_stores\bc-2587667d-8302-5d0e-97b8-9cb6a5b288fc\files\us120-r2-903337a0`.
  It contains baseline/final sweep reports and images, focused US-120 images, named
  captures, import report, manifest, build metadata, package sizes, sweep comparison,
  logs, three PIL analyses, secret scan, and `SHA256SUMS.txt` (**364/364 verified**).

### Size and art/collision findings
- Baseline packaged `.pak` total at `5bce1a16`: **74,828,470 bytes**. US-120 package
  total at `903337a0`: **74,793,639 bytes**. Delta: **-34,831 bytes**. UE IoStore
  payloads are separate, so this requested `.pak` measurement is not the asset payload
  size. Cooked `MordecaiCore/Content/World/Terrain` is **79,505,040 bytes** across
  163 files.
- D3 visual overhang measurements beyond 15 cm are recorded without mesh edits:
  Cliff Straight 01 24.9, Straight 02 26.3, Straight 03 25.0, Cliff OuterCorner 22.0,
  Cliff InnerCorner 100.0, EndCap 22.9, Pit Edge 23.3, Pit OuterCorner 20.6,
  Pit InnerCorner 100.0, Ramp 2T 18.0, Ramp 3T 18.0 cm. Pond basin measurements
  (allowed into the water tile by the amended contract) are Edge 79.6,
  OuterCorner 78.0, InnerCorner 103.5, Deep Edge 79.8, Deep OuterCorner 78.0, and
  Deep InnerCorner 103.5 cm. Every pond hull's actual XYZ extents are in the import
  report.
- No collision defect is proven by the corrected fixture: shallow traversal and all
  three deep-bank rejection probes pass. The r1 curved-bank claim is withdrawn because
  those probes walked over the old basin-covering floor. No movement, capsule, step
  height, jump, walkable angle, mesh, or UCX value was changed.
