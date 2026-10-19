---
name: unreal-testing
description: >
  Write and run automated tests in Unreal Engine 5.x using the Automation Test Framework.
  Use when: (1) writing C++ unit/integration tests (Simple, Complex, or Spec/BDD style),
  (2) writing Python editor automation tests, (3) creating Functional Test actors for in-level testing,
  (4) screenshot comparison tests, (5) FBX import/export test plans,
  (6) UI automation via the Automation Driver (simulated input),
  (7) running tests from editor or command line, (8) configuring test groups/exclusions,
  (9) reviewing test results (log, JSON, HTML), (10) writing Editor Utility Blueprint tests,
  (11) Project Mordecai verification ladder and packaged-build playtests — "playtest", "packaged build",
  "does the feature work in game", "is the story done", package_playtest.py, playtest_boot.py,
  check_content_committed.py, STALE BUILD, wrong experience (B_LyraDefaultExperience), black screenshots.
  NOT for: gameplay logic (use gameplay framework), performance profiling (use Insights), or CI/CD pipeline setup.
---

# Unreal Engine Test Automation

## Test Types Overview

| Type | Language | Best For |
|------|----------|----------|
| **Simple Test** | C++ | Unit tests, single atomic checks |
| **Complex Test** | C++ | Data-driven / content stress tests (e.g., load all maps) |
| **Spec (BDD)** | C++ | Self-documenting, isolated expectations with `Describe`/`It` |
| **Python Test** | Python | Editor automation, screenshot capture, quick iteration |
| **Functional Test** | Blueprint/C++ | In-level actor-based tests with setup/teardown |
| **Editor Utility Test** | Blueprint | Editor utility testing via Blueprint graph |
| **Screenshot Test** | Actor/Blueprint | Visual regression via image comparison |
| **FBX Test** | JSON plan | Import/reimport validation of FBX files |

## Project Mordecai: Verification Ladder & Packaged Playtests

> **Playable in the packaged build, or it isn't done.** Applies to every Project Mordecai story with player-facing behavior. Everything here is a local script run with `py` from the repo root `C:\Users\jeffd\Documents\Gamedev\ProjectMordecai` — no BuildGraph, Horde, Gauntlet, or CI.

**Definition of Done:** a playtest on a FRESH packaged build — built by `package_playtest.py` from the commit being delivered and accepted by `playtest_boot.py`'s staleness gate — shows the feature working, reached the way a player reaches it. Open every screenshot with the **Read** tool and state what is visibly on screen before calling the story done.

### Verification Ladder

A lower tier **never** substitutes for a higher tier. Passing Tiers 1–2 means "ready for Tier 3", not "done".

| Tier | Proves | Run |
|------|--------|-----|
| **1. Headless automation** | C++ logic | `py Scripts/run_tests.py [Mordecai.<Area>]` or `Scripts\RunTests.bat` (wraps it); NullRHI, default filter `Mordecai`; judges the test log — a raw `UnrealEditor-Cmd` run exits 0 even when tests fail |
| **2. PIE smoke** | Editor integration (in-editor, possibly unsaved state) | `py Scripts/run_pie_verify.py [--no-launch] [--tests hud,spawn]` |
| **3. Packaged playtest** | The shipped game boots the right experience and shows the feature | `py Scripts/package_playtest.py` → `py Scripts/playtest_boot.py` → Read the screenshots |
| **4. Real-input playtest** | Player input reaches the feature through Enhanced Input | *Phase 2 — planned, NOT available yet* (agent bridge; see Automation Driver below) |

### Rules

**DO:**
- Save every asset an editor script creates or edits (`unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)`, `save_current_level()`, …) and commit the resulting `.uasset`/`.umap` (Git LFS) in the same branch/PR as the C++ and scripts. Content that exists only in editor memory does not exist.
- Get exit 0 from `py Scripts/check_content_committed.py` before calling a story done.
- Make every Mordecai map resolve its Mordecai experience in a packaged build (map World Settings, `?Experience=`, or `AMordecaiGameMode::DefaultExperience`). `playtest_boot.py`'s experience check is the proof.
- Label smoke-test checks that call gameplay functions directly as **LOGIC/diagnostic** (tag the PASS/FAIL line `[LOGIC]`) in the script output and the story notes.
- Cite the run dir (`Saved\Playtests\<run>\report.md` + screenshot paths) and what the screenshots show in the story's completion notes AND the body of the closing commit made after the playtest (it touches only `stories/`; a change under `Source/` `Plugins/` `Content/` `Config/` means re-package and re-run). Report failures honestly; never relabel a FAIL.

**DO NOT:**
- Gate gameplay setup on `World->IsPlayInEditor()`, `WITH_EDITOR`, `ULyraDeveloperSettings` (`ExperienceOverride` is honored only in PIE), or any other editor-only state.
- Accept a player-facing AC on direct calls (teleport + `try_pickup_focused()`, `toggle_pause_menu_to_tab('inventory')`, …). That needs the player path: Tier 3 now, Tier 4 when available.
- Cite a `--allow-stale` run or `Scripts\Package.bat` output (no `build_info.json`) as Done evidence.

### Content Guard: `check_content_committed.py`

```
py Scripts/check_content_committed.py                    # range = merge-base(origin/main, HEAD)..HEAD
py Scripts/check_content_committed.py --base main --json # other base ref, machine-readable output
```

- **Check 1** — any modified/untracked/deleted `.uasset`/`.umap` → FAIL ("saved on disk but not committed"). Fix: commit them (or revert unintended changes).
- **Check 2** — a `Scripts/*.py` that writes assets (`save_asset(`, `save_loaded_asset`, `save_current_level`, `save_directory(`, `save_packages`, `create_asset(`, `save_dirty_packages`, `save_all_dirty_levels`, `save_map(`, `import_asset_tasks(`) was added/modified in the range (or is modified/untracked in the working tree), but zero `.uasset`/`.umap` files were added/modified in the range or on disk → FAIL, naming the scripts. Fix: run the script in the editor, confirm it saved, commit the assets. `[no-content]` exempts a script only if EVERY commit in the range that touched it carries the tag and it has no uncommitted changes (check 2 is SKIPPED when all such scripts are exempt) — use it only when the script genuinely produces nothing to commit, and say why in that message.
- Exit `0` pass, `1` fail, `2` git error / bad ref.

### Packaging: `package_playtest.py`

```
py Scripts/package_playtest.py                     # Development Win64, iterative cook, L_LyraFrontEnd + DevTestMap
py Scripts/package_playtest.py --clean             # full cook (no -iterate): after cook oddities or big content moves
py Scripts/package_playtest.py --all-maps          # adds -CookAll -CookMapsOnly: every map, in addition to the two playtest maps
py Scripts/package_playtest.py --config DebugGame  # Development (default) / DebugGame (binary LyraGame-Win64-DebugGame.exe: pass it to playtest_boot.py --exe) / Shipping (strips dev features; don't playtest it)
py Scripts/package_playtest.py --dry-run           # print the RunUAT BuildCookRun command, exit 0
```

- Commit first (code AND content), then package; don't edit or commit while it runs. `build_info.json` records commit, branch, dirty state (`dirty`, `dirty_files`, `dirty_count`, `dirty_fingerprint`), `source_changed_during_build`, `built_at`, `config`, `maps`, `all_maps`, `game_exe`, `uat_log`; `playtest_boot.py` rejects a build that doesn't match the tree you are delivering.
- Output: `PackagedBuilds\Playtest\` + `PackagedBuilds\Playtest\build_info.json` (the previous `build_info.json` and `Windows\` folder are deleted first; `build_info.json` is written only on success); UAT log at `Saved\Playtests\package-<YYYYmmdd-HHMMSS>.log`. Exit `0` packaged (or `--dry-run`); `1` UAT succeeded but the game binary is missing; `2` infrastructure (uproject/RunUAT missing, git error, previous package locked — close a running packaged game); any other non-zero = UAT's own exit code (130 on Ctrl+C), after printing the log tail and `Error` lines — read them, fix, re-package.
- Slow: the first package can take an hour+; iterative re-cooks are faster. Run it with `run_in_background` and wait. The cost is accepted.
- Packaged layout: `PackagedBuilds\Playtest\Windows\LyraGame.exe` is only a bootstrapper; the real game is `PackagedBuilds\Playtest\Windows\ProjectMordecai\Binaries\Win64\LyraGame.exe` (what `playtest_boot.py` launches).

### Boot Playtest: `playtest_boot.py`

```
py Scripts/playtest_boot.py                       # DevTestMap, expects B_MordecaiArenaExperience
py Scripts/playtest_boot.py --frontend            # no map arg: real player entry (L_LyraFrontEnd), expects B_LyraFrontEnd_Experience
py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/<Map> --expect-experience <ExperienceName>
py Scripts/playtest_boot.py --settle 10 --shots 5 --shot-interval 1.0 --timeout 400
py Scripts/playtest_boot.py --fail-on-errors      # logged 'Error:' lines fail the run
py Scripts/playtest_boot.py --keep-running        # leave the game open (default: taskkill /T /F)
py Scripts/playtest_boot.py --exe <path>          # non-default executable
py Scripts/playtest_boot.py --allow-stale         # STALE BUILD becomes a warning: diagnosis only, never Done evidence
```

Defaults: `--timeout 240`, `--settle 6`, `--shots 3`, `--shot-interval 2.0`. Launches `-windowed -ResX=1280 -ResY=720 -log -nosplash -nosound`, tails the log for `Identified experience LyraExperienceDefinition:<Expected>` and `EXPERIENCE: OnExperienceLoadComplete`, then screenshots the game window.

Run dir `Saved\Playtests\<YYYYmmdd-HHMMSS>-boot\`: `game.log`, `engine_01.png`… (the game saves these from its own viewport via `-PlaytestCapture`; primary evidence, works when the machine is locked), `shot_01.png`… (desktop capture; all black when the workstation is locked), `report.json`, `report.md`. The `engine_screenshots` check FAILs (infra) if the game wrote no `engine_NN.png` (Shipping build, or a build without `UMordecaiPlaytestCaptureSubsystem`). Keep `--settle` at 3s or more. Lyra holds its loading screen about 2s past experience load for texture streaming, so `--settle 0` captures the Lyra logo, not the game. The `content_cooked` check FAILs (exit 1) when the game asked for a `/MordecaiCore` asset that isn't in the build. Exit `0` all checks pass, `1` gameplay failure (wrong experience, crash, timeout), `2` infrastructure (exe missing, STALE BUILD, window not found, git error).

**Reading `report.md`:** check every PASS/FAIL line; confirm the identified experience and its source; note the load time; read the error/warning counts and top errors (informational unless `--fail-on-errors`, but new errors from your systems are bugs). Then **Read every screenshot path it lists** and describe what is actually on screen (pawn, HUD, arena, pickups, menus). PASS lines without a screen that shows the feature ≠ done.

| Symptom | Meaning | Do |
|---------|---------|-----|
| `STALE BUILD` (exit 2) | `build_info.json` missing/unreadable, built from a commit other than HEAD, the tree changed while UAT ran (`source_changed_during_build`), packaged binaries/paks newer than `built_at` (re-packaged by something else), uncommitted changes under `Source/` `Plugins/` `Content/` `Config/` or the `.uproject` since a clean build, or a dirty build whose dirty-file list or uncommitted content (`dirty_fingerprint`) changed | Commit, re-run `package_playtest.py`, boot again. Don't reach for `--allow-stale`. |
| Identified `B_LyraDefaultExperience`, or `falling back to the default` (exit 1) | The Mordecai experience did NOT load — you are looking at Lyra's shooter pawn/HUD/input | Check map World Settings / `?Experience=` / `AMordecaiGameMode::DefaultExperience`, the `Config/DefaultGame.ini` primary-asset scans + `+MapsToCook`, and that the experience is cooked. Never "fix" it with a PIE-only override. |
| `Failed to identify experience` (exit 1) | The asset manager can't find the experience in the cooked build | Primary-asset scan paths / cook rules in `Config/DefaultGame.ini`. |
| `Fatal error`, `Unhandled Exception`, `Assertion failed`, `=== Critical error: ===`, or process exited early (exit 1) | Crash | Read `game.log` in the run dir; the callstack follows the marker. |
| Timeout (exit 1) | Never reached `OnExperienceLoadComplete` | Read the tail of `game.log`; raise `--timeout` only if it shows steady progress (e.g. first-boot shader compile). |
| Black / uniform screenshot warning, or "another window covers part of the game window" | Captured before first render, window occluded/minimized, desktop locked, or rendering broken | Re-run with a larger `--settle`, desktop unlocked, window uncovered. Still black → treat it as a render failure. Read the image either way. |
| Window not found (exit 2) | No visible top-level window for the game PID | Check `game.log` — usually the process died before creating its window. |

## C++ Tests

### Simple Test

```cpp
// NOTE: In UE 5.7, ApplicationContextMask is a standalone constexpr, NOT an enum member.
// Use EAutomationTestFlags_ApplicationContextMask (no ::), not EAutomationTestFlags::ApplicationContextMask.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyTest, "Project.Unit.MyTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMyTest::RunTest(const FString& Parameters)
{
    TestTrue("Value should be positive", MyValue > 0);
    return true;
}
```

### Complex Test (Data-Driven)

Override `GetTests()` to provide inputs, `RunTest()` receives each as `Parameters`:

```cpp
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FLoadAllMaps, "Project.Stress.LoadAllMaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FLoadAllMaps::GetTests(TArray<FString>& OutNames, TArray<FString>& OutCommands) const
{
    // Populate OutNames (display) and OutCommands (parameters) for each map
}

bool FLoadAllMaps::RunTest(const FString& Parameters)
{
    FEngineAutomationTestUtilities::LoadMap(Parameters);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));
    return true;
}
```

### Spec (BDD) — Recommended for New Tests

For full Spec documentation: [references/automation-spec.md](references/automation-spec.md)

```cpp
BEGIN_DEFINE_SPEC(FMySpec, "Project.MyFeature", EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
    TSharedPtr<FMyClass> Instance;
END_DEFINE_SPEC(FMySpec)

void FMySpec::Define()
{
    BeforeEach([this]() {
        Instance = MakeShared<FMyClass>();
    });

    Describe("Execute()", [this]() {
        It("should return true when valid", [this]() {
            TestTrue("Execute result", Instance->Execute());
        });

        It("should return false when invalid", [this]() {
            Instance->Invalidate();
            TestFalse("Execute result", Instance->Execute());
        });
    });

    AfterEach([this]() {
        Instance.Reset();
    });
}
```

**File naming:** `MyClass.spec.cpp` in `Private/Tests/`

**Key Spec features:**
- `Describe()` / `It()` — scoping and expectations
- `BeforeEach()` / `AfterEach()` — setup/teardown (scope-aware)
- `LatentIt()` / `LatentBeforeEach()` — async with `Done` delegate
- `EAsyncExecution::ThreadPool` — run off GameThread
- `xDescribe()` / `xIt()` — disable tests (prefix with `x`)
- Parameterized tests via loops in `Define()`

### Latent Commands

```cpp
DEFINE_LATENT_AUTOMATION_COMMAND(FWaitForCondition);
bool FWaitForCondition::Update() { return ConditionMet(); }

// With parameter:
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FLoadMapCommand, FString, MapName);
bool FLoadMapCommand::Update() { /* ... */ return true; }

// Usage in RunTest:
ADD_LATENT_AUTOMATION_COMMAND(FWaitForCondition());
ADD_LATENT_AUTOMATION_COMMAND(FLoadMapCommand(TEXT("/Game/Maps/TestMap")));
```

## Python Tests

Enable the **PythonAutomationTest** plugin. Place scripts named `test_*.py` in `Content/Python/`. They appear under `Editor > Python > ...` in the Test Automation window.

### Basic Python Test

```python
import unreal

# Simple assertion
assert some_condition, "Condition failed"

# Log errors (continues execution)
unreal.log_error("Something went wrong")

# Raise to stop
raise Exception("Fatal test error")
```

### Latent Commands (Multi-Frame)

```python
@unreal.AutomationScheduler.add_latent_command
def step_one():
    unreal.EditorLevelLibrary.load_level("/Game/TestMap")

@unreal.AutomationScheduler.add_latent_command
def step_two():
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    assert len(actors) > 0, "No actors in level"
```

Generator-based for loops:

```python
@unreal.AutomationScheduler.add_latent_command
def multi_step():
    print("start")
    yield  # wait one frame
    for i in range(10):
        print(f"step {i}")
        yield
```

### Screenshots from Python

```python
@unreal.AutomationScheduler.add_latent_command
def take_screenshots():
    level_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    cameras = unreal.EditorFilterLibrary.by_class(level_actors, unreal.CameraActor)
    for cam in cameras:
        task = unreal.AutomationLibrary.take_high_res_screenshot(cam.get_actor_label())
        if not task.is_valid_task():
            continue
        while not task.is_task_done():
            yield
```

### Expected Errors & Telemetry

```python
unreal.AutomationLibrary.add_expected_log_error("expected pattern", occurrence_count)
unreal.AutomationLibrary.compare_image_against_reference("image_name")
unreal.AutomationLibrary.add_test_telemetry_data("datapoint", measurement_float)
unreal.AutomationLibrary.set_test_telemetry_storage("storage_name")
```

## Functional Tests (In-Level)

Place a **Functional Test Actor** in the level. Override in Blueprint or C++:

| Function | Purpose |
|----------|---------|
| `PrepareTest` | Initial setup (can be multi-frame) |
| `IsReady` | Return `true` when setup complete (called every tick) |
| `OnTestStart` | Main test logic — must call `FinishTest` at end |
| `OnTestFinished` | Cleanup |

Support features: `RegisterAutoDestroyActor()`, `LogMessage()`, `ObservationPoint`, `Enabled` toggle.

**Ground Truth Data:** Save expected results from a known-good run, compare in future runs. Reset via `ResetGroundTruth` property.

## Editor Utility Blueprint Tests

Enable the **EditorTests** plugin. Create via Content Browser: `Editor Utilities > Editor Utility Blueprint`, parent class `EditorUtilityTest`.

Events: `Prepare Test` → `Finish Prepare Test` → `Start Test` → `Finish Test`

Test name pattern: `Project.Blueprints.EditorUtilities.<content_path>.<asset_name>`

## Screenshot Comparison

For full screenshot comparison docs: [references/screenshot-comparison.md](references/screenshot-comparison.md)

**Functional Screenshot Test Actor:** Place in level, configure camera, tolerance, resolution. Two varieties:
- `Functional Screenshot Test` — scene view
- `Functional UI Screenshot Test` — UI capture

**Tolerance levels:** Zero, Low (default), Medium, High, Custom (per-channel RGBA + brightness)

**Ground truth workflow:**
1. Run test → warning about missing ground truth
2. Click **Add New** in Screenshot Browser
3. Future runs compare against ground truth
4. On failure: **Replace** or **Add As Alternative**

Screenshots saved to: `Saved/Automation/Comparisons/`

## Automation Driver (UI Input Simulation)

For full Automation Driver docs: [references/automation-driver.md](references/automation-driver.md)

Simulates mouse/keyboard input for UI functional tests. Use with Spec tests (runs off GameThread).

```cpp
IAutomationDriverModule::Get().Enable();
FAutomationDriverPtr Driver = IAutomationDriverModule::Get().CreateDriver();

// Find elements
FDriverElementRef Elem = Driver->FindElement(By::Id("MyWidget"));
FDriverElementRef Elem2 = Driver->FindElement(By::Path("#Form//<SEditableText>"));

// Actions
Elem->Click();
Elem->Type(TEXT("Hello World"));

// Sequences
FDriverSequenceRef Seq = Driver->CreateSequence();
Seq->Actions().Focus(Elem).Type(TEXT("test\tvalue")).Click(Elem2);
Seq->Perform();

// Waiting
Driver->Wait(Until::ElementIsVisible(Elem, FWaitTimeout::InSeconds(5)));

IAutomationDriverModule::Get().Disable();
```

**Locators:** `By::Id()`, `By::Path()`, `By::Cursor()`, `By::Delegate()`

> **Project Mordecai — real-input playtests are Phase 2 (planned, NOT available yet).** The Automation Driver runs inside the editor/automation framework, so it is Tier 1–2 evidence, not packaged-game proof. Phase 2 adds an in-game **agent bridge** subsystem compiled into non-Shipping builds: it injects real key/hold/mouse input at the Slate/viewport level (so Enhanced Input mappings are exercised), returns state JSON + screenshots, and is wrapped as an MCP server so agents can play the packaged game, with per-story real-input playtest scripts (Tier 4). Until it lands, `playtest_boot.py` (Tier 3) is the highest tier available — don't claim input-path verification you didn't run.

## Running Tests

**From Editor:** `Tools > Test Automation` → select tests → Start Tests

**From Command Line:**
```
-ExecCmds="Automation RunTest MyTest;Quit"
-ExecCmds="Automation RunTest MySet.MySubSet;Quit"
-ExecCmds="Automation RunTest Group:MyGroup;Quit"
-ReportExportPath="C:/TestResults"
-ResumeRunTest
```

## Configuration

In `DefaultEngine.ini`:

```ini
# Test groups
+Groups=(Name="Smoke", Filters=((Contains="Project.Smoke.")))

# Exclude tests
+ExcludeTest=(Test="Project.Flaky.TestName", Reason="Known flaky - JIRA-123")
+ExcludeTest=(Test="Project.RHI.", Reason="RHI specific", RHI="D3D12")
```

## Test Results

- **Log:** `Test Started. Name={...}` / `Test Completed. Result={...}` / exit code 0 = pass
- **JSON:** `-ReportExportPath` produces `.json` with devices, durations, events, artifacts
- **HTML:** Served via Automation Test Report Server

## File Organization

```
Source/MyModule/Private/Tests/
├── MyClass.spec.cpp           # Spec (BDD) tests
├── MyClassTest.cpp            # Simple/Complex tests
Content/Python/
├── test_level_setup.py        # Python automation tests
├── test_asset_validation.py
```

## Detailed References

- [references/automation-spec.md](references/automation-spec.md) — Full Spec/BDD guide with advanced features
- [references/automation-driver.md](references/automation-driver.md) — UI input simulation API
- [references/screenshot-comparison.md](references/screenshot-comparison.md) — Visual regression testing
- [references/fbx-testing.md](references/fbx-testing.md) — FBX import test plans
