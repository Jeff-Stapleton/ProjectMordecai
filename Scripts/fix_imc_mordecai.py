# Populate IMC_Mordecai with key bindings for movement, look, and basic actions.
# Uses UE5.7 Enhanced Input API.
import unreal

EAL = unreal.EditorAssetLibrary


def make_key(name):
    """Create an FKey with the given key name."""
    k = unreal.Key()
    k.set_editor_property('key_name', name)
    return k


def make_mapping(action, key_name, modifiers=None):
    """Create an EnhancedActionKeyMapping."""
    m = unreal.EnhancedActionKeyMapping()
    m.set_editor_property('action', action)
    m.set_editor_property('key', make_key(key_name))
    if modifiers:
        m.set_editor_property('modifiers', modifiers)
    return m


imc = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')
if not imc:
    print('FATAL: IMC_Mordecai not found!')
else:
    # Load all Input Actions
    actions = {}
    action_names = [
        'IA_Mordecai_Move', 'IA_Mordecai_Look_Mouse', 'IA_Mordecai_Look_Stick',
        'IA_Mordecai_Sprint', 'IA_Mordecai_Jump', 'IA_Mordecai_Dodge',
        'IA_Mordecai_LightAttack', 'IA_Mordecai_HeavyAttack', 'IA_Mordecai_Block',
        'IA_Mordecai_Interact',
    ]
    for name in action_names:
        asset = unreal.load_asset(f'/MordecaiCore/Input/Actions/{name}')
        if asset:
            actions[name] = asset
        else:
            print(f'WARNING: Could not load {name}')

    mappings = []

    # --- Movement: WASD ---
    if 'IA_Mordecai_Move' in actions:
        move = actions['IA_Mordecai_Move']

        # W key (forward = +Y): Swizzle to put 1D on Y axis
        swizzle_w = unreal.InputModifierSwizzleAxis()
        swizzle_w.set_editor_property('order', unreal.InputAxisSwizzle.YXZ)
        mappings.append(make_mapping(move, 'W', [swizzle_w]))

        # S key (backward = -Y): Negate then Swizzle
        negate_s = unreal.InputModifierNegate()
        swizzle_s = unreal.InputModifierSwizzleAxis()
        swizzle_s.set_editor_property('order', unreal.InputAxisSwizzle.YXZ)
        mappings.append(make_mapping(move, 'S', [negate_s, swizzle_s]))

        # D key (right = +X): no modifiers needed
        mappings.append(make_mapping(move, 'D'))

        # A key (left = -X): Negate
        negate_a = unreal.InputModifierNegate()
        mappings.append(make_mapping(move, 'A', [negate_a]))

        # Gamepad Left Stick 2D
        dead_zone = unreal.InputModifierDeadZone()
        dead_zone.set_editor_property('lower_threshold', 0.2)
        mappings.append(make_mapping(move, 'Gamepad_LeftStick2D', [dead_zone]))

    # --- Mouse Look ---
    if 'IA_Mordecai_Look_Mouse' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_Look_Mouse'], 'Mouse2D'))

    # --- Gamepad Right Stick Look ---
    if 'IA_Mordecai_Look_Stick' in actions:
        dead_zone2 = unreal.InputModifierDeadZone()
        dead_zone2.set_editor_property('lower_threshold', 0.2)
        mappings.append(make_mapping(actions['IA_Mordecai_Look_Stick'], 'Gamepad_RightStick2D', [dead_zone2]))

    # --- Sprint (Shift / L3) ---
    if 'IA_Mordecai_Sprint' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_Sprint'], 'LeftShift'))
        mappings.append(make_mapping(actions['IA_Mordecai_Sprint'], 'Gamepad_LeftThumbstickButton'))

    # --- Jump (Space / Gamepad A) ---
    if 'IA_Mordecai_Jump' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_Jump'], 'SpaceBar'))
        mappings.append(make_mapping(actions['IA_Mordecai_Jump'], 'Gamepad_FaceButton_Bottom'))

    # --- Dodge (Alt / Gamepad B) ---
    if 'IA_Mordecai_Dodge' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_Dodge'], 'LeftAlt'))
        mappings.append(make_mapping(actions['IA_Mordecai_Dodge'], 'Gamepad_FaceButton_Right'))

    # --- Light Attack (LMB / Right Trigger) ---
    if 'IA_Mordecai_LightAttack' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_LightAttack'], 'LeftMouseButton'))
        mappings.append(make_mapping(actions['IA_Mordecai_LightAttack'], 'Gamepad_RightTrigger'))

    # --- Heavy Attack (RMB / Right Bumper) ---
    if 'IA_Mordecai_HeavyAttack' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_HeavyAttack'], 'RightMouseButton'))
        mappings.append(make_mapping(actions['IA_Mordecai_HeavyAttack'], 'Gamepad_RightShoulder'))

    # --- Interact (E / Gamepad X) ---
    if 'IA_Mordecai_Interact' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_Interact'], 'E'))
        mappings.append(make_mapping(actions['IA_Mordecai_Interact'], 'Gamepad_FaceButton_Left'))

    # --- Block (Q / Left Trigger) ---
    if 'IA_Mordecai_Block' in actions:
        mappings.append(make_mapping(actions['IA_Mordecai_Block'], 'Q'))
        mappings.append(make_mapping(actions['IA_Mordecai_Block'], 'Gamepad_LeftTrigger'))

    # Apply all mappings to IMC
    try:
        imc.set_editor_property('default_key_mappings', mappings)
        print('Used default_key_mappings API')
    except Exception as e:
        print(f'default_key_mappings failed ({e}), trying mappings...')
        imc.set_editor_property('mappings', mappings)
        print('Used mappings API (deprecated)')

    EAL.save_asset('/MordecaiCore/Input/IMC_Mordecai', only_if_is_dirty=False)
    print(f'IMC_Mordecai: Added {len(mappings)} key mappings')
    print('Key bindings saved successfully!')
