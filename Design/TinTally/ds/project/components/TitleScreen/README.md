# TitleScreen

The Mordecai title screen: a riveted wordmark plate, four menu tags threaded under the rail, the toy-island backdrop, a hint bar and a dev-only version stamp. Use it as the first screen after boot, in place of Lyra's `W_LyraFrontEnd` on `L_LyraFrontEnd` (issues #57, #58, #61).

## Anatomy
1. **Backdrop** (the front-end level, not UI): a toy island on a tin plinth in a dark display case.
   - The stand-in fixes the composition: the island is about 800 px wide, centred near (1400, 560), and nothing sits in the left column (x 0-940).
   - Its top face is the core diorama (`.tt-diorama` tiles, a path, a pond) with core props (trees, rocks, a bush, posts) and the relic: an Automation-Age counting machine gone to moss, still printing a ticket.
   - Fixed camera at the game pitch, never rotated; an optional slow dolly. The light pool and vignette are post-process in the level. No characters (#58).
   - Level art only uses the theme-invariant `world-*` and `biome-*` tokens: High Contrast never recolours it.
2. **Wordmark plate**: `tt-plate--menu` at (160, 120), about 740 x 310, padding 36 / 48 / 34 / 36, with four 12 px corner rivets (`rim-shade` with a `rim` highlight and a 2 px ink ring). Rivets are allowed here because this is a front-end menu plate.
   - Seal: a 136 px `enamel` disc in the ink outline and print, rotated -8°, a 2 px dashed `ink-muted` ring 8 px inside, the `seal` glyph at 64 px in `ink`. The seal is one of the tally mark's three jobs.
   - "PROJECT MORDECAI" in `tally-display` (Rowdies 700, 80/84) on two lines, `text`, with a hard 5 px `ink` print under the letters.
   - The rule under the word: core `.tt-divider`, 448 x 2, `rim` at 50%, 18 px under the word. Never a row of tally strokes.
   - Tagline in `ui-title`, `text-muted`, 16 px under the rule.
3. **Menu**: the rail and four 360 x 64 enamel tags at x 160, 28 px apart, the top tag at y 600.
   - The rail is core `.tt-railbar` (the one rail family): 408 x 20 at (136, 536), a teal bar with an end cap at each end holding an eyelet; the caps overhang 16 px (visual x 120-560).
   - One cord threads all four tags: core's light string (a 4 px `rim` core with 1 px `ink` sides), so it reads over the dark case and over enamel.
   - Threading follows Tag's threaded stack. The cord runs behind the tags and shows only in the 28 px gaps. On each tag a short segment crosses the face from the top edge into the eyelet (the core grommet, centre x 22). Under Quit the cord ends in a 14 px `rim` knot with a 2 px ink ring, 24 px below the tag. Read top to bottom it goes over, through the hole, behind: a threaded cord, never a line drawn across the tags.
   - Each tag: eyelet stub with a dashed `ink-muted` perforation at x 44, then the label in `tally-heading` (Rowdies 32) `ink` from x 64.
   - The focused tag shows the confirm keycap (`tt-key--sm tt-key--wide` "Enter") LEADING its label, so the label steps right; the keycap leads its label everywhere.
4. **Version stamp** (non-Shipping builds only): a 52 px plate at bottom-left (32, -32), `num-s` `text-muted`, with a "Dev" well tag.
5. **Hint bar**: core `.tt-hintbar` at bottom-right (-32, -32): [↑][↓] (the `arrow-up` / `arrow-down` glyphs in keycaps) "Move" | [Enter] "Select". One device per screen: keyboard and mouse by default; after a pad is used it reads (D-pad) "Move" | (A) "Select" (drawn in the spec band).

## States
| State | Look |
|---|---|
| Boot | The tags drop from the rail one after another, each with the TicketDrop swing about its eyelet. Then Play takes focus. |
| Default | Enamel tag, 3 px ink outline, 4 px print. |
| Hover (mouse) | Lifts 2 px, print 6 px. |
| Focused | Rotates 3° about the eyelet and keeps its ink outline and print; core focus (ink, focus ring, ink keyline) right outside the outline; the Enter keycap appears before the label. Play has focus on boot. |
| Pressed | Sinks 3 px, print collapses to 1 px, the face goes `enamel-shade` with a 5 px sunk lip (`ink` at 24%) along its top edge; then the next screen pushes. The lip keeps Pressed distinct from Focused in High Contrast, where `enamel-shade` turns white and the print vanishes on black; the ink outline stays whole. |
| Reduce Motion | No drop and no swing: the tags fade in; the focused tag sits at 3° without animating. |

- Navigation is vertical, no wrap. Back (Esc / gamepad B) on the title screen opens the quit Dialog.
- Prompts show keyboard keys until device-aware glyphs ship (design log 2026-07-10).

## Data and implementation
- New `UMordecaiTitleScreenWidget` (`ULyraActivatableWidget`, CommonUI) as the front-end main screen: set it as `ULyraFrontendStateComponent::MainScreenClass` on the front-end experience in place of `W_LyraFrontEnd`. Drop the press-start screen (`W_LyraStartup`) on every platform that skips it today.
- Buttons: four `UMordecaiMenuTagButton` (`UCommonButtonBase`) named `PlayButton`, `OptionsButton`, `CreditsButton`, `QuitButton`, each with a real `UCommonTextBlock` `LabelText`, so the playtest bridge can `click_button "Play"`. Each button holds a `ConfirmKey` keycap (Keycap's `UMordecaiKeycapWidget`, the confirm action), Collapsed unless the button has focus, placed before `LabelText`.
  - Play pushes ExpeditionSelect. Options pushes OptionsScreen. Credits pushes Credits. Quit opens the quit Dialog, which calls `UKismetSystemLibrary::QuitGame` on "Quit".
  - `TODO(DECISION)`: what Play does (#57, #60). This design pushes ExpeditionSelect (a playlist screen); booting straight into the arena is the alternative.
- Construction from the asset manifest (03): the wordmark plate is the plate stack (menu print); the tags are the tag stack (pivot at the eyelet for TicketDrop); the cord is the Strings light cord, `CordImage`, in the button column's Overlay BEHIND the buttons, from the rail to the knot (`CordKnotImage`); each button draws its own `ThreadImage` from its top edge to the eyelet, so the segment swings with the tag. The rail is the rail family (`RailImage`, the plate stack at radius 10 with two eyelet end caps).
- Wordmark: `TitleText` holds "Project Mordecai" with `TextTransformPolicy = ToUpper`, auto-wrapped at 520 px so it breaks into two lines (the string stays "Project Mordecai"). `TaglineText`. `SealImage` (from the `seal` glyph). `TitleRule`: a 448 x 2 Image, `rim` at 50%.
- Hint bar: a CommonUI action bar (`UCommonBoundActionBar`) styled as the core hint bar; see Keycap for how its entries get the caps (`TODO(DECISION)` there).
- Version: `VersionText` = `UGeneralProjectSettings::ProjectVersion` + commit + build configuration, Collapsed when `UE_BUILD_SHIPPING`. `TODO(DECISION)`: the version scheme; `ProjectVersion` is unset today (#61). The string in the preview is an example.
- Remove from the front end: the LYRA logo and its intro animation, the UE roundel, "SHOW REPLAY", "Player 1: <machine>", the NULL net ID, "UE5-CL-0 (Development)", "Player 2: Not logged in" and ADD PLAYER (#57, #61). `TODO(DECISION)`: whether Lyra's hosting, joining and replay screens stay reachable (#59, #60); this design reaches none of them.
- Shared front-end classes live in this component's CSS (`tt-fe-*`: the screen frame, the front-end level stand-in, cord placement (`tt-fe-cord`, `--threaded`, `--knot`, `tt-fe-thread`), the hanging menu tag, the hint-bar and version-stamp placement, spec-band helpers). ExpeditionSelect, OptionsScreen, Credits and Dialog use them. A cord that hangs one piece runs in front, straight into its eyelet; a cord that threads several pieces is always the threaded build.
- Tokens: `plate`, `plate-raised`, `rim`, `rim-shade`, `ink`, `enamel`, `enamel-shade`, `ink-muted`, `text`, `text-muted`, `well`, `lip`, `focus`; type `tally-display`, `tally-heading`, `ui-title`, `num-s`. The backdrop uses `world-*` and `biome-*`; the real level owns its colours.

| Element | Anchor | Offset | Size |
|---|---|---|---|
| Wordmark plate | top-left | (160, 120) | ~740 x 310 |
| Rail | top-left | (136, 536) | 408 x 20 (caps to x 120-560) |
| Menu tags | top-left | (160, 600), 92 px pitch | 360 x 64 each |
| Cord knot | top-left | centre (182, 964) | 14 x 14 |
| Version stamp | bottom-left | (32, -32) | auto x 52 |
| Hint bar | bottom-right | (-32, -32) | auto x 52 |

## Copy
- "PROJECT MORDECAI" (ToUpper of "Project Mordecai"), "Everything counts. Nothing lasts."
- Tags: "Play", "Options", "Credits", "Quit". Hints: "Move", "Select". Stamp: "Dev" + version (example).
- No Tally remarks on the title screen: the tagline is the only line in the Tally's voice.

## Do / Don't
- Do keep the left column (wordmark and rail at x 160) clear of the island and the island clear of x < 980, and the cord's knot clear of the version stamp (30 px).
- Do keep the tag labels real text in sentence case; `tally-heading` only, no caps transform.
- Do thread one cord through every eyelet: behind the tags, across each face into the eyelet, ending in the knot. Strings never end in mid-air, and never run over a tag's label.
- Do put the keycap before the label on the focused tag, never after it.
- Don't draw a row of tally strokes as a rule: dividers are `.tt-divider`.
- Don't show any Lyra branding, watermark or split-screen widget, in any build.
- Don't put the version stamp in Shipping builds.
- Don't rotate the camera or put a character in the backdrop.
