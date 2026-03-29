# US-023: Tactical Spells (Snare, Enfeeble, Enchant Weapon, Illusion, Blur)

## Overview
Implement five tactical spells using the Spell Framework from US-019. Snare (single-target root), Enfeeble (single-target Weakened debuff), Enchant Weapon (self-buff adding elemental damage to melee), Illusion (summon aggro-drawing decoy), and Blur (self-buff evasion). Each spell implements Rank 1 base behavior only; rank milestone unlocks are deferred to future stories.

**Scope risk:** This story has 5 spells with 15 ACs and 15 tests. If implementation exceeds a single session, split into US-023a (Snare, Enfeeble, Enchant Weapon — simpler apply-GE spells) and US-023b (Illusion, Blur — more complex mechanics).

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `ability_schema_v1.md` (delivery types: TraceHit, InstantApply, SummonActor; targeting types)
- Design doc: `new_spells_proposal.md` (Snare: Rank 1 root single target; Enfeeble: Rank 1 apply Weakened; Enchant Weapon: Rank 1 add elemental damage; Illusion: Rank 1 summon decoy; Blur: Rank 1 evasion buff)
- Design doc: `skill_sheet_v1.1.md` (Illusion: creates illusion that mirrors actions; Blink/Float/etc. for reference patterns)
- Design doc: `status_effects_v1.md` (Weakened, Rooted status effect definitions — if applicable from Epic 4)
- Agent rules: `agent_rules_v2.md` (GAS for abilities, fixed diorama camera)
- Existing code: `UMordecaiGA_SpellBase`, `UMordecaiSpellDataAsset` (US-019), `UMordecaiHitDetectionSubsystem` (US-003, line trace), `AMordecaiEnemyAIController` (US-051, aggro system for Illusion)

---

## Acceptance Criteria

### Snare (Root)
- [ ] AC-023.1: `UMordecaiGA_Snare` extends `UMordecaiGA_SpellBase`. Delivery: TraceHit (single target, line trace in aim direction up to Range). On hit, applies a Root GE to the first enemy hit
- [ ] AC-023.2: Root GE applies `Mordecai.Status.Rooted` tag for a configurable Duration. While rooted: target cannot move (movement disabled) but CAN still attack and block. Tag and movement restriction removed when Duration expires
- [ ] AC-023.3: Ability tag: `Mordecai.Ability.Spell.Snare`. New tag: `Mordecai.Status.Rooted`

### Enfeeble (Weakened debuff)
- [ ] AC-023.4: `UMordecaiGA_Enfeeble` extends `UMordecaiGA_SpellBase`. Delivery: TraceHit (single target, line trace in aim direction up to Range). On hit, applies a Weakened GE to the first enemy hit
- [ ] AC-023.5: Weakened GE applies `Mordecai.Status.Weakened` tag for a configurable Duration. While weakened: target's `PhysicalDamageMultiplier` is reduced by a configurable amount (read from BasePower as percentage, e.g., BasePower=25 means −0.25). Reverts when GE expires
- [ ] AC-023.6: Ability tag: `Mordecai.Ability.Spell.Enfeeble`. New tag: `Mordecai.Status.Weakened` (may already exist from Epic 4 US-016 — reuse if present)

### Enchant Weapon (Elemental melee buff)
- [ ] AC-023.7: `UMordecaiGA_EnchantWeapon` extends `UMordecaiGA_SpellBase`. Delivery: InstantApply self-targeted. On cast, applies a duration GE that adds bonus elemental damage to melee attacks
- [ ] AC-023.8: Enchant Weapon GE applies tag `Mordecai.Status.EnchantedWeapon` for configurable Duration. While active, melee attacks deal bonus damage = BasePower (from SpellDataAsset) of the element matching the caster's highest elemental attribute. Default element: Fire (tagged `Mordecai.Damage.Fire`). TODO(DECISION): Element selection logic depends on attribute definitions not yet finalized — default to Fire for Rank 1
- [ ] AC-023.9: Ability tag: `Mordecai.Ability.Spell.EnchantWeapon`. New tag: `Mordecai.Status.EnchantedWeapon`

### Illusion (Decoy summon)
- [ ] AC-023.10: `UMordecaiGA_Illusion` extends `UMordecaiGA_SpellBase`. Delivery: SummonActor. On cast, spawns an `AMordecaiIllusionActor` at a position slightly in front of the caster
- [ ] AC-023.11: `AMordecaiIllusionActor` exists in `Mordecai/Magic/`. It has configurable HP (from BasePower) and Duration (from SpellDataAsset). Enemies within aggro range prefer targeting the Illusion over the player (implements `Mordecai.Team.Ally` tag and aggro priority). Destroyed when HP reaches 0 or Duration expires
- [ ] AC-023.12: Ability tag: `Mordecai.Ability.Spell.Illusion`

### Blur (Evasion buff)
- [ ] AC-023.13: `UMordecaiGA_Blur` extends `UMordecaiGA_SpellBase`. Delivery: InstantApply self-targeted. On cast, applies a duration GE granting an evasion buff
- [ ] AC-023.14: Blur GE applies `Mordecai.Status.Blurred` tag for configurable Duration. While active, incoming ranged projectile attacks have a configurable MissChance (from BasePower as percentage, e.g., BasePower=30 means 30% chance to miss). Melee attacks are unaffected at Rank 1. Reverts when GE expires
- [ ] AC-023.15: Ability tag: `Mordecai.Ability.Spell.Blur`. New tag: `Mordecai.Status.Blurred`

## Technical Notes
- **Snare Root mechanics:** Root disables movement but not actions. The simplest approach: apply a tag that the movement component checks. `CharacterMovementComponent` can be configured to check for `Mordecai.Status.Rooted` and zero out velocity/input. Alternatively, set `MaxWalkSpeed = 0` via GE attribute modifier on a `MoveSpeedMultiplier` attribute.
- **Enfeeble Weakened:** Straightforward attribute modifier GE. Reduces `PhysicalDamageMultiplier` additively. If US-016 (Combat Modifier Debuffs) has already been implemented, the Weakened GE/tag may already exist — reuse it and just wire the spell to apply it.
- **Enchant Weapon integration:** The melee attack ability (`UMordecaiGA_MeleeAttack`) needs to check for `Mordecai.Status.EnchantedWeapon` tag and apply bonus elemental damage on hit. This requires a small modification to the melee attack's hit processing. The bonus damage is a separate GE application (not modifying the base melee damage).
- **Illusion actor:** This is the most complex part. `AMordecaiIllusionActor` is a minimal actor with: a collision capsule, health attribute (simple float, not full ASC), team tag, and a "die on HP = 0 or timeout" behavior. Enemy AI controllers (`AMordecaiEnemyAIController`) need a small extension: when choosing targets, prefer actors with `Mordecai.Team.Ally` tag within aggro range (decoy draws aggro). This is a lightweight change to the existing AI target selection.
- **Blur miss chance:** The projectile hit processing needs to check if the target has `Mordecai.Status.Blurred` and roll against MissChance. If miss, the projectile passes through. This requires a small modification to `AMordecaiProjectile::OnHit` or the hit detection pipeline. The miss chance value needs to be stored somewhere accessible — either as an attribute or read from the active GE's magnitude.
- **Scope risk mitigation:** Snare and Enfeeble are simple "apply GE to target" spells (1-2 hours each). Enchant Weapon requires melee integration (1-2 hours). Illusion requires a new actor + AI modification (2-3 hours). Blur requires projectile pipeline modification (1-2 hours). Total: 6-11 hours. If this exceeds a session, split after Enchant Weapon.
- **Rank 1 only:** No Thorned Bind (Snare Rank 5), no Corrode (Enfeeble Rank 5), no Status Imbue (Enchant Rank 5), no Mirror Bait (Illusion Rank 5), no Afterimage (Blur Rank 5).

## Tests Required
- [ ] `Mordecai.Spell.Snare.RootsTarget` — Target receives Rooted tag and cannot move (verifies AC-023.1, AC-023.2)
- [ ] `Mordecai.Spell.Snare.RootedTargetCanStillAttack` — Rooted target can activate attack abilities (verifies AC-023.2)
- [ ] `Mordecai.Spell.Snare.RootExpiresAfterDuration` — Root removed after Duration (verifies AC-023.2)
- [ ] `Mordecai.Spell.Enfeeble.AppliesWeakened` — Target receives Weakened tag and reduced PhysicalDamageMultiplier (verifies AC-023.4, AC-023.5)
- [ ] `Mordecai.Spell.Enfeeble.WeakenedExpiresAfterDuration` — Weakened removed and multiplier reverts (verifies AC-023.5)
- [ ] `Mordecai.Spell.EnchantWeapon.AddsBonusDamageToMelee` — Melee attacks deal extra elemental damage while active (verifies AC-023.7, AC-023.8)
- [ ] `Mordecai.Spell.EnchantWeapon.BuffExpiresAfterDuration` — Enchant removed after Duration (verifies AC-023.8)
- [ ] `Mordecai.Spell.Illusion.SpawnsDecoyActor` — IllusionActor spawned near caster on cast (verifies AC-023.10)
- [ ] `Mordecai.Spell.Illusion.DecoyDrawsAggro` — Enemy AI targets decoy over player when in range (verifies AC-023.11)
- [ ] `Mordecai.Spell.Illusion.DecoyDiesOnTimeout` — Decoy destroyed after Duration (verifies AC-023.11)
- [ ] `Mordecai.Spell.Illusion.DecoyDiesOnHPDepleted` — Decoy destroyed when HP reaches 0 (verifies AC-023.11)
- [ ] `Mordecai.Spell.Blur.GrantsEvasion` — Blurred tag applied, incoming ranged attacks can miss (verifies AC-023.13, AC-023.14)
- [ ] `Mordecai.Spell.Blur.MeleeAttacksUnaffected` — Melee attacks are not affected by Blur at Rank 1 (verifies AC-023.14)
- [ ] `Mordecai.Spell.Blur.EvasionExpiresAfterDuration` — Blur removed after Duration (verifies AC-023.14)
- [ ] `Mordecai.Spell.Blur.MissChanceMatchesConfig` — Over many trials, miss rate approximates configured MissChance (verifies AC-023.14)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-023]` prefix
