# US-006: Block & Parry System

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement blocking and parrying as GAS abilities. Block is a held ability (LT / Right Mouse hold) that mitigates incoming damage at the cost of stamina drain per hit. Parry is a timed simultaneous press (LT+RT / RMB+LMB) that, if timed correctly, deals large posture damage to the attacker and opens a punish/riposte window. Failed parry has meaningful consequences. Block can be canceled into from attack Recovery (if the attack profile allows it).

## References
- Design doc: `combat_system_v1.md` — Block (stability, mitigation), Parry (risk/reward), Perfect Block
- Design doc: `control_bindings_v1.1.md` — Block: LT hold / Right Mouse hold; Parry: LT+RT / RMB+LMB timed
- Design doc: `character_attributes_v1.md` — DIS affects posture/stability; STR affects posture
- Design doc: `stat_formulas_v1.md` — `FinalDamage = IncomingDamage × (1 − Mitigation)`, `Posture = BasePosture + (STR × 2) + (DIS × 1)`
- Design doc: `game_design_v2.md` — "Defense depth: Dodge (i-frames), Block (stability-based), Parry (high-risk/high-reward)"
- Agent rules: `agent_rules_v2.md` — GAS for all abilities; server-authoritative; UE 5.7

---

## Acceptance Criteria

### Block
- [ ] AC-006.1: `UMordecaiGA_Block` (UGameplayAbility subclass) exists as a held ability — active while input is held, ends on release
- [ ] AC-006.2: While blocking, `Mordecai.State.Blocking` gameplay tag is applied to the character
- [ ] AC-006.3: While blocking, incoming damage is reduced by mitigation: `FinalDamage = IncomingDamage × (1 − Mitigation)` where Mitigation is a configurable float (stubbed, sourced from gear/attributes in later epics)
- [ ] AC-006.4: Each blocked hit drains stamina: `StaminaDrain = IncomingDamage × (1 − Stability)` where Stability is a configurable float (stubbed, increased by DIS/gear in later epics)
- [ ] AC-006.5: If stamina reaches 0 while blocking, block breaks: `Mordecai.State.Blocking` removed, `Mordecai.State.GuardBroken` applied for a configurable stagger duration (`GuardBrokenDurationMs`, default 800ms)
- [ ] AC-006.6: Block can be activated during attack Recovery phase if the active attack profile has `CancelableIntoBlock=true`
- [ ] AC-006.7: Block does NOT prevent movement (character can move at reduced speed while blocking, configurable `BlockMoveSpeedMultiplier`, default 0.5)
- [ ] AC-006.8: Block is NOT blocked by `Mordecai.State.Rooted` — per status effects doc, Rooted prevents movement but attack/block/cast are OK

### Perfect Block
- [ ] AC-006.9: Perfect Block window: if block activates within 250ms (`PerfectBlockWindowMs`, default 250) before an incoming hit lands, `Mordecai.State.PerfectBlock` tag is applied briefly. The perfect window is entirely inside the standard block window — all perfect blocks are blocks, but not all blocks are perfect blocks.
- [ ] AC-006.10: Perfect Block reward: no stamina drain for the blocked hit + applies configurable posture damage to the attacker (`PerfectBlockPostureDamage`, default 30.0)

### Parry
- [ ] AC-006.11: `UMordecaiGA_Parry` (UGameplayAbility subclass) exists, activated by simultaneous block+attack input (LT+RT / RMB+LMB)
- [ ] AC-006.12: Parry has a 500ms active window (`ParryWindowMs`, default 500) during which incoming attacks are parried. The perfect parry window (first 250ms) is inside this — all perfect parries are parries.
- [ ] AC-006.13: Successful parry: `Mordecai.State.Parried` applied to the attacker (stagger), `Mordecai.State.Riposte` applied to the player (punish window), large posture damage dealt to attacker (`ParryPostureDamage`, default 50.0)
- [ ] AC-006.14: Failed parry (window expires without parrying a hit): player is animation-locked for a brief duration (`ParryWhiffDurationMs`, default 600ms) — `Mordecai.State.ParryWhiff` tag applied. During this window the player cannot activate any abilities (blocked by tag). No extra damage multiplier — the punishment is being helpless and unable to dodge/block/act, making you an easy target (Smash Bros style). Think: the recovery animation plays and you're stuck in it.
- [ ] AC-006.15: Parry consumes stamina on activation (`ParryStaminaCost`, configurable, default placeholder: 20.0)
- [ ] AC-006.16: Cannot parry while `Mordecai.State.PostureBroken` or `Mordecai.State.GuardBroken` is active

### General
- [ ] AC-006.17: Block and Parry are mutually exclusive — cannot be active simultaneously. Parry input (block+attack) takes priority over sustained block.
- [ ] AC-006.18: Parry activation cancels an active block (briefly drops guard to attempt parry)

## Technical Notes
- Place in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- Block: use `UGameplayAbility` with `InstancedPerActor`. Activation tied to input hold; end on input release via `UAbilityTask_WaitInputRelease` or `WaitCancel`.
- Damage mitigation: implement via a custom `UGameplayEffectExecutionCalculation` or by modifying the damage GE's magnitude based on `Mordecai.State.Blocking` tag presence. Prefer execution calculation for flexibility.
- Stamina drain per hit: apply a GE to self when damage is blocked, with magnitude = `IncomingDamage × (1 − Stability)`. This requires the damage pipeline to notify the blocker.
- Guard break: when stamina hits 0, apply `Mordecai.State.GuardBroken` tag via GE with duration. Remove `Mordecai.State.Blocking`. Force-end the block ability.
- Perfect Block: track block activation time. When a hit lands, compare to activation time. If within `PerfectBlockWindowMs`, grant PerfectBlock tag and apply rewards. This timing check happens in the damage receive path.
- Parry: separate ability activated by a gameplay event fired when both block and attack inputs are pressed simultaneously. The input detection component fires `Mordecai.Event.ParryInput`. The parry ability listens for incoming hits during its window via `UAbilityTask_WaitGameplayEvent`.
- Failed parry: if the parry window ends without a successful parry, apply the whiff penalty GE.
- Mitigation and Stability are stubbed as UPROPERTY floats on the block ability for now. They will be sourced from the attribute set + gear in Epic 3/6.
- Do NOT implement animation integration — timer-based for now.

## Tests Required

### Block Tests
- [ ] `Mordecai.Block.AbilityActivatesOnInputHold` — hold block input, verify Blocking tag applied (AC-006.1, AC-006.2)
- [ ] `Mordecai.Block.AbilityEndsOnInputRelease` — release block input, verify Blocking tag removed (AC-006.1)
- [ ] `Mordecai.Block.MitigatesDamage` — set Mitigation=0.5, take 100 damage while blocking, verify 50 health lost (AC-006.3)
- [ ] `Mordecai.Block.DrainsStaminaPerHit` — set Stability=0.3, take 100 damage while blocking, verify stamina drained by 70 (AC-006.4)
- [ ] `Mordecai.Block.GuardBreaksAtZeroStamina` — block with low stamina, take hit that depletes it, verify GuardBroken tag applied and Blocking removed (AC-006.5)
- [ ] `Mordecai.Block.CancelsAttackRecoveryPhase` — attack with CancelableIntoBlock=true, trigger block during Recovery, verify attack ends and block activates (AC-006.6)
- [ ] `Mordecai.Block.AllowsReducedMovement` — block, attempt movement, verify movement at reduced speed (AC-006.7)

### Perfect Block Tests
- [ ] `Mordecai.Block.PerfectBlockGrantsTag` — activate block, take hit within PerfectBlockWindow, verify PerfectBlock tag applied (AC-006.9)
- [ ] `Mordecai.Block.PerfectBlockNoStaminaDrain` — take hit during PerfectBlock window, verify zero stamina drain (AC-006.10)
- [ ] `Mordecai.Block.PerfectBlockDamagesAttackerPosture` — take hit during PerfectBlock, verify attacker posture reduced (AC-006.10)

### Parry Tests
- [ ] `Mordecai.Parry.ActivatesOnSimultaneousInput` — fire block+attack simultaneously, verify Parry ability activates (AC-006.11)
- [ ] `Mordecai.Parry.SuccessfulParryStaggersAttacker` — parry an incoming hit, verify Parried tag on attacker and Riposte tag on player (AC-006.13)
- [ ] `Mordecai.Parry.SuccessfulParryDamagesPosture` — parry a hit, verify attacker posture reduced by ParryPostureDamage (AC-006.13)
- [ ] `Mordecai.Parry.FailedParryAnimationLocks` — activate parry, let window expire without hit, verify ParryWhiff tag applied and all ability activation blocked during whiff duration (AC-006.14)
- [ ] `Mordecai.Parry.ConsumesStaminaOnActivation` — activate parry, verify stamina reduced (AC-006.15)
- [ ] `Mordecai.Parry.BlockedDuringPostureBroken` — apply PostureBroken, attempt parry, verify blocked (AC-006.16)
- [ ] `Mordecai.Parry.TakesPriorityOverBlock` — hold block, then press attack (parry input), verify parry activates and block is interrupted (AC-006.17, AC-006.18)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-006]` prefix
