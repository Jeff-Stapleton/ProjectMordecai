# US-063: Kill Counter Display

## Overview
Persistent kill counter widget in the combat HUD corner. Increments on enemy kills, resets on player death/respawn.

## Execution Mode
**Mode:** HEADLESS

## References
- Recovery: US-086 / issue #6 replaced the dead ASC tag-count listeners with the authoritative `AMordecaiGameMode` count-change delegate
- Existing code: `UMordecaiCombatHUDWidget` (root HUD container)

---

## Acceptance Criteria
- [x] AC-063.1: `UMordecaiKillCounterWidget` exists in `Mordecai/UI/` and displays the authoritative `AMordecaiGameMode` enemy kill count.
- [x] AC-063.2: The authoritative count and widget reset to 0 in the player-death notification that follows `Mordecai.Event.PlayerDeath`.
- [x] AC-063.3: `FormatKillCount(int32)` static, testable. Returns "Kills: N".
- [x] AC-063.4: `UMordecaiCombatHUDWidget` extended with `BindWidgetOptional` slot for KillCounter. `BindToASC` chains to it.

## Tests Required
- [x] `Mordecai.UI.KillCounter.FormatKillCount` — "Kills: 0", "Kills: 1", "Kills: 99"
- [x] `Mordecai.Recovery.KillCounter.IncrementsOnPlayerKill`
- [x] `Mordecai.Recovery.KillCounter.ResetsOnPlayerDeath`

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-063]` prefix
