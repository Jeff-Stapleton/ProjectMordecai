# PauseShell

The pause screen frame: the "Paused" nameplate, five folder tabs over the 1000 x 584 content plate, the hint bar and the Tally spike, over the veiled toy diorama and the real HUD. Use it for every in-game menu tab (Character, Skills, Feats, Inventory, Settings); each tab draws only inside the 948 x 540 content box, and service screens on `UI.Layer.Menu` reuse its nameplate and hint bar.

## Anatomy
All positions are design px at 1920 x 1080 (top-left origin). Every screen is drawn for keyboard and mouse (D5); the gamepad version is in the spec band under the screen.

| Part | Box | Build |
|---|---|---|
| World | full screen | The toy diorama (`.tt-diorama`: 96 px floor tiles and a few blocky props in `world-*` / `biome-*`), never a flat grid, never blurred (D12). In game this is the paused level itself. |
| HUD | its HUD map boxes | The real HUD (D10), HUDLayout's markup and classes: the always-visible pieces (D20) GaugePlate, the four SpellSlots, the TallyRail head ("Kills: 12", no badge: pausing filed the waiting tickets to the spike), the "[I] Inventory" pill and LoadoutPlate; plus what the example moment has: the stocked QuickItems row, copied from HUDLayout (G, the dashed unbound cap on Item A, 2, 3; Health Potion x2 as in the inventory; the Antidote on `tonic`), and on the five tabs the status strip with HUDLayout's fight-frame chips (Frostbitten 2 of 4 stacks, Focused), the same two CharacterTab lists. The strip (x 32-488) runs under the plate's left edge (x 460), as in game. It lives on `UI.Layer.Game`, under the veil. |
| Veil | full screen | `veil` (72%; 85% in High Contrast), flat, never blurred. No text sits on it. |
| Nameplate | x 839-1081, y 140-202 | Enamel sign, `tally-heading` "Paused" in `ink`, padding 11 / 64 / 13, radius 10, a 4 px `enamel-shade` foot, 3 px `ink` outline, 6 px print. Two grommets 20 px in from the top corners (12 px `well` hole, 3 px `enamel-shade` ring, 2 px `ink` ring). |
| Strings | x 863-867 and 1053-1057, y 0-145 | Core's light string (4 px `rim` with 1 px `ink` sides, through `--tt-string-light-*`) into each grommet. Light, so it holds on the veil in both themes. |
| TabBar | x 460-1460, y 220-276 | Five tabs centred, gap 6 (x 536-1384). Keyboard and mouse: nothing at the bar ends. |
| Tab | content width, min 140 | Padding 0 16, glyph 24 + 8 + label `ui-title`. 3 px `ink` border on top, left and right, none at the bottom; top radius 10. Measured widths: Character 188, Skills 140, Feats 140, Inventory 186, Settings 171. |
| TabContentBorder | x 460-1460, y 276-860 (1000 x 584) | `plate`, 2 px `rim` top/left and `rim-shade` bottom/right, radius 12, 3 px `ink` outline, 6 px print (`.tt-plate--menu`). Padding 20 / 24: the content box is 948 x 540 at (486, 298). |
| Hint bar | centred on x 960, y 892-944 (276 wide) | Core `.tt-hintbar` (D25): its own plate, keycap first, "Esc Back \| I Close", 32 px keycaps (`.tt-key--sm`). |
| Spike head | x 1500-1880, y 142-202 | Plate: kicker "The spike" in `text`, `tt-label` "Last 12 tickets" in `text-muted`. |
| Spike rod | x 1515-1521, tip y 204-220, foot at y 812 | 6 px `lip` rod with a 2 px `ink` outline and a 16 px pointed tip. It passes behind every entry and through its grommet. |
| Spike list (ScrollBox) | x 1500-1880, entries from y 222, clipped at y 748 | Entries newest on top, 56 tall, gap 8; 12 are kept. 8 show (to y 726) and the 9th shows its top 14 px (its edge and stub, no text) as the scroll cue. With one entry opened to 88 the 8th is cut 10 px under its text. Size the box so no entry is ever cut by 1-3 px: that reads as a mistake, not a scroll edge. The box reaches 10 px past the entries at the sides and top so a focus ring is never clipped. |
| Spike base | bar x 1521-1859, y 802-822 | Core `.tt-railbar` (D23): the rail family's teal bar with an eyelet in each end cap. The rod stands in the left cap (x 1518). |

**Spike entries (D9).** The spike shows the rail's own ticket kinds (TallyRail), compressed to one line at spike width, with TallyRail's classes:
- 380 x 56: the slip's height, the stub cut to 36 px (the grommet at x 18 is threaded on the rod; its hole shows the rod's `lip`), the perforation at x 36, text from x 46, right padding 10: a 324 px line.
- **Slip** (stub `rarity-common`): glyph, name (`ui-label` size at 700), and the count in a 38 px box ("x2"), or the rarity socket when the item does not stack.
- **SKILL UP** (stub `good`): kicker "Skill up", name, the rank "12 / 20" (`num-s`) in a 90 px box.
- **FEAT PROGRESS** (stub = the feat tier colour): kicker "Feat progress" and the name. The count is on the full ticket and the Feats tab: beside the 192 px kicker the name keeps 124 px, which fits TallyRail's feat-name rule (158 px at 26 px).
- **STORED** (stub `rarity-common`): kicker "Stored" and the resource names ("Iron Ore, Town Stone"); counts on the full ticket.
- **ITEM** (stub = rarity colour): kicker "Item · Purple" and the name.
- **IDENTIFIED** (stub = rarity colour): kicker "Identified" and the name.
- The kicker and the number never cut; the name ellipsizes. Measured: "Longswords" (111 px) fits beside "SKILL UP" and "12 / 20"; "Mysterious Amulet" fits beside "IDENTIFIED"; "Unidentified Purple Amulet" cuts after "Unidentified" beside "ITEM · PURPLE".
- Plates (feat unlocked, technique) are not spike entries: they wait and print after the menu closes. TODO(DECISION): 02-hud-and-pacing says a waiting plate is also "on the spike"; if it should be listed, give it a one-line form.

## States
- **Tabs** (the spec band shows all six):
  - Idle: 50 tall, `plate-raised`, 2 px `lip` inner edge on top, left and right, label `text-muted`; its foot tucks under the plate's outline.
  - Hover (mouse): 53 tall, fill `plate-raised` with 18% `lip`, a `rim` top line, label `text`.
  - Press: 47 tall (sinks 3 px), `plate` fill, the `lip` edge kept on all three sides with a 4 px `well` shade under the top one. In High Contrast the edge stays white; only the offset shows the sink (D22).
  - Selected: the cream `enamel` fill (core `.tt-selectable.is-selected`, D6), 56 tall, `ink` label and glyph, drawn above the plate so it covers the plate's top outline and meets the cream rim.
  - Focus, and Selected + focused: core's ring and keyline (`.tt-focus--own`, because the tab is open at the bottom). The 3 px `focus` ring sits right on the tab's own `ink` border, a 2 px `ink` keyline outside it, open at the bottom so the fused join stays intact. It goes on top of every other state.
- **Open / closed.** Opening drops the nameplate in on its strings with TicketDrop (it hangs from two grommets, like a ticket) while the column and the spike come in with TabSwitch's "in"; closing runs TicketRetract on the nameplate and TabSwitch's "out" on the rest. Reduce Motion: the brand book's fades, no swing. TODO(DECISION): if the menu needs its own timing, add a MenuOpen row to the brand book Motion table instead of local numbers.
- **Settings.** "Coming Soon" is the one empty state (core `.tt-empty`, D8), centred in the content box. No remark.
- **Spike.**
  - 0 to 12 entries; at 0 only the rod and the base show.
  - Unread: entries the pause filed (02-hud-and-pacing, Pause) carry TallyRail's peeled corner (16 px here) until they have been focused or the menu closes.
  - Focus (keyboard, gamepad) or hover: the entry opens to its full 88 px rail ticket at spike width (on screen: the Purple Amulet, kicker and socket over the name in two lines), so a cut name reads in full. Core focus ring outside its ink outline.
  - Activating an entry (click, A, Enter) jumps to its tab: SKILL UP to Skills, FEAT PROGRESS to Feats, every item kind to Inventory.
  - More than 8: the 9th entry's top edge peeks at the ScrollBox's foot; the list scrolls with the wheel or with focus.
- **Input device (D5).** One device per screen; never mix keyboard and gamepad caps.
  - Keyboard and mouse (every preview): no keycaps at the bar ends; hint bar "Esc Back | I Close". Tabs change by clicking a header. The tab buttons stay focusable, so arrow-key navigation can reach a header and Enter selects it; that is navigation, not a cycle key.
  - Gamepad (spec band only): LB / RB keycaps at the bar ends (x 460 and 1404, y 231-271), the tabs keep their place; hint bar "B Back | LB / RB Tabs".
  - TODO(DECISION): keyboard Previous / Next Tab keys. `DefaultInput.ini` binds `UI.Action.Mordecai.PauseMenu.PreviousTab` / `.NextTab` to the shoulders only. Proposal: Q / E, the keys that sit where LB / RB do in gameplay (Fireball, Blink); Menu input mode blocks game input, so they would not cast. Never draw them until they are bound: the Tier 4 playtests press real keys.
- **Contextual hints.** A tab may add hints for its focused widget, after the shell's own: "[Esc] Back | [I] Close | [Enter] Identify" while an Identify keycap has focus in the Inventory tab (spec band). At most two extra pairs; they never replace Back or Close, and they leave when the focus leaves.

## Data and implementation
- Widget: `UMordecaiPauseMenuWidget` (`ULyraActivatableWidget`, `InputConfig = Menu`, `bIsBackHandler = true`), pushed on `UI.Layer.Menu` by `UMordecaiPauseMenuSubsystem`. Keep the `BindWidgetOptional` names `TitleText` (the nameplate TextBlock), `TabBar` (a `UHorizontalBox`; a centred HBox needs no retype), `TabContentBorder`. A designer WBP may place `TitleText` outside the MenuColumn on its own canvas slot, as long as the name is kept.
- Proposed new optional names: `TitlePlate`, `PrevTabKey`, `NextTabKey`, `FooterBar`, `SpikePanel`, `SpikeList` (a ScrollBox), `SpikeBase`.
- MenuColumn canvas slot: anchors (0.5, 0.5), alignment (0.5, 0.5), size 1000 x 640: the TabBar 56 tall (an HBox, centre-aligned) over TabContentBorder 1000 x 584.
- `BackgroundDim`: replace the (0, 0, 0, 0.65) brush with `veil` (sRGB, convert with `FLinearColor::FromSRGBColor`).
- The HUD under the veil: the HUD root on `UI.Layer.Game` (D20, 03-building-in-unreal). The pause menu does not hide or redraw it.
- Tabs: `UMordecaiTabButton` (FName id, `OnClickedWithId`). Normal / Hovered / Pressed brushes as above; the Selected brush (enamel) is applied when the bar is rebuilt; the focus brush is the system ring Image (ink 2 | focus 3) with its bottom edge open. Keep the label as the button's first TextBlock child (the bridge reads it); the glyph is a UImage before it.
- Tab ids and labels, in order: `character` "Character", `skills` "Skills", `feats` "Feats", `inventory` "Inventory", `settings` "Settings". Default tab: character; the last tab is remembered.
- Actions: Escape / `Gamepad_Special_Right` toggle; CommonUI back (Escape, `Gamepad_FaceButton_Right`) closes; `UI.Action.Mordecai.PauseMenu.PreviousTab` (`Gamepad_LeftShoulder`) and `.NextTab` (`Gamepad_RightShoulder`) cycle with wrap; `.Inventory` (I) closes on key-up.
- Hint bar: a `UCommonBoundActionBar` styled as `.tt-hintbar` (its own plate, `FooterBar`), with `bDisplayInActionBar` on Back, Close and Tabs, so the keycaps follow the current input type from the input glyph data. `PrevTabKey` / `NextTabKey` are collapsed unless the input type is gamepad.
- Contextual hints: the focused widget in a tab registers its own action binding (`RegisterUIActionBinding` with `bDisplayInActionBar = true`, e.g. Identify on Enter / A); the bound action bar appends it after the shell's actions and drops it on focus loss.
- Service screens on `UI.Layer.Menu` (MageTower, WorldMap, OptionsScreen) reuse the nameplate and hint bar with these hooks instead of two-class overrides:
  - `.tt-pause__title--screen`: the nameplate on 52 px strings, a glyph (32) before the title, one line.
  - `--tt-pause-strings`: the string length (default 140 px) on `.tt-pause__title` or any ancestor.
  - `--tt-pause-footer-top`: the hint bar's top (default 892 px) on `.tt-pause__footer` or any ancestor.
- The spike needs a ticket history that does not exist yet. TODO(DECISION): the proposed `UMordecaiTallySubsystem` keeps the last 12 entries (kind, kicker, name, number, rarity, unread, target tab) for the rail and this spike. Each entry widget is TallyRail's `UMordecaiTallyTicketWidget` with a compact layout (a WidgetSwitcher: one line, or the full ticket while focused or hovered).
- Placeholder: `UMordecaiPauseMenuPlaceholderWidget` keeps `PlaceholderText` reading "Coming Soon", styled as the empty state.
- Type in UMG points (px x 0.75): `tally-heading` 24 pt, `ui-title` 19.5 pt, `ui-label` 15 pt, `kicker` 15 pt, `num-s` 15 pt, keycap letters 15 pt on the 32 px caps.
- Tokens: `veil`, `plate`, `plate-raised`, `well`, `lip`, `rim`, `rim-shade`, `ink`, `enamel`, `enamel-shade`, `text`, `text-muted`, `text-disabled`, `ink-muted`, `focus`, `good`, `rarity-*`, `feat-*`; the world stand-in uses `world-*` and `biome-*`.
- Motion: TabSwitch for the content; TicketDrop / TicketRetract for the nameplate on open and close. TicketDrop is not used on the spike (entries are filed, not printed).

## Copy
- Shipping strings, exact: "Paused", "Character", "Skills", "Feats", "Inventory", "Settings", "Coming Soon".
- Hint verbs: "Back", "Close", "Tabs" (one hint for Previous Tab and Next Tab, gamepad only); contextual verbs come from the tab ("Identify").
- Spike head: "The spike" (kicker, ToUpper at display), "Last 12 tickets".
- Spike entries reuse TallyRail's kinds table exactly (D26): "Skill up", "Feat progress", "Stored", "Item · {Rarity}", "Identified"; names and numbers as on the rail ("12 / 20", "x2").

## Do / Don't
- Do draw the selected tab over the plate's top outline; an outline under it breaks the folder.
- Do keep every text on a plate, tag or keycap; the veil carries no text.
- Do keep the strings light (`rim` with `ink` sides); an ink string disappears on the veil in High Contrast.
- Do draw the real HUD and the diorama under the veil in previews, never stand-ins or a grey grid.
- Don't size tabs to a fixed 180: labels are content-sized so translations fit; keep min 140.
- Don't add a ring around the selected tab's foot; the focus ring stays open at the bottom.
- Don't show LB / RB keycaps to a keyboard player, at the bar ends or in the hint bar.
- Don't print Tally remarks on the nameplate, the hint bar or the empty state.
- Don't put spec notes inside the 1920 x 1080 frame; they go in the spec band below it (D7).
- TODO(DECISION): a real Settings tab (the direction proposes the OptionsScreen sections Gameplay, Display, Audio, Controls, Accessibility).
