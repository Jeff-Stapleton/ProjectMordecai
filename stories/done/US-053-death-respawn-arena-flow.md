# US-053: Player Death & Arena Game Flow

## Overview
Implement the player death/respawn loop and arena game mode logic so the vertical slice has a complete play session cycle. Player dies → respawn → enemies reset → fight again. Without this, the prototype is a one-shot — Jeff kills or gets killed and has to relaunch.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `game_design_v2.md` (Health reaching 0 → death or downed state)
- Design doc: `combat_system_v1.md` (Health 0-100%, reaching 0 → death)
- Agent rules: `agent_rules_v2.md` (GAS for attributes, server-authoritative)
- Existing code: `AMordecaiCharacter`, `AMordecaiGameMode`, `AMordecaiEnemyCharacter` (US-050), `UMordecaiAttributeSet`

---

## Acceptance Criteria
- [x] AC-053.1: When the player's Health attribute reaches ≤ 0, a `Mordecai.State.Dead` tag is applied to the player's ASC and all ability input is blocked
- [x] AC-053.2: Player death disables character movement (movement component deactivated) and disables input processing on the controller
- [x] AC-053.3: Player death broadcasts a `Mordecai.Event.PlayerDeath` gameplay event via the ASC
- [x] AC-053.4: `AMordecaiGameMode` has a `RespawnDelaySeconds` UPROPERTY (default 3.0s). After the delay, the player is respawned at a designated spawn point
- [x] AC-053.5: On respawn, the player's Health is restored to MaxHealth, Stamina to MaxStamina, Posture to MaxPosture, and the `Mordecai.State.Dead` tag is removed
- [x] AC-053.6: On respawn, input and movement are re-enabled
- [x] AC-053.7: `AMordecaiGameMode` tracks enemy kill count via a delegate/event listener for `Mordecai.Event.Death` from enemies
- [x] AC-053.8: `AMordecaiGameMode` has an `ArenaResetOnPlayerRespawn` bool (default true). When true, all dead enemies are respawned at their original spawn locations with full health/posture on player respawn
- [x] AC-053.9: `AMordecaiGameMode::ResetArena()` restores all enemies to full health/posture and resets their AI to Idle state
- [x] AC-053.10: New gameplay tags declared: `Mordecai.State.Dead`, `Mordecai.Event.PlayerDeath`, `Mordecai.Event.EnemyKill` (if not already declared in US-050)
- [x] AC-053.11: The respawn flow is server-authoritative — respawn timing and state reset happen on the server and replicate to clients

## Technical Notes
- **Class modifications:**
  - `AMordecaiCharacter` — add death handling (attribute change delegate for Health ≤ 0)
  - `AMordecaiGameMode` — add respawn timer, arena reset, kill tracking
  - `MordecaiGameplayTags.h/.cpp` — add new tags
- **Death on player:** Similar pattern to enemy death (US-050 AC-050.6) but with respawn flow. Consider extracting a shared death handling utility or base class method.
- **Respawn flow:** `GameMode::RestartPlayer()` is the UE standard, but for the vertical slice it may be simpler to just restore attributes, remove the Dead tag, teleport to spawn, and re-enable input. Use whichever approach is cleaner.
- **Enemy respawn:** The GameMode should maintain a registry of spawned enemies (actor + spawn transform). On `ResetArena()`, iterate the registry: for dead enemies, either respawn or restore health. For live enemies, reset to full stats and Idle AI state.
- **Kill tracking:** Listen for `Mordecai.Event.Death` gameplay events from any actor with `Mordecai.Team.Enemy`. Increment a counter and broadcast `Mordecai.Event.EnemyKill`.
- Timer for respawn: use `GetWorldTimerManager().SetTimer()` with `RespawnDelaySeconds`.
- Do NOT implement scoring, wave systems, or progression. This is a simple arena reset for playtesting.

## Tests Required
- [x] `Mordecai.Arena.PlayerDeathAtZeroHealth` — Health ≤ 0 applies Dead tag (verifies AC-053.1)
- [x] `Mordecai.Arena.PlayerDeathDisablesMovement` — movement component deactivated (verifies AC-053.2)
- [x] `Mordecai.Arena.PlayerDeathBlocksInput` — ability activation blocked (verifies AC-053.2)
- [x] `Mordecai.Arena.PlayerDeathBroadcastsEvent` — gameplay event fires (verifies AC-053.3)
- [x] `Mordecai.Arena.RespawnRestoresHealth` — after delay, Health = MaxHealth (verifies AC-053.5)
- [x] `Mordecai.Arena.RespawnRestoresStamina` — Stamina = MaxStamina (verifies AC-053.5)
- [x] `Mordecai.Arena.RespawnRemovesDeadTag` — Dead tag cleared (verifies AC-053.5)
- [x] `Mordecai.Arena.RespawnReenablesMovement` — movement works again (verifies AC-053.6)
- [x] `Mordecai.Arena.KillCountIncrementsOnEnemyDeath` — game mode tracks kills (verifies AC-053.7)
- [x] `Mordecai.Arena.ArenaResetRestoresEnemies` — ResetArena() restores enemy health (verifies AC-053.9)
- [x] `Mordecai.Arena.RespawnDelayIsConfigurable` — custom delay value works (verifies AC-053.4)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-053]` prefix
