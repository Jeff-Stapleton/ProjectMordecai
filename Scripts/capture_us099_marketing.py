#!/usr/bin/env python3
"""Capture smooth, non-acceptance US-099 wall-fade marketing media in the packaged game."""

import argparse
import glob
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from capture_us090_marketing import _label_proof, _sha256  # noqa: E402
from verify_marketing_media import decode_motion_frames, frame_psnr, verify_video  # noqa: E402

FPS = 30
OUTPUT_SIZE = (1920, 1080)
WALL_NAME = "Arena_Wall_W"
MIN_UNIQUE_FPS = 25.0
MIN_CLIP_SECONDS = 5.0
MAX_CLIP_SECONDS = 20.0


def _occluder(state, name=WALL_NAME):
    camera = state.get("camera") if isinstance(state, dict) else None
    occluders = camera.get("occluders") if isinstance(camera, dict) else None
    wanted = name.casefold()
    if not isinstance(occluders, list):
        return None
    return next(
        (entry for entry in occluders
         if isinstance(entry, dict) and str(entry.get("name", "")).casefold() == wanted),
        None,
    )


def _marketing_label_proof(state):
    """Account for TextRender being disabled at the viewport show-flag level."""
    proof = _label_proof(state)
    visible_huds = [
        widget for widget in (state.get("widgets") or [])
        if isinstance(widget, dict)
        and any(
            marker in str(widget.get("class") or "")
            for marker in ("MordecaiCombatHUDWidget", "WBP_CombatHUD")
        )
        and widget.get("visible", True) is not False
    ]
    proof["text_label_components_suppressed_by_show_flag"] = proof.pop(
        "text_label_violations", [])
    proof["text_label_violations"] = []
    proof["combat_hud_widgets"] = visible_huds
    proof["combat_hud_visible"] = bool(visible_huds)
    proof["text_render_show_flag"] = False
    proof["passed"] = bool(visible_huds) and not any((
        proof["actor_widget_violations"],
        proof["rendered_actor_name_violations"],
        proof["enemy_indicator_widgets"],
        proof["debug_text_violations"],
    ))
    return proof


def _capture_still(bridge, output_dir, filename, expected):
    path = os.path.join(output_dir, filename)
    bridge.screenshot(path)
    # Read state after the screenshot has completed. In particular, never attach the
    # pre-request transition state to a frame that may have landed after the fade.
    state = bridge.state(
        ["game", "player", "camera", "actors", "widgets"], radius=3000)
    with Image.open(path) as image:
        size = image.size
    return {
        "file": filename,
        "sha256": _sha256(path),
        "width": size[0],
        "height": size[1],
        "expected": expected,
        "player": (state.get("player") or {}).get("location"),
        "west_wall": _occluder(state),
    }


def _validate_stills(stills):
    problems = []
    if len(stills) not in range(3, 7):
        problems.append(f"still count {len(stills)} is outside 3-6")
    for entry in stills:
        if (entry["width"], entry["height"]) != OUTPUT_SIZE:
            problems.append(
                f"{entry['file']} is {entry['width']}x{entry['height']}, not "
                f"{OUTPUT_SIZE[0]}x{OUTPUT_SIZE[1]}")
    fades = [
        float((entry.get("west_wall") or {}).get("fade", -1.0))
        for entry in stills
    ]
    if fades and fades[0] < 0.999:
        problems.append(f"approach still west-wall fade is {fades[0]:.3f}, not opaque")
    if len(fades) > 1 and not 0.05 < fades[1] < 0.95:
        problems.append(f"mid-fade still state is {fades[1]:.3f}, not transitional")
    if len(fades) > 2 and fades[2] > 0.05:
        problems.append(f"fully-faded still state is {fades[2]:.3f}, not <= 0.05")
    if len(fades) > 3 and fades[3] < 0.999:
        problems.append(f"restored still state is {fades[3]:.3f}, not >= 0.999")
    player = stills[2].get("player") if len(stills) > 2 else None
    if not isinstance(player, list) or not player or float(player[0]) > -915.0:
        problems.append(f"fully-faded still player is not at x<=-915: {player}")
    return problems


def _validate_source_frame_sequence(paths):
    """Reject dropped ticks instead of compacting the surviving screenshots."""
    problems = []
    numbered = {}
    for path in paths:
        match = re.fullmatch(r"frame_(\d{6})\.png", os.path.basename(path))
        if not match:
            problems.append(f"unexpected source frame name: {os.path.basename(path)}")
            continue
        numbered[int(match.group(1))] = path
    if not numbered:
        return problems + ["no source frames"]
    for index in range(1, max(numbered) + 1):
        if index not in numbered:
            problems.append(f"missing frame_{index:06d}.png")
    return problems


def _marketing_frame_dir_argument(path):
    return f"-MarketingFrameDir={ab.pb.quote_value(path)}"


def _require_empty_output_dir(output_dir):
    if os.path.isdir(output_dir) and os.listdir(output_dir):
        raise RuntimeError(
            f"marketing output directory must be empty: {output_dir}")
    if os.path.exists(output_dir) and not os.path.isdir(output_dir):
        raise RuntimeError(
            f"marketing output path is not a directory: {output_dir}")
    os.makedirs(output_dir, exist_ok=True)


def _best_alignment(frames, reference, start=0):
    scores = [frame_psnr(frame, reference) for frame in frames[start:]]
    if not scores:
        return None, 0.0
    relative = max(range(len(scores)), key=scores.__getitem__)
    return start + relative, scores[relative]


def _encode_clip(source_avi, first_still, last_still, clip):
    samples = decode_motion_frames(source_avi, sample_fps=10, size=(320, 180))
    with Image.open(first_still) as image:
        first = image.convert("RGB").resize((320, 180))
    with Image.open(last_still) as image:
        last = image.convert("RGB").resize((320, 180))
    first_index, first_psnr = _best_alignment(samples, first)
    if first_index is None or first_psnr < 15.0:
        raise RuntimeError(
            f"could not align continuous source to approach still (PSNR {first_psnr:.3f})")
    last_index, last_psnr = _best_alignment(samples, last, first_index + 1)
    if last_index is None or last_psnr < 15.0:
        raise RuntimeError(
            f"could not align continuous source to restored still (PSNR {last_psnr:.3f})")

    start_seconds = first_index / 10.0
    duration_seconds = (last_index - first_index + 1) / 10.0
    if duration_seconds < MIN_CLIP_SECONDS:
        raise RuntimeError(
            f"aligned wall-fade sequence is only {duration_seconds:.3f}s; need >= "
            f"{MIN_CLIP_SECONDS:g}s of genuine continuous footage")
    duration_seconds = min(duration_seconds, MAX_CLIP_SECONDS)
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error",
        "-ss", f"{start_seconds:.3f}", "-i", source_avi,
        "-t", f"{duration_seconds:.3f}", "-an",
        "-vf", f"scale={OUTPUT_SIZE[0]}:{OUTPUT_SIZE[1]}",
        "-r", str(FPS), "-c:v", "libx264", "-preset", "medium",
        "-crf", "18", "-pix_fmt", "yuv420p", clip,
    ])
    return {
        "source_sha256": _sha256(source_avi),
        "source_file": os.path.basename(source_avi),
        "alignment_start_seconds": round(start_seconds, 3),
        "approach_alignment_psnr_db": round(first_psnr, 3),
        "restored_alignment_psnr_db": round(last_psnr, 3),
        "duration_seconds": round(duration_seconds, 3),
    }


def _make_source_manifest(source_movie, output_dir):
    frame_dir = os.path.join(output_dir, "source-frames")
    os.makedirs(frame_dir, exist_ok=True)
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error", "-i", source_movie,
        "-vf", f"fps={FPS},scale=960:540", "-q:v", "3",
        os.path.join(frame_dir, "frame_%04d.jpg"),
    ])
    frames = []
    for index, path in enumerate(sorted(glob.glob(os.path.join(frame_dir, "*.jpg"))), 1):
        with Image.open(path) as image:
            width, height = image.size
        frames.append({
            "index": index,
            "file": os.path.relpath(path, output_dir).replace("\\", "/"),
            "sha256": _sha256(path),
            "width": width,
            "height": height,
        })
    manifest = {
        "version": 1,
        "capture": (
            "30 fps samples decoded from the retained continuous-source.mp4, "
            "which was encoded from the validated contiguous fixed-timestep Unreal PNGs"
        ),
        "source_movie": os.path.basename(source_movie),
        "source_movie_sha256": _sha256(source_movie),
        "frames": frames,
    }
    path = os.path.join(output_dir, "source-frame-manifest.json")
    with open(path, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, indent=2)
        stream.write("\n")
    return path


def _mpdecimate_check(clip, duration_seconds):
    output = subprocess.check_output([
        "ffmpeg", "-v", "error", "-i", clip, "-vf", "mpdecimate",
        "-an", "-f", "framehash", "-",
    ], text=True, stderr=subprocess.STDOUT)
    kept = sum(
        1 for line in output.splitlines()
        if line and not line.startswith("#") and re.match(r"^\s*0\s*,", line)
    )
    unique_fps = kept / duration_seconds if duration_seconds > 0 else 0.0
    return {
        "method": "ffmpeg mpdecimate -> framehash kept-frame count",
        "kept_frames": kept,
        "duration_seconds": duration_seconds,
        "unique_frames_per_second": round(unique_fps, 3),
        "required_unique_frames_per_second": MIN_UNIQUE_FPS,
        "passed": unique_fps >= MIN_UNIQUE_FPS,
    }


def _capture_extra_args(movie_dir=None):
    capture_exec_commands = ab.pb.quote_value(
        "Mordecai.Debug.DrawMeleeHitShapes 0,"
        "ShowFlag.TextRender 0,t.MaxFPS 30,r.SetRes 1920x1080f")
    args = [
        "-MordecaiMarketingCapture",
        "-NOSCREENMESSAGES",
        f"-ResX={OUTPUT_SIZE[0]}",
        f"-ResY={OUTPUT_SIZE[1]}",
        "-WINDOWED",
        f"-ExecCmds={capture_exec_commands}",
    ]
    if movie_dir is not None:
        args.extend((
            "-BENCHMARK",
            f"-FPS={FPS}",
            "-MarketingFrameCapture",
            _marketing_frame_dir_argument(movie_dir),
            "-MarketingFrameLimit=1200",
        ))
    return tuple(args)


def _launch_capture_session(run_dir, movie_dir=None):
    return ab.Bridge.launch(
        map=kit.DEFAULT_MAP,
        expect_experience=kit.DEFAULT_EXPERIENCE,
        run_suffix="us099marketing",
        run_dir=run_dir,
        extra_args=_capture_extra_args(movie_dir),
    )


def _drive_wall_fade_route(bridge, output_dir=None, phase_prefix="capture"):
    """Drive one complete fade cycle; stills are optional to keep the movie uninterrupted."""
    stills = []
    proofs = []
    route_states = []
    movement_held = False
    try:
        state = bridge.state(
            ["game", "player", "camera", "actors", "widgets"], radius=3000)
        proof = _marketing_label_proof(state)
        proof["phase"] = f"{phase_prefix}_start"
        proofs.append(proof)
        if not proof["passed"]:
            raise RuntimeError(
                f"HUD/label presentation contract failed at {phase_prefix} start: {proof}")
        if output_dir is not None:
            stills.append(_capture_still(
                bridge, output_dir, "still_01_approach.png",
                "the player beginning the approach from the arena centre with HUD visible"))

        bridge.tap("LeftShift")
        bridge.walk_to(-760.0, 0.0, tol=20.0, timeout_s=90.0)
        bridge.walk_to(-760.0, 935.0, tol=20.0, timeout_s=90.0)
        bridge.press("S")
        movement_held = True
        mid_state = bridge.wait_for(
            lambda value: (
                _occluder(value) is not None
                and 0.35 <= float(_occluder(value).get("fade", 1.0)) <= 0.65
            ),
            timeout_s=90.0,
            sections=["camera", "player"],
            poll_s=0.01,
        )
        bridge.release("S")
        movement_held = False
        route_states.append({
            "phase": "mid_fade_observed",
            "player": (mid_state.get("player") or {}).get("location"),
            "west_wall": _occluder(mid_state),
        })
        if output_dir is not None:
            mid_still = _capture_still(
                bridge, output_dir, "still_02_mid_fade.png",
                "the west wall in its dithered transition while the player enters occlusion")
            stills.append(mid_still)
            mid_fade = float((mid_still.get("west_wall") or {}).get("fade", -1.0))
            if not 0.05 < mid_fade < 0.95:
                raise RuntimeError(
                    f"mid-fade screenshot completed after transition: fade={mid_fade:.3f}")

        faded_state = bridge.wait_for(
            lambda value: (
                _occluder(value) is not None
                and float(_occluder(value).get("fade", 1.0)) <= 0.05
                and float((value.get("player") or {}).get("location", [0])[0]) <= -915.0
            ),
            timeout_s=30.0,
            sections=["camera", "player"],
            poll_s=0.01,
        )
        route_states.append({
            "phase": "fully_faded_observed",
            "player": (faded_state.get("player") or {}).get("location"),
            "west_wall": _occluder(faded_state),
        })
        if output_dir is not None:
            stills.append(_capture_still(
                bridge, output_dir, "still_03_fully_faded.png",
                "the west wall fully faded with the whole player clear and HUD visible"))

        bridge.tap("LeftShift")
        bridge.walk_to(-935.0, 0.0, tol=20.0, timeout_s=90.0)
        bridge.walk_to(0.0, 0.0, tol=20.0, timeout_s=90.0)
        restored_state = bridge.wait_for(
            lambda value: (
                _occluder(value) is not None
                and float(_occluder(value).get("fade", 0.0)) >= 0.999
            ),
            timeout_s=30.0,
            sections=["camera", "player"],
            poll_s=0.01,
        )
        route_states.append({
            "phase": "restored_observed",
            "player": (restored_state.get("player") or {}).get("location"),
            "west_wall": _occluder(restored_state),
        })
        if output_dir is not None:
            stills.append(_capture_still(
                bridge, output_dir, "still_04_restored.png",
                "the player back at centre with the fully restored west wall still in frame"))

        final = bridge.state(
            ["game", "player", "camera", "actors", "widgets"], radius=3000)
        proof = _marketing_label_proof(final)
        proof["phase"] = f"{phase_prefix}_end"
        proofs.append(proof)
        if not proof["passed"]:
            raise RuntimeError(
                f"HUD/label presentation contract failed at {phase_prefix} end: {proof}")
        return stills, proofs, route_states
    finally:
        if movement_held:
            try:
                bridge.release("S")
            except Exception:
                pass


def capture(output_dir):
    output_dir = os.path.abspath(output_dir)
    _require_empty_output_dir(output_dir)
    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        raise RuntimeError("ffmpeg and ffprobe are required")
    continuous_session_dir = os.path.join(output_dir, "continuous-session")
    still_session_dir = os.path.join(output_dir, "stills-session")
    movie_dir = os.path.join(output_dir, "continuous-source")
    os.makedirs(continuous_session_dir)
    os.makedirs(still_session_dir)
    os.makedirs(movie_dir, exist_ok=True)

    bridge = None
    proofs = []
    route_states = []
    try:
        # This session never asks Agent Bridge for a screenshot. Every fixed-timestep tick
        # belongs to the continuous movie, so no UI still can steal or compact a frame.
        bridge = _launch_capture_session(
            continuous_session_dir, movie_dir=movie_dir)
        _, continuous_proofs, continuous_route = _drive_wall_fade_route(
            bridge, phase_prefix="continuous")
        proofs.extend(continuous_proofs)
        route_states.extend(continuous_route)
        bridge.quit()
        bridge = None
    finally:
        if bridge is not None:
            bridge.close()

    source_frames = sorted(glob.glob(os.path.join(movie_dir, "frame_*.png")))
    source_problems = _validate_source_frame_sequence(source_frames)
    if len(source_frames) < FPS * MIN_CLIP_SECONDS:
        source_problems.append(
            f"expected at least {FPS * MIN_CLIP_SECONDS:g} fixed-timestep Unreal frames, "
            f"found {len(source_frames)}")
    if source_problems:
        raise RuntimeError(
            "continuous source has dropped/invalid fixed-timestep frames: "
            + "; ".join(source_problems))

    # Stills use a second fixed-timestep session. This leaves the movie contiguous and lets
    # the bridge's screenshot complete before its corresponding fade state is recorded.
    bridge = None
    try:
        bridge = _launch_capture_session(still_session_dir)
        stills, still_proofs, still_route = _drive_wall_fade_route(
            bridge, output_dir=output_dir, phase_prefix="stills")
        proofs.extend(still_proofs)
        route_states.extend(still_route)
        still_problems = _validate_stills(stills)
        if still_problems:
            raise RuntimeError(
                "still capture contract failed: " + "; ".join(still_problems))
        bridge.quit()
        bridge = None
    finally:
        if bridge is not None:
            bridge.close()

    source_movie = os.path.join(output_dir, "continuous-source.mp4")
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error",
        "-framerate", str(FPS), "-start_number", "1",
        "-i", os.path.join(movie_dir, "frame_%06d.png"),
        "-an", "-c:v", "libx264", "-preset", "ultrafast", "-crf", "12",
        "-pix_fmt", "yuv420p", source_movie,
    ])
    clip = os.path.join(output_dir, "wall-fade-and-restore.mp4")
    continuous = _encode_clip(
        source_movie,
        os.path.join(output_dir, stills[0]["file"]),
        os.path.join(output_dir, stills[-1]["file"]),
        clip,
    )
    continuous["source_frame_count"] = len(source_frames)
    continuous["source_frame_sequence_contiguous"] = True
    continuous["source_movie_retained"] = True
    source_manifest = _make_source_manifest(source_movie, output_dir)
    content = verify_video(clip, source_manifest)
    content["source_provenance"] = {
        "source_movie": os.path.basename(source_movie),
        "source_movie_sha256": _sha256(source_movie),
        "source_movie_retained": True,
        "manifest_decoded_from_source_movie": True,
        "raw_frame_count": len(source_frames),
        "raw_frame_sequence_contiguous": True,
    }
    content["mpdecimate"] = _mpdecimate_check(
        clip, float(content["metadata"]["duration_seconds"]))
    content["passed"] = bool(
        content["passed"]
        and content["mpdecimate"]["passed"]
        and content["source_provenance"]["source_movie_retained"]
        and content["source_provenance"]["raw_frame_sequence_contiguous"]
    )
    content["lock_screen_static_footage_rejected"] = bool(
        content["temporal_continuity"]["passed"] and content["mpdecimate"]["passed"])
    with open(os.path.join(output_dir, "content-check.json"), "w", encoding="utf-8") as stream:
        json.dump(content, stream, indent=2)
        stream.write("\n")
    with open(os.path.join(output_dir, "label-proof.json"), "w", encoding="utf-8") as stream:
        json.dump({
            "proofs": proofs,
            "route_states": route_states,
            "passed": all(proof["passed"] for proof in proofs),
        }, stream, indent=2)
        stream.write("\n")
    if not content["passed"]:
        raise RuntimeError(f"marketing media content/motion contract failed: {content}")

    result = {
        "output_dir": output_dir,
        "clip": os.path.basename(clip),
        "stills": [entry["file"] for entry in stills],
        "still_records": stills,
        "continuous_movie": continuous,
        "continuous_route_states": route_states[:3],
        "source_movie": os.path.basename(source_movie),
        "source_movie_sha256": _sha256(source_movie),
        "source_movie_retained": True,
        "mpdecimate": content["mpdecimate"],
        "resolution": list(OUTPUT_SIZE),
        "fps": FPS,
        "camera_distance": 1000,
        "non_acceptance": True,
    }
    with open(os.path.join(output_dir, "capture-result.json"), "w", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    still_names = ", ".join(f"`{entry['file']}`" for entry in stills)
    with open(os.path.join(output_dir, "manifest.md"), "w", encoding="utf-8") as stream:
        stream.write(
            "# US-099 wall-fade marketing media (non-acceptance)\n\n"
            "- These stills and this clip are marketing media only, not Tier 3/Tier 4 "
            "acceptance evidence.\n"
            f"- Clip: `{os.path.basename(clip)}` ({continuous['duration_seconds']:.3f}s, "
            f"{FPS} fps, {OUTPUT_SIZE[0]}x{OUTPUT_SIZE[1]}), captured as per-frame Unreal "
            "viewport screenshots with UI under `-BENCHMARK -FPS=30`; the contiguous raw "
            "frame numbers are validated before encoding.\n"
            f"- Provenance: retained `{os.path.basename(source_movie)}` "
            f"(SHA256 `{_sha256(source_movie)}`) was assembled from the contiguous Unreal "
            "frames. `source-frame-manifest.json` is decoded from that source movie, not "
            "from the final clip.\n"
            "- Stills: captured in a separate fixed-timestep presentation session so their "
            "screenshot requests cannot interrupt or compact the continuous clip; each "
            "record reads fade state after the screenshot completes.\n"
            "- Camera: 1000 cm marketing camera (`-MordecaiMarketingCapture`), not the 1800 cm "
            "gameplay camera; the closer view keeps the wall transition legible.\n"
            "- Content: approach, dither transition, fully faded west wall with the player "
            "clear, and fully restored wall in frame; the combat HUD remains visible.\n"
            f"- Stills: {still_names}.\n"
            "- Presentation: `Mordecai.Debug.DrawMeleeHitShapes 0`, "
            "`ShowFlag.TextRender 0`, `t.MaxFPS 30`, and `-NOSCREENMESSAGES`; label-proof "
            "and content checks must pass.\n"
            f"- Motion: ffmpeg mpdecimate kept {content['mpdecimate']['kept_frames']} frames "
            f"({content['mpdecimate']['unique_frames_per_second']:.3f}/s; required >= "
            f"{MIN_UNIQUE_FPS:g}/s).\n"
            "- Lock/static defense: retained-source samples, explicit visible combat-HUD "
            "proof, temporal continuity, and mpdecimate counts are in `content-check.json` "
            "and `label-proof.json`.\n"
        )
    shutil.rmtree(movie_dir, ignore_errors=True)
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args(argv)
    result = capture(args.output_dir)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
