"""SpellCard preview generator (not published). Real data: Scripts/us057_01_create_spell_data_assets.py
(the DA_Spell_* values) and MordecaiHeroComponent's bindings."""
import sys, pathlib
SCRATCH = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SCRATCH / "pause_gen"))
from shell import COMP, g  # noqa: E402

# name, glyph, icon modifier, key, SP, role line, rows [(label, value html)], effects [(glyph, text)], description (example)
SPELLS = [
    ("Fireball", "fireball", " tt-spell__icon--fire", "Q", 3, "Spell · Fire damage",
     [("Cooldown", "2.0 s"), ("Cast time", "0.5 s"), ("Targeting", "Projectile, 15 m"), ("While casting", "Slowed"),
      ("Base power", "25"), ("Damage", "fire"), ("Slot", "Q")],
     [("fire", "Fire damage on hit. It can crit.")],
     "Hurls a ball of fire where you aim."),
    ("Blink", "blink", "", "E", 2, "Spell · Mobility",
     [("Cooldown", "4.0 s"), ("Cast time", "Instant"), ("Targeting", "Dash, 5 m"), ("While casting", "Free"), ("Slot", "E")],
     [("blink", "Teleports up to 5 m the way you face."), ("immune", "Untouchable for 0.3 s.")],
     "Step through the gap before the blow lands."),
    ("Stone Skin", "stoneskin", "", "R", 4, "Spell · Defensive buff",
     [("Cooldown", "15.0 s"), ("Cast time", "0.8 s"), ("Targeting", "Self"), ("While casting", "Rooted"),
      ("Base power", "50"), ("Slot", "R")],
     [("stoneskin", "Stone Skin for 10 s: a flat cut to damage taken.")],
     "Your skin turns to stone for a while."),
    ("Restoration", "restoration", " tt-spell__icon--heal", "1", 3, "Spell · Heal over time",
     [("Cooldown", "10.0 s"), ("Cast time", "0.6 s"), ("Targeting", "Self"), ("While casting", "Slowed"),
      ("Base power", "40"), ("Heals", "heal"), ("Slot", "1")],
     [("restoration", "Restoration: heals over 5 s, a tick every 1.0 s.")],
     "A slow warmth that mends what the fight broke."),
]

NUMERIC = {"Cooldown", "Cast time", "Base power"}


def value(label, v):
    if v == "fire":
        return '<span class="tt-pill tt-spellcard__type tt-spellcard__type--fire">' + g("fire") + 'Fire</span>'
    if v == "heal":
        return '<span class="tt-pill tt-spellcard__type tt-spellcard__type--heal">' + g("heal") + 'Heal</span>'
    if label == "Slot":
        return f'<span class="tt-key tt-key--sm">{v}</span>'
    if label in NUMERIC and v != "Instant":
        return f'<span class="num-s">{v}</span>'
    return f'<span class="ui-label tt-spellcard__word">{v}</span>'


def card(sp, compact=False, hung=False):
    name, glyph, icon, key, cost, role, rows, effects, desc = sp
    cls = "tt-itemcard tt-spellcard" + (" tt-itemcard--compact" if compact else "") + (" tt-itemcard--hung" if hung else "")
    pips = '<span class="tt-pips tt-pips--enamel" role="img" aria-label="Costs ' + str(cost) + ' Spell Points">' + '<span class="tt-pip is-on"></span>' * cost + '</span>'
    stats = "".join(f'<dt class="ui-label">{lab}</dt><dd>{value(lab, v)}</dd>' for lab, v in rows)
    eff = "".join(f'<li class="tt-itemcard__effect">{g(gl, 20)}<span>{t}</span></li>' for gl, t in effects)
    return (f'<article class="{cls}" aria-label="{name} spell card">'
            f'<div class="tt-itemcard__band">{pips}<span class="tt-itemcard__band-text">SPELL · {cost} SP</span></div>'
            '<div class="tt-itemcard__body">'
            f'<div class="tt-itemcard__head"><span class="tt-spellcard__well"><span class="tt-glyph tt-glyph--48 tt-spell__icon{icon}" data-glyph="{glyph}" aria-label="{name}"></span></span>'
            f'<div><p class="tt-itemcard__name ui-title">{name}</p><p class="tt-itemcard__type ui-label">{role}</p></div></div>'
            f'<dl class="tt-itemcard__stats">{stats}</dl>'
            f'<ul class="tt-itemcard__effects ui-label">{eff}</ul>'
            f'<p class="tt-itemcard__desc ui-body">{desc}</p>'
            '</div></article>')


def slot(sp, focused=False):
    name, glyph, icon, key, cost = sp[:5]
    dot = '<span class="tt-spell__dot"></span>'
    return (f'<div class="tt-plate tt-spell{" is-focused" if focused else ""}" aria-label="{name}, {key}, {cost} SP">'
            f'<div class="tt-spell__well"><span class="tt-glyph tt-glyph--48 tt-spell__icon{icon}" data-glyph="{glyph}" aria-hidden="true"></span></div>'
            f'<div class="tt-spell__cost">{dot * cost}</div><span class="tt-key tt-key--sm tt-spell__key">{key}</span></div>')


def cell(caption, inner):
    return f'<div class="tt-spellcard-sheet__cell"><span class="tt-label">{caption}</span>{inner}</div>'


row1 = '<div class="tt-spellcard-sheet__row">' + "".join(
    cell(f"{s[0]} · {s[4]} SP · slot {s[3]}", card(s)) for s in SPELLS) + '</div>'

hang = ('<div class="tt-spellcard-hang"><div class="tt-spellcard-hang__slots">'
        + "".join(slot(s, i == 0) for i, s in enumerate(SPELLS)) + '</div>' + card(SPELLS[0], compact=True, hung=True) + '</div>')

row2 = ('<div class="tt-spellcard-sheet__row">'
        + cell("Compact (320) · a menu column", card(SPELLS[1], compact=True))
        + cell("Proposal: hung from the focused slot of a loadout row (where it lives is TODO(DECISION))", hang).replace("tt-spellcard-sheet__cell\"", "tt-spellcard-sheet__cell tt-spellcard-sheet__cell--hang\"", 1)
        + '</div>')


def item(title, parts):
    return f'<div class="tt-specband__item"><p class="tt-specband__title">{title}</p>' + "".join(f'<p class="tt-label">{p}</p>' for p in parts) + '</div>'


band = ('<div class="tt-specband" aria-label="Spec notes, not on screen">'
        + item("ItemCard's sibling", ["The same enamel card, band, grommet, stat strip and effect lines (.tt-itemcard + .tt-spellcard). "
                                       "The band is spell-points with the SP cost as pips; the head carries SpellSlot's 64 px glyph well."])
        + item("Real data", ["Costs, cooldowns, timing, targeting, range and movement are the DA_Spell_* values. "
                             "Cast time = WindupTime + CastTime (when the spell fires). Descriptions are example copy: "
                             "UMordecaiSpellDataAsset has no Description field yet (TODO(DECISION))."])
        + item("One device (D5)", ["Slot keycaps are drawn for keyboard (Q, E, R, 1). Gamepad: LB, RB, none, D-pad Up; "
                                   "Stone Skin has no gamepad button (US-097)."])
        + item("Colour is never alone", ["Damage type = pill colour + type glyph + word; heal = the heal pill. On enamel every glyph "
                                         "and word is ink; the colour lives in the pill and the well."])
        + '</div>')

html = f'''<!-- @dsCard group="Menus" height={{H}} width=1760 subtitle="Spell detail card, ItemCard's sibling: SP cost band, the spell glyph well, cooldown, cast time, targeting and damage type, for Fireball, Blink, Stone Skin and Restoration" -->
<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>SpellCard</title></head>
<body>
<div class="tt-root tt-spellcard-sheet">
<div class="tt-spellcard-sheet__body">
{row1}
{row2}
</div>
{band}
</div>
</body>
</html>
'''

if __name__ == "__main__":
    h = int(sys.argv[1]) if len(sys.argv) > 1 else 1814
    d = COMP / "SpellCard"
    d.mkdir(parents=True, exist_ok=True)
    (d / "preview.html").write_text(html.replace("{H}", str(h)), encoding="utf-8")
    print("wrote", d / "preview.html")
