"""MageTower preview generator (not published). The world behind the veil, the service-screen nameplate
and the hint bar come from PauseShell's pause_gen/shell.py, so the frame never drifts from the pause menu's."""
import sys, pathlib
SCRATCH = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRATCH / "pause_gen"))
from shell import COMP, g, world, nameplate, hintbar  # noqa: E402


def sealed_card(desc=True, note=True, stamp_overlay=False):
    out = ['<article class="tt-itemcard tt-itemcard--purple"><span class="tt-mage-ear" aria-hidden="true"></span>'
           '<div class="tt-itemcard__band"><span class="tt-rarity" data-rarity="purple"></span><span class="tt-itemcard__band-text">PURPLE · Amulet</span></div>']
    if stamp_overlay:
        out.append('<span class="tt-mage-stamp" aria-hidden="true"><span class="tt-rarity" data-rarity="purple" data-height="28"></span></span>')
    out.append('<div class="tt-itemcard__body"><div class="tt-itemcard__head"><span class="tt-itemcard__stamp">' + g("unknown", label="Unidentified") + '</span>'
               '<div><p class="tt-itemcard__name ui-title">Unidentified Purple Amulet</p><p class="tt-itemcard__type ui-label">Magical Item · Amulet</p></div></div>')
    if note:
        out.append('<p class="tt-itemcard__note ui-label">' + g("mage-tower", 20) + '<span>Unidentified — the Mage Tower can read this.</span></p>')
    out.append('<dl class="tt-itemcard__stats tt-itemcard__stats--sealed"><dt class="ui-label">Properties</dt><dd class="num-m">???</dd></dl>')
    if desc:
        out.append('<p class="tt-itemcard__desc ui-body">An amulet of unknown power.</p>')
    out.append('</div></article>')
    return "".join(out)


def open_card(wipe=False, footer=False):
    name = ('<span class="tt-mage-wipe">Mysterious Amulet<span class="tt-mage-wipe__mask" aria-hidden="true">' + g("arrow-right", 20) + '</span></span>'
            if wipe else "Mysterious Amulet")
    foot = ('<div class="tt-itemcard__actions"><span class="tt-itemcard__action">' + g("check", 20) + 'Identified</span></div>') if footer else ""
    return ('<article class="tt-itemcard tt-itemcard--purple"><span class="tt-mage-ear" aria-hidden="true"></span>'
            '<div class="tt-itemcard__band"><span class="tt-rarity" data-rarity="purple"></span><span class="tt-itemcard__band-text">PURPLE · Amulet</span></div>'
            '<div class="tt-itemcard__body"><div class="tt-itemcard__head"><span class="tt-itemcard__stamp is-struck"><span class="tt-rarity" data-rarity="purple"></span></span>'
            f'<div><p class="tt-itemcard__name ui-title">{name}</p><p class="tt-itemcard__type ui-label">Magical Item · Amulet</p></div></div>'
            '<p class="tt-itemcard__desc ui-body">A faintly humming amulet. The Mage Tower can identify it.</p>'
            f'</div>{foot}</article>')


def tray(card, scale_1x=False, label="Appraisal tray"):
    hang = "tt-mage-tray__hang" + (" tt-mage-tray__hang--1x" if scale_1x else "")
    return (f'<div class="tt-mage-tray" aria-label="{label}">'
            '<span class="tt-railbar tt-mage-tray__rail" aria-hidden="true"></span>'
            '<span class="tt-string tt-string--light tt-mage-tray__string" aria-hidden="true"></span>'
            f'<div class="{hang}">{card}</div></div>')


def tag(r, name, sub, selected=False):
    cls = "tt-tag tt-mage-tag" + (" is-selected is-focused" if selected else "")
    return (f'<li class="{cls}" tabindex="0"><span class="tt-socket"><span class="tt-rarity tt-r-{r}" data-rarity="{r}"></span></span>'
            f'<span class="tt-mage-tag__text"><span class="tt-mage-tag__name ui-title">{name}</span><span class="tt-mage-tag__sub ui-label">{sub}</span></span>'
            '<span class="tt-mage-tag__stamp">' + g("unknown", 20, "Unidentified") + '</span></li>')


def identify(state="", disabled=False):
    if disabled:
        return '<span class="tt-mage-identify" role="button" aria-disabled="true">' + g("lock", label="Locked") + '<span class="ui-title">Identify</span></span>'
    cls = "tt-mage-identify" + (f" is-{state}" if state else "")
    key = "tt-key is-pressed" if state == "pressed" else "tt-key"
    return f'<span class="{cls}" role="button"><span class="{key}">Enter</span><span class="ui-title">Identify</span></span>'


panel = ('<section class="tt-mage-panel" aria-label="Unidentified items">'
         '<h2 class="tt-mage-panel__head ui-title">' + g("seal") + 'Unidentified items<span class="tt-mage-panel__count tt-pill">4</span></h2>'
         '<ul class="tt-mage-list">'
         + tag("purple", "Unidentified Purple Amulet", "Magical Item · Amulet", True)
         + tag("blue", "Unidentified Blue Sword", "Magical Item · Sword")
         + tag("green", "Unidentified Green Ring", "Trinket · Ring")
         + tag("red", "Unknown Item", "Magical Item")
         + '</ul></section>')

counter = ('<section class="tt-mage-counter" aria-label="Appraisal">'
           '<div class="tt-mage-quote__rows ui-label">'
           '<div class="tt-mage-quote__row">Cost<span class="tt-mage-quote__dots"></span><b>Free</b></div>'
           '<div class="tt-mage-quote__row">Time<span class="tt-mage-quote__dots"></span><b>Instant</b></div></div>'
           '<p class="tt-mage-quote__body ui-body">Identifying reveals its name and properties. Then it can be equipped.</p>'
           + identify() + '</section>')

screen = ('<section class="tt-mage-screen" aria-label="Mage Tower identify at 1920 by 1080, before the reveal">\n'
          + world() + "\n"
          + nameplate("Mage Tower · Identify", screen=True, glyph="mage-tower") + "\n"
          + '<div class="tt-mage-shell tt-plate tt-plate--menu">' + panel
          + '<div class="tt-mage-main">' + tray(sealed_card()) + counter + '</div></div>\n'
          + hintbar([("Esc", "Back"), ("Enter", "Identify")], cls="tt-pause__footer") + "\n</section>")


def item(title, parts):
    return f'<div class="tt-specband__item"><p class="tt-specband__title">{title}</p>' + "".join(parts) + '</div>'


def note(t):
    return f'<p class="tt-label">{t}</p>'


empty_panel = ('<section class="tt-mage-panel" aria-label="Unidentified items, empty">'
               '<h2 class="tt-mage-panel__head ui-title">' + g("seal") + 'Unidentified items<span class="tt-mage-panel__count tt-pill">0</span></h2>'
               '<div class="tt-empty-box"><p class="tt-empty" data-bind="EmptyText">Nothing to identify.</p></div>'
               + identify(disabled=True) + '</section>')

states = ('<div class="tt-mage-states">'
          + "".join(f'<figure class="tt-mage-specimen">{b}<figcaption class="tt-label">{c}</figcaption></figure>'
                    for b, c in ((identify(), "Idle"), (identify("pressed"), "Pressed: sinks 4 px, the keycap presses (KeyPress)"),
                                 (identify("focused"), "Focused: the core ring and keyline"), (identify(disabled=True), "Disabled: dashed lip, lock glyph")))
          + '</div>')

band = ('<div class="tt-specband tt-mage-band" aria-label="Spec notes, not on screen">'
        + item("Frame 1 · 110 ms: the rarity stamp strikes", [tray(sealed_card(desc=False, stamp_overlay=True), True, "Frame 1"),
               note("StampReveal: the rarity stamp lands on the \"?\" stamp; the card shakes.")])
        + item("Frame 2 · 380 ms: the name wipes in", [tray(open_card(wipe=True), True, "Frame 2"),
               note("TypeOn: a hatched retainer mask slides off the final name behind a rarity-colour wipe head.")])
        + item("Frame 3 · settled: identified", [tray(open_card(footer=True), True, "Frame 3"),
               note("One TicketDrop swing on the string, then rest. The list drops from 4 to 3.")])
        + item("Empty (D8)", [empty_panel, note("Core .tt-empty, centred. \"Nothing to identify.\" is example copy. Identify is disabled.")])
        + item("IdentifyButton", [states])
        + item("Gamepad: drawn only here (D5)", [hintbar([("B", "Back"), ("A", "Identify")], device="pad", cls="tt-pause__bandbar"),
               note("The screen above is drawn for keyboard and mouse: \"[Esc] Back | [Enter] Identify\". CommonInput swaps the keycaps "
                    "with the last input device.")])
        + item("Frame", [note("PauseShell's service-screen nameplate (.tt-pause__title--screen: 52 px light strings, the mage-tower glyph), "
                              "core .tt-hintbar at y 966 (--tt-pause-footer-top). The toy diorama and the real HUD stay under the veil (D10, D12)."),
                         note("The tray rail is the one rail family (core .tt-railbar, D23). No rivets: this is a service screen, not a Tally plate.")])
        + item("After the screen closes", [note("The rail prints an IDENTIFIED ticket (TallyRail's kinds, D26): kicker \"Identified\", "
                                                "\"Mysterious Amulet\", the rarity in the footer. Where the remark \"It had a name all along.\" "
                                                "prints is TODO(DECISION). This screen shows no remark.")])
        + '</div>')

html = f'''<!-- @dsCard group="Menus" height={{H}} width=1920 subtitle="Mage Tower identify service: sealed tags, the appraisal tray on the rail, a storyboard of the reveal from Unidentified Purple Amulet to Mysterious Amulet" -->
<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>MageTower</title></head>
<body>
<div class="tt-root tt-mage-sheet">
{screen}
{band}
</div>
</body>
</html>
'''

if __name__ == "__main__":
    h = int(sys.argv[1]) if len(sys.argv) > 1 else 2354
    out = COMP / "MageTower" / "preview.html"
    out.write_text(html.replace("{H}", str(h)), encoding="utf-8")
    print("wrote", out)
