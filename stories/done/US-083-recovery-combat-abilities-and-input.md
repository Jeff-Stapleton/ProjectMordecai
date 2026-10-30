# US-083: Recovery — Player Combat Abilities Granted at Runtime + Attack/Dodge/Block/Sprint/Jump Bound

**Epic:** Epic R: Playability Recovery
**Priority:** P0
**Depends on:** none
**Paired story:** none

## Overview
In the packaged build the player cannot attack, heavy-attack, dodge, block, parry, sprint or jump: the combat abilities exist only as an unsaved CDO edit (N10) and the programmatic input context maps Sprint/Jump without binding them and never maps attack/block/dodge at all. This story makes the player's melee loop real: abilities granted from native code, combat input mapped and bound in `MordecaiIMC_Runtime`, and the melee ability fed by the equipped weapon's attack profiles. Features: melee_attack, dodge, block_parry, sprint (movement). It unblocks the kill path (US-086) and the US-054 / US-078 / US-001 combat checks.

## Execution Mode
**HEADLESS**
All fixes are C++ (ability granting, IMC mapping/binding, melee profile routing); the input actions (`IA_Mordecai_LightAttack`, `IA_Mordecai_HeavyAttack`, `IA_Mordecai_Block`, `IA_Mordecai_Dodge`, `IA_Mordecai_Jump`, `IA_Mordecai_Sprint`) and the weapon attack profiles (`/MordecaiCore/Weapons/AttackProfiles/DA_AP_*`) already exist as committed content.

## References
- `Specs/agent_playtest_plan_v1.md` — Known issues: "Sprint, jump, attacks, block and dodge are not bound at runtime"; N10; "Melee `AttackProfiles` CDO write ... same GC crash pattern" (FIXED, issue #40)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md` (fresh package of `8ea853b`): runs `Saved/Playtests/20261022-184725-us001`, `Saved/Playtests/20261022-184803-us054`, `Saved/Playtests/20261022-184940-us078`
- `Specs/control_bindings_v1.1.md` — Light Attack RT tap / LMB tap; Heavy RT hold / LMB hold; Block LT hold / RMB hold; Parry LT+RT / RMB+LMB timed; Dodge B / Left Alt; Jump A / Space; Sprint (toggle) L3 / Left Shift
- `Specs/combat_system_v1.md`, `Specs/attack_taxonomy_v1.md`, `Specs/player_attacks_agent_brief_v1.md` — light/heavy input semantics
- Done stories whose ACs this restores: US-054 AC-054.5, AC-054.6; US-078 AC-078.13, AC-078.14; US-001 AC-1.3.3, AC-2.1.3, AC-2.1.4
- Existing code:
  - `Source/LyraGame/Mordecai/MordecaiPlayerController.cpp:114-147` (`GrantCombatAbilities`; iterates `DefaultCombatAbilities`, which only `Scripts/us054_10_configure_all.py:15-42` ever set, on the CDO, unsaved; the melee CDO write is gone since issue #40)
  - `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:122-134` (Jump and Sprint mapped, never bound — no `BindAction` for them at :211-259); no LMB/RMB/LeftAlt mapping anywhere in :76-209
  - `Source/LyraGame/Mordecai/Weapons/MordecaiEquipmentComponent.cpp:269-294` (`GetActiveLightAttackProfiles`, `GetActiveHeavyAttackProfile` — the equipped weapon's profiles)
  - `Source/LyraGame/Mordecai/Combat/MordecaiGA_MeleeAttack.*`, `MordecaiGA_Dodge.*`, `MordecaiGA_Block.*`, `MordecaiGA_Parry.*`
  - `MordecaiCharacter::SetSprinting` (no caller today)

## Acceptance Criteria
- [x] AC-083.1: After possession in a game world (no editor state), the player's ASC (on the PlayerState) holds ability specs for MeleeAttack, Dodge, Block and Parry; the list comes from native defaults (constructor / `TSubclassOf` defaults in C++), not from a CDO edit made by a script — verified by `Mordecai.Recovery.Combat.AbilitiesGrantedFromNativeDefaults`
- [x] AC-083.2: `GrantCombatAbilities` no longer writes `AttackProfiles` onto the `UMordecaiGA_MeleeAttack` CDO; light attacks use `UMordecaiEquipmentComponent::GetActiveLightAttackProfiles()` and heavy attacks `GetActiveHeavyAttackProfile()` of the equipped weapon (Unarmed profiles when nothing is equipped) — verified by `Mordecai.Recovery.Combat.MeleeUsesEquippedWeaponProfiles` and `Mordecai.Recovery.Combat.NoMeleeCDOWrite` _(Split at Jeff's call: the unarmed clause is not met in the game - no unarmed attack profiles exist in content (`UnarmedAttackProfiles` is never set and there is no `DA_AP_Unarmed_*`), so with nothing equipped both attacks end at once; it is unreachable in the arena today because the loadout always equips a weapon. Filed as N29 in the Known issues backlog of `Specs/agent_playtest_plan_v1.md`, `TODO(DECISION)`: unarmed content DataAssets vs `UMordecaiWeaponProfileFactory::CreateUnarmed`. This AC is the weapon routing and the removed CDO write.)_
- [x] AC-083.3: `MordecaiIMC_Runtime` maps and binds: LeftMouseButton + Gamepad_RightTriggerAxis (tap) → light attack, the same keys held → heavy attack, RightMouseButton + Gamepad_LeftTriggerAxis (hold) → block, RMB+LMB / LT+RT → parry, LeftAlt + Gamepad_FaceButton_Right → dodge, SpaceBar + Gamepad_FaceButton_Bottom → jump (`ACharacter::Jump`/`StopJumping`), LeftShift → sprint toggle (`SetSprinting`) — verified by `Mordecai.Recovery.Input.CombatActionsMappedAndBound`
- [x] AC-083.4: In the packaged build, the player clicks the left mouse button and the light attack starts (`Mordecai.State.Attacking`) — verified by check `light_attack_lmb_tap` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `light_attack_lmb` of `Scripts/playtests/us078_weapons.py` (Tier 4) and check `melee_lmb` of `Scripts/playtests/us054_arena.py` (Tier 4, after AC-083.9) _(us083 checks added by the implementing agent: the owning stories' scripts exit 1 on other stories' failures, and only an exit-0 run accepts an AC, so the story's own script plays these inputs too.)_ _(Closed at Jeff's call, 2026-10-24: passes on real input on fresh packages, but only in runs that exit 1 on N14 body-blocking or other stories' failures, so no exit-0 run accepted it; see Completion Notes and T1)_
- [x] AC-083.5: In the packaged build, the player holds the left mouse button and a heavy attack starts — verified by check `heavy_attack_lmb_hold` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `heavy_attack_hold` of `Scripts/playtests/us078_weapons.py` (Tier 4) _(Closed at Jeff's call, 2026-10-24: passes on real input on fresh packages, but only in runs that exit 1 on N14 body-blocking or other stories' failures, so no exit-0 run accepted it; see Completion Notes and T1)_
- [x] AC-083.6: In the packaged build, the player taps Left Alt and dodges (Dodging tag or > 150 cm displacement) — verified by check `dodge_left_alt` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `dodge_lalt` of `Scripts/playtests/us054_arena.py` (Tier 4) _(Closed at Jeff's call, 2026-10-24: passes on real input on fresh packages, but only in runs that exit 1 on N14 body-blocking or other stories' failures, so no exit-0 run accepted it; see Completion Notes and T1)_
- [x] AC-083.7: In the packaged build, the player taps Space and jumps (`movement_mode == Falling`) — verified by check `jump_space_bar` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `jump_space` of `Scripts/playtests/us001_movement.py` (Tier 4) _(Closed at Jeff's call, 2026-10-24: passes on real input on fresh packages, but only in runs that exit 1 on N14 body-blocking or other stories' failures, so no exit-0 run accepted it; see Completion Notes and T1)_
- [x] AC-083.8: In the packaged build, the player taps Left Shift and sprints (> 700 cm in 1 s of W), and a second tap toggles back — verified by check `sprint_left_shift` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run, in lane L1) and by check `sprint_shift` of `Scripts/playtests/us001_movement.py` (Tier 4) _(Closed at Jeff's call, 2026-10-24: sprint_shift passes in us001 on 01abb8b (dx 747 cm, toggle-back 600 cm/s), a run that exits 1 on other stories' failures; the us083 sprint never ran unobstructed, because the invisible enemies body-blocked the lane walk or the sprint (N14); see Completion Notes and T1)_
- [x] AC-083.9: `melee_lmb` in `Scripts/playtests/us054_arena.py` compares the nearest enemy's health with its value read before the click (not the constant 200). Today it PASSes falsely (`FIXED?` in the sweep) only because the enemies have 100 max health (N13, US-085). Offline test of the script against `Scripts/tests/fake_bridge.py` (`py Scripts/run_python_tests.py`)
- [x] AC-083.10: In the packaged build, block and parry work with real input on the gamepad: LT held blocks (`Mordecai.State.Blocking`), RT pressed while blocking parries and drops the guard without attacking, the guard comes back after the parry and its whiff while LT is held, and releasing LT ends the block — verified by checks `block_lt_hold`, `parry_rt_while_blocking`, `guard_returns_after_parry` and `block_release_lt` of `Scripts/playtests/us083_combat_input.py` (Tier 4). _(Split by the implementing agent: the bridge can hold a gamepad trigger across state reads, so the gamepad path is not B2; the mouse path is AC-083.11.)_
- [ ] AC-083.11: Block (RMB hold) and parry (RMB+LMB) with real input on the mouse: PENDING TIER 4 (bridge cannot drive: B2 — the playtest client cannot hold a mouse button across state reads). Tier 1 covers the binding (AC-083.3); this AC is not claimed

## Technical Notes
- Grant path: populate `DefaultCombatAbilities` in `AMordecaiPlayerController`'s constructor (`UMordecaiGA_MeleeAttack::StaticClass()`, Dodge, Block, Parry) — native class refs need no content and are always cooked. Remove the reliance on `Scripts/us054_10_configure_all.py`'s unsaved CDO edit; leave the script but stop it writing the CDO (or tag the commit `[no-content]` with the reason).
- Melee CDO write: removed by issue #40 (same GC-crash pattern the spells had). The controller is the melee grant's SourceObject and `UMordecaiGA_MeleeAttack::TakeAttackProfilesFromSpec` copies `MeleeAttackProfiles` onto the instance in `OnGiveAbility`. The ability instance should read profiles from the avatar's `UMordecaiEquipmentComponent` at activation; light attacks already do (`GetResolvedAttackProfiles`), and heavy routing (`GetActiveHeavyAttackProfile`) is still this story's.
- Input: load the existing IAs by path like the others in `MordecaiHeroComponent.cpp:63-67` (all of `/MordecaiCore` is cooked). Light vs heavy: use Enhanced Input `Tap` / `Hold` triggers on the same key, or one action with `Started`/`Triggered` routing. `TODO(DECISION)`: hold threshold for heavy (control_bindings: "charge time depends on weapon"); default to 0.4 s and expose it as a `UPROPERTY` until Jeff decides.
- Activation: activate by ability class or by an input tag on the ASC (Lyra pattern `AbilityInputTagPressed`), whichever the existing GAs support; do not call gameplay functions from input handlers that bypass GAS.
- Gamepad B is both Dodge (game) and Back (CommonUI menus); in Menu input mode game input is blocked, so there is no conflict — verify `gamepad_back_closes` in `us069_pause_menu.py` still PASSes.
- Gamepad L3 sprint needs the FKey fix in US-097 (`Gamepad_LeftThumbstickButton` → `Gamepad_LeftThumbstick`); `sprint_l3` is NOT this story's check.
- Observed in `Saved/Playtests/20261022-184940-us078`: `player.tags` contained `Status.Crouching` after the weapon-cycling steps. Triage whether Lyra's crouch ability (bound to a key the script pressed?) is granted by the arena experience and whether it blocks Mordecai ability activation; file it if it is a separate bug.
- Script bookkeeping (commit with the code, BEFORE packaging): remove the `known_issue` from the Steps `sprint_shift`, `jump_space` (us001), `melee_lmb`, `dodge_lalt` (us054), `light_attack_lmb`, `heavy_attack_hold` (us078); `sprint_l3` keeps its gamepad-FKey `known_issue` (US-097). Update the `does` text of `melee_lmb` for AC-083.9 and refresh the owning stories' Steps tables (`--plan`) only via the implementing agent's closing notes (do not edit stories/done files beyond what `--lint` needs; if `--lint` needs the US-054 table refreshed, do it in the same commit).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Combat.AbilitiesGrantedFromNativeDefaults` — AC-083.1
- `Mordecai.Recovery.Combat.MeleeUsesEquippedWeaponProfiles` — AC-083.2
- `Mordecai.Recovery.Combat.NoMeleeCDOWrite` — AC-083.2 (the CDO's `AttackProfiles` stays empty after granting). Exists since issue #40 in `Source/LyraGame/Tests/Combat/MordecaiMeleeGrantTests.cpp`, next to `Mordecai.Combat.MeleeGrant.*`; keep them green. Its `FMordecaiCombatGrantTestEnv` can host `AbilitiesGrantedFromNativeDefaults` (`GrantCombatAbilities` is public for testing)
- `Mordecai.Recovery.Input.CombatActionsMappedAndBound` — AC-083.3 (inspects the runtime IMC mappings and the EIC bindings)
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_spawn.py` + a new `Scripts/verify_pie_combat_input.py`: the PIE pawn's ASC has the 4 combat specs; `MordecaiIMC_Runtime` contains LMB/RMB/LeftAlt/SpaceBar/LeftShift mappings. Any check that activates an ability directly is labeled `[LOGIC]`
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- Existing checks: `sprint_shift`, `jump_space` of `Scripts/playtests/us001_movement.py`; `melee_lmb`, `dodge_lalt` of `Scripts/playtests/us054_arena.py`; `light_attack_lmb`, `heavy_attack_hold` of `Scripts/playtests/us078_weapons.py`
- New script `Scripts/playtests/us083_combat_input.py`: the gamepad (block, parry and its input rules, RT light/heavy attack, B dodge, A jump) AND the keyboard/mouse inputs (LMB tap/hold, LeftAlt dodge, Space jump, LeftShift sprint in lane L1); accepts AC-083.3..AC-083.8 and AC-083.10
- `melee_lmb` fix (AC-083.9) tested offline: `py Scripts/run_python_tests.py`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → B_MordecaiArenaExperience).
  - Tier 4: the story's own run `py Scripts/playtest_story.py US-083` (MovementTestMap, same package) accepts AC-083.3..AC-083.8 and AC-083.10.
  - Then the owning stories' scripts on the same package: `py Scripts/playtest_story.py US-001` (MovementTestMap), `py Scripts/playtest_story.py US-054` (its `us054_combat.py` on CombatTestMap plays LMB and Left Alt), `py Scripts/playtest_story.py US-078` (CombatTestMap).
  - Since US-100 (T1, issue #46) us083 runs whole on the enemy-free MovementTestMap: none of its checks needs an enemy, because block, parry and attacks are judged by the player's tags. On DevTestMap the Frontliners body-blocked its lane walk (N14) and killed the player mid-run.
- **Player steps:** `us083_combat_input.py`, first, from the spawn before any dodge: walk to lane L1, run up along it to (-300, -800), tap Left Shift then hold W 1 s (sprint), tap Left Shift again and hold S 1 s (back to a walk). MovementTestMap keeps DevTestMap's PlayerStart (0, 0) and lane L1, so the waypoints and the walk-first order are unchanged; on DevTestMap they existed to keep the enemies behind the player. Then the gamepad: hold LT (block), pull RT while blocking (parry), keep LT held until the guard returns, release LT; LT + RT at 0.3 (no parry); RT held, then LT (block, no parry); tap RT (light attack), hold RT 0.8 s (heavy), tap B (dodge), tap A (jump). Then the keyboard and mouse: click LMB (light attack), hold LMB 0.8 s (heavy), tap Left Alt (dodge), tap Space (jump). The owning stories' scripts play the same keyboard/mouse inputs: us001 Space and Left Shift in lane L1 (MovementTestMap); us054 LMB and Left Alt next to the CombatTestMap TestDummy (`us054_combat.py`); us078 LMB tap and hold at the CombatTestMap spawn
- **Visible on screen:** the player on MovementTestMap's grid floor with the combat HUD and no enemy; the attack/dodge are state-checked (tags, displacement) — us083 screenshots `lane_start` (the player at the lane L1 start) and `blocking` (LT held, after the sprint); the us054_combat screenshot beside the TestDummy shows the player next to its target
- **Proof:** Tier 3 boot shows the MovementTestMap floor, the player and the HUD; Tier 4 `Scripts/playtests/us083_combat_input.py` (the story's own run, exit 0) accepts AC-083.3..AC-083.8 and AC-083.10: gamepad block and parry (AC-083.10), the gamepad attack/dodge/jump keys, and the keyboard/mouse checks `light_attack_lmb_tap`, `heavy_attack_lmb_hold`, `dodge_left_alt`, `jump_space_bar`, `sprint_left_shift`; the owning stories' checks `jump_space`, `sprint_shift` (us001), `melee_lmb`, `dodge_lalt` (us054, in `us054_combat.py` since US-100), `light_attack_lmb`, `heavy_attack_hold` (us078) play the same inputs; mouse block/parry (AC-083.11) PENDING TIER 4 (bridge cannot drive: B2 mouse hold)
- **Tier 4 script:** `Scripts/playtests/us083_combat_input.py` (`py Scripts/playtest_story.py US-083`) for the gamepad AND the keyboard/mouse inputs (LMB tap/hold, LeftAlt, Space, LeftShift sprint in lane L1), plus the owning stories' scripts — `Scripts/playtests/us001_movement.py` (`py Scripts/playtest_story.py US-001`), `Scripts/playtests/us054_combat.py` (`py Scripts/playtest_story.py US-054`, which also runs `us054_arena.py`), `Scripts/playtests/us078_weapons.py` (`py Scripts/playtest_story.py US-078`); run them together with `py Scripts/playtest_story.py --all` on the same package (that also covers `gamepad_back_closes` of us069, Technical Notes)
- **Steps:** `Scripts/playtests/us083_combat_input.py` (from `py Scripts/playtest_story.py US-083 --plan`):

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `walk_lane_wp1` | walk_to(-650, 0) -> within 60 cm (waypoint to lane L1: west along y=0 from the spawn) | - |
| 2 | `walk_lane_wp2` | walk_to(-700, -650) -> within 60 cm (waypoint to lane L1: south to us001's waypoint) | - |
| 3 | `walk_lane_start` | walk_to(-700, -800) -> within 60 cm (lane L1 start) | - |
| 4 | `walk_lane_run_up` | walk_to(-300, -800) -> within 60 cm (along lane L1, +X: the sprint start) | - |
| 5 | `sprint_left_shift` | at the run-up point: tap LeftShift, hold W 1s -> dx > 700 cm; tap LeftShift again, hold S 1s reading player.speed every 0.05s -> back to a walk (peak 500..750 cm/s) | AC-083.8, AC-083.3 |
| 6 | `block_lt_hold` | axis Gamepad_LeftTriggerAxis=1 (held) -> 'Mordecai.State.Blocking' in player.tags within 1s | AC-083.10, AC-083.3 |
| 7 | `parry_rt_while_blocking` | LT held, axis Gamepad_RightTriggerAxis=1 -> within 0.5s a parry tag (Mordecai.State.Parrying, Mordecai.State.ParryWhiff, Mordecai.State.Riposte) with no 'Mordecai.State.Blocking' and no 'Mordecai.State.Attacking'; then RT=0 | AC-083.10, AC-083.3 |
| 8 | `guard_returns_after_parry` | LT still held -> 'Mordecai.State.Blocking' again without a parry tag within 2.5s, and no 'Mordecai.State.Attacking' at any read meanwhile | AC-083.10 |
| 9 | `block_release_lt` | axis Gamepad_LeftTriggerAxis=0 -> 'Mordecai.State.Blocking' gone within 1s | AC-083.10 |
| 10 | `partial_rt_no_parry` | LT held (Blocking), axis Gamepad_RightTriggerAxis=0.3 for 0.6s -> no parry tag and 'Mordecai.State.Blocking' at every read; then both 0 | AC-083.3 |
| 11 | `block_over_held_rt_no_parry` | axis Gamepad_RightTriggerAxis=1, 0.15s later Gamepad_LeftTriggerAxis=1 -> 'Mordecai.State.Blocking' within 1s and no parry tag for 0.6s more; then both 0 | AC-083.3 |
| 12 | `light_attack_rt_tap` | idle -> axis Gamepad_RightTriggerAxis=1 hold_s 0.1 -> 'Mordecai.State.Attacking' without 'Mordecai.Attack.Heavy' within 1s | AC-083.3 |
| 13 | `heavy_attack_rt_hold` | idle -> axis Gamepad_RightTriggerAxis=1 hold_s 0.8 -> 'Mordecai.Attack.Heavy' in player.tags within 0.5s of the release | AC-083.3 |
| 14 | `dodge_gamepad_b` | tap Gamepad_FaceButton_Right (B) -> a Dodging tag in player.tags within 0.5s (the displacement is recorded, not judged: N27) | AC-083.3 |
| 15 | `jump_gamepad_a` | wait until the dodge is over (no 'Mordecai.State.Dodging') and player.movement_mode=="Walking", tap Gamepad_FaceButton_Bottom (A) -> wait_until player.movement_mode=="Falling" within 1s | AC-083.3 |
| 16 | `light_attack_lmb_tap` | idle -> click Left at (640, 360) (the bridge's default click) -> 'Mordecai.State.Attacking' without 'Mordecai.Attack.Heavy' within 1s | AC-083.4, AC-083.3 |
| 17 | `heavy_attack_lmb_hold` | idle -> click Left at (640, 360) held 0.8s -> 'Mordecai.Attack.Heavy' in player.tags within 0.5s of the release | AC-083.5, AC-083.3 |
| 18 | `dodge_left_alt` | idle (and no Dodging / DodgeCooldown tag left) -> tap LeftAlt -> a Dodging tag in player.tags within 0.5s (the displacement is recorded, not judged: N27) | AC-083.6, AC-083.3 |
| 19 | `jump_space_bar` | wait until the dodge is over (no 'Mordecai.State.Dodging') and player.movement_mode=="Walking", tap SpaceBar -> wait_until player.movement_mode=="Falling" within 1s | AC-083.7, AC-083.3 |

  The owning stories' rows (from `py Scripts/playtest_story.py US-0XX --plan`):

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us001 | `sprint_shift` | tap LeftShift, hold W 1s -> dx > 700 cm (then tap LeftShift again to toggle back) | FAIL (known): dx 550 cm |
| us001 | `jump_space` | tap SpaceBar -> wait_until player.movement_mode=="Falling" within 1s | FAIL (known): stayed Walking |
| us054 | `melee_lmb` | click Left at (640, 360) -> within 1s 'Mordecai.State.Attacking' in player.tags or the nearest enemy's health drops (AC-083.9) | PASS (FIXED?) — false positive, enemy at 100/100 |
| us054 | `dodge_lalt` | tap LeftAlt -> within 0.5s a Dodging tag in player.tags or a displacement > 150 cm | FAIL (known): moved 0 cm |
| us078 | `light_attack_lmb` | no Mordecai.State.Attacking -> click Left at (640, 360) -> Mordecai.State.Attacking in player.tags within 1 s | FAIL (known) |
| us078 | `heavy_attack_hold` | click Left at (640, 360) held 0.8 s -> Mordecai.State.Attacking in player.tags within 0.5 s | FAIL (known) |

- **Playable alone:** YES

## Definition of Done
- [x] All ACs checked off (AC-083.11 stays unchecked: PENDING TIER 4, not claimed)
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: no asset changes expected; `py Scripts/check_content_committed.py` exits 0
- [x] Code AND the script edits (`known_issue` removed from the six fixed Steps, `melee_lmb` made honest) committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [x] On that same fresh package: `py Scripts/playtest_story.py US-001`, `py Scripts/playtest_story.py US-054` and `py Scripts/playtest_story.py US-078` run (or `--all`, which also runs us069's `gamepad_back_closes`); checks `sprint_shift`, `jump_space`, `melee_lmb`, `dodge_lalt`, `light_attack_lmb`, `heavy_attack_hold` go from FAIL (sweep `Saved/Playtests/20261022-184702-sweep`) to PASS; no check that PASSed in the sweep now FAILs; every remaining FAIL belongs to another Epic R story (cite which). `--lint` exits 0 for US-001, US-054 and US-078 — never `--allow-stale` _(met on `01abb8b` with one correction: in that sweep `melee_lmb` was a false PASS (AC-083.9), not a FAIL, so five of the six went FAIL→PASS)_
- [x] On that same fresh package: `py Scripts/playtest_story.py US-083` (Tier 3 boot + `Scripts/playtests/us083_combat_input.py`) exits 0; `--lint` exits 0 for US-083 _(met on `01abb8b` by the gamepad script, twice. With the keyboard/mouse half added (`ef69eb0`, `5cdadd5`), every input check passes, but the lane walk is body-blocked (N14), so the run exits 1. Accepted at Jeff's call, 2026-10-24.)_
- [x] Known issues entries "Sprint, jump, attacks, block and dodge are not bound at runtime" and N10 marked FIXED in `Specs/agent_playtest_plan_v1.md` with the run dirs
- [x] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [x] Evidence cited: run dirs (Tier 3 `Saved/Playtests/<run>-boot/report.md`, Tier 4 `Saved/Playtests/<run>-us083|us001|us054|us078/report.md`) in Completion Notes AND in the body of the `[US-083]` commit that closes the story
- [x] Pushed; story moved to `stories/done/` only after all of the above pass. A failing playtest is reported honestly and the story stays in progress _(moved at Jeff's call, 2026-10-24, with the N14 failures reported above)_

## Completion Notes
**Closed at Jeff's call (2026-10-24).**
- Every combat input this story binds is played with real input on fresh packages, and every input check PASSes.
- The runs that play the keyboard/mouse half (AC-083.4..AC-083.8) still exit 1, so the kit accepted none of those ACs. The only failures are the lane walk and the sprint, which the arena's invisible enemies body-block (N14, US-085's).
- Jeff's direction: playtests need their own maps (filed as T1 in `Specs/agent_playtest_plan_v1.md`).
- AC-083.11 (mouse block/parry) stays unchecked: PENDING TIER 4 (bridge cannot drive: B2).

**Commits** on branch `fix/issue-15-combat-abilities-input` (issue #15):
- `4b311c6`: native grant, heavy attack, input mapping and binding, ability rules, Tier 1 tests, PIE script, docs.
- `ad08fb8`: playtest scripts, `Scripts/playtests/us083_combat_input.py` (gamepad), offline tests.
- `01abb8b`: `Scripts/us054_10_configure_all.py` no longer writes the player controller CDO, `[no-content]`.
- `ef69eb0`: the us083 script plays the keyboard/mouse inputs too (AC-083.4..AC-083.8); N29 filed.
- `5cdadd5`: the us083 script walks and sprints first, with a run-up along lane L1.

The game code last changed in `01abb8b`.

**Packages** (all `py Scripts/package_playtest.py` exit 0, `dirty: false`, never `--allow-stale`):
- `01abb8be0c`: UAT 8 min 47 s, `Saved/Playtests/package-20261023-191432.log`.
- `ef69eb0d70`: UAT 6 min 43 s, `Saved/Playtests/package-20261024-105448.log`.
- `5cdadd5410`: 5.3 min, `Saved/Playtests/package-20261024-130754.log`.

**Decisions taken for Jeff (`TODO(DECISION)` in code):**
- **Heavy attack timing:** the hold threshold is 0.4 s (`UMordecaiHeroComponent::HeavyAttackHoldSeconds`), and the heavy attack fires on the release.
- **Input tag namespace:** `Mordecai.Input.*`, not `InputTag.Mordecai.*`.
- **Sprint is client-local** (N28).
- **Unarmed attacks:** AC-083.2's unarmed clause was split off at Jeff's call. No unarmed attack profiles exist in the game, so with nothing equipped both attacks end at once. This is latent: the arena always equips a weapon. Filed as N29, with `TODO(DECISION)` DataAssets vs `UMordecaiWeaponProfileFactory::CreateUnarmed`.

**Story changes made while implementing:**
- AC-083.10 split: the gamepad block/parry is AC-083.10; the mouse path is AC-083.11, PENDING B2.
- AC-083.4..AC-083.8 name the us083 checks that play them.
- AC-083.2 split (N29).
- The us083 Steps table comes from `--plan`.

**Tier 1:** `py Scripts/run_tests.py`: 692 passed / 0 failed (`Saved/TestLogs/tests-20261023-183506.log`). Red first: 10 of the 11 `Mordecai.Recovery.*` tests failed for the intended reason. `NoMeleeCDOWrite` was already green since issue #40.

**Tier 2:** `py Scripts/run_pie_verify.py`: 11/11. `Scripts/verify_pie_combat_input.py` passes 29/29:
- the PIE world, controller and pawn;
- the 5 abilities in `DefaultCombatAbilities` and as ASC specs;
- the hero component and `MordecaiIMC_Runtime`, and its 13 key-to-action mappings with their triggers.

It has no `[LOGIC]` checks. Spec input tags and Enhanced Input bindings are covered at Tier 1 only.

**Python tooling:** `py Scripts/run_python_tests.py` on `5cdadd5`: 446 run, 0 failed.
- Earlier full runs each had one load-dependent failure in `test_us057_playtest` `test_rewalks_when_the_enemy_moved`. It is not touched by this story and passes alone.
- The us083 offline tests kill the mutants of each precondition: the jump waits for the dodge to end, the dodge waits for no leftover cooldown, attacks wait for idle, and the run-up.
- `--lint` PASS for US-083, US-001, US-054, US-078.

**Content:** no asset changes. `py Scripts/check_content_committed.py` exit 0.

**Tier 3** (`py Scripts/playtest_boot.py` via `playtest_story.py`). All PASS: B_MordecaiArenaExperience, `content_cooked` PASS, no crash, 0 errors in `game.log`, engine screenshots 3/3.
- Runs: `Saved/Playtests/20261023-192402-boot` and `20261023-193225-boot` (`01abb8b`); `20261024-110226-boot` (`ef69eb0`); `20261024-133928-boot` (`5cdadd5`).
- `engine_01-03.png` in every run show the same view:
  - the Mordecai diorama arena from the fixed overhead camera (white blockout);
  - the grey cylinder player at the centre;
  - the combat HUD top-left (health, stamina, posture bars; SP "10 / 10"), "Kills: 0" top-right;
  - three floating red enemy health bars with no bodies (invisible Frontliners, N13, US-085);
  - the six pickups with rotated name labels, and "[I] Inventory";
  - a yellow Steam socket deprecation message (known): P2PConnectionTimeout or bAllowP2PPacketRelay in some frames, Missing P2PCleanupTimeout in others.
- `shot_01-03.png`:
  - `20261023-193225-boot`: the game window at 1920x1080, the same view; `shot_03` adds a "PassLoadingScreenWindowBackToGame failed. No Window" debug line (logged once at frame 0).
  - The other three runs: a locked desktop: the Windows lock-screen photo (`192402`, `133928`) or black (`110226`). The engine frames are the evidence there.

**Tier 4, US-083's own script** (`py Scripts/playtest_story.py US-083`):
- `01abb8b`, gamepad half (`Saved/Playtests/20261023-193244-us083`; also `20261023-192719-us083` in the sweep): **exit 0, ACCEPTANCE CANDIDATE**, all 10 player checks PASS. Accepted: AC-083.10 and AC-083.3; AC-083.11 PENDING (B2).
  - `block_lt_hold`: Blocking + PerfectBlock.
  - `parry_rt_while_blocking`: Parrying, guard dropped, no attack.
  - `guard_returns_after_parry`: Blocking again after 1.16 s, no attack at any of 18 reads.
  - `block_release_lt`.
  - `partial_rt_no_parry`: 10 reads.
  - `block_over_held_rt_no_parry`: 11 reads.
  - `light_attack_rt_tap`: Attacking without Heavy.
  - `heavy_attack_rt_hold`: `Mordecai.Attack.Heavy`.
  - `dodge_gamepad_b`, `jump_gamepad_a`.
- `ef69eb0`, gamepad + keyboard/mouse (`Saved/Playtests/20261024-110250-us083`): exit 1.
  - All 10 gamepad checks PASS, and so do `light_attack_lmb_tap` (the first Left click after gamepad input), `heavy_attack_lmb_hold`, `dodge_left_alt` and `jump_space_bar`.
  - FAIL `walk_lane_wp1`: after two backward dodges the player was boxed in at (-39, 0) by the invisible enemies. S moved it 0 cm, A and D about 2 cm.
  - So `walk_lane_wp2`, `walk_lane_start` and `sprint_left_shift` SKIP.
  - A diagnostic re-run with `--keep-running` (`20261024-115717-us083`): the walk passed, but `sprint_left_shift` FAILed. It covered 372 cm in 1 s of W, ending 21 cm up (onto an enemy capsule); the toggle-back walk was a normal 600 cm/s. A state read then showed all three Frontliners 194-199 cm from the player, one on the lane ahead.
- `5cdadd5`, walk and sprint first (`Saved/Playtests/20261024-133951-us083`; story summary `Saved/Playtests/20261024-133928-story-us083/summary.md`): exit 1.
  - FAIL `walk_lane_wp1`: stuck at (-8, -7) from the spawn, before any dodge. The enemies crowd the player from the start.
  - The lane, run-up and sprint checks SKIP.
  - The 10 gamepad and 4 keyboard/mouse input checks all PASS again.
- `bridge_01.png` (`blocking`) in every run: the player in the arena with LT held and **"Perfect Block!"** in yellow above "[I] Inventory", the block's on-screen feedback. In the `ef69eb0` and `5cdadd5` runs, two enemy bars sit right on the player.
- In the diagnostic run, `bridge_02.png` (`lane_start`) shows the player at the lane L1 start, south-west corner, with the three enemy bars just +X (the sprint direction). `bridge_01.png` was overwritten by a diagnostic CLI screenshot: the player back near the corner after the sprint, with the bars on the player. In both, the HUD's stamina bar is empty (N30).

**Tier 4, the owning stories' scripts** on `01abb8b` (sweep `Saved/Playtests/20261023-192402-sweep/summary.md`): each exits 1 only on other stories' failures. Compared with the `8ea853b` sweep:
- `sprint_shift`, `jump_space`, `dodge_lalt`, `light_attack_lmb` and `heavy_attack_hold` went FAIL→PASS.
- `melee_lmb` was a false PASS there (enemy health 100 < 200, no Attacking tag). Made honest (AC-083.9), it PASSes on `Mordecai.State.Attacking`.
- `move_a`, `walk_lane_wp1`, `walk_lane_wp2`, `walk_lane_start` and `walk_speed_baseline` went FAIL or SKIP → PASS.
- **No check went PASS→FAIL.**

Per script:
- **us001** (`20261023-192424-us001`):
  - `sprint_shift` PASS: dx 747 cm in 1 s of W (walk baseline 556 cm); after the second tap, S peaks at 600 cm/s.
  - `jump_space` PASS.
  - FAIL, all known:
    - `sprint_l3` (US-097; so AC-1.3.3 and AC-2.1.3 still FAIL);
    - `facing_mouse` (US-098);
    - `facing_stick` (US-097/US-098);
    - `gamepad_move` (US-097).
  - Screenshots:
    - `bridge_01`: the arena at the start, with the HUD, three bars and the pickups.
    - `bridge_02`: the same camera angle with the player at the south-west corner.
    - `bridge_03`: the player in the west lane with the enemy bars beside them, and no facing arrow (N12).
- **us054** (`20261023-192458-us054`):
  - `melee_lmb` PASS.
  - `dodge_lalt` PASS. The check reports 98 cm at the first read while the player was still moving; it came to rest about 111 cm from the start (N27). Attacking and Dodging were on together (N25).
  - FAIL:
    - `enemies_present` (N13, US-085);
    - `enemies_visible` (US-085);
    - `enemy_indicators` (N17, US-092);
    - `enemy_damages_player` (N7, US-084/US-085).
    - The 11 SKIPs that follow belong to US-085 (AC-085.3), US-086 (`death_screen`) and US-090 (`restoration_heals`), plus three screenshots.
  - Screenshots:
    - `bridge_01`: the arena, with three bars and no bodies.
    - `bridge_02`: the player by the Rusty Sword ("[F] Pick up Rusty Sword") with the enemy bars bunched just +X.
- **us078** (`20261023-192629-us078`):
  - `light_attack_lmb` PASS and `heavy_attack_hold` PASS, both with the Dagger (left equipped by `cycle_gamepad_y`).
  - FAIL known: `weapon_hud_text` (N3, US-091) and `cycle_prev_shift_tab` (N9, US-096).
  - `weapon_hud_updates` SKIP, so AC-078.14 is NOT RUN.
  - Screenshots: `bridge_01` and `bridge_02` show the arena and HUD with no weapon name anywhere (N3).
- **us057** (`20261023-192535-us057`): the same FAILs as before.
  - `cooldown_widgets` (N8, US-088);
  - `fireball_projectile` and `fireball_hits_enemy` (N15, US-087);
  - `fireball_cooldown_state` and `cooldown_hud` (US-088/US-089);
  - `stone_skin_indicator` (N16, US-091).
  - Screenshots:
    - `bridge_01`: SP "7 / 10", no damage number.
    - `bridge_02`: SP "5 / 10".
    - `bridge_03` and `bridge_04`: SP "1 / 10" and "Status 10.0" (N16).
- **us069** (`20261023-192602-us069`):
  - `gamepad_back_closes` PASS: B still closes the menu with dodge bound to B (the run does not read the player's tags after that press).
  - FAIL:
    - `character_tab_content`, `skills_tab_content` and `feats_tab_content` (N4/N5, US-093);
    - `shoulder_switches_tab` (N18, US-095);
    - `i_closes` (US-095).
  - Screenshots:
    - `bridge_01`: the Character tab, blank.
    - `bridge_02`: the Skills tab, "Coming Soon".
    - `bridge_03`: the Feats tab, "Coming Soon".
    - `bridge_04`: the menu closed.
- **us079** (`20261023-192645-us079`):
  - FAIL `menu_rows` .. `filter_all`: the stale pooled inventory, US-094.
  - `pickup_labels`: bridge gap B1, a tooling gap that is not an Epic R story.
  - Screenshots:
    - `bridge_01`: the six pickups.
    - `bridge_02`: "Inventory is empty."; the filter bar overlaps "Stored Resources".
    - `bridge_03`: still "Inventory is empty." after six pickups.
    - `bridge_04`: the menu closed.

**Also found and filed** in `Specs/agent_playtest_plan_v1.md`:
- **N23 Left Shift crouched the player:** FIXED. No `Status.Crouching` in the `01abb8b` sweep, against 12 + 31 reads in the `8ea853b` sweep.
- **N24..N28:** combo input unwired, no attack cancel windows, spells ignore Dead/ParryWhiff, the dodge covers about 110 cm (measured), client-local sprint.
- **N29:** no unarmed attack profiles.
- **N30:** the player's stamina never regenerates. Nothing calls `UMordecaiStaminaSystem`'s regen. In about 15 s of combat it fell to the -50 floor and stayed there.
- **T1:** dedicated test maps per playtest.

**Playable status:**
- The light and heavy attack (LMB and RT), dodge (Left Alt and B), jump (Space and A), block and parry (LT and RT), sprint (Left Shift) and WASD movement all work with real input in the packaged game.
- Block and parry on the mouse are not yet accepted (AC-083.11, B2).
- Gamepad stick movement and L3 sprint FAIL (US-097).
- In the arena, the invisible enemies body-block the player (N14), and the player's stamina never regenerates (N30).
