# CombatCallout

The centre-lane combat feedback: one shipping callout in ink-outlined letters under the player's feet, with the "Hit N" combo plate below it. Use it only for the five defence outcomes and the light-attack combo window; nothing else may use the centre lane.

## Anatomy

| Part | Spec |
|---|---|
| Callout slot | 520 wide, anchor centre (0.5, 0.5), alignment (0.5, 1), bottom edge at +180. Normal size: 52 high, so it spans +128 to +180 (the HUD map's (0, +128) box). The player is the screen centre, so this sits just under their feet. The slot only ever grows upward (Callout size, High Contrast), so the combo slot below never moves. |
| Callout letters | `callout` (Rowdies 700, 40/44). Fill `good` for the four positive strings, `danger-text` for "Parry Failed!". 4 px `ink` outline (`.tt-outlined-4`) and a hard (0, 4) `ink` print shadow. Letters only: no plate, no icon, no ornament. |
| High Contrast plate | In the High Contrast theme the letters sit on a plate: `plate` fill, 2 px `rim`, `radius-plate`, 4 px `ink` outline, no print shadow. It is anchored to the slot's bottom with 8 px to spare, so its outline ends at +176, 4 px above the combo plate's outline (+180), at every callout size. |
| Combo slot | 220 x 36, anchor centre, offset (0, +184), alignment (0.5, 0): +184 to +220. |
| Combo plate | A 36 px tin plate (`plate`, `rim` / `rim-shade`, `radius-plate`, 3 px `ink` outline, print shadow), centred in the slot. It holds ComboText "Hit N" alone, in `num-m` (Mono 700, 24/28), `good`, in a fixed box 5 Mono cells wide ("Hit 5", the longest value: 76 px), so the plate is always 108 x 36. No tally strokes: the mark has three jobs (skill ranks, rarity, the seal and throbber), and a combo is not one of them. |

Callout size (Settings > Accessibility > "Callout size", OptionsScreen's table):

| Size | Letters | Slot | Spans |
|---|---|---|---|
| Normal | 40 px (30 pt) | 520 x 52 | +128 to +180 |
| Large | 48 px (36 pt) | 520 x 60 | +120 to +180 |
| XL | 56 px (42 pt) | 520 x 68 | +112 to +180 |

Measured at XL: "Posture Broken!" is 466 px, inside the 520 slot.

## States

| State | Trigger | What shows |
|---|---|---|
| Idle | none | Nothing. Both slots are Collapsed. |
| Positive callout | `Mordecai.State.PerfectDodge`, `PerfectBlock`, `Parried`, `PostureBroken` | The string in `good`, for the CalloutPop duration (1.0 s, the shipped `FeedbackDisplayDuration`). |
| Negative callout | `Mordecai.State.ParryWhiff` | "Parry Failed!" in `danger-text`, plus the Shake motion. |
| Replaced | Any new callout while one shows | The new one replaces the old one at once. Callouts never queue and never stack. |
| Combo open | `Mordecai.State.ComboWindow` present | The combo plate with "Hit N". |
| Combo step | Each light hit that lands in the window | The number advances in place. The plate never changes width. |
| Combo closed | ComboWindow removed (800 ms timeout or chain end) | The plate fades out (a whole-widget fade; the plate fill stays opaque). |

Motion: cite the brand book's Motion rows; this component adds no timings of its own.
- **CalloutPop** for every callout.
- **Shake** on "Parry Failed!".
- **Reduce Motion:** the system rule, 150 ms fades with no scale or shake.

Pacing: this is the Callout lane. It ignores the combat hold (callouts and the combo always show at once) and is never queued behind tickets, plates or banners.

## Data and implementation

- **Widgets (keep the classes and BindWidgetOptional names):**
  - `UMordecaiCombatFeedbackWidget`, slot `CombatFeedback`, text `FeedbackText`.
  - `UMordecaiComboCounterWidget`, slot `ComboCounter`, text `ComboText`.
  - Both are children of the HUD root (`UMordecaiCombatHUDWidget`, pushed once on `UI.Layer.Game`), never pushed on their own.
- **Move the slots.**
  - CombatFeedback: from BottomCenter (0, -80) 300 x 30 to anchor (0.5, 0.5), alignment (0.5, 1), offset (0, 180), size 520 x 52 (60 at Large, 68 at XL). Bottom alignment is what makes the slot grow upward.
  - ComboCounter: from BottomCenter (0, -120) 200 x 40 to anchor (0.5, 0.5), offset (0, 184), alignment (0.5, 0), size 220 x 36.
  - The old bottom-centre positions collide with the interact prompt. The new ones keep the eye where it already is, on the player.
- **FeedbackText:**
  - Font F_Tally_Display Bold, 30 pt (40 px x 0.75); 36 pt at Large, 42 pt at XL.
  - `OutlineSettings.OutlineSize` 4, OutlineColor `ink`.
  - ShadowOffset (0, 4), ShadowColorAndOpacity `ink`.
  - VAlign Bottom in the slot.
  - Colour per tag: `good` or `danger-text`. These replace `MordecaiFeedbackColors::Gold` and `::Red`. Author the sRGB hex and convert it with `FLinearColor::FromSRGBColor`.
- **High Contrast:** add an optional `UBorder` "CalloutPlate" behind FeedbackText. It is visible only when the High Contrast theme is active, and its brush is the plate stack (03) with the rim and the ink outline and no print. Slot it VAlign Bottom with 8 px bottom padding, HAlign Center, auto width; text line height = the letter size. On the plate FeedbackText drops its outline and shadow (OutlineSize 0, no ShadowOffset): the plate is the edge, and an ink outline would only notch the white rim.
- **ComboText:**
  - F_Tally_Mono Bold, 18 pt (24 px), `good`, centred in a fixed-width box (5 cells, 76 px).
  - Format stays `"Hit %d"` (ComboIndex + 1).
  - Visibility: HitTestInvisible while `Mordecai.State.ComboWindow` is present, Collapsed otherwise.
  - No stroke widget. The chain length (`LightAttackProfiles.Num()`: Longsword 3, Dagger 5, Greatsword 2, Spear 3, Mace 2, Shortsword 4, Axe 2, Quarterstaff 3, Unarmed 3) is not drawn anywhere on the HUD, and LoadoutPlate draws no strokes either.
- **Known gaps** (the design does not depend on them, but the callouts only tell the truth once they are fixed):
  - The combo never shows: `SetComboIndex` has no caller and nothing adds `ComboWindow` (N24).
  - "Perfect Dodge!" and "Perfect Block!" fire when the window opens, not on success.
  - "Parried!" listens on the player's ASC, but `State.Parried` goes on the attacker (N33).
  - "Posture Broken!" listens on the player, but only enemies break posture. TODO(DECISION): fire it when the player breaks an enemy's posture.
  - The Callout size setting does not exist yet (Settings is "Coming Soon"); Normal is the default.
- **Tokens:** `good`, `danger-text`, `ink`, `plate`, `rim`, `rim-shade`, `callout`, `num-m`, `radius-plate`, `stroke-rim`.

## Copy

Kept exactly. Playtests compare them, so restyle and never reword:
- "Perfect Dodge!"
- "Perfect Block!"
- "Parried!"
- "Parry Failed!"
- "Posture Broken!"
- "Hit N", where N = ComboIndex + 1.

No Tally remarks in this lane, ever. These are the only exclamation marks in the plain voice.

## Do / Don't

- Do show one callout at a time; the newest replaces the current one.
- Do keep the letters off any plate in the Diorama theme. The 4 px ink outline is the edge, and it holds on every floor (see the blockout and night columns).
- Do grow the callout slot upward only; the combo slot stays at (0, +184) at every size and theme.
- Don't add glyphs, icons, bursts, tally strokes or Tally ornament to callouts or the combo.
- Don't route any other message (pickups, rank-ups, warnings) through the centre lane.
- Don't change the case or punctuation of the strings.
- Don't colour the combo by stamina tier or weapon; it is always `good`.
- Don't show the combo plate outside the combo window.
