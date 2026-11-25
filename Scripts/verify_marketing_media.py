#!/usr/bin/env python3
"""Verify that packaged marketing clips contain the game shown by bridge screenshots."""

import argparse
import hashlib
import json
import math
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops, ImageStat


def _normalized_histogram(image, bins=16):
    resized = image.convert("RGB").resize((128, 72))
    values = []
    for channel in resized.split():
        histogram = channel.histogram()
        grouped = [
            sum(histogram[index * 256 // bins:(index + 1) * 256 // bins])
            for index in range(bins)
        ]
        total = float(sum(grouped)) or 1.0
        values.extend(value / total for value in grouped)
    return values


def histogram_similarity(left, right):
    """Return RGB histogram intersection in [0, 1]."""
    a = _normalized_histogram(left)
    b = _normalized_histogram(right)
    # Each RGB channel is normalized independently, so the maximum intersection is three.
    return sum(min(x, y) for x, y in zip(a, b)) / 3.0


def hud_region_non_uniform(image, minimum_stddev=8.0):
    """The bottom HUD band must contain more structure than a wallpaper/blank field."""
    rgb = image.convert("RGB")
    width, height = rgb.size
    hud = rgb.crop((0, int(height * 0.72), width, height))
    return max(ImageStat.Stat(hud).stddev) >= minimum_stddev


def debug_color_stats(image):
    """Detect the pure green/yellow palette used by Mordecai debug geometry/text.

    The world check excludes the bottom HUD band, where green resource fills are
    legitimate. Yellow is checked over the full frame because the shipped HUD
    does not use the near-primary yellow used by on-screen debug messages.
    """
    rgb = image.convert("RGB")
    width, height = rgb.size
    world = rgb.crop((0, 0, width, int(height * 0.72)))

    def mask_count(source, predicates):
        channels = source.split()
        mask = channels[0].point(
            lambda value: 255 if predicates[0](value) else 0)
        for channel, predicate in zip(channels[1:], predicates[1:]):
            mask = ImageChops.multiply(
                mask, channel.point(lambda value: 255 if predicate(value) else 0))
        return mask.histogram()[255]

    green = mask_count(
        world,
        (lambda value: value <= 24,
         lambda value: value >= 230,
         lambda value: value <= 24),
    )
    yellow = mask_count(
        rgb,
        (lambda value: value >= 230,
         lambda value: value >= 230,
         lambda value: value <= 30),
    )
    return {
        "world_debug_green_pixels": green,
        "screen_debug_yellow_pixels": yellow,
        "maximum_allowed_pixels": 100,
        "passed": green <= 100 and yellow <= 100,
    }


def frame_motion_score(left, right):
    """Return normalized RGB RMS change in [0, 1] after a small common resize."""
    a = left.convert("RGB").resize((128, 72))
    b = right.convert("RGB").resize((128, 72))
    rms = ImageStat.Stat(ImageChops.difference(a, b)).rms
    return sum(rms) / (len(rms) * 255.0)


def frame_psnr(left, right):
    """Return RGB PSNR in dB after a common resize; infinity means pixel-identical."""
    a = left.convert("RGB").resize((320, 180))
    b = right.convert("RGB").resize((320, 180))
    stat = ImageStat.Stat(ImageChops.difference(a, b))
    mse = sum(value * value for value in stat.rms) / len(stat.rms)
    if mse <= 0.0:
        return math.inf
    return 20.0 * math.log10(255.0 / math.sqrt(mse))


def _sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_source_manifest(path):
    with open(path, encoding="utf-8") as stream:
        payload = json.load(stream)
    if payload.get("version") != 1 or not isinstance(payload.get("frames"), list):
        raise ValueError("source manifest must be version 1 with a frames array")
    root = os.path.dirname(os.path.abspath(path))
    frames = []
    errors = []
    for entry in payload["frames"]:
        relative = entry.get("file") if isinstance(entry, dict) else None
        expected = entry.get("sha256") if isinstance(entry, dict) else None
        source = os.path.abspath(os.path.join(root, relative or ""))
        if not relative or not os.path.isfile(source):
            errors.append(f"missing source frame: {relative!r}")
            continue
        actual = _sha256(source)
        if not expected or actual.lower() != str(expected).lower():
            errors.append(f"SHA256 mismatch: {relative!r}")
            continue
        frames.append(source)
    if errors:
        raise ValueError("; ".join(errors))
    if not frames:
        raise ValueError("source manifest has no verified frames")
    return payload, frames


def _run_json(command):
    return json.loads(subprocess.check_output(command, text=True, stderr=subprocess.STDOUT))


def probe_video(path):
    payload = _run_json([
        "ffprobe", "-v", "error", "-count_frames", "-select_streams", "v:0",
        "-show_entries",
        "stream=width,height,pix_fmt,r_frame_rate,nb_read_frames:format=duration,size",
        "-of", "json", path,
    ])
    stream = payload["streams"][0]
    fmt = payload["format"]
    return {
        "width": int(stream["width"]),
        "height": int(stream["height"]),
        "pixel_format": stream.get("pix_fmt"),
        "frame_rate": stream.get("r_frame_rate"),
        "frame_count": int(stream.get("nb_read_frames") or 0),
        "duration_seconds": float(fmt["duration"]),
        "size_bytes": int(fmt["size"]),
    }


def mpdecimate_stats(path, minimum_unique_ratio=0.9):
    metadata = probe_video(path)
    output = subprocess.check_output([
        "ffmpeg", "-v", "error", "-i", path, "-vf", "mpdecimate",
        "-an", "-f", "framehash", "-",
    ], text=True, stderr=subprocess.STDOUT)
    kept = sum(
        1 for line in output.splitlines()
        if line and not line.startswith("#") and line.lstrip().startswith("0,")
    )
    total = metadata["frame_count"]
    ratio = kept / total if total else 0.0
    return {
        "method": "ffmpeg mpdecimate -> framehash kept-frame count",
        "input_frames": total,
        "kept_frames": kept,
        "unique_frame_ratio": round(ratio, 6),
        "minimum_unique_frame_ratio": minimum_unique_ratio,
        "passed": total > 0 and ratio >= minimum_unique_ratio,
    }


def verify_still(path):
    with Image.open(path) as source:
        image = source.convert("RGB")
        stat = ImageStat.Stat(image)
        colors = debug_color_stats(image)
        return {
            "file": os.path.abspath(path),
            "width": image.width,
            "height": image.height,
            "luminance_stddev_max_channel": round(max(stat.stddev), 4),
            "non_uniform": max(stat.stddev) >= 8.0,
            "hud_region_non_uniform": hud_region_non_uniform(image),
            "debug_colors": colors,
            "passed": (
                image.size == (1920, 1080)
                and max(stat.stddev) >= 8.0
                and hud_region_non_uniform(image)
                and colors["passed"]
            ),
        }


def _sample_frame(video, second, output):
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error", "-ss", f"{second:.3f}", "-i", video,
        "-frames:v", "1", output,
    ])


def decode_motion_frames(video, sample_fps=10, size=(128, 72)):
    """Decode evenly spaced RGB frames without using desktop capture."""
    width, height = size
    raw = subprocess.check_output([
        "ffmpeg", "-loglevel", "error", "-i", video,
        "-vf", f"fps={sample_fps},scale={width}:{height}",
        "-f", "rawvideo", "-pix_fmt", "rgb24", "-",
    ])
    stride = width * height * 3
    return [
        Image.frombytes("RGB", size, raw[offset:offset + stride])
        for offset in range(0, len(raw) - stride + 1, stride)
    ]


def temporal_continuity(images, minimum_motion=0.00015, minimum_ratio=0.6):
    """Reject duplicated-frame slideshows and static/lock-screen footage."""
    scores = [
        frame_motion_score(images[index - 1], images[index])
        for index in range(1, len(images))
    ]
    moving = sum(score >= minimum_motion for score in scores)
    ratio = moving / len(scores) if scores else 0.0
    return {
        "sample_count": len(images),
        "moving_pairs": moving,
        "pair_count": len(scores),
        "moving_pair_ratio": round(ratio, 4),
        "minimum_motion": minimum_motion,
        "minimum_ratio": minimum_ratio,
        "passed": len(images) >= 40 and ratio >= minimum_ratio,
    }


def verify_video(video, source_manifest, minimum_psnr=24.0, minimum_motion=0.00035):
    metadata = probe_video(video)
    duration = metadata["duration_seconds"]
    sample_times = [duration * fraction for fraction in (0.1, 0.2, 0.3, 0.4, 0.5,
                                                          0.6, 0.7, 0.8, 0.9)]
    _, source_paths = load_source_manifest(source_manifest)
    source_images = []
    for path in source_paths:
        with Image.open(path) as source:
            source_images.append(source.convert("RGB").resize((320, 180)))

    samples = []
    sampled_images = []
    with tempfile.TemporaryDirectory(prefix="mordecai-media-") as temp:
        for index, second in enumerate(sample_times, 1):
            frame_path = os.path.join(temp, f"frame-{index}.png")
            _sample_frame(video, second, frame_path)
            with Image.open(frame_path) as source:
                frame = source.convert("RGB")
                sampled_images.append(frame.copy())
                matches = [frame_psnr(frame, source) for source in source_images]
                best = max(matches, default=0.0)
                best_index = matches.index(best) if matches else None
                hud_ok = hud_region_non_uniform(frame)
                debug_colors = debug_color_stats(frame)
                samples.append({
                    "second": round(second, 3),
                    "best_source_frame": (
                        os.path.basename(source_paths[best_index])
                        if best_index is not None else None),
                    "best_source_psnr_db": (
                        "inf" if math.isinf(best) else round(best, 3)),
                    "hud_region_non_uniform": hud_ok,
                    "debug_colors": debug_colors,
                    "passed": best >= minimum_psnr and hud_ok
                              and debug_colors["passed"],
                })

    motion_scores = [
        round(frame_motion_score(sampled_images[index - 1], sampled_images[index]), 6)
        for index in range(1, len(sampled_images))
    ]
    moving_pairs = sum(score >= minimum_motion for score in motion_scores)
    motion_passed = moving_pairs >= 6
    continuity = temporal_continuity(decode_motion_frames(video))
    decimate = mpdecimate_stats(video)
    metadata_ok = (
        5.0 <= duration <= 20.0
        and (metadata["width"], metadata["height"]) in ((1280, 720), (1920, 1080))
        and metadata["pixel_format"] == "yuv420p"
        and metadata["frame_rate"] == "30/1"
        and metadata["frame_count"] >= math.floor(duration * 24.0)
        and metadata["size_bytes"] < 50 * 1024 * 1024
    )
    return {
        "video": os.path.abspath(video),
        "source_manifest": os.path.abspath(source_manifest),
        "source_frame_count": len(source_paths),
        "minimum_source_psnr_db": minimum_psnr,
        "minimum_motion": minimum_motion,
        "metadata": metadata,
        "metadata_passed": metadata_ok,
        "samples": samples,
        "motion": {
            "consecutive_sample_scores": motion_scores,
            "moving_pairs": moving_pairs,
            "required_moving_pairs": 6,
            "passed": motion_passed,
        },
        "temporal_continuity": continuity,
        "mpdecimate": decimate,
        "passed": (metadata_ok and bool(samples) and all(item["passed"] for item in samples)
                   and motion_passed and continuity["passed"]
                   and decimate["passed"]),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--video", action="append", required=True)
    parser.add_argument("--source-manifest", required=True)
    parser.add_argument("--minimum-psnr", type=float, default=24.0)
    parser.add_argument("--minimum-motion", type=float, default=0.00035)
    parser.add_argument("--output")
    args = parser.parse_args(argv)

    missing = [path for path in args.video + [args.source_manifest] if not os.path.isfile(path)]
    if missing:
        parser.error("missing file(s): " + ", ".join(missing))

    videos = [
        verify_video(path, args.source_manifest, args.minimum_psnr, args.minimum_motion)
        for path in args.video
    ]
    result = {
        "method": ("required SHA256 source-frame manifest + decoded-frame PSNR source matching "
                   "+ HUD structure + consecutive-frame motion + 10 fps temporal continuity"),
        "videos": videos,
        "passed": all(video["passed"] for video in videos),
    }
    text = json.dumps(result, indent=2) + "\n"
    if args.output:
        with open(args.output, "w", encoding="utf-8") as stream:
            stream.write(text)
    print(text, end="")
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
