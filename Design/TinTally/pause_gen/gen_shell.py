import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from shell import page, tab, footer, hintbar, note, nameplate

# Settings today: the placeholder "Coming Soon" as the one empty state (core .tt-empty, D8).
content = ('<div class="tt-empty-box"><p class="tt-empty" data-bind="PlaceholderText">Coming Soon</p></div>')


def state(caption, **kw):
    return (f'<figure class="tt-pause__state"><div class="tt-pause__statebar">{tab("skills", "Skills", static=True, **kw)}</div>'
            f'<figcaption class="tt-label">{caption}</figcaption></figure>')


tab_states = ('<div class="tt-pause__states">'
              + state("Idle") + state("Hover", hover=True) + state("Press", pressed=True)
              + state("Focus", focused=True) + state("Selected", selected=True) + state("Selected + focused", selected=True, focused=True)
              + '</div>')

pad_bar = ('<div class="tt-pause__statebar tt-pause__statebar--pad"><span class="tt-key tt-key--shoulder tt-pause__bumper">LB</span>'
           + tab("character", "Character", selected=True, static=True)
           + '<span class="tt-key tt-key--shoulder tt-pause__bumper">RB</span></div>')

band = [
    ("Tab states", [tab_states,
                    note("Selected is the cream enamel fill on every tab (D6); focus adds the open-bottom ring on top. "
                         "Press sinks 3 px and keeps its full edge in High Contrast (D22).")]),
    ("Gamepad: drawn only here (D5)", [pad_bar, footer("pad", cls="tt-pause__bandbar"),
                                        note("LB / RB keycaps stand at the bar ends (x 460 and 1404); the tabs keep their place. "
                                             "Keyboard Previous / Next Tab keys are TODO(DECISION) (proposal Q / E) and are never drawn as bound.")]),
    ("Service screens", ['<div class="tt-pause__bandscreen">' + nameplate("Mage Tower", screen=True, glyph="mage-tower") + '</div>',
                         note("Screens on UI.Layer.Menu (Mage Tower, World Map, Options) reuse the nameplate and hint bar: "
                              ".tt-pause__title--screen (above, title example) hangs a glyph + title on 52 px strings; "
                              "--tt-pause-strings and --tt-pause-footer-top move the strings and the hint bar without two-class overrides.")]),
    ("The spike", [note("The rail's ticket kinds, one line each: slip, SKILL UP, FEAT PROGRESS, STORED, ITEM · rarity, IDENTIFIED (D9). "
                        "Stub colour and kicker as on the rail; the name ellipsizes, the kicker and the number never do."),
                   note("The focused entry opens to its full 88 px ticket (on screen: the Purple Amulet). Unread entries carry the rail's peeled corner. "
                        "8 show and the 9th's top edge peeks at the ScrollBox's foot as the scroll cue (with an entry open, the 8th is cut "
                        "under its text); 12 are kept. The rod stands in a .tt-railbar base (D23).")]),
    ("Contextual hints", [hintbar([("Esc", "Back"), ("I", "Close"), ("Enter", "Identify")], cls="tt-pause__bandbar"),
                          note("A tab may add hints for the focused widget through the CommonUI action bar, after Back and Close: "
                               "here an Identify keycap has focus in the Inventory tab. At most two extra pairs; never replace Back or Close.")]),
    ("Geometry", [note("Tab bar 1000 x 56 at (460, 220). TabContentBorder 1000 x 584 at (460, 276), padding 20 / 24: "
                       "every tab draws inside the content box 948 x 540 at (486, 298)."),
                  note("Hint bar centred on x 960 at y 892. Spike x 1500-1880, y 142-834. Behind the veil: the toy diorama and the real HUD (D10, D12), "
                       "HUDLayout's markup: the always-visible pieces, the stocked quick items and the two active statuses CharacterTab lists. "
                       "The status strip (x 32-488) runs under the plate's left edge (x 460), as in game.")]),
]

page("PauseShell", "Pause screen: Paused nameplate, folder tabs over the 1000x584 plate, hint bar and the Tally spike, over the diorama",
     "settings", content, band, height=1782, spike_open=1)
