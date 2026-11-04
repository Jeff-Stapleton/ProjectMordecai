# ServiceScreen

The town service screen pattern, drawn here as the Alchemist's Restock. Over the veil it hangs a two-string nameplate, a list of what the service works on, the selected one in a tray, a quote counter and one action keycap; the Alchemist is the second instance after the Mage Tower's Identify. Use it for every building service in town (Mage Tower, Alchemist, and later Blacksmith and Clinic); the numbers on this card are examples, because the Alchemist's stock, field cap and restock rules are not designed yet.

## Anatomy (1920x1080)
The pattern is the Mage Tower's frame with PauseShell's service-screen hooks; this card reuses those classes and adds the list rows.
- **World and HUD**: the town under the `veil`; the HUD (`UI.Layer.Game`) stays under it, dimmed, never blurred. The preview draws the toy town and HUDLayout's markup.
- **Nameplate**: PauseShell's service-screen nameplate (`.tt-pause__title--screen`: the enamel sign on two 52 px light strings, the sign at y 52), the building glyph (32) and "Building · Service" in `tally-heading`, `ink`: "Alchemist · Restock".
- **Shell**: one menu plate (`.tt-mage-shell`: `plate`, menu print) at (96, 140), 1728 x 796, padding 24, two columns 560 | fill, 24 apart.
- **List panel** (`.tt-mage-panel`: `plate-raised`, `lip`, radius 10, padding 16 / 18 / 18):
  - Head: the service's glyph, the list name in `ui-title` ("Village stock") and, on the right, the building tier in `ui-label`, `text-muted` ("Tier 1 · Functional").
  - **StockRow** (`.tt-svc-row`, 84 tall, 12 apart): a `plate` card with a 2 px `lip` border, radius 10. A 52 px `well` socket with the item glyph (32, `text`); the name in `ui-title`; one line in `ui-label` with the counts in `num-s` ("Carry 3 / 5 · Shelf 6 / 8", 10 px either side of the "·"); on the right the carry pips (`.tt-pips`): one pip per point of the field cap, filled = carried, hollow = room.
  - The Mage Tower is the one exception, as the brand book records it: enamel tickets on a rod select by sliding, not by fill (its sealed tags slide 12 px). Every other instance uses StockRow-style plate cards with the D6 Selected look.
  - **Next batch**: a `.tt-divider`, the line "Next batch · after your next expedition" and dotted-leader rows (MageTower's quote rows) with the batch size per item ("+4").
- **Tray** (`.tt-mage-tray`: `well`, `lip`, radius 12): the one rail (core `.tt-railbar`, 440 wide, 40 px from the top), the core light string (`.tt-string--light`) into the reinforced ear (`.tt-mage-ear`) of the selected item's ItemCard, hung at 1.25 with a render transform (the Mage Tower hangs at 1.4; this counter is a row taller). The card: the rarity band, the name and type, a stat strip ("You carry", "Field cap", "On the shelf") and the item's description.
- **Counter** (`.tt-mage-counter`: `plate-raised`, `lip`): quote rows with dotted `lip` leaders ("Take ...... 2", "You carry ...... 3 → 5", "Shelf ...... 6 → 4"; the arrow is the `arrow-right` glyph), one plain line in `ui-body`, `text-muted`, and the action keycap (`.tt-mage-identify`, 300 x 72: `enamel`, 3 px ink border, 6 px lower lip, ink outline and print, the Enter keycap leading "Restock" in `ui-title`).
- **Footer**: core `.tt-hintbar` placed by PauseShell's `.tt-pause__footer`, centred, with `--tt-pause-footer-top: 966px` (under the taller shell): "[Esc] Back | [Enter] Restock". Keyboard and mouse only (D5); the gamepad hint bar is in the spec band.

## States
| State | Look |
|---|---|
| StockRow idle | `plate` card, `lip` border, `text` name, `text-muted` line. |
| Selected (D6) | The `enamel` fill with `ink` text (core `.tt-selectable.is-selected`); the line turns `ink-muted`; the pips switch to `.tt-pips--enamel`. The tray and the quote follow the selection. |
| Selected + focused | The enamel fill with core `.tt-focus--fill-gap`: an `ink` ring in the gap, the `focus` ring outside it. Keyboard focus and selection move together. |
| Out of stock | Dashed `lip` border (unavailable), the line "Out of stock · Shelf 0 / 2" in `text-disabled`. Still focusable so the player can read it; Restock is disabled. |
| Locked by building tier | Dashed border, the `lock` glyph in a dashed socket, `text-disabled` name and the tier it needs ("Needs Alchemist Tier 2 · Improved"). Not focusable. |
| At the field cap | All pips filled ("Carry 5 / 5"); the quote reads "Take 0"; Restock is disabled. |
| Empty shelf | The list keeps its rows (each Out of stock); a fully empty list uses the one empty state (`.tt-empty`, example copy "Nothing on the shelf." with the sub-line "The next batch comes after an expedition."). |
| RestockButton | Idle; Pressed (sinks 4 px, its lip collapses, the Enter keycap presses: KeyPress); Focused (the core ring right outside its ink outline); Disabled (`plate` face, dashed `lip`, the `lock` glyph, `text-disabled`). |
| After Restock | The row's pips fill, the shelf count drops, the HUD's quick-item count rises when the screen closes. No stamp and no ticket here; the rail prints nothing for a restock (TODO(DECISION) if it should). |

The preview draws the selected Health Potion at full size (keyboard and mouse), then a StockRow and RestockButton state sheet.

## Data and implementation
- **Widget**: one native base, `UMordecaiServiceScreenWidget` (`ULyraActivatableWidget`, InputConfig Menu, `bIsBackHandler`), pushed on `UI.Layer.Menu`; each building is a subclass or a data asset on it (`UMordecaiMageTowerWidget` is the first). Opened from the building's NPC (the Dialogue choice "Restock my potions.") or its interact prompt. CommonUI pools it: bind in `NativeOnActivated`, unbind in `NativeOnDeactivated`. Whether it pauses the game is the Mage Tower's TODO(DECISION).
- **Named widgets** (`Mordecai*` native classes so the bridge can read and click them): `TitleText` (the nameplate), `ServiceList` with `StockRow`s (`NameText`, `CountsText`, `CarryPips`), `TierText`, `NextBatchBox`, `ServiceTray`, `TrayCard` (ItemCard), `QuoteRows`, `QuoteBodyText`, `ActionButton` (its first TextBlock reads "Restock"), `EmptyText`.
- **Input**: Up / Down (D-pad, left stick) move the selection; Enter or gamepad A restocks the selected row (CommonUI accept); Esc or gamepad B goes back. Gamepad hint bar: "B Back | A Restock". No new key is bound.
- **Data, real**: the Health Potion (Common, "Consumable · stacks to 10", "Restores a chunk of health. The Alchemist limits how many you can field.", from the test pickups in `Scripts/create_test_pickups.py`). The Alchemist provides health potions, antidotes / cures, resistance tonics and specialty expedition consumables (`village_system_v1` 4.1). Healing pressure comes from Alchemist stock and replenishment, not carry capacity (4.1, `agent_rules_v2` 2). Building tiers are named Functional, Improved, Advanced, Masterwork (2).
- **Data, example**: the stock list (Antidote, Fire and Frost Resistance Tonic, "Expedition supplies"), every shelf size, field cap, batch size, and the Tier 2 gate.
- **TODO(DECISION)**:
  - the field-cap model: a hard cap, a soft stock or refill-style provisioning (`agent_rules_v2` Open Item 5, `game_design_v2` 12). It decides whether the HUD's quick-item count reads "x3" or "3 / 5";
  - the restock clock: after each expedition (the preview's proposal), over time, or on return to town;
  - the shelf and batch sizes per building tier, and which items each tier unlocks; building tiers do not exist in code;
  - a price: there is no currency or wallet, so the quote shows no cost; Restock draws on the village shelf only;
  - Blacksmith (4.3) and Clinic (4.4) instances: not drawn; they reuse this frame with their own list, tray card and action verb.
- **Sizes**: nameplate centred, sign top at y 52; shell 1728 x 796 at (96, 140); list panel 560 wide, rows 84 tall; tray card 384 at 1.25 (480 wide); counter about 120 tall; action keycap 300 x 72; hint bar centred at y 966.
- **Tokens**:
  - Colour: `veil`, `plate`, `plate-raised`, `well`, `lip`, `rim`, `rim-shade`, `enamel`, `enamel-shade`, `ink`, `ink-muted`, `text`, `text-muted`, `text-disabled`, `rarity-common`, `focus`.
  - Type: `tally-heading`, `ui-title`, `ui-body`, `ui-label`, `num-s`.
  - Motion: KeyPress, TicketDrop (the tray card swings in when the selection changes).

## Copy
- Proposed: "Alchemist · Restock", "Village stock", "Tier 1 · Functional", "Carry", "Shelf", "Out of stock", "Needs Alchemist Tier 2 · Improved", "Next batch · after your next expedition", "You carry", "Field cap", "On the shelf", "Take", "Restock fills you up to the field cap from the village shelf.", "Restock", "Back".
- Example: "Antidote", "Fire Resistance Tonic", "Frost Resistance Tonic", "Expedition supplies", "Nothing on the shelf.", "The next batch comes after an expedition.", every number except "stacks to 10".
- Shipping item data, exact: "Health Potion", "Restores a chunk of health. The Alchemist limits how many you can field."
- No Tally remark on a service screen.

## Do / Don't
- Do build every town service on this frame: the nameplate, the list, the tray, the counter, one action keycap, the hint bar.
- Do show counts that are not ranks or rarity as pips (the field cap) or numerals, never tally strokes.
- Do disable the action with the lock glyph and a dashed edge, and say why in the row or the quote, never by colour alone.
- Do keep the quote honest: before and after values, no price until a currency exists.
- Don't hide out-of-stock or tier-locked items; they tell the player what to bring home.
- Don't add rivets to the shell or the rail: this is an in-game menu plate, not a Tally plate.
- Don't put text on the tray's `well` without a card or plate under it.
