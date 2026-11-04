## name
Tin Tally

## thesis
The interface is the Tally: a cheerful pressed-tin counting machine left running since the Automation Age. It counts everything you do and hands you the result as tickets hung from a rail. Dungeon Crawler Carl supplies the spine: a constant, dry-witted stream of skill-ups, feats, loot reveals and post-fight receipts. That stream is paced as tickets that wait while you fight and print the moment you are safe. Link's Awakening supplies the restraint: four small, chunky, rounded plates on screen, flat lithograph colours behind a heavy ink outline that reads over a bright toy floor, and a cosy toy-box subscreen. It is neither sci-fi nor parchment. It is a wind-up tin toy that keeps count, which is also the game's Control theme in miniature (a machine that cannot stop counting); on the Acceptance ending it prints its last ticket, "Count closed."

## voice
TWO REGISTERS, ONE RULE: the headline is always plain; the joke, if any, is the second line.

1) Plain voice (the game). Used for labels, buttons, prompts, system facts and every shipping string.
- Sentence case, verbs on buttons ("Identify", "Allocate", "Stay", "Quit") and digits for numbers.
- Exact system vocabulary: Spell Points, Posture Broken, Exhausted, Rank 12 / 20, "Unidentified Purple Amulet".
- No exclamation marks, except the shipping combat callouts.
- Status tooltips use the launch-set identity lines verbatim ("You're on a clock", "Recovery is denied", "Next mistake hurts more", "Mastery streak reward", and the rest).

2) The Tally (remarks). An Automation-Age counting engine: deadpan, literal, quietly proud of numbers, puzzled by anything it cannot count.
- At most 9 words; it talks about you or about itself.
- It never mocks a failure and never speaks during combat.
- No remarks on boss intros, biome cards, endings or status lines. The world's "Impermanence vs Control" tone owns those.
- Remarks are optional: Settings > Accessibility > Tally remarks On/Off. With remarks off, every ticket still makes sense.

SHIPPING STRINGS KEPT EXACTLY. They are restyled, never reworded, because playtests compare them:
- Combat: "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!", "Hit N"
- HUD counters and prompts: "Kills: N", "x / y" (SP), "[F] Pick up {0}", "[I] Inventory"
- Death and loading: "You Died", "PROJECT MORDECAI", "Loading..."
- Pause menu: "Paused", the five tab labels, "Coming Soon"
- Inventory: "Inventory is empty.", "No items match this filter.", "No resources stored.", "Identify", "Stored Resources"
- Feats header: "Feats: N unlocked — a Common, b Rare, c Legendary"
- Status names stay tag-derived ("StoneSkin") until US-091 decides on the display name "Stone Skin".

SAMPLES
1. Feat unlocked (Plate):
   - kicker "FEAT UNLOCKED · COMMON"
   - title "Pyromaniac"
   - cause "Burned 6 enemies at once."
   - remark "Six fires, one decision. Filed under enthusiasm."
2. Skill rank-up (Slip): kicker "SKILL UP", line "Dodging · Rank 6 / 20". Two rank-ups merged in one receipt read "Dodging · Rank 6 / 20 (+2)".
3. Skill milestone (Plate):
   - kicker "TECHNIQUE · RANK 10"
   - title "Dodging"
   - line: the milestone description from data, else "No data available."
   - remark "Ten clean exits. The Tally rounds up."
4. Identify reveal: the row reads "Unidentified Purple Amulet" and resolves to:
   - kicker "IDENTIFIED · PURPLE"
   - title "Mysterious Amulet"
   - remark "It had a name all along. It wasn't telling."
5. Level up (Plate): kicker "LEVEL UP", title "Level 7", line "+1 Skill Point", remark "Seven. Up from six, as forecast."
6. Death screen: "You Died" / countdown "3" / "Kills: 4" / new remark line "The Tally keeps your place."
7. Low-stamina hint (a tag hung from the stamina row, once per session): "Exhausted: everything at ×0.5. Ease off and breathe."
8. Main menu tagline: "Everything counts. Nothing lasts."

Also: the quit dialog body reads "The Tally will keep count." Overflow tickets read "6 more on the spike".

## palette
- ink-outline: #0E1016 / HC #000000 — 3 px (HC 4 px) outer stroke on every world-facing HUD element: plates, tags, keycaps, callout letters, damage numbers, enemy bars. It is the dark half of the dual edge: 15.2:1 on the white blockout floor, 9.3:1 on grass. Also used as the hard print shadow at 55% alpha.
- plate: #1A1E2B / HC #000000 — Pressed-tin plate fill: gauge plate, rail head, spell slots, status chips, enemy name pill, menu shell, dialogs, loading screen. 94% opacity in the HUD (still 13.2:1 for text over the brightest floor) and 100% in menus and HC.
- plate-raised: #272D3F / HC #161616 — Inactive tabs, hovered or selected list rows, attribute cards and detail panes inside the shell. The lowest pair is text-disabled at 4.55:1.
- track: #3A4157 / HC #000000 — Empty bar wells, empty SP token fill, cooldown well. Every fill clears 3:1 against it (lowest: foe 3.29:1). In HC the well is black with a 2 px white rim.
- rim-tin: #C5CFD9 / HC #FFFFFF — 2 px inner pressed rim on plates, the light half of the dual edge (7.6:1 on a dark swamp floor, 10.0:1 on a cave floor). Also dividers, tally-mark ornaments and empty-token rings (10.5:1 on plate). With ink-outline it guarantees 3.47:1 on ANY floor colour (4.6:1 in HC).
- dim-veil: #0E1016 / HC #000000 — Modal, pause and death backdrop at 72% (HC 85%), over the frozen world. Text never sits directly on the veil, only on plates or tags.
- text-primary: #F4F6F9 / HC #FFFFFF — All text on plate (15.3:1) and plate-raised (12.7:1). Also cooldown seconds over the shutter (7.1:1 even over a white icon).
- text-muted: #AAB3C4 / HC #E0E0E0 — Secondary text, row labels (HP/ST/SP) and inactive tab labels: 7.9:1 on plate, 6.5:1 on raised (HC 13.7:1).
- text-disabled: #8C95A8 / HC #BDBDBD — Disabled labels such as 'Allocate' at 0 points: 5.5:1 on plate, 4.55:1 on raised (HC 9.6:1). Always paired with a lock glyph, so the state is not carried by colour alone.
- tag-enamel: #F3F5F8 / HC #FFFFFF — Light enamel ground of tickets and tags, keycap faces, the active tab, tooltip and item cards, the pickup prompt and the 'Paused' nameplate. It holds over light floors because of its 3 px ink-outline and print shadow.
- text-on-tag: #12151C / HC #000000 — Text on tag-enamel (16.7:1) and on every coloured band or pill: stamina tier pills (lowest Red 6.4:1), rarity bands (lowest Red 6.5:1), feat badges (lowest Legendary 7.2:1).
- text-on-tag-muted: #4A5265 / HC #333333 — Tally remark line and secondary text on tags: 7.2:1 (HC 12.6:1).
- focus: #FFD43B / HC #FFFF00 — 3 px focus ring plus a 2 px ink inner ring on every focusable control. The marigold carries it on dark plates (11.7:1); the ink inner ring carries it on light tags (17.4:1).
- vital: #FF5D73 / HC #FF8C9D — Player health fill (3.41:1 vs track) and the low-health edge frame. A watermelon red, kept apart from the stamina Red tier and enemy red.
- vital-trail: #FFE08A / HC #FFFFFF — Damage-trail ghost segment on the player, enemy and boss health bars (7.85:1 vs track).
- heal: #5BE08A / HC #7CFFA8 — Healing lead segment on the health bar, '+N' heal numbers and the Restoration chip glyph.
- stamina-green: #3FCF78 / HC #4DFF88 — Stamina Green tier fill (5.0:1 vs track) and the '×1.25' pill ground (ink text 9.1:1).
- stamina-yellow: #FFD23F / HC #FFE600 — Stamina Yellow tier fill (7.0:1 vs track) and the '×1.0' pill (ink text 12.7:1).
- stamina-red: #FF6A3D / HC #FF8A5C — Stamina Red tier fill with ink stripes (3.56:1 vs track) and the '×0.75' pill (ink text 6.4:1). In the Exhausted state the same hue becomes the debt hatch: stripes on ink (6.7:1); the HC hatch is #FF5C5C on black (6.9:1).
- posture-steel: #B4C3D4 / HC #D6E2EE — Player and enemy posture bars (5.6:1 vs track). A deliberately neutral 'steadiness' metal so it never competes with the stamina tier colours beside it.
- sp-token: #52D4E6 / HC #4DF2FF — Spell Point tokens and the SP cost dots on spell slots (5.75:1 vs track). Empty tokens are a rim-tin ring over the track colour.
- good: #FFC93C / HC #FFE14D — Positive callouts (12.4:1 against their ink outline), reward stamps, Elite tier diamonds and rarity Gold. One gold for one meaning: worth having.
- bad: #FF6A55 / HC #FF8C7A — 'Parry Failed!', 'You Died', error bands, the low-health HP number and danger text: 5.9:1 on plate, 4.86:1 on raised (HC 8.0:1).
- foe: #FF5A4E / HC #FF7A70 — Enemy and boss health fill (3.29:1 vs track). One colour at every health level: the low-health warning comes from the trail and a 25% notch, not a hue shift.

## semantic_scales
All hex values are sRGB. Contrast was computed with the WCAG formula, not guessed. In UE, author each value as FColor or hex and convert it with FLinearColor::FromSRGBColor. Today's linear literals display much lighter than their comments say.

STAMINA TIERS. Every tier carries four cues: colour, the fill's position against notches cut at 33% and 66%, a multiplier pill and a fill pattern.
- Green: solid #3FCF78 (HC #4DFF88). Pill "×1.25", ink on green, 9.1:1.
- Yellow: solid #FFD23F (HC #FFE600). Pill "×1.0", 12.7:1.
- Red: #FF6A3D (HC #FF8A5C) with 25% ink diagonal stripes. Pill "×0.75", 6.4:1.
- Exhausted:
  - The fill is gone. A debt hatch of #FF6A3D / #0E1016 45° stripes (6.7:1) grows from the right as stamina falls from 0 toward −50 (HC #FF5C5C on black).
  - The pill inverts to an ink ground with #FF6A3D text "×0.5" (6.7:1) and a hatched rim.
  - The word "Exhausted" appears in the one-time hint.
  - Red and Exhausted share a hue on purpose. They are told apart by solid vs hatch, coloured vs inverted pill, and the multiplier.

RARITY (text on plate / text on raised row / ink text on the rarity band; HC on #161616):
| Rarity | Default | Plate | Raised | Band | HC | HC on #161616 |
|---|---|---|---|---|---|---|
| Common | #C9CED8 | 10.5 | 8.7 | 11.6 | #E6E6E6 | 14.5 |
| Green | #4FD67A | 8.9 | 7.3 | 9.8 | #7CF29C | 13.0 |
| Blue | #5CB2FF | 7.3 | 6.0 | 8.1 | #8CC8FF | 10.2 |
| Purple | #C495FF | 7.2 | 5.9 | 7.9 | #DDB8FF | 10.7 |
| Red | #FF6B5B | 5.9 | 4.9 | 6.5 | #FF9C8F | 9.0 |
| Gold | #FFC93C | 10.8 | 8.9 | 11.9 | #FFD966 | 13.2 |

Without colour, the rarity is counted:
- Every row, pickup label and tooltip carries a rarity tally badge: Common = empty ring, Green = 1 stroke, Blue = 2, Purple = 3, Red = 4, Gold = the full bundle (4 strokes + slash).
- The colour WORD is printed in the tooltip band ("PURPLE · Amulet") and in unidentified names.

FEAT TIERS. Separate hues from rarity, so "Gold" never means both Legendary feat and Gold item. Each tier has its own badge silhouette and its word printed on the badge ribbon.
| Tier | Default | Text on plate | Ink on badge | Badge shape | HC |
|---|---|---|---|---|---|
| Common | #C9D2DC tin | 10.9:1 | 12.0:1 | circle | #FFFFFF |
| Rare | #3FD6BC verdigris | 9.1:1 | 10.1:1 | hexagon | #5CFFE4 |
| Legendary | #FF6AD5 flare magenta | 6.5:1 | 7.2:1 | 8-point starburst | #FF9AE6 |

Rare and Legendary badges also carry a hatched "Drawback" box with a ▼ marker.

DAMAGE TYPES AND HEALING. These are world-space numbers with a 3 px ink outline; the ratio is fill vs outline. Each non-Physical number carries its stamp glyph, at 60% of cap height, after the digits.
- Physical, Slash/Pierce/Blunt: #F4F6F9, 17.6:1, no glyph (optional subtype tick).
- Fire: #FF8A3D, 8.1, flame.
- Frost: #8FE3FF, 13.2, snow-hex.
- Lightning: #FFE45C, 14.9, zigzag.
- Poison: #A8E05A, 12.2, droplet.
- Corrosive: #4FE3C1, 11.9, pitted ring.
- Arcane: #C79BFF, 8.7, 4-point sparkle.
- Shadow (Curse): INVERTED PRINT, a dark #2B1838 fill with a light #E7D3FF outline (11.75:1), plus a crescent glyph. Its silhouette is unlike any other number.
- Radiant (Holy): #FFF0A8 fill, 16.6, with an inner 2 px #FFC93C halo inside the ink outline and a sun-ray glyph.
- Healing: #5BE08A, 11.3, always prefixed "+".
- Crits: 1.5× scale, a starburst backing and a "!" suffix.
- Blocked hits: a small shield glyph at 80% scale. Weakness and resist: ▲ / ▼ marks. Immune: the word "Immune".
- HC colours (outline #000, all ≥10.4:1): #FFFFFF, #FFA060, #A8ECFF, #FFF07A, #C2F27A, #6FFFDB, #D9B8FF, Radiant #FFF7CC, Healing #7CFFA8, Shadow #000000 on #F0E0FF.

BUFF VS DEBUFF CHIPS:
- Buff: rim #45D483 (8.7:1 on plate; HC #4DFF88) on a chip with a raised notch tab on top.
- Debuff: rim #FF7A52 (6.45:1; HC #FF8A5C) on a chip with a pinked, sawtooth bottom edge.
- Every chip prints the status name and a unique stamp glyph; Frostbitten and Shocked show stack pips. Shape, name and glyph carry it; colour only confirms.

ENEMY TIER MARKS. Shown left of the name, #FFC93C diamonds with an ink outline (10.8:1 on plate; HC #FFE14D):
- Basic: no mark. Elite: 1 diamond. Champion: 2. Miniboss: 3, on a ribbon.
- Summon: a dashed rim-tin ring (#C5CFD9, 10.5:1) around the name pill, with the name in text-muted.
- Boss: no world mark; it gets the top-centre boss bar.
The count of marks is the code; colour is decoration.

## typography
THREE FAMILIES (Google Fonts, OFL):
1. Rowdies, 700 and 400: the display voice.
   - Chunky, rounded-shouldered letters with a slightly irregular, stamped weight, like lettering on a tin toy. Cheerful, but not comic-book, and not a Nintendo face.
   - Used only at 20 px and up: titles, callouts, banners, ticket kickers and the transient damage pops.
2. Atkinson Hyperlegible Next, 500 / 700 / 800: the UI and body voice.
   - Built by the Braille Institute for low-vision legibility: unambiguous I/l/1 and 0/O, open sturdy forms.
   - It underwrites the High Contrast promise without the Inter or Space Grotesk look.
3. Atkinson Hyperlegible Mono, 600 / 700 / 800: every number that changes.
   - Its monospaced digits never shift, which matters because Slate ignores OpenType tnum.
   - It shares a skeleton with Next, so mixed lines do not clash.
   - Its 'typed ticket' texture is the Tally's own handwriting.

Import the static TTFs from each Google download's /static folder; UE does not handle variable fonts well. Build composite fonts F_Tally_Display, F_Tally_UI and F_Tally_Mono, plus CommonUI text styles TS_Tally_<style>. Credits carry the OFL notices.

TYPE SCALE (design px at 1920×1080; ScaleToFit renders 0.666× at 720p. No text below 18 px, so nothing drops below 12 px at 720p):
| Style | Font, weight | Size/line | Used for |
|---|---|---|---|
| display-xl | Rowdies 700, +2% tracking | 72/80 | wordmark, "You Died" |
| display-l | Rowdies 700 | 48/56 | biome and boss banners, the level number |
| display-m | Rowdies 700 | 32/38 | "Paused", plate titles, dialog titles, boss name |
| callout | Rowdies 700, 4 px ink outline | 40/44 | "Perfect Dodge!" and the other callouts |
| kicker | Rowdies 400, uppercase, +6% | 20/24 | "FEAT UNLOCKED · RARE" |
| title | Next 700 | 24/30 | tabs, ticket main line, item names, list headers |
| body | Next 500 | 20/28 | descriptions, menu rows, prompts (prompt at 22/700) |
| body-s | Next 500 | 18/24 | remarks, status names, hints, inactive labels |
| num-xl | Mono 800 | 48/52 | death countdown, Skill Points token |
| num-l | Mono 700 | 28/32 | HP "100%", "Kills: 12" |
| num-m | Mono 700 | 22/26 | see below |
| num-s | Mono 600 | 18/22 | see below |

num-m covers SP "7 / 10", cooldown seconds, "Hit 3", "Rank 12 / 20" and attribute values. num-s covers durations, stacks, ledger "x3", "+9.4%" and keycap letters (at 800).

STEADY NUMERALS:
- Every counting TextBlock is set in Mono end to end: KillCountText, SpellPointsText, CooldownText, DurationText, CountdownText, ComboText and the new HealthPercentText and StaminaTierText.
- They sit in fixed-width boxes sized for their longest value: "100%" is 4 cells, "10 / 10" is 7 cells.
- Right-aligned for HP and SP, centred for cooldowns. Auto-size stays off (SetAutoSize(false)).
- Rowdies digits appear only in damage pops, which live 1.5 s and never tick in place.
- Signs are text ("+", "−"). Arrows, checks and ▲▼ are drawn as glyph images, not font characters, so glyph coverage is never a risk.

## shape_language
SIGNATURE CONSTRUCTION: the pressed-tin plate. It is three UMG layers in an Overlay:
1. A print shadow: the fill texture tinted ink at 55%, offset (0, 4) px, no blur.
2. T_UI_PlateFill: 64×64 white rounded rect, radius 12, Draw As Box, margin 0.25, tinted to the theme token.
3. T_UI_PlateFrame, pre-coloured:
   - a 3 px ink-outline outside;
   - a 2 px rim-tin bevel inside, 100% on the top and left edges, 35% on the bottom and right, so the plate reads 'pressed'.
The dual edge (ink outside, tin inside) is the core rule. One of the two always clears 3:1 on any floor: the worst case is 3.47:1, at a mid grey-green.

SIGNATURE MOTIF: the tally mark (four strokes and a slash). It is used for:
- skill rank bars: 20 strokes in 4 bundles, with the slashes on milestones 5/10/15/20 and the Rank 1 stroke capped with a dot;
- rarity badges: 0–5 strokes;
- SP tokens grouped in fives;
- the combo chain under the weapon name;
- the divider texture T_UI_TallyDivider, tiled;
- the loading throbber: 5 frames, the bundle drawn stroke by stroke;
- the Tally's seal: a round stamp with a bundle inside, the game's mark.

NOTIFICATION SHAPE: the hanging tag.
- T_UI_TagFill and T_UI_TagFrame: 128×64, Box margins left 0.375 (a fixed 48 px cap) and right 0.125.
- The left cap has 8 px 45° chamfers and a 12 px punched hole (alpha cut-out).
- The right end has a 10 px radius.
- A 2 px ink Image line is the string from the rail to the hole. Tags pivot at the hole.

OTHER PIECES:
- Keycaps: T_UI_Keycap, 48×48, radius 8, 2 px ink outline, a 4 px darker lip at the bottom (1 px when pressed), letters in Mono 800.
- Chips: pill radius = half the height.
  - Buff variant: a raised notch tab on top.
  - Debuff variant: a pinked, sawtooth bottom edge (T_UI_ChipDebuff).
- Bars: track radius 6 and fill radius 4, with a 2 px ink outline.
  - Stamina notches: 2 px ink ticks that overhang 3 px above and below.
  - Patterns come from M_UI_PatternFill: Pattern = solid, stripe or hatch, tiled in screen pixels via a T_UI_Hatch16 mask. It is used for the Red-tier stripes, the Exhausted debt, Drawback boxes and the cooldown shutter.
  - SP tokens: 22 px flat discs with a 3 px slot notch at the top, like a ticket punch, not gems.
- Radii: plates 12, tags 10, keycaps 8, cards 10, tooltips 10. No sharp corners anywhere except the tag chamfers.
- Strokes: world-facing outline 3 px (4 px HC), inner rim 2 px, dividers 2 px rim-tin at 40%, focus ring 3 px marigold + 2 px ink.
- Shadows: hard offset only. HUD (0, 4) at 55%; menus (0, 6) at 40%; pressed (0, 1). No blur, no glows, no glass.

ICONOGRAPHY: stamp icons.
- 48 px grid with a 36 px live area; 3 px ink outline (2 px at 32 px and below); at most 2 flat fills from the palette; 3 px corner rounding; no gradients; no interior line thinner than 3 px.
- At 48 px and up, a 2 px 'misprint' offset layer in a darker tone gives the lithograph feel; it is dropped at small sizes.
- Every status and damage glyph has a unique silhouette that works in monochrome: flame, seep drop, haze cloud, snow-hex, zigzag, dulled fist, cracked shard, muzzle ring, vine loop, eye-slash, shiver lines, smoke curl, target sigil, pitted shield, drip-wave, streak star.
- Icons are UImage with a brush tint, so HC can flatten them to white on black.
- No hearts, rupees, gems, scrolls or parchment.

BUILD NOTES:
- Theme tokens live in a UMordecaiUIThemeDataAsset (Default and HighContrast) that is read at construct time. The existing EditAnywhere colour UPROPERTYs are filled from it: stamina tiers, posture, feat TierColors, DamageTypeColors, HealingColor and StatusIconMap.
- Everything is textures, 9-slice Box brushes, Overlays and one pattern material. Nothing needs custom Slate.

## hud_layout
Canvas: 1920×1080, ScaleToFit. Edge margin 32 px; a HUD-margin option takes it to 96 for TVs.

ZONING PRINCIPLE:
- LEFT = you: state, always on.
- RIGHT-TOP = the Tally: news.
- CENTRE = now: combat callouts.
- BOTTOM-CENTRE = interact.
- BOTTOM-RIGHT = loadout.
- TOP-CENTRE = big moments: bosses and plates.

All BindWidgetOptional names are kept. Widgets may sit inside decorative containers, because binding is by name. The HUD moves from AddToViewport into UI.Layer.Game so menus and the death screen cover it.

ALWAYS VISIBLE
1. Gauge Plate. New decor widget 'GaugePlate', anchor TL (0,0), offset (32,32), 456×152.
   - Row labels "HP", "ST" and "SP" in num-s text-muted at x 48.
   - HealthBar (84,46) 300×24: vital fill, trail ghost, 25% notch.
   - New HealthPercentText (392,42) 80×32, num-l right-aligned "100%". It satisfies AC-052.2.
   - StaminaBar (84,80) 300×18, with notches at 33% and 66%.
   - New StaminaTierText pill (392,77) 80×24: "×1.25" / "×1.0" / "×0.75" / "×0.5".
   - PostureBar (84,104) 300×8: CONTEXTUAL. Shown while Posture < Max; fades out 3 s after it refills.
   - SpellPointsBar (84,118) 300×24: new SpellPointTokens HBox of up to 10 discs of 22 px, grouped in fives (16 px tokens if MaxSP > 10). SpellPointsProgressBar is restyled as a 3 px underline at y 144. SpellPointsText "7 / 10" (kept for the bridge) at (392,116) 80×28, num-m right-aligned.
2. Spell slots: SpellCooldownA, SpellCooldownB, plus C and D from US-088. Row at y 200, x 32 / 120 / 208 / 296, each 78×78.
   - Contents: a stamp icon (48); a keycap overlapping the bottom-left corner (Q / E / R / 1; gamepad LB / RB / — / D-pad Up); SP cost dots at top-right.
   - CooldownProgressBar becomes a top-down hatched shutter (ink 72%). CooldownText is centred num-m with a 2 px ink outline.
   - States:
     - Ready: a tin glint, 140 ms.
     - Cooling.
     - Unaffordable: hollow cost dots, icon at 40%, an "SP" lock chip.
     - Casting: marigold rim.
     - Silenced or blocked: a muzzle-ring glyph.
3. Rail head. New 'TallyRailHead', anchor TR (1,0), offset (−32,32), alignment (1,0), 320×56.
   - It hosts KillCounter: "Kills: 12" in num-l on the left.
   - At right, a pending badge (tally glyph + "+3"), shown only during the combat hold.
   - A 240×6 ink ticket slot on its bottom edge.
4. Loadout plate (EquippedWeaponWidget; fixes N3): anchor BR (1,1), offset (−32,−32), 340×84.
   - A weapon stamp icon in a well with a "Tab" keycap.
   - DisplayedWeaponName in title 24.
   - A sub-line with the damage subtype word and the light-chain tally (Longsword 3 strokes, Dagger 5); strokes light per landed hit.

CONTEXTUAL
5. Status chips (StatusEffectBar / IndicatorContainer, max 8): anchor TL, offset (32,292).
   - They wrap in a 456 px column, at most 2 rows of 36 px.
   - Each chip: glyph 24, StatusNameText body-s, DurationText num-s, stack pips. TintImage becomes the buff or debuff rim.
   - Sort order: Control > DoT > Debuff > Environmental > Buff. Hidden when empty. Focused shows its streak glyph with no timer.
6. Tickets hang under the rail head: x −32 right-aligned, from y 96, width 320, 8 px gaps, at most 4 hanging (down to about y 470).
   - Slip, 56 px: glyph + one line + an optional count.
   - Ticket, 88 px: kicker + line + remark.
7. Plates: top-centre anchor (0.5,0), offset (0,40), 640×128; drops to y 150 if a boss bar is up.
   - Used for: feat unlocked, technique (skill milestone), level up, upgrade key or signature item, new area.
   - Out of combat only.
8. Banners: biome title card / boss intro, a full-width strip anchored (0.5,0.28), 1920×132. Boss phase changes show as a strip under the boss bar.
9. CombatFeedback: anchor centre (0.5,0.5), offset (0,+128), alignment (0.5,0), 520×52.
   - Placed just below the player's feet, where the eye already is.
   - Letters only, with a 4 px outline; a plate behind them in HC. Shown 1.0 s; the latest replaces the previous one.
10. ComboCounter: anchor (0.5,0.5), offset (0,+184), 220×36. "Hit 3" in num-m 800 plus the weapon's chain as tally strokes. Shown during ComboWindow.
11. PickupPrompt: anchor BC (0.5,1), offset (0,−36), alignment (0.5,1), 540×60.
    - A tag-enamel tag with a rarity tally badge, and PromptText "[F] Pick up Unidentified Purple Amulet" in Next 700 22, ink.
    - InventoryHintText "[I] Inventory" in body-s on a small plate pill under it.
    - Interact prompts (Talk, Activate) reuse the same tag.
12. Quick items A–D and Throw: above the loadout plate, offset (−32,−128). Four 56×56 slots with keycaps 1–4 / D-pad and a "×3" count in num-s. Hidden when empty.
13. Boss bar: anchor TC (0.5,0), offset (0,32), 1000×96.
    - A display-m name on a nameplate tag.
    - At right, phase pips as 3 tally strokes. Enraged turns them into a starburst + an "Enraged" chip.
    - Health 1000×22 foe with trail; posture 1000×8 steel.
    - On a break, the posture bar floods with good/ink stripes + a "Broken" chip.
14. Enemy world-space indicator (US-092; retire WBP_EnemyHealthBar): bottom-centre anchored at Z+120, 150 wide.
    - Row 1: tier diamonds + NameText body-s on a plate pill (EnemyDisplayName, never the actor name).
    - Row 2: HealthProgressBar 140×10, foe, 2 px ink outline, trail.
    - Row 3: posture 140×5 steel. Broken → stripes + "Broken" chip. Knocked Down → "Down ×2.0" chip. Critical window → corner brackets around the enemy + a "Riposte" keycap.
    - Row 4: StatusIconContainer, up to 3 glyphs of 18 px + "+N".
    - Who shows it: Basic enemies only after aggro or damage; Elite and up always, within 2000 uu.
15. Damage numbers: Rowdies 700 30 px (crits 45 px) at the hit point; type colour + glyph; 3 px ink outline.
16. Hints attach to what they explain. Example: the low-stamina tag hangs from the stamina row for 4 s, once per session.
17. Low health (≤25%): HP number in bad, the gauge-plate rim pulses at 1 Hz, and a 12 px vital edge frame at 35%.

THE TALLY'S PACING RULES (the spine)
- Lanes: Callout (centre, never queued), Slip and Ticket (the rail), Plate (top-centre), Banner.
- Combat hold:
  - In combat = you dealt or took damage in the last 4 s, or an aggroed enemy is within 1500 uu.
  - While held, only callouts, numbers and the rail's "+N" pending badge show.
  - Player-initiated pickups still print, as compact 2 s slips.
- The receipt: after 2.5 s of calm, pending items print 350 ms apart: slips, then tickets, then plates one at a time (4 s each; Legendary 5.5 s).
- Merging: identical slips merge ("Iron Ore ×3"); repeated rank-ups merge.
- Caps: 4 hanging; more than 6 pending collapse into "6 more on the spike".
- History: everything is kept on the pause-menu Tally spike.
- Density setting: Full / Calm (no remarks; slips only out of combat) / Minimal (plates only).

## menus
FRONT END (replaces L_LyraFrontEnd; Mordecai* widgets so the bridge can click them)
- Title screen.
  - Background: a diorama vignette, a tilt-shift toy island with a slow dolly and no rotation (#58). No Lyra watermarks (#61).
  - The wordmark "PROJECT MORDECAI" in display-xl, pressed into a tin plate, with the Tally seal and a tally underline. Tagline "Everything counts. Nothing lasts."
  - Four 360×64 tags hang from a rail at x 160: "Play", "Options", "Credits", "Quit".
  - A dev-only version stamp in num-s at bottom-left.
  - Signature moment: on boot the tags drop one by one (90 ms stagger) and "Play" takes focus with a 3° swing.
- Play: "Choose an Expedition". One large tag card per playlist ("Mordecai Arena", "Dev test arena") with a thumbnail; later Continue / New. Signature moment: confirming punches the tag, and the screen prints into Loading.
- Options: the pause shell (1000×640) with sub-tabs Gameplay, Display, Audio, Controls, Accessibility.
  - Sliders are tally bars; toggles are keycap switches.
  - Accessibility lists first: High Contrast, Tally remarks, notification density, ticket duration, UI scale, HUD margin, Reduce motion, callout size.
  - Signature moment: a live preview ticket redraws as you change the settings.
- Credits: one long ticket printing out of the Tally slot at 40 px/s, including the OFL notices (#63).
- Quit confirm: a 600×260 plate on the dim. Title "Quit to desktop?", body "The Tally will keep count.", buttons "Stay" (default focus) and "Quit".
- Loading: opaque plate #1A1E2B with a faint tally pattern. "PROJECT MORDECAI" in display-xl, "Loading..." in body text-muted, the 5-frame tally throbber, and one remark or tip line. The tree is still built in Initialize().
- Dialogs: a 640-wide modal plate with a display-m title, a body, and keycap-tag buttons on the right. The error variant adds a bad band and a "!" stamp. It replaces W_ConfirmationDefault / W_ConfirmationError (#62).

PAUSE SHELL
- The dim-veil covers the frozen world; the HUD now sits under it.
- MenuColumn is 1000×640, centred:
  - TitleText "Paused" in display-m ink on a nameplate tag hanging at top centre.
  - TabBar: five 180×52 folder tabs with stamp glyphs. Active = tag-enamel with ink text, raised 6 px and fused to the content plate. Inactive = raised with text-muted. On gamepad, LB and RB keycaps sit at the ends of the bar.
  - TabContentBorder: a plate with a tin rim, padding 24.
- Footer below the column: "Esc Back", "I Close", "LB / RB Tabs".
- The Tally spike (x 1500–1880): the last 12 tickets, newest on top. Clicking one jumps to its tab.
- Signature moment: the shell drops in on its string.

THE FIVE TABS
- Character.
  - Header strip: a Level badge ("Level --" until levelling exists), a Skill Points token, and "Health 100 / 100" plus the other three resources in num-m.
  - Body: a 3×3 grid of 300×132 attribute cards in columns Physical / Resilience / Magical. Each card holds:
    - a keycap chip "STR", the name, and the base value in num-l;
    - the Effective Mod "+9.4%";
    - a 4-segment band meter (100 / 70 / 45 / 25%) showing the diminishing returns;
    - its two derived stats ("Physical Damage +28.2%", "Armor Penetration +14.1%"). Crit chances show as a raw "0.0%".
  - Signature moment: focusing a card strokes a tally line under its two derived rows.
- Skills.
  - Left: 200 px rail of the 5 category tags.
  - Centre: rows with the name, "Rank 12 / 20", the 20-stroke tally bar and an "Allocate" keycap (disabled with a lock at 0 points or Rank 20).
  - Right: a 330 px detail panel with 5 milestone rows: reached = filled stamp; next = outline + "3 ranks to go"; locked = dashed.
  - Header token "Skill Points: 3".
  - Signature moment: Allocate scratches in the next stroke, and finishing a bundle stamps the slash and fires the Technique plate.
- Feats.
  - Header "Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary".
  - Grid of badges (circle / hex / starburst) with the name and the tier word.
  - Locked: a dashed outline (text-disabled), the name, "???", and condition rows "TotalPerfectParries 32 / 50" with mini tally bars.
  - Detail panel with a hatched Drawback box.
  - Signature moment: an unlocked badge flips like a coin from blank to stamped.
- Inventory.
  - FilterBar (the same HBox) now spans the full content width: 8 glyph + label tags that fit in 968 px. This fixes #37.
  - Below it, ItemListBox (fill) and a 300 px right column: the focused item's tag card above the "Stored Resources" ledger (LedgerHeader, "Iron Ore  x1").
  - Rows: rarity tally badge, the name in its rarity colour, "  x3", "  [equipped]" + a pin glyph, the "? " prefix + a ? stamp, and an "Identify" keycap.
  - Empty states print as small tickets.
  - Signature moment: Identify plays the stamp reveal in the row.
- Settings: "Coming Soon" printed on a ticket until it becomes the Options sub-tabs.

ITEM TOOLTIP AND COMPARE
- A 360-wide tag card hanging from the focused row:
  - a rarity band with "PURPLE · Amulet" and the tally pips;
  - the name in title 24, then the type line;
  - Mono stats with deltas against the equipped item, "▲ +3" (#17703A, 5.6:1) or "▼ −2" (#B42318, 6.0:1);
  - the description and keycap actions.
- Unidentified: the line "Unidentified — the Mage Tower can read this.", stats shown as "???", and the ? stamp.
- Compare: two tags side by side; the equipped one is pinned and labelled "Equipped".

DEATH SCREEN
- The dim at 60% (HitTestInvisible) and a 560×300 plate hanging from the rail head:
  - DeathText "You Died" in display-xl, bad (5.9:1);
  - CountdownText in num-xl inside a round token;
  - KillCountText "Kills: 4" in num-l;
  - the remark "The Tally keeps your place."
- Signature moment: the plate prints out of the rail like a receipt of the life, and retracts on respawn.

TOWN SERVICE: MAGE TOWER IDENTIFY
- A full-screen service screen on UI.Layer.Menu, header "Mage Tower · Identify":
  - left: the sealed tags (? stamp, rarity tally, "Unidentified Purple Amulet");
  - centre: an appraisal tray with the selected tag enlarged on its string;
  - right: a cost line ("Free" today) and an "Identify" keycap. Cost and batch identify are TODO(DECISION).
- Signature moment: the tag spins once on its string, the rarity tally stamp strikes out the ?, the band floods from the hole, and the name types in Mono, then settles into Next.

MAP
- A tin tile board: rounded cell tiles on a plate with a desaturated biome tint, joined by pressed grooves.
  - Unexplored cells are blank and debossed; gates are keyhole stamps showing the required tool; the player is a token.
  - Cartography ranks add markers.
- Signature moment: newly charted cells flip over.

## motion
PRINCIPLES: wound, not floaty.
- Things hang on strings, drop, swing once and settle; stamps strike; keys press; tickets retract into the slot they came from.
- Combat-time motion lasts at most 160 ms in; celebrations wait until combat ends and run at most 600 ms.
- Only two things loop: the loading throbber and the 1 Hz low-health pulse.
- Every animation is a UMG widget animation or a material parameter, so it is buildable without code.
- Reduce Motion turns swings, stamps and shakes into 150 ms fades.

NAMED ANIMATIONS
1. TicketDrop (toast in):
   - Translate Y −28 → 0 over 220 ms, ease-out back (overshoot 1.4).
   - Rotation −5° → 0 as a damped swing over 420 ms, with one overshoot to +1.5°, pivoting at the punched hole.
   - Opacity 0 → 1 in 80 ms.
2. TicketRetract (toast out):
   - 180 ms ease-in cubic, Y 0 → −28 into the slot; opacity → 0 over the last 100 ms.
   - The remaining tickets reflow in 160 ms ease-out cubic.
3. DamageTrail:
   - Damage: the vital-trail ghost holds 350 ms, then drains to the new value over 450 ms ease-in-out quad.
   - Heals: a heal lead segment fills in 250 ms ease-out, and the main fill follows 300 ms later.
   - Enemy and boss bars use the same curve, shortened to 250 + 350 ms.
4. StampReveal (identify reveal, feat unlock, technique):
   - 0–110 ms: stamp scale 1.7 → 0.92, ease-in quad.
   - 110–230 ms: 0.92 → 1.0, ease-out back.
   - A 60 ms, 3 px plate shake.
   - The rarity band wipes from the hole end over 260 ms, ease-out cubic.
   - The name types on at 18 ms per character (capped at 450 ms).
5. TabSwitch:
   - Outgoing content: 90 ms fade + 12 px slide against the direction of travel, ease-in.
   - Incoming content: 160 ms slide 12 px in from the direction of travel + fade, ease-out cubic.
   - The active tab rises 6 px over 120 ms, ease-out back.
   - LB/RB wraparound slides in the wrap direction.
6. KeyPress (button press):
   - Press, 60 ms linear: Y +3 px, lip 4 → 1 px, shadow 4 → 1 px.
   - Release, 120 ms ease-out back.
   - Hover lifts 2 px over 100 ms ease-out.
7. CalloutPop:
   - Scale 0.6 → 1.12 → 1.0 over 160 ms ease-out back, hold 640 ms, fade 200 ms (1.0 s in total, as shipped).
   - Negative callouts add a 3-cycle, 6 px horizontal shake over 180 ms.
8. ReceiptPrint: when the combat hold releases, tickets emit 350 ms apart, each with a print tick.

## accessibility
HIGH CONTRAST THEME (Settings > Accessibility, shipped as a second theme DataAsset)
- Plates go opaque black and tags pure white; text is white or black.
- Every text/ground pair is 7:1 or better, all computed. The lowest is HC rarity Red on a raised row at 8.97:1; most pairs are 9–21:1.
- Outlines go to 4 px, rims to white, and the focus ring to #FFFF00 (19.6:1).
- Bars get white 2 px rims over black wells.
- Callouts sit on plates, and patterns run at full strength.

DEFAULT-THEME CONTRAST (computed with the WCAG formula):
- Text: on plate 15.3:1; muted 7.9:1; disabled 5.5:1 (4.55:1 on raised rows).
- Tags: ink on tag 16.7:1; remark 7.2:1.
- Every rarity colour is at least 4.9:1 on raised rows, and every coloured pill or band is at least 6.1:1 with ink text.
- Damage numbers are at least 8.1:1 against their own outline.
- The dual edge guarantees 3.47:1 against ANY floor colour (4.6:1 in HC), so plates never vanish on light floors, snow or dark swamp.
- Rule: no text sits directly on the dim or the world. Text is always on a plate or a tag.

COLOUR IS NEVER THE ONLY SIGNAL:
- Rarity: tally badge plus the colour word.
- Stamina tiers: notches, multiplier pill, solid / striped / hatched fill, inverted Exhausted pill.
- Statuses: name plus a unique glyph; buffs have a notched top, debuffs a pinked edge.
- Damage: type glyphs; Shadow uses inverted print, Radiant a halo, healing a "+".
- Feats: badge silhouettes plus the tier word.
- Enemy tiers: diamond count; summons a dashed ring.
- Deltas: ▲/▼ plus the sign.
- Disabled controls: a lock glyph.

READABILITY:
- Minimum text is 18 design px (12 px at 720p).
- UI scale runs 80–150%. Atkinson Hyperlegible is used for all UI text, and Mono for all live numbers.
- Callout size has Large and XL options.

NOTIFICATION CONTROL (the angle's own accessibility):
- Density: Full / Calm / Minimal.
- Tally remarks on or off.
- Ticket duration ×1 / ×1.5 / ×2.
- "Hold plates until I pause".
- The full history lives on the pause-menu spike, so nothing can be missed.

MOTION AND FLASH:
- Reduce Motion: no swing, shake or stamp; 150 ms fades instead.
- Nothing flashes faster than 3 Hz; the low-health pulse is 1 Hz.
- Damage-number density: All / Mine / Off.

INPUT:
- Device-aware keycaps through CommonInput (KBM / Xbox / PS), replacing the hard-coded "[F]" once the playtests are updated together.
- Every control is reachable by gamepad focus and has the 3 px + 2 px ring. Mouse targets are at least 48 px.
- No hold-to-confirm.

SOUND REDUNDANCY: each lane has its own sound: slip tick, ticket print, plate stamp, negative thunk, and the receipt run.

TEXT STAYS TEXT: all text remains real UTextBlocks. The bridge and future narration (accessible text on every ticket: headline + cause) can read it. Icons carry accessible names.

COLOUR-BLIND CHECK: verify with r.Color.Deficiency (protanopia / deuteranopia / tritanopia) on packaged screenshots. The stamina Green and Red tiers converge under deuteranopia by design: the pill and pattern carry the tier.

## risks
WHAT COULD GO WRONG
1. Tone clash. A wry counting machine could undercut a serious "Impermanence vs Control" world.
   - Mitigations: remarks are optional, never in combat, and banned on boss intros, biome cards, endings and status lines.
   - Jeff should read 20 sample remarks before any are written into data.
2. The combat hold severs cause and effect. A feat earned mid-fight that appears 10 s later can feel random.
   - Mitigations: every ticket carries a plain cause line ("Burned 6 enemies at once."), and the rail's "+N" badge proves something happened.
   - Test whether players connect them.
3. Light tags on light ground. The prompt, tickets and active tab rely on the 3 px ink outline and print shadow. At 720p (0.666×) the outline becomes 2 px and the 2 px rim becomes about 1.3 px, which may smear.
   - Fallback: a 4 px outline baked at 1080.
4. Rowdies may read cartoonish at banner size next to the Lich King.
   - Fallback: keep Rowdies for callouts, kickers and pops; set banners in Atkinson Next 800.
5. Font availability.
   - Confirm "Atkinson Hyperlegible Next" and "Atkinson Hyperlegible Mono" on fonts.google.com and import the static TTFs, not the variable ones.
   - Fallback pair: "Atkinson Hyperlegible" (400/700) + "Red Hat Mono".
6. Colour collisions. These reds are near-identical: vital, foe, rarity Red, bad and stamina Red. Gold is shared by good, rarity Gold and Elite.
   - They are separated by context, shape and words.
   - Verify with CVD simulation on real frames.
7. Status names always visible versus pillar 8 ("not icon soup").
   - A Compact chips option exists, but names must stay visible by default: playtests compare "StoneSkin" and "Burning".
8. System assumptions.
   - The rank-up stream assumes ranks arrive as events. Today ranks are bought with skill points (Allocate), and "levelling from use" is not a decision yet: TODO(DECISION).
   - Level-up and XP have no system yet.
9. Structure.
   - New optional widgets (HealthPercentText, StaminaTierText, SpellPointTokens, TallyRailHead, GaugePlate) need C++ fallbacks.
   - KillCounter and FilterBar move into new containers. Their names and types must survive the move.
   - The HUD must move into UI.Layer.Game, or the pause and death designs draw under it.
   - The bridge cannot assert colours or fills (B2), so this direction is verified only by reading screenshots.

WHAT TO TEST FIRST
1. A static paint-over of the gauge plate, a ticket, the prompt tag and a callout on fresh packaged 1280×720 frames: DevTestMap's white floor, plus green, sand, snow and dark-swamp swatches. Read them at 720p.
2. A Tier 4 scripted fight that triggers 2 rank-ups, a pickup and 1 feat. Measure the ticket count during the hold and the time to receipt, and check that nothing covers enemies on the right third.
3. Run the HC theme and CVD simulation over the same frames.
4. A tone read of the remark list with Jeff.

