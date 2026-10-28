---
name: unreal-tribal-knowledge
description: >
  Hard-won lessons from debugging UE5/Lyra/Mordecai. Read this before any
  editor-facing, asset-configuration, packaging or playtest work. If you solve a non-obvious problem
  during your session, ADD it here so future agents don't repeat the same fight.
---

# Unreal Tribal Knowledge

Lessons learned the hard way. Each entry is a problem that cost significant debugging time and its root cause + fix.

**IMPORTANT — Contributing rule:** Any agent that encounters a non-obvious problem and solves it MUST add the lesson to this file before finishing the task. Format: `### Short title`, then Problem / Root Cause / Fix sections. This is how we prevent repeating mistakes.

---

## Lyra Experience System

### Experiences must be Blueprint classes, not DataAsset instances

**Problem:** `EXPERIENCE: Wanted to use LyraExperienceDefinition:B_MordecaiDevExperience but couldn't find it, falling back to the default`

**Root cause:** Lyra's `DefaultGame.ini` configures the AssetManager to scan for experiences with `bHasBlueprintClasses=True`. This means the AssetManager only indexes **Blueprint classes** derived from `ULyraExperienceDefinition`, not plain DataAsset instances. If you create an experience via "Miscellaneous > Data Asset" instead of "Blueprint Class", the AssetManager ignores it and the GameMode falls back to `B_LyraDefaultExperience`.

**Fix:** Create experiences as Blueprint classes:
```python
bf = unreal.BlueprintFactory()
bf.set_editor_property('parent_class', unreal.LyraExperienceDefinition)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
new_bp = asset_tools.create_asset('B_MyExperience', '/My/Path', unreal.Blueprint, bf)
```
Then set properties on the CDO via `new_bp.generated_class()` → `get_default_object()` → `set_editor_property()`. Compile twice (properties can reset on first compile).

### Experience scan paths must include your plugin directory

**Problem:** Even with a correct Blueprint experience, it's not found if it lives in a GameFeature plugin content directory.

**Root cause:** The AssetManager scan in `DefaultGame.ini` only looks at `Directories=((Path="/Game/System/Experiences"))` by default. Assets in `/MordecaiCore/System/Experiences/` are invisible.

**Fix:** Add your plugin's experience directory to the scan config in `DefaultGame.ini`:
```ini
+PrimaryAssetTypesToScan=(PrimaryAssetType="LyraExperienceDefinition",...,Directories=((Path="/Game/System/Experiences"),(Path="/MordecaiCore/System/Experiences")),...)
```
Requires an Editor restart after changing. Maps and playlists in the plugin need the same treatment — see the next entry.

### Playlists and Maps in a GameFeature plugin need asset-manager scan paths

**Problem:** A player cannot reach DevTestMap from the front end (`L_LyraFrontEnd` is `GameDefaultMap`): no Mordecai tile is listed, and a playlist `MapID` for DevTestMap does not resolve. In a packaged build the map may not be cooked at all.

**Root cause:** The front end lists playlists (`LyraUserFacingExperienceDefinition`) by asking the AssetManager for that primary asset type, then travels using the playlist's `MapID` (a `Map` primary asset id) and `ExperienceID`. Lyra's stock scans cover only `/Game/UI/Temp` + `/Game/System/Playlists` (playlists) and `/Game/Maps` (maps), so anything under `/MordecaiCore/...` is invisible — same failure as the experience scan above. MordecaiCore also had no playlist asset at all.

**Fix:** In `Config/DefaultGame.ini`, append the plugin paths to the EXISTING scan entries (keep `CookRule=AlwaysCook`) and cook the map explicitly:
```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="Map",...,Directories=((Path="/Game/Maps"),(Path="/MordecaiCore/Game/Maps")),...,CookRule=AlwaysCook))
+PrimaryAssetTypesToScan=(PrimaryAssetType="LyraUserFacingExperienceDefinition",...,Directories=((Path="/Game/UI/Temp"),(Path="/Game/System/Playlists"),(Path="/MordecaiCore/System/Playlists")),...,CookRule=AlwaysCook))

[/Script/UnrealEd.ProjectPackagingSettings]
+MapsToCook=(FilePath="/MordecaiCore/Game/Maps/DevTestMap")
```
Restart the editor, then run `Scripts/create_arena_playlist.py` in the editor (via `ue_remote_exec`; idempotent) to create `/MordecaiCore/System/Playlists/DA_Mordecai_Arena` (`MapID=Map:/MordecaiCore/Game/Maps/DevTestMap`, `ExperienceID=LyraExperienceDefinition:B_MordecaiArenaExperience`, `bShowInFrontEnd=True`). The `Map` primary asset name is the FULL package path (Lyra's own playlists use e.g. `Map:/ShooterCore/Maps/L_ShooterGym`); a short `Map:DevTestMap` never resolves, and the front-end tile fails with "Can't find asset data for MapID". Unlike experiences, playlists are plain DataAsset instances (`bHasBlueprintClasses=False`), so `DataAssetFactory` is correct here. The script FAILS if the map or experience is registered under a different id, and only WARNs if the AssetManager does not list the playlist/map/experience yet — that means the editor has not been restarted since the ini change. Commit the new `.uasset` (see "Python-created content that is never saved and committed does not exist"). Every new Mordecai map needs a playlist (or another player-reachable route) to count as playable.

### ExperienceOverride for PIE (Developer Settings)

> **WARNING — PIE ONLY.** Lyra reads `ExperienceOverride` only when `World->IsPlayInEditor()` (`ALyraGameMode::HandleMatchAssignmentIfNotExpectingOne`). In a packaged build or a standalone `-game` launch it does nothing, and a map that relies on it silently loads `B_LyraDefaultExperience`. Use it only as a PIE convenience — never as the way a map gets its experience. See "Packaged builds load B_LyraDefaultExperience on Mordecai maps" below.

**Problem:** `DefaultGameplayExperience` on World Settings is a `TSoftClassPtr` with `EditDefaultsOnly` — can't be set via Python on instances.

**Root cause:** The property only accepts Blueprint class references and is restricted to archetype editing.

**Fix:** Use `LyraDeveloperSettings::ExperienceOverride` instead — it uses `FPrimaryAssetId` (works with both Blueprints and DataAssets) and has **higher priority** than World Settings for PIE:
```python
ds = unreal.get_default_object(unreal.LyraDeveloperSettings)
asset_id = unreal.PrimaryAssetId()
asset_id.primary_asset_type = unreal.PrimaryAssetType('LyraExperienceDefinition')
asset_id.primary_asset_name = unreal.Name('B_MyExperience')
ds.set_editor_property('experience_override', asset_id)
```
Also set it in `Config/DefaultEditorPerProjectUserSettings.ini` for persistence:
```ini
[/Script/LyraGame.LyraDeveloperSettings]
ExperienceOverride=(PrimaryAssetType="LyraExperienceDefinition",PrimaryAssetName="B_MyExperience")
```

### Packaged builds load B_LyraDefaultExperience on Mordecai maps

**Problem:** DevTestMap worked in PIE for months, but outside PIE (packaged build, standalone) the same map spawned Lyra's shooter pawn, HUD and input — none of the Mordecai systems. Nothing errors; the log just says `Identified experience LyraExperienceDefinition:B_LyraDefaultExperience (Source: Default)`.

**Root cause:** `AMordecaiGameMode::InitGame` forced `B_MordecaiArenaExperience` only inside `if (World->IsPlayInEditor())`, by writing `LyraDeveloperSettings::ExperienceOverride` — which Lyra itself only honors in PIE. DevTestMap's World Settings has no `DefaultGameplayExperience`. Outside PIE nothing named an experience, so Lyra's final fallback won. Lyra's precedence (highest first): URL option `?Experience=` → Developer Settings (**PIE only**) → `-Experience=` command line → World Settings `DefaultGameplayExperience` → `B_LyraDefaultExperience` (source `Default`). If an experience IS named but is not scanned/cooked, you get `EXPERIENCE: Wanted to use ... but couldn't find it, falling back to the default` instead.

**Fix:** `AMordecaiGameMode` has `UPROPERTY FPrimaryAssetId DefaultExperience` (= `LyraExperienceDefinition:B_MordecaiArenaExperience`) and `static bool ShouldUseDefaultExperience(Options, CommandLine, bDeveloperOverrideApplies, bWorldSettingsHasExperience)`. `InitGame` appends `?Experience=<DefaultExperience name>` to `OptionsString` in ALL builds when no other source (URL option, PIE developer override, `-Experience=`, World Settings) names one — so `DefaultExperience` replaces only Lyra's `B_LyraDefaultExperience` fallback. Tests: `Mordecai.GameMode.DefaultExperience.*`.

**Rule:**
- DO NOT make gameplay setup depend on `World->IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or any other editor-only state. Anything set that way is PIE-only — a player never gets it.
- Every Mordecai map MUST resolve the Mordecai experience in a packaged build.
- Verify with `py Scripts/playtest_boot.py` (`--expect-experience`, default `B_MordecaiArenaExperience`). It reads the `Identified experience LyraExperienceDefinition:<name>` log line and fails, naming the experience, if a different one loads. PIE cannot catch this bug.

---

## Blueprint / C++ Subobject Issues

### New C++ default subobjects don't appear on existing Blueprints

**Problem:** Added `CreateDefaultSubobject<UMyComponent>()` to a C++ class, but Blueprints derived from it don't have the component.

**Root cause:** Blueprint CDOs are serialized at creation time. When a new default subobject is added to the parent C++ class later, the Blueprint's serialized CDO doesn't include it. Recompiling the Blueprint doesn't always pick it up.

**Fix options:**
1. **Point PawnData/references at the C++ class** instead of the Blueprint (the C++ CDO always has the component)
2. **Recreate the Blueprint** from the C++ class (nuclear but reliable)
3. For GameFeature-style injection, add the component via `GameFeatureAction_AddComponents` on the Experience instead of the C++ constructor

### BP assets referencing classes in the wrong module won't load

**Problem:** `Failed to load Class /Script/MordecaiCoreRuntime.MordecaiCameraMode_Diorama as Parent for BlueprintGeneratedClass`

**Root cause:** A Blueprint was created when a C++ class lived in module A, then the class was moved to module B. The Blueprint's serialized parent class path still points to module A.

**Fix:** Recreate the Blueprint, or bypass it entirely by referencing the C++ class directly (e.g. set `PawnData.DefaultCameraMode` to the C++ class rather than the broken Blueprint wrapper).

---

## Lyra Pawn / Camera Pipeline

### LyraHeroComponent is required for camera mode and input

**Problem:** Character spawns but has default free-rotation camera instead of the PawnData's configured camera mode. Input from PawnData's InputConfig doesn't work.

**Root cause:** The `LyraHeroComponent` is what binds `DetermineCameraModeDelegate` on `LyraCameraComponent` and initializes input from `PawnData->InputConfig`. Without it, the camera falls back to default behavior. In Lyra's stock setup, this component is injected via `GameFeatureAction_AddComponents` in the Experience — it's NOT on `ALyraCharacter` by default.

**Fix:** Either:
1. Add `ULyraHeroComponent` as a default subobject on your character class
2. Or add a `GameFeatureAction_AddComponents` action to your Experience that injects it

### PawnData drives pawn class and camera, not GameMode::DefaultPawnClass

The `DefaultPawnClass` on the GameMode is a fallback. In practice, Lyra resolves the pawn class from:
1. `PlayerState->PawnData` (per-player override)
2. `Experience->DefaultPawnData->PawnClass` (normal path)
3. `GameMode::DefaultPawnClass` (last resort)

Camera mode comes from `PawnData->DefaultCameraMode`, read by `LyraHeroComponent::DetermineCameraMode()`.

---

## Ability Sets

### Ability set entries with broken class references log errors but don't crash

**Problem:** `GrantedGameplayAbilities[0] on ability set [AS_MordecaiCombat] is not valid`

**Root cause:** The ability set references Blueprint ability classes that can't load (typically because their parent C++ class moved modules). The entries become null but the set still processes.

**Fix:** Clear or rebuild the broken entries:
```python
ab = unreal.load_asset('/MordecaiCore/System/AbilitySets/AS_MordecaiCombat')
ab.set_editor_property('granted_gameplay_abilities', [])
unreal.EditorAssetLibrary.save_asset('/MordecaiCore/System/AbilitySets/AS_MordecaiCombat')
```
Then repopulate with valid C++ or Blueprint ability classes.

---

## Python Editor Scripting Gotchas

### Docstrings with backslashes cause SyntaxError in exec()

**Problem:** `SyntaxError: (unicode error) 'unicodeescape' codec can't decode bytes`

**Root cause:** When running `exec(open(r'path').read())` in the Editor Python console, triple-quoted docstrings containing `\U` (from Windows paths like `\Users`) are interpreted as unicode escapes.

**Fix:** Use `#` comments instead of docstrings in scripts meant for `exec()`, or ensure all paths in docstrings use forward slashes.

### EditDefaultsOnly properties can't be set via Python on instances

**Problem:** `Property 'X' for attribute 'x' on 'Y' cannot be edited on instances`

**Root cause:** UE5 Python enforces `EditDefaultsOnly` restrictions. World Settings actors in levels are considered instances.

**Fix:** Set the property on the CDO via `unreal.get_default_object(ClassName)`, or find an alternative property/config that achieves the same goal (e.g. `LyraDeveloperSettings.ExperienceOverride` instead of `WorldSettings.DefaultGameplayExperience` — but that one is PIE-only; see "Packaged builds load B_LyraDefaultExperience on Mordecai maps"). A CDO write does not fix placed actors that already override the property: read the instances, and see the next entry.

### A placed actor can override its Blueprint's EditDefaultsOnly value, invisibly (N13, issue #18)

**Problem:** The packaged Frontliners had 100/100 health. Their Blueprint's CDO had `DefaultMaxHealth` 200 (AC-054.2), saved and committed, and every script that checked it read 200. The first guess, "the BP edit was not saved" (like N10), was wrong.

**Root cause:** DevTestMap saved `DefaultMaxHealth` 100 on each placed Frontliner, an instance override. A level saves every property in which a placed actor differs from its archetype, the Blueprint CDO. The placed actors never received the CDO writes of the script that set 200: in the map's first commit they also held the native `AIControllerClass` that the same script had set on the CDO. That one was later repaired on the instances, because it is EditAnywhere. `EditDefaultsOnly` hides the property on placed actors in the Details panel and blocks Python writes there, so the stale value can't be seen in the editor UI and can't be written directly on the instance. Reading the CDO says nothing about the placed actors.

**Diagnose:**
- Read the instances, not the CDO: `get_editor_property` works on placed actors. Only writes are refused.
- Or offline: a property name in a `.umap`'s name table means some actor in it serializes that property. DevTestMap (before the fix) had `DefaultMaxHealth` and nothing else of the enemy's.

**Fix (Python, no re-placing):** A Python write to a template (a CDO) also writes every loaded instance that holds the template's current value (`PyWrapperObject.cpp` -> `PropertyAccessUtil::GetArchetypeInstancesInheritingPropertyValue`, whatever the notify mode). So:
1. Set the CDO to the stale value: now every placed actor matches it.
2. Set the CDO back: they all follow, and equal their archetype again.
3. `actor.modify()` each actor first (the propagated write does not dirty the level), then `save_current_level()`.

Use `notify_mode=unreal.PropertyAccessChangeNotifyMode.NEVER` on the CDO writes and the Blueprint package stays clean. See `Scripts/us085_configure_frontliner.py`.

**Guard:** `AMordecaiEnemyCharacter::ValidateContent` logs a `LogMordecaiEnemy` error when an enemy's base stats differ from its class's, so the next one shows in every playtest's `game.log`.

### UInputModifier subobjects created via Python don't serialize

**Problem:** `UInputModifierSwizzleAxis`, `UInputModifierNegate`, `UInputModifierDeadZone` objects created in Python and added to `EnhancedActionKeyMapping.Modifiers` array save as NULL references in the .uasset. The key mappings persist (action + key name survive) but all modifiers are lost on reload.

**Root cause:** UE Python API creates the modifier UObjects but they don't get proper outer/ownership setup required for asset serialization. `EditorAssetLibrary.save_asset()` writes the IMC but the modifier sub-objects are garbage collected or not serialized into the package.

**Fix:** Create input mapping contexts with modifiers **entirely in C++** using `UInputMappingContext::MapKey()` and `NewObject<UInputModifierSwizzleAxis>(IMC)`. This creates proper ownership chains that work at runtime:
```cpp
UInputMappingContext* IMC = NewObject<UInputMappingContext>(this, TEXT("MyIMC_Runtime"));
FEnhancedActionKeyMapping& W = IMC->MapKey(MoveAction, FKey(TEXT("W")));
UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(IMC);
Swizzle->Order = EInputAxisSwizzle::YXZ;
W.Modifiers.Add(Swizzle);
Subsystem->AddMappingContext(IMC, 1);
```
This is a runtime-only IMC (not saved to disk) but it's created in C++ code so it persists across editor restarts via recompilation. For WASD movement with a 2D axis `IA_Move` action, you MUST have Swizzle modifiers on W/S keys — without them, all keys produce input on the X axis and forward/backward movement won't work.

### Python-set config values (LyraDeveloperSettings) don't persist across editor restarts

> **SUPERSEDED for experience selection.** The `IsPlayInEditor()`-gated `InitGame` below is exactly what made packaged builds load `B_LyraDefaultExperience`. DO NOT copy it to choose a map's experience — use the all-builds fix in "Packaged builds load B_LyraDefaultExperience on Mordecai maps". The persistence lesson itself still stands.

**Problem:** Setting `LyraDeveloperSettings.ExperienceOverride` via Python works for the current session but reverts on editor restart. Writing to `DefaultEditorPerProjectUserSettings.ini` also doesn't take effect.

**Root cause:** `ULyraDeveloperSettings` uses `config=EditorPerProjectUserSettings` which has a complex config hierarchy. The editor's saved config in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` overrides the `Default` version, and the editor may overwrite manual edits on startup.

**Fix:** Set the value in **C++ code** instead of config files. Override `GameMode::InitGame()` to set it before the base class processes it:
```cpp
void AMordecaiGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    if (GetWorld() && GetWorld()->IsPlayInEditor())
    {
        ULyraDeveloperSettings* Settings = GetMutableDefault<ULyraDeveloperSettings>();
        if (Settings && !Settings->ExperienceOverride.IsValid())
        {
            Settings->ExperienceOverride = FPrimaryAssetId(
                FPrimaryAssetType(TEXT("LyraExperienceDefinition")),
                FName(TEXT("B_MyExperience")));
        }
    }
    Super::InitGame(MapName, Options, ErrorMessage);
}
```

### Python-set PawnData properties (DefaultCameraMode, InputConfig) don't persist

**Problem:** Setting `PawnData.DefaultCameraMode` or `PawnData.InputConfig` via Python and calling `EditorAssetLibrary.save_asset()` appears to work, but reverts on editor restart.

**Root cause:** The `save_asset` call may not properly dirty the asset for class/soft-class reference properties, or the saved value gets overwritten by other editor processes.

**Fix:** Bypass PawnData asset properties entirely with C++ overrides:
- **Camera:** Rebind `LyraCameraComponent::DetermineCameraModeDelegate` in `BeginPlay` to return your camera mode class directly
- **Input:** Create a programmatic `UInputMappingContext` and bind actions in `SetupPlayerInputComponent` (see "UInputModifier subobjects" entry above)

### Python-created content that is never saved and committed does not exist

**Problem:** US-079 ("playable inventory arena") landed C++, tests and asset scripts (`Scripts/create_test_pickups.py`, `Scripts/place_test_pickups.py`), but its commits contained ZERO `.uasset`/`.umap` files. DevTestMap had nothing to pick up; the newest MordecaiCore asset on disk predated the story. The feature existed only as C++.

**Root cause:** The scripts contain `save_asset` / `save_current_level` calls but were never successfully run to completion, and nobody checked that files appeared on disk or in git. PIE smoke tests ran against whatever was in editor memory, so they passed anyway. `.uasset`/`.umap` are Git LFS files: if they are not committed, no other machine, cook, or packaged build ever sees them.

**Fix:**
- Every asset-creating editor script MUST save what it creates (`unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)`; `save_current_level()` for map edits), then reload from disk and print what it finds. Treat a script that did not print its verification as not run.
- After running it, confirm the files exist on disk (`git status`). Commit the resulting `.uasset`/`.umap` in the SAME branch/PR as the C++ and the script.
- Run `py Scripts/check_content_committed.py` before calling a story done (`--base REF`, default `origin/main`; `--json` for machine output). Exit 1 = FAIL: either asset files are saved on disk but not committed, or the branch adds/modifies an asset-writing script under `Scripts/` with zero `.uasset`/`.umap` changes. Exit 2 = git error / bad ref.
- Put `[no-content]` in a commit message only when the script change genuinely should change no assets (e.g. a refactor of a script whose output is unchanged), and say why in the commit body. It exempts a script only if EVERY commit in the range that touched it carries the tag and it has no uncommitted changes. Never use it to get past a missing asset.

---

## Lyra Input System

### LyraHeroComponent::InitializePlayerInput gates ALL input on PawnData->InputConfig

**Problem:** `DefaultInputMappings` on HeroComponent adds the IMC to Enhanced Input, but no native actions (Move, Look) get bound. The character has the IMC active but WASD doesn't move.

**Root cause:** In `LyraHeroComponent::InitializePlayerInput()`, the entire input binding block — including both `DefaultInputMappings` IMC registration AND `BindNativeAction` calls — is gated inside `if (const ULyraInputConfig* InputConfig = PawnData->InputConfig)`. If `InputConfig` is null (e.g. not persisted on the PawnData asset), the block is skipped entirely and no native actions are bound.

**Fix:** Bind input actions in your own code (e.g. `SetupPlayerInputComponent` override on the character) as a fallback:
```cpp
void AMordecaiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (HeroComponent)
    {
        HeroComponent->EnsureMordecaiInputBound(Cast<APlayerController>(GetController()));
    }
}
```
Where `EnsureMordecaiInputBound` creates a programmatic IMC and calls `EIC->BindAction()` directly.

### DefaultInputMappings on HeroComponent is protected — use a subclass

**Problem:** Can't set `ULyraHeroComponent::DefaultInputMappings` from the character constructor because it's `protected`.

**Root cause:** Standard C++ access control. The array is `UPROPERTY(EditAnywhere)` but `protected`.

**Fix:** Create a subclass (e.g. `UMordecaiHeroComponent : public ULyraHeroComponent`) that sets `DefaultInputMappings` in its constructor. Use `TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(...))` for the `InputMapping` field — NOT `ConstructorHelpers::FObjectFinder` — because GameFeature plugin content isn't mounted at CDO construction time.

### ConstructorHelpers::FObjectFinder fails for GameFeature plugin assets

**Problem:** `ConstructorHelpers::FObjectFinder<UInputMappingContext>(TEXT("/MordecaiCore/Input/IMC_Mordecai"))` silently fails (returns null).

**Root cause:** `ConstructorHelpers` runs during CDO construction, which happens at module load time. GameFeature plugins mount their content directories LATER during engine init. At CDO construction time, `/MordecaiCore/` doesn't exist yet.

**Fix:** Use `TSoftObjectPtr` or `FSoftObjectPath` instead:
```cpp
MordecaiMapping.InputMapping = TSoftObjectPtr<UInputMappingContext>(
    FSoftObjectPath(TEXT("/MordecaiCore/Input/IMC_Mordecai.IMC_Mordecai")));
```
The soft reference stores the path and resolves it later when `LoadSynchronous()` is called at runtime, by which point the GameFeature is active.

### Invalid FKey names fail silently: Mordecai's gamepad sticks were never bound

**Problem:** Gamepad move, gamepad look and L3 sprint can never work in Mordecai, and nothing reports it. The agent bridge's `axis` command on `Gamepad_LeftX`/`Gamepad_LeftY` moves nothing, so `walk_to` uses WASD. Found while mapping Phase 2.

**Root cause:** `UMordecaiHeroComponent::BuildAndBindRuntimeInput` (called from `EnsureMordecaiInputBound`) maps `FKey(TEXT("Gamepad_LeftStick2D"))` (`MordecaiHeroComponent.cpp:167`), `FKey(TEXT("Gamepad_RightStick2D"))` (`:180`) and `FKey(TEXT("Gamepad_LeftThumbstickButton"))` (`:197`). None of these keys exist. The engine names are `Gamepad_Left2D`, `Gamepad_Right2D` and `Gamepad_LeftThumbstick` (`Engine/Source/Runtime/InputCore/Private/InputCoreTypes.cpp:169,172,178`), and no key redirect maps the wrong names. `FKey(TEXT("..."))` accepts any string and `MapKey` accepts the result, so the mapping exists but never matches an event: no error, no warning.

**Fix:** Use the `EKeys::` constants (`EKeys::Gamepad_Left2D`, …) instead of string names. Where a key must come from a string, check it with `FKey::IsValid()` and log or fail loudly (a headless test over the mapping list catches typos). The bridge resolves key names the same way and rejects unknown ones with `bad_key` (`MordecaiAgentBridgeProtocolTests.cpp:410` asserts the broken stick name is not a key).

**Evidence:** the files and lines above; `Specs/agent_bridge_v1.md` §5 and §9.

### Lyra binds the InputConfig's native tags for you: a wrong pairing in content silently rebinds a Mordecai key

**Problem:** Every Left Shift press crouched the player (`Status.Crouching`, walk speed capped at 300 cm/s, jumping refused) although no Mordecai code mentions crouch. It would have broken the sprint and jump playtests once those were bound (N23, US-083).

**Root cause:** `ULyraHeroComponent::InitializePlayerInput` binds `InputTag_Move`, `Look_Mouse`, `Look_Stick`, `Crouch` and `AutoRun` from `PawnData->InputConfig` onto the pawn's input component (`LyraHeroComponent.cpp:285-289`). The arena's `InputData_Mordecai` pairs `IA_Mordecai_Sprint` with `InputTag.Crouch`, and `MordecaiIMC_Runtime` maps Left Shift to that action, so Lyra's `Input_Crouch` -> `ToggleCrouch` fired on Triggered, every frame the key was held (the end state depended on the frame count).

**Fix:** `UMordecaiHeroComponent` overrides `InitializePlayerInput`: after `Super`, `RemoveNativeBindingsForTag` removes the bindings of the config's crouch action to this component (`GetActionEventBindings` + `RemoveBindingByHandle`; collect the handles first). The content entry is still wrong (an EDITOR fix). When a Mordecai key does something unexplained, read the PawnData's `InputConfig` pairs first: Lyra binds them whether or not Mordecai code uses them.

### Light and heavy attacks on one button, and parry on the same button while blocking (Enhanced Input)

Findings from the engine source while binding US-083 (`EnhancedPlayerInput.cpp`, `InputTriggers.cpp`, `EnhancedInputSubsystemInterface.cpp`):
- Put triggers on the **mappings** (`FEnhancedActionKeyMapping::Triggers`, `NewObject<...>(IMC)` right after each `MapKey`; the next `MapKey` invalidates the returned reference), never on the shared IA assets: action-level triggers are copied once per player and edits to an asset are shared state.
- Two actions on one key in one context both fire. **Tap(T)** fires Triggered on the release when held < T; **Hold(T)** at T while held; **HoldAndRelease(T)** on the release when held >= T. Tap and Hold use the same accumulator, so equal thresholds split every press exactly. HoldAndRelease also counts the release frame's delta, so with Tap(T) both can fire on a release right at T: bind the light action first so it activates first and the heavy one is refused (`State.Attacking`). Both report Started on every press: bind Triggered, never Started.
- **Chords:** a mapping with `UInputTriggerChordAction{ChordAction = Block}` is Triggered only while Block is Triggered (not Ongoing), so Block's trigger must sit on Block's mapping in the same (or a higher-priority) context. The engine injects a `UInputTriggerChordBlocker` into the later mappings of the same key, which blocks them only on frames the chorded action is Triggered. Add `UInputTriggerPressed` to the chord mapping so only a fresh press counts (else a held button plus a later block parries). Then the chord is Ongoing while held and ends in Canceled: bind the release on Canceled too.
- A trigger-less Boolean mapping fires Triggered every held frame: bind toggles (sprint) and one-shot actions on Started, or give the mapping a `UInputTriggerPressed`.
- A mouse button pressed and released inside one frame is invisible to Enhanced Input (only button-axis keys get the `KeysPressedThisTick` rescue). The agent bridge holds clicks for at least a frame.

---

## Python Remote Execution

### Remote execution doesn't respond on Windows (multicast on loopback)

**Problem:** `check_editor.py` pings `239.0.0.1:6766` but never gets a pong, even though `netstat` shows the port is bound.

**Root cause:** UE5's default `RemoteExecutionMulticastBindAddress` is `127.0.0.1`. Windows has poor multicast support on the loopback interface — multicast packets sent from external scripts don't get delivered to a loopback-bound listener.

**Fix:** Add to `Config/DefaultEngine.ini`:
```ini
[/Script/PythonScriptPlugin.PythonScriptPluginSettings]
bRemoteExecution=True
RemoteExecutionMulticastBindAddress=0.0.0.0
```
This binds the multicast listener to all interfaces, allowing external Python scripts to communicate. Requires editor restart.

### Remote execution multicast is intermittent on Windows with multiple NICs

**Problem:** `ue_remote_exec.py` works on the first editor launch but fails after closing and relaunching the editor. `netstat` confirms port 6766 is bound. Multicast pings arrive (visible on Hyper-V adapter 172.31.112.1) but no pong comes back.

**Root cause:** On Windows with Hyper-V or WSL virtual adapters, multicast routing is non-deterministic. The OS may route multicast packets through a virtual adapter that the editor's multicast group membership doesn't cover. The bind address is `0.0.0.0` but the actual multicast join may only happen on one interface.

**Workaround:** If remote exec stops working, try:
1. Close and relaunch the editor (sometimes fixes routing)
2. Disable Hyper-V/WSL virtual adapters temporarily
3. Use the editor's built-in Python console (`Window > Developer Tools > Output Log > Cmd: Python`) to run smoke test scripts directly
4. If running smoke tests, paste the script content directly into the Python console

---

## UMG Widget Blueprints

### Python-created Widget Blueprints are empty shells — no visual content

**Problem:** Widget Blueprints created via Python `WidgetBlueprintFactory` + `create_asset()` exist and load correctly, but the HUD is completely invisible at runtime. All `BindWidgetOptional` properties are null. Tests pass because they test static helpers, not the widget tree.

**Root cause:** `WidgetBlueprintFactory.create_asset()` only creates a Blueprint with the correct parent C++ class. It does NOT add any visual content to the UMG designer tree (no Canvas Panel, no ProgressBars, no TextBlocks). Since every visual element uses `BindWidgetOptional` (not `BindWidget`), there's no compile error — properties silently resolve to null, and all `if (Prop) { ... }` blocks are skipped.

**Fix:** Add `NativeOnInitialized()` overrides to each widget class that check `if (WidgetTree && !WidgetTree->RootWidget)` and call `BuildDefaultContent()` to programmatically create the visual tree using `WidgetTree->ConstructWidget<>()`. Manually assign the constructed widgets to the `BindWidgetOptional` UPROPERTY members. This fires after `Initialize()` but before `TakeWidget()/RebuildWidget()` builds the Slate tree, so the programmatic content is picked up.

**Key detail:** Local variables named `Slot` shadow `UWidget::Slot` and cause C4458 errors (treated as errors by UE5). Use names like `BarSlot` or `TextSlot` instead.

### WidgetTree->ConstructWidget works for UWidget, CreateWidget for UUserWidget

When building widget trees programmatically:
- Use `WidgetTree->ConstructWidget<T>()` for primitive UMG types: `UCanvasPanel`, `UProgressBar`, `UTextBlock`, `UHorizontalBox`, `UImage`
- Use `CreateWidget<T>(this)` for `UUserWidget` subclasses (they have their own WidgetTree)
- Child `UUserWidget` instances can be added to a parent's `UCanvasPanel` via `AddChildToCanvas()` — they render as nested widgets

### CommonUI Menu input mode blocks every unbound key: a menu that closes through an Enhanced Input action can never be closed

**Problem:** The first Tier 4 run (`Saved/Playtests/20261020-144643-us079`) opened the Mordecai pause menu with a real I press, then tapped a real Escape: the menu was still open 3 s later (check `menu_closes_escape` FAIL, `commonui_input_mode='Menu'`, `game_input_allowed=False`; `bridge_03.png` still shows "Paused" and the inventory tab). Pressing I does not close it either. A player who opens the menu cannot close it. Tier 2 never noticed: `verify_pie_inventory.py` toggles the menu with a direct call (`toggle_pause_menu_to_tab`).

**Root cause:** `UMordecaiPauseMenuWidget` activates with `InputConfig = ELyraWidgetInputMode::Menu` (`MordecaiPauseMenuWidget.cpp:38`). In Menu mode the CommonUI action router returns `BlockGameInput` for every key without a UI action binding (`Engine/Plugins/Runtime/CommonUI/.../CommonUIActionRouterBase.cpp:647`), so the Enhanced Input `Pause`/`Inventory` actions that OPEN the menu never fire while it is open. And the widget is not a back handler (`bIsBackHandler` defaults to false, `CommonActivatableWidget.h:189`, and no Mordecai code sets it), so CommonUI's own back action (Escape, gamepad back) does nothing either.

**Fix:**
- A CommonUI menu must close itself through CommonUI, not through a gameplay input action: set `bIsBackHandler = true` so CommonUI's default back action deactivates it (→ `NativeOnDeactivated` → close), and register UI action bindings (`RegisterUIActionBinding`) for any other close key such as I — or use an input mode that lets those keys through (`GameAndMenu`).
- In this project the back action is the data-table one (CommonInput `bEnableEnhancedInputSupport` is off): `/Game/UI/B_CommonInputData` → `DT_UniversalActions` row `DefaultBack` = keyboard `Escape`, gamepad `Gamepad_FaceButton_Right` (checked by `Mordecai.UI.PauseMenu.DefaultBackActionIsEscape`). The binding is registered in `UCommonActivatableWidget::NativeConstruct`, so it exists only for back handlers.
- **Fixed** for the pause menu: `bIsBackHandler = true` in the `UMordecaiPauseMenuWidget` constructor (tests `Mordecai.UI.PauseMenu.IsCommonUIBackHandler`, `...BackActionDeactivationClosesMenu`). Real Escape closed the menu in `Saved/Playtests/20261020-192656-us079` (check `menu_closes_escape` PASS, `bridge_04.png` shows the arena and HUD again). I still does not close it.
- Prove it with a Tier 4 run that presses the key. Never close the menu by a direct call in a playtest: that hides exactly this bug.

**Evidence:** `Saved/Playtests/20261020-144643-us079/report.md` check 44 and `bridge_03.png` (bug); `Saved/Playtests/20261020-192656-us079/report.md` (fix); the files and lines above; `Specs/agent_bridge_v1.md` §9.

---

## Testing Gaps

### Headless tests (NullRHI) cannot catch visual/integration bugs

**Problem:** All 370 headless tests passed, but the combat HUD was completely invisible during playtesting. Tests exercised static helper methods (CalcHealthPercent, CalcStaminaTier, etc.) which don't require a widget tree or viewport.

**Root cause:** NullRHI doesn't render anything. Widget trees created from empty Blueprints have null BindWidgetOptional bindings, but since tests never construct the widget or add it to a viewport, the null bindings are never exercised. The tests proved the math was correct but not that anything was visible.

**Fix:** PIE smoke tests (`Scripts/verify_pie_*.py`) run during Play In Editor via Python remote execution. They query live game state: widget visibility, child widget existence, spawn chain integrity, camera mode, component presence. Run via `py Scripts/run_pie_verify.py` after headless tests pass.

**Rule:** PIE smoke tests must pass for **every story, regardless of Execution Mode**. This is now in `agent_rules_v2.md` and `CLAUDE.md` Step 4b.

**PIE has the same blind spot one tier up.** PIE runs against in-editor memory with editor-only settings, so it cannot catch packaging/cook problems (maps or assets not scanned or not cooked), PIE-only code paths (`IsPlayInEditor()`, `LyraDeveloperSettings::ExperienceOverride`), or content that was never saved/committed. Passing PIE is NOT proof a player can use the feature. Any story with player-facing behavior also needs the **Tier 3 packaged playtest** (`py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`), and every AC that needs input needs the **Tier 4 real-input playtest** (`py Scripts/playtests/usXXX_<name>.py`) on the same package — see "Packaged Builds & Playability". A lower tier never substitutes for a higher one.

### Smoke tests that call gameplay functions directly pass on unplayable features

**Problem:** `Scripts/verify_pie_inventory.py` passed while no player could pick anything up or open the inventory.

**Root cause:** The script teleported the pawn next to each pickup and called `interaction.try_pickup_focused()` and `subsystem.toggle_pause_menu_to_tab('inventory')` directly. It never pressed F or I, so the input mappings and the real key path were never exercised — and it ran against unsaved in-editor content. It proved the functions work, not that the feature is reachable.

**Fix:**
- Label any check that teleports the pawn, sets state, or calls gameplay/UI functions directly as **logic/diagnostic** — in the script output and in the story's completion notes. It is useful for diagnosis; it is NEVER acceptance evidence for a player-facing AC.
- DO NOT describe a feature as "playable" or "working in game" from Tier 1/Tier 2 results.
- Acceptance of a player-facing AC requires the player path: the Tier 3 packaged playtest for what is on screen, and for anything that needs input, a passing Tier 4 real-input playtest (`py Scripts/playtests/usXXX_<name>.py`, real key/mouse events through the in-game agent bridge) on the same fresh package. The Tier 4 version of this very script, `Scripts/playtests/us079_inventory.py`, presses F and I for real — see "Real-Input Playtests (Agent Bridge, Tier 4)".

### Ticking FTimerManager by hand in a test: one tick per frame, and the first only activates timers

**Problem:** A test activated a melee ability (its windup is a 0.1 s `SetTimer`) and called `World->GetTimerManager().Tick(0.15f)`; the windup never fired. A second `Tick` in the same test did nothing either.

**Root cause:** A timer set before the manager's first tick of the current frame goes into `PendingTimerSet`, which is moved to the active heap only at the END of `FTimerManager::Tick`. And `Tick` returns at once when `HasBeenTickedThisFrame()` (`LastTickedFrame == GFrameCounter`), so a test that never advances frames gets exactly one effective tick.

**Fix:** Advance the frame between ticks: `for (int32 Frame = 0; Frame < 2; ++Frame) { ++GFrameCounter; World->GetTimerManager().Tick(Dt); }` (see `Tests/Recovery/MordecaiRecoveryEnemyTests.cpp`). A test world from `UWorld::CreateWorld` has no game instance, so `GetTimerManager()` is the world's own manager.

### Mordecai has no Lyra teams: every pawn is FGenericTeamId::NoTeam

**Problem (issue #12, N7):** Enemies swarmed the player in Attack state and never did damage. Their attack profiles were fine (no "has no attack profiles" error in PIE).

**Root cause:** The Mordecai experiences add no `ULyraTeamCreationComponent`, so the player and every enemy have `FGenericTeamId::NoTeam` (255). `UMordecaiHitDetectionSubsystem::AreActorsFriendly` compared the ids, found them equal, and filtered the player out of every enemy swing (and every enemy out of the player's).

**Fix:** Sides come from the `Mordecai.Team.Enemy` tag whenever an enemy is involved (both tagged = friends, one tagged = foes). Only when neither is an enemy does the Lyra team id decide, and there NoTeam == NoTeam is right: two co-op players are friends, and friendly fire stays off (AC-003.9). Any new hostility check must do the same — never compare `GetGenericTeamId()` alone for player vs enemy. (A first fix that also made "NoTeam never friendly" turned co-op friendly fire on; `Mordecai.Recovery.Enemy.PlayerSweepsSpareOtherPlayers` guards it.)

### The player's ASC does not exist at BeginPlay or PossessedBy: bind in OnAbilitySystemInitialized

**Problem (N31, found by the issue #12 playtest):** once enemies hit the player, Health sat at 0 for a minute and the player never died. `AMordecaiCharacter` bound its Health→death delegate in `BeginPlay` and `PossessedBy`.

**Root cause:** the player's ASC lives on the PlayerState; the pawn reaches it through `ULyraPawnExtensionComponent`, which `ULyraHeroComponent` initializes on DataAvailable→DataInitialized. For a locally controlled pawn that step waits for `Pawn->InputComponent`, created by `APawn::PawnClientRestart` → `SetupPlayerInputComponent`, after `PossessedBy` (PlayerController.cpp: `PossessedBy` before `ClientRestart`). So `GetAbilitySystemComponent()` is null at both points and the `if (ASC)` bind silently did nothing. game.log order proves it: "Weapon loadout initialized" (PossessedBy) → "Removed 1 Lyra crouch binding" (InitializePlayerInput, right after InitializeAbilitySystem) → "SetupPlayerInputComponent called".

**Fix:** override `ALyraCharacter::OnAbilitySystemInitialized` / `OnAbilitySystemUninitialized` (protected virtuals the base constructor already binds) and bind/unbind there, idempotently. Do not register a second delegate from the same object with `OnAbilitySystemInitialized_RegisterAndCall`: it de-dupes by object, so the second one is silently dropped. Anything else on the player pawn that needs the ASC must do the same or resolve it lazily (the HUD binds through the PlayerState in `AMordecaiPlayerController::CreateCombatHUD`). The US-053 tests missed it because they call `HandlePlayerDeath()` directly on a character without an ASC; `Mordecai.Recovery.Player.*` drives Health to 0 through a pawn-extension-attached PlayerState ASC.

### An enemy that attacks from beyond its weapon's reach swings at air forever

**Problem (issue #12, second cause):** with hostility fixed, a player standing at the spawn still sat at 100/100 for 60 s: all three Frontliners were in Attack and Rooted at ~195 cm.

**Root cause:** the AI enters Attack at `AttackRange` (200, AC-054.2) and roots; the slash's hit shape reaches 150 cm (arc radius, measured centre to centre by `IsPointInArcSector`). Whether an enemy hits depended on where it happened to stop.

**Fix:** `AMordecaiEnemyAIController::GetEffectiveAttackRange()` = `min(AttackRange, UMordecaiGA_MeleeAttack::GetAttackReach())` (arc radius; circle offset + radius; capsule/box length). Any new attacker AI must derive its engage distance from the attack's actual hit shape, never from a separate range number alone.

### UnrealEditor-Cmd exits 0 even when automation tests fail

**Problem:** `Scripts\RunTests.bat` printed "ALL TESTS PASSED" while a test was failing. Its pass/fail decision was the editor's exit code.

**Root cause:** With `-TestExit="Automation Test Queue Empty"`, `UnrealEditor-Cmd` quits through a normal exit request when the queue empties and returns 0 whatever the results were. Failures appear only in the log, as `Test Completed. Result={Fail}` followed by `LogAutomationController: Error:` lines. (Found 2026-10-18; a deliberately failing test gave exit code 0.)

**Fix:** Run tests with `py Scripts/run_tests.py [filter]` (`RunTests.bat` now delegates to it). It parses the log: exit 1 if any test failed, exit 2 if no tests matched or the queue never finished, exit 0 only if at least one test ran and all succeeded. Never judge a raw `UnrealEditor-Cmd ... Automation RunTests` run by its exit code.

### `TestEqual` on strings is case-INSENSITIVE — a case test written with it can never fail

**Problem:** New tests asserting that the bridge emits lowercase ids (`TestEqual(TEXT("active_tab is lowercase"), Actual, FString(TEXT("inventory")))`) PASSED in the red phase while the code still emitted `"Inventory"`.

**Root cause:** `FAutomationTestBase::TestEqual(const TCHAR*, const FString&, const FString&)` and the `TCHAR*`/`FStringView` overloads compare **ignoring case**; `TestEqualInsensitive` simply forwards to them (`Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h:2016-2054`, `Private/Misc/AutomationTest.cpp:2300-2308`).

**Fix:** Use `TestEqualSensitive(What, Actual, Expected)` whenever case matters (ids, GUID strings, anything a tool compares exactly). Its `What` is `const TCHAR*`, so pass `*FString::Printf(...)` for formatted names. And always watch a new test fail before you trust it.

### Headless test worlds: spawned actors skip PostInitializeComponents, native widgets skip NativeOnInitialized

**Problem:** An enemy spawned in a `UWorld::CreateWorld(EWorldType::Game, false)` test world had an ASC with no attribute sets registered (`GetSet<UMordecaiAttributeSet>()` null), and a `NewObject`'d `UMordecaiInventoryWidget` had no widget tree, so the tests only ever exercised fallback branches.

**Root cause:** `AActor::PostActorConstruction` runs `PreInitializeComponents` / `InitializeComponents` / `PostInitializeComponents` only when `World->AreActorsInitialized()`, which `InitializeActorsForPlay` sets (`World.cpp:5912`); without it the ASC never runs `InitializeComponent` (where it registers the owner's attribute-set subobjects) and controllers are never added to the world's controller list. `UUserWidget::Initialize` calls `NativeOnInitialized` only when the widget has a valid player context (`UserWidget.cpp:173`), and `FLocalPlayerContext::IsValid` needs a `ULocalPlayer` whose controller is in the world with `PC->Player` set (`LocalPlayer.cpp:97-110`).

**Fix (see `FBridgeGameTestEnv` in `Source/LyraGame/Tests/Playtest/MordecaiAgentBridgeStateTests.cpp`):**
- After creating the world and its world context, call `World->InitializeActorsForPlay(FURL())` (no game mode needed). Lyra player states additionally need a game state: spawn `ALyraGameState` first (`ALyraPlayerState::PostInitializeComponents` checks it).
- Spawn an `APlayerController`, `NewObject<ULocalPlayer>(GEngine, ULocalPlayer::StaticClass())`, set `PC->Player = LocalPlayer; LocalPlayer->PlayerController = PC;`. Create widgets with `NewObject<T>(World)`, `SetPlayerContext(FLocalPlayerContext(LocalPlayer, World))`, `Initialize()`. Clear `PC->Player` and `LocalPlayer->PlayerController` before destroying the world.
- Spawn AI pawns deferred with `AutoPossessAI = EAutoPossessAI::Disabled` to avoid spawning AI controllers.
- Still not possible headlessly: anything that needs painted geometry or a native `SWindow` (on-screen visibility, widget centres) — test the math with a hand-built `FGeometry::MakeRoot(Size, FSlateLayoutTransform(Scale, Pos))` instead.

### Python names differ from C++/Blueprint names — and smoke tests hid it

**Problem:** `verify_pie_inventory.py` had never run past widget creation: `unreal.WidgetBlueprintLibrary` and `unreal.SubsystemBlueprintLibrary` raised AttributeError. `verify_pie_pause_menu.py` recorded PASS when it couldn't reach its subsystem ("acceptable" fallbacks), and read the property `b_can_pause_game`, which doesn't exist.

**Root cause:** Editor Python uses each class's `ScriptName` and strips prefixes:
- `UWidgetBlueprintLibrary` is `unreal.WidgetLibrary` (`meta=(ScriptName="WidgetLibrary")`). Its `Create` is `BlueprintInternalUseOnly`, so Python can't create widgets with it at all.
- `UAbilitySystemBlueprintLibrary` is `unreal.AbilitySystemLibrary`. Read granted abilities with `asc.get_all_abilities()` + `unreal.AbilitySystemLibrary.get_gameplay_ability_from_spec_handle(asc, handle)` (returns `(ability, is_instance)`); `asc` from `player_state.get_lyra_ability_system_component()`. `GetOwnedGameplayTags` is not a UFUNCTION.
- `USubsystemBlueprintLibrary`'s getters are internal-only, and `GameInstance.get_subsystem` isn't exposed.
- A bool UPROPERTY `bCanPauseGame` is `can_pause_game` in Python.
- `get_all_widgets_of_class(world, cls, True)` returns only top-level widgets, so it misses anything nested in the HUD layout.

**Fix:**
- Find a game-instance subsystem in PIE with `next((s for s in unreal.ObjectIterator(unreal.MyGameInstanceSubsystem) if s.get_outer() == unreal.GameplayStatics.get_game_instance(world)), None)`.
- Pass `top_level_only=False` to find nested HUD widgets.
- Check a name in the live editor (`[n for n in dir(unreal) if 'Widget' in n]`) or grep the header for `ScriptName` before using it.
- Never write a check that records PASS when it can't reach what it is checking. That is a false pass, and it hid both bugs above for months.

### check_editor.py must bind receive socket to multicast port 6766

**Problem:** `check_editor.py` always timed out waiting for the editor, even though `ue_remote_exec.py` worked fine on the same machine.

**Root cause:** `check_editor.py` bound its receive socket to a random port (`sock.bind(("0.0.0.0", 0))`), but UE sends pong responses to the multicast group on port 6766. Since the socket wasn't listening on port 6766, it never received the pong. Meanwhile, `ue_remote_exec.py` correctly used `recv_sock.bind(("", MCAST_PORT))`.

**Fix:** Use separate send/receive sockets. Bind receive socket to `MCAST_PORT` (6766). Add multi-interface fallback for Windows with multiple NICs (Hyper-V, WSL). Match the socket setup in `ue_remote_exec.py`.

### run_pie_verify.py multiline scripts fail in ExecuteStatement mode

**Problem:** `wait_for_pie_world()` always timed out even though PIE was running and `ue_remote_exec.py` worked for single-line commands.

**Root cause:** The check script was a multiline Python string passed as a command-line argument to `ue_remote_exec.py`. Without `--file`, the script uses `exec_mode = "ExecuteStatement"` which only accepts single statements. Multiline scripts cause `SyntaxError: multiple statements found while compiling a single statement`.

**Fix:** Wrap multiline scripts in `exec("...")` to make them a single statement, or use `--file` mode. For inline commands in orchestrator scripts, prefer single-line commands or `exec()` wrapping.

### PIE smoke test scripts must scope all UObject references in a function

**Problem:** After PIE stops, editor logs `FPyReferenceCollector` errors: "World from PIE level still referenced. This reference is preventing the old World from being GC'd."

**Root cause:** Smoke test scripts (verify_pie_hud.py, verify_pie_spawn.py) stored PIE world, controller, pawn, and component references in module-level Python variables. The Python interpreter holds these references after PIE ends, preventing UE's garbage collector from cleaning up PIE objects.

**Fix:** Wrap all test logic in a `def _run():` function so all UObject references are local variables that go out of scope when the function returns. Call `_run()` then `del _run` to clean up the function itself:
```python
def _run():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    # ... all checks using world, pc, pawn ...

_run()
del _run
```

---

## Packaged Builds & Playability

**Rule: playable in the packaged build, or it isn't done.** Verification ladder — a lower tier NEVER substitutes for a higher one:
1. **Tier 1** — headless automation tests (logic)
2. **Tier 2** — PIE smoke tests, `py Scripts/run_pie_verify.py` (editor integration)
3. **Tier 3** — packaged playtest: `py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`. Open every screenshot with the Read tool and state what is visibly on screen.
4. **Tier 4** — real-input playtest on the same fresh package: `py Scripts/playtests/usXXX_<name>.py` (first: `us079_inventory.py`), through the in-game agent bridge (`UMordecaiAgentBridgeSubsystem`, non-Shipping, `-AgentBridge`; `Specs/agent_bridge_v1.md`). Ad hoc: `py Scripts/agent_bridge.py ...` or the `mordecai-play` MCP server (`.mcp.json`). An AC that needs input is accepted only here. Open every `bridge_NN.png`.

Content counts only when saved AND committed: `py Scripts/check_content_committed.py` must pass too. Cite the run dirs (`Saved/Playtests/<run>/report.md` + screenshot paths, for Tier 3 and Tier 4) in the story's completion notes and the body of the closing commit made after the playtests (that commit touches only `stories/`). Report failures as failures — never paper over them. All of this is local Python scripts only (no BuildGraph/Horde/CI/Gauntlet).

### Content loaded by a path string from C++ is not cooked: works in PIE, missing for players

**Problem:** The first packaged playtest (2026-10-18) booted the Mordecai arena with **0 weapons, 0 spells, no combat HUD, and no pause, spell or weapon-cycle keys**, while every headless and PIE test passed. `game.log` had 16 `LogStreaming: Warning: LoadPackage: SkipPackage: /MordecaiCore/...` and `Failed to find object` lines, for `WBP_CombatHUD`, `DA_Weapon_*`, `DA_Spell_*` and `IA_Mordecai_Spell1-4/Pause/WeaponCyclePrev`. They are Warnings, so nothing flagged them.

**Root cause:** Mordecai C++ loads these with literal paths: `LoadClass<...>(nullptr, TEXT("/MordecaiCore/UI/WBP_CombatHUD.WBP_CombatHUD_C"))`, `LoadObject` for starting weapons, spells and input actions (23 `TEXT("/MordecaiCore/...")` sites). The cooker only cooks maps, AlwaysCook primary assets, and whatever those *reference*. A path string inside a function body is not a reference, so those assets never reached the paks. PIE loads everything straight from disk, so it can't see the problem.

**Fix:**
- `Config/DefaultGame.ini` `[/Script/UnrealEd.ProjectPackagingSettings]`: `+DirectoriesToAlwaysCook=(Path="/MordecaiCore")`. All game-feature content is cooked.
- `playtest_boot.py`'s `content_cooked` check now FAILs (exit 1) on any `SkipPackage` / `Failed to find object` for a `/MordecaiCore` path.
- **Rule for new code:** content referenced only from C++ must either live under a cooked directory (all of `/MordecaiCore` is now), or be referenced through a `UPROPERTY` soft/hard pointer on an asset the cooker already includes (a data asset, a Blueprint CDO, experience/pawn data). Never assume "it loads in PIE" means "it ships".

### Writing a loaded asset into a native CDO crashes packaged builds (GC "Disregard for GC" fatal)

**Problem:** After all of `/MordecaiCore` was cooked, the packaged game crashed about 6s after load: `Fatal error: [GarbageCollectionVerification.cpp] Encountered 4 object(s) breaking Disregard for GC assumptions`, preceded by `LogGarbage: Warning: Disregard for GC object MordecaiGA_Blink /Script/LyraGame.Default__MordecaiGA_Blink referencing MordecaiSpellDataAsset /MordecaiCore/Spells/DA_Spell_Blink... which is not part of root set` (x4 spells). PIE never crashed. Before cooking was fixed, the assets were missing, so the pointers were null and the spells were silently absent instead.

**Root cause:** `AMordecaiPlayerController::GrantCombatAbilities` wrote the loaded `DA_Spell_*` into each spell ability's **class default object** (`SpellClass->GetDefaultObject<...>()->SpellData = Data`). In cooked builds, native CDOs created at startup live in the disregard-for-GC pool. GC verification requires them to reference only rooted or disregarded objects, and a runtime-loaded data asset is neither, so it is fatal. The editor doesn't use that pool, so PIE can't catch it. (Mutating a CDO is also shared state across every instance and every PIE session.)

**Fix:** Never write runtime objects into a CDO (`GetDefaultObject`, `GetMutableDefault`) in game code. Pass per-grant data through the spec instead: `FGameplayAbilitySpec Spec(AbilityClass, Level, INDEX_NONE, /*SourceObject*/ DataAsset)`. The instanced ability picks it up in `OnGiveAbility` (`UMordecaiGA_SpellBase::TakeSpellDataFromSpec`, which skips the CDO; GAS calls `OnGiveAbility` on the CDO when no instance exists).

**Melee had the same latent bug (issue #40), fixed the same way.** A `TArray` cannot be a SourceObject, so the melee grant's SourceObject is the controller that owns `MeleeAttackProfiles`, and `UMordecaiGA_MeleeAttack::TakeAttackProfilesFromSpec` copies them in `OnGiveAbility` (it skips the CDO and keeps profiles the ability class already has). Don't write the instance after `GiveAbility` instead (the enemy pattern): that reaches only the server's instance. An owning client builds its own instance of a `ReplicateNo` ability when the spec replicates (`FGameplayAbilitySpec::PostReplicatedAdd` -> `ASC::OnGiveAbility`), and only that call sees the spec.

**A native CDO write doesn't even reach new instances.** `NewObject` of a native class, with its own CDO as the template, copies only `Config` properties from the CDO (`FObjectInitializer::InitProperties` fast path in `UObjectGlobals.cpp`; `UClass::GetPropertyLinkFlags` in `Class.cpp`). Every other property comes from the C++ constructor. So the melee CDO write never reached the `UMordecaiGA_MeleeAttack` instance (the red run of the melee grant tests showed it); it only planted the crash. The same holds for actors spawned from a native class (`SpawnActor` uses the CDO as the template). Blueprint classes are no safer: their instances copy the Blueprint-owned properties, but native-owned ones only from a custom list of values that differed from the native parent's CDO when the class loaded (`UBlueprintGeneratedClass::InitPropertiesFromCustomList`). A runtime CDO write does not reliably reach new instances of any class. Pass per-grant data through the spec.

### Desktop screenshots are black when the workstation is locked

**Problem:** The first packaged playtest (2026-10-18) loaded the Mordecai experience correctly, but all three window screenshots were pure black (mean 0.0, stddev 0.0), and every sample point reported another window on top of the game.

**Root cause:** The workstation was locked (`LogonUI.exe` running). While the session is locked, desktop capture (`PIL.ImageGrab`, BitBlt) returns black for the whole screen, and `SetForegroundWindow` fails. Unattended agent runs (nightly cron) usually happen on a locked machine, so desktop capture can never be the agent's eyes.

**Fix:** The game captures itself. `UMordecaiPlaytestCaptureSubsystem` (non-Shipping only, enabled by `-PlaytestCapture`) waits for `ULyraExperienceManagerComponent::IsExperienceLoaded()`, then calls `FScreenshotRequest::RequestScreenshot(<dir>/engine_NN.png, bShowUI=true, false)` on a schedule (`-PlaytestShotDir= -PlaytestShots= -PlaytestShotDelay= -PlaytestShotInterval=`). The viewport reads its own rendered frame, so it doesn't depend on the desktop. `playtest_boot.py` passes these flags automatically and treats `engine_NN.png` as the primary evidence. Quick check for a locked session: `Get-Process LogonUI`.

### A game executable built months ago is not "the game"

**Problem:** Months of stories were marked done, but `Binaries/Win64/LyraGame.exe` dated from 2026-04-09 and `PackagedBuilds/` was empty — the game had never been packaged. Everything after ~US-057 had only ever run in the editor; nobody had seen it the way a player would.

**Root cause:** Nothing in the loop produced a packaged build, and nothing tied a build to the commit being delivered. A stale exe or an editor session keeps "working" while the shipped code/content diverges.

**Fix:**
- Package from the commit you are delivering: `py Scripts/package_playtest.py` (Development Win64, iterative cook, cooks `L_LyraFrontEnd` + `DevTestMap`). Flags: `--clean` (full cook, no `-iterate`), `--all-maps` (adds `-CookAll -CookMapsOnly`: every map, plus the two defaults), `--config Development|DebugGame|Shipping` (Shipping strips dev features; DebugGame's binary is `LyraGame-Win64-DebugGame.exe`, run it with `playtest_boot.py --exe`), `--dry-run` (print the UAT command). Commit first so the build is not dirty.
- It writes `PackagedBuilds/Playtest/build_info.json` (`commit`, `branch`, `dirty`, `dirty_files`, `dirty_count`, `dirty_fingerprint`, `source_changed_during_build`, `built_at`, `config`, `maps`, `all_maps`, `game_exe`, `uat_log`) only on success, and logs UAT to `Saved/Playtests/package-<YYYYmmdd-HHMMSS>.log`. On failure, read that log — do not guess.
- Boot it with `py Scripts/playtest_boot.py` (`--map`, `--frontend`, `--expect-experience`, `--timeout`, `--settle`, `--shots`, `--shot-interval`, `--fail-on-errors`, `--keep-running`, `--exe`). Its staleness gate exits 2 with `STALE BUILD` if `build_info.json` is missing/unreadable, the build commit != `git rev-parse HEAD`, the tree changed while UAT ran, packaged binaries/paks are newer than `built_at`, a clean build now has uncommitted changes under `Source/`, `Plugins/`, `Content/`, `Config/` or the `.uproject`, or a dirty build's dirty-file list or uncommitted content no longer matches. Fix it by committing and re-packaging. `--allow-stale` only downgrades the gate to a warning in the report — use it for diagnosis, NEVER as evidence for a story.
- Exit codes: `0` all checks pass; `1` the game failed (wrong experience, crash, timeout) — fix the game; `2` infrastructure (exe missing, stale build, window not found, git error) — fix the build/environment.
- The real binary is `PackagedBuilds/Playtest/Windows/ProjectMordecai/Binaries/Win64/LyraGame.exe`; `PackagedBuilds/Playtest/Windows/LyraGame.exe` is only a bootstrapper. Never use `Binaries/Win64/LyraGame.exe` as playtest evidence.
- Packaging is slow (first package can take an hour+; iterative cooks are faster). That cost is accepted — the "fast" loop shipped nothing playable.

### `-game` on the editor binaries does not load the Mordecai experience — use the packaged build

**Problem:** To smoke-test the agent bridge without packaging, the game was started from the editor binaries in standalone mode: `UnrealEditor.exe ProjectMordecai.uproject /MordecaiCore/Game/Maps/DevTestMap -game -windowed ... -AgentBridge`. The bridge's `ping` reported `"experience": "B_LyraDefaultExperience"`: no pickup was ever focused, no `[F] Pick up` prompt, no inventory, and I did nothing. The same map loads `B_MordecaiArenaExperience` in PIE and in the packaged build.

**Root cause:** `game.log` shows `LogLyraExperience: Error: EXPERIENCE: Wanted to use LyraExperienceDefinition:B_MordecaiArenaExperience but couldn't find it, falling back to the default)` followed by `Identified experience LyraExperienceDefinition:B_LyraDefaultExperience (Source: Default)`. `AMordecaiGameMode::DefaultExperience` does name the experience, but in `-game` mode on the editor binaries `LyraAssetManager` cannot find the GameFeature plugin's experience at that point, so Lyra falls back. (Why the asset manager misses it in this mode was not chased further.)

**Fix:** Never use `UnrealEditor.exe <uproject> <map> -game` as a real-game check, and never as evidence. Package (`py Scripts/package_playtest.py`) and run the packaged build (`playtest_boot.py`, the Tier 4 scripts, `agent_bridge.py launch`). Whatever launches the game, check the experience first (`ping.experience`, or the `Identified experience` log line): `B_LyraDefaultExperience` means you are not testing Mordecai.

**Evidence:** the C++ bridge smoke run of 2026-10-20 (first `ping`: `experience: B_LyraDefaultExperience`; the log lines above).

---

## Real-Input Playtests (Agent Bridge, Tier 4)

Lessons from building and first running the in-game agent bridge (`UMordecaiAgentBridgeSubsystem`, `Scripts/agent_bridge.py`, `Scripts/mordecai_play_mcp.py`, `Scripts/playtests/us079_inventory.py`; contract `Specs/agent_bridge_v1.md`). The first real Tier 4 run, `Saved/Playtests/20261020-144643-us079` (a diagnostic `--allow-stale` build), found two bugs: the FName case lesson below, and "CommonUI Menu input mode blocks every unbound key" under UMG Widget Blueprints. Both were invisible at Tier 1, Tier 2 and against the Python fake bridge.

### Packaged builds do not preserve FName case: `FName("inventory").ToString()` returned "Inventory"

**Problem:** In the first Tier 4 run, tapping I really did open the pause menu on the inventory tab (`bridge_02.png`), but `wait_until('pause_menu.open==true && pause_menu.active_tab=="inventory"')` timed out: the packaged game reported `active_tab='Inventory'` (check `menu_opens_inventory` FAIL, five dependent checks SKIPped). Tier 1, PIE and the 189 offline Python tests against the fake bridge all saw `inventory`.

**Root cause:** `WITH_CASE_PRESERVING_NAME` is defined as `WITH_EDITORONLY_DATA` (`Engine/Source/Runtime/Core/Public/UObject/NameTypes.h:33`), which is 0 in a cooked game. Without it an FName keeps ONE spelling per case-insensitive name: whichever was registered first. The reflected parameter name `Inventory` of `UMordecaiInventoryWidget::BindToInventory(UMordecaiInventoryComponent* Inventory, ...)` (`MordecaiInventoryWidget.h:187`, registered by the generated code at module startup) registered before the tab id `FName(TEXT("inventory"))`, so the tab id's `ToString()` is `"Inventory"`. The `character` tab becomes `"Character"` the same way (`ACharacter`). The bridge first exported `GetActiveTabId().ToString()` as-is (it now lowercases it through `CanonicalNameId`), and `wait_until`'s string `==` is case-sensitive (`ValuesEqual` in `MordecaiAgentBridgePredicate.cpp`). Editor builds (Tier 1, PIE) preserve case, so they show the spelling you typed.

**Fix:**
- Never compare FName-derived strings case-sensitively across PIE and the package — in game code, tests, bridge state or playtest scripts. Compare `FName`s as `FName`s (case-insensitive), or lowercase both sides.
- Code that exports an FName as text (JSON state, save data, anything a tool string-compares) emits a canonical string: `.ToString().ToLower()`, or keep the id as an `FString` in the first place. Never rely on `ToString()` returning the spelling you wrote.
- Tier 4 scripts compare ids lowercase. A headless test can check the canonicalization, but cannot reproduce the bug (editor builds preserve case).

**Evidence:** `Saved/Playtests/20261020-144643-us079/report.md` check 36; the files and lines above.

### Lyra's loading screen eats ALL Slate input outside the editor — wait for `ping.ready`

**Problem:** Input sent to the packaged game as soon as the log shows `OnExperienceLoadComplete` never reaches the game, although the same input works in PIE. The bridge's first `ping` after the experience loaded reported `loading_screen: true`; `ready` came about 2-3 s later.

**Root cause:** CommonLoadingScreen registers `FLoadingScreenInputPreProcessor` while the loading screen is shown. It eats every key, analog, mouse-move, button and wheel event by returning `CanEatInput()`, which is `!GIsEditor` (`Plugins/CommonLoadingScreen/Source/CommonLoadingScreen/Private/LoadingScreenManager.cpp:99-123`) — so it never eats input in PIE. The screen also lingers `HoldLoadingScreenAdditionalSecs = 2.0f` after the experience loads (`CommonLoadingScreenSettings.h:39`). A player sees the loading screen and waits; a script does not.

**Fix:** Send input only once the bridge's `ping.ready` is true (`experience_loaded && !loading_screen && has_pawn`). `Bridge.launch()` waits for it, so `agent_bridge.py launch`, the playtest scripts and the MCP `launch_game` do too. Don't bypass the preprocessor: it is the player's path. (Same root as "Keep `--settle` at 3s or more" for Tier 3 screenshots.)

**Evidence:** the files and lines above; `Specs/agent_bridge_v1.md` §4 `ping`.

### Injecting real input: the ways it crashes or silently does nothing

**Problem:** The obvious ways to inject input into a running game crash it, get dropped without a trace, or click the wrong place on a locked desktop. (Found from the engine source while building the agent bridge.)

**Root cause and fix:**
1. **`FInputKeyEventArgs` with a null `Viewport` crashes.** `FInputKeyEventArgs::CreateSimulated(...)` defaults to `Viewport = nullptr` and `InputDevice = INPUTDEVICEID_NONE` (`Engine/Source/Runtime/Engine/Private/InputKeyEventArgs.cpp:30-37`), and `UGameViewportClient::RemapControllerInput` dereferences `InOutEventArgs.Viewport->IsPlayInEditorViewport()` (`GameViewportClient.cpp:690`). Always pass the real viewport. Better: don't inject at the viewport at all, because that skips the Slate input preprocessors and focus routing a player's input goes through. Inject Slate events (`FSlateApplication::ProcessKeyDownEvent`/`ProcessKeyUpEvent`/`ProcessAnalogInputEvent`/`ProcessMouse*Event`), as the bridge does.
2. **`INPUTDEVICEID_NONE` input is silently dropped.** `Config/DefaultInput.ini:34` sets `bFilterInputByPlatformUser=True`, so `APlayerController::InputKey` returns false for any device that the player's platform user does not own (`PlayerController.cpp:2412-2416`), and `INPUTDEVICEID_NONE` maps to no user. Use `IPlatformInputDeviceMapper::Get().GetDefaultInputDevice()` for gamepad events (`FMordecaiAgentBridgeInput::SendKey` / `SendAnalog` in `MordecaiAgentBridgeInput.cpp`).
3. **Mouse clicks on a locked desktop need Slate's in-memory cursor, and Slate takes it back mid-click.** The OS cursor cannot be moved while the session is locked (`SetCursorPos` fails there), so hover and click hit-testing use the wrong position. Switch Slate's cursor user to a virtual cursor with `FSlateApplication::UsePlatformCursorForCursorUser(false)` and move that. Slate swaps back to the OS cursor **synchronously inside your own mouse-down** when CommonInput switches the input method to MouseAndKeyboard (any earlier gamepad key; `UCommonInputSubsystem::RecalculateCurrentInputType`, `CommonInputSubsystem.cpp:298-303`), and after more than 3 real OS mouse moves (`SlateApplication.cpp:6211`). The next synthesized move then lands at the OS cursor, the pressed `SButton` (the captor) gets `OnMouseLeave`, and `SButton::OnMouseButtonUp` does not click because it is not hovered. So: re-install the virtual cursor after every injected mouse event and every frame while a button is down, and send a **non-synthetic** `ProcessMouseMoveEvent` at the click position right before the up (the captor gets `OnMouseEnter` again, `SlateApplication.cpp:5725-5736`). Report "virtual cursor" from live Slate state (the cursor user's cursor != `GetPlatformCursor()`), never from a flag you set.
4. **An inactive (backgrounded or locked) app ignores device input** unless `FSlateApplication::SetHandleDeviceInputWhenApplicationNotActive(true)`. Restore user-0 focus to the game viewport only when the user-0 focus path holds **no live widget at all** — not merely when `GetUserFocusedWidget(0)` is null (that only pins the leaf; when a focused button is destroyed, a player's keys still bubble along the live part of the path, `SlateUser.cpp:160-163`, `WidgetPath.h:240`) — and never while the CommonUI router is in Menu mode.
5. **Auto-repeat like the devices, not "every held key".** A Windows keyboard repeats only the most recently pressed key (pressing another key moves the repeat; the first never resumes), so after a `FlushPressedKeys` only that key is re-pressed through `Input.AutoReconcilePressedEventsOnFirstRepeat`. XInput repeats every held button on its own schedule (`InitialButtonRepeatDelay` 0.2 s, `ButtonRepeatDelay` 0.1 s from Input.ini, `XInputInterface.cpp:51-55, 339-354`). And send each analog axis **once per frame**: `UPlayerInput` sums the samples it gets in a frame (`PlayerInput.cpp:324`), so a second sample doubles the value. A real pad also sends digital threshold buttons (`Gamepad_LeftTrigger`, `Gamepad_LeftStick_Up`, ...) when an axis passes 30/255 or the thumb dead zones (`XInputInterface.cpp:260-292`); bindings on those keys never fire from analog events alone.

**Evidence:** the files and lines above; `Specs/agent_bridge_v1.md` §1 and §4 (`key`, `mouse`).

### Reading SLATE_SCOPE members of `FSlateUser` from game code (cursor, focus path)

**Problem:** The bridge needed to know, live, whether Slate's cursor user is on the in-memory cursor, and whether user 0's focus path still holds any live widget. `FSlateUser::GetCursor()`, `GetWeakFocusPath()`, `GetFocusPath()` and `HasValidFocusPath()` exist but do not compile from LyraGame: "cannot access protected member".

**Root cause:** They are declared under `SLATE_SCOPE:` (`Framework/Application/SlateUser.h:129`), which `SlateScope.h` defines as `public` only inside the Slate module and `protected` everywhere else. There is no public equivalent (`GetUserFocusedWidget` pins only the leaf; nothing public tells you which cursor the user has).

**Fix:** Read them through a pointer to the inherited member formed via a derived class (legal per C++ [class.protected]); the derived type is never instantiated and nothing in Slate changes. See `FSlateUserReadAccess` in `Source/LyraGame/Mordecai/Playtest/MordecaiAgentBridgeInput.cpp`:
```cpp
struct FSlateUserReadAccess : public FSlateUser
{
	FSlateUserReadAccess() = delete;
	static TSharedPtr<ICursor> CursorOf(const FSlateUser& User)
	{
		TSharedPtr<ICursor> (FSlateUser::*Getter)() const = &FSlateUserReadAccess::GetCursor;
		return (User.*Getter)();
	}
};
// virtual cursor active: CursorOf(*Slate.GetCursorUser()) != Slate.GetPlatformCursor()
```
Use it read-only. Prefer a public API whenever one exists.

### The `mcp` Python SDK 2.x moved FastMCP: `mcp.server.fastmcp` raises ModuleNotFoundError

**Problem:** `from mcp.server.fastmcp import FastMCP`, the import in every 1.x example, raises `ModuleNotFoundError` after `pip install mcp` (2.3.0).

**Root cause:** In 2.x the FastMCP server became `MCPServer` in `mcp.server.mcpserver`; `mcp/server/fastmcp.py` is a stub that only raises. The constructor's positional argument order changed as well.

**Fix:**
- `from mcp.server.mcpserver import MCPServer, Image` and `from mcp.server.mcpserver.exceptions import ToolError`. Pass every constructor argument except `name` by keyword: `MCPServer(name, version=..., instructions=...)`.
- Declare tools with `@mcp.tool(name=..., structured_output=False, description=...)`.
- Raise `ToolError(message)` for anything the model must see (a refused STALE build, a gameplay timeout, a lost connection).
- stdout is the stdio protocol channel: log to stderr only.
- Pin it: `Scripts/requirements.txt` has `mcp>=2.3,<3`. Install once with `py -m pip install -r Scripts/requirements.txt`; `mordecai_play_mcp.py` exits 2 with that hint when the SDK is missing.

**Evidence:** `Scripts/mordecai_play_mcp.py:64-65` (imports), `:496` (`MCPServer(...)`), `:500` (first `@mcp.tool`); `Scripts/requirements.txt`.

---

## Build System

### UBA memory loop kills compilation indefinitely on high-memory machines

**Problem:** `Build.bat` hangs forever. UBT log shows thousands of `UbaSessionServer - Killed process Module.LyraGame.1.cpp (Compile [x64]) (<1ms) - Low on memory` lines. The DLL never gets linked.

**Root cause:** Unreal Build Accelerator (UBA) manages parallel compilation and has a kill threshold at ~95% of system RAM. On machines with 60+ GB RAM and heavy background usage (Claude Code, browsers, etc.), the threshold is exceeded before even a single `cl.exe` instance finishes compiling a unity file (~500-600 MB). UBA kills the process and immediately retries, creating an infinite retry loop. No compilation units ever complete.

**Fix:** Bypass UBA with the `-NoUBA` flag and limit parallelism to avoid swapping:
```powershell
& "Engine\Build\BatchFiles\Build.bat" LyraEditor Win64 Development `
  -Project="path\to\ProjectMordecai.uproject" `
  -WaitMutex -FromMsBuild -NoUBA -MaxParallelActions=4
```
This uses UBT's built-in Parallel executor instead of UBA. `-MaxParallelActions=4` keeps memory usage under control. A full 22-action rebuild completes in ~75 seconds vs. hanging forever with UBA.

**The same loop hits `package_playtest.py` at the `LyraGame.exe` link (2026-10-23, issue #40).** UAT builds through UBA and takes no `-NoUBA`. The game target's unity compiles all succeeded, then `UbaSessionServer - Killed process LyraGame.exe (Link [x64]) ... Low on memory (34.5gb/36.2gb)` repeated about 141,000 times until the 2 h tool timeout killed the run (`Saved/Playtests/package-20261023-091641.log`). The monolithic link pushes commit charge about 15 GB above an idle ~20 GB, past UBA's threshold, on a 32 GB machine with a small system-managed page file. Without UBA the same link took 24 s, because the page file can grow. Fix: build the game target first with the same target arguments UAT uses, then package; UAT then finds it up to date:
```powershell
& "Engine\Build\BatchFiles\Build.bat" LyraGame Win64 Development `
  -Project="path\to\ProjectMordecai.uproject" -remoteini="path\to\ProjectMordecai" `
  -WaitMutex -FromMsBuild -NoUBA -MaxParallelActions=4
py Scripts/package_playtest.py
```
The loop never fails the build, so watch the UAT log for `Low on memory` and stop early instead of waiting out the timeout.

### Unity builds leak file-scope `using namespace` across test cpp files

**Problem:** Adding a new automation test file with a file-scope `using namespace MyNewTestHelpers;` broke compilation of an *unrelated, untouched* test file (`error C2668: ambiguous call to overloaded function` for helper names like `MakeWidget`/`MakeDef` that exist in several test-helper namespaces).

**Root cause:** LyraGame compiles with Unity builds — multiple `.cpp` files under `Source/LyraGame/Tests/` are concatenated into one translation unit. Every file-scope `using namespace` directive from earlier files in the unity blob leaks into later files. Two helper namespaces both exposing `MakeWidget()` become ambiguous in any file compiled after both.

**Fix:** In test files, never put `using namespace XyzTestHelpers;` at file scope. Put the using-directive *inside each `RunTest` function body* (function-scope using does not leak), or fully qualify the helper calls. Also give the helper namespace a unique per-file name.

### TWeakObjectPtr assignment needs the complete type, even in a helper header

**Problem:** `WeakObjectPtrTemplates.h ... error C2679: binary '=': no operator found which takes a right-hand operand of type 'AMyActor *'` from a UHT-generated test-helper header that only forward-declared the actor.

**Root cause:** Assigning `TWeakObjectPtr<T> = T*` requires `T` to be a complete type (the conversion goes through `UObject*`). A forward declaration compiles the member fine but fails at the assignment site.

**Fix:** Include the actor's real header in any header whose inline functions assign into a `TWeakObjectPtr` of it.
