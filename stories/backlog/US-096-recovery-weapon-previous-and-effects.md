# US-096: Recovery — Previous-Weapon Key (Shift+Tab vs BackSlash) + Weapon Effects Reach the ASC

**Epic:** Epic R: Playability Recovery
**Priority:** P2
**Depends on:** TODO(DECISION) N9 (Jeff) for AC-096.1/096.2; US-083 (sprint on Left Shift is bound there)
**Paired story:** none

## Overview
AC-078.7 promises Shift+Tab for "previous weapon", but the game maps only BackSlash and Tab still fires "next" when Shift is held (N9): Shift+Tab on the Mace goes to the Longsword instead of the Spear. Shift is also the sprint toggle (control_bindings), so a real Shift+Tab chord also toggles sprint once US-083 binds it. Separately, the equipped weapon's `GrantedTags` and `StatModifiers` never reach the player's ASC, because `UMordecaiEquipmentComponent` looks for the ASC on the character while Lyra keeps it on the PlayerState. Features: weapons.

## Execution Mode
**HEADLESS**
Programmatic IMC mapping (chord trigger) and the equipment component's ASC lookup, all C++.

## References
- `Specs/agent_playtest_plan_v1.md` — N9 (`MordecaiHeroComponent.cpp:187-192`), "Weapon effects never reach the ASC" (`MordecaiEquipmentComponent.cpp:55-56`), B2 (no equip slot on `player.weapon`, AC-078.15)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184940-us078`: `cycle_prev_shift_tab` FAIL known ("tapped LeftShift+Tab on 4 'Mace'; expected index 3 'Spear' ... the weapon is index 0/5 'Longsword'"); `cycle_prev_backslash` PASS; AC-078.15 PENDING (bridge cannot drive: no equip slot)
- Done stories: US-078 AC-078.7 ("Bound to Shift+Tab (MKB) only"), AC-078.8, AC-078.15; US-077 AC-077.4; US-024 (weapon GrantedTags / StatModifiers via GAS)
- `Specs/control_bindings_v1.1.md` — Tab = Weapon Set Swap; Left Shift = Sprint toggle; no "previous weapon" row
- Existing code: `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:142-192,240-248`; `Source/LyraGame/Mordecai/Weapons/MordecaiEquipmentComponent.cpp:55-56`

## Acceptance Criteria
- [ ] AC-096.1 (per TODO(DECISION) N9): the previous-weapon input is what Jeff decides — Option A: Shift+Tab chord cycles to the previous weapon, Tab alone still cycles next, and the chord does NOT toggle sprint; Option B: BackSlash stays and AC-078.7 is amended (recorded in `Specs/design_decisions_log.md`) — verified by `Mordecai.Recovery.Weapons.PreviousWeaponInput`
- [ ] AC-096.2 (Option A): In the packaged build, on the Mace the player presses Shift+Tab and the Spear is equipped, and sprint is not toggled — verified by check `cycle_prev_shift_tab` of `Scripts/playtests/us078_weapons.py` (Tier 4). (Option B: the check is removed from the us078 plan with a note citing the decision; `cycle_prev_backslash` remains the proof)
- [ ] AC-096.3: Equipping a weapon applies its `GrantedTags` and `StatModifiers` to the ASC found via the owning PlayerState (Lyra pattern), and unequipping removes them — verified by `Mordecai.Recovery.Weapons.EffectsAppliedToPlayerStateASC`
- [ ] AC-096.4: AC-078.15 (two-hand clears both slots) stays PENDING TIER 4 (bridge cannot drive: B2 — no equip slot on `player.weapon`); Tier 1 only, not claimed

## Technical Notes
- TODO(DECISION) N9 (Jeff): Shift+Tab (AC-078.7) vs BackSlash (current code; the code comment calls it "a pragmatic alternative"). Note the conflict with Sprint (Left Shift toggle): Option A needs the sprint toggle to fire on Shift release-without-chord, or on a different trigger, so that Shift+Tab does not also toggle sprint. Also: control_bindings lists Tab as "Weapon Set Swap (Set 1 ↔ Set 2)", not "next weapon" — the 5-weapon cycle itself is a US-078 test-arena convenience; flag if Jeff wants that revisited.
- Option A implementation: Enhanced Input `UInputTriggerChordAction` on Tab with a "Shift held" chord action; give the plain-Tab mapping a blocker (`UInputTriggerChordBlocker`) so it does not fire with Shift held.
- ASC lookup: use `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn)` (Lyra's character implements the interface and forwards to the PlayerState) or the PlayerState directly; re-apply on possession/respawn.
- Script bookkeeping (before packaging): Option A — remove the N9 `known_issue` from `cycle_prev_shift_tab`; Option B — remove the step, update AC ids in the plan and US-078's Steps table so `--lint` passes.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Weapons.PreviousWeaponInput` — AC-096.1
- `Mordecai.Recovery.Weapons.EffectsAppliedToPlayerStateASC` — AC-096.3
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_weapons.py` (new or extended): `[LOGIC]` equip each weapon → its granted tags on the PlayerState ASC
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us078_weapons.py` check `cycle_prev_shift_tab` (Option A)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-078` on the same package
- **Player steps:** tap Tab four times (Mace), press Shift+Tab (Option A) or BackSlash (Option B)
- **Visible on screen:** the weapon HUD (after US-091) reads "Spear"; the player is not sprinting
- **Proof:** Tier 4 check `cycle_prev_shift_tab` (Option A) / `cycle_prev_backslash` (Option B) of `Scripts/playtests/us078_weapons.py`
- **Tier 4 script:** `Scripts/playtests/us078_weapons.py` — run: `py Scripts/playtest_story.py US-078`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us078 | `cycle_prev_shift_tab` | tap LeftShift+Tab on the Mace -> wait_until player.weapon.index==3, display_name 'Spear' | FAIL (known N9): Longsword |
| us078 | `cycle_prev_backslash` | tap BackSlash on the first weapon -> wait_until player.weapon.index==4, display_name 'Mace' | PASS |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off (AC-096.4 stays unchecked: PENDING TIER 4, not claimed)
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-078`: `cycle_prev_shift_tab` goes from FAIL to PASS (Option A) or is removed per the recorded decision (Option B); no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issues N9 and "Weapon effects never reach the ASC" marked FIXED/decided in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-096]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
