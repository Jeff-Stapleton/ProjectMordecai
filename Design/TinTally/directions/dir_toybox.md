## name
Punchboard

## thesis
The whole interface is the inside of a board-game box set on the table next to the diorama: die-cut cardboard tokens, glossy plastic trays and pegs, felt-lined inserts and tab dividers. All of it is lit from above and casts the same soft contact shadow as the tilt-shift world. Link's Awakening (Switch) comes in as material and light: toy-plastic gloss, felt fuzz, card stock and that resting-on-the-table weight, with no Nintendo art, hearts, rupees or fonts. Dungeon Crawler Carl comes in only at the prize layer. The Tally is an Automaton-Age record engine that never stopped counting. Whenever you earn something it slaps a loud magenta sticker, enamel pin or scratch-off card onto your board with one wry line. The world, the HUD and the menus stay quiet, terse and serious.

## voice
TWO VOICES, never mixed in one widget.
(1) WORLD VOICE (default: HUD, menus, prompts, death, loading). Terse, second person, plain verbs, no jokes. Modelled on the status taglines ("You're on a clock", "Next mistake hurts more"). Labels are nouns and title case. Sentences end with a full stop. Numbers are numerals.
(2) THE TALLY (DCC layer, toasts and identify reveals only). Proposed diegetic frame, TODO(DECISION) for Jeff: an Automaton-Age tally engine, kin to the Continuants, still logging deeds a thousand years after anyone asked. That makes it a small comic echo of the Control theme.
- Each toast has a DISPLAY-CAPS headline of 1-2 words, then the item/skill/feat name, then at most one wry line of 70 characters or fewer.
- Bureaucratic enthusiasm, dry, never cruel. It never comments on death and never mentions viewers, sponsors, shows, floors or loot boxes. It never breaks lore and never explains mechanics badly; the mechanical fact always comes first.
- Throttle: one quip per 30 s, then headlines only. Accessibility option "Tally commentary: Full / Headlines only / Off".
SHIPPING STRINGS UNCHANGED (the playtest bridge compares them): "Paused"; "Character" / "Skills" / "Feats" / "Inventory" / "Settings"; "Coming Soon"; "You Died"; "Kills: N"; "Hit N"; "Perfect Dodge!"; "Perfect Block!"; "Parried!"; "Parry Failed!"; "Posture Broken!"; "[F] Pick up {0}"; "[I] Inventory"; "Identify"; "Stored Resources"; "Inventory is empty."; "No items match this filter."; "No resources stored."; "x / y" SP; "PROJECT MORDECAI"; "Loading..."; filter labels; "? " unidentified prefix; "  [equipped]". New copy goes only in NEW TextBlocks.
SAMPLES:
1. Feat unlocked toast: "FEAT UNLOCKED" / "Pyromaniac · Common" / "Six enemies on fire at once. Filed under 'enthusiasm'."
2. Skill rank-up toast: "RANK UP" / "Longswords  Rank 4 / 20" / "Logged. Your sword arm is now slightly less of a rumour."
3. Skill milestone toast: "MILESTONE" / "Dodging  Rank 5" / "Perfect dodges now grant a speed burst. Recorded: you are harder to hit."
4. Identify reveal: "IDENTIFIED" / "Mysterious Amulet" / "Purple, as advertised."
5. Level up: "LEVEL 7" / "+1 skill point" / "Spend it before it starts to feel permanent."
6. Death screen line (world voice, new subline under "You Died" / countdown / "Kills: N"): "Nothing stays. Not even this."
7. Low-stamina hint (world voice, chip beside the stamina gauge on entering Exhausted, max once per 60 s): "Exhausted. Everything lands at half strength until you catch your breath."
8. Main menu tagline (world voice, from pillar 2): "Everything you do is a skill."

## palette
- ink: #1B1924 / HC #000000 — 3px outline on every HUD plate and world label (the light-floor guarantee). Text on card: 15.14:1, HC 21:1. Damage-numeral stroke. 2px focus halo on card grounds. Against light floors: arena grey 11.93, grass 7.95, sand 11.93, white 15.49, mid shade 5.16.
- tray: #272338 / HC #000000 — Glossy dark plastic insert tray: HUD resource tray, spell rack, kill counter, combat-feedback plate, enemy nameplates, boss bar, loading-screen ground. Grounds card text (13.25:1) and text-dim (8.27:1). Against floors: worst case 4.51 on mid shade #8C8C8C, 13.56 on white.
- tray-well: #121019 / HC #2A2A2A — Recessed bar channels and empty SP sockets inside the tray. Every resource fill is at least 5.01:1 against it (HC at least 4.80). Never carries an edge alone: see well-lip.
- well-lip: #7A7390 / HC #9A9A9A — 2px lip around every bar well, empty SP bead rings, dashed outline of locked feat slots and Summon plates. 3.38:1 against tray, 4.20:1 against well (HC 7.46 / 5.10). It fixes the only computed failure: well against tray was 1.24:1.
- card: #F6EFDF / HC #FFFFFF — Card-stock faces: menu cards, item rows, tooltips, dialogs, pickup luggage tag, buff chips, active tab divider. Also the primary text colour on dark grounds: tray 13.25, felt 8.71, felt-recess 10.89, ink 15.14. On light floors only 1.02-2.94, so a card face in the world or HUD ALWAYS has the ink line.
- card-edge: #B39C7C / HC #000000 — The 4px die-cut chipboard thickness under every cardboard token (offset 0,+4). Decorative depth only; never carries meaning, always sits behind the ink line.
- text-dim: #C3BCD2 / HC #E6E6E6 — Secondary text on dark grounds: weapon type line, Effective Mod '+9.4%', inactive tab labels, '[I] Inventory' chip. Tray 8.27, felt 5.44, ink 9.45 (HC 16.83). Disabled controls use this plus a lock glyph, never a lower-contrast grey.
- text-dim-card: #585166 / HC #262626 — Secondary text on card: item suffixes 'x3' and '[equipped]', descriptions, type lines. 6.58:1 (HC 15.13).
- felt: #1D4A3F / HC #000000 — Green baize lining of the pause-menu insert tray (TabContentBorder), the front-end Options tray, map table. Card text 8.71, text-dim 5.44, focus ring 6.45. Rarity-coloured text is NOT allowed on felt (Blue/Purple/Red light variants fail at 4.09-4.48).
- felt-recess: #163A31 / HC #000000 — Compartments sunk into the felt: inventory list well, Stored Resources ledger, inactive tab dividers, locked sticker slots. Card text 10.89:1.
- focus: #FFC83A / HC #FFE500 — Glossy marigold 4px focus ring: felt 6.45, tray 9.81, ink 11.21. On card, a 2px ink halo sits outside it (15.14). Also positive combat-feedback text on the ink plate (11.21) and the crit starburst.
- tally: #F55FB8 / HC #FF8FD6 — The Tally's prize-sticker ink: toast sticker faces, peeled-corner marker on new feats, identify reveal sticker. Ink text on it 5.95:1 (HC 10.15). Achievement layer only; never used in combat HUD or world copy.
- brass: #D9A441 / HC #FFD34D — Enamel-pin rims, enemy tier pegs, skill milestone eyelets (ranks 1/5/10/15/20), boss phase pegs, menu tacks. 6.75:1 against tray; ink on brass 7.71 (HC 14.68).
- health: #F0405A / HC #FF5C70 — Cherry gel fill for player and enemy health (one hue means health everywhere). 5.01:1 against well (HC 4.80). Never grounds text.
- stamina-green: #5BD06A / HC #4CFF6A — Stamina fill in the Green tier (66% and above, 1.25x) with 3 gauge pegs lit. 9.58:1 against well. Ink on it 8.81.
- stamina-yellow: #F5C83A / HC #FFD21F — Stamina fill in the Yellow tier (33% and above, 1.0x) with 2 pegs lit. 11.86:1 against well (HC 9.90). Ink on it 10.90 (HC 14.49). The HC value is kept distinct from the HC focus yellow.
- stamina-red: #FF6A3D / HC #FF7A45 — Stamina fill in the Red tier (above 0, 0.75x) with 1 peg lit. 6.63:1 against well. Ink on it 6.09.
- stamina-exhausted: #8C8698 / HC #BDBDBD — Hatched 'debt' fill for Exhausted (at or below 0, 0.5x, floor -50): it grows from the left as stamina goes negative, 0 pegs lit, 'Exhausted' chip shown. 5.37:1 against well (HC 7.64).
- posture: #5B97F5 / HC #6FB0FF — Posture fill on the player tray and enemy plates. 6.45:1 against well. Broken state = danger hatch plus crack overlay plus the word.
- spell-points: #A87BFF / HC #C9A6FF — Glass SP beads and spell-cost pips. 6.23:1 against well, 5.02:1 against tray (HC 7.10 / 10.38).
- danger: #FF6A3D / HC #FF7A45 — Negative feedback text on dark ('Parry Failed!' on ink 6.09, on tray 5.33; HC 8.12), debuff chip rim, broken/enrage hatch, low-health tray hatching. Same value as stamina-red, separate role.
- danger-ink: #B0221F / HC #8E0E0E — Negative text and glyphs ON CARD: compare '▼ −2', 'Locked', drawback stamp. 5.92:1 on card (HC 9.46 on white).
- buff: #1E7A33 / HC #00602A — Buff chip rim and '+' corner mark on card. Positive compare text '▲ +4' on card: 4.72:1 (HC 7.76 on white).
- death: #FF6A6A / HC #FF8080 — 'You Died' (64px) on the death curtain: ink at 92% opacity. Worst case composited over a white floor = #2C2A34, giving 5.06:1 (needs 3:1 at this size); over grass 5.41; HC 7.65.

## semantic_scales
All values are sRGB hex. In code, convert with FLinearColor::FromSRGBColor(FColor::FromHex(...)); UMG pickers use the 'Hex sRGB' field. Contrast is WCAG 2.x, computed rather than estimated (see Accessibility).

STAMINA TIERS (fill on tray-well; a 3-peg gauge token right of the bar; ink notch cuts at 33% and 66%):
- Green #5BD06A: 3 pegs lit.
- Yellow #F5C83A: 2 pegs lit.
- Red #FF6A3D: 1 peg lit.
- Exhausted #8C8698: 45° hatched debt fill growing from the left as stamina goes 0 to -50; 0 pegs; the word 'Exhausted' appears in a chip.
- HC: #4CFF6A / #FFD21F / #FF7A45 / #BDBDBD.
- Without colour: the peg count, the bar position against the notches, and hatching. This is required, not optional: simulated deuteranopia puts Green against Red at only 1.20:1.

RARITY (die-face token: swatch face, ink pips, 2px ink border; pip count Common 1, Green 2, Blue 3, Purple 4, Red 5, Gold 6; the rarity WORD stays in the name):
| Rarity | Swatch | Text on card | Light text (ink/tray only) | HC on white / black | Ink pips on swatch |
|---|---|---|---|---|---|
| Common | #B9B4A8 | #5A5550 (6.43) | #D4CFC4 (ink 11.16, tray 9.77) | #3D3D3D / #E0E0E0 | 8.39 |
| Green | #4FC25E | #1E7A33 (4.72) | #6BDB78 (9.93 / 8.69) | #00602A / #5CFF7A | 7.61 |
| Blue | #3F7FF0 | #1F55C0 (5.88) | #82ACFF (7.67 / 6.72) | #0A3EA8 / #8EC0FF | 4.55 |
| Purple | #9B5BEF | #6B34C2 (6.33) | #C29BFF (7.78 / 6.81) | #55209E / #D7B8FF | 4.24 |
| Red | #E8443A | #B0221F (5.92) | #FF8075 (7.10 / 6.21) | #8E0E0E / #FF9A90 | 4.39 |
| Gold | #F0B830 | #845C00 (5.22) | #FFD45E (12.25 / 10.72) | #5E4100 / #FFE07A | 9.58 |
Every HC value is at least 7:1. Rules:
- Swatches never sit bare on card: Common, Green and Gold fail at 1.58-1.99 there, so they always sit inside an ink line.
- Rarity-coloured text never sits on felt.

FEAT TIERS (Tally stickers with a white vinyl margin and an ink line; the tier word is always printed):
- Common: round matte sticker #E9E3D3, ink text 13.54.
- Rare: scalloped rosette, enamel #6E95F2, ink text 5.98.
- Legendary: 12-point starburst #F2B636 with an animated holo-foil sheen, ink text 9.51.
- HC plates: #FFFFFF / #9CC0FF / #FFD34D with black text (21 / 11.40 / 14.68).
- Shape is the primary signal. The plates against card are only 1.12-2.53, which is why the ink line is mandatory.

DAMAGE TYPES AND HEALING (world numerals: Nunito Black fill, 3px ink stroke, 4px white vinyl margin; each type has a glyph token at 0.6em to the left of the number):
| Type | Fill | Fill vs ink | Glyph |
|---|---|---|---|
| Physical (Slash, Pierce, Blunt) | #FFFFFF | 17.34 | diagonal cut / arrowhead / 4-point burst |
| Fire | #FF8B3D | 7.44 | flame tongue |
| Frost | #8FDBFF | 11.35 | hex crystal |
| Lightning | #F4F05A | 14.41 | zig-zag |
| Poison | #A5D64A | 10.17 | bubbling droplet |
| Corrosive | #4FD8C4 | 9.87 | bitten drip circle |
| Arcane | #C99CFF | 7.99 | 4-point sparkle |
| Radiant | #FFC94D | 11.32 | ringed sun |
| Healing | #6FF2B4 | 12.41 | '+' prefix, no glyph, rises with a bob |
- Shadow is printed in NEGATIVE: dark fill #2A1630 with an orchid stroke #E8A3D0 (8.42:1) and a crescent glyph. The polarity flip identifies it without colour.
- Crit: x1.5 scale plus a marigold #FFC83A starburst backer (11.21 against ink).
- HC: black stroke; every fill is at least 9.01:1 against black.
- Without colour: glyph, the Shadow polarity, the '+' on healing, and the crit burst.

BUFF vs DEBUFF (status chips; four socket shapes by category):
- Buff: LIGHT card chip, rounded pill, round coin socket, buff #1E7A33 rim and '+' corner mark. Ink text 15.14.
- Debuff: DARK ink chip, clipped-corner tag, danger #FF6A3D rim (6.09) and '−' corner mark. Card text 15.14.
- DoT (Burning, Bleeding, Poisoned): the debuff tag with a 3-dot ticker along the bottom.
- Control (Rooted, Frozen, Fear, Silenced, Sleeping): octagon socket.
- Environmental (Drenched): rounded-square socket.
- Stacking statuses (Frostbitten 1-5, Shocked 1-3) show stack pips.
- Without colour: polarity, shape, corner mark, glyph and name.

ENEMY TIER MARKS (brass #D9A441 pegs left of the name, 6.75 against tray):
- Basic: no pegs.
- Elite: 1 peg.
- Champion: 2 pegs.
- Miniboss: 3 pegs, a brass plate rim, and the caption word 'Miniboss'.
- Boss: no plate; the boss bar is used instead.
- Summon: dashed well-lip outline, plate face at 80%, hourglass glyph.
- Elite and above also print the tier word as a caption. Peg count plus word, never colour alone.

## typography
FAMILIES (Google Fonts, OFL). I checked both licence files: neither declares a Reserved Font Name, so subsetting and instancing are allowed. Credit all three in Credits (#63).

1. DELA GOTHIC ONE 400 (display, the box-print voice).
- Uses: logotype, screen titles, 'You Died', boss names, the Tally's caps headlines.
- Why: heavy stamped game-box print that reads as solemn at 64px and as a punchy announcement at 32px.
- Only at 32px and above. Never for live numbers: its digits are proportional (0 = 917, 1 = 588 units; I measured the advances).
- Subset to Latin with pyftsubset: the TTF is 2.5 MB because it includes Japanese.

2. NUNITO 400/600/700/800/900 (all UI text and ALL numerals).
- Why: rounded terminals read as molded toy plastic.
- Its default digits are TABULAR at every weight. I measured with fontTools on google/fonts Nunito[wght].ttf: all ten digits advance 600/1000 em at the default instance and when instanced at 700, 800 and 900. Slate's lack of tnum support therefore does not matter.
- Upstream ships only the variable font, and Slate needs static faces. Take the static/ TTFs from the fonts.google.com download, or instance with fontTools varLib.instancer (verified: digits stay tabular).

3. ATKINSON HYPERLEGIBLE NEXT 400/700 (optional 'Legible text' accessibility swap).
- Menu body copy only: descriptions, tooltips, status lines.
- Its digits are proportional (measured), so it never replaces HUD numerals.

UE SETUP:
- One Font Face asset per weight, Loading Policy Inline (cook-safe; commit them).
- F_Mordecai_UI composite: Nunito typefaces Regular/SemiBold/Bold/ExtraBold/Black.
- F_Mordecai_Display composite: Dela.
- UMG Size is in points at 96 DPI, so UMG size = px x 0.75.

TYPE SCALE at 1920x1080 (px / UMG size / line height / weight; 720p renders at x0.666):
1. logotype: 96px / 72 / 1.0, Dela, letter spacing 40. 'PROJECT MORDECAI' on the title and loading screens.
2. display-l: 64px / 48 / 1.05, Dela. 'You Died', level-up slam.
3. display-m: 44px / 33 / 1.1, Dela. 'Paused', dialog titles, boss name on the bar.
4. display-s: 32px / 24 / 1.15, Dela, ALL CAPS, letter spacing 20. Tally headlines ('FEAT UNLOCKED').
5. title: 28px / 21 / 1.2, Nunito 900. Tab labels, card names, toast item line.
6. label: 24px / 18 / 1.25, Nunito 800. Buttons, '[F] Pick up {0}', inventory rows, combat feedback base.
7. body: 20px / 15 / 1.4, Nunito 600. Descriptions, Tally quip line, settings rows.
8. caption: 18px / 13.5 / 1.3, Nunito 700. Status chip names, tier words, keycap letters. This is the HUD minimum (12px at 720p).
9. num-hud-l: 32px / 24 / 1.0, Nunito 900. Health '87%', death countdown (scaled to 64px).
10. num-hud-m: 28px / 21 / 1.0, Nunito 900. SP '7 / 10', cooldown seconds, 'Kills: 12', 'Hit 3'.
11. num-hud-s: 20px / 15 / 1.0, Nunito 800. Status timers, enemy plate numbers.
12. num-world: 36px / 27 base (crit 54px), Nunito 900. UMG OutlineSettings 3px ink, plus a duplicate TextBlock behind it with a 7px white outline for the vinyl margin.

STEADY NUMERALS:
- Every live number uses Nunito.
- Each sits in a fixed-width SizeBox sized for its widest value ('100%', '10 / 10', '15.0') with Justification Right. A '%' or '.' therefore never shifts the digits.

## shape_language
MATERIALS: four physical parts, and every element is one of them.
- Die-cut CARDBOARD: card face, 4px card-edge thickness, ink line, faint paper-grain tiling at 5%. Used for menus, prompts, stickers.
- Glossy PLASTIC: tray, a top-half white gloss band at 16%, a 2px light lip. Used for the HUD tray, keycaps, pegs, beads.
- FELT: tiling fuzz at 6%, 4px inner shadow. Used for menu linings.
- ENAMEL / VINYL: white 4px margin outside the ink line, foil sheen material on Legendary. Used for the Tally layer.

TOKEN CONSTRUCTION (UOverlay, back to front):
1. Contact shadow: a 9-slice PNG (ink at 35%, baked 10px blur, Box margins 0.25), offset (0, +6px). +10px when lifted, +2px when pressed. Light always comes from above, so shadows fall straight down, matching the diorama.
2. Thickness edge: the same silhouette tinted card-edge (cardboard) or tray-well (plastic), offset (0, +4px).
3. Face: a white 9-slice tinted with the token colour.
4. Outline: a separate 9-slice so the ink stays ink whatever the tint.
5. Gloss or grain layer.
6. Content.
Instead of stacked images, one SDF material M_UI_Token (User Interface domain) can draw rounded-rect face, outline, edge and shadow in one pass. Parameters: Radius, OutlinePx, EdgePx, FaceColor. Use it for many-instance widgets such as enemy plates.

RADII at 1080: tokens 12px; cards 10px; dialogs 16px; keycaps 8px (plus a 4px extrusion); chips and pills = height/2; die faces 6px.
STROKES: ink 3px on the HUD and world (2px effective at 720p); 2px in menus; well lip 2px; focus ring 4px marigold plus a 2px ink halo on light grounds; perforation separators 2px dashed (6 on, 4 off).
SIGNATURE FRAME, the 'Insert Tray' (pause menu, Options, Mage Tower):
- 16px glossy tray rim around a felt bed sunk with a 4px inner shadow.
- Tabs are cardboard INDEX DIVIDERS standing 36px proud of the rim. The active divider is a card face raised 8px; inactive ones are felt-recess, sunk 4px.
- Content sits on the felt as cards and chits, never on bare text.
- Built from: UBorder rim (Box brush), UBorder felt (tiling material), 9-slice divider brushes on UMordecaiTabButton (Normal / Hovered / Pressed brushes plus a 4px vertical offset by state).

MOTIFS:
- Peg tracks and punch holes: SP beads, skill ranks with brass star eyelets at 1/5/10/15/20.
- Dice pips: rarity.
- 45° printed hatching, 6px pitch, as a screen-aligned M_UI_Hatch material: negative states (Exhausted debt, Posture Broken, locked, enrage, low health).
- Perforated tear lines between list sections.
- Luggage tags with a string hole: interact prompts and world labels.
- Silver scratch-off panels: unidentified items, M_UI_Scratch with a Reveal parameter.
- Tally layer: halftone dots and starburst die-cuts.

BARS: UProgressBar.
- BackgroundImage = well 9-slice with the lip.
- FillImage = a gel 9-slice with the gloss band (FillColorAndOpacity per state), plus an overlay UImage of ink notch cuts.
- SP beads: FillImage uses a bead-mask material tiled by a Count parameter (needs MaxSP pushed from code).
- Cooldown: the spell icon itself is the FillImage (BarFillType BottomToTop) over a desaturated, ink-washed copy as BackgroundImage. The colour icon 'refills' upward as the cooldown elapses.

ICONOGRAPHY:
- 'Token glyphs': one weight, 3px stroke at 40px, rounded caps and joins, flat single colour (ink on light, card on dark), seated in sockets (coin / tag / octagon / rounded square). 32px HUD sockets, 22px on enemy plates.
- No emoji, no hearts, no gems/rupees.
- Health glyph = a pulse line in a coin. Stamina = a breath swirl. Posture = a spirit-level bubble. SP = a bead.
- KEYCAPS for keyboard (card face, ink letter, 4px extrusion) and PLASTIC BUTTONS for gamepad (round face buttons, pill shoulders). Original art, mapped through CommonInput controller data so prompts become device-aware.

## hud_layout
CANVAS 1920x1080, ScaleToFit. Rules:
- Every HUD plate is opaque tray or card with a 3px ink line. Nothing relies on the floor behind it.
- The HUD must collapse while the pause menu or death screen is up. Today it is AddToViewport and draws over both dims; move it into UI.Layer.Game or hide it on those layers' activation.
- Names in CAPS are existing BindWidgetOptional slots. Names marked NEW are additive optional slots that need small C++ binds.

PLAYER BOARD (anchor top-left 0,0):
1. RESOURCE TRAY plate: offset (32,28), size 492x144, tray, gloss. Inside, x from the tray origin:
   - HealthBar row (y 16): 32px health glyph coin at x 16. HealthBar/HealthProgressBar gel at x 56, 320x28, with quarter notches. NEW HealthPercentText '87%' num-hud-l in an 88x40 right-justified box at x 388 (the spec wants Health as %).
   - StaminaBar row (y 56): StaminaProgressBar 320x20 with notches at 33% and 66%. Tier gauge of 3 pegs (38x20) at x 388. 'Exhausted' chip pops out to the right (contextual).
   - PostureBar row (y 84): PostureProgressBar 320x10. The row is reserved, but the bar shows only while posture < max and hides 3 s after it refills. Broken = hatch plus crack plus 'Broken' caption.
   - SpellPointsBar row (y 104): 10 glass beads (24px, 8px gap = 312px) via SpellPointsProgressBar. SpellPointsText 'x / y' num-hud-m right-justified at x 388, width 88.
2. SPELL RACK: offset (32,188), 4 tokens 80x80 with a 12px gap (356 wide). SpellCooldownA/B, plus C/D per US-088.
   - Each token: the spell icon is the cooldown fill. CooldownText seconds (num-hud-m, ink outline) centred, empty when ready.
   - Keycap 28x28 overhangs the bottom-right corner: Q / E / R / 1 (pad LB / RB / none / D-pad Up, pending US-097). SP cost as 1-4 beads on the top-left rim.
   - States: ready (gloss); cooling; 'not enough SP' (beads flash danger, icon ink-washed); casting (a ring traces the token edge over the cast time); silenced (octagon lock overlay).
3. STATUS CHIPS: StatusEffectBar/IndicatorContainer at offset (32,288), chips 40px tall, 8px gap, max 8.
   - Each chip: TintImage = the chip ground (card or ink by polarity), glyph socket, StatusNameText (caption), DurationText (num-hud-s).
   - With more than 4 chips, older ones collapse to glyph-only sockets.
   - Contextual: the row is empty most of the time (status is 'primarily VFX').

TOP-RIGHT (anchor 1,0):
4. KillCounter: offset (-232,28), 200x52, tray, KillCountText 'Kills: N' num-hud-m with a tally-mark glyph. Arena only.
5. TOAST RAIL (NEW widget): offset (-552,96), width 520. Sticker cards 520x112, gap 12, max 3 stacked, newest on top.
   - Each card: Tally headline (display-s), name (title), quip (body).
   - Feats 3.5 s, rank-ups 2.5 s, milestones 4 s.

BOTTOM-CENTRE (anchor 0.5,1, alignment 0.5,1):
6. PickupPrompt: offset (0,-32), region 560x96.
   - PromptText '[F] Pick up {0}' (label, ink) on a card luggage tag 64px tall with a keycap drawn behind the '[F]'. The text stays a real TextBlock.
   - InventoryHintText '[I] Inventory' is a small tray chip (caption, text-dim) at the region's bottom-right. It is always visible but quiet.
7. CombatFeedback: offset (0,-148), 360x52, ink 'stamped' plate, FeedbackText label at 32px.
   - Positive in focus marigold: 'Perfect Dodge!', 'Perfect Block!', 'Parried!', 'Posture Broken!'.
   - 'Parry Failed!' in danger with a hatched border.
8. ComboCounter: offset (0,-212), 160x56, tray, ComboText 'Hit 3' num-hud-m plus chain pips (filled pips for hits so far, outline pips for the weapon's chain length). Visible during the combo window only.

BOTTOM-RIGHT (anchor 1,1, alignment 1,1):
9. WEAPON CARD (the existing EquippedWeaponWidget gets a tree): offset (-32,-32), 340x88, card.
   - 64px weapon coin glyph.
   - NEW WeaponNameText (title, ink) 'Longsword'.
   - NEW WeaponTypeText (caption, text-dim-card) 'Slash · 3-hit chain'.
   - Keycap 'Tab' / pad Y. Always visible.
10. QUICK ITEMS (spec, future): offset (-32,-136), 4 tokens 60x60 with an 8px gap. Keycaps 1-4 / D-pad, stack count num-hud-s. Contextual: a slot shows only when stocked.

TOP-CENTRE (anchor 0.5,0):
11. BOSS BAR: offset (0,24), 960x104, tray with a brass rim.
   - Boss name display-m on a lid banner.
   - Health gel 912x24 with brass phase pegs at the phase thresholds.
   - Posture bar 912x10 below it.
   - Phase pips 'I II III'.
   - Enrage: a danger hatch sweeps the rim and an 'Enraged' stamp appears.
   - While it is up, level-up and Legendary slams go to the toast rail.
12. SLAM (level-up / Legendary feat): centred at (0.5, 0.28), 640x200, 2.4 s. Never during a boss fight.

WORLD SPACE:
13. ENEMY PLATE (Lyra indicator, bottom-centre anchored at Z +120, visible within 2000):
   - 176x56 tray plate with the ink line.
   - Tier pegs left. NameText (caption at 20px, card).
   - HealthProgressBar 150x10 cherry with quarter notches.
   - NEW PostureProgressBar 150x6 that fills toward 'BROKEN' hatch; critical window = the plate rim pulses marigold plus a riposte keycap.
   - StatusIconContainer: up to 3 glyph sockets of 22px.
   - Shows on aggro or damage; fades 4 s after combat. A windup flashes the top rim danger plus a '!' token, but telegraphs are primarily ground VFX.
   - Retire the duplicate red WBP_EnemyHealthBar.
14. DAMAGE NUMBERS: spawn at the hit point with ±30cm scatter, rise 100cm/s, 1.5 s life, max 20, as described in the scales. Evaded or parried hits show no number.
15. WORLD PICKUP LABELS: replace the faint TextRender with a screen-space luggage-tag widget (card, ink line, caption-to-label size).

ALWAYS VISIBLE: resource tray (health %, stamina and gauge, SP beads and 'x / y'), spell rack, weapon card, '[I] Inventory' chip, kill counter (arena).
CONTEXTUAL: posture row, status chips, combo, feedback, pickup tag, toasts and slams, boss bar, enemy plates, damage numbers, quick items, the Exhausted chip, and low-health hatching on the tray rim below 25%.

## menus
FRONT END (replaces Lyra, issues #57-#63):
- TITLE screen:
  - Background: a live tilt-shift vignette (a village block on the edge of a felt table, depth of field top and bottom, smoke and a turning mill).
  - Left 40%: the 'box lid' logotype 'PROJECT MORDECAI' (logotype size, card face, ink line, 6px chipboard edge, tilted -3°), with the tagline on a tray strip.
  - Button stack at (120,520): token buttons 440x64 with a 12px gap: 'Continue' (only with a save), 'Play', 'Options', 'Credits', 'Quit'.
  - Version slip bottom-right in caption text-dim. It replaces the machine-name, NULL and ADD PLAYER watermarks.
  - SIGNATURE: on Play the lid card lifts away (Y -40, +6°) and the camera dollies into the diorama as the opaque loading screen slides in.
- PLAY: the arena is a single game card (art, 'Mordecai Arena'). It skips Lyra's host, browse and replay screens (#60).
- OPTIONS: an Insert Tray 1280x760 with dividers Gameplay / Controls / Display / Audio / Accessibility.
  - Rows are card strips: discrete sliders as peg tracks with a Nunito value, toggles as two-position plastic slide switches with the state word printed beside them.
  - Controls shows a keycap board with a KBM / Gamepad flip.
- CREDITS: a rules-booklet page (card, perforated top) scrolling at 40px/s, including the OFL notices.
- QUIT CONFIRM and all dialogs (UI.Layer.Modal, replacing W_ConfirmationDefault):
  - Ink dim at 70%. One 520x280 card is 'dealt' in with a 2° settle.
  - Title display-m 'Quit to desktop?', body text, buttons 'Quit' and 'Keep playing'. Focus defaults to the safe choice.
  - Error variant: a danger-hatched top band plus an ink '!' token.
- LOADING (UMordecaiLoadingScreenWidget, still opaque, still built in Initialize()):
  - Tray ground with felt grain. TitleText 'PROJECT MORDECAI' logotype in card. StatusText 'Loading...' in label text-dim.
  - The UThrobber uses 3 brass peg pieces that hop in sequence.
  - Optional world-voice tip slip.

PAUSE MENU SHELL, the 'Insert Tray':
- BackgroundDim: ink at 70%. MenuColumn stays 1000x640.
- TitleText 'Paused' (display-m, card) is printed on the tray's top lip.
- TabBar: five index dividers, 176x56 each, plus LB / RB shoulder-pill glyphs at the ends (976px total).
  - Active divider: raised card with ink label.
  - Inactive dividers: sunk felt-recess with text-dim labels.
  - Hover lifts 4px; focus adds the marigold ring.
- TabContentBorder: felt with a 4px inner shadow and 24px padding.
- NEW bottom hint strip (ink, 40px): keycaps 'Esc Back · LB RB Tabs · I Close', device-aware.
- SIGNATURE: switching tabs pulls the new divider up while the content cards slide out of the tray.

TABS:
- CHARACTER, the 'player board':
  - Top strip: Level well, then Health / Stamina / Spell Points / Posture as 'x / y' number wells.
  - Three printed columns, Physical / Resilience / Magical. Each row has a code chip ('STR'), the full name, the base value in a recessed number well (num-hud-m) and Effective Mod '+9.4%' in text-dim.
  - A perforated derived-stats slip in two columns, signed % right-justified.
  - Moment: hovering an attribute draws brass threads to the derived stats it feeds.
- SKILLS:
  - Vertical category dividers on the left (Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic) and a 'Skill points: 3' counter well.
  - Scrollable PUNCH-CARD rows, 72px each: name (title), 'Rank 7 / 20', and a 20-hole strip with brass star eyelets at 1/5/10/15/20.
  - 'Allocate' token. When disabled: text-dim, a lock glyph and the reason ('No skill points', 'Max rank').
  - Detail card: a 5-row milestone ladder (reached = brass 'Reached' seal, next = outlined, locked = hatched 'Rank 15').
  - Moment: Allocate punches the next hole and the chad pops out.
- FEATS, the 'sticker album':
  - Header 'Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary'.
  - A 3-column grid of 288x132 slots. Unlocked = a sticker in its tier shape with name, tier word and short description. Locked = a dashed die-cut outline with the name, '???' and per-condition peg progress ('Enemies burned at once: 4 / 6').
  - Detail card with a danger-ink 'Drawbacks: 1' stamp for Rare and Legendary.
  - Moment: new stickers sit with one corner peeled (tally pink underside) until viewed, then smooth down.
- INVENTORY, the 'component tray':
  - The FilterBar spans the full 952px width above both columns, fixing the #37 overlap: 8 chips at label 20px.
  - ItemListBox rows are 56px card strips: rarity die, name in the rarity card-text colour, 'x3' / '[equipped]' suffix, and an 'Identify' token with a lens glyph.
  - Unidentified rows keep the '? ' prefix over a silver scratch panel.
  - LedgerColumn, 300px: 'Stored Resources' with a felt-recess compartment of card chits ('Iron Ore  x1').
  - Empty states are pencilled notes on a card.
  - Moment: Identify = scratch-off.
- SETTINGS: 'Coming Soon' printed on a card pinned with a brass tack. Later it reuses the Options rows.

ITEM TOOLTIP AND COMPARE:
- A 360px card fans out 12px to the right of the focused row, at +1°.
- Contents: rarity band (inside the ink line), die and rarity word; type line ('Magical Item · Amulet'); name (title); stats with '▲ +4' in buff and '▼ −2' in danger-ink (the arrow and sign carry the meaning); description; identification line ('Unidentified. Identify at the Mage Tower to use.').
- Compare puts two cards side by side; the equipped one has a brass 'Equipped' tab.

DEATH SCREEN (UI.Layer.Game, HUD hidden):
- Optional 60% desaturation. An ink curtain band (92%, full width, 300px tall) slides in.
- DeathText 'You Died' display-l in death colour; CountdownText Nunito 900 at 64px in card (steady digits); KillCountText 'Kills: N' label text-dim; NEW subline in body text-dim.
- SIGNATURE: a small player standee token on the band tips over.

TOWN SERVICE, Mage Tower Identify:
- A 1280x720 Insert Tray titled 'Mage Tower'.
- Left: 'Sealed' column of unidentified cards with scratch panels and dice ('Unidentified Purple Amulet').
- Centre: 'the Lens', a large card slot showing the selected item, a cost line (TODO(DECISION)), and 'Identify' (plus 'Identify all' once batch is decided).
- Right: stack of identified results.
- SIGNATURE: a card-sized scratch-off, then the Tally 'IDENTIFIED' sticker.

MAP (M / View):
- A folded paper map spread on felt, with crease texture. Biome cells are printed tiles; unexplored cells are blank card with a dotted grid.
- Gates are stamped symbols (explosives wall, magnet rail, phase barrier, glider gap), 'legible before usable'.
- The player is a standee pawn; the legend sits on a slip. Cartography ranks add stamps.

## motion
PRINCIPLES:
- Everything has weight and rests on the table. Focus LIFTS (Y -4 to -6px, shadow grows), press SINKS (Y +3px, shadow shrinks), placement settles with a small overshoot.
- Tally items SLAP and PEEL; nothing glows in or floats.
- UI response stays under 300ms and gameplay time is never slowed.
- Exits are faster than entrances.
- With Reduce Motion on, every move becomes a 120ms opacity fade, with no slaps, rolls, tips or screen shake.
- Build: UMG Widget Animations in the WBPs (transform, render opacity, material parameters) with curve easing. C++ fallback widgets lerp in NativeTick using the same numbers.

NAMED ANIMATIONS:
1. token-press (every button)
   - Press, 70ms: TranslateY 0 to +3px, shadow offset 6 to 2px, ease-in quad.
   - Release, 160ms: back to rest, ease-out back (overshoot 1.5).
   - Hover lift: 120ms, ease-out cubic.
2. divider-pull (tab switch)
   - Outgoing divider sinks 8px: 100ms, ease-in quad.
   - Incoming divider rises 8px: 180ms, ease-out back (1.3).
   - Content cards slide 24px from the tray edge and fade 0 to 1: 200ms, ease-out cubic, 40ms stagger, max 5 cards.
3. sticker-slap / sticker-peel (toasts)
   - In, 240ms: scale 1.3 to 1.0, rotation -8° to -2°, opacity 0 to 1, ease-out back (1.8), with a 1-frame 0.95 Y squash on landing.
   - Hold: 2.5 s rank-up, 3.5 s feat, 4 s milestone.
   - Out, 280ms: rotation +10°, X +48, Y -16, opacity to 0, ease-in cubic.
   - Toasts below shift down 120ms, ease-out quad.
4. damage-trail (health and enemy health)
   - Loss: a card-at-60% chip holds at the old value for 450ms, then drains 300ms, ease-in-out sine.
   - Gain: fill grows 250ms, ease-out quad, plus a gloss sweep 300ms linear.
5. scratch-reveal (Identify)
   - M_UI_Scratch Reveal 0 to 1 over 650ms, ease-in-out sine. The name text swaps at 70%.
   - Then die-roll, 400ms: 3 face flicks at 60ms each, then settle with ease-out back.
   - Then the Tally 'IDENTIFIED' sticker-slap.
6. standee-tip (death)
   - Curtain slides in 300ms, ease-out quart.
   - Standee rotates 0 to 90° about its base over 450ms, ease-in quad, then bounces 10° and 3° over 160ms.
   - 'You Died' fades in 200ms after landing.
Extras: punch (Allocate) 140ms, scale 1 to 0.9 to 1, plus a chad particle; coin-flip (level-up slam) 520ms Y-axis flip, ease-in-out cubic.

## accessibility
CONTRAST:
- Every pair above was computed with WCAG 2.x relative luminance, not estimated.
- Scripts: contrast.py and scales.py (throwaway tools from the direction round, not kept).
- Default theme: every text pair is at least 4.5:1. Large text, borders and focus rings are at least 3:1.
- HUD plates hold at least 4.51:1 against sampled diorama floors (arena grey, grass, sand, water, white, mid shade) because they are opaque tray with an ink line. Card faces never touch a floor without that line.
- One computed failure (well against tray, 1.24) was fixed with the well-lip token.

HIGH CONTRAST THEME (ship as Options > Accessibility > Theme):
- Swap a DA_UITheme data asset of these tokens, read by every widget.
- Dark grounds become #000000 and card becomes #FFFFFF. Felt, grain, gloss and foil layers turn off. Ink lines go from 3px to 4px. Focus becomes #FFE500.
- Every HC text pair is at least 7:1 (lowest: ink on HC health 7.02, and that pairing is never used for text).

COLOUR IS NEVER THE ONLY SIGNAL:
- Health shows numerals.
- Stamina uses the peg gauge, notches and debt hatching (needed: simulated deuteranopia puts Green against Red at 1.20:1).
- Rarity has its word and die pips.
- Feats have tier shape and word.
- Statuses use chip polarity, shape, corner mark, glyph and name.
- Damage types use glyphs and the Shadow negative print.
- Posture Broken uses hatch plus the word.
- Enemy tier uses peg count plus the word.
- Compare uses ▲/▼ plus the sign.

TEXT:
- Text size 100 / 115 / 130%. The HUD caps at 115% so its layout holds.
- HUD minimum is 18px at 1080 (12px at 720p).
- Optional 'Legible text' swaps menu body copy to Atkinson Hyperlegible Next.
- Every visible string stays a real UTextBlock (the playtest bridge reads them).

MOTION AND FLASH: Reduce Motion (see Motion). Screen-shake slider. Crit and feedback flashes limited to 2 per second. Low-health hatching does not strobe (a 1.2 s ease pulse).

TIMING: toasts hold at least 2.5 s and never carry information that is not also in the menus. A future 'Tally log' in Feats lists recent entries. Tally commentary can be Full, Headlines only or Off.

INPUT:
- Every control is focusable by gamepad and shows the 4px focus ring.
- Hit targets are at least 48px at 1080.
- Prompts are device-aware (keycaps vs pad buttons through CommonInput data).
- Nothing requires hover.

HUD SCALE 90 / 100 / 115%. No HUD opacity slider: lowering plate opacity would break the computed floor contrast.

## risks
1. TONE BLEED. The toy materials could make the Withered Kingdom and Lich King feel cute.
   - Mitigation: boss bar, death and loading drop gloss and stickers and use ink plus Dela only.
   - Test first: the boss bar and death screen over a dark, desaturated greybox.
2. TALLY FATIGUE. The DCC layer could grate, or clash with the serious lore.
   - Mitigation: throttle and a commentary option. The Tally's diegetic frame is a TODO(DECISION) for Jeff.
   - Test: 20 minutes of play with toasts on, then ask whether any line annoyed.
3. HUE OVERLOAD. Red, yellow and green are claimed by stamina tiers, rarity and damage. Health against stamina-red is only 1.32:1, and the Tally against health is 1.29:1.
   - Separation relies on position, shape and words.
   - Test: deuteranopia and protanopia simulation of a 720p capture with Exhausted, low health and a Red-rarity pickup all on screen.
4. PERFORMANCE. 4-6 layered images per token, multiplied by 20 enemy plates and 20 damage pops, breaks Slate batching.
   - Prototype the single-pass M_UI_Token SDF material early, and profile with stat slate.
5. UMG ASSUMPTIONS TO VERIFY.
   - That UProgressBar clips rather than squashes an image FillImage (the cooldown-icon refill). Fallback: an M_UI_Radial material.
   - Bead SP needs MaxSP pushed to a material parameter.
   - Slate does not render variable fonts, so static Nunito faces are needed. This is verified only on the font side.
6. CODE TOUCHES. The design needs small C++ work:
   - New optional binds: HealthPercentText, WeaponNameText/WeaponTypeText, enemy PostureProgressBar, death subline, toast rail.
   - SpellCooldownC/D.
   - Moving or hiding the HUD under menu and death layers.
   - Retiring WBP_EnemyHealthBar.
   - Replacing the TextRender pickup labels with widgets.
   WBPs must be saved and committed, or the package shows nothing.
7. DISPLAY FONT RISK. Dela Gothic One is wide and has tight counters.
   - Test the longest strings at display-s 32px and at 720p: 'FEAT UNLOCKED', 'Tempest Ascendant', 'Quit to desktop?'.
   - Fallback: Nunito Black caps for toasts.
TEST FIRST:
1. A greybox of the resource tray, spell rack, pickup tag and two damage numbers on DevTestMap's light floor.
2. Capture it with playtest_boot at 720p.
3. Read engine_NN.png for edge legibility of the 2px-effective ink line and the 18px caption.

