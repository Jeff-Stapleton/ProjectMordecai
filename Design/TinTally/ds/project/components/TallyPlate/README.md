# TallyPlate

The big-moment plate at top centre: a 640 x 128 tin plate with a badge, kicker, title and cause, and the Tally's optional remark hung below it. Use it for feat unlocks, skill techniques (milestones), upgrade keys, traversal tools and level ups, out of combat only; it is also where the Tally goes dark and prints "Count closed." on the Acceptance ending.

## Anatomy

| Part | Spec |
|---|---|
| Plate | `.tt-plate.tt-plate--moment` (Plate): 640 x 128, anchor top-centre (0.5, 0), offset (0, 40), alignment (0.5, 0); offset (0, 168) while a boss bar is up. `plate` fill, `rim` / `rim-shade` 2 px, radius 12, 3 px `ink` outline (4 in High Contrast), print (0, 4). The fill is 100% opaque; the whole widget fades for show and hide. |
| Rivets | Plate's `.tt-plate--riveted`, the system rivets: four 12 px pressed rivets 8 px inside the rim's corners (a `rim-shade` face, a `rim` highlight, a 2 px `ink` ring). Allowed because this is a Tally plate (D13); never on HUD plates or World-voice plates (banners, boss bar, death). |
| Grid | Padding 0 24, so the badge (from x 26) and the text clear the rivets (x 10-22 and 618-630). Badge column 96, gap 18, text column from x 140, 474 px wide, vertically centred. |
| Badge | 96 x 96 silhouette: 3 px `ink` edge (4 in HC) around a coloured face, print (0, 4). The silhouette carries the kind (table below). Content in `ink`: the feat's own glyph at 48 (40 in the starburst, so its points stay clear), the rank number in `num-xl`, or the item or tool glyph at 48. |
| Kicker | `kicker` (Rowdies 400, 20/24, tracked 0.08 em, ToUpper) in the accent colour. |
| Title | `tally-title` (Rowdies 700, 48/54), `text`, one line, in the full 474 px column. "Philosopher's Stone" (471 px) fits. A wider title drops to `tally-heading` (32/38): "Two-Weapon Fighting" is 531 px at 48 and 354 px at 32. Past 474 px at 32, shorten the data. |
| Cause | `ui-label` (Next 600, 20/26), `text-muted`, one line, ellipsis: what the player did, or what the thing now does, plain and complete. |
| Technique tally | Technique plate only: the 20-stroke rank tally at the Tally card's skill-row size (24 tall, stroke 3, pitch 7: 148 px), 12 px after the kicker on the kicker row, so the title keeps the whole column. Lit strokes in `text` (lit rank strokes are `text` on plate, never `good`); unlit strokes are the core dotted strokes (`lip`), so lit and unlit differ by shape in both themes. Slashes close a bundle at 5, 10, 15 and 20; rank 1 carries the cap dot; Rank 20 is the full bundle in `text`. "TECHNIQUE · RANK 20" (256 px) + 12 + the tally is 416 px. |
| Drawback box | Rare and Legendary feats only (and a tool with a cost, below). The plate grows to 640 x 184 (padding-top 10; rows 104 + 44, gap 10) and the one Drawback box, core `.tt-drawback`, spans it: the `.tt-hatch` strip (`danger` / `ink`, 7 px stripes), then "Drawback" inline in `danger-text` 800 and the DrawbackText in `text`, `ui-label`, on `plate-raised`. One line. |
| Remark tag | Tag's `.tt-tag-hang.tt-tag-hang--light`: the light cord (a 4 px `rim` core with 1 px `ink` sides, core `.tt-string--light`), which drops 18 px from the plate into the eyelet at x 140 (the text column) and holds on every floor. An `enamel` tag, 48 high, with the `seal` glyph (20) and the remark in `ui-label` `ink-muted`. At most 518 px wide, so it ends (outline included) inside the plate's 640 px box: about 430 px of text after the eyelet and the seal. |
| Ending slot | Ending only: a 240 x 6 `ink` slit with a 2 px `lip` edge cut through the bottom rim, centred under the seal (x 200-440), like the rail head's ticket slot. |
| Last ticket | Ending only: a plain `.tt-tag`, at least 250 wide, with "Count closed." in `ui-title` `ink`. It hangs on a 22 px light cord (`.tt-tag-hang--light`) from the slot's left end: the eyelet sits at x 212, 12 px inside the slot, as tickets leave the rail head. |

Badges (one texture set with FeatsTab and the spike):

| Kind | Silhouette | Face | Content | Accent (kicker) |
|---|---|---|---|---|
| Feat, Common | circle | `feat-common` | the feat's own glyph (Pyromaniac: `fire`) | `feat-common` |
| Feat, Rare | FLAT-TOP hexagon | `feat-rare` | the feat's own glyph | `feat-rare` |
| Feat, Legendary | eight-point starburst | `feat-legendary` | the feat's own glyph at 40 | `feat-legendary` |
| Technique | rounded square (16) | `good` | rank "10" in `num-xl` `ink` | `good` |
| Level up | rounded square (16) | `enamel` | two odometer drums (`well`, 2 px `lip`, 40 x 62) with the level in `num-xl` `text` | `good` |
| Upgrade key | key tag (chamfered top corners, a grommet: 12 px `well` hole, 3 px `enamel-shade` ring, 2 px `ink` ring) | `enamel` | `upgrade-key` glyph | `good` |
| Tool | key tag, as above | `good` | the tool's glyph: `explosives`, `magnet`, `shock-device`, `invisibility`, `phase`, `glider` | `good` |
| Unlit (ending) | circle | `well` with a 2 px `lip` ring inside the ink edge | `seal` glyph at 64 in `text-disabled` | none |

## States

| State | Trigger | What shows |
|---|---|---|
| Feat unlocked, Common | `Mordecai.Event.FeatUnlocked`, Tier Common | Circle badge with the feat's glyph, "Feat unlocked · Common", title, cause, remark tag. 4 s. |
| Feat unlocked, Rare | Tier Rare | Flat-top hexagon, 184 high with the Drawback box. 4 s. |
| Feat unlocked, Legendary | Tier Legendary | Starburst, Drawback box. 5.5 s. |
| Technique | `Mordecai.Event.SkillMilestone` (ranks 1 / 5 / 10 / 15 / 20) | Square rank badge, "Technique · Rank 10" + the 20-stroke tally, the skill name, the milestone text. 4 s. |
| Upgrade key | Pickup of an UpgradeKey item | Key-tag badge on `enamel`, "Upgrade key", the item name, what it unlocks. 4 s. |
| Tool | A traversal tool is acquired (TODO(DECISION): no tool system exists) | Key-tag badge on `good` with the tool's glyph, "Tool", the tool's name, what it opens ("Opens cracked walls.", example). A tool with a cost (the Phase Talisman drains Health) shows it in the Drawback box and grows to 184. 4 s. |
| Level up | TODO(DECISION): no Level or XP system exists | Drum badge, "Level up", "Level 7", "+1 Skill Point". Specified so it lands in the family. |
| Tally remarks Off | Settings > Accessibility > Tally remarks | No tag hangs. Nothing else moves. |
| Under a boss bar | A boss bar is showing | The plate drops to (0, 168) and no remark tag hangs. A plate ends at y 296, a Drawback plate at y 352; that is inside the Banner box, which is safe only because a plate never shares the screen with a Banner (a boss intro or phase banner). |
| Unlit | Acceptance ending | Plate's `.is-unlit`: rim to `lip` / `well` (High Contrast keeps the whole white outline), rivets off, the seal badge dark with its `lip` ring. The slot opens; "Count closed." prints from it and stays. |

Pacing (ReceiptTimeline is the source of truth):
- Plates never print during the combat hold, and the 20 s cap never forces them. They print in the receipt after the slips and tickets, one at a time.
- A plate never shares the screen with a Banner: it waits until the banner has gone.
- A plate still waiting when the pause menu opens prints after the menu closes, once it is calm. It is on the spike as well.
- Tally notifications Full, Calm and Minimal all print plates (4 s / 5.5 s).

Motion (cited by name from the brand book's Motion table; one plate-stamp sound):
- **In / out:** PlateDrop. The badge strikes with StampReveal, at the offset the PlateDrop row gives.
- **Remark tag:** once StampReveal has landed, it swings in on its cord with TicketDrop; it leaves with the plate.
- **Unlit:** the colours step to `.is-unlit`, then the last ticket drops from the slot with TicketDrop. No shake, no stamp. The step's timing belongs to the ending sequence (TODO(DECISION) below).
- **Reduce Motion:** PlateDrop, StampReveal and TicketDrop become the system's fades.

## Data and implementation

- **Where it lives:** `UMordecaiTallyPlateWidget` (proposed), bound as `TallyPlate` (BindWidgetOptional, with a C++ fallback like the rest of the HUD), is a child of the HUD root canvas (`UMordecaiCombatHUDWidget`, reparented to `ULyraActivatableWidget` and pushed ONCE on `UI.Layer.Game`). Never push it on its own: a second push on the layer stack hides the HUD. It is owned by the Tally queue (see TallyRail and ReceiptTimeline).
- **Its own BindWidgetOptional names:** `PlateBorder`, `BadgeImage`, `BadgeGlyph`, `BadgeText`, `KickerText`, `RankTally`, `TitleText`, `CauseText`, `DrawbackBox`, `DrawbackText`, `RemarkTag`, `RemarkText`, `EndingSlot`, `LastTicket`, `LastTicketText`.
- **Assets (03, the asset manifest):** the **Plate stack** (RoundedBox, no plate textures), with Plate's rivets (`WBP_TT_Plate` with `bRiveted` on; hidden when unlit); the **Badges** (96 px silhouettes, an `ink` edge layer and a white face layer tinted per kind: feat circle, flat-top hexagon, eight-point starburst, the technique rounded square, the key tag), with the glyph Image on top; the **Tally atlas** (`UMordecaiTallyWidget`, `Count`, `Max`) for `RankTally`; `M_UI_PatternFill` (sampling `T_UI_Hatch16`) for the Drawback hatch; the **Tag stack** and **Strings** light cord for the remark tag and the last ticket.
- **Title size:** pick the style in code: measure the title with `FSlateFontMeasure` at `TS_Tally_tally-title`; if it is wider than 474 px, use `TS_Tally_tally-heading`.
- **Feat:** `Mordecai.Event.FeatUnlocked` and `OnFeatUnlockedBP` (US-012). Title = `UMordecaiFeatDataAsset::DisplayName`; tier = `Tier`.
  - TODO(DECISION): the feat text fields are proposed once, on `UMordecaiFeatDataAsset`, and shared with FeatsTab and TallyRail: `CauseText` (this plate's cause line, "Burned 6 enemies at once."), `DrawbackText` (the Drawback box, "Blocking costs 25% more Stamina."), and a `DisplayLabel` per unlock condition (TallyRail's FEAT PROGRESS footer, the Feats tab's condition bar). All three are proposed. Until they exist, the cause uses the condition in plain words or the Description, and the Drawback box shows the Description of the `DrawbackEffects`.
  - TODO(DECISION): the medallion carries the feat's own glyph everywhere (Pyromaniac = `fire`), but feat assets have no icon field. Proposal: an `Icon` (glyph name) on `UMordecaiFeatDataAsset`; until it exists, the badge falls back to the `feats` glyph.
- **Technique:** `Mordecai.Event.SkillMilestone` and `OnSkillMilestoneReachedBP`; the tag `Mordecai.Skill.<SkillName>.Rank<N>` gives the rank. The line is the milestone description from the skill data (`FMordecaiSkillTreeMilestoneRow`), with its `{x}` placeholders resolved; if it is empty, the line reads the skill's category.
- **Upgrade key:** an item with ItemType UpgradeKey prints this plate instead of a slip. Title = DisplayName; line = ShortDescription.
- **Tool:** TODO(DECISION): the six traversal tools exist only in the world design (`world_design_v1`: Explosives from the Verdant Wilds, Magnetic Device from the Searing Expanse, Invisibility Potion from the Mire of Echoes, Phase Talisman from the Crystal Caverns, Glider Cloak from the Storm Peaks; the Shock Device's origin is open). There is no tool item type, tag or grant event in code. Proposal: a `Mordecai.Tool.<Name>` tag granted to the player, and a tool data row with DisplayName, a glyph name and a one-line `UnlockText` for the cause. The plate prints after the boss fight, in the receipt, like any plate.
- **Level:** TODO(DECISION): no Level or XP attribute exists. Level max is 60; "+1 Skill Point" assumes one point per level, which is also undecided (US-011 skill point economy).
- **Ending:** TODO(DECISION): no ending flow exists yet. When the Acceptance ending starts, set the plate and the rail head to `.is-unlit`, show `EndingSlot`, then print `LastTicket`; the ending sequence then takes the screen.
- **Sizes in UMG points (px x 0.75):** title 36 (24 when it drops to `tally-heading`), kicker 15, cause 15, `num-xl` 42, last ticket 19.5.
- **Tokens:** `plate`, `plate-raised`, `rim`, `rim-shade`, `lip`, `ink`, `well`, `enamel`, `enamel-shade`, `text`, `text-muted`, `text-disabled`, `ink-muted`, `good`, `danger`, `danger-text`, `feat-common`, `feat-rare`, `feat-legendary`, `kicker`, `tally-title`, `tally-heading`, `ui-label`, `ui-title`, `num-xl`, `radius-plate`, `stroke-rim`.

## Copy

- Kickers (sentence case in data, ToUpper on screen): "Feat unlocked · Common", "Feat unlocked · Rare", "Feat unlocked · Legendary", "Technique · Rank 10", "Upgrade key", "Tool", "Level up".
- Real data: "Pyromaniac" / "Burned 6 enemies at once." (the docs' only feat, Common, "Burn 6 enemies at once", glyph `fire`); "Philosopher's Stone" / "Unlocks the next Mage Tower tier." (from the design doc's "Philosopher's Stone unlocks Mage Tower tier"); the tool names "Explosives", "Magnetic Device", "Shock Device", "Invisibility Potion", "Phase Talisman", "Glider Cloak" (world design).
- Examples (invented for the preview, not game data): the Rare feat "Steel Rhythm" (glyph `block`, "Landed 50 perfect parries.", drawback "Blocking costs 25% more Stamina."), the Legendary feat "Living Furnace" (glyph `fireball`, "Burned 6 enemies at once, 10 times.", drawback "Frost damage taken +50%."), the tool cause "Opens cracked walls.", the Dodging rank 10 line "Dodging now cancels light-attack recovery." and the Two-Weapon Fighting rank 20 line "Combo finishers now strike with both weapons." (paraphrases of the skill sheet).
- Level up (TODO(DECISION)): "Level 7", "+1 Skill Point".
- The box word is "Drawback". The last ticket is "Count closed." with the full stop, and nothing else.
- Tally remarks (example copy, awaiting a tone read): at most 9 words, deadpan and literal, about the count rather than the player, and they fit the 518 px tag (about 430 px of text). Examples: "Six at once. The Tally checked twice.", "Fifty parries. The Tally kept time.", "Ten. A tidy number.", "One stone. Philosophy not included.", "Seven. Up from six, as forecast." Never on the ending, never about failure, story, NPCs or the Lich King, never "Filed under ...". The plate must be complete without it.

## Do / Don't

- Do let the silhouette and the tier word carry the feat tier; the colour only supports them.
- Do put the feat's own glyph in its medallion, the same glyph the Feats tab and the spike use.
- Do draw the Rare hexagon flat-top, as everywhere else.
- Do show the drawback on the plate itself for Rare and Legendary, in the one Drawback box (`.tt-drawback`). It is part of the headline, not a remark.
- Do light the technique tally in `text`; Rank 20 is the full bundle in `text`.
- Do hang the remark on the light cord at the text column (x 140) so it reads as the Tally's second line on any floor.
- Do drop to (0, 168) under a boss bar and hang no remark there.
- Don't print a plate during the combat hold, or while a banner is up.
- Don't put the cause or the drawback in the remark tag; a remark is optional and can be switched off.
- Don't shrink the title below `tally-heading`; shorten the data instead.
- Don't build a local drawback band or hatch: use `.tt-drawback`.
- Don't light the technique tally in `good`; a gold full bundle means rarity Gold only.
- Don't light the plate again after the Acceptance ending, and don't add a kicker, remark or ornament to "Count closed.".
