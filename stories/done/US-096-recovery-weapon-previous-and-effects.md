# US-096: Recovery — Previous-Weapon Key (Shift+Tab vs BackSlash) + Weapon Effects Reach the ASC

**Epic:** Epic R: Playability Recovery
**Priority:** P2
**Depends on:** US-083 (sprint on Left Shift is bound there); N9 decided by Jeff on 2026-11-24
**Paired story:** none

## Overview
AC-078.7 promises Shift+Tab for "previous weapon", but the game maps only BackSlash and Tab still fires "next" when Shift is held (N9): Shift+Tab on the Mace goes to the Longsword instead of the Spear. Shift is also the sprint toggle (control_bindings), so a real Shift+Tab chord also toggles sprint once US-083 binds it. Separately, the equipped weapon's `GrantedTags` and `StatModifiers` never reach the player's ASC, because `UMordecaiEquipmentComponent` looks for the ASC on the character while Lyra keeps it on the PlayerState. Features: weapons.

## Execution Mode
**HEADLESS**
Programmatic IMC mapping (physical-Shift trigger/blocker) and the equipment component's ASC lookup, all C++.
Issue #36 used a one-time EDITOR authoring step (Scripts/us078_01_create_weapon_assets.py) to re-save the five DA_Weapon_* type tags at Jeff's request; the rest stays HEADLESS.

## References
- `Specs/agent_playtest_plan_v1.md` — N9 (`MordecaiHeroComponent.cpp:227-232`), "Weapon effects never reach the ASC" (`MordecaiEquipmentComponent.cpp:55-56`), B2 (no equip slot on `player.weapon`, AC-078.15)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184940-us078`: `cycle_prev_shift_tab` FAIL known ("tapped LeftShift+Tab on 4 'Mace'; expected index 3 'Spear' ... the weapon is index 0/5 'Longsword'"); `cycle_prev_backslash` PASS; AC-078.15 PENDING (bridge cannot drive: no equip slot)
- Done stories: US-078 AC-078.7 ("Bound to Shift+Tab (MKB) only"), AC-078.8, AC-078.15; US-077 AC-077.4; US-024 (weapon GrantedTags / StatModifiers via GAS)
- `Specs/control_bindings_v1.1.md` — Tab = Weapon Set Swap; Left Shift = Sprint toggle; no "previous weapon" row
- Existing code: `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:105-107,220-232,343-351`; `Source/LyraGame/Mordecai/Weapons/MordecaiEquipmentComponent.cpp:55-56`

## Decisions
- Shift+Tab cycles to the previous weapon.
- Tab alone continues to cycle to the next weapon.
- Q remains ability/spell slot 1.
- Left Shift remains the sprint toggle. A plain tap toggles immediately with no
  added delay; completing Shift+Tab restores the sprint state from before the
  Shift press, so the chord has no net effect on sprint.
- Either Left Shift or Right Shift may form the chord. Right Shift is not a
  sprint key and never changes sprint state.
- BackSlash remains an additional previous-weapon binding.
- Decided by Jeff for issue #14 and PR #167 review on 2026-11-24; also recorded
  in `Specs/design_decisions_log.md`.

## Acceptance Criteria
- [x] AC-096.1: Shift+Tab cycles to the previous weapon, Tab alone still cycles next, and the chord does NOT toggle sprint; Q remains ability slot 1 and BackSlash remains an extra previous binding — verified by `Mordecai.Recovery.Weapons.PreviousWeaponInput`
- [x] AC-096.2: In the packaged build, on the Mace the player presses Shift+Tab and the Spear is equipped, and sprint is not toggled — verified by check `cycle_prev_shift_tab` of `Scripts/playtests/us078_weapons.py` (Tier 4)
- [x] AC-096.3: Equipping a weapon applies its `GrantedTags` and `StatModifiers` to the ASC found via the owning PlayerState (Lyra pattern), and unequipping removes them — verified by `Mordecai.Recovery.Weapons.EffectsAppliedToPlayerStateASC`. **Closed at Jeff's call (2026-11-09):** "Only AC-096.3 in this PR; leave previous-weapon N9 open."
- [ ] AC-096.4: AC-078.15 (two-hand clears both slots) stays PENDING TIER 4 (bridge cannot drive: B2 — no equip slot on `player.weapon`); Tier 1 only, not claimed

Issue #36 was scoped to AC-096.3 only. Issue #14 closes N9 / AC-096.1-.2. Multiplayer
equipment replication remains out of scope and is tracked by issue #95.

## Technical Notes
- N9 is decided: Shift+Tab is previous, plain Tab is next, and BackSlash remains an extra previous binding.
- Runtime implementation: a dedicated Tab action uses a dual-Shift
  `UMordecaiInputTriggerShiftPressed`, while plain Tab uses a dual-Shift blocker
  plus `UInputTriggerPressed`. The keyboard sprint action records only the
  current physical Left Shift press; its same-frame token makes callback order
  irrelevant and expires on release or context rebuild. The dedicated chord
  action is retained because an Enhanced Input action callback cannot identify
  whether BackSlash or Tab fired the existing previous action; sharing it would
  risk applying sprint correction to BackSlash.
- ASC lookup: use `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerPawn)` (Lyra's character implements the interface and forwards to the PlayerState) or the PlayerState directly; re-apply on possession/respawn.
- Script bookkeeping: remove the N9 `known_issue` from `cycle_prev_shift_tab` and assert the sprint tag is unchanged.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Weapons.PreviousWeaponInput` — AC-096.1
- `Mordecai.Recovery.Weapons.ShiftTabSameFrameOrdering` — both callback orders
- `Mordecai.Recovery.Weapons.TabRequiresFreshPressThroughShiftTransitions` —
  real key-state trigger checks for fresh Tab, dual-Shift blocker, and release order
- `Mordecai.Recovery.Weapons.ShiftChordIgnoresStaleSprintToggleState` — L3 and
  context-rebuild state cannot leak into a keyboard chord
- `Mordecai.Stamina.Runtime.ShiftTabPreservesSprintDuringInterrupts` — attack,
  dodge, and block refuse sprint without changing the chord's pre-press intent
- `Mordecai.Recovery.Weapons.EffectsAppliedToPlayerStateASC` — AC-096.3
- `Mordecai.Recovery.Weapons.PlayerStateFallbackRemainsSingleAcrossASCInitialization` — AC-096.3
- `Mordecai.Recovery.Weapons.EffectsAppliedAfterLateASCInitialization` — AC-096.3
- `Mordecai.Recovery.Weapons.PawnDestroyClearsRecordedPlayerStateEffects` — AC-096.3
- `Mordecai.Recovery.Weapons.SwapDoesNotStackPreviousEffects` — AC-096.3
- `Mordecai.Recovery.Weapons.SerializedAssetsHaveGrantedTags` — guards the five cooked weapon tags
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_weapons.py` (new or extended): `[LOGIC]` equip each weapon → its granted tags on the PlayerState ASC
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us078_weapons.py` check `cycle_prev_shift_tab`
- `Scripts/playtests/us078_weapons.py` check `weapon_effects_on_asc` — AC-096.3

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience; us078's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-078` on the same package
- **Player steps:** read the Longsword tag, tap Tab through all five weapons and verify each
  current type tag is present while the previous type tag is absent; then tap Tab four times
  (Mace) and press Shift+Tab
- **Visible on screen:** the weapon HUD (after US-091) reads "Spear"; the player is not sprinting
- **Proof:** Tier 4 check `cycle_prev_shift_tab` of `Scripts/playtests/us078_weapons.py`
- **Tier 4 script:** `Scripts/playtests/us078_weapons.py` — run: `py Scripts/playtest_story.py US-078`
- **Visual review:** Per Jeff's standing rule, builders never open PNG/JPG/video
  files. The coordinator performs the visual review; builders run scripted
  non-black/non-uniform/static-content checks and preserve the captures.
- **Steps:**

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `starting_weapon` | state player -> player.weapon.index==0 && player.weapon.count==5 && display_name 'Longsword' (case-insensitive) | AC-078.9, AC-078.10, US-077:AC-077.1, US-077:AC-077.5, US-077:AC-077.6 |
| 2 | `weapon_hud_text` | state player, widgets -> visible MordecaiEquippedWeaponWidget text equals player.weapon.display_name | AC-078.11, AC-078.12, US-077:AC-077.7 |
| 3 | `weapon_effects_on_asc` | state player on Longsword, then tap Tab through all five weapons -> each current Mordecai.Weapon.Type.* tag is present and the previous type tag is absent in player.tags | US-096:AC-096.3 |
| 4 | `cycle_next_1` | tap Tab -> wait_until player.weapon.index==1 (from 0), display_name 'Dagger' | AC-078.6, AC-078.8, AC-078.14, US-077:AC-077.3 |
| 5 | `cycle_next_2` | tap Tab -> wait_until player.weapon.index==2 (from 1), display_name 'Greatsword' | AC-078.6, AC-078.14, US-077:AC-077.3 |
| 6 | `cycle_next_3` | tap Tab -> wait_until player.weapon.index==3 (from 2), display_name 'Spear' | AC-078.6, AC-078.14, US-077:AC-077.3 |
| 7 | `cycle_next_4` | tap Tab -> wait_until player.weapon.index==4 (from 3), display_name 'Mace' | AC-078.6, AC-078.14, US-077:AC-077.3 |
| 8 | `cycle_wrap` | tap Tab on the last weapon -> wait_until player.weapon.index==0, display_name 'Longsword' | AC-078.6, US-077:AC-077.3 |
| 9 | `cycle_prev_backslash` | tap BackSlash on the first weapon -> wait_until player.weapon.index==4, display_name 'Mace' | AC-078.8, US-077:AC-077.4 |
| 10 | `cycle_prev_shift_tab` | tap LeftShift+Tab on the Mace -> wait_until player.weapon.index==3, display_name 'Spear', visible weapon HUD reads 'Spear', and Mordecai.State.Sprinting presence is unchanged | AC-078.7, US-096:AC-096.2 |
| 11 | `cycle_gamepad_y` | tap Gamepad_FaceButton_Top (Y) -> wait_until player.weapon.index==(index+1)%5 with the matching display_name | AC-078.6 |
| 12 | `cycle_prev_shift_tab_same_frame` | tap LeftShift+Tab in one input frame -> wait_until player.weapon.index==(index-1)%5, with Mordecai.State.Sprinting presence unchanged | US-096:AC-096.2 |
| 13 | `cycle_prev_release_shift_first` | hold LeftShift+Tab, release LeftShift while Tab stays held -> one previous cycle only; releasing the modifier does not re-fire plain Tab | US-096:AC-096.1, US-096:AC-096.2 |
| 14 | `cycle_shift_tap_while_tab_held` | hold Tab, tap LeftShift, then release Tab -> fresh Tab cycles next exactly once; releasing LeftShift does not re-fire plain Tab, and the plain Shift tap toggles sprint | US-096:AC-096.1 |
| 15 | `cycle_prev_right_shift_tab` | tap RightShift+Tab -> one previous cycle with Mordecai.State.Sprinting presence unchanged | US-096:AC-096.1, US-096:AC-096.2 |
| 16 | `cycle_prev_shift_tab_while_sprinting` | tap LeftShift to start sprinting, then LeftShift+Tab -> one previous cycle and Mordecai.State.Sprinting remains present | US-096:AC-096.2 |
| 17 | `weapon_hud_updates` | tap Tab -> the new weapon is equipped and visible MordecaiEquippedWeaponWidget text equals its display_name within 3 s (needs weapon_hud_text) | AC-078.12, AC-078.14, US-077:AC-077.8 |
| 18 | `light_attack_lmb` | alive, no Mordecai.State.Attacking -> click Left at (640, 360) -> Mordecai.State.Attacking without Mordecai.Attack.Heavy in player.tags within 1 s | AC-078.14 |
| 19 | `heavy_attack_hold` | alive, no Mordecai.State.Attacking -> click Left at (640, 360) held 0.8 s -> Mordecai.Attack.Heavy in player.tags within 0.5 s | AC-078.13 |

- **Playable alone:** YES

## Definition of Done
- [x] All decided/drivable ACs checked off (AC-096.4 stays unchecked: PENDING TIER 4, not claimed)
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: weapon type-tag repairs remain committed; `py Scripts/check_content_committed.py` exits 0
- [x] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [x] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — no `--allow-stale`
- [x] On that same fresh package `py Scripts/playtest_story.py US-078`: `cycle_prev_shift_tab` goes from FAIL to PASS; no baseline PASS regresses; `--lint` exits 0
- [x] Known issues N9 and "Weapon effects never reach the ASC" marked FIXED/decided in `Specs/agent_playtest_plan_v1.md`
- [x] Per Jeff's task instruction, PNG/JPG/video files were never opened with the file-read tool. The playtest and marketing scripts verified every cited frame as non-black/non-uniform and checked the promised widget/state.
- [x] Evidence cited in Completion Notes and the closing commit body
- [x] Pushed; story moved to `stories/done/` after verification and evidence bundle passed

## Completion Notes
### Issue #36 / AC-096.3 slice (2026-11-09)

Closed at Jeff's call for the scoped slice: "Only AC-096.3 in this PR; leave previous-weapon N9 open." AC-096.1, AC-096.2, and AC-096.4 remain unchecked; this story remains in backlog. Multiplayer equipment replication remains issue #95, and the synthetic +5 Strength modifier exists only in tests (no balance change).

- Commits: RED coverage `9be1aec`; PlayerState resolution, five weapon tag repairs, and packaged check `a226a30`; teardown and R1 regression hardening `9fc8aeb`.
- Tier 1: `Saved/TestLogs/issue36-r1-tier1-full.log` — 803/803 PASS. Focused `Mordecai.Recovery.Weapons.*` — 6/6 PASS, including real-order PlayerState fallback/idempotence and unpossess + pawn-destroy cleanup. Removing the same-ASC guard failed with duplicate Strength and tag count 4 (`issue36-r1-mutation-same-asc.log`); resolving a different ASC during removal left the original modified (`issue36-r1-mutation-recorded-asc.log`). Python tooling: 625/625 PASS (`issue36-r1-python-full.log`). PIE: 17/17 PASS (`issue36-r1-pie-full.log`). Shipping compile: PASS (`issue36-r1-shipping-build.log`).
- RED: `Saved/Playtests/20261109-175702-us078` on clean `9be1aec`, exit 1 — `weapon_effects_on_asc` failed all six observations (Longsword, Dagger, Greatsword, Spear, Mace, Longsword); N9 also failed as known. `bridge_01.png` shows CombatTestMap, player/HUD, and the Longsword loadout plate; `bridge_02.png` shows the same arena with the Mace plate after four Tabs.
- GREEN before R1: `Saved/Playtests/20261109-194903-us078` on clean `a226a30`, exit 1 only for N9 — `weapon_effects_on_asc` PASS with every current type tag present and every previous type tag absent. `bridge_01.png` shows the CombatTestMap start with Longsword; `bridge_02.png` shows the stationary player and Mace loadout plate.
- Tier 3 before R1: `Saved/Playtests/20261109-195020-boot` on clean `a226a30`, exit 0. `engine_loading.png` shows the Project Mordecai loading screen; `engine_01.png`–`engine_03.png` show CombatTestMap's grid, player, combat/weapon HUD, and TestDummy. The desktop `shot_01.png`–`shot_03.png` show the same game window.
- Sweep before R1: baseline `Saved/Playtests/20261109-144434-sweep` (`4b3dd65`) vs `Saved/Playtests/20261109-194942-sweep` (`a226a30`): 0 PASS→FAIL and 0 missing shared checks. The sweep remained exit 1 for unrelated filed/known failures plus pre-existing arena-spacing, Stone Skin indicator, and gamepad-jump findings; US-078 had 22 PASS and only N9 FAIL.
- Following the #17 evidence approach, the stories-only closing commit intentionally precedes the exact-final-HEAD package. The clean package, final US-078 run, final CombatTestMap boot, two-baseline sweep comparison, and all final frame descriptions are recorded in PR #96 rather than retroactively changing this commit.

### Previous weapon / issue #14 slice (2026-11-24)

- RED commit: `6e4b3d6449614aec35592cb7af6796cde4904579`. On main it failed only because the Shift+Tab mapping was absent; the tests also guard immediate plain-Shift sprint toggling and sprint restoration from both off and on.
- Implementation/package commit: `86076f3d9b2af11b2a730183b0bda3ed2965ec15`. `PackagedBuilds/Playtest/build_info.json` records `dirty=false`, `source_changed_during_build=false`, and that exact commit.
- Tier 1: compile PASS; `Saved/TestLogs/us096-r4-green-full.log` — 969/969 PASS, including `Mordecai.Recovery.Weapons.PreviousWeaponInput`.
- Tier 2: `py Scripts/run_pie_verify.py` — 23/23 smoke scripts PASS, including the US-078 weapon verification.
- Tier 3: `Saved/Playtests/20261124-122154-boot` — all checks PASS on CombatTestMap, correct `B_MordecaiArenaExperience`, 0 errors, Mordecai loading widget, and three game-owned frames. Scripted image metrics report `engine_loading.png` and `engine_01.png`–`engine_03.png` as 1280x720, non-black and non-uniform; desktop frames were also unobscured and non-uniform.
- Tier 4: `Saved/Playtests/20261124-122215-us078` — PASS. `cycle_prev_shift_tab` physically pressed Left Shift, then Tab, then released Shift; Mace changed to Spear immediately, the visible weapon widget reported `Spear`, and `Mordecai.State.Sprinting` remained false. Scripted frame checks report `bridge_01.png` as the Longsword HUD start, `bridge_02.png` as Mace after four Tabs, and `bridge_03.png` as Spear after Shift+Tab; all are 1280x720, non-black and non-uniform.
- Full same-package sweep: main baseline `Saved/Playtests/20261124-084025-sweep` at `a5af557665e7452934f3200341f9e2a59f6b05b8` had only `cycle_prev_shift_tab` failing. Final `Saved/Playtests/20261124-122250-sweep` passed all 21 Tier 4 scripts and all 6 Tier 3 boots, with 533 recorded checks, 0 failures, no missing baseline PASS, and no #135/#141/#163 rerun needed. `Saved/Playtests/us096-comparison-86076f3d.json` records `pass_to_fail: []` and the Shift+Tab check as fixed.
- Marketing: `marketing/us096/` contains five 1920x1080 game-owned stills and one 6.233-second, 30 fps 1920x1080 MP4 showing Longsword → Dagger → Greatsword → Dagger → Longsword. `content-check.json` passes source-frame SHA-256 matching, HUD structure, and 8/8 motion pairs; `label-proof.json` passes with no actor labels, debug text, or enemy indicators.
- Python tooling: the focused US-078 and US-096 marketing suites pass (21/21 and 5/5), and US-078 lint passes. The full tooling run reported 791/792 PASS with one unrelated pre-existing error: `test_us057_playtest.TestUs057Report.test_ac_ids_exist_in_the_story_files` hard-codes the former `stories/in-progress/US-090-recovery-spell-point-budget.md` path even though main moved that story to `stories/done/`. No test or gate was weakened.
- AC-096.4 remains unchecked and unclaimed because the bridge still exposes no equip-slot state.

### Round 2 review fixes (2026-11-24)

- RED commits: B1 same-frame callback ordering
  `a9e14efe9e339863b8f4e2bd999d1733953c8d57`; B2 held-Tab fresh-press
  semantics `efe6e337052159b811e3298e317604e17cec0912`; and M3
  press-scoped sprint intent, dual Shift, and interrupt coverage
  `5bdd79a2618aaf581f9f6cc937bc24e4211384ee`.
- Implementation commit `252ccecd4b41d2ed38259976d6aa2aaa9242ba7a`
  makes same-frame Shift+Tab callback-order independent, requires a fresh Tab
  press for plain next-weapon input, scopes the saved sprint state to the
  current Left Shift press, separates L3, supports either Shift for previous
  weapon, and preserves sprint intent across attack, dodge, and block
  interrupts. The dedicated chord action remains separate from
  `IA_Mordecai_WeaponCyclePrev` because one shared action callback cannot
  identify whether BackSlash or Tab triggered it.
- Tier 1: compile PASS and
  `Saved/TestLogs/tests-20261124-153506.log` — 973/973 PASS. The tests inject
  real key state through `UEnhancedPlayerInput`, cover both same-frame callback
  orders, Shift-release-first, Shift-tap-while-Tab-held, both Shift keys,
  press-scoped cleanup, gamepad L3 isolation, interrupt states, immediate plain
  Shift toggling, and Q remaining ability slot 1.
- Python tooling: 794/794 PASS, including the corrected
  `test_us057_playtest` done-story path. US-096 playtest lint PASS.
- Tier 2: clean-editor `py Scripts/run_pie_verify.py` — 23/23 smoke scripts
  PASS.
- Clean package commit:
  `127dca8dc0cf019232591654f542c438bc63838a`.
  `PackagedBuilds/Playtest/build_info.json` records `dirty=false`,
  `source_changed_during_build=false`, and that exact commit.
- Focused packaged evidence:
  `Saved/Playtests/20261124-172110-boot` Tier 3 PASS and
  `Saved/Playtests/20261124-172130-us078` Tier 4 PASS. The real-input checks
  pass staggered and simultaneous LeftShift+Tab, Shift-release-first,
  Shift-tap-while-Tab-held, RightShift+Tab, and Shift+Tab starting while
  sprinting; fresh Tab cycles once and no Shift transition re-fires held Tab.
- Full same-package sweep:
  `Saved/Playtests/20261124-172202-sweep` — PASS, all 21 Tier 4 scripts and all
  6 Tier 3 boots, 538 recorded checks (514 PASS, 2 WARN, 1 SKIP, 21 INFO),
  0 FAIL. Against main baseline
  `Saved/Playtests/20261124-084025-sweep`,
  `comparison.json` records `pass_to_fail: []`, `check_pass_to_fail: []`, and
  `pass_to_missing: []`. No #135/#141/#163 rerun was needed.
- Marketing: `marketing/us096/` contains five game-owned 1280x720 stills and a
  6.5-second, 30 fps, 195-frame MP4 showing Longsword → Dagger → Greatsword →
  Dagger → Longsword. Packaged `LevelCapture` is compiled behind
  `WITH_EDITOR`, so the recapture uses Unreal's packaged-game `-DUMPMOVIE`
  viewport path with `-BENCHMARK -FPS=30`; it is a contiguous fixed-time-step
  render, not a slideshow or desktop capture. `content-check.json` passes all
  nine source-frame matches, HUD structure, 8/8 sampled motion pairs, and
  64/64 adjacent 10 fps motion pairs. `label-proof.json` passes with no
  actor/debug labels or enemy indicators.
- Per Jeff's standing rule, the builder did not open any PNG, JPG, or video.
  Scripted non-black, provenance, HUD, motion, and temporal-continuity checks
  passed; the coordinator performs the visual review in place of the image
  review steps in `CLAUDE.md` 4c, 4d, and 6.
- Evidence bundle: Agent Store `us096-r2-127dca8d` (89 payload files).
  Independent SHA-256 verification of every `SHA256SUMS` entry passed with
  zero mismatches.
