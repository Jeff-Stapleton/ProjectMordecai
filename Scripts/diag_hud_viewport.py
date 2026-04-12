# Check if the HUD widget is actually in the viewport
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not w:
    print('NO PIE WORLD')
else:
    pc = unreal.GameplayStatics.get_player_controller(w, 0)

    # Try to find the HUD widget via the viewport widget tree
    # Method: iterate all widgets on the player's screen
    try:
        # Get all UUserWidgets currently in viewport
        all_widgets = unreal.WidgetLayoutLibrary.get_all_widgets_of_class(w, unreal.UserWidget)
        print(f'Total UserWidgets in viewport: {len(all_widgets)}')
        for widget in all_widgets:
            cls_name = widget.get_class().get_name()
            visible = widget.is_visible()
            print(f'  {cls_name} visible={visible}')
    except Exception as e:
        print(f'get_all_widgets error: {e}')

    # Alternative: try getting property names on the PC to find the right one
    try:
        # Check if the property is named differently in Python
        props = [p for p in dir(pc) if 'hud' in p.lower() or 'combat' in p.lower()]
        print(f'PC HUD-related properties: {props}')
    except Exception as e:
        print(f'Property scan error: {e}')
