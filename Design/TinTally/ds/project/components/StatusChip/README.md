# StatusChip

One active status on the player: its glyph, its name and its time left, on a buff or debuff chip. Use them in the strip under the spell slots; the status VFX stay the primary channel ("Status is primarily VFX-driven, not icon soup"), and the strip is the minimal readout.

## Anatomy

- **Body:** 36 px tall, 3 px `ink` outline (4 px in High Contrast). Padding 0 8 0 6, 6 px gaps. Contents: glyph 24 | `StatusNameText` (`ui-label` 20/26, 600) | stack pips | `DurationText` (`num-s`).
- **Buff** (light): `enamel` fill, `ink` text and glyph, 2 px `buff` rim, radius 10. A raised tab sits on top: 30 x 10, `buff` fill with a 3 px `ink` border (4 px in HC) on its top and sides, 4 px from the left. It rises 10 px above the body, 7 px past the outline.
- **Debuff** (dark): `plate` fill, `text`, 2 px `debuff` rim, radius 10 10 3 3. A pinked hem (pinking-shear teeth) runs along the bottom, 4 px in from each side. It is two sawtooth strips at a 10 px pitch:
  - `debuff` teeth hang 4 px below the body;
  - `ink` teeth hang 7 px, showing 3 px proud of them (the dual edge).
- **Glyph tint** confirms the family; the shape and the name carry it:
  - Debuffs:
    - Burning `dmg-fire`, Bleeding `vital`, Poisoned `dmg-poison`
    - Frostbitten, Frozen, Brittle and Drenched `dmg-frost`
    - Shocked `dmg-lightning`, Corroded `dmg-corrosive`
    - Cursed and Fear `dmg-shadow-edge`, Exposed `danger-text`
    - Weakened, Silenced, Rooted, Blinded, Sleeping `text`
  - Buff glyphs are `ink`.
- **Duration box:** sized once from the status's full duration: 3 Mono cells (38 px) under 10 s, 4 cells (51 px) from 10 s. Left-aligned, so a chip never changes width while it ticks.
- **Stack pips** (D1: core `.tt-pips`, never tally strokes): one 14 px pip per possible stack, 6 px apart. A stack reached is a filled disc in the status colour (Frostbitten `dmg-frost`, Shocked `dmg-lightning`); a stack still ahead is a hollow 2 px `lip` ring, so reached vs ahead reads by shape even in High Contrast, where `lip` is white.
  - Frostbitten shows 4 pips (74 px): the 5th application turns it into Frozen.
  - Shocked shows 3 (54 px).

## The strip

- Anchor top-left, offset (32, 318), 456 x 112.
  - 12 px headroom for buff tabs.
  - Two 36 px rows 20 px apart (chip tops at y 330 and 386).
  - 8 px below the last row for the teeth.
- Chips sit 10 px apart. At most 8 (`MaxVisibleIndicators`), in 2 rows.
- **Sort:** Control > DoT > Debuff > Environmental > Buff. Within a category, chips keep the order they were applied, so they never reshuffle while ticking.
  - When a 9th status arrives, the last chip in that order is not shown.
- **Full or compact.** Chips print full (glyph, name, time) while they all fit in two rows. If they would need a third row, every chip drops its name at once: glyph, pips and time only. It is one switch for the whole strip; full and compact chips never mix.
- **Overflow "+N".** Stack pips make some compact chips wide (a compact Frostbitten is 166 px, a plain one 86 px), so 8 compact chips can still need a third row. Then the last chips in sort order give way to one "+N" chip (`plate`, 2 px `lip` rim, no tab, no teeth, `num-s` "+2"). Buffs sort last, so they are the first to fold.
- Collapsed when no status is active.

## States

| State | Look |
|---|---|
| Timed | "4.2" counts down at 10 Hz ("%.1f") |
| Stacking | Frostbitten 1-4 fills a pip per stack. The 5th application removes Frostbitten and adds Frozen 2.0; the chip swaps in place. Shocked 1-3; a perfect block empties one pip |
| Focused | No timer: `DurationText` reads "3/3", the perfect-action streak (FocusedStreakThreshold 3). It lasts until the player is hit |
| Exposed | "1 hit" in the UI face instead of a timer (it is consumed by the next hit, or expires at 5.0 s) |
| Arrives | 120 ms drop (Y −8 to 0) |
| Leaves | 100 ms fade |
| Compact | Names collapse (see The strip) |
| Overflow | "+N" replaces the last chips in sort order while compact chips still need a third row |
| Hover / focus (pause menu only) | hangs its StatusTooltip (glyph, name, tagline, time, pips, counterplay). The HUD chips are never focusable in play |

No chip blinks or flashes as it expires: the number tells it. **Reduce Motion:** the drop becomes a 150 ms fade.

## Data and implementation

- **Bar:** UMordecaiStatusEffectBarWidget, slot `StatusEffectBar`, moves from bottom-left (20, −60) to the box above. It is a child of the HUD root canvas (D20: the root is pushed once on `UI.Layer.Game`), never pushed on its own.
  - Its `IndicatorContainer` is a UHorizontalBox today. Make it a `UWrapBox` (wrap width 456, slot padding 10 / 20), and change the UPROPERTY to `UPanelWidget*` so either type binds.
- **Chip:** UMordecaiStatusEffectIndicatorWidget, one per status. Keep `StatusNameText`, `DurationText` and `TintImage`.
  - `TintImage` becomes the chip body: a 9-slice, pre-coloured brush, `T_UI_ChipBuff` or `T_UI_ChipDebuff` (tab and teeth baked in).
  - New optional widgets: `StatusGlyph` (UImage from `StatusIconMap`) and `StackPips` (UHorizontalBox of 14 px pip Images, filled or hollow brush). Plus `OverflowChip` ("+%d") on the bar.
- **Fix N16** (US-091):
  - Register one indicator per child tag of `Mordecai.Status`, not on the parent.
  - Tick `DurationText` from the GE's remaining time; today it is set once.
- **Name:** `StatusNameText` returns the tag's last segment ("StoneSkin") until US-091 changes it together with the playtests. The playtests compare "StoneSkin" and "Burning". The mockups show display names ("Stone Skin").
- **Polarity:**
  - `BuffTags` today: Focused, Blessed, StoneSkin, FireWard, Restoration.
  - Add EnchantedWeapon, Blurred and Resolved; they show as debuffs today.
- **Category:** from `Mordecai.Status.Category.<DoT|Debuff|Control|Buff|Environmental>` (MordecaiStatusEffectTypes.cpp). Unknown tags count as Debuff.
- **Stacks:** the GE stack count. Frostbitten max 5 (MordecaiGE_Frostbitten), Shocked max 3.
- **Compact switch:** after each rebuild, if the WrapBox's desired height is more than two rows, set every `StatusNameText` Collapsed.
  - The bridge reads only visible text. `TODO(DECISION)`: either playtests that compare status names keep the count at 4 or fewer, or the bridge reads the chip's accessible name.
- **Durations** (code defaults):

  | Status | Duration |
  |---|---|
  | Burning | 5.0 |
  | Bleeding | 6.0 |
  | Poisoned | 8.0 |
  | Frostbitten | 6.0 |
  | Shocked | 5.0 |
  | Weakened | 8.0 |
  | Brittle | 7.0 |
  | Silenced | 5.0 |
  | Rooted | 3.0 |
  | Blinded | 4.0 |
  | Fear | 5.0 |
  | Cursed | 10.0 |
  | Exposed | 5.0 or 1 hit |
  | Corroded | 10.0 |
  | Drenched | 15.0 |
  | Frozen | 2.0 |
  | Stone Skin | 10.0 |
  | Restoration | 5.0 (heal over time, 1.0 s ticks) |

- `TODO(DECISION)`:
  - Stacking policy (Open Item 2, `EMordecaiStackingPolicy`).
  - Whether buildup meters exist (the specs describe buildup; the code applies statuses instantly).
  - Whether Exposed shows its 5.0 s timer as well as "1 hit".
  - A "Focused 2/3" building chip before the buff exists.
- **Tokens:** `plate`, `enamel`, `ink`, `text`, `lip`, `buff`, `debuff`, `vital`, `danger-text`, `dmg-fire`, `dmg-frost`, `dmg-lightning`, `dmg-poison`, `dmg-corrosive`, `dmg-shadow-edge`.

## Copy

- **Names** (display):
  - Debuffs: Burning, Bleeding, Poisoned, Frostbitten, Frozen, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched, Sleeping.
  - Buffs: Focused, Stone Skin, Fire Ward, Blessed, Restoration, Enchanted Weapon, Blurred, Resolved.
- **Time:** "%.1f" ("4.2", "14.0"), "1 hit", "3/3".
- Overflow: "+N".
- **Taglines** go in the StatusTooltip (and a codex, `TODO(DECISION)`), never on the HUD chip. Use them verbatim:

  | Status | Tagline |
  |---|---|
  | Burning | "You're on a clock" |
  | Bleeding | "Recovery is denied" |
  | Poisoned | "You can't catch your breath" |
  | Frostbitten | "Sluggish body, slower reactions" |
  | Shocked | "Timing disruption" |
  | Weakened | "You can't break them" |
  | Brittle | "You break easily" |
  | Silenced | "Magic is shut off" |
  | Rooted | "Position solved or punished" |
  | Blinded | "Reads become unreliable" |
  | Fear | "Composure collapse" |
  | Cursed | "Resource rules broken" |
  | Exposed | "Next mistake hurts more" |
  | Corroded | "Gear advantage denied" |
  | Drenched | "Elemental rules shift" |
  | Focused | "Mastery streak reward" |

- World voice: no Tally remarks and no playful ornament on status lines.

## Do / Don't

- Do let the shape carry polarity: tab on top for a buff, teeth below for a debuff. Colour only confirms.
- Do size the time box from the full duration so chips never jitter.
- Don't show more than 8 chips, and never open a third row: fold the tail into "+N".
- Don't draw stacks as tally strokes: they are pips.
- Don't mix full and compact chips in one strip.
- Don't flash, blink or pulse an expiring chip.
- Don't put taglines or remarks on the HUD.
