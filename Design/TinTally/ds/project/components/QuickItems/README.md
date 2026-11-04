# QuickItems

Four consumable slots (Item A-D) and the Throw slot, in one row above the loadout plate: item glyph, keycap and count. Show a slot only when something is assigned to it; collapse the whole row when all five are empty.

## Anatomy

- **Row** anchor bottom-right (1,1), offset (−32, −132), alignment (1,1): 336 x 56, x 1552-1888, y 892-948, right-aligned with the loadout plate (340 x 84 at y 964-1048), 16 px above it.
  - Order, left to right: Throw, a 20 px gap, then A B C D 12 px apart (56 + 20 + 4 x 56 + 3 x 12 = 336).
  - **Footprint to reserve on the HUD map:** x 1552-1888, y 868-948. The row is Throw + 4 x 56 (336 wide), not "4 x 56", and the keycaps perch 24 px above it, to y 868. The count pills hang into the 16 px gap above the loadout plate, to y 955 with their outline. Nothing else may sit in that box.
- **Item slot.** 56 x 56 `.tt-plate` (radius 12, 2 px `rim`, 3 px `ink` outline, print shadow; no rivets: HUD plate). Three parts are stacked so none covers another (y measured from the slot's top edge):
  - **Keycap** `.tt-key--sm` (32 px) perched on the top-left corner: y −24 to 8, x −4 to 28. It rises 24 px into the reserved space above the row and stays 5 px clear of the left neighbour's outline.
  - **Glyph** 28 px, centred across, y 8 to 36, in `text`. The Health Potion glyph is `vital`.
  - **Count** `CountText` in a 26 px `plate` pill (2 px `lip`, 3 px `ink` outline), "x3" in `num-s`, centred, y 34 to 60. It hangs 4 px below the slot, so its outline ends 7 px down and stays 6 px clear of the loadout plate's outline. "x10" (52 px) still fits inside the slot's width.
- **Throw slot.** The same 56 px plate cut round, so it never reads as Item A. Its keycap is centred on top instead of on the corner.
- **Keycaps:**
  - Keyboard: square keys "2", "3", "4" and "G". Item A shows the dashed unbound cap (below).
  - Gamepad: Keycap's 32 px D-pad cross (`.tt-key--sm.tt-key--dpad` + `--dpad-down` / `-left` / `-right`): 10 px `enamel` arms in a 2 px `ink` edge, a 6 x 8 `ink` insert in the slot's own arm. Item C and Throw show the dashed unbound cap.
  - **Unbound cap** (Keycap's `.tt-key--none` on a `plate` face, dashed `lip`, no letter): the slot's spec key is taken by another action in the game today, so the HUD never shows two caps with the same key. Dashed means unavailable (D24), which is exactly this.
  - Spell slots hang their keycap from the bottom-left; here the loadout plate is too close below, so the keycap sits on top.

## States

| State | Look |
|---|---|
| Stocked | glyph, keycap, count ("x3") |
| Full stack | "x10" is the widest count (Health Potion stacks to 10); the pill grows from its centre and stays inside the slot |
| Used | keycap sinks (Motion: KeyPress), the slot dips 2 px for 60 ms, the count steps down (x3 to x2) and its lip flashes `rim` once. The outline stays whole in High Contrast; only the print collapses |
| No key yet | the dashed unbound cap; the item still shows its glyph and count (`TODO(DECISION)` US-097) |
| Empty | nothing assigned, or the stack ran out: the slot is Hidden (not Collapsed), so A-D never slide sideways |
| All empty | the row is Collapsed |

**Reduce Motion:** no dip; the count changes with a 150 ms fade.

## Data and implementation

- **Nothing exists in the game yet.** `IA_Mordecai_ItemA` to `ItemD` and `IA_Mordecai_Throw` exist but are not mapped, and no widget exists. These are all `TODO(DECISION)`:
  - how items get assigned to slots (quickslot assignment is not drawn);
  - the potion field cap and use cooldown (Open Item 5: the Alchemist sets how many potions the player can field; "x3" vs "3 / 5");
  - what happens when a stack runs out (hide, or show "x0");
  - which throwable is selected.
- **Proposed widget:** `UMordecaiQuickItemsWidget`, placed as `QuickItems` in the root canvas of `UMordecaiCombatHUDWidget`: anchor (1,1), offset (−32, −132), alignment (1,1), size 336 x 56, `bAutoSize` false. It is a child of the HUD root (D20: the root is pushed once on `UI.Layer.Game`), never pushed on its own. Leave the 24 px above it free for the keycaps.
  - Children: `QuickItemA` to `QuickItemD` and `ThrowSlot`, each a slot widget (an Overlay on the plate stack from the asset manifest; the Throw slot on the pill stack at full radius) with `ItemIcon` (UImage, 28 px, the item's `Icon`), `CountText` (UTextBlock "x%d", Mono, fixed 3-cell box in a pill UBorder) and `SlotKey` (keycap, or the unbound cap).
  - The class name starts with "Mordecai", so the bridge reads `CountText`.
  - Count = the stack quantity from the inventory for the assigned `ItemId`.
- **Keys** (control_bindings_v1.1) and what the HUD draws today:

  | Slot | Keyboard (spec) | Gamepad (spec) | Drawn |
  |---|---|---|---|
  | Item A | 1 | D-pad Down | keyboard: unbound cap (1 is Spell 4, Restoration); gamepad: D-pad Down |
  | Item B | 2 | D-pad Left | as the spec |
  | Item C | 3 | D-pad Up | keyboard: 3; gamepad: unbound cap (D-pad Up is Spell 4) |
  | Item D | 4 | D-pad Right | as the spec |
  | Throw | G (tap) | Y (tap) | keyboard: G; gamepad: unbound cap (Y tap is next weapon, AC-078.6; the spec also holds Y for Weapon Set Swap) |

- **`TODO(DECISION)` key conflicts** (US-097): key 1 and D-pad Up cast Spell 4 (MordecaiHeroComponent), and Y tap cycles weapons. When US-097 frees a key, the slot swaps the unbound cap for the real one; never draw a key the game already uses for something else.
- **Items in the preview:**
  - Health Potion is a real item (Consumable, Common, stack 10), glyph `potion`.
  - Antidote, Bandage and Throwing Knife are **example** items, named after the cleanse items in the status spec and the spec's "knife/bomb/vial". Their glyphs: Antidote `tonic`, Bandage `bandage`, Throwing Knife the `throwable` weapon-type glyph. Real items take their glyph from their item data (`Icon`).
- **Tokens:** `plate`, `rim`, `ink`, `lip`, `text`, `vital`, `enamel` (keycaps).

## Copy

- Counts: "x3", "x10". Use a lowercase x, the same as the ledger's "Iron Ore  x1".
- Keycaps: "2" "3" "4" "G" (and "1" once it is freed); gamepad D-pad crosses; the unbound cap has no text (accessible name "Unbound").
- Item names appear in tooltips and the inventory, not on the slot.
- No Tally remarks.

## Do / Don't

- Do keep each slot in its place when its neighbour is empty.
- Do keep the count in its pill and inside the slot's width.
- Do reserve x 1552-1888, y 868-948 for the row and its keycaps.
- Don't let the keycap or the count cover the glyph.
- Don't put text straight on the world or on the slot glyph.
- Don't draw a key that another action owns (two "1" caps on one HUD): use the unbound cap until US-097 decides.
- Don't show the row at all when nothing is assigned.
