"""Generate Dialog/preview.html. The six variants share the shell and the shot scaffolding."""
import pathlib
OUT = pathlib.Path(__file__).resolve().parent.parent / "ds" / "project" / "components" / "Dialog" / "preview.html"


def world(mod, props):
    return f'<div class="tt-diorama{mod}" aria-hidden="true">{props}</div><div class="tt-veil" aria-hidden="true"></div>'


PROPS_A = ('<span class="tt-diorama__path" style="left:288px;top:0;height:448px"></span>'
           '<span class="tt-diorama__tree" style="left:40px;top:36px"></span>'
           '<span class="tt-diorama__rock" style="left:600px;top:56px"></span>'
           '<span class="tt-diorama__bush" style="left:580px;top:340px"></span>'
           '<span class="tt-diorama__crate" style="left:110px;top:330px"></span>')
PROPS_B = ('<span class="tt-diorama__pond" style="left:500px;top:28px"></span>'
           '<span class="tt-diorama__post" style="left:96px;top:60px"></span><span class="tt-diorama__post" style="left:160px;top:60px"></span>'
           '<span class="tt-diorama__crate" style="left:600px;top:330px"></span>'
           '<span class="tt-diorama__rock" style="left:60px;top:340px"></span>')
PROPS_C = ('<span class="tt-diorama__patch" style="left:-80px;top:220px"></span>'
           '<span class="tt-diorama__tree" style="left:590px;top:40px"></span>'
           '<span class="tt-diorama__bush" style="left:60px;top:60px"></span>'
           '<span class="tt-diorama__rock" style="left:600px;top:340px"></span>')


def stamp(glyph, label, danger=False):
    cls = "tt-dlg__stamp tt-dlg__stamp--danger" if danger else "tt-dlg__stamp"
    return f'<span class="{cls}"><span class="tt-glyph tt-glyph--32" data-glyph="{glyph}" aria-label="{label}"></span></span>'


def dialog(did, title, body, buttons, stamp_html, band=False):
    b = f'<p class="ui-body tt-dlg__body" id="{did}b">{body}</p>' if body else ''
    desc = f' aria-describedby="{did}b"' if body else ''
    band_html = ('<div class="tt-dlg__band"><span class="tt-glyph tt-glyph--20" data-glyph="cross" aria-hidden="true"></span>'
                 '<span class="tt-dlg__band-word tt-caps">Error</span><span class="tt-dlg__hatch tt-hatch" aria-hidden="true"></span></div>') if band else ''
    return (f'<section class="tt-plate tt-plate--menu tt-dlg" role="alertdialog" aria-labelledby="{did}t"{desc}>{band_html}'
            f'<div class="tt-dlg__main">{stamp_html}<div class="tt-dlg__text"><h2 class="tally-heading tt-dlg__title" id="{did}t">{title}</h2>{b}</div></div>'
            f'<div class="tt-dlg__foot">{buttons}</div></section>')


def btn(label, primary=False, key=None, state=""):
    cls = "tt-btn tt-dlg__btn" + (" tt-btn--primary" if primary else "") + (f" {state}" if state else "")
    k = f'<span class="tt-key tt-key--sm{" tt-key--wide" if key and len(key) > 3 else ""}">{key}</span>' if key else ''
    return f'<button class="{cls}" type="button">{k}{label}</button>'


HINT_KBM = ('<div class="tt-hintbar" aria-label="Dialog actions"><span class="tt-hint"><span class="tt-key tt-key--sm"><span class="tt-glyph tt-glyph--20" data-glyph="arrow-left" aria-label="Left"></span></span>'
            '<span class="tt-key tt-key--sm"><span class="tt-glyph tt-glyph--20" data-glyph="arrow-right" aria-label="Right"></span></span>Move</span>'
            '<span class="tt-hintbar__sep" aria-hidden="true"></span><span class="tt-hint"><span class="tt-key tt-key--sm tt-key--wide">Enter</span>Select</span></div>')


def cell(world_html, inner, title, notes, extra=""):
    notes_html = "".join(f'<p class="tt-label">{n}</p>' for n in notes)
    return (f'  <div class="tt-dlg-cell">\n    <div class="tt-dlg-shot">{world_html}\n      {inner}\n    </div>\n'
            f'    <div class="tt-specband"><div class="tt-specband__item tt-fe-specwide"><p class="tt-specband__title">{title}</p>{notes_html}{extra}</div></div>\n  </div>\n')


SEAL = stamp("seal", "The Tally seal")


def quit_btns(q="", s="is-focused"):
    return btn("Quit", state=q) + btn("Stay", True, "Esc", s)


cells = []
cells.append(cell(world("", PROPS_A),
    dialog("q1", "Quit to desktop?", "The Tally will keep count.", quit_btns(), SEAL) + "\n      " + HINT_KBM,
    "1. Quit confirm, as it opens",
    ["Default focus on Stay, the safe answer, on the right: the primary button. Quit, the risky answer, is a plain button on the left, never danger.",
     "The keycap leads its label: [Esc] Stay. Back (Esc) always picks Stay."]))
cells.append(cell(world("", PROPS_A),
    dialog("q2", "Quit to desktop?", "The Tally will keep count.", quit_btns("is-focused", ""), SEAL) + "\n      " + HINT_KBM,
    "2. Focus moved to Quit",
    ["Left / Right (or the mouse) move the ring. Esc still means Stay; Enter picks the focused button."]))
cells.append(cell(world("", PROPS_A),
    dialog("q3", "Quit to desktop?", "", quit_btns("is-focused is-pressed", ""), SEAL) + "\n      " + HINT_KBM,
    "3. Tally remarks Off, Quit pressed",
    ["The body line is a Tally remark: with Tally remarks Off it is not shown and the title stands alone.",
     "Pressed: the button sinks 3 px and its print collapses; the dialog closes with that result."]))
cells.append(cell(world(" tt-diorama--blockout", PROPS_B),
    dialog("e1", "Couldn't load the map", "The Expedition failed to start. You will return to the title screen.",
           btn("Return to title", True, "Esc", "is-focused"), stamp("cross", "Error", True), band=True),
    "4. Error (example copy)",
    ["A danger band with the hatch and a danger stamp with the cross: errors only, for failures the player did not choose.",
     "One primary button, which is also Back. No Tally remark."]))
PAD_SPEC = ('<div class="tt-fe-specimen"><button class="tt-btn tt-btn--primary tt-dlg__btn" type="button" tabindex="-1">'
            '<span class="tt-key tt-key--sm tt-key--pad">A</span>Continue</button></div>')
cells.append(cell(world(" tt-diorama--sand", PROPS_C),
    dialog("c1", "Controller disconnected", "Reconnect it to keep playing.",
           btn("Continue", True, "Enter", "is-focused"), stamp("controls", "Controller")),
    "5. Controller disconnected",
    ["The same shell: an enamel stamp with the controls glyph and one primary button. Drawn for keyboard and mouse: [Enter] Continue goes on with the keyboard.",
     "With a pad the button reads (A) Continue; the dialog also closes by itself when a controller reconnects (Lyra issues #62, #68)."],
    PAD_SPEC))
cells.append(cell(world("", PROPS_C),
    dialog("k1", "Key already bound", '<span class="tt-key tt-key--sm">Q</span>is Fireball\'s key. Bind it to Blink and leave Fireball without one?',
           btn("Replace") + btn("Cancel", True, "Esc", "is-focused"), SEAL) + "\n      " + HINT_KBM,
    "6. Key already bound (Options, Controls)",
    ["Opened when a rebind picks a key another action uses (example copy). Replace is the risky answer (left); Cancel is safe (right, default, Back)."]))

html = ('<!-- @dsCard group="Menus" height=2200 width=1536 subtitle="Modal dialog over the veil: quit confirm (Stay is the default, on the right), error, controller disconnected and key already bound" -->\n'
        '<!doctype html>\n<html lang="en">\n<head><meta charset="utf-8"><title>Dialog</title></head>\n<body>\n'
        '<div class="tt-root tt-dlg-board">\n' + "".join(cells) + '</div>\n</body>\n</html>\n')
OUT.write_text(html, encoding="utf-8")
print("wrote", OUT)
