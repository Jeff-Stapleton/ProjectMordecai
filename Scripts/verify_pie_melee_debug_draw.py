"""Issue #92 PIE smoke: the melee debug-draw CVars exist and toggle live."""

import unreal

CVAR = "Mordecai.Debug.DrawMeleeHitShapes"
DURATION_CVAR = "Mordecai.Debug.DrawMeleeHitShapesDuration"

editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = editor.get_game_world()
if world is None:
    raise RuntimeError("FAIL: No PIE game world")

default_value = unreal.SystemLibrary.get_console_variable_int_value(CVAR)
duration = unreal.SystemLibrary.get_console_variable_float_value(DURATION_CVAR)
if default_value != 1:
    raise RuntimeError(f"FAIL: {CVAR} default is {default_value}, expected 1")
if abs(duration - 1.0) > 0.001:
    raise RuntimeError(f"FAIL: {DURATION_CVAR} default is {duration}, expected 1.0")
print(f"[PIE] PASS: {CVAR} defaults to 1")
print(f"[PIE] PASS: {DURATION_CVAR} defaults to 1.0 seconds")

unreal.SystemLibrary.execute_console_command(world, f"{CVAR} 0")
off_value = unreal.SystemLibrary.get_console_variable_int_value(CVAR)
if off_value != 0:
    raise RuntimeError(f"FAIL: runtime toggle to 0 read back {off_value}")
print(f"[PIE] PASS: {CVAR} changed to 0 without restart")

unreal.SystemLibrary.execute_console_command(world, f"{CVAR} 1")
on_value = unreal.SystemLibrary.get_console_variable_int_value(CVAR)
if on_value != 1:
    raise RuntimeError(f"FAIL: runtime toggle back to 1 read back {on_value}")
print(f"[PIE] PASS: {CVAR} changed back to 1 without restart")
