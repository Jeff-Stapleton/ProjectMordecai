# PIE Smoke Test: defenses against enemy melee (US-101, issue #53 / N32)
# Run during PIE via run_pie_verify.py (or ue_remote_exec.py, one phase per call). Any map: it only
# reads the local player pawn and its PlayerState ASC.
#
# Checks:
#   (a) the grounded (Walking) player has no Mordecai.State.Airborne on its ASC (read-only)
#   (b) the ASC holds the Block, Parry and Dodge abilities (read-only: get_all_abilities() +
#       AbilitySystemLibrary.get_gameplay_ability_from_spec_handle)
#   (c) [LOGIC] jump(): on the next phase the player is Falling and carries Mordecai.State.Airborne;
#       on a later phase it is Walking again and the tag is gone (AC-101.8: the tag exactly while
#       Falling)
# Every check that calls a gameplay function (jump(), the gravity scale below) or reads state those
# calls produced is tagged [LOGIC]: a logic/diagnostic check, never acceptance of a player-facing AC
# (CLAUDE.md Step 4b). The real jump with SpaceBar is Tier 4: Scripts/playtests/us101_defenses.py
# jump_evades.
#
# Tags: UAbilitySystemComponent::GetOwnedGameplayTags is not exposed to Python, so the tag reader
# uses the BlueprintPure UAbilitySystemComponent::GetGameplayTagCount (ScriptName
# get_gameplay_tag_count), falling back to IGameplayTagAssetInterface::HasMatchingGameplayTag.
# Lyra's own loose movement tag (Movement.Mode.Walking / Movement.Mode.Falling, set the same way)
# is the reader's positive control: if the reader cannot see that tag, no Airborne result counts,
# and the check FAILs loudly instead of reading "no Airborne" as a pass.
#
# Phases (one remote exec runs inside one editor frame; run_pie_verify.py runs the script again
# PHASE_DELAY_S = 2 s later whenever it prints PIE_NEXT_PHASE). A normal jump lands in ~0.7 s, well
# before the next phase, so phase 1 lowers the pawn's CharacterMovement GravityScale to
# LOW_GRAVITY_SCALE before jump() ([LOGIC]): the same jump, just long enough to be read on a later
# frame. The phase is derived from PIE state (an actor tag on the pawn); nothing is kept between
# runs:
#   1. no marker tag: (a), (b); then [LOGIC] gravity scale -> LOW_GRAVITY_SCALE, marker PHASE_JUMPED,
#      jump(). A dead, falling or paused player is waited for (a later phase), never judged.
#   2. PHASE_JUMPED: [LOGIC] Falling (else it says precisely that it already landed or never left the
#      ground), Movement.Mode.Falling seen (the reader still works), Mordecai.State.Airborne present.
#      Gravity restored (the class default), marker -> PHASE_LANDING.
#   3+. PHASE_LANDING: still Falling -> Airborne still present, another phase; Walking -> Airborne
#      gone, marker removed, ALL CHECKS PASSED.
# A failure restores the gravity scale and removes the marker before it raises.
#
# Usage:
#   py Scripts/run_pie_verify.py --tests defenses                  # all phases (DevTestMap)
#   py Scripts/run_pie_verify.py --map /MordecaiCore/Game/Maps/CombatTestMap --tests spawn,hud,test_maps,defenses
#   py Scripts/ue_remote_exec.py "exec(open(r'Scripts/verify_pie_defenses.py').read())"   # one phase
#
# NOTE: All PIE references are held inside _run() so they go out of scope when it returns.

import unreal

TAG_AIRBORNE = "Mordecai.State.Airborne"
TAG_WALKING = "Movement.Mode.Walking"      # Lyra (ALyraCharacter::SetMovementModeTag)
TAG_FALLING = "Movement.Mode.Falling"
DEFENSE_ABILITIES = ("MordecaiGA_Block", "MordecaiGA_Parry", "MordecaiGA_Dodge")
PHASE_JUMPED = "MordecaiVerifyDefenses_Jumped"     # actor tags on the pawn: the phase marker
PHASE_LANDING = "MordecaiVerifyDefenses_Landing"
LOW_GRAVITY_SCALE = 0.1                    # a ~420 cm/s jump then lasts ~8.6 s instead of ~0.86 s
DEFAULT_GRAVITY_SCALE = 1.0                # only if the class default cannot be read


def _run():
    pass_count = 0
    fail_count = 0

    def check(name, condition, detail="", why=""):
        """detail: printed either way; why: printed on a FAIL only."""
        nonlocal pass_count, fail_count
        if condition:
            pass_count += 1
            print(f"  PASS: {name}" + (f" -- {detail}" if detail else ""))
        else:
            fail_count += 1
            text = "; ".join(t for t in (detail, why) if t)
            print(f"  FAIL: {name}" + (f" -- {text}" if text else ""))

    def logic(name, condition, detail="", why=""):
        # Direct gameplay call (or state it produced): logic/diagnostic only
        check(f"[LOGIC] {name}", condition, detail, why)

    def finish(next_phase=None, cleanup=None):
        print("")
        print(f"RESULT: {pass_count} passed, {fail_count} failed")
        if fail_count > 0:
            if cleanup:
                cleanup()
            raise RuntimeError(f"PIE defenses smoke test failed: {fail_count} failure(s)")
        if next_phase:
            print("PHASE PASSED (not finished: run again after the editor ticks)")
            print(f"PIE_NEXT_PHASE: {next_phase}")
        else:
            print("ALL CHECKS PASSED")

    # --- gameplay tags from Python ---------------------------------------------------------------
    def tag_name(tag):
        try:
            return str(tag.get_editor_property("tag_name"))
        except Exception:
            return None

    def make_tag(name):
        """An FGameplayTag for `name`, or (None, why). In UE 5.7 TagName is read-only from Python,
        GameplayTag(str) fails and there is no request_gameplay_tag; the struct's text import works."""
        try:
            tag = unreal.GameplayTag()
            if not tag.import_text(f'(TagName="{name}")'):
                return None, "import_text returned False"
            if tag_name(tag) == name:
                return tag, None
            return None, f"import_text read back {tag_name(tag)!r}"
        except Exception as e:
            return None, f"GameplayTag.import_text: {e}"

    def tag_count(asc, tag):
        """(count or None, how): get_gameplay_tag_count, else has_matching_gameplay_tag."""
        errors = []
        try:
            return int(asc.get_gameplay_tag_count(tag)), "get_gameplay_tag_count"
        except Exception as e:
            errors.append(f"get_gameplay_tag_count: {e}")
        try:
            return (1 if asc.has_matching_gameplay_tag(tag) else 0), "has_matching_gameplay_tag"
        except Exception as e:
            errors.append(f"has_matching_gameplay_tag: {e}")
        return None, "; ".join(errors)

    # --- the pawn --------------------------------------------------------------------------------
    def actor_tags(pawn):
        try:
            return [str(t) for t in pawn.get_editor_property("tags")]
        except Exception:
            return []

    def set_marker(pawn, marker):
        keep = [t for t in actor_tags(pawn) if t not in (PHASE_JUMPED, PHASE_LANDING)]
        if marker:
            keep.append(marker)
        pawn.set_editor_property("tags", [unreal.Name(t) for t in keep])

    def default_gravity(pawn):
        try:
            cdo = unreal.get_default_object(pawn.get_class())
            return float(cdo.get_editor_property("character_movement")
                         .get_editor_property("gravity_scale")), "the class default"
        except Exception as e:
            return DEFAULT_GRAVITY_SCALE, f"{DEFAULT_GRAVITY_SCALE:g} (class default unreadable: {e})"

    def movement(pawn, cmc):
        try:
            mode = str(cmc.get_editor_property("movement_mode"))
        except Exception as e:
            mode = f"unreadable ({e})"
        try:
            falling = bool(cmc.is_falling())
        except Exception:
            falling = "FALLING" in mode.upper()
        loc = pawn.get_actor_location()
        vel = pawn.get_velocity()
        return falling, f"movement mode {mode}, z {loc.z:.0f}, velocity z {vel.z:.0f}"

    def is_dead(pawn):
        try:
            return bool(pawn.is_dead())
        except Exception:
            return False

    print("=" * 60)
    print("PIE SMOKE TEST: DEFENSES / AIRBORNE (US-101)")
    print("=" * 60)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    check("PIE world exists", world is not None, "" if world else "Start PIE before running this test")
    if not world:
        return finish()
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    check("Player controller exists", pc is not None)
    check("Player pawn exists", pawn is not None)
    if not pc or not pawn:
        return finish()
    asc = None
    try:
        asc = pc.player_state.get_lyra_ability_system_component()
    except Exception as e:
        check("PlayerState ASC readable", False, str(e))
    check("PlayerState ASC exists", asc is not None)
    cmc = None
    try:
        cmc = pawn.get_editor_property("character_movement")
    except Exception as e:
        check("CharacterMovement readable", False, str(e))
    check("CharacterMovement exists", cmc is not None)
    if not asc or not cmc:
        return finish()

    airborne, why_a = make_tag(TAG_AIRBORNE)
    walking_tag, why_w = make_tag(TAG_WALKING)
    falling_tag, why_f = make_tag(TAG_FALLING)
    check(f"FGameplayTag '{TAG_AIRBORNE}' can be made from Python", airborne is not None, why_a or "")
    check(f"FGameplayTag '{TAG_WALKING}' can be made from Python", walking_tag is not None, why_w or "")
    check(f"FGameplayTag '{TAG_FALLING}' can be made from Python", falling_tag is not None, why_f or "")
    if airborne is None or walking_tag is None or falling_tag is None:
        return finish()

    markers = actor_tags(pawn)
    original_gravity, gravity_from = default_gravity(pawn)

    def cleanup():
        try:
            cmc.set_editor_property("gravity_scale", original_gravity)
            set_marker(pawn, None)
            print(f"  (cleanup: gravity scale back to {original_gravity:g}, phase marker removed)")
        except Exception as e:
            print(f"  (cleanup failed: {e})")

    falling, how = movement(pawn, cmc)

    # --- phase 2: the jump is in progress ----------------------------------------------------------
    if PHASE_JUMPED in markers:
        print(f"  Phase 2 (jump started on the previous phase): {how}")
        logic("Falling after jump() (gravity scale lowered so it outlasts the phase delay)", falling,
              how, "the player already landed, or never left the ground, before this phase ran")
        if falling:
            n, reader = tag_count(asc, falling_tag)
            logic(f"Lyra's '{TAG_FALLING}' on the ASC while Falling (the tag reader works: {reader})",
                  n is not None and n >= 1, f"count {n}",
                  "the tag reader cannot see a tag that must be there, so the Airborne result below "
                  "does not count")
            n, reader = tag_count(asc, airborne)
            logic(f"'{TAG_AIRBORNE}' on the ASC while Falling ({reader})", n is not None and n >= 1,
                  f"count {n}", "the tag is not granted while Falling" if n is not None else
                  f"the tag count cannot be read from Python: {reader}")
        if fail_count:
            return finish(cleanup=cleanup)
        cmc.set_editor_property("gravity_scale", original_gravity)
        set_marker(pawn, PHASE_LANDING)
        print(f"  [LOGIC] gravity scale restored to {original_gravity:g} ({gravity_from}); landing next")
        return finish(next_phase="the player is falling back down: check Airborne is gone once Walking")

    # --- phase 3+: landing -----------------------------------------------------------------------
    if PHASE_LANDING in markers:
        n, reader = tag_count(asc, airborne)
        if falling:
            print(f"  Phase 3+ (landing): still falling, {how}")
            logic(f"'{TAG_AIRBORNE}' still on while Falling ({reader})", n is not None and n >= 1,
                  f"count {n}", "the tag went before the landing")
            if fail_count:
                return finish(cleanup=cleanup)
            return finish(next_phase="still falling: check again once Walking")
        print(f"  Phase 3+ (landing): landed, {how}")
        w, wreader = tag_count(asc, walking_tag)
        logic(f"Lyra's '{TAG_WALKING}' on the ASC after the landing (the tag reader works: {wreader})",
              w is not None and w >= 1, f"count {w}",
              "the tag reader cannot see a tag that must be there, so the Airborne result below "
              "does not count")
        logic(f"'{TAG_AIRBORNE}' gone after the landing ({reader})",
              n == 0 and w is not None and w >= 1, f"count {n}",
              "the tag stuck after the landing" if n else
              f"the tag count cannot be read from Python: {reader}" if n is None else
              "not judged: the tag reader failed")
        set_marker(pawn, None)
        return finish(cleanup=cleanup)

    # --- phase 1: grounded checks, then the jump ------------------------------------------------------
    paused = False
    try:
        paused = bool(unreal.GameplayStatics.is_game_paused(world))
    except Exception:
        pass
    if is_dead(pawn) or falling or paused:
        state = "dead" if is_dead(pawn) else ("falling" if falling else "the PIE world is paused")
        print(f"  WAIT: the player is not a grounded, living player in a running world ({state}; "
              f"{how}): nothing judged yet")
        return finish(next_phase=f"waiting for a grounded, living player ({state})")

    print(f"  Phase 1 (grounded): {how}")
    w, reader = tag_count(asc, walking_tag)
    reader_ok = w is not None and w >= 1
    check(f"Lyra's '{TAG_WALKING}' on the ASC of the grounded player (the tag reader works: {reader})",
          reader_ok, f"count {w}",
          "the tag reader cannot see a tag that must be there, so no Airborne result below counts"
          if w is not None else f"no tag reader works from Python: {reader}")
    n, reader = tag_count(asc, airborne)
    check(f"(a) no '{TAG_AIRBORNE}' on the grounded player's ASC ({reader})", n == 0 and reader_ok,
          f"count {n}", "the grounded player carries the tag" if n else
          "not judged: the tag reader failed")

    granted = []
    try:
        for handle in asc.get_all_abilities():
            # UAbilitySystemBlueprintLibrary's ScriptName is AbilitySystemLibrary
            ability, _ = unreal.AbilitySystemLibrary.get_gameplay_ability_from_spec_handle(asc, handle)
            if ability:
                granted.append(ability)
    except Exception as e:
        check("(b) ability specs readable", False, str(e))
    names = sorted({a.get_class().get_name() for a in granted})
    for name in DEFENSE_ABILITIES:
        native = getattr(unreal, name, None)
        has = any(c == name or c.startswith(name + "_") for c in names) or (
            native is not None and any(isinstance(a, native) for a in granted))
        check(f"(b) the ASC holds a {name} spec", has, f"granted: {names}")
    if fail_count:
        return finish()

    can_jump = True
    try:
        can_jump = bool(pawn.can_jump())
    except Exception:
        pass
    logic("can_jump() on the grounded player", can_jump)
    if fail_count:
        return finish()
    cmc.set_editor_property("gravity_scale", LOW_GRAVITY_SCALE)
    set_marker(pawn, PHASE_JUMPED)
    pawn.jump()
    print(f"  [LOGIC] jump() called with the gravity scale at {LOW_GRAVITY_SCALE:g} (was "
          f"{original_gravity:g}, {gravity_from}) so the jump outlasts the phase delay")
    return finish(next_phase="jump() called: check Falling and Airborne after the editor ticks")


_run()
del _run
