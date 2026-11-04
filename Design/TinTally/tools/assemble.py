"""Assemble the Tin Tally publish tree for the Design System artifact.

  py -I assemble.py              -> Design/TinTally/build/publish/project/... plus files_map.json

It rebuilds the bundle (render.py build), copies the publishable files, adds BUILD.md as the brand-book section
05-build-contract.md (without its repo paths block), and writes project/design-system.json, the artifact's index,
with the Icons asset group from icon_ids.txt (glyph name -> uploaded asset id). See Design/TinTally/README.md.
"""
import datetime, glob, json, pathlib, shutil, subprocess, sys

HERE = pathlib.Path(__file__).resolve().parent
TT = HERE.parent                      # Design/TinTally
DS = TT / "ds"
P = DS / "project"
ROOT = TT / "build" / "publish"
OUT = ROOT / "project"
ICONS = DS / "assets_out" / "Icons"   # written by render.py build
NOW = sys.argv[1] if len(sys.argv) > 1 else datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
BY = "Jeff Stapleton"

r = subprocess.run([sys.executable, "-I", "render.py", "build"], cwd=HERE, capture_output=True, text=True)
print(r.stdout.strip(), r.stderr.strip())
if r.returncode:
    sys.exit("render.py build failed")

if ROOT.exists():
    shutil.rmtree(ROOT)
OUT.mkdir(parents=True)


def clean(text):
    return text.replace("BUILD.md", "the build contract (05-build-contract.md)")


files = []


def put(rel, data):
    p = OUT / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(data, bytes):
        p.write_bytes(data)
    else:
        p.write_text(data, encoding="utf-8", newline="\n")
    files.append("project/" + rel)


for name in ["README.md", "01-voice-and-copy.md", "02-hud-and-pacing.md", "03-building-in-unreal.md", "04-open-decisions.md", "tokens.json"]:
    put(name, clean((P / name).read_text(encoding="utf-8")))
for f in sorted((P / "fonts").glob("*.woff2")):
    put("fonts/" + f.name, f.read_bytes())
put("assets/Icons/README.md", (P / "assets/Icons/README.md").read_text(encoding="utf-8"))
put("components/bundle.css", clean((P / "components/bundle.css").read_text(encoding="utf-8")))
put("components/bundle.js", (P / "components/bundle.js").read_text(encoding="utf-8"))
comps = []
for d in sorted((P / "components").iterdir()):
    if not d.is_dir():
        continue
    for fn in ("README.md", "preview.html"):
        if (d / fn).exists():
            put(f"components/{d.name}/{fn}", clean((d / fn).read_text(encoding="utf-8")))
    stray = [x.name for x in d.iterdir() if x.name not in ("README.md", "preview.html")]
    if stray:
        print("NOT PUBLISHED (stray)", d.name, stray)
    comps.append(d.name)

# BUILD.md as a brand-book section, without the repo paths block (lines from "Paths (" to the first blank line).
out, skip = [], False
for line in (DS / "BUILD.md").read_text(encoding="utf-8").splitlines():
    if line.startswith("Paths (relative to"):
        skip = True
        out.append("Where things live in this system: `tokens.json` holds every token; `components/bundle.css` is `_core.css` "
                   "(the shared primitives) followed by one partial per component; `components/bundle.js` is `window.TinTally` "
                   "(glyphs, tally strokes, rarity badges); the brand book is `README.md` and sections 01-04, and `03` is the "
                   "canonical asset manifest (D21). The sources live in the game repo under `Design/TinTally/`; the repo "
                   "(`Specs/`, `Source/LyraGame/Mordecai/`, `Config/DefaultInput.ini`) is the truth for numbers, names and bindings.")
        continue
    if skip:
        if line.strip() == "":
            skip = False
            out.append("")
        continue
    out.append(line.replace("## CSS partial rules (ds/css/<Comp>.css)", "## CSS partial rules (one partial per component)"))
txt = "\n".join(out) + "\n"
txt = txt.replace("# Tin Tally — build contract for component authors", "# Build contract", 1).replace("BUILD.md", "this contract")
put("05-build-contract.md", txt)

# The index.
ids = dict(l.split() for l in (HERE / "icon_ids.txt").read_text().splitlines() if l.strip())
order = []
for p in sorted(glob.glob(str(DS / "glyphs" / "*.json"))):
    for k in json.loads(pathlib.Path(p).read_text(encoding="utf-8")):
        if k not in order:
            order.append(k)
missing = [k for k in order if k not in ids]
if missing:
    sys.exit(f"glyphs without an uploaded asset id (upload ds/assets_out/Icons/<name>.svg, add to icon_ids.txt): {missing}")
recs = {f"{k}.svg": {"name": f"{k}.svg", "blob": ids[k], "size": (ICONS / f"{k}.svg").stat().st_size, "type": "image/svg+xml"}
        for k in order}
index = {
    "v": 3, "layout": "files",
    "createdOnFiles": {"v": 1, "at": "2026-11-04T00:37:20Z"},
    "title": "Mordecai · Tin Tally",
    "namespace": "TinTally",
    "libraries": [],
    "sections": {}, "groups": ["Icons"],
    "assetGroups": {"Icons": {"name": "Icons", "tile": "s", "order": [f"{k}.svg" for k in order], "files": recs}},
    "blobs": {}, "docs": {"readme": "project/README.md", "sections": []},
    "lastChange": {"by": BY, "at": NOW, "via": "Claude Code", "note": "Tin Tally from Design/TinTally"},
}
(OUT / "design-system.json").write_text(json.dumps(index, indent=1, ensure_ascii=False), encoding="utf-8", newline="\n")

total = sum((ROOT / f).stat().st_size for f in files)
print(f"{len(files)} files + index, {total // 1024} KB, {len(comps)} component folders -> {ROOT}")
for f in files:
    if f.endswith((".md", ".html", ".css", ".js", ".json")):
        t = (ROOT / f).read_text(encoding="utf-8")
        for bad in ("AppData", "scratchpad", "Design/TinTally/ds"):
            if bad in t:
                print("LOCAL PATH LEFT", f, bad)
        if f.endswith("bundle.js") and ("</script" in t.lower() or "<!--" in t):
            print("BUNDLE.JS UNSAFE")
        if f.endswith("bundle.css") and "</style" in t.lower():
            print("BUNDLE.CSS UNSAFE")
(ROOT / "files_map.json").write_text(json.dumps({f: f for f in files if f != "project/design-system.json"}, indent=0), encoding="utf-8")
