#!/usr/bin/env python3
"""Packaged Tin Tally visual-gallery capture and comparison harness.

US-103 RED scaffold: the public helpers exist so the tests compile and fail at
assertion level. GREEN implements the package freshness gate, metadata checks,
image comparison/evidence, packaged capture orchestration, and sweep diff.
"""

import argparse
import os


FOUNDATION_COMPONENTS = ("Plate", "Tag", "Keycap", "Tally", "Glyphs")
THEMES = ("diorama", "high-contrast")
CAPTURE_SIZE = (1920, 1080)


def make_capture_metadata(component, theme, state, build_info, crop_rect):
    """Return deterministic capture metadata for one gallery frame."""
    return {}


def validate_capture(image_path, metadata, build_info, expected_theme):
    """Return a list of rejection reasons; an empty list means valid."""
    return ["US-103 gallery validation is not implemented"]


def compare_images(reference_path, capture_path, output_dir, stem):
    """Compare one preview/capture pair and write side-by-side/diff evidence."""
    return {
        "passed": False,
        "reason": "US-103 gallery comparison is not implemented",
        "geometry_score": 0.0,
        "side_by_side": None,
        "diff": None,
    }


def compare_sweeps(baseline_summary, final_summary):
    """Report every script/check PASS-to-FAIL transition."""
    return {"script_pass_to_fail": [], "check_pass_to_fail": []}


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
    print("Tin Tally gallery RED scaffold: capture/compare not implemented")
    if args.run_dir:
        print(os.path.abspath(args.run_dir))
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
