/* Tin Tally helpers: draws glyphs, tally strokes and rarity badges into [data-glyph], [data-tally] and [data-rarity] elements. */
window.TinTally = (function () {
  var GLYPHS = __GLYPHS__;
  var NS = 'http://www.w3.org/2000/svg';

  function glyph(name, title) {
    var body = GLYPHS[name];
    if (!body) body = GLYPHS.unknown || '';
    return '<svg xmlns="' + NS + '" viewBox="0 0 24 24" fill="currentColor" role="img" aria-label="' + (title || name) + '">' + body + '</svg>';
  }

  /* Tally strokes: groups of four upright strokes closed by a slash. Lit strokes use currentColor
     (text on plate, ink on enamel: never good). Slashes land on 5, 10, 15, 20 (skill milestones);
     rank 1 gets a cap dot. Unlit strokes are rows of dots (.tt-tally-empty, lip; CSS may
     recolour them, e.g. ink-muted on enamel), so lit vs unlit differs by shape, not colour alone:
     - an unlit upright is a thinner column of dots fitted end to end; the dot spacing scales with the
       pitch (much tighter down a stroke than across strokes), so columns never form a dot grid;
     - an unlit slash is three dots on the slash line: one past each end of the bundle and one in the
       middle gap, sized to clear the uprights, so its dots never land on an upright (lit or unlit). */
  function r2(v) { return Math.round(v * 100) / 100; }
  function tally(n, max, opts) {
    opts = opts || {};
    max = Math.max(1, max || n || 1);
    var h = opts.height || 20, sw = opts.stroke || 3, pitch = opts.pitch || 6, gap = opts.gap || 8;
    var bundleW = pitch * 3 + sw * 2 + 4;
    var bundles = Math.ceil(max / 5);
    var w = bundles * bundleW + (bundles - 1) * gap;
    var EMPTY = ' class="tt-tally-empty" fill="var(--lip)"';
    /* Unlit upright: a beaded line of dots 0.7 x the stroke width, fitted from cap to cap, spaced
       about 0.55 x the pitch, so columns read as dotted lines, never as a dot grid. */
    var dotR = Math.min(sw, pitch / 2) * 0.35;
    var len = h - 2 * sw;
    var target = Math.max(dotR * 2 + 1.4, pitch * 0.55);
    var segs = Math.max(1, Math.round(len / target)), step = len / segs;
    /* Unlit slash: dots clear each upright by at least 0.5 px. */
    var slashR = Math.max(0.75, Math.min(dotR * 1.2, pitch / 2 - sw / 2 - 0.5));
    var out = [];
    for (var i = 0; i < max; i++) {
      var b = Math.floor(i / 5), p = i % 5, x0 = b * (bundleW + gap) + sw;
      var on = i < n, k;
      if (p < 4) {
        var x = x0 + p * pitch + 2;
        if (on) {
          out.push('<line x1="' + x + '" y1="' + sw + '" x2="' + x + '" y2="' + (h - sw) + '" stroke="currentColor" stroke-width="' + sw + '" stroke-linecap="round"/>');
          if (i === 0 && opts.capFirst) out.push('<circle cx="' + x + '" cy="' + sw + '" r="' + (sw * 0.9) + '" fill="currentColor"/>');
        } else {
          for (k = 0; k <= segs; k++) out.push('<circle cx="' + x + '" cy="' + r2(sw + k * step) + '" r="' + r2(dotR) + '"' + EMPTY + '/>');
        }
      } else {
        var sx1 = x0 - 1, sy1 = h - sw - 1, sx2 = x0 + bundleW - sw - 1, sy2 = sw + 1;
        if (on) {
          out.push('<line x1="' + sx1 + '" y1="' + sy1 + '" x2="' + sx2 + '" y2="' + sy2 + '" stroke="currentColor" stroke-width="' + (sw + 1) + '" stroke-linecap="round"/>');
        } else {
          for (k = 0; k < 5; k += 2) {
            var dx = Math.min(sx2, Math.max(sx1, x0 + 2 + (k - 0.5) * pitch));
            var dy = sy1 + (dx - sx1) / (sx2 - sx1) * (sy2 - sy1);
            out.push('<circle cx="' + r2(dx) + '" cy="' + r2(dy) + '" r="' + r2(slashR) + '"' + EMPTY + '/>');
          }
        }
      }
    }
    return '<svg xmlns="' + NS + '" width="' + w + '" height="' + h + '" viewBox="0 0 ' + w + ' ' + h + '" role="img" aria-label="' + n + ' of ' + max + '">' + out.join('') + '</svg>';
  }

  var RARITY = { common: 0, green: 1, blue: 2, purple: 3, red: 4, gold: 5 };
  /* Rarity badge: Common = an empty ring, Green..Red = 1-4 strokes, Gold = the full bundle. */
  function rarity(r, size) {
    var n = RARITY[r] || 0, h = size || 20;
    if (n === 0) {
      return '<svg xmlns="' + NS + '" width="' + h + '" height="' + h + '" viewBox="0 0 20 20" role="img" aria-label="Common"><circle cx="10" cy="10" r="6.5" fill="none" stroke="currentColor" stroke-width="3"/></svg>';
    }
    var label = r.charAt(0).toUpperCase() + r.slice(1);
    if (n === 5) return tally(5, 5, { height: h }).replace('aria-label="5 of 5"', 'aria-label="' + label + '"');
    var sw = 3, pitch = 6, w = sw * 2 + (n - 1) * pitch + 2, out = [];
    for (var i = 0; i < n; i++) {
      var x = sw + 1 + i * pitch;
      out.push('<line x1="' + x + '" y1="' + sw + '" x2="' + x + '" y2="' + (h - sw) + '" stroke="currentColor" stroke-width="' + sw + '" stroke-linecap="round"/>');
    }
    return '<svg xmlns="' + NS + '" width="' + w + '" height="' + h + '" viewBox="0 0 ' + w + ' ' + h + '" role="img" aria-label="' + label + '">' + out.join('') + '</svg>';
  }

  function mount(root) {
    root = root || document;
    var els = root.querySelectorAll('[data-glyph]');
    for (var i = 0; i < els.length; i++) els[i].innerHTML = glyph(els[i].getAttribute('data-glyph'), els[i].getAttribute('aria-label'));
    els = root.querySelectorAll('[data-tally]');
    for (i = 0; i < els.length; i++) {
      var e = els[i];
      e.innerHTML = tally(+e.getAttribute('data-tally'), +(e.getAttribute('data-max') || e.getAttribute('data-tally')), {
        height: +(e.getAttribute('data-height') || 20), capFirst: e.hasAttribute('data-cap'),
        stroke: +(e.getAttribute('data-stroke') || 3), pitch: +(e.getAttribute('data-pitch') || 6)
      });
    }
    els = root.querySelectorAll('[data-rarity]');
    for (i = 0; i < els.length; i++) els[i].innerHTML = rarity(els[i].getAttribute('data-rarity'), +(els[i].getAttribute('data-height') || 20));
  }

  if (typeof document !== 'undefined') {
    if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', function () { mount(document); });
    else mount(document);
  }
  return { glyph: glyph, tally: tally, rarity: rarity, mount: mount, glyphs: GLYPHS, version: '1.1.0' };
})();
