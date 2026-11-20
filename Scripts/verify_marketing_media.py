#!/usr/bin/env python3
"""Verify that packaged marketing clips contain the game shown by bridge screenshots."""

import argparse
import json
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageStat


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


def _run_json(command):
    return json.loads(subprocess.check_output(command, text=True, stderr=subprocess.STDOUT))


def probe_video(path):
    payload = _run_json([
        "ffprobe", "-v", "error", "-select_streams", "v:0",
        "-show_entries", "stream=width,height,pix_fmt,r_frame_rate:format=duration,size",
        "-of", "json", path,
    ])
    stream = payload["streams"][0]
    fmt = payload["format"]
    return {
        "width": int(stream["width"]),
        "height": int(stream["height"]),
        "pixel_format": stream.get("pix_fmt"),
        "frame_rate": stream.get("r_frame_rate"),
        "duration_seconds": float(fmt["duration"]),
        "size_bytes": int(fmt["size"]),
    }


def _sample_frame(video, second, output):
    subprocess.check_call([
        "ffmpeg", "-y", "-loglevel", "error", "-ss", f"{second:.3f}", "-i", video,
        "-frames:v", "1", output,
    ])


def verify_video(video, references, minimum_similarity=0.55):
    metadata = probe_video(video)
    duration = metadata["duration_seconds"]
    sample_times = [duration * fraction for fraction in (0.25, 0.5, 0.75)]
    reference_images = []
    for path in references:
        with Image.open(path) as source:
            reference_images.append(source.convert("RGB"))

    samples = []
    with tempfile.TemporaryDirectory(prefix="mordecai-media-") as temp:
        for index, second in enumerate(sample_times, 1):
            frame_path = os.path.join(temp, f"frame-{index}.png")
            _sample_frame(video, second, frame_path)
            with Image.open(frame_path) as source:
                frame = source.convert("RGB")
                similarities = [
                    histogram_similarity(frame, reference) for reference in reference_images
                ]
                best = max(similarities, default=0.0)
                hud_ok = hud_region_non_uniform(frame)
                samples.append({
                    "second": round(second, 3),
                    "best_histogram_similarity": round(best, 4),
                    "hud_region_non_uniform": hud_ok,
                    "passed": best >= minimum_similarity and hud_ok,
                })

    metadata_ok = (
        5.0 <= duration <= 20.0
        and metadata["width"] == 1920
        and metadata["height"] == 1080
        and metadata["pixel_format"] == "yuv420p"
        and metadata["size_bytes"] < 50 * 1024 * 1024
    )
    return {
        "video": os.path.abspath(video),
        "references": [os.path.abspath(path) for path in references],
        "minimum_similarity": minimum_similarity,
        "metadata": metadata,
        "metadata_passed": metadata_ok,
        "samples": samples,
        "passed": metadata_ok and bool(samples) and all(item["passed"] for item in samples),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--video", action="append", required=True)
    parser.add_argument("--reference", action="append", required=True)
    parser.add_argument("--minimum-similarity", type=float, default=0.55)
    parser.add_argument("--output")
    args = parser.parse_args(argv)

    missing = [path for path in args.video + args.reference if not os.path.isfile(path)]
    if missing:
        parser.error("missing file(s): " + ", ".join(missing))

    videos = [
        verify_video(path, args.reference, args.minimum_similarity) for path in args.video
    ]
    result = {
        "method": "ffprobe metadata + sampled-frame RGB histogram/HUD comparison",
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
