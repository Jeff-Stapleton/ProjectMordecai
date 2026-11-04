"""Local build + preview harness for the Mordecai Tin Tally design system (tooling, never published).

  py -I render.py build                      assemble project/components/bundle.css + bundle.js, export icon SVGs
  py -I render.py render [Comp,Comp] [--themes diorama,high-contrast]
                                             render components/<Comp>/preview.html -> build/shots/<Comp>.<theme>.png
  py -I render.py check                      static checks of every preview (the full line-1 marker, banned tags,
                                             unknown classes/glyphs, missing README)

The harness mimics the design-system page: it preloads tokens.css (compiled from tokens.json), bundle.css,
bundle.js, and sets <html data-theme>. Each render run assembles into its own temp folder, so parallel
agents never clobber each other.
"""
import json, os, re, shutil, subprocess, sys, pathlib, tempfile, glob

DS = pathlib.Path(__file__).resolve().parent.parent / "ds"
PROJ = DS / "project"
CHROME = r"C:\Program Files\Google\Chrome\Application\chrome.exe"


def css_val(v):
    m = re.fullmatch(r"\{([A-Za-z0-9_.-]+)\}", v.strip())
    return f"var(--{m.group(1)})" if m else v


def compile_tokens_css(font_base_uri=None):
    t = json.loads((PROJ / "tokens.json").read_text(encoding="utf-8"))
    out = []
    themes = [th["id"] for th in t["color"]["themes"]]
    first = themes[0]
    for font in t["type"].get("fonts", []):
        f = font["file"] if "/" in font["file"] else "fonts/" + font["file"]
        url = (PROJ / f).resolve().as_uri()
        out.append('@font-face { font-family: "%s"; src: url("%s") format("woff2"); font-weight: %s; font-style: %s; font-display: block; }'
                   % (font["family"], url, font.get("weight", "400"), font.get("style", "normal")))
    for i, th in enumerate(themes):
        sel = f':root, [data-theme="{th}"]' if i == 0 else f'[data-theme="{th}"]'
        lines = []
        for tok in t["color"]["tokens"]:
            v = tok["value"]
            if isinstance(v, dict):
                v = v.get(th, v.get(first))
            elif i > 0:
                pass
            lines.append(f"  --{tok['name']}: {css_val(v)};")
        for tok in t.get("shadow", {}).get("tokens", []):
            v = tok["value"]
            if isinstance(v, dict):
                v = v.get(th, v.get(first))
            lines.append(f"  --{tok['name']}: {v};")
        out.append(sel + " {\n" + "\n".join(lines) + "\n}")
    lines = []
    for fam, block in t.items():
        if fam in ("color", "type", "shadow", "name", "version", "meta") or not isinstance(block, dict):
            continue
        for tok in block.get("tokens", []):
            v = tok["value"]
            if isinstance(v, (int, float)) and fam in ("spacing", "radius"):
                v = f"{v}px"
            lines.append(f"  --{tok['name']}: {v};")
    for k, stack in t["type"]["families"].items():
        lines.append(f"  --font-{k}: {stack};")
    out.append(":root {\n" + "\n".join(lines) + "\n}")
    for g in t["type"]["groups"]:
        for s in g["styles"]:
            fam = s.get("family", g.get("family"))
            decl = [f"font-family: var(--font-{fam})", f"font-size: {s['fontSize']}"]
            for k, css in (("lineHeight", "line-height"), ("fontWeight", "font-weight"), ("letterSpacing", "letter-spacing"), ("fontStyle", "font-style")):
                if k in s:
                    decl.append(f"{css}: {s[k]}")
            out.append(f".{s['name']} {{ " + "; ".join(decl) + "; }")
    return "\n".join(out) + "\n", themes


def load_glyphs():
    g = {}
    for p in sorted(glob.glob(str(DS / "glyphs" / "*.json"))):
        try:
            g.update(json.loads(pathlib.Path(p).read_text(encoding="utf-8")))
        except Exception as e:
            print("GLYPH FILE BROKEN", p, e)
    return g


def bundle_css():
    parts = [(DS / "css" / "_core.css").read_text(encoding="utf-8")]
    for p in sorted(glob.glob(str(DS / "css" / "*.css"))):
        if os.path.basename(p) == "_core.css":
            continue
        parts.append(f"\n/* ===== {os.path.basename(p)[:-4]} ===== */\n" + pathlib.Path(p).read_text(encoding="utf-8"))
    return "".join(parts)


def bundle_js():
    src = (DS / "js" / "core.js").read_text(encoding="utf-8")
    glyphs = load_glyphs()
    js = src.replace("__GLYPHS__", json.dumps(glyphs, separators=(",", ":"), sort_keys=True))
    return js, glyphs


def assemble(outdir):
    outdir = pathlib.Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    tcss, themes = compile_tokens_css()
    (outdir / "tokens.css").write_text(tcss, encoding="utf-8")
    (outdir / "bundle.css").write_text(bundle_css(), encoding="utf-8")
    js, _ = bundle_js()
    (outdir / "bundle.js").write_text(js, encoding="utf-8")
    return themes


def build():
    css = bundle_css()
    js, glyphs = bundle_js()
    for bad in ("</script", "<!--"):
        if bad in js.lower():
            print("BUNDLE.JS CONTAINS", bad)
    if "</style" in css.lower():
        print("BUNDLE.CSS CONTAINS </style")
    (PROJ / "components").mkdir(parents=True, exist_ok=True)
    (PROJ / "components" / "bundle.css").write_text(css, encoding="utf-8")
    (PROJ / "components" / "bundle.js").write_text(js, encoding="utf-8")
    icons = DS / "assets_out" / "Icons"
    icons.mkdir(parents=True, exist_ok=True)
    for name, body in glyphs.items():
        svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 24 24" fill="#0F1418">'
               + body.replace("currentColor", "#0F1418") + "</svg>\n")
        (icons / f"{name}.svg").write_text(svg, encoding="utf-8")
    print(f"bundle.css {len(css)//1024} KB, bundle.js {len(js)//1024} KB, {len(glyphs)} glyphs exported")


def render(only=None, themes=None):
    (DS / "build").mkdir(parents=True, exist_ok=True)
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="ttbuild_", dir=str(DS / "build")))
    try:
        _render(tmp, only, themes)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)  # the per-run bundle and Chrome profiles


def _render(tmp, only, themes):
    all_themes = assemble(tmp)
    themes = themes or all_themes
    shots = DS / "build" / "shots"
    shots.mkdir(parents=True, exist_ok=True)
    inject = (f'<link rel="stylesheet" href="{(tmp / "tokens.css").as_uri()}">'
              f'<link rel="stylesheet" href="{(tmp / "bundle.css").as_uri()}">'
              f'<script src="{(tmp / "bundle.js").as_uri()}"></script>')
    for p in sorted((PROJ / "components").glob("*/preview.html")):
        name = p.parent.name
        if only and name not in only:
            continue
        src = p.read_text(encoding="utf-8")
        m = re.match(r"\s*<!--\s*@dsCard([^>]*)-->", src)
        height, width = 120, 960
        if m:
            hm = re.search(r"height=(\d+)", m.group(1)); wm = re.search(r"width=(\d+)", m.group(1))
            height = int(hm.group(1)) if hm else 120
            width = int(wm.group(1)) if wm else 960
        for th in themes:
            html = re.sub(r"<html([^>]*)>", lambda mm: f'<html{mm.group(1)} data-theme="{th}">', src, count=1)
            if "data-theme" not in html:
                html = f'<html data-theme="{th}">' + html
            html = html.replace("<head>", "<head>" + inject, 1) if "<head>" in html else inject + html
            page = p.parent / f"_harness.{os.getpid()}.{th}.html"
            page.write_text(html, encoding="utf-8")
            out = shots / f"{name}.{th}.png"
            if out.exists():
                out.unlink()
            cmd = [CHROME, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--allow-file-access-from-files",
                   f"--user-data-dir={tmp / ('chrome-' + th)}", f"--window-size={width},{max(height, 40)}",
                   "--virtual-time-budget=5000", f"--screenshot={out}", page.as_uri()]
            try:
                subprocess.run(cmd, capture_output=True, timeout=120)
            finally:
                page.unlink(missing_ok=True)
            print(("OK  " if out.exists() else "FAIL"), f"{name}.{th}.png", f"{width}x{height}", str(out))


KNOWN_CLASS_RE = re.compile(r'class="([^"]+)"')

# Line 1 of every preview (BUILD.md 'Line 1 is the marker'): exactly group, height, width, subtitle, in that order,
# single spaces, nothing before "<!--". The design-system format also knows two bare flags, page (a showcase page)
# and floor (a generated no-example card); they may follow subtitle, in that order. No card uses them today.
MARKER_RE = re.compile(r'^<!-- @dsCard group="([^"]+)" height=(\d+) width=(\d+) subtitle="([^"]*)"( page)?( floor)? -->$')
MARKER_GROUPS = ("Foundations", "HUD", "World", "Tally", "Menus", "Front end")


def check():
    css = bundle_css() + compile_tokens_css()[0]
    defined = set(re.findall(r"\.([A-Za-z][A-Za-z0-9_-]*)", css))
    glyphs = load_glyphs()
    problems = 0
    for p in sorted((PROJ / "components").glob("*/preview.html")):
        name = p.parent.name
        src = p.read_text(encoding="utf-8")
        first = src.splitlines()[0] if src else ""
        mk = MARKER_RE.match(first)
        if not re.match(r"<!--\s*@dsCard", first):
            print(f"{name}: line 1 is not the @dsCard marker"); problems += 1
        elif not mk:
            print(f'{name}: line 1 is not exactly <!-- @dsCard group="..." height=N width=N subtitle="..." --> '
                  f"(single spaces, that order): {first[:120]}"); problems += 1
        elif mk.group(1) not in MARKER_GROUPS:
            print(f'{name}: marker group "{mk.group(1)}" is not one of {", ".join(MARKER_GROUPS)}'); problems += 1
        for tag in ("<iframe", "<frame", "<object", "<embed", "<portal", "<noscript"):
            if tag in src.lower():
                print(f"{name}: banned tag {tag}"); problems += 1
        if len(src.encode("utf-8")) > 256 * 1024:
            print(f"{name}: preview over 256 KB"); problems += 1
        own = set(re.findall(r"\.([A-Za-z][A-Za-z0-9_-]*)", " ".join(re.findall(r"<style[^>]*>([\s\S]*?)</style>", src))))
        for cls in KNOWN_CLASS_RE.findall(src):
            for c in cls.split():
                if c.startswith("tt-") and c not in defined and c not in own:
                    print(f"{name}: class .{c} is not defined in bundle.css"); problems += 1
        for g in re.findall(r'data-glyph="([^"]+)"', src):
            if glyphs and g not in glyphs:
                print(f"{name}: glyph '{g}' not in the glyph set"); problems += 1
        readme = p.parent / "README.md"
        if not readme.exists():
            print(f"{name}: no README.md"); problems += 1
    for p in sorted((PROJ / "components").glob("*/README.md")):
        if not (p.parent / "preview.html").exists() and p.parent.name not in ("Cover",):
            print(f"{p.parent.name}: README without preview")
    print("problems:", problems)


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "render"
    if cmd == "build":
        build()
    elif cmd == "check":
        check()
    else:
        only = set(sys.argv[2].split(",")) if len(sys.argv) > 2 and not sys.argv[2].startswith("--") else None
        themes = sys.argv[sys.argv.index("--themes") + 1].split(",") if "--themes" in sys.argv else None
        render(only, themes)
