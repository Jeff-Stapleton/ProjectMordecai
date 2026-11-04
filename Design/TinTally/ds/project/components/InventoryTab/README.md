# InventoryTab

The pause menu's Inventory tab: filter tags and a search field, the unlimited-carry item list as tin row slots, and a right column with the selected item's card over the Stored Resources ledger. Use it for the `inventory` tab of `UMordecaiPauseMenuWidget`; it replaces the fallback built by `UMordecaiInventoryWidget::BuildDefaultLayout()` and keeps every BindWidgetOptional name.

## Anatomy
All sizes are design px at 1920 x 1080. The screen is drawn for keyboard and mouse (D5); the gamepad version is in the spec band.
- **Shell** (PauseShell's, generated from the same markup): the toy diorama and the real HUD under the veil (D10, D12), the "Paused" nameplate on two light strings, the five folder tabs with Inventory selected and no keycaps at the bar ends, the 1000 x 584 content plate, the hint bar and the spike. This tab draws only inside the shell's content box: **948 x 540** at (486, 298).
- **FilterBar** (two rows, 104 tall): eight filter tags, then the search field, in one wrap row; 8 px apart both ways. At 948 px the tags wrap 5 + 3 and the search field fills the rest of row 2 (471 px).
  - A filter tag is SkillsTab's category tag: 48 tall, `plate-raised` face, 2 px `lip` edge, radius 4 at the grommet end and `radius-tag` at the other, a grommet at x 16 (12 px `well` hole, a 3 px `plate` ring, a 2 px `lip` ring), a 20 px category glyph (`all`, `weapon`, `armor`, `trinket`, `consumable`, `material`, `quest`, `magical`) and the label (`ui-label`).
  - Selected is the cream fill (D6, core `.tt-selectable.is-selected`): `enamel`, `ink` label and glyph at 700, an `ink` edge, a 4 px `enamel-shade` foot, and the core grommet (`enamel-shade` ring, `ink` ring).
- **Search field** (proposed, see Data): a sunk `well` with a 2 px `lip` edge, radius 8, 48 tall: the `search` glyph (24), the placeholder "Search" in `text-muted`, or the query in `text` 700 with a 2 px caret; a 34 px clear key (`cross` glyph) at the right while there is a query.
- **Body** (424 tall, 12 px under the filters): `ItemListBox` 604 wide (948 - 24 - 320) and a 320 px right column, 24 apart.
- **ItemListBox**: a ScrollBox in a `well` with a 2 px `lip`, radius 10, 8 px padding (a focused row's ring reaches 6 px, 7 in High Contrast, and is never cut). Rows are 56 tall, 6 apart; six fit before the list scrolls. A row holds, left to right:
  - the rarity badge in its `ink` socket (a 36 px cell);
  - the name TextBlock (fill, `ui-title`, in the rarity colour; it ellipsizes);
  - the trailing slot: ItemCard's **Equipped pill** (core chip: `plate`, 2 px `rim` edge, `ink` outline, the `pin` glyph and "Equipped") on an equipped row, or the **Identify** row action on an unidentified row.
- **Identify**: Button's row action (`.tt-btn.tt-btn--sm`, 48 tall): `plate-raised` face, 2 px `lip` edge, 4 px `ink` print, the `mage-tower` glyph (20) leading "Identify" (Next 700, 22 px).
- **Right column** (14 px gap):
  - ItemCard, compact (320 wide, no action footer, dot deltas), bound to the selected row. For an unidentified item it shows the "?" stamp, the sealed "Properties ???" strip and the ShortDescription, and drops the sealed note: the row's Identify carries the action.
  - The ledger panel (`plate-raised`, `lip` edge, radius 10, 118 tall with two rows): `LedgerHeader` "Stored Resources" in `ui-title` with the `material` glyph; one row per resource: its glyph (`ore`, `stone`), the name (`ui-label`), a 3 px dotted `lip` leader, the count (`num-s`, right-aligned in a 40 px box).
- **Empty states** (D8): core `.tt-empty`, one look: a plain `enamel` tag, `ink` text, no eyelet, no dot, centred in its box (the list well, or under the ledger header).

## States
- **Filter tag**: Idle; Hover (mouse: lifts 2 px, `rim` edge, `text` label); Press (sinks 2 px, `plate` face, a 3 px `rim-shade` band under the top edge; the `lip` edge stays whole, so High Contrast keeps the outline, D22); Focus (core `.tt-focus--fill-gap`: an `ink` ring in the gap and the `focus` ring outside it); Selected; Selected + focused. Exactly one tag is selected; the default is All.
- **Search field**: Idle (the placeholder); Focused (core fill-gap ring, `rim` edge, the caret); With a query (the list narrows as the player types; the clear key shows). No matches: the list shows "No items match this filter.".
- **Row**: Idle (`plate-raised`, transparent edge); Hover (`rim` edge); Selected (the cream fill, D6: the row that drives the card; the name turns `ink`, the badge keeps its rarity colour inside the socket); Selected + focused (the fill-gap ring on top). Focus and selection move together on keyboard and gamepad; a mouse hover does not change the selection.
- **Identify**: Button's states (idle, hover, pressed, focused, disabled). On screen: the amulet row is selected and its Identify has focus, so the hint bar reads "[Esc] Back | [I] Close | [Enter] Identify".
- **List**: populated; "Inventory is empty." (the inventory and the ledger are both empty); "No items match this filter." (the filter or the search hides every item); unbound "--" stays the C++ default and is never designed for.
- **Ledger**: populated, "No resources stored." (the header stays), or unbound "--".
- **Overflow**: the ScrollBox scrolls; a long name ellipsizes inside its row and never pushes the trailing slot out.

## Data and implementation
- **Widget**: `UMordecaiInventoryWidget`, pause tab id `inventory`, hosted in the shell's `TabContentBorder`.
  - Keep `FilterBar`, `ItemListBox`, `ListPlaceholder`, `LedgerHeader`, `LedgerBox` and `LedgerPlaceholder`. The bridge looks up `ItemListBox`, `LedgerBox`, `ListPlaceholder` and `LedgerPlaceholder` by name.
  - CommonUI pools the pause menu: rebuild the rows in `NativeOnActivated`, as the shell already does.
- **FilterBar retype**: `FilterBar` is declared `TObjectPtr<UHorizontalBox>` (`UI/MordecaiInventoryWidget.h`). Retype it to `UPanelWidget*` and build a `UWrapBox` (inner slot padding 8 / 8) in the C++ fallback; otherwise a designer WBP with a WrapBox named FilterBar will not bind. Today's HBox is wider than the content area and draws over "Stored Resources" (issue #37). See Building it in Unreal, Panel types.
  - It holds the eight `UMordecaiTabButton`s, ids `all`, `weapons`, `armor`, `trinkets`, `consumables`, `materials`, `quest`, `magical`. Each label stays the first TextBlock child, so `click_button("Weapons")` keeps working; the glyph is a UImage.
  - The filter mapping is unchanged: Materials = Material + TownResource; Magical = MagicalItem + UpgradeKey; Quest = QuestItem.
- **Search** (agent_rules_v2 section 2 requires "organization, filtering, and search"; nothing exists in C++). TODO(DECISION):
  - Proposed BindWidgetOptional `ItemSearchBox` (a `UEditableTextBox`), the last child of the FilterBar WrapBox with `bFillEmptySpace`, so it fills the rest of its row.
  - Proposed behaviour: a case-insensitive substring match on the displayed name, applied inside the selected filter, as the player types; clearing restores the list. The no-match state reuses "No items match this filter." until a search string is decided.
  - Open: how the keyboard reaches it (a click today; Menu input mode blocks unbound keys, so a focus key needs a `UI.Action.*` tag), and gamepad entry (the platform virtual keyboard).
- **Row text**
  - Keep ONE name TextBlock with the composed shipping string: the "? " prefix when unidentified and "  x%d" when stackable with quantity > 1 ("Health Potion  x2", "? Unidentified Purple Amulet").
  - The equipped affix "  [equipped]" becomes the Equipped pill (a `pin` UImage and an "Equipped" TextBlock in `EquippedPill`, proposed). Drop the affix from the composed string in the same change that adds the pill; no playtest reads it today (us079's Rusty Sword is never equipped: there is no equip action, N22).
  - Colour the name with the rarity token (`rarity-common` ... `rarity-gold`, all at least 4.7:1 on `plate-raised`); on the selected row it turns `ink`. The badge (in an `ink` socket) and the pill are images beside it; they add signals and never replace text.
- **Focus**
  - Make each row focusable (a `UCommonButtonBase`, or a UButton wrapping the HBox) so the card follows the selection.
  - Identify stays a separate UButton whose first TextBlock reads "Identify" (us079 clicks it by label). Right moves from a row to its Identify; Enter / A presses it. The focused Identify registers its action with `bDisplayInActionBar` so the bound action bar appends "Enter Identify" / "A Identify" after Back and Close, and drops it when focus leaves (PauseShell, Contextual hints).
  - TODO(DECISION): whether Enter / A on an unidentified row identifies directly.
  - Identify works anywhere in the field today (US-079); the Mage Tower is the town version of the same action. TODO(DECISION) (agent_rules Open Item 4): whether field Identify stays once the Mage Tower ships.
- **Right-column card**: ItemCard (compact), bound to the selected row's model. `Description`, `Type`, `Rarity` and the identification state are already cached in the row model but not drawn today. An unidentified item shows its ShortDescription ("An amulet of unknown power."). A weapon shows dot deltas against the equipped main hand.
- **Ledger rows**: today one TextBlock, "{0}  x{1}" ("Iron Ore  x1"), sorted alphabetically. The leader layout needs two TextBlocks (`LedgerNameText`, `LedgerCountText`); split them only in the same change that updates the playtest reading the ledger text, the same rule as device glyphs. Until then keep the single TextBlock and drop the leader.
- **Sort order** (`CompareSortPriority`): SortPriority, highest first: Critical (Upgrade Keys), Signature (Magical Items), High (Resources, ledger only), Normal, Low. Ties by type rank: Weapon, Armor, Trinket, MagicalItem, UpgradeKey, QuestItem, Consumable, Material/TownResource, CurrencyProxy; then ItemId. TODO(DECISION): agent_rules asks for "quest items, upgrade keys, magical items, then common gear", but the type tie-break puts gear before quest items.
- **Sizes at 1920 x 1080**: content box 948 x 540 at (486, 298); FilterBar 948 x 104 (tags 48); body 424; list 604 x 424 at (486, 414); right column 320 at (1114, 414): card 284 tall, ledger 118; rows 56; Identify 48 tall (the 48 px mouse target); search field 471 x 48.
- **Tokens**: colour `plate`, `plate-raised`, `well`, `lip`, `rim`, `rim-shade`, `enamel`, `enamel-shade`, `ink`, `ink-muted`, `text`, `text-muted`, `rarity-*`, `focus`; type `ui-title` (names, ledger header), `ui-label` (filters, ledger rows, search), `num-s` (counts); radius `radius-key`, `radius-tag`.
- **Motion**: TabSwitch when the tab opens. Identify plays StampReveal on the row's badge, scaled to the 56 px row; the row TextBlock is set once, to the new name ("Mysterious Amulet"), and Identify leaves the row (see MageTower).
- **Example data in the preview**: Emberfang, the equipped Rusty Sword and the ledger counts (Iron Ore x4) are examples; Upgrade Key, the Mysterious Amulet, Rusty Sword, Health Potion, Iron Ore and Town Stone are the shipping test items.

## Copy
- **Shipping, exact**:
  - Tab and filters: "Inventory", "All", "Weapons", "Armor", "Trinkets", "Consumables", "Materials", "Quest", "Magical".
  - Buttons and headers: "Identify", "Stored Resources".
  - Empty states: "Inventory is empty.", "No items match this filter.", "No resources stored."
  - Ledger row: "Iron Ore  x1" (the preview shows "x4" after more pickups; same format).
  - Row parts: the prefix "? ", the suffix "  x2", names "Unidentified Purple Amulet" and "Unknown Item"; "  [equipped]" until the Equipped pill replaces it.
- **Proposed**: "Search" (placeholder), "Equipped" (the pill, ItemCard's).
- **Hint bar** (the shell's): keyboard "[Esc] Back | [I] Close", plus "[Enter] Identify" while an Identify has focus; gamepad "[B] Back | [LB] / [RB] Tabs", plus "[A] Identify". Keycaps lead their labels.
- No Tally remarks in this tab.

## Do / Don't
- Do draw only inside the shell's 948 x 540 content box; the nameplate, tabs, hint bar and spike are PauseShell's.
- Do keep the composed row string in one TextBlock; add the badge and the pill as images and their own TextBlocks.
- Do let the FilterBar wrap. Never shrink a label below 20 px to force one row.
- Do show Identify only on unidentified rows, and the Equipped pill only on equipped rows.
- Do keep rarity as colour plus badge strokes plus the colour word ("Unidentified Purple Amulet").
- Don't put rarity colours on the cream: a selected row's name turns `ink`; on the card the rarity rides the band.
- Don't add a second "?" to a row; the shipping prefix already says it.
- Don't draw bracket text ("[equipped]") once the pill ships, and never both.
- Don't add bag slots, weight or capacity readouts: carry is unlimited.
- Don't draw LB / RB on this screen for a keyboard player; keyboard tab cycling is TODO(DECISION) (proposal Q / E) and never drawn as bound.
