# US-005: Dodge System

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement the dodge mechanic as a GAS Gameplay Ability. Dodge grants invincibility frames (i-frames) during a configurable window, consumes stamina, moves the character in the input direction, and has a "perfect dodge" timing window that rewards skilled play. Dodge is on a dedicated button (B / Left Alt) and is always available regardless of stamina (per stamina tier rules — effectiveness may be reduced at low stamina, but never blocked).

## References
- Design doc: `combat_system_v1.md` — Defense section (dodge i-frames, perfect dodge)
- Design doc: `control_bindings_v1.1.md` — Dodge: B / Left Alt, always possible
- Design doc: `character_attributes_v1.md` — END affects stamina regen; DEX affects attack speed (dodge speed TODO)
- Design doc: `game_design_v2.md` — "Skill-first" pillar; dodge is primary defensive tool
- Agent rules: `agent_rules_v2.md` — GAS for all abilities; server-authoritative; UE 5.7

---

## Acceptance Criteria
- [ ] AC-005.1: `UMordecaiGA_Dodge` (UGameplayAbility subclass) exists and can be granted to an ASC
- [ ] AC-005.2: Activating dodge applies `Mordecai.State.Dodging` gameplay tag for the full dodge duration
- [ ] AC-005.3: During dodge, incoming damage is negated — implemented by checking `Mordecai.State.Dodging` tag in the damage pipeline (gameplay tag requirement on damage GE, NOT collision disable)
- [ ] AC-005.4: Dodge direction is determined by movement input at activation time; if no input, dodge direction defaults to backward (opposite of character facing)
- [ ] AC-005.5: Dodge applies a movement impulse in the dodge direction (configurable `DodgeDistance` and `DodgeDuration`)
- [ ] AC-005.6: Dodge consumes stamina on activation — configurable `StaminaCost` (default placeholder: 15.0)
- [ ] AC-005.7: Dodge can be activated during an attack's Recovery phase if the active attack profile has `CancelableIntoDodge=true` (cancel the attack ability, then activate dodge)
- [ ] AC-005.8: Perfect Dodge window: the first N ms of dodge (configurable `PerfectDodgeWindowMs`, default `TODO(DECISION)`) applies `Mordecai.State.PerfectDodge` tag for that window duration
- [ ] AC-005.9: Perfect Dodge reward: if `Mordecai.State.PerfectDodge` is active when an enemy attack would have hit, a configurable stamina refund is applied (`PerfectDodgeStaminaRefund`, default `TODO(DECISION)` — stubbed as configurable float)
- [ ] AC-005.10: Dodge has a recovery period after completion — cannot dodge again until recovery expires (configurable `DodgeCooldownMs`, default placeholder: 300ms)
- [ ] AC-005.11: Dodge is blocked while `Mordecai.State.PostureBroken` is active (staggered characters cannot dodge)
- [ ] AC-005.12: Dodge is NOT blocked by `Mordecai.State.Rooted` — per status_effects doc, Rooted prevents movement but dodge is a stamina-break action that can break root (interface only for now; root-break logic in Epic 4)
- [ ] AC-005.13: Character movement is locked to the dodge impulse direction during dodge (no steering mid-dodge)

## Technical Notes
- Place in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- Use `UGameplayAbility` subclass with instancing policy `InstancedPerActor`
- I-frames: do NOT disable collision. Instead, add `Mordecai.State.Dodging` as a tag requirement on the damage-receiving GE (or check in the damage execution calculation). This keeps physics/overlap intact.
- Movement impulse: use `LaunchCharacter` or `AddImpulse` on the character movement component. Alternatively, use a root motion source if Lyra supports it. Keep it simple — timer-based displacement is fine for now.
- Perfect Dodge detection: apply `Mordecai.State.PerfectDodge` tag during the window. Detection of "would have hit" can be deferred — for now, just grant the tag. Actual reward trigger hooks into the damage pipeline later.
- Dodge cancel: listen for dodge input via `UAbilityTask_WaitInputPress` or gameplay event during attack Recovery phase. The melee attack ability (US-004) should check for and allow this.
- Cooldown: use GAS cooldown GE or a simple timer-based check via gameplay tag `Mordecai.State.DodgeCooldown`
- Do NOT implement animation integration — use timer-based phases. Animation hookup is a separate story.
- Stamina cost is consumed even if stamina is at 0 (stamina can go negative per tier system design). The dodge still fires.

## Tests Required
- [ ] `Mordecai.Dodge.AbilityActivatesSuccessfully` — grant ability, activate, verify Dodging tag applied (AC-005.1, AC-005.2)
- [ ] `Mordecai.Dodge.GrantsIFramesDuringDodge` — activate dodge, apply damage during dodge, verify no health loss (AC-005.3)
- [ ] `Mordecai.Dodge.DamageAppliesAfterDodgeEnds` — apply damage after dodge expires, verify health reduced (AC-005.3)
- [ ] `Mordecai.Dodge.DirectionMatchesMovementInput` — set movement input to right, dodge, verify impulse direction is right (AC-005.4)
- [ ] `Mordecai.Dodge.DefaultsToBackwardWithNoInput` — no movement input, dodge, verify impulse is backward relative to facing (AC-005.4)
- [ ] `Mordecai.Dodge.AppliesMovementImpulse` — dodge, verify character position changes in dodge direction (AC-005.5)
- [ ] `Mordecai.Dodge.ConsumesStaminaOnActivation` — activate dodge, verify Stamina reduced by StaminaCost (AC-005.6)
- [ ] `Mordecai.Dodge.CancelsAttackRecoveryPhase` — activate melee attack with CancelableIntoDodge=true, trigger dodge during Recovery, verify attack ends and dodge activates (AC-005.7)
- [ ] `Mordecai.Dodge.PerfectDodgeWindowGrantsTag` — dodge, verify PerfectDodge tag is active within the window and removed after (AC-005.8)
- [ ] `Mordecai.Dodge.CooldownPreventsImmediateRedodge` — dodge, immediately try to dodge again, verify second dodge blocked until cooldown expires (AC-005.10)
- [ ] `Mordecai.Dodge.BlockedDuringPostureBroken` — apply PostureBroken tag, attempt dodge, verify blocked (AC-005.11)
- [ ] `Mordecai.Dodge.MovementLockedDuringDodge` — during dodge, attempt to change movement direction, verify character continues on original dodge vector (AC-005.13)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-005]` prefix
