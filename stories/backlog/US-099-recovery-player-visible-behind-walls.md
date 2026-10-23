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

## Acceptance Criteria
- [ ] AC-099.1: TODO(DECISION) N21 answered by Jeff and recorded in `Specs/design_decisions_log.md`: (a) fade/hide occluding geometry between camera and player, (b) player silhouette through walls (custom depth / stencil), (c) lower near-side walls by level design (map rule in `map_style_technical_design_v1.md`), or a combination
- [ ] AC-099.2: The chosen mechanism is implemented and covered by Tier 1 tests named after it (e.g. `Mordecai.Recovery.Camera.OccluderFadesWhenBlockingPlayer`)
- [ ] AC-099.3: In the packaged build, the player walks (WASD) into the south-west corner and the character is visible on screen — verified by a NEW check + screenshot `walk_sw_corner` / `screenshot sw_corner` added to `Scripts/playtests/us001_movement.py` (Tier 4: the walk is the input; the visibility is judged from `bridge_NN.png`, which the agent opens and describes; add a state assertion if the bridge reports occluder/fade state)
- [ ] AC-099.4: The camera still never rotates: `camera_fixed`, `no_camera_rotation_input` PASS

## Technical Notes
- TODO(DECISION) N21 — options listed in AC-099.1; do not pick one.
- New us001 step (script edit committed before packaging): `walk_sw_corner` = `walk_to(-750, -850)` (or the actual SW corner inside the arena; read the arena bounds from DevTestMap), then `shot("sw_corner")` with `SCREENSHOTS["sw_corner"] = "the player fully visible at the south-west corner (near wall faded/cut away/silhouette)"`; `proves` `US-099:AC-099.3`, US-099 added to `COVERS`. Offline test against `Scripts/tests/fake_bridge.py`.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- Per decision (AC-099.2)
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_spawn.py`: camera unchanged; the occlusion component (if any) present
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us001_movement.py` new `walk_sw_corner` + `sw_corner` screenshot

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-099` (after adding US-099 to `COVERS`) or `US-001`, on the same package
- **Player steps:** walk (WASD) to the south-west corner of the arena
- **Visible on screen:** the `sw_corner` screenshot shows the whole character (not just the top of its head) next to the near wall
- **Proof:** Tier 4 check `walk_sw_corner` and the `sw_corner` screenshot of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** `Scripts/playtests/us001_movement.py` — run: `py Scripts/playtest_story.py US-001`
- **Steps:** paste the new rows from `py Scripts/playtest_story.py US-099 --plan` after implementing
- **Playable alone:** YES (after the decision; NO if an EDITOR pair is split off — then "playable after US-XXX")

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: every asset (if any) saved and committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-099` on that same fresh package: `walk_sw_corner` PASSes, the `sw_corner` screenshot shows the whole character; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issue N21 marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-099]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
