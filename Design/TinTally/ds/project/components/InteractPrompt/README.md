# InteractPrompt

The bottom-centre interact unit: an enamel tag with the interact keycap, the action and its target, and a rarity or context socket, docked to the always-visible Inventory hint pill on its right. Show the tag whenever something is focused for Interact; the hint pill stays on screen for the whole of play.

## Anatomy

| Part | Spec |
|---|---|
| Prompt box | Anchor bottom-centre (0.5, 1), offset (0, -36), alignment (0.5, 1), 540 x 60: x 690-1230, y 984-1044 at 1920 x 1080 (the HUD map box). The tag hugs its content and is **right-aligned** in the box, so its right end always meets the hint pill across the same 12 px gap. The pair reads as one unit at any name length, and a retarget only moves the tag's free (eyelet) end. |
| Tag | Core `.tt-tag` at 60 high: `enamel`, radius 4 / 10, 3 px `ink` outline + print shadow, the system grommet eyelet at x 16. Left padding 40, right 14, 12 px gaps, max width 540. |
| Keycap | `.tt-key`, 40 px, 22 px letter: square "F" on keyboard and mouse, round "X" on gamepad (Interact = `Gamepad_FaceButton_Left`). |
| PromptText | `ui-body` size (22/30) at weight 700, `ink`, one line, ellipsis past 540. Measured: the longest current string, "Pick up Unidentified Purple Amulet", is 366 px, so the tag is 516 px (x 714-1230); "Pick up Iron Ore" is 170 px, a 318 px tag (x 912-1230). |
| Socket | Core `.tt-socket` (`ink`) at the right end. For items: the rarity badge, 18 px. For anything else: a context glyph, 22 px, in `enamel`. |
| Inventory hint | A plate pill, 40 high: `plate`, 2 px `rim` / `rim-shade`, 3 px `ink` outline + print, radius pill. Holds the small keycap "I" (32 px, 20 px letter) + "Inventory" in `ui-label`, `text-muted`. Fixed at x 1242-1393 (151 wide; 139 in the Today form), y 994-1034: 12 px right of the prompt box, centred on its height. One of the five always-visible HUD pieces (with the gauge plate, spell slots, rail head and loadout plate): it never moves and never hides during play, with or without a prompt. |
| Unit | At most x 690-1393, y 984-1044. It clears the loadout plate (x 1548+), the quick items (y 892-948) and the callout and combo lanes (centre +128 / +184), and keeps the 36 px bottom margin of the map. |

## States

| State | When | What shows |
|---|---|---|
| No target | Nothing focused | The tag is Collapsed; the hint pill stays. |
| Pick up | A pickup is focused | Keycap + "Pick up {item}" + rarity socket. |
| Unidentified | The focused item is unidentified | "Pick up Unidentified Purple Amulet" (partial info) or "Pick up Unknown Item" (no partial info). The rarity socket still shows. |
| Gamepad | The last input was a gamepad (CommonInput) | Round "X" keycap on the tag. The pill's gamepad form is open (see Keycap device). The card draws every crop in keyboard and mouse (one device per screen) and describes this state in its spec band. |
| Pressed | Interact pressed | The keycap sinks (the KeyPress motion, `is-pressed`); then the item leaves and the tag collapses or retargets. |
| Targeting (proposed) | A ground-target reticle is up (WorldMarkers; ground targeting does not exist in code yet) | The tag collapses and the prompt box holds the TargetingHint: the core hint bar (`.tt-hintbar`) "Cast \| Cancel", keycap first, right-aligned against the pill. Keyboard and mouse: the drawn left and right mouse caps; gamepad: A and B (attack_taxonomy_v1 §6.2). The pill stays. |
| Talk / Activate / Climb / Open | Other interactables (examples; see below) | Same tag, the verb and target, a context glyph in the socket. |
| Today | Until the playtest strings change (see below) | No keycaps: the tag holds the shipped text "[F] Pick up {item}" and the pill holds "[I] Inventory". Same positions. |

Motion: PromptSlide (the brand book's Motion table). The tag slides out from the pill and fades in, and collapses back toward the pill the way a ticket retracts into its slot. A retarget swaps the text in place: the right end stays and the tag's width eases to the new length. Reduce Motion: the system's 150 ms fades.

Pacing: prompts are not tickets. They show at once, also during the combat hold, because the player is acting.

## Data and implementation

- **Widget:** `UMordecaiPickupPromptWidget`, slot `PickupPrompt` in `UMordecaiCombatHUDWidget`.
  - Keep the BindWidgetOptional names `PromptText` and `InventoryHintText`. The bridge looks both up by name.
  - Add optional `PromptTag` (Border with the tag brush), `InteractKey` (keycap widget), `PromptSocket` (Image), `HintPill` (Border) and `HintKey`.
- **Slot:** today BottomCenter (0, -30), 420 x 56, a VerticalBox with the hint under the prompt. New layout:
  - The slot is 720 x 60 at anchor (0.5, 1), alignment (0.5, 1), offset (90, -36), so x 690-1410. The 540 x 60 prompt box inside it is the HUD map box at (0, -36).
  - Inside, a CanvasPanel: the prompt box at (0, 0), 540 x 60, holding `PromptTag` with HAlign Right and VAlign Center; the hint pill `HintPill` at (552, 10), auto size.
  - Do not centre the tag in the box: a short name would leave over 100 px of floor between the tag and the pill ("Pick up Iron Ore": 123 px), and they would read as two unrelated pieces.
- **Text:** F_Tally_UI Bold 16.5 pt (22 px), `ink`, `OverflowPolicy` Ellipsis. The name is identification-aware (`GetItemDisplayName()` via `OnFocusedPickupChanged`).
- **Playtest strings (must change together):**
  - us079 compares `PROMPT_PREFIX = "[F] Pick up "` and `HUD_HINT = "[I] Inventory"` against PromptText and InventoryHintText.
  - Drawing the keycaps means PromptText becomes "Pick up {item}" and InventoryHintText becomes "Inventory". Land that with the us079 constants and the device-glyph work, in one change.
  - Until then, ship the "Today" form (the last crop in the preview): the tag holds "[F] Pick up {item}", and the pill holds "[I] Inventory" with no keycap. The restyle changes no string.
- **Keycap device:** CommonInput's current input type picks F or X. The design log defers device-aware glyphs ("prompts show keyboard keys for the slice").
  - TODO(DECISION): on gamepad, Inventory has no direct button (I opens the pause menu on Inventory; Start opens it on the last tab). Either hide the hint on gamepad (an exception to always-visible) or show a "Start" pill keycap with "Menu". Until it is decided, nothing gamepad-specific is drawn for the pill: the card never draws an unbound key as bound.
- **TargetingHint (proposed):** an optional `TargetingHint` (the hint bar: an HBox of two keycap + label pairs and a divider) in the prompt box, Collapsed unless the local player is in ground-targeting mode. While it shows, `PromptTag` is Collapsed and `PromptText` keeps its last string. Labels "Cast" and "Cancel" are proposed copy; the reticle itself is WorldMarkers'.
- **Socket:** items use `Mordecai.Item.Rarity.*` with the core rarity badge.
  - Other interactions use a glyph: `alchemist` (and the other town buildings) for Talk, `gate` for Activate, `arrow-up` for Climb (until a climb glyph exists), `chest` for Open.
- **Other verbs:** only "Pick up" exists in code. `control_bindings_v1.1` lists "Talk, loot, activate, climb prompts" for Interact. TODO(DECISION):
  - the interactable interface;
  - a format string per verb (`FText::Format("{Verb} {Target}")`, localisable);
  - the targets. "Talk to Alchemist", "Activate Shortcut Gate", "Climb Ledge" and "Open Chest" are examples.
- **Layer:** `PickupPrompt` is a child of the HUD root (`UMordecaiCombatHUDWidget`, reparented to `ULyraActivatableWidget` and pushed once on `UI.Layer.Game`), never a push of its own. Today the HUD is added with `AddToViewport`, so "[I] Inventory" draws over the pause menu and the death screen. With the HUD root on the Game layer, the pause menu's veil (`UI.Layer.Menu`) covers the pill, and the death screen, the only other push on the Game stack, hides it while the player is dead; it returns with the HUD.
- **Brushes (03's manifest):** the tag stack for `PromptTag` (grommet at x 16), the pill stack for `HintPill`, Keycap's RoundedBox keycaps. No textures.
- **Tokens:** `enamel`, `ink`, `plate`, `rim`, `rim-shade`, `text-muted`, `ui-body`, `ui-label`, `radius-tag`, `radius-pill`, and the `rarity-*` badge colours.
- **Glyphs:** `alchemist`, `gate`, `arrow-up`, `chest`.

## Copy

- "Pick up {item}" with the keycap, or the shipped "[F] Pick up {item}" until the playtest change. `{item}` is the display name exactly: "Health Potion", "Iron Ore", "Town Stone", "Rusty Sword", "Upgrade Key", "Unidentified Purple Amulet", "Unknown Item".
- "Inventory" with the I keycap, or the shipped "[I] Inventory".
- Other verbs (examples): "Talk to Alchemist", "Activate Shortcut Gate", "Climb Ledge", "Open Chest". Sentence case, verb first, no trailing punctuation.
- Targeting (proposed): "Cast", "Cancel", each led by its keycap.
- No Tally remarks on prompts.

## Do / Don't

- Do keep the prompt and the focused PickupLabel naming the same pickup.
- Do keep the hint pill fixed in place and always visible during play; only the tag appears and disappears.
- Do right-align the tag against the pill, so the gap between them is always 12 px.
- Do put rarity in the socket and keep the text `ink`.
- Don't print the key twice (a keycap and "[F]" in the text).
- Don't show more than one prompt; the focused target wins.
- Don't let the tag grow past 540 or wrap to two lines.
