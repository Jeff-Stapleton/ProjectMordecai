# PIE Smoke Test: Loading Screen (issue #39)
# Run during PIE via ue_remote_exec.py to verify the loading screen manager shows the
# Mordecai loading screen: CommonLoadingScreenSettings.LoadingScreenWidget names the native
# UMordecaiLoadingScreenWidget, not Lyra's W_LoadingScreen_Host (the LYRA logo).
#
# Read-only. PIE has no loading screen to look at by the time this runs: the editor does not
# hold it up after the experience loads (CommonLoadingScreen applies its 2 s hold outside the
# editor only). The widget's tree is Tier 1 (Mordecai.UI.LoadingScreen.*); what a player sees
# is Tier 3 (playtest_boot.py's loading_screen check and engine_loading.png).
#
# Usage:
#   py Scripts/run_pie_verify.py --tests loading_screen
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_loading_screen.py').read())"
#
# NOTE: All PIE references are held inside _run() so they go out of scope
# when the function returns, avoiding GC reference leaks after PIE ends.

import unreal

def _run():
    pass_count = 0
    fail_count = 0

    def check(name, condition, detail=""):
        nonlocal pass_count, fail_count
        if condition:
            pass_count += 1
            print(f"  PASS: {name}")
        else:
            fail_count += 1
            msg = f"  FAIL: {name}"
            if detail:
                msg += f" -- {detail}"
            print(msg)

    def finish():
        print("")
        print(f"RESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE loading screen smoke test failed: {fail_count} failure(s)")
        print("ALL CHECKS PASSED")

    print("=" * 60)
    print("PIE SMOKE TEST: LOADING SCREEN")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")
    if not world:
        finish()
        return

    widget_cls = getattr(unreal, 'MordecaiLoadingScreenWidget', None)
    check("MordecaiLoadingScreenWidget class registered", widget_cls is not None,
          "Class not found in unreal module")
    if widget_cls is None:
        finish()
        return

    expected = widget_cls.static_class().get_path_name()
    configured = widget_cls.get_configured_loading_screen_widget_class()
    got = configured.get_path_name() if configured else 'None'
    check("CommonLoadingScreenSettings.LoadingScreenWidget is the Mordecai loading screen",
          got == expected, f"configured {got}, expected {expected}")
    check("Not Lyra's W_LoadingScreen_Host", 'W_LoadingScreen_Host' not in got, got)

    game_instance = unreal.GameplayStatics.get_game_instance(world)
    manager = next((s for s in unreal.ObjectIterator(unreal.LoadingScreenManager)
                    if s.get_outer() == game_instance), None)
    check("The PIE game instance has a loading screen manager", manager is not None)
    if manager:
        print(f"  INFO: loading screen reason: {manager.get_debug_reason_for_showing_or_hiding_loading_screen()}")

    finish()

_run()
del _run
