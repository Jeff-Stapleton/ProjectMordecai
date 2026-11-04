"""WorldMap preview generator (not published). World, service-screen nameplate and hint bar from
PauseShell's pause_gen/shell.py."""
import sys, pathlib
SCRATCH = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRATCH / "pause_gen"))
from shell import COMP, g, world, nameplate, hintbar  # noqa: E402


def cell(c, r, kind, state="", inner="", label=None):
    cls = "tt-wmap-cell" + (f" tt-wmap-cell--{kind}" if kind else "") + (f" is-{state}" if state else "")
    lab = f' aria-label="{label}"' if label else ""
    return f'<div class="{cls}" style="--c:{c}; --r:{r}"{lab}>{inner}</div>'


def groove(c, r, d, mod=""):
    return f'<span class="tt-wmap-groove tt-wmap-groove--{d}{(" tt-wmap-groove--" + mod) if mod else ""}" style="--c:{c}; --r:{r}"></span>'


grooves = "".join([groove(3, 2, "v"), groove(2, 2, "h"), groove(2, 1, "v"), groove(1, 1, "h"), groove(2, 2, "v"),
                   groove(2, 3, "h", "shortcut"), groove(3, 2, "h"), groove(4, 2, "h"), groove(3, 3, "h", "locked"),
                   groove(3, 3, "v"), groove(5, 1, "v")])

cells = "".join([
    # Verdant Wilds: entered, every cell charted
    cell(1, 1, "verdant", inner=g("diamond", 32, "Boss lair")), cell(2, 1, "verdant", inner=g("chest", 32, "Secret found")),
    cell(2, 2, "verdant"), cell(3, 2, "verdant"), cell(2, 3, "verdant"),
    # Town
    cell(3, 3, "town", inner=g("keep", 32) + '<span class="tt-wmap-cell__label">Town</span>'),
    # Searing Expanse: entered, 2 of 6 charted
    cell(4, 2, "searing"), cell(5, 2, "searing", label="Current cell"),
    cell(6, 2, "searing", "uncharted"), cell(4, 3, "searing", "uncharted"), cell(5, 3, "searing", "uncharted"), cell(6, 3, "searing", "uncharted"),
    # Regions never entered: unknown ground, no biome tint
    *[cell(c, r, "", "unknown") for c, r in ((2, 4), (3, 4), (2, 5), (3, 5), (4, 5), (0, 3), (0, 4), (1, 4), (0, 5),
                                             (5, 0), (6, 0), (5, 1), (6, 1), (7, 2), (8, 2), (7, 3), (8, 3))],
])

gates = ('<span class="tt-wmap-gate" style="--gx:578px; --gy:320px">' + g("explosives", label="Explosives gate, open")
         + '<span class="tt-wmap-gate__badge">' + g("check") + '</span></span>'
         '<span class="tt-wmap-gate" style="--gx:508px; --gy:488px">' + g("magnet", label="Magnetic Device gate") + '</span>'
         '<span class="tt-wmap-gate tt-wmap-gate--unknown" style="--gx:788px; --gy:264px">' + g("lock", label="Gate, tool unknown") + '</span>')

board = ('<div class="tt-wmap-board" role="img" aria-label="World board: Verdant Wilds 5 of 5 charted, Searing Expanse 2 of 6 charted">'
         + grooves + cells + gates
         + '<span class="tt-wmap-player" style="--c:5; --r:2">' + g("player", 32, "You are here") + '</span>'
         '<span class="tt-wmap-cursor tt-focus--fill-gap is-focused" style="--c:5; --r:2" aria-label="Map cursor on the current cell"></span>'
         '<span class="tt-wmap-name tt-chip" style="--c:1; --r:0">Verdant Wilds<span class="num-s">5 / 5</span></span>'
         '<span class="tt-wmap-name tt-chip" style="--c:3; --r:1">Searing Expanse<span class="num-s">2 / 6</span></span>'
         '</div>')


def legend_item(sample, label, pair=False, gate=False):
    cls = "tt-wmap-legend__sample" + (" tt-wmap-legend__sample--pair" if pair else "") + (" tt-wmap-legend__sample--gate" if gate else "")
    return f'<li class="tt-wmap-legend__item"><span class="{cls}">{sample}</span>{label}</li>'


legend = ('<ul class="tt-wmap-legend ui-label" aria-label="Legend"><li><h3 class="tt-wmap-legend__head ui-title">Legend</h3></li>'
          + legend_item('<span class="tt-wmap-cell tt-wmap-cell--verdant"></span><span class="tt-wmap-cell tt-wmap-cell--searing"></span>',
                        "Charted (biome colour)", pair=True)
          + legend_item('<span class="tt-wmap-cell tt-wmap-cell--searing is-uncharted"></span>', "Uncharted")
          + legend_item('<span class="tt-wmap-cell is-unknown"></span>', "Unknown ground")
          + legend_item('<span class="tt-wmap-cell tt-wmap-cell--town">' + g("keep") + '</span>', "Town")
          + legend_item('<span class="tt-wmap-player">' + g("player") + '</span>', "You")
          + legend_item('<span class="tt-wmap-groove"></span>', "Path")
          + legend_item('<span class="tt-wmap-groove tt-wmap-groove--shortcut"></span>', "Shortcut, open")
          + legend_item('<span class="tt-wmap-groove tt-wmap-groove--h tt-wmap-groove--locked"></span>', "Shortcut, locked")
          + legend_item('<span class="tt-wmap-gate">' + g("magnet") + '</span>', "Gate, tool shown", gate=True)
          + legend_item('<span class="tt-wmap-gate tt-wmap-gate--unknown">' + g("lock") + '</span>', "Gate, tool unknown", gate=True)
          + legend_item('<span class="tt-wmap-cell tt-wmap-cell--verdant">' + g("diamond") + '</span>', "Boss lair")
          + legend_item('<span class="tt-wmap-cell tt-wmap-cell--verdant">' + g("chest") + '</span>', "Secret found")
          + '</ul>')

side = ('<aside class="tt-wmap-side tt-plate tt-plate--menu"><section class="tt-wmap-region">'
        '<span class="tt-wmap-region__kicker ui-label">Current region</span>'
        '<h2 class="tt-wmap-region__name world-heading tt-caps">Searing Expanse</h2>'
        '<div class="tt-wmap-region__row ui-label">Charted<span class="num-m">2 / 6</span></div>'
        '<div class="tt-wmap-region__row ui-label">' + g("explosives") + 'Explosives<span class="num-m">' + g("check", 20, "Open") + '</span></div>'
        '<div class="tt-wmap-region__row ui-label">' + g("magnet") + 'Magnetic Device<span class="num-m">' + g("lock", 20, "Needed") + '</span></div>'
        '</section>' + legend + '</aside>')

screen = ('<section class="tt-wmap-screen" aria-label="World map proposal at 1920 by 1080">\n'
          + world() + "\n" + nameplate("World Map", screen=True, glyph="map") + "\n"
          + '<div class="tt-wmap-shell tt-plate tt-plate--menu">' + board + '</div>\n' + side + "\n"
          + hintbar([("Esc", "Back")], cls="tt-pause__footer") + "\n</section>")


def item(title, parts):
    return f'<div class="tt-specband__item"><p class="tt-specband__title">{title}</p>' + "".join(parts) + '</div>'


def note(t):
    return f'<p class="tt-label">{t}</p>'


pad_bar = ('<footer class="tt-hintbar tt-pause__bandbar" aria-label="Screen actions, gamepad">'
           '<span class="tt-hint"><span class="tt-key tt-key--sm tt-key--pad">B</span>Back</span></footer>')

band = ('<div class="tt-specband tt-wmap-band" aria-label="Spec notes, not on screen">'
        + item("Proposal only (TODO(DECISION))", [note("US-073 is planned and unspecified: this tin tile board is one option beside a "
                                                        "room-graph map and a screen grid. The region layout, the gate order and every cell "
                                                        "count here are examples.")])
        + item("Cell fills: only what the legend lists", [note("Charted cells take their biome's colour; the region pill names it. "
                                                                "Uncharted cells of an entered region are debossed in a third of that colour. "
                                                                "Regions never entered are unknown ground: an etched outline, no fill, no tint, no name."),
                                                           note("Counts are numerals (\"2 / 6\"), never tally strokes (D1).")])
        + item("Gamepad: drawn only here (D5)", [pad_bar, note("The screen above is drawn for keyboard and mouse: \"[Esc] Back\". "
                                                                "M proposed (View on gamepad): IA_Mordecai_Map exists with no binding, "
                                                                "so \"[M] Close\" joins the bar only once it is bound. Never draw an unbound key as bound.")])
        + item("Frame", [note("PauseShell's service-screen nameplate (.tt-pause__title--screen, the map glyph) and core .tt-hintbar at y 966. "
                              "The toy diorama and the real HUD stay under the veil (D10, D12). Focus is one cursor image on the top layer "
                              "(core .tt-focus--fill-gap).")])
        + '</div>')

html = f'''<!-- @dsCard group="Menus" height={{H}} width=1920 subtitle="Proposal for US-073: a tin tile board of biome cells, debossed uncharted cells, unknown ground, keyhole gate stamps, shortcut grooves, the player token" -->
<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>WorldMap</title></head>
<body>
<div class="tt-root tt-wmap-sheet">
{screen}
{band}
</div>
</body>
</html>
'''

if __name__ == "__main__":
    h = int(sys.argv[1]) if len(sys.argv) > 1 else 1396
    out = COMP / "WorldMap" / "preview.html"
    out.write_text(html.replace("{H}", str(h)), encoding="utf-8")
    print("wrote", out)
