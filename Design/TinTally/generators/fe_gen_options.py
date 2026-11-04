"""Generate OptionsScreen/preview.html: the Accessibility frame and its band, the Controls frame
and its band. Bindings are the game's (MordecaiHeroComponent.cpp, Config/DefaultInput.ini)."""
import pathlib
OUT = pathlib.Path(__file__).resolve().parent.parent / "ds" / "project" / "components" / "OptionsScreen" / "preview.html"


def g(name, label=None, size=""):
    s = f" tt-glyph--{size}" if size else ""
    a = f' aria-label="{label}"' if label else ' aria-hidden="true"'
    return f'<span class="tt-glyph{s}" data-glyph="{name}"{a}></span>'


def key(t, mods=""):
    return f'<span class="tt-key{(" " + mods) if mods else ""}">{t}</span>'


def mouse(side, label):
    return f'<span class="tt-key tt-key--mouse" aria-label="{label}"><span class="tt-key__mouse tt-key__mouse--{side}"></span></span>'


HOLD = '<span class="tt-keys__word">Hold</span>'
PLUS = '<span class="tt-keys__plus">+</span>'
NONE = '<span class="tt-key tt-key--none" aria-label="Unbound"></span>'
LMB, RMB = mouse("l", "Left mouse button"), mouse("r", "Right mouse button")


def world():
    props = ('<span class="tt-diorama__path" style="left:864px;top:0;width:96px;height:1080px"></span>'
             '<span class="tt-diorama__path" style="left:0;top:576px;width:1920px;height:96px"></span>'
             '<span class="tt-diorama__pond" style="left:1500px;top:760px"></span>'
             '<span class="tt-diorama__tree" style="left:120px;top:150px"></span><span class="tt-diorama__tree" style="left:230px;top:200px"></span>'
             '<span class="tt-diorama__tree" style="left:1640px;top:420px"></span>'
             '<span class="tt-diorama__bush" style="left:300px;top:820px"></span><span class="tt-diorama__bush" style="left:1180px;top:860px"></span>'
             '<span class="tt-diorama__rock" style="left:620px;top:120px"></span><span class="tt-diorama__rock" style="left:1440px;top:180px"></span>'
             '<span class="tt-diorama__crate" style="left:1000px;top:470px"></span><span class="tt-diorama__crate" style="left:1064px;top:470px"></span>'
             '<span class="tt-diorama__post" style="left:760px;top:700px"></span><span class="tt-diorama__post" style="left:760px;top:780px"></span>')
    return (f'<div class="tt-diorama" aria-hidden="true">{props}</div>'
            '<div class="tt-veil tt-opt-veil" aria-hidden="true"></div>')


TABS = [("settings", "Gameplay"), ("display", "Display"), ("audio", "Audio"), ("controls", "Controls"), ("accessibility", "Accessibility")]


def chrome(active):
    tabs = []
    for glyph, label in TABS:
        on = label == active
        cls = "tt-pause__tab tt-selectable" + (" is-active is-selected" if on else "")
        tabs.append(f'<button class="{cls}" role="tab" aria-selected="{"true" if on else "false"}">{g(glyph)}<span class="ui-title">{label}</span></button>')
    return ('<div class="tt-pause__title"><span class="tt-pause__strings" aria-hidden="true"></span><h1 class="tt-pause__nameplate tally-heading">Options</h1></div>\n'
            '    <nav class="tt-pause__tabbar tt-pause__tabbar--kbm" aria-label="Options sections">\n      <div class="tt-pause__tabs" role="tablist">\n        '
            + "\n        ".join(tabs) + '\n      </div>\n    </nav>')


def hintbar(pairs, cls=""):
    out = []
    for keys, label in pairs:
        out.append(f'<span class="tt-hint">{keys}{label}</span>')
    return f'<div class="tt-hintbar{(" " + cls) if cls else ""}" aria-label="Options actions">' + '<span class="tt-hintbar__sep" aria-hidden="true"></span>'.join(out) + '</div>'


ARROWS = key(g("arrow-left", "Left", "20"), "tt-key--sm") + key(g("arrow-right", "Right", "20"), "tt-key--sm")


def switch(on, extra="", disabled=False):
    cls = "tt-opt-switch" + (" is-on" if on else "") + (" is-disabled" if disabled else "") + (f" {extra}" if extra else "")
    mark = "lock" if disabled else ("check" if on else "cross")
    dis = ' aria-disabled="true"' if disabled else ""
    return (f'<span class="{cls}" role="switch" aria-checked="{"true" if on else "false"}"{dis}>'
            f'<span class="tt-glyph tt-opt-switch__mark" data-glyph="{mark}" aria-hidden="true"></span><span class="tt-opt-switch__key">{"On" if on else "Off"}</span></span>')


def seg(opts, chosen, mono=False):
    out = []
    for o in opts:
        cls = "tt-opt-seg__opt" + (" num-s" if mono else "") + " tt-selectable" + (" is-selected" if o == chosen else "")
        out.append(f'<span class="{cls}" role="radio" aria-checked="{"true" if o == chosen else "false"}">{o}</span>')
    return '<span class="tt-opt-seg" role="radiogroup">' + "".join(out) + '</span>'


def slider(value, left_dis=False, right_dis=False):
    v = (value - 80) / 70 * 100
    notches = "".join(f'<span class="tt-bar__notch" style="--at:{k * 100 / 14:.2f}%"></span>' for k in range(2, 14, 2))
    ld = ' aria-disabled="true"' if left_dis else ""
    rd = ' aria-disabled="true"' if right_dis else ""
    return (f'<span class="tt-opt-slider" role="slider" aria-valuemin="80" aria-valuemax="150" aria-valuenow="{value}">'
            f'<span class="tt-key tt-key--sm"{ld}>{g("arrow-left", "Smaller", "20")}</span>'
            f'<span class="tt-opt-track" style="--v:{v:.2f}%"><span class="tt-bar"><span class="tt-bar__fill"></span>{notches}</span><span class="tt-opt-track__thumb" aria-hidden="true"></span></span>'
            f'<span class="tt-key tt-key--sm"{rd}>{g("arrow-right", "Larger", "20")}</span>'
            f'<span class="num-m tt-opt-slider__val">{value}%</span></span>')


def row(label, control, focused=False):
    cls = "tt-opt-row tt-focus--fill-gap" + (" is-focused" if focused else "")
    return f'<li class="{cls}"><span class="ui-title tt-opt-row__label">{label}</span>{control}</li>'


ACC_ROWS = [
    row("High Contrast", switch(False, "tt-opt-hc-off") + switch(True, "tt-opt-hc-on")),
    row("Reduce motion", switch(False)),
    row("Tally notifications", seg(["Full", "Calm", "Minimal"], "Full")),
    row("Tally remarks", switch(True), focused=True),
    row("Ticket duration", seg(["×1", "×1.5", "×2"], "×1", mono=True)),
    row("Callout size", seg(["Normal", "Large", "XL"], "Normal")),
    row("Damage numbers", seg(["All", "Mine", "Off"], "All")),
    row("UI scale", slider(100)),
    row("HUD margin", seg(["32 px", "96 px"], "32 px", mono=True)),
]

PREVIEW = '''<aside class="tt-opt-preview" aria-label="Live preview">
      <div class="tt-rail" style="--tt-rail-tail:28px">
        <div class="tt-plate tt-plate--rail tt-railhead"><div class="tt-plate__row"><span class="tt-railhead__kills num-l">Kills: 12</span></div><span class="tt-railhead__slot" aria-hidden="true"></span></div>
        <div class="tt-tag-stack">
          <div class="tt-tag tt-ticket tt-ticket--tall tt-ticket--skill has-foot is-new">
            <div class="tt-ticket__row"><span class="tt-tag__kicker">Skill up</span><span class="tt-ticket__rank tt-ticket__rank--head num-m">6 / 20</span></div>
            <div class="tt-ticket__row"><span class="tt-tag__line tt-ticket__name">Dodging</span></div>
            <div class="tt-ticket__foot"><span class="tt-glyph tt-glyph--20" data-glyph="seal" aria-label="The Tally"></span><span class="tt-ticket__foottext">Six. Up from five.</span></div>
            <span class="tt-ticket__peel" aria-hidden="true"></span>
          </div>
          <div class="tt-tag tt-ticket tt-ticket--slip tt-ticket--r-common"><span class="tt-glyph" data-glyph="ore" aria-label="Iron Ore"></span><span class="tt-tag__line tt-ticket__name">Iron Ore</span><span class="tt-ticket__count num-s">x2</span></div>
        </div>
      </div>
      <span class="tt-plate tt-opt-preview__label tt-label">Live preview</span>
    </aside>'''

frame1 = f'''  <!-- 1920x1080: Options from the title screen, Accessibility section (in game the same panel becomes the pause menu's
       Settings tab). The toy world under the veil (D12). Keyboard and mouse (D5): no bumpers at the tab-bar ends. -->
  <section class="tt-pause tt-opt-screen" aria-label="Options, Accessibility, at 1920 by 1080">
    {world()}
    {chrome("Accessibility")}
    <section class="tt-plate tt-plate--menu tt-pause__content tt-opt-content" role="tabpanel" aria-label="Accessibility">
      <ul class="tt-opt-list">
        {chr(10).join("        " + r for r in ACC_ROWS).strip()}
      </ul>
      <p class="tt-opt-desc ui-label"><span class="tt-opt-desc__name">Tally remarks</span><span class="tt-opt-desc__text">The Tally's second line on tickets and plates. Headlines read complete without it.</span></p>
    </section>
    {hintbar([(key("Esc", "tt-key--sm tt-key--wide"), "Back"), (ARROWS, "Change")], "tt-opt-footer")}
    {PREVIEW}
  </section>
'''

TICKET_OFF = ('<div class="tt-tag tt-ticket tt-ticket--tall tt-ticket--skill">'
              '<div class="tt-ticket__row"><span class="tt-tag__kicker">Skill up</span><span class="tt-ticket__rank tt-ticket__rank--head num-m">6 / 20</span></div>'
              '<div class="tt-ticket__row"><span class="tt-tag__line tt-ticket__name">Dodging</span></div></div>')
PAD_HINT = hintbar([(key("B", "tt-key--pad"), "Back"),
                    (key("LB", "tt-key--shoulder") + '<span class="tt-keys__plus">/</span>' + key("RB", "tt-key--shoulder"), "Sections"),
                    ('<span class="tt-key tt-key--dpad tt-key--dpad-right" aria-label="D-pad"></span>', "Change")])


def item(title, body, wide=""):
    cls = "tt-specband__item" + (f" tt-opt-specitem--{wide}" if wide else "")
    return f'    <div class="{cls}">\n      <p class="tt-specband__title">{title}</p>\n      {body}\n    </div>\n'


band1 = ('  <section class="tt-specband tt-fe-band" aria-label="Accessibility controls">\n'
         + item("Keycap switch: Off, On, Locked",
                f'<div class="tt-opt-states"><div class="tt-opt-states__row">{switch(False)}{switch(True)}{switch(False, disabled=True)}</div>'
                '<p class="tt-label">Two values. The keycap slides and prints its word; a check or cross marks the other half. Enter, a click, or Left / Right toggles it.</p></div>')
         + item("Segmented keys: Calm chosen",
                f'<div class="tt-opt-states">{seg(["Full", "Calm", "Minimal"], "Calm")}'
                '<p class="tt-label">Three or four values. The chosen key is Selected: the cream fill with ink text used for every selected piece.</p></div>')
         + item("Notched track: the two ends",
                f'<div class="tt-opt-states">{slider(80, left_dis=True)}{slider(150, right_dis=True)}'
                '<p class="tt-label">UI scale, 80-150%. The value moves in 5% steps; a notch marks every 10%. At an end its arrow keycap is disabled.</p></div>')
         + item("Live preview, Tally remarks Off",
                f'<div class="tt-opt-states">{TICKET_OFF}<p class="tt-label">The preview re-prints on every change: here the footer band is gone.</p></div>')
         + item("Gamepad hint bar",
                f'<div class="tt-opt-states">{PAD_HINT}<p class="tt-label">Shown only after a pad is used; LB and RB pills then sit at the tab-bar ends. Keyboard section cycling is undecided (Q / E proposed), so it is never drawn.</p></div>')
         + '  </section>\n')

# Controls rows: (action, kbm caps, pad caps). The game's bindings today.
CTL = [
    ("Move", key("W") + key("A") + key("S") + key("D"), key("L", "tt-key--stick")),
    ("Sprint (toggle)", key("Shift", "tt-key--wide"), key("L", "tt-key--stick") + '<span class="tt-keys__word">Click</span>'),
    ("Jump", key("Space", "tt-key--wide"), key("A", "tt-key--pad")),
    ("Dodge", None, key("B", "tt-key--pad")),
    ("Light attack", LMB, key("RT", "tt-key--trigger")),
    ("Heavy attack", HOLD + LMB, HOLD + key("RT", "tt-key--trigger")),
    ("Block", HOLD + RMB, HOLD + key("LT", "tt-key--trigger")),
    ("Parry", RMB + PLUS + LMB, key("LT", "tt-key--trigger") + PLUS + key("RT", "tt-key--trigger")),
    ("Fireball", key("Q"), key("LB", "tt-key--shoulder")),
    ("Blink", key("E"), key("RB", "tt-key--shoulder")),
    ("Stone Skin", key("R"), NONE),
]


def ctlrow(action, kbm, pad, focused=False):
    k = kbm if kbm is not None else '<span class="tt-opt-listen">Press a key...</span>'
    cls = "tt-opt-row tt-opt-ctlrow tt-focus--fill-gap" + (" is-focused" if focused else "")
    return (f'<li class="{cls}"><span class="ui-title tt-opt-row__label">{action}</span>'
            f'<span class="tt-opt-keys">{k}</span><span class="tt-opt-keys">{pad}</span></li>')


ctl_rows = "\n          ".join(ctlrow(a, k, p, focused=(a == "Dodge")) for a, k, p in CTL)

frame2 = f'''  <!-- 1920x1080: the Controls section while Dodge listens for a new key. Rows carry the game's bindings today. -->
  <section class="tt-pause tt-opt-screen" aria-label="Options, Controls, at 1920 by 1080">
    {world()}
    {chrome("Controls")}
    <section class="tt-plate tt-plate--menu tt-pause__content" role="tabpanel" aria-label="Controls">
      <div class="tt-opt-ctl">
        <div class="tt-opt-ctl__head tt-label"><span>Action</span><span>Keyboard and mouse</span><span>Gamepad</span></div>
        <div class="tt-opt-ctl__body">
          <ul class="tt-opt-list">
          {ctl_rows}
          </ul>
          <span class="tt-opt-scrollbar" aria-hidden="true"><span class="tt-opt-scrollbar__thumb"></span></span>
        </div>
      </div>
    </section>
    {hintbar([(key("Esc", "tt-key--sm tt-key--wide"), "Cancel")], "tt-opt-footer")}
  </section>
'''

SPEC_ROWS = ('<div class="tt-opt-specrows"><ul class="tt-opt-list">'
             + ctlrow("Interact", key("F"), key("X", "tt-key--pad"))
             + ctlrow("Restoration", key("1"), '<span class="tt-key tt-key--dpad tt-key--dpad-up" aria-label="D-pad Up"></span>')
             + ctlrow("Inventory", key("I"), NONE)
             + '</ul></div>')

DIALOG = ('<section class="tt-plate tt-plate--menu tt-dlg" role="alertdialog" aria-labelledby="okt" aria-describedby="okb">'
          '<div class="tt-dlg__main"><span class="tt-dlg__stamp"><span class="tt-glyph tt-glyph--32" data-glyph="seal" aria-label="The Tally seal"></span></span>'
          '<div class="tt-dlg__text"><h2 class="tally-heading tt-dlg__title" id="okt">Key already bound</h2>'
          '<p class="ui-body tt-dlg__body" id="okb"><span class="tt-key tt-key--sm">Q</span>is Fireball\'s key. Bind it to Blink and leave Fireball without one?</p></div></div>'
          '<div class="tt-dlg__foot"><button class="tt-btn tt-dlg__btn" type="button">Replace</button>'
          '<button class="tt-btn tt-btn--primary tt-dlg__btn is-focused" type="button"><span class="tt-key tt-key--sm">Esc</span>Cancel</button></div></section>')

band2 = ('  <section class="tt-specband tt-fe-band" aria-label="Controls rows">\n'
         + item("Controls rows: idle, D-pad, unbound",
                f'<div class="tt-opt-states">{SPEC_ROWS}<p class="tt-label">Action in ui-title; the keyboard-and-mouse cap and the gamepad cap in fixed columns, drawn as Keycap draws them (mouse buttons as a mouse, "Hold" before a held input, "+" for a chord). An action with no button on a device shows the dashed unbound cap. The list scrolls; below Blink: Stone Skin (R / none), Restoration (1 / D-pad Up), Interact (F / X), Next weapon (Tab / Y), Previous weapon (\\ / none), Inventory (I / none), Pause (Esc / Menu).</p></div>',
                wide="rows")
         + item("Listening, then already bound",
                f'<div class="tt-opt-states"><p class="tt-label">Enter or a click on a cap: the slot reads "Press a key..." and the next key binds; Esc cancels (so Esc cannot be bound). A key another action uses opens the Dialog\'s Key already bound variant (example copy):</p>{DIALOG}</div>',
                wide="dlg")
         + '  </section>\n')

html = ('<!-- @dsCard group="Front end" height=3106 width=1920 subtitle="Options in the pause-shell frame over the toy world: the Accessibility rows (switches, segmented keys, a notched track) with a live rail preview, and the Controls rows" -->\n'
        '<!doctype html>\n<html lang="en">\n<head><meta charset="utf-8"><title>OptionsScreen</title></head>\n<body>\n<div class="tt-root">\n\n'
        + frame1 + '\n  <!-- Spec band (D7): control states, the remarks-off preview and the gamepad hint bar. -->\n' + band1 + '\n'
        + frame2 + '\n  <!-- Spec band (D7): Controls row states and the rebind flow. -->\n' + band2
        + '</div>\n</body>\n</html>\n')
OUT.write_text(html, encoding="utf-8")
print("wrote", OUT)
