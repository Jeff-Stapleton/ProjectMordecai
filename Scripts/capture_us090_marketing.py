#!/usr/bin/env python3
"""Capture and encode US-090 marketing media from game-owned bridge viewport frames."""

import argparse
import hashlib
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
import playtest_kit as kit  # noqa: E402

FPS = 30
MIN_CLIP_FRAMES = 5 * FPS
DEFAULT_FRAME_COUNT = 180
DEBUG_TEXT_MARKERS = (
    "ability activated",
    "ability log",
    "debug:",
    "input action:",
)


def _sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _player(state):
    value = state.get("player") if isinstance(state, dict) else None
    return value if isinstance(value, dict) and value.get("valid") is True else {}


def _spell_points(state):
    value = (_player(state).get("attributes") or {}).get("spell_points")
    return float(value) if isinstance(value, (int, float)) else None


def _widget_texts(state):
    return [
        str(text)
        for widget in (state.get("widgets") or [])
        if isinstance(widget, dict) and widget.get("visible", True) is not False
        for text in (widget.get("texts") or [])
    ]


def _label_proof(state):
    actors = [actor for actor in (state.get("actors") or []) if isinstance(actor, dict)]
    actor_names = {
        str(actor.get(key))
        for actor in actors
        for key in ("label", "name", "class")
        if actor.get(key)
    }
    texts = _widget_texts(state)
    actor_widgets = [
        {"actor": actor.get("label") or actor.get("name"), "widget": widget}
        for actor in actors
        for widget in (actor.get("widgets") or [])
        if isinstance(widget, dict) and widget.get("visible", True) is not False
    ]
    text_labels = [
        {"actor": actor.get("label") or actor.get("name"), "label": label}
        for actor in actors
        for label in (actor.get("text_labels") or [])
        if isinstance(label, dict) and label.get("visible", True) is not False
    ]
    rendered_names = [text for text in texts if text in actor_names]
    enemy_indicators = [
        widget for widget in (state.get("widgets") or [])
        if isinstance(widget, dict)
        and "EnemyIndicator" in str(widget.get("class") or "")
        and widget.get("visible", True) is not False
    ]
    debug_texts = [
        text for text in texts
        if any(marker in text.lower() for marker in DEBUG_TEXT_MARKERS)
    ]
    proof = {
        "map": (state.get("game") or {}).get("map"),
        "experience": (state.get("game") or {}).get("experience"),
        "command_line_flag": "-MordecaiMarketingCapture",
        "actor_count": len(actors),
        "rendered_widget_texts": texts,
        "actor_widget_violations": actor_widgets,
        "text_label_violations": text_labels,
        "rendered_actor_name_violations": rendered_names,
        "enemy_indicator_widgets": enemy_indicators,
        "debug_text_violations": debug_texts,
    }
    proof["passed"] = not any(
        (actor_widgets, text_labels, rendered_names, enemy_indicators, debug_texts))
    return proof


def _encode(sequence_dir, output):
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error",
        "-framerate", str(FPS),
        "-i", os.path.join(sequence_dir, "frame_%04d.png"),
        "-c:v", "libx264", "-preset", "slow", "-crf", "18",
        "-pix_fmt", "yuv420p", "-r", str(FPS), output,
    ])


def capture(output_dir, frame_count=DEFAULT_FRAME_COUNT):
    if not shutil.which("ffmpeg"):
        raise RuntimeError("ffmpeg is required")
    output_dir = os.path.abspath(output_dir)
    sequence_dir = os.path.join(output_dir, "source-frames")
    session_dir = os.path.join(output_dir, "session")
    os.makedirs(sequence_dir, exist_ok=True)
    os.makedirs(session_dir, exist_ok=True)

    bridge = None
    timeline = []
    frame_records = []
    try:
        bridge = ab.Bridge.launch(
            map=kit.COMBAT_TEST_MAP,
            expect_experience=kit.DEFAULT_EXPERIENCE,
            run_suffix="us090marketing",
            run_dir=session_dir,
            extra_args=(
                "-MordecaiMarketingCapture",
                "-BENCHMARK",
                f"-FPS={FPS}",
            ),
        )
        bridge.wait(1.0)
        loaded = bridge.state(["game", "player", "actors", "widgets"], radius=3000)
        label_proof = _label_proof(loaded)
        with open(os.path.join(output_dir, "label-proof.json"), "w", encoding="utf-8") as stream:
            json.dump(label_proof, stream, indent=2)
            stream.write("\n")
        if not label_proof["passed"]:
            raise RuntimeError(f"actor/debug labels remain after map load: {label_proof}")

        def shot(event):
            number = len(frame_records) + 1
            filename = f"frame_{number:04d}.png"
            path = os.path.join(sequence_dir, filename)
            bridge.screenshot(path)
            state = bridge.state(["player", "widgets"])
            player = _player(state)
            entry = {
                "index": number,
                "file": os.path.join("source-frames", filename).replace("\\", "/"),
                "sha256": _sha256(path),
                "event": event,
                "spell_points": _spell_points(state),
                "max_spell_points": (player.get("attributes") or {}).get("max_spell_points"),
                "widget_texts": _widget_texts(state),
            }
            frame_records.append(entry)
            timeline.append(dict(entry))
            return state

        for _ in range(15):
            shot("full")
            bridge.wait(1.0 / FPS)

        for key, event in (("Q", "fireball"), ("E", "blink"), ("R", "stone_skin")):
            bridge.tap(key, hold_s=0.1)
            for _ in range(12):
                shot(event)
                bridge.wait(1.0 / FPS)

        while len(frame_records) < frame_count:
            state = shot("regen_to_full")
            if (_spell_points(state) or 0.0) >= 10.0 and len(frame_records) >= MIN_CLIP_FRAMES:
                break
            bridge.wait(0.25)

        while len(frame_records) < max(frame_count, MIN_CLIP_FRAMES):
            shot("full_hold")
            bridge.wait(1.0 / FPS)
    finally:
        if bridge is not None:
            bridge.close()

    manifest = {
        "version": 1,
        "capture": "game-owned agent bridge viewport screenshots",
        "fps": FPS,
        "frame_count": len(frame_records),
        "frames": frame_records,
    }
    manifest_path = os.path.join(output_dir, "source-frame-manifest.json")
    with open(manifest_path, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, indent=2)
        stream.write("\n")

    values = [entry["spell_points"] for entry in timeline
              if entry["spell_points"] is not None]
    with open(os.path.join(output_dir, "state-timeline.json"), "w", encoding="utf-8") as stream:
        json.dump({
            "fps": FPS,
            "duration_seconds": len(frame_records) / FPS,
            "minimum_spell_points": min(values),
            "maximum_spell_points": max(values),
            "timeline": timeline,
        }, stream, indent=2)
        stream.write("\n")

    clip = os.path.join(output_dir, "spell-points-drain-and-refill.mp4")
    _encode(sequence_dir, clip)

    still_indices = sorted(set((
        1,
        min(range(1, len(frame_records) + 1),
            key=lambda index: frame_records[index - 1]["spell_points"]
            if frame_records[index - 1]["spell_points"] is not None else float("inf")),
        max(1, len(frame_records) // 2),
        len(frame_records),
    )))
    stills = []
    for order, index in enumerate(still_indices, 1):
        source = os.path.join(
            output_dir,
            frame_records[index - 1]["file"].replace("/", os.sep))
        target = os.path.join(output_dir, f"still_{order:02d}.png")
        shutil.copy2(source, target)
        stills.append(os.path.basename(target))

    result = {
        "output_dir": output_dir,
        "source_manifest": manifest_path,
        "clip": clip,
        "stills": stills,
        "frame_count": len(frame_records),
        "duration_seconds": len(frame_records) / FPS,
    }
    with open(os.path.join(output_dir, "capture-result.json"), "w", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--frames", type=int, default=DEFAULT_FRAME_COUNT)
    args = parser.parse_args(argv)
    if args.frames < MIN_CLIP_FRAMES or args.frames > 20 * FPS:
        parser.error(f"--frames must be between {MIN_CLIP_FRAMES} and {20 * FPS}")
    result = capture(args.output_dir, args.frames)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
