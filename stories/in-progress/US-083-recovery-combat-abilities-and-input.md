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
- [ ] AC-083.1: After possession in a game world (no editor state), the player's ASC (on the PlayerState) holds ability specs for MeleeAttack, Dodge, Block and Parry; the list comes from native defaults (constructor / `TSubclassOf` defaults in C++), not from a CDO edit made by a script — verified by `Mordecai.Recovery.Combat.AbilitiesGrantedFromNativeDefaults`
- [ ] AC-083.2: `GrantCombatAbilities` no longer writes `AttackProfiles` onto the `UMordecaiGA_MeleeAttack` CDO; light attacks use `UMordecaiEquipmentComponent::GetActiveLightAttackProfiles()` and heavy attacks `GetActiveHeavyAttackProfile()` of the equipped weapon (Unarmed profiles when nothing is equipped) — verified by `Mordecai.Recovery.Combat.MeleeUsesEquippedWeaponProfiles` and `Mordecai.Recovery.Combat.NoMeleeCDOWrite` _(Split at Jeff's call: the unarmed clause is not met in the game - no unarmed attack profiles exist in content (`UnarmedAttackProfiles` is never set and there is no `DA_AP_Unarmed_*`), so with nothing equipped both attacks end at once; it is unreachable in the arena today because the loadout always equips a weapon. Filed as N29 in the Known issues backlog of `Specs/agent_playtest_plan_v1.md`, `TODO(DECISION)`: unarmed content DataAssets vs `UMordecaiWeaponProfileFactory::CreateUnarmed`. This AC is the weapon routing and the removed CDO write.)_
- [ ] AC-083.3: `MordecaiIMC_Runtime` maps and binds: LeftMouseButton + Gamepad_RightTriggerAxis (tap) → light attack, the same keys held → heavy attack, RightMouseButton + Gamepad_LeftTriggerAxis (hold) → block, RMB+LMB / LT+RT → parry, LeftAlt + Gamepad_FaceButton_Right → dodge, SpaceBar + Gamepad_FaceButton_Bottom → jump (`ACharacter::Jump`/`StopJumping`), LeftShift → sprint toggle (`SetSprinting`) — verified by `Mordecai.Recovery.Input.CombatActionsMappedAndBound`
- [ ] AC-083.4: In the packaged build, the player clicks the left mouse button and the light attack starts (`Mordecai.State.Attacking`) — verified by check `light_attack_lmb_tap` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `light_attack_lmb` of `Scripts/playtests/us078_weapons.py` (Tier 4) and check `melee_lmb` of `Scripts/playtests/us054_arena.py` (Tier 4, after AC-083.9) _(us083 checks added by the implementing agent: the owning stories' scripts exit 1 on other stories' failures, and only an exit-0 run accepts an AC, so the story's own script plays these inputs too.)_
- [ ] AC-083.5: In the packaged build, the player holds the left mouse button and a heavy attack starts — verified by check `heavy_attack_lmb_hold` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `heavy_attack_hold` of `Scripts/playtests/us078_weapons.py` (Tier 4)
- [ ] AC-083.6: In the packaged build, the player taps Left Alt and dodges (Dodging tag or > 150 cm displacement) — verified by check `dodge_left_alt` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `dodge_lalt` of `Scripts/playtests/us054_arena.py` (Tier 4)
- [ ] AC-083.7: In the packaged build, the player taps Space and jumps (`movement_mode == Falling`) — verified by check `jump_space_bar` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run) and by check `jump_space` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-083.8: In the packaged build, the player taps Left Shift and sprints (> 700 cm in 1 s of W), and a second tap toggles back — verified by check `sprint_left_shift` of `Scripts/playtests/us083_combat_input.py` (Tier 4, the story's own run, in lane L1) and by check `sprint_shift` of `Scripts/playtests/us001_movement.py` (Tier 4)
- [ ] AC-083.9: `melee_lmb` in `Scripts/playtests/us054_arena.py` compares the nearest enemy's health with its value read before the click (not the constant 200). Today it PASSes falsely (`FIXED?` in the sweep) only because the enemies have 100 max health (N13, US-085). Offline test of the script against `Scripts/tests/fake_bridge.py` (`py Scripts/run_python_tests.py`)
- [ ] AC-083.10: In the packaged build, block and parry work with real input on the gamepad: LT held blocks (`Mordecai.State.Blocking`), RT pressed while blocking parries and drops the guard without attacking, the guard comes back after the parry and its whiff while LT is held, and releasing LT ends the block — verified by checks `block_lt_hold`, `parry_rt_while_blocking`, `guard_returns_after_parry` and `block_release_lt` of `Scripts/playtests/us083_combat_input.py` (Tier 4). _(Split by the implementing agent: the bridge can hold a gamepad trigger across state reads, so the gamepad path is not B2; the mouse path is AC-083.11.)_
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
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: the story's own run `py Scripts/playtest_story.py US-083` (DevTestMap, same package), which accepts AC-083.3..AC-083.8 and AC-083.10, then the owning stories' scripts on the same package — `py Scripts/playtest_story.py US-001`, `py Scripts/playtest_story.py US-054`, `py Scripts/playtest_story.py US-078`
- **Player steps:** `us083_combat_input.py`, first, from the spawn before any dodge: walk to lane L1, run up along it to (-300, -800), tap Left Shift then hold W 1 s (sprint), tap Left Shift again and hold S 1 s (back to a walk). The invisible enemies (N14) follow the player at 400 cm/s; walking first, and running up along the lane, keeps them behind it. Then the gamepad: hold LT (block), pull RT while blocking (parry), keep LT held until the guard returns, release LT; LT + RT at 0.3 (no parry); RT held, then LT (block, no parry); tap RT (light attack), hold RT 0.8 s (heavy), tap B (dodge), tap A (jump). Then the keyboard and mouse: click LMB (light attack), hold LMB 0.8 s (heavy), tap Left Alt (dodge), tap Space (jump). The owning stories' scripts play the same keyboard/mouse inputs: us001 Space and Left Shift in lane L1; us054 LMB and Left Alt next to Frontliner_3; us078 LMB tap and hold
- **Visible on screen:** the player in the diorama arena with the combat HUD; the attack/dodge are state-checked (tags, displacement) — us083 screenshots `lane_start` (the player at the lane L1 start) and `blocking` (LT held, after the sprint); `us054` `near_enemy` shows the player next to the enemy
- **Proof:** Tier 3 boot shows the arena and HUD; Tier 4 `Scripts/playtests/us083_combat_input.py` (the story's own run, exit 0) accepts AC-083.3..AC-083.8 and AC-083.10: gamepad block and parry (AC-083.10), the gamepad attack/dodge/jump keys, and the keyboard/mouse checks `light_attack_lmb_tap`, `heavy_attack_lmb_hold`, `dodge_left_alt`, `jump_space_bar`, `sprint_left_shift`; the owning stories' checks `jump_space`, `sprint_shift` (us001), `melee_lmb`, `dodge_lalt` (us054), `light_attack_lmb`, `heavy_attack_hold` (us078) play the same inputs; mouse block/parry (AC-083.11) PENDING TIER 4 (bridge cannot drive: B2 mouse hold)
- **Tier 4 script:** `Scripts/playtests/us083_combat_input.py` (`py Scripts/playtest_story.py US-083`) for the gamepad AND the keyboard/mouse inputs (LMB tap/hold, LeftAlt, Space, LeftShift sprint in lane L1), plus the owning stories' scripts — `Scripts/playtests/us001_movement.py` (`py Scripts/playtest_story.py US-001`), `Scripts/playtests/us054_arena.py` (`py Scripts/playtest_story.py US-054`), `Scripts/playtests/us078_weapons.py` (`py Scripts/playtest_story.py US-078`); run them together with `py Scripts/playtest_story.py --all` on the same package (that also covers `gamepad_back_closes` of us069, Technical Notes)
- **Steps:** `Scripts/playtests/us083_combat_input.py` (from `py Scripts/playtest_story.py US-083 --plan`):

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `walk_lane_wp1` | walk_to(-650, 0) -> within 60 cm (waypoint to lane L1: west along y=0, between the enemies' spawn points) | - |
| 2 | `walk_lane_wp2` | walk_to(-700, -650) -> within 60 cm (waypoint to lane L1: south, west of Box_02, to us001's waypoint) | - |
| 3 | `walk_lane_start` | walk_to(-700, -800) -> within 60 cm (lane L1 start) | - |
| 4 | `walk_lane_run_up` | walk_to(-300, -800) -> within 60 cm (along lane L1, +X: the enemies that followed the player end up behind it) | - |
| 5 | `sprint_left_shift` | at the run-up point: tap LeftShift, hold W 1s -> dx > 700 cm; tap LeftShift again, hold S 1s reading player.speed every 0.05s -> back to a walk (peak 500..750 cm/s) | AC-083.8, AC-083.3 |
| 6 | `block_lt_hold` | axis Gamepad_LeftTriggerAxis=1 (held) -> 'Mordecai.State.Blocking' in player.tags within 1s | AC-083.10, AC-083.3 |
| 7 | `parry_rt_while_blocking` | LT held, axis Gamepad_RightTriggerAxis=1 -> within 0.5s a parry tag (Mordecai.State.Parrying, Mordecai.State.ParryWhiff, Mordecai.State.Riposte) with no 'Mordecai.State.Blocking' and no 'Mordecai.State.Attacking'; then RT=0 | AC-083.10, AC-083.3 |
| 8 | `guard_returns_after_parry` | LT still held -> 'Mordecai.State.Blocking' again without a parry tag within 2.5s, and no 'Mordecai.State.Attacking' at any read meanwhile | AC-083.10 |
| 9 | `block_release_lt` | axis Gamepad_LeftTriggerAxis=0 -> 'Mordecai.State.Blocking' gone within 1s | AC-083.10 |
| 10 | `partial_rt_no_parry` | LT held (Blocking), axis Gamepad_RightTriggerAxis=0.3 for 0.6s -> no parry tag and 'Mordecai.State.Blocking' at every read; then both 0 | AC-083.3 |
| 11 | `block_over_held_rt_no_parry` | axis Gamepad_RightTriggerAxis=1, 0.15s later Gamepad_LeftTriggerAxis=1 -> 'Mordecai.State.Blocking' within 1s and no parry tag for 0.6s more; then both 0 | AC-083.3 |
| 12 | `light_attack_rt_tap` | idle -> axis Gamepad_RightTriggerAxis=1 hold_s 0.1 -> 'Mordecai.State.Attacking' without 'Mordecai.Attack.Heavy' within 1s | AC-083.3 |
| 13 | `heavy_attack_rt_hold` | idle -> axis Gamepad_RightTriggerAxis=1 hold_s 0.8 -> 'Mordecai.Attack.Heavy' in player.tags within 0.5s of the release | AC-083.3 |
| 14 | `dodge_gamepad_b` | tap Gamepad_FaceButton_Right (B) -> a Dodging tag in player.tags within 0.5s | AC-083.3 |
| 15 | `jump_gamepad_a` | wait until the dodge is over (no 'Mordecai.State.Dodging') and player.movement_mode=="Walking", tap Gamepad_FaceButton_Bottom (A) -> wait_until player.movement_mode=="Falling" within 1s | AC-083.3 |
| 16 | `light_attack_lmb_tap` | idle -> click Left at (640, 360) (the bridge's default click) -> 'Mordecai.State.Attacking' without 'Mordecai.Attack.Heavy' within 1s | AC-083.4, AC-083.3 |
| 17 | `heavy_attack_lmb_hold` | idle -> click Left at (640, 360) held 0.8s -> 'Mordecai.Attack.Heavy' in player.tags within 0.5s of the release | AC-083.5, AC-083.3 |
| 18 | `dodge_left_alt` | idle (and no Dodging / DodgeCooldown tag left) -> tap LeftAlt -> a Dodging tag in player.tags within 0.5s | AC-083.6, AC-083.3 |
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
- [ ] All ACs checked off (AC-083.11 stays unchecked: PENDING TIER 4, not claimed)
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: no asset changes expected; `py Scripts/check_content_committed.py` exits 0
- [ ] Code AND the script edits (`known_issue` removed from the six fixed Steps, `melee_lmb` made honest) committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`
- [ ] On that same fresh package: `py Scripts/playtest_story.py US-001`, `py Scripts/playtest_story.py US-054` and `py Scripts/playtest_story.py US-078` run (or `--all`, which also runs us069's `gamepad_back_closes`); checks `sprint_shift`, `jump_space`, `melee_lmb`, `dodge_lalt`, `light_attack_lmb`, `heavy_attack_hold` go from FAIL (sweep `Saved/Playtests/20261022-184702-sweep`) to PASS; no check that PASSed in the sweep now FAILs; every remaining FAIL belongs to another Epic R story (cite which). `--lint` exits 0 for US-001, US-054 and US-078 — never `--allow-stale`
- [ ] On that same fresh package: `py Scripts/playtest_story.py US-083` (Tier 3 boot + `Scripts/playtests/us083_combat_input.py`) exits 0; `--lint` exits 0 for US-083
- [ ] Known issues entries "Sprint, jump, attacks, block and dodge are not bound at runtime" and N10 marked FIXED in `Specs/agent_playtest_plan_v1.md` with the run dirs
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs (Tier 3 `Saved/Playtests/<run>-boot/report.md`, Tier 4 `Saved/Playtests/<run>-us083|us001|us054|us078/report.md`) in Completion Notes AND in the body of the `[US-083]` commit that closes the story
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass. A failing playtest is reported honestly and the story stays in progress

## Completion Notes
<Filled in by the coding agent.>
