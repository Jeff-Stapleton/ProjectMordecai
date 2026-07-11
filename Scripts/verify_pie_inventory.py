# PIE Smoke Test: Inventory Vertical Slice (US-071 + US-079)
# Run during PIE via ue_remote_exec.py. Exercises the full flow:
#   widget sanity (US-071) -> component wiring -> press-to-pickup with
#   prompt checks -> auto-store routing -> pause menu inventory tab ->
#   identify the unidentified amulet.
#
# Usage:
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_inventory.py').read())"
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
            raise RuntimeError(f"PIE inventory smoke test failed: {fail_count} failure(s)")
        print("ALL CHECKS PASSED")

    print("=" * 60)
    print("PIE SMOKE TEST: INVENTORY VERTICAL SLICE (US-071 / US-079)")
    print("=" * 60)

    # Get PIE world
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "Start PIE before running this test")
    if not world:
        finish()
        return

    # --- Class registration ---
    for cls_name in ('MordecaiInventoryWidget', 'MordecaiPauseMenuWidget',
                     'MordecaiItemPickup', 'MordecaiPickupInteractionComponent',
                     'MordecaiPickupPromptWidget'):
        check(f"{cls_name} class registered", hasattr(unreal, cls_name),
              "Class not found in unreal module")
    if fail_count > 0:
        finish()
        return

    # --- US-071: standalone widget creation + unbound behavior ---
    widget = None
    try:
        widget = unreal.WidgetBlueprintLibrary.create(world, unreal.MordecaiInventoryWidget)
    except Exception as e:
        check("Inventory widget created", False, str(e))

    if widget:
        check("Inventory widget created", True)
        try:
            list_ph = str(widget.get_list_placeholder_text())
            check("Unbound list placeholder is '--'", list_ph == "--", f"Got '{list_ph}'")
            ledger_ph = str(widget.get_ledger_placeholder_text())
            check("Unbound ledger placeholder is '--'", ledger_ph == "--", f"Got '{ledger_ph}'")
            check("Zero visible rows while unbound", widget.get_visible_row_count() == 0,
                  f"Got {widget.get_visible_row_count()}")
            widget.set_filter(unreal.MordecaiInventoryFilter.WEAPONS)
            widget.set_filter(unreal.MordecaiInventoryFilter.ALL)
            check("Filter switching does not crash", True)
        except Exception as e:
            check("Unbound widget state queries", False, str(e))

    # --- AC-079.7: pawn component wiring ---
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    check("Player pawn exists", pawn is not None)
    if not pawn:
        finish()
        return

    inventory = pawn.get_component_by_class(unreal.MordecaiInventoryComponent)
    ledger = pawn.get_component_by_class(unreal.MordecaiResourceLedger)
    interaction = pawn.get_component_by_class(unreal.MordecaiPickupInteractionComponent)
    check("Pawn has InventoryComponent", inventory is not None)
    check("Pawn has ResourceLedger", ledger is not None)
    check("Pawn has PickupInteractionComponent", interaction is not None)
    if not (inventory and ledger and interaction):
        finish()
        return

    # --- HUD prompt widget present (AC-079.18) ---
    prompts = unreal.WidgetBlueprintLibrary.get_all_widgets_of_class(world, unreal.MordecaiPickupPromptWidget, True)
    prompt = prompts[0] if prompts else None
    check("Pickup prompt widget lives in the HUD", prompt is not None)
    if prompt:
        hint = str(unreal.MordecaiPickupPromptWidget.get_inventory_hint_text())
        check("Inventory assist hint is '[I] Inventory'", hint == "[I] Inventory", f"Got '{hint}'")

    # --- AC-079.6 / AC-079.12: press-to-pickup loop over all placed pickups ---
    pickups = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MordecaiItemPickup)
    check("6 test pickups placed in the map", len(pickups) == 6,
          f"Found {len(pickups)} (run Scripts/place_test_pickups.py)")

    collected = 0
    for pickup in list(pickups):
        label = pickup.get_actor_label()
        item_name = str(pickup.get_item_display_name())
        loc = pickup.get_actor_location()

        # Walk-up stand-in: teleport beside the pickup; overlap fires synchronously
        pawn.set_actor_location(unreal.Vector(loc.x + 60, loc.y, loc.z + 60), False, True)
        interaction.evaluate_focus()

        focused = interaction.get_focused_pickup()
        check(f"{label}: focused after walking up", focused == pickup,
              f"Focused: {focused.get_actor_label() if focused else 'None'}")

        if prompt:
            prompt_text = str(prompt.get_prompt_text())
            check(f"{label}: prompt shows '[F] Pick up {item_name}'",
                  prompt.is_prompt_visible() and item_name in prompt_text and prompt_text.startswith("[F]"),
                  f"Got '{prompt_text}'")

        # F-press stand-in
        picked = interaction.try_pickup_focused()
        check(f"{label}: try_pickup_focused succeeded", bool(picked))
        if picked:
            collected += 1
        check(f"{label}: pickup actor consumed",
              not unreal.SystemLibrary.is_valid(pickup) or pickup.is_actor_being_destroyed())

    check("All 6 pickups collected", collected == 6, f"Collected {collected}")

    # --- Routing: 4 flat-inventory instances + 2 auto-stored resources ---
    items = inventory.get_sorted_items()
    check("Inventory holds 4 non-autostored instances", len(items) == 4,
          f"Got {len(items)}: {[str(i.item_definition.get_editor_property('item_id')) for i in items if i.item_definition]}")
    check("Ledger IronOre == 1", ledger.get_resource_count("TestItem_IronOre") == 1,
          f"Got {ledger.get_resource_count('TestItem_IronOre')}")
    check("Ledger TownStone == 1", ledger.get_resource_count("TestItem_TownStone") == 1,
          f"Got {ledger.get_resource_count('TestItem_TownStone')}")

    if prompt:
        check("Prompt hidden after everything collected", not prompt.is_prompt_visible())

    # --- AC-079.8/.9: open the pause menu on the inventory tab (I-key path) ---
    subsystem = unreal.SubsystemBlueprintLibrary.get_game_instance_subsystem(
        world, unreal.MordecaiPauseMenuSubsystem)
    check("Pause menu subsystem available", subsystem is not None)

    menu_widget = None
    if subsystem:
        subsystem.toggle_pause_menu_to_tab("inventory")
        check("Menu reports open", subsystem.is_menu_open())
        menu_widget = subsystem.get_active_menu_widget()
        check("Active menu widget exists", menu_widget is not None,
              "PrimaryGameLayout missing? Menu widget was not pushed")

    inventory_tab = None
    if menu_widget:
        active_tab = str(menu_widget.get_active_tab_id())
        check("Active tab is 'inventory'", active_tab == "inventory", f"Got '{active_tab}'")
        content = menu_widget.get_current_content_widget()
        if content and content.get_class().get_name() == "MordecaiInventoryWidget":
            inventory_tab = content
        check("Inventory tab content is the inventory widget", inventory_tab is not None,
              f"Got {content.get_class().get_name() if content else 'None'}")

    if inventory_tab:
        check("Inventory tab bound to the pawn's inventory", inventory_tab.is_bound_to_inventory())
        rows = inventory_tab.get_visible_row_count()
        check("Inventory tab shows the 4 collected items", rows == 4, f"Got {rows} rows")
        ledger_rows = inventory_tab.get_ledger_rows()
        check("Ledger panel shows 2 stored resources", len(ledger_rows) == 2, f"Got {len(ledger_rows)}")

        # --- Identify the Mysterious Amulet (AC-079.12 step 7) ---
        amulet_id = None
        for inst in inventory.get_sorted_items():
            item_def = inst.get_editor_property('item_definition')
            if item_def and str(item_def.get_editor_property('item_id')) == 'TestItem_MysteriousAmulet':
                amulet_id = inst.get_editor_property('instance_id')
                unidentified_name = str(unreal.MordecaiItemLibrary.get_display_name(inst))
                check("Amulet shows partial name before identify",
                      "Unidentified" in unidentified_name, f"Got '{unidentified_name}'")
                break
        check("Amulet instance found", amulet_id is not None)

        if amulet_id is not None:
            identified = inventory_tab.try_identify(amulet_id)
            check("TryIdentify succeeded", bool(identified))
            for inst in inventory.get_sorted_items():
                if inst.get_editor_property('instance_id') == amulet_id:
                    new_name = str(unreal.MordecaiItemLibrary.get_display_name(inst))
                    check("Amulet row shows identified name",
                          new_name == "Mysterious Amulet", f"Got '{new_name}'")
                    break
            check("TryIdentify no-op on identified instance",
                  inventory_tab.try_identify(amulet_id) == False)

    if subsystem and subsystem.is_menu_open():
        subsystem.close_pause_menu()
        check("Menu closes cleanly", not subsystem.is_menu_open())

    finish()

_run()
del _run
