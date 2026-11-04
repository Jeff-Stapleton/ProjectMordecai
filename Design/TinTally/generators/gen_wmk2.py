"""Generate components/WorldMarkers/preview.html.
Ground geometry (UMordecaiCameraMode_Diorama: pitch -50, distance 1800 cm, FOV 60): 1 cm = 0.92 px across at the
screen centre; the floor is foreshortened by sin(50) = 0.77 up the screen. Decal groups are
translate(feet) scale(1 0.77) rotate(a); shapes are in 'ground px' (cm x 0.92). Rotation 0 points up the screen (+X)."""
import math, pathlib, sys

OUT = pathlib.Path(__file__).resolve().parent.parent / "ds" / "project" / "components" / "WorldMarkers" / "preview.html"
HEIGHT = int(sys.argv[1]) if len(sys.argv) > 1 else 3544
K = 0.92
FS = 0.77
_clip = [0]


def f(v):
    return ("%.1f" % v).rstrip("0").rstrip(".")


def svg(w, h, body, left=0, top=0):
    return ('<svg class="tt-wm-svg" style="left:%dpx; top:%dpx;" width="%d" height="%d" viewBox="0 0 %d %d" aria-hidden="true">%s</svg>'
            % (left, top, w, h, w, h, body))


def ground(x, y, rot, body):
    return '<g transform="translate(%s %s) scale(1 %s) rotate(%s)">%s</g>' % (f(x), f(y), FS, f(rot), body)


def sector_d(r, ang):
    h = math.radians(ang / 2)
    x, y = r * math.sin(h), -r * math.cos(h)
    return "M0,0 L%s,%s A%s,%s 0 0 1 %s,%s Z" % (f(-x), f(y), f(r), f(r), f(x), f(y))


def arc_d(r, ang):
    h = math.radians(ang / 2)
    x, y = r * math.sin(h), -r * math.cos(h)
    return "M%s,%s A%s,%s 0 0 1 %s,%s" % (f(-x), f(y), f(r), f(r), f(x), f(y))


def lane_d(w, length, k=1.0):
    hw = w / 2
    L = length * k
    if L <= length - hw:
        return "M%s,0 L%s,%s L%s,%s L%s,0 Z" % (f(-hw), f(-hw), f(-L), f(hw), f(-L), f(hw))
    return "M%s,0 L%s,%s A%s,%s 0 0 1 %s,%s L%s,0 Z" % (f(-hw), f(-hw), f(-(length - hw)), f(hw), f(hw), f(hw), f(-(length - hw)), f(hw))


def circle_d(r):
    return "M%s,0 A%s,%s 0 1 0 %s,0 A%s,%s 0 1 0 %s,0 Z" % (f(-r), f(r), f(r), f(r), f(r), f(r), f(-r))


def edge(d, wide=False, dash=False):
    ink = "tt-wm__ink" + (" tt-wm__ink--wide" if wide else "")
    rim = "tt-wm__rim" + (" tt-wm__rim--wide" if wide else "") + (" tt-wm__rim--dash" if dash else "")
    return '<path d="%s" class="%s"/><path d="%s" class="%s"/>' % (d, ink, d, rim)


HATCH = 'fill="url(#tt-wm-hatch-danger)"'


def sweep(x, y, rot, r_cm, ang, k, band):
    r = r_cm * K
    body = '<path d="%s" %s/>' % (sector_d(r, ang), HATCH)
    if k > 0:
        body += '<path d="%s" class="tt-wm__charge"/>' % sector_d(r * k, ang)
    if band:
        a = arc_d(r - 12, ang - 6)
        body += '<path d="%s" class="tt-wm__band-base"/><path d="%s" class="tt-wm__band"/>' % (a, a)
    body += edge(sector_d(r, ang))
    return ground(x, y, rot, body)


def thrust(x, y, rot, w_cm, l_cm, k):
    w, L = w_cm * K, l_cm * K
    body = '<path d="%s" %s/>' % (lane_d(w, L), HATCH)
    if k > 0:
        body += '<path d="%s" class="tt-wm__charge"/>' % lane_d(w, L, k)
    body += edge(lane_d(w, L))
    return ground(x, y, rot, body)


def slam(x, y, r_cm, k, ring_cm=0):
    r = r_cm * K
    body = ""
    if ring_cm:
        body += edge(circle_d(ring_cm * K))
    body += '<path d="%s" %s/>' % (circle_d(r), HATCH)
    if k > 0:
        body += '<path d="%s" class="tt-wm__charge"/>' % circle_d(r * k)
    body += edge(circle_d(r))
    return ground(x, y, 0, body)


BLOB = "M-120,0 C-122,-58 -64,-86 -6,-80 C62,-92 126,-52 120,-4 C116,52 64,84 4,76 C-64,90 -118,54 -120,0 Z"


def hazard(x, y, r_cm=0, blob=False, scale=1.0):
    """Persistent hazard: the hatch, a solid danger band clipped INSIDE the shape (so the ink outline stays the
    outermost edge), then the dual edge."""
    _clip[0] += 1
    cid = "tt-wm-clip-%d" % _clip[0]
    if blob:
        d = BLOB
        pre, post = '<g transform="scale(%s)">' % f(scale), "</g>"
    else:
        d = circle_d(r_cm * K)
        pre, post = "", ""
    body = (pre + '<clipPath id="%s"><path d="%s"/></clipPath>' % (cid, d)
            + '<path d="%s" %s/>' % (d, HATCH)
            + '<path d="%s" class="tt-wm__hazard-band" clip-path="url(#%s)"/>' % (d, cid)
            + edge(d) + post)
    return ground(x, y, 0, body)


def reticle(x, y, r_cm, state, scale=1.0):
    r = r_cm * K * scale
    body = ""
    if state in ("valid", "cancel"):
        body += '<path d="%s" fill="url(#tt-wm-hatch-rim)"/>' % circle_d(r)
    if state == "committed":
        body += '<path d="%s" fill="url(#tt-wm-hatch-rim-dense)"/>' % circle_d(r)
    body += edge(circle_d(r), wide=True, dash=(state == "invalid"))
    if state in ("valid", "cancel"):
        c = 12 * scale
        body += ('<path d="M%s,0 L%s,0 M0,%s L0,%s" class="tt-wm__mark-line-ink"/><path d="M%s,0 L%s,0 M0,%s L0,%s" class="tt-wm__mark-line"/>'
                 % (f(-c), f(c), f(-c), f(c), f(-c), f(c), f(-c), f(c)))
    if state == "committed":
        body += '<circle r="9" class="tt-wm__mark"/>'
    g = ground(x, y, 0, body)
    if state == "cancel":
        g = '<g opacity="0.5">%s</g>' % g
    return g


def chevron(fx, fy, rot, dist_cm=140):
    d = dist_cm * K
    v = "M-24,10 L0,-12 L24,10"
    return ground(fx, fy, rot, '<g transform="translate(0 %s)"><path d="%s" class="tt-wm__chev-ink"/><path d="%s" class="tt-wm__chev"/></g>' % (f(-d), v, v))


def pawn(fx, fy):
    return ('<div class="pv-abs" style="left:%dpx; top:%dpx; display:flex; flex-direction:column; align-items:center; width:76px;" aria-label="Player">'
            '<div class="tt-callout__pawn"></div><div class="tt-callout__foot"></div></div>' % (fx - 38, fy - 77))


def foe(fx, fy, H, body_cls="", tag=""):
    return ('<div class="pv-abs pv-foe" style="left:%dpx; bottom:%dpx;">%s<div class="tt-etag__body %s"></div><div class="tt-etag__shadow"></div></div>'
            % (fx, H - fy - 10, tag, body_cls))


def stamp(x, y, glyph, label, kind):
    return ('<span class="tt-wm-stamp tt-wm-stamp--%s" style="left:%dpx; top:%dpx;"><span class="tt-glyph" data-glyph="%s" aria-label="%s"></span></span>'
            % (kind, x, y, glyph, label))


def teardrop(R, T):
    phi = math.acos(R / T)
    tx, ty = R * math.cos(phi), R * math.sin(phi)
    return "M%s,0 L%s,%s A%s,%s 0 1 0 %s,%s Z" % (f(T), f(tx), f(-ty), f(R), f(R), f(tx), f(ty))


def pip(x, y, a, elite=False, winding=False, mapscale=False):
    R, T = (20, 33) if elite else (16, 27)
    d = teardrop(R, T)
    body = ""
    if winding:
        rr = R + 10
        body += ('<circle r="%d" class="tt-wm__ink"/><circle r="%d" class="tt-wm__rim"/>' % (rr, rr))
    body += '<g transform="rotate(%s)"><path d="%s" class="tt-wm-pip__body"/><path d="%s" class="tt-wm-pip__edge"/></g>' % (f(a), d, d)
    if elite:
        body += '<rect x="-6" y="-6" width="12" height="12" transform="rotate(45)" class="tt-wm-pip__dia"/>'
    size = 32 if mapscale else 64
    cls = "tt-wm-pip" + (" tt-wm-pip--map" if mapscale else "")
    label = ("Elite enemy" if elite else "Enemy") + (" winding up" if winding else "") + " off screen"
    return ('<span class="%s" style="left:%spx; top:%spx;" role="img" aria-label="%s"><svg width="%d" height="%d" viewBox="-32 -32 64 64">%s</svg></span>'
            % (cls, f(x), f(y), label, size, size, body))


def etag(name, hp, tr, po, elite=False):
    marks = '<span class="tt-etag__marks" aria-label="Elite"><i class="tt-etag__dia"></i></span>' if elite else ""
    return ('<div class="tt-etag"><div class="tt-etag__head">%s<span class="ui-label tt-etag__name">%s</span></div>'
            '<div class="tt-etag__gauge"><div class="tt-bar tt-bar--foe tt-etag__hp" style="--v:%d%%;--t:%d%%"><span class="tt-bar__trail"></span><span class="tt-bar__fill"></span><span class="tt-bar__notch" style="--at:25%%"></span></div>'
            '<div class="tt-bar tt-bar--posture tt-etag__po" style="--v:%d%%"><span class="tt-bar__fill"></span></div></div></div>'
            % (marks, name, hp, tr, po))


def split(left="tt-floor-blockout", right="tt-floor-mire"):
    return '<div class="pv-half %s"></div><div class="pv-half pv-half--r %s"></div>' % (left, right)


def cell(w, h, inner, cap):
    return ('<figure class="pv-cell"><div class="pv-crop" style="width:%dpx; height:%dpx;">%s</div><figcaption class="tt-label pv-cap">%s</figcaption></figure>'
            % (w, h, inner, cap))


def ang_to(x0, y0, x1, y1):
    """Ground rotation (deg, 0 = up the screen) from screen point 0 to screen point 1."""
    return math.degrees(math.atan2(x1 - x0, -(y1 - y0) / FS))


DEFS = ('<svg class="pv-defs" width="0" height="0" aria-hidden="true"><defs>'
        '<pattern id="tt-wm-hatch-danger" width="12" height="12" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><rect width="5" height="12" class="tt-wm__p-danger"/></pattern>'
        '<pattern id="tt-wm-hatch-rim" width="14" height="14" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><rect width="3" height="14" class="tt-wm__p-rim"/></pattern>'
        '<pattern id="tt-wm-hatch-rim-dense" width="10" height="10" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><rect width="5" height="10" class="tt-wm__p-rim"/></pattern>'
        '</defs></svg>')

parts = []
A = parts.append

# ---------------------------------------------------------------- 1. In context (960 x 500, 1:1)
H = 500
PX, PY = 486, 330          # the player's feet
FX, FY = 352, 296          # the Frontliner, sweeping at the player
KX, KY = 806, 214          # the Corrupted Knight (Elite), slamming toward the player
SX, SY = 676, 262          # the slam's point
ctx = []
ctx.append('<div class="tt-diorama tt-diorama--sand"><span class="tt-diorama__rock" style="left:64px; top:60px;"></span>'
           '<span class="tt-diorama__crate" style="left:880px; top:360px;"></span><span class="tt-diorama__post" style="left:120px; top:430px;"></span>'
           '<span class="tt-diorama__bush" style="left:560px; top:30px;"></span></div>')
dec = ""
dec += hazard(250, 432, 95)
dec += slam(SX, SY, 105, 0.35)
dec += sweep(FX, FY, ang_to(FX, FY, PX, PY), 150, 90, 0.6, band=True)
dec += chevron(PX, PY, 0)
ctx.append(svg(960, H, dec))
ctx.append(stamp(250, 432, "fire", "Fire", "fire"))
ctx.append(foe(FX, FY, H, "", etag("Frontliner", 72, 72, 88)))
ctx.append(foe(KX, KY, H, "tt-etag__body--elite", etag("Corrupted Knight", 90, 90, 64, elite=True)))
ctx.append(pawn(PX, PY))
A('<p class="tt-label pv-head"><b>In context</b> · 1:1 around the player · a Frontliner\'s low sweep at 60% of its windup, an Elite\'s slam at 35%, a burning patch (example), the aim chevron (today: +X)</p>')
A('<section class="pv-ctx" style="height:%dpx;">%s</section>' % (H, "".join(ctx)))
A('<p class="tt-label pv-cap pv-cap--wide">Red is threat, cream is yours. A telegraph\'s solid charge grows from the attacker (a slam from its point) and reaches the edge as the hit goes active. The player stands in the sweep: its striped band says a jump clears it.</p>')

# ---------------------------------------------------------------- 2. Telegraphs per shape
cells = []
W, Hc = 240, 240
c1 = split() + svg(W, Hc, sweep(120, 212, 0, 150, 90, 0.6, band=True)) + foe(120, 212, Hc, "tt-etag__body--small")
cells.append(cell(W, Hc, c1, "<b>Low sweep</b> · jumpable · 60%"))
c2 = split() + svg(W, Hc, sweep(120, 212, 0, 150, 90, 0.2, band=False)) + foe(120, 212, Hc, "tt-etag__body--small")
cells.append(cell(W, Hc, c2, "<b>Sweep</b> · not jumpable · 20%"))
c3 = split() + svg(W, Hc, thrust(120, 216, 0, 80, 250, 1.0)) + foe(120, 216, Hc, "tt-etag__body--small")
cells.append(cell(W, Hc, c3, "<b>Thrust</b> lane · active"))
c4 = split() + svg(W, Hc, slam(120, 112, 90, 0.5, ring_cm=125)) + foe(120, 224, Hc, "tt-etag__body--small")
cells.append(cell(W, Hc, c4, "<b>Slam</b> + shockwave ring · 50%"))
A('<p class="tt-label pv-head"><b>Telegraphs</b> · one decal per hit shape, each over blockout (left half) and mire (right half)</p>')
A('<section class="pv-row">%s</section>' % "".join(cells))

# ---------------------------------------------------------------- 3. Hazard zones
cells = []
W, Hc = 480, 210
h1 = split() + svg(W, Hc, hazard(240, 106, 130)) + stamp(240, 106, "fire", "Fire", "fire")
cells.append(cell(W, Hc, h1, "<b>Burning patch</b> (example) · persistent · the fire stamp"))
h2 = split("tt-floor-sand", "tt-floor-night") + svg(W, Hc, hazard(240, 106, blob=True, scale=1.15)) + stamp(240, 104, "poison", "Poison", "poison")
cells.append(cell(W, Hc, h2, "<b>Poison pool</b> (example) · sand and night"))
A('<p class="tt-label pv-head"><b>Hazard zones</b> · persistent ground that hurts: a solid danger band just inside the edge, the hatch, and the damage type\'s stamp</p>')
A('<section class="pv-row pv-row--2">%s</section>' % "".join(cells))

# ---------------------------------------------------------------- 4. The player's markers
cells = []
W, Hc = 320, 250
p1 = split() + svg(W, Hc, chevron(160, 222, 0)) + pawn(160, 222)
cells.append(cell(W, Hc, p1, "<b>Aim</b> · today, pinned to +X"))
p2 = split() + svg(W, Hc, chevron(160, 196, 60)) + pawn(160, 196)
cells.append(cell(W, Hc, p2, "<b>Aim</b> · US-098, toward the cursor"))
p3 = split() + svg(W, Hc, reticle(160, 92, 100, "valid")) + pawn(160, 236)
cells.append(cell(W, Hc, p3, "<b>Ground target</b> · valid"))
p4 = split() + '<span class="tt-wm__pit" style="left:84px; top:36px; width:152px; height:106px;"></span>' + svg(W, Hc, reticle(160, 92, 100, "invalid")) + stamp(160, 92, "cross", "Invalid target", "player") + pawn(160, 236)
cells.append(cell(W, Hc, p4, "<b>Invalid</b> · over a pit: dashed rim, cross"))
p5 = split() + svg(W, Hc, reticle(160, 92, 100, "committed")) + pawn(160, 236)
cells.append(cell(W, Hc, p5, "<b>Committed</b> · cast released"))
p6 = split() + svg(W, Hc, reticle(160, 112, 100, "cancel", scale=0.55)) + pawn(160, 236)
cells.append(cell(W, Hc, p6, "<b>Cancel</b> · shrinks back and fades"))
A('<p class="tt-label pv-head"><b>Your markers</b> · cream with ink · the reticle\'s 100 cm radius is example data</p>')
A('<section class="pv-row pv-row--3">%s</section>' % "".join(cells))
A('<div class="pv-hint"><div class="tt-hintbar"><span class="tt-hint"><span class="tt-key tt-key--sm tt-key--mouse" aria-label="Left mouse button"><span class="tt-key__mouse tt-key__mouse--l"></span></span>Cast</span>'
  '<span class="tt-hintbar__sep"></span><span class="tt-hint"><span class="tt-key tt-key--sm tt-key--mouse" aria-label="Right mouse button"><span class="tt-key__mouse tt-key__mouse--r"></span></span>Cancel</span></div>'
  '<p class="tt-label pv-hintcap"><b>TargetingHint</b> · in the interact prompt box while targeting (InteractPrompt) · keyboard and mouse</p></div>')

# ---------------------------------------------------------------- 5. Off-screen threat pips (the screen at 1/2)
S = 0.5
boxes = [  # x, y, w, h (1920 x 1080 design px), label
    (32, 32, 456, 152, "Gauge plate"), (32, 216, 342, 88, "Spell slots"), (32, 318, 456, 112, "Status chips"),
    (1568, 32, 320, 56, "Rail head"), (1552, 868, 336, 80, "Quick items"), (1548, 964, 340, 84, "Loadout"),
    (690, 984, 540, 60, "Prompt"), (1242, 994, 151, 40, ""),
]
MW, MH = 960, 540
m = ['<div class="tt-wm-map" style="width:%dpx; height:%dpx;">' % (MW, MH)]
lane = 56 * S
m.append('<div class="tt-wm-map__lane" style="left:%spx; top:%spx; right:%spx; bottom:%spx;"></div>' % (f(lane - 1), f(lane - 1), f(lane - 1), f(lane - 1)))
cx, cy = MW / 2, MH / 2


def ray(x2, y2):
    dx, dy = x2 - cx, y2 - cy
    return '<div class="tt-wm-map__ray" style="left:%spx; top:%spx; width:%spx; transform: rotate(%sdeg);"></div>' % (f(cx), f(cy - 1), f(math.hypot(dx, dy)), f(math.degrees(math.atan2(dy, dx))))


pins = [(lane, 318, 180, False, False), (236, MH - lane, 90, True, False), (MW - lane, 236, 0, False, True)]
ghost = (790, MH - lane)
slid = (740, MH - lane)
for x, y, a, el, wd in pins:
    m.append(ray(x, y))
m.append(ray(*ghost))
for x, y, w, h, lab in boxes:  # the boxes cover the lane and rays: a pip can never land under one
    m.append('<div class="tt-wm-map__box tt-label" style="left:%spx; top:%spx; width:%spx; height:%spx;">%s</div>' % (f(x * S), f(y * S), f(w * S), f(h * S), lab))
m.append('<div class="tt-wm-map__ghost" style="left:%spx; top:%spx;"></div>' % (f(ghost[0]), f(ghost[1])))
m.append('<div class="tt-wm-map__you" style="left:%dpx; top:%dpx;"></div>' % (cx, cy))
for x, y, a, el, wd in pins:
    m.append(pip(x, y, a, el, wd, mapscale=True))
m.append(pip(slid[0], slid[1], math.degrees(math.atan2(ghost[1] - cy, ghost[0] - cx)), mapscale=True))
m.append("</div>")
A('<p class="tt-label pv-head"><b>Off-screen threats</b> · pips on a lane 56 px inside the screen edge, pointing at aggroed enemies the camera cannot see · the screen at 1/2</p>')
A('<section class="pv-map">%s</section>' % "".join(m))
A('<p class="tt-label pv-cap pv-cap--wide">Lines run from the player to each enemy. A pip that would land on a HUD box slides along the lane to the nearest free spot (the hollow ring is where it would have landed: under the loadout plate).</p>')
spec = []
sw, sh = 320, 100
spec.append(cell(sw, sh, '<div class="pv-full tt-floor-blockout"></div>' + pip(56, 50, 180), "<b>Basic</b> · left edge · blockout"))
spec.append(cell(sw, sh, '<div class="pv-full tt-floor-mire"></div>' + pip(160, sh - 56, 90, elite=True), "<b>Elite</b> · bottom edge · mire"))
spec.append(cell(sw, sh, '<div class="pv-full tt-floor-night"></div>' + pip(sw - 56, 50, 0, winding=True), "<b>Winding up</b> · right edge · night"))
A('<section class="pv-row pv-row--3">%s</section>' % "".join(spec))

# ---------------------------------------------------------------- 6. Spec band
notes = [
    ("Colours", "danger for every enemy telegraph and hazard (threat only); rim cream on ink for the player's own markers; a damage type's colour only in a hazard's stamp."),
    ("Dual edge", "Every decal edge is ink | rim | ink: 3 | 2 | 3 px on screen (4 | 2 | 4 in High Contrast), held at every depth."),
    ("Timing", "The charge fills over WindupTimeMs and meets the edge as the hit goes active; the whole shape is solid for ActiveTimeMs and clears at Recovery."),
    ("Jump band", "Only a JumpAvoidable sweep carries the striped band. Thrusts and slams hit the airborne and never carry it."),
    ("Ground target", "Keyboard and mouse (drawn): the reticle follows the cursor, left click casts, right click cancels. Gamepad: right stick, A casts, B cancels."),
    ("No text on the floor", "Shapes and glyph stamps only. Who builds the decals, UI or VFX, is TODO(DECISION)."),
]
band = "".join('<div class="tt-specband__item"><p class="tt-specband__title">%s</p><p class="tt-label">%s</p></div>' % n for n in notes)
A('<div class="tt-specband pv-spec">%s</div>' % band)

STYLE = """/* preview-only layout scaffolding */
.pv-defs { position: absolute; width: 0; height: 0; }
.pv-head { margin: 0; padding: 16px 20px 10px; }
.pv-head b, .pv-cap b, .pv-hintcap b { color: var(--text); }
.pv-ctx { position: relative; overflow: hidden; }
.pv-abs { position: absolute; }
.pv-foe { display: flex; flex-direction: column; align-items: center; transform: translateX(-50%); }
.pv-foe > .tt-etag { margin-bottom: 14px; }
.pv-row { display: grid; grid-template-columns: repeat(4, 240px); }
.pv-row--2 { grid-template-columns: repeat(2, 480px); }
.pv-row--3 { grid-template-columns: repeat(3, 320px); }
.pv-cell { margin: 0; display: flex; flex-direction: column; }
.pv-crop { position: relative; overflow: hidden; }
.pv-half { position: absolute; top: 0; bottom: 0; left: 0; width: 50%; }
.pv-half--r { left: 50%; }
.pv-full { position: absolute; inset: 0; }
.pv-cap { margin: 0; padding: 8px 14px 14px; }
.pv-cap--wide { padding: 10px 20px 4px; }
.pv-hint { display: flex; align-items: center; gap: 24px; padding: 14px 20px 18px; }
.pv-hintcap { margin: 0; }
.pv-map { padding: 0; }
.pv-spec { margin-top: 8px; }
.pv-spec > .tt-specband__item { flex: 1 1 240px; max-width: none; }
"""

html = ('<!-- @dsCard group="World" height=%d width=960 subtitle="Floor decals and screen-edge pips for the fixed camera: enemy telegraphs per hit shape, the jump band, hazard zones, the aim chevron, the ground-target reticle and off-screen threats" -->\n' % HEIGHT
        + '<!doctype html>\n<html lang="en">\n<head><meta charset="utf-8"><title>WorldMarkers</title>\n<style>\n' + STYLE + '</style>\n</head>\n<body>\n'
        '<div class="tt-root">\n' + DEFS + "\n" + "\n".join(parts) + "\n</div>\n</body>\n</html>\n")
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(html, encoding="utf-8")
print("wrote", OUT, len(html))
