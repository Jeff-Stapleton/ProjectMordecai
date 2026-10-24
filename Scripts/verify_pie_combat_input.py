# PIE Smoke Test: Combat abilities and combat input (US-083)
# Run during PIE via run_pie_verify.py (or ue_remote_exec.py --file) to check that the player's
# combat abilities are granted from native defaults and that the runtime input mapping context
# maps the combat, jump and sprint keys.
#
# Checks (all read live state; nothing activates an ability or presses a key, so there is no
# [LOGIC] check here -- the keys are played for real only by the Tier 4 playtests):
#   - the controller's DefaultCombatAbilities holds the 5 combat abilities (native defaults)
#   - the player state's ASC holds a spec for each of them
#   - the hero component's MordecaiIMC_Runtime maps LMB / RT (light Tap, heavy HoldAndRelease,
#     parry chorded on Block + Pressed), RMB / LT (block, Down), LeftAlt / Gamepad B (dodge,
#     Pressed), SpaceBar / Gamepad A (jump) and LeftShift (sprint)
# Whether Left Shift still crouches (Lyra's binding on the sprint action, removed in
# UMordecaiHeroComponent::InitializePlayerInput) needs a real key press: Tier 4 (us001 sprint_shift).
#
# NOTE: All PIE references are held inside _run() so they go out of scope when it returns.

import unreal

COMBAT_ABILITIES = ("MordecaiGA_MeleeAttack", "MordecaiGA_HeavyAttack", "MordecaiGA_Dodge",
                    "MordecaiGA_Block", "MordecaiGA_Parry")

# (key name, input action name, trigger classes the mapping must carry)
EXPECTED_MAPPINGS = (
    ("LeftMouseButton", "IA_Mordecai_LightAttack", ("InputTriggerTap",)),
    ("LeftMouseButton", "IA_Mordecai_HeavyAttack", ("InputTriggerHoldAndRelease",)),
    ("LeftMouseButton", "IA_Mordecai_Parry_Runtime", ("InputTriggerChordAction", "InputTriggerPressed")),
    ("Gamepad_RightTriggerAxis", "IA_Mordecai_LightAttack", ("InputTriggerTap",)),
    ("Gamepad_RightTriggerAxis", "IA_Mordecai_HeavyAttack", ("InputTriggerHoldAndRelease",)),
    ("Gamepad_RightTriggerAxis", "IA_Mordecai_Parry_Runtime", ("InputTriggerChordAction", "InputTriggerPressed")),
    ("RightMouseButton", "IA_Mordecai_Block", ("InputTriggerDown",)),
    ("Gamepad_LeftTriggerAxis", "IA_Mordecai_Block", ("InputTriggerDown",)),
    ("LeftAlt", "IA_Mordecai_Dodge", ("InputTriggerPressed",)),
    ("Gamepad_FaceButton_Right", "IA_Mordecai_Dodge", ("InputTriggerPressed",)),
    ("SpaceBar", "IA_Mordecai_Jump", ()),
    ("Gamepad_FaceButton_Bottom", "IA_Mordecai_Jump", ()),
    ("LeftShift", "IA_Mordecai_Sprint", ()),
)


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
            print(f"  FAIL: {name}" + (f" -- {detail}" if detail else ""))

    def finish():
        print("")
        print(f"RESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE combat input smoke test failed: {fail_count} failure(s)")
        print("ALL CHECKS PASSED")

    print("=" * 60)
    print("PIE SMOKE TEST: COMBAT ABILITIES AND INPUT (US-083)")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")
    if not world:
        return finish()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    check("Player controller exists", pc is not None)
    check("Player pawn exists", pawn is not None)
    if not pc or not pawn:
        return finish()

    # --- Grant: native defaults on the controller, specs on the player state's ASC ---
    try:
        defaults = [c.get_name() for c in pc.get_editor_property("default_combat_abilities") if c]
    except Exception as e:  # a missing property is a failure, never a pass
        defaults = []
        check("DefaultCombatAbilities readable", False, str(e))
    for name in COMBAT_ABILITIES:
        check(f"DefaultCombatAbilities holds {name}", name in defaults, f"got {defaults}")

    granted = []
    try:
        asc = pc.player_state.get_lyra_ability_system_component()
        for handle in asc.get_all_abilities():
            # UAbilitySystemBlueprintLibrary's ScriptName is AbilitySystemLibrary
            ability, _ = unreal.AbilitySystemLibrary.get_gameplay_ability_from_spec_handle(asc, handle)
            if ability:
                granted.append(ability.get_class().get_name())
    except Exception as e:
        check("Ability specs readable", False, str(e))
    for name in COMBAT_ABILITIES:
        check(f"ASC holds a {name} spec", name in granted, f"granted: {granted}")

    # --- The runtime mapping context ---
    hero = pawn.get_component_by_class(unreal.MordecaiHeroComponent)
    check("MordecaiHeroComponent on the pawn", hero is not None)
    imc = None
    if hero:
        try:
            imc = hero.get_runtime_mapping_context()
        except Exception as e:
            check("get_runtime_mapping_context() callable", False, str(e))
    check("Runtime mapping context built", imc is not None)
    if imc:
        check("Context is MordecaiIMC_Runtime", imc.get_name() == "MordecaiIMC_Runtime", imc.get_name())
        mappings = []
        try:
            for m in imc.get_editor_property("default_key_mappings").get_editor_property("mappings"):
                action = m.get_editor_property("action")
                key = str(m.get_editor_property("key").get_editor_property("key_name"))
                triggers = [t.get_class().get_name() for t in m.get_editor_property("triggers") if t]
                mappings.append((key, action.get_name() if action else None, triggers))
        except Exception as e:
            check("Mappings readable", False, str(e))
        for key, action, triggers in EXPECTED_MAPPINGS:
            found = [m for m in mappings if m[0] == key and m[1] == action]
            ok = any(all(t in m[2] for t in triggers) for m in found)
            check(f"{key} -> {action}" + (f" ({', '.join(triggers)})" if triggers else ""), ok,
                  f"mappings of {key}: {[m for m in mappings if m[0] == key]}")

    finish()


_run()
del _run
