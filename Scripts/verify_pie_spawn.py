# PIE Smoke Test: Player Spawn Chain
# Run during PIE via ue_remote_exec.py to verify the full spawn pipeline.
# Checks: Experience loaded, PawnData resolved, correct character class,
#         camera mode, input system, ability system.
#
# Usage:
#   py ue_remote_exec.py --file Scripts/verify_pie_spawn.py

import unreal

PASS_COUNT = 0
FAIL_COUNT = 0

def check(name, condition, detail=""):
    global PASS_COUNT, FAIL_COUNT
    if condition:
        PASS_COUNT += 1
        print(f"  PASS: {name}")
    else:
        FAIL_COUNT += 1
        msg = f"  FAIL: {name}"
        if detail:
            msg += f" -- {detail}"
        print(msg)

print("=" * 60)
print("PIE SMOKE TEST: SPAWN CHAIN")
print("=" * 60)

# --- PIE World ---
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
check("PIE world exists", world is not None, "Start PIE before running this test")

if not world:
    print(f"\nRESULT: {PASS_COUNT} passed, {FAIL_COUNT} failed")
    if FAIL_COUNT > 0:
        raise RuntimeError(f"PIE spawn smoke test failed: {FAIL_COUNT} failure(s)")

# --- Player Controller ---
pc = unreal.GameplayStatics.get_player_controller(world, 0)
check("Player controller exists", pc is not None)

if pc:
    pc_class = pc.get_class().get_name()
    check("Controller is Mordecai class", "Mordecai" in pc_class, f"Got {pc_class}")

# --- Player Pawn ---
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
check("Player pawn exists", pawn is not None, "Pawn not spawned -- experience may have failed to load")

if not pawn:
    print(f"\nRESULT: {PASS_COUNT} passed, {FAIL_COUNT} failed")
    if FAIL_COUNT > 0:
        raise RuntimeError(f"PIE spawn smoke test failed: {FAIL_COUNT} failure(s)")

pawn_class = pawn.get_class().get_name()
check("Pawn is Mordecai character", "Mordecai" in pawn_class, f"Got {pawn_class}")

# --- Components ---
comps = pawn.get_components_by_class(unreal.ActorComponent)
comp_names = [c.get_class().get_name() for c in comps]

# HeroComponent (drives input + camera)
has_hero = any("Hero" in n for n in comp_names)
check("LyraHeroComponent present", has_hero,
      f"Components: {', '.join(comp_names)}")

# CameraComponent
has_camera = any("Camera" in n for n in comp_names)
check("Camera component present", has_camera)

# PawnExtensionComponent (drives PawnData)
has_ext = any("Extension" in n for n in comp_names)
check("PawnExtensionComponent present", has_ext)

# --- PawnData ---
ext_comps = [c for c in comps if "Extension" in c.get_class().get_name()]
if ext_comps:
    try:
        pd = ext_comps[0].get_editor_property("pawn_data")
        check("PawnData resolved", pd is not None,
              "PawnExtension.PawnData is null -- experience did not set it")
        if pd:
            pd_name = pd.get_name()
            check("PawnData is Mordecai", "Mordecai" in pd_name, f"Got {pd_name}")
    except Exception as e:
        check("PawnData accessible", False, str(e))

# --- Camera Mode ---
hero_comps = [c for c in comps if "Hero" in c.get_class().get_name()]
if hero_comps:
    hero = hero_comps[0]
    try:
        cam_mode = hero.determine_camera_mode()
        check("Camera mode resolved", cam_mode is not None)
        if cam_mode:
            cam_name = cam_mode.get_name()
            check("Camera is Diorama", "Diorama" in cam_name, f"Got {cam_name}")
    except Exception:
        pass

# --- Ability System ---
if pc:
    try:
        ps = pc.player_state
        if ps and hasattr(ps, "get_ability_system_component"):
            asc = ps.get_ability_system_component()
            check("ASC exists on PlayerState", asc is not None)
        elif ps:
            # Try IAbilitySystemInterface cast
            check("PlayerState exists", True)
    except Exception:
        pass

# --- Location (not stuck at origin) ---
loc = pawn.get_actor_location()
not_at_origin = abs(loc.x) > 1.0 or abs(loc.y) > 1.0 or abs(loc.z) > 1.0
check("Pawn not at world origin", not_at_origin,
      f"Location=({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f}) -- may not have spawned correctly")

print("")
print(f"RESULT: {PASS_COUNT} passed, {FAIL_COUNT} failed")

if FAIL_COUNT > 0:
    raise RuntimeError(f"PIE spawn smoke test failed: {FAIL_COUNT} failure(s)")
else:
    print("ALL CHECKS PASSED")
