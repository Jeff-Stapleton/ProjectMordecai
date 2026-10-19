# US-063: Kill Counter Display

## Overview
Persistent kill counter widget in the combat HUD corner. Increments on enemy kills, resets on player death/respawn.

## Execution Mode
**Mode:** HEADLESS

## References
- Gameplay tags: `Mordecai.Event.EnemyKill` (increment), `Mordecai.Event.PlayerDeath` (reset)
- Existing code: `UMordecaiCombatHUDWidget` (root HUD container)

---

## Acceptance Criteria
- [x] AC-063.1: `UMordecaiKillCounterWidget` exists in `Mordecai/UI/`. Listens to `Mordecai.Event.EnemyKill` tag events and increments kill count.
- [x] AC-063.2: Kill count resets to 0 on `Mordecai.Event.PlayerDeath`.
- [x] AC-063.3: `FormatKillCount(int32)` static, testable. Returns "Kills: N".
- [x] AC-063.4: `UMordecaiCombatHUDWidget` extended with `BindWidgetOptional` slot for KillCounter. `BindToASC` chains to it.

## Tests Required
- [x] `Mordecai.UI.KillCounter.FormatKillCount` — "Kills: 0", "Kills: 1", "Kills: 99"

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-063]` prefix
