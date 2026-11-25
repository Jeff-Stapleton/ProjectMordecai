# PIE Smoke Test: Player Spawn Chain
# Run during PIE via ue_remote_exec.py to verify the full spawn pipeline.
# Checks: Experience loaded, PawnData resolved, correct character class,
#         camera mode, input system, ability system.
#
# Usage:
#   py ue_remote_exec.py --file Scripts/verify_pie_spawn.py
#
# NOTE: All PIE references are held inside _run() so they go out of scope
# when the function returns, avoiding GC reference leaks after PIE ends.

import unreal

def _run():
    pass_count = 0
    fail_count = 0

    def check(name, condition, detail=""):
        nonlocal pass_count, fail_count
        if condition:
            pass_count += 1
            print(f"  PASS: {name}")
        else:
            fail_count += 1
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
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE spawn smoke test failed: {fail_count} failure(s)")
        return

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
        print(f"\nRESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE spawn smoke test failed: {fail_count} failure(s)")
        return

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

    # --- PlayerState ---
    if pc:
        try:
            ps = pc.player_state
            check("PlayerState exists", ps is not None)
        except Exception:
            pass

    # --- Location (not stuck at origin) ---
    loc = pawn.get_actor_location()
    not_at_origin = abs(loc.x) > 1.0 or abs(loc.y) > 1.0 or abs(loc.z) > 1.0
    check("Pawn not at world origin", not_at_origin,
          f"Location=({loc.x:.0f}, {loc.y:.0f}, {loc.z:.0f}) -- may not have spawned correctly")

    # --- US-099 wall-fade content contract (DevTestMap only) ---
    if "DevTestMap" in world.get_path_name():
        fade_tag = "Mordecai.OcclusionFade"
        fade_material = "M_OcclusionFadeWall"
        fade_parameter = "MordecaiOcclusionFade"
        fade_wall_labels = {
            "Arena_Wall_N", "Arena_Wall_S", "Arena_Wall_E", "Arena_Wall_W",
            "Wall_01", "Wall_02",
        }
        static_actors = unreal.GameplayStatics.get_all_actors_of_class(
            world, unreal.StaticMeshActor)
        walls = [actor for actor in static_actors
                 if actor.get_actor_label() in fade_wall_labels]
        check("US-099 has six fade-enabled DevTestMap walls",
              {actor.get_actor_label() for actor in walls} == fade_wall_labels,
              f"Found {[actor.get_actor_label() for actor in walls]}")
        for wall in walls:
            mesh = wall.static_mesh_component
            tags = [str(tag) for tag in mesh.get_editor_property("component_tags")]
            material = mesh.get_material(0)
            check(f"US-099 {wall.get_actor_label()} tagged for occlusion fade",
                  fade_tag in tags, f"Tags={tags}")
            check(f"US-099 {wall.get_actor_label()} uses masked fade material",
                  material is not None and fade_material in material.get_name(),
                  f"Material={material}")
            base_material = material.get_base_material() if material is not None else None
            blend_mode = (
                base_material.get_editor_property("blend_mode")
                if base_material is not None else None
            )
            clip_value = (
                float(base_material.get_editor_property("opacity_mask_clip_value"))
                if base_material is not None else None
            )
            check(f"US-099 {wall.get_actor_label()} base material is BLEND_MASKED",
                  blend_mode == unreal.BlendMode.BLEND_MASKED,
                  f"Base={base_material}, blend={blend_mode}")
            check(f"US-099 {wall.get_actor_label()} opacity-mask clip is 0.333",
                  clip_value is not None and abs(clip_value - 0.333) <= 0.001,
                  f"Base={base_material}, clip={clip_value}")
            parameter_names = (
                [str(name) for name in
                 unreal.MaterialEditingLibrary.get_scalar_parameter_names(material)]
                if material is not None else []
            )
            check(f"US-099 {wall.get_actor_label()} material exposes fade scalar",
                  fade_parameter in parameter_names,
                  f"Scalar parameters={parameter_names}")
            if material is not None and fade_parameter in parameter_names:
                fade_value = unreal.MaterialEditingLibrary.get_material_default_scalar_parameter_value(
                    material, fade_parameter)
                check(f"US-099 {wall.get_actor_label()} fade scalar starts opaque",
                      abs(float(fade_value) - 1.0) <= 0.001,
                      f"{fade_parameter}={fade_value}")
        floors = [actor for actor in static_actors
                  if actor.get_actor_label() == "Arena_Floor"]
        floor_tags = [str(tag) for floor in floors
                      for tag in floor.static_mesh_component.get_editor_property("component_tags")]
        check("US-099 arena floor is never fade-enabled",
              bool(floors) and fade_tag not in floor_tags, f"Floor tags={floor_tags}")

    print("")
    print(f"RESULT: {pass_count} passed, {fail_count} failed")

    if fail_count > 0:
        raise RuntimeError(f"PIE spawn smoke test failed: {fail_count} failure(s)")
    else:
        print("ALL CHECKS PASSED")

_run()
del _run
