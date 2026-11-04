# OptionsScreen

The Options screen in the pause-shell frame, whose settings table is the system's single list. Use it from Options on the title screen and, once a real Settings tab exists, inside the pause menu; its keycap switches, segmented keys, notched tracks, live rail preview and Controls rows (Anatomy) replace Lyra's `W_LyraSettingScreen` styling (#62).

## Anatomy
1. **Backdrop**: the toy diorama (floor tiles and a few blocky props) under the `veil`, never a flat grid and never blurred.
2. **Frame**: PauseShell's pieces: the "Options" nameplate hung on two light strings, the folder tab bar at (460, 220), the 1000 x 584 content plate at (460, 276) whose content box is 948 x 540 (padding 20 / 24), and a hint bar at y 892.
3. **Section tabs**: "Gameplay", "Display", "Audio", "Controls", "Accessibility", each with its glyph (`settings`, `display`, `audio`, `controls`, `accessibility`), so the bar matches the pause tabs. The current section is Selected: the cream (`enamel`) fill, fused to the plate (`.tt-selectable.is-selected`). Keyboard and mouse: no bumpers at the bar's ends (with a pad, LB / RB pills sit there).
4. **Rows** (Accessibility, Gameplay, Display, Audio): `plate-raised` slats, 48 px tall, 4 px apart, radius 8; the label in `ui-title` `text` at left, the control right-aligned. Nine Accessibility rows and the description band fit the 540 px box with no scrolling.
5. **Keycap switch** (two values), 124 x 40: a `well` with a 2 px `lip` border and a 58 x 32 keycap that slides and prints its word in Mono 800 20.
   - Off: keycap left, `plate-raised` face, `lip` border, "Off" in `text-muted`; the `cross` glyph in the empty right half.
   - On: keycap right, `enamel` face, ink border, `enamel-shade` lip, "On" in `ink`; the `check` glyph in the empty left half.
   - Locked: dashed well and keycap (dashed means unavailable), `text-disabled`, the `lock` glyph.
6. **Segmented keys** (3-4 values): one `well`, 3 px inset; each option 76+ x 30; the chosen one is Selected (cream fill, ink edge and 800 text) with a keycap lip; the others `text-muted` 600. Numeric options (×1.5, 32 px) in Mono.
7. **Notched track** (ranges; never tally strokes): left and right arrow keycaps (the `arrow-left` and `arrow-right` glyphs), a 224 x 20 core bar well whose `rim` fill runs to the value, core notches every 10%, a 16 x 32 enamel thumb with an ink edge on the value, and the value in `num-m` in a 76 px right-aligned box (fits "150%"). The value moves in 5% steps.
8. **Description band**: at the bottom of the plate, `plate` with a 2 px `lip` border: the focused setting's name in `ui-label` 700 `text`, its one-line description in `ui-label` `text-muted`.
9. **Live preview** (Accessibility only; top-right, 32 px margins, 320 wide): a real TallyRail (the rail head "Kills: 12", a SKILL UP ticket with its rank on the kicker row and the remark in the footer band, and an "Iron Ore x2" slip, TallyRail's markup) and a small "Live preview" plate under it.
10. **Controls rows**: a column head ("Action", "Keyboard and mouse", "Gamepad" in `ui-label` `text-muted`), then one 48 px slat per action in a ScrollBox with a scrollbar: the action in `ui-title`, the keyboard-and-mouse cap in a 260 px column, the gamepad cap in a 196 px column, drawn as Keycap draws them (mouse buttons as a mouse, "Hold" before a held input, "+" for a chord, the dashed unbound cap where a device has no button).
11. **Hint bar**: core `.tt-hintbar`, centred at y 892. Keyboard and mouse: [Esc] "Back" | [←][→] "Change"; while a Controls row listens: [Esc] "Cancel". With a pad: (B) "Back" | [LB] / [RB] "Sections" | (D-pad) "Change" (drawn in the spec band only).

## Settings (the single list; every other card cites this table)
| Section | Setting | Values | Default | Effect |
|---|---|---|---|---|
| Accessibility | High Contrast | Off / On | Off | Swaps the theme DataAsset (Diorama to High Contrast): black plates, white tags, 4 px outlines, a yellow focus ring. |
| Accessibility | Reduce motion | Off / On | Off | Swings, drops, stamps and shakes become 150 ms fades; the loading throbber keeps looping. Overrides the OS setting only when On. |
| Accessibility | Tally notifications | Full / Calm / Minimal | Full | Full: everything. Calm: no remarks, slips only out of combat. Minimal: plates only; tickets go to the spike. (ReceiptTimeline is the source of the pacing.) |
| Accessibility | Tally remarks | On / Off | On | Off collapses every remark line (ticket footers, plate remarks, the quit Dialog body). |
| Accessibility | Ticket duration | ×1 / ×1.5 / ×2 | ×1 | Multiplies TallyRail's base hang times: slip 3 s, ticket 5 s. |
| Accessibility | Callout size | Normal / Large / XL | Normal | Combat callout letters 40 / 48 / 56 px (CombatCallout's; its slot grows upward only). |
| Accessibility | Damage numbers | All / Mine / Off | All | Mine shows only the local player's hits and heals (DamageNumber). |
| Accessibility | UI scale | 80-150%, 5% steps | 100% | Scales the HUD and menu roots. `TODO(DECISION)`: render scale on the layout roots vs the Slate application scale. |
| Accessibility | HUD margin | 32 px / 96 px | 32 px | Every HUD edge offset (the HUD map's 32 becomes 96: TV-safe). |

- None of these exists in the game yet: the pause menu's Settings tab is "Coming Soon" (`TODO(DECISION)`, US-093), and the Accessibility values need a save slot.

## Controls (the game's bindings today)
| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Move | W A S D | Left stick |
| Sprint (toggle) | Shift | Left stick click |
| Jump | Space | A |
| Dodge | Alt | B |
| Light attack | Left mouse | RT |
| Heavy attack | Hold left mouse | Hold RT |
| Block | Hold right mouse | Hold LT |
| Parry | Right + left mouse | LT + RT |
| Fireball | Q | LB |
| Blink | E | RB |
| Stone Skin | R | unbound |
| Restoration | 1 | D-pad Up |
| Interact | F | X |
| Next weapon | Tab | Y |
| Previous weapon | \ | unbound |
| Inventory | I | unbound |
| Pause | Esc | Menu |

- Source: `MordecaiHeroComponent.cpp` (the runtime input mapping context) and `Config/DefaultInput.ini` (CommonUI actions). Open items carried from there: the gamepad stick key names are invalid (US-097), so stick movement and stick-click sprint do not work on a pad yet; LB / RB carry Q / E but the bindings doc says RB = Skill A (US-097 `TODO(DECISION)`); Stone Skin has no pad button.
- Rebinding flow: Enter (or a click on a cap) makes that cap's slot listen: an enamel field reading "Press a key..." (or "Press a button..." in the Gamepad column), focus ring on the row, the hint bar reading [Esc] "Cancel". The next key binds; Esc cancels and so cannot be bound. A key another action already uses opens the Dialog card's "Key already bound" variant (Replace / [Esc] Cancel).
- `TODO(DECISION)`: rebinding does not exist. The hero component builds its input mapping context in code, so nothing is player-mappable and Lyra's Mouse & Keyboard and Gamepad settings pages would list none of these actions. Making them `UPlayerMappableKeySettings` (or a Mordecai binding store) is a code change to decide; until then the Controls section is read-only and shows no listening state.

## States
| State | Look |
|---|---|
| Row focused | Core fill-gap focus: an `ink` ring in the gap and the 3 px `focus` ring outside the slat (no keyline: the plate is the ground). The description band shows that row. |
| Change | Left / Right steps the value; Enter or a click toggles a switch; clicking an option or an arrow key sets it. Applied at once and saved; no Apply button for these settings. |
| Track ends | The arrow key at an end is `aria-disabled` (core's flat `plate` keycap with a dashed `lip` edge and a `text-disabled` arrow). |
| Locked row | Label `text-disabled`, the control dashed with the `lock` glyph, the description says why. |
| Section change | A click on a tab (or LB / RB on a pad) swaps the plate's content with the TabSwitch motion; focus goes to the first row. Keyboard section cycling is `TODO(DECISION)` (Q / E proposed, mirroring LB / RB): never drawn as bound. |
| Listening | See Controls: the slot becomes the enamel "Press a key..." field. |
| Preview | Re-prints its ticket on every change: theme, remarks (the footer band on or off), notifications (Minimal hides tickets), duration (the ticket hangs for the new time), UI scale (the preview scales), HUD margin (the preview moves to 32 or 96 px from the corner). |

## Data and implementation
- New `UMordecaiOptionsScreen` (`ULyraActivatableWidget`) on `UI.Layer.Menu`, pushed by `OptionsButton`. `BindWidgetOptional`: `SectionTabBar`, `SettingsList`, `DescriptionNameText`, `DescriptionText`, `PreviewRail`, `ActionBar`.
- Settings live in Lyra's GameSettings registry (`ULyraGameSettingRegistry`). Restyle its list entries rather than inventing a second settings system:
  - two-option `UGameSettingValueDiscrete` → `UMordecaiSettingSwitch`;
  - three or more options → `UMordecaiSettingSegmented`;
  - `UGameSettingValueScalar` → `UMordecaiSettingSlider` (the notched track: a UProgressBar on the well brush plus notch and thumb Images);
  - key bindings → `UMordecaiSettingKeyRow` (two `UMordecaiKeycapWidget` slots), replacing Lyra's `W_PressAnyKey` and `W_KeyAlreadyBoundWarning` with the listening field and the Dialog variant.
  Each keeps real `UCommonTextBlock`s (`LabelText`, option labels) so the bridge can read and click them.
- Sections: Display maps to Lyra's Video page; Controls holds Mouse & Keyboard and Gamepad. `TODO(DECISION)`: which of Lyra's existing rows stay (language picker #64, replays #60, brightness and gamma).
- The Accessibility settings are new per-player values on `ULyraSettingsShared` (saved with the player's shared settings).
- In game the pause menu's Settings tab shows "Coming Soon" (`UMordecaiPauseMenuPlaceholderWidget`) until a real tab ships (US-093). Then the five sections become a row of filter tags (InventoryTab's FilterBar look) at the top of the Settings content, and the rows scroll in a `UScrollBox`.
- Live preview: an instance of the TallyRail widget fed sample tickets (TallyRail's own: SKILL UP "Dodging" "6 / 20" with the remark "Six. Up from five."; "Iron Ore" "x2"). Skill ranks from use are `TODO(DECISION)`; any remark-carrying ticket kind works.
- Construction from the asset manifest (03): the plate stack, wells, keycaps, the focus ring brush (fill-gap variant on rows).
- Tokens: `veil`, `plate`, `plate-raised`, `well`, `lip`, `rim`, `ink`, `enamel`, `enamel-shade`, `text`, `text-muted`, `text-disabled`, `focus`; type `tally-heading`, `ui-title`, `ui-label`, `num-s`, `num-m`. The backdrop uses `world-*` and `biome-*` only.

| Element | Anchor | Offset | Size |
|---|---|---|---|
| Tab bar | top-left | (460, 220) | 1000 x 56 |
| Content plate | top-left | (460, 276) | 1000 x 584; content box 948 x 540 |
| Hint bar | top-centre | (0, 892) | auto x 52 |
| Live preview | top-right | (-32, 32) | 320 wide |

## Copy
- Title "Options". Sections "Gameplay", "Display", "Audio", "Controls", "Accessibility". Hints "Back", "Change", "Cancel" (and "Sections" with a pad).
- Accessibility rows and descriptions (plain voice):
  - "High Contrast": "Black plates, white tags, thicker outlines and a yellow focus ring."
  - "Reduce motion": "Stops swings, shakes and stamps. Things fade in instead."
  - "Tally notifications": "Full shows everything. Calm drops remarks and holds slips until combat ends. Minimal shows plates only."
  - "Tally remarks": "The Tally's second line on tickets and plates. Headlines read complete without it."
  - "Ticket duration": "How long each ticket hangs on the rail before it moves to the spike."
  - "Callout size": "Size of combat callouts such as Perfect Dodge!"
  - "Damage numbers": "All shows every hit. Mine shows only your hits and heals."
  - "UI scale": "Size of every menu and HUD piece, from 80% to 150%."
  - "HUD margin": "Distance from the screen edge. 96 px keeps the HUD inside a TV's safe area."
- Values: "Off", "On", "Full", "Calm", "Minimal", "×1", "×1.5", "×2", "Normal", "Large", "XL", "All", "Mine", "80%"-"150%", "32 px", "96 px". "Live preview".
- Controls: the column heads "Action", "Keyboard and mouse", "Gamepad"; the action names in the Controls table; "Press a key...", "Press a button...".
- No Tally remarks on this screen.

## Do / Don't
- Do apply accessibility changes immediately and show them in the preview.
- Do keep every control at least 30 px tall inside a 48 px row (the hit area), and every value as text, never colour alone.
- Do keep the value box fixed-width so the row never shifts when the number changes.
- Do draw one device per screen: keyboard and mouse by default, the pad version only after a pad is used.
- Don't use a checkbox, a radio dot, a thin web slider or tally strokes: switches are keycaps, choices are keys, ranges are notched tracks.
- Don't hide High Contrast or Reduce motion behind another section: Accessibility lists them first.
