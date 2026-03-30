# US-023: Tactical Spells — Debuff Spells (Snare, Enfeeble, Enchant Weapon)

## Overview
Implement three tactical debuff/buff spells using the Spell Framework from US-019. Snare (single-target root), Enfeeble (single-target Weakened debuff), and Enchant Weapon (self-buff adding elemental damage to melee). These are simple "apply GE to target/self" spells that follow the established pattern.

Split from original US-023 scope. Illusion and Blur moved to US-060 (more complex mechanics).

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `ability_schema_v1.md` (delivery types: TraceHit, InstantApply; targeting types)
- Design doc: `new_spells_proposal.md` (Snare: Rank 1 root single target; Enfeeble: Rank 1 apply Weakened; Enchant Weapon: Rank 1 add elemental damage)
- Design doc: `status_effects_v1.md` (Weakened, Rooted status effect definitions — from Epic 4)
- Agent rules: `agent_rules_v2.md` (GAS for abilities, fixed diorama camera)
- Existing code: `UMordecaiGA_SpellBase`, `UMordecaiSpellDataAsset` (US-019), `UMordecaiHitDetectionSubsystem` (US-003, line trace)

---

## Acceptance Criteria

### Snare (Root)
- [ ] AC-023.1: `UMordecaiGA_Snare` extends `UMordecaiGA_SpellBase`. Delivery: TraceHit (single target, line trace in aim direction up to Range). On hit, applies a Root GE to the first enemy hit
- [ ] AC-023.2: Root GE applies `Mordecai.Status.Rooted` tag for a configurable Duration. While rooted: target cannot move (movement disabled) but CAN still attack and block. Tag and movement restriction removed when Duration expires
- [ ] AC-023.3: Ability tag: `Mordecai.Ability.Spell.Snare`. Reuses `Mordecai.Status.Rooted` tag from US-017

### Enfeeble (Weakened debuff)
- [ ] AC-023.4: `UMordecaiGA_Enfeeble` extends `UMordecaiGA_SpellBase`. Delivery: TraceHit (single target, line trace in aim direction up to Range). On hit, applies a Weakened GE to the first enemy hit
- [ ] AC-023.5: Weakened GE applies `Mordecai.Status.Weakened` tag for a configurable Duration. While weakened: target's `PhysicalDamageMultiplier` is reduced by a configurable amount (read from BasePower as percentage, e.g., BasePower=25 means −0.25). Reverts when GE expires
- [ ] AC-023.6: Ability tag: `Mordecai.Ability.Spell.Enfeeble`. Reuses `Mordecai.Status.Weakened` tag from US-016

### Enchant Weapon (Elemental melee buff)
- [ ] AC-023.7: `UMordecaiGA_EnchantWeapon` extends `UMordecaiGA_SpellBase`. Delivery: InstantApply self-targeted. On cast, applies a duration GE that adds bonus elemental damage to melee attacks
- [ ] AC-023.8: Enchant Weapon GE applies tag `Mordecai.Status.EnchantedWeapon` for configurable Duration. While active, melee attacks deal bonus damage = BasePower (from SpellDataAsset) of Fire element (tagged `Mordecai.Damage.Fire`). TODO(DECISION): Element selection logic depends on attribute definitions not yet finalized — default to Fire for Rank 1
- [ ] AC-023.9: Ability tag: `Mordecai.Ability.Spell.EnchantWeapon`. New tag: `Mordecai.Status.EnchantedWeapon`

## Technical Notes
- **Snare Root mechanics:** Reuses the Rooted status from US-017. The spell simply applies the same GE/tag that the Rooted status effect uses. If US-017 is implemented first, reference its GE directly. If not, create a compatible GE that applies the same tag.
- **Enfeeble Weakened:** Reuses the Weakened status from US-016. Same pattern — spell applies the existing GE.
- **Enchant Weapon integration:** The melee attack ability (`UMordecaiGA_MeleeAttack`) needs to check for `Mordecai.Status.EnchantedWeapon` tag and apply bonus elemental damage on hit. This requires a small modification to the melee attack's hit processing. The bonus damage is a separate GE application (not modifying the base melee damage).
- **Rank 1 only:** No Thorned Bind (Snare Rank 5), no Corrode (Enfeeble Rank 5), no Status Imbue (Enchant Rank 5).

## Tests Required
- [ ] `Mordecai.Spell.Snare.RootsTarget` — Target receives Rooted tag and cannot move (verifies AC-023.1, AC-023.2)
- [ ] `Mordecai.Spell.Snare.RootedTargetCanStillAttack` — Rooted target can activate attack abilities (verifies AC-023.2)
- [ ] `Mordecai.Spell.Snare.RootExpiresAfterDuration` — Root removed after Duration (verifies AC-023.2)
- [ ] `Mordecai.Spell.Enfeeble.AppliesWeakened` — Target receives Weakened tag and reduced PhysicalDamageMultiplier (verifies AC-023.4, AC-023.5)
- [ ] `Mordecai.Spell.Enfeeble.WeakenedExpiresAfterDuration` — Weakened removed and multiplier reverts (verifies AC-023.5)
- [ ] `Mordecai.Spell.EnchantWeapon.AddsBonusDamageToMelee` — Melee attacks deal extra elemental damage while active (verifies AC-023.7, AC-023.8)
- [ ] `Mordecai.Spell.EnchantWeapon.BuffExpiresAfterDuration` — Enchant removed after Duration (verifies AC-023.8)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-023]` prefix
