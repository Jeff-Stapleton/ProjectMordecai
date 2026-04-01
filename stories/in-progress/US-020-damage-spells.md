# US-020: Damage Spells (Fireball, Cone of Cold, Magic Missile, Stone Skin)

## Overview
Implement the four damage/defensive spells using the Spell Framework from US-019. Fireball (projectile, fire damage), Cone of Cold (AoE cone, cold damage), Magic Missile (channeled homing projectiles, force damage), and Stone Skin (self-buff, damage absorption shield). Each spell implements Rank 1 base behavior only; rank milestone unlocks (Rank 5/10/15/20) are deferred to future stories.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `ability_schema_v1.md` (delivery types: SpawnProjectile, TraceHit, InstantApply; targeting types; scaling)
- Design doc: `skill_sheet_v1.1.md` (Fireball: channeled fire projectiles; Cone of Cold: ice cone + freeze; Magic Missile: channeled force projectiles; Stone Skin: armor buff)
- Design doc: `damage_types_v1.md` (Fire, Cold, Force damage types)
- Design doc: `combat_system_v1.md` (damage application via GAS)
- Agent rules: `agent_rules_v2.md` (GAS for abilities, data-driven)
- Existing code: `UMordecaiGA_SpellBase`, `UMordecaiSpellDataAsset` (US-019), `AMordecaiProjectile` (US-009), `UMordecaiAimAssistSubsystem` (US-009), `UMordecaiHitDetectionSubsystem` (US-003, shape queries for cone)

---

## Acceptance Criteria
- [ ] AC-020.1: `UMordecaiGA_Fireball` extends `UMordecaiGA_SpellBase`. Overrides `OnSpellCast()` to spawn a `AMordecaiProjectile` in the caster's aim direction. On projectile hit, applies a fire damage GE with magnitude = SpellPower
- [ ] AC-020.2: Fireball projectile speed, lifetime, and collision radius are configurable via the SpellDataAsset (not hardcoded). Default: speed 2000 cm/s, lifetime 3.0s, radius 30cm
- [ ] AC-020.3: Fireball applies damage tagged with `Mordecai.Damage.Fire` gameplay tag
- [ ] AC-020.4: `UMordecaiGA_ConeOfCold` extends `UMordecaiGA_SpellBase`. Overrides `OnSpellCast()` to perform a cone shape query (using `UMordecaiHitDetectionSubsystem` arc sector query) hitting all enemies within ArcDegrees and Range from the SpellDataAsset. Applies cold damage GE to each hit target with magnitude = SpellPower
- [ ] AC-020.5: Cone of Cold applies damage tagged with `Mordecai.Damage.Cold` gameplay tag
- [ ] AC-020.6: Cone of Cold does NOT hit targets behind the caster or outside the arc. Targets at the edge of the arc are included
- [ ] AC-020.7: `UMordecaiGA_MagicMissile` extends `UMordecaiGA_SpellBase`. Overrides casting to fire N projectiles (configurable, default 5) over the CastTime duration. Each projectile deals force damage = SpellPower / N
- [ ] AC-020.8: Magic Missile projectiles use the `UMordecaiAimAssistSubsystem` to acquire the nearest valid target within Range and home toward it. If no valid target exists, projectiles fire in the caster's aim direction
- [ ] AC-020.9: Magic Missile applies damage tagged with `Mordecai.Damage.Force` gameplay tag
- [ ] AC-020.10: `UMordecaiGA_StoneSkin` extends `UMordecaiGA_SpellBase`. Overrides `OnSpellCast()` to apply a duration GameplayEffect to the caster that grants a flat `DamageReduction` attribute bonus (configurable, read from BasePower in SpellDataAsset)
- [ ] AC-020.11: Stone Skin GE lasts for a configurable Duration (from SpellDataAsset) and applies tag `Mordecai.Status.StoneSkin`. Removed when duration expires
- [ ] AC-020.12: All 4 spells declare their ability tags: `Mordecai.Ability.Spell.Fireball`, `Mordecai.Ability.Spell.ConeOfCold`, `Mordecai.Ability.Spell.MagicMissile`, `Mordecai.Ability.Spell.StoneSkin`

## Technical Notes
- **Fireball:** Reuses the existing `AMordecaiProjectile` from US-009. Configure it with fire-specific parameters. The on-hit callback applies the damage GE via the spell's ASC.
- **Cone of Cold:** Reuses `UMordecaiHitDetectionSubsystem::PerformArcSectorQuery` from US-003. The cone shape is an arc sector centered on the caster's facing direction.
- **Magic Missile:** This is a channeled spell. Instead of a single `OnSpellCast()`, it fires projectiles at intervals during the Cast phase (`CastTime / N` interval). Each projectile is a `AMordecaiProjectile` with homing enabled. Homing target acquired via `UMordecaiAimAssistSubsystem::FindBestTarget`.
- **Stone Skin:** Simple self-buff. The DamageReduction attribute should already exist in `UMordecaiAttributeSet` or be added. If `DamageReduction` doesn't exist, add it as a new attribute (flat subtraction from incoming damage, clamped at 0).
- **Damage GE:** Use SetByCaller magnitude for the damage value, tagged with the appropriate damage type. This mirrors the melee damage GE pattern.
- **Rank 1 only:** Do NOT implement rank milestone effects (e.g., Fireball explosion on impact at Rank 5, Cone of Cold freeze at Rank 10). Those are separate stories. Power scaling from rank is already handled by SpellBase (US-019).
- **TODO(DECISION): Frostbitten buildup** — Cone of Cold should ideally apply Frostbitten status buildup, but US-015 is BLOCKED. For now, apply cold damage only. Frostbitten integration can be added when US-015 is resolved.

## Tests Required
- [ ] `Mordecai.Spell.Fireball.ProjectileSpawnsOnCast` — Fireball spawns a projectile in aim direction (verifies AC-020.1)
- [ ] `Mordecai.Spell.Fireball.DealsDamageOnHit` — Projectile hit applies fire damage GE (verifies AC-020.1, AC-020.3)
- [ ] `Mordecai.Spell.Fireball.DamageEqualsSpellPower` — Damage magnitude matches computed SpellPower (verifies AC-020.1)
- [ ] `Mordecai.Spell.ConeOfCold.HitsAllEnemiesInCone` — All enemies within arc+range are damaged (verifies AC-020.4)
- [ ] `Mordecai.Spell.ConeOfCold.MissesTargetsOutsideCone` — Enemies behind caster or outside arc are not hit (verifies AC-020.6)
- [ ] `Mordecai.Spell.ConeOfCold.DealsColdDamage` — Damage tagged with Mordecai.Damage.Cold (verifies AC-020.5)
- [ ] `Mordecai.Spell.MagicMissile.FiresMultipleProjectiles` — N projectiles spawned over CastTime (verifies AC-020.7)
- [ ] `Mordecai.Spell.MagicMissile.ProjectilesHome` — Projectiles seek nearest valid target (verifies AC-020.8)
- [ ] `Mordecai.Spell.MagicMissile.TotalDamageEqualsSpellPower` — Sum of all projectile damage = SpellPower (verifies AC-020.7)
- [ ] `Mordecai.Spell.StoneSkin.AppliesDamageReduction` — DamageReduction attribute increased while active (verifies AC-020.10)
- [ ] `Mordecai.Spell.StoneSkin.BuffExpiresAfterDuration` — GE and tag removed after Duration (verifies AC-020.11)
- [ ] `Mordecai.Spell.StoneSkin.ReducesIncomingDamage` — Incoming damage reduced by DamageReduction amount (verifies AC-020.10)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-020]` prefix
