# Check HUD widget via alternative approaches
import unreal

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)

# Try get_hud()
try:
    hud = pc.get_hud()
    print(f'get_hud(): {hud.get_class().get_name() if hud else None}')
except Exception as e:
    print(f'get_hud error: {e}')

# Check output log for the HUD warning
# grep for the "Failed to load WBP_CombatHUD" message
try:
    import os
    log_path = r'C:/Users/jeffd/Documents/Gamedev/ProjectMordecai/Saved/Logs/ProjectMordecai.log'
    if os.path.exists(log_path):
        with open(log_path, 'r', errors='replace') as f:
            lines = f.readlines()
        hud_lines = [l.strip() for l in lines if 'CombatHUD' in l or 'combat_hud' in l.lower() or 'CreateCombatHUD' in l]
        print(f'HUD-related log lines ({len(hud_lines)}):')
        for line in hud_lines[-10:]:
            print(f'  {line}')
    else:
        print('Log file not found')
except Exception as e:
    print(f'Log check error: {e}')

# Try to find widgets by iterating the viewport
try:
    gi = unreal.GameplayStatics.get_game_instance(w)
    print(f'GameInstance: {gi.get_class().get_name() if gi else None}')
except Exception as e:
    print(f'GameInstance error: {e}')
