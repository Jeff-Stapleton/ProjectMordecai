"""InventoryTab preview generator (not published). Reuses PauseShell's frame from pause_gen/shell.py
(diorama + real HUD + veil, nameplate, kbm tab bar, content plate, hint bar, spike) and fills the 948 x 540 box."""
import sys, pathlib
SCRATCH = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRATCH / "pause_gen"))
from shell import page, g, note, footer, hintbar  # noqa: E402

FILTERS = [("all", "All", "all"), ("weapons", "Weapons", "weapon"), ("armor", "Armor", "armor"),
           ("trinkets", "Trinkets", "trinket"), ("consumables", "Consumables", "consumable"),
           ("materials", "Materials", "material"), ("quest", "Quest", "quest"), ("magical", "Magical", "magical")]


def badge(r):
    return f'<span class="tt-inv-row__badge"><span class="tt-socket"><span class="tt-rarity tt-r-{r}" data-rarity="{r}"></span></span></span>'


def filt(fid, label, glyph, state=(), static=False):
    cls = "tt-inv-filter tt-selectable tt-focus--fill-gap" + "".join(f" is-{s}" for s in state)
    sel = "selected" in state
    attrs = ' tabindex="-1"' if static else f' role="tab" aria-selected="{"true" if sel else "false"}" data-filter-id="{fid}"'
    return f'<button class="{cls}"{attrs}>{g(glyph, 20)}{label}</button>'


def search(query="", focused=False, static=False):
    cls = "tt-inv-search tt-focus--fill-gap" + (" has-query" if query else "") + (" is-focused" if focused else "")
    text = query if query else "Search"
    caret = '<span class="tt-inv-search__caret" aria-hidden="true"></span>' if focused else ""
    clear = ('<span class="tt-inv-search__clear" role="button" aria-label="Clear the search">' + g("cross", 20) + '</span>') if query else ""
    bind = "" if static else ' data-bind="ItemSearchBox"'
    return (f'<label class="{cls}"{bind}>{g("search", 24, "Search")}'
            f'<span class="tt-inv-search__text">{text}{caret}</span>{clear}</label>')


def identify(state=""):
    """Button's row action (.tt-btn--sm): the mage-tower glyph + "Identify"; its label stays the first TextBlock."""
    cls = "tt-btn tt-btn--sm tt-inv-row__action" + (f" is-{state}" if state else "")
    return f'<button class="{cls}" type="button">' + g("mage-tower", cls="tt-btn__glyph") + '<span class="tt-btn__label">Identify</span></button>'


def equipped():
    """ItemCard's Equipped pill (core chip, rim edge): pin glyph + "Equipped"."""
    return '<span class="tt-chip tt-itemcard__flag">' + g("pin", 20) + 'Equipped</span>'


def row(rarity, name, end="", state=(), tag="li"):
    cls = "tt-inv-row tt-selectable tt-focus--fill-gap" + "".join(f" is-{s}" for s in state)
    endhtml = f'<span class="tt-inv-row__end">{end}</span>' if end else ""
    return (f'<{tag} class="{cls}" tabindex="0">{badge(rarity)}'
            f'<span class="tt-inv-row__name ui-title tt-r-{rarity}">{name}</span>{endhtml}</{tag}>')


def ledger(rows=True):
    head = '<h3 class="tt-inv-ledger__head ui-title" data-bind="LedgerHeader">' + g("material") + 'Stored Resources</h3>'
    if not rows:
        return (f'<section class="tt-inv-ledger" aria-label="LedgerBox">{head}'
                '<p class="tt-empty" data-bind="LedgerPlaceholder">No resources stored.</p></section>')
    body = "".join(
        f'<div class="tt-inv-ledger__row ui-label">{g(gl, 20)}{n}<span class="tt-inv-ledger__dots" aria-hidden="true"></span>'
        f'<span class="tt-inv-ledger__count num-s">x{c}</span></div>'
        for gl, n, c in (("ore", "Iron Ore", 4), ("stone", "Town Stone", 1)))
    return f'<section class="tt-inv-ledger" aria-label="LedgerBox">{head}{body}</section>'


CARD = ('<article class="tt-itemcard tt-itemcard--purple tt-itemcard--compact" aria-label="Selected item card">'
        '<div class="tt-itemcard__band"><span class="tt-rarity" data-rarity="purple"></span><span class="tt-itemcard__band-text">PURPLE · Amulet</span></div>'
        '<div class="tt-itemcard__body">'
        '<div class="tt-itemcard__head"><span class="tt-itemcard__stamp">' + g("unknown", label="Unidentified") + '</span>'
        '<div><p class="tt-itemcard__name ui-title">Unidentified Purple Amulet</p><p class="tt-itemcard__type ui-label">Magical Item · Amulet</p></div></div>'
        '<dl class="tt-itemcard__stats tt-itemcard__stats--sealed"><dt class="ui-label">Properties</dt><dd class="num-m">???</dd></dl>'
        '<p class="tt-itemcard__desc ui-body">An amulet of unknown power.</p>'
        '</div></article>')

filters = ('<div class="tt-inv-filters" role="tablist" aria-label="FilterBar" data-bind="FilterBar">'
           + "".join(filt(fid, lab, gl, ("selected",) if fid == "all" else ()) for fid, lab, gl in FILTERS)
           + search() + '</div>')

rows = ('<ul class="tt-inv-list" aria-label="ItemListBox" data-bind="ItemListBox">'
        + row("blue", "Upgrade Key")
        + row("purple", "Emberfang")
        + row("purple", "Mysterious Amulet")
        + row("purple", "? Unidentified Purple Amulet", identify("focused"), ("selected",))
        + row("common", "Rusty Sword", equipped())
        + row("common", "Health Potion  x2")
        + '</ul>')

content = (f'<div class="tt-inv">{filters}<div class="tt-inv-body">{rows}'
           f'<aside class="tt-inv-side">{CARD}{ledger()}</aside></div></div>')

# ---------------------------------------------------------------- the spec band (D7)

def spec(caption, inner):
    return f'<figure class="tt-inv-specimen">{inner}<figcaption class="tt-label">{caption}</figcaption></figure>'


filter_states = ('<div class="tt-inv-specimens">'
                 + spec("Idle", filt("weapons", "Weapons", "weapon", static=True))
                 + spec("Hover", filt("weapons", "Weapons", "weapon", ("hover",), static=True))
                 + spec("Press", filt("weapons", "Weapons", "weapon", ("pressed",), static=True))
                 + spec("Focus", filt("armor", "Armor", "armor", ("focused",), static=True))
                 + spec("Selected", filt("all", "All", "all", ("selected",), static=True))
                 + spec("Selected + focused", filt("all", "All", "all", ("selected", "focused"), static=True))
                 + '</div>')

search_states = ('<div class="tt-inv-searchspec">'
                 + spec("Idle: the placeholder", search(static=True))
                 + spec("Focused, typing: the list filters as you type", search("amu", focused=True, static=True))
                 + '</div>')


def rowspec(rarity, name, end, state, caption):
    return row(rarity, name, end, state, tag="div") + f'<p class="tt-label">{caption}</p>'


row_states = ('<div class="tt-inv-rowspec">'
              + rowspec("common", "Health Potion  x2", "", (), "Idle")
              + rowspec("blue", "Upgrade Key", "", ("hover",), "Hover")
              + rowspec("purple", "Emberfang", "", ("selected",), "Selected")
              + rowspec("purple", "Mysterious Amulet", "", ("selected", "focused"), "Selected + focused")
              + rowspec("common", "Rusty Sword", equipped(), (), "Equipped")
              + '</div>')

identify_states = ('<div class="tt-inv-specimens tt-inv-specimens--4">'
                   + spec("Idle", identify()) + spec("Hover", identify("hover"))
                   + spec("Pressed", identify("pressed")) + spec("Focused", identify("focused"))
                   + '</div>')

empty_list = ('<div class="tt-inv-specimens tt-inv-specimens--2">'
              + spec("Inventory and ledger both empty",
                     '<div class="tt-inv-list tt-inv-mini"><div class="tt-empty-box"><p class="tt-empty" data-bind="ListPlaceholder">Inventory is empty.</p></div></div>')
              + spec("A filter or a search hides every item",
                     '<div class="tt-inv-list tt-inv-mini"><div class="tt-empty-box"><p class="tt-empty" data-bind="ListPlaceholder">No items match this filter.</p></div></div>')
              + spec("No resources", '<div class="tt-inv-mini--ledger">' + ledger(rows=False) + '</div>')
              + '</div>')

sort_list = ('<ol class="tt-inv-sort">'
             '<li>Critical <span>· Upgrade Keys</span></li>'
             '<li>Signature <span>· Magical Items, identified or not</span></li>'
             '<li>High <span>· Resources (ledger only)</span></li>'
             '<li>Normal <span>· weapons, armor, consumables</span></li>'
             '<li>Low</li></ol>')

band = [
    ("Filter tags (FilterBar)", [filter_states,
                                 note("SkillsTab's category tag: tin face, lip edge, grommet. Selected is the cream fill (D6); "
                                      "exactly one is selected, All by default. Focus is core's fill-gap ring; press keeps the "
                                      "full edge in High Contrast (D22).")]),
    ("Search (proposed, TODO(DECISION))", [search_states,
                                           note("agent_rules_v2 section 2 asks for search; no C++ exists. Proposed "
                                                "BindWidgetOptional ItemSearchBox (UEditableTextBox), the last child of the "
                                                "FilterBar WrapBox, filling the rest of its row. It narrows the selected filter "
                                                "by name, as you type. Placeholder copy \"Search\" is proposed.")]),
    ("Rows", [row_states,
              note("The selected row drives the card: cream fill, ink name, the badge keeps its rarity in the socket. "
                   "Equipped is ItemCard's pill, not bracket text.")]),
    ("Identify (Button's row action)", [identify_states,
                                        note("Core .tt-btn--sm: the mage-tower glyph leads \"Identify\". While it has focus "
                                             "the hint bar adds \"[Enter] Identify\" (PauseShell's contextual hints).")]),
    ("Empty states (D8)", [empty_list,
                           note("One look: core .tt-empty, centred, ink on enamel, no eyelet and no dot. "
                                "The shipping strings, exact.")], "wide"),
    ("Gamepad: drawn only here (D5)", [footer("pad", extra=[("A", "Identify")], cls="tt-pause__bandbar"),
                                       note("B Back | LB / RB Tabs | A Identify while an Identify button has focus. "
                                            "On the screen above (keyboard and mouse) the bar reads Esc Back | I Close | "
                                            "Enter Identify. Search on gamepad (a virtual keyboard) is TODO(DECISION).")]),
    ("Sort order (CompareSortPriority)", [sort_list,
                                          note("Ties: Weapon, Armor, Trinket, Magical Item, Upgrade Key, Quest Item, "
                                               "Consumable, Material; then ItemId.")]),
    ("Geometry", [note("Content box 948 x 540 at (486, 298). FilterBar two rows, 104 tall (tags 48, 8 apart). "
                       "Body 424 tall, 12 below: list 604 (948 - 24 - 320), right column 320. Rows 56, 6 apart."),
                  note("FilterBar is a UHorizontalBox in C++ today: retype it to UPanelWidget* (a UWrapBox) "
                       "so the tags and the search field wrap.")]),
]

if __name__ == "__main__":
    height = int(sys.argv[1]) if len(sys.argv) > 1 else 2000
    page("InventoryTab",
         "Pause menu Inventory tab: filter tags and search, tin item rows with Identify, the selected item's card and the Stored Resources ledger",
         "inventory", content, band, height=height, extra_hints=[("Enter", "Identify")])
