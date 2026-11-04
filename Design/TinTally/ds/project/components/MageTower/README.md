# MageTower

The Mage Tower's Identify service screen: the sealed (unidentified) tags on the left, the selected tag hung on its string in an appraisal tray, and a counter with the quote and the Identify keycap. Use it when the player uses the Mage Tower in town; it is its own screen on `UI.Layer.Menu` and the first instance of the ServiceScreen pattern (the Alchemist's Restock is the second).

## Anatomy (1920x1080)
The screen is drawn for keyboard and mouse (D5); the gamepad version is in the spec band.
- **World and frame**
  - Behind the `veil`: the paused village as the toy diorama (D12) and the real HUD on `UI.Layer.Game` (D10: GaugePlate, SpellSlots, the rail head, the "[I] Inventory" pill, QuickItems, LoadoutPlate, copied from HUDLayout), dimmed, never blurred. Proposal: the screen pauses the game the way the pause menu does (`SetGamePaused`); TODO(DECISION) for multiplayer, like AC-069.13.
  - **Nameplate**: PauseShell's service-screen nameplate (`.tt-pause__title--screen`): the enamel sign on two 52 px light strings (core's light cord: 4 px `rim` with 1 px `ink` sides) into two grommets, the sign at y 52, the `mage-tower` glyph (32) and "Mage Tower · Identify" in `tally-heading`, `ink`. No local overrides.
  - **Hint bar**: core `.tt-hintbar` (D25) placed by PauseShell's `.tt-pause__footer`, centred, with `--tt-pause-footer-top: 966px` (under this screen's taller shell): "[Esc] Back | [Enter] Identify", keycaps first.
- **Shell**: one menu plate (`plate`, `rim` / `rim-shade`, `ink` outline, menu print) at (96, 140), 1728 x 796, padding 24. Two columns: 560 | fill, 24 apart. No rivets: rivets belong to Tally and front-end plates only (D13).
- **Left panel** (`plate-raised`, `lip`, radius 10):
  - Header "Unidentified items" in `ui-title` with the `seal` glyph and a count pill (`num-s`).
  - Then the sealed tags, 68 tall, 14 apart, 12 px narrower than the panel. Each is a core `.tt-tag` (enamel, ink outline, grommet eyelet) with the rarity badge in an ink socket, the name (`ui-title`, ink), the type line (`ui-label`, `ink-muted`) and the "?" stamp at the right.
  - The selected tag slides 12 px right and carries focus (the core ring and keyline outside its ink outline); its ring clears the panel's lip.
  - Enamel tickets on a rod select by sliding, not by fill: this is the brand book's one exception to D6's cream Selected look.
- **Tray** (`well`, `lip`, radius 12):
  - The one rail family (core `.tt-railbar`, D23): a 440 px teal bar with an eyelet in each end cap, 36 px from the top.
  - The core light string (`.tt-string--light`) from the rail into the card's ear.
  - The selected item's ItemCard hangs from a reinforced ear (58 x 36 `enamel`) through a grommet (10 px `well` hole, `enamel-shade` ring, `ink` ring), scaled 1.4 with a render transform (538 px wide). Do not use bigger type styles for it.
- **Counter** under the tray (`plate-raised`, `lip`):
  - Quote rows "Cost ...... Free" and "Time ...... Instant" in `ui-label`, dotted `lip` leaders, the values in `text` 700.
  - One plain line in `ui-body`, `text-muted`.
  - The Identify keycap button: 300 x 72, `enamel`, 3 px `ink` border, 6 px `enamel-shade` lower lip, `ink` outline and print; the Enter keycap leads "Identify" (`ui-title`) (D4).

## States
- **Before**: a tag selected and hung in the tray (the screen above).
- **IdentifyButton**: idle; pressed (sinks 4 px, its lip collapses to 2 px, the Enter keycap presses: KeyPress); focused (mouse hover-focus or Tab: the core ring right outside its ink outline, then the keyline); disabled (`plate` face, dashed `lip`, the `lock` glyph, `text-disabled`). All four are in the spec band.
- **Reveal**: see Motion and the three storyboard frames in the spec band.
- **After**: the tray holds the identified card ("Mysterious Amulet") with an "Identified" footer (the `check` glyph). The tag leaves the list (4 to 3) and focus moves to the next tag.
- **Empty**: count 0, the one empty state (core `.tt-empty`, D8) centred in the panel, "Nothing to identify." (example copy), and Identify disabled.
- **Focus**: on the selected tag. Up / Down moves the selection; Enter (gamepad A) identifies.

## Motion: the reveal
Every timing comes from the brand book's Motion table; this screen only orders the rows. They run on an overlay layer above the card. The name TextBlock is set ONCE, to the final name, when `OnItemIdentified` fires; the typed-in look is a mask, never per-character SetText, because the bridge reads the TextBlock.
1. **StampReveal** (frame 1): a rarity stamp (a 60 px disc in the rarity colour, `ink` rings, the badge in `ink`) strikes the "?" stamp and shakes the card. The "?" becomes the struck rarity seal.
2. As the stamp settles, the sealed note and the "???" strip fade out, the body resizes to the identified content and the identified description fades in. The band does not change: the rarity was already known.
3. **TypeOn** (frame 2): a retainer mask slides off the new name, left to right, behind a 4 px wipe head in the rarity colour. The mask is `enamel` with the sealed strip's 14% `ink` hatch, so a paused frame reads "not yet revealed", never "clipped".
4. **TicketDrop** (frame 3 is the settled card): the tag swings once on its string and settles.

Reduce Motion: the brand book's 150 ms cross-fade from the sealed card to the identified card. Sound: one stamp strike. The storyboard labels and the hatch exist only in the spec band and in the overlay, never in the card's own widgets.

After the screen closes, the Tally prints an IDENTIFIED ticket on the rail, owned by TallyRail and written from its kinds table (D26): kicker "Identified", the name "Mysterious Amulet", the rarity in the footer. "It had a name all along." is the one identify remark; where it prints, if anywhere, is TODO(DECISION) (Open decisions). This screen shows no remark.

## Data and implementation
- **Widget**: new `UMordecaiMageTowerWidget` (`ULyraActivatableWidget`, InputConfig Menu, `bIsBackHandler`), pushed on `UI.Layer.Menu`. CommonUI pools it, so bind in `NativeOnActivated` and unbind in `NativeOnDeactivated`.
  - Enter and A are CommonUI accept on the focused tag; this screen needs no new menu key. (A new one would need a `UI.Action.*` tag, a `+InputActions=` entry and `RegisterUIActionBinding`; see Building it in Unreal.)
  - The hint bar is a `UCommonBoundActionBar` styled as `.tt-hintbar` (PauseShell's); its keycaps follow the last input device.
- **Data**
  - The player's inventory instances that are still unidentified.
  - Names come from `UMordecaiItemLibrary`: "Unidentified <Rarity> <Subtype or type string>", or "Unknown Item" when partial info is off.
  - Identify calls `UMordecaiIdentificationService::IdentifyInstance(Inventory, InstanceId, "MageTower")`, then waits for `OnItemIdentified`.
- **Named widgets** (proposed): `TitleText` (the nameplate), `UnidentifiedList`, `UnidentifiedCountText`, `AppraisalTray`, `TrayRail`, `TrayCard` (ItemCard), `CostText`, `TimeText`, `IdentifyButton` (its first TextBlock reads "Identify"), `EmptyText`. Use `Mordecai*` native classes so the bridge can list and click them.
- **Assets** (03, the asset manifest): the plate stack for the shell, panels and counter; the tag stack for the sealed tags and the card; the Strings image (the light cord); the rail is a RoundedBox bar with two end-cap images, as `.tt-railbar`.
- **Open items**: TODO(DECISION) (agent_rules Open Item 4, game_design_v2 §12, US-033):
  - the cost model: gold, resources or a service fee (today free);
  - the timing: instant, timed or a service queue (today instant);
  - batch identify (out of scope in US-033);
  - whether partial hints show;
  - whether the field Identify button in the Inventory tab stays once this screen ships.
  - Cost and Time show values from data; the counter keeps room for a third row (a batch total) without a relayout.
- **Data note for Jeff**: the test amulet's identified Description reads "A faintly humming amulet. The Mage Tower can identify it." After identification that sentence is out of date. The fix belongs in the item data, not the UI.
- **Sizes**: nameplate centred, sign top at y 52; shell 1728 x 796 at (96, 140); hint bar centred at y 966; left panel 560 wide; tag rows 68; tray 1092 x 624 with the rail 440 x 20 at its top centre; tray card 384 at 1.4x (538 wide); counter 104 tall; Identify 300 x 72.
- **Tokens**:
  - Colour: `veil`, `plate`, `plate-raised`, `well`, `lip`, `rim`, `rim-shade`, `enamel`, `enamel-shade`, `ink`, `ink-muted`, `text`, `text-muted`, `text-disabled`, `rarity-*`, `focus`.
  - Type: `tally-heading`, `ui-title`, `ui-label`, `ui-body`, `num-s`.

## Copy
- Nameplate and panels: "Mage Tower · Identify", "Unidentified items".
- Counter: "Cost", "Free", "Time", "Instant", "Identifying reveals its name and properties. Then it can be equipped."
- Buttons and states: "Identify", "Identified", "Nothing to identify." (example), "Back".
- Items: "Unidentified Purple Amulet" becomes "Mysterious Amulet" (shipping data). "Unidentified Blue Sword", "Unidentified Green Ring" and the red "Unknown Item" are examples.
- All but "Identify" and the item names are proposed copy. No Tally remark on this screen; the remark question lives with the rail ticket.

## Do / Don't
- Do frame the screen with PauseShell's service-screen hooks (`.tt-pause__title--screen`, `--tt-pause-footer-top`), never with local overrides of its classes.
- Do hang the tray card from the one rail (`.tt-railbar`), never a custom rod.
- Do lead every action with its keycap: "[Enter] Identify".
- Do set the name TextBlock once and animate a mask over it.
- Do keep the rarity word in every unidentified name; the badge and the band repeat it.
- Do disable Identify with the lock glyph and a dashed edge when nothing is selected, never by colour alone.
- Don't show a price, a timer or a batch button until the TODO(DECISION) above is settled.
- Don't type the name character by character with SetText.
- Don't put text on the tray's `well` without a plate or tag under it, or spec labels inside the screen (D7).
