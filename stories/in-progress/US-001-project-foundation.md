# US-001: Project Foundation & Initial Player Character

## Execution Mode
**Mode:** HEADLESS

## Overview
Get a running UE 5.7 project with a controllable player character in a test level. Twin-stick controls, fixed diorama camera, GAS wired up with base attributes. This is the foundation everything else builds on.

---

## Story 1.1 — Project Scaffolding

### Acceptance Criteria
- [ ] AC-1.1.1: UE 5.7 C++ project compiles with zero errors and zero warnings in game module
- [ ] AC-1.1.2: Plugins enabled: GameplayAbilities, GameplayTags, GameplayTasks, EnhancedInput
- [ ] AC-1.1.3: Base classes exist: `AMordecaiGameMode`, `AMordecaiGameState`, `AMordecaiPlayerState`, `AMordecaiPlayerController`
- [ ] AC-1.1.4: Custom `UMordecaiAbilitySystemComponent` subclass exists on PlayerState
- [ ] AC-1.1.5: `.gitignore` and `README.md` present
- [ ] AC-1.1.6: `Scripts/Build.bat` compiles the project successfully

### Tests Required
- [ ] `Mordecai.Foundation.GameModeClassExists`
- [ ] `Mordecai.Foundation.GameStateClassExists`
- [ ] `Mordecai.Foundation.PlayerStateClassExists`
- [ ] `Mordecai.Foundation.PlayerControllerClassExists`
- [ ] `Mordecai.Foundation.ASCExistsOnPlayerState`
- [ ] `Mordecai.Foundation.RequiredPluginsLoaded`

---

## Story 1.2 — Diorama Camera

### Acceptance Criteria
- [ ] AC-1.2.1: Camera follows the player character smoothly
- [ ] AC-1.2.2: Camera does NOT rotate (fixed orientation at all times)
- [ ] AC-1.2.3: Designer-exposed properties: pitch angle, distance, follow speed
- [ ] AC-1.2.4: No camera rotation input bindings exist

### Tests Required
- [ ] `Mordecai.Camera.FollowsCharacterPosition`
- [ ] `Mordecai.Camera.RotationRemainsFixed`
- [ ] `Mordecai.Camera.ExposesDesignerProperties`

---

## Story 1.3 — Enhanced Input Setup

### Acceptance Criteria
- [ ] AC-1.3.1: Input Action assets exist for: Move, Look/Aim, Sprint, Jump, Dodge, LightAttack, HeavyAttack, Block, Interact, SkillA, SkillB, Throw, WeaponSwap, Items A-D, Pause, Map
- [ ] AC-1.3.2: Default Input Mapping Context maps controller (left/right stick) and KBM (WASD/mouse) per bindings doc
- [ ] AC-1.3.3: Sprint toggle works on L3 / Shift

### Tests Required
- [ ] `Mordecai.Input.AllInputActionsExist`
- [ ] `Mordecai.Input.DefaultMappingContextExists`
- [ ] `Mordecai.Input.SprintToggleBound`

---

## Story 2.1 — Player Character (Twin-Stick Movement)

### Acceptance Criteria
- [ ] AC-2.1.1: Character moves in world space via left stick / WASD (up = north, relative to fixed camera)
- [ ] AC-2.1.2: Character rotation driven by right stick / mouse aim direction (independent of movement)
- [ ] AC-2.1.3: Sprint toggle increases move speed
- [ ] AC-2.1.4: Jump works with gravity and landing
- [ ] AC-2.1.5: Placeholder visuals: capsule + arrow component showing facing direction
- [ ] AC-2.1.6: Movement feels responsive (tunable accel/decel)

### Tests Required
- [ ] `Mordecai.Character.MovesInWorldSpace`
- [ ] `Mordecai.Character.RotatesIndependentlyOfMovement`
- [ ] `Mordecai.Character.SprintIncreasesSpeed`
- [ ] `Mordecai.Character.JumpAppliesGravity`
- [ ] `Mordecai.Character.HasPlaceholderVisuals`
- [ ] `Mordecai.Character.MovementIsConfigurable`

---

## Story 2.2 — Base Attributes via GAS

### Acceptance Criteria
- [ ] AC-2.2.1: `UMordecaiAttributeSet` exists with: Health, MaxHealth, Stamina, MaxStamina, SpellPoints, MaxSpellPoints, Posture, MaxPosture
- [ ] AC-2.2.2: Primary attributes stubbed per character_attributes_v1: STR, DEX, END, CON, RES, DIS, INT, WIS, CHA (initialized, not wired to derived stats)
- [ ] AC-2.2.3: Attributes initialized with placeholder defaults (Health=100, Stamina=100, SP=10)
- [ ] AC-2.2.4: ASC on PlayerState initializes AttributeSet correctly
- [ ] AC-2.2.5: Basic debug HUD displays Health%, Stamina, SP

### Tests Required
- [ ] `Mordecai.Attributes.AttributeSetHasAllCoreAttributes`
- [ ] `Mordecai.Attributes.AttributeSetHasAllPrimaryAttributes` — verifies STR, DEX, END, CON, RES, DIS, INT, WIS, CHA
- [ ] `Mordecai.Attributes.DefaultsInitializedCorrectly`
- [ ] `Mordecai.Attributes.HealthClampedToMax`
- [ ] `Mordecai.Attributes.ASCInitializesAttributeSet`

---

## Story 2.3 — Dev Test Level

### Acceptance Criteria
- [ ] AC-2.3.1: `DevTestMap` exists with flat ground + obstacles
- [ ] AC-2.3.2: Ramp or ledge for jump testing
- [ ] AC-2.3.3: PlayerStart + GameMode spawn `AMordecaiCharacter`
- [ ] AC-2.3.4: Set as default editor startup map
- [ ] AC-2.3.5: PIE works: character spawns, controls work, camera follows, HUD displays

### Tests Required
- [ ] `Mordecai.Level.DevTestMapExists`
- [ ] `Mordecai.Level.PlayerSpawnsCorrectly`
- [ ] `Mordecai.Level.GameModeIsCorrectClass`

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] `Scripts/Verify.bat` passes (tests + build)
- [ ] Code committed and pushed with `[US-001]` prefix
