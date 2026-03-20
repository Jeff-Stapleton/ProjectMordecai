# TODO: Editor Work Required for US-001

These items require the Unreal Editor and cannot be created from CLI.
Complete them in order after verifying the C++ code compiles.

---

## 1. Input Actions (.uasset)

Create Input Action assets in `MordecaiCore/Content/Input/Actions/`:

- [ ] `IA_Mordecai_Move` (Value Type: Axis2D) - WASD / Left Stick
- [ ] `IA_Mordecai_Look_Mouse` (Value Type: Axis2D) - Mouse delta
- [ ] `IA_Mordecai_Look_Stick` (Value Type: Axis2D) - Right Stick
- [ ] `IA_Mordecai_Sprint` (Value Type: Digital/Bool) - Shift / L3
- [ ] `IA_Mordecai_Jump` (Value Type: Digital/Bool) - Space / South button
- [ ] `IA_Mordecai_Dodge` (Value Type: Digital/Bool) - stub
- [ ] `IA_Mordecai_LightAttack` (Value Type: Digital/Bool) - LMB / Right Trigger
- [ ] `IA_Mordecai_HeavyAttack` (Value Type: Digital/Bool) - RMB / Right Bumper
- [ ] `IA_Mordecai_Block` (Value Type: Digital/Bool) - stub
- [ ] `IA_Mordecai_Interact` (Value Type: Digital/Bool) - E / West button
- [ ] `IA_Mordecai_SkillA` (Value Type: Digital/Bool) - Q
- [ ] `IA_Mordecai_SkillB` (Value Type: Digital/Bool) - R
- [ ] `IA_Mordecai_Throw` (Value Type: Digital/Bool) - G
- [ ] `IA_Mordecai_WeaponSwap` (Value Type: Digital/Bool) - Tab / Y button
- [ ] `IA_Mordecai_ItemA-D` (Value Type: Digital/Bool) - 1-4 / D-pad
- [ ] `IA_Mordecai_Pause` (Value Type: Digital/Bool) - Escape / Start
- [ ] `IA_Mordecai_Map` (Value Type: Digital/Bool) - M / Select

**Alternative**: You can reuse Lyra's existing IA assets (IA_Move, IA_Look_Mouse, IA_Look_Stick) and only create Mordecai-specific ones for combat/skills.

## 2. Input Mapping Context (.uasset)

- [ ] Create `IMC_Mordecai` in `MordecaiCore/Content/Input/`
  - Map all Input Actions to their keys/buttons
  - Controller: Left Stick → Move, Right Stick → Look_Stick
  - KBM: WASD → Move, Mouse → Look_Mouse, Shift → Sprint
  - See `Docs/INPUT_BINDINGS.md` for full mapping table

## 3. Input Config (ULyraInputConfig DataAsset)

- [ ] Create `InputData_Mordecai` in `MordecaiCore/Content/Input/`
  - Map each IA to its corresponding Gameplay Tag (e.g., IA_Mordecai_Move → InputTag.Move)
  - This is what ULyraInputComponent reads for native action bindings

## 4. Camera Mode Blueprint

- [ ] Create `BP_CameraMode_Diorama` (Blueprint subclass of UMordecaiCameraMode_Diorama)
  - Set PitchAngle = -50
  - Set YawAngle = 0
  - Set Distance = 1800
  - Set FollowSpeed = 8
  - Set FieldOfView = 60

## 5. Pawn Data (ULyraPawnData DataAsset)

- [ ] Create `PawnData_Mordecai` in `MordecaiCore/Content/System/`
  - PawnClass = ALyraCharacter (or a BP subclass — see below)
  - InputConfig = InputData_Mordecai (from step 3)
  - DefaultCameraMode = BP_CameraMode_Diorama (from step 4)
  - AbilitySets = (create a basic set or leave empty for now)

## 6. Character Blueprint

- [ ] Create `BP_MordecaiCharacter` (Blueprint subclass of ALyraCharacter)
  - Add UMordecaiHeroComponent (replaces the default ULyraHeroComponent)
  - Configure DefaultInputMappings on the hero component to include IMC_Mordecai
  - Add ArrowComponent for facing direction visualization
  - Set capsule as visible for placeholder visuals
  - Disable mesh component or set to invisible

## 7. Experience Definition (.uasset)

- [ ] Create `B_MordecaiDevExperience` in `MordecaiCore/Content/System/Experiences/`
  - GameFeaturesToEnable: ["MordecaiCore"]
  - DefaultPawnData: PawnData_Mordecai
  - Actions: Add necessary GameFeatureActions (AddComponents, AddInputMapping, etc.)

## 8. Dev Test Map

- [ ] Create `DevTestMap` in `MordecaiCore/Content/Game/Maps/`
  - Flat ground plane (large BSP or landscape)
  - Several BSP box obstacles of varying sizes
  - A ramp and a ledge for jump testing
  - PlayerStart actor
  - Configure World Settings:
    - GameMode Override → LyraGameMode
    - Default Experience → B_MordecaiDevExperience

## 9. Debug HUD Widget Blueprint

- [ ] Create `WBP_MordecaiDebugHUD` (Blueprint subclass of UMordecaiDebugHUD)
  - Add Text blocks for Health, Stamina, SP, Posture
  - Bind text to the BlueprintReadOnly properties
  - OR: simply rely on the PrintString debug output from the C++ base

## 10. Project Settings

- [ ] Set DevTestMap as the default editor startup map in Project Settings → Maps & Modes
- [ ] Verify MordecaiCore GameFeature plugin is enabled/registered
- [ ] Verify MordecaiTags.ini tags appear in the Gameplay Tags editor

---

## Order of Operations

1. Compile C++ (verify zero errors)
2. Create Input Actions (step 1)
3. Create IMC_Mordecai (step 2) and InputData_Mordecai (step 3)
4. Create Camera Mode BP (step 4)
5. Create PawnData_Mordecai (step 5)
6. Create Character BP (step 6)
7. Create Experience (step 7)
8. Create DevTestMap (step 8)
9. Create Debug HUD (step 9)
10. Configure project settings (step 10)
11. PIE test: character spawns, twin-stick works, camera follows, HUD shows attributes
