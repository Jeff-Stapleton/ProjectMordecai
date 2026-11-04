# Banner

The World-voice title card at top centre: a 928 x 132 inlaid plaque that names a biome, a boss or a new area, or announces a boss phase, with no Tally voice at all. Use it when the player crosses into a new place, when a boss fight begins and when a boss changes phase; everything about progress belongs to the rail and the plate instead.

## Anatomy

| Part | Spec |
|---|---|
| Slot | 928 x 132, anchor top-centre (0.5, 0), offset (0, 296), alignment (0.5, 0). Clear of the boss bar (y 32-128) and of a plate at (0, 40) (y 40-168). A plate under a boss bar (y 168-296, or 168-352 with a Drawback box) would reach into this box, so plates wait until the banner has gone. |
| Plate | `.tt-plate`: `plate` fill, `rim` / `rim-shade` 2 px, radius 12, 3 px `ink` outline (4 in High Contrast), print (0, 4). Padding 8. No rivets: this is a World-voice plate. The fill is 100% opaque; the whole widget fades for show and hide. |
| Mat | The coloured inlay that carries the place: fill = the biome swatch (`biome-*`) or `foe` for a boss, radius 8. Its edge is doubled: a 2 px `lip` ring outside (3.45:1 on `plate`) and a 2 px `ink` line inside. The ring is what separates the dark swatches from the tin: `biome-mire` is only 2.3:1 on `plate` and `biome-withered` 2.8:1. 56 px wide at the ends and 10 px on the rails (new area and phase: 20 / 5). Fill only: text never sits on it. |
| Panel | The dark inner panel that holds the words: `plate`, radius 4, a 2 px `ink` edge and a 2 px top highlight (`rim` at 25% mixed into `plate`, opaque). Inside: 796 x 92 on the full card, padding 0 32, content centred, 6 px gap. |
| Title | `world-heading` (Next 800, 40/48, tracked 0.05 em, ToUpper), `text`, one line. The longest names fit: "TEMPEST ASCENDANT" is 472 px of the 732 available. |
| Line | Optional, `ui-body` (Next 500, 22/30), `text-muted`, one line, ellipsis. Only the boss intro (its `IntroLine`) and the new area (the biome it belongs to, after a 16 px swatch chip with a 2 px `ink` ring and a 2 px `lip` ring) carry one. Biome cards and phase cards carry none. |
| Phase card | A smaller piece for a smaller beat: 560 x 96, top-centred in the slot (y 296-392), the slim mat in `foe`, the title alone ("PHASE 2", "ENRAGED"; "ENRAGED" is 204 px). Inside the panel: 500 x 66. |

Biome mats (tokens; identical in both themes):

| Biome | Token | On `plate` (Diorama) | Against its `ink` line |
|---|---|---|---|
| Verdant Wilds | `biome-verdant` | 4.2:1 | 5.9:1 |
| Searing Expanse | `biome-searing` | 5.4:1 | 7.7:1 |
| Mire of Echoes | `biome-mire` | 2.3:1: the `lip` ring carries the edge | 3.3:1 |
| Crystal Caverns | `biome-crystal` | 6.1:1 | 8.6:1 |
| Storm Peaks | `biome-storm` | 4.3:1 | 6.2:1 |
| Withered Kingdom | `biome-withered` | 2.8:1: the `lip` ring carries the edge | 4.0:1 |

So every mat edge reads twice: `lip` against `plate` (3.45:1) and `ink` against the swatch (3.3:1 or more).

## States

| State | Trigger | What shows | Time |
|---|---|---|---|
| Biome title card | First entry into a biome | Biome mat and the biome name alone. | 3 s |
| Boss intro | The boss encounter starts | `foe` mat (the boss bar's red), the boss name, its `IntroLine`. The boss bar drops in at (0, 32) when the intro has gone (BossBar). | 3.5 s |
| Boss phase | The boss enters phase 2 | The 560 x 96 phase card, slim `foe` mat, "PHASE 2". Under the boss bar, which stays up and counts the phases with its pips. | 1.5 s |
| Boss phase, enraged | The boss enters phase 3 (enrage) | The phase card with "ENRAGED" instead of the number, matching the boss bar's Enraged chip. | 2 s |
| New area | First entry into a named area or dungeon | A slim mat in the biome's swatch, the area name, and the biome with its chip. | 2.5 s |
| Revisit | Entering a place already seen | Nothing. TODO(DECISION): a revisit card, if any. |  |
| Queued | Two banners at once (a new area inside a new biome) | The biome card first, then the area card. A waiting TallyPlate holds until both have gone. |  |

The Banner is its own lane: the combat hold does not delay it (a boss intro starts the fight; a phase card lands mid-fight), callouts may show over the world while it is up, and plates wait for it. It carries no Tally remark in any state, and the Tally notifications setting does not touch it.

Motion (cited by name from the brand book's Motion table): BannerFade in and out. No swing, stamp, shake or print tick: this is not the Tally. Reduce Motion: BannerFade without the drop. Sound: its own low cue (audio's call), never the ticket or plate sounds.

## Data and implementation

- **Where it lives:** `UMordecaiBannerWidget` (proposed), bound as `Banner` (BindWidgetOptional, with a C++ fallback like the rest of the HUD), is a child of the HUD root canvas (`UMordecaiCombatHUDWidget`, reparented to `ULyraActivatableWidget` and pushed ONCE on `UI.Layer.Game`). Never push it on its own: a second push on the layer stack hides the HUD. One instance; banners queue.
- **Its own BindWidgetOptional names:** `PlateBorder`, `MatImage` (tinted; the `lip` ring and `ink` line are in the brush), `PanelBorder`, `TitleText`, `LineText`, `SwatchChip`. The phase card is the same widget at 560 x 96 (a size override on the root SizeBox), with `LineText` and `SwatchChip` Collapsed.
- **Assets (03, the asset manifest):** the **Plate stack** for the plate; the mat and panel are two more RoundedBox layers in the same Overlay (mat: radius 8, tinted per biome; panel: radius 4, `plate`), with the `lip` ring and the `ink` lines as RoundedBox outlines. No textures.
- **Swatch:** the biome-to-token map lives in the UI theme DataAsset next to the other tokens. Author sRGB hex and convert with `FLinearColor::FromSRGBColor`.
- **Title text** is stored in title case ("Mire of Echoes", "Iron Warden", "Phase 2", "Enraged") and cased with TextTransformPolicy ToUpper, so GetText keeps the data string. Boss titles use `EnemyDisplayName`, never the actor name.
- **TODO(DECISION):** no biome cells or area names exist yet (only DevTestMap, MovementTestMap, CombatTestMap), and nothing marks "entered biome" or "entered area". Proposal: a biome / area id on the cell or level data and a trigger volume that asks the banner queue once per save.
- **TODO(DECISION):** no boss exists yet. The intro line is one string in the boss data, the proposed `IntroLine` field, used as is wherever the boss is introduced.
- **TODO(DECISION):** there is no boss phase system in code (BossBar keeps the same open item). The phase card needs the phase source (for example a `Mordecai.Boss.Phase.2` / `.3` tag or a replicated int on the boss) and an event when it changes. `enemy_ai_framework_v1` defines phase 1 basic attacks, phase 2 AoE attacks unlocked and phase 3 enrage, so the cards are "Phase 2" and "Enraged"; a boss with other phases would need a `PhaseName` per phase in its data.
- **Sizes in UMG points (px x 0.75):** title 30, line 16.5.
- **Tokens:** `plate`, `rim`, `rim-shade`, `ink`, `lip`, `text`, `text-muted`, `foe`, `biome-verdant`, `biome-searing`, `biome-mire`, `biome-crystal`, `biome-storm`, `biome-withered`, `world-heading`, `ui-body`, `radius-plate`, `stroke-rim`.

## Copy

- Biome names, exact, and alone on the biome card: "Verdant Wilds", "Searing Expanse", "Mire of Echoes", "Crystal Caverns", "Storm Peaks", "Withered Kingdom". The world bible's theme lines ("Bury it", ...) and arc words (Denial, Control, Avoidance ...) are design notes, not UI copy.
- Boss names, exact: "Thorn Guardian", "Iron Warden", "Drowned King", "Crystal Seer", "Tempest Ascendant", "The Lich King".
- Boss `IntroLine` (data; one plain description of how the boss fights, in short sentences; every line here is example copy): Iron Warden "Enforces strict patterns. Punishes deviation." It is one string, the same in every card that introduces the boss. Examples for the rest, from the world design: "Regenerates. Raises barriers and overgrowth traps." (Thorn Guardian), "Attacks from hiding. Phases in water. Damage lands late." (Drowned King), "Repeats its attacks. Freezes zones. Leaves delayed echoes." (Crystal Seer), "Fights from the air. Never stops moving." (Tempest Ascendant), "Uses your abilities. Controls time and space." (The Lich King).
- Phase cards: "Phase 2", "Enraged" (shown in caps).
- "Sunken Causeway" in the preview is an example area name.
- No Tally remarks, no jokes and no exclamation marks.

## Do / Don't

- Do keep the words on the dark panel; the mat is colour only.
- Do keep the `lip` ring on every mat, so the dark swatches never melt into the plate.
- Do use `foe` for every boss intro and phase card, whatever the biome, so it reads as threat and matches the boss bar.
- Do show a biome card once, on first entry, with the name alone.
- Do keep the phase card small (560 x 96) and short: it lands mid-fight.
- Don't add rivets, tally strokes, pips, glyphs, a seal or any Tally lettering (Rowdies) to a banner.
- Don't put a banner and a plate on screen together.
- Don't write second lines for biome or phase cards.
