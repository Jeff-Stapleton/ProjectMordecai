# Diagnose HUD widget loading issue during PIE
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not w:
    print('NO PIE WORLD')
else:
    # Check if WBP_CombatHUD can be loaded
    cls = unreal.load_class(None, '/MordecaiCore/UI/WBP_CombatHUD.WBP_CombatHUD_C')
    print(f'WBP_CombatHUD class: {cls}')

    # Check the asset exists
    eal = unreal.EditorAssetLibrary
    exists = eal.does_asset_exist('/MordecaiCore/UI/WBP_CombatHUD')
    print(f'Asset exists: {exists}')

    # Check the PC
    pc = unreal.GameplayStatics.get_player_controller(w, 0)
    print(f'PC class: {pc.get_class().get_name()}')

    # Check HUD properties
    try:
        hud = pc.get_editor_property('combat_hud_widget')
        print(f'combat_hud_widget: {hud}')
    except Exception as e:
        print(f'combat_hud_widget error: {e}')

    try:
        hud_class = pc.get_editor_property('combat_hud_widget_class')
        print(f'combat_hud_widget_class: {hud_class}')
    except Exception as e:
        print(f'combat_hud_widget_class error: {e}')

    # Check if OnPossess happened (pawn should exist)
    pawn = pc.get_pawn()
    print(f'Pawn: {pawn.get_class().get_name() if pawn else None}')

    # Check PlayerState for ASC
    try:
        ps = pc.player_state
        print(f'PlayerState: {ps.get_class().get_name() if ps else None}')
    except Exception as e:
        print(f'PlayerState error: {e}')
