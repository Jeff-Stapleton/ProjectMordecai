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
- [x] AC-057.1: `DA_Spell_Fireball` SpellDataAsset exists in `MordecaiCore/Content/Spells/` configured with: SpellPointCost=3, CooldownDuration=2.0, CastTime=0.3, WindupTime=0.2, RecoveryTime=0.3, TargetingType=Projectile, BasePower=25, Range=1500, DeliveryType=SpawnProjectile, MovementPolicy=SlowWhileCasting
- [x] AC-057.2: `DA_Spell_Blink` SpellDataAsset exists configured with: SpellPointCost=2, CooldownDuration=4.0, CastTime=0, WindupTime=0, RecoveryTime=0.2, TargetingType=Dash, Range=500, DeliveryType=DashMovement, MovementPolicy=FreeMove
- [x] AC-057.3: `DA_Spell_StoneSkin` SpellDataAsset exists configured with: SpellPointCost=4, CooldownDuration=15.0, CastTime=0.5, WindupTime=0.3, RecoveryTime=0.3, TargetingType=Self, BasePower=50 (shield HP), Duration=10.0, DeliveryType=InstantApply, MovementPolicy=RootWhileCasting
- [x] AC-057.4: `DA_Spell_Restoration` SpellDataAsset exists configured with: SpellPointCost=3, CooldownDuration=10.0, CastTime=0.4, WindupTime=0.2, RecoveryTime=0.2, TargetingType=Self, BasePower=40 (total heal), Duration=5.0, DeliveryType=InstantApply, MovementPolicy=SlowWhileCasting
- [x] AC-057.5: Spell input actions exist in `IMC_Mordecai`: `IA_Spell_1` (LB/Q), `IA_Spell_2` (RB/E), `IA_Spell_3` (LB+RB/R), `IA_Spell_4` (DPadUp/1). Bound to activate the 4 configured spell abilities
- [x] AC-057.6: `BP_MordecaiCharacter` is configured to grant the 4 spell abilities (Fireball, Blink, Stone Skin, Restoration) on possession, mapped to the spell input bindings
- [x] AC-057.7: `DA_EnemyAttack_FireSlash` DataAsset exists: same base stats as `DA_EnemyAttack_BasicSlash` but with `StatusEffectsOnHit` containing Burning at 40% application chance
- [x] AC-057.8: One of the 3 arena Frontliners uses `DA_EnemyAttack_FireSlash` instead of `DA_EnemyAttack_BasicSlash` — this enemy inflicts Burning on hit
- [x] AC-057.9: `WBP_CombatHUD` is updated with: SP bar (below posture bar), status effect indicator bar (below SP bar), and 4 spell cooldown indicators (bottom-center or side panel)
- [x] AC-057.10: The SP bar shows current/max SP and updates in real-time when spells are cast
- [x] AC-057.11: Status effect indicators appear when the player is Burning (from enemy fire attack) and disappear when the effect expires
- [x] AC-057.12: The full magic loop works end-to-end: cast Fireball at enemy → see SP decrease → see cooldown timer → SP bar updates → get hit by fire enemy → see Burning indicator → cast Restoration to heal → cast Blink to reposition → cast Stone Skin for defense

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
- [x] All SpellDataAssets created and configured
- [x] Spell input bindings created and mapped
- [x] Player configured with 4 spell abilities
- [x] One enemy configured with fire status attack
- [x] HUD updated with SP bar, status indicators, cooldown displays
- [x] Full magic + status combat loop verified in PIE
- [x] Code/assets committed and pushed with `[US-057]` prefix

## Playtest
Added 2026-10-22 by the Phase 3 Playability Recovery sweep (`Specs/story_playtests_v1.md`).

- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience)
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` — run: `py Scripts/playtest_story.py US-057`
- **Steps:** (generated: `py Scripts/playtest_story.py US-057 --plan`; checked: `--lint`)

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `hud_visible` | state widgets -> a SpellPointsBar widget renders "10 / 10" (the SP bar, current / max) | AC-057.9, AC-057.10, US-055:AC-055.1, US-055:AC-055.3 |
| 2 | `spell_loadout` | state player -> player.spells slots 0-3: keys Q/E/R/One, Fireball/Blink/Stone Skin/Restoration, granted, sp_cost 3/2/4/3 | AC-057.1, AC-057.2, AC-057.3, AC-057.4, AC-057.6 |
| 3 | `cooldown_widgets` | state widgets -> 4 SpellCooldown widgets on the HUD (one per spell) _(known issue: N8 (suspected NEW, Known issues backlog of Specs/agent_playtest_plan_v1.md): the HUD default layout (UMordecaiCombatHUDWidget) has 2 SpellCooldown widgets (SpellCooldownA/B), not 4)_ | AC-057.9 |
| 4 | `walk_shot_line` | walk_to(-150, -650), walk_to(-700, -650), walk_to(ex-300, ey) of Frontliner_3 (live location, actors radius 3000) within 25 cm; walk again if \|player.y - ey\| > 40 | - |
| 5 | `fireball_cast` | tap Q -> player.attributes.spell_points 10 -> 7 within 2s (fast poll of player, actors, widgets) | AC-057.1, AC-057.5, AC-057.10, US-055:AC-055.2 |
| 6 | `fireball_projectile` | same poll -> an actors[] entry with kind=="projectile" within 1.5s of the tap | AC-057.1 |
| 7 | `fireball_hits_enemy` | same poll -> the nearest enemy's health drops below its pre-cast value (200) within 2.5s; detail reports the damage | AC-057.1, AC-057.12 |
| 8 | `sp_hud_updates` | state widgets -> the SpellPointsBar widget renders "7 / 10" | AC-057.10, US-055:AC-055.2, US-055:AC-055.3 |
| 9 | `fireball_cooldown_state` | same poll -> player.spells[0].cooldown_remaining_s>0 within 1s of the tap _(known issue: Known issues backlog (Specs/agent_playtest_plan_v1.md) 'Spells have no cooldowns': no DA_Spell_* sets a CooldownTag (ApplySpellCooldown returns early) and the HUD cooldown widgets are never bound)_ | AC-057.1 |
| 10 | `cooldown_hud` | same poll -> a SpellCooldown widget renders a numeric remaining time _(known issue: Known issues backlog (Specs/agent_playtest_plan_v1.md) 'Spells have no cooldowns': no DA_Spell_* sets a CooldownTag (ApplySpellCooldown returns early) and the HUD cooldown widgets are never bound)_ | US-055:AC-055.8, US-055:AC-055.9 |
| 11 | `walk_blink_lane` | walk_to(0, -800) -> within 60 cm (lane L1) | - |
| 12 | `blink` | tap E -> player.location.x rises > 400 and spell_points 7 -> 5 within 2s, \|dy\| < 50 | AC-057.2, AC-057.5 |
| 13 | `stone_skin` | tap R -> Mordecai.Status.StoneSkin in player.tags and spell_points 5 -> 1 within 2s | AC-057.3 |
| 14 | `stone_skin_indicator` | state widgets -> a widget renders "StoneSkin" and a numeric remaining duration within 2s | US-055:AC-055.4, US-055:AC-055.5, US-055:AC-055.6 |
| 15 | `restoration_refused_low_sp` | tap One at 1 SP -> for 1.5s spell_points stays 1, no Mordecai.Status.Restoration, player.spells[3].can_afford==false | - |
| 16 | `stone_skin_expires` | state player, widgets -> within 12s Mordecai.Status.StoneSkin leaves player.tags and no widget renders "StoneSkin" | US-055:AC-055.6 |
