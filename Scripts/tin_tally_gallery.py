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

from PIL import Image, ImageChops, ImageFilter, ImageOps, ImageStat

import playtest_boot


FOUNDATION_COMPONENTS = ("Plate", "Tag", "Keycap", "Tally", "Glyphs")
THEMES = ("diorama", "high-contrast")
CAPTURE_SIZE = (1920, 1080)


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


def _geometry_mask(image):
    gray = ImageOps.grayscale(image.convert("RGB"))
    return gray.filter(ImageFilter.FIND_EDGES).point(lambda value: 255 if value >= 20 else 0)


def compare_images(reference_path, capture_path, output_dir, stem):
    """Compare one preview/capture pair and write side-by-side/diff evidence."""
    os.makedirs(output_dir, exist_ok=True)
    safe_stem = stem.replace("/", "_").replace("\\", "_")
    side_path = os.path.join(output_dir, f"{safe_stem}-side-by-side.png")
    diff_path = os.path.join(output_dir, f"{safe_stem}-diff.png")
    with Image.open(reference_path) as ref_source, Image.open(capture_path) as cap_source:
        reference = ref_source.convert("RGB")
        capture = cap_source.convert("RGB")
        if capture.size != reference.size:
            capture = ImageOps.fit(capture, reference.size, method=Image.Resampling.LANCZOS)
        ref_edges = _geometry_mask(reference)
        cap_edges = _geometry_mask(capture)
        edge_diff = ImageChops.difference(ref_edges, cap_edges)
        ref_shapes = ImageOps.grayscale(reference).point(
            lambda value: 255 if value >= 128 else 0
        )
        cap_shapes = ImageOps.grayscale(capture).point(
            lambda value: 255 if value >= 128 else 0
        )
        shape_diff = ImageChops.difference(ref_shapes, cap_shapes)
        divergence = max(
            ImageStat.Stat(edge_diff).mean[0],
            ImageStat.Stat(shape_diff).mean[0],
        )
        geometry_score = 1.0 - (divergence / 255.0)
        difference = ImageChops.difference(reference, capture)
        difference.save(diff_path)
        side = Image.new("RGB", (reference.width * 2, reference.height))
        side.paste(reference, (0, 0))
        side.paste(capture, (reference.width, 0))
        side.save(side_path)
    passed = geometry_score >= 0.90
    return {
        "passed": passed,
        "reason": (
            "geometry is within tolerance"
            if passed
            else f"geometrically divergent (score {geometry_score:.4f} < 0.9000)"
        ),
        "geometry_score": geometry_score,
        "side_by_side": side_path,
        "diff": diff_path,
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
    if not args.compare_only:
        for component in components:
            for theme in themes:
                print(f"[Gallery] {component} / {theme}")
                results.append(
                    _capture_one(exe, run_dir, build_info, component, theme, args.state)
                )
    report = {
        "schema": 1,
        "commit": build_info.get("commit"),
        "dirty": build_info.get("dirty"),
        "run_dir": run_dir,
        "results": results,
        "passed": bool(results) and all(result["passed"] for result in results),
    }
    with open(os.path.join(run_dir, "summary.json"), "w", encoding="utf-8") as target:
        json.dump(report, target, indent=2)
        target.write("\n")
    print(json.dumps(report, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
