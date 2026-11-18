# US-098: Recovery — Facing and Aim Follow the Mouse / Right Stick

**Epic:** Epic R: Playability Recovery
**Priority:** P2
**Depends on:** US-097 (right-stick key name)
**Paired story:** none

## Overview
The player always faces +X: the diorama camera writes the control rotation every frame, so mouse and right-stick aim cannot turn the character, and every attack and spell goes the same way. `control_bindings_v1.1.md` says "aim sets facing and attack direction" and US-001 AC-2.1.2 promises it. The owner has now decided that mouse and right-stick aim drive character facing while the camera and movement basis remain fixed. Features: movement, camera, projectiles_aim.

## Execution Mode
**HEADLESS**
Camera mode and character rotation logic in C++ (`UMordecaiCameraMode_Diorama`, `AMordecaiCharacter`); no assets.

## References
- `Specs/agent_playtest_plan_v1.md` — "Facing and aim are pinned to +X" (`MordecaiCameraMode_Diorama.cpp:51`), N12 (`MordecaiCharacter.cpp:66-70`, arrow hidden in game)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184725-us001`: `facing_mouse` FAIL known ("mouse moved to (1100, 360); player.rotation yaw stayed 0 (expected 90 +-30 within 1s)"), `facing_stick` FAIL known; `camera_fixed`, `no_camera_rotation_input` PASS (the camera must keep not rotating); AC-2.1.5 PENDING (bridge cannot drive: no capsule/arrow visibility in state)
- `Specs/control_bindings_v1.1.md` — Look / Aim Direction: Right Stick / Mouse movement, "No target lock; aim sets facing and attack direction"
- `Specs/game_design_v2.md` (diorama camera), CLAUDE.md: "Never implement camera rotation"
- Done stories: US-001 AC-2.1.2 (rotation driven by right stick / mouse aim, independent of movement), AC-2.1.5 (facing arrow), AC-1.2.2/1.2.4 (camera fixed)

## Acceptance Criteria
- [x] AC-098.1: The owner's final aim-driven-facing decision is recorded in `Specs/design_decisions_log.md`
- [x] AC-098.2: The character's yaw follows the aim direction (mouse cursor projected onto the ground plane at the character's height; right-stick direction above a 0.2 deadzone), independent of movement, while the camera's rotation stays fixed at pitch -50 / yaw 0 — verified by `Mordecai.Recovery.Facing.FollowsMouseAim`, `Mordecai.Recovery.Facing.FollowsStickAim`, `Mordecai.Recovery.Camera.StaysFixedWhileAiming`
- [x] AC-098.3: In the packaged build, the player moves the physical OS mouse right of the character and the character faces +Y (90° ± 30) within 1 s — verified by check `facing_mouse_physical` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [x] AC-098.4: In the packaged build, the player holds the right stick left and the character faces -Y (-90° ± 30) within 1 s — verified by check `facing_stick` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [x] AC-098.5: `camera_fixed` and `no_camera_rotation_input` still PASS (no camera rotation, ever)
- [x] AC-098.6: The facing arrow (AC-2.1.5) is visible in game and follows pawn forward — Tier 4 state check `facing_arrow` plus the `physical_mouse_off_axis` / `facing_arrow` screenshots
- [x] AC-098.7: With a +Y aim established, W / left-stick up still moves along camera-relative +X without changing yaw; releasing the right stick keeps the last facing
- [x] AC-098.8: Melee hit direction, block facing, no-input dodge fallback, Blink and aimed spells consistently consume the character's actor-facing direction
- [x] AC-098.9: The player capsule remains radius 40, half-height 90 and maximum step height 45
- [x] AC-098.10: The locally controlled character turns immediately. The existing unreliable aim RPC and normal actor-rotation replication remain in place, but server-authority behavior is not claimed without a real multi-client net world; multiplayer verification/prediction/reconciliation is follow-up scope
- [x] AC-098.11: Aim-facing is locked when controller look input is ignored and while Dead, MicroStunned, PostureBroken, KnockedDown, Dodging, Parrying, or melee Windup/Active (`State.Attacking`); it resumes in melee Recovery
- [x] AC-098.12: Packaged mouse aim follows a real Win32 OS cursor, recomputes every local-player tick, and ignores projected points within the 40 cm capsule-radius dead zone
- [x] AC-098.13: The gameplay cursor and a hit-test-invisible crosshair are visible while mouse+keyboard is last-used, LMB still starts an attack, and both cursor affordances hide when gamepad is last-used
- [x] AC-098.14: The right-stick mapping applies its 0.2 deadzone exactly once, while the movement stick retains its independent 0.25 deadzone
- [x] AC-098.15: The strengthened tests drive the real look and movement handlers, tick movement after input, perform the real melee arc query, and do not claim net authority without a multi-client net world

## Technical Notes
- The camera keeps writing its fixed view/control rotation, but character yaw no longer derives from controller desired rotation. Character yaw comes from aim vectors set by the Look actions. Movement uses the fixed camera-yaw-zero basis, not actor or control rotation.
- Melee, no-input dodge fallback, Blink, Fireball, Magic Missile, Illusion, Enfeeble, Snare, Sleep and Cone of Cold already read `GetActorForwardVector()` / actor rotation and therefore follow the new facing. Block is currently direction-agnostic (`MordecaiHitDefense.cpp` TODO); keeping actor facing current ensures a future frontal block uses the same source without inventing a block cone in this story.
- Mouse aim: deproject the live OS cursor to the ground plane at the player's Z every tick while CommonInput reports MouseAndKeyboard. The packaged acceptance step uses Win32 `SetCursorPos`/`SendInput`, not the agent bridge's virtual Slate cursor.
- Script bookkeeping (before packaging): remove the facing `known_issue` from `facing_mouse` and `facing_stick`, add an aimed-movement independence check and refresh US-001's Steps table so `--lint` passes.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Facing.FollowsMouseAim`, `Mordecai.Recovery.Facing.FollowsStickAim`, `Mordecai.Recovery.Facing.MovementDoesNotOverrideAim`, `Mordecai.Recovery.Camera.StaysFixedWhileAiming`, `Mordecai.Recovery.Facing.CombatConsumersUseActorForward`, `Mordecai.Recovery.Facing.PreservesCapsuleContract` — AC-098.2, AC-098.7–AC-098.9
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_spawn.py`: camera rotation fixed; arrow component visible in game
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us001_movement.py` checks `facing_mouse_physical`, `cursor_visible_mouse`, `attack_click_physical`, `movement_keeps_aim`, `facing_stick`, `cursor_hidden_gamepad`, `camera_fixed`, and `no_camera_rotation_input`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience; us001's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-001` on the same package
- **Player steps:** move the mouse to the right of the player; hold W while that +Y aim remains set; hold the right stick left
- **Visible on screen:** us001 `physical_mouse_off_axis` screenshot: the visible crosshair and green facing arrow point toward the off-axis physical cursor (+Y) after W moved the character camera-relative +X; `facing_arrow` shows the final -Y stick-facing direction; camera angle unchanged
- **Proof:** Tier 4 checks `facing_mouse_physical`, `cursor_visible_mouse`, `attack_click_physical`, `movement_keeps_aim`, `facing_stick`, `cursor_hidden_gamepad`, `camera_fixed`, and `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** `Scripts/playtests/us001_movement.py` — run: `py Scripts/playtest_story.py US-001`
- **Steps:**

| Script | Check | Player step → expected | Final packaged result (3b3e8b4c) |
|---|---|---|---|
| us001 | `facing_mouse_physical` | Win32 `SetCursorPos` to client (1100, 360), with no bridge mouse command → yaw 90 (+-30) within 1s | PASS: yaw 90 |
| us001 | `cursor_visible_mouse` / `attack_click_physical` | physical mouse last-used → cursor/crosshair visible; Slate-routed LMB still reaches Enhanced Input | PASS: visible and attack started |
| us001 | `movement_keeps_aim` | with mouse aim still +Y, hold W 0.4s → move >100 cm along +X, <30 cm across, yaw remains 90 (+-30) | PASS: +186 cm X, 0 cm across, yaw 90.852 |
| us001 | `facing_stick` | axis Gamepad_RightX=-1 held (left, -Y) -> wait_until player.rotation yaw -90 (+-30) within 1s | PASS: yaw -90, retained after release |
| us001 | `cursor_hidden_gamepad` | right stick becomes last-used input | PASS: cursor hidden |

- **Playable alone:** YES (after the decision and US-097)

## Definition of Done
- [x] All ACs checked off
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: none; `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [x] On that same fresh package `py Scripts/playtest_story.py US-001`: physical mouse, visible cursor/crosshair, attack click, movement-independent aim, stick aim, gamepad cursor hiding, `camera_fixed`, and `no_camera_rotation_input` all PASS; `--lint` exits 0 — never `--allow-stale`
- [x] Known issues "Facing and aim are pinned to +X" and N12 marked FIXED/decided in `Specs/agent_playtest_plan_v1.md`
- [x] Every screenshot is preserved and validated by script into `image-verification.json`; the draft PR remains subject to Mordecai Bot image review before merge (laptop-builder rule in `Specs/design_decisions_log.md`)
- [x] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-098]` closing commit
- [x] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
- Owner decisions are recorded in `Specs/design_decisions_log.md`: facing locks during melee Windup/Active and resumes in Recovery; physical mouse gameplay shows a live hit-test-invisible crosshair while preserving attack clicks and hides it after gamepad use. Round-2 implementation commits: `73bb3a7c`, `26da2c94`, `8050165c`, `99684072`; packaged-probe stabilization: `3b3e8b4c`.
- RED: `%TEMP%\core-34-r2-red-facing.log` ran 17 real-path facing tests and failed on the intended missing lock/cursor/tick/deadzone contracts; `%TEMP%\core-34-r2-red-python.log` failed because `facing_mouse_physical` was not yet in the Tier 4 plan.
- GREEN: `%TEMP%\core-34-r2-green-build.log` clean compile; `%TEMP%\core-34-r2-green-facing.log` 17/17 facing tests PASS; `%TEMP%\core-34-r2-green-full.log` 871/871 Mordecai tests PASS; `%TEMP%\core-34-r2-green-python-movement-final.log` 712/712 Python tests PASS; `%TEMP%\core-34-r2-green-pie.log` 21/21 Tier 2 smoke suites PASS. US-001/054/091/120 lints and the content guard also passed.
- Fresh package: `Saved/Playtests/package-20261117-212137.log`; `PackagedBuilds/Playtest/build_info.json` records commit `3b3e8b4c298c68302aa302acf1fb4537fa41446c`, `dirty=false`, `source_changed_during_build=false`.
- Tier 3 PASS: `Saved/Playtests/20261117-212936-boot/report.md`. Script verification found the Mordecai loading frame and every engine/desktop capture non-uniform.
- Tier 4 PASS (37 PASS, 0 FAIL): `Saved/Playtests/20261117-213000-us001/report.md`. Win32 `SetCursorPos` (not a bridge mouse command) produced yaw 90; the cursor/crosshair was visible; Slate-routed LMB started melee; W moved +186 cm X / 0 cm across while retaining yaw 90.852; right-stick aim reached and retained yaw -90 and hid the cursor; fixed-camera checks passed.
- Script-only image verification (no image file was opened by the builder): `%TEMP%\core-34-r2-image-verification-3b3e8b4c.json` is PASS. Required `bridge_03.png` is non-uniform (luminance standard deviation 33.025), has 320 central green pixels at `[653,332,713,341]`, and has the white/black crosshair detected at physical cursor client coordinate `[1100,360]` (52 white pixels, 8 black pixels).
- Exact-main baseline: `Saved/Playtests/20261117-134636-sweep/summary.json` at `9176ce1f`. Final-package first sweep: `Saved/Playtests/20261117-213159-sweep/summary.json`; same-package rerun: `Saved/Playtests/20261117-214149-sweep/summary.json`. The first sweep passed `move_a`, `respawn_moves`, `cliff_jump_blocked`, and `deep_bank_blocks`; the rerun again passed the two formerly missed short inputs (`move_a`, `respawn_moves`). `%TEMP%\core-34-r2-comparison-3b3e8b4c.json` compares 424 common checks and reports persistent `pass_to_fail=[]`; it records baseline FAIL→PASS for `facing_stick`, `move_a`, `respawn_moves`, and `deep_bank_blocks`. The first sweep retained only baseline failures (`enemies_present`, known `cycle_prev_shift_tab`); rerun-only `sprint_shift` and cliff-approach stalls were not persistent and do not replace first-run passes.
- Direction audit: the real melee arc query, no-input dodge fallback, Blink, and aimed spells consume actor forward and therefore the new facing. Block remains direction-agnostic because there is no block cone yet.
- Multiplayer follow-ups only: aim RPC throttling; stale facing after a dropped unreliable packet; saved-move rotation merge; predicted aim path. Server-authority is not claimed without a real multi-client net world.
