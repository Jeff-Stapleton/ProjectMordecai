# US-060: Tactical Spells — Summon & Evasion (Illusion, Blur)

## Overview
Implement two tactical spells with more complex mechanics using the Spell Framework from US-019. Illusion (summon an aggro-drawing decoy actor) and Blur (self-buff granting ranged evasion chance). These require new actor types and projectile pipeline modifications, respectively.

Split from original US-023 scope. Snare, Enfeeble, and Enchant Weapon are in US-023.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `ability_schema_v1.md` (delivery types: SummonActor, InstantApply)
- Design doc: `new_spells_proposal.md` (Illusion: Rank 1 summon decoy; Blur: Rank 1 evasion buff)
- Design doc: `skill_sheet_v1.1.md` (Illusion: creates illusion that mirrors actions)
- Agent rules: `agent_rules_v2.md` (GAS for abilities, fixed diorama camera)
- Existing code: `UMordecaiGA_SpellBase`, `UMordecaiSpellDataAsset` (US-019), `AMordecaiEnemyAIController` (US-051, aggro system for Illusion), `AMordecaiProjectile` (US-009, projectile hit for Blur miss)

---

## Acceptance Criteria

### Illusion (Decoy summon)
- [ ] AC-060.1: `UMordecaiGA_Illusion` extends `UMordecaiGA_SpellBase`. Delivery: SummonActor. On cast, spawns an `AMordecaiIllusionActor` at a position slightly in front of the caster
- [ ] AC-060.2: `AMordecaiIllusionActor` exists in `Mordecai/Magic/`. It has configurable HP (from BasePower) and Duration (from SpellDataAsset). Destroyed when HP reaches 0 or Duration expires
- [ ] AC-060.3: Enemies within aggro range prefer targeting the Illusion over the player. Implementation: `AMordecaiIllusionActor` has `Mordecai.Team.Ally` tag. `AMordecaiEnemyAIController` target selection is extended to prefer actors with this tag within aggro range (decoy draws aggro)
- [ ] AC-060.4: Ability tag: `Mordecai.Ability.Spell.Illusion`

### Blur (Evasion buff)
- [ ] AC-060.5: `UMordecaiGA_Blur` extends `UMordecaiGA_SpellBase`. Delivery: InstantApply self-targeted. On cast, applies a duration GE granting an evasion buff
- [ ] AC-060.6: Blur GE applies `Mordecai.Status.Blurred` tag for configurable Duration. While active, incoming ranged projectile attacks have a configurable MissChance (from BasePower as percentage, e.g., BasePower=30 means 30% chance to miss). Melee attacks are unaffected at Rank 1. Reverts when GE expires
- [ ] AC-060.7: Miss chance implementation: when a projectile hits a target with `Mordecai.Status.Blurred`, roll against the MissChance. On miss, skip damage application. TODO(DECISION): Recommend option (a) — simple roll in the damage pipeline, skip damage if miss
- [ ] AC-060.8: Ability tag: `Mordecai.Ability.Spell.Blur`. New tag: `Mordecai.Status.Blurred`

## Technical Notes
- **Illusion actor:** `AMordecaiIllusionActor` is a minimal actor with: a collision capsule, health (simple float, not full ASC), team tag (`Mordecai.Team.Ally`), and a "die on HP = 0 or timeout" behavior. It does NOT need a full ASC — just enough to take damage and be targetable.
- **Illusion aggro modification:** `AMordecaiEnemyAIController::FindBestTarget()` (from US-051) needs a small extension: when choosing targets, prefer actors with `Mordecai.Team.Ally` tag within aggro range. This is a lightweight priority tweak, not a full AI rewrite.
- **Blur miss chance:** The projectile hit processing (`AMordecaiProjectile::OnHit` or the damage execution) needs to check if the target has `Mordecai.Status.Blurred` and roll against MissChance. If miss, the projectile passes through or damage is zeroed. The miss chance value can be stored as an attribute (`RangedEvasionChance`, default 0) modified by the Blur GE, or read from the active GE's SetByCaller magnitude.
- **Rank 1 only:** No Mirror Bait (Illusion Rank 5), no Afterimage (Blur Rank 5).
- Place spell classes in `Source/LyraGame/Mordecai/Magic/`, Illusion actor also in `Mordecai/Magic/`.

## Tests Required
- [ ] `Mordecai.Spell.Illusion.SpawnsDecoyActor` — IllusionActor spawned near caster on cast (verifies AC-060.1)
- [ ] `Mordecai.Spell.Illusion.DecoyDrawsAggro` — Enemy AI targets decoy over player when in range (verifies AC-060.3)
- [ ] `Mordecai.Spell.Illusion.DecoyDiesOnTimeout` — Decoy destroyed after Duration (verifies AC-060.2)
- [ ] `Mordecai.Spell.Illusion.DecoyDiesOnHPDepleted` — Decoy destroyed when HP reaches 0 (verifies AC-060.2)
- [ ] `Mordecai.Spell.Blur.GrantsEvasion` — Blurred tag applied, incoming ranged attacks can miss (verifies AC-060.5, AC-060.6)
- [ ] `Mordecai.Spell.Blur.MeleeAttacksUnaffected` — Melee attacks are not affected by Blur at Rank 1 (verifies AC-060.6)
- [ ] `Mordecai.Spell.Blur.EvasionExpiresAfterDuration` — Blur removed after Duration (verifies AC-060.6)
- [ ] `Mordecai.Spell.Blur.MissChanceMatchesConfig` — Over many trials, miss rate approximates configured MissChance (verifies AC-060.7)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-060]` prefix
