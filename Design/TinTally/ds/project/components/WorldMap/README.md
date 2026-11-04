# WorldMap

A proposal for the US-073 world map: a tin tile board of biome cells. Charted cells are pressed per biome, uncharted cells debossed and unknown ground etched; paths and shortcuts are grooves, gates are keyhole stamps that show the tool they need, and the player is a token. Use it as the starting point for US-073 only; the map's form is TODO(DECISION) and nothing here is built yet.

## Anatomy (1920x1080)
The screen is drawn for keyboard and mouse (D5); the gamepad version is in the spec band.
- **World and frame** (the pause shell's family: it is its own screen, not a pause tab)
  - Behind the `veil`: the paused level as the toy diorama (D12) and the real HUD on `UI.Layer.Game` (D10), dimmed, never blurred.
  - **Nameplate**: PauseShell's service-screen nameplate (`.tt-pause__title--screen`): the enamel sign on two 52 px light strings (core's light cord) into two grommets, the sign at y 52, the `map` glyph (32, a folded map) and "World Map" in `tally-heading`, `ink`. No local overrides.
  - **Hint bar**: core `.tt-hintbar` (D25) placed by PauseShell's `.tt-pause__footer` with `--tt-pause-footer-top: 966px`: "[Esc] Back" only, keycap first. Gamepad (spec band): "[B] Back". "[M] Close" (gamepad "[View] Close", View as a pill keycap) joins the bar only once `IA_Mordecai_Map` is bound: M is proposed, and an unbound key is never drawn as bound (D5; the Tier 4 scripts press real keys).
- **Board plate** (`plate`, menu print) at (96, 140), 1344 x 800, padding 24.
- **The board**: a `plate-raised` tin tray with a 2 px `lip` and a punched dot grid (lip at 40%, 28 px pitch). Cells sit on a 140 x 112 pitch, 116 x 88 each, radius 14, so every groove is 24 px.
- **Charted cell**:
  - Fill is the biome token (`biome-verdant`, `biome-searing`, `biome-mire`, `biome-crystal`, `biome-storm`, `biome-withered`).
  - The plate's pressed rim (`rim` top-left, `rim-shade` bottom-right), a 3 px ink outline and a 5 px print.
  - Markers are glyphs in ink at 32: `diamond` boss lair, `chest` secret found.
  - Biome fills never carry text.
- **Uncharted cell** (a region the player has entered): pressed into the board. Fill is 34% biome mixed into `well`, a soft biome-tinted edge, a 6 px inner ink shadow at the top, no outline and no detail.
- **Unknown ground** (a region never entered): only an etched 2 px outline (`lip` at 60% on the tray), no fill, no biome tint and no name, so the board never gives away a region's biome before the player reaches it. It differs from the debossed cell by build, not by hue.
- **Fills are limited to the legend**: biome colour (charted), debossed biome (uncharted, entered), etched (unknown), `enamel` (town). Nothing else fills a cell.
- **Town cell**: `enamel` with the `keep` glyph and "Town" (Next 700, 20 px, ink).
- **Grooves** run under the tiles: 18 px channels, `well` fill, 2 px `lip`.
  - Shortcut open: a `rim` fill with a 3 px ink edge (a polished rail).
  - Shortcut locked: a dashed `lip` / `well` fill.
- **Gate stamp**: a keyhole, 62 x 78, centred on its groove, so it overlaps both cells by about 19 px. The disc carries the tool glyph (32); the slot points down the route it locks. Enamel face, ink edge, hard print.
  - Open: a 28 px `good` badge with the `check` glyph.
  - Tool unknown: `plate` face, `lip` edge, the `lock` glyph in `text-muted`.
- **Player token**: a 48 px enamel disc with the `player` glyph, then rings of ink, `vital` and ink, and a print.
- **Map cursor**: one cell-sized image (116 x 88, radius 14) on the top layer, above gates and the token. It is core focus with the fill-gap variant (`.tt-focus--fill-gap`): an `ink` ring over the cell's own outline, the 3 px `focus` ring outside it. No local ring. Gate stamps sit on the junctions, so a ring drawn by the cell itself would be covered; the cursor never is.
- **Region pills** (entered regions only): a core chip with the region name (`ui-label`, `text`) and its charted count in numerals ("2 / 6", `num-s`, `text-muted`). Never tally strokes: the tally mark has three jobs only (D1).
- **Side panel** (`plate`) at (1464, 140), 360 x 800:
  - The region card: "Current region" (`ui-label`, `text-muted`); the biome name in `world-heading`, upper case; "Charted" and "2 / 6" in `num-m`; the gates seen there with their tool glyph and a `check` or `lock`.
  - Under it, the legend: one row per fill and per symbol, 34 tall (46 for the two gate rows), no gap, samples in a 64 px cell, every sample glyph at least 20 px. The gate sample is the keyhole drawn at legend size (36 x 46, 2 px edge, 20 px glyph), never a scaled-down stamp; the player sample is a 28 px token with a 20 px glyph. The rows: "Charted (biome colour)" (a Verdant and a Searing sample), "Uncharted", "Unknown ground", "Town", "You", "Path", "Shortcut, open", "Shortcut, locked", "Gate, tool shown", "Gate, tool unknown", "Boss lair", "Secret found".

## States
- **Cell**: charted, uncharted (region entered), unknown ground, current (holds the player token).
- **Cursor**: on open it sits on the current cell; it moves cell to cell and may leave the player's cell. It is the only focus visual on the board.
- **Groove**: path (charted), shortcut open, shortcut locked (seen, not opened).
- **Gate**: tool shown (seen, locked), open (check badge), tool unknown (lock, plate face). This follows the map spec's rule: "The gate should be legible even before it is usable."
- **Region**: unknown ground until first entered; then its cells take the biome tint and its pill shows the name and the charted count.
- **Motion** (proposal): newly charted cells flip from debossed to raised when the map opens after a charting event; a gate that opens strikes its check badge (StampReveal); the cursor slides between cells. TODO(DECISION): the flip and the cursor slide need rows in the brand book Motion table (proposed names CellFlip and CursorSlide) before they get numbers; until then use TabSwitch's "in" timing. Reduce Motion: the brand book's fades, the cursor jumps.

## Data and implementation
- **Form**: TODO(DECISION). US-073 is planned and unspecified: a room-graph node map, a screen-grid map, a minimap, or this board.
- **Proposed widget**: `UMordecaiWorldMapWidget` (`ULyraActivatableWidget`, InputConfig Menu, `bIsBackHandler`) on `UI.Layer.Menu`, opened by `IA_Mordecai_Map` (M / gamepad View; the input action exists, no binding does).
  - Menu input mode blocks unbound keys, so closing with M needs a native `UI.Action.Mordecai.Map.Close` tag, a `+InputActions=` entry in `DefaultInput.ini` and `RegisterUIActionBinding` in `NativeOnInitialized`, closing on key-up like I in the pause menu. Esc / B close through the CommonUI back action. M and View are proposed bindings; nothing binds them today, so the hint bar shows "[Esc] Back" only until they exist.
  - The hint bar is a `UCommonBoundActionBar` styled as `.tt-hintbar` (PauseShell's), so its keycaps follow the last input device.
  - CommonUI pools activatable widgets: rebuild the board in `NativeOnActivated`.
  - Cells are generated from the room graph. Node types: Path, Arena, Gate, Vista, Secret, Service, DungeonEntrance, ShortcutReturn, Transition (`map_style_technical_design_v1.md`).
  - Each cell is a UButton for cursor focus; `MapCursor` is one UImage on an overlay above the gates, moved to the focused cell's slot in `NativeOnFocusChanging`.
  - Proposed named widgets: `TitleText`, `Board`, `MapCursor`, `PlayerToken`, `RegionNameText`, `ChartedCountText` (TextBlock, "%d / %d"), `GateList`, `Legend`.
  - Unknown ground: TODO(DECISION) whether unknown cells show at all, or only where the player has seen a route lead into them (a vista, a gate).
- **Examples in this preview, not data**:
  - The region layout, the cell counts, which biome sits next to which.
  - The gate order (Verdant's Explosives opens Searing; Searing's Magnetic Device opens the Mire), read from the unlock column in `world_design_v1.md`.
- **Real vocabulary**:
  - Biome names: Verdant Wilds, Searing Expanse, Mire of Echoes, Crystal Caverns, Storm Peaks, Withered Kingdom.
  - Tools: Explosives, Magnetic Device, Invisibility Potion, Phase Talisman, Glider Cloak, Shock Device. Glyphs: `explosives`, `magnet`, `invisibility`, `phase`, `glider`, `shock-device`.
- **Cartography** (`skill_sheet_v1.1.md`) adds markers for merchants, shrines and hazards at higher ranks. They use the same 32 px ink glyph slot on charted cells. TODO(DECISION): the marker set and glyphs.
- **Sizes**: nameplate centred with its sign at y 52; board plate 1344 x 800 at (96, 140); cell 116 x 88 on a 140 x 112 pitch; groove 18 wide; gate 62 x 78 centred on the groove (legend 36 x 46); player token 48 (legend 28); cursor 116 x 88; side panel 360 at (1464, 140); footer centred at y 966.
- **Tokens**:
  - Colour: `biome-*` (fills only), `plate`, `plate-raised`, `well`, `lip`, `rim`, `rim-shade`, `enamel`, `enamel-shade`, `ink`, `good`, `vital`, `text`, `text-muted`, `focus`, `veil`; the world stand-in behind the veil uses `world-*`.
  - Type: `tally-heading`, `world-heading`, `ui-title`, `ui-label`, `num-m`.

## Copy
- Proposed strings: "World Map", "Town", "Current region", "Charted", "2 / 6", "Legend", "Charted (biome colour)", "Uncharted", "Unknown ground", "You", "Path", "Shortcut, open", "Shortcut, locked", "Gate, tool shown", "Gate, tool unknown", "Boss lair", "Secret found", "Back", "Close".
- Biome names render upper case in the region card (World voice). No Tally remarks on the map: biome lines follow the tone-bleed rule.

## Do / Don't
- Do frame the screen with PauseShell's service-screen hooks (`.tt-pause__title--screen`, `--tt-pause-footer-top`) and core `.tt-hintbar`, never with local overrides of its classes.
- Do show a gate's tool as soon as the player has seen the gate.
- Do keep every label on a plate chip or the enamel town cell, never on a biome fill.
- Do keep cells chunky (116 x 88 minimum) so the board reads at 720p.
- Do draw focus with the one cursor image, above the stamps.
- Don't reveal an uncharted cell's markers, names or connections, or an unknown region's biome colour.
- Don't fill a cell with anything the legend does not list.
- Don't count with tally strokes; charted counts are numerals (D1).
- Don't rotate or pan the board on a diagonal; screen up stays world +X, like the camera.
- Don't build this before US-073 settles the form.
