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
- [x] AC1: For a profile allowing Dodge/Block cancellation, real input during Recovery
  cancels melee first, removes Attacking/ComboWindow, then activates the defense; the two
  states never overlap. A canceled recovery cannot advance or land a queued follow-up hit.
- [x] AC2: Windup/Active are committed at baseline: Dodge, Block, and Parry do not activate
  alongside `State.AttackCommitted`. Parry is not an attack-cancel target.
- [x] AC3: At Normal/Yellow tier, an unobstructed dodge settles 400 ±40 cm from its start;
  Green/Red/Exhausted retain 1.25/0.75/0.5 scaling. Direction remains locked.
- [x] AC4: `State.Dodging` protects for the configured 400 ms. A hit in the first 150 ms
  grants one 10-stamina refund and one perfect-action event; later evasion grants neither.
  “Perfect Dodge!” appears only after a successful perfect evasion.
- [x] AC5: Swapping from a long chain to a shorter weapon during any melee phase cannot
  replace the in-flight step or leave Attacking stuck. The current step uses its pinned
  profile; the chain ends after that step, and the next attack starts at step 1 on the new weapon.
- [x] AC6: PostureBroken/KnockedDown/Parried interruption still cancels melee/dodge safely,
  preserves MOVE_None while locked, and permits no new cancel/defense activation.
- [x] AC7: Tier 4 on MovementTestMap proves real Recovery dodge/block cancellation and
  tier-aware displacement; Tier 4 on CombatTestMap proves perfect-dodge reward and
  mid-combo weapon-swap recovery.
- [x] AC8: The evidence run includes label-free clips of:
  parry → enemy posture break → riposte; player posture break; and attack Recovery →
  successful dodge cancel. Bundle `us125-r3m-246ba4da` uses in-engine capture with a
  content check; the coordinator completed the required contact-sheet/media visual review.

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
| 1 | `setup_yellow` | walk to lane L1 and spend six real light attacks to reach Yellow stamina | - |
| 2 | `recovery_dodge_cancel` | tap RT, wait for ComboWindow, tap LeftAlt -> Attacking/ComboWindow clear before Dodging; settled displacement is 400 +/-40 cm at Yellow | AC1, AC3, AC7 |
| 3 | `ordinary_dodge_has_no_perfect_feedback` | on enemy-free MovementTestMap, observe the completed dodge -> no PerfectDodge tag and no 'Perfect Dodge!' HUD text | AC4, AC7 |
| 4 | `committed_dodge_rejected` | tap RT and immediately tap LeftAlt during AttackCommitted -> attack remains and no dodge | AC2 |
| 5 | `held_block_needs_fresh_press` | hold LT during AttackCommitted and through Recovery -> Block never activates; release LT | AC2 |
| 6 | `recovery_block_cancel` | on a new light attack wait for ComboWindow, freshly hold LT -> attack ends before Block; release LT | AC1, AC7 |

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
- [x] Decisions D125-1 through D125-6 plus held-block and HUD rules logged
- [x] RED tests committed and pushed failing for production-path reasons
- [x] Full build, all Mordecai tests, Python tests, and PIE suite pass
- [x] Content guard passes; no new asset is required
- [x] Fresh clean package records `dirty=false` and `source_changed_during_build=false`
- [x] Both Tier 3 paths and both Tier 4 scripts pass without `--allow-stale`
- [x] Full sweep has no persistent new PASS→FAIL; only #135, #141 `combo_hit2`, and #157
  `sprint_auto_clear_restart` may receive one same-package rerun
- [x] Label-free marketing clips and image/video verification are bundled; script checks and
  coordinator visual review PASS
- [x] Evidence bundle `us125-<package-short-sha>` contains the required logs, reports,
  `comparison.json`, `build_info.json`, marketing media, manifest, and verified `SHA256SUMS`
- [x] Draft PR opened with `Closes #160`

## Completion Notes
### Delivery
- Round 3 implementation/package commit: `246ba4da2df9ac691e07fcff8803e82f25cb267f`.
- Draft PR: https://github.com/Jeff-Stapleton/ProjectMordecai/pull/161 (`Closes #160`).
- Round 3 evidence bundle:
  `C:\Users\jeffd\Documents\Gamedev\ProjectMordecai\Saved\Playtests\us125-r3-246ba4da`.
  `SHA256SUMS` covers the bundle. `marketing/media-verification.json` and
  `marketing/label-proof.json` pass. Two FFmpeg contact sheets cover all 14 final US-125
  `engine_*`/`bridge_*` primary frames.
- AC8 replacement media bundle: `us125-r3m-246ba4da`. It uses in-engine viewport capture
  and validates sampled clip content against known game frames and the rejected lock-screen
  capture.
- Decisions D125-1 through D125-6, the fresh-held-Block rule, and the success-only HUD rule
  are logged in `Specs/design_decisions_log.md`. The rank-10 Dodging perk remains unchanged and
  is explicitly deferred for later rework.
- On 2026-11-21, the coordinator (Mordecai Bot) viewed the `frames/` contact sheets: boot,
  attack-cancel, and perfect-dodge-combo frames all show the expected state, including the
  “Perfect Dodge!” callout. The coordinator also viewed all replacement marketing clips.
  This satisfies the `CLAUDE.md` coordinator frame-review requirement.
- Follow-ups: [#162](https://github.com/Jeff-Stapleton/ProjectMordecai/issues/162) and
  [#163](https://github.com/Jeff-Stapleton/ProjectMordecai/issues/163).

### Test-first and verification evidence
- Round 3 RED commit `e1223b48`; the bundle records the attack-cancel, combo-reset,
  measured-distance, and partial-final-tick RED logs.
- Build: PASS.
- Tier 1: `Saved/TestLogs/tests-20261121-165805.log` — 959 passed, 0 failed.
- Python tooling: 741 passed, 0 failed.
- Tier 2: 23 passed, 0 failed; `verify_pie_attack_cancel.py` confirms capsule 40,
  half-height 90, step height 45, and the runtime ability/tag integration.
- Content guard: PASS; no uncommitted content and no asset-writing script/content changes.
- Package `PackagedBuilds/Playtest/build_info.json`: commit `246ba4da2d`, `dirty=false`,
  `source_changed_during_build=false`.

### Tier 3 and Tier 4
- Story summary: `Saved/Playtests/20261121-173429-story-us125` — PASS.
- Tier 3 MovementTestMap: `Saved/Playtests/20261121-173429-boot` — all mandatory checks PASS;
  Mordecai loading screen, correct experience, 3/3 engine frames, no crash/missing content.
- Tier 3 CombatTestMap: `Saved/Playtests/20261121-173452-boot` — all mandatory checks PASS;
  Mordecai loading screen, correct experience, 3/3 engine frames, no crash/missing content.
- Tier 4 attack cancel: `Saved/Playtests/20261121-173512-us125-attack-cancel` —
  PASS. Recovery Dodge settled at 400.1 cm; committed Dodge and
  held Block were rejected; fresh Block canceled Recovery without overlap.
- Tier 4 perfect dodge/combo: `Saved/Playtests/20261121-173534-us125-perfect-dodge-combo` —
  PASS. Health stayed unchanged, stamina had one ten-point perfect refund, PerfectDodge
  success was observed, the queued Dagger continuation cleared on Greatsword swap, and a fresh
  Greatsword attack completed.

The task prohibited this agent from opening PNG/JPG/video files. The bundle therefore includes
two FFmpeg-generated, filename-labelled contact sheets and `frames/index.md`, covering every
final primary frame. The coordinator reviewed those sheets and the replacement marketing media
on 2026-11-21 and approved the visible results.

### Regression comparison
- Baseline: `Saved/Playtests/20261120-171709-sweep/summary.json` at `6613e17e`.
- Feature: `Saved/Playtests/20261121-174305-sweep/summary.json` at `246ba4da`.
- Bundle `comparison.json`: `script_pass_to_fail=[]`, `check_pass_to_fail=[]`.
  US-078 `cycle_prev_shift_tab` remained baseline-known. US-123 reached and passed setup but
  retained its baseline-existing, nondeterministic `knockdown` fixture/timing failure.

### US-123 diagnosis
- Requested run 1: `Saved/Playtests/20261121-173610-us123` — setup and primary posture/riposte
  route PASS; knockdown FAIL because the second route did not observe PostureBroken.
- Requested run 2: `Saved/Playtests/20261121-173714-us123` — setup/primary route PASS;
  knockdown FAIL. L1 landed at 750 ms and L2 at 1641 ms, after the 1500 ms broken window.
- Requested run 3: `Saved/Playtests/20261121-173811-us123` — setup/primary route PASS;
  knockdown FAIL because post-parry stamina was 66.607, crossing the real 66/33 boundary
  into Green.
- The baseline run `Saved/Playtests/20261120-172654-us123` also failed `knockdown`.
  Round 3 fixed the feature-only setup regression caused by the longer dodge by using the
  sprint-based Yellow route. Remaining failures are nondeterministic fixture/timing races in
  `Scripts/playtests/us123_posture.py:529-610`, not a demonstrated product regression. Attached
  diagnostics later passed the same route, including `Saved/Playtests/20261121-181322-us123`.

### AC evidence
- AC1: Tier 1 Recovery cancel tests plus Tier 4 `recovery_dodge_cancel` and
  `recovery_block_cancel` PASS.
- AC2: Tier 1 committed-defense/Parry tests plus Tier 4 `committed_dodge_rejected` and
  `held_block_needs_fresh_press` PASS.
- AC3: Tier 1 proves 500/400/300/200 cm and Tier 4 measured Yellow at 399.9999 cm.
- AC4: Tier 1 proves one refund/event and no ordinary refund; Tier 4 proves unchanged health,
  net 5 stamina cost, and success feedback state.
- AC5: Tier 1 pins the step/resets the chain; Tier 4 proves swap recovery and a fresh new attack.
- AC6: full Tier 1 posture-break, knockdown, and Parry interruption regressions PASS.
- AC7: both fresh-package real-input scripts PASS on their required maps.
- AC8: bundle `us125-r3m-246ba4da` contains matching stills and three label-free in-engine
  viewport-capture MP4s for all required sequences, with sampled-content, motion, and post-load
  label checks. The coordinator viewed and approved the clips and contact sheets on 2026-11-21.
