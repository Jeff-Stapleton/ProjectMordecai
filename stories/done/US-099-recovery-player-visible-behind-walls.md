# US-099: Recovery — The Player Stays Visible Behind Near Walls

**Epic:** Epic R: Playability Recovery
**Priority:** P2
**Depends on:** N21 cutaway decision — resolved by Jeff on 2026-11-17
**Paired story:** none; this story owns the authorized material and DevTestMap wall update

## Overview
The exploratory agent found that at DevTestMap's original F07 evidence point
`(-935, +935)` the near (-X) wall hides everything but the top of the player's head
(N21), and the fixed diorama camera has no cutaway or fade. Under the yaw-0 gameplay
camera this +Y corner is screen south-east, not south-west. In a fixed-camera game the
player must always be able to see their character. Features: camera, movement.

## Execution Mode
**EDITOR**
Jeff selected a dithered wall fade and explicitly authorized the material asset, committed
editor Python script and asset/map changes in this story.

## References
- `Specs/agent_playtest_plan_v1.md` — N21 ("Found by the first exploratory-agent run")
- Exploratory run `Saved/Playtests/20261022-185207-explore/report.md` — `bridge_11.png`: the near wall hides the player at the original F07 +Y corner (the old report called it south-west; at camera yaw 0 it is screen south-east)
- `Specs/map_style_technical_design_v1.md` (height tiers, wall heights, cutaway rules if any), `Specs/game_design_v2.md` (diorama camera), CLAUDE.md "Never implement camera rotation"
- Existing code: `Source/LyraGame/Mordecai/Camera/MordecaiCameraMode_Diorama.*` (fixed pitch -50, yaw 0)
- Lane geometry used by the playtests: lane L1 runs along y = -800 from x = -700 (`Scripts/playtests/us001_movement.py` `walk_lane_start`)
- DevTestMap's arena: walls 200 cm high, inner faces at ±975 (`Scripts/us054_04_setup_arena.py`; `ARENA_WALLS` in `Scripts/tests/fake_bridge.py`). F07 stood against the near (-X) wall at (-935, +935).
- US-100 (issue #46): `us001_movement.py` plays MovementTestMap since then. Its walls are 100 cm high at ±2000, with nothing tall on the camera side, so N21 cannot be reproduced there.

## Acceptance Criteria
- [x] AC-099.1: TODO(DECISION) N21 answered by Jeff and recorded in `Specs/design_decisions_log.md`: (a) fade/hide occluding geometry between camera and player, (b) player silhouette through walls (custom depth / stencil), (c) lower near-side walls by level design (map rule in `map_style_technical_design_v1.md`), or a combination
- [x] AC-099.2: The chosen mechanism is implemented and covered by Tier 1 tests named after it, including a world trace that proves tagged walls behind nearer untagged/stacked geometry fade and then restore
- [x] AC-099.3: In the packaged build on DevTestMap, the player walks (WASD) to the original F07 point at `(-935,+935)`, against the 200 cm near (-X) wall, and the character is visible on screen. Tier 4 requires x <= -915, `Arena_Wall_W` fade <= 0.0001 within 0.5 s, then a real-input return to `(0,0)` and fade >= 0.999 within 0.5 s. The coordinator reviews `f07_corner` and `restored` screenshots from the bundle under the standing no-Read rule.
- [x] AC-099.4: The camera still never rotates: `camera_fixed`, `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py` PASS

## Technical Notes
- **Decision N21 (Jeff, 2026-11-17): option (a).** Geometry that actually blocks the
  camera-to-player line of sight fades. Use a dithered/masked fade over roughly 0.2–0.3 s and
  restore it fully when clear. Floors/ground and the player's tile never fade; enemies and other
  actors are untouched; camera follow and rotation are unchanged.
- **Implementation call:** three visibility traces from the fixed local camera to the capsule
  top/centre/bottom drive `MordecaiOcclusionFade` on tagged `WorldStatic` wall primitives. The
  explicit wall tag and masked material keep floors and non-wall actors out; only current trace
  blockers enter the camera mode's small restoration map.
- `Scripts/playtests/us099_walls.py` is a `PlaytestRun` with `STORY = "US-099"`,
  `MAP = kit.DEFAULT_MAP` and its own `RUN_SUFFIX`. It taps sprint and walks to the
  original F07 evidence spot at `(-935,+935)`, requires x <= -915 and
  `Arena_Wall_W` fade <= 0.0001, captures `f07_corner`, returns to `(0,0)`, requires
  fade >= 0.999, and captures `restored`. The +Y route is used because F07 is the
  original bug location recorded by the story on `main`; the drafted -Y route was
  not that evidence location.
  - Why not us001: since US-100 it plays MovementTestMap, whose walls are 100 cm high at ±2000; a corner there shows the whole character with or without the fix, a false PASS.
  - DevTestMap's Frontliners swarm the spawn and kill a standing player in about 10 s (N14, T1). Walk to the corner first and take the screenshot at once; a death or a shove before the shot is a FAIL to report, not to hide. Do not use `us054_arena.py`: its run stands at the spawn for the AI checks.
  - Alternative, decided when the story starts: add a 200 cm near-side wall segment to MovementTestMap (an EDITOR change to `Scripts/us100_02_create_test_maps.py`, re-run and committed with the map) and put the check in us001 there.
- AC-099.4's `camera_fixed` / `no_camera_rotation_input` stay in `us001_movement.py`.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- Per decision (AC-099.2)
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_spawn.py`: camera unchanged; the occlusion component (if any) present
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us099_walls.py`: F07 position + fade assertion, `f07_corner`
  screenshot, real-input return to centre, restore assertion, and `restored` screenshot
- `Scripts/playtests/us001_movement.py` (MovementTestMap): `camera_fixed`, `no_camera_rotation_input` (AC-099.4)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-099` on the same package (it runs the new DevTestMap script), plus `py Scripts/playtest_story.py US-001` (MovementTestMap) for AC-099.4
- **Player steps:** walk (WASD) to DevTestMap's original F07 point `(-935,+935)` against the 200 cm near (-X) wall, then walk back to `(0,0)`
- **Visible on screen:** `f07_corner` shows the whole character (not just the top of the head) beside the faded west wall; `restored` shows that wall fully restored after the player returns to centre
- **Proof:** Tier 4 checks `walk_f07_corner`, `wall_fades_at_corner`,
  `walk_back_to_centre`, and `wall_restores_clear`, plus the two screenshots;
  `camera_fixed`, `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** the new DevTestMap script (e.g. `Scripts/playtests/us099_walls.py`) — run: `py Scripts/playtest_story.py US-099`
- **Steps:** generated by `py Scripts/playtest_story.py US-099 --plan`; checked by `--lint`

<!-- Scripts/playtests/us099_walls.py -->
| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `walk_f07_corner` | tap `LeftShift`, `walk_to(-935, 0)`, then `walk_to(-935, 935)` with real WASD → within 20 cm, x <= -915, and alive | AC-099.3 |
| 2 | `wall_fades_at_corner` | observe `camera.occluders[Arena_Wall_W].fade <= 0.0001` within 0.5 s, then screenshot `f07_corner` | AC-099.2, AC-099.3 |
| 3 | `walk_back_to_centre` | `walk_to(-935, 0)`, then `walk_to(0, 0)` with real WASD | AC-099.3 |
| 4 | `wall_restores_clear` | observe `camera.occluders[Arena_Wall_W].fade >= 0.999` within 0.5 s, then screenshot `restored` | AC-099.2, AC-099.3 |
- **Playable alone:** YES (after the decision; NO if an EDITOR pair is split off — then "playable after US-XXX")

## Definition of Done
- [x] All ACs checked off
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: every asset (if any) saved and committed; `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [x] `py Scripts/playtest_story.py US-099` on that same fresh package: all fade,
  position, restore, and screenshot checks PASS; `py Scripts/playtest_story.py US-001`
  (MovementTestMap): `camera_fixed`, `no_camera_rotation_input` PASS; no check that
  PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [x] Known issue N21 marked FIXED on DevTestMap in `Specs/agent_playtest_plan_v1.md`
- [x] Every screenshot (engine_NN.png, bridge_NN.png) reviewed by the coordinator under the standing no-Read rule; Completion Notes state what is on screen.
- [x] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-099]` closing commit
- [x] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
- Decision and implementation: Jeff selected N21 option (a) on 2026-11-17. Three
  camera-to-capsule visibility rays re-trace through up to four hits each and fade only tagged,
  non-instanced `WorldStatic` wall meshes through `MordecaiOcclusionFade`. The production
  0.25-second masked dither reaches exact fade 0 and restores on a clear path, target destruction,
  target EndPlay and camera-mode deactivation. Floors, Pawns, enemies, untagged geometry and the
  fixed camera transform are unchanged.
- RED: round-2 commits `88a4eddd` and `98bc6454`; round-3 RED commit `125bc1a1`.
  `us099-r3-red.log` proves `-MordecaiMarketingCapture` incorrectly froze AI before the fix;
  `us099-r3-lifecycle-red.log` proves destroying the target left the wall at fade 0 before the
  destruction binding was added. The retained negative control removes `UpdateOcclusionFades`;
  Tier 4 then reaches F07 alive and fails because `Arena_Wall_W` stays at fade 1.
- Final code/package commit: `f8eb3244ed59936513a929c8cf0ecd39ebe78bfe`.
  `build_info.json` records `dirty=false` and `source_changed_during_build=false`.
- Build and tests: compile PASS; Tier 1 981/981 PASS on the package commit; Python tooling
  811/811 PASS; content guard and LFS smudge checks PASS. The first final PIE attempt retained
  three transient phase/state failures after the defense test; a clean-editor retry passed all
  23/23 smoke scripts, including the six-wall masked material, base blend mode and 0.333 clip
  checks.
- Final US-099 Tier 3 PASS: `Saved/Playtests/20261125-135848-boot`.
  Final Tier 4 PASS: `Saved/Playtests/20261125-135906-us099` (13 PASS): real Shift+WASD reached
  `(-934.0, 934.0)` alive with x <= -915, `Arena_Wall_W` reached fade 0.000 within 0.5 s,
  real WASD returned to centre, and the wall restored to 1.000 within 0.5 s.
- Final AC-099.4 regression PASS: `Saved/Playtests/20261125-135940-story-us001`;
  Tier 3 `Saved/Playtests/20261125-135940-boot` and Tier 4
  `Saved/Playtests/20261125-135958-us001`, including `camera_fixed` and
  `no_camera_rotation_input`.
- Final full sweep PASS: `Saved/Playtests/20261125-140148-sweep`; six Tier 3 launch paths and
  22 Tier 4 scripts, 527 PASS checks and zero failures. Its current-main comparison uses baseline
  `Saved/Playtests/20261125-110354-sweep` at
  `4d4eb20bff8e10af1f33114dd67fce9b84c10eb7`; `script_pass_to_fail`,
  `check_pass_to_fail`, `pass_to_fail` and `pass_to_missing` are all empty.
- US-090 marketing regression PASS:
  `Saved/Playtests/20261125-140100-us090-marketing-r3-final`; the shared
  `-MordecaiMarketingCapture` flag left TestAttacker active, health fell to the capture
  precondition, and the 54-frame 14.471-second spend/regen/death/respawn contract completed.
- Round-2 coordinator screenshot review (standing no-Read rule):
  Tier 4 `bridge_01.png` (`f07_corner`) shows the living player capsule fully visible at F07
  beside the west wall, the west wall clipped away and the HUD present. Tier 4 `bridge_02.png`
  (`restored`) shows the player at the arena centre, the west wall restored at the left edge and
  the HUD present. The Tier 3 engine frames show the normal loading-screen boot sequence, the
  arena with the Frontliner swarm, and the usual boot death screen.
- Round-3 coordinator review is pending from the checksummed bundle for
  `engine_loading.png`, `engine_01.png`–`engine_03.png`, `bridge_01.png`,
  `bridge_02.png`, four marketing stills and the MP4. Per the standing decision, the builder
  did not open PNG/JPG/video; all image/video checks were scripted.
- Marketing (non-acceptance): `marketing/us099/` contains four 1920x1080 HUD stills and a
  10.633-second 1920x1080 30-fps gameplay-only clip. The retained 319-frame source is contiguous;
  mpdecimate kept 319/319 (30.000 unique fps), all nine sampled frames have HUD structure,
  all 8/8 sampled motion pairs and 105/105 continuity pairs pass, and lock/static checks pass.
  Enemies are inert only via `-MordecaiMarketingFreezeAI`; the camera is the labeled 1000 cm
  marketing camera and stills disclose the 2.0-second presentation fade.
- Route note: F07 `(-935,+935)` is the original bug location from the story on `main`; the
  drafted negative-Y route was not. At camera yaw 0, +Y is screen-right, not screen south-west.
- Follow-up (not filed): audit and tag eligible occluding walls in maps other than DevTestMap;
  N21 is fixed only on DevTestMap.
