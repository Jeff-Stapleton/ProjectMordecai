# DeathScreen

The death overlay: one World-voice plate in the family's `foe` inlay mat, holding "You Died", the respawn countdown in a seal ring and the kill count, over the `veil-death` backdrop and the still-running arena. Use it while `Mordecai.State.Dead` is on the player; as the top of the `UI.Layer.Game` stack it hides the HUD, and it carries no Tally remark and no playful ornament.

## Anatomy (1920x1080)
- **World**: the arena keeps running (death does not pause; only the pause menu does). The preview draws it as the toy diorama (`.tt-diorama`), never a flat grid, never blurred.
- **Veil**: full screen, `veil-death` (lighter than the pause `veil`, so the arena stays visible). HitTestInvisible. No text sits on it.
- **Plate**: a menu plate (`plate`, `rim` / `rim-shade`, 3 px `ink` outline, menu print), 600 wide, centred on (960, 540), padding 8.
- **Mat**: Banner's inlay mat (`.tt-banner__mat`) in `foe`: 36 px ends, 14 px rails, a 2 px `lip` ring outside and a 2 px `ink` line inside. The same frame as the boss intro banner, so death reads as part of the World family. Text never sits on the mat.
- **Panel**: Banner's inner panel (`.tt-banner__panel`): `plate` with a 2 px `ink` ring, padding 28 / 28 / 32, contents centred. Every word is on it.
- **DeathText** "You Died" in `world-title` (Next 800, 72/80, 0.06em tracking), cased upper with ToUpper, `danger-text` (6.1:1 on `plate`).
- **Rule**: core `.tt-divider` (2 px `rim` at 50%), 16 px under the title, 26 px above the ring.
- **Countdown ring** (132 px, the seal treatment):
  - Outside: the 3 px `ink` outline (4 in High Contrast) and the HUD print.
  - A 2 px `lip` edge, then a 7 px progress band: `rim` for the time left, `well` for the time spent. It starts full at 12 o'clock and its end retreats anticlockwise to 12 o'clock over `RespawnDelaySeconds`.
  - A 2 px `ink` ring, the `well` disc, and a 2 px dashed `lip` ring 18 px in (the seal's dashed inner ring).
  - `CountdownText` in `num-xl` (56/60), `text`, centred in a 72 px box.
- **KillCountText** "Kills: 4" in `num-l`, `text`, centred in a 190 px box (the longest value, "Kills: 999", is 10 Mono cells at 19 px), 22 px under the ring. It is the last line: no second line, no remark, no tip.

## States
- **Enter**: the plate comes in with BannerFade (in), the World voice's motion (the Banner's), never PlateDrop's bounce; the veil fades in over the same time. Reduce Motion: BannerFade without the drop.
- **Countdown 3, 2, 1**: CountdownText shows the ceiling of the remaining time; the ring stands at remaining / `RespawnDelaySeconds` (the preview's "1" frame is at 0.25).
- **Paused while dead**: the countdown and the ring hold; the pause menu (`UI.Layer.Menu`) draws its veil over the plate.
- **Leave** (CountdownText empty, then `Mordecai.State.Dead` removed): the plate goes out with BannerFade (out) and the veil clears with it, then the widget is removed from the stack.
- **Respawned**: the HUD is the top of `UI.Layer.Game` again. It re-reads its values in `NativeOnActivated`: full Health, Green stamina, Spell Points full, and "Kills: 0" (the counter resets on `Mordecai.Event.PlayerDeath`).
- The preview shows countdown 3 at full size, then Enter, Countdown 1 and Leave as 640 x 520 crops at 1:1 (no letter shrinks), then the respawned screen with the real HUD (HUDLayout's markup and classes).

## Data and implementation
- **Widget**: `UMordecaiDeathScreenWidget` (`ULyraActivatableWidget`, InputConfig GameAndMenu, `CapturePermanently_IncludingInitialMouseDown`), pushed on `UI.Layer.Game` by `UMordecaiDeathScreenComponent` when `Mordecai.State.Dead` is added, removed when it goes. `InitializeDeathScreen(RespawnDelay, KillCount)` feeds it; `RespawnDelaySeconds` is 3.0 on `AMordecaiGameMode`.
  - Keep `DeathText`, `CountdownText` and `KillCountText`. Rename nothing: playtests compare "You Died" and "Kills: N", and the agent bridge reads these texts in the widget's own tree.
- **Getting a designer tree**: the component pushes `UMordecaiDeathScreenWidget::StaticClass()`, which has no designer tree, so `BuildDefaultContent` builds today's dim and three TextBlocks in C++. To ship this look, add a `TSubclassOf<UMordecaiDeathScreenWidget> DeathScreenClass` on `UMordecaiDeathScreenComponent` (proposed; default the native class) and point it at a `W_DeathScreen` Widget Blueprint whose tree keeps the three names. A class with a tree skips `BuildDefaultContent`, so the playtests keep working.
- **HUD hiding**: the HUD root is pushed once on `UI.Layer.Game`, and the death screen is the only other widget pushed onto that stack. A stack shows only its top widget, so the HUD (and the "[I] Inventory" hint) disappear while the player is dead and return when the death screen pops (Building it in Unreal, "The HUD root and its layer"). Until the HUD moves off `AddToViewport`, today's HUD draws over the dim.
- **Root**: CanvasPanel, HitTestInvisible.
  - `BackgroundDim` becomes the `veil-death` Border (one colour from the theme asset, not a hand-mixed alpha).
  - `MessageColumn` becomes the plate: the plate stack (03, a RoundedBox Overlay), the mat Border (`foe` fill, `lip` and `ink` edges, Banner's build), and the panel Border holding a VerticalBox with the title, the rule, the ring Overlay and `KillCountText`.
  - New, optional: `CountdownRing`, a UImage with a radial-progress material driven by remaining / `RespawnDelaySeconds` (`rim` over `well`), stacked with the static seal rings (outline, lip, ink ring, dashed ring).
- **Text**
  - CountdownText keeps its format: the ceiling integer, empty at or below 0.
  - KillCountText keeps "Kills: %d".
  - Every live number is Mono, in a fixed box (SetAutoSize false): CountdownText 72, KillCountText 190.
- **Input**: the screen takes no input and shows no hint bar, so it looks the same on keyboard and gamepad.
- **Sizes**: plate 600 x 420 (measured), centred on (960, 540); ring 132; title 72 px. At 720p (0.666x) the title renders at 48 px and the countdown at 37 px.
- **Tokens**:
  - Colour: `veil-death`, `ink`, `plate`, `rim`, `rim-shade`, `lip`, `well`, `foe`, `text`, `danger-text`.
  - Type: `world-title`, `num-xl`, `num-l`; radius `radius-plate`.
  - Motion: BannerFade (the brand book's row; no local timings).
- **Remote clients** get no death screen today (the tag does not replicate). TODO(DECISION) with multiplayer.
- **Campaign death** (what is lost, where the player respawns, whether a button is needed) is undefined; this screen covers the arena flow only. TODO(DECISION).

## Copy
- Shipping, exact: "You Died" (shown as YOU DIED through ToUpper), "Kills: 4", and the countdown digits "3", "2", "1". Nothing else is written on this screen.
- Tone-bleed rule: no remark, no second line, no tally strokes, no rivets, no eyelets on this screen. The mat, the plate and the seal ring are the family's construction, not its voice. The Tally stays silent on death.

## Do / Don't
- Do keep the plate calm: one headline, one number, one count.
- Do use `veil-death`, not the pause `veil` and not a local alpha.
- Do hide the HUD (and the "[I] Inventory" hint) under the death screen, by stack, not by hand.
- Do put every word on the panel; nothing sits on the veil or the mat.
- Do keep the seal ring's outline and dashed ring in High Contrast; only the band moves.
- Don't add a Tally remark, a second line, a receipt or a ticket here; wait until the player is back.
- Don't flash or pulse the headline. After BannerFade in, the only motion is the ring draining.
- No respawn button in the arena flow; the campaign variant is TODO(DECISION).
