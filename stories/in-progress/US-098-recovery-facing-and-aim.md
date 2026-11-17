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
- [ ] AC-098.2: The character's yaw follows the aim direction (mouse cursor projected onto the ground plane at the character's height; right-stick direction above a 0.2 deadzone), independent of movement, while the camera's rotation stays fixed at pitch -50 / yaw 0 — verified by `Mordecai.Recovery.Facing.FollowsMouseAim`, `Mordecai.Recovery.Facing.FollowsStickAim`, `Mordecai.Recovery.Camera.StaysFixedWhileAiming`
- [ ] AC-098.3: In the packaged build, the player moves the mouse right of the character and the character faces +Y (90° ± 30) within 1 s — verified by check `facing_mouse` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-098.4: In the packaged build, the player holds the right stick left and the character faces -Y (-90° ± 30) within 1 s — verified by check `facing_stick` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-098.5: `camera_fixed` and `no_camera_rotation_input` still PASS (no camera rotation, ever)
- [ ] AC-098.6: The facing arrow (AC-2.1.5) is visible in game and follows pawn forward — Tier 4 state check `facing_arrow` plus the `mouse_facing_off_axis` / `facing_arrow` screenshots
- [ ] AC-098.7: With a +Y aim established, W / left-stick up still moves along camera-relative +X without changing yaw; releasing the right stick keeps the last facing
- [ ] AC-098.8: Melee hit direction, block facing, no-input dodge fallback, Blink and aimed spells consistently consume the character's actor-facing direction
- [ ] AC-098.9: The player capsule remains radius 40, half-height 90 and maximum step height 45
- [ ] AC-098.10: The locally controlled character turns immediately, and non-authority aim updates are sent to the authority through the existing replicated character rotation path; broader multiplayer prediction/reconciliation is documented as follow-up scope

## Technical Notes
- The camera keeps writing its fixed view/control rotation, but character yaw no longer derives from controller desired rotation. Character yaw comes from aim vectors set by the Look actions. Movement uses the fixed camera-yaw-zero basis, not actor or control rotation.
- Melee, no-input dodge fallback, Blink, Fireball, Magic Missile, Illusion, Enfeeble, Snare, Sleep and Cone of Cold already read `GetActorForwardVector()` / actor rotation and therefore follow the new facing. Block is currently direction-agnostic (`MordecaiHitDefense.cpp` TODO); keeping actor facing current ensures a future frontal block uses the same source without inventing a block cone in this story.
- Mouse aim: deproject the cursor to the ground plane at the player's Z. Watch the agent bridge's virtual cursor: the playtest sets the cursor with `mouse X Y`.
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
- `Scripts/playtests/us001_movement.py` checks `facing_mouse`, `facing_stick`, `camera_fixed`, `no_camera_rotation_input`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience; us001's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-001` on the same package
- **Player steps:** move the mouse to the right of the player; hold W while that +Y aim remains set; hold the right stick left
- **Visible on screen:** us001 `mouse_facing_off_axis` screenshot: the character and green facing arrow point toward the off-axis cursor (+Y) after W moved the character camera-relative +X; `facing_arrow` shows the final -Y stick-facing direction; camera angle unchanged
- **Proof:** Tier 4 checks `facing_mouse`, `facing_stick`, `camera_fixed`, `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** `Scripts/playtests/us001_movement.py` — run: `py Scripts/playtest_story.py US-001`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us001 | `facing_mouse` | mouse 1100 360 (right of the player, +Y) -> wait_until player.rotation yaw 90 (+-30) within 1s | FAIL (known): yaw 0 |
| us001 | `movement_keeps_aim` | with mouse aim still +Y, hold W 0.4s -> move >100 cm along +X, <30 cm across, yaw remains 90 (+-30) | NOT RUN (blocked by facing_mouse) |
| us001 | `facing_stick` | axis Gamepad_RightX=-1 held (left, -Y) -> wait_until player.rotation yaw -90 (+-30) within 1s | FAIL (known): yaw 0 |

- **Playable alone:** YES (after the decision and US-097)

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-001`: `facing_mouse`, `facing_stick` go from FAIL to PASS (option a) or are rewritten per the decision; `camera_fixed`, `no_camera_rotation_input` still PASS; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issues "Facing and aim are pinned to +X" and N12 marked FIXED/decided in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot is preserved and validated by script into `image-verification.json`; a human/coordinator performs image review before merge (laptop-builder rule in `Specs/design_decisions_log.md`)
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-098]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
