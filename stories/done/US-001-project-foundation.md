# US-001: Project Foundation & Initial Player Character

## Execution Mode
**Mode:** HEADLESS

## Overview
Get a running UE 5.7 project with a controllable player character in a test level. Twin-stick controls, fixed diorama camera, GAS wired up with base attributes. This is the foundation everything else builds on.

---

## Story 1.1 — Project Scaffolding

### Acceptance Criteria
- [x] AC-1.1.1: UE 5.7 C++ project compiles with zero errors and zero warnings in game module
- [x] AC-1.1.2: Plugins enabled: GameplayAbilities, GameplayTags, GameplayTasks, EnhancedInput
- [x] AC-1.1.3: Base classes exist: `AMordecaiGameMode`, `AMordecaiGameState`, `AMordecaiPlayerState`, `AMordecaiPlayerController`
- [x] AC-1.1.4: Custom `UMordecaiAbilitySystemComponent` subclass exists on PlayerState
- [x] AC-1.1.5: `.gitignore` and `README.md` present
- [x] AC-1.1.6: `Scripts/Build.bat` compiles the project successfully

### Tests Required
- [x] `Mordecai.Foundation.GameModeClassExists`
- [x] `Mordecai.Foundation.GameStateClassExists`
- [x] `Mordecai.Foundation.PlayerStateClassExists`
- [x] `Mordecai.Foundation.PlayerControllerClassExists`
- [x] `Mordecai.Foundation.ASCExistsOnPlayerState`
- [x] `Mordecai.Foundation.RequiredPluginsLoaded`

---

## Story 1.2 — Diorama Camera

### Acceptance Criteria
- [x] AC-1.2.1: Camera follows the player character smoothly
- [x] AC-1.2.2: Camera does NOT rotate (fixed orientation at all times)
- [x] AC-1.2.3: Designer-exposed properties: pitch angle, distance, follow speed
- [x] AC-1.2.4: No camera rotation input bindings exist

### Tests Required
- [x] `Mordecai.Camera.FollowsCharacterPosition`
- [x] `Mordecai.Camera.RotationRemainsFixed`
- [x] `Mordecai.Camera.ExposesDesignerProperties`

---

## Story 1.3 — Enhanced Input Setup

### Acceptance Criteria
- [x] AC-1.3.1: Input Action assets exist for: Move, Look/Aim, Sprint, Jump, Dodge, LightAttack, HeavyAttack, Block, Interact, SkillA, SkillB, Throw, WeaponSwap, Items A-D, Pause, Map
- [x] AC-1.3.2: Default Input Mapping Context maps controller (left/right stick) and KBM (WASD/mouse) per bindings doc
- [x] AC-1.3.3: Sprint toggle works on L3 / Shift

### Tests Required
- [x] `Mordecai.Input.AllInputActionsExist`
- [x] `Mordecai.Input.DefaultMappingContextExists`
- [x] `Mordecai.Input.SprintToggleBound`

---

## Story 2.1 — Player Character (Twin-Stick Movement)

### Acceptance Criteria
- [x] AC-2.1.1: Character moves in world space via left stick / WASD (up = north, relative to fixed camera)
- [x] AC-2.1.2: Character rotation driven by right stick / mouse aim direction (independent of movement)
- [x] AC-2.1.3: Sprint toggle increases move speed
- [x] AC-2.1.4: Jump works with gravity and landing
- [x] AC-2.1.5: Placeholder visuals: capsule + arrow component showing facing direction
- [x] AC-2.1.6: Movement feels responsive (tunable accel/decel)

### Tests Required
- [x] `Mordecai.Character.MovesInWorldSpace`
- [x] `Mordecai.Character.RotatesIndependentlyOfMovement`
- [x] `Mordecai.Character.SprintIncreasesSpeed`
- [x] `Mordecai.Character.JumpAppliesGravity`
- [x] `Mordecai.Character.HasPlaceholderVisuals`
- [x] `Mordecai.Character.MovementIsConfigurable`

---

## Story 2.2 — Base Attributes via GAS

### Acceptance Criteria
- [x] AC-2.2.1: `UMordecaiAttributeSet` exists with: Health, MaxHealth, Stamina, MaxStamina, SpellPoints, MaxSpellPoints, Posture, MaxPosture
- [x] AC-2.2.2: Primary attributes stubbed per character_attributes_v1: STR, DEX, END, CON, RES, DIS, INT, WIS, CHA (initialized, not wired to derived stats)
- [x] AC-2.2.3: Attributes initialized with placeholder defaults (Health=100, Stamina=100, SP=10)
- [x] AC-2.2.4: ASC on PlayerState initializes AttributeSet correctly
- [x] AC-2.2.5: Basic debug HUD displays Health%, Stamina, SP

### Tests Required
- [x] `Mordecai.Attributes.AttributeSetHasAllCoreAttributes`
- [x] `Mordecai.Attributes.AttributeSetHasAllPrimaryAttributes` — verifies STR, DEX, END, CON, RES, DIS, INT, WIS, CHA
- [x] `Mordecai.Attributes.DefaultsInitializedCorrectly`
- [x] `Mordecai.Attributes.HealthClampedToMax`
- [x] `Mordecai.Attributes.ASCInitializesAttributeSet`

---

## Story 2.3 — Dev Test Level

### Acceptance Criteria
- [x] AC-2.3.1: `DevTestMap` exists with flat ground + obstacles
- [x] AC-2.3.2: Ramp or ledge for jump testing
- [x] AC-2.3.3: PlayerStart + GameMode spawn `AMordecaiCharacter`
- [x] AC-2.3.4: Set as default editor startup map
- [x] AC-2.3.5: PIE works: character spawns, controls work, camera follows, HUD displays

### Tests Required
- [x] `Mordecai.Level.DevTestMapExists`
- [x] `Mordecai.Level.PlayerSpawnsCorrectly`
- [x] `Mordecai.Level.GameModeIsCorrectClass`

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] `Scripts/Verify.bat` passes (tests + build)
- [x] Code committed and pushed with `[US-001]` prefix

## Playtest
Added 2026-10-22 by the Phase 3 Playability Recovery sweep (`Specs/story_playtests_v1.md`).

- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience). Since US-100 (T1, issue #46) the script plays the enemy-free MovementTestMap instead of DevTestMap, whose Frontliners swarmed the spawn. The PlayerStart (0, 0) and lane L1 (y = -800) are the same, so the waypoints and checks below are unchanged.
- **Tier 4 script:** `Scripts/playtests/us001_movement.py` — run: `py Scripts/playtest_story.py US-001`
- **Steps:** (generated: `py Scripts/playtest_story.py US-001 --plan`; checked: `--lint`)

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `hud_visible` | state widgets -> a CombatHUD, a HealthBar and a StaminaBar widget on screen and the text "10 / 10" (SP bar) | AC-2.2.5 |
| 2 | `spawn_state` | state player, actors -> player.valid && !player.is_dead, within 100 cm of (0, 0), pawn native class MordecaiCharacter | AC-2.3.3 |
| 3 | `imc_active` | state input -> "MordecaiIMC_Runtime" in input.mapping_contexts | AC-1.3.2 |
| 4 | `move_a` | hold A 0.4s -> player.location moves > 100 cm along -Y, < 30 cm across | AC-2.1.1, AC-1.3.2 |
| 5 | `move_w` | hold W 0.4s -> player.location moves > 100 cm along +X, < 30 cm across | AC-2.1.1, AC-1.3.2 |
| 6 | `move_d` | hold D 0.4s -> player.location moves > 100 cm along +Y, < 30 cm across | AC-2.1.1, AC-1.3.2 |
| 7 | `move_s` | hold S 0.4s -> player.location moves > 100 cm along -X, < 30 cm across | AC-2.1.1, AC-1.3.2 |
| 8 | `camera_fixed` | wait_until player.control_rotation == [-50, 0, 0] (+-0.5) after the moves | AC-1.2.2 |
| 9 | `no_camera_rotation_input` | axis Gamepad_RightX=1, Gamepad_RightY=1 held; mouse to (200,100), (1100,620), (640,360) -> player.control_rotation stays [-50, 0, 0] at every sample | AC-1.2.4, AC-1.2.2 |
| 10 | `walk_lane_wp1` | walk_to(0, -650) -> within 60 cm (waypoint to lane L1) | - |
| 11 | `walk_lane_wp2` | walk_to(-700, -650) -> within 60 cm (waypoint to lane L1) | - |
| 12 | `walk_lane_start` | walk_to(-700, -800) -> within 60 cm (lane L1 start) | - |
| 13 | `walk_speed_baseline` | hold W 1s -> dx in [400, 700] cm and wait_until player.speed<1 within 0.75s of the release | AC-2.1.6 |
| 14 | `walk_lane_reset` | walk_to(-700, -800) -> within 60 cm | - |
| 15 | `sprint_shift` | tap LeftShift, hold W 1s -> dx > 700 cm; tap LeftShift again, hold S 1s -> back to a walk (peak player.speed 500..750 cm/s) | AC-1.3.3, AC-2.1.3 |
| 16 | `walk_lane_reset2` | walk_to(-700, -800) -> within 60 cm | - |
| 17 | `sprint_l3` | tap Gamepad_LeftThumbstick (L3), hold W 1s -> dx > 700 cm (then tap L3 again) _(known issue: gamepad stick and L3 mappings use invalid FKey names (Specs/agent_playtest_plan_v1.md))_ | AC-1.3.3, AC-2.1.3 |
| 18 | `jump_space` | tap SpaceBar -> wait_until player.movement_mode=="Falling" within 1s | AC-2.1.4 |
| 19 | `facing_mouse` | mouse 1100 360 (right of the player, +Y) -> wait_until player.rotation yaw 90 (+-30) within 1s _(known issue: facing and aim are pinned to +X by the diorama camera (Specs/agent_playtest_plan_v1.md))_ | AC-2.1.2 |
| 20 | `facing_stick` | axis Gamepad_RightX=-1 held (left, -Y) -> wait_until player.rotation yaw -90 (+-30) within 1s, then release _(known issue: facing and aim are pinned to +X by the diorama camera (Specs/agent_playtest_plan_v1.md); Known issue: gamepad stick and L3 mappings use invalid FKey names (Specs/agent_playtest_plan_v1.md))_ | AC-2.1.2 |
| 21 | `gamepad_move` | axis Gamepad_LeftY=1 for 0.5s -> player.location x rises > 100 cm _(known issue: gamepad stick and L3 mappings use invalid FKey names (Specs/agent_playtest_plan_v1.md))_ | AC-2.1.1, AC-1.3.2 |
