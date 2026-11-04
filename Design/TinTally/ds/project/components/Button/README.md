# Button

A menu button: a tag-shaped control on `plate-raised` that lifts on hover and sinks on press, led by one keycap or glyph before its label. Use it for every clickable action in menus, dialogs and service screens; use primary for the one safe answer in a view, danger only for a destructive action (deleting a save), and the small row action for actions inside list rows.

## Anatomy
- **Body**: minimum height 56, padding 10 / 24, gap 12 (`space-3`), radius 10 (`radius-tag`), 2 px edge.
- **Print**: (0, 4) solid `ink` under the body. It grows to 6 on hover and drops to 1 when pressed.
- **Lead**, ONE only, always before the label (D4: the keycap leads its label everywhere, as in "[F] Pick up ..."):
  - **Keycap** (`.tt-btn__key`): a 32 px keycap (`.tt-key--sm`) when a key fires the button: "[Esc] Stay", "[Enter] Identify". It sits 18 px from the left edge (a -6 px pull).
  - **Glyph** (`.tt-btn__glyph`): otherwise an optional 24 px glyph in the label colour: `arrow-left` Back, `cross` Close, `mage-tower` Identify, `skill-point` Allocate. A list uses glyphs on every item or on none.
  - **Disabled**: the lead becomes the `lock` glyph; the keycap hides.
- **Label** (`.tt-btn__label`): `ui-title` 26 / 32 700, one line, a verb in sentence case.
- **Variants**:
  - Default: `plate-raised` face, `text` label, `lip` edge. Quit is a default button, always.
  - Primary (`--primary`): `enamel` face, `ink` label, `ink` edge. The safe answer: one per view.
  - Danger (`--danger`): `danger` face, `ink` label (6.0:1), `ink` edge. Destructive actions only (delete save); never Quit, never an error's only button. No shipping screen has a destructive action yet: "Delete save" is example copy.
- **Sizes**:
  - Regular: 56 tall.
  - **Row action** (`--sm`): the one look for actions inside list rows: InventoryTab's "Identify" and SkillsTab's "Allocate" use it, with no local button styles. Default variant (never the cream fill: cream is Selected), 48 tall (the minimum mouse target, `space-7`), padding 6 / 16, gap 8, a 20 px leading glyph (`mage-tower`, `skill-point`; `lock` when disabled), label 22 / 28 700. It sits at the right end of its row.
  - Block (`--block`): fills its column; the label stays left-aligned.
- **Groups** (Dialog's rule):
  - `.tt-btn-row`: right-aligned, 16 apart, each at least 152 wide. The risky action on the LEFT, the safe action on the RIGHT: primary, default focus, and the Back keycap ([Esc] / B) leads it, because Back always picks the safe answer. The risky action is a plain default button with no keycap.
  - `.tt-btn-list`: a stacked list, 12 apart. Front-end menus are not button lists: TitleScreen hangs tags on a rail.

## States
| State | Default | Primary | Danger |
|---|---|---|---|
| Rest | print 4 | print 4 | print 4 |
| Hover (mouse) | lifts 2, print 6, edge turns `rim` and doubles to 4 px | lifts 2, print 6, ink edge doubles to 4 px | same as primary |
| Pressed | sinks 3, print 1, face `plate`, edge `rim`, 3 px `rim-shade` band inside the top | sinks 3, print 1, face `enamel-shade`, 3 px band of `ink` 30% inside the top; the keycap presses too | sinks 3, print 1, face `danger` mixed 18% toward `ink`, 3 px darker band inside the top |
| Focused (keyboard, gamepad) | core button focus: 3 px `focus` ring 2 px outside, a 2 px `ink` ring in the gap and the `ink` keyline outside; the print stays; combines with the other states | same | same |
| Disabled | face `plate`, dashed `lip` edge, `text-disabled` label, no print; the lead becomes `lock` | same, colour dropped | same, colour dropped |

- The doubled edge and the top band keep hover and pressed apart in High Contrast, where `lip`, `rim` and `enamel-shade` are all white. The white edge stays whole when pressed (D22): the press shows by the sink and the band, never by losing an edge.
- Transitions are 100 ms, ease-out (Motion KeyPress for the keycap). With Reduce Motion there is no lift and no sink; the edge and face changes stay.
- Disabled buttons are skipped by navigation (the CommonUI default). The reason sits next to them in plain text ("0 skill points").

## Data and implementation
- **Widget**: subclass `UCommonButtonBase` (new, e.g. `UMordecaiButton`), with one `UCommonButtonStyle` asset per variant: `BS_TT_Default`, `BS_TT_Primary`, `BS_TT_Danger`, `BS_TT_Row` (the `--sm` row action).
  - Brushes:
    - `NormalBase`, `NormalHovered` and `NormalPressed`: RoundedBox brushes, radius 10, with a 2 px outline in the edge colour from the table.
    - `Disabled`: a `plate` face with a `lip` outline. The dash comes from a 9-slice texture: RoundedBox outlines cannot dash.
  - Metrics: `MinHeight` 56 (48 for the row action), `ButtonPadding` (24, 10) ((16, 6) for the row action).
  - Text styles: `NormalTextStyle`, `NormalHoveredTextStyle` and `DisabledTextStyle` as CommonTextStyles, Atkinson Hyperlegible Next 700 at 19.5 pt (26 px x 0.75); 16.5 pt (22 px) for the row action.
- **Lift, print and sink** cannot live in a button style.
  - Put an `ink` print Border behind the face inside the button's Overlay.
  - Drive both from `NativeOnHovered`, `NativeOnUnhovered`, `NativeOnPressed` and `NativeOnReleased` with two 100 ms UMG animations: face RenderTranslation -2 / +3, print height 6 / 1.
- **Keycap lead**: `UCommonButtonBase`'s `InputActionWidget` slot (`BindWidgetOptional`, a `UCommonActionWidget`) placed FIRST in the content HBox. It shows `TriggeringInputAction` / `TriggeringEnhancedInputAction` in the current device's caps (see Keycap). The dialog keycaps are the CommonUI universal actions: Back (Esc / B) on the safe answer and Confirm.
- **Label**: a real `UCommonTextBlock`, and it must be the button's FIRST TextBlock. The playtest bridge clicks a button by its first TextBlock's text: `click_button("Identify")`. The action widget draws its cap as an image, so it adds no TextBlock. If Keycap's text-driven `UMordecaiKeycapWidget` is chosen instead (Keycap `TODO(DECISION)`), keep the label first in tree order: an Overlay whose first child is the label (left padding = cap width + 8) and whose second child is the cap.
- **Glyph lead**: a UImage, never text.
- **Focus ring**: the core focus ring brush (`03` Focus ring) in its button form: a 2 px `ink` gap, the 3 px `focus` ring, the 2 px `ink` keyline. Show it when the button has keyboard or gamepad focus (`HasAnyUserFocus` with a non-mouse input method), never on mouse hover alone.
- **Replaces**:
  - The engine default `UButton` style (a dark box with a thin light outline) on the "Identify" button in `UMordecaiInventoryWidget` rows. That button becomes the row action (`BS_TT_Row`), keeps its "Identify" TextBlock, and gains a `mage-tower` glyph lead. The row's name TextBlock keeps its composed string ("? Unidentified Purple Amulet").
  - The future Allocate action in `UMordecaiSkillTreeWidget`: the row action with a `skill-point` lead, enabled when `CanAllocateSkillPoint`, disabled at 0 points or rank 20.
  - The MageTower's Identify: a regular button with the [Enter] keycap lead.
  - Lyra's confirmation buttons in Dialog (`W_ConfirmationDefault`).
- **Not buttons**: tabs and inventory filters (`UMordecaiTabButton`) are PauseShell's folder tabs and InventoryTab's filter tags, with the core Selected fill. Title-screen choices are TitleScreen's tags on a rail.
- **Colours**: theme DataAsset tokens converted with `FLinearColor::FromSRGBColor`: `plate-raised`, `plate`, `lip`, `rim`, `rim-shade`, `enamel`, `enamel-shade`, `danger`, `ink`, `text`, `text-disabled`, `focus`.

## Copy
- Labels are verbs in sentence case, with no exclamation marks, kept exactly: "Identify", "Allocate", "Stay", "Quit". "Back" and "Close" are the CommonUI action names. Never "OK", "Yes" or "No".
- The quit dialog is Dialog's: headline "Quit to desktop?" (`tally-heading`), the Tally remark "The Tally will keep count." as the optional second line (hidden when remarks are off), "Quit" (default, left) and "[Esc] Stay" (primary, right, focused). The footer holds the buttons only: the hints are Dialog's hint bar, core `.tt-hintbar` centred 24 px under the plate, [←][→] "Move" | [Enter] "Select".
- Example copy in the preview: "Delete save" (no save screen exists yet: `TODO(DECISION)`), "Longswords 12 / 20", "Dodging 20 / 20". The title-menu labels "Play", "Options", "Credits", "Quit" are TitleScreen's.
- No Tally remark on a button, ever.

## Do / Don't
- Do put the risky action left and the safe action right, make the safe action primary, give it focus when a confirmation opens, and let Back pick it.
- Do lead with the keycap when a key fires the button, and keep one lead per button.
- Do use the row action (`--sm` + glyph) for every action inside a list row.
- Do pair every disabled button with the lock glyph and a plain reason nearby.
- Don't colour Quit `danger`, and don't use `danger` for anything that is not destructive.
- Don't show state by colour alone: hover changes edge and height, pressed changes face and height.
- Don't bake labels into textures, and don't put a TextBlock before the label.
- Don't mix glyph and no-glyph items in one list, or show a keycap for an input that is not bound on the current device.
