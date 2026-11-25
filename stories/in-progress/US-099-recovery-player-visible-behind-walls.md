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
- Exploratory run `Saved/Playtests/20261022-185207-explore/report.md` — `bridge_11.png`: the near wall hides the player at the south-west corner
- `Specs/map_style_technical_design_v1.md` (height tiers, wall heights, cutaway rules if any), `Specs/game_design_v2.md` (diorama camera), CLAUDE.md "Never implement camera rotation"
- Existing code: `Source/LyraGame/Mordecai/Camera/MordecaiCameraMode_Diorama.*` (fixed pitch -50, yaw 0)
- Lane geometry used by the playtests: lane L1 runs along y = -800 from x = -700 (`Scripts/playtests/us001_movement.py` `walk_lane_start`)
- DevTestMap's arena: walls 200 cm high, inner faces at ±975 (`Scripts/us054_04_setup_arena.py`; `ARENA_WALLS` in `Scripts/tests/fake_bridge.py`). F07 stood against the near (-X) wall at (-935, +935).
- US-100 (issue #46): `us001_movement.py` plays MovementTestMap since then. Its walls are 100 cm high at ±2000, with nothing tall on the camera side, so N21 cannot be reproduced there.

## Acceptance Criteria
- [x] AC-099.1: TODO(DECISION) N21 answered by Jeff and recorded in `Specs/design_decisions_log.md`: (a) fade/hide occluding geometry between camera and player, (b) player silhouette through walls (custom depth / stencil), (c) lower near-side walls by level design (map rule in `map_style_technical_design_v1.md`), or a combination
- [ ] AC-099.2: The chosen mechanism is implemented and covered by Tier 1 tests named after it, including a world trace that proves tagged walls behind nearer untagged/stacked geometry fade and then restore
- [ ] AC-099.3: In the packaged build on DevTestMap, the player walks (WASD) to the original F07 point at `(-935,+935)`, against the 200 cm near (-X) wall, and the character is visible on screen. Tier 4 requires x <= -915, `Arena_Wall_W` fade <= 0.05 within 0.5 s, then a real-input return to `(0,0)` and fade >= 0.999 within 0.5 s. The coordinator reviews `sw_corner` and `restored` screenshots from the bundle under the standing no-Read rule.
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
  `Arena_Wall_W` fade <= 0.05, captures `sw_corner`, returns to `(0,0)`, requires
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
- `Scripts/playtests/us099_walls.py`: F07 position + fade assertion, `sw_corner`
  screenshot, real-input return to centre, restore assertion, and `restored` screenshot
- `Scripts/playtests/us001_movement.py` (MovementTestMap): `camera_fixed`, `no_camera_rotation_input` (AC-099.4)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-099` on the same package (it runs the new DevTestMap script), plus `py Scripts/playtest_story.py US-001` (MovementTestMap) for AC-099.4
- **Player steps:** walk (WASD) to DevTestMap's original F07 point `(-935,+935)` against the 200 cm near (-X) wall, then walk back to `(0,0)`
- **Visible on screen:** `sw_corner` shows the whole character (not just the top of the head) beside the faded west wall; `restored` shows that wall fully restored after the player returns to centre
- **Proof:** Tier 4 checks `walk_sw_corner`, `wall_fades_at_corner`,
  `walk_back_to_centre`, and `wall_restores_clear`, plus the two screenshots;
  `camera_fixed`, `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** the new DevTestMap script (e.g. `Scripts/playtests/us099_walls.py`) — run: `py Scripts/playtest_story.py US-099`
- **Steps:** generated by `py Scripts/playtest_story.py US-099 --plan`; checked by `--lint`

<!-- Scripts/playtests/us099_walls.py -->
| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `walk_sw_corner` | tap `LeftShift`, `walk_to(-935, 0)`, then `walk_to(-935, 935)` with real WASD → within 20 cm, x <= -915, and alive | AC-099.3 |
| 2 | `wall_fades_at_corner` | observe `camera.occluders[Arena_Wall_W].fade <= 0.05` within 0.5 s, then screenshot `sw_corner` | AC-099.2, AC-099.3 |
| 3 | `walk_back_to_centre` | `walk_to(-935, 0)`, then `walk_to(0, 0)` with real WASD | AC-099.3 |
| 4 | `wall_restores_clear` | observe `camera.occluders[Arena_Wall_W].fade >= 0.999` within 0.5 s, then screenshot `restored` | AC-099.2, AC-099.3 |
- **Playable alone:** YES (after the decision; NO if an EDITOR pair is split off — then "playable after US-XXX")

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: every asset (if any) saved and committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-099` on that same fresh package: all fade,
  position, restore, and screenshot checks PASS; `py Scripts/playtest_story.py US-001`
  (MovementTestMap): `camera_fixed`, `no_camera_rotation_input` PASS; no check that
  PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issue N21 marked FIXED on DevTestMap in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (engine_NN.png, bridge_NN.png) reviewed by the coordinator under the standing no-Read rule; Completion Notes state what is on screen.
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-099]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
- Decision: Jeff selected N21 option (a) on 2026-11-17. Three camera-to-capsule
  visibility traces fade only tagged `WorldStatic` wall meshes through the masked
  `MordecaiOcclusionFade` material parameter. The 0.25-second eased transition restores to
  fully opaque when clear; floors, Pawns, enemies and untagged geometry are excluded. Camera
  position, follow and fixed rotation are unchanged.
- RED: commit `91443b41`; `Saved/TestLogs/us099-red.log` failed the fade and restore tests for
  the intended missing implementation. GREEN: project compiled; 976/976 Mordecai automation
  tests passed, including the final-head log `Saved/TestLogs/us099-green-final-head.log`;
  797/797 Python tooling tests passed in `Saved/TestLogs/us099-python-final.log`; all 23 PIE
  scripts passed. `check_content_committed.py` passed with the material and DevTestMap assets.
- Package: `493f45c0c2ee5deafc2be8d5b9b0ca97cf843472`, clean
  (`dirty=false`, `source_changed_during_build=false`).
- US-099 Tier 3 PASS:
  `Saved/Playtests/20261125-000348-boot`. All boot, experience, cooked-content, loading-screen,
  game-window and three engine-screenshot checks passed with zero logged errors.
- US-099 Tier 4 PASS:
  `Saved/Playtests/20261125-000409-us099`. Real Shift+WASD reached the original F07 evidence
  point at `(-935.0, 934.4)` alive, 13.8 cm from the target; `walk_sw_corner`,
  `screenshot_sw_corner`, no-crash and cooked-content checks passed. Evidence frame:
  `bridge_01.png`.
- Round-1 coordinator screenshot review (standing no-Read rule):
  `bridge_01.png` from the US-099 Tier 4 run shows the player capsule fully visible at
  the corner directly beside the west wall with the HUD present. `engine_02.png` from
  the Tier 3 boot shows the arena from the default camera with the usual death screen
  from the boot swarm.
- AC-099.4 regression PASS:
  `Saved/Playtests/20261125-000459-us001`; both `camera_fixed` and
  `no_camera_rotation_input` passed with control rotation fixed at `[-50, 0, 0]`. Its Tier 3
  MovementTestMap boot also passed at `Saved/Playtests/20261125-000441-boot`.
- Full sweep PASS:
  `Saved/Playtests/20261125-000549-sweep` ran 22 Tier 4 scripts and six Tier 3 launch paths
  with no failures. Compared with baseline `Saved/Playtests/20261124-172202-sweep`,
  `comparison.json` has `pass_to_fail: []`, no new or known failures, 21 unchanged existing
  PASS scripts, and US-099 as the one added PASS script. No known-flake rerun was needed.
- Marketing: `marketing/us099/` has five 1920x1080 game-owned stills and one 5.4-second,
  30-fps HUD-visible MP4. The player walks behind the wall, the wall fades, and real
  Blink/dodge/WASD input escapes the swarm while it restores. `content-check.json` passed
  source matching, HUD structure, all 8/8 sampled motion pairs, 53/53 temporal-continuity
  pairs and 21/21 unique source-frame hashes. `label-proof.json` passed with TextRender
  labels suppressed and no rendered actor/debug labels or enemy indicators.
- Under the standing no-Read rule, the builder did not open PNG/JPG/video. Scripts
  verified dimensions, non-static motion, HUD structure, provenance and clean
  presentation; the coordinator reviews every acceptance and marketing frame from the
  checksummed bundle.
- Route note: the acceptance route uses the original F07 `(-935, +935)` evidence point
  recorded by the story on `main`. The drafted negative-Y route was dropped because it
  was not the original bug location. With camera yaw 0, +Y is screen-right, so this point
  is not described as screen south-west.
- Bundle: `us099-493f45c0` in the assigned Agent Store.
- Round 2 reopened the story after evidence review. New world-trace, bridge-state,
  negative-control, package, sweep, media and coordinator screenshot evidence is pending;
  the story remains in `stories/in-progress/` until those checks pass.
