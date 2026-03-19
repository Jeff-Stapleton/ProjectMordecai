# US-004: Melee Attack GAS Ability & Combo System

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement the core melee attack as a GAS Gameplay Ability. Reads from an `AttackProfile` data asset (US-002), uses hit detection (US-003) during the active window, applies damage and posture damage via Gameplay Effects, and supports light attack combo chains. This is the ability that makes swinging a weapon actually work.

## References
- Design doc: `attack_taxonomy_v1.md` — Sections 2-4 (data model, melee details, weapon profiles)
- Design doc: `player_attacks_agent_brief_v1.md` — Sections 2-4 (same, agent-focused)
- Design doc: `combat_system_v1.md` — Damage Formula, Posture System, Critical Hits
- Design doc: `damage_types_v1.md` — Section 5 (Damage Delivery Metadata)
- Design doc: `stat_formulas_v1.md` — Derived stats referenced by damage formula
- Agent rules: `agent_rules_v2.md` — GAS for all abilities; server-authoritative

---

## Acceptance Criteria
- [ ] AC-004.1: `UMordecaiGA_MeleeAttack` (UGameplayAbility subclass) exists and can be granted to an ASC
- [ ] AC-004.2: Ability accepts a `UMordecaiAttackProfileDataAsset` reference (set per weapon/combo step)
- [ ] AC-004.3: Ability executes three phases in sequence: **Windup** → **Active** → **Recovery**, with durations read from `WindupTimeMs`, `ActiveTimeMs`, `RecoveryTimeMs`
- [ ] AC-004.4: During **Active** phase, hit detection fires once using `UMordecaiHitDetectionSubsystem::PerformMeleeHitDetection` with the profile's shape parameters
- [ ] AC-004.5: Each hit target receives a `UGameplayEffect` that applies damage based on `DamageProfile.BasePower × PostureDamageScalar` (posture) and `DamageProfile.BasePower` (health), using `SetByCaller` magnitude
- [ ] AC-004.6: Damage is tagged with the correct `Mordecai.Damage.*` gameplay tag from the profile's DamageType
- [ ] AC-004.7: Stamina is consumed on ability activation (not on hit) using `StaminaCost` from the profile
- [ ] AC-004.8: If `RootedDuring != None`, character movement is disabled during the specified phase(s)
- [ ] AC-004.9: **Combo chain**: if the player presses Light Attack again during the Recovery phase of a Light attack, the next combo step activates (ComboIndex advances 1→2→3→...→reset)
- [ ] AC-004.10: Combo resets to step 1 if: (a) combo window expires (configurable timeout, default 800ms after Recovery starts), (b) player uses a different input, or (c) player takes a hit
- [ ] AC-004.11: `OnHitPayload` Gameplay Effects are applied to each hit target; `OnUsePayload` effects are applied to the attacker on activation
- [ ] AC-004.12: Ability can be canceled into Dodge if `CancelableIntoDodge=true` (during Recovery phase only)
- [ ] AC-004.13: Ability can be canceled into Block if `CancelableIntoBlock=true` (during Recovery phase only)
- [ ] AC-004.14: Ability blocks concurrent activation (cannot double-swing)

## Technical Notes
- Place in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- Use **Ability Tasks** (`UAbilityTask`) for phase timing — `UAbilityTask_WaitDelay` or a custom `UMordecaiAT_AttackPhase` that manages the three-phase sequence
- Damage application: create a `UGameplayEffect` class `GE_MordecaiMeleeDamage` with `SetByCaller` magnitude for Health and Posture damage. The ability sets the magnitude at runtime from the AttackProfile.
- Stamina cost: use a `UGameplayEffect` cost with `SetByCaller` or a direct attribute modification in `ActivateAbility`
- Combo state: track `CurrentComboIndex` as an instanced ability variable. Use a gameplay tag (`Mordecai.State.ComboWindow`) during the combo-accepting window.
- Movement lock: apply/remove `Mordecai.State.Rooted` gameplay tag during rooted phases; the character movement component should check this tag
- Cancel-into-dodge: during Recovery, if the ability is cancelable, listen for Dodge input via `UAbilityTask_WaitGameplayEvent` or input tag, then call `CancelAbility()`
- Damage formula from combat_system_v1.md: `Damage = BaseDamage × SkillModifier × AttributeScaling × CriticalModifier × StatusModifier`. For this story, implement only `BasePower` (from profile). SkillModifier, AttributeScaling, CriticalModifier, and StatusModifier are `TODO(DECISION)` — stubbed as 1.0 multipliers until Epic 3 (Attributes) and Epic 4 (Status Effects) are implemented.
- Do NOT implement animation integration (AnimMontage/Notifies) — use timer-based phases for now. Animation hookup is a separate story.

## Tests Required
- [ ] `Mordecai.MeleeAttack.AbilityActivatesSuccessfully` — grant ability, activate, verify it enters Windup (AC-004.1, AC-004.3)
- [ ] `Mordecai.MeleeAttack.PhasesExecuteInOrder` — activate, verify Windup→Active→Recovery sequence with correct durations (AC-004.3)
- [ ] `Mordecai.MeleeAttack.HitDetectionFiresDuringActive` — place target in range, verify hit detected during Active phase (AC-004.4)
- [ ] `Mordecai.MeleeAttack.DamageAppliedToHitTarget` — hit a target, verify Health reduced by BasePower (AC-004.5)
- [ ] `Mordecai.MeleeAttack.PostureDamageAppliedToHitTarget` — hit a target, verify Posture reduced by BasePower × PostureDamageScalar (AC-004.5)
- [ ] `Mordecai.MeleeAttack.DamageTaggedCorrectly` — verify damage event carries correct Mordecai.Damage.* tag (AC-004.6)
- [ ] `Mordecai.MeleeAttack.StaminaConsumedOnActivation` — activate attack, verify Stamina reduced by StaminaCost (AC-004.7)
- [ ] `Mordecai.MeleeAttack.RootedDuringActiveLocksMovement` — set RootedDuring=Active, verify movement disabled during Active only (AC-004.8)
- [ ] `Mordecai.MeleeAttack.ComboAdvancesOnLightInput` — Light→Light during window, verify ComboIndex advances (AC-004.9)
- [ ] `Mordecai.MeleeAttack.ComboResetsAfterTimeout` — Light attack, wait past combo window, verify ComboIndex resets to 1 (AC-004.10)
- [ ] `Mordecai.MeleeAttack.ComboResetsOnDifferentInput` — Light→Heavy, verify combo resets (AC-004.10)
- [ ] `Mordecai.MeleeAttack.OnHitPayloadApplied` — configure OnHitPayload with a test GE, verify it's applied on hit (AC-004.11)
- [ ] `Mordecai.MeleeAttack.CancelIntoDodgeDuringRecovery` — set CancelableIntoDodge=true, trigger dodge during Recovery, verify ability ends (AC-004.12)
- [ ] `Mordecai.MeleeAttack.CannotDoubleActivate` — try to activate while already active, verify blocked (AC-004.14)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] `Scripts/Verify.bat` passes (tests + build)
- [ ] Code committed and pushed with `[US-004]` prefix
