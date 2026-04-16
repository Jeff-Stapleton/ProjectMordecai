"""US-078 PIE Smoke Test: Verify weapon system end-to-end in PIE.

Checks:
- PIE world exists
- Player pawn has EquipmentComponent
- EquipmentComponent has 5 AvailableWeapons after BeginPlay
- A weapon is auto-equipped (GetCurrentWeaponIndex >= 0)
- Cycling changes GetCurrentWeaponIndex

Run during PIE. Fails with RuntimeError if any invariant is broken.
"""
import unreal

# --- Find PIE world ---
editor_subsys = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
pie_world = editor_subsys.get_game_world()
if pie_world is None:
    raise RuntimeError('No PIE game world — is PIE running?')

print(f"[PIE] Game world: {pie_world.get_name()}")

# --- Find all actors of class MordecaiCharacter ---
all_actors = unreal.GameplayStatics.get_all_actors_of_class(pie_world, unreal.MordecaiCharacter)
if len(all_actors) == 0:
    raise RuntimeError('FAIL: No MordecaiCharacter in PIE world')

# Pick the one that is locally controlled
pawn = None
for a in all_actors:
    if a.is_locally_controlled():
        pawn = a
        break
if pawn is None:
    pawn = all_actors[0]  # Fallback

print(f"[PIE] Pawn: {pawn.get_name()} class={pawn.get_class().get_name()}")

# --- Find EquipmentComponent ---
equip_comp = pawn.get_component_by_class(unreal.MordecaiEquipmentComponent)
if equip_comp is None:
    raise RuntimeError('FAIL: Pawn has no UMordecaiEquipmentComponent')
print(f"[PIE] PASS: EquipmentComponent found on pawn")

# --- Check AvailableWeapons count ---
count = equip_comp.get_available_weapon_count()
if count != 5:
    raise RuntimeError(f'FAIL: Expected 5 weapons, got {count}')
print(f"[PIE] PASS: {count} weapons available")

# --- Check auto-equip ---
idx = equip_comp.get_current_weapon_index()
if idx < 0:
    raise RuntimeError(f'FAIL: No weapon auto-equipped (index={idx})')
print(f"[PIE] PASS: Weapon auto-equipped at index {idx}")

# --- Cycle forward and verify change ---
prev_idx = idx
equip_comp.cycle_next_weapon()
new_idx = equip_comp.get_current_weapon_index()
if new_idx == prev_idx:
    raise RuntimeError(f'FAIL: CycleNextWeapon did not change index (still {new_idx})')
print(f"[PIE] PASS: CycleNext moved {prev_idx} -> {new_idx}")

# --- Cycle backward and verify ---
equip_comp.cycle_prev_weapon()
back_idx = equip_comp.get_current_weapon_index()
if back_idx != prev_idx:
    raise RuntimeError(f'FAIL: CyclePrev expected {prev_idx}, got {back_idx}')
print(f"[PIE] PASS: CyclePrev returned to {back_idx}")

# --- Final success message ---
print("[PIE] ========================================")
print("[PIE] US-078 weapon system verification: ALL PASS")
print("[PIE] ========================================")
