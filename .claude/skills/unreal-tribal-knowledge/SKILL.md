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

**Fix:** Set the property on the CDO via `unreal.get_default_object(ClassName)`, or find an alternative property/config that achieves the same goal (e.g. `LyraDeveloperSettings.ExperienceOverride` instead of `WorldSettings.DefaultGameplayExperience` — but that one is PIE-only; see "Packaged builds load B_LyraDefaultExperience on Mordecai maps").

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

---

## Testing Gaps

### Headless tests (NullRHI) cannot catch visual/integration bugs

**Problem:** All 370 headless tests passed, but the combat HUD was completely invisible during playtesting. Tests exercised static helper methods (CalcHealthPercent, CalcStaminaTier, etc.) which don't require a widget tree or viewport.

**Root cause:** NullRHI doesn't render anything. Widget trees created from empty Blueprints have null BindWidgetOptional bindings, but since tests never construct the widget or add it to a viewport, the null bindings are never exercised. The tests proved the math was correct but not that anything was visible.

**Fix:** PIE smoke tests (`Scripts/verify_pie_*.py`) run during Play In Editor via Python remote execution. They query live game state: widget visibility, child widget existence, spawn chain integrity, camera mode, component presence. Run via `py Scripts/run_pie_verify.py` after headless tests pass.

**Rule:** PIE smoke tests must pass for **every story, regardless of Execution Mode**. This is now in `agent_rules_v2.md` and `CLAUDE.md` Step 4b.

**PIE has the same blind spot one tier up.** PIE runs against in-editor memory with editor-only settings, so it cannot catch packaging/cook problems (maps or assets not scanned or not cooked), PIE-only code paths (`IsPlayInEditor()`, `LyraDeveloperSettings::ExperienceOverride`), or content that was never saved/committed. Passing PIE is NOT proof a player can use the feature. Any story with player-facing behavior also needs the **Tier 3 packaged playtest** (`py Scripts/package_playtest.py`, then `py Scripts/playtest_boot.py`) — see "Packaged Builds & Playability". A lower tier never substitutes for a higher one.

### Smoke tests that call gameplay functions directly pass on unplayable features

**Problem:** `Scripts/verify_pie_inventory.py` passed while no player could pick anything up or open the inventory.

**Root cause:** The script teleported the pawn next to each pickup and called `interaction.try_pickup_focused()` and `subsystem.toggle_pause_menu_to_tab('inventory')` directly. It never pressed F or I, so the input mappings and the real key path were never exercised — and it ran against unsaved in-editor content. It proved the functions work, not that the feature is reachable.

**Fix:**
- Label any check that teleports the pawn, sets state, or calls gameplay/UI functions directly as **logic/diagnostic** — in the script output and in the story's completion notes. It is useful for diagnosis; it is NEVER acceptance evidence for a player-facing AC.
- DO NOT describe a feature as "playable" or "working in game" from Tier 1/Tier 2 results.
- Acceptance of a player-facing AC requires the player path: Tier 3 packaged playtest now; Tier 4 real-input playtests (agent bridge, Phase 2) when available.

### UnrealEditor-Cmd exits 0 even when automation tests fail

**Problem:** `Scripts\RunTests.bat` printed "ALL TESTS PASSED" while a test was failing. Its pass/fail decision was the editor's exit code.

**Root cause:** With `-TestExit="Automation Test Queue Empty"`, `UnrealEditor-Cmd` quits through a normal exit request when the queue empties and returns 0 whatever the results were. Failures appear only in the log, as `Test Completed. Result={Fail}` followed by `LogAutomationController: Error:` lines. (Found 2026-10-18; a deliberately failing test gave exit code 0.)

**Fix:** Run tests with `py Scripts/run_tests.py [filter]` (`RunTests.bat` now delegates to it). It parses the log: exit 1 if any test failed, exit 2 if no tests matched or the queue never finished, exit 0 only if at least one test ran and all succeeded. Never judge a raw `UnrealEditor-Cmd ... Automation RunTests` run by its exit code.

### Python names differ from C++/Blueprint names — and smoke tests hid it

**Problem:** `verify_pie_inventory.py` had never run past widget creation: `unreal.WidgetBlueprintLibrary` and `unreal.SubsystemBlueprintLibrary` raised AttributeError. `verify_pie_pause_menu.py` recorded PASS when it couldn't reach its subsystem ("acceptable" fallbacks), and read the property `b_can_pause_game`, which doesn't exist.

**Root cause:** Editor Python uses each class's `ScriptName` and strips prefixes:
- `UWidgetBlueprintLibrary` is `unreal.WidgetLibrary` (`meta=(ScriptName="WidgetLibrary")`). Its `Create` is `BlueprintInternalUseOnly`, so Python can't create widgets with it at all.
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
4. **Tier 4** — real-input playtests through an in-game agent bridge exposed as an MCP server (**Phase 2 — planned, NOT available yet**).

Content counts only when saved AND committed: `py Scripts/check_content_committed.py` must pass too. Cite the run dir (`Saved/Playtests/<run>/report.md` + screenshot paths) in the story's completion notes and the body of the closing commit made after the playtest (that commit touches only `stories/`). Report failures as failures — never paper over them. All of this is local Python scripts only (no BuildGraph/Horde/CI/Gauntlet).

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

**Fix:** Never write runtime objects into a CDO (`GetDefaultObject`, `GetMutableDefault`) in game code. Pass per-grant data through the spec instead: `FGameplayAbilitySpec Spec(AbilityClass, Level, INDEX_NONE, /*SourceObject*/ DataAsset)`. The instanced ability picks it up in `OnGiveAbility` (`UMordecaiGA_SpellBase::TakeSpellDataFromSpec`, which skips the CDO; GAS calls `OnGiveAbility` on the CDO when no instance exists). **Same latent bug:** the melee `AttackProfiles` CDO write in `GrantCombatAbilities`, which runs only when `MeleeAttackProfiles` is set.

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

### Unity builds leak file-scope `using namespace` across test cpp files

**Problem:** Adding a new automation test file with a file-scope `using namespace MyNewTestHelpers;` broke compilation of an *unrelated, untouched* test file (`error C2668: ambiguous call to overloaded function` for helper names like `MakeWidget`/`MakeDef` that exist in several test-helper namespaces).

**Root cause:** LyraGame compiles with Unity builds — multiple `.cpp` files under `Source/LyraGame/Tests/` are concatenated into one translation unit. Every file-scope `using namespace` directive from earlier files in the unity blob leaks into later files. Two helper namespaces both exposing `MakeWidget()` become ambiguous in any file compiled after both.

**Fix:** In test files, never put `using namespace XyzTestHelpers;` at file scope. Put the using-directive *inside each `RunTest` function body* (function-scope using does not leak), or fully qualify the helper calls. Also give the helper namespace a unique per-file name.

### TWeakObjectPtr assignment needs the complete type, even in a helper header

**Problem:** `WeakObjectPtrTemplates.h ... error C2679: binary '=': no operator found which takes a right-hand operand of type 'AMyActor *'` from a UHT-generated test-helper header that only forward-declared the actor.

**Root cause:** Assigning `TWeakObjectPtr<T> = T*` requires `T` to be a complete type (the conversion goes through `UObject*`). A forward declaration compiles the member fine but fails at the assignment site.

**Fix:** Include the actor's real header in any header whose inline functions assign into a `TWeakObjectPtr` of it.
