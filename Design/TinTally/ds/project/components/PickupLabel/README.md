# PickupLabel

A small enamel tag tied by a string to a pickup on the ground, showing its rarity and its name. It replaces the faint white TextRender name label: show it over every pickup in view, show the interact keycap on the one the interact key will take, and shrink it to a rarity badge at distance.

## Anatomy

| Part | Spec |
|---|---|
| Tag | `enamel`, 36 high, radius 4 (left) / 10 (right, `radius-tag`), 3 px `ink` outline + 4 px print shadow. Right padding 12, 8 px gaps. |
| Rarity band | The tag's left cap, 22 wide: the rarity colour (`rarity-common` ... `rarity-gold`) with a 2 px `ink` seam on its right. It holds the system grommet (the same eyelet as every tag): an 8 px `well` hole, a 2 px `enamel-shade` ring and a 2 px `ink` ring, 16 px overall (a 6 px hole in the far form). |
| String | Core's light cord (`.tt-string--light`: a 4 px `rim` core with 1 px `ink` sides), from the eyelet down past the tag to the top of the item. Length 30 px (unfocused), 40 (focused), 18 (far). The cord holds on night and mire, where a plain 2 px ink string is 1.2:1 and 2.5:1. It is part of the label, not of the tag, so focus never thickens it. The tag is tied at its eyelet, so it hangs to the right of its item. |
| Rarity socket | The core `.tt-socket` (`ink`, radius 6) holding the rarity tally badge, 18 px: Common an empty ring, Green to Red 1-4 strokes, Gold the full bundle. |
| Unidentified mark | The `unknown` glyph, 20 px, `ink`, before the name of an unidentified item. |
| Name | `ui-label` (20/26, 600), `ink`. The identification-aware display name. |
| Focused | 44 high, band 24. The interact keycap (`.tt-key--sm`, 32 px, 20 px letter: square "F" on keyboard, round "X" on gamepad; the card draws keyboard and mouse, one device per screen) after the band. Core focus, default variant (`.is-focused`): the 3 px `focus` ring right outside the tag's own ink outline and core's 2 px `ink` keyline outside the ring (ink | focus | ink), so it reads on blockout and snow as well as on night. No local ring: the tag keeps its own outline and print. |
| Far | Band (18) + socket (16 px badge) only, 30 high. No name, no keycap. |

Rarity is told three ways: the band colour, the stroke count in the socket, and, for unidentified items, the colour word in the name ("Unidentified Purple Amulet").

## States

| State | When | What shows |
|---|---|---|
| Unfocused | The pickup is in view and within 1400 cm | Band, socket, name. |
| Focused | It is the focused pickup (nearest in the 120 cm trigger, re-evaluated every 0.15 s) | Taller tag, keycap, focus ring, longer string. Only one label is focused at a time, and it matches the InteractPrompt. |
| Unidentified, partial info | `ShowPartialInfoBeforeIdentify` on | `unknown` glyph + "Unidentified <Rarity> <Subtype or type>". |
| Unidentified, no partial info | Partial info off | `unknown` glyph + "Unknown Item". Rarity still shows: the rarity tag stays visible. |
| Far | Beyond 1400 cm from the player (example value, the same as EnemyTag; TODO(DECISION)) | Band + socket only. |
| Picked up | The actor is destroyed | The label goes with it. No exit animation. |

Motion: a focus change is LabelLift (the brand book's Motion table): the tag lifts as the string lengthens and the keycap fades in; losing focus reverses it. Labels never bob or spin. Reduce Motion: LabelLift's form, no lift; a fade swaps the focused look.

## Data and implementation

- **Actor:** `AMordecaiItemPickup`. Today `NameLabel` is a `UTextRenderComponent`:
  - Z +70, WorldSize 22, `FColor::White`, rendered through the lit text material, so it is faint on light floors.
  - It is billboarded each frame by `FaceNameLabelTo` / `ComputeNameLabelRotation`.
  - Text comes from `RefreshNameLabel()` -> `GetItemDisplayName()`, which is identification-aware.
- **Replacement:** a `UWidgetComponent` "NameTag" in Screen space with `UMordecaiPickupLabelWidget` (new). It is unlit and constant pixel size, with no billboard math.
  - Pivot: the eyelet, at x = 11 / width and y = 1.0 plus the string. Simplest: the string is part of the widget, and the widget's pivot is the string's bottom end at (11 px, bottom).
  - Place the component at the item's top (about Z +30 for the 0.35-scale test shapes), not +70.
  - BindWidgetOptional: `RarityBand` (Image), `RaritySocket` (Image), `UnknownMark` (Image), `NameText` (UTextBlock), `InteractKey` (keycap widget), `FocusRing` (Border).
- **Playtest dependency (must land together):** the agent bridge reports `actors[].text_labels` only from `UTextRenderComponent`s (`MordecaiAgentBridgeState.cpp`). us079 asserts that each pickup has a visible label whose text is its display name.
  - Replacing `NameLabel` therefore needs the bridge to report the NameTag's `NameText`, and us079 updated in the same change.
  - Until then, keep `NameLabel` and draw the tag alongside it, or leave the TextRender as is.
- **Focus:** `UMordecaiPickupInteractionComponent` already tracks the focused pickup (`GetFocusedPickup()`) and broadcasts `OnFocusedPickupChanged`, the same event the prompt uses.
  - Bind to it on the local player.
  - Set the focused state on the new pickup's label and clear it on the previous one. A new `AMordecaiItemPickup::SetLabelFocused(bool)` that forwards to the widget is enough.
  - The keycap comes from CommonInput's current input type: KBM "F", gamepad "X" (Interact = Gamepad Face Button Left).
- **Rarity:** `Mordecai.Item.Rarity.*` from the instance (still visible when unidentified). Colours are the `rarity-*` tokens: author sRGB, convert with `FLinearColor::FromSRGBColor`.
- **Far:** distance to the local pawn, checked every 0.25 s (like EnemyTag). TODO(DECISION) on the threshold.
- **Crowding:** labels never overlap their own item. When two tags overlap on screen, the farther one drops to the far form; the focused one always stays full.
- **Sizes:** tag 36 (focused 44, far 30); name 15 pt (20 px); keycap 32 px; socket badge 18 px (far 16).
- **Brushes (03's manifest):** the tag stack (RoundedBox, the grommet in the band), the light-cord strings, and 03's one focus ring brush (`FocusRing`, ink 2 | focus 3, drawn the outline width outside the tag). Many labels on screen may use 03's SDF rounded-rect material instead of the Overlay stack.
- **Tokens:** `enamel`, `enamel-shade`, `ink`, `well`, `rim`, `focus`, `rarity-common`, `rarity-green`, `rarity-blue`, `rarity-purple`, `rarity-red`, `rarity-gold`, `ui-label`, `radius-tag`, `stroke-focus`.
- **Glyphs:** `unknown`.

## Copy

- The name is exactly `GetItemDisplayName()`: "Health Potion", "Iron Ore", "Town Stone", "Rusty Sword", "Upgrade Key", "Emberfang" (item schema example), "Unidentified Purple Amulet", "Unknown Item".
- The keycap shows the bound key only ("F", "X"). No verb on the world label; the verb lives on the InteractPrompt.
- No Tally remarks.

## Do / Don't

- Do keep text `ink` on `enamel`. Rarity colour goes in the band and the socket, never in the name text.
- Do tie the tag at its eyelet so tags consistently hang right of their items.
- Do keep the focused label in step with the InteractPrompt; they always name the same pickup.
- Don't show a label for an auto-stored item after pickup; it is gone with the actor.
- Don't tint the whole tag by rarity; enamel stays enamel on every floor.
- Don't let the TextRender and the tag both show in a shipped build once the bridge change lands.
