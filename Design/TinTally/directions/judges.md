# judge:2 winner: broadcast
## broadcast total 38: fit 8 read 7 coh 9 orig 7 feas 7
strong:
- The combat hold and receipt. Tickets wait while you fight and print 350 ms apart after 2.5 s of calm, a '+N' pending badge shows something happened, and every ticket has a plain cause line. This keeps DCC's notification stream without ever covering an enemy, and no other direction solves that.
- The tally mark (four strokes and a slash) is one motif that maps exactly onto the skill system. A 20-stroke rank bar has its bundle slashes at milestones 5/10/15/20, and Rank 1 is a capped dot.
- Copy rule: the headline is always plain and the joke goes on the second line. Remarks are 9 words or fewer, never appear in combat, and are banned on boss intros, biome cards, endings and status lines. With remarks off, every ticket still makes sense.
- The Tally spike in the pause menu keeps the last 12 tickets, so nothing can be missed. Density is Full / Calm / Minimal, plus 'Hold plates until I pause', and each lane has its own sound.
- The dual edge works. Ink #0E1016 outside plus rim-tin #C5CFD9 inside gives a computed worst case of 3.48:1 against any floor luminance, and this is the only direction whose 'any floor' claim holds.
- Feat tiers get their own hues (Legendary is magenta #FF6AD5, 6.54:1 on plate), so 'gold' never means both a Legendary feat and a Gold item.
- It is honest about system gaps: ranks are bought with Allocate and not levelled by use (TODO(DECISION)), there is no level/XP system yet, and status names stay 'StoneSkin' until US-091.
- Hanging tags pivot at a punched hole, and on the Acceptance ending the Tally prints its last ticket, 'Count closed.'
problems:
- The focus ring #FFD43B vs stamina Yellow #FFD23F is 1.01:1, and vs 'good' / rarity Gold / Elite diamonds #FFC93C it is 1.08:1. One yellow carries five meanings, so a focus ring on a Gold item row or at the Yellow tier vanishes.
- There are five near-identical reds. Vital #FF5D73 vs foe #FF5A4E is 1.04. Vital vs stamina Red #FF6A3D is 1.04, and those are two stacked bars about 10 px apart on the gauge plate. 'bad' #FF6A55 vs rarity Red #FF6B5B is 1.01.
- The bar edges sit below 3:1. Track #3A4157 vs plate #1A1E2B is 1.64:1 and the ink bar outline vs plate is 1.15:1, so the full extent of a bar is weak. Fills only just clear the track: foe 3.29, vital 3.41, stamina Red 3.56.
- It contradicts its own rule. It says 'no text sits directly on the dim or the world', yet default-theme combat callouts are 'letters only, with a 4 px outline' over the floor.
- The text floor is 18 px, which is 12 px at 720p. That covers remarks, status names and keycap letters (num-s 18), which is too small at TV distance.
- 'Tin Tally' never reaches the palette. A dark slate-navy plate with off-white tags and a marigold accent is the stock dark-HUD-with-yellow look. A lithographed tin toy implies printed reds, creams and teals.
- The tally motif is overloaded: rarity strokes, rank strokes, SP grouped in fives, phase pips, the throbber, and combo strokes under the weapon name that repeat the 'Hit N' counter. Once a row of strokes means six things, it stops meaning one.
- The death screen hangs from the top-right rail head (about x 1328-1888), so the most important screen in the game is off-centre.
- StampReveal types the item name into the real TextBlock at 18 ms per character and then swaps font. The playtest bridge can read a partial name.
- The combat hold needs new C++: aggro radius 1500 uu, damage in the last 4 s, a queue and merging. In an arena wave loop such as the DevTestMap spawn swarm, the calm window may never come and the receipt may never print.
- 'Filed under enthusiasm.' is word for word Toybox's joke, and it shares the 'Tally' name and the Automation-Age-counter lore move with Toybox.
- Rowdies at display-xl for 'You Died' and boss banners risks looking cartoonish next to the Lich King. The direction flags this but ships it anyway.
- Missing: village management screens (Initiatives, Population/Housing, building tiers, Upgrade Keys inventory) and the Skill A/B radial select.
## machine total 35: fit 7 read 7 coh 8 orig 6 feas 7
strong:
- The UI is the Registry, a machine layer laid over the toy world. In the Acceptance ending it fails: the plates go dark and the slips stop, so the interface literally lets go. This is the strongest theme payoff of the three.
- The voice is 'indifferent, not cruel', and it is funny because it cares precisely about the wrong thing. That is a lore-native twist on DCC's System. It has a hard rule never to comment on story, NPC loss or the Lich King.
- Slips feed out of a 200×6 aperture in the kill-counter plate in 4 stepped moves (constant interpolation), and there is a death record slip.
- Registry chatter is Full / Brief / Off. Loot is aggregated ('Stored: Iron Ore x2'), and the flavour line is always the one that gets cut.
- The stamina multiplier chip ('×1.25', 'Exhausted ×0.50') is the tier name the player can use, alongside rim notches at 33/66.
- Theming is one greyscale mask texture (R fill, G rim, B keyline) plus an MPC, so the High Contrast swap is one parameter push.
- 20 px minimum text (13.3 px at 720p). Atkinson Mono digits are measured at 632/1000 em with a fixed-width SizeBox. ToUpper is display-only, so the bridge strings stay exact.
- 0-5 grade notches for rarity and enemy tier chevrons.
- Registry tags are tethered to their owner by a rim line and collapse to marks beyond 1400 cm. This fixes the faint pickup labels.
- Dotted-leader ledgers for derived stats, Stored Resources and credits.
- A self-imposed budget of 7% of screen area for the HUD.
problems:
- The claim that the plates 'guarantee an edge of at least 3:1 on any floor' is false. The computed worst case for keyline #040908 + rim #5DB8A1 is 2.91:1, at a floor luminance of about 0.10. The plate itself is 1.08:1 vs a night floor #1E2430 and 2.30 vs mire #4E5A45.
- Bar tracks are invisible. Plate-sunk #070F0D vs plate #10201D is 1.15:1 and keyline vs plate is 1.19, so the empty part of every bar merges into the plate and missing health reads only from the % text.
- Danger #FF6E61 vs rarity Red #FF6E6E is 1.01 (the same colour). Health #FF5C72 vs danger is 1.09, and health vs rarity Red is 1.10.
- The lamp-focus amber #FFB547 vs rarity Gold / Champion chevrons #FFD24A is 1.22, and vs stamina Yellow #E6D84A it is 1.20.
- Other collisions: stamina Red is literally the Fire damage hex #FF8A3D. Stamina Green vs buff is 1.18, Arcane vs SP is 1.14, and Corrosive vs the brand verdigris rim is 1.52.
- There is no combat hold. The 'frequent' slips stack up to 3×112 px (y 88-440) over the right third mid-fight, which works against pillar 8, 'Clean UI'.
- Rank-up slips ('PRACTICE NOTED · Dodging — Rank 4/20 · Harm avoided: 112 times') assume skills level from use. In fact ranks are bought with Allocate, so the slip would announce the player's own menu click.
- 'LEVEL 7 · +1 skill point issued' assumes a levelling system that does not exist. Neither is flagged as TODO(DECISION).
- The identity reads municipal or dieselpunk: dark verdigris-black enamel, rivets, stencil caps, seal-red 'FILED' stamps. LA's bright toy charm survives only in a 3 px press offset, so this risks a 'Papers, Please' dashboard over a pastel diorama.
- The deadpan bureaucratic AI clerk is a stock AI-design trope. 'Filed under: fires, intentional.' is the same 'Filed under' joke all three directions wrote.
- The stencil slip headers at display-s 28 px render at 18.6 px at 720p, where the stencil bridges break up.
- The red ToUpper 'YOU DIED' stamp evokes Dark Souls.
- SealBreak cycles mono glyphs through the item-name TextBlock for 450 ms, so the bridge can read a scrambled name unless the scramble is a separate overlay.
- Minor factual slip: it says the numero sign and arrows are missing from all three fonts, but Big Shoulders Stencil contains U+2116 and ▲▼ (measured).
- The health glyph is a 'notched porcelain rhombus', and a rhombus reads as a gem, close to a rupee silhouette.
- Missing: village management screens (Initiatives, Population/Housing, building tiers), the Skill A/B radial and the Throw slot.
## toybox total 26: fit 6 read 5 coh 5 orig 6 feas 4
strong:
- The well-lip token: a 2 px #7A7390 lip at 3.38:1 vs the tray and 4.20 vs the well. It fixes the well-vs-tray contrast of 1.24, and it is the only direction that caught the bar-track edge failure.
- Buff vs debuff by polarity: a light card chip for buffs and a dark ink chip for debuffs, plus four socket shapes by category. The non-colour cue reads instantly.
- Punch-card skill rows with brass eyelets at milestones. A disabled Allocate shows its reason ('No skill points', 'Max rank').
- A peeled-corner marker on unseen feats until they are viewed.
- Nunito's default digits are tabular, verified at 600/1000 em, so it works as a single UI-and-numeral family.
- Contact shadows fall straight down, matching the diorama's top light.
- Luggage-tag pickup labels.
- The Insert Tray pause menu with cardboard index dividers.
- The death line 'Nothing stays. Not even this.' suits Impermanence vs Control better than any other line in the three directions.
problems:
- It stacks two metaphors: a board-game box (felt, cardboard, dice pips, brass tacks, scratch-off panels, a paper map, a standee) and an Automaton-Age Tally that slaps magenta vinyl stickers. Nothing in the lore puts a Continuant engine inside a game box, so the result is a pile of materials, not one system.
- DCC is confined to a toast layer, the weakest DCC fit of the three. Scratch-off identify, holo-foil and magenta prize stickers read as lottery or gacha, while the lore has no loot boxes.
- It fails on dark floors. Plates are tray #272338 with an ink #1B1924 outline and no light rim: tray vs a night floor #1E2430 is 1.02 and ink vs it is 1.11. Ink plus card-edge has a worst case of 2.57. The claim of 'at least 4.51:1' was tested only on light sample floors, yet the Mire, the caves and the Withered Kingdom are dark.
- The compare arrows are written as text ('▲ +4', '▼ −2'), but Nunito has no U+25B2 or U+25BC (measured), so they render as tofu boxes in Slate.
- Focus #FFC83A vs stamina Yellow #F5C83A is 1.03, and vs the Gold swatch it is 1.17. The same yellow is also the positive feedback text and the crit starburst.
- Health #F0405A vs stamina Red / danger #FF6A3D is 1.32, and the Tally pink vs health is 1.29. Rarity Red card text is exactly danger-ink #B0221F, and buff is exactly the rarity Green text #1E7A33.
- Layout collision: the toast rail (x 1368-1888, from y 96) overlaps the boss bar (x 480-1440, y 24-128) in the box x 1368-1440, y 96-128.
- Combat feedback (y 880-932) and the combo counter (y 812-868) sit at bottom centre, about 350 px below the player, so the eyes have to leave the fight.
- The 18 px caption minimum is 12 px at 720p on status chip names and tier words.
- Heaviest build for a solo dev: 4-6 stacked images per token × 20 enemy plates, and a duplicate TextBlock per damage number for the vinyl margin. On top of that come felt, grain, gloss, foil and scratch materials, brass threads drawn between attributes, a lid-lift camera dolly, a standee tip and chad particles.
- Generic choices: Nunito is the stock friendly-rounded font, and navy-purple tray + cream card + marigold is the stock cosy-game palette. The green baize reads as a casino or pool table.
- A cartoon standee tipping over on death undercuts the serious tone. Dela Gothic One is 2.5 MB because of its CJK glyphs and needs subsetting.
- It shares the 'Tally' name and the 'Filed under enthusiasm' joke with Tin Tally.
- Rank-up toasts assume skills level from use, while Allocate is manual.
- Missing: village management screens and the Skill A/B radial.
graft:
- From Machine: carry the theme to the end. On the Acceptance ending the Tally fails: the plates dim, the rail stops printing and the HUD visibly lets go. Tin Tally's single 'Count closed.' ticket becomes the last line of that sequence.
- From Machine: raise the text floor from 18 px to 20 px (13.3 px at 720p). This covers remarks, status names and keycap letters.
- From Machine: the voice rule 'indifferent, not cruel; it cares precisely about the wrong thing', a Full / Brief / Off chatter control where the flavour line is always the one cut, and aggregated loot slips ('Stored: Iron Ore x2'). Drop the 'Filed under' joke that all three directions share.
- From Toybox: the well-lip token. Give every bar track a 2 px lip at 3:1 or better vs both plate and track. This fixes Tin Tally's track vs plate (1.64) and ink outline vs plate (1.15).
- Fix the yellow collision with Machine's approach. Move stamina Yellow to lemon (like Machine's #E6D84A), and give focus a hue no reward, rarity or Elite mark uses. Today focus #FFD43B vs stamina Yellow #FFD23F is 1.01 and vs good #FFC93C is 1.08.
- Fix the red collision. The vital fill must differ from the stamina Red tier by hue and lightness, not only by the Red-tier ink stripes (vital vs stamina Red is 1.04 on adjacent bars). Give 'bad' and rarity Red distinct values (1.01 today).
- From Machine: Registry-style world tags for enemies and pickups. A small plate is tethered to its owner by a line, collapses to marks and the bar beyond 1400 cm, and far pickups show only the rarity glyph. This fixes 'faint on the light floor'.
- From Toybox: buff and debuff chips by polarity, a light enamel chip for buffs and a dark plate chip for debuffs, on top of the notched-top vs pinked-edge shapes.
- From Toybox: a peeled-corner or unstamped 'new' marker on unseen feats until viewed, and a disabled Allocate that prints its reason ('No skill points', 'Max rank').
- From Machine: dotted-leader ledgers for derived stats, Stored Resources and credits.
- From Machine: device-shaped keycaps. KBM keys are square porcelain caps, gamepad face buttons are round, and shoulders and triggers are pills, so the device reads by shape.
- From Machine: bridge-safe reveals. Scrambles, typing and ghost digits run on an Image or overlay layer, and the real TextBlock is set once to the final shipped string.
- From Toybox: the death line 'Nothing stays. Not even this.' in plain voice, which suits Impermanence better than a Tally remark. Centre the death plate rather than hanging it from the top-right rail head.
- Pull the tin-litho identity into the palette: printed cream, teal and signal red on the plates' rims and tags, so the look is not the stock navy-and-marigold dark HUD.
- Thin the tally motif. Keep it for skill ranks (milestones 5/10/15/20), rarity and the throbber. Drop the duplicate combo strokes under the weapon name, which repeat 'Hit N'.
- Give the combat hold a hard release cap (for example, a receipt forced after 20 s or on any pause) so arena wave loops cannot hold rewards forever.

# judge:1 winner: broadcast
## machine total 39: fit 7 read 7 coh 9 orig 8 feas 8
strong:
- The UI is the Registry's machine layer, so the Acceptance ending can have it fail: plates go dark and slips stop printing. It is the strongest theme payoff of the three.
- One greyscale R/G/B mask in a 9-slice (fill, rim, keyline) plus one UI material that reads theme colours, so the High Contrast swap is a single parameter push. Cheap and elegant for a solo dev.
- Registry tags on enemies and pickups: a plated tag tethered to its owner like a price tag, collapsing to marks only beyond 1400 cm, and to a grade glyph only for distant pickups. This fixes the faint pickup labels on the light floor.
- Slips print from an aperture in the kill-counter slot, and loot is aggregated ('Stored: Iron Ore x2, Town Stone x1'), so loot never floods the screen.
- Safe for the playtest bridge: TextTransformPolicy ToUpper so GetText still returns the shipping string, flavour lines in separate TextBlocks, decorative digits and stamps as Images.
- Explicit layout clearance: the boss bar spans x 480-1440 and clears the 456-wide vitals plate. It is the only direction whose boss bar does not overlap the HUD.
- Next and Mono share one skeleton. Mono's 632-unit advances, and the proportional Next and Stencil digits, are exactly as claimed.
- Best voice lines: 'Dying is not a skill. The Registry checked.' and 'The Registry regrets the earlier vagueness.'
problems:
- Overclaim: 'keyline + rim guarantees an edge of at least 3:1 on any floor' is false. The analytic worst case is 2.90:1 at floor luminance about 0.10, e.g. a mossy Mire tone #525E4A (keyline 2.92, rim 2.89). Only 'every floor tested' (mire #4E5A45 = 3.08) holds.
- The focus ring is 2 px with a 2 px offset (1.3 px at 720p), the thinnest of the three for gamepad focus at TV distance. Amber lamp #FFB547 vs rarity Gold #FFD24A is 1.22:1 (deuteranopia dE 7.7), so the ring disappears against a Gold row.
- The Health % readout is num-m 24 px (16 px at 720p), the smallest headline number of the three for the value pillar 8 requires to be always visible.
- Combat feedback is an opaque 360x52 plate at centre+150, right under the player's feet. Combo is an opaque 140x44 plate at centre+(120,40). Both hide the combat space next to the player at the moment the player most needs it.
- The slip's top edge is perforated with no keyline (keyline on the sides and bottom only). Slip-paper #E3EEE7 is 1.19:1 on a white floor, so each stacked slip loses its top edge across the 8 px gaps on light floors.
- The stencil display face's minimum of 28 px becomes 18.7 px at 720p. Stencil bridges on 'FEAT RECORDED' slip headers and boss names degrade at that size, and the face carries a military / shipping-crate connotation (admitted).
- The dark verdigris enamel HUD is the heaviest, least Link's Awakening palette. The LA half rests only on rounded corners and the 3 px 'toy click'.
- The rule 'the Registry never comments on story beats' is contradicted by its own boss card ('Deviation will be corrected.') and by biome Status lines that editorialise each biome's theme.
- There is no combat hold: slips print mid-fight into the right third (400 wide, down to y about 440).
- Rarity Blue #74AEFF vs Purple #C68FFF collapses under deuteranopia (dE 1.9). The grade notches carry it, but the row's colour cue is lost.
- The pickup prompt claims its left edge is at x 720; that is true only at its 480 px minimum width. At the 720 px maximum the left edge is x 600 (harmless, but the stated clearance is wrong).
- Every quoted contrast pair checked out to 0.01 (15.21, 7.09, 9.59, 14.16, 5.60, 4.79, 4.17, scrim #393F3D 9.71/5.54, HC 9.04/8.77/8.56). Fire vs keyline is exactly 8.55.
## toybox total 28.5: fit 6 read 5.5 coh 6 orig 6 feas 5
strong:
- The contact shadow falls straight down to match the diorama's top light, so the UI looks like it rests on the same table as the world.
- A stamina peg gauge (3/2/1/0 lit pegs) as a count-based tier cue. Their own deuteranopia check (Green vs Red at 1.20:1) proves why it is needed.
- A status category is shown by socket shape (coin / tag / octagon / rounded square) plus polarity: a good non-colour status code.
- The 'No HUD opacity slider' rule, because lowering plate opacity would break the computed floor contrast.
- The largest vitals for TV distance: a 32 px health %, a 320x28 health bar, and a 4 px focus ring with a 2 px ink halo.
- Evaded and parried hits show no number. The tone-bleed rule: the boss bar, death and loading screens drop gloss and stickers.
- The death line 'Nothing stays. Not even this.' and the level-up quip 'Spend it before it starts to feel permanent.' are the best theme-bound copy.
- Nunito digits are tabular at 600 units (verified at 600/800/900), so it is a valid rounded alternative for numerals.
problems:
- Layout collision: the boss bar is 960 wide and centred (x 480-1440, y 24-128), so it overlaps the resource tray (x 32-524, y 28-172) by 44 px.
- Layout collision: the toast rail (x 1368-1888 from y 96) overlaps the boss bar at x 1368-1440, y 96-128, and the spec routes level-up and Legendary slams into that rail during boss fights.
- Floor-proofing was tested only on light floors. Tray tokens have only dark edges: tray vs a night or cave floor is 1.02:1, ink 1.11-1.16, mire 2.08/2.37. The resource tray, spell rack, kill counter and every enemy plate lose their silhouette in the Mire, Caverns and Withered Kingdom. The '2px light lip' has no hex.
- The compare deltas '▲ +4' / '▼ −2' are set as text, but Nunito has no U+25B2 or U+25BC (measured with fontTools), so Slate will show tofu or a fallback font in the item compare.
- Focus marigold #FFC83A vs stamina-yellow #F5C83A: dE 4.5 normal, 0.7 protan, effectively the same colour. In HC, focus #FFE500 vs stamina yellow #FFD21F is 1.14:1, so the claim that the HC values are 'kept distinct' is false.
- The HUD minimum is an 18 px caption (12 px at 720p), used for status names, tier words and keycap letters. That is too small at TV distance.
- Adjacent HUD rows share near-hues: health #F0405A vs stamina-red #FF6A3D is 1.32:1, and posture #5B97F5 vs SP #A87BFF has dE 3.4 under deuteranopia.
- Combat feedback and combo sit at bottom-centre (y about 812-932), 300-400 px from the player, so the eye must travel during combat.
- Heaviest build: 4-6 layered images per token, baked-blur shadows, felt/grain/gloss/foil/scratch materials, chad particles, brass threads between stats, standee tip and coin flip. With 20 enemy plates and 20 number pops it risks Slate batching (admitted).
- Dungeon Crawler Carl is reduced to a throttled prize layer (one quip per 30 s), the weakest DCC presence of the three.
- Die-cut cardboard, a sticker album and peel-corner stickers sit close to Paper Mario Sticker Star / Color Splash, another Nintendo property's signature. It also reads as a board game more than as Link's Awakening.
- Four materials (card, plastic, felt, enamel) plus a magenta sticker layer make it the least unified system.
- Minor: 'You Died' on the 92% curtain over white composites to #2D2B36 at 4.98:1 (claim 5.06). It still passes for display size.
## broadcast total 39.0: fit 8 read 7.5 coh 8.5 orig 8 feas 7
strong:
- Combat hold and receipt: Tally items queue while you dealt or took damage in the last 4 s, or while an aggroed enemy is within 1500 uu, and print 350 ms apart after 2.5 s of calm. The rail head shows a '+N' pending badge. Items merge, overflow reads 'N more on the spike', and the full history lives on the pause-menu spike. It is the biggest combat-clarity win in the set and the most systemic DCC idea.
- A dual edge with ink outside and tin inside, verified at 3.47:1 against ANY floor (HC 4.58). Light enamel tags with an ink outline reach 4.17:1 on any floor. It is the only direction whose floor claim holds analytically.
- The tally-mark motif is one system for skill rank bars (bundles of five slash on milestones 5/10/15/20), rarity badges (0-5 strokes), SP tokens grouped in fives, the combo chain, the throbber and the seal.
- Lane zoning: left = you, top-right = news, centre = now, bottom-centre = interact, bottom-right = loadout, top-centre = big moments.
- The headline is always plain and the joke is the optional second line. Remarks are banned in combat, boss intros, biome cards and endings, and every ticket has a cause line so cause and effect survive the delay.
- Callouts are letters only (4 px outline) under the player's feet, with no occluding plate. Each lane has its own sound.
- Feat-tier hues are separate from rarity, so Gold never means both Legendary feat and Gold item. Arrows and checks are drawn as images, which is correct: Atkinson has no ▲▼.
- The stamina tier has four cues (notches, multiplier pill, solid/striped/hatched fill, inverted Exhausted pill). 'Count closed.' is the last ticket of the Acceptance ending.
problems:
- Bar fills on the light #3A4157 track are the weakest in the set: vital 3.41, foe 3.29, stamina-red 3.56:1. They pass 3:1 only barely, on the HUD's most important TV-distance read (how much health). A sunk track of about #121520 raises them to 6.12, 5.91 and 6.40.
- Five near-identical reds: vital #FF5D73, foe #FF5A4E, rarity Red #FF6B5B, bad #FF6A55, stamina-red #FF6A3D. Vital vs stamina-red is dE 5.2 under tritanopia on adjacent rows; only the Red-tier stripes separate them.
- Focus #FFD43B vs good/Gold #FFC93C: dE 6.9, 3.3 under deuteranopia. On a Gold row or reward stamp, only the 2 px ink inner ring carries focus.
- Layout collision: the boss bar is 1000 wide and centred (x 460-1460, y 32-128), so it overlaps the gauge plate (x 32-488, y 32-184) by 28 px. The full-width banner at y about 0.28 crosses the spell slots and status chips in the left column.
- The rule 'no text sits directly on the dim or the world' is contradicted by the default-theme callouts, which are letters on the world. They are legible (good 12.38, bad 6.74 vs their ink outline), but the rule or the widget must change.
- The 18 px text minimum (12 px at 720p) applies to remarks, status names and hints, which is small at TV distance.
- The HUD plate at 94% opacity composites to #282C38 over white: text is 12.87:1, not the claimed 13.2, and the translucency lets the floor bleed through. 'Ink 15.2:1 on white blockout' implies an off-white floor (pure white is 19.02), so the floor values are unstated.
- Rowdies on banners, 'You Died' and the wordmark may read cartoonish against the Lich King (admitted). Rowdies digits are proportional (measured), which is fine only because pops never tick in place.
- The combat hold, merge queue, 'in combat' detection and spike history are real C++ systems. The rank-up stream assumes events that do not exist, because ranks are bought with Allocate (admitted TODO(DECISION)).
- Visually it has the thinnest Link's Awakening presence: tin lithograph is a different toy era from LA's plastic. LA is carried mostly by restraint and the toy-box metaphor.
graft:
- From machine: carry the Acceptance ending to the end. The Tally's plates go dark and the tickets stop, so the interface lets go, not just one 'Count closed.' ticket.
- From machine: build every plate from one greyscale R/G/B mask 9-slice (fill / tin rim / ink keyline) plus one M_UI_Plate material reading theme parameters. This replaces the three-layer Overlay, and HC becomes one parameter push.
- From machine: a sunk, near-black bar track (about #121520 or #070F0D class). Broadcast's vital, foe and stamina-red fills go from 3.29-3.56:1 to about 5.9-6.4:1. Keep the 2 px ink outline.
- From machine: raise the text floor to 20 px (13.3 px at 720p) and drop body-s 18 px for remarks, status names and hints.
- From toybox: TV-scale vitals: a 32 px health % and an approximately 320x28 health bar.
- From toybox: a 4 px focus ring with a 2 px ink halo, in a focus hue that is not the good/Gold marigold (#FFD43B vs #FFC93C is only dE 6.9).
- From toybox: 100% opaque HUD plates and no HUD opacity slider (drop the 94% plate). Floor contrast must stay computable.
- Fix the boss-bar collision with machine's explicit clearance rule: cap the boss bar so it starts right of the 488 px gauge plate, or drop it below. Keep banners out of the left HUD column.
- From machine: Registry-style world tags with a tether and distance LOD for enemies and pickups (marks and bar only beyond about 1400 cm; distant pickups show only the rarity tally glyph).
- From machine: bridge-safety rules: TextTransformPolicy ToUpper for display caps, remarks and flavour in separate TextBlocks, decorative digits and stamps as Images.
- From toybox: the stamina peg gauge as a count cue next to the multiplier pill, and status category socket shapes (coin / tag / octagon / rounded square).
- From toybox: evaded and parried hits show no damage number. The tone-bleed rule: boss bar, death and loading drop the playful ornaments, not just the remarks.
- Break up the five reds: give foe health a pattern or notch identity distinct from the player's vital, and always pair 'bad' with a glyph.
- From machine: the aggregated loot copy 'Stored: Iron Ore x2, Town Stone x1' and its best deadpan lines. From toybox: 'Nothing stays. Not even this.' as a death-line candidate for Jeff's tone read.
- From machine: put a Mire-tone floor near luminance 0.10 (e.g. #525E4A) in the contrast and colour-blind test set; that is where dual-edge systems are weakest. Simulate rarity Blue vs Purple (dE under 5 under deuteranopia in every direction) to confirm the tally badge carries it.

# judge:3 winner: broadcast
## broadcast total 39.0: fit 8.5 read 7 coh 8.5 orig 8 feas 7
strong:
- Combat hold plus post-fight receipt: tickets wait while you fight, a '+N' pending badge proves something happened, and they print 350 ms apart after 2.5 s of calm, with merging, a cap of 4 hanging, '6 more on the spike', a pause-menu history spike and Full/Calm/Minimal density. This is the only direction that solves DCC notification spam over an action game.
- The tally mark (four strokes and a slash) is one motif that carries data: rarity as 0-5 strokes, a 20-stroke rank bar with slashes on milestones 5/10/15/20, SP grouped in fives, the combo chain, the throbber and the seal. Rarity stays readable without colour, and the system reads as one machine.
- Dual edge (ink #0E1016 outside, tin rim #C5CFD9 inside). Rim against ink is 12.04:1, so the worst case on ANY floor is sqrt = 3.47:1, which I verified. It is the only edge guarantee of the three that holds on every floor.
- Feat tiers get their own hues and silhouettes (circle, hexagon, starburst; magenta Legendary), so 'Gold' never means both a Legendary feat and a Gold item.
- Respects the bridge: status names stay tag-derived ('StoneSkin'), flavour copy goes only in new TextBlocks, and the rank-up stream assumption is flagged as a TODO(DECISION).
- Lean build: textures, 9-slice Box brushes, Overlays and one pattern material, with no custom Slate. Tags pivot at the punched hole through RenderTransformPivot.
- A wind-up tin toy that keeps count sits between LA's toy box and DCC's System AI, and 'Count closed.' on the Acceptance ending ties it to the theme.
- Lanes with their own sounds (slip tick, ticket print, plate stamp), plus HUD zoning: left = you, top-right = news, centre = now.
problems:
- SpellPointsText '10 / 10' in Atkinson Mono 700 at 22 px (num-m) measures 97.3 px (7 x 0.632 em). Its box is 80 px wide, so it overflows by 17 px. Its own text says '10 / 10 is 7 cells', but the box is sized for about 5.7 cells.
- Layout collision: the boss bar is 1000 wide at top-centre, so it spans x 460-1460 at y 32-128. The gauge plate spans x 32-488 at y 32-184. They overlap by 28 px.
- Ticket main line 'Dodging · Rank 6 / 20 (+2)' in title style (Next 700, 24 px) measures 301 px. The ticket is 320 px wide, which leaves about 248 px after glyph and padding, so the line overflows. It has no truncation rule.
- The claim that the filter row fits 8 glyph + label tags in 968 px is not demonstrated. At title 24 px, with 24 px glyphs and normal padding, I measure about 1195 px. It fits only at body-s 18 px, which is not stated, so #37 could recur.
- Bar extent is weak. Track #3A4157 on plate #1A1E2B is 1.64:1, and the '2 px ink outline' on bars is only 1.15:1 against the plate. Fills barely clear 3:1 against the track: foe 3.29, vital 3.41, stamina-red 3.56.
- Four near-identical reds: vital #FF5D73 against foe #FF5A4E is 1.04:1, against bad #FF6A55 1.05:1, and against stamina-red #FF6A3D 1.04:1. HP and ST are adjacent rows, so separation relies on row labels and stripes.
- The HUD plate is at 94% opacity. Text over a white floor composites to #282C38 and measures 12.87:1, not the claimed 13.2. It still passes, but the claim is off.
- Minimum text is 18 px (12 px at 720p) for remarks, status names and timers. That is small for TV distance.
- The type scale is given in px with no UMG point conversion. Typing 24 into UMG renders 32 px, because Slate sizes are points at 96 DPI (px x 0.75).
- The damage-number render path is unspecified. Lyra's NumberPop is Niagara, and the pops are set in Rowdies, whose digits are proportional (404-737 units), so they need an atlas or a UMG pop system.
- Nothing replaces the faint TextRender pickup labels on light floors. The other two directions both fix them with widget tags.
- The full-width banner strip at y≈236-368 covers the spell slots and status chips on the left.
- Rank-up and level-up tickets assume event-driven ranks and an XP system. Neither exists today: ranks are bought with Allocate and XP is undefined. They flag this, but the stream's DCC energy depends on it.
## machine total 38: fit 7 read 8 coh 8 orig 7 feas 8
strong:
- Every contrast claim I recomputed matched to two decimals: plate against white 16.85, lamp 9.59, seal on paper 5.60, Exhausted hatch against track 4.17, Shadow inverted 12.73, and the 80% scrim over white = #393F3D.
- Typography discipline: Atkinson Hyperlegible Mono advances 632/1000 em for digits, %, /, space and × at every weight (measured), and live numbers sit in fixed SizeBoxes. Stencil is used only for static strings at 28 px and up. The minimum text size is 20 px. ToUpper is display-only, so GetText keeps the shipped strings. Its fixed boxes check out: SP '10 / 10' is 88.5 px in a 96 px column.
- The most UE-literate build plan: px x 0.75 to UMG points, LineHeightPercentage, letter spacing in 1/1000 em, FontOutlineSettings, a baked SDF digit atlas for Lyra's Niagara number pops, a material hook because UProgressBar has no radial fill, and SP pips from a SegmentCount fill material. The pips clip correctly because UE5's ProgressBar uses Mask fill.
- Theme swap is one RGB-mask 9-slice (R fill, G rim, B keyline) lerped from an MPC, plus a UMordecaiUITheme DataAsset. High Contrast becomes one parameter push.
- Registry tags with a tether replace both the faint TextRender pickup labels and the duplicate red WBP_EnemyHealthBar. They collapse with distance and add a posture sub-bar, a riposte bracket and 'Down x2.0'.
- Exact HUD layout with no collisions I could find. The boss bar at x 480-1440 clears the vitals (ends 456) and the slips (start 1496). The HUD covers about 6.5% of the screen.
- The UI is the machine layer, so in the Acceptance ending the plates go dark and the slips stop printing: the interface lets go.
- Voice rules: fact first, flavour second, and the flavour line is cut first. Chatter is Full/Brief/Off, there is never comment on story, NPCs or the Lich King, and loot is aggregated ('Stored: Iron Ore x2').
problems:
- The edge guarantee is overstated. Keyline #040908 against rim #5DB8A1 is 8.43:1, so the worst-case floor (about #5A5A5A mid-dark stone) gets only 2.90:1 from either edge, with the plate body at about 2.4:1. 'At least 3:1 on any floor' is false. Their own mire sample, at 3.08, sits next to that minimum.
- The bar track is invisible on the plate. plate-sunk #070F0D against plate #10201D is 1.15:1, and the keyline against the plate is 1.19:1. The empty extent of the stamina and posture bars cannot be seen; only the notches and readouts carry it.
- The 'Exhausted ×0.50' tier chip lives in the 96 px readout column. It measures 189.6 px in Mono 20 px, or 164 px in Next 700 with +20 tracking, so it overflows the 432 px vitals plate.
- FilterBar stays an HBox, yet the text claims 'room to wrap' at 150% text scale. Eight labels in Next 700 measure 611 px at 20 px, about 917 px at 30 px, before padding, so #37 recurs. Wrapping needs a UWrapBox, which changes the bound widget type.
- StatusNameText is set to the display name 'Stone Skin'. That contradicts its own rule that shipping strings never change, and the bridge's 'StoneSkin' checks.
- Slips print whenever an event happens and stack 3 at top-right during fights. There is no combat hold, so DCC chatter competes with combat.
- The 'PRACTICE NOTED ... Harm avoided: 112 times' copy assumes use-based ranks and a stat counter that does not exist; ranks are bought with Allocate. The Allocate and milestone slips would also print inside the pause menu, whose subline says 'Registry suspended. Nothing is being counted.'
- Identity skews municipal and military. Dark verdigris enamel, Big Shoulders Stencil, rivets and cogs over a bright toy diorama make the LA half rest on depth and the click animation alone. They flag the risk themselves.
- Hue neighbours rely on context: SP #B996FF against Purple rarity #C68FFF is 1.01:1, stamina-green against buff 1.18:1, rarity Gold against the focus lamp 1.22:1, and stamina-red against health 1.27:1 on adjacent bars.
- Minor font inaccuracies:
- It says arrows, triangles and numero are missing from all three fonts, but Big Shoulders Stencil has →, ▲, ▼ and №.
- The stencil digit range (560-1096) depends on the opsz instance: I measured 503-1079.
- It does not say which opsz instance (28 or 72) to import from css2.
## toybox total 29.5: fit 6 read 5 coh 5.5 orig 7 feas 6
strong:
- The well-lip token (#7A7390): the only direction that caught the bar-track problem. Well against tray was 1.24:1; the lip measures 3.38 against the tray and 4.20 against the well (verified).
- Cooldown as refill: the spell icon is the UProgressBar FillImage (BottomToTop) over a desaturated copy. It works because UE5's default Mask fill style clips rather than scales.
- A single-pass SDF M_UI_Token material (radius, outline, edge, face) for widgets with many instances, such as enemy plates.
- A correct, verified font claim: Nunito's default digits are tabular, 600/1000 em at weights 600, 800 and 900, so Slate's lack of tnum support does not matter. Dela Gothic One's 2.5 MB size and its proportional digits (0 = 917, 1 = 588) also match my measurements.
- Contact shadows lit from above to match the tilt-shift diorama; focus lifts and press sinks.
- Small UX touches: new feats keep a peeled corner until viewed, dialogs default focus to the safe choice ('Keep playing'), and Identify is a scratch-off reveal.
problems:
- Floor-proofing covers light floors only. The tray family (#272338 with ink #1B1924) disappears on dark biomes: night #1E2430 at 1.02 / 1.11, cave #2A2238 at 1.00 / 1.14, withered #3A3438 at 1.25 / 1.43, mire #4E5A45 at 2.08 / 2.37. That covers the resource tray, spell rack, kill counter, enemy plates and boss bar. The '2 px light lip' has no colour token.
- Fixed boxes overflow:
- SpellPointsText '10 / 10' in Nunito 900 at 28 px is 93.0 px in an 88 px box.
- HealthPercentText '100%' at 32 px is 88.4 px in an 88 px box.
- The filter bar says 'label 20px', but the label style in its own scale is 24 px. At 24 px the eight labels total 726 px, about 1038 px with padding, against 952 available, so #37 recurs. At the 115% text scale, even 20 px reaches about 1007 px.
- The logotype 'PROJECT MORDECAI' in Dela Gothic One at 96 px with +40 tracking measures about 1389 px, about 1328 px untracked. That cannot fit the 'Left 40%' (768 px) of the title screen.
- Compare deltas '▲ +4' and '▼ −2' are written as text, but Nunito has no U+25B2 or U+25BC (verified). They render as fallback glyphs or tofu unless they are images.
- Layout collision: the toast rail spans x 1368-1888 from y 96, and the boss bar spans x 480-1440 at y 24-128. They overlap by 72 x 32 px, and toasts are explicitly routed to the rail during boss fights.
- The HUD minimum is 18 px (12 px at 720p) for the status-chip names and the caption-size text, which is weak at TV distance.
- Health #F0405A against stamina-red #FF6A3D is 1.32:1, and stamina-red and danger are the same hex. Death text over the curtain on a white floor is 4.98:1, not the claimed 5.06.
- Combat feedback stays bottom-centre (y 880-932), about 350 px from the player in a fixed-camera game.
- DCC is under-delivered: confined to a magenta sticker layer while the HUD and menus stay 'quiet and serious'. The board-game box is a third source that is not in the brief.
- A pile of motifs rather than one system: cardboard, plastic, felt, enamel/vinyl, dice pips, peg tracks, punch cards with chads, luggage tags, scratch-off, standee, rules booklet, folded map, holo-foil and halftone.
- Feasibility for a solo developer:
- 4-6 layered images per token, multiplied across 20 plates and 20 pops (flagged).
- Brass threads on hover need NativePaint line drawing.
- Chad particles have no UMG particle path.
- Standee art.
- Damage numbers as duplicated outlined TextBlocks, rather than Lyra's Niagara pops.
- Rank-up toasts assume use-based ranks, but ranks are bought with Allocate, and the direction does not flag this.
graft:
- From machine: every live number in Atkinson Hyperlegible Mono in a fixed-width SizeBox (632/1000 em: one character is 0.632 x size). Fix Broadcast's SP box: '10 / 10' at 22 px needs 98 px or more, not 80.
- From machine: a 20 px minimum text size (13.3 px at 720p) instead of 18, plus the px-to-UMG-point table (UMG pt = px x 0.75), LineHeightPercentage, and letter spacing in 1/1000 em.
- From machine: TextTransformPolicy ToUpper as display-only casing (GetText keeps 'Project Mordecai', 'Paused' and the other shipped strings). The fact line comes first and the flavour line is cut first; chatter levels Full/Brief/Off.
- From machine: a baked SDF digit atlas in Atkinson Mono for Lyra's Niagara number pops, replacing the unspecified Rowdies pops (Rowdies digits are proportional). Keep Rowdies for static display text only.
- From machine: tethered world tags (widget components) that replace the faint TextRender pickup labels and the duplicate red WBP_EnemyHealthBar. Each tag carries the rarity tally badge, a posture sub-bar and a distance collapse (beyond 1400 cm, only the mark and the bar).
- From machine: one RGB-mask 9-slice (fill, rim and keyline channels) lerped from an MPC, plus a UMordecaiUITheme DataAsset, so the High Contrast swap is one parameter push.
- From machine: in the Acceptance ending the plates actually go dark and the rail stops printing, as the visual partner of the 'Count closed.' ticket.
- From machine: aggregated loot tickets ('Stored: Iron Ore x2, Town Stone x1') and a hard rule that the Tally never comments on story, NPCs or the Lich King.
- From machine: 4 Hz riposte bracket corners that become a steady glow under reduced motion, and the stepped ticket-print animation clipped by the rail's slot.
- From toybox: a well-lip on every bar track. Broadcast's track against plate is 1.64:1, and its ink bar outline against plate is 1.15:1; add a rim-tin or #7A7390-class lip at 3:1 or more against the plate, and darken the track so vital and foe clear 3:1 by more than 0.3.
- From toybox: the cooldown spell icon as the UProgressBar FillImage (BottomToTop, Mask fill style) over a desaturated copy. Keep Broadcast's hatched shutter only for the Silenced state.
- From toybox: the single-pass SDF M_UI_Token material for widgets with many instances (enemy indicators, tags), so 20 plates do not stack 3-layer Overlays.
- From toybox: a peeled-corner 'new' marker on spike tickets and feats until they are viewed, and dialogs that default focus to the safe choice.
- Layout fixes the winner must absorb:
- Narrow the boss bar to 880 px or less (x 520-1400) so it clears the gauge plate (ends x 488).
- Give ticket lines an ellipsis rule or a two-line title.
- Make the filter row a UWrapBox at body-s, or remove the glyphs, so #37 cannot recur at 150% text.

