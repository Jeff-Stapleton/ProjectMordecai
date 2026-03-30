# US-056: Status-Applying Attack Profiles

## Overview
Extend the attack profile data model to support optional status effect application on hit. This allows enemies (and eventually players with weapon affixes) to apply status effects like Burning, Weakened, or Frostbitten through their normal attacks without requiring separate ability logic. The status-on-hit system feeds into the playable magic arena (US-057) where enemies use status-applying attacks.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `attack_taxonomy_v1.md` — Section 2.1 `OnHitPayload: statuses, AoE-on-hit, spawn effects, etc.`
- Design doc: `combat_system_v1.md` — Damage application via GAS
- Design doc: `status_effects_launch_set_v1.md` — Status effects that enemies can apply
- Design doc: `damage_types_v1.md` — Fire→Burning, Frost→Frostbitten, etc. damage-to-status relationships
- Agent rules: `agent_rules_v2.md` — GAS for all effects, data-driven
- Existing code: `UMordecaiAttackProfileDataAsset`, `FMordecaiDamageProfile` (US-002), `UMordecaiGA_MeleeAttack` (US-004), `UMordecaiStatusEffectComponent` (US-013)

---

## Acceptance Criteria
- [ ] AC-056.1: `FMordecaiStatusOnHitEntry` struct exists with fields: `StatusEffectTag` (FGameplayTag — identifies which status to apply, e.g., `Mordecai.Status.Burning`), `ApplicationChance` (float 0.0–1.0, default 1.0), `StatusEffectGEClass` (TSubclassOf<UGameplayEffect> — the GE to apply)
- [ ] AC-056.2: `UMordecaiAttackProfileDataAsset` has a new `TArray<FMordecaiStatusOnHitEntry> StatusEffectsOnHit` UPROPERTY field. Empty by default (backwards compatible with existing profiles)
- [ ] AC-056.3: `UMordecaiGA_MeleeAttack` hit processing is extended: after applying damage, iterate `StatusEffectsOnHit` from the active attack profile. For each entry, roll against `ApplicationChance`. On success, apply `StatusEffectGEClass` to the target via the attacker's ASC
- [ ] AC-056.4: Status application respects immunity tags: if the target has the corresponding `Mordecai.Immunity.*` tag, the status is not applied (this is already handled by the GE's application requirements from US-013, but verify it works through this pipeline)
- [ ] AC-056.5: `AMordecaiProjectile` on-hit processing is extended with the same pattern: projectile reads `StatusEffectsOnHit` from its associated attack profile and applies statuses on hit
- [ ] AC-056.6: The `ApplicationChance` roll uses a deterministic seed based on the hit instance (not pure random) so that the same hit always produces the same result in replicated environments. Implementation: use the hit actor's network ID + frame number as seed, or use `FMath::FRand()` (acceptable for vertical slice — deterministic seeding is TODO for production)
- [ ] AC-056.7: Multiple status entries on a single attack profile all resolve independently. An attack with both Burning (50% chance) and Weakened (100% chance) rolls separately for each

## Technical Notes
- **Backwards compatible:** The new `StatusEffectsOnHit` array defaults to empty. All existing attack profiles (player combo, enemy basic slash) continue to work with no status application.
- **GE class reference:** The `StatusEffectGEClass` field stores a `TSubclassOf<UGameplayEffect>`. This allows referencing C++ GE classes directly (e.g., `UMordecaiGE_Burning::StaticClass()`). For Blueprint GEs, the DataAsset configured in EDITOR stories can point to BP GE classes.
- **Roll location:** The chance roll should happen on the server (authority) since damage application is server-authoritative. The melee attack ability already runs on the server.
- **Pattern:** This mirrors the `OnHitPayload` field described in `attack_taxonomy_v1.md`. We're implementing the "statuses" portion of that payload. AoE-on-hit and spawn effects are deferred.
- Place the struct in `MordecaiCombatTypes.h` alongside existing combat structs.

## Tests Required
- [ ] `Mordecai.Combat.StatusOnHit.AppliesStatusOnMeleeHit` — Melee attack with StatusEffectsOnHit (100% chance) applies the status GE to the target (verifies AC-056.3)
- [ ] `Mordecai.Combat.StatusOnHit.RespectsProbability` — Attack with 0% chance never applies status; 100% always applies (verifies AC-056.3, AC-056.6)
- [ ] `Mordecai.Combat.StatusOnHit.RespectImmunity` — Target with immunity tag is not affected even when roll succeeds (verifies AC-056.4)
- [ ] `Mordecai.Combat.StatusOnHit.AppliesStatusOnProjectileHit` — Projectile with StatusEffectsOnHit applies status to target (verifies AC-056.5)
- [ ] `Mordecai.Combat.StatusOnHit.MultipleStatusesResolveIndependently` — Attack with 2 status entries rolls each independently (verifies AC-056.7)
- [ ] `Mordecai.Combat.StatusOnHit.EmptyArrayNoEffect` — Attack with empty StatusEffectsOnHit applies no statuses (verifies AC-056.2, backwards compatibility)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-056]` prefix
