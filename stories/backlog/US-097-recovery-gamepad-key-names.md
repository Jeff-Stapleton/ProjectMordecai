# US-097: Recovery — Gamepad Move, Look and Sprint Use Valid Key Names

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** US-083 (sprint handler bound)
**Paired story:** none

## Overview
A gamepad player cannot move, aim or sprint: the programmatic input context maps `Gamepad_LeftStick2D`, `Gamepad_RightStick2D` and `Gamepad_LeftThumbstickButton`, which are not engine key names (the engine's are `Gamepad_Left2D`, `Gamepad_Right2D`, `Gamepad_LeftThumbstick`), so those mappings are silently dead. Features: movement, sprint (gamepad).

## Execution Mode
**HEADLESS**
FKey names in `MordecaiHeroComponent.cpp` plus a validation test; no assets.

## References
- `Specs/agent_playtest_plan_v1.md` — "Gamepad stick and L3 mappings use invalid FKey names" (`MordecaiHeroComponent.cpp:167,180,197`)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184725-us001`: `gamepad_move` FAIL known ("Gamepad_LeftY=1 for 0.799s: (-379, -790) -> (-379, -790), dx 0 cm"), `sprint_l3` FAIL known (dx 282 cm), `facing_stick` FAIL known (also blocked by facing, US-098)
- `Specs/control_bindings_v1.1.md` — Move: Left Stick; Look/Aim: Right Stick; Sprint toggle: L3; Use Skill A: RB / Q; Use Skill B: LB / E
- Done stories: US-001 AC-1.3.2, AC-1.3.3, AC-2.1.1, AC-2.1.3; US-057 AC-057.5 (`IA_Spell_1` LB/Q, `IA_Spell_2` RB/E, `IA_Spell_3` LB+RB/R, `IA_Spell_4` DPadUp/1)
- Existing code: `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:166-198,200-218`

## Acceptance Criteria
- [ ] AC-097.1: Every key in `MordecaiIMC_Runtime` is a valid engine `FKey` (`FKey::IsValid()`); the stick and L3 mappings use `Gamepad_Left2D`, `Gamepad_Right2D`, `Gamepad_LeftThumbstick` — verified by `Mordecai.Recovery.Input.AllRuntimeKeysValid`
- [ ] AC-097.2: In the packaged build, the player pushes the left stick forward and moves +X (> 100 cm in 0.5 s) — verified by check `gamepad_move` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-097.3: In the packaged build, the player clicks L3 and sprints (> 700 cm in 1 s of forward), clicks again to stop — verified by check `sprint_l3` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-097.4: The right stick drives the Look/Aim action (its input reaches the aim handler) — verified by `Mordecai.Recovery.Input.RightStickReachesLookHandler`; the visible facing result is US-098 (`facing_stick` is NOT claimed here)

## Technical Notes
- AC-097.1's test iterates the IMC mappings and fails on any invalid key, so this class of bug cannot return silently.
- TODO(DECISION) (Jeff): gamepad spell buttons — AC-057.5 maps Spell 1 = LB/Q, Spell 2 = RB/E, Spell 3 = LB+RB/R, but `control_bindings_v1.1.md` says Skill A = RB/Q and Skill B = LB/E, and LB+RB for Spell 3 is not mapped at all (R only, `MordecaiHeroComponent.cpp:210-213`). Which is authoritative? Do not change the spell gamepad mapping until decided; no Tier 4 check covers gamepad spells today (us057 Notes).
- Script bookkeeping (before packaging): remove the gamepad-FKey `known_issue` from `gamepad_move` and `sprint_l3` in `us001_movement.py`; `facing_stick` keeps its facing `known_issue` (US-098), drop only the FKey part.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Input.AllRuntimeKeysValid` — AC-097.1
- `Mordecai.Recovery.Input.RightStickReachesLookHandler` — AC-097.4
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_spawn.py` or a new input smoke: every runtime IMC key is valid
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us001_movement.py` checks `gamepad_move`, `sprint_l3`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience; us001's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-001` on the same package
- **Player steps:** push the left stick forward 0.5 s; on lane L1 click L3 and push forward 1 s, click L3 again
- **Visible on screen:** the player moves in the diorama arena (state-checked); us001 screenshots unchanged
- **Proof:** Tier 4 checks `gamepad_move`, `sprint_l3` of `Scripts/playtests/us001_movement.py`
- **Tier 4 script:** `Scripts/playtests/us001_movement.py` — run: `py Scripts/playtest_story.py US-001`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us001 | `sprint_l3` | tap Gamepad_LeftThumbstick (L3), hold W 1s -> dx > 700 cm (then tap L3 again) | FAIL (known): 282 cm |
| us001 | `gamepad_move` | axis Gamepad_LeftY=1 for 0.5s -> player.location x rises > 100 cm | FAIL (known): 0 cm |

- **Playable alone:** YES (with US-083)

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-001`: `gamepad_move` and `sprint_l3` go from FAIL to PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issue "Gamepad stick and L3 mappings use invalid FKey names" marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-097]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
