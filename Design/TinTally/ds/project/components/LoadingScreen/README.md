# LoadingScreen

The opaque loading screen: a `plate` ground with the wordmark, "Loading..." beside the throbber, and one plain tip. Use it for every map and experience load; it is the existing `UMordecaiLoadingScreenWidget` (issue #39) restyled.

## Anatomy
1. **Ground**: full-screen, 100% opaque `plate`. The world renders behind it during the load, so it never takes the veil or any transparency.
2. **Wordmark**: "PROJECT MORDECAI" in `tally-display` (Rowdies 700, 80/84), one line, `text`, with a hard 5 px `ink` print, centred, top at y 432.
3. **Status row**, 44 px under the wordmark, centred: the throbber, 16 px gap, "Loading..." in `ui-body` `text-muted`.
4. **Throbber**: the Tally's throbber (`.tt-tally-throbber`): one bundle, 32 tall, 4 px strokes at a 7 px pitch, drawn stroke by stroke over its own dotted ghost in `text`. Unlit strokes stay drawn, dotted in `lip` (the core rule for an unlit stroke), so the bundle never changes size and each frame reads by shape in both themes. Frames: 1, 2, 3, 4 strokes, the slash, blank.
5. **Tip**: bottom-centre, bottom edge at y 984 (96 px margin), on a `plate-raised` strip with a 2 px `lip` border and radius 10: "Tip" in `ui-label` `text-muted` behind a lip divider, an optional keycap, one sentence in `ui-body` `text`.

The loading screen is World voice and keeps exactly two Tally pieces, the wordmark and the throbber: no tally field, no tally rule, no seal medallion, no remarks, no rivets.

## States
| State | Look |
|---|---|
| Shown | Cuts in on load start (no fade from the game: the frame is opaque). Logs "MordecaiLoadingScreen: shown". |
| Loading | The throbber runs the Throbber motion row on a loop. |
| Hold | Stays 2 s after the experience loads, as today, then the whole widget fades out. |
| No tip | Tip strip collapsed; nothing else moves. |
| Reduce Motion | The throbber keeps the same loop (it is the only proof the game has not hung, one 32 px stroke per step, never a flash); the fade-out becomes a 150 ms fade. |
| High Contrast | `plate` black, the word and lit strokes white, unlit strokes white and dotted, the tip strip black with a white lip. |

## Data and implementation
- `UMordecaiLoadingScreenWidget` (`UI/MordecaiLoadingScreenWidget.*`), set as `CommonLoadingScreenSettings.LoadingScreenWidget` in `Config/DefaultGame.ini`. It keeps building its tree in `Initialize()`.
- Keep the names `Background` (`UBorder`, brush `plate`, opaque), `TitleText` ("Project Mordecai", `TextTransformPolicy = ToUpper`) and `StatusText` ("Loading..."). Add `BindWidgetOptional` `TallyThrobber` (replaces the `UThrobber`: a `UMordecaiTallyWidget` stepping the Tally atlas frames, see 03's asset manifest), `TipBorder`, `TipKey` and `TipText`.
- Keep the log line "MordecaiLoadingScreen: shown": `playtest_boot.py`'s `loading_screen` check depends on it.
- Colours today are FLinearColor literals; replace them with the tokens: background (0.010, 0.010, 0.012) becomes `plate`, title (0.86, 0.80, 0.68) becomes `text`, status (0.55, 0.55, 0.55) becomes `text-muted`. Author sRGB hex and convert with `FLinearColor::FromSRGBColor`.
- Timing: the Throbber row of the brand book's Motion table. No local numbers.
- Tips: a DataTable `DT_LoadingTips` (row: optional key name, text); pick one at random per load and never repeat the last one. Plain voice only.
- Font sizes in UMG points (px x 0.75): wordmark 60, status 16.5, tip 16.5, label 15.
- Tokens: `plate`, `plate-raised`, `lip`, `ink`, `text`, `text-muted`; type `tally-display`, `ui-body`, `ui-label`.
- `TODO(DECISION)`: the code's open item "the loading screen's art (logo, key art, palette) has no direction yet" (#39). This page is the proposed answer; per-biome loading art is not designed.

## Copy
- "PROJECT MORDECAI" (ToUpper of "Project Mordecai"), "Loading...", "Tip".
- Example tips (true today, plain voice, one sentence each where possible):
  - [F] "Pick up items. You can carry everything you find."
  - [I] "Open your Inventory. Press I again to close it."
  - "Hold the attack for 0.4 s for a heavy attack."
  - "Low stamina weakens your actions. It never blocks them."
- No Tally remarks on the loading screen (World voice), and no tip about story, death or the Lich King.

## Do / Don't
- Do keep the ground opaque and the lockup centred; the screen must survive any resolution with ScaleToFit.
- Do keep the throbber running for the whole load and hold, Reduce Motion included.
- Don't add a tally field, a tally rule, a seal or rivets: the wordmark and the throbber are the only Tally pieces.
- Don't swing, stamp or animate anything but the throbber and the final fade.
- Don't print a progress percentage you cannot measure.
