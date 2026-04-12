# Diagnose PIE readiness: check the full spawn chain
# Experience -> PawnData -> PawnClass -> Input -> Camera
import unreal

EAL = unreal.EditorAssetLibrary

print("=" * 60)
print("PIE READINESS DIAGNOSTIC")
print("=" * 60)

# 1. Check ExperienceOverride in developer settings
print("\n--- Developer Settings ---")
dev_settings = unreal.get_default_object(unreal.LyraDeveloperSettings)
if dev_settings:
    exp_override = dev_settings.get_editor_property('experience_override')
    print(f"ExperienceOverride: {exp_override}")
else:
    print("WARNING: Could not load LyraDeveloperSettings")

# 2. Check B_MordecaiDevExperience
print("\n--- B_MordecaiDevExperience ---")
dev_exp = unreal.load_asset('/MordecaiCore/System/Experiences/B_MordecaiDevExperience')
if dev_exp:
    pawn_data_ref = dev_exp.get_editor_property('default_pawn_data')
    print(f"DefaultPawnData: {pawn_data_ref.get_path_name() if pawn_data_ref else 'NOT SET'}")

    gf_to_enable = dev_exp.get_editor_property('game_features_to_enable')
    print(f"GameFeaturesToEnable: {list(gf_to_enable) if gf_to_enable else 'EMPTY'}")
else:
    print("WARNING: Could not load B_MordecaiDevExperience")

# 3. Check PawnData_Mordecai
print("\n--- PawnData_Mordecai ---")
pd = unreal.load_asset('/MordecaiCore/System/PawnData_Mordecai')
if pd:
    pc = pd.get_editor_property('pawn_class')
    print(f"PawnClass: {pc.get_name() if pc else 'NOT SET'}")

    cam = pd.get_editor_property('default_camera_mode')
    print(f"DefaultCameraMode: {cam.get_name() if cam else 'NOT SET'}")

    ic = pd.get_editor_property('input_config')
    print(f"InputConfig: {ic.get_path_name() if ic else 'NOT SET'}")

    ability_sets = pd.get_editor_property('ability_sets')
    print(f"AbilitySets: {len(ability_sets) if ability_sets else 0} set(s)")
else:
    print("WARNING: Could not load PawnData_Mordecai")

# 4. Check InputData_Mordecai
print("\n--- InputData_Mordecai ---")
input_config = unreal.load_asset('/MordecaiCore/Input/InputData_Mordecai')
if input_config:
    native_actions = input_config.get_editor_property('native_input_actions')
    if native_actions:
        print(f"NativeInputActions: {len(native_actions)} action(s)")
        for action in native_actions:
            tag = action.get_editor_property('input_tag')
            ia = action.get_editor_property('input_action')
            print(f"  {tag} -> {ia.get_name() if ia else 'None'}")
    else:
        print("NativeInputActions: EMPTY")

    ability_actions = input_config.get_editor_property('ability_input_actions')
    if ability_actions:
        print(f"AbilityInputActions: {len(ability_actions)} action(s)")
    else:
        print("AbilityInputActions: EMPTY")
else:
    print("WARNING: Could not load InputData_Mordecai")

# 5. Check BP_MordecaiCharacter HeroComponent DefaultInputMappings
print("\n--- BP_MordecaiCharacter ---")
bp_char = unreal.load_asset('/MordecaiCore/Game/BP_MordecaiCharacter')
if bp_char:
    bp_class = unreal.load_class(None, '/MordecaiCore/Game/BP_MordecaiCharacter.BP_MordecaiCharacter_C')
    if bp_class:
        cdo = unreal.get_default_object(bp_class)
        if cdo:
            comps = cdo.get_components_by_class(unreal.ActorComponent)
            hero_comps = [c for c in comps if 'Hero' in c.get_class().get_name()]
            if hero_comps:
                hero = hero_comps[0]
                mappings = hero.get_editor_property('default_input_mappings')
                if mappings:
                    print(f"DefaultInputMappings: {len(mappings)} mapping(s)")
                    for m in mappings:
                        imc = m.get_editor_property('input_mapping')
                        priority = m.get_editor_property('priority')
                        print(f"  IMC: {imc} Priority: {priority}")
                else:
                    print("DefaultInputMappings: EMPTY")
            else:
                print("WARNING: No HeroComponent found on BP_MordecaiCharacter CDO")
        else:
            print("WARNING: Could not get CDO for BP_MordecaiCharacter")
    else:
        print("WARNING: Could not load BP_MordecaiCharacter_C class")
else:
    print("WARNING: Could not load BP_MordecaiCharacter")

# 6. Check IMC_Mordecai
print("\n--- IMC_Mordecai ---")
imc = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')
if imc:
    mappings = imc.get_editor_property('mappings')
    if mappings:
        print(f"Key Mappings: {len(mappings)} mapping(s)")
        for m in mappings:
            action = m.get_editor_property('action')
            key = m.get_editor_property('key')
            print(f"  {action.get_name() if action else 'None'} -> {key}")
    else:
        print("Key Mappings: EMPTY")
else:
    print("WARNING: Could not load IMC_Mordecai")

print("\n" + "=" * 60)
print("DIAGNOSTIC COMPLETE")
print("=" * 60)
