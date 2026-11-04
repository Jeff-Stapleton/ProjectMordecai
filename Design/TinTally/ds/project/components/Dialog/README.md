# Dialog

A 640-wide modal plate that asks one question or reports one problem, over the veil. Use it for the quit confirm, other two-answer confirms (key already bound), system errors and the controller-disconnected prompt, in the front end and in game; it replaces Lyra's `W_ConfirmationDefault`, `W_ConfirmationError` and `W_ControllerDisconnected` styling (#62, #68).

## Anatomy
1. **Veil**: `veil` over the frozen world (the toy diorama in the previews) or the front-end backdrop. No text sits on it.
2. **Plate**: `tt-plate tt-plate--menu`, 640 wide, auto height (about 236 with a one-line body, 216 without, 310 for a two-line error). Clips its band to the rim radius.
3. **Error band** (error variant only): 44 px `danger` fill with a 2 px ink rule under it; the `cross` glyph (20), "ERROR" (Atkinson Next 800, 20 px, tracked, ToUpper, `ink`) and a 16 px strip of the system's one warning hatch (core `.tt-hatch`: `danger` and `ink` at -45°) in a 2 px ink box, filling the rest.
4. **Stamp**: a 64 px disc with the seal treatment (a 2 px dashed inner ring 5 px inside, a 2 px ink outline, a 3 px ink print), turned a few degrees.
   - Confirm: `enamel` with the `seal` glyph (32) in `ink`.
   - Controller disconnected: `enamel` with the `controls` glyph (a gamepad).
   - Error: `danger` with the `cross` glyph in `ink` (a failure mark, not a question).
5. **Title**: `tally-heading` (Rowdies 32/38) in `text`. One line; a question for confirms.
6. **Body**: `ui-body` (22/30) in `text-muted`, at most two lines at 640. Optional. A key named in the body is a keycap leading the words about it ("[Q] is Fireball's key.").
7. **Footer**: the buttons, right-aligned, 16 px apart, min 152 wide, 56 tall (`tt-btn`). **The button rule (D4; this card is its source):**
   - The risky answer is a plain `tt-btn` (`plate-raised`, `lip` border) on the LEFT, with no keycap. Quit is a plain button: never `--danger`. `--danger` is only for destructive actions such as deleting a save.
   - The safe answer is `tt-btn--primary` (enamel) on the RIGHT. It has default focus and carries the Back keycap: Back always picks the safe answer.
   - The keycap LEADS its label: "[Esc] Stay", "[Esc] Cancel", "[Enter] Continue", as in the shipping "[F] Pick up ...".
8. **Hint bar**: a dialog with two or more buttons gets core `.tt-hintbar` centred 24 px under the plate: [←][→] "Move" | [Enter] "Select". A one-button dialog needs none: its button carries the key.

## Variants
| Variant | Title / body | Buttons (left, right) |
|---|---|---|
| Quit confirm | "Quit to desktop?" / "The Tally will keep count." (a Tally remark) | "Quit"; "[Esc] Stay" (default) |
| Error (example copy) | "Couldn't load the map" / "The Expedition failed to start. You will return to the title screen." | "[Esc] Return to title" (default) |
| Controller disconnected | "Controller disconnected" / "Reconnect it to keep playing." | "[Enter] Continue" (default; (A) Continue with a pad) |
| Key already bound (example copy) | "Key already bound" / "[Q] is Fireball's key. Bind it to Blink and leave Fireball without one?" | "Replace"; "[Esc] Cancel" (default) |

## States
| State | Look |
|---|---|
| Open | The veil fades in; the plate drops 12 px and settles (Reduce Motion: a fade only). Default focus on the primary (safe) button. |
| Focus on primary | Core button focus: the 3 px `focus` ring 2 px outside, an ink ring in the gap, the ink keyline, the print kept. |
| Focus moved | Left / Right or the D-pad moves the ring; Esc / gamepad B still picks the safe answer. |
| Pressed | The button sinks 3 px and its print collapses; the dialog closes with that result. |
| Remarks Off | The quit body "The Tally will keep count." is a Tally remark: with Tally remarks Off it is not shown and the title stands alone. Plain bodies (errors, key already bound, controller) always show. |
| Error | Danger band with the hatch, danger stamp with the cross, one primary button that is also the Back action. |
| Controller back | The controller-disconnected dialog closes by itself when a controller reconnects. |

- One device per screen (D5): the previews are keyboard and mouse. With a pad the Back keycap on the safe button becomes (B), "[Enter] Continue" becomes (A) "Continue", and the hint bar reads (D-pad) "Move" | (A) "Select".

## Data and implementation
- New `UMordecaiConfirmationDialog`, a `UCommonGameDialog` subclass (Lyra's `ULyraConfirmationScreen` is the model). Register it in `Config/DefaultGame.ini` as Lyra UI Messaging's `ConfirmationDialogClass` and `ErrorDialogClass` (where `W_ConfirmationDefault` / `W_ConfirmationError` are set today, lines 80-81). One class with an `bIsError` style switch, or two subclasses; either way the same widget tree. Pushed on `UI.Layer.Modal`, above the pause menu (`UI.Layer.Menu`) and the HUD (`UI.Layer.Game`). In game the world stays paused under it if it was paused.
- Keep Lyra's bind names if you subclass `ULyraConfirmationScreen`: `Text_Title`, `RichText_Description`, `EntryBox_Buttons`, `Border_TapToCloseZone`, plus the `CancelAction` row. Add `BindWidgetOptional` `StampImage`, `ErrorBand`, `ActionBar` (the hint bar). The body's inline keycap is a rich-text decorator (`RichText_Description` with a keycap inline).
- Content comes from `UCommonGameDialogDescriptor`: `Header` → title, `Body` → body (collapse the body when empty), `ButtonActions` → buttons (Result + OptionalDisplayText). `EntryBox_Buttons` lays them out with the `Declined` / `Cancelled` (safe) answer last, so it lands on the right.
  - Quit confirm: `Confirmed` "Quit", `Declined` "Stay"; `Cancelled` (Back) resolves as Stay. Quit calls `UKismetSystemLibrary::QuitGame`.
  - Error: one `Confirmed` action with OptionalDisplayText "Return to title".
  - Key already bound (from OptionsScreen's Controls rows): `Confirmed` "Replace", `Declined` "Cancel"; Back resolves as Cancel. It replaces Lyra's `W_KeyAlreadyBoundWarning`.
- Controller disconnected: new `UMordecaiControllerDisconnectedScreen`, a subclass of `ULyraControllerDisconnectedScreen` built with this widget tree, set as `ULyraHUDLayout::ControllerDisconnectedScreen` on the HUD root (the HUD root becomes a Lyra HUD layout, D20). Lyra pushes it on `UI.Layer.Menu` when every controller disconnects and pops it on reconnect. Keep its required `BindWidget`s `HBox_SwitchUser` and `Button_ChangeUser` (Collapsed unless the platform has strict controller pairing). Add `ContinueButton` (deactivates the screen and goes on with keyboard and mouse).
  - `TODO(DECISION)`: Lyra shows this screen only on platforms with the `Platform.Trait.Input.PrimarlyController` trait (consoles), so on PC it never appears today. Whether a PC player who was on a pad gets it, and whether "Continue" exists, is open.
- `GetDesiredFocusTarget()` returns the primary (safe) button. Buttons are real `UCommonButtonBase` with `UCommonTextBlock` labels so the bridge can `click_button "Stay"`.
- Construction from the asset manifest (03): the plate stack (menu print), the hatch from `M_UI_PatternFill`, the stamp a badge-style disc with a dashed ring Image.
- Tokens: `veil`, `plate`, `plate-raised`, `rim`, `rim-shade`, `lip`, `ink`, `enamel`, `enamel-shade`, `ink-muted`, `text`, `text-muted`, `danger`, `focus`; type `tally-heading`, `ui-body`, `ui-label`.

| Part | Size |
|---|---|
| Plate | 640 x auto, centred on screen |
| Error band | 640 x 44 |
| Content padding | 30 top, 36 right, 30 left |
| Stamp | 64 x 64, 22 px gap to the text |
| Footer | padding 26 / 32 / 30 / 30; buttons 152+ x 56, 16 apart, right-aligned |
| Hint bar | centred, 24 px under the plate, auto x 52 |

## Copy
- Quit: title "Quit to desktop?", body "The Tally will keep count." (a Tally remark: hidden when remarks are Off), buttons "Quit" (left) and "[Esc] Stay" (right, primary, default, Back).
- Error example: title "Couldn't load the map", body "The Expedition failed to start. You will return to the title screen.", button "Return to title". Band word "Error". Example copy for a failed `HostSession` from ExpeditionSelect (the front end hosts offline, so network errors are not the common case); `TODO(DECISION)`: the error strings, written per failure the code can report.
- Controller disconnected: title "Controller disconnected", body "Reconnect it to keep playing.", button "Continue". Plain voice, no remark.
- Key already bound (example copy): title "Key already bound", body "[Q] is Fireball's key. Bind it to Blink and leave Fireball without one?", buttons "Replace" and "Cancel".
- Hints: "Move", "Select". Buttons are verbs; never "OK", "Yes" or "No".

## Do / Don't
- Do make the safe answer the right-hand primary button, the default focus and the Back action, every time.
- Do put the keycap before the label on a button.
- Do keep titles to one line and bodies to two; longer content is a screen, not a dialog.
- Do use the error variant only for failures the player did not choose.
- Don't colour Quit (or any risky answer that is not destructive) `danger`: the band and stamp carry danger only on errors.
- Don't put a Tally remark on an error, a death, a disconnect or anything about loss.
- Don't stack dialogs; queue them.
