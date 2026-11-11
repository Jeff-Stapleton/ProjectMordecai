# US-119: Recovery — Gamepad A Reaches Jump

**Epic:** Epic R: Playability Recovery
**Priority:** P0
**Depends on:** US-083, US-091, US-097
**Paired story:** none

## Overview
Gamepad A is mapped to jump, but CommonUI converts it into a virtual mouse click before Enhanced Input can trigger `ACharacter::Jump`. This story restores controller jumping while preserving both the combat HUD's gameplay input mode and its first-click mouse capture behavior.

## Execution Mode
**HEADLESS**
The fix is native C++ input routing with headless automation coverage; Tier 2 uses PIE only as an integration smoke, and acceptance is packaged Tier 4 real input.

## References
- GitHub issue #85 — `jump_gamepad_a` leaves movement mode `Walking`
- `Specs/control_bindings_v1.1.md` — Jump: controller A / keyboard Space
- `Specs/agent_bridge_v1.md` — gamepad buttons use Slate `FKeyEvent`s and Tier 4 real input
- Existing code: `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp`, `Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp`, `Scripts/playtests/us083_combat_input.py`

## Acceptance Criteria
- [ ] AC-119.1: In gameplay input mode, CommonUI does not convert virtual gamepad Accept (Gamepad A on Windows) into a simulated mouse click; Menu and All modes retain CommonUI's accept behavior — verified by `Mordecai.Recovery.Input.GamepadAcceptPassesThroughGameplayHUD`
- [ ] AC-119.2: In a fresh packaged build, tapping `Gamepad_FaceButton_Bottom` while grounded changes the player movement mode from `Walking` to `Falling` — verified by check `jump_gamepad_a` of `Scripts/playtests/us083_combat_input.py` (Tier 4)
- [ ] AC-119.3: In the fresh-package full sweep, `jump_gamepad_a` is not a NEW failure and no check that PASSed on the `4adaa1bf` baseline changes to FAIL

## Technical Notes
- Root cause (diagnosed before the RED change): `MordecaiIMC_Runtime` already maps the valid `EKeys::Gamepad_FaceButton_Bottom` to `IA_Mordecai_Jump`, and the same action's Space binding passes. The bridge resolves the key through `EKeys` and constructs the same gamepad `FKeyEvent` shape as `FSlateApplication::OnControllerButtonPressed`, so neither the mapping nor the bridge is the fault.
- Regression source: PR #84 commit `2ae4e602` changed the persistent combat HUD to `CapturePermanently_IncludingInitialMouseDown`. In UE 5.7, `FCommonAnalogCursor::IsRelevantInput` specially accepts virtual gamepad Accept when `CanReleaseMouseCapture()` is true; that function returns true for this capture mode. `FAnalogCursor::HandleKeyDownEvent` then converts Gamepad A to a left-click and consumes the key before the viewport and Enhanced Input. This matches the issue history: the `3273068` package (before `2ae4e602`) passed, while the post-round-1 package failed.
- The minimal fix must keep PR #84's capture mode so a mouse click that refocuses the game is not swallowed. A project CommonUI action router will supply a cursor that suppresses virtual-Accept click simulation only while the active input mode is `Game`; Menu/All behavior remains unchanged.
- PR #118 / US-097's invalid-key precedent does not apply here: `AllRuntimeKeysValid` passes and `Gamepad_FaceButton_Bottom` is a valid engine key.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.
- TODO(DECISION): none.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Input.GamepadAcceptPassesThroughGameplayHUD` — AC-119.1 and preservation of PR #84's capture mode
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_gamepad_jump.py` — the live local player uses the Mordecai action router, Game input remains active, and virtual Accept is passed to gameplay; read-only, no simulated jump
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us083_combat_input.py` check `jump_gamepad_a`; real `key` input only, no direct calls or `debug_*`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-083` on the same package
- **Player steps:** on the enemy-free grid, wait until the player is grounded and no longer dodging, then tap controller A (`Gamepad_FaceButton_Bottom`)
- **Visible on screen:** MovementTestMap's open grid, the live Mordecai player, and the combat HUD under the fixed overhead camera
- **Proof:** Tier 3 proves the packaged MovementTestMap and HUD boot; Tier 4 check `jump_gamepad_a` proves the real Gamepad A press changes `Walking` to `Falling`; the full sweep proves no PASS→FAIL regression
- **Tier 4 script:** `Scripts/playtests/us083_combat_input.py` — run: `py Scripts/playtest_story.py US-083`
- **Steps:**

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `jump_gamepad_a` | wait for grounded `Walking`, tap `Gamepad_FaceButton_Bottom` → `player.movement_mode=="Falling"` within 1 s | AC-119.2 |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); no Tier 2 result is acceptance evidence
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh clean package of that commit: `py Scripts/package_playtest.py --clean` exits 0 with `dirty=false`
- [ ] `py Scripts/playtest_story.py US-083` exits 0 on that package, including `jump_gamepad_a`; never `--allow-stale`
- [ ] Full sweep has zero PASS→FAIL versus the raw `4adaa1bf` baseline summary and `jump_gamepad_a` is not NEW
- [ ] Every cited Tier 3 and Tier 4 screenshot opened with the Read tool and described in Completion Notes
- [ ] Evidence bundle with `MANIFEST.sha256` contains build info, story/sweep reports and logs, screenshots, raw comparison, and branch patch
- [ ] Pushed; draft PR opened; story remains in `stories/in-progress/` until coordinator review

## Completion Notes
Pending implementation and packaged verification. Baseline package: `4adaa1bf5a79bc16173b34a74887536650ecdda7`, `dirty=false`, `source_changed_during_build=false`; baseline sweep `Saved/Playtests/20261110-174544-sweep` has `jump_gamepad_a` NEW and will be compared from raw `summary.json`.
