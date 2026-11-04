# FeatsTab

The Feats tab of the pause menu: the shipping summary, earned feats as medallions whose silhouette tells the tier, locked feats with per-condition progress bars, and a detail panel with the Drawback box. Use it as the content of the `feats` tab inside the PauseShell's 948 x 540 content box.

## Anatomy
Positions are design px at 1920 x 1080 (content box at x 486-1434, y 298-838). States not on screen are in the spec band under the screen.

| Part | Box | Build |
|---|---|---|
| Header | 948 x 44 at (486, 298) | "Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary" in `ui-title` `text`, one TextBlock. |
| Section heads | 598 x 26 at y 356 ("Unlocked") and y 564 ("Locked") | `ui-label` caps `text-muted`, tracked 0.06em, the count in `num-s` `text`, then a core `.tt-divider` filling the rest. |
| Unlocked board | 598 wide from y 390; cells 194 x 76, gap 8, three to a row, every row centred | A medal board: rows fill left to right and each row is centred, so 5 feats hang 3 + 2 (row 1 x 486 / 688 / 890; row 2 x 587 / 789, y 474) and a short last row never leaves an empty socket. Cell: `plate`, 2 px `lip`, radius 10, padding 0 6 0 8; medallion 48 + 10 gap + name (`ui-label` `text` on 22 px lines, wraps at 118 px, two lines at most: 2 x 22 + the 22 px tier line = 66 in the 72 px face; "Steel Rhythm" and "Living Furnace" take two) over the tier word (`ui-label` in `feat-common` / `feat-rare` / `feat-legendary`). |
| Medallion | 48 (cells, locked rows), 88 (detail), the same textures on TallyPlate | Silhouette per tier (D14): circle Common, FLAT-TOP hexagon Rare (flat edges top and bottom, points left and right), 8-point starburst Legendary. `ink` outer silhouette, the tier fill inset 3 / 4 / 5 px (4 / 5 / 7 px at 88), and the feat's OWN glyph in `ink` everywhere (Pyromaniac = `fire`; 24, 20 in a 48 starburst so its points stay clear; 48 at 88). |
| New marker | 16-20 px, the cell's top-right corner | TallyRail's peeled corner (`.tt-ticket__peel`: an `ink` void, an `enamel-shade` flap with 2 px `ink` legs), the one "new" mark of the system. The fold is the signal. |
| Locked row | 598 x 70 at y 598 / 676 / 754 | `plate`, 2 px dashed `lip` (dashed = locked, D24), radius 10, padding 6 12 6 10, column gap 10; columns 48 (hollow medallion) / 138 (name, then tier word + "???" 6 px apart) / conditions (366). |
| Condition row | 26 tall, column gap 8 | Label `ui-label` `text-muted` (ellipsis past its column, 169 px beside a bar); right: a thin bar (core `.tt-bar--thin`, 80 x 10, `rim` fill on `well`, 2 px `lip`) and the count `num-s` `text` right-aligned in a 101 px box ("88 / 200"). No tally strokes (D1). Tag condition: "Requires: {Label}" with `check` + "Met" in `good`, or `cross` + "Not met" in `text-muted` (red means threat only). |
| Detail panel | 330 x 482 at (1104, 356) | `plate-raised`, 2 px `lip`, radius 10, padding 14 16 16, gap 10: medallion 88 + tier pill (`ui-label` 700, `ink` on the tier colour, 30 tall) + "Unlocked" with `check`; name `tally-heading`; description `ui-body`; counts `ui-label` `text-muted`; a core `.tt-divider`, "Earned by" and its conditions (bar 64, count box 76); the Drawback box. |
| Drawback box | 294 wide | Core `.tt-drawback` (D14): the `.tt-hatch` strip (`danger` / `ink`) in a 2 px `ink` box, then "Drawback" inline (`danger-text`, 800) and the cost in `text`. On the `plate-raised` panel it takes the `plate` ground so it reads as a box. |

Contrast: tier words are text in the tier colour on `plate` (Common 10.0:1, Rare 7.2:1, Legendary 5.62:1) or `plate-raised` (Legendary 4.88:1, the lowest pair, still above 4.5:1, so plate-raised is allowed). Coloured fills (the tier pill, medallions) carry `ink`: Legendary pill 7.9:1. On the cream selected cell the tier word turns `ink-muted`: never a feat colour on cream.

## States
- **Unlocked cell:** idle; selected (drives the detail panel): the cream `enamel` fill with a 2 px `ink` edge (core `.is-selected`, D6), name `ink`, tier word `ink-muted`; focused: core focus with `.tt-focus--fill-gap` on top (the cell has no outline of its own); new: the peeled corner until the cell has been focused once.
- **Locked row:** hollow medallion (`lip` silhouette around `plate`, `unknown` glyph in `text-disabled`), name and tier word in `text-muted`, description "???", one condition row per unlock condition. A live unlock moves the feat from Locked to Unlocked at once and gives it the peeled corner.
- **Detail:** an unlocked Common feat has no Drawback box; Rare and Legendary always have one. A locked feat in the detail shows "???" for the description and its condition rows, no counts.
- **Empty:** the header still reads "Feats: 0 unlocked — 0 Common, 0 Rare, 0 Legendary"; the empty board is the one empty state (core `.tt-empty`, D8): "No feats unlocked yet." with the second line "Feats are earned, not picked." (proposed copy, spec band). The Locked list still shows what can be earned.
- **Overflow:** more than 6 unlocked feats add board rows; the left column becomes a ScrollBox and the detail panel stays put.

## Data and implementation
- Widget: `UMordecaiFeatDisplayWidget` (API only, not registered: N5). Register it for `feats` with `RegisterTab`; US-093 builds the tree (AC-093.2, AC-093.5). Rebind in `NativeOnActivated`.
- Data: unlocked rows (name, tier, description, applied record); locked rows (name, tier, "???", condition progress from `FMordecaiFeatConditionProgress`: `ConditionType` StatThreshold / EventCount / TagPresent / SkillRank, `StatName`, `RequiredTag`, `CurrentValue`, `ThresholdValue`, `bMet`); the summary `TotalUnlocked` and the Common / Rare / Legendary counts. Feats come from `UMordecaiFeatDataAsset` (FeatName, DisplayName, Description, Tier, GrantedEffects, GrantedAbilities, DrawbackEffects, FeatTag `Mordecai.Feat.<FeatName>`, UnlockConditions).
- Feat text fields, proposed once on `UMordecaiFeatDataAsset` for TallyPlate, TallyRail and this tab (D14). TODO(DECISION), all three:
  - `CauseText`: the plate's cause line ("Burn 6 enemies at once.").
  - `DrawbackText`: the Drawback box's cost. Until it exists the box shows the count only ("1 Drawback"), never invented text.
  - A `DisplayLabel` per unlock condition: the condition rows here and the FEAT PROGRESS ticket's footer. Until it exists the label prints the raw name, as US-068 formats it today ("TotalPerfectParries", "Requires: Mordecai.Feat.Pyromaniac"). The preview shows proposed labels ("Perfect parries", "Requires: Pyromaniac"), labelled example.
- Live unlock: listen for `Mordecai.Event.FeatUnlocked` and move the row.
- Unlocked board in UMG: a `UWrapBox` with `HorizontalAlignment` Center (each line centred), `InnerSlotPadding` (8, 8), explicit `WrapSize` 598 (three cells), slots 194 x 76 (a `USizeBox` per cell). The order follows the widget's unlocked rows.
- `TierColors` (EditDefaultsOnly): replace Gray / Blue / Gold with `feat-common`, `feat-rare`, `feat-legendary` (sRGB, convert with `FLinearColor::FromSRGBColor`), so "Gold" never means both a Legendary feat and a Gold item.
- Condition formats (US-068): StatThreshold / EventCount: the label, the bar (Current / Threshold, clamped 0..1) and "Current / Threshold"; SkillRank: "{Skill} Rank" and "x / y"; TagPresent: "Requires: {Tag}" with check or cross. The display checks a tag condition against the player's granted feat tags (`HasGrantedFeatTag`), so the preview's tag conditions name feats: "Requires: Pyromaniac" is met (unlocked), "Requires: Iron Will" is not (locked). The bar is a `UProgressBar` (FillType LeftToRight, `rim` fill, `well` background, `lip` 2 px edge).
- Counts line: "{n} Effect(s), {n} Abilit(y/ies), {n} Drawback(s)" from GrantedEffects, GrantedAbilities, DrawbackEffects.
- New marker: TODO(DECISION): needs a per-feat "seen" flag (not in code); until then, mark feats unlocked this session.
- "Earned by" in the detail is optional: it lists the feat's UnlockConditions, all met.
- Proposed `BindWidgetOptional` names: `SummaryText`, `UnlockedGrid`, `LockedList`, `DetailBadge`, `DetailTierText`, `DetailNameText`, `DetailDescText`, `DetailCountsText`, `DrawbackBox`, `DrawbackText`. Keep `SummaryText` a real TextBlock (playtests look for "Feats: ").
- Medallions in UE: 03's Badges (96 px silhouettes: circle, flat-top hexagon, starburst), an `ink` layer and a tint layer, the feat's glyph on top. The hatch strip is the shared `M_UI_PatternFill` / `T_UI_Hatch16` pattern.
- Content: only Pyromaniac (Common, "Burn 6 enemies at once", glyph `fire`) is real. Every other feat is an EXAMPLE. The board reuses the brand book's recurring example feats (01-voice-and-copy), so this tab and the feat plates show the same player:
  - Steel Rhythm, the Rare: glyph `block`, earned by 50 perfect parries, drawback "Blocking costs 25% more Stamina." (TallyPlate's).
  - Living Furnace, the Legendary, selected in the detail: glyph `fireball`, earned by burning 6 enemies at once 10 times ("Burns of 6", 10 / 10), drawback "Frost damage taken +50%." (TallyPlate's). Its effect line "Fire spells cost 1 less Spell Point." is example text.
  - Unflinching, a Common: glyph `resolved`.
  - Fillers, also examples: Packrat (Common, `chest`) on the board, and the locked Iron Will, Fireproof and Untouchable with their conditions and labels. Iron Will's "Perfect parries 64 / 100" stands past Steel Rhythm's 50; "Parrying rank 3" matches the spike's Parrying 3 / 20.
  - No feat content assets exist.
- UMG sizes: `tally-heading` 24 pt, `ui-title` 19.5 pt, `ui-body` 16.5 pt, `ui-label` 15 pt, `num-s` 15 pt.
- Tokens: `plate`, `plate-raised`, `well`, `lip`, `rim`, `ink`, `ink-muted`, `enamel`, `enamel-shade`, `text`, `text-muted`, `text-disabled`, `feat-common`, `feat-rare`, `feat-legendary`, `good`, `danger`, `danger-text`, `focus`.

## Copy
- Header, exact format: "Feats: N unlocked — a Common, b Rare, c Legendary" (em dash).
- Sections: "Unlocked", "Locked" (ToUpper at display) + count.
- Tier words: "Common", "Rare", "Legendary". Locked description: "???".
- "Unlocked", "Earned by", "Drawback", "Met", "Not met", "Requires: {Label}".
- Empty (proposed): "No feats unlocked yet." / "Feats are earned, not picked."
- No Tally remark on this screen; the feat plate carries it.

## Do / Don't
- Do let the silhouette and the tier word carry the tier; the colour only repeats them.
- Do give every medallion the feat's own glyph, here, on TallyPlate and on the spike.
- Do centre every board row; a left-packed short row reads as a missing feat.
- Do show the Drawback box for every Rare and Legendary feat, even when its text is missing.
- Do use core `.tt-drawback`; there is one Drawback box in the system.
- Don't reveal a locked feat's description; its name, tier and conditions are the tease.
- Don't draw condition progress as tally strokes; strokes are ranks, rarity and the seal only.
- Don't reuse item rarity colours or rarity badges for feats.
- Don't put text on the hatch strip.
