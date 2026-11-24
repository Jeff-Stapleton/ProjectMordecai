#!/usr/bin/env python3
"""Capture US-096 weapon cycling from game-owned bridge viewport frames."""

import argparse
import json
import os
import shutil
import sys
import time

from PIL import Image

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import agent_bridge as ab  # noqa: E402
import playtest_kit as kit  # noqa: E402
from capture_us090_marketing import _encode, _label_proof, _sha256  # noqa: E402

FPS = 30
OUTPUT_SIZE = (1920, 1080)
MIN_SECONDS = 5.0
MAX_SECONDS = 20.0
SPRINT_TAG = "Mordecai.State.Sprinting"
EXPECTED_SEQUENCE = ("Longsword", "Dagger", "Greatsword", "Dagger", "Longsword")
INPUT_SCHEDULE = (
    (1.0, ("Tab",), 1),
    (2.2, ("Tab",), 2),
    (3.4, ("LeftShift", "Tab"), 1),
    (4.6, ("LeftShift", "Tab"), 0),
)


def _player(state):
    value = state.get("player") if isinstance(state, dict) else None
    return value if isinstance(value, dict) and value.get("valid") is True else {}


def _weapon(state):
    value = _player(state).get("weapon")
    return value if isinstance(value, dict) else {}


def _tags(state):
    value = _player(state).get("tags")
    return [str(tag) for tag in value] if isinstance(value, list) else []


def weapon_sequence(records):
    """Return consecutive weapon-name changes shown in captured frames."""
    sequence = []
    for record in records:
        name = record.get("weapon")
        if name and (not sequence or sequence[-1] != name):
            sequence.append(name)
    return sequence


def still_indices(records):
    """One-based first frame for each forward/backward weapon transition."""
    indices = []
    previous = None
    for record in records:
        name = record.get("weapon")
        if name and name != previous:
            indices.append(record["index"])
            previous = name
    return indices


def capture_contract_problems(records, label_proofs):
    """Validate real-time duration, transitions, provenance, and clean presentation."""
    if not records:
        return ["no source frames"]
    problems = []
    duration = float(records[-1]["elapsed_s"]) - float(records[0]["elapsed_s"])
    if duration < MIN_SECONDS or duration > MAX_SECONDS:
        problems.append(f"real duration {duration:.3f}s is outside 5-20s")
    wrong_size = [
        record["index"] for record in records
        if (record.get("width"), record.get("height")) != OUTPUT_SIZE
    ]
    if wrong_size:
        problems.append(f"source frames are not 1920x1080: {wrong_size}")
    sequence = tuple(weapon_sequence(records))
    if sequence != EXPECTED_SEQUENCE:
        problems.append(f"weapon sequence {sequence!r} != {EXPECTED_SEQUENCE!r}")
    if len(still_indices(records)) not in range(3, 7):
        problems.append(f"still count {len(still_indices(records))} is outside 3-6")
    sprint_changes = [
        record["index"] for record in records
        if record.get("shift_tab") and record.get("sprinting_before")
        != record.get("sprinting_after")
    ]
    if sprint_changes:
        problems.append(f"Shift+Tab changed sprint state in records {sprint_changes}")
    failed_labels = [
        proof.get("phase", "unknown") for proof in label_proofs
        if proof.get("passed") is not True
    ]
    if failed_labels:
        problems.append(f"actor/debug label proof failed: {failed_labels}")
    return problems


def _capture_frame(bridge, sequence_dir, records, started, event=None,
                   sprinting_before=None, sprinting_after=None):
    number = len(records) + 1
    filename = f"frame_{number:04d}.png"
    path = os.path.join(sequence_dir, filename)
    bridge.screenshot(path)
    state = bridge.state(["player", "widgets"])
    with Image.open(path) as image:
        width, height = image.size
    entry = {
        "index": number,
        "file": os.path.join("source-frames", filename).replace("\\", "/"),
        "sha256": _sha256(path),
        "elapsed_s": round(time.monotonic() - started, 4),
        "event": event or "live",
        "weapon": _weapon(state).get("display_name"),
        "weapon_index": _weapon(state).get("index"),
        "sprinting": SPRINT_TAG in _tags(state),
        "shift_tab": event == "previous",
        "sprinting_before": sprinting_before,
        "sprinting_after": sprinting_after,
        "width": width,
        "height": height,
    }
    records.append(entry)
    return state


def capture(output_dir, duration_seconds=6.5):
    if not shutil.which("ffmpeg") or not shutil.which("ffprobe"):
        raise RuntimeError("ffmpeg and ffprobe are required")
    if duration_seconds < MIN_SECONDS or duration_seconds > MAX_SECONDS:
        raise ValueError("duration must be between 5 and 20 seconds")

    output_dir = os.path.abspath(output_dir)
    sequence_dir = os.path.join(output_dir, "source-frames")
    session_dir = os.path.join(output_dir, "session")
    os.makedirs(sequence_dir, exist_ok=True)
    os.makedirs(session_dir, exist_ok=True)

    bridge = None
    records = []
    label_proofs = []
    movement_key = None
    try:
        bridge = ab.Bridge.launch(
            map=kit.COMBAT_TEST_MAP,
            expect_experience=kit.DEFAULT_EXPERIENCE,
            run_suffix="us096marketing",
            run_dir=session_dir,
            extra_args=(
                "-MordecaiMarketingCapture",
                "-NOSCREENMESSAGES",
                f"-ExecCmds={ab.pb.quote_value('r.SetRes 1920x1080f')}",
            ),
        )
        bridge.wait(1.0)
        loaded = bridge.state(["game", "player", "actors", "widgets"], radius=3000)
        proof = _label_proof(loaded)
        proof["phase"] = "start"
        label_proofs.append(proof)
        if not proof["passed"]:
            raise RuntimeError(f"actor/debug labels remain after map load: {proof}")
        if _weapon(loaded).get("display_name") != EXPECTED_SEQUENCE[0]:
            raise RuntimeError(f"capture did not start on Longsword: {_weapon(loaded)}")

        started = time.monotonic()
        movement_key = "W"
        bridge.press(movement_key)
        next_input = 0
        turned_back = False
        pending_event = None
        pending_before = None
        pending_after = None

        while True:
            elapsed = time.monotonic() - started
            if not turned_back and elapsed >= duration_seconds / 2.0:
                bridge.release(movement_key)
                movement_key = "S"
                bridge.press(movement_key)
                turned_back = True
            if next_input < len(INPUT_SCHEDULE) and elapsed >= INPUT_SCHEDULE[next_input][0]:
                _, keys, expected_index = INPUT_SCHEDULE[next_input]
                before = bridge.state(["player"])
                sprint_before = SPRINT_TAG in _tags(before)
                if len(keys) == 2:
                    modifier, main_key = keys
                    bridge.press(modifier)
                    try:
                        bridge.wait(0.05)
                        bridge.tap(main_key, hold_s=0.1)
                    finally:
                        bridge.release(modifier)
                else:
                    bridge.tap(keys[0], hold_s=0.1)
                bridge.wait_until(
                    f"player.weapon.index=={expected_index}", timeout_s=2.0)
                after = bridge.state(["player"])
                sprint_after = SPRINT_TAG in _tags(after)
                pending_event = "previous" if len(keys) == 2 else "next"
                pending_before, pending_after = sprint_before, sprint_after
                if pending_event == "previous" and sprint_before != sprint_after:
                    raise RuntimeError(
                        f"Shift+Tab changed sprint state: {sprint_before} -> {sprint_after}")
                next_input += 1

            _capture_frame(
                bridge, sequence_dir, records, started, pending_event,
                pending_before, pending_after)
            pending_event = None
            pending_before = pending_after = None
            if time.monotonic() - started >= duration_seconds and next_input == len(INPUT_SCHEDULE):
                break

        bridge.release(movement_key)
        movement_key = None
        final = bridge.state(["game", "player", "actors", "widgets"], radius=3000)
        proof = _label_proof(final)
        proof["phase"] = "end"
        label_proofs.append(proof)
        problems = capture_contract_problems(records, label_proofs)
        if problems:
            raise RuntimeError("capture contract failed: " + "; ".join(problems))
    finally:
        if bridge is not None:
            if movement_key:
                try:
                    bridge.release(movement_key)
                except Exception:
                    pass
            bridge.close()

    first_elapsed = records[0]["elapsed_s"]
    for record in records:
        record["elapsed_s"] = round(record["elapsed_s"] - first_elapsed, 4)
    duration = records[-1]["elapsed_s"] + 1.0 / FPS
    manifest = {
        "version": 1,
        "capture": "game-owned agent bridge viewport screenshots",
        "fps": FPS,
        "frame_count": len(records),
        "duration_seconds": duration,
        "weapon_sequence": weapon_sequence(records),
        "frames": records,
    }
    manifest_path = os.path.join(output_dir, "source-frame-manifest.json")
    with open(manifest_path, "w", encoding="utf-8") as stream:
        json.dump(manifest, stream, indent=2)
        stream.write("\n")
    with open(os.path.join(output_dir, "label-proof.json"), "w", encoding="utf-8") as stream:
        json.dump({"proofs": label_proofs,
                   "passed": all(proof["passed"] for proof in label_proofs)},
                  stream, indent=2)
        stream.write("\n")

    clip = os.path.join(output_dir, "weapon-cycling-back-and-forth.mp4")
    _encode(records, output_dir, clip)
    stills = []
    for order, index in enumerate(still_indices(records), 1):
        source = os.path.join(output_dir, records[index - 1]["file"].replace("/", os.sep))
        weapon = str(records[index - 1]["weapon"]).lower()
        target = os.path.join(output_dir, f"still_{order:02d}_{weapon}.png")
        shutil.copy2(source, target)
        stills.append(os.path.basename(target))

    result = {
        "output_dir": output_dir,
        "source_manifest": manifest_path,
        "clip": clip,
        "stills": stills,
        "frame_count": len(records),
        "duration_seconds": duration,
        "resolution": list(OUTPUT_SIZE),
        "weapon_sequence": weapon_sequence(records),
        "contract": "Tab forward twice + Shift+Tab backward twice; sprint unchanged",
    }
    with open(os.path.join(output_dir, "capture-result.json"), "w", encoding="utf-8") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    with open(os.path.join(output_dir, "manifest.md"), "w", encoding="utf-8") as stream:
        stream.write(
            "# US-096 in-engine marketing media\n\n"
            f"- Clip: `{os.path.basename(clip)}` ({duration:.3f} real seconds, {FPS} fps, "
            f"{OUTPUT_SIZE[0]}x{OUTPUT_SIZE[1]}).\n"
            "- Content: Tab cycles Longsword → Dagger → Greatsword; Shift+Tab cycles "
            "Greatsword → Dagger → Longsword while the player moves through CombatTestMap.\n"
            f"- Stills: {', '.join(f'`{name}`' for name in stills)}.\n"
            "- Source: game-owned Agent Bridge viewport frames only; no desktop capture.\n"
            "- Provenance: `source-frame-manifest.json` stores every frame SHA256, real elapsed "
            "timestamp, equipped weapon, and sprint state.\n"
            "- Presentation: launched with `-MordecaiMarketingCapture -NOSCREENMESSAGES`; "
            "`label-proof.json` must pass with no actor/debug labels or enemy indicators.\n"
            "- Static/lock-screen defense: `Scripts/verify_marketing_media.py` decodes the MP4, "
            "matches samples to SHA256-pinned viewport frames, requires HUD structure, and "
            "requires motion across at least six sample pairs. See `content-check.json`.\n")
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--seconds", type=float, default=6.5)
    args = parser.parse_args(argv)
    result = capture(args.output_dir, args.seconds)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
