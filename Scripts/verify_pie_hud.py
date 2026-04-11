# PIE Smoke Test: HUD Visibility
# Run during PIE via ue_remote_exec.py to verify the combat HUD is visible.
# Exit: prints PASS/FAIL lines. Any FAIL means the HUD is broken.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_hud.py').read())"

import unreal

PASS_COUNT = 0
FAIL_COUNT = 0

def check(name, condition, detail=""):
    global PASS_COUNT, FAIL_COUNT
    if condition:
        PASS_COUNT += 1
        print(f"  PASS: {name}")
    else:
        FAIL_COUNT += 1
        msg = f"  FAIL: {name}"
        if detail:
            msg += f" -- {detail}"
        print(msg)

print("=" * 60)
print("PIE SMOKE TEST: HUD")
print("=" * 60)

# Get PIE world
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
check("PIE world exists", world is not None, "Start PIE before running this test")

if not world:
    print(f"\nRESULT: {PASS_COUNT} passed, {FAIL_COUNT} failed")
    if FAIL_COUNT > 0:
        raise RuntimeError(f"PIE HUD smoke test failed: {FAIL_COUNT} failure(s)")

# Get player controller
pc = unreal.GameplayStatics.get_player_controller(world, 0)
check("Player controller exists", pc is not None)

if not pc:
    print(f"\nRESULT: {PASS_COUNT} passed, {FAIL_COUNT} failed")
    if FAIL_COUNT > 0:
        raise RuntimeError(f"PIE HUD smoke test failed: {FAIL_COUNT} failure(s)")

# Check controller class
pc_class_name = pc.get_class().get_name()
check("Controller is MordecaiPlayerController",
      "Mordecai" in pc_class_name,
      f"Got {pc_class_name}")

# Check CombatHUDWidget exists (BlueprintReadOnly property)
hud_widget = None
try:
    hud_widget = pc.get_editor_property("combat_hud_widget")
except Exception:
    pass

check("CombatHUDWidget is not None", hud_widget is not None,
      "PlayerController.CombatHUDWidget is null -- HUD was never created")

if hud_widget:
    # Check visibility
    is_visible = hud_widget.is_visible()
    check("CombatHUDWidget is visible", is_visible)

    # Check child widget count via BlueprintCallable accessor
    # (child properties are protected, so we use GetBoundChildCount instead)
    try:
        child_count = hud_widget.get_bound_child_count()
        check(f"Child widgets bound ({child_count}/10)",
              child_count >= 5,
              f"Only {child_count}/10 children bound -- BuildDefaultLayout may have failed")

        # Detailed count check
        if child_count == 10:
            check("All 10 child widgets present", True)
        elif child_count >= 5:
            check("Core child widgets present (5+)", True,
                  f"{child_count}/10 -- some optional widgets missing")
    except Exception as e:
        check("GetBoundChildCount accessible", False, str(e))

print("")
print(f"RESULT: {PASS_COUNT} passed, {FAIL_COUNT} failed")

if FAIL_COUNT > 0:
    raise RuntimeError(f"PIE HUD smoke test failed: {FAIL_COUNT} failure(s)")
else:
    print("ALL CHECKS PASSED")
