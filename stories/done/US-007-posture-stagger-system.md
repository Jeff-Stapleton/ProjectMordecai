# US-007: Posture & Stagger System

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement the posture meter and stagger system. Every actor with an ASC has a posture meter that depletes when taking posture damage (from heavy attacks, perfect parries, slams, certain abilities). When posture reaches zero, the actor enters a "posture broken" stagger state — a critical window where they are vulnerable to a riposte for bonus damage. Posture regenerates after a delay when not taking posture damage. This is a core combat pillar: "enemies are not sponges" — posture break creates openings through skilled play.

## References
- Design doc: `combat_system_v1.md` — Posture System, Riposte/Critical Window
- Design doc: `attack_taxonomy_v1.md` — `PostureDamageScalar` field on attack profiles
- Design doc: `character_attributes_v1.md` — STR and DIS affect posture: `Posture = BasePosture + (STR × 2) + (DIS × 1)`
- Design doc: `stat_formulas_v1.md` — Posture formula, DIS posture recovery scaling: `PostureRecovery = Base + (DIS × 2.0%)`
- Design doc: `status_effects_v1.md` — Weakened reduces posture damage dealt; Brittle increases posture damage taken (interfaces only for now)
- Agent rules: `agent_rules_v2.md` — GAS for all attributes/effects; server-authoritative; UE 5.7

---

## Acceptance Criteria

### Posture Attributes
- [x] AC-007.1: `Posture` and `MaxPosture` attributes exist in `UMordecaiAttributeSet` (should already exist from US-001; verify and ensure proper clamping: 0 ≤ Posture ≤ MaxPosture)
- [x] AC-007.2: MaxPosture is initialized from base value (placeholder default: 100.0). Attribute scaling from STR/DIS is `TODO(DECISION)` — stubbed as base value only until Epic 3 (Attributes & Progression)

### Posture Damage
- [x] AC-007.3: Posture damage is applied via a Gameplay Effect using SetByCaller magnitude with a dedicated `Mordecai.Data.PostureDamage` gameplay tag
- [x] AC-007.4: Posture damage is calculated as: `PostureDamage = BasePower × PostureDamageScalar` (values from the attack profile, applied by the melee attack ability US-004)
- [x] AC-007.5: Posture damage respects `Mordecai.State.Blocking` — blocked hits still apply posture damage but at a reduced rate (configurable `BlockPostureDamageMultiplier`, default 0.5)
- [x] AC-007.6: Perfect Parry applies large posture damage to the attacker (value set by US-006's `ParryPostureDamage` parameter)

### Posture Break (Stagger)
- [x] AC-007.7: When Posture reaches 0, `Mordecai.State.PostureBroken` gameplay tag is applied to the actor
- [x] AC-007.8: During PostureBroken state, the actor cannot activate abilities (use GAS activation blocked tags: `Mordecai.State.PostureBroken` blocks all combat abilities)
- [x] AC-007.9: PostureBroken (Staggered) lasts for 1 second (`PostureBrokenDurationMs`, default 1000ms)
- [x] AC-007.10: During Staggered state, incoming damage is multiplied by 1.5x (`StaggeredDamageMultiplier`, default 1.5)
- [x] AC-007.10a: **Knock Down**: If the actor takes enough damage during the Staggered window (configurable `KnockDownDamageThreshold`), they transition to `Mordecai.State.KnockedDown` which extends the vulnerability window to a longer duration (`KnockDownDurationMs`, default 2000ms) with a 2.0x damage multiplier (`KnockDownDamageMultiplier`, default 2.0). Knock Down replaces the remaining Stagger — it's an escalation, not stacked on top.
- [x] AC-007.11: After PostureBroken expires, Posture resets to MaxPosture

### Posture Regeneration
- [x] AC-007.12: Posture regenerates passively at a configurable rate (`PostureRegenRate`, units per second, default placeholder: 10.0)
- [x] AC-007.13: Posture regeneration is paused for a configurable delay after taking posture damage (`PostureRegenDelayMs`, default placeholder: 3000ms)
- [x] AC-007.14: Posture does NOT regenerate during `Mordecai.State.PostureBroken`

### Status Effect Interfaces (Stubs Only)
- [x] AC-007.15: An interface/hook exists for `Mordecai.Status.Weakened` to reduce outgoing posture damage by a configurable multiplier (default 1.0 — no effect until Epic 4)
- [x] AC-007.16: An interface/hook exists for `Mordecai.Status.Brittle` to increase incoming posture damage by a configurable multiplier (default 1.0 — no effect until Epic 4)

## Technical Notes
- Place in `Plugins/GameFeatures/MordecaiCore/Source/MordecaiCoreRuntime/Combat/`
- **Posture damage GE**: Create `GE_MordecaiPostureDamage` with SetByCaller magnitude. The melee attack ability (US-004) applies this alongside health damage.
- **Posture break detection**: Use `PostGameplayEffectExecute` in the AttributeSet to check if Posture reached 0, then apply the PostureBroken GE (duration-based, grants tag).
- **Riposte multiplier**: Implement via a custom `UGameplayEffectExecutionCalculation` that checks for `Mordecai.State.PostureBroken` on the target and multiplies damage accordingly. Alternatively, use a passive GE on the broken actor that modifies incoming damage.
- **Posture regen**: Use a passive infinite-duration GE with periodic tick that adds posture. Track "last posture damage time" and skip regen ticks within the delay window.
- **PostureBroken state**: Apply via a duration GE (1s) that grants the tag and blocks abilities. On expire, reset Posture to MaxPosture via a second GE or in the PostGameplayEffectExecute callback.
- **Knock Down escalation**: Track cumulative damage during PostureBroken. If it exceeds `KnockDownDamageThreshold`, remove the stagger GE and apply a `KnockedDown` GE (longer duration, 2.0x damage multiplier). The knocked down state replaces stagger — it's not additive. Use `Mordecai.State.KnockedDown` tag. Blocked abilities should include both PostureBroken and KnockedDown.
- **Blocked posture damage**: In the damage execution, check `Mordecai.State.Blocking` and multiply posture damage by `BlockPostureDamageMultiplier`.
- **Weakened/Brittle stubs**: Add `float WeakenedPostureMultiplier = 1.0f` and `float BrittlePostureMultiplier = 1.0f` as configurable properties. The damage execution reads these but they have no effect until Epic 4 wires them.
- Do NOT implement animation integration — stagger is tag-based only for now.

## Tests Required
- [x] `Mordecai.Posture.PostureDamageReducesPosture` — apply posture damage GE, verify Posture attribute decreases (AC-007.3)
- [x] `Mordecai.Posture.PostureDamageUsesScalar` — apply BasePower=20, PostureDamageScalar=1.5, verify 30 posture damage (AC-007.4)
- [x] `Mordecai.Posture.BlockReducesPostureDamage` — block a hit, verify posture damage = base × BlockPostureDamageMultiplier (AC-007.5)
- [x] `Mordecai.Posture.BreaksAtZero` — reduce posture to 0, verify PostureBroken tag applied (AC-007.7)
- [x] `Mordecai.Posture.BrokenStateBlocksAbilities` — apply PostureBroken, attempt to activate combat ability, verify blocked (AC-007.8)
- [x] `Mordecai.Posture.StaggerLastsOneSecond` — break posture, verify PostureBroken expires after 1000ms (AC-007.9)
- [x] `Mordecai.Posture.StaggeredDamageMultiplied` — break posture, deal damage during stagger, verify damage × 1.5 (AC-007.10)
- [x] `Mordecai.Posture.KnockDownOnThresholdDamage` — break posture, deal damage exceeding KnockDownDamageThreshold during stagger, verify KnockedDown tag applied (AC-007.10a)
- [x] `Mordecai.Posture.KnockDownExtendsDuration` — trigger knock down, verify vulnerability window extends to KnockDownDurationMs (AC-007.10a)
- [x] `Mordecai.Posture.KnockDownDoubleDamage` — deal damage during KnockedDown state, verify damage × 2.0 (AC-007.10a)
- [x] `Mordecai.Posture.ResetsAfterBrokenExpires` — break posture, wait for expiry, verify Posture == MaxPosture (AC-007.11)
- [x] `Mordecai.Posture.RegeneratesOverTime` — deal partial posture damage, wait past regen delay, verify posture increases over time (AC-007.12)
- [x] `Mordecai.Posture.RegenPausedAfterDamage` — deal posture damage, immediately check regen, verify no regen during delay period (AC-007.13)
- [x] `Mordecai.Posture.NoRegenDuringBroken` — break posture, verify no regen during PostureBroken state (AC-007.14)
- [x] `Mordecai.Posture.PostureClampedToMax` — attempt to set Posture above MaxPosture, verify clamped (AC-007.1)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-007]` prefix
