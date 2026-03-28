"""US-054 Step 8: Configure player controller with HUD widget class.

Sets CombatHUDWidgetClass on the MordecaiPlayerController CDO (or BP).
"""
import unreal

EAL = unreal.EditorAssetLibrary

# The player controller is AMordecaiPlayerController (C++ class)
# We need to set CombatHUDWidgetClass on it.
# Options:
# 1. Set on the C++ CDO directly
# 2. Create a Blueprint subclass and configure it there

# Try setting on C++ CDO
try:
    pc_cdo = unreal.get_default_object(unreal.MordecaiPlayerController)
    if pc_cdo:
        # Set HUD widget class
        hud_path = '/MordecaiCore/UI/WBP_CombatHUD'
        if EAL.does_asset_exist(hud_path):
            hud_class = unreal.load_class(None, hud_path + '.WBP_CombatHUD_C')
            if hud_class:
                pc_cdo.set_editor_property('combat_hud_widget_class', hud_class)
                unreal.log('Set CombatHUDWidgetClass = WBP_CombatHUD on MordecaiPlayerController CDO')
            else:
                unreal.log_warning('Could not load WBP_CombatHUD class')
        else:
            unreal.log_warning('WBP_CombatHUD not found')
except Exception as e:
    unreal.log_warning(f'Could not set on C++ CDO: {e}')
    unreal.log('NOTE: CombatHUDWidgetClass needs to be set via Blueprint or CDO in C++')

print('SUCCESS: Player controller HUD configuration attempted.')
print('NOTE: If CDO setting didnt persist, configure via Blueprint defaults in editor.')
