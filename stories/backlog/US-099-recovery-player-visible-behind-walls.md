# US-099: Recovery — The Player Stays Visible Behind Near Walls (BLOCKED on cutaway decision)

**Epic:** Epic R: Playability Recovery
**Priority:** P2
**Depends on:** TODO(DECISION) N21 cutaway approach (Jeff)
**Paired story:** possibly an EDITOR pair, depending on the decision (see Technical Notes)

## Overview
The exploratory agent found that at the arena's south-west corner the near wall hides everything but the top of the player's head (N21), and the fixed diorama camera has no cutaway or fade. In a fixed-camera game the player must always be able to see their character. Features: camera, movement.

## Execution Mode
**HEADLESS**
If the chosen approach is code-only (camera-to-player occlusion trace that hides/fades occluding actors, or a character silhouette via custom depth stencil driven from C++). If it needs a material / post-process asset (dithered fade material, silhouette post-process material), split an EDITOR pair at decision time and queue it directly after this story.

## References
- `Specs/agent_playtest_plan_v1.md` — N21 ("Found by the first exploratory-agent run")
- Exploratory run `Saved/Playtests/20261022-185207-explore/report.md` — `bridge_11.png`: the near wall hides the player at the south-west corner
- `Specs/map_style_technical_design_v1.md` (height tiers, wall heights, cutaway rules if any), `Specs/game_design_v2.md` (diorama camera), CLAUDE.md "Never implement camera rotation"
- Existing code: `Source/LyraGame/Mordecai/Camera/MordecaiCameraMode_Diorama.*` (fixed pitch -50, yaw 0)
- Lane geometry used by the playtests: lane L1 runs along y = -800 from x = -700 (`Scripts/playtests/us001_movement.py` `walk_lane_start`)
- DevTestMap's arena: walls 200 cm high, inner faces at ±975 (`Scripts/us054_04_setup_arena.py`; `ARENA_WALLS` in `Scripts/tests/fake_bridge.py`). F07 stood against the near (-X) wall at (-935, 935).
- US-100 (issue #46): `us001_movement.py` plays MovementTestMap since then. Its walls are 100 cm high at ±2000, with nothing tall on the camera side, so N21 cannot be reproduced there.

## Acceptance Criteria
- [ ] AC-099.1: TODO(DECISION) N21 answered by Jeff and recorded in `Specs/design_decisions_log.md`: (a) fade/hide occluding geometry between camera and player, (b) player silhouette through walls (custom depth / stencil), (c) lower near-side walls by level design (map rule in `map_style_technical_design_v1.md`), or a combination
- [ ] AC-099.2: The chosen mechanism is implemented and covered by Tier 1 tests named after it (e.g. `Mordecai.Recovery.Camera.OccluderFadesWhenBlockingPlayer`)
- [ ] AC-099.3: In the packaged build on DevTestMap, the player walks (WASD) into the south-west corner, against the 200 cm near (-X) wall, and the character is visible on screen — verified by a NEW check + screenshot `walk_sw_corner` / `screenshot sw_corner` in a DevTestMap script, e.g. a new `Scripts/playtests/us099_walls.py` (`STORY` "US-099", `MAP = kit.DEFAULT_MAP`, its own `RUN_SUFFIX`) — not `us001_movement.py`, which plays MovementTestMap (Tier 4: the walk is the input; the visibility is judged from `bridge_NN.png`, which the agent opens and describes; add a state assertion if the bridge reports occluder/fade state)
- [ ] AC-099.4: The camera still never rotates: `camera_fixed`, `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py` PASS

## Technical Notes
- TODO(DECISION) N21 — options listed in AC-099.1; do not pick one.
- New DevTestMap script (committed before packaging), e.g. `Scripts/playtests/us099_walls.py`, a `PlaytestRun` with `STORY = "US-099"`, `MAP = kit.DEFAULT_MAP` and its own `RUN_SUFFIX`: `walk_sw_corner` = `walk_to` a spot hugging the near (-X) wall inside the ±975 arena, about (-930, -930) (F07 stood at x ≈ -935), then `shot("sw_corner")` with `SCREENSHOTS["sw_corner"] = "the player fully visible at the south-west corner (near wall faded/cut away/silhouette)"`; `proves` `AC-099.3`. Offline test against `Scripts/tests/fake_bridge.py` (`FakeWorld(**MAP_PRESETS[DEFAULT_MAP])`, which has the arena walls).
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
- the new DevTestMap script (e.g. `Scripts/playtests/us099_walls.py`): `walk_sw_corner` + `sw_corner` screenshot
- `Scripts/playtests/us001_movement.py` (MovementTestMap): `camera_fixed`, `no_camera_rotation_input` (AC-099.4)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-099` on the same package (it runs the new DevTestMap script), plus `py Scripts/playtest_story.py US-001` (MovementTestMap) for AC-099.4
- **Player steps:** walk (WASD) to the south-west corner of DevTestMap's arena, against the 200 cm near (-X) wall
- **Visible on screen:** the `sw_corner` screenshot shows the whole character (not just the top of its head) next to the near wall
- **Proof:** Tier 4 check `walk_sw_corner` and the `sw_corner` screenshot of the new DevTestMap script (e.g. `Scripts/playtests/us099_walls.py`); `camera_fixed`, `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** the new DevTestMap script (e.g. `Scripts/playtests/us099_walls.py`) — run: `py Scripts/playtest_story.py US-099`
- **Steps:** paste the new rows from `py Scripts/playtest_story.py US-099 --plan` after implementing
- **Playable alone:** YES (after the decision; NO if an EDITOR pair is split off — then "playable after US-XXX")

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: every asset (if any) saved and committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-099` on that same fresh package: `walk_sw_corner` PASSes, the `sw_corner` screenshot shows the whole character; `py Scripts/playtest_story.py US-001` (MovementTestMap): `camera_fixed`, `no_camera_rotation_input` PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issue N21 marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-099]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
