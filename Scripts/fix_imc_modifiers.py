# Fix NULL modifiers on IMC_Mordecai WASD key mappings.
# The Swizzle and Negate modifiers saved as null references from the previous script.
# This script recreates them properly and saves.
import unreal

EAL = unreal.EditorAssetLibrary

imc = unreal.load_asset('/MordecaiCore/Input/IMC_Mordecai')
if not imc:
    print('FATAL: IMC_Mordecai not found!')
else:
    mappings = imc.get_editor_property('mappings')
    move_action = unreal.load_asset('/MordecaiCore/Input/Actions/IA_Mordecai_Move')

    fixed = 0
    for m in mappings:
        action = m.get_editor_property('action')
        if not action or action != move_action:
            continue

        key = m.get_editor_property('key')
        key_name = str(key.get_editor_property('key_name'))

        if key_name == 'W':
            # W = forward = +Y axis. Swizzle YXZ converts X→Y
            swizzle = unreal.InputModifierSwizzleAxis()
            swizzle.set_editor_property('order', unreal.InputAxisSwizzle.YXZ)
            m.set_editor_property('modifiers', [swizzle])
            print(f'Fixed W: SwizzleYXZ')
            fixed += 1

        elif key_name == 'S':
            # S = backward = -Y axis. Negate then Swizzle
            negate = unreal.InputModifierNegate()
            swizzle = unreal.InputModifierSwizzleAxis()
            swizzle.set_editor_property('order', unreal.InputAxisSwizzle.YXZ)
            m.set_editor_property('modifiers', [negate, swizzle])
            print(f'Fixed S: Negate + SwizzleYXZ')
            fixed += 1

        elif key_name == 'A':
            # A = left = -X axis. Negate
            negate = unreal.InputModifierNegate()
            m.set_editor_property('modifiers', [negate])
            print(f'Fixed A: Negate')
            fixed += 1

        elif key_name == 'D':
            # D = right = +X axis. No modifiers needed.
            print(f'D: OK (no modifiers)')

        elif key_name == 'Gamepad_LeftStick2D':
            # Gamepad stick is natively 2D, just needs deadzone
            dead_zone = unreal.InputModifierDeadZone()
            dead_zone.set_editor_property('lower_threshold', 0.2)
            m.set_editor_property('modifiers', [dead_zone])
            print(f'Fixed Gamepad_LeftStick2D: DeadZone(0.2)')
            fixed += 1

    # Also fix the right stick look deadzone
    look_stick_action = unreal.load_asset('/MordecaiCore/Input/Actions/IA_Mordecai_Look_Stick')
    for m in mappings:
        action = m.get_editor_property('action')
        if action and action == look_stick_action:
            key = m.get_editor_property('key')
            key_name = str(key.get_editor_property('key_name'))
            if key_name == 'Gamepad_RightStick2D':
                dead_zone = unreal.InputModifierDeadZone()
                dead_zone.set_editor_property('lower_threshold', 0.2)
                m.set_editor_property('modifiers', [dead_zone])
                print(f'Fixed Gamepad_RightStick2D: DeadZone(0.2)')
                fixed += 1

    # Save
    imc.set_editor_property('mappings', mappings)
    EAL.save_asset('/MordecaiCore/Input/IMC_Mordecai', only_if_is_dirty=False)
    print(f'\nFixed {fixed} mappings. Saved to disk.')
    print('Stop and restart PIE to pick up changes.')
