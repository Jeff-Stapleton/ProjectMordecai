# Dialogue

The town NPC conversation box: a speaker nameplate tag in the World voice on an enamel speech panel at bottom centre, an "[Enter] Continue" hint, and up to four choice buttons, over the veiled world and HUD. Use it when the player talks to a town NPC (the interact prompt reads "Talk to Alchemist"); the dialogue system behind it does not exist yet, so every line and choice here is example copy.

## Anatomy (1920x1080)
- **World and HUD**: the town keeps its place under the `veil` (drawn by this widget), and so does the HUD on `UI.Layer.Game`: dimmed, never blurred, never hidden by hand. The interact prompt hides while talking (nothing is focused); the "[I] Inventory" pill stays, veiled.
- **SpeakerTag** (x 472, y 756, 68 tall): the core eyelet tag (`.tt-tag`: `enamel`, grommet, 3 px `ink` outline, print), sitting 36 px over the panel's top edge.
  - The building glyph (32, `ink`) for a building NPC (`alchemist`, `mage-tower`, `blacksmith`, `clinic`, `keep`); the `speech` glyph for anyone else.
  - The name in the World voice: `world-heading` (Next 800, 40/48, tracked), ToUpper, `ink`. Names are the role until NPCs have names (TODO(DECISION)).
- **SpeechPanel** (x 432-1488, y 792-960, 1056 x 168): `enamel`, `radius-tag`, a 4 px `enamel-shade` foot, 3 px `ink` outline, menu print. Padding 50 / 40 / 28. A fixed three-line box: it never resizes between lines.
  - **LineText**: `ui-body` (22/30), `ink`, at most three lines in 940 px (about 85 characters a line).
  - **NextCue**: the `arrow-down` glyph (24, `ink`), 28 px from the right and 18 px from the bottom, shown once the line is complete and another line follows; hidden on the last line and while choices show. Static (it does not bob).
- **HintBar** (centred on x 960, y 984-1036): core `.tt-hintbar`. Line: "[Enter] Continue | [Esc] Leave". Choice: "[Enter] Choose | [Esc] Leave". Centred so it clears the HUD's Inventory pill at x 1242.
- **ChoiceList** (choice lines only): a menu plate (`.tt-plate--menu`) above the panel's right end, x 968-1488, bottom at y 768, padding 16, up to four `ChoiceButton`s 10 px apart (56 tall each, 286 for the plate).
  - **ChoiceButton**: core `.tt-btn --block` (`plate-raised`, `lip` border, ink print, 700 26/32), the choice text left-aligned.
  - **Stat note**: a stat-gated choice names its stat on the right in `ui-label` ("Charisma 14").

## States
| State | Look |
|---|---|
| Line | The panel holds one line; the hint bar reads "Continue". The line reveals with TypeOn; Enter or a click during TypeOn completes it, the next Enter continues. NextCue shows when the line is complete and another follows. |
| Choice | The panel holds the question; the ChoiceList appears above it (PlateDrop in); focus starts on the first choice; the hint bar reads "Choose". |
| ChoiceButton idle, hover, pressed | Core button states (hover lifts 2 px, press sinks 3 px). |
| Focused | Core button focus: the ink gap, the `focus` ring, the ink keyline. |
| Picked | Selected + focused (D6): the `enamel` fill and `ink` text with the focus ring on top, held while the reply's TypeOn runs, then the list closes. |
| Locked (stat too low) | Core disabled button: `plate` face, dashed `lip` edge, `text-disabled`, the `lock` glyph before the text and the stat note ("Charisma 14") after it. Shown, not focusable. |
| Check met | A normal button whose note keeps the stat name with the `check` glyph, in `text-muted` (`ink-muted` when picked). |
| Leave | Esc (CommonUI back) or the last choice ends the conversation; the panel and tag go out with PlateDrop (out). |
| Reduce Motion | TypeOn shows the whole line at once; PlateDrop becomes a 150 ms fade. |

The preview draws the line state at full size, then the choice state as the lower 680 px of the screen at 1:1, then a ChoiceButton and SpeakerTag state sheet. Keyboard and mouse only; the gamepad differences are in the spec band.

## Data and implementation
- **Widget**: new `UMordecaiDialogueWidget` (`ULyraActivatableWidget`, InputConfig Menu, `bIsBackHandler`), pushed on `UI.Layer.GameMenu` when Interact (F / gamepad X) is pressed on a talkable NPC. It is not pushed on `UI.Layer.Game` (only the death screen is), so the HUD stays and the widget's own `veil` Border covers it. The pause menu (`UI.Layer.Menu`) still opens above it. CommonUI pools it: bind in `NativeOnActivated`, unbind in `NativeOnDeactivated`.
  - Whether talking pauses the world is TODO(DECISION) (the town is safe; the preview assumes it does not).
- **Named widgets** (`Mordecai*` native classes so the agent bridge can read them): `SpeakerTag` with `SpeakerNameText` and `SpeakerIcon`, `SpeechPanel` with `LineText` and `NextCue` (UImage), `ChoiceList` (a VerticalBox of `UCommonButtonBase` `ChoiceButton`s whose first TextBlock is the choice text, so the bridge can `click_button "Restock my potions."`), `StatNoteText` inside a gated button, `HintBar` (a CommonBoundActionBar styled as `.tt-hintbar`).
- **Input**: Enter and gamepad A are CommonUI accept (continue, choose); Esc and gamepad B are back (leave). Up / Down and the D-pad move between choices; the mouse clicks the panel to continue or a choice to pick it. No new key is bound.
- **Data**: no dialogue system exists. TODO(DECISION): a dialogue asset (speaker id, lines, choices, each choice's condition and outcome, such as "open the Alchemist's ServiceScreen"). Until then nothing opens this widget.
- **Charisma**: `attribute_scaling_v1` says Charisma "focuses on town attraction, dialogue leverage, vendor outcomes"; `character_attributes_v1` and the code (`UMordecaiAttributeSet`) give it Cast Speed and Magic Crit Chance instead. Whether CHA gates choices, and at what value, is TODO(DECISION). "Charisma 14" is an example threshold (the default CHA is 10).
- **Text rules**: set `LineText` once per line, to the final string; TypeOn is a mask on an overlay layer, never per-character SetText (the bridge reads the TextBlock).
- **Sizes**: SpeechPanel 1056 x 168 at (432, 792); SpeakerTag at (472, 756), 68 tall; ChoiceList 520 wide, bottom at y 768; HintBar 52 tall at y 984. At 720p the line renders at 14.7 px (`ui-body`), the name at 26.7 px.
- **Tokens**:
  - Colour: `veil`, `enamel`, `enamel-shade`, `ink`, `ink-muted`, `plate`, `plate-raised`, `lip`, `rim`, `rim-shade`, `text`, `text-muted`, `text-disabled`, `focus`.
  - Type: `world-heading` (speaker), `ui-body` (line), the `.tt-btn` style (choices), `ui-label` (stat note, hints).
  - Motion: TypeOn, PlateDrop, KeyPress (the brand book's rows; no local timings).

## Copy
- Example only (no dialogue data exists): the speaker "Alchemist"; the lines "Back already? The kiln has been busy. Six health potions are on the shelf, and the antidotes are still cooling, so take what you need and mind the step." and "What'll it be? Six on the shelf today."; the choices "Restock my potions.", "What do you brew here?", "Spare a few extra?", "Goodbye."; the stat note "Charisma 14"; the speaker "Villager".
- Hints (proposed): "Continue", "Choose", "Leave", keycap first.
- NPC speech is the plain voice of the world: sentence case, no exclamation marks unless the character shouts, no game-system jokes. No Tally remarks in NPC speech: the Tally never speaks for, or about, an NPC.

## Do / Don't
- Do keep every word on the panel, the tag, the choice plate or the hint bar; nothing sits on the veil.
- Do keep the panel one fixed size; a growing box makes the text jump.
- Do show a stat-gated choice the player cannot take, dashed and locked with the stat named, so the player learns the gate exists.
- Do let Esc leave at any time; never trap the player in a conversation.
- Don't hang the choices on strings or draw them as tickets: they are buttons, and tickets are the Tally's.
- Don't put a Tally remark, a ticket or a rank-up inside a conversation; the rail prints them after it closes.
- Don't draw the gamepad caps on the keyboard screen; CommonUI swaps them by the last input.
