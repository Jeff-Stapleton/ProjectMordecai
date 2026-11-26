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

from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402

FPS = 30
OUTPUT_SIZE = (1920, 1080)
MIN_CLIP_SECONDS = 5.0
DEFAULT_MAX_CLIP_SECONDS = 18.0
PRECAPTURE_HEALTH = 55.0
ATTACKER_FIGHT_SPOT = (2000.0, 0.0)
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
    game = state.get("game") if isinstance(state, dict) else {}
    game = game if isinstance(game, dict) else {}
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
    presentation_violations = [
        key for key, expected in (
            ("marketing_capture", True),
            ("debug_draw_helpers_enabled", False),
            ("melee_hit_shapes_enabled", False),
            ("screen_messages_enabled", False),
            ("marketing_presentation_clean", True),
        )
        if game.get(key) is not expected
    ]
    facing_arrow_violations = []
    player_arrow = ((_player(state).get("facing_arrow")
                     if isinstance(state, dict) else None))
    if isinstance(player_arrow, dict) and player_arrow.get("hidden_in_game") is not True:
        facing_arrow_violations.append("player")
    facing_arrow_violations.extend(
        str(actor.get("label") or actor.get("name") or "actor")
        for actor in actors
        if isinstance(actor.get("facing_arrow"), dict)
        and actor["facing_arrow"].get("hidden_in_game") is not True
    )
    proof = {
        "map": game.get("map"),
        "experience": game.get("experience"),
        "command_line_flag": "-MordecaiMarketingCapture",
        "actor_count": len(actors),
        "rendered_widget_texts": texts,
        "actor_widget_violations": actor_widgets,
        "text_label_violations": text_labels,
        "rendered_actor_name_violations": rendered_names,
        "enemy_indicator_widgets": enemy_indicators,
        "debug_text_violations": debug_texts,
        "presentation_state": {
            key: game.get(key)
            for key in (
                "marketing_capture",
                "debug_draw_helpers_enabled",
                "melee_hit_shapes_enabled",
                "screen_messages_enabled",
                "marketing_presentation_clean",
            )
        },
        "presentation_violations": presentation_violations,
        "facing_arrow_violations": facing_arrow_violations,
    }
    proof["passed"] = not any(
        (actor_widgets, text_labels, rendered_names, enemy_indicators, debug_texts,
         presentation_violations, facing_arrow_violations))
    return proof


def frame_durations(records):
    """Durations for ffmpeg's concat demuxer, preserving real capture time."""
    if not records:
        return []
    durations = [
        max(0.001, float(records[index + 1]["elapsed_s"])
            - float(records[index]["elapsed_s"]))
        for index in range(len(records) - 1)
    ]
    durations.append(1.0 / FPS)
    return durations


def still_indices(records):
    """One-based full, minimum-SP, first-dead and first-respawn source indices."""
    full = records[0]["index"]
    spend = min(
        (record for record in records if record.get("spell_points") is not None),
        key=lambda record: record["spell_points"])["index"]
    dead = next(record["index"] for record in records if record.get("event") == "dead")
    respawn = next(record["index"] for record in records if record.get("event") == "respawn")
    return list(dict.fromkeys((full, spend, dead, respawn)))


def capture_contract_problems(records):
    """Validate that the clip proves the requested real-time SP story without a static tail."""
    if not records:
        return ["no source frames"]
    problems = []
    duration = float(records[-1]["elapsed_s"]) - float(records[0]["elapsed_s"])
    if duration < MIN_CLIP_SECONDS or duration > 20.0:
        problems.append(f"real duration {duration:.3f}s is outside 5-20s")
    wrong_size = [
        record["index"] for record in records
        if (record.get("width"), record.get("height")) != OUTPUT_SIZE
    ]
    if wrong_size:
        problems.append(f"source frames are not 1920x1080: {wrong_size}")
    initial_sp = records[0].get("spell_points")
    initial_max = records[0].get("max_spell_points")
    if initial_sp is None or initial_max is None or initial_sp != initial_max:
        problems.append(f"initial SP is not full: {initial_sp} / {initial_max}")
    by_event = {
        event: [record for record in records if record.get("event") == event]
        for event in ("spend", "regen", "dead", "respawn")
    }
    for event in ("spend", "dead", "respawn"):
        if not by_event[event]:
            problems.append(f"missing {event} event")
    if len(by_event["regen"]) < 2:
        problems.append(f"saw {len(by_event['regen'])} regen ticks, expected at least 2")
    if by_event["spend"] and by_event["dead"]:
        regen_window = by_event["dead"][0]["elapsed_s"] - by_event["spend"][0]["elapsed_s"]
        if regen_window < 2.0:
            problems.append(f"spend-to-death window {regen_window:.3f}s is under 2 real seconds")
    if by_event["spend"]:
        before = records[0].get("spell_points")
        after = by_event["spend"][0].get("spell_points")
        if before is None or after is None or before - after < 2.74:
            problems.append(f"SP spend is not visible: {before} -> {after}")
    if by_event["respawn"]:
        respawn = by_event["respawn"][0]
        if respawn.get("spell_points") != respawn.get("max_spell_points"):
            problems.append(
                "respawn did not refill SP exactly: "
                f"{respawn.get('spell_points')} / {respawn.get('max_spell_points')}")
        if records[-1]["index"] != respawn["index"]:
            problems.append(
                f"static tail after respawn frame {respawn['index']} "
                f"(last frame {records[-1]['index']})")
    return problems


def _ffconcat_path(path):
    return os.path.abspath(path).replace("\\", "/").replace("'", "'\\''")


def _encode(records, output_dir, output):
    concat_path = os.path.join(output_dir, "source-frames.ffconcat")
    durations = frame_durations(records)
    with open(concat_path, "w", encoding="utf-8", newline="\n") as stream:
        stream.write("ffconcat version 1.0\n")
        for record, duration in zip(records, durations):
            source = os.path.join(output_dir, record["file"].replace("/", os.sep))
            stream.write(f"file '{_ffconcat_path(source)}'\n")
            stream.write(f"duration {duration:.9f}\n")
        # The concat demuxer needs the last file repeated to honor its duration.
        source = os.path.join(output_dir, records[-1]["file"].replace("/", os.sep))
        stream.write(f"file '{_ffconcat_path(source)}'\n")
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error",
        "-f", "concat", "-safe", "0", "-i", concat_path,
        "-vf", f"fps={FPS}",
        "-c:v", "libx264", "-preset", "slow", "-crf", "18",
        "-pix_fmt", "yuv420p", "-r", str(FPS), output,
    ])


def capture(output_dir, max_clip_seconds=DEFAULT_MAX_CLIP_SECONDS):
    if not shutil.which("ffmpeg"):
        raise RuntimeError("ffmpeg is required")
    output_dir = os.path.abspath(output_dir)
    sequence_dir = os.path.join(output_dir, "source-frames")
    session_dir = os.path.join(output_dir, "session")
    os.makedirs(sequence_dir, exist_ok=True)
    os.makedirs(session_dir, exist_ok=True)

    bridge = None
    frame_records = []
    try:
        bridge = ab.Bridge.launch(
            map=kit.COMBAT_TEST_MAP,
            expect_experience=kit.DEFAULT_EXPERIENCE,
            run_suffix="us090marketing",
            run_dir=session_dir,
            extra_args=(
                "-MordecaiMarketingCapture",
                f"-ExecCmds={ab.pb.quote_value('r.SetRes 1920x1080f')}",
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

        bridge.walk_to(*ATTACKER_FIGHT_SPOT, tol=25.0, timeout_s=30.0)
        precondition_deadline = time.monotonic() + 60.0
        while True:
            state = bridge.state(["player"])
            player = _player(state)
            attrs = player.get("attributes") or {}
            health = attrs.get("health")
            if player.get("is_dead"):
                raise RuntimeError("player died before the real-time capture window")
            if isinstance(health, (int, float)) and health <= PRECAPTURE_HEALTH:
                break
            if time.monotonic() >= precondition_deadline:
                raise RuntimeError(
                    f"attacker did not lower health to {PRECAPTURE_HEALTH:g}; last {health}")
            bridge.wait(0.1)

        capture_started = time.monotonic()

        def shot(event):
            number = len(frame_records) + 1
            filename = f"frame_{number:04d}.png"
            path = os.path.join(sequence_dir, filename)
            bridge.screenshot(path)
            state = bridge.state(["player", "widgets"])
            player = _player(state)
            with Image.open(path) as image:
                width, height = image.size
            spell_points = _spell_points(state)
            is_dead = player.get("is_dead") is True
            if is_dead:
                event = "dead"
            elif any(record["event"] == "dead" for record in frame_records):
                event = "respawn"
            elif frame_records and spell_points is not None:
                previous = frame_records[-1].get("spell_points")
                if previous is not None and spell_points < previous - 0.1:
                    event = "spend"
                elif previous is not None and spell_points > previous + 0.1:
                    event = "regen"
            entry = {
                "index": number,
                "file": os.path.join("source-frames", filename).replace("\\", "/"),
                "sha256": _sha256(path),
                "elapsed_s": round(time.monotonic() - capture_started, 4),
                "event": event,
                "spell_points": spell_points,
                "max_spell_points": (player.get("attributes") or {}).get("max_spell_points"),
                "health": (player.get("attributes") or {}).get("health"),
                "is_dead": is_dead,
                "width": width,
                "height": height,
                "widget_texts": _widget_texts(state),
            }
            frame_records.append(entry)
            return state

        shot("full")
        bridge.tap("Q", hold_s=0.1)
        while True:
            iteration_started = time.monotonic()
            state = shot("live")
            latest = frame_records[-1]
            if latest["event"] == "respawn":
                if latest["spell_points"] != latest["max_spell_points"]:
                    raise RuntimeError(
                        "first respawn frame did not show exact SP refill: "
                        f"{latest['spell_points']} / {latest['max_spell_points']}")
                break
            if latest["elapsed_s"] >= max_clip_seconds:
                raise RuntimeError(
                    f"no death/respawn refill within {max_clip_seconds:g} real seconds")
            remaining = 1.0 / FPS - (time.monotonic() - iteration_started)
            if remaining > 0:
                bridge.wait(remaining)

        first_elapsed = frame_records[0]["elapsed_s"]
        for record in frame_records:
            record["elapsed_s"] = round(record["elapsed_s"] - first_elapsed, 4)
        problems = capture_contract_problems(frame_records)
        if problems:
            raise RuntimeError("capture contract failed: " + "; ".join(problems))
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

    values = [entry["spell_points"] for entry in frame_records
              if entry["spell_points"] is not None]
    duration_seconds = (
        frame_records[-1]["elapsed_s"] - frame_records[0]["elapsed_s"] + 1.0 / FPS)
    with open(os.path.join(output_dir, "state-timeline.json"), "w", encoding="utf-8") as stream:
        json.dump({
            "fps": FPS,
            "duration_seconds": duration_seconds,
            "minimum_spell_points": min(values),
            "maximum_spell_points": max(values),
            "timeline": frame_records,
        }, stream, indent=2)
        stream.write("\n")

    clip = os.path.join(output_dir, "spell-points-drain-and-refill.mp4")
    _encode(frame_records, output_dir, clip)

    stills = []
    for order, index in enumerate(still_indices(frame_records), 1):
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
        "duration_seconds": duration_seconds,
        "resolution": list(OUTPUT_SIZE),
        "contract": "spend + >=2 real-time regen ticks + death + exact respawn refill",
    }
    with open(os.path.join(output_dir, "capture-result.json"), "w", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    with open(os.path.join(output_dir, "manifest.md"), "w", encoding="utf-8") as stream:
        stream.write(
            "# US-090 in-engine marketing media\n\n"
            f"- Clip: `{os.path.basename(clip)}` ({duration_seconds:.3f} real seconds, "
            f"{FPS} fps, {OUTPUT_SIZE[0]}x{OUTPUT_SIZE[1]})\n"
            "- Content: a real Q spend, periodic SP ticks over several real seconds, death, "
            "and the exact Max-SP respawn refill; capture ends on the refill frame.\n"
            f"- Stills: {', '.join(f'`{name}`' for name in stills)}\n"
            "- Source: game-owned Agent Bridge viewport frames only; no desktop capture.\n"
            "- Provenance: `source-frame-manifest.json` stores every source-frame SHA256 and "
            "real elapsed timestamp.\n"
            "- Labels: `label-proof.json` must pass after map load; actor/debug labels and the "
            "yellow ability log are absent.\n")
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--max-seconds", type=float, default=DEFAULT_MAX_CLIP_SECONDS)
    args = parser.parse_args(argv)
    if args.max_seconds < MIN_CLIP_SECONDS or args.max_seconds > 20.0:
        parser.error("--max-seconds must be between 5 and 20")
    result = capture(args.output_dir, args.max_seconds)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
