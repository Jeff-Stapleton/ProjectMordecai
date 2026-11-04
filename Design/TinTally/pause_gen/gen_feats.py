import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from shell import page, g, note

# Pyromaniac is the only real feat (US-012 sample). Every other feat here is an EXAMPLE: the brand book's
# recurring examples (01-voice-and-copy) Steel Rhythm (Rare) and Living Furnace (Legendary), with TallyPlate's
# glyphs, causes and drawbacks, and Unflinching (Common); Packrat and the three locked feats fill the board.
# (name, tier, the feat's own glyph (D14), new, selected)
UNLOCKED = [
    ("Pyromaniac", "common", "fire", True, False),
    ("Unflinching", "common", "resolved", False, False),
    ("Packrat", "common", "chest", False, False),
    ("Steel Rhythm", "rare", "block", False, False),
    ("Living Furnace", "legendary", "fireball", False, True),
]
TIER = {"common": "Common", "rare": "Rare", "legendary": "Legendary"}

# Condition labels are the proposed DisplayLabel per unlock condition (D14, example values); today the raw
# stat / tag name prints ("TotalPerfectParries", "Requires: Mordecai.Feat.Pyromaniac"). The feat display checks a
# tag condition against the player's granted FEAT tags (HasGrantedFeatTag), so "Requires: X" names a feat.
# Perfect parries stand past Steel Rhythm's 50; Parrying rank 3 as on the spike.
LOCKED = [
    ("Iron Will", "rare", [("stat", "Perfect parries", 64, 100), ("stat", "Parrying rank", 3, 10)]),
    ("Fireproof", "common", [("tag", "Pyromaniac", True), ("stat", "Burns survived", 7, 20)]),
    ("Untouchable", "legendary", [("stat", "Perfect dodges", 88, 200), ("tag", "Iron Will", False)]),
]


def badge(tier, glyph, size="", locked=False):
    cls = f"tt-feats__badge tt-feats__badge--{tier}" + (" tt-feats__badge--lg" if size == "lg" else "") + (" is-locked" if locked else "")
    gs = 48 if size == "lg" else None
    return f'<span class="{cls}" role="img" aria-label="{TIER[tier]} badge">{g("unknown" if locked else glyph, gs)}</span>'


def cell(name, tier, glyph, new, selected, focused=None):
    focused = selected if focused is None else focused
    cls = ("tt-feats__cell tt-selectable tt-focus--fill-gap" + (" is-selected" if selected else "")
           + (" is-focused" if focused else "") + (" is-new" if new else ""))
    peel = '<span class="tt-ticket__peel" role="img" aria-label="New"></span>' if new else ""
    return (f'<li class="{cls}" tabindex="0">{badge(tier, glyph)}<span class="tt-feats__ctext">'
            f'<span class="tt-feats__cname ui-label">{name}</span><span class="tt-feats__tier tt-feats__tier--{tier} ui-label">{TIER[tier]}</span></span>{peel}</li>')


def cond(c):
    """A condition row: the label, then a thin bar (core .tt-bar--thin) and the count "64 / 100" (D1: no strokes);
    a tag condition reads "Requires: {label}" with check + "Met" or cross + "Not met"."""
    if c[0] == "stat":
        _, label, cur, thr = c
        pct = round(100 * cur / thr)
        return (f'<div class="tt-feats__cond"><dt class="ui-label">{label}</dt>'
                f'<dd><span class="tt-bar tt-bar--thin tt-feats__condbar" style="--v:{pct}%"><span class="tt-bar__fill"></span></span>'
                f'<span class="tt-feats__val num-s">{cur} / {thr}</span></dd></div>')
    _, tag, met = c
    gl, word, cls = ("check", "Met", "is-met") if met else ("cross", "Not met", "is-unmet")
    return (f'<div class="tt-feats__cond"><dt class="ui-label">Requires: {tag}</dt>'
            f'<dd class="tt-feats__req {cls} ui-label">{g(gl, label=word)}{word}</dd></div>')


def lrow(name, tier, conds):
    return (f'<li class="tt-feats__lrow">{badge(tier, "", locked=True)}'
            f'<span class="tt-feats__lname"><span class="ui-label">{name}</span><span class="tt-feats__lsub"><span class="tt-feats__tier tt-feats__tier--{tier} ui-label">{TIER[tier]}</span>'
            f'<span class="tt-feats__q ui-label">???</span></span></span>'
            f'<dl class="tt-feats__conds">{"".join(cond(c) for c in conds)}</dl></li>')


def drawback(text):
    """The one Drawback box (core .tt-drawback, D14): the hatch strip, then "Drawback" inline and the cost."""
    return (f'<div class="tt-drawback tt-feats__drawback"><p class="tt-drawback__text"><span class="tt-drawback__label">Drawback</span>{text}</p></div>')


def sechead(label, n):
    return (f'<h3 class="tt-feats__sechead ui-label tt-caps">{label} <span class="num-s">{n}</span>'
            '<span class="tt-divider" aria-hidden="true"></span></h3>')


head = '<header class="tt-feats__head"><h2 class="tt-feats__title ui-title">Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary</h2></header>'

main = ('<div class="tt-feats__main">' + sechead("Unlocked", 5)
        + '<ul class="tt-feats__grid">' + "".join(cell(*u) for u in UNLOCKED) + '</ul>'
        + sechead("Locked", 3)
        + '<ul class="tt-feats__locked">' + "".join(lrow(*l) for l in LOCKED) + '</ul></div>')

detail = ('<aside class="tt-feats__detail" aria-label="Living Furnace">'
          '<div class="tt-feats__dtop">' + badge("legendary", "fireball", "lg") +
          '<div class="tt-feats__dmeta"><span class="tt-feats__pill tt-feats__pill--legendary">Legendary</span>'
          '<span class="tt-feats__dstatus ui-label">' + g("check", 20, "Unlocked") + 'Unlocked</span></div></div>'
          '<h3 class="tt-feats__dname tally-heading">Living Furnace</h3>'
          '<p class="tt-feats__ddesc ui-body">Fire spells cost 1 less Spell Point.</p>'
          '<span class="tt-feats__dcount ui-label">1 Effect, 0 Abilities, 1 Drawback</span>'
          '<div class="tt-feats__dsec"><span class="tt-divider" aria-hidden="true"></span><h4 class="tt-feats__sechead tt-feats__sechead--sub ui-label tt-caps">Earned by</h4>'
          '<dl class="tt-feats__conds">' + cond(("stat", "Burns of 6", 10, 10)) + '</dl></div>'
          + drawback("Frost damage taken +50%.") +
          '</aside>')

content = f'<div class="tt-feats">{head}<div class="tt-feats__body">{main}{detail}</div></div>'

shapes = ('<div class="tt-feats__noteshapes">'
          + "".join(f'<span class="tt-feats__noteshape">{badge(t, gl)}<span class="tt-feats__tier tt-feats__tier--{t} ui-label">{TIER[t]}</span></span>'
                    for t, gl in (("common", "fire"), ("rare", "block"), ("legendary", "fireball")))
          + "".join(f'<span class="tt-feats__noteshape">{badge(t, "", locked=True)}<span class="ui-label tt-feats__q">Locked</span></span>'
                    for t in ("common", "rare", "legendary"))
          + '</div>')

cells = ('<div class="tt-feats__notecells">'
         + "".join(f'<figure class="tt-feats__notecell"><ul class="tt-feats__grid tt-feats__grid--note">{c}</ul><figcaption class="tt-label">{cap}</figcaption></figure>'
                   for c, cap in ((cell("Pyromaniac", "common", "fire", False, False), "Idle"),
                                  (cell("Pyromaniac", "common", "fire", True, False), "New"),
                                  (cell("Steel Rhythm", "rare", "block", False, True, focused=False), "Selected"),
                                  (cell("Steel Rhythm", "rare", "block", False, True, focused=True), "Selected + focused")))
         + '</div>')

empty = ('<div class="tt-feats__noteempty"><span class="ui-title">Feats: 0 unlocked — 0 Common, 0 Rare, 0 Legendary</span>'
         '<div class="tt-empty-box"><p class="tt-empty">No feats unlocked yet.<span class="tt-empty__sub">Feats are earned, not picked.</span></p></div></div>')

conds = ('<dl class="tt-feats__conds tt-feats__conds--note">'
         + cond(("stat", "Perfect parries", 64, 100)) + cond(("tag", "Pyromaniac", True)) + cond(("tag", "Iron Will", False)) + '</dl>')

band = [
    ("Tier = silhouette + word", [shapes, note("Circle Common, flat-top hexagon Rare, eight-point starburst Legendary (D14), each carrying "
                                                 "the feat's own glyph (Pyromaniac = fire). Locked: a hollow lip silhouette with the unknown glyph.")]),
    ("Board cells", [cells, note("Selected is the cream fill (D6): the tier word turns ink-muted, never a feat colour on cream. "
                                 "New: the rail's peeled corner until the cell has been focused once.")]),
    ("Condition rows", [conds, note("A thin bar and the count \"64 / 100\" (D1: no strokes). Tag conditions name a feat the player must hold: "
                                    "check + Met, or cross + Not met in text-muted (red means threat only). "
                                    "Labels are the proposed DisplayLabel (example); today the raw stat or tag name prints.")]),
    ("No feats yet", [empty, note("The shipping header still reads \"Feats: 0 unlocked — ...\"; the empty board is the one empty state (core .tt-empty, D8). "
                                  "The Locked list still shows what can be earned. Copy proposed.")], "wide"),
    ("Rare detail: the Drawback box", ['<div class="tt-feats__notedetail">' + drawback("Blocking costs 25% more Stamina.") + '</div>',
                                       note("Core .tt-drawback: the hatch strip, \"Drawback\" inline, the cost from the proposed DrawbackText (D14), "
                                            "here Steel Rhythm's. Every Rare and Legendary feat shows it; Common feats have none.")]),
]

if __name__ == "__main__":
    page("FeatsTab", "Feats tab: tier badges with the feat's glyph, locked feats with condition bars, detail with the Drawback box",
         "feats", content, band, height=1792)
