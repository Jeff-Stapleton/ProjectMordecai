"""Generate project/components/ReceiptTimeline/preview.html (the ReceiptTimeline generator; edit here and re-run:
   py -I gen_receipt_rv2.py)."""
import pathlib

OUT = pathlib.Path(__file__).resolve().parent.parent / "ds" / "project" / "components" / "ReceiptTimeline" / "preview.html"

AXIS_MAX = 23
HITS = [(0.0, 0), (0.5, 0), (1.0, 1), (1.6, 0), (2.2, 0), (3.0, 0), (3.4, 0), (4.1, 1), (5.0, 0), (5.6, 0),
        (7.0, 0), (7.5, 0), (8.3, 1), (9.2, 0), (10.5, 0)]
EVENTS = [(1.0, "1", "+1"), (3.0, "2", "+2"), (5.0, "3", "+3"), (7.5, "5", "+3"), (9.5, "6", "+3")]
PICKUP = 6.0
LAST_HIT = 10.5
HOLD_END = LAST_HIT + 4.0
RECEIPT = HOLD_END + 2.5


def f(x):
    return f"{x:g}"


def axis():
    out = []
    for s in range(AXIS_MAX + 1):
        major = s % 2 == 0
        out.append(f'<span class="tt-receipt__tick{" tt-receipt__tick--major" if major else ""}" style="--t:{s}"></span>')
        if major:
            out.append(f'<span class="tt-receipt__ticklabel num-s" style="--t:{s}">{s}</span>')
    return "\n            ".join(out)


def hits():
    return "\n            ".join(
        f'<span class="tt-receipt__at tt-receipt__hit{" tt-receipt__hit--taken" if taken else ""}" style="--t:{f(t)}" aria-label="{"Damage taken" if taken else "Damage dealt"} at {f(t)} s"></span>'
        for t, taken in HITS)


def mark(k, t=None, extra=""):
    """A numbered ink stamp for an event (not a keycap: a keycap "1" would read as the 1 key)."""
    style = f' style="--t:{f(t)}"' if t is not None else ""
    return f'<span class="tt-receipt__mark{extra}"{style} aria-label="Event {k}">{k}</span>'


def keys():
    ks = [(t, k) for t, k, _ in EVENTS] + [(PICKUP, "4")]
    return "\n            ".join(mark(k, t) for t, k in sorted(ks))


def pills():
    out = [f'<span class="tt-receipt__span tt-receipt__band tt-receipt__band--live" style="--t:{f(EVENTS[0][0])}; --d:{f(RECEIPT + 0.7 - EVENTS[0][0])}"></span>']
    for t, _, p in EVENTS:
        out.append(f'<span class="tt-receipt__at tt-pill tt-receipt__pill" style="--t:{f(t)}" aria-label="{p[1:]} waiting"><span class="num-s">{p}</span></span>')
    out.append(f'<span class="tt-receipt__after" style="--t:{f(EVENTS[0][0])}; --d:{f(RECEIPT + 0.7 - EVENTS[0][0])}">counts down</span>')
    return "\n            ".join(out)


page = f"""<!-- @dsCard group="Tally" height=1903 width=1280 subtitle="The pacing source of truth: one fight held and printed, the +N badge, 2.5 s of calm, 350 ms apart, the 20 s cap, overflow, pause and Tally notifications" -->
<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title>ReceiptTimeline</title>
<style>
/* Preview scaffolding only. */
.pv {{ padding: 24px 24px 32px; }}
.pv-legend {{ margin: 18px 0 0; padding: 0; list-style: none; display: grid; grid-template-columns: 1fr 1fr; gap: 10px 32px; }}
.pv-legend li {{ display: flex; align-items: flex-start; gap: 12px; font: 600 20px/26px var(--font-ui); color: var(--ink); }}
.pv-legend .tt-receipt__mark {{ margin-top: -1px; }}
.pv-legend .num-s {{ color: var(--ink-muted); min-width: 84px; white-space: nowrap; }}
.pv-muted {{ color: var(--ink-muted); }}
.pv-right {{ display: flex; flex-direction: column; gap: 24px; }}
</style>
</head>
<body>
<div class="tt-root pv">
<div class="tt-receipt">

  <div class="tt-plate tt-plate--menu tt-receipt__head">
    <span class="tt-receipt__title tally-heading">Receipt pacing</span>
    <span class="tt-receipt__sub ui-body">One fight, as the Tally holds it and prints it. Callouts and damage numbers never wait.</span>
    <span class="tt-receipt__slot" aria-hidden="true"></span>
  </div>

  <div class="tt-receipt__paper">
    <div class="tt-receipt__grid">
      <div class="tt-receipt__guides" aria-hidden="true">
        <span class="tt-receipt__guide" style="--t:{f(LAST_HIT)}"></span>
        <span class="tt-receipt__guide" style="--t:{f(HOLD_END)}"></span>
        <span class="tt-receipt__guide" style="--t:{f(RECEIPT)}"></span>
      </div>
      <span class="tt-receipt__name tt-receipt__name--axis">Seconds</span>
      <div class="tt-receipt__track tt-receipt__track--axis">
            {axis()}
      </div>

      <span class="tt-receipt__name">Hits</span>
      <div class="tt-receipt__track">
            {hits()}
      </div>

      <span class="tt-receipt__name">Earned</span>
      <div class="tt-receipt__track">
            {keys()}
      </div>

      <span class="tt-receipt__name">Badge</span>
      <div class="tt-receipt__track">
            {pills()}
      </div>

      <span class="tt-receipt__name">Hold</span>
      <div class="tt-receipt__track tt-receipt__track--tall tt-receipt__track--hold">
            <span class="tt-receipt__span tt-receipt__band tt-receipt__band--hold tt-receipt__y1" style="--t:0; --d:{f(HOLD_END)}"><span class="tt-receipt__note">Combat hold</span></span>
            <span class="tt-receipt__span tt-receipt__band tt-receipt__band--calm tt-receipt__y1" style="--t:{f(HOLD_END)}; --d:2.5">Calm</span>
            <span class="tt-receipt__dim" style="--t:{f(LAST_HIT)}; --d:4"></span>
            <span class="tt-receipt__at tt-receipt__note tt-receipt__y3" style="--t:{f(LAST_HIT + 2)}">4 s, no hits</span>
            <span class="tt-receipt__dim" style="--t:{f(HOLD_END)}; --d:2.5"></span>
            <span class="tt-receipt__at tt-receipt__note tt-receipt__y3" style="--t:{f(HOLD_END + 1.25)}">2.5 s</span>
      </div>

      <span class="tt-receipt__name">Slips</span>
      <div class="tt-receipt__track">
            <span class="tt-receipt__span tt-receipt__band tt-receipt__band--slip" style="--t:{f(PICKUP)}; --d:2"></span>
            <span class="tt-receipt__after" style="--t:{f(PICKUP)}; --d:2">Health Potion x1 (2 s)</span>
      </div>

      <span class="tt-receipt__name">Tickets</span>
      <div class="tt-receipt__track tt-receipt__track--tall">
        <div class="tt-receipt__stack">
            <span class="tt-receipt__span tt-receipt__band tt-receipt__band--ticket" style="--t:{f(RECEIPT)}; --d:5">Dodging, +2 ranks</span>
            <span class="tt-receipt__span tt-receipt__band tt-receipt__band--ticket" style="--t:{f(RECEIPT + 0.35)}; --d:5">Parrying</span>
        </div>
      </div>

      <span class="tt-receipt__name">Plate</span>
      <div class="tt-receipt__track">
            <span class="tt-receipt__span tt-receipt__band tt-receipt__band--plate" style="--t:{f(RECEIPT + 0.7)}; --d:4"><span class="tt-glyph tt-glyph--20" data-glyph="fire" aria-label="Pyromaniac"></span>Pyromaniac</span>
      </div>

      <span class="tt-receipt__name"></span>
      <div class="tt-receipt__track">
            <span class="tt-receipt__guidelabel tt-receipt__guidelabel--end tt-receipt__note" style="--t:{f(LAST_HIT)}">Last hit</span>
            <span class="tt-receipt__guidelabel tt-receipt__guidelabel--end tt-receipt__note" style="--t:{f(HOLD_END)}">Hold ends</span>
            <span class="tt-receipt__guidelabel tt-receipt__guidelabel--start tt-receipt__note" style="--t:{f(RECEIPT)}">Receipt prints</span>
      </div>


    </div>

    <ol class="pv-legend">
      <li>{mark("1")}<span class="num-s">1.0 s</span><span>Dodging reaches 5 / 20. Held: the badge reads +1.</span></li>
      <li>{mark("4")}<span class="num-s">6.0 s</span><span>You press F on a Health Potion: a 2 s slip prints now. Pressed pickups never wait.</span></li>
      <li>{mark("2")}<span class="num-s">3.0 s</span><span>Parrying reaches 3 / 20. +2.</span></li>
      <li>{mark("5")}<span class="num-s">7.5 s</span><span>Pyromaniac unlocks. Its plate replaces the progress ticket: still +3.</span></li>
      <li>{mark("3")}<span class="num-s">5.0 s</span><span>Pyromaniac progress 4 / 6. +3.</span></li>
      <li>{mark("6")}<span class="num-s">9.5 s</span><span>Dodging 6 / 20 merges with 1 into one ticket ("+2 ranks"): still +3.</span></li>
    </ol>
  </div>

  <div class="tt-receipt__lower">
    <div class="pv-right">
      <div class="tt-plate tt-plate--menu tt-receipt__box">
        <span class="tt-receipt__boxtitle ui-title">Print order · 350 ms apart</span>
        <p class="tt-receipt__boxtext ui-label">Slips first, then tickets, then plates one at a time. Each prints with a tick; the badge counts down.</p>
        <div class="tt-receipt__order">
          <span class="tt-receipt__stamp num-m">+0 ms</span>
          <div class="tt-tag tt-ticket tt-ticket--tall tt-ticket--skill has-foot">
            <div class="tt-ticket__row"><span class="tt-tag__kicker">Skill up</span><span class="tt-ticket__rank tt-ticket__rank--head num-m">6 / 20</span></div>
            <div class="tt-ticket__row"><span class="tt-tag__line tt-ticket__name">Dodging</span></div>
            <div class="tt-ticket__foot"><span class="tt-ticket__foottext"><span class="tt-ticket__footnum num-s">+2</span> ranks</span></div>
          </div>
          <span class="tt-receipt__stamp num-m">+350 ms</span>
          <div class="tt-tag tt-ticket tt-ticket--tall tt-ticket--skill">
            <div class="tt-ticket__row"><span class="tt-tag__kicker">Skill up</span><span class="tt-ticket__rank tt-ticket__rank--head num-m">3 / 20</span></div>
            <div class="tt-ticket__row"><span class="tt-tag__line tt-ticket__name">Parrying</span></div>
          </div>
          <span class="tt-receipt__stamp num-m">+700 ms</span>
          <div class="tt-receipt__band tt-receipt__band--plate tt-receipt__band--big"><span class="tt-glyph" data-glyph="fire" aria-label="Pyromaniac"></span>Pyromaniac plate, top centre</div>
        </div>
      </div>
      <div class="tt-plate tt-plate--menu tt-receipt__box">
        <span class="tt-receipt__boxtitle ui-title">Pause</span>
        <p class="tt-receipt__boxtext ui-label">Pausing files every waiting slip and ticket onto the spike, unread; they never print afterwards, and the badge clears. A waiting plate prints after the menu closes, once it is calm.</p>
      </div>
    </div>

    <div class="pv-right">
      <div class="tt-plate tt-plate--menu tt-receipt__box">
        <span class="tt-receipt__boxtitle ui-title">Hard cap · 20 s</span>
        <p class="tt-receipt__boxtext ui-label">A fight that never calms down: 20 s after the oldest waiting entry, the waiting slips and tickets print anyway, without remarks. Plates are never forced: they wait for calm.</p>
        <div class="tt-receipt__mini">
          <span class="tt-receipt__span tt-receipt__band tt-receipt__band--cap tt-receipt__y1" style="--t:0.4; --d:29.2"></span>
          <span class="tt-receipt__at tt-receipt__note tt-receipt__y1" style="--t:9">Combat hold, 30 s</span>
          <span class="tt-receipt__span tt-receipt__band tt-receipt__band--live tt-receipt__y2" style="--t:2; --d:20"></span>
          {mark("1", 2, " tt-receipt__y2")}
          <span class="tt-receipt__at tt-receipt__note tt-receipt__y2" style="--t:12">20 s</span>
          <span class="tt-receipt__span tt-receipt__band tt-receipt__band--ticket tt-receipt__y2" style="--t:22; --d:5">Prints</span>
        </div>
      </div>
      <div class="tt-plate tt-plate--menu tt-receipt__box">
        <span class="tt-receipt__boxtitle ui-title">Overflow · more than 4</span>
        <p class="tt-receipt__boxtext ui-label">When more than 4 pieces would hang, 3 hang and the 4th place is the stub. The spike keeps the last 12 (8 visible, the rest scroll).</p>
        <div class="tt-receipt__more">
          <div class="tt-rail__more">
            <span class="tt-glyph" data-glyph="pin" aria-label="Spike"></span>
            <span><span class="tt-rail__morenum num-s">6</span> more on the spike</span>
          </div>
        </div>
      </div>
    </div>
  </div>

  <div class="tt-plate tt-plate--menu tt-receipt__density">
    <div class="tt-receipt__dgrid">
      <span class="tt-receipt__dhead tt-receipt__boxtitle ui-title">Tally notifications</span>
      <span class="tt-receipt__dhead"><span class="tt-receipt__dtab tt-selectable is-selected tt-focus--border is-focused">Full</span></span>
      <span class="tt-receipt__dhead"><span class="tt-receipt__dtab tt-selectable">Calm</span></span>
      <span class="tt-receipt__dhead"><span class="tt-receipt__dtab tt-selectable">Minimal</span></span>

      <span class="tt-receipt__drow">Pickup slips</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>Always</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>Out of combat</span>
      <span class="tt-receipt__no"><span class="tt-glyph" data-glyph="cross" aria-label="No"></span>Spike only</span>

      <span class="tt-receipt__drow">Tickets and the +N badge</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>Receipt</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>Receipt</span>
      <span class="tt-receipt__no"><span class="tt-glyph" data-glyph="cross" aria-label="No"></span>Spike only</span>

      <span class="tt-receipt__drow">Plates</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>4 s / 5.5 s</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>4 s / 5.5 s</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>4 s / 5.5 s</span>

      <span class="tt-receipt__drow">Tally remarks</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>If remarks are On</span>
      <span class="tt-receipt__no"><span class="tt-glyph" data-glyph="cross" aria-label="No"></span>Off</span>
      <span class="tt-receipt__no"><span class="tt-glyph" data-glyph="cross" aria-label="No"></span>Off</span>

      <span class="tt-receipt__drow">Spike (12 kept, 8 visible)</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>Everything</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>Everything</span>
      <span class="tt-receipt__yes"><span class="tt-glyph" data-glyph="check" aria-label="Yes"></span>Everything</span>
    </div>
  </div>

</div>
</div>
</body>
</html>
"""

OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(page, encoding="utf-8")
print("wrote", OUT)
