# PIE Smoke Test: Inventory Vertical Slice (US-071 + US-079) and the pooled reopen (US-094, issue #28)
# Run during PIE via ue_remote_exec.py. Exercises the full flow:
#   widget sanity (US-071) -> component wiring -> press-to-pickup with
#   prompt checks -> auto-store routing -> pause menu inventory tab ->
#   identify the unidentified amulet.
#
# Three phases, one remote exec each. A remote exec runs inside one editor
# frame, and CommonUI releases a closed menu's Slate tree on a later core tick
# (~SObjectWidget -> the content's NativeDestruct -> Unbind). So
# run_pie_verify.py runs the script again, after the editor has ticked,
# whenever it prints PIE_NEXT_PHASE. The phase comes from PIE state (pickups
# left, pause menu instances); nothing is kept between runs:
#   1. Fresh PIE, no pause menu yet: every pickup's name label reads its
#      display name, square to the PIE camera and left to right (issues #30,
#      #38; read-only, untagged). Then open the inventory before any pickup
#      ("Inventory is empty."), close it.
#   2. Pickups left, a pooled (closed) menu exists: pick everything up, then
#      reopen. CommonUI hands back the SAME menu instance; its tab must show
#      the 4 items and 2 resources, and identify must work (issue #28).
#   3. No pickups left: change the inventory while the menu is closed, reopen
#      the pooled menu again; a change while open redraws, filters work.
# Every phase closes the menu (verify_pie_pause_menu.py expects it closed).
# The phases assume a fresh PIE session, as run_pie_verify.py starts one.
# On the build without the US-094 fix, phase 2 FAILS: the reopened tab is
# unbound ('--', 0 rows), which only the Slate teardown between the execs can
# cause, so this exercises the pooled reopen the game takes. (Whether the
# editor paints PIE between the execs does not matter, and is not checked:
# with a locked or hidden desktop it paints nothing.)
#
# Checks that call gameplay/UI functions directly (teleport, try_pickup_focused,
# the pause-menu subsystem, add_item, set_filter, try_identify), or read state
# those calls produced, are tagged [LOGIC]: diagnostics, never acceptance of a
# player-facing AC (CLAUDE.md Step 4b). Rows are read from the widget's caches;
# the rendered rows are Tier 4's job (Scripts/playtests/us079_inventory.py).
#
# Usage:
#   py Scripts/run_pie_verify.py --tests inventory      # all phases
#   py ue_remote_exec.py "exec(open(r'Scripts/verify_pie_inventory.py').read())"   # one phase per call
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

    def logic(name, condition, detail=""):
        # Direct gameplay/UI call: logic/diagnostic only, never acceptance of a player-facing AC
        check(f"[LOGIC] {name}", condition, detail)

    def finish(next_phase=None):
        print("")
        print(f"RESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            raise RuntimeError(f"PIE inventory smoke test failed: {fail_count} failure(s)")
        if next_phase:
            print("PHASE PASSED (not finished: run again after the editor ticks)")
            print(f"PIE_NEXT_PHASE: {next_phase}")
        else:
            print("ALL CHECKS PASSED")

    print("=" * 60)
    print("PIE SMOKE TEST: INVENTORY VERTICAL SLICE (US-071 / US-079 / US-094)")
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

    # US-071 unbound-widget behavior (placeholders, filters) is covered by the headless
    # Mordecai.UI.Inventory* tests: WidgetLibrary.create is BlueprintInternalUseOnly and
    # not exposed to Python, so a standalone widget can't be created here.

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

    # SubsystemBlueprintLibrary isn't exposed to Python; find the PIE game instance's subsystem directly
    game_instance = unreal.GameplayStatics.get_game_instance(world)
    subsystem = next((s for s in unreal.ObjectIterator(unreal.MordecaiPauseMenuSubsystem)
                      if s.get_outer() == game_instance), None)
    check("Pause menu subsystem available", subsystem is not None)
    if not subsystem:
        finish()
        return

    check("Pause menu starts closed", not subsystem.is_menu_open())

    # --- Phase, from PIE state ---
    pickups = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.MordecaiItemPickup)
    menus = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.MordecaiPauseMenuWidget, False)
    if not menus:
        phase = 1
    else:
        phase = 2 if pickups else 3
    print(f"PHASE {phase} ({len(pickups)} pickups left, {len(menus)} pause menu instance(s))")

    if phase == 1 and not pickups:
        check("6 test pickups placed in the map", False,
              "Found none, and no earlier phase ran in this PIE session (run Scripts/place_test_pickups.py)")
        finish()
        return

    def open_inventory_tab(label):
        """I-key stand-in: the subsystem pushes the menu onto UI.Layer.Menu and selects the inventory tab."""
        subsystem.toggle_pause_menu_to_tab("inventory")
        logic(f"{label}: menu reports open", subsystem.is_menu_open())
        menu = subsystem.get_active_menu_widget()
        logic(f"{label}: active menu widget exists", menu is not None,
              "PrimaryGameLayout missing? Menu widget was not pushed")
        if not menu:
            return None, None
        active_tab = str(menu.get_active_tab_id())
        logic(f"{label}: active tab is 'inventory'", active_tab == "inventory", f"Got '{active_tab}'")
        content = menu.get_current_content_widget()
        tab = content if content and content.get_class().get_name() == "MordecaiInventoryWidget" else None
        logic(f"{label}: tab content is the inventory widget", tab is not None,
              f"Got {content.get_class().get_name() if content else 'None'}")
        return menu, tab

    def check_bound(label, tab):
        logic(f"{label}: tab bound to the pawn's inventory", tab.is_bound_to_inventory())
        logic(f"{label}: tab bound to the pawn's ledger", tab.is_bound_to_ledger())
        list_text = str(tab.get_list_placeholder_text())
        ledger_text = str(tab.get_ledger_placeholder_text())
        logic(f"{label}: list placeholder is not the unbound '--'", list_text != "--", f"Got '{list_text}'")
        logic(f"{label}: ledger placeholder is not the unbound '--'", ledger_text != "--", f"Got '{ledger_text}'")

    def close_menu(label):
        if subsystem.is_menu_open():
            subsystem.close_pause_menu()
        logic(f"{label}: menu closes", not subsystem.is_menu_open())

    def pooled_menu(label):
        """The closed menu CommonUI keeps in its pool (the instance the next open must reuse)."""
        found = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.MordecaiPauseMenuWidget, False)
        check(f"{label}: one pause menu instance (CommonUI pool)", len(found) == 1, f"Got {len(found)}")
        pooled = found[0] if found else None
        if pooled:
            check(f"{label}: the pooled menu is not active", not pooled.is_activated())
        return pooled

    # =======================================================================
    # Phase 1: the inventory before any pickup (issue #28 repro step 1)
    # =======================================================================
    if phase == 1:
        # The name labels as the PIE camera sees them (read-only). A TextRender's readable face
        # is its +X and its text runs along -Y, so: facing = face . -camera forward, upright =
        # text direction . camera right (1.0 each for a billboard; the old fixed rotation read
        # 0.542 and 0.000, issue #38).
        def dot(a, b):
            return a.x * b.x + a.y * b.y + a.z * b.z

        def neg(v):
            return unreal.Vector(-v.x, -v.y, -v.z)

        camera = unreal.GameplayStatics.get_player_camera_manager(world, 0)
        check("PIE player camera manager exists", camera is not None)
        if camera:
            view = camera.get_camera_rotation()
            back = neg(view.get_forward_vector())
            right = view.get_right_vector()
            print(f"  INFO: camera rotation {view}")
            for pickup in pickups:
                label = pickup.get_actor_label()
                name_label = pickup.get_editor_property('name_label')
                check(f"{label}: has a name label", name_label is not None)
                if not name_label:
                    continue
                item_name = str(pickup.get_item_display_name())
                text = str(name_label.get_editor_property('text'))
                facing = dot(name_label.get_forward_vector(), back)
                upright = dot(neg(name_label.get_right_vector()), right)
                print(f"  INFO: {label}: name label world rotation {name_label.get_world_rotation()}")
                check(f"{label}: name label visible", name_label.is_visible())
                check(f"{label}: name label reads '{item_name}'", text == item_name, f"Got '{text}'")
                check(f"{label}: name label square to the camera (facing {facing:.3f})", facing >= 0.99)
                check(f"{label}: name label reads left to right (upright {upright:.3f})", upright >= 0.99)

        try:
            menu, tab = open_inventory_tab("Open 1 (before pickups)")
            if menu:
                layer = next((s for s in unreal.ObjectIterator(unreal.CommonActivatableWidgetStack)
                              if s.get_active_widget() == menu), None)
                print(f"  INFO: Menu layer TransitionDuration = {layer.get_transition_duration() if layer else 'n/a'}")
            if tab:
                check_bound("Open 1", tab)
                list_text = str(tab.get_list_placeholder_text())
                ledger_text = str(tab.get_ledger_placeholder_text())
                logic("Open 1: 'Inventory is empty.'", list_text == "Inventory is empty.", f"Got '{list_text}'")
                logic("Open 1: 'No resources stored.'", ledger_text == "No resources stored.", f"Got '{ledger_text}'")
        finally:
            close_menu("Open 1")
        finish(next_phase="pick everything up, then reopen the pooled menu")
        return

    # =======================================================================
    # Phase 2: pick everything up, reopen the pooled menu (issue #28)
    # =======================================================================
    if phase == 2:
        pooled = pooled_menu("Before open 2")

        # --- HUD prompt widget present (AC-079.18) ---
        # Not top-level: the prompt is nested inside the HUD layout
        prompts = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.MordecaiPickupPromptWidget, False)
        prompt = prompts[0] if prompts else None
        check("Pickup prompt widget lives in the HUD", prompt is not None)
        if prompt:
            hint = str(unreal.MordecaiPickupPromptWidget.get_inventory_hint_text())
            check("Inventory assist hint is '[I] Inventory'", hint == "[I] Inventory", f"Got '{hint}'")

        # --- AC-079.6 / AC-079.12: press-to-pickup loop over all placed pickups ---
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
            logic(f"{label}: focused after walking up", focused == pickup,
                  f"Focused: {focused.get_actor_label() if focused else 'None'}")

            if prompt:
                prompt_text = str(prompt.get_prompt_text())
                logic(f"{label}: prompt shows '[F] Pick up {item_name}'",
                      prompt.is_prompt_visible() and item_name in prompt_text and prompt_text.startswith("[F]"),
                      f"Got '{prompt_text}'")

            # F-press stand-in
            picked = interaction.try_pickup_focused()
            logic(f"{label}: try_pickup_focused succeeded", bool(picked))
            if picked:
                collected += 1
            logic(f"{label}: pickup actor consumed",
                  not unreal.SystemLibrary.is_valid(pickup) or pickup.is_actor_being_destroyed())

        logic("All 6 pickups collected", collected == 6, f"Collected {collected}")

        # --- Routing: 4 flat-inventory instances + 2 auto-stored resources ---
        items = inventory.get_sorted_items()
        logic("Inventory holds 4 non-autostored instances", len(items) == 4,
              f"Got {len(items)}: {[str(i.item_definition.get_editor_property('item_id')) for i in items if i.item_definition]}")
        logic("Ledger IronOre == 1", ledger.get_resource_count("TestItem_IronOre") == 1,
              f"Got {ledger.get_resource_count('TestItem_IronOre')}")
        logic("Ledger TownStone == 1", ledger.get_resource_count("TestItem_TownStone") == 1,
              f"Got {ledger.get_resource_count('TestItem_TownStone')}")

        if prompt:
            logic("Prompt hidden after everything collected", not prompt.is_prompt_visible())

        # --- AC-079.8/.9 + US-094: reopen the pooled pause menu on the inventory tab (I-key path) ---
        try:
            menu, tab = open_inventory_tab("Open 2 (pooled, after pickups)")
            if menu and pooled:
                logic("Open 2: CommonUI reused the pooled menu instance", menu == pooled)
            if tab:
                check_bound("Open 2", tab)
                rows = tab.get_visible_row_count()
                logic("Open 2: inventory tab shows the 4 collected items", rows == 4, f"Got {rows} rows")
                ledger_rows = tab.get_ledger_rows()
                logic("Open 2: ledger panel shows 2 stored resources", len(ledger_rows) == 2, f"Got {len(ledger_rows)}")

                # --- Identify the Mysterious Amulet (AC-079.12 step 7) ---
                amulet_id = None
                for inst in inventory.get_sorted_items():
                    item_def = inst.get_editor_property('item_definition')
                    if item_def and str(item_def.get_editor_property('item_id')) == 'TestItem_MysteriousAmulet':
                        amulet_id = inst.get_editor_property('instance_id')
                        unidentified_name = str(unreal.MordecaiItemLibrary.get_display_name(inst))
                        logic("Amulet shows partial name before identify",
                              "Unidentified" in unidentified_name, f"Got '{unidentified_name}'")
                        break
                logic("Amulet instance found", amulet_id is not None)

                if amulet_id is not None:
                    identified = tab.try_identify(amulet_id)
                    logic("TryIdentify succeeded", bool(identified))
                    for inst in inventory.get_sorted_items():
                        if inst.get_editor_property('instance_id') == amulet_id:
                            new_name = str(unreal.MordecaiItemLibrary.get_display_name(inst))
                            logic("Amulet row shows identified name",
                                  new_name == "Mysterious Amulet", f"Got '{new_name}'")
                            break
                    names = [str(r.get_editor_property('display_name')) for r in tab.get_visible_rows()]
                    logic("The tab's rows show the identified name", "Mysterious Amulet" in names, f"Got {names}")
                    logic("TryIdentify no-op on identified instance",
                          tab.try_identify(amulet_id) == False)
        finally:
            close_menu("Open 2")
        finish(next_phase="change the inventory while closed, then reopen the pooled menu again")
        return

    # =======================================================================
    # Phase 3: change the inventory while closed, reopen again (US-094 AC-094.1/.2)
    # =======================================================================
    pooled = pooled_menu("Before open 3")

    sword_def = None
    for inst in inventory.get_sorted_items():
        item_def = inst.get_editor_property('item_definition')
        if item_def and str(item_def.get_editor_property('item_id')) == 'TestItem_RustySword':
            sword_def = item_def
    ore_def = unreal.load_asset('/MordecaiCore/Items/TestPickups/DA_TestItem_IronOre')
    logic("Rusty Sword and Iron Ore definitions found", sword_def is not None and ore_def is not None,
          "Phase 2 did not leave the Rusty Sword in the inventory (is this a fresh PIE session?)")
    if not (sword_def and ore_def):
        finish()
        return

    # Pickup stand-ins while the menu is closed
    new_sword = inventory.add_item(sword_def, 1)
    inventory.pickup_item(ore_def, 1)
    logic("Closed: inventory holds 5 instances", len(inventory.get_sorted_items()) == 5,
          f"Got {len(inventory.get_sorted_items())}")
    logic("Closed: ledger IronOre == 2", ledger.get_resource_count("TestItem_IronOre") == 2,
          f"Got {ledger.get_resource_count('TestItem_IronOre')}")

    try:
        menu, tab = open_inventory_tab("Open 3 (pooled, changed while closed)")
        if menu and pooled:
            logic("Open 3: CommonUI reused the pooled menu instance", menu == pooled)
        if tab:
            check_bound("Open 3", tab)
            names = [str(r.get_editor_property('display_name')) for r in tab.get_visible_rows()]
            logic("Open 3: rows show the 5 items, two Rusty Swords",
                  len(names) == 5 and names.count("Rusty Sword") == 2, f"Got {names}")
            ore = [r.get_editor_property('count') for r in tab.get_ledger_rows()
                   if str(r.get_editor_property('item_id')) == 'TestItem_IronOre']
            logic("Open 3: ledger shows Iron Ore x2", ore == [2], f"Got {ore}")

            # A change while the menu is open redraws the tab
            inventory.remove_item(new_sword, 1)
            logic("Open 3: removing an item while open updates the rows", tab.get_visible_row_count() == 4,
                  f"Got {tab.get_visible_row_count()}")

            tab.set_filter(unreal.MordecaiInventoryFilter.ARMOR)
            list_text = str(tab.get_list_placeholder_text())
            logic("Open 3: Armor filter -> 'No items match this filter.'",
                  list_text == "No items match this filter.", f"Got '{list_text}'")
            tab.set_filter(unreal.MordecaiInventoryFilter.ALL)
            logic("Open 3: All filter -> 4 rows", tab.get_visible_row_count() == 4,
                  f"Got {tab.get_visible_row_count()}")
    finally:
        close_menu("Open 3")

    finish()

_run()
del _run
