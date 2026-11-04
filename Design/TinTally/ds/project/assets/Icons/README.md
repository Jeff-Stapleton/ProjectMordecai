# Icons

The Tin Tally stamp glyphs as standalone SVG files, one per glyph (117), named exactly as in the system (`burning.svg`, `fireball.svg`, `upgrade-key.svg`). They are the same shapes `TinTally.glyph(name)` draws in the previews. The Glyphs component card is the contact sheet and holds the construction and "Kept apart" rules.

- **Single ink.** Every file is drawn in `ink` (#0F1418) on a 48 px canvas over a 24-unit grid. Recolour by tint, never by editing the file. The files use only paths, circles, `fill-rule="evenodd"` cut-outs and `rotate`/`translate`/`scale` groups, so any SVG rasteriser reads them.
- **Unreal.** Follow the Glyphs row of the asset manifest in 03-building-in-unreal:
  - Import each SVG, or rasterise it at 64 px as a white mask. Texture Group UI, compression UserInterface2D, no mips.
  - Tint it with the UImage brush tint from the theme asset: `text` on `plate`, `ink` on `enamel`, or the damage or rarity token where the colour is the meaning.
  - High Contrast flattens every icon to `text`.
- **Families:**
  - status (25), damage (16), spells (4), weapons (13);
  - items (17, with `bandage` and `tonic`);
  - system (23, with `search`);
  - options (4: `display`, `audio`, `controls`, `accessibility`);
  - HUD and world (4: `summon`, `sprint`, `reticle`, `speech`);
  - tools and town (11).
- **Changed in this round, same file names:**
  - redrawn on request: `quarterstaff`, `dagger`, `shortsword`, `corroded`, `bleeding`, `stoneskin`, `inventory`, `magic-missile`, `map`;
  - redrawn to clear collisions: `physical`, `corrosive`, `consumable`, `fireward`, `drawback`.
  Re-import those files. Asset names do not change.
- Always pair a glyph with its word in menus and tooltips. Arrows, checks and crosses are glyphs because the fonts have no such characters.
