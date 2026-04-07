# US-062: Death Screen & Respawn Countdown

## Overview
Full-screen death overlay using CommonUI activatable widget stack. Pushed to UI.Layer.Modal on player death. Shows "You Died" message, respawn countdown timer, and session kill count. Auto-removed on respawn.

## Execution Mode
**Mode:** HEADLESS

## References
- Existing code: `AMordecaiCharacter::HandleDeath()` broadcasts `Event_PlayerDeath`, `AMordecaiGameMode::RespawnDelaySeconds` (3.0f default)
- Base class: `ULyraActivatableWidget` with `ELyraWidgetInputMode::GameAndMenu`

---

## Acceptance Criteria
- [x] AC-062.1: `UMordecaiDeathScreenWidget` inherits `ULyraActivatableWidget`. Shows "You Died" + respawn countdown (ceiling integer). `FormatRespawnCountdown(float)` static, testable.
- [x] AC-062.2: Death screen shows session kill count. `FormatKillCount(int32)` static, testable.
- [x] AC-062.3: Input mode set to `GameAndMenu` so respawn flow is not blocked.
- [x] AC-062.4: Countdown ticks down via `NativeTick`. Designed for `UI.Layer.Modal` push/pop (wiring deferred to EDITOR story).

## Tests Required
- [x] `Mordecai.UI.DeathScreen.FormatCountdownNormal` — 2.5s→"3", 1.1s→"2", 0.5s→"1"
- [x] `Mordecai.UI.DeathScreen.FormatCountdownZero` — 0 or negative → empty
- [x] `Mordecai.UI.DeathScreen.FormatKillCount` — "Kills: N" format

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-062]` prefix
