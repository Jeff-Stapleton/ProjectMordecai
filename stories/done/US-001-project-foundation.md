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
