# US-069: Pause Menu Framework (CommonUI)

## Overview
Build the C++ pause menu framework using Lyra's CommonUI integration. A tabbed menu activated by Pause input (Start/Esc) that switches input mode from Game to Menu, pauses the game, and provides a tab registration API for future content (character sheet, skills, feats, inventory, settings). Tab content is placeholder for now — actual tab UIs are scoped in US-066 through US-068. Blueprint visual polish is US-076 (EDITOR).

## Execution Mode
**Mode:** HEADLESS

## References
- Lyra CommonUI: `Source/LyraGame/UI/LyraActivatableWidget.h` — `ULyraActivatableWidget`, `FUIInputConfig`
- Lyra Tab System: `Source/LyraGame/UI/Common/LyraTabListWidgetBase.h` — tab list base class
- Design doc: `control_bindings_v1.1.md` — Pause/Menu: Start button (controller), Esc (keyboard)
- Design doc: `game_design_v2.md` Section 9 — clean UI
- Agent rules: `agent_rules_v2.md` — GAS for attributes (character sheet will read from ASC)

---

## Acceptance Criteria

### Pause Menu Widget
- [x] AC-069.1: `UMordecaiPauseMenuWidget` extends `ULyraActivatableWidget` in `Mordecai/UI/`. Overrides `GetDesiredInputConfig()` to return Menu input mode (blocks game input, shows mouse cursor).
- [x] AC-069.2: The pause menu has a tab bar with configurable tabs. Default tabs registered on construction: "Character" (`character`), "Skills" (`skills`), "Feats" (`feats`), "Inventory" (`inventory`), "Settings" (`settings`). Each tab has an `FName` id and `FText` display name.
- [x] AC-069.3: Tab content area displays the widget associated with the active tab. Initially, all tabs use a placeholder widget class (`UMordecaiPauseMenuPlaceholderWidget`) that displays "Coming Soon" text.
- [x] AC-069.4: Tab switching via LB/RB (controller) or clicking tab headers (mouse). Active tab is tracked. Switching tabs swaps the content area widget.

### Pause/Resume System
- [x] AC-069.5: `UMordecaiPauseMenuSubsystem` (UGameInstanceSubsystem) exists in `Mordecai/UI/`. Provides `TogglePauseMenu()`, `OpenPauseMenu()`, `ClosePauseMenu()`. Manages the pause menu widget lifecycle.
- [x] AC-069.6: `OpenPauseMenu()` creates the pause menu widget, pushes it onto the CommonUI widget stack (via the local player's UI subsystem), and pauses the game via `UGameplayStatics::SetGamePaused(true)`. `ClosePauseMenu()` deactivates the widget, removes it from the stack, and unpauses via `SetGamePaused(false)`. `TogglePauseMenu()` calls Open or Close based on current state.
- [x] AC-069.7: The subsystem listens for the Pause input action (`IA_Pause`). When pressed, calls `TogglePauseMenu()`. If `IA_Pause` does not exist in the current IMC, the subsystem registers it.

### Tab Registration API
- [x] AC-069.8: `UMordecaiPauseMenuWidget::RegisterTab(FName TabId, FText TabDisplayName, TSubclassOf<UUserWidget> ContentWidgetClass)` registers or replaces a tab's content widget class. When a tab is selected, an instance of `ContentWidgetClass` is created and displayed in the content area. This allows future stories (US-066/067/068) to plug in their content.
- [x] AC-069.9: `UMordecaiPauseMenuWidget::UnregisterTab(FName TabId)` removes a tab from the tab bar entirely. Returns the tab to its placeholder or removes it from the list.

### Navigation
- [x] AC-069.10: Pressing Back (B button / Esc while menu is open) closes the menu and resumes gameplay. This uses CommonUI's back action handling.
- [x] AC-069.11: The pause menu tracks which tab was last active and restores it when reopened within the same session (not persisted across game launches).

### Edge Cases
- [x] AC-069.12: Opening the pause menu while already open is a no-op. Closing while already closed is a no-op. No double-pause or double-unpause.
- [x] AC-069.13: `TODO(DECISION)`: Multiplayer pause behavior. For now, always pause (single-player first). The subsystem has a `bCanPauseGame` flag (default true) that can be set to false for multiplayer where pausing only switches input mode without freezing the game.

## Technical Notes
- **Lyra's CommonUI integration:** `ULyraActivatableWidget` extends `UCommonActivatableWidget` and adds `GetDesiredInputConfig()` which returns `FUIInputConfig` controlling input mode (Game, Menu, GameAndMenu). Override to return Menu mode.
- **Widget stack:** Lyra uses `UCommonUIExtensions` or the local player's `UUIManagerSubsystem` to push/pop activatable widgets. The pause menu should be pushed as a layer.
- **Tab system:** `ULyraTabListWidgetBase` provides tab list functionality with controller navigation. Compose with or extend this for the tab bar.
- **Placeholder widget:** `UMordecaiPauseMenuPlaceholderWidget` is a trivial UUserWidget subclass with a single text block. Minimal code.
- **Input action:** Check if Lyra already has a Pause/Menu input action. If not, create `IA_Pause` and add it to the Mordecai IMC. Bind via Enhanced Input.
- **Subsystem choice:** `UGameInstanceSubsystem` persists across level loads, which is correct for a menu system. The subsystem holds a weak pointer to the active widget.
- **NullRHI testing:** Widget creation and lifecycle can be tested headlessly — verify subsystem state, tab registration, and pause state without visual rendering.
- Place code in `Source/LyraGame/Mordecai/UI/`.

## Tests Required
- [x] `Mordecai.UI.PauseMenu.OpensOnToggle` — TogglePauseMenu when closed creates and activates the widget (AC-069.5, AC-069.6)
- [x] `Mordecai.UI.PauseMenu.ClosesOnToggle` — TogglePauseMenu when open deactivates and removes the widget (AC-069.6)
- [x] `Mordecai.UI.PauseMenu.PausesGameOnOpen` — Game is paused when menu opens (AC-069.6)
- [x] `Mordecai.UI.PauseMenu.UnpausesGameOnClose` — Game is unpaused when menu closes (AC-069.6)
- [x] `Mordecai.UI.PauseMenu.HasDefaultTabs` — Menu has 5 default tabs with correct ids (AC-069.2)
- [x] `Mordecai.UI.PauseMenu.TabSwitchChangesContent` — Switching active tab changes the displayed content widget (AC-069.3, AC-069.4)
- [x] `Mordecai.UI.PauseMenu.RegisterTabReplacesPlaceholder` — Registering a content class for a tab replaces its placeholder (AC-069.8)
- [x] `Mordecai.UI.PauseMenu.UnregisterTabRemoves` — Unregistering a tab removes it from the tab bar (AC-069.9)
- [x] `Mordecai.UI.PauseMenu.RemembersLastTab` — Reopening the menu restores the last active tab (AC-069.11)
- [x] `Mordecai.UI.PauseMenu.DoubleOpenIsNoOp` — Opening when already open does not double-pause or create duplicate widgets (AC-069.12)
- [x] `Mordecai.UI.PauseMenu.DoubleCloseIsNoOp` — Closing when already closed does not double-unpause (AC-069.12)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-069]` prefix
