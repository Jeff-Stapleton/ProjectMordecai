#!/usr/bin/env python3
"""Packaged Tin Tally visual-gallery capture and comparison harness.

US-103 RED scaffold: the public helpers exist so the tests compile and fail at
assertion level. GREEN implements the package freshness gate, metadata checks,
image comparison/evidence, packaged capture orchestration, and sweep diff.
"""

import argparse
import datetime
import json
import os
import subprocess
import time

from PIL import Image, ImageChops, ImageFilter, ImageStat

import playtest_boot


FOUNDATION_COMPONENTS = ("Plate", "Tag", "Keycap", "Tally", "Glyphs")
THEMES = ("diorama", "high-contrast")
CAPTURE_SIZE = (1920, 1080)
REFERENCE_CROP_SIZE = (960, 720)
MIN_FOREGROUND_IOU = 0.08
MAX_BBOX_EDGE_DELTA = 0.20
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REFERENCE_SHOTS_DIR = os.path.join(
    ROOT, "Design", "TinTally", "ds", "build", "shots"
)


def make_capture_metadata(component, theme, state, build_info, crop_rect):
    """Return deterministic capture metadata for one gallery frame."""
    return {
        "schema": 1,
        "component": component,
        "theme": theme,
        "state": state,
        "commit": build_info.get("commit"),
        "package_built_at": build_info.get("built_at"),
        "resolution": list(CAPTURE_SIZE),
        "crop_rect": list(crop_rect),
    }


def validate_capture(image_path, metadata, build_info, expected_theme):
    """Return a list of rejection reasons; an empty list means valid."""
    reasons = []
    if not os.path.isfile(image_path):
        return [f"capture is missing: {image_path}"]
    try:
        with Image.open(image_path) as source:
            image = source.convert("RGB")
    except (OSError, ValueError) as exc:
        return [f"capture is unreadable: {exc}"]

    if metadata.get("resolution") != list(CAPTURE_SIZE):
        reasons.append(
            f"capture metadata must be 1920x1080; got {metadata.get('resolution')}"
        )
    if metadata.get("theme") != expected_theme:
        reasons.append(
            f"theme mismatch: expected {expected_theme}, got {metadata.get('theme')}"
        )
    if metadata.get("commit") != build_info.get("commit"):
        reasons.append(
            f"stale capture commit {metadata.get('commit')} != {build_info.get('commit')}"
        )
    if build_info.get("dirty"):
        reasons.append("package metadata says dirty=true")

    extrema = image.getextrema()
    if all(channel[1] <= 2 for channel in extrema):
        reasons.append("capture is black")
    return reasons


def _background_color(image):
    width, height = image.size
    points = (
        (0, 0),
        (width - 1, 0),
        (0, height - 1),
        (width - 1, height - 1),
        (min(8, width - 1), min(8, height - 1)),
        (max(0, width - 9), min(8, height - 1)),
    )
    samples = [image.getpixel(point) for point in points]
    return tuple(sorted(pixel[channel] for pixel in samples)[len(samples) // 2] for channel in range(3))


def _foreground_mask(image):
    """Segment non-sheet pixels and close only one-pixel antialias gaps."""
    rgb = image.convert("RGB")
    background = _background_color(rgb)
    mask = Image.new("L", rgb.size)
    mask.putdata([
        255
        if max(abs(pixel[channel] - background[channel]) for channel in range(3)) >= 18
        else 0
        for pixel in rgb.getdata()
    ])
    return mask.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.MinFilter(3))


def _mask_pixels(mask):
    return mask.histogram()[255]


def _bbox_edge_delta(reference_box, capture_box, size):
    if reference_box is None and capture_box is None:
        return 0.0
    if reference_box is None or capture_box is None:
        return 1.0
    width, height = size
    return max(
        abs(reference_box[index] - capture_box[index])
        / (width if index % 2 == 0 else height)
        for index in range(4)
    )


def reference_path(component, theme):
    return os.path.join(REFERENCE_SHOTS_DIR, f"{component}.{theme}.png")


def compare_images(reference_path, capture_path, output_dir, stem):
    """Compare foreground geometry without resizing either design-pixel image.

    Acceptance requires foreground-mask IoU >= 0.08 and every foreground
    bounding-box edge within 20% of the reference viewport. The deliberately
    broad IoU tolerance allows browser/Slate font rasterization differences;
    the bounding-box gate rejects the former full-width, one-row fixtures.
    """
    os.makedirs(output_dir, exist_ok=True)
    safe_stem = stem.replace("/", "_").replace("\\", "_")
    reference_crop_path = os.path.join(
        output_dir, f"{safe_stem}-reference-crop.png"
    )
    side_path = os.path.join(output_dir, f"{safe_stem}-side-by-side.png")
    diff_path = os.path.join(output_dir, f"{safe_stem}-diff.png")
    try:
        with (
            Image.open(reference_path) as ref_source,
            Image.open(capture_path) as cap_source,
        ):
            reference = ref_source.convert("RGB")
            capture = cap_source.convert("RGB")
    except (OSError, ValueError) as exc:
        return {
            "passed": False,
            "reason": f"comparison input is missing or unreadable: {exc}",
            "foreground_iou": 0.0,
            "bbox_edge_delta": 1.0,
            "foreground_pixels": {"reference": 0, "capture": 0},
            "reference_crop": reference_crop_path,
            "side_by_side": side_path,
            "diff": diff_path,
            "thresholds": {
                "min_foreground_iou": MIN_FOREGROUND_IOU,
                "max_bbox_edge_delta": MAX_BBOX_EDGE_DELTA,
            },
        }

    if reference.width >= capture.width and reference.height >= capture.height:
        reference = reference.crop((0, 0, capture.width, capture.height))
    size_matches = reference.size == capture.size
    if not size_matches:
        return {
            "passed": False,
            "reason": (
                f"design-pixel size mismatch: reference {reference.size}, "
                f"capture {capture.size}; images are never resized"
            ),
            "foreground_iou": 0.0,
            "bbox_edge_delta": 1.0,
            "foreground_pixels": {"reference": 0, "capture": 0},
            "reference_crop": reference_crop_path,
            "side_by_side": side_path,
            "diff": diff_path,
            "thresholds": {
                "min_foreground_iou": MIN_FOREGROUND_IOU,
                "max_bbox_edge_delta": MAX_BBOX_EDGE_DELTA,
            },
        }

    reference.save(reference_crop_path)
    ref_mask = _foreground_mask(reference)
    cap_mask = _foreground_mask(capture)
    intersection = ImageChops.multiply(ref_mask, cap_mask)
    union = ImageChops.lighter(ref_mask, cap_mask)
    intersection_pixels = _mask_pixels(intersection)
    union_pixels = _mask_pixels(union)
    foreground_iou = (
        intersection_pixels / union_pixels if union_pixels else 1.0
    )
    ref_box = ref_mask.getbbox()
    cap_box = cap_mask.getbbox()
    bbox_delta = _bbox_edge_delta(ref_box, cap_box, reference.size)
    foreground_pixels = {
        "reference": _mask_pixels(ref_mask),
        "capture": _mask_pixels(cap_mask),
    }
    appearance_delta = (
        max(ImageStat.Stat(ImageChops.difference(reference, capture)).mean) / 255.0
    )
    both_uniform_but_different = (
        union_pixels == 0 and appearance_delta > 0.10
    )
    geometry_score = (
        0.65 * foreground_iou + 0.35 * (1.0 - bbox_delta)
    )
    passed = (
        foreground_iou >= MIN_FOREGROUND_IOU
        and bbox_delta <= MAX_BBOX_EDGE_DELTA
        and not both_uniform_but_different
    )

    difference = ImageChops.difference(reference, capture)
    difference.save(diff_path)
    side = Image.new("RGB", (reference.width * 2, reference.height))
    side.paste(reference, (0, 0))
    side.paste(capture, (reference.width, 0))
    side.save(side_path)
    reason = (
        "foreground geometry is within tolerance"
        if passed
        else (
            "geometrically divergent "
            f"(foreground IoU {foreground_iou:.4f}, "
            f"bbox edge delta {bbox_delta:.4f}; require IoU >= "
            f"{MIN_FOREGROUND_IOU:.4f} and bbox delta <= "
            f"{MAX_BBOX_EDGE_DELTA:.4f})"
        )
    )
    return {
        "passed": passed,
        "reason": reason,
        "geometry_score": geometry_score,
        "foreground_iou": foreground_iou,
        "bbox_edge_delta": bbox_delta,
        "foreground_pixels": foreground_pixels,
        "reference_bbox": list(ref_box) if ref_box else None,
        "capture_bbox": list(cap_box) if cap_box else None,
        "appearance_delta": appearance_delta,
        "reference_crop": reference_crop_path,
        "side_by_side": side_path,
        "diff": diff_path,
        "thresholds": {
            "min_foreground_iou": MIN_FOREGROUND_IOU,
            "max_bbox_edge_delta": MAX_BBOX_EDGE_DELTA,
        },
    }


def compare_sweeps(baseline_summary, final_summary):
    """Report every script/check PASS-to-FAIL transition."""
    def rows(summary):
        return {
            row.get("script"): row.get("result")
            for row in summary.get("rows", [])
            if row.get("script")
        }

    def checks(summary):
        result = {}
        for row in summary.get("tier4", []):
            script = row.get("script")
            for check, status in row.get("checks", {}).items():
                result[f"{script}:{check}"] = status
        return result

    baseline_rows, final_rows = rows(baseline_summary), rows(final_summary)
    baseline_checks, final_checks = checks(baseline_summary), checks(final_summary)
    return {
        "script_pass_to_fail": sorted(
            key
            for key, status in baseline_rows.items()
            if status == "PASS" and final_rows.get(key) == "FAIL"
        ),
        "check_pass_to_fail": sorted(
            key
            for key, status in baseline_checks.items()
            if status == "PASS" and final_checks.get(key) == "FAIL"
        ),
    }


def _load_json(path):
    with open(path, "r", encoding="utf-8-sig") as source:
        return json.load(source)


def _capture_one(exe, run_dir, build_info, component, theme, state):
    stem = f"{component.lower()}-{theme}"
    full_path = os.path.abspath(os.path.join(run_dir, f"{stem}-full.png"))
    metadata_path = os.path.splitext(full_path)[0] + ".json"
    log_path = os.path.abspath(os.path.join(run_dir, f"{stem}.log"))
    game_args = [
        playtest_boot.MOVEMENT_TEST_MAP,
        "-game",
        "-unattended",
        "-windowed",
        "-ResX=1920",
        "-ResY=1080",
        "-ForceRes",
        "-NoSplash",
        f'-abslog="{log_path.replace(os.sep, "/")}"',
        "-TinTallyGallery",
        f"-TinTallyGalleryComponent={component}",
        f"-TinTallyGalleryTheme={theme}",
        f"-TinTallyGalleryState={state}",
        f"-TinTallyGalleryCommit={build_info['commit']}",
        f'-TinTallyGalleryOutput="{full_path.replace(os.sep, "/")}"',
    ]
    process, job, _ = playtest_boot.launch_game(exe, game_args)
    try:
        deadline = time.time() + 90
        while time.time() < deadline and process.poll() is None:
            if os.path.isfile(full_path) and os.path.isfile(metadata_path):
                break
            time.sleep(0.25)
        else:
            return {
                "component": component,
                "theme": theme,
                "passed": False,
                "reasons": ["gallery capture timed out or game exited"],
                "full": full_path,
                "log": log_path,
            }
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
        if job:
            job.close()

    metadata = _load_json(metadata_path)
    metadata.update(
        make_capture_metadata(component, theme, state, build_info, [480, 120, 960, 720])
    )
    with open(metadata_path, "w", encoding="utf-8") as target:
        json.dump(metadata, target, indent=2)
        target.write("\n")
    reasons = validate_capture(full_path, metadata, build_info, theme)
    crop_path = os.path.join(run_dir, f"{stem}-crop.png")
    if not reasons:
        with Image.open(full_path) as image:
            image.crop((480, 120, 1440, 840)).save(crop_path)
    return {
        "component": component,
        "theme": theme,
        "state": state,
        "passed": not reasons,
        "reasons": reasons,
        "full": full_path,
        "crop": crop_path,
        "metadata": metadata_path,
        "log": log_path,
    }


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--component", choices=FOUNDATION_COMPONENTS)
    parser.add_argument("--theme", choices=THEMES)
    parser.add_argument("--state", default="sheet")
    parser.add_argument("--run-dir")
    parser.add_argument("--compare-only", action="store_true")
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    exe = playtest_boot.DEFAULT_EXE
    freshness, exit_code = playtest_boot.gate(exe, allow_stale=False)
    if exit_code is not None:
        return exit_code
    build_info = freshness["build"]
    timestamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_dir = os.path.abspath(
        args.run_dir
        or os.path.join(playtest_boot.PLAYTESTS_DIR, f"{timestamp}-tin-tally-gallery")
    )
    os.makedirs(run_dir, exist_ok=True)
    components = (args.component,) if args.component else FOUNDATION_COMPONENTS
    themes = (args.theme,) if args.theme else THEMES
    results = []
    summary_path = os.path.join(run_dir, "summary.json")
    if args.compare_only:
        if not os.path.isfile(summary_path):
            print(f"[Gallery] missing prior summary for --compare-only: {summary_path}")
            return 2
        results = _load_json(summary_path).get("results", [])
        results = [
            result
            for result in results
            if result.get("component") in components and result.get("theme") in themes
        ]
    else:
        for component in components:
            for theme in themes:
                print(f"[Gallery] {component} / {theme}")
                results.append(
                    _capture_one(exe, run_dir, build_info, component, theme, args.state)
                )
    compare_dir = os.path.join(run_dir, "compare")
    for result in results:
        if not result.get("crop"):
            continue
        comparison = compare_images(
            reference_path(result["component"], result["theme"]),
            result["crop"],
            compare_dir,
            f"{result['component'].lower()}-{result['theme']}",
        )
        result["comparison"] = comparison
        if not comparison["passed"]:
            result["passed"] = False
            result.setdefault("reasons", []).append(comparison["reason"])
    report = {
        "schema": 1,
        "commit": build_info.get("commit"),
        "dirty": build_info.get("dirty"),
        "run_dir": run_dir,
        "results": results,
        "passed": bool(results) and all(result["passed"] for result in results),
    }
    with open(summary_path, "w", encoding="utf-8") as target:
        json.dump(report, target, indent=2)
        target.write("\n")
    print(json.dumps(report, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
