"""Shared pause-shell markup for the PauseShell / CharacterTab / SkillsTab / FeatsTab previews.
Preview generator (not published). Each tab module calls page(...) and writes its preview.html.

Other components that draw the pause frame (InventoryTab, OptionsScreen, service screens) can import this
module instead of copying markup:  sys.path.insert(0, Design/TinTally/pause_gen); from shell import frame, footer.
  frame(active, content, ...)  -> the whole 1920 x 1080 screen (diorama, real HUD, veil, nameplate, tabs,
                                  content plate, hint bar, spike)
  footer(device, extra)        -> the hint bar; extra = [("Enter", "Identify")] adds contextual hints
  spike(open_index, focus)     -> the spike alone
"""
import pathlib

COMP = pathlib.Path(__file__).resolve().parent.parent / "ds" / "project" / "components"

TABS = [("character", "Character"), ("skills", "Skills"), ("feats", "Feats"), ("inventory", "Inventory"), ("settings", "Settings")]


def g(name, size=None, label=None, cls=""):
    s = f' tt-glyph--{size}' if size else ''
    lab = f' aria-label="{label}"' if label else ' aria-hidden="true"'
    return f'<span class="tt-glyph{s}{(" " + cls) if cls else ""}" data-glyph="{name}"{lab}></span>'


def rarity(r, height=None):
    h = f' data-height="{height}"' if height else ''
    return f'<span class="tt-socket"><span class="tt-rarity tt-r-{r}" data-rarity="{r}"{h}></span></span>'


# ---------------------------------------------------------------- the world behind the veil (D12, D10)

def diorama():
    """The toy diorama (core .tt-diorama, grass) with a few blocky props, under the veil. Props sit where the
    shell leaves the world visible: the left column, the bottom strip and the bottom right."""
    props = [
        ("patch", "left:-140px; top:640px;"),
        ("path", "left:912px; top:860px; height:240px;"),
        ("pond", "left:1600px; top:858px;"),
        ("tree", "left:72px; top:356px;"),
        ("tree", "left:300px; top:612px;"),
        ("tree", "left:40px; top:1010px;"),
        ("bush", "left:196px; top:500px;"),
        ("bush", "left:64px; top:930px;"),
        ("bush", "left:1240px; top:1010px;"),
        ("rock", "left:356px; top:860px;"),
        ("rock", "left:1460px; top:930px;"),
        ("crate", "left:150px; top:760px;"),
        ("crate", "left:620px; top:990px;"),
        ("post", "left:1500px; top:870px;"),
        ("post", "left:1530px; top:870px;"),
        ("post", "left:760px; top:1000px;"),
    ]
    inner = "".join(f'<span class="tt-diorama__{k}" style="{s}"></span>' for k, s in props)
    return f'<div class="tt-diorama tt-pause__world" aria-hidden="true">{inner}</div>'


def _tokens(n, mx, cls="tt-tally-sp"):
    """Spell Point tokens, groups of five (Tally's .tt-tally-sp markup, as GaugePlate builds it)."""
    out = [f'<span class="{cls}" aria-hidden="true">']
    for grp in range((mx + 4) // 5):
        out.append('<span class="tt-tally-sp__group">')
        for i in range(grp * 5, min(mx, grp * 5 + 5)):
            out.append(f'<span class="tt-tally-sp__token{" is-on" if i < n else ""}"></span>')
        out.append('</span>')
    out.append('</span>')
    return "".join(out)


# One moment for every pause preview: the example build of CharacterTab (Health 118 / 156, Stamina full,
# posture full so its row is hidden, Spell Points 7 / 12), Kills: 12, the Longsword equipped, the stocked quick
# items (Health Potion x2) and, on the pause tabs, Frostbitten and Focused active (STATUS_CHIPS).
HUD_STATE = {"hp": 76, "sp": 7, "sp_max": 12, "kills": 12}


def gauge(hp, sp, sp_max):
    """GaugePlate markup, exactly as HUDLayout's script builds it (hp %, Stamina Green at 100%, posture hidden)."""
    def tick(at):
        return f'<span class="tt-gauge__tick" style="--at:{at}%"></span>'

    def notch(at):
        return f'<span class="tt-bar__notch" style="--at:{at}%"></span>'
    sp_cls = "tt-tally-sp tt-tally-sp--16" if sp_max > 10 else "tt-tally-sp"
    return ('<div class="tt-plate tt-gauge" role="group" aria-label="Player resources">'
            f'<div class="tt-gauge__row tt-gauge__row--hp"><span class="num-s tt-gauge__label">HP</span><div class="tt-gauge__track">{tick(25)}'
            f'<div class="tt-bar tt-bar--vital" style="--v:{hp}%;--t:0%;--l:0%"><span class="tt-bar__trail"></span><span class="tt-bar__lead"></span><span class="tt-bar__fill"></span>{notch(25)}</div></div>'
            f'<span class="num-l tt-gauge__value tt-gauge__value--hp">{hp}%</span></div>'
            f'<div class="tt-gauge__row tt-gauge__row--st"><span class="num-s tt-gauge__label">ST</span><div class="tt-gauge__track">{tick(33)}{tick(66)}'
            f'<div class="tt-bar tt-bar--green" style="--v:100%;--d:0%"><span class="tt-bar__fill"></span>{notch(33)}{notch(66)}</div></div>'
            '<span class="tt-pill tt-pill--green tt-gauge__pill">×1.25</span></div>'
            '<div class="tt-gauge__row tt-gauge__row--po is-hidden"><span></span><div class="tt-gauge__track">'
            '<div class="tt-bar tt-bar--posture" style="--v:0%"><span class="tt-bar__fill"></span></div></div><span></span></div>'
            f'<div class="tt-gauge__row tt-gauge__row--sp"><span class="num-s tt-gauge__label">SP</span>{_tokens(sp, sp_max, sp_cls)}'
            f'<span class="num-m tt-gauge__value">{sp} / {sp_max}</span></div>'
            '</div>')


SPELLS = [("Fireball", "fireball", "Q", 3, " tt-spell__icon--fire"), ("Blink", "blink", "E", 2, ""),
          ("Stone Skin", "stoneskin", "R", 4, ""), ("Restoration", "restoration", "1", 3, " tt-spell__icon--heal")]

# The two active statuses of the pause moment: HUDLayout/preview.html's fight frame (frame 1), verbatim:
# Frostbitten at 2 of 4 stacks (5.1 s left, frozen while paused) and Focused (the 3/3 streak). The HUD copy shows
# them in its status strip; CharacterTab's "Active statuses" row shows the same two chips, focusable.
STATUS_CHIPS = [
    '<span class="tt-status tt-status--debuff"{a} aria-label="Frostbitten"><span class="tt-glyph tt-status__glyph tt-status__glyph--frost" data-glyph="frostbitten" aria-hidden="true"></span><span class="tt-status__name">Frostbitten</span><span class="tt-pips tt-status__pips tt-status__pips--frost" role="img" aria-label="2 of 4 stacks"><span class="tt-pip is-on"></span><span class="tt-pip is-on"></span><span class="tt-pip"></span><span class="tt-pip"></span></span><span class="num-s tt-status__time">5.1</span></span>',
    '<span class="tt-status tt-status--buff"{a} aria-label="Focused"><span class="tt-glyph tt-status__glyph" data-glyph="focused" aria-hidden="true"></span><span class="tt-status__name">Focused</span><span class="num-s tt-status__time tt-status__time--word">3/3</span></span>',
]


def status_chips(focusable=False):
    """StatusChip's markup for the two statuses; focusable=True adds tabindex (a menu chip that opens StatusTooltip)."""
    a = ' tabindex="0"' if focusable else ''
    return "".join(c.format(a=a) for c in STATUS_CHIPS)


# The stocked quick items, HUDLayout/preview.html's markup verbatim (frame 3, Health Potion x2 as the inventory
# holds): G on the Throwing Knife, the dashed unbound cap on Item A, 2 and 3; the Antidote on `tonic`; no Item D.
QUICK_ITEMS = (
    '<div class="tt-hud__quick"><div class="tt-quick">'
    '<div class="tt-plate tt-quick__slot tt-quick__slot--throw" aria-label="Throwing Knife, 5"><span class="tt-glyph tt-quick__icon" data-glyph="throwable" aria-hidden="true"></span><span class="num-s tt-quick__count">x5</span><span class="tt-key tt-key--sm tt-quick__key">G</span></div>'
    '<div class="tt-plate tt-quick__slot" aria-label="Health Potion, 2"><span class="tt-glyph tt-quick__icon tt-quick__icon--potion" data-glyph="potion" aria-hidden="true"></span><span class="num-s tt-quick__count">x2</span><span class="tt-key tt-key--sm tt-key--none tt-quick__key" aria-label="Unbound"></span></div>'
    '<div class="tt-plate tt-quick__slot" aria-label="Antidote, 1"><span class="tt-glyph tt-quick__icon" data-glyph="tonic" aria-hidden="true"></span><span class="num-s tt-quick__count">x1</span><span class="tt-key tt-key--sm tt-quick__key">2</span></div>'
    '<div class="tt-plate tt-quick__slot" aria-label="Bandage, 2"><span class="tt-glyph tt-quick__icon" data-glyph="bandage" aria-hidden="true"></span><span class="num-s tt-quick__count">x2</span><span class="tt-key tt-key--sm tt-quick__key">3</span></div>'
    '<div class="tt-plate tt-quick__slot is-empty" aria-hidden="true"></div>'
    '</div></div>')


def hud(state=HUD_STATE, statuses=False):
    """The real HUD (D10): HUDLayout's markup and classes for the always-visible pieces (D20): gauge plate,
    spell slots, rail head, the [I] Inventory hint pill and the loadout plate, plus the stocked QuickItems row
    (the player carries Health Potion x2). statuses=True adds the status strip with the pause moment's two chips
    (the pause tabs; service screens leave it out). The pause filed the waiting tickets to the spike, so the rail
    head has no badge and no tickets. Under the veil (UI.Layer.Game)."""
    dot = '<span class="tt-spell__dot"></span>'
    spells = "".join(
        f'<div class="tt-plate tt-spell"><div class="tt-spell__well"><span class="tt-glyph tt-glyph--48 tt-spell__icon{ic}" data-glyph="{gl}" aria-hidden="true"></span></div>'
        f'<div class="tt-spell__cost">{dot * cost}</div><span class="tt-key tt-key--sm tt-spell__key">{key}</span></div>'
        for name, gl, key, cost, ic in SPELLS)
    strip = f'<div class="tt-hud__status"><div class="tt-status-row">{status_chips()}</div></div>' if statuses else ''
    return ('<div class="tt-hud tt-pause__hud" aria-hidden="true">'
            f'<div class="tt-hud__gauge">{gauge(state["hp"], state["sp"], state["sp_max"])}</div>'
            f'<div class="tt-hud__spells">{spells}</div>'
            f'{strip}'
            '<div class="tt-hud__rail"><div class="tt-rail"><div class="tt-plate tt-plate--rail tt-railhead">'
            f'<div class="tt-plate__row"><span class="tt-railhead__kills num-l">Kills: {state["kills"]}</span></div>'
            '<span class="tt-railhead__slot"></span></div></div></div>'
            '<div class="tt-hud__prompt"><div class="tt-iprompt"><div class="tt-iprompt__box"></div>'
            '<div class="tt-iprompt__hint"><span class="tt-key tt-key--sm">I</span><span class="ui-label tt-iprompt__hint-text">Inventory</span></div></div></div>'
            f'{QUICK_ITEMS}'
            '<div class="tt-hud__loadout"><div class="tt-plate tt-loadout"><div class="tt-loadout__well">' + g("longsword", 48) + '</div>'
            '<div class="tt-loadout__body"><span class="ui-title tt-loadout__name">Longsword</span>'
            '<span class="ui-label tt-loadout__sub">' + g("slash") + '<span>Slash</span></span></div>'
            '<span class="tt-key tt-loadout__key">Tab</span></div></div>'
            '</div>')


def world(statuses=False):
    """The diorama, the HUD and the veil. statuses=True: the pause tabs' moment (the HUD shows the two active
    statuses that CharacterTab lists); service screens (Mage Tower, World Map) call world() without them."""
    return diorama() + "\n" + hud(statuses=statuses) + '\n<div class="tt-pause__veil tt-veil" aria-hidden="true"></div>'


# ---------------------------------------------------------------- nameplate, tabs, footer

def nameplate(title="Paused", screen=False, glyph=None):
    """The enamel nameplate on two light strings. screen=True: a service screen on UI.Layer.Menu
    (.tt-pause__title--screen: 52 px strings, an optional glyph before the title)."""
    gl = g(glyph, 32) if glyph else ""
    cls = "tt-pause__title" + (" tt-pause__title--screen" if screen else "")
    return (f'<div class="{cls}"><span class="tt-pause__strings" aria-hidden="true"></span>'
            f'<h1 class="tt-pause__nameplate tally-heading">{gl}{title}</h1></div>')


def tab(tid, label, selected=False, focused=False, hover=False, pressed=False, static=False):
    cls = "tt-pause__tab tt-focus--own"
    if selected:
        cls += " tt-selectable is-selected"
    if focused:
        cls += " is-focused"
    if hover:
        cls += " is-hover"
    if pressed:
        cls += " is-pressed"
    attrs = ' tabindex="-1"' if static else f' role="tab" aria-selected="{"true" if selected else "false"}" data-tab-id="{tid}"'
    return f'<button class="{cls}"{attrs}>{g(tid)}<span class="ui-title">{label}</span></button>'


def tabbar(active, focused=None, hover=None, device="kbm"):
    """device "kbm" (every screen, D5): no keycaps at the bar ends, tabs centred, headers are clicked.
    device "pad" (spec band only): LB / RB keycaps at the bar ends."""
    pad = device == "pad"
    out = [f'<nav class="tt-pause__tabbar{"" if pad else " tt-pause__tabbar--kbm"}" aria-label="Pause menu tabs">']
    if pad:
        out.append('  <span class="tt-key tt-key--shoulder tt-pause__bumper">LB</span>')
    out.append('  <div class="tt-pause__tabs" role="tablist">')
    for tid, label in TABS:
        out.append("    " + tab(tid, label, selected=tid == active, focused=tid == focused, hover=tid == hover))
    out.append('  </div>')
    if pad:
        out.append('  <span class="tt-key tt-key--shoulder tt-pause__bumper">RB</span>')
    out.append('</nav>')
    return "\n".join(out)


def hint(keys, verb, device="kbm"):
    kc = "tt-key tt-key--sm" + (" tt-key--pad" if device == "pad" and keys in ("A", "B", "X", "Y") else "")
    if keys == "LB / RB":
        caps = ('<span class="tt-key tt-key--sm tt-key--shoulder">LB</span><span class="tt-pause__slash">/</span>'
                '<span class="tt-key tt-key--sm tt-key--shoulder">RB</span>')
    else:
        caps = f'<span class="{kc}">{keys}</span>'
    return f'<span class="tt-hint">{caps}{verb}</span>'


def hintbar(pairs, device="kbm", cls=""):
    sep = '<span class="tt-hintbar__sep" aria-hidden="true"></span>'
    return (f'<footer class="tt-hintbar{(" " + cls) if cls else ""}" aria-label="Menu actions">'
            + sep.join(hint(k, v, device) for k, v in pairs) + '</footer>')


def footer(device="kbm", extra=(), cls="tt-pause__footer", static=False):
    """The hint bar (core .tt-hintbar, D25), keycap first. One device per screen (D5).
    Keyboard and mouse: Esc Back | I Close. Gamepad (spec band only): B Back | LB / RB Tabs.
    extra: contextual CommonUI actions from the focused widget, after the shell's own, e.g. [("Enter", "Identify")].
    static=True (or cls="tt-pause__bandbar"): a free-standing bar for a spec band."""
    if static:
        cls = "tt-pause__bandbar"
    pairs = [("B", "Back"), ("LB / RB", "Tabs")] if device == "pad" else [("Esc", "Back"), ("I", "Close")]
    return hintbar(pairs + list(extra), device, cls)


# ---------------------------------------------------------------- the spike (D9, D23)

# The last 12 entries, newest first. Ticket kinds and copy are TallyRail's (D26), compressed to one line:
# (kind css, kicker or None, name, number or None, glyph or rarity for a slip, unread, jump target, full-ticket extras)
SPIKE = [
    ("skill", "Skill up", "Longswords", "12 / 20", None, True, "skills"),
    ("r-purple", "Item · Purple", "Unidentified Purple Amulet", None, None, True, "inventory"),
    ("r-common", None, "Health Potion", "x2", "potion", True, "inventory"),
    ("feat-common", "Feat progress", "Pyromaniac", None, None, False, "feats"),
    ("r-common", "Stored", "Iron Ore, Town Stone", None, None, False, "inventory"),
    ("skill", "Skill up", "Dodging", "6 / 20", None, False, "skills"),
    ("r-blue", "Item · Blue", "Upgrade Key", None, None, False, "inventory"),
    ("r-common", None, "Rusty Sword", None, "weapon", False, "inventory"),
    ("r-purple", "Identified", "Mysterious Amulet", None, None, False, "inventory"),
    ("skill", "Skill up", "Parrying", "3 / 20", None, False, "skills"),
    ("r-common", None, "Iron Ore", "x1", "ore", False, "inventory"),
    ("r-common", None, "Town Stone", "x1", "stone", False, "inventory"),
]


def entry(kind, kicker, name, num, glyph, unread, jump, focused=False):
    """One spike entry: TallyRail's ticket classes (.tt-tag.tt-ticket + the kind's stub), one line."""
    cls = f"tt-tag tt-ticket tt-ticket--{kind} tt-pause__entry" + (" is-new" if unread else "") + (" is-focused" if focused else "")
    peel = '<span class="tt-ticket__peel" aria-hidden="true"></span>' if unread else ""
    if kicker is None:  # a slip: glyph + name + count (or the rarity socket when the item does not stack)
        end = f'<span class="tt-ticket__count num-s">{num}</span>' if num else rarity("common", 18)
        body = f'{g(glyph, label=name)}<span class="tt-tag__line tt-ticket__name">{name}</span>{end}'
    else:
        end = f'<span class="tt-ticket__rank tt-pause__entryrank num-s">{num}</span>' if num else ""
        body = f'<span class="tt-tag__kicker">{kicker}</span><span class="tt-tag__line tt-ticket__name">{name}</span>{end}'
    return f'<li class="{cls}" tabindex="0" data-jump="{jump}">{body}{peel}</li>'


def entry_open(focused=True):
    """The focused entry opens to its full rail ticket (TallyRail's 88 px ITEM ticket, at spike width)."""
    return ('<li class="tt-tag tt-ticket tt-ticket--tall tt-ticket--r-purple tt-pause__entry is-new'
            + (' is-focused' if focused else '') + '" tabindex="0" data-jump="inventory">'
            '<div class="tt-ticket__row"><span class="tt-tag__kicker">Item · Purple</span>' + rarity("purple") + '</div>'
            '<div class="tt-ticket__row"><span class="tt-tag__line tt-ticket__name tt-ticket__name--wrap">Unidentified Purple Amulet</span></div>'
            '<span class="tt-ticket__peel" aria-hidden="true"></span></li>')


def spike(open_index=None):
    """The spike: head plate, a rod through every eyelet, a ScrollBox of one-line entries (8 visible, the 9th
    cut by the box edge, 12 kept) and the rail-family base (.tt-railbar) the rod stands in."""
    items = []
    for i, e in enumerate(SPIKE):
        items.append(entry_open() if i == open_index else entry(*e))
    return ('<aside class="tt-pause__spike" aria-label="The spike: the last 12 tickets">\n'
            '  <div class="tt-plate tt-pause__spikehead"><span class="kicker tt-caps">The spike</span><span class="tt-label">Last 12 tickets</span></div>\n'
            '  <span class="tt-pause__rod" aria-hidden="true"></span>\n'
            '  <span class="tt-railbar tt-pause__spikebase" aria-hidden="true"></span>\n'
            '  <ol class="tt-pause__list">\n    ' + "\n    ".join(items) + '\n  </ol>\n'
            '</aside>')


def points(n):
    """The skill-point badge, one look on the Character and Skills tabs (SkillsTab.css .tt-skills__points):
    a gold coin with the skill-point glyph, then "Skill Points" over the count. At 0 it goes dark (.is-empty)."""
    empty = " is-empty" if n == 0 else ""
    return (f'<div class="tt-skills__points{empty}"><span class="tt-skills__coin">' + g("skill-point", label="Skill point") + '</span>'
            f'<span class="tt-skills__pointstext"><span class="ui-label">Skill Points</span><span class="num-l">{n}</span></span></div>')


# ---------------------------------------------------------------- the frame and the page

def frame(active, content, focused_tab=None, hover_tab=None, spike_open=None, extra_hints=(), label=None, inner_extra=""):
    """The whole 1920 x 1080 pause screen, keyboard and mouse (D5). inner_extra: raw markup placed inside the screen."""
    return (f'<section class="tt-pause" data-active-tab="{active}" aria-label="{label or dict(TABS)[active] + " tab at 1920 by 1080"}">\n'
            f'{world(statuses=True)}\n{nameplate()}\n{tabbar(active, focused_tab, hover_tab)}\n'
            f'<section class="tt-plate tt-plate--menu tt-pause__content" role="tabpanel" aria-label="{dict(TABS)[active]}">\n{content}\n</section>\n'
            f'{footer(extra=extra_hints)}\n{spike(spike_open)}\n{inner_extra}</section>')


def band(items):
    """The spec band under the screen (core .tt-specband, D7). items: (title, [html, ...]) or
    (title, [html, ...], "wide") for a note that needs 700 px (a full-width shipping string)."""
    out = ['<div class="tt-specband tt-pause__band" aria-label="Spec notes, not on screen">']
    for item in items:
        title, parts = item[0], item[1]
        wide = " tt-pause__bandwide" if len(item) > 2 and item[2] == "wide" else ""
        out.append(f'  <div class="tt-specband__item{wide}"><p class="tt-specband__title">{title}</p>' + "".join(parts) + '</div>')
    out.append('</div>')
    return "\n".join(out)


def note(text):
    return f'<p class="tt-label">{text}</p>'


def page(comp, subtitle, active, content, band_items=(), height=None, focused_tab=None, hover_tab=None, spike_open=None,
         extra_hints=(), spike_focus=None, extra_script=""):
    """Writes components/<comp>/preview.html: the screen, then the spec band (band_items). A str band_items is
    the old call style (raw markup inside the screen) and is still accepted."""
    inner = ""
    if isinstance(band_items, str):
        inner, band_items = band_items, ()
    if spike_open is None:
        spike_open = spike_focus
    if height is None:
        height = 1080
    html = f'''<!-- @dsCard group="Menus" height={height} width=1920 subtitle="{subtitle}" -->
<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>{comp}</title></head>
<body>
<div class="tt-root tt-pause-sheet">
{frame(active, content, focused_tab, hover_tab, spike_open, extra_hints, inner_extra=inner)}
{band(band_items) if band_items else ""}
</div>
{extra_script}
</body>
</html>
'''
    d = COMP / comp
    d.mkdir(parents=True, exist_ok=True)
    (d / "preview.html").write_text(html, encoding="utf-8")
    print("wrote", d / "preview.html", len(html.encode("utf-8")) // 1024, "KB")
