# US-077: Weapon Cycling & Equipped Weapon Display

## Overview
Add weapon cycling support to the EquipmentComponent and a minimal HUD widget showing the currently equipped weapon. This is the C++ plumbing needed before the Playable Weapon Arena (US-078) can wire up weapon swapping in-game. The player should be able to cycle through a list of available weapons, with the HUD updating to show what's equipped.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `control_bindings_v1.1.md` (Weapon Set Swap: Tab / Y hold — simplified to cycling for this slice)
- Design doc: `game_design_v2.md` (diorama ARPG, twin-stick controls)
- Existing code: `UMordecaiEquipmentComponent` (US-024), `UMordecaiWeaponDataAsset` (US-024), `FMordecaiWeaponInstance` (US-024)
- Existing code: `UMordecaiCombatHUDWidget` (US-052) — parent HUD to add weapon display to
- Agent rules: `agent_rules_v2.md` — GAS, data-driven

---

## Acceptance Criteria

### Weapon Cycling on EquipmentComponent
- [ ] AC-077.1: `UMordecaiEquipmentComponent` gains `AvailableWeapons` (TArray<FMordecaiWeaponInstance>, UPROPERTY, BlueprintReadOnly). This is the list of weapons the player can cycle through. Populated by game logic (Blueprint or experience setup).
- [ ] AC-077.2: `AddAvailableWeapon(const UMordecaiWeaponDataAsset* WeaponAsset)` creates an `FMordecaiWeaponInstance` from the asset and appends it to `AvailableWeapons`. Returns the index. Does not auto-equip.
- [ ] AC-077.3: `CycleNextWeapon()` equips the next weapon in `AvailableWeapons` relative to the currently equipped weapon. Wraps from last to first. If no weapon is currently equipped, equips index 0. If `AvailableWeapons` is empty, does nothing.
- [ ] AC-077.4: `CyclePrevWeapon()` equips the previous weapon in `AvailableWeapons`. Wraps from first to last. Same empty/no-equip handling as CycleNextWeapon.
- [ ] AC-077.5: `GetCurrentWeaponIndex()` returns the index in `AvailableWeapons` of the currently equipped MainHand weapon, or -1 if none equipped or not in the list.
- [ ] AC-077.6: `GetAvailableWeaponCount()` returns the number of weapons in the available list.

### Equipped Weapon Widget
- [ ] AC-077.7: `UMordecaiEquippedWeaponWidget` (UUserWidget subclass) exists in `Mordecai/UI/`. Displays the equipped weapon's `DisplayName` and `WeaponType` (as text). Shows "Unarmed" when no weapon is equipped.
- [ ] AC-077.8: The widget exposes `BindToEquipmentComponent(UMordecaiEquipmentComponent* Comp)` to connect to a player's equipment state. Updates automatically when the equipped weapon changes.
- [ ] AC-077.9: The widget exposes `GetDisplayedWeaponName()` const and `GetDisplayedWeaponType()` const for test verification.

### Delegate for Weapon Change
- [ ] AC-077.10: `UMordecaiEquipmentComponent` broadcasts an `OnWeaponChanged` delegate (FOnWeaponChanged, multicast) whenever `EquipWeapon` or `UnequipWeapon` is called. Payload: `EMordecaiEquipSlot Slot, const UMordecaiWeaponDataAsset* NewWeapon` (NewWeapon is nullptr on unequip). The widget binds to this delegate.

## Technical Notes
- **File locations:**
  - Extend `Source/LyraGame/Mordecai/Weapons/MordecaiEquipmentComponent.h/.cpp`
  - New files: `Source/LyraGame/Mordecai/UI/MordecaiEquippedWeaponWidget.h/.cpp`
  - New test file: `Source/LyraGame/Tests/Weapons/MordecaiWeaponCyclingTests.cpp`
- **Cycling logic:** Track `CurrentWeaponIndex` internally. CycleNext increments mod count. CyclePrev decrements (with wrap). Each calls `EquipWeapon()` internally with the `MainHand` slot.
- **TwoHand weapons in cycle:** When cycling to a TwoHand weapon, the existing EquipWeapon logic already handles clearing both slots. When cycling away from TwoHand, the next weapon equips to MainHand normally.
- **Widget pattern:** Follow the same pattern as `UMordecaiHealthBarWidget` etc. — UUserWidget with bound text blocks, `BindTo*` method for connecting to data source.
- **Do NOT wire into HUD Blueprint or input.** That's US-078 (EDITOR). This story creates the C++ classes and tests only.
- **The full "Weapon Set Swap" (Set 1 vs Set 2 loadouts) from control_bindings_v1.1 is deferred.** This slice implements simple linear cycling, which is sufficient for play-testing.

## Tests Required
- [ ] `Mordecai.Weapon.Cycling.NextCyclesToNextWeapon` — Add 3 weapons, equip first, CycleNext → second is equipped (AC-077.3)
- [ ] `Mordecai.Weapon.Cycling.NextWrapsAround` — On last weapon, CycleNext → wraps to first (AC-077.3)
- [ ] `Mordecai.Weapon.Cycling.NextFromNoWeaponEquipsFirst` — No weapon equipped, CycleNext → equips index 0 (AC-077.3)
- [ ] `Mordecai.Weapon.Cycling.NextWithEmptyListDoesNothing` — Empty AvailableWeapons, CycleNext → no crash, nothing equipped (AC-077.3)
- [ ] `Mordecai.Weapon.Cycling.PrevCyclesToPrevWeapon` — Add 3 weapons, equip second, CyclePrev → first is equipped (AC-077.4)
- [ ] `Mordecai.Weapon.Cycling.PrevWrapsAround` — On first weapon, CyclePrev → wraps to last (AC-077.4)
- [ ] `Mordecai.Weapon.Cycling.GetCurrentWeaponIndex` — Returns correct index for equipped weapon, -1 when none (AC-077.5)
- [ ] `Mordecai.Weapon.Cycling.AddAvailableWeapon` — AddAvailableWeapon creates instance and appends; count increments (AC-077.2, AC-077.6)
- [ ] `Mordecai.Weapon.Cycling.OnWeaponChangedFires` — EquipWeapon fires OnWeaponChanged with correct slot and weapon; UnequipWeapon fires with nullptr (AC-077.10)
- [ ] `Mordecai.Weapon.Widget.ShowsEquippedWeaponName` — Widget bound to equipment with Longsword equipped → GetDisplayedWeaponName() returns "Longsword" (AC-077.7, AC-077.9)
- [ ] `Mordecai.Weapon.Widget.ShowsUnarmedWhenNoWeapon` — Widget bound to equipment with nothing equipped → shows "Unarmed" (AC-077.7)
- [ ] `Mordecai.Weapon.Widget.UpdatesOnWeaponChange` — Widget bound, cycle weapon → displayed name updates to new weapon (AC-077.8)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-077]` prefix
