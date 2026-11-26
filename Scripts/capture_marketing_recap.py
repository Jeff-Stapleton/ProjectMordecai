#!/usr/bin/env python3
"""Capture clean US-090 and US-096 marketing media through US-099's fixed-frame path."""

import argparse
import glob
import json
import os
import shutil
import subprocess
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import agent_bridge as ab  # noqa: E402
import capture_us099_marketing as fixed  # noqa: E402
import playtest_kit as kit  # noqa: E402
from capture_us090_marketing import _sha256  # noqa: E402
from verify_marketing_media import verify_still, verify_video  # noqa: E402

FPS = 30
OUTPUT_SIZE = (1920, 1080)
MAX_FRAMES = FPS * 20
MIN_FRAMES = FPS * 5
DRAINED_SPELL_POINTS_MAX = 0.5
MAP = kit.DEFAULT_MAP
EXPECTED_EXPERIENCE = kit.DEFAULT_EXPERIENCE


def _frame_paths(frame_dir):
    return sorted(glob.glob(os.path.join(frame_dir, "frame_*.png")))


def _wait_for_frame_count(bridge, frame_dir, target, timeout_s=180.0):
    deadline = time.monotonic() + timeout_s
    while True:
        count = len(_frame_paths(frame_dir))
        if count >= target:
            return count
        if time.monotonic() >= deadline:
            raise RuntimeError(
                f"fixed-frame capture reached {count}, expected at least {target}")
        bridge.wait(0.1)


def _player(state):
    value = state.get("player") if isinstance(state, dict) else None
    return value if isinstance(value, dict) and value.get("valid") is True else {}


def _attrs(state):
    value = _player(state).get("attributes")
    return value if isinstance(value, dict) else {}


def _spell_points_are_drained(state):
    value = _attrs(state).get("spell_points")
    return (
        isinstance(value, (int, float))
        and value <= DRAINED_SPELL_POINTS_MAX
    )


def _weapon(state):
    value = _player(state).get("weapon")
    return value if isinstance(value, dict) else {}


def _visible_weapon_plate(state, expected):
    wanted = str(expected).casefold()
    return any(
        str(widget.get("class") or "") == "MordecaiEquippedWeaponWidget"
        and widget.get("visible", True) is not False
        and any(str(text).casefold() == wanted for text in (widget.get("texts") or []))
        for widget in (state.get("widgets") or [])
        if isinstance(widget, dict)
    )


def _presentation_proof(state, phase):
    proof = fixed._marketing_label_proof(state)
    proof["phase"] = phase
    if not proof["passed"]:
        raise RuntimeError(f"marketing presentation verifier failed at {phase}: {proof}")
    return proof


def _record_event(bridge, frame_dir, event, state=None):
    state = state or bridge.state(
        ["game", "player", "actors", "widgets"], radius=3000)
    before = len(_frame_paths(frame_dir))
    frame = _wait_for_frame_count(bridge, frame_dir, min(before + 2, MAX_FRAMES))
    player = _player(state)
    attrs = _attrs(state)
    return {
        "event": event,
        "source_frame": frame,
        "world_time_s": (state.get("game") or {}).get("world_time_s"),
        "spell_points": attrs.get("spell_points"),
        "max_spell_points": attrs.get("max_spell_points"),
        "health": attrs.get("health"),
        "max_health": attrs.get("max_health"),
        "is_dead": player.get("is_dead"),
        "weapon": _weapon(state).get("display_name"),
        "weapon_index": _weapon(state).get("index"),
        "weapon_plate_visible": _visible_weapon_plate(
            state, _weapon(state).get("display_name")),
    }


def _capture_args(frame_dir):
    exec_commands = ab.pb.quote_value(
        "ShowFlag.TextRender 0,t.MaxFPS 30,r.SetRes 1920x1080f")
    return (
        "-MordecaiMarketingCapture",
        "-MordecaiMarketingFreezeAIUntilFrames",
        "-NOSCREENMESSAGES",
        f"-ResX={OUTPUT_SIZE[0]}",
        f"-ResY={OUTPUT_SIZE[1]}",
        "-WINDOWED",
        f"-ExecCmds={exec_commands}",
        "-BENCHMARK",
        f"-FPS={FPS}",
        "-MarketingFrameCapture",
        fixed._marketing_frame_dir_argument(frame_dir),
        f"-MarketingFrameLimit={MAX_FRAMES}",
    )


def _drive_spell_points(bridge, frame_dir):
    events = []
    proofs = []
    state = bridge.state(
        ["game", "player", "actors", "widgets"], radius=3000)
    proofs.append(_presentation_proof(state, "start"))
    attrs = _attrs(state)
    if attrs.get("spell_points") != attrs.get("max_spell_points"):
        raise RuntimeError(f"spell points did not start full: {attrs}")
    events.append(_record_event(bridge, frame_dir, "full", state))

    for key, maximum, event in (
        ("Q", 7.25, "fireball_spend"),
        ("R", 3.5, "stone_skin_spend"),
    ):
        bridge.tap(key, hold_s=0.1)
        state = bridge.wait_for(
            lambda value, cap=maximum: (
                isinstance(_attrs(value).get("spell_points"), (int, float))
                and _attrs(value)["spell_points"] <= cap
            ),
            timeout_s=20.0,
            sections=["game", "player", "actors", "widgets"],
            poll_s=0.02,
        )
        events.append(_record_event(bridge, frame_dir, event, state))

    state = bridge.wait_for(
        lambda value: (
            isinstance(_attrs(value).get("health"), (int, float))
            and isinstance(_attrs(value).get("max_health"), (int, float))
            and _attrs(value)["health"] < _attrs(value)["max_health"]
            and _player(value).get("is_dead") is not True
        ),
        timeout_s=30.0,
        sections=["game", "player", "actors", "widgets"],
        poll_s=0.02,
    )
    bridge.tap("One", hold_s=0.1)
    state = bridge.wait_for(
        _spell_points_are_drained,
        timeout_s=20.0,
        sections=["game", "player", "actors", "widgets"],
        poll_s=0.02,
    )
    events.append(_record_event(bridge, frame_dir, "drained", state))
    drained = float(_attrs(state)["spell_points"])

    state = bridge.wait_for(
        lambda value: (
            isinstance(_attrs(value).get("spell_points"), (int, float))
            and drained + 0.24 <= _attrs(value)["spell_points"] <= drained + 0.26
            and _player(value).get("is_dead") is False
        ),
        timeout_s=30.0,
        sections=["game", "player", "actors", "widgets"],
        poll_s=0.02,
    )
    events.append(_record_event(bridge, frame_dir, "regen_tick_1", state))
    state = bridge.wait_for(
        lambda value: (
            isinstance(_attrs(value).get("spell_points"), (int, float))
            and drained + 0.49 <= _attrs(value)["spell_points"] <= drained + 0.51
            and _player(value).get("is_dead") is False
        ),
        timeout_s=30.0,
        sections=["game", "player", "actors", "widgets"],
        poll_s=0.02,
    )
    events.append(_record_event(bridge, frame_dir, "slow_regen", state))

    state = bridge.wait_for(
        lambda value: _player(value).get("is_dead") is True,
        timeout_s=90.0,
        sections=["game", "player", "actors", "widgets"],
        poll_s=0.02,
    )
    events.append(_record_event(bridge, frame_dir, "dead", state))
    state = bridge.wait_for(
        lambda value: (
            _player(value).get("is_dead") is False
            and _attrs(value).get("spell_points")
            == _attrs(value).get("max_spell_points")
        ),
        timeout_s=30.0,
        sections=["game", "player", "actors", "widgets"],
        poll_s=0.02,
    )
    events.append(_record_event(bridge, frame_dir, "respawn_refill", state))
    proofs.append(_presentation_proof(state, "end"))
    return events, proofs


def _cycle(bridge, modifier, expected_index):
    if modifier:
        bridge.press(modifier)
        try:
            bridge.wait(0.05)
            bridge.tap("Tab", hold_s=0.1)
        finally:
            bridge.release(modifier)
    else:
        bridge.tap("Tab", hold_s=0.1)
    return bridge.wait_for(
        lambda value: _weapon(value).get("index") == expected_index
        and _visible_weapon_plate(value, _weapon(value).get("display_name")),
        timeout_s=10.0,
        sections=["game", "player", "actors", "widgets"],
        poll_s=0.02,
    )


def _drive_weapon_cycle(bridge, frame_dir):
    events = []
    proofs = []
    movement_key = "W"
    state = bridge.state(
        ["game", "player", "actors", "widgets"], radius=3000)
    proofs.append(_presentation_proof(state, "start"))
    if _weapon(state).get("display_name") != "Longsword":
        raise RuntimeError(f"weapon capture did not start on Longsword: {_weapon(state)}")
    if not _visible_weapon_plate(state, "Longsword"):
        raise RuntimeError("Longsword is not visible in the equipped-weapon HUD plate")

    try:
        bridge.press(movement_key)
        start = len(_frame_paths(frame_dir))
        _wait_for_frame_count(bridge, frame_dir, start + 20)
        events.append(_record_event(bridge, frame_dir, "longsword_start", state))

        for order, (modifier, expected_index, expected_name) in enumerate((
            (None, 1, "Dagger"),
            (None, 2, "Greatsword"),
            ("LeftShift", 1, "Dagger"),
            ("LeftShift", 0, "Longsword"),
        ), 1):
            state = _cycle(bridge, modifier, expected_index)
            if _weapon(state).get("display_name") != expected_name:
                raise RuntimeError(
                    f"cycle {order} reached {_weapon(state)}, expected {expected_name}")
            _wait_for_frame_count(
                bridge, frame_dir, len(_frame_paths(frame_dir)) + 30)
            events.append(_record_event(
                bridge, frame_dir,
                ("previous_" if modifier else "next_") + expected_name.lower(),
                state,
            ))
            if order == 2:
                bridge.release(movement_key)
                movement_key = "S"
                bridge.press(movement_key)

        _wait_for_frame_count(
            bridge, frame_dir, max(MIN_FRAMES, len(_frame_paths(frame_dir)) + 15))
    finally:
        try:
            bridge.release(movement_key)
        except Exception:
            pass

    state = bridge.state(
        ["game", "player", "actors", "widgets"], radius=3000)
    proofs.append(_presentation_proof(state, "end"))
    return events, proofs


def _encode_outputs(output_dir, frame_dir, clip_name, selected_frames=None):
    frames = _frame_paths(frame_dir)
    problems = fixed._validate_source_frame_sequence(frames)
    if len(frames) < MIN_FRAMES or len(frames) > MAX_FRAMES:
        problems.append(
            f"fixed-frame count {len(frames)} is outside {MIN_FRAMES}-{MAX_FRAMES}")
    if problems:
        raise RuntimeError("fixed-frame source contract failed: " + "; ".join(problems))

    encoded_frames = selected_frames or frames
    if len(encoded_frames) < MIN_FRAMES or len(encoded_frames) > MAX_FRAMES:
        raise RuntimeError(
            f"edited frame count {len(encoded_frames)} is outside "
            f"{MIN_FRAMES}-{MAX_FRAMES}")
    encode_dir = os.path.join(output_dir, "encode-frames")
    os.makedirs(encode_dir)
    for index, source in enumerate(encoded_frames, 1):
        shutil.copy2(source, os.path.join(encode_dir, f"frame_{index:06d}.png"))

    source_movie = os.path.join(output_dir, "continuous-source.mp4")
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error",
        "-framerate", str(FPS), "-start_number", "1",
        "-i", os.path.join(encode_dir, "frame_%06d.png"),
        "-an", "-c:v", "libx264", "-preset", "ultrafast", "-crf", "12",
        "-pix_fmt", "yuv420p", source_movie,
    ])
    clip = os.path.join(output_dir, clip_name)
    duration = len(encoded_frames) / FPS
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error", "-i", source_movie,
        "-t", f"{duration:.3f}", "-an",
        "-vf", f"scale={OUTPUT_SIZE[0]}:{OUTPUT_SIZE[1]}",
        "-r", str(FPS), "-c:v", "libx264", "-preset", "medium",
        "-crf", "18", "-pix_fmt", "yuv420p", clip,
    ])
    source_manifest = fixed._make_source_manifest(source_movie, output_dir)
    shutil.rmtree(encode_dir, ignore_errors=True)
    return frames, encoded_frames, source_movie, clip, source_manifest


def _copy_stills(output_dir, frames, events, selections):
    stills = []
    by_event = {event["event"]: event for event in events}
    for order, (event_name, suffix, caption) in enumerate(selections, 1):
        event = by_event[event_name]
        index = max(1, min(int(event["source_frame"]), len(frames)))
        filename = f"still_{order:02d}_{suffix}.png"
        target = os.path.join(output_dir, filename)
        shutil.copy2(frames[index - 1], target)
        stills.append({
            "file": filename,
            "source_frame": index,
            "caption": caption,
            "verification": verify_still(target),
        })
    return stills


def capture_feature(output_dir, feature):
    output_dir = os.path.abspath(output_dir)
    fixed._require_empty_output_dir(output_dir)
    frame_dir = os.path.join(output_dir, "continuous-source")
    session_dir = os.path.join(output_dir, "session")
    os.makedirs(frame_dir)
    os.makedirs(session_dir)

    drive = _drive_spell_points if feature == "us090" else _drive_weapon_cycle
    clip_name = (
        "spell-points-drain-regen-respawn.mp4"
        if feature == "us090"
        else "weapon-cycling-forward-and-back.mp4"
    )
    bridge = None
    try:
        bridge = ab.Bridge.launch(
            map=MAP,
            expect_experience=EXPECTED_EXPERIENCE,
            run_suffix=f"{feature}cleanmarketing",
            run_dir=session_dir,
            extra_args=_capture_args(frame_dir),
        )
        events, proofs = drive(bridge, frame_dir)
        bridge.quit()
        bridge = None
    finally:
        if bridge is not None:
            bridge.close()

    raw_frames = _frame_paths(frame_dir)
    selected_frames = None
    edit_caveat = "No editorial frame cut."
    if feature == "us090":
        by_event = {event["event"]: event for event in events}
        death_frame = int(by_event["dead"]["source_frame"])
        respawn_frame = int(by_event["respawn_refill"]["source_frame"])
        # The death screen is intentionally static for three seconds. Keep the live
        # combat up to the death transition, then cut to the rendered respawn frames
        # so the postable clip retains >=90% unique rendered frames.
        live_end = max(MIN_FRAMES, death_frame - 8)
        respawn_start = max(live_end + 1, respawn_frame - 7)
        selected_frames = (
            raw_frames[:live_end]
            + raw_frames[respawn_start - 1:respawn_frame + 2]
        )
        edit_caveat = (
            f"One disclosed cut removes the static death-screen wait: raw frames "
            f"1-{live_end}, then {respawn_start}-{min(respawn_frame + 2, len(raw_frames))}. "
            "Every retained frame is an original rendered Unreal frame."
        )
    frames, encoded_frames, source_movie, clip, source_manifest = _encode_outputs(
        output_dir, frame_dir, clip_name, selected_frames)
    if feature == "us090":
        stills = _copy_stills(output_dir, frames, events, (
            ("full", "full", "Full spell-point bar before casting."),
            ("drained", "drained", "Spell-point bar drained after three casts."),
            ("slow_regen", "regen", "Spell-point bar partially restored by slow regeneration."),
            ("respawn_refill", "respawn", "Spell-point bar full after respawn."),
        ))
        description = (
            "The player casts Fireball, Stone Skin and Restoration to drain spell points, "
            "the bar gains two quarter-point regeneration ticks, enemies kill the player, "
            "and respawn refills the bar.")
    else:
        stills = _copy_stills(output_dir, frames, events, (
            ("longsword_start", "longsword", "Longsword shown in the weapon HUD plate."),
            ("next_dagger", "dagger", "Dagger shown after Tab."),
            ("next_greatsword", "greatsword", "Greatsword shown after Tab."),
            ("previous_longsword", "longsword_return", "Longsword shown after Shift+Tab cycling back."),
        ))
        description = (
            "The player uses Tab from Longsword to Dagger to Greatsword, then Shift+Tab "
            "back through Dagger to Longsword; the weapon HUD plate updates each time.")

    video_check = verify_video(clip, source_manifest)
    content = {
        "video": video_check,
        "stills": stills,
        "game_owned_source": True,
        "hud_required": True,
        "lock_screen_rejected_by_game_owned_source_and_hud": True,
        "passed": video_check["passed"]
                  and all(still["verification"]["passed"] for still in stills),
    }
    with open(os.path.join(output_dir, "content-check.json"), "w", encoding="utf-8") as stream:
        json.dump(content, stream, indent=2)
        stream.write("\n")
    with open(os.path.join(output_dir, "label-proof.json"), "w", encoding="utf-8") as stream:
        json.dump({
            "proofs": proofs,
            "passed": all(proof["passed"] for proof in proofs),
        }, stream, indent=2)
        stream.write("\n")
    with open(os.path.join(output_dir, "state-timeline.json"), "w", encoding="utf-8") as stream:
        json.dump({"events": events}, stream, indent=2)
        stream.write("\n")
    if not content["passed"]:
        raise RuntimeError(f"{feature} content verification failed: {content}")

    metadata = video_check["metadata"]
    decimate = video_check["mpdecimate"]
    result = {
        "feature": feature,
        "map": MAP,
        "clip": os.path.basename(clip),
        "clip_sha256": _sha256(clip),
        "source_movie": os.path.basename(source_movie),
        "source_movie_sha256": _sha256(source_movie),
        "duration_seconds": metadata["duration_seconds"],
        "fps": metadata["frame_rate"],
        "frame_count": metadata["frame_count"],
        "raw_frame_count": len(frames),
        "retained_rendered_frame_count": len(encoded_frames),
        "edit_caveat": edit_caveat,
        "mpdecimate_kept_frames": decimate["kept_frames"],
        "unique_frame_ratio": decimate["unique_frame_ratio"],
        "stills": [still["file"] for still in stills],
        "enemies_frozen": False,
        "ai_frozen_before_numbered_frames": True,
        "content_check_passed": True,
    }
    with open(os.path.join(output_dir, "capture-result.json"), "w", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")

    with open(os.path.join(output_dir, "manifest.md"), "w", encoding="utf-8") as stream:
        stream.write(
            f"# {feature.upper()} clean marketing recapture\n\n"
            f"{description}\n\n"
            f"- Map: `{MAP}`. DevTestMap is used because the existing dressed "
            "`L_VW_TileTest` terrain fixture has no enemies; the existing maps with "
            "working enemies are grey test fixtures. No new content was built.\n"
            f"- Clip: `{os.path.basename(clip)}` — {metadata['duration_seconds']:.3f} s, "
            f"{metadata['frame_rate']} fps, {metadata['width']}x{metadata['height']}, "
            f"{metadata['frame_count']} frames.\n"
            f"- Unique frames: ffmpeg mpdecimate kept {decimate['kept_frames']} / "
            f"{decimate['input_frames']} ({decimate['unique_frame_ratio']:.3%}; "
            "required at least 90%).\n"
            "- Capture: game-owned, contiguous fixed-timestep frames from the US-099 "
            "`-MarketingFrameCapture -BENCHMARK -FPS=30` path; no desktop capture.\n"
            "- HUD: on. Debug draws, facing arrows and on-screen messages: off and "
            "verified from runtime state plus scripted green/yellow pixel checks.\n"
            "- Loading screen: absent from numbered frames after the built-in warm-up.\n"
            "- AI: frozen only while loading and warming up, then automatically released "
            "before numbered frame 1; AI is active throughout retained footage. The "
            "full-session `-MordecaiMarketingFreezeAI` flag was not used.\n"
            f"- Edit: {edit_caveat}\n"
            "- Caveat: this is postable marketing media, not acceptance evidence.\n\n"
            "## Stills\n\n"
        )
        for still in stills:
            stream.write(f"- `{still['file']}`: {still['caption']}\n")

    shutil.rmtree(frame_dir, ignore_errors=True)
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", required=True)
    parser.add_argument("--feature", choices=("us090", "us096", "all"), default="all")
    args = parser.parse_args(argv)
    selected = ("us090", "us096") if args.feature == "all" else (args.feature,)
    results = {}
    for feature in selected:
        results[feature] = capture_feature(
            os.path.join(os.path.abspath(args.output_root), feature), feature)
    print(json.dumps(results, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
