#!/usr/bin/env python3
"""Packaged Tin Tally visual-gallery capture and comparison harness."""

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
REFERENCE_CROP_SIZE = (960, 720)
MIN_FOREGROUND_IOU = 0.08
MAX_BBOX_EDGE_DELTA = 0.20
MIN_EDGE_GRID_SIMILARITY = 0.28
MIN_TEXT_ROW_SIMILARITY = 0.15
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REFERENCE_SHOTS_DIR = os.path.join(
    ROOT, "Design", "TinTally", "ds", "build", "shots"
)
TOKENS_PATH = os.path.join(
    ROOT, "Design", "TinTally", "ds", "project", "tokens.json"
)
TEXT_BANDS = {
    "plate": ((18, 54), (60, 292), (324, 354), (386, 442), (478, 620)),
    "tag": ((18, 54), (252, 308), (326, 358), (390, 450), (470, 570), (620, 680)),
    "keycap": ((18, 54), (68, 108), (182, 250), (288, 326), (388, 418), (478, 508), (526, 558), (582, 620)),
    "tally": ((18, 54), (70, 112), (124, 238), (270, 304), (314, 510), (536, 570), (586, 692)),
    "glyphs": ((18, 54), (174, 208), (328, 362), (482, 516), (636, 670), (682, 718)),
}
# Chrome and Slate rasterize the same font and large solid regions differently,
# especially in High Contrast. These per-sheet floors preserve a meaningful
# edge/text gate without pretending the renderers have one global density.
# They are intentionally above the retained round-two Tally fixture
# (edge-grid 0.3376, text-row 0.2362), which Tally/Diorama must reject.
TEXT_EDGE_THRESHOLDS = {
    ("plate", "diorama"): (0.32, 0.30),
    ("plate", "high-contrast"): (0.30, 0.30),
    ("tag", "diorama"): (0.38, 0.28),
    ("tag", "high-contrast"): (0.33, 0.27),
    ("keycap", "diorama"): (0.39, 0.19),
    ("keycap", "high-contrast"): (0.31, 0.18),
    ("tally", "diorama"): (0.34, 0.25),
    ("tally", "high-contrast"): (0.28, 0.22),
    ("glyphs", "diorama"): (0.36, 0.16),
    ("glyphs", "high-contrast"): (0.32, 0.15),
}
LOCALIZED_REGIONS = {
    "tally": {
        "captions": ((40, 190, 304, 320), (344, 190, 608, 320), (648, 190, 912, 320)),
        "rarity": ((344, 108, 608, 174),),
        "seal_throbber": ((648, 108, 912, 174),),
        "anatomy": ((24, 402, 354, 602),),
    },
    "keycap": {
        "wide_keys": ((330, 108, 836, 166),),
        "mouse": ((40, 174, 286, 224),),
        "gamepad_spacing": ((228, 332, 748, 430),),
        "state_headers": ((140, 578, 902, 620),),
    },
    "glyphs": {
        "tile_chips": ((24, 56, 936, 558),),
        "labels": (
            (24, 116, 936, 148),
            (24, 216, 936, 248),
            (24, 316, 936, 348),
            (24, 416, 936, 448),
            (24, 516, 936, 548),
        ),
    },
}
# Localized comparisons use 12 px density cells, then combine edge-map and
# foreground-mask overlap. These floors are deliberately stricter than the
# whole-sheet gates: a correct heading cannot compensate for a missing mouse,
# tally detail, inverse glyph, or label inside the component's design region.
LOCALIZED_MIN_SCORE = {
    "tally": {
        "captions": 0.28,
        "rarity": 0.18,
        "seal_throbber": 0.28,
        "anatomy": 0.28,
    },
    "keycap": {
        "wide_keys": 0.52,
        "mouse": 0.35,
        "gamepad_spacing": 0.20,
        "state_headers": 0.28,
    },
    "glyphs": {
        "tile_chips": 0.60,
        "labels": 0.60,
    },
}


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


def validate_capture(
    image_path,
    metadata,
    build_info,
    expected_theme,
    expected_component=None,
    expected_state="sheet",
):
    """Return a list of rejection reasons; an empty list means valid."""
    reasons = []
    if not os.path.isfile(image_path):
        return [f"capture is missing: {image_path}"]
    try:
        with Image.open(image_path) as source:
            image = source.convert("RGB")
    except (OSError, ValueError) as exc:
        return [f"capture is unreadable: {exc}"]

    if image.size != CAPTURE_SIZE:
        reasons.append(
            f"capture image size must be 1920x1080; got {image.size[0]}x{image.size[1]}"
        )
    if metadata.get("resolution") != list(CAPTURE_SIZE):
        reasons.append(
            f"capture metadata must be 1920x1080; got {metadata.get('resolution')}"
        )
    if metadata.get("theme") != expected_theme:
        reasons.append(
            f"theme mismatch: expected {expected_theme}, got {metadata.get('theme')}"
        )
    if (
        expected_component
        and str(metadata.get("component", "")).casefold()
        != expected_component.casefold()
    ):
        reasons.append(
            "component mismatch: "
            f"expected {expected_component}, got {metadata.get('component')}"
        )
    if metadata.get("state") != expected_state:
        reasons.append(
            f"state mismatch: expected {expected_state}, got {metadata.get('state')}"
        )
    if metadata.get("commit") != build_info.get("commit"):
        reasons.append(
            f"stale capture commit {metadata.get('commit')} != {build_info.get('commit')}"
        )
    if build_info.get("dirty"):
        reasons.append("package metadata says dirty=true")
    if not metadata.get("build_stamp"):
        reasons.append("capture metadata has no game build stamp")
    if metadata.get("package_built_at") != build_info.get("built_at"):
        reasons.append(
            "capture package timestamp does not match the clean package metadata"
        )

    extrema = image.getextrema()
    if all(channel[1] <= 2 for channel in extrema):
        reasons.append("capture is black")
    if image.size == CAPTURE_SIZE:
        expected_plate = _theme_plate_rgb(expected_theme)
        sample = image.getpixel((CAPTURE_SIZE[0] - 490, 120 + 710))
        if max(abs(sample[index] - expected_plate[index]) for index in range(3)) > 12:
            reasons.append(
                "sheet plate pixel does not match the resolved "
                f"{expected_theme} plate token: got {sample}, expected {expected_plate}"
            )
    return reasons


def _theme_plate_rgb(theme):
    data = _load_json(TOKENS_PATH)
    plate = next(
        token["value"]
        for token in data["color"]["tokens"]
        if token["name"] == "plate"
    )
    value = plate[theme] if isinstance(plate, dict) else plate
    value = value.lstrip("#")
    return tuple(int(value[index:index + 2], 16) for index in (0, 2, 4))


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


def _edge_mask(image):
    edges = ImageOps.grayscale(image).filter(ImageFilter.FIND_EDGES)
    return edges.point(lambda pixel: 255 if pixel >= 24 else 0)


def _density_overlap(reference_values, capture_values):
    union = sum(max(reference, capture) for reference, capture in zip(reference_values, capture_values))
    if union <= 1e-9:
        return 1.0
    return (
        sum(min(reference, capture) for reference, capture in zip(reference_values, capture_values))
        / union
    )


def _grid_density(mask, columns=48, rows=36):
    width, height = mask.size
    pixels = mask.load()
    values = []
    for row in range(rows):
        y0, y1 = row * height // rows, (row + 1) * height // rows
        for column in range(columns):
            x0, x1 = column * width // columns, (column + 1) * width // columns
            area = max(1, (x1 - x0) * (y1 - y0))
            values.append(
                sum(
                    pixels[x, y] > 0
                    for y in range(y0, y1)
                    for x in range(x0, x1)
                )
                / area
            )
    return values


def _text_row_density(mask, bands):
    width = mask.width
    pixels = mask.load()
    values = []
    for y0, y1 in bands:
        for y in range(max(0, y0), min(mask.height, y1)):
            values.append(sum(pixels[x, y] > 0 for x in range(width)) / width)
    return values


def _comparison_thresholds(component=None, theme=None):
    edge_grid, text_row = TEXT_EDGE_THRESHOLDS.get(
        (component, theme),
        (MIN_EDGE_GRID_SIMILARITY, MIN_TEXT_ROW_SIMILARITY),
    )
    return {
        "min_foreground_iou": MIN_FOREGROUND_IOU,
        "max_bbox_edge_delta": MAX_BBOX_EDGE_DELTA,
        "edge_grid": edge_grid,
        "text_row": text_row,
    }


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


def _localized_region_metrics(reference_edges, capture_edges, ref_mask, cap_mask, component):
    """Compare canonical component regions without allowing cross-region credit."""
    region_thresholds = LOCALIZED_MIN_SCORE.get(component)
    if region_thresholds is None:
        return {}
    results = {}
    for name, rectangles in LOCALIZED_REGIONS[component].items():
        threshold = region_thresholds[name]
        reference_edge_values = []
        capture_edge_values = []
        reference_foreground_values = []
        capture_foreground_values = []
        for rectangle in rectangles:
            width = max(1, rectangle[2] - rectangle[0])
            height = max(1, rectangle[3] - rectangle[1])
            columns = max(1, width // 12)
            rows = max(1, height // 12)
            reference_edge_values.extend(
                _grid_density(reference_edges.crop(rectangle), columns, rows)
            )
            capture_edge_values.extend(
                _grid_density(capture_edges.crop(rectangle), columns, rows)
            )
            reference_foreground_values.extend(
                _grid_density(ref_mask.crop(rectangle), columns, rows)
            )
            capture_foreground_values.extend(
                _grid_density(cap_mask.crop(rectangle), columns, rows)
            )
        edge_similarity = _density_overlap(
            reference_edge_values, capture_edge_values
        )
        foreground_similarity = _density_overlap(
            reference_foreground_values, capture_foreground_values
        )
        score = 0.7 * edge_similarity + 0.3 * foreground_similarity
        results[name] = {
            "score": score,
            "edge_similarity": edge_similarity,
            "foreground_similarity": foreground_similarity,
            "threshold": threshold,
            "passed": score >= threshold,
        }
    return results


def reference_path(component, theme):
    return os.path.join(REFERENCE_SHOTS_DIR, f"{component}.{theme}.png")


def compare_images(reference_path, capture_path, output_dir, stem):
    """Compare foreground geometry without resizing either design-pixel image.

    Acceptance requires foreground-mask IoU >= 0.08 and every foreground
    bounding-box edge within 20% of the reference viewport, plus the documented
    per-sheet edge-grid and text-row floors. The IoU tolerance allows
    browser/Slate font rasterization differences; the other gates reject the
    former stretched, all-caps, and structurally incomplete fixtures.
    """
    os.makedirs(output_dir, exist_ok=True)
    safe_stem = stem.replace("/", "_").replace("\\", "_")
    normalized_stem = safe_stem.lower().replace(".", "-")
    component = normalized_stem.split("-")[0]
    theme = "high-contrast" if "high-contrast" in normalized_stem else "diorama"
    thresholds = _comparison_thresholds(component, theme)
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
            "edge_grid_similarity": 0.0,
            "text_row_similarity": 0.0,
            "foreground_pixels": {"reference": 0, "capture": 0},
            "reference_crop": reference_crop_path,
            "side_by_side": side_path,
            "diff": diff_path,
            "thresholds": thresholds,
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
            "edge_grid_similarity": 0.0,
            "text_row_similarity": 0.0,
            "foreground_pixels": {"reference": 0, "capture": 0},
            "reference_crop": reference_crop_path,
            "side_by_side": side_path,
            "diff": diff_path,
            "thresholds": thresholds,
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
    reference_edges = _edge_mask(reference)
    capture_edges = _edge_mask(capture)
    edge_grid_similarity = _density_overlap(
        _grid_density(reference_edges),
        _grid_density(capture_edges),
    )
    text_bands = TEXT_BANDS.get(component, ((0, reference.height),))
    text_row_similarity = _density_overlap(
        _text_row_density(reference_edges, text_bands),
        _text_row_density(capture_edges, text_bands),
    )
    localized_regions = _localized_region_metrics(
        reference_edges,
        capture_edges,
        ref_mask,
        cap_mask,
        component,
    )
    localized_passed = all(
        region["passed"] for region in localized_regions.values()
    )
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
        foreground_iou >= thresholds["min_foreground_iou"]
        and bbox_delta <= thresholds["max_bbox_edge_delta"]
        and edge_grid_similarity >= thresholds["edge_grid"]
        and text_row_similarity >= thresholds["text_row"]
        and localized_passed
        and not both_uniform_but_different
    )

    difference = ImageChops.difference(reference, capture)
    difference.save(diff_path)
    side = Image.new("RGB", (reference.width * 2, reference.height))
    side.paste(reference, (0, 0))
    side.paste(capture, (reference.width, 0))
    side.save(side_path)
    localized_summary = ", ".join(
        f"{name} {region['score']:.4f}/{region['threshold']:.4f}"
        for name, region in localized_regions.items()
    )
    reason = (
        "foreground geometry is within tolerance"
        if passed
        else (
            "geometrically, text/edge, or localized-region divergent "
            f"(foreground IoU {foreground_iou:.4f}, "
            f"bbox edge delta {bbox_delta:.4f}, "
            f"edge-grid {edge_grid_similarity:.4f}, "
            f"text-row {text_row_similarity:.4f}; require IoU >= "
            f"{thresholds['min_foreground_iou']:.4f}, bbox delta <= "
            f"{thresholds['max_bbox_edge_delta']:.4f}, edge-grid >= "
            f"{thresholds['edge_grid']:.4f}, and text-row >= "
            f"{thresholds['text_row']:.4f}; localized: "
            f"{localized_summary or 'not applicable'})"
        )
    )
    return {
        "passed": passed,
        "reason": reason,
        "geometry_score": geometry_score,
        "foreground_iou": foreground_iou,
        "bbox_edge_delta": bbox_delta,
        "edge_grid_similarity": edge_grid_similarity,
        "text_row_similarity": text_row_similarity,
        "localized_regions": localized_regions,
        "foreground_pixels": foreground_pixels,
        "reference_bbox": list(ref_box) if ref_box else None,
        "capture_bbox": list(cap_box) if cap_box else None,
        "appearance_delta": appearance_delta,
        "reference_crop": reference_crop_path,
        "side_by_side": side_path,
        "diff": diff_path,
        "thresholds": thresholds,
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
    for stale_path in (full_path, metadata_path):
        try:
            os.remove(stale_path)
        except FileNotFoundError:
            pass
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
        f"-TinTallyGalleryBuiltAt={build_info['built_at']}",
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
    reasons = validate_capture(
        full_path,
        metadata,
        build_info,
        theme,
        expected_component=component,
        expected_state=state,
    )
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
        result["compare_failures"] = []
        comparison = compare_images(
            reference_path(result["component"], result["theme"]),
            result["crop"],
            compare_dir,
            f"{result['component'].lower()}-{result['theme']}",
        )
        result["comparison"] = comparison
        result["passed"] = not result.get("reasons") and comparison["passed"]
        if not comparison["passed"]:
            result["passed"] = False
            result["compare_failures"].append(comparison["reason"])
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
