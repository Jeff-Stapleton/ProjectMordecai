# ExpeditionSelect

"Choose an Expedition": one large enamel tag card per front-end playlist hung from the rail, a locked Continue / New Game slot, and compact dev cards for the test maps. Use it as the screen Play opens on the title screen, in place of Lyra's `W_ExperienceSelectionScreen` and its hosting, joining and replay screens (issues #59, #60).

## Anatomy
1. **Backdrop**: the front-end level (level art, not UI) with the camera dollied in over the title island (no rotation), so the island's top face fills the frame: the core `.tt-diorama` grass tiles and a few of the island's props (the sand path, the pond, trees, rocks, a bush, posts, flowers and the relic), under the `veil`. Never a blank backdrop, never blurred. Nothing sits on the veil but plates and tags.
2. **Header**: Tag's front-end header (`.tt-tag.tt-tag--header` hung in `.tt-tag-hang.tt-tag-hang--light`: one eyelet, the ink outline, the menu print), its top at (160, 72), 64 tall, "Choose an Expedition" in `tally-heading`, hung on ONE light cord from the top edge into its eyelet (centre (182, 104)). Never the in-game two-cord nameplate. Over the veil the enamel face carries the edge; the ink outline and print carry it on light ground.
3. **Rail**: core `.tt-railbar` (the one rail family) across the screen at (136, 200), 1648 x 20; its eyelet caps overhang 16 px (visual x 120-1800). A single card hangs on its own cord, which runs in front from the rail straight into the card's eyelet. The dev column is threaded on one cord (see 6). Cords are core's light string (4 px `rim` core, 1 px `ink` sides).
4. **Playlist card** (large vertical tag), 600 x 604:
   - Stub: a 56 px `enamel-shade` band with the eyelet at top centre (core Tag's grommet: 12 px `well` hole, 3 px ring, 2 px ink ring; the ring is `enamel` here so it stands out from the band), a kicker ("EXPEDITION", `kicker`, `ink-muted`) at left and a dashed `ink-muted` perforation under it.
   - Thumbnail: 552 x 284, radius 8: the playlist's `TileIcon`, framed by an ink ring of `--tt-outline` minus 1 (2 px, 3 px in High Contrast).
   - `TileTitle` in `tally-title` (Rowdies 48), `ink`; `TileSubTitle` in `ui-title`, `ink-muted`; `TileDescription` in `ui-body`, `ink`.
   - Footer: a 60 px `enamel-shade` band; when focused it shows [Enter] "Start" at the right, the keycap leading its label.
5. **Locked slot** (Continue / New Game), 360 x 604:
   - `plate` ground, a 2 px dashed `lip` border (dashed means unavailable), ink outline, no print.
   - A `plate-raised` stub inside the dashed border (so the dashed edge runs round all four sides, in High Contrast too), its grommet with a `lip` ring; a `well` with the `lock` glyph (64).
   - "Continue" and "New Game" in `tally-heading` `text-disabled`, each with the `lock` glyph.
   - The reason as the system's one empty state: core `.tt-empty` "Coming Soon" (a plain enamel tag, no eyelet, ink text), centred in the rest of the card.
6. **Dev cards** (non-Shipping builds only), 560 x 272 each, horizontal eyelet tags at x 1200, threaded on one cord at x 1222 (TitleScreen's threaded build): the cord runs behind the cards and shows in the 60 px gap, a segment crosses each stub from the top edge into the eyelet, and the cord ends in a knot 24 px under the last card.
   - Stub with the core tag eyelet (x 22) and a dashed perforation at x 44.
   - Thumbnail 176 x 224, then a "Dev" pill and the subtitle in `ui-label` `ink-muted`, the map name in `ui-title` `ink`, the description in `ui-body` `ink`.
7. **Dev version stamp** bottom-left and the **hint bar** (core `.tt-hintbar`) bottom-right: [←][→] "Move" | [Enter] "Start" | [Esc] "Back". Keyboard and mouse by default; after a pad is used: (D-pad) "Move" | (A) "Start" | (B) "Back", and the large card's footer reads (A) "Start" (drawn in the spec band).
8. **Thumbnails in High Contrast**: a `TileIcon` is level art, so High Contrast never recolours or flattens it; only its ink frame thickens. The stand-ins (a blockout floor, blocks, pawns, spawn pads) use only the theme-invariant `world-*` and `biome-*` tokens (enemy pawns borrow `foe`), and render the same in both themes.

## States
| State | Look |
|---|---|
| Open | Cards drop from the rail one after another, each with the TicketDrop swing; focus lands on the default playlist (`bIsDefaultExperience`), else the first card. |
| Default | Enamel card, ink outline, 6 px print (4 px on dev cards). |
| Hover (mouse) | Lifts 2 px, the print grows by 2 px. |
| Focused | Swings 1° about its eyelet and keeps its ink outline and print; core focus (ink, focus ring, ink keyline) right outside the outline; the large card's footer shows [Enter] "Start". |
| Confirmed | Sinks 3 px, the print collapses to 1 px, the face goes `enamel-shade` with a 5 px sunk lip (`ink` at 24%) along the top, a hole punches through the stub under the eyelet, then the LoadingScreen cuts in. |
| Locked | Plate ground, dashed lip, `text-disabled`, the `lock` glyph on every line, the `.tt-empty` "Coming Soon". Not focusable: navigation skips it (`SetIsFocusable(false)`, interaction disabled). |
| Reduce Motion | No drop and no swing; cards fade in; the punch is instant. |

- Navigation: Left / Right moves across the columns (skipping the locked slot); Up / Down within the dev column. Back returns to the title screen with focus on Play.

## Data and implementation
- New `UMordecaiExpeditionSelectScreen` (`ULyraActivatableWidget`), pushed on `UI.Layer.Menu` by `PlayButton`. `BindWidgetOptional`: `HeaderText`, `CardRow`, `DevColumn`, `ActionBar`.
- Cards come from `ULyraUserFacingExperienceDefinition` assets with `bShowInFrontEnd`, queried through the Asset Manager. Only Mordecai playlists (under `/MordecaiCore/`) are listed: Lyra's Elimination, Control, Exploder, Exploder Multiplayer and Test tiles never show (#59). `TODO(DECISION)`: remove the Lyra sample plugins, or filter them here.
- Card widget `UMordecaiExpeditionCard` (`UCommonButtonBase`) with `BindWidgetOptional` `TileTitleText`, `TileSubTitleText`, `TileDescriptionText`, `TileIconImage`, `KickerText`, `StartHint`. The button's text is `TileTitle`, so the bridge can `click_button "Mordecai Arena"`: the Tier 4 "choose the arena tile" playtest uses exactly that.
- Start: `Definition->CreateHostingRequest()` then `UCommonSessionSubsystem::HostSession` (offline), as Lyra's tile does today. No hosting, joining, bot or replay options are offered (#60).
- Today's only playlist is `/MordecaiCore/System/Playlists/DA_Mordecai_Arena`: TileTitle "Mordecai Arena", TileSubTitle "Dev test arena", TileDescription "Project Mordecai diorama arena (DevTestMap)", no `TileIcon`. `TODO(DECISION)`: a TileIcon render (552 x 284 for large cards, 176 x 224 for dev cards).
- Dev cards: `TODO(DECISION)` playlist assets for MovementTestMap and CombatTestMap (example names `DA_Mordecai_MovementTest`, `DA_Mordecai_CombatTest`) and a dev-only flag (`ULyraUserFacingExperienceDefinition` has none: a Mordecai subclass with `bDevOnly`, or an asset tag). Hidden in Shipping. Their titles and descriptions here are examples taken from the US-100 test-map notes.
- Continue / New Game: `TODO(DECISION)`. No save system exists; the slot stays locked with "Coming Soon", or is hidden until saves ship.
- Construction from the asset manifest (03): cards and the header are the tag stack (the header with the menu print); the locked slot is the plate stack with a dashed `lip` Border; the rail is the rail family (`RailImage`); cords are the Strings light cord. The single-card cords are Images in front of their card; the dev column's cord is one Image behind the cards with a `ThreadImage` in each card and a `CordKnotImage` at the end (see TitleScreen). The empty state is the `.tt-empty` tag (a plain tag stack, no grommet).
- Tokens: `veil`, `plate`, `plate-raised`, `enamel`, `enamel-shade`, `ink`, `ink-muted`, `well`, `lip`, `rim`, `text-disabled`, `focus`; type `tally-heading`, `tally-title`, `kicker`, `ui-title`, `ui-body`, `ui-label`. The backdrop uses `world-*` and `biome-*` only; the thumbnails use `world-*`, `biome-*` and `foe`, never other UI tokens.

| Element | Anchor | Offset | Size |
|---|---|---|---|
| Header tag | top-left | (160, 72) | auto x 64 (eyelet centre y 104) |
| Rail | top-left | (136, 200) | 1648 x 20 (caps to x 120-1800) |
| Playlist card | top-left | (160, 252) | 600 x 604 |
| Locked slot | top-left | (800, 252) | 360 x 604 |
| Dev cards | top-left | (1200, 252) and (1200, 584) | 560 x 272 |
| Hint bar | bottom-right | (-32, -32) | auto x 52 |

## Copy
- "Choose an Expedition", kicker "EXPEDITION", "Start", hints "Move", "Start", "Back".
- Playlist fields verbatim from the asset: "Mordecai Arena", "Dev test arena", "Project Mordecai diorama arena (DevTestMap)".
- Locked slot: kicker "SAVE", "Continue", "New Game", "Coming Soon".
- Dev cards (examples): "Dev", "Dev test map", "MovementTestMap" / "An enemy-free floor with steps.", "CombatTestMap" / "A passive TestDummy and a fixed-cadence TestAttacker."
- No Tally remarks on this screen.

## Do / Don't
- Do keep one cord per hanging column: a single card's cord ends in its eyelet; a threaded column's cord runs behind the cards and ends in a knot.
- Do size cards from their data: titles one line, descriptions at most two (large) or three (dev) lines.
- Do list only Mordecai playlists, and dev cards only outside Shipping.
- Do say why a slot is locked with the one empty state (`.tt-empty`), never a dashed pill.
- Don't offer Lyra's Quickplay, Host, Join or replay paths from here.
- Don't make the locked slot look selectable: no enamel face, no print, always the lock glyph.
