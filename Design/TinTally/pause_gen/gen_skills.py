import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from shell import page, g, note, points

CATS = [("Weapon", "weapon", True), ("Defense & Armor", "armor", False), ("Movement & Exploration", "map", False),
        ("Utility & Interaction", "chest", False), ("Magic", "magical", False)]

# (name, rank, state, reason)
ROWS = [
    ("Longswords", 12, "selected", ""),
    ("Greatswords", 0, "unlearned", "Not learned"),
    ("Shortswords", 4, "", ""),
    ("Daggers", 20, "max", "Max rank"),
    ("Axes", 7, "", ""),
]

MILES = [
    (1, "Arcing sweep that widens with rank", "reached"),
    (5, "Charged heavy: a spinning slash", "reached"),
    (10, "Light combo extends to 3 swings", "reached"),
    (15, "Heavy leaves a cyclone for 2 s", "next"),
    (20, "Full health: combos fire projectiles", "locked"),
]


def allocate(disabled=False, state=""):
    """Button's row action (.tt-btn--sm): skill-point glyph + "Allocate"; locked = core disabled + lock glyph."""
    cls = "tt-btn tt-btn--sm tt-skills__allocate" + (f" is-{state}" if state else "")
    if disabled:
        return f'<button class="{cls}" aria-disabled="true">' + g("lock", cls="tt-btn__glyph") + '<span class="tt-btn__label">Allocate</span></button>'
    return f'<button class="{cls}">' + g("skill-point", cls="tt-btn__glyph") + '<span class="tt-btn__label">Allocate</span></button>'


def tally(rank, enamel=False):
    # Tally's skill-row size (BUILD: 24 tall, stroke 3, pitch 7 = 148 px); lit strokes text on plate, ink on enamel.
    return (f'<span class="tt-skills__bar tt-tally{" tt-tally--enamel" if enamel else ""}" data-tally="{rank}" data-max="20" '
            f'data-height="24" data-pitch="7" data-cap aria-label="Rank {rank} of 20"></span>')


def row(name, rank, state, reason, points_left=3, static=False):
    cls = "tt-skills__row tt-selectable tt-focus--fill-gap"
    sel = state == "selected"
    if sel:
        cls += " is-selected" + ("" if static else " is-focused")
    if state == "unlearned":
        cls += " is-unlearned"
    if state == "max":
        cls += " is-max"
    disabled = state == "max" or points_left == 0
    why = f'<span class="tt-skills__why ui-label">{reason}</span>'
    return (f'<li class="{cls}" tabindex="0" data-skill="{name}">'
            f'<div class="tt-skills__line1"><span class="tt-skills__name ui-title">{name}</span>'
            f'<span class="tt-skills__rank"><span class="ui-label">Rank</span><span class="num-m">{rank} / 20</span></span></div>'
            f'<div class="tt-skills__line2">{tally(rank, sel)}{why}{allocate(disabled)}</div></li>')


rail = '<ul class="tt-skills__rail" role="tablist">' + "".join(
    f'<li><button class="tt-skills__cat tt-selectable tt-focus--fill-gap{" is-selected" if sel else ""}" role="tab" aria-selected="{"true" if sel else "false"}">'
    f'{g(gl)}<span class="ui-label">{name}</span></button></li>'
    for name, gl, sel in CATS) + '</ul>'

lst = ('<div class="tt-skills__listwrap"><ul class="tt-skills__list" aria-label="Weapon skills">'
       + "".join(row(*r) for r in ROWS)
       + '</ul><span class="tt-skills__scroll" aria-hidden="true"><span class="tt-skills__thumb"></span></span></div>')


def mile(rank, text, state):
    extra = '<span class="ui-label tt-skills__togo">3 ranks to go</span>' if state == "next" else ""
    body = f'<span class="ui-label">{text}</span>'
    if state == "locked":
        body = f'<span class="tt-skills__locked">{g("lock", 20, "Locked")}<span class="ui-label">{text}</span></span>'
    return (f'<li class="tt-skills__mile is-{state}"><span class="tt-skills__stamp" aria-label="Rank {rank}">{rank}</span>'
            f'<span class="tt-skills__mtext">{body}{extra}</span></li>')


detail = ('<aside class="tt-skills__detail" aria-label="Longswords milestones">'
          '<span class="tt-skills__dname ui-title">Longswords</span>'
          '<span class="tt-skills__drank"><span class="ui-label">Rank</span><span class="num-m">12 / 20</span></span>'
          '<span class="tt-divider tt-skills__ddiv" aria-hidden="true"></span>'
          '<span class="tt-skills__dhead ui-label tt-caps">Milestones</span>'
          '<ol class="tt-skills__miles">' + "".join(mile(*m) for m in MILES) + '</ol></aside>')

head = ('<header class="tt-skills__head">' + points(3)
        + '<span class="tt-skills__headmeta ui-label"><b>Weapon</b> · 13 skills</span></header>')

content = f'<div class="tt-skills">{head}<div class="tt-skills__body">{rail}{lst}{detail}</div></div>'

zero = ('<div class="tt-skills__note"><div class="tt-skills__head tt-skills__head--note">' + points(0)
        + '<span class="tt-skills__headwhy ui-label">No skill points</span></div>'
        '<ul class="tt-skills__list tt-skills__list--note">' + row("Longswords", 12, "", "", points_left=0) + '</ul></div>')

alloc = ('<div class="tt-skills__note tt-skills__specimens">'
         + "".join(f'<figure class="tt-skills__specimen">{allocate(d, s)}<figcaption class="tt-label">{c}</figcaption></figure>'
                   for d, s, c in ((False, "", "Idle"), (False, "hover", "Hover"), (False, "pressed", "Pressed"),
                                   (False, "focused", "Focused"), (True, "", "Locked")))
         + '</div>')

rows_note = ('<div class="tt-skills__note"><ul class="tt-skills__list tt-skills__list--note">'
             + row("Shortswords", 4, "", "") + row("Longswords", 12, "selected", "", static=True) + row("Daggers", 20, "max", "Max rank")
             + '</ul></div>')

miles_note = ('<div class="tt-skills__note tt-skills__notemiles">'
              '<span class="tt-skills__mile is-reached"><span class="tt-skills__stamp">5</span><span class="ui-label">Reached</span></span>'
              '<span class="tt-skills__mile is-next"><span class="tt-skills__stamp">15</span><span class="ui-label">Next</span></span>'
              '<span class="tt-skills__mile is-locked"><span class="tt-skills__stamp">20</span><span class="ui-label">Locked</span></span>'
              '</div>')

empty = ('<div class="tt-skills__note tt-skills__noteempty"><div class="tt-empty-box"><p class="tt-empty">No skills in this category yet.</p></div></div>')

band = [
    ("Rows: idle, selected, Rank 20", [rows_note,
                                       note("Selected is the cream fill (D6): lit strokes turn ink, unlit dots ink-muted. "
                                            "Rank 20 is a full bundle in text, never gold (D1). The reason sits beside a locked Allocate.")]),
    ("Allocate (Button, small row action)", [alloc,
                                             note("The core button at --sm (48 tall): the skill-point glyph + \"Allocate\"; locked: dashed edge, lock glyph. "
                                                  "The label stays \"Allocate\" in every state, so click_button(\"Allocate\") keeps working.")]),
    ("0 skill points", [zero, note("The badge goes dark and says why once, in the header; every row's Allocate locks without repeating it.")]),
    ("Milestone stamps", [miles_note, note("Reached: light face. Next: dark face, solid rim, \"3 ranks to go\". "
                                           "Locked: dashed rim, lock glyph. Never colour alone.")]),
    ("Empty category", [empty, note("Core .tt-empty (D8). Proposed copy: no skill data assets exist yet.")]),
]

if __name__ == "__main__":
    page("SkillsTab", "Skills tab: category rail, ranked rows with 20-stroke tally bars and Allocate, milestone detail",
         "skills", content, band, height=1788)
