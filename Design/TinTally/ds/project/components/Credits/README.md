# Credits

The credits as one long enamel ticket that prints up out of the Tally's slot: the people, the engine notice and the three font licence notices. Use it for Credits on the title screen, in place of Lyra's `W_Credits` (issue #63).

## Anatomy
1. **Backdrop**: the front-end level (level art, not UI) with the camera dollied in over the title island (no rotation), so the island's top face fills the frame: the core `.tt-diorama` grass tiles and a few of the island's props (the sand path, the pond, trees, rocks, a bush, posts, flowers and the relic) either side of the ticket, under the `veil`. Never a blank backdrop, never blurred. Nothing sits on the veil but the ticket, the machine and the hint bar.
2. **The machine** (bottom centre): an 800 x 136 `tt-plate--menu` at (560, 912) with four rivets (a front-end menu plate). Centred in it: the seal disc and the status in `ui-body` `text-muted`: "Printing...", then the `check` glyph and "Printed". Text only: no throbber (the throbber belongs to the loading screen alone).
3. **The mouth**: a 752 x 20 `well` slot with a `lip` border and ink outline at (584, 902), drawn over the ticket's foot so the paper reads as coming out of it.
4. **The ticket**: 720 wide at x 600, `enamel`, ink outline, padding 36 / 48 / 56. It rises at 40 px/s; the newest line is always just above the mouth.
   - Top end: a dashed `ink-muted` tear line 14 px from the top: the paper's perforation, as on every ticket stub.
   - Head: the seal (a 64 px `enamel-shade` disc with a dashed ring, the Tally's mark), "CREDITS" in `kicker` `ink-muted`, "PROJECT MORDECAI" in `tally-title` (ToUpper).
   - Rules: core `.tt-divider--enamel` (2 px, `ink` at 30%) across the column, 28 px above and below, after the head and between sections. Never a row of tally strokes.
   - Sections, each with a `kicker` heading: "MADE BY", "BUILT WITH", "FONTS".
   - Receipt rows: role in `ui-body` `ink`, a 3 px dotted `ink-muted` leader, the name in `ui-title` `ink`. A name not given yet is a dashed "TODO" chip (Mono 700 20, `ink-muted`; dashed means unavailable).
   - Notices in `ui-label` (20 px, the floor): `ink` for copyright lines, `ink-muted` for the rest. Long URLs break after "github.com/".
   - End: a rule, "Thank you for playing." in `ui-title` and the seal, then the foot: the machine cuts the paper on a dashed tear line 14 px above the foot, matching the top end.
5. **Hint bar**: core `.tt-hintbar` at bottom-right (-32, -32), like every front-end screen: [↑][↓] "Scroll" | [Esc] "Back". After a pad is used: (R stick) "Scroll" | (B) "Back" (drawn in the spec band).
6. **Cut pieces** (review only, never in game): where the spec band shows a piece cut from the long ticket, the paper ends in a torn zigzag, 16 px teeth 8 px deep, with a 3 px `ink` edge. The teeth take the colour of the ground behind and cover the outline and print along the cut, so a cut reads as paper, never as clipping.

## States
| State | Look |
|---|---|
| Open | The ticket's tear line appears at the mouth and rises at 40 px/s. Status "Printing...". |
| Printing | The ticket keeps rising; Up / Down (or the right stick) scrolls back through what has printed, then snaps back to the newest line after 3 s idle. |
| Printed | The end has cleared the mouth: the ticket stops with the seal 120 px above the mouth; the status shows the `check` glyph and "Printed". |
| Back | Esc / gamepad B returns to the title screen with focus on Credits. Nothing auto-closes. |
| Reduce Motion | The same 40 px/s rise (reading speed, not decoration); no paper wobble. |

## Data and implementation
- New `UMordecaiCreditsScreen` (`ULyraActivatableWidget`) on `UI.Layer.Menu`, pushed by `CreditsButton`. `BindWidgetOptional`: `TicketScroll` (`UScrollBox`, clipped at the mouth), `TicketBox`, `StatusText`, `StatusIcon` (the `check` glyph, Collapsed while printing), `ActionBar`.
- Content from data, not the widget: a DataTable `DT_Credits` (Section, Role, Name, bPlaceholder) for people and a String Table `ST_Credits` for the notices. Every line is a real `UTextBlock` so the bridge can read it.
- Rows with `bPlaceholder` print the "TODO" chip in development builds and are left out of Shipping. `TODO(DECISION)`: the real names and roles (#63).
- Print: advance the scroll offset 40 design px per second (scale by the DPI scale). The full ticket is about 1800 px, about 45 s.
- Construction from the asset manifest (03): the machine is the plate stack (menu print) with rivet Images; the ticket is a tag-stack face with no grommet; the rules are 2 px Images (`ink` at 30%); the mouth is a well.
- Fonts: ship each family's full `OFL.txt` with the build (for example under `Content/Fonts/Licenses/`, cooked as non-asset files); the credits lines are the attribution, not a substitute for the licence text.
- Remove Lyra's credits content: the LYRA logo, the UE5 logo, the placeholder rows "CREDIT TITLE : Credit Person Name N". Orbitron needs a notice only while a Lyra screen still ships it.
- `TODO(DECISION)`: confirm the engine notice's wording against the Unreal Engine EULA in force before shipping.
- Tokens: `veil`, `plate`, `rim`, `rim-shade`, `well`, `lip`, `ink`, `ink-muted`, `enamel`, `enamel-shade`, `text`, `text-muted`; type `tally-title`, `kicker`, `ui-title`, `ui-body`, `ui-label`. The backdrop uses `world-*` and `biome-*` only.

| Element | Anchor | Offset | Size |
|---|---|---|---|
| Ticket | bottom-centre | x 600, foot at y 930 (behind the machine) | 720 x ~1800 |
| Mouth | bottom-centre | (584, 902) | 752 x 20 |
| Machine | bottom-centre | (560, 912), 32 px bottom margin | 800 x 136 |
| Hint bar | bottom-right | (-32, -32) | auto x 52 |

## Copy
Kept exactly, in print order:
- "CREDITS", "PROJECT MORDECAI".
- "MADE BY": "Created by", "Design", "Programming", "Art", "Audio", "Playtesting", each with "TODO" until named.
- "BUILT WITH": "Engine" / "Unreal Engine 5.7", "Starter project" / "Lyra Starter Game".
  - "Project Mordecai uses the Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere."
  - "Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved."
- "FONTS" (copyright lines as in each font's name table):
  - "Rowdies" / "Designed by Jaikishan Patel (Magictype)" / "Copyright 2018 The Rowdies Project Authors (https://github.com/magictype/rowdies)"
  - "Atkinson Hyperlegible Next" / "Braille Institute · Applied Design Works · Letters from Sweden" / "Copyright 2020-2024 The Atkinson Hyperlegible Next Project Authors (https://github.com/googlefonts/atkinson-hyperlegible-next)"
  - "Atkinson Hyperlegible Mono" / "Braille Institute · Applied Design Works · Letters from Sweden" / "Copyright 2020-2024 The Atkinson Hyperlegible Mono Project Authors (https://github.com/googlefonts/atkinson-hyperlegible-next-mono)"
  - "All three are licensed under the SIL Open Font License, Version 1.1. This license is available with a FAQ at: https://openfontlicense.org"
- "Thank you for playing."
- Machine: "Printing...", "Printed". Hints: "Scroll", "Back".
- No Tally remarks: credits may one day follow an ending, and "Count closed." belongs to the Acceptance ending only.

## Do / Don't
- Do keep the font notices word for word from the font files when versions change.
- Do let the player scroll back; never make them wait to read a line again.
- Do keep names in `ui-title` and roles in `ui-body`, one row each, leaders between.
- Don't run the throbber here or draw tally strokes as rules: "Printing..." is text, rules are `.tt-divider--enamel`.
- Don't ship a "TODO" chip.
- Don't add logos to the ticket; the seal is the only mark.
