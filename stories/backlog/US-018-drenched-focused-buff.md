# US-018: Drenched & Focused Buff (Environmental + Self-Buff)

## Execution Mode
**Mode:** HEADLESS

## Overview
Implement the final two statuses from the launch set: Drenched (environmental interaction modifier that shifts elemental rules) and Focused (self-buff from perfect action streaks that rewards mastery). These are unique — Drenched is the only status that modifies how other statuses interact with the target, and Focused is the only positive status in the launch set.

## References
- Design doc: `status_effects_launch_set_v1.md` — Drenched ("Elemental rules shift"), Focused ("Mastery streak reward")
- Design doc: `damage_types_v1.md` — Fire, Frost, Lightning interactions with Drenched
- Design doc: `game_design_v2.md` — Section 2 (Skill-first combat; "perfect dodge bonus", "perfect block can reward")
- Design doc: `combat_system_v1.md` — Dodge/Block/Parry perfect action rewards
- Agent rules: `agent_rules_v2.md` — GAS for all status effects
- Depends on: US-013 (Status Effect Framework), interfaces with US-014 (Burning), US-015 (Frostbitten, Shocked)

---

## Acceptance Criteria

### Drenched
- [ ] AC-018.1: `UMordecaiGE_Drenched` exists. Applies `Mordecai.Status.Drenched` tag. Duration: configurable `DrenchedDurationSec` (default placeholder: 15.0s).
- [ ] AC-018.2: While Drenched, fire damage taken is REDUCED by a configurable percentage: `DrenchedFireDamageReductionPct` (default placeholder: 0.25, meaning 25% less fire damage taken). Implementation: when calculating incoming fire damage (`Mordecai.Damage.Fire`), if target has `Mordecai.Status.Drenched`, apply reduction. This can be a `FireDamageReceivedMultiplier` attribute (default 1.0) modified by the GE, or a check in the damage execution.
- [ ] AC-018.3: While Drenched, Shocked effects are increased. Implementation: when Shocked is applied to a Drenched target, stack count increases by `DrenchedShockedBonusStacks` (default placeholder: 1, meaning 1 extra stack per application). Alternatively, Shocked micro-stun chance is multiplied by `DrenchedShockedMultiplier` (default placeholder: 1.50, meaning 50% more likely). `TODO(DECISION)` — which approach. Implement the multiplier approach as default.
- [ ] AC-018.4: While Drenched, Frostbitten buildup is increased. Implementation: when Frostbitten is applied to a Drenched target, stack count increases by `DrenchedFrostBonusStacks` (default placeholder: 1, meaning 1 extra frost stack per application). This accelerates the path to Frozen.
- [ ] AC-018.5: Drenched can be tactically beneficial: applying Drenched to a Burning target removes Burning (dousing). Implementation: when Drenched is applied, check for `Mordecai.Status.Burning` and remove it if present.
- [ ] AC-018.6: Drenched can be removed by heat/fire sources or zone transitions. Implementation: applying a fire effect to a Drenched target removes Drenched (and does NOT apply Burning — the water neutralizes). `TODO(DECISION)` — exact rules for fire-vs-drenched interaction. For now: fire on drenched → remove drenched, skip burning application.
- [ ] AC-018.7: Drenched is blocked if target has `Mordecai.Immunity.Drenched` tag.

### Focused (Buff)
- [ ] AC-018.8: `UMordecaiGE_Focused` exists. Applies `Mordecai.Status.Focused` tag. Duration: Infinite (stays until broken by hit). Not time-limited.
- [ ] AC-018.9: Focused is granted by perfect action streaks: when the player performs consecutive perfect dodges, perfect blocks, or perfect parries (configurable `FocusedStreakThreshold`, default placeholder: 3 consecutive perfect actions), apply Focused. Implementation: `UMordecaiPerfectActionTracker` component tracks consecutive perfect actions. Register `Mordecai.Event.PerfectAction` gameplay event. When received, increment counter. On reaching threshold, apply Focused GE.
- [ ] AC-018.10: While Focused, stamina tier penalties are forgiven (tier multipliers treated as Green tier regardless of actual stamina level). Implementation: modify `StaminaTierPenaltyMultiplier` to 0 (no penalties). Alternatively, set `StaminaTierOverride` attribute to force Green tier. `TODO(DECISION)` — approach. For now, use `StaminaTierPenaltyMultiplier = 0` approach.
- [ ] AC-018.11: While Focused, posture damage dealt receives a bonus: `FocusedPostureDamageBonus` (default placeholder: 0.20, meaning 20% more posture damage). Implementation: modify `OutgoingPostureDamageMultiplier` attribute.
- [ ] AC-018.12: Focused breaks on taking any hit: when the Focused target receives damage, remove Focused GE and reset the perfect action counter. Implementation: listen for `Mordecai.Event.DamageTaken`; on receive while Focused, remove GE.
- [ ] AC-018.13: Focused is a self-buff — it cannot be applied by external sources. Only the `UMordecaiPerfectActionTracker` component can grant it. It is NOT blocked by Cursed (Cursed disables "some buffs" but Focused is earned through mastery and should persist — `TODO(DECISION)` on which buffs Cursed blocks).
- [ ] AC-018.14: Perfect action counter resets on taking a hit (even if Focused threshold not yet reached). Missing a perfect window (normal dodge/block instead of perfect) also resets the counter.
- [ ] AC-018.15: Focused is NOT blocked by immunity tags (it's a self-buff, not a debuff). No `Mordecai.Immunity.Focused` check needed on application.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/StatusEffects/Effects/`
- `UMordecaiPerfectActionTracker`: `UActorComponent` that:
  - Listens for `Mordecai.Event.PerfectAction` gameplay events
  - Tracks consecutive count
  - Applies Focused GE when threshold reached
  - Listens for `Mordecai.Event.DamageTaken` to reset counter and break Focused
  - Listens for non-perfect action events to reset counter
- Drenched elemental synergies: the cleanest approach is to have the damage execution / status application code check for `Mordecai.Status.Drenched` on the target and adjust accordingly. This means:
  - Burning application code checks for Drenched → removes Drenched, skips Burning
  - Drenched application code checks for Burning → removes Burning
  - Shocked application code checks for Drenched → adds bonus stacks
  - Frostbitten application code checks for Drenched → adds bonus stacks
  - These checks live in the respective status GE application logic or in the `UMordecaiStatusEffectComponent`
- New attributes needed:
  - `FireDamageReceivedMultiplier` (default 1.0) — element-specific damage modifier
- New tags to register:
  - `Mordecai.Event.PerfectAction` — generic event for any perfect action (dodge, block, parry)
  - `Mordecai.Event.DamageTaken` — fired when actor takes damage (if not already registered)
- Drenched fire interaction: this creates a bidirectional relationship (fire removes drenched, drenched removes burning). Implement as explicit checks in both Drenched and Burning application pathways.
- Focused infinite duration: use `EGameplayEffectDurationType::Infinite` with explicit removal on damage.

## Tests Required

### Drenched Tests
- [ ] `Mordecai.StatusEffect.Drenched.AppliesTag` — apply Drenched, verify tag active (AC-018.1)
- [ ] `Mordecai.StatusEffect.Drenched.ReducesFireDamage` — apply Drenched, deal fire damage, verify reduced by `DrenchedFireDamageReductionPct` (AC-018.2)
- [ ] `Mordecai.StatusEffect.Drenched.AmplifiesShocked` — apply Drenched, then apply Shocked, verify Shocked effect is amplified (extra stacks or multiplied chance) (AC-018.3)
- [ ] `Mordecai.StatusEffect.Drenched.AmplifiesFrostbitten` — apply Drenched, then apply Frostbitten, verify extra frost stacks (AC-018.4)
- [ ] `Mordecai.StatusEffect.Drenched.DousesBurning` — apply Burning, then apply Drenched, verify Burning removed (AC-018.5)
- [ ] `Mordecai.StatusEffect.Drenched.FireRemovesDrenched` — apply Drenched, then apply fire effect, verify Drenched removed and Burning NOT applied (AC-018.6)
- [ ] `Mordecai.StatusEffect.Drenched.BlockedByImmunity` — add immunity, apply, verify not applied (AC-018.7)

### Focused Tests
- [ ] `Mordecai.StatusEffect.Focused.GrantedAfterPerfectStreak` — send `PerfectAction` events reaching threshold, verify Focused applied (AC-018.9)
- [ ] `Mordecai.StatusEffect.Focused.ForgivesStaminaPenalties` — apply Focused, verify `StaminaTierPenaltyMultiplier` is 0 (AC-018.10)
- [ ] `Mordecai.StatusEffect.Focused.BoostsPostureDamage` — apply Focused, verify `OutgoingPostureDamageMultiplier` increased (AC-018.11)
- [ ] `Mordecai.StatusEffect.Focused.BreaksOnDamageTaken` — apply Focused, deal damage, verify Focused removed (AC-018.12)
- [ ] `Mordecai.StatusEffect.Focused.CounterResetsOnHit` — start building perfect streak, deal damage before threshold, verify counter resets to 0 (AC-018.14)
- [ ] `Mordecai.StatusEffect.Focused.CounterResetsOnNonPerfectAction` — start building streak, send non-perfect action event, verify counter resets (AC-018.14)
- [ ] `Mordecai.StatusEffect.Focused.PersistUntilBroken` — apply Focused, wait extended time, verify still active (infinite duration) (AC-018.8)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-018]` prefix
