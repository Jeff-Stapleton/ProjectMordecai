# Glyphs

The stamp icon set: 117 flat, single-colour silhouettes for statuses, damage types, spells, weapons, items, system actions, options, HUD and world markers, traversal tools and town services. Use a glyph wherever a word alone would be hard to scan, and always next to its word in menus.

## Anatomy
- **Grid.** 24 x 24 units with a 20 x 20 live area (2 to 22). Filled shapes in `currentColor`, plus round-capped strokes where a line is the idea (2.0 to 3.6 units wide). Cut-outs use `fill-rule='evenodd'`.
- **Minimums.** No detail, hole or gap is thinner than 2 units, so every glyph survives at 20 px (13 px at 720p). The "Kept apart" rows on the card are checked at true 20 px pixels in both themes.
- **Turned shapes.** Blades, the staff, the bandage and the missile bolts are drawn upright and turned 45 degrees, so every weapon points up-right.
- **Dashes.** Only where the dash is the idea: `invisibility` (a figure you can't quite see) and the vanishing origin ring of `blink`. A dashed glyph never means locked, unbound or unavailable. That is the `lock` glyph, or the dashed rim of the container.
- **Tally strokes.** Only `seal` draws four strokes and a slash, because the seal is one of the tally mark's three jobs (ranks, rarity badges, the seal and throbber). No other glyph may draw tally strokes, or three tall bars that read as the Purple rarity badge. That is why `map` is a folded sheet.
- **Sets** (the contact sheet, in this order): Status 25, Damage 16, Spells 4, Weapons 13, Items 17, System 23, Options 4, HUD and world 4, Tools and town 11.
- **Sizes.** `.tt-glyph` is 24 px by default. Variants: `--20` (chips, rows, small buttons), `--24`, `--32` (the sheet, menu tabs), `--48` (spell-slot and item wells), `--64`. The sheet shows each glyph at 32 px on `plate` and at 20 px on `enamel`.

### Kept apart
Siblings that share a job are told apart by silhouette, never by colour or size alone. These are the groups the card shows side by side:

| Group | Members and their tell |
|---|---|
| Blades | `longsword`: long narrow blade, long guard. `shortsword`: wide pointed leaf (gladius), round pommel. `dagger`: short triangular blade, small guard, ring pommel. `quarterstaff`: straight staff, flat cut ends, two wraps a quarter in. |
| Helmets and shields | `armor`: helmet with a T visor. `corroded`: the same helmet with two bites out of its side and a heavy drip. `block`: nested shield. `fireward`: a ward dome over a small flame, on a base. |
| Figures | `player`: pawn. `stoneskin`: head and shoulders over a torso of three stone tiles. `summon`: a figure rising out of a portal ring under two sparkles. `character`: a framed bust. Also apart: `invisibility` (dashed bust), `accessibility` (figure in a ring). |
| Bottles and dressings | `potion`: round flask, half full. `consumable`: a bottle tipped to pour, one drop. `tonic`: tall corked vial. `bandage`: an adhesive strip with a raised pad. Only `heal` and `clinic` draw a cross. |
| Boxes | `chest`: wide box, domed lid, keyhole. `inventory`: a taller-than-wide pack with a small carry loop, a rounded flap and a buckle tab. `lock`: tall shackle, keyhole. `drawback`: a seesaw tipped down by a block (the trade-off). |
| Drops | `poison`: drop and bubbles. `drenched`: drop over a wave. `bleeding`: a large drop cut through by a slash, two small drops falling. `corrosive`: a pitted ring (three bites out of the rim) with a drop-shaped bore. |
| Bursts | `physical`: an uneven impact burst. `settings`: cog with a round bore. `new`: five-point star. `throwable`: four-point shuriken. The Legendary feat badge (a regular eight-point starburst) is a badge, not a glyph; `physical` stays uneven so the two never match. |
| Bars and streaks | `map`: three panels joined in a zigzag, with a pin hole (never three loose bars beside the Purple badge). `slash`: three curved claw marks. `magic-missile`: three star-headed bolts with short tails. |
| Targets and talk | `reticle`: four corner brackets and a dot. `focused`: four points turned inward. `exposed`: a ring crosshair. `speech`: two bubbles. `silenced`: one bubble with a cross. |

### Changed in this round (names unchanged)
- Redrawn: `quarterstaff`, `dagger`, `shortsword`, `corroded`, `bleeding`, `stoneskin`, `inventory`, `magic-missile`, `map`.
- Also redrawn to clear a collision: `physical` (it matched the settings cog once notched; now an uneven burst), `corrosive` (the drop over a basin read like `drenched` and `burning`; now the pitted ring the `dmg-corrosive` token names), `consumable` (was a second flask beside `potion`), `fireward` (was a second shield beside `block`), `drawback` (the old weight read as the padlock at 20 px, next to locked feats).
- New: `bandage`, `tonic`, `search`, `summon`, `sprint`, `reticle`, `speech`, `display`, `audio`, `controls`, `accessibility`.

## States
Glyphs have no states of their own. The container colours them:
- `text` on `plate`; `ink` on `enamel`.
- A damage, rarity or feat token where the colour is the meaning.
- `text-disabled` at `opacity-unaffordable` when unavailable.

High Contrast swaps the tokens, not the shapes.

## Data and implementation
- **HTML:** `<span class="tt-glyph tt-glyph--32" data-glyph="burning" aria-label="Burning"></span>`. `TinTally.mount()` draws it with `currentColor`. The names are the contract's list plus the eleven above; never invent one.
- **Unreal:** the **Glyphs** row of the asset manifest in 03-building-in-unreal. They are single-ink masks: import the SVGs, or rasterise them at 64 px as white masks (Texture Group UI, compression UserInterface2D). Tint them with the brush colour from the theme asset (`FLinearColor::FromSRGBColor`), so a theme can retint any of them. The files use only paths, circles, `evenodd` and `rotate`/`translate`/`scale` groups, which any SVG rasteriser handles.
- **Where they go:**
  - Status glyphs: `UMordecaiEnemyIndicatorWidget::StatusIconMap` and `UMordecaiStatusEffectIndicatorWidget` (`TintImage`).
  - Spell glyphs: the spell slots (`SpellCooldownA` to `D`).
  - Damage glyphs: the damage pops, on every non-Physical number. 03 builds them as a pool of `UMordecaiDamagePopWidget` (proposed). Today `UMordecaiDamagePopComponent` (a `ULyraNumberPopComponent`) draws the numbers, with no glyph. Physical numbers carry no type glyph (the `dmg-physical` token note stands). `physical` is for item cards, compare rows and legends. DamageNumber keeps the TODO about busy melee fights.
  - Weapon glyphs: LoadoutPlate and item cards. Item category glyphs: the inventory filter row (`FilterBar`).
  - System glyphs:
    - Arrows, check and cross for deltas, directions and conditions.
    - `lock` on disabled and locked controls; `unknown` for unidentified items; `pin` for equipped; `skill-point` beside Skill Points.
    - `seal` is the Tally's seal.
    - `drawback` marks a feat's drawback in rows and tooltips. The Drawback box itself is core `.tt-drawback` (the hatch strip and the "Drawback" label), with no glyph.
  - Feat medallions carry the feat's own glyph (Pyromaniac: `fire`). There is no per-feat icon field yet; TallyPlate holds that TODO.
  - Options: the OptionsScreen section tabs. Gameplay is `settings`; then `display`, `audio`, `controls` and `accessibility`.
  - `search`: the inventory search field. agent_rules_v2 requires "organization, filtering, and search", but no screen draws the field yet. TODO(DECISION): where the field sits on InventoryTab.
  - `summon`: marks a summoned enemy beside its name on EnemyTag, together with a lighter fill. Never mark a summon with a dashed rim (dashed means unavailable).
  - `sprint`: the sprint toggle (Left Shift, US-083, `UMordecaiHeroComponent::HandleSprintToggleInput`). TODO(DECISION): whether the gauge plate shows the toggle state.
  - `reticle`: the ground-target and aim reticle. TODO(DECISION): the world-marker layer (aim indicator, ground target, telegraphs) is not drawn yet, and may belong to VFX.
  - `speech`: "Talk to …" prompts and the NPC dialogue box. TODO(DECISION): no dialogue component exists yet.
  - `bandage` and `tonic`: quick items. Both are **example** items: no bandage or tonic exists in code. The tonics follow the Alchemist's resistance tonics in village_system_v1. TODO(DECISION): the item list.
- **Token notes:** `dmg-corrosive` names "the pitted-ring glyph", which is `corrosive`. `dmg-shadow` still says "crescent", but `shadow` is the spiral, and the glyph is the one to follow.

## Copy
- A glyph's name is an asset key, not copy. The word beside it is the display name: `stoneskin` "Stone Skin", `mage-tower` "Mage Tower", `skill-point` "Skill Points", `magic-missile` "Magic Missile", `accessibility` "Accessibility".
- Give every glyph that stands alone an accessible label in the display name: `aria-label` here, a tooltip or the row's TextBlock in UMG.

## Do / Don't
- Do pair a glyph with its word in menus and tooltips.
- Do use `arrow-up`, `arrow-down`, `arrow-left`, `arrow-right`, `check` and `cross` for deltas, directions and conditions: the fonts have no arrow, triangle or check characters.
- Do check a new glyph at true 20 px against its group in "Kept apart" before adding it, in both themes.
- Don't recolour a glyph to carry a meaning its silhouette does not already carry.
- Don't use a glyph below 20 px.
- Don't draw tally strokes in any glyph but `seal`, a cross in any glyph but `heal` and `clinic`, or a dashed outline to mean "unavailable".
- Don't draw hearts, rupees, gems or scrolls: the set stays original.
