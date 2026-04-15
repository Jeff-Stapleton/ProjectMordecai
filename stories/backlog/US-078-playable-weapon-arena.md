# US-078: Playable Weapon Arena Integration

## Overview
Wire the weapon system into a playable experience. Create DataAssets for a representative set of melee weapons, add weapon cycling input, update the combat HUD with a weapon indicator, and configure the arena so Jeff can launch the game, fight enemies with different weapons, swap between them with Tab/Y, and feel the difference between a fast Dagger and a heavy Greatsword.

This is the integration milestone for Epic 6 (per Playability-First Rule). After this story, the weapon system is play-testable.

## Execution Mode
**Mode:** EDITOR

## References
- Design doc: `control_bindings_v1.1.md` (Weapon Set Swap: Tab / Y hold — simplified to cycling)
- Design doc: `game_design_v2.md` (diorama camera, twin-stick controls, ARPG feel)
- Design doc: `attack_taxonomy_v1.md` (weapon class profiles — sweep/thrust/slam shapes)
- Design doc: `combat_system_v1.md` (damage formula with weapon BaseDamage)
- Existing code: `UMordecaiWeaponProfileFactory` (US-025/026 — factory methods for all melee weapons)
- Existing code: `UMordecaiEquipmentComponent` (US-024 + US-077 — equip/unequip + cycling)
- Existing code: `UMordecaiEquippedWeaponWidget` (US-077 — weapon display widget)
- Existing assets: `BP_MordecaiCharacter`, `DevTestMap`, `IMC_Mordecai`, `WBP_CombatHUD`, existing attack profile DataAssets from US-054
- Agent rules: `agent_rules_v2.md` — GAS, data-driven

---

## Acceptance Criteria

### Weapon DataAssets (5 representative weapons from blade + blunt/polearm families)
- [ ] AC-078.1: `DA_Weapon_Longsword` DataAsset exists in `MordecaiCore/Content/Weapons/` with values matching the factory's `CreateLongsword()` output (US-025). Includes 3 light attack profile DataAssets and 1 heavy attack profile DataAsset.
- [ ] AC-078.2: `DA_Weapon_Dagger` DataAsset exists with values matching `CreateDagger()`. 5 light + 1 heavy attack profile DataAssets. All Pierce damage.
- [ ] AC-078.3: `DA_Weapon_Greatsword` DataAsset exists with values matching `CreateGreatsword()`. TwoHand. 2 light + 1 heavy (slam) attack profile DataAssets.
- [ ] AC-078.4: `DA_Weapon_Spear` DataAsset exists with values matching `CreateSpear()`. TwoHand. 2 thrust + 1 sweep finisher light + 1 lunge heavy. All Pierce.
- [ ] AC-078.5: `DA_Weapon_Mace` DataAsset exists with values matching `CreateMace()`. All Blunt. L1 sweep + L2 slam lights + 1 slam heavy. Highest PostureDamageBonus.

### Weapon Cycling Input
- [ ] AC-078.6: `IA_WeaponCycleNext` input action exists. Bound to Tab (MKB) and Y tap (Gamepad). Triggers `CycleNextWeapon()` on the player's EquipmentComponent.
- [ ] AC-078.7: `IA_WeaponCyclePrev` input action exists. Bound to Shift+Tab (MKB) only. Triggers `CyclePrevWeapon()`.
- [ ] AC-078.8: Both input actions are added to the `IMC_Mordecai` input mapping context.

### Player Setup
- [ ] AC-078.9: On game start (pawn possession), the player's EquipmentComponent is populated with all 5 weapon DataAssets via `AddAvailableWeapon()`. The Longsword is auto-equipped as the starting weapon.
- [ ] AC-078.10: The player's `BP_MordecaiCharacter` has a `UMordecaiEquipmentComponent` attached (verify it's configured from US-024 or add if missing).

### HUD Integration
- [ ] AC-078.11: `WBP_CombatHUD` is updated to include the weapon display widget (`UMordecaiEquippedWeaponWidget`) in the HUD layout. Position: bottom-center or bottom-right of screen.
- [ ] AC-078.12: The weapon widget is bound to the player's EquipmentComponent on HUD creation. Shows the equipped weapon name. Updates when weapon is cycled.

### Heavy Attack Input
- [ ] AC-078.13: Heavy attack is triggered by holding RT/LMB (per control_bindings_v1.1). If heavy attack input is not already wired from US-054, add a `IA_HeavyAttack` input action bound to RT hold / LMB hold. The melee ability uses the weapon's `HeavyAttackProfile` for heavy input. `TODO(DECISION)` if the existing melee ability does not support heavy input routing — note what's needed but do not block the story.

### End-to-End Verification
- [ ] AC-078.14: Full weapon loop works in PIE: launch game → start with Longsword → attack enemies → press Tab → weapon changes to Dagger → attack pattern visibly different (faster, shorter range, thrust shape) → cycle through all 5 weapons → each feels distinct → HUD shows current weapon name
- [ ] AC-078.15: Equipping TwoHand weapons (Greatsword, Spear) works correctly — both hand slots cleared, attacks use the two-hand weapon's profiles
- [ ] AC-078.16: Damage differences are visible — Greatsword deals noticeably more damage per hit than Dagger, Mace deals high posture damage, Spear has longest reach

## Technical Notes
- **This is an EDITOR story** — requires the Unreal Editor running with Python remote execution.
- **Creating weapon DataAssets:** Use Python editor scripting to create `UMordecaiWeaponDataAsset` and `UMordecaiAttackProfileDataAsset` instances. Set all UPROPERTY values to match the factory output. Alternatively, create them in C++ test setup and export, but Python creation is preferred for EDITOR stories.
- **Attack profile DataAssets per weapon:** Each weapon's light chain steps and heavy attack need separate DataAsset instances. For Longsword that's 4 DataAssets (3 light + 1 heavy). Total: ~17 attack profile DataAssets across 5 weapons.
- **Input binding:** Extend `IMC_Mordecai` with the new weapon cycling actions. The input action should call a Blueprint-exposed function on the EquipmentComponent.
- **Weapon population on spawn:** Configure via the experience setup, player controller, or pawn Blueprint. The simplest approach: in `BP_MordecaiCharacter`'s BeginPlay, call AddAvailableWeapon for each weapon DataAsset (set via UPROPERTY references in the BP).
- **Heavy attacks:** The existing melee ability (US-004) handles light combo chains. Heavy attack support depends on whether the ability routes InputSlot::Heavy to the HeavyAttackProfile. If this isn't implemented yet, note it as a gap for a follow-up story but don't let it block weapon cycling + light attack verification.
- **Replacing US-054's hardcoded profiles:** The original arena (US-054) created `DA_PlayerAttack_LightSlash_1/2/3` DataAssets. These can coexist alongside the new weapon DataAssets — the EquipmentComponent's weapon profiles take priority over hardcoded profiles per US-024.

## Tests Required
This story is verified via manual playtesting in PIE. The C++ systems are tested in US-024, US-025, US-026, and US-077.

- [ ] Manual: Launch PIE, verify player starts with Longsword equipped
- [ ] Manual: Attack enemies with LMB/RT — see Longsword's 3-sweep combo
- [ ] Manual: Press Tab — weapon cycles to next, HUD updates
- [ ] Manual: Cycle through all 5 weapons, each attack feels different:
  - Longsword: Medium speed sweeps, balanced
  - Dagger: Very fast thrusts, short range
  - Greatsword: Slow wide sweeps, high damage
  - Spear: Long-range thrusts, medium speed
  - Mace: Slow, high posture damage, mixed sweeps/slams
- [ ] Manual: Kill an enemy with each weapon type to verify damage differences
- [ ] Manual: Shift+Tab cycles backwards through weapons
- [ ] Manual: HUD weapon display shows correct weapon name at all times

---

## Definition of Done
- [ ] All weapon DataAssets created in editor
- [ ] Weapon cycling input wired and functional
- [ ] HUD shows equipped weapon name
- [ ] All 5 weapons feel distinct in combat
- [ ] PIE smoke test passing
- [ ] Code/assets committed and pushed with `[US-078]` prefix
