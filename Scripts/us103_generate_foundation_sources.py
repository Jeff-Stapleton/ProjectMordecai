#!/usr/bin/env python3
"""Generate deterministic PNG import sources for the US-103 Tin Tally assets."""

import argparse
import html
import os
import pathlib
import re
import subprocess
import tempfile

from PIL import Image, ImageDraw


ROOT = pathlib.Path(__file__).resolve().parent.parent
ICONS = ROOT / "Design" / "TinTally" / "ds" / "assets_out" / "Icons"
CHROME = pathlib.Path(r"C:\Program Files\Google\Chrome\Application\chrome.exe")


def asset_name(value):
    return re.sub(r"[^A-Za-z0-9_]", "_", value)


def render_glyphs(output):
    svgs = sorted(ICONS.glob("*.svg"))
    if len(svgs) != 117:
        raise RuntimeError(f"expected 117 exported glyphs, found {len(svgs)}")
    columns, cell = 13, 64
    rows = (len(svgs) + columns - 1) // columns
    page = [
        "<!doctype html><style>",
        f"*{{box-sizing:border-box}}html,body{{margin:0;width:{columns*cell}px;"
        f"height:{rows*cell}px;background:transparent}}",
        f"main{{display:grid;grid-template-columns:repeat({columns},{cell}px)}}",
        f"div{{width:{cell}px;height:{cell}px;padding:8px}}",
        "img{width:48px;height:48px;filter:brightness(0) invert(1)}",
        "</style><main>",
    ]
    for svg in svgs:
        page.append(f'<div><img src="{html.escape(svg.resolve().as_uri())}"></div>')
    page.append("</main>")
    with tempfile.TemporaryDirectory(prefix="us103-glyphs-") as tmp:
        html_path = pathlib.Path(tmp) / "glyphs.html"
        sheet_path = pathlib.Path(tmp) / "glyphs.png"
        html_path.write_text("".join(page), encoding="utf-8")
        subprocess.run(
            [
                str(CHROME),
                "--headless=new",
                "--disable-gpu",
                "--hide-scrollbars",
                "--default-background-color=00000000",
                f"--window-size={columns*cell},{rows*cell}",
                f"--user-data-dir={pathlib.Path(tmp) / 'profile'}",
                f"--screenshot={sheet_path}",
                html_path.resolve().as_uri(),
            ],
            check=True,
            capture_output=True,
            timeout=120,
        )
        with Image.open(sheet_path) as sheet:
            for index, svg in enumerate(svgs):
                x, y = (index % columns) * cell, (index // columns) * cell
                crop = sheet.crop((x, y, x + cell, y + cell)).convert("RGBA")
                crop.save(output / f"T_UI_Glyph_{asset_name(svg.stem)}.png")


def draw_foundation(output):
    hatch = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    draw = ImageDraw.Draw(hatch)
    for offset in (-16, 0, 16):
        draw.line((offset, 16, offset + 16, 0), fill="white", width=2)
    hatch.save(output / "T_UI_Hatch16.png")

    atlas = Image.new("RGBA", (384, 128), (0, 0, 0, 0))
    draw = ImageDraw.Draw(atlas)

    def rounded_line(points, width, fill="white"):
        """Pillow's joint curve does not round terminal caps, so add them."""
        draw.line(points, fill=fill, width=width)
        radius = width / 2
        for x, y in (points[0], points[-1]):
            draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=fill)

    # Frame zero is the dotted capacity layer. Filled frames contain only lit
    # marks and are composited over frame zero by UMordecaiTallyWidget.
    upright_x = (12, 24, 36, 48)
    for x in upright_x:
        for y in range(14, 115, 10):
            draw.ellipse((x - 2, y - 2, x + 2, y + 2), fill=(255, 255, 255, 180))
    for x, y in ((8, 114), (32, 64), (56, 14)):
        draw.ellipse((x - 2, y - 2, x + 2, y + 2), fill=(255, 255, 255, 180))

    for frame in range(1, 6):
        left = frame * 64
        for mark in range(min(frame, 4)):
            rounded_line(
                ((left + upright_x[mark], 14), (left + upright_x[mark], 114)),
                7,
            )
        if frame == 5:
            rounded_line(((left + 8, 114), (left + 56, 14)), 9)
    atlas.save(output / "T_UI_TallyAtlas.png")

    badges = {
        "T_UI_Badge_FeatCommon": "ellipse",
        "T_UI_Badge_FeatRare": "hex",
        "T_UI_Badge_FeatLegendary": "star",
        "T_UI_Badge_Technique": "square",
        "T_UI_Badge_KeyTag": "tag",
    }
    for name, shape in badges.items():
        image = Image.new("RGBA", (96, 96), (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        if shape == "ellipse":
            draw.ellipse((10, 10, 86, 86), fill="white")
        elif shape == "hex":
            draw.polygon(((24, 8), (72, 8), (90, 48), (72, 88), (24, 88), (6, 48)), fill="white")
        elif shape == "star":
            points = []
            import math
            for i in range(16):
                radius = 42 if i % 2 == 0 else 27
                angle = -math.pi / 2 + i * math.pi / 8
                points.append((48 + math.cos(angle) * radius, 48 + math.sin(angle) * radius))
            draw.polygon(points, fill="white")
        elif shape == "square":
            draw.rounded_rectangle((8, 8, 88, 88), radius=15, fill="white")
        else:
            draw.rounded_rectangle((7, 20, 89, 76), radius=13, fill="white")
            draw.ellipse((15, 39, 25, 49), fill=(0, 0, 0, 0))
        image.save(output / f"{name}.png")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        default=os.path.join(os.environ.get("TEMP", "."), "us103-foundation"),
    )
    args = parser.parse_args(argv)
    output = pathlib.Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    render_glyphs(output)
    draw_foundation(output)
    print(f"generated {len(list(output.glob('*.png')))} PNG sources in {output}")


if __name__ == "__main__":
    main()
