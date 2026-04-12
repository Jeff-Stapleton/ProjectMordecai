# Fix the full PIE spawn chain: Experience -> PawnData -> PawnClass -> Input -> Camera
# Idempotent: checks each link and only fixes what's broken.
import unreal

EAL = unreal.EditorAssetLibrary
fixes_applied = 0

print("=" * 60)
print("PIE PIPELINE FIX")
print("=" * 60)

# --- 1. Fix PawnData_Mordecai ---
print("\n--- Fixing PawnData_Mordecai ---")
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if not pd:
    print("FATAL: PawnData_Mordecai not found!")
else:
    # PawnClass -> BP_MordecaiCharacter (BP subclass, standard Lyra pattern)
    bp_class = unreal.load_class(None, '/MordecaiCore/Game/BP_MordecaiCharacter.BP_MordecaiCharacter_C')
    if bp_class:
        current_pc = pd.get_editor_property('pawn_class')
        if current_pc != bp_class:
            pd.set_editor_property('pawn_class', bp_class)
            print(f"  Fixed PawnClass: {bp_class.get_name()}")
            fixes_applied += 1
        else:
            print(f"  PawnClass OK: {current_pc.get_name()}")
    else:
        # Fallback: use C++ class directly
        pd.set_editor_property('pawn_class', unreal.MordecaiCharacter)
        print("  Fixed PawnClass: MordecaiCharacter (C++ fallback)")
        fixes_applied += 1

    # DefaultCameraMode -> MordecaiCameraMode_Diorama
    cam = pd.get_editor_property('default_camera_mode')
    cam_class = unreal.MordecaiCameraMode_Diorama if hasattr(unreal, 'MordecaiCameraMode_Diorama') else None
    if cam_class and cam != cam_class:
        pd.set_editor_property('default_camera_mode', cam_class)
        print(f"  Fixed DefaultCameraMode: MordecaiCameraMode_Diorama")
        fixes_applied += 1
    elif cam:
        print(f"  DefaultCameraMode OK: {cam.get_name()}")
    else:
        print("  WARNING: Could not set DefaultCameraMode")

    # InputConfig -> InputData_Mordecai
    ic = pd.get_editor_property('input_config')
    input_data = unreal.load_asset('/MordecaiCore/Input/InputData_Mordecai')
    if input_data and ic != input_data:
        pd.set_editor_property('input_config', input_data)
        print(f"  Fixed InputConfig: InputData_Mordecai")
        fixes_applied += 1
    elif ic:
        print(f"  InputConfig OK: {ic.get_path_name()}")
    else:
        print("  WARNING: Could not set InputConfig")

    EAL.save_asset('/MordecaiCore/System/PawnData_Mordecai', only_if_is_dirty=False)

# --- 2. Fix InputData_Mordecai NativeInputActions ---
print("\n--- Fixing InputData_Mordecai ---")
input_config = unreal.load_asset('/MordecaiCore/Input/InputData_Mordecai')
if not input_config:
    print("FATAL: InputData_Mordecai not found!")
else:
    native_actions = input_config.get_editor_property('native_input_actions')
    existing_tags = set()
    if native_actions:
        for action in native_actions:
            tag = action.get_editor_property('input_tag')
            existing_tags.add(str(tag))

    required_mappings = [
        ('InputTag.Move', '/MordecaiCore/Input/Actions/IA_Mordecai_Move'),
        ('InputTag.Look.Mouse', '/MordecaiCore/Input/Actions/IA_Mordecai_Look_Mouse'),
        ('InputTag.Look.Stick', '/MordecaiCore/Input/Actions/IA_Mordecai_Look_Stick'),
    ]

    for tag_name, action_path in required_mappings:
        tag_found = any(tag_name in t for t in existing_tags)
        if tag_found:
            print(f"  {tag_name} OK")
        else:
            ia = unreal.load_asset(action_path)
            if ia:
                tag = unreal.GameplayTag.request_gameplay_tag(unreal.Name(tag_name))
                entry = unreal.LyraInputAction()
                entry.set_editor_property('input_action', ia)
                entry.set_editor_property('input_tag', tag)
                native_actions.append(entry)
                print(f"  Added {tag_name} -> {ia.get_name()}")
                fixes_applied += 1
            else:
                print(f"  WARNING: Could not load {action_path}")

    if fixes_applied > 0:
        input_config.set_editor_property('native_input_actions', native_actions)
        EAL.save_asset('/MordecaiCore/Input/InputData_Mordecai', only_if_is_dirty=False)

# --- 3. Fix B_MordecaiDevExperience ---
print("\n--- Fixing B_MordecaiDevExperience ---")
dev_exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
if not dev_exp:
    print("FATAL: B_MordecaiDevExperience not found!")
else:
    pawn_data_ref = dev_exp.get_editor_property('default_pawn_data')
    if pawn_data_ref != pd:
        dev_exp.set_editor_property('default_pawn_data', pd)
        print(f"  Fixed DefaultPawnData: PawnData_Mordecai")
        fixes_applied += 1
    else:
        print(f"  DefaultPawnData OK")

    EAL.save_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience', only_if_is_dirty=False)

# --- 4. Fix BP_MordecaiCharacter HeroComponent DefaultInputMappings ---
print("\n--- Fixing BP_MordecaiCharacter DefaultInputMappings ---")
bp_class = unreal.load_class(None, '/MordecaiCore/Game/BP_MordecaiCharacter.BP_MordecaiCharacter_C')
if bp_class:
    cdo = unreal.get_default_object(bp_class)
    if cdo:
        comps = cdo.get_components_by_class(unreal.ActorComponent)
        hero_comps = [c for c in comps if 'Hero' in c.get_class().get_name()]
        if hero_comps:
            hero = hero_comps[0]
            mappings = hero.get_editor_property('default_input_mappings')
            imc_mordecai = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')

            has_imc = False
            if mappings and imc_mordecai:
                for m in mappings:
                    imc_ref = m.get_editor_property('input_mapping')
                    if imc_ref and str(imc_ref) == str(imc_mordecai):
                        has_imc = True
                        break

            if not has_imc and imc_mordecai:
                new_mapping = unreal.InputMappingContextAndPriority()
                new_mapping.set_editor_property('input_mapping', imc_mordecai)
                new_mapping.set_editor_property('priority', 1)
                new_mapping.set_editor_property('register_with_settings', True)
                if not mappings:
                    mappings = [new_mapping]
                else:
                    mappings.append(new_mapping)
                hero.set_editor_property('default_input_mappings', mappings)
                print(f"  Added IMC_Mordecai to DefaultInputMappings")
                fixes_applied += 1

                # Save the Blueprint asset
                EAL.save_asset('/MordecaiCore/Game/BP_MordecaiCharacter', only_if_is_dirty=False)
            elif has_imc:
                print("  IMC_Mordecai already in DefaultInputMappings")
            else:
                print("  WARNING: IMC_Mordecai not found")
        else:
            print("  WARNING: No HeroComponent on BP_MordecaiCharacter")
    else:
        print("  WARNING: Could not get CDO")
else:
    print("  WARNING: Could not load BP_MordecaiCharacter class")

print(f"\n{'=' * 60}")
print(f"FIXES APPLIED: {fixes_applied}")
print(f"{'=' * 60}")
