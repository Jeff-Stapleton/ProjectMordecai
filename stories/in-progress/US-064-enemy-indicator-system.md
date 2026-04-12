# US-064: Enemy Indicator System Migration (Lyra IndicatorSystem)

## Overview
Migrate enemy world-space indicators to Lyra's built-in IndicatorSystem for proper world-to-screen projection, distance-based visibility, and performance-optimized rendering via SActorCanvas. Enemies show health bars, names, and status effect icons above their heads in screen-space, tracking their world position from the diorama camera.

## Execution Mode
**Mode:** HEADLESS

## References
- Lyra IndicatorSystem: `Source/LyraGame/UI/IndicatorSystem/` — `LyraIndicatorManagerComponent`, `IndicatorDescriptor`, `IndicatorLayer`, `IActorIndicatorWidget`, `SActorCanvas`
- Existing code: `MordecaiEnemyHealthBarWidget` (US-052), `AMordecaiEnemyCharacter` (US-050), `UMordecaiStatusEffectComponent` (US-013)
- Design doc: `enemy_archetypes_v1.md` — telegraph clarity, readable from diorama camera
- Design doc: `game_design_v2.md` Section 9 — clean UI, status primarily VFX-driven with minimal critical UI
- Agent rules: `agent_rules_v2.md` — fixed diorama camera (no rotation)

---

## Acceptance Criteria

### Indicator Registration
- [x] AC-064.1: `UMordecaiEnemyIndicatorComponent` (UActorComponent) exists in `Mordecai/UI/`. Attached to enemy characters. Handles registration/unregistration with the Lyra IndicatorSystem.
- [x] AC-064.2: When an enemy takes damage or enters player aggro range, the component registers an indicator via `ULyraIndicatorManagerComponent::AddIndicator()`. The indicator is positioned above the enemy's head (component-relative Z offset, configurable `IndicatorHeightOffset` default 120.0cm).
- [x] AC-064.3: When the enemy dies, the indicator is unregistered via `RemoveIndicator()`. No dangling indicators after enemy destruction.
- [x] AC-064.4: Indicators are only shown for enemies within a configurable `IndicatorVisibilityRange` (default 2000.0cm). Enemies beyond this range have their indicators hidden. When they re-enter range, indicators reappear.

### Indicator Widget
- [x] AC-064.5: `UMordecaiEnemyIndicatorWidget` (UUserWidget) implements `IActorIndicatorWidget`. Displays: enemy display name (FText), health bar (float 0.0-1.0 progress), and a horizontal row of active status effect icons.
- [x] AC-064.6: The widget binds to the enemy's ASC via `BindIndicator()`. Health bar reads `Mordecai.Attribute.Health` / `Mordecai.Attribute.MaxHealth` and updates in real-time when attributes change.
- [x] AC-064.7: Status icons are driven by a configurable `TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>>` that maps status tags (e.g., `Mordecai.Status.Burning`) to icon textures. When a status tag is added to the enemy ASC, the corresponding icon appears. When removed, the icon disappears. Missing textures gracefully show a fallback placeholder.
- [x] AC-064.8: The health bar changes color based on health percentage: Green (>50%), Yellow (25%-50%), Red (<25%). Colors are configurable via UPROPERTY.

### Integration
- [x] AC-064.9: Enemy indicators use Lyra's `IndicatorLayer` for world-to-screen projection. The indicator appears in screen-space above the enemy character, tracking their world position each frame.
- [x] AC-064.10: Multiple enemies each get their own independent indicator instance. Indicators for different enemies do not interfere with each other.

## Technical Notes
- **Lyra IndicatorSystem architecture:** `ULyraIndicatorManagerComponent` lives on the PlayerController. It dispatches `OnIndicatorAdded`/`OnIndicatorRemoved` events. `IndicatorLayer` (UWidget) listens for these and renders via `SActorCanvas` with world-to-screen projection.
- **IActorIndicatorWidget interface:** Requires `BindIndicator(UIndicatorDescriptor&)` and `UnbindIndicator(UIndicatorDescriptor&)`. Use `BindIndicator` to connect ASC attribute listeners.
- **Distance-based visibility:** Check distance from player on tick (low frequency, e.g., every 0.25s) or use the IndicatorDescriptor's visibility flag. Set `bVisible = false` when beyond range.
- **Existing MordecaiEnemyHealthBarWidget:** This widget currently exists but is not wired into the IndicatorSystem. The new `MordecaiEnemyIndicatorWidget` replaces it for world-space rendering. The old widget can be deprecated or repurposed.
- **Status icon textures:** Use `TSoftObjectPtr<UTexture2D>` so textures can be nullptr in headless tests. The C++ code handles the tag → icon mapping; actual texture assets are provided in EDITOR stories.
- **Aggro-based registration:** `AMordecaiEnemyAIController` already has aggro detection (US-051). The indicator component can listen for a `Mordecai.Event.AggroActivated` event or be triggered when the enemy first takes damage.
- Place all new code in `Source/LyraGame/Mordecai/UI/`.

## Tests Required
- [x] `Mordecai.UI.EnemyIndicator.ComponentExists` — Component CDO exists and is an ActorComponent (AC-064.1)
- [x] `Mordecai.UI.EnemyIndicator.DefaultProperties` — Default height offset and visibility range (AC-064.2, AC-064.4)
- [x] `Mordecai.UI.EnemyIndicator.RegistersOnDamage` — Enemy indicator is created when enemy takes damage (AC-064.2)
- [x] `Mordecai.UI.EnemyIndicator.UnregistersOnDeath` — Enemy indicator is removed when enemy dies (AC-064.3)
- [x] `Mordecai.UI.EnemyIndicator.HiddenBeyondRange` — Indicator not shown for enemies beyond IndicatorVisibilityRange (AC-064.4)
- [x] `Mordecai.UI.EnemyIndicator.ShownWhenReEntersRange` — Indicator reappears when enemy moves back within range (AC-064.4)
- [x] `Mordecai.UI.EnemyIndicator.HealthBarColorThresholds` — Health bar color is Green >50%, Yellow 25-50%, Red <25% (AC-064.8)
- [x] `Mordecai.UI.EnemyIndicator.WidgetExists` — Widget CDO exists and is a UUserWidget (AC-064.5)
- [x] `Mordecai.UI.EnemyIndicator.StatusIconMapping` — Default status icon map and color boundary (AC-064.7)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green) — 9/9 new tests + 418 total Mordecai tests pass
- [x] Project compiles with zero errors
- [x] PIE smoke tests pass (4/4 including new verify_pie_indicator.py)
- [ ] Code committed and pushed with `[US-064]` prefix
