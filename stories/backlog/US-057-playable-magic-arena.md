# US-057: Playable Magic Arena Integration

## Overview
Wire up spells, status effects, and the new HUD elements into the playable arena from Epic 2.5. This is the **Playable Slice 2** — after Epics 4 and 5 added status effects and magic as backend systems, this story makes them tangible. Jeff can cast spells, see SP on the HUD, fight enemies that inflict status effects, and experience the full combat + magic loop.

## Execution Mode
**Mode:** EDITOR

## References
- Design doc: `game_design_v2.md` (core combat loop with spells, SP as core resource)
- Design doc: `control_bindings_v1.1.md` (spell input bindings)
- Design doc: `skill_sheet_v1.1.md` (spell names and descriptions for DataAssets)
- Design doc: `enemy_archetypes_v1.md` (Frontliner archetype — extend with status attacks)
- Agent rules: `agent_rules_v2.md` (GAS, no camera rotation)
- Existing assets: `BP_MordecaiCharacter`, `BP_MordecaiEnemy_Frontliner`, `WBP_CombatHUD`, `DevTestMap`, `IMC_Mordecai`, `B_MordecaiDevExperience`
- Depends on: US-019 (spell framework), US-020 (Fireball, Stone Skin), US-021 (Blink), US-022 (Bless/Restoration), US-055 (spell HUD widgets), US-056 (status-applying attacks)

---

## Acceptance Criteria
- [ ] AC-057.1: `DA_Spell_Fireball` SpellDataAsset exists in `MordecaiCore/Content/Spells/` configured with: SpellPointCost=3, CooldownDuration=2.0, CastTime=0.3, WindupTime=0.2, RecoveryTime=0.3, TargetingType=Projectile, BasePower=25, Range=1500, DeliveryType=SpawnProjectile, MovementPolicy=SlowWhileCasting
- [ ] AC-057.2: `DA_Spell_Blink` SpellDataAsset exists configured with: SpellPointCost=2, CooldownDuration=4.0, CastTime=0, WindupTime=0, RecoveryTime=0.2, TargetingType=Dash, Range=500, DeliveryType=DashMovement, MovementPolicy=FreeMove
- [ ] AC-057.3: `DA_Spell_StoneSkin` SpellDataAsset exists configured with: SpellPointCost=4, CooldownDuration=15.0, CastTime=0.5, WindupTime=0.3, RecoveryTime=0.3, TargetingType=Self, BasePower=50 (shield HP), Duration=10.0, DeliveryType=InstantApply, MovementPolicy=RootWhileCasting
- [ ] AC-057.4: `DA_Spell_Restoration` SpellDataAsset exists configured with: SpellPointCost=3, CooldownDuration=10.0, CastTime=0.4, WindupTime=0.2, RecoveryTime=0.2, TargetingType=Self, BasePower=40 (total heal), Duration=5.0, DeliveryType=InstantApply, MovementPolicy=SlowWhileCasting
- [ ] AC-057.5: Spell input actions exist in `IMC_Mordecai`: `IA_Spell_1` (LB/Q), `IA_Spell_2` (RB/E), `IA_Spell_3` (LB+RB/R), `IA_Spell_4` (DPadUp/1). Bound to activate the 4 configured spell abilities
- [ ] AC-057.6: `BP_MordecaiCharacter` is configured to grant the 4 spell abilities (Fireball, Blink, Stone Skin, Restoration) on possession, mapped to the spell input bindings
- [ ] AC-057.7: `DA_EnemyAttack_FireSlash` DataAsset exists: same base stats as `DA_EnemyAttack_BasicSlash` but with `StatusEffectsOnHit` containing Burning at 40% application chance
- [ ] AC-057.8: One of the 3 arena Frontliners uses `DA_EnemyAttack_FireSlash` instead of `DA_EnemyAttack_BasicSlash` — this enemy inflicts Burning on hit
- [ ] AC-057.9: `WBP_CombatHUD` is updated with: SP bar (below posture bar), status effect indicator bar (below SP bar), and 4 spell cooldown indicators (bottom-center or side panel)
- [ ] AC-057.10: The SP bar shows current/max SP and updates in real-time when spells are cast
- [ ] AC-057.11: Status effect indicators appear when the player is Burning (from enemy fire attack) and disappear when the effect expires
- [ ] AC-057.12: The full magic loop works end-to-end: cast Fireball at enemy → see SP decrease → see cooldown timer → SP bar updates → get hit by fire enemy → see Burning indicator → cast Restoration to heal → cast Blink to reposition → cast Stone Skin for defense

## Technical Notes
- **This is an EDITOR story** — requires the Unreal Editor running with Python remote execution.
- **Leverage existing patterns:** US-054 created the arena, enemy BPs, attack DataAssets, and HUD. This story extends that work with spell-related additions.
- **Spell granting:** Same pattern as combat abilities in US-054 — configure abilities in the ASC defaults or via a GameplayEffect with `GrantedAbilities`.
- **Input bindings:** Create 4 new input actions (`IA_Spell_1` through `IA_Spell_4`) in the existing `IMC_Mordecai`. Bind to LB/Q, RB/E, LB+RB/R, DPadUp/1 respectively. Each triggers the corresponding spell ability.
- **Enemy fire attack:** Create a new DataAsset variant of the basic slash that includes `StatusEffectsOnHit`. This requires US-056 (status-applying attack extension) to be complete.
- **HUD updates:** Extend the existing `WBP_CombatHUD` Blueprint to include the new C++ widget classes from US-055. Use UMG to layout SP bar, status indicators, and cooldown displays.
- **Verification:** After setup, launch PIE and manually verify AC-057.12 (the full magic loop). This is the "Jeff can play-test magic" acceptance criterion.

## Tests Required
This story does not have headless automation tests — it is verified via manual playtesting in the editor (PIE). The C++ systems it wires together are individually tested in US-019–022, US-055, US-056.

- [ ] Manual: Cast Fireball with Q/LB, see projectile fly and damage enemy
- [ ] Manual: Cast Blink with E/RB, teleport forward with i-frames
- [ ] Manual: Cast Stone Skin with R/LB+RB, see damage reduction
- [ ] Manual: Cast Restoration with 1/DPadUp, see health recover over time
- [ ] Manual: See SP bar decrease when casting spells
- [ ] Manual: See spell cooldown timers after casting
- [ ] Manual: Get hit by fire enemy, see Burning status indicator appear
- [ ] Manual: See Burning indicator disappear when effect expires
- [ ] Manual: Die with Burning active, respawn with clean state

---

## Definition of Done
- [ ] All SpellDataAssets created and configured
- [ ] Spell input bindings created and mapped
- [ ] Player configured with 4 spell abilities
- [ ] One enemy configured with fire status attack
- [ ] HUD updated with SP bar, status indicators, cooldown displays
- [ ] Full magic + status combat loop verified in PIE
- [ ] Code/assets committed and pushed with `[US-057]` prefix
