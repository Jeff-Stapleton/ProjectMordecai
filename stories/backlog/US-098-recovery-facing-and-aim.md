# US-098: Recovery — Facing and Aim Follow the Mouse / Right Stick (BLOCKED on decision)

**Epic:** Epic R: Playability Recovery
**Priority:** P2
**Depends on:** TODO(DECISION) "facing pinned to +X" (Jeff); US-097 (right-stick key name)
**Paired story:** none

## Overview
The player always faces +X: the diorama camera writes the control rotation every frame, so mouse and right-stick aim cannot turn the character, and every attack and spell goes the same way. `control_bindings_v1.1.md` says "aim sets facing and attack direction" and US-001 AC-2.1.2 promises it, but whether a fixed facing is intended for the diorama style is Jeff's design call. The facing arrow (AC-2.1.5) may also be hidden in game (N12). Features: movement, camera, projectiles_aim.

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
- [ ] AC-098.1: TODO(DECISION) answered by Jeff and recorded in `Specs/design_decisions_log.md`: (a) aim-driven facing as specified, or (b) fixed facing / movement-driven facing (then AC-2.1.2 is amended and `facing_mouse`/`facing_stick` are rewritten or removed). No code before the answer
- [ ] AC-098.2 (option a): The character's yaw follows the aim direction (mouse cursor projected onto the ground plane relative to the player; right stick direction), independent of movement, while the camera's rotation stays fixed at pitch -50 / yaw 0 — verified by `Mordecai.Recovery.Facing.FollowsMouseAim`, `Mordecai.Recovery.Facing.FollowsStickAim`, `Mordecai.Recovery.Camera.StaysFixedWhileAiming`
- [ ] AC-098.3 (option a): In the packaged build, the player moves the mouse right of the character and the character faces +Y (90° ± 30) within 1 s — verified by check `facing_mouse` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-098.4 (option a): In the packaged build, the player holds the right stick left and the character faces -Y (-90° ± 30) within 1 s — verified by check `facing_stick` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-098.5: `camera_fixed` and `no_camera_rotation_input` still PASS (no camera rotation, ever)
- [ ] AC-098.6: The facing arrow (AC-2.1.5) is visible in game (N12: `bHiddenInGame` false or a visible facing indicator) — Tier 3/4 screenshot `facing_arrow` (`bridge_03.png` of us001) shows it; the state assertion stays PENDING TIER 4 (bridge cannot drive: B2, no component visibility in state)

## Technical Notes
- The camera mode must stop deriving the pawn's facing from the control rotation (or stop writing the control rotation, keeping its own fixed view rotation); character yaw then comes from an aim vector set by the Look actions. Projectiles (Fireball, Magic Missile) already use `GetActorForwardVector()` + aim assist, so they will follow facing.
- Mouse aim: deproject the cursor to the ground plane at the player's Z. Watch the agent bridge's virtual cursor: the playtest sets the cursor with `mouse X Y`.
- Script bookkeeping (before packaging): option (a) — remove the facing `known_issue` from `facing_mouse` and `facing_stick`; option (b) — rewrite/remove those steps per the decision and refresh US-001's Steps table so `--lint` passes.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Facing.FollowsMouseAim`, `Mordecai.Recovery.Facing.FollowsStickAim`, `Mordecai.Recovery.Camera.StaysFixedWhileAiming` — AC-098.2
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_spawn.py`: camera rotation fixed; arrow component visible in game
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us001_movement.py` checks `facing_mouse`, `facing_stick`, `camera_fixed`, `no_camera_rotation_input`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-001` on the same package
- **Player steps:** move the mouse to the right of the player; hold the right stick left
- **Visible on screen:** us001 `bridge_03.png` (`facing_arrow`): the character (and its facing arrow) turned toward the last aim direction, camera angle unchanged
- **Proof:** Tier 4 checks `facing_mouse`, `facing_stick`, `camera_fixed`, `no_camera_rotation_input` of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** `Scripts/playtests/us001_movement.py` — run: `py Scripts/playtest_story.py US-001`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us001 | `facing_mouse` | mouse 1100 360 (right of the player, +Y) -> wait_until player.rotation yaw 90 (+-30) within 1s | FAIL (known): yaw 0 |
| us001 | `facing_stick` | axis Gamepad_RightX=-1 held (left, -Y) -> wait_until player.rotation yaw -90 (+-30) within 1s | FAIL (known): yaw 0 |

- **Playable alone:** YES (after the decision and US-097)

## Definition of Done
- [ ] All ACs checked off (AC-098.6's state assertion stays PENDING TIER 4, not claimed)
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-001`: `facing_mouse`, `facing_stick` go from FAIL to PASS (option a) or are rewritten per the decision; `camera_fixed`, `no_camera_rotation_input` still PASS; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issues "Facing and aim are pinned to +X" and N12 marked FIXED/decided in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-098]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
