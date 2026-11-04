## name
Still Running

## thesis
The diorama is a living toy world. The interface does not belong to it: it is laid over the world by the Registry, the Ancient Age's automated administration that the Continuants still maintain. The Registry tags, numbers and files everything you do, and its stamped motto is the pillar itself: "Everything you do is a skill." DCC shows up as the Registry's voice and appetite: frequent, deadpan, over-precise slips printed from a slot at the top right, numbered deeds, and commendations for things nobody asked it to count. Link's Awakening shows up as the toy diorama itself and as the UI's physicality: chunky rounded enamel tiles with solid toy depth that click and thunk like game pieces. Because the UI is the machine layer, it can carry the theme to the end: in the Acceptance ending the Registry fails, plates go dark and slips stop printing, so the interface literally lets go.

## voice
TWO VOICES
1. Plain UI voice (prompts, buttons, labels, hints, status taglines): short, imperative, sentence or title case as shipped. Every shipping string stays exactly as it is: "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!", "Hit N", "Kills: N", "You Died", "Paused", "Character / Skills / Feats / Inventory / Settings", "Coming Soon", "Inventory is empty.", "No items match this filter.", "No resources stored.", "Stored Resources", "Identify", "[F] Pick up {0}", "[I] Inventory", "x / y", "Loading...", "Project Mordecai" (ToUpper in display only, so GetText still returns the shipped string), "Feats: N unlocked — ...", "rank / 20". The status identities are used verbatim as tooltip and tag copy ("You're on a clock", "Recovery is denied", "Next mistake hurts more", ...).
2. The Registry (system notices, slips, death record, level, skill and feat events, biome and boss cards): the Ancient Age's still-running administration. Rules:
- Third person ("The Registry has..."), never "I".
- Present tense, exact numbers, official verbs: recorded, filed, issued, noted, amended, regrets.
- Indifferent, not cruel. DCC's AI enjoys your pain; this one does not notice it. The humour comes from caring precisely about the wrong thing, which is the theme of Control.
- At most two lines: the fact first (name, rank, tier), the flavour second. "Registry chatter: Brief" removes the flavour line; "Off" keeps only critical slips.
- It never comments on story beats, NPC loss or the Lich King.
- Bracketed mono notation is how the system writes keys: [F], [I], [Esc], [LB].
- Flavour lines are always NEW TextBlocks, added beside the shipping strings and never replacing them.

8 SAMPLES
1. Feat unlocked (slip): header "FEAT RECORDED" / "Pyromaniac — Common" / "Six enemies burning at once. Filed under: fires, intentional."
2. Skill rank-up (slip): header "PRACTICE NOTED" / "Dodging — Rank 4 / 20" / "Harm avoided: 112 times. Keep avoiding it."
3. Skill milestone (slip): header "MILESTONE ISSUED" / "Longswords — Rank 5 / 20" / "Technique on file: charged spinning heavy."
4. Identify reveal (Mage Tower card, then slip): "Unidentified Purple Amulet" resolves to "Mysterious Amulet"; slip "ENTRY AMENDED" / "Mysterious Amulet — Purple" / "The Registry regrets the earlier vagueness."
5. Level up (slip): header "LEVEL 7" / "+1 skill point issued" / "Unspent points are also counted."
6. Death screen line (under "You Died"): "Dying is not a skill. The Registry checked."
7. Low-stamina hint (plain voice, once per session, beside the stamina chip): "Exhausted — everything at half strength. Stop spending to recover."
8. Main menu tagline: "Everything you do is a skill. The machines are still counting."

MORE LINES FOR TONE
- Pause subline: "Registry suspended. Nothing is being counted."
- Loading: "Indexing region..."
- Quit confirm: "Quit to desktop?" / "The Registry will keep counting."
- Biome cards, one Registry status per theme quote:
  - Verdant Wilds: "Status: nominal. Everything is fine."
  - Searing Expanse: "Status: orderly. Chaos removed."
  - Mire of Echoes: "Status: records sealed."
  - Crystal Caverns: "Status: preserved. No changes permitted."
  - Storm Peaks: "Status: ascending."
  - Withered Kingdom: "Status: held. Indefinitely."
- Boss card: "IRON WARDEN" / "Enforcing a pattern since the Ancient Age. Deviation will be corrected."
- Auto-stored loot is aggregated, never vomited: "Stored: Iron Ore x2, Town Stone x1". Number signs use "No." (Atkinson has no numero glyph).

## palette
- plate: #10201D / HC #000000 — Enamel ground of every HUD plate, menu panel, tag and chip (dark verdigris-black). All HUD text sits on an opaque plate, never on the floor. Against floors: 16.85:1 on a white floor, 13.27 light blockout, 7.87 grass, 11.35 sand, 6.03 water. On dark floors the rim carries the edge.
- plate-raised: #1A2E2A / HC #161616 — Hovered or selected list rows, tooltip bodies, hovered index tabs. Text 12.91:1 (HC 18.1).
- plate-sunk: #070F0D / HC #000000 — Bar tracks, wells, inactive index tabs, cooldown shutters, SP pip sockets. Text 17.5:1.
- keyline: #040908 / HC #000000 — Outer 2 px stroke of every plate and slip, the 4 px solid toy drop shadow, and the 3 px outline on world damage numbers. It is the separator against light floors.
- rim-verdigris: #5DB8A1 / HC #FFFFFF — Inner 2 px rim on every plate, engraved notches (stamina 33/66, boss phase posts), ledger dotted leaders. 7.09:1 on plate, 6.02 on plate-raised, 6.54 vs a night floor (#1E2430), 3.08 vs a mire floor (#4E5A45). With the plate it guarantees an edge of at least 3:1 on any floor.
- lamp-focus: #FFB547 / HC #FFD400 — The Registry lamp. Uses: gamepad/keyboard focus ring (2 px, offset 2 px), the active index-tab lamp bar, the cooldown-ready ping, positive combat feedback text ('Perfect Dodge!'), slip-stack header text on plate. 9.59:1 on plate, 8.14 on raised (HC 14.67/12.64).
- text: #EEF5F0 / HC #FFFFFF — Primary porcelain text and the health damage-trail chunk. 15.21:1 on plate, 12.91 raised, 17.5 sunk. Over the 80% scrim on a white floor: 9.71.
- text-muted: #A9BFB7 / HC #E6E6E6 — Secondary text: Registry flavour lines, [I] Inventory hint, subtitles, derived-stat labels. 8.68:1 on plate, 7.37 raised, 5.54 over the scrim (HC 16.83/14.5). It never sits on the bare floor.
- text-disabled: #8EA39C / HC #C8C8C8 — Locked feats, maxed or unaffordable Allocate, empty quick slots. Always paired with a lock or dash glyph. 6.31:1 on plate, 5.36 raised (HC 12.55/10.82).
- slip-paper: #E3EEE7 / HC #FFFFFF — Registry slips: notification toasts, the death record, confirmation dialogs, credits ledger. A cool porcelain-celadon, not cream. It always has a 2 px keyline border because it is only 1.07-1.25:1 against light floors; on mire and night floors the paper itself reads (6.14 / 13.07).
- slip-ink: #10201D / HC #000000 — All text on slips. 14.16:1 (HC 21). Also the label colour on filled semantic chips (ink on lamp-focus 9.59, on rim 7.09, on danger 6.14).
- seal-red: #B0271C / HC #9E0000 — The Registry's stamp ink on slips only: slip headers ('FEAT RECORDED'), the 'YOU DIED' stamp, the error-dialog 'NOTICE'. 5.6:1 on slip-paper (HC 8.56). Never used on plates.
- health: #FF5C72 / HC #FF8095 — Player and enemy health fill (vital rose, kept distinct from stamina orange). 6.49:1 against the plate-sunk track (HC 8.77). The % readout itself is in text, or danger under 25%.
- stamina-green: #46D485 / HC #5CF29A — Stamina Green tier fill (66% and up, x1.25) and its '×1.25' chip text. 10.17:1 vs track, 8.84 on plate (HC 14.62).
- stamina-yellow: #E6D84A / HC #FFF04D — Stamina Yellow tier (33% and up, x1.00). Shifted to lemon so it never reads as the amber focus lamp. 13.19:1 vs track, 11.46 on plate.
- stamina-red: #FF8A3D / HC #FFA05A — Stamina Red tier (above 0, x0.75). 8.27:1 vs track, 7.18 on plate. The same hex as the Fire damage number; the contexts never overlap.
- stamina-exhausted: #C2503F / HC #FF7A66 — The Exhausted tier (at or below 0, x0.50, down to the -50 floor), always drawn as 45° hatch stripes alternating with plate-sunk. 4.17:1 vs track (HC 8.23). It is never a text ground (porcelain on it is only 4.2:1); the 'Exhausted ×0.50' label sits on the plate.
- posture: #7CC6FF / HC #8FD3FF — Player posture bar and the enemy posture sub-bar (ice blue). 10.5:1 vs track. Broken switches to danger hatch plus the word.
- spell-points: #B996FF / HC #CDB3FF — Lit SP lamp pips and spell cost numerals. A flat violet, no gradient. 8.19:1 vs socket, 7.12 on plate (HC 11.49).
- danger: #FF6E61 / HC #FF8A7E — Debuff chip rim, 'Parry Failed!', broken posture, boss enrage rim, the health readout under 25%, insufficient-SP cost text, Miniboss marks. 6.14:1 on plate, 5.21 raised (HC 9.19/7.92).
- buff: #62E3A5 / HC #7CFFC0 — Buff chip rim (Stone Skin, Focused, Blessed...), the heal lead-in on the health trail, positive stat deltas. 10.47:1 on plate (HC 16.92).
- scrim: #070F0DCC / HC #000000D9 — Modal dim under the pause menu, death record, dialogs and service screens: plate-sunk at 80% (HC black at 85%). Over a white floor it becomes #393F3D, where text is 9.71:1 and muted 5.54 (HC 15.13/12.13). The dim is raised from today's 50-65% because the floors are light.

## semantic_scales
All values are sRGB hex, converted with FLinearColor::FromSRGBColor. Contrast was computed with the WCAG formula, not estimated. Default theme minimum: 4.5:1 for text, 3:1 for graphics. HC minimum: 7:1 for text, 4.5:1 for graphics.

STAMINA TIERS (fill / chip text on plate)
| Tier | Rule | Default | HC | Multiplier |
|---|---|---|---|---|
| Green | 66% and up | #46D485 | #5CF29A | ×1.25 |
| Yellow | 33% and up | #E6D84A | #FFF04D | ×1.00 |
| Red | above 0 | #FF8A3D | #FFA05A | ×0.75 |
| Exhausted | 0 or below | #C2503F hatch | #FF7A66 hatch | ×0.50 |
Non-colour cues:
- Two engraved rim notches on the track at 33% and 66%, so position shows the tier.
- A mono chip shows the actual multiplier ('×1.25'); it is the tier name a player can use.
- Exhausted fills right-to-left with 45° hatch as stamina debt grows toward -50. Its label reads 'Exhausted ×0.50'.

RARITY (item-name text on plate / plate-raised)
| Rarity | Default | Contrast | HC |
|---|---|---|---|
| Common | #C5D1CC | 10.72 | #FFFFFF |
| Green | #5FDB7C | 9.56 | #7CFF9A |
| Blue | #74AEFF | 7.41 | #A6CBFF |
| Purple | #C68FFF | 7.04 | #DDB8FF |
| Red | #FF6E6E | 6.18 (5.25 on raised) | #FF9C9C |
| Gold | #FFD24A | 11.69 | #FFE070 |
Every HC rarity is 9.04:1 or more.
Non-colour cues:
- The rarity WORD (already in unidentified names, always in the tooltip header).
- 'Grade notches': 0-5 engraved 4×14 px bars left of every item name (Common 0 ... Gold 5), so order reads even on identified items like 'Mysterious Amulet'.
- Unidentified rows add a wax-seal glyph and keep the shipped '? ' prefix.

FEAT TIERS (unified with rarity so 'gold = top' means one thing everywhere)
| Tier | Default | HC | Seal |
|---|---|---|---|
| Common | #C5D1CC | #FFFFFF | single ring |
| Rare | #74AEFF | #A6CBFF | double ring |
| Legendary | #FFD24A | #FFE070 | 12-tooth cog ring |
The tier word is always printed. Locked feats get a dashed, unstamped seal. Drawback lines for Rare and Legendary get a hatched danger edge.

DAMAGE TYPES (world number fill + 3 px keyline outline; fill vs outline is 8.55:1 or more, HC 10.43 or more)
| Type | Default | HC | Glyph |
|---|---|---|---|
| Physical | #F4F8F5 | #FFFFFF | Slash = blade, Pierce = point, Blunt = hammer |
| Fire | #FF8A3D | #FFA05A | flame |
| Frost | #86DCFF | #A8E8FF | snowflake |
| Lightning | #F7EE5A | #FFF66E | bolt |
| Poison | #9BE35A | #B4F27A | droplet |
| Corrosive | #4FE6C3 | #6FF5D5 | bubbling drip |
| Arcane | #C9A2FF | #DDC2FF | spiral rune |
| Shadow / Curse | INVERTED: fill #2B1530, outline #F0D9F5 (12.73:1) | fill #000000, outline #FFFFFF | crescent-eye |
| Radiant / Holy | #FFC94A | #FFD970 | sun |
| Healing | #7CF5B8 | #9CFFCC | '+N' prefix |
Corrosive is verdigris, which ties it to copper corrosion. Shadow's inverted 'negative print' is a shape cue no other type has. Every number carries its type glyph (18 px icon, left), so Lightning vs Radiant and Poison vs Healing never rely on hue. Healing also always has the '+' prefix and drifts down-left instead of up. Crits use 1.5× size (48 px) plus a 4-corner tick frame image.

BUFF vs DEBUFF chips (name + mono seconds in text on plate)
- Buff: rim #62E3A5 (HC #7CFFC0). The chip has a raised tab on its top edge, and a category glyph (up-chevron).
- Debuff: rim #FF6E61 (HC #FF8A7E). The chip has an 8 px chamfered bottom-right corner, and a category glyph: DoT = drop, Debuff = down-chevron, Control = shackle, Environmental = wave.
- Debuffs sort left of buffs.
- The status name is always printed. The identity tagline shows in the tooltip and the menu.

ENEMY TIER MARKS (on the enemy's Registry tag; colours on plate)
| Tier | Mark | Colour | Contrast |
|---|---|---|---|
| Basic | none | — | — |
| Summon | hollow ring + dashed tag rim ('temporary registration') | #A9BFB7 | 8.68 |
| Elite | 1 chevron | #EEF5F0 | 15.21 |
| Champion | 2 chevrons | #FFD24A | 11.69 |
| Miniboss | 3 chevrons, tag at 1.25× size | #FF6E61 | 6.14 |
| Boss | no tag; top boss bar with the name in stencil, phase posts and a 'PHASE n' chip | — | — |
Enraged: hatched danger rim.
Every tier differs by mark count or shape, so colour is redundant.

Enemy posture states:
- Broken: hatched danger posture bar + '×1.5' chip.
- Knocked Down: 'Down ×2.0' chip.
- Critical/riposte window: amber bracket corners around the tag, pulsing at 4 Hz.

## typography
THREE GOOGLE FONTS FAMILIES (OFL; names checked against the fonts.googleapis.com css2 API, which returns static per-weight TTFs to import)
1. Big Shoulders Stencil (opsz 10-72, wght 100-900). Display only, 28 px and up, uppercased with TextTransformPolicy ToUpper so GetText keeps the shipped string ('Paused', 'You Died', 'Project Mordecai').
   - Why: condensed civic signage cut as a stencil, so every headline looks spray-stamped by the old administration. Condensed width fits long names ('TEMPEST ASCENDANT', 'WITHERED KINGDOM') in boss bars and cards.
   - Not Orbitron sci-fi: this is municipal machinery, not the future.
2. Atkinson Hyperlegible Next (wght 200-800). All UI prose: labels, tabs, buttons, prompts, descriptions.
   - Why: the Braille Institute's low-vision face. Institutional (fits the administration), with unambiguous letterforms that survive 0.666 scaling at 720p.
3. Atkinson Hyperlegible Mono (wght 200-800). Every number that changes, plus ledger values.
   - Why: the same skeleton as Next, so a row of words and numbers reads as one voice, and its 0/O, 1/l/I, 5/S and 8/B are disambiguated.

WHY NUMERALS STAY STEADY IN SLATE (no tnum)
- Measured from the TTFs:
  - Atkinson Hyperlegible Mono: every glyph advance is 632/1000 em (digits, %, /, space).
  - Atkinson Next digits are proportional (440-659 units).
  - Big Shoulders Stencil digits are proportional (560-1096 units).
- Rules:
  - Live numbers use only Mono, right-justified in a fixed-width SizeBox sized for the maximum string ('100%' = 4 ch; '10 / 10' = 7 ch; '15.0' = 4 ch). At 24 px one Mono character is 15.2 px.
  - Never animate number scale.
  - Optional odometer 'ghost zeros' are a decorative Image behind the text, not a TextBlock, so the playtest bridge reads only real values.
  - Stencil numerals appear only in static strings ('LEVEL 7', 'PHASE 2').

UMG FONT SIZE
- UE font sizes are points rendered at 96 DPI, so UMG Size = px × 0.75.
- Line height uses UTextBlock LineHeightPercentage.
- Letter spacing is in 1/1000 em.

TYPE SCALE (1920×1080 design px)
| Style | Face | px (UMG pt) / line height | Weight | Use |
|---|---|---|---|---|
| display-xl | Stencil | 96 (72) / 88 | 800, +40 | title wordmark, 'YOU DIED' |
| display-l | Stencil | 64 (48) / 60 | 800, +40 | biome card, boss intro name |
| display-m | Stencil | 40 (30) / 40 | 700, +30 | 'PAUSED', screen titles |
| display-s | Stencil | 28 (21) / 28 | 700, +40 | boss-bar name, slip headers 'FEAT RECORDED'; the floor for stencil |
| heading | Next | 28 (21) / 34 | 700 | panel headings, weapon name, tooltip item name |
| body-l | Next | 24 (18) / 32 | 500 | pickup prompt, list rows, menu buttons |
| body | Next | 20 (15) / 28 | 400 | descriptions, Registry flavour lines, tooltips |
| label | Next | 20 (15) / 24 | 700, +20 | tabs, filter chips, buttons, status names, HUD labels; shipped title case, no forced caps |
| num-xl | Mono | 72 (54) / 72 | 700 | death countdown |
| num-l | Mono | 32 (24) / 32, crit 48 (36) | 800 | damage and heal numbers (3 px keyline outline via FontOutlineSettings, or a baked SDF digit atlas for Lyra's Niagara number pops) |
| num-m | Mono | 24 (18) / 28 | 700 | health '74%', 'Kills: 12', 'Hit 3', cooldown seconds on keys |
| num-s | Mono | 20 (15) / 24 | 500 | 'x / y' SP, '×1.25', status timers, stat values, ledger counts |

Minimum text anywhere is 20 px (13.3 px at 720p). Arrows, stars, checks and the numero sign are missing from all three fonts, so they are images.

## shape_language
SIGNATURE: THE REGISTRY PLATE
A machine-issued enamel tile with toy thickness. From outside in:
- 2 px keyline (#040908)
- 2 px verdigris rim
- 1 px top-inner highlight (rim at 35%)
- the plate fill
Shape and depth:
- Outer corner radius 10 px: soft like a toy tile, never pill-round.
- A SOLID 4 px drop of keyline straight down: the tile's physical thickness. Never a blur.
- Plates over 200 px wide get four 4 px rivets inset 8 px from the corners. Smaller plates get none, to avoid noise.
- A pressed plate drops 3 px (shadow 4 to 1). This 'toy click' is the LA half.
- The keyline + rim pair is what makes every plate read on both light floors (the plate body is 6-17:1) and dark floors (the rim is 3.1-6.5:1).

BUILD IN UMG
- Option A: a 9-slice Box brush. One greyscale mask texture T_UI_Plate (64×64, margins 0.25; the bottom margin includes the 4 px drop) with channels R = fill, G = rim, B = keyline. M_UI_Plate (UI domain) lerps the theme colours from MPC_UITheme, or from dynamic material instance parameters, so the High Contrast swap is one parameter push.
- Option B, for odd sizes: an SDF rounded-rect material with the same parameters.
- Text colours come from a UMordecaiUITheme DataAsset of sRGB hex tokens (FromSRGBColor), applied on construct and on theme change.

FAMILY OF PARTS (all 9-slice or Image)
- Slip: the Registry's printout. Slip-paper ground with a 2 px keyline on sides and bottom. The top edge is perforated (6 px rounded notches) where it tore from the printer. Seal-red stamp mark top-right, rotated -4°.
- Index tab: rounded top corners (10 px), square bottom. The active tab merges into the panel (no bottom rim) and carries a 3 px amber lamp bar. Inactive tabs are plate-sunk and sit 4 px lower.
- Keycap: KBM keys are a porcelain rounded square (28×28, r 6, 3 px drop) with ink mono letter. Gamepad faces are round, shoulders and triggers are pills, so the device shape differs too.
- Bar: a plate-sunk track with a 1 px inner shadow. Flat fill (no gradient) with a 2 px gloss line on top at 30% white (toy sheen). Square ends. Engraved rim notches for thresholds. Broken, exhausted or enraged states use a 45° 4 px hatch.
- SP lamps: 10 discrete 22×18 pill sockets, lit or unlit. The bound UProgressBar uses a fill material with a SegmentCount parameter, so it renders as pips.
- Chips: buff = top tab; debuff = chamfered corner. No left-border accent strips anywhere.
- Seals (feats): ring, double ring, 12-tooth cog.
- Grade notches: 0-5 bars for rarity.
- Registry tag: the world-space tag on enemies and pickups. A small plate with a 6 px notch at top centre, tethered to its owner by a 2 px rim line, like a toy price tag.

PATTERNS
- Hatch: warning, debt, broken.
- Dotted leaders: ledgers, derived stats, credits.
- Engraved 32 px grid: map, loading screen.
- A faint Continuant cog motif: loading and title only.

ICONOGRAPHY
- 24 px grid, 2 px rounded strokes (3 px at 32 px and up), chunky toy silhouettes that read at 16 px, single-tint so the theme recolours them.
- Filled variants mean active or on. Engraved (stroke-only) variants mean inactive or locked.
- No hearts, no rupee-like gems, no Nintendo shapes. The health glyph is a notched porcelain rhombus ('vital mark').

SHADOWS AND EFFECTS
- Only hard offset shadows. No blur, no glass, no gradients.
- UI materials avoid translucency stacking, so the HUD stays a few draw calls.

## hud_layout
CANVAS
- 1920×1080 design px, ScaleToFit (0.666 at 720p).
- 24 px safe margin, with a HUD-inset option of 0-48 px.
- Every HUD text sits on an opaque plate. The only plate-less text is world damage numbers, which carry a keyline outline.
- Recommended: move WBP_CombatHUD from AddToViewport into the CommonUI layer stack (UI.Layer.Game), so the pause menu, death record and dialogs cover it. Today the HUD draws over their dims.
- Slot names below are the shipping BindWidgetOptional names. NEW marks additions that need a small code change.

ALWAYS VISIBLE
1. Vitals plate ('the Register'). Anchor TL (0,0), offset (24,24), size 432×132, alignment (0,0). It holds HealthBar, StaminaBar, PostureBar and SpellPointsBar.
   - Rows: a 24 px glyph column at x 16; bars at x 56, width 256; readouts right-aligned in a 96 px mono column at x 320.
   - Health: bar 256×24 at y 16. Readout '74%' in num-m (NEW HealthPercentText; AC-052.2 already requires a %).
     - Damage trail: the porcelain chunk holds, then drains.
     - Under 25%: the readout turns danger and the plate rim pulses danger at 1 Hz.
   - Stamina: bar 256×16 at y 50, with engraved notches at 33% and 66%. Tier chip '×1.25' in num-s in the tier colour (NEW StaminaTierText).
     - Exhausted: hatch debt fill and the chip reads 'Exhausted ×0.50'.
   - Posture: thin bar 256×8 at y 76.
     - Broken: danger hatch and a 'Broken' label.
   - Spell Points: 10 lamp pips at y 94 (22×18 each, 4 px gaps). SpellPointsText 'x / y' in num-s, right column.
2. Spell keys. Anchor TL, offset (24,168), four 92×92 key plates with 8 px gaps (392 wide). Slots SpellCooldownA, SpellCooldownB and NEW C, D (US-088).
   - Each key: keycap glyph top-left (Q, E, R, 1 / LB, RB, -, D-pad Up), a 44 px spell icon, and the SP cost in num-s bottom-right.
   - CooldownProgressBar is a 6 px recharge strip along the bottom. CooldownText '1.4' in num-m is centred over a dimmed icon.
   - Ready: an amber ping on the rim.
   - Not enough SP: the cost turns danger and gets a hatch strip.
   - Silenced: all four keys hatched with a lock glyph.
   - A full-key shutter needs a NEW material hook, because UProgressBar has no radial fill.
3. Weapon plate. Anchor BR (1,1), offset (-24,-24), alignment (1,1), size 360×80. NEW slot that moves the EquippedWeapon widget into the HUD (US-091).
   - Weapon glyph 48 px, the name 'Longsword' in heading style, a muted sub-line 'Slash · 3-hit chain', and a '[Tab]' keycap.
4. Registry slot (KillCounter). Anchor TR (1,0), offset (-24,24), alignment (1,0), size 240×56.
   - KillCountText 'Kills: 12' in num-m. A 200×6 sunk aperture along the bottom edge is where slips print from.
   - In the campaign the counter can hide, but the aperture stays.

CONTEXTUAL
5. Status chips (StatusEffectBar / IndicatorContainer HBox). Anchor TL, offset (24,272), size 728×44.
   - Chips are 216×44 with 6 px gaps. Up to 3 show, plus a '+N' chip (NEW; the code cap of 8 stays).
   - TintImage is the chip frame (buff or debuff shape and rim). StatusNameText is the display name ('Stone Skin') in label style. DurationText is the countdown in num-s.
   - Only when any status is active. Status is still primarily VFX ('not icon soup').
6. Registry slips (NEW UMordecaiRegistrySlipStack). Anchor TR, offset (-24,88), alignment (1,0), 400 wide.
   - Heights 88 px (skill, level, loot) or 112 px (feat with drawback). Up to 3 stacked, 8 px gap, newest on top.
   - Feat, rank-up, milestone, level, identify, aggregated 'Stored:' loot.
7. Boss bar. Anchor TC (0.5,0), offset (0,28), alignment (0.5,0), size 960×84. Shown for bosses only.
   - A 400×36 name tab with display-s stencil 'THORN GUARDIAN'.
   - Health bar 960×24, posture bar 960×8.
   - Engraved phase posts, a 'PHASE 2' chip, and a hatched danger rim when enraged.
   - Spans x 480-1440, clear of the vitals plate (ends at 456) and the slot (starts at 1656).
8. Combat feedback. Anchor C (0.5,0.5), offset (0,150), alignment (0.5,0), a 360×52 stamp plate under the player's feet.
   - FeedbackText keeps the exact strings: 'Perfect Dodge!', 'Perfect Block!', 'Parried!' and 'Posture Broken!' in amber with a check-stamp glyph; 'Parry Failed!' in danger with a cross-stamp glyph.
   - 1.0 s total.
9. Combo. Anchor C, offset (120,40), alignment (0,0.5), a 140×44 plate.
   - ComboText 'Hit 3' in num-m, plus a chain-pip row (filled out of max).
   - Only while ComboWindow is active.
10. Pickup prompt. Anchor BC (0.5,1), offset (0,-28), alignment (0.5,1).
   - An auto-width plate, 480 px minimum, 720 px maximum. PromptText '[F] Pick up Unidentified Purple Amulet' in body-l.
   - InventoryHintText '[I] Inventory' sits below in label-muted, on its own 160×36 mini-plate. It stays when no pickup is focused, and could fade after 3 uses (TODO(DECISION)).
   - Its left edge at x 720 clears the weapon plate and quick items.
11. Quick items A-D (future IA_Mordecai_ItemA-D). Anchor BR, offset (-24,-116), alignment (1,1), four 72×72 slots with 8 px gaps.
   - Keycaps 1-4 / D-pad, counts in num-s, dashed rim when empty.
   - This is where the Alchemist potion stock shows.

WORLD-SPACE
12. Enemy Registry tag (MordecaiEnemyIndicator: NameText, HealthProgressBar, StatusIconContainer). Lyra indicator at Z +120, VAlign bottom, 184×56 tag with a 16 px tether.
   - Row 1: tier chevrons and the name in label style ('Frontliner').
   - Health bar 168×10, then a NEW posture sub-bar 168×5.
   - Up to 3 status glyphs (16 px) plus '+N'.
   - States: broken = hatched posture and '×1.5'; knocked down = 'Down ×2.0'; riposte window = amber bracket corners.
   - Beyond 1400 cm it collapses to marks and the bar only (96×12).
   - Replaces the old red WBP_EnemyHealthBar, which closes the US-092 duplicate.
13. Damage numbers: num-l Mono with a 3 px keyline outline, an 18 px type glyph, crits at 48 px with a corner-tick frame, heals as '+N' in mint, 1.5 s.
14. Pickup tags: replace the faint TextRender label with a widget-component Registry tag.
   - Name in label style, grade notches, and a seal glyph when unidentified.
   - Within 600 cm. Farther pickups show only the grade glyph, which fixes 'faint on the light floor'.

## menus
FRONT END (replaces Lyra; issues #57-#65)

Title screen
- The background is a live diorama vignette (#58): one Verdant Wilds cell with a dormant Continuant machine, seen at the game camera pitch (-50), so the toy world is the art.
- No centred glass panel. A left column at x 120 holds:
  - The stencil display-xl wordmark 'PROJECT MORDECAI' (the final title is TODO(DECISION)).
  - The tagline in mono, muted, on a slim plate: 'Everything you do is a skill. The machines are still counting.'
  - A stack of 360×64 menu plates: Play / Options / Credits / Quit (Continue appears when saves exist).
- Bottom-left, a mono Registry status line: 'Registry online · Deeds on file: 0'. This replaces the machine-name, build stamp and ADD PLAYER watermarks (#61).
- Signature moment, the Registry boot (1.2 s):
  - The plates clunk in one by one, 80 ms apart, each with a toy drop.
  - The amber lamp blinks twice.
  - The tagline types out at 20 ms per character.

Play
- An 'Expeditions' list of playlist tiles (DA_Mordecai_Arena: 'Mordecai Arena' / 'Dev test arena') as Registry tags with key art.
- One press goes straight to loading. The Lyra host and browse screens are skipped (#60).

Options
- The same screen as the pause Settings tab: Display, Accessibility, Audio, Controls.

Credits
- A long slip scrolling up like printer tape: dotted-leader ledger rows, plus the OFL attributions for all three fonts (#63).

Quit confirm and all dialogs
- A slip on UI.Layer.Modal over the scrim, replacing W_ConfirmationDefault and Error.
- Example: seal header 'QUIT' / 'Quit to desktop?' / 'The Registry will keep counting.' / [Quit] [Cancel].
- Error dialogs use a seal 'NOTICE' header.

Loading (TitleText / StatusText kept)
- Opaque plate ground with an engraved grid and a faint cog.
- 'PROJECT MORDECAI' in display-l stencil, 'Loading...' in num-s muted.
- The throbber becomes a ring of 12 lamp ticks lighting in sequence (an Image material on time).
- An optional NEW lore line: 'Indexing region: Verdant Wilds. Status: nominal.'

PAUSE MENU SHELL (MenuColumn 1000×640 kept)
- Scrim at 80%.
- 'Paused' in display-m stencil, upper-left of the column, with the subline 'Registry suspended. Nothing is being counted.' in body-muted.
- TabBar: five index tabs of 170×44 rising from the panel top, with [LB] and [RB] keycaps at the ends.
- TabContentBorder: the Registry plate, 16 px padding, about 968×520 usable.
- NEW action bar under the plate, in num-s keycaps: '[Esc] Back  [LB][RB] Tabs  [I] Close'.
- Focus: amber ring. Hover: plate-raised. Active tab: merged with the panel, with an amber lamp bar.
- Signature moment, the Index Tab Shift: the active tab rises and the content slides in from the direction of LB or RB.

Character ('Registry File')
- Three columns:
  - Left 280: name, 'Level --', and the 4 resources 'Health 100 / 100' in mono.
  - Middle 360: three group plates (Physical, Resilience, Magical), each with rows of code (Mono bold 'STR'), name, base value and Effective Mod '+9.4%'.
  - Right 300: the 5 primary-derived and 9 secondary-derived stats as a dotted-leader ledger, '+15.0%' right-aligned.
- Signature moment: the ledger leaders.

Skills
- Header: 'Skill Points: 3' in mono.
- Left rail (220 wide): Weapon / Defense & Armor / Movement & Exploration / Utility & Interaction / Magic.
- Centre: a ScrollBox of rows. Each row has the skill name, a 20-tick rank ruler (milestone posts at 1/5/10/15/20 stand taller and get stamped when reached), '12 / 20' in mono, and an [Allocate] button (disabled = engraved, with a lock).
- Right: a detail panel with the milestone list ('No data available.' fallback).
- Signature moment: on Allocate a tick stamps in. At a milestone the post gets a seal and a slip prints.

Feats
- Header: 'Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary'.
- A grid of 300×120 seal cards: tier seal shape, name, tier word, one-line description.
- Locked cards: dashed seal, name visible, '???', and per-condition meters in Mono ('TotalPerfectParries: 32 / 50', 'Requires: Tag' with a check or cross image).
- Rare and Legendary cards show a hatched 'Drawback' line.
- Signature moment: a live unlock stamps the seal and the card slides from Locked to Unlocked.

Inventory (fixes #37 structurally)
- FilterBar (an HBox, as shipped) spans the full 968 px width on top. Eight label chips at 20 px total about 790 px, so they no longer collide with the ledger.
- Below:
  - Left list, 640 wide: 48 px rows with grade notches, the rarity-coloured name, 'x2' in mono, an '[equipped]' chip, and an [Identify] button with an amber rim.
  - Right column, 312 wide: 'Registry Entry' detail on top (name, rarity word, notches, type, description; unidentified items show 'Contents sealed. Appraise at the Mage Tower.'), and the 'Stored Resources' ledger with dotted leaders below ('Iron Ore ......... x1').

Settings
- Display, Accessibility, Audio, Controls ('Coming Soon' until the Settings TODO(DECISION) is made).

ITEM TOOLTIP / COMPARE
- A 360-wide Registry Entry card.
- Header plate: name in rarity colour, notches, rarity word and type.
- Stat rows in mono, with a delta column vs equipped: '+4' in buff with an up-tick image, '-2' in danger with a down-tick.
- Compare shows two cards side by side; the left one is stamped 'EQUIPPED'.

DEATH SCREEN (DeathText / CountdownText / KillCountText)
- Scrim, then a 720×380 slip prints from the top, then the seal stamp 'YOU DIED' lands at -6° (display-xl in seal red, 5.6:1 on paper).
- The record lines on the slip:
  - 'Re-entry in' and the countdown in num-xl.
  - 'Kills: 7' in num-m.
  - 'Dying is not a skill. The Registry checked.'
- Still HitTestInvisible.

TOWN SERVICE: MAGE TOWER (Identify)
- A full CommonUI screen.
- Left: a list of sealed tags (unidentified items).
- Centre: the 'Appraisal Table', one large item card with a wax seal.
- Right: the tower crest, tier 'Functional', and the cost line ('Free' today; TODO(DECISION)). The [Identify] button keeps its word.
- Signature moment, the Seal Break: the seal shakes and splits, the name resolves character by character, the grade notches light, and a slip prints 'ENTRY AMENDED'.

MAP ('Survey Sheet', IA_Mordecai_Map, M / View)
- Plate ground with an engraved 32 px grid.
- Cells drawn as rounded toy tiles, muted by biome. The current cell has an amber ring.
- Gates are stamped glyphs (explosive, magnet, phase, glide, town gates). Shortcuts are dashed. Cartography notes appear as Registry footnotes.
- Signature moment: entering a new cell stamps 'SURVEYED' on its tile.

BIOME CARD
- Display-l stencil name on a slip strip printing across the top centre, with the Registry status line ('Status: nominal. Everything is fine.').

## motion
PRINCIPLES
- The Registry moves in discrete machine steps: stepped curves, ticks, stamps, printouts.
- Physical plates move with toy weight: small overshoot, and the solid drop shadow compresses on press.
- Nothing floats or blurs.
- Numbers never scale-animate; they change colour or shake, which keeps the mono readouts steady.
- Interaction feedback stays at 160 ms or less, so held LB/RB and held cycling stay crisp. Only rewards (slips, seal break) run longer.
- 'Reduced motion' replaces every animation below with a 150 ms or shorter opacity crossfade and turns off shake, scramble and pulses.
- UMG implementation: widget animations with constant-interpolation (stepped) keys for printing, and cubic or back easing curves elsewhere.

NAMED ANIMATIONS
1. SlipPrint / SlipTear (toast in and out).
   - In: the slip feeds out of the Registry slot aperture in 4 stepped moves of 60 ms (240 ms, constant interpolation), clipped by the aperture, with a 1-frame 1 px jitter on each step.
   - Hold: Common, skill and loot 3.0 s; Rare and level 4.0 s; Legendary 5.0 s; all multiplied by the accessibility duration setting.
   - Out: 180 ms ease-in-cubic, 24 px up, fade to 0.
   - The remaining stack closes the gap in 160 ms ease-out-cubic.
2. DamageTrail (health, also boss and enemy bars).
   - On a hit the fill snaps at once. A porcelain trail chunk holds 450 ms, then shrinks in 300 ms ease-in-quad.
   - On a heal a mint lead-in appears at once and the fill follows in 250 ms ease-out-quad.
   - Under 25% the rim pulses danger at 1 Hz, sine (off in reduced motion).
3. SealBreak (identify reveal).
   - 0-150 ms: the seal shakes ±2 px for 3 cycles, linear.
   - 150-270 ms: the seal halves split 12 px apart and fade, ease-out-quad.
   - 270-720 ms: the item name resolves left to right, each character cycling through 2 mono glyphs (25 ms per character, 450 ms cap).
   - 720-1020 ms: the grade notches light 60 ms apart, each scaling 1.15 to 1.0 ease-out-back (overshoot 1.7).
   - At 1020 ms the 'ENTRY AMENDED' slip prints.
4. TabShift (pause-menu tab switch, by click or LB/RB).
   - The new active tab rises 4 px and its amber lamp bar grows 0 to 100% width in 120 ms ease-out-cubic.
   - The old content fades in 80 ms linear.
   - The new content slides 24 px in from the travel direction (RB from the right, LB from the left, wrap included) and fades in over 140 ms ease-out-cubic.
   - 160 ms total.
5. KeyPress (buttons, keycaps, tabs).
   - Press: the plate drops 3 px (shadow 4 to 1 px) in 50 ms linear.
   - Release: it returns with a 1 px overshoot in 110 ms ease-out-back.
   - Focus gained: the amber ring scales 1.06 to 1.0 in 120 ms ease-out-quad.
6. Stamp (combat feedback, death 'YOU DIED', milestone seal).
   - Scale 1.35 to 1.0 with rotation -6° to -3° in 90 ms ease-out-quad, plus a 1-frame 2 px down-offset 'thunk'.
   - Hold until 820 ms, then fade over 180 ms linear: exactly 1.0 s, matching FeedbackDisplayDuration.
   - ReadyPing (a cooldown is ready): the rim switches to amber at once and eases back to verdigris over 240 ms ease-out-quad.

## accessibility
THEMES
- Default, plus a shipped High Contrast theme:
  - HC tokens: black plates, white rims and text, white slips with black ink, brighter fills.
  - Every HC text pair is 7:1 or more (lowest: health 8.77, rarity Red 9.04 on raised, seal 8.56 on paper). Every HC graphic is 4.5:1 or more.
- Default: every text pair is 4.5:1 or more (lowest: health as text on raised 4.79, rarity Red on raised 5.25, seal on paper 5.6). Graphics are 3:1 or more (lowest: the Exhausted hatch at 4.17).
- Contrast was computed from the hex values with the WCAG formula.
- The theme swap is one DataAsset plus one material parameter push. Nothing is baked into textures except greyscale masks.

FLOOR-PROOFING
- No HUD text sits on the world.
- Plates are opaque. The dual keyline (dark outer, verdigris inner) gives 3:1 or more against any floor tested: white, light blockout, grass, sand, water, crystal, mire, night.
- World numbers carry a 3 px keyline outline.
- Pickup and enemy labels become plated Registry tags.

COLOUR IS NEVER ALONE
| Signal | Carried also by |
|---|---|
| Health | % readout |
| Stamina tier | notches + '×1.25' chip + Exhausted hatch and word |
| Posture broken | hatch + 'Broken' |
| SP | count of lit pips + 'x / y' |
| Rarity | colour word + 0-5 grade notches + seal on unidentified items |
| Feat tier | tier word + seal shape |
| Damage type | glyph (+ Shadow inverted print, Healing '+') |
| Buff / debuff | chip shape + category glyph + name |
| Enemy tier | chevron count |
| Combat feedback | wording + check or cross stamp |
| Focus | ring + raised plate |
| Disabled | lock glyph |
A colour-blind simulation pass (deuteranopia, protanopia, tritanopia) on playtest screenshots is part of acceptance.

TEXT
- Minimum 20 px at 1080 (13.3 px at 720p).
- Stencil only at 28 px and up, never for body text. No all-caps body text.
- Atkinson Hyperlegible for low-vision legibility; Mono for all live numbers.
- Menu text scale: 100 / 125 / 150%. Layouts use SizeBox minimums and ScrollBoxes, and the inventory filter row has room to wrap.
- HUD scale: 75-125%. HUD inset: 0-48 px.

MOTION AND SENSORY
- Reduced motion (crossfades only).
- Screen-shake slider.
- No full-screen flashes; nothing blinks faster than 3 Hz except the 4 Hz riposte brackets, which become a steady glow under reduced motion.
- Optional low-health edge vignette.

COGNITIVE
- 'Registry chatter': Full / Brief (facts only) / Off (critical only).
- Toast duration ×1 / ×1.5 / ×2.
- Aggregated loot slips (no loot vomit).
- Statuses capped at 3 chips plus '+N' on the HUD.
- Every key is shown in bracket notation, and later as device-aware keycaps (KBM square, gamepad round), so input is never implied.

INPUT
- Every interactive element has a visible amber focus ring for gamepad and keyboard.
- The menus are fully navigable with LB/RB and the D-pad.
- Hold actions show a fill ring on the keycap.

## risks
WHAT COULD GO WRONG
1. Lore approval. The Registry is an invented institution that extends the Continuants and the 'automated systems still running without purpose' line. It needs Jeff's TODO(DECISION).
   - If he rejects it, the visual system (plates, slips, tags) still stands, and the voice falls back to the neutral plain voice.
2. Tone clash. DCC deadpan against a serious 'Impermanence vs Control' world. The Registry could cheapen sad beats.
   - Mitigations: chatter levels; a hard rule that the Registry never comments on story, NPCs or the Lich King; the flavour line is always the one that gets cut.
3. Weight. Dark enamel plates on a bright toy world could feel like a heavy dashboard and fight the charm.
   - Keep the HUD at 7% of the screen area or less.
   - Test plate opacity at 100% vs 92%. At 92% the contrast must be recomputed against a white floor.
4. Stencil legibility and connotation. It could read military or shipping-crate.
   - It is limited to 28 px and up, and needs checking at 720p, where 28 px becomes 18.6 px.
5. Hue neighbours that rely on glyphs and shapes: Lightning vs Radiant, Poison vs Healing, stamina-green vs buff, Arcane vs SP, rarity Gold vs the amber focus lamp (1.22:1 apart).
   - If the colour-blind simulation shows confusion, shift hues before adding more glyphs.
6. Build cost.
   - UMG font size is in points (px × 0.75).
   - Import the static TTFs (the css2 API serves per-weight files), not the variable fonts.
   - Fonts live under /MordecaiCore, are saved and committed, cooked, and get OFL credits.
   - Lyra's Niagara number pops need a baked Atkinson Mono digit atlas.
   - UProgressBar has no radial fill, so the key shutter needs a material hook.
7. Code changes this direction needs (small, but real):
   - HealthPercentText, StaminaTierText, SpellCooldownC/D (US-088).
   - EquippedWeapon slot in the HUD (US-091).
   - A '+N' status chip.
   - An enemy posture sub-bar.
   - UMordecaiRegistrySlipStack.
   - Pickup Registry tags (widget component).
   - The HUD moved into UI.Layer.Game.
   - UMordecaiUITheme.
8. Playtest bridge. It reads only UTextBlocks on Mordecai widgets.
   - Decorative digits and stamps must be Images.
   - The shipping strings stay exact; ToUpper is display-only.
   - Flavour lines are separate TextBlocks so exact-match checks still pass.

TEST FIRST
1. Build the vitals plate, the 4 spell keys, one slip and one enemy tag as WBPs over DevTestMap and CombatTestMap.
   - Capture with playtest_boot engine screenshots at 720p.
   - Measure on-frame contrast against the real light floor plus one saturated grass or sand material.
   - Check the HUD area.
2. A 20-line Registry voice read with Jeff, in context on those screenshots, before writing any more copy.
3. Colour-blind simulation of the damage numbers, stamina tiers and rarity rows.
4. The inventory filter row at 20 px and 150% text scale, to confirm #37 cannot recur.

