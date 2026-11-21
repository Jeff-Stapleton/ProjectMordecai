# US-125: Attack Cancellation, Dodge Travel, and Combo-Swap Safety

**Epic:** Core Combat Runtime
**Priority:** P0
**Depends on:** US-121, US-122, US-123, US-124
**Paired story:** none

## Overview
Attack commitment and defensive input become mutually consistent: profile-gated Recovery
cancels end the attack before Dodge or Block starts, committed phases cannot overlap defenses,
dodge travels its configured tier-scaled distance, successful perfect dodges grant their reward,
and weapon swaps cannot strand a combo. Existing posture interruption remains authoritative.

## Execution Mode
**HEADLESS**
Production changes are C++ GAS/movement logic and Python playtests using existing cooked maps.

## References
- `attack_taxonomy_v1.md` — combo and default commitment
- `combat_system_v1.md` — dodge i-frames and perfect reward
- `control_bindings_v1.1.md` — combat inputs
- `skill_sheet_v1.1.md` — Dodging milestone conflict
- `design_decisions_log.md` — stamina tiers, facing, defense precedence
- Existing code: `MordecaiGA_MeleeAttack`, `MordecaiGA_Dodge`,
  `MordecaiHitDefense`, `MordecaiHeroComponent`, `MordecaiEquipmentComponent`

## Acceptance Criteria
- [ ] AC1: For a profile allowing Dodge/Block cancellation, real input during Recovery
  cancels melee first, removes Attacking/ComboWindow, then activates the defense; the two
  states never overlap. A canceled recovery cannot advance or land a queued follow-up hit.
- [ ] AC2: Windup/Active are committed at baseline: Dodge, Block, and Parry do not activate
  alongside `State.AttackCommitted`. Parry is not an attack-cancel target.
- [ ] AC3: At Normal/Yellow tier, an unobstructed dodge settles 400 ±40 cm from its start;
  Green/Red/Exhausted retain 1.25/0.75/0.5 scaling. Direction remains locked.
- [ ] AC4: `State.Dodging` protects for the configured 400 ms. A hit in the first 150 ms
  grants one 10-stamina refund and one perfect-action event; later evasion grants neither.
  “Perfect Dodge!” appears only after a successful perfect evasion.
- [ ] AC5: Swapping from a long chain to a shorter weapon during any melee phase cannot
  replace the in-flight step or leave Attacking stuck. The current step uses its pinned
  profile; the chain ends after that step, and the next attack starts at step 1 on the new weapon.
- [ ] AC6: PostureBroken/KnockedDown/Parried interruption still cancels melee/dodge safely,
  preserves MOVE_None while locked, and permits no new cancel/defense activation.
- [ ] AC7: Tier 4 on MovementTestMap proves real Recovery dodge/block cancellation and
  tier-aware displacement; Tier 4 on CombatTestMap proves perfect-dodge reward and
  mid-combo weapon-swap recovery.
- [ ] AC8: The evidence run includes label-free clips of:
  parry → enemy posture break → riposte; player posture break; and attack Recovery →
  successful dodge cancel.

## Technical Notes
- Input arbitration cancels the active melee spec before forwarding Dodge/Block activation.
- Everyone may cancel light-attack Recovery into Dodge or Block when the active profile's
  `CancelableIntoDodge` / `CancelableIntoBlock` flag allows it. Heavy attacks remain committed.
  Parry never cancels an attack.
- A held Block that was rejected during a committed phase must not activate when Recovery begins;
  the player must release and press Block again.
- Pin the current step profile and weapon-derived values. On `OnWeaponChanged`, clear queued
  continuation and finish after the pinned step.
- Use a CharacterMovement root-motion constant-force task/source with collision and zero finish
  velocity; do not change shared braking/friction or capsule radius 40, half-height 90, step 45.
- The perfect window is a tunable 150 ms. A real perfect evasion refunds 10 stamina once,
  sends `Mordecai.Event.PerfectAction`, and produces the HUD's “Perfect Dodge!” feedback.
  Opening the timing window alone must not produce HUD feedback. No slow-motion is added.
- Dodge distance is 400 cm at Yellow/Normal, scaled to 500/400/300/200 cm for
  Green/Yellow/Red/Exhausted.
- The Dodging rank 10 perk is intentionally unchanged in this story and will be reworked later.
- Spear's light chain is three hits.
- Do not restore movement mode or braking state over PostureBroken/KnockedDown.
- No PIE-only paths.

## Tests Required
### Tier 1 — Automation
- `Mordecai.AttackCancel.RecoveryDodgeCancelsBeforeActivation`
- `Mordecai.AttackCancel.RecoveryBlockCancelsBeforeActivation`
- `Mordecai.AttackCancel.CommittedDefenseRejected`
- `Mordecai.AttackCancel.ParryDoesNotCancelAttack`
- `Mordecai.Dodge.RuntimeTravelsTierScaledDistance`
- `Mordecai.Dodge.PerfectEvadeRefundsOnce`
- `Mordecai.Dodge.OrdinaryEvadeDoesNotRefund`
- `Mordecai.Combo.WeaponSwapPinsStepAndResetsChain`
- posture-break/knockdown cancellation regressions

### Tier 2 — PIE
- `Scripts/verify_pie_attack_cancel.py` verifies granted abilities, tags, movement integration,
  and labels direct state manipulation `[LOGIC]`.

### Tier 3 — Packaged
- Boot MovementTestMap and CombatTestMap.

### Tier 4 — Real input
- `Scripts/playtests/us125_attack_cancel.py`: MovementTestMap; real light attack into
  Recovery, then Dodge or a fresh Block press; assert ordered tags and Yellow-tier displacement.
- `Scripts/playtests/us125_perfect_dodge_combo.py`: CombatTestMap; evade a timed TestAttacker
  hit and verify health/refund; build a Dagger combo, Tab to a shorter Greatsword, verify no stuck
  Attacking and a fresh Greatsword attack.
- Offline fake-bridge tests cover timeout/failure paths.
- `Scripts/playtests/us083_combat_input.py` judges the corrected dodge distance and retires N27.

## Playtest
- **Launch paths:** MovementTestMap and CombatTestMap through `playtest_story.py US-125`.
- **Player steps:** real attack input; wait for visible combo window; press Dodge or release and
  freshly press Block; perform a Yellow-tier dodge; time Dodge into TestAttacker; build combo and
  press Tab.
- **Visible on screen:** attack transitions immediately into dodge/block; full-length dodge;
  unchanged health and net five-stamina perfect-dodge cost; weapon HUD changes and combat resumes.
- **Proof:** `Scripts/playtests/us125_attack_cancel.py` and
  `Scripts/playtests/us125_perfect_dodge_combo.py`, their state audits, and script-verified
  screenshots.

<!-- Scripts/playtests/us125_attack_cancel.py -->

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `setup_yellow` | walk to lane L1 and spend five real light attacks to reach about 60 stamina | - |
| 2 | `recovery_dodge_cancel` | tap RT, wait for ComboWindow, tap LeftAlt -> Attacking/ComboWindow clear before Dodging; settled displacement is 400 +/-40 cm at Yellow | AC1, AC3, AC7 |
| 3 | `committed_dodge_rejected` | tap RT and immediately tap LeftAlt during AttackCommitted -> attack remains and no dodge | AC2 |
| 4 | `held_block_needs_fresh_press` | hold LT during AttackCommitted and through Recovery -> Block never activates; release LT | AC2 |
| 5 | `recovery_block_cancel` | on a new light attack wait for ComboWindow, freshly hold LT -> attack ends before Block; release LT | AC1, AC7 |

<!-- Scripts/playtests/us125_perfect_dodge_combo.py -->

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `setup` | walk within 100 cm of CombatTestMap's fixed-cadence TestAttacker | - |
| 2 | `perfect_dodge` | sync to the TestAttacker attack edge and tap LeftAlt about 0.40 s later -> hit evaded inside the 150 ms perfect window, health unchanged, net dodge stamina cost 5, and PerfectDodge success tag observed | AC4, AC7 |
| 3 | `equip_dagger` | press Tab with real input until the weapon state reports Dagger | - |
| 4 | `swap_during_combo` | tap RT, queue the next light step during ComboWindow, then press Tab -> weapon changes, queued continuation is cleared, and Attacking cannot stick | AC5, AC7 |
| 5 | `fresh_new_weapon_attack` | tap RT after the swapped chain ended -> the new weapon starts and finishes a fresh attack | AC5 |

- **Marketing:** capture label-free parry → enemy posture break → riposte, player posture break,
  and light-attack Recovery → dodge-cancel clips; include 3–6 stills, 1–3 MP4s, and a manifest.
- **Playable alone:** YES

## Definition of Done
- [ ] Decisions D125-1 through D125-6 plus held-block and HUD rules logged
- [ ] RED tests committed and pushed failing for production-path reasons
- [ ] Full build, all Mordecai tests, Python tests, and PIE suite pass
- [ ] Content guard passes; no new asset is required
- [ ] Fresh clean package records `dirty=false` and `source_changed_during_build=false`
- [ ] Both Tier 3 paths and both Tier 4 scripts pass without `--allow-stale`
- [ ] Full sweep has no persistent new PASS→FAIL; only #135, #141 `combo_hit2`, and #157
  `sprint_auto_clear_restart` may receive one same-package rerun
- [ ] Label-free marketing clips and image/video verification are bundled
- [ ] Evidence bundle `us125-<package-short-sha>` contains the required logs, reports,
  `comparison.json`, `build_info.json`, marketing media, manifest, and verified `SHA256SUMS`
- [ ] Draft PR opened with `Closes #160`

## Completion Notes
Filled by the builder.
