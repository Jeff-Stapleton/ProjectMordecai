# Tag

The enamel ticket: an `enamel` face with `ink` text inside a 3 px ink outline, punched with a grommet eyelet so it can hang on a string from the rail head or from a plate. Use it for Tally tickets, the interact prompt, hints that hang from the row they explain, the screen headers (the in-game nameplate and the front-end header) and item cards; use the plain variant (no eyelet) for a tag that never hangs.

## Anatomy
- **Face**: `enamel`. Corners are 4 at the eyelet end and 10 (`radius-tag`) at the far end.
- **Outline**: 3 px `ink` (4 px in High Contrast). **Print**: (0, 4) `ink` at 55% (`print-hud`).
- **Eyelet**: a grommet, 22 px across.
  - A 12 px `well` hole, a 3 px `enamel-shade` ring, then a 2 px `ink` ring.
  - Its centre is at x 22, vertically centred, and text starts at x 44.
  - It is the tag's pivot and the point every string meets.
  - In High Contrast `enamel-shade` is white, so the grommet reads as a black hole and a black ring on the white face.
- **Box**: padding 10 top and bottom, 16 right; minimum height 52.
- **Type**, always ink on enamel:
  - line: `ui-title` 26 / 32 700, `ink` (`.tt-tag__line`)
  - kicker: `kicker`, Rowdies 20 caps tracked 0.08 em, `ink-muted` (`.tt-tag__kicker`)
  - remark or footer: `ui-label`, `ink-muted` (`.tt-sub`)
  - count: Mono 700 24, `ink` (`.tt-tag__count`, "x3")
- **End slot** (`.tt-tag__end`): pushed right, 12 px after the text. It holds one of:
  - a count;
  - a **rarity socket** (`.tt-socket`): an `ink` well, radius 6, padding 4 / 6, holding the rarity badge (see Tally);
  - a keycap.
- **String**: 2 px `ink`, always plumb, ending at the eyelet centre. The tag carries its own edge; the string shows what it hangs from.
- **Light cord** (`--light` on `.tt-tag-hang` or `.tt-tag-stack`): core `.tt-string--light`, read through its custom properties (`--tt-string-light-w`, `-core`, `-shadow`): a 4 px `rim` core with 1 px `ink` sides. The 2 px ink string disappears on dark floors (1.2:1 on night, 2.5:1 on mire) and on the veil; the cord holds on every floor. Use it wherever the ground under a string can be dark, and on plates.
- **Rail stack** (`.tt-tag-stack`): one string drops from the rail head's ticket slot and threads every eyelet.
  - Tickets are 320 wide (`.tt-tag--block` fills the column), 8 apart (`space-2`), at most 4 hanging.
  - The string shows in the gaps and runs over each face from its top edge into its eyelet.
- **Hung hint** (`.tt-tag-hang`): a single tag on its own string from a plate edge.
  - `--tt-tag-drop` is the string length above the tag (18 to 32).
  - `--tt-tag-half` is half the tag height (26 for one line).
- **Plain** (`.tt-tag--plain`): no eyelet, 16 padding both sides, for a tag that never hangs (a prompt beside an enemy, an item card in a menu).

### Screen headers
Two headers, one per place. Both are enamel with `ink` text, the 3 px ink outline and the menu print (0, 6).
- **In-game nameplate** (`.tt-tag-nameplate` > `.tt-tag-nameplate__face`): THE screen header on every in-game screen: "Paused" (`UMordecaiPauseMenuWidget` `TitleText`), "Options", "Mage Tower · Identify", "World Map".
  - An enamel sign centred at the top of the screen, hung from the top edge on TWO light cords, each threaded into a grommet whose centre sits 26 px in from its end (grommet top 10).
  - Padding 11 / 64 / 13, one `tally-heading` (32 / 38) and an optional 32 px glyph 12 px before it; a 4 px `enamel-shade` lip along the bottom; radius 10.
  - `--tt-tag-drop` sets the cord length above the sign (140 on the pause menu, so the sign sits over the tab bar). A service screen sets its own drop; nothing else changes.
  - PauseShell, OptionsScreen, MageTower and WorldMap draw it today with PauseShell's `.tt-pause__title` classes at the same metrics.
- **Front-end header** (`.tt-tag.tt-tag--header`, hung in `.tt-tag-hang.tt-tag-hang--light`): its own variant for the front end ("Choose an Expedition" on ExpeditionSelect). A single-eyelet tag on ONE light cord from the top edge into its eyelet (x 22), 64 tall, padding 8 / 28 / 8 / 52, one `tally-heading` on one line. `.tt-tag--nameplate` is its earlier name, kept as an alias until ExpeditionSelect switches.
- Boss names are not tags: they sit on the BossBar's plate tab in the World voice.

### The dual edge
The ink outline holds on light floors (blockout 14.4:1, snow 16.8:1, sand 11.2:1, grass 7.6:1). The enamel face holds on dark ones (mire 6.6:1, night 14.1:1).

## States
- **Rest**: hanging still.
- **Printing** (`.is-printing`): one swing about the eyelet, Motion **TicketDrop** (the brand book's row; no local timing).
  - The far end starts LOW, so a new ticket never sweeps across the one above it, and the overshoot stays inside the 8 px gap on a 320 px ticket.
  - `.is-swinging` freezes the first frame for static mockups.
  - With Reduce Motion, the tag fades in place with no swing.
- **Focused** (`.is-focused` / `:focus-visible`): on the pause-menu spike, where filed tickets are a focusable list, and on front-end tags. The core default ring: 3 px `focus` right outside the tag's own 3 px ink outline (the ink in the gap), with the 2 px ink keyline outside it. The print stays.
  - The spike shows the SAME ticket kinds as TallyRail, compressed to one line (stub colour, kicker, name, count): SKILL UP, ITEM · PURPLE, STORED, FEAT PROGRESS, IDENTIFIED. PauseShell owns the spike; the preview's example uses TallyRail's slip classes.
- Queueing, merging ("Iron Ore x3"), the 4-ticket cap and "N more on the spike" belong to the ticket lane and the pacing rules (see HUD and pacing), not to the tag.

## Data and implementation
- **Construction**: an Overlay of UBorders with `RoundedBox` brushes (per-corner `CornerRadii` = top-left, top-right, bottom-right, bottom-left), bottom to top:
  1. Print: radii (7, 13, 13, 7), `ink` at alpha 0.55, render translation (0, 4).
  2. Outline: radii (7, 13, 13, 7), `ink`.
  3. Face: padding 3, radii (4, 10, 10, 4), `enamel`.
  4. Eyelet: three stacked RoundedBox Images centred at (22, middle): 22 x 22 `ink`, 18 x 18 `enamel-shade`, 12 x 12 `well`.
  5. Content: an HBox with padding (44, 10, 16, 10).
- **Pivot**: set the tag's `RenderTransformPivot` to (22 / width, 0.5) and animate `RenderTransform.Angle` for the swing.
- **String**: an Image in the parent canvas, from the rail slot to the eyelet centre: 2 px `ink`, or the light cord (a 4 px `rim` Image with a 1 px `ink` Image on each side). It is not a child of the tag, so it stays plumb while the tag swings.
- **Pickup prompt** (`UMordecaiPickupPromptWidget`, see InteractPrompt): an eyelet tag in the 540 x 60 slot at bottom-centre (0, -36), holding a 40 px keycap and the item name, with the rarity socket at the end.
  - Keep `PromptText` and `InventoryHintText` by name; the playtest bridge reads them directly.
  - Until device glyphs ship, `PromptText` keeps reading "[F] Pick up {0}" and the keycap widget is not shown.
  - When the keycap widget lands, the keycap carries the key and the TextBlock reads "Pick up {0}", in the same change as the playtest update.
  - `InventoryHintText` ("[I] Inventory") sits on a plate pill docked 12 px right of the prompt box (x 1242-1393, y 994-1034; see InteractPrompt), never on a tag and never under it. It is always visible during play, with or without a prompt.
- **Tickets** (new widget, ticket lane, see TallyRail): 320 wide under the rail head, top-right (-32, 104), 8 px gaps, at most 4.
- **Nameplate**: the in-game nameplate keeps `UMordecaiPauseMenuWidget` `TitleText` as its TextBlock ("Paused", sentence case). Build: the Tag stack below with two grommets instead of one eyelet, and two cord Images in the parent canvas (03 Strings), so the sign can swing while the cords stay plumb.
- **Front-end header**: the Tag stack with the eyelet, padding (52, 8, 28, 8), print (0, 6), one cord Image.
- **Tokens**: `enamel`, `enamel-shade`, `ink`, `ink-muted`, `well`, `rim`, `focus`, `print-hud`, `print-menu`, `radius-tag`, `stroke-outline`.

## Copy
- Shipping strings stay exact: "[F] Pick up {item}", "[I] Inventory", "Count closed." (the Tally's last ticket).
- Merged tickets read "Iron Ore x3". Inventory ledger rows read "Iron Ore  x1", with two spaces, in the menu.
- Kickers are written in sentence case in the data and cased with ToUpper at display ("Stored" shows as STORED).
- A Tally remark is an optional second line in `ink-muted`, at most 9 words, and never in combat. The headline must read complete without it.
- Screen headers are `tally-heading`, sentence case: "Paused" (shipping), "Options", "Mage Tower · Identify", "World Map", "Choose an Expedition".
- Example copy in the preview, not shipping strings: the hint "Exhausted: every action at ×0.5", "Riposte" (the EnemyTag's prompt), the spike entries "Dodging 6 / 20" and "Iron Ore x4".

## Do / Don't
- Do keep tag text `ink` or `ink-muted`; colour lives only in the socket or a keycap.
- Do hang strings plumb to the eyelet centre, and pivot the tag there.
- Do use the light cord where the floor under a string can be dark.
- Do use the plain variant for anything that never hangs.
- Don't put light text on enamel, or tint the face to show rarity or state.
- Don't swing a tag more than once per print, and never print tickets during the combat hold: they wait for the receipt.
- Don't use a single-cord tag as an in-game screen header, or the two-cord nameplate on the front end.
- Don't stack tags inside tags, or hang an eyelet tag with no string in a context where strings are shown.
