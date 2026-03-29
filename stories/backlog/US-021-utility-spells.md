# US-021: Utility Spells (Blink, Sleep, Fire Ward)

## Overview
Implement three utility spells using the Spell Framework from US-019. Blink (short-range teleport with i-frames), Sleep (single-target crowd control), and Fire Ward (self-targeted absorb shield). Each spell implements Rank 1 base behavior only; rank milestone unlocks are deferred to future stories.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `ability_schema_v1.md` (delivery types: DashMovement, TraceHit, InstantApply; targeting types)
- Design doc: `skill_sheet_v1.1.md` (Blink: teleport forward; Sleep: put target to sleep; Fire Ward: absorb fire damage)
- Design doc: `new_spells_proposal.md` (Blink rank progression: Rank 1 = short teleport + i-frames; Fire Ward rank progression: Rank 1 = absorbs flat magic damage)
- Design doc: `combat_system_v1.md` (dodge i-frames pattern — Blink uses similar invulnerability)
- Agent rules: `agent_rules_v2.md` (GAS for abilities, fixed diorama camera — teleport targeting must work without camera rotation)
- Existing code: `UMordecaiGA_SpellBase`, `UMordecaiSpellDataAsset` (US-019), `UMordecaiGA_Dodge` (US-005, i-frame pattern), `UMordecaiHitDetectionSubsystem` (US-003, line trace)

---

## Acceptance Criteria
- [ ] AC-021.1: `UMordecaiGA_Blink` extends `UMordecaiGA_SpellBase`. Delivery: DashMovement. On cast, teleports the caster forward in their aim direction by Range cm (from SpellDataAsset)
- [ ] AC-021.2: Blink performs a line trace from origin to destination to prevent teleporting through walls or geometry. If the trace hits, the teleport destination is clamped to the hit point minus a small buffer (e.g., 50cm before the wall)
- [ ] AC-021.3: Blink grants brief invulnerability by applying `Mordecai.State.Invulnerable` tag for a configurable InvulnerabilityDuration (stored in SpellDataAsset as a custom field or derived from RecoveryTime, default 0.2s)
- [ ] AC-021.4: Blink has minimal cast time (WindupTime ≈ 0, CastTime ≈ 0) — it is near-instant. Recovery time applies normally
- [ ] AC-021.5: `UMordecaiGA_Sleep` extends `UMordecaiGA_SpellBase`. Delivery: TraceHit (single target, line trace in aim direction up to Range). On hit, applies a Sleep GE to the first enemy hit
- [ ] AC-021.6: Sleep GE applies `Mordecai.Status.Sleeping` tag for a configurable Duration. While sleeping: target cannot move (movement disabled), cannot activate abilities (ability activation blocked by tag), cannot attack
- [ ] AC-021.7: Any damage dealt to a sleeping target removes the Sleep GE immediately (wakes them up). Implement via attribute change callback on Health — if Health decreases while Sleeping tag is active, remove the Sleep GE
- [ ] AC-021.8: `UMordecaiGA_FireWard` extends `UMordecaiGA_SpellBase`. Delivery: InstantApply self-targeted. Applies a shield GE that creates an absorb shield with ShieldHP = BasePower from SpellDataAsset
- [ ] AC-021.9: Fire Ward GE lasts for a configurable Duration (from SpellDataAsset) or until the shield is depleted. Applies tag `Mordecai.Status.FireWard`. When fire damage is received, the shield absorbs it (reduces damage, decrements shield HP). When shield HP reaches 0, the GE is removed
- [ ] AC-021.10: Fire Ward only absorbs damage tagged with `Mordecai.Damage.Fire`. Other damage types pass through the shield unaffected
- [ ] AC-021.11: New gameplay tags declared: `Mordecai.Ability.Spell.Blink`, `Mordecai.Ability.Spell.Sleep`, `Mordecai.Ability.Spell.FireWard`, `Mordecai.Status.Sleeping`, `Mordecai.Status.FireWard`

## Technical Notes
- **Blink teleport:** Use `SetActorLocation` with sweep disabled for the teleport. The pre-teleport line trace (AC-021.2) prevents clipping through geometry. This is different from Dodge (which uses movement component) — Blink is a true teleport.
- **Blink i-frames:** Same pattern as Dodge (US-005). Apply `Mordecai.State.Invulnerable` tag briefly. The damage pipeline should already respect this tag from US-005.
- **Sleep mechanics:** The Sleep GE should use `InhibitAbilityActivation` or block via tag requirements. Movement can be disabled by applying a movement-blocking tag that the movement component respects. The "wake on damage" requires monitoring Health attribute changes while the GE is active.
- **Fire Ward absorb shield:** This is a more complex GE. Options: (a) Use a custom `UGameplayEffectExecutionCalculation` that intercepts fire damage and reduces it by remaining shield HP, (b) Use a separate "ShieldHP" attribute that decrements on fire damage. Recommend (b) — add a `FireWardShieldHP` attribute to `UMordecaiAttributeSet` (or use a stacking GE). When the attribute reaches 0, remove the GE.
- **TODO(DECISION): Fire Ward scope** — The new_spells_proposal.md says Fire Ward "absorbs a flat amount of magic damage" (not just fire). The skill_sheet says "absorbs all fire damage." Defaulting to fire-only per skill_sheet. Revisit when rank milestones are implemented (Rank 5 "Elemental Attune" adapts to first element absorbed).
- **Rank 1 only:** No Blink Strike (Rank 5), no Sleep spread (Rank 10), no Fire Ward reflect (Rank 10). Those are future stories.

## Tests Required
- [ ] `Mordecai.Spell.Blink.TeleportsCasterForward` — Caster location changes by ~Range in aim direction (verifies AC-021.1)
- [ ] `Mordecai.Spell.Blink.ClampsToValidPosition` — Teleport stops before wall if wall is within Range (verifies AC-021.2)
- [ ] `Mordecai.Spell.Blink.GrantsIFrames` — Invulnerable tag applied for InvulnerabilityDuration after teleport (verifies AC-021.3)
- [ ] `Mordecai.Spell.Blink.CostsSP` — SpellPoints deducted on cast (verifies SpellBase integration)
- [ ] `Mordecai.Spell.Sleep.AppliesSleepToTarget` — First enemy in line trace receives Sleeping tag (verifies AC-021.5, AC-021.6)
- [ ] `Mordecai.Spell.Sleep.SleepingTargetCannotAct` — Sleeping target cannot move or activate abilities (verifies AC-021.6)
- [ ] `Mordecai.Spell.Sleep.DamageWakesSleepingTarget` — Dealing damage to sleeping target removes Sleep GE (verifies AC-021.7)
- [ ] `Mordecai.Spell.FireWard.AbsorbsFireDamage` — Fire damage reduced by shield, shield HP decremented (verifies AC-021.9, AC-021.10)
- [ ] `Mordecai.Spell.FireWard.DoesNotAbsorbNonFireDamage` — Physical/cold/force damage passes through unaffected (verifies AC-021.10)
- [ ] `Mordecai.Spell.FireWard.ExpiresAfterDuration` — GE and tag removed when Duration expires (verifies AC-021.9)
- [ ] `Mordecai.Spell.FireWard.BreaksWhenShieldDepleted` — GE removed when shield HP reaches 0 (verifies AC-021.9)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-021]` prefix
