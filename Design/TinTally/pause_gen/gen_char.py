import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from shell import page, g, note, _tokens, points, status_chips

# Example build (README: "example build"). Effective Mod = min(M,5) + 0.7*(6..10) + 0.45*(11..15) + 0.25*(16+).
ATTRS = [
    ("Physical", [("STR", "Strength", 12, "+9.4%", (5, 5, 2, 0)),
                  ("DEX", "Dexterity", 20, "+12.0%", (5, 5, 5, 5)),
                  ("END", "Endurance", 10, "+8.5%", (5, 5, 0, 0))]),
    ("Resilience", [("CON", "Constitution", 12, "+9.4%", (5, 5, 2, 0)),
                    ("RES", "Resistance", 9, "+7.8%", (5, 4, 0, 0)),
                    ("DIS", "Discipline", 10, "+8.5%", (5, 5, 0, 0))]),
    ("Magical", [("INT", "Intelligence", 6, "+5.7%", (5, 1, 0, 0)),
                 ("WIS", "Wisdom", 6, "+5.7%", (5, 1, 0, 0)),
                 ("CHA", "Charisma", 7, "+6.4%", (5, 2, 0, 0))]),
]
PRIMARY = [("Physical Damage", "+28.2%", "STR"), ("Attack Speed", "+18.0%", "DEX"), ("Affliction Resist", "+23.4%", "RES"),
           ("Magic Damage", "+17.1%", "WIS"), ("Cast Speed", "+9.6%", "CHA")]
SECONDARY = [("Armor Penetration", "+14.1%", "STR"), ("Physical Crit Chance", "9.0%", "DEX"), ("Stamina Regen", "+17.0%", "END"),
             ("Health Regen", "+14.1%", "CON"), ("Affliction Recovery", "+11.7%", "RES"), ("Posture Recovery", "+17.0%", "DIS"),
             ("Spell Points Regen", "+11.4%", "INT"), ("Resistance Penetration", "+7.1%", "WIS"), ("Magic Crit Chance", "4.8%", "CHA")]
FOCUS = "STR"


def bands(pts, label):
    segs = "".join(f'<span class="tt-charsheet__band" style="--f:{p * 20}%"></span>' for p in pts)
    return f'<span class="tt-charsheet__bands" role="img" aria-label="{label}">{segs}</span>'


def card(abbr, name, base, mod, pts, focused=False, unbound=False):
    cls = "tt-charsheet__card tt-focusable tt-focus--fill-gap" + (" is-focused" if focused else "")
    b = "--" if unbound else str(base)
    m = "--" if unbound else mod
    pts = (0, 0, 0, 0) if unbound else pts
    return (f'<article class="{cls}" tabindex="0" data-attr="{abbr}">'
            f'<div class="tt-charsheet__cardtop"><span class="tt-charsheet__abbr">{abbr}</span><span class="tt-charsheet__base num-l">{b}</span></div>'
            f'<span class="tt-charsheet__name ui-label">{name}</span>'
            f'<div class="tt-charsheet__modrow">{bands(pts, f"Effective Mod bands for {name}")}<span class="tt-charsheet__mod num-s">{m}</span></div>'
            f'</article>')


def res(label, value, bar, v, extra="", debt=None):
    meter = (f'<div class="tt-bar tt-bar--hair" style="--v:0%;--d:{debt}%"><span class="tt-bar__debt"></span></div>' if debt is not None
             else f'<div class="tt-bar tt-bar--hair {bar}" style="--v:{v}%"><span class="tt-bar__fill"></span></div>')
    return f'<div class="tt-charsheet__res{extra}"><span class="ui-label">{label}</span><span class="num-m">{value}</span>{meter}</div>'


def sp_res(cur, mx):
    """Spell Points: tokens, never a bar (Tally's .tt-tally-sp markup at the sheet's 8 px size)."""
    return (f'<div class="tt-charsheet__res tt-charsheet__res--sp"><span class="ui-label">Spell Points</span><span class="num-m">{cur} / {mx}</span>'
            f'{_tokens(cur, mx, "tt-tally-sp tt-charsheet__tokens")}</div>')


def level():
    return ('<div class="tt-charsheet__level" title="TODO(DECISION): levelling and XP"><span class="ui-label">Level</span>'
            '<span class="num-l">--</span></div>')


def rows(items):
    out = []
    for label, val, src in items:
        linked = " is-linked" if src == FOCUS else ""
        out.append(f'<div class="tt-charsheet__row{linked}" data-src="{src}"><dt><span class="ui-label">{label}</span>'
                   f'<span class="tt-charsheet__leader" aria-hidden="true"></span></dt><dd class="num-s">{val}</dd></div>')
    return "\n".join(out)


head = ('<header class="tt-charsheet__head">' + level() + points(3)
        + '<span class="tt-divider tt-divider--v tt-charsheet__sep" aria-hidden="true"></span>'
        '<div class="tt-charsheet__resources">'
        + res("Health", "118 / 156", "tt-bar--vital", 76)
        + res("Stamina", "134 / 134", "tt-bar--green", 100)
        + sp_res(7, 12)
        + res("Posture", "134 / 134", "tt-bar--posture", 100)
        + '</div></header>')

def statuses(chips=None):
    """The "Active statuses" row (StatusTooltip's host): the label, then StatusChip's real chips, each focusable
    (focus or hover hangs its StatusTooltip). No status: the one empty state (core .tt-empty, D8) at chip height."""
    body = (f'<div class="tt-charsheet__chips">{status_chips(focusable=True)}</div>' if chips is None else chips)
    return (f'<section class="tt-charsheet__statuses" aria-label="Active statuses">'
            f'<span class="tt-charsheet__statlabel ui-label">Statuses</span>{body}</section>')


groups = []
for gname, cards in ATTRS:
    cs = "".join(card(*c, focused=(c[0] == FOCUS)) for c in cards)
    groups.append(f'<section class="tt-charsheet__group"><h3 class="tt-charsheet__grouphead ui-label tt-caps">{gname}'
                  f'<span class="tt-divider" aria-hidden="true"></span></h3>{cs}</section>')

receipt = ('<aside class="tt-charsheet__receipt" aria-label="Derived stats">'
           '<h3 class="tt-charsheet__rhead kicker tt-caps">Derived · primary</h3>'
           f'<dl class="tt-charsheet__rows">{rows(PRIMARY)}</dl>'
           '<span class="tt-divider tt-divider--enamel tt-charsheet__rdiv" aria-hidden="true"></span>'
           '<h3 class="tt-charsheet__rhead kicker tt-caps">Derived · secondary</h3>'
           f'<dl class="tt-charsheet__rows">{rows(SECONDARY)}</dl>'
           '</aside>')

content = (f'<div class="tt-charsheet">{head}<div class="tt-charsheet__body">'
           f'<div class="tt-charsheet__left">{statuses()}<div class="tt-charsheet__attrs">{"".join(groups)}</div></div>'
           f'{receipt}</div></div>')

legend = ('<div class="tt-charsheet__legend">'
          + "".join(f'<span class="tt-charsheet__legendcell">{bands(p, "")}<span class="ui-label">Mod {m}</span><span class="num-s">{r}</span></span>'
                    for p, r, m in (((5, 0, 0, 0), "100%", "1-5"), ((0, 5, 0, 0), "70%", "6-10"), ((0, 0, 5, 0), "45%", "11-15"), ((0, 0, 0, 5), "25%", "16+")))
          + '</div>')

exhausted = ('<div class="tt-charsheet__head tt-charsheet__head--note">'
             + res("Stamina", "-12 / 134", "", 0, " is-low", debt=9)
             + res("Stamina", "-50 / 134", "", 0, " is-low", debt=37)
             + '<span class="tt-pill tt-pill--exhausted">×0.5</span><span class="ui-label tt-charsheet__notetext">Exhausted</span>'
             '</div>')

points_states = ('<div class="tt-charsheet__head tt-charsheet__head--note">' + level() + points(3) + points(0) + '</div>')

sp_states = ('<div class="tt-charsheet__head tt-charsheet__head--note">' + sp_res(7, 12) + sp_res(13, 13) + sp_res(16, 16) + '</div>')

status_states = ('<div class="tt-charsheet__notestatuses">'
                 + statuses('<div class="tt-charsheet__chips">' + status_chips(focusable=True).replace(
                     'tt-status--debuff" tabindex', 'tt-status--debuff is-focused" tabindex', 1) + '</div>')
                 + statuses('<p class="tt-empty">None.</p>') + '</div>')

band = [
    ("Unbound (no ASC yet)", ['<div class="tt-charsheet__notecards">' + card("STR", "Strength", 0, "", (0, 0, 0, 0), unbound=True) + '</div>',
                              note('Every base value, Effective Mod and resource reads "--"; the band meters are empty.')]),
    ("Band meter: what a Mod point counts", [legend, note("Four wells for Mod 1-5 / 6-10 / 11-15 / 16+, stepping down 100 / 70 / 45 / 25 % "
                                                          "in height: the staircase says each point counts for less.")]),
    ("Stamina below zero (Exhausted)", [exhausted,
                                        note("StaminaDebtBar (D2): the hatch fills from the LEFT by |Stamina| / MaxStamina "
                                             "(-12 = 9%; the -50 floor = 37% at MaxStamina 134). Regen eats it from its right edge, "
                                             "then the tier fill grows from the same left edge.")]),
    ("Skill Points and Level", [points_states,
                                note("One skill-point badge on the Character and Skills tabs: the gold coin with the skill-point glyph, "
                                     "the count in gold. At 0 the coin goes dark. Level and XP do not exist yet (TODO(DECISION)): "
                                     'the Level badge stays dashed and reads <span style="white-space:nowrap">"--"</span>.')]),
    ("Spell Points in the sheet", [sp_states,
                                   note("Tokens, never a bar: 8 px tokens in fives fit 16 in the 170 px cell "
                                        "(MaxSpellPoints = 10 x (1 + 0.04 x EffInt); 13 at the default INT). Above 16 the cell shows the number alone.")]),
    ("Active statuses: focus opens StatusTooltip", [status_states,
                                                    note("StatusChip's chips, the same statuses as the HUD strip, frozen while paused. Each chip is in the "
                                                         "tab's focus order: focus (above, Frostbitten) or hover hangs its StatusTooltip under it. "
                                                         "No status: the one empty state, \"None.\" (proposed copy).")]),
]

if __name__ == "__main__":
    page("CharacterTab", "Character tab: Level and Skill Points, resources, active statuses, 3x3 attribute cards with band meters, derived-stat receipt",
         "character", content, band, height=1712)
