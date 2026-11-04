Tin Tally is the interface of Project Mordecai: the front end, the in-game menus and the gameplay HUD. The world is a toy diorama. The interface is **the Tally**, a cheerful pressed-tin counting machine left running since the Automation Age. It counts everything the player does and hands over the result as tickets, plates and receipts. The pieces are few, chunky, outlined in ink and printed in flat lithograph colour. They read at a glance over a bright toy floor.

Build every screen from the components here and every value from `tokens.json`. Sizes are design px at 1920 x 1080. UMG point sizes are px x 0.75. Four sections go deeper: Voice and copy, HUD and pacing, Building it in Unreal, and Open decisions.

## The idea in one paragraph

Two sources meet here, and each does one job.
- **Dungeon Crawler Carl** supplies the spine: "Everything you do is a skill". Progress arrives as a constant, dry-witted stream: skill ranks, feats earned by deeds, identify reveals, post-fight receipts.
- **Link's Awakening (Switch)** supplies the restraint and the toy. Only a handful of pieces stay on screen. The colours are flat and saturated, the shapes rounded and thick-limbed, and the subscreen feels cosy. Menus sit over the toy diorama, never over a blank backdrop.

The machine belongs to this world's own lore: the Ancient Age built "automated systems still running without purpose", and the Tally is one of them. That makes it the game's theme in miniature: a machine that cannot stop counting is Control. On the Acceptance ending the Tally fails. The plates go dark, the rail stops, and it prints its last ticket: "Count closed."

## Principles

1. **The world is the toy; the interface is the machine laid over it.** UI never pretends to be scenery: no parchment, no wood, no glass. It is tin, enamel, ink and stamps.
2. **Count, don't clutter.** Only the gauge plate, the spell slots, the rail head, the loadout plate and the "[I] Inventory" hint are always on screen. Everything else is contextual. Progress waits until the fight is over (see HUD and pacing).
3. **Every edge holds on every floor.** Anything that sits on the world has the dual edge: a 3 px `ink` outline outside and a light `rim` or `lip` inside. One of the two always clears 3.8:1, whatever the floor colour.
4. **Shape tells, colour confirms.** Every signal has a non-colour carrier: tally strokes, notches, pips, pills, glyphs, badge silhouettes, words.
5. **Plain headline first.** The game speaks plainly. The Tally may add one dry line after it, never instead of it.

## Voice

- **Plain voice (the game):** sentence case, a verb on every button ("Identify", "Allocate", "Stay", "Quit"), digits for numbers, the game's exact vocabulary ("Spell Points", "Posture Broken", "Exhausted", "Unidentified Purple Amulet").
- **The Tally (remarks):** an Automation-Age counting engine, deadpan and literal. It is indifferent rather than cruel, and funny because it cares precisely about the wrong thing ("Seven. Up from six, as forecast."). A remark is at most 9 words and always the optional second line. Players can turn remarks off.
- **The world keeps its weight.** Death, bosses, biome cards, loading, endings and status lines speak in the World voice: no remarks, no Rowdies lettering, no tally strokes, no rivets. The loading screen keeps exactly two Tally pieces, the wordmark and the throbber.
- **A keycap leads its label:** "[F] Pick up Rusty Sword", "[I] Inventory", "[Esc] Stay", "[Enter] Identify".
- **Shipping strings are fixed.** Playtests compare them. Restyle them, never reword them: "Perfect Dodge!", "Kills: 4", "You Died", "Paused", "Identify", "[F] Pick up Rusty Sword". The full list is in Voice and copy.

## Colour

Two grounds carry everything:
- **Dark `plate`.** Text on it is `text`, `text-muted` or `text-disabled`, plus the rarity, feat, `good` and `danger-text` colours.
- **Light `enamel` tags.** Text on them is `ink` or `ink-muted`.
- **Coloured fills** (pills, bands, badges, bars) always carry `ink` text.

Never put light text on `enamel`, ink on `plate`, or any text straight onto the world or the `veil`.

- **Structure:** `plate`, `plate-raised` (rows, inactive tabs, cards), `well` (bar tracks and sockets) with its `lip`, the cream `rim` and `rim-shade` (the pressed edge), `ink` (outline and print shadow), `enamel` and `enamel-shade`, and `veil` / `veil-death` (the pause, dialog and death backdrops).
- **Resources:** `vital` (Health), `vital-trail` (the damage ghost), `heal`, and the stamina tiers `stamina-green`, `stamina-yellow` and `stamina-red`. `stamina-red` is an alias of `danger`. Also `posture` (a neutral pewter) and `spell-points`.
- **Signals:** `good` (reward gold, one meaning: worth having), `danger` (as a fill) and `danger-text` (as text), and `focus` (white, a hue no signal uses).
  - **Red means threat only.** Enemy health, the stamina Red tier, the Exhausted debt and errors are `danger`. The player's broken posture is not: it is `posture` under an `ink` hatch.
- **Rarity:** `rarity-common`, `rarity-green`, `rarity-blue`, `rarity-purple`, `rarity-red`, `rarity-gold`: six tiers. The player-facing rarity words ARE the colour words, so every rarity also prints its word and a tally badge: Common is an empty ring, Green to Red are 1 to 4 strokes, Gold is the full bundle. A gold full bundle means rarity Gold and nothing else.
- **Feat tiers:** `feat-common` (circle), `feat-rare` (flat-top hexagon) and `feat-legendary` (eight-point starburst). Feat tiers have their own hues so "Gold" never means both a Legendary feat and a Gold item.
- **Damage numbers:** `dmg-physical`, `dmg-fire`, `dmg-frost`, `dmg-lightning`, `dmg-poison`, `dmg-corrosive`, `dmg-arcane`, `dmg-shadow` with `dmg-shadow-edge` (printed inverted), and `dmg-radiant` (with a `good` halo). Every non-Physical number carries its type glyph; Physical, Slash, Pierce and Blunt numbers carry none. Heals carry "+".
- **Item compare:** `delta-up` and `delta-down`, so a compare never borrows a stamina tier's meaning.
- **Biomes:** `biome-verdant`, `biome-searing`, `biome-mire`, `biome-crystal`, `biome-storm` and `biome-withered`. They are fills for map tiles, banner mats and title-card swatches, never under text.
- **Scenery stand-ins:** the `world-*` tokens (`world-blockout`, `world-grass`, `world-sand`, `world-mire`, `world-night`, `world-snow`, their `-dot` and `-line` partners, `world-light`, `world-dark`, `world-shade`). They fake level art in previews, are the same in both themes because High Contrast never restyles the world, and are never UI colours.

These colours share a family on purpose. Each pair is kept apart by something other than colour:

| Pair | Kept apart by |
|---|---|
| `vital` and `danger` | Separate rows, and the ink stripes on the stamina Red tier |
| `rarity-red` and `danger-text` | The four-stroke badge and the word "Red" |
| `stamina-yellow` and `good` | The lemon vs gold hue, the notches and the "×1.0" pill |
| `rarity-blue` and `rarity-purple` (deuteranopia) | 2 vs 3 strokes and the word |
| `posture` broken and stamina Red | Height (8 px hairline vs 18 px bar), the `ink` hatch vs the stripes, and the "×0.75" pill |

**Themes:**
- **Diorama** is the default.
- **High Contrast** is an Accessibility setting the game ships. In it:
  - Plates are black and tags white.
  - Every text pair holds at least 7:1.
  - Outlines grow to 4 px and the focus ring turns yellow.
  - Callouts sit on plates.
  - Sunk, pressed and unlit states keep their full white outline. Sinking shows by the print offset only, never by darkening the rim.

**In Unreal:** author every colour as the sRGB hex in `tokens.json` and convert it with `FLinearColor::FromSRGBColor`. Today's literals are linear values written as if they were sRGB, so `(1, 0.5, 0)` "Orange" displays as amber.

## Type

Three families, all from Google Fonts under the OFL. Import the static TTF instances from each family's download.
- **Rowdies** is the Tally voice. Use `tally-display` for the wordmark, `tally-title` and `tally-heading` for plates, dialogs and "Paused", `callout` for combat callouts, `kicker` for ticket kickers, and `pop` for damage numbers.
- **Atkinson Hyperlegible Next** is the interface and the World voice. Use `ui-title`, `ui-body` and `ui-label` for interface text. Use `world-title` and `world-heading` (800 weight, tracked caps) for death, biomes, bosses and endings.
- **Atkinson Hyperlegible Mono** sets every number that changes: `num-xl`, `num-l`, `num-m`, `num-s`. Its digits are all 0.632 em wide by default, so counters never jitter. Unreal's Slate ignores OpenType tabular figures, so this matters. Size each number box for its longest value: "13 / 13" is 7 cells.

Type rules:
- Nothing goes below `ui-label` (20 px, which is 13.3 px at 720p).
- Caps come from TextTransformPolicy ToUpper, so the TextBlock keeps the shipped string.
- Arrows, checks, crosses and triangles are glyphs, never characters: none of the three fonts has them.

## Shape and construction

- **Plate:** `plate` fill, 2 px `rim` on the top and left and `rim-shade` on the bottom and right, a 3 px `ink` outline, `radius-plate`, and the `print-hud` shadow below (`print-menu` on menu plates).
  - The fill is always 100% opaque. A whole widget may fade its opacity to show or hide, and Reduce Motion uses those fades; a plate's fill alpha never drops below 100%.
  - Each component owns its plate's size. The spell slot is a 78 x 78 plate around a 64 x 64 well.
  - **Rivets** go only on Tally plates (TallyPlate) and front-end menu plates (the title wordmark, Credits). Never on HUD plates or World-voice plates (banners, the boss bar, death). SpellSlot's cost rivets are Spell Point data, not ornament.
- **Tag:** an `enamel` ticket with a grommet eyelet (a 12 px `well` hole in an `enamel-shade` ring and an `ink` ring), a 3 px `ink` outline and `radius-tag`. Tickets hang from the rail on a 2 px ink string, or on the light cord (a 4 px `rim` core with 1 px `ink` sides) wherever the ground behind the string can be dark.
- **Keycap:** the shape tells the device. Keyboard caps are square with `radius-key`, gamepad face buttons are round, and shoulders and triggers are pills. Letters are set in Mono 800. Mouse buttons are drawn as a mouse, never spelled.
- **Chip:** a pill for statuses and states.
  - Buff chips are light enamel with a raised notch on top.
  - Debuff chips are dark plate with a pinked bottom edge and a `debuff` rim.
- **Bar:** a sunk `well` with a 2 px `lip`, a fill, a trail and notches.
  - Patterns carry meaning: solid, striped (stamina Red), and the debt hatch (stamina Exhausted, below 0).
  - **The debt rule:** the hatch is anchored LEFT and its width is |Stamina| / MaxStamina; regen eats it from its right edge, then the fill grows from the same left edge. One scale for debt and fill. The floor is an absolute −50, so the deepest debt is 37% of the bar at the default MaxStamina of 134.
  - Over the world, add the ink outline.
- **Pips:** a short row of 14 px round marks (20 px for boss phases) for small counts that are not ranks or rarity: boss phases and status stacks. A reached pip is a filled disc, one still ahead is a hollow `lip` ring, so they differ by shape.
- **Rail:** one family for every rail (`.tt-railbar`): a teal `plate` bar with a `rim` top edge, the ink outline and the print, and an end cap at each end holding an eyelet. The title-screen rail, the Mage Tower rod and the spike's base plate are all this piece at different widths.
- **Dashed means unavailable:** a dashed edge marks locked, disabled, unbound and "Coming Soon" pieces, and nothing else. A summoned enemy is marked with the `summon` glyph and a lighter fill, never a dashed rim.
- **Shadows:** hard print offsets only. Never blur, glow, gradients, glass or HUD transparency. A veil darkens the world behind a menu; it never blurs it.

## Interaction states

Every interactive piece draws these states the same way:

| State | Look |
|---|---|
| Idle | The piece at rest. |
| Hover (mouse only) | Lifts 2 px; the print grows to 6 px. |
| Press | Sinks 3 px; the print collapses to 1 px. In High Contrast the white outline stays whole. |
| Focus | Ink, focus, ink: a 3 px `focus` ring right outside the piece's own `ink` outline, and a 2 px `ink` keyline outside the ring, so focus reads on dark plates and on the lightest floors alike. Pieces whose ink is their border (keycaps, folder tabs) take the ring at offset 0 (`.tt-focus--border`). Unoutlined pieces inside a plate (rows, cells, inset cards) get an ink ring in the gap and no keyline (`.tt-focus--fill-gap`). Buttons have it built in. Focus never removes the outline or the print. |
| Selected | A cream `enamel` fill with `ink` text (`.tt-selectable.is-selected`): tabs, filter chips, skill categories, plate cards. One look everywhere but one: enamel tickets in a list (the Mage Tower's sealed tags, the ServiceScreen instance whose items are tickets) are enamel already, so they select by sliding 12 px right, not by fill. |
| Selected + focused | The enamel fill with the focus ring on top. |
| Disabled | `text-disabled`, a dashed edge and the `lock` glyph. |

- Tab-state sheets show Idle, Hover, Press, Focus, Selected and Selected + focused.
- **Buttons in a row:** the risky action on the left, the safe action on the right, default focus on the safe action, and Back always picks it. Quit is a plain button. `danger` buttons are only for destructive actions such as deleting a save.
- **Empty states** use one piece: `.tt-empty`, a plain `enamel` tag (no eyelet, no radio dot) with `ink` text, centred in its content box: "Inventory is empty.", "No items match this filter.", "No resources stored.", "Nothing to identify." (example copy), "Coming Soon". Never put a radio dot on text that cannot be pressed.
- **Hint bars** use one piece: `.tt-hintbar`, its own plate holding keycap + label pairs, the keycap first, split by 2 px `lip` dividers; 8 px from key to label, 16 px around a divider. Never on the veil or the world. Its place on screen can vary; its look cannot.

## Shared primitives

The core classes every component builds on (`css/_core.css`; component CSS reuses them and never restyles them globally):

| Class | What it is |
|---|---|
| `.tt-plate` (`--menu`, `--inset`) | The plate, the menu plate (6 px print) and the inset card inside a plate |
| `.tt-tag` (`--plain`), `.tt-string`, `.tt-string--light` | The eyelet tag, the plain tag, the 2 px ink string and the light cord |
| `.tt-key` (`--pad`, `--shoulder`, `--wide`, `--sm`) | Keycaps |
| `.tt-btn` (`--primary`, `--danger`, `--block`) | Buttons |
| `.tt-bar` with `__fill`, `__trail`, `__lead`, `__notch`, `__debt` | Meters, including the debt rule |
| `.tt-pill`, `.tt-chip` | Multiplier and state pills; status chips |
| `.tt-glyph`, `.tt-tally`, `.tt-rarity`, `.tt-socket` | Glyphs, tally strokes, rarity badges and the ink socket that holds a badge on enamel |
| `.tt-hatch` | The one warning hatch (`danger` and `ink` at −45°) |
| `.tt-outlined`, `.tt-outlined-4` | Ink-outlined lettering for world-space text |
| `.tt-divider` (`--enamel`, `--v`) | A 2 px `rim` rule at 50%: the only divider, on every screen, never a row of tally strokes |
| `.tt-empty` (`__sub`), `.tt-empty-box` | The one empty state, and the box that centres it |
| `.tt-drawback` (`__label`, `__text`, `--enamel`) | The one Drawback box: a hatch strip, then the inline label "Drawback" and the text. TallyPlate, FeatsTab and the spike |
| `.tt-hintbar` (`__sep`) | The one hint bar |
| `.tt-pips` (`--lg`, `--enamel`) with `.tt-pip` / `.is-on` | Pips for boss phases and status stacks |
| `.tt-railbar` | The one rail: a bar with eyelet end caps |
| `.tt-selectable` | Opts a piece into the Selected look (`.is-selected`) |
| `.tt-focus--border`, `.tt-focus--fill-gap` | The two focus variants (Interaction states) |
| `.tt-specband` (`__item`, `__title`) | Preview only: the band of spec notes under a screen |
| `.tt-diorama` (`--blockout`, `--sand`, `--mire`, `--snow`, `--night`) and its props (`__tree`, `__bush`, `__rock`, `__crate`, `__post`, `__pond`, `__path`, `__patch`) | Preview only: the toy world behind pause, death and options veils |
| `.is-focused`, `.is-selected`, `.is-hover`, `.is-pressed`, `.is-unlit` | Static state classes for previews |

The shared values behind them are custom properties: `--tt-outline` (3 px, 4 in High Contrast), `--tt-print`, `--tt-hatch`, `--tt-focus-keyline` and `--tt-string-light-*`. Copies in a component's pseudo-elements read these, so they never drift.

## Iconography

Stamp glyphs:
- A 24 x 24 grid, one colour, chunky and rounded, with no detail thinner than 2 units.
- Each status, damage type and weapon has a silhouette unlike its siblings at 20 px.
- In Unreal they are single-ink images tinted by the brush, so High Contrast can flatten them.
- No hearts, rupees, gems or scrolls.
- The set is in the Glyphs card. It includes `bandage`, `summon`, `display`, `audio`, `controls`, `accessibility`, `search`, `speech`, `sprint`, `reticle` and `tonic`; use these names rather than borrowing a near neighbour.
- A feat medallion carries the feat's own glyph everywhere (Pyromaniac: `fire`), on the plate, the Feats tab and the spike.

The tally mark (four strokes and a slash) is the system's one motif. It has exactly three jobs:
- **skill ranks:** 20 strokes, with the slashes on milestones 5, 10, 15 and 20. Lit strokes are `text` on plate and `ink` on enamel, never `good`. Rank 20 is a full bundle in `text`;
- **rarity badges;**
- **the seal and the throbber.** The throbber runs on the loading screen only.

Everything else that counts uses another mark:

| Count | Mark |
|---|---|
| Light-attack combo | "Hit N" alone (CombatCallout). No strokes under the weapon name (LoadoutPlate). |
| Boss phases, status stacks | Pips (`.tt-pips`) |
| Feat conditions | A thin bar with a count ("6 / 10") |
| Options ranges | A notched track |
| Title and Credits dividers | `.tt-divider` |
| Loading | The wordmark and the throbber only: no tally field, no tally rule, no seal medallion |
| Credits printing | "Printing..." with no throbber |
| The rail's pending badge | The number only ("+3") |
| World map charted cells | Numerals |
| Spell Points | Tokens, never strokes |
| Kills, enemy tier, feat tier | A number, gold diamonds, badge silhouettes |

## Motion

Motion is wound, not floaty: things hang on strings, drop, swing once and settle. Stamps strike, keys press, and tickets retract into the slot they came from.
- During combat, animations last at most 160 ms in. Celebrations wait for the receipt.
- Only the loading throbber and the 1 Hz low-health pulse loop.
- Reduce Motion turns swings, drops, stamps and shakes into 150 ms fades. The throbber keeps looping. Nothing flashes faster than 3 Hz.

Each motion is defined once, here. Components cite it by name and do not restate its numbers.

| Motion | Timing | Used by |
|---|---|---|
| TicketDrop | One swing about the eyelet over 350 ms: +8° to 0 with a −1.5° overshoot (keys +8°, −1.5° at 175 ms, +0.5° at 280 ms, 0°); opacity 0 to 1 over the first 80 ms. The pieces below reflow down in 160 ms, ease-out. | TallyRail tickets, Tag, the plate's remark tag, front-end tags |
| TicketRetract | 180 ms ease-in, 28 px up into the slot, fading over the last 100 ms; the rest reflow up in 160 ms. | TallyRail |
| DamageTrail | The ghost holds the old value 350 ms, then drains to the fill over 450 ms, ease-in-out. A hit during the hold restarts the hold from where the ghost is. | GaugePlate, Bar, EnemyTag, BossBar |
| HealLead | The `heal` lead snaps to the new value; the fill follows over 400 ms, ease-out; the lead hides when the fill meets it. | GaugePlate (player Health only) |
| PlateDrop | In: a 24 px drop with a fade, 220 ms ease-out back. Out: a 200 ms fade with a 12 px rise. | TallyPlate |
| StampReveal | Scale 1.7 to 0.92 over 110 ms (ease-in quad), then to 1.0 over 120 ms (ease-out back), with a 60 ms, 3 px shake of what it lands on: 230 ms. | TallyPlate badge (120 ms into PlateDrop), MageTower identify, WorldMap gates |
| TypeOn | A mask slides off the final text at 18 ms per character, capped at 450 ms, on an overlay layer. | MageTower, LoadoutPlate |
| CalloutPop | Scale 0.6 to 1.12 to 1.0 over 160 ms (ease-out back), hold 640 ms, fade 200 ms: 1.0 s. | CombatCallout |
| Shake | 3 cycles, 6 px sideways, 180 ms, with the negative thunk. | SpellSlot refused press, "Parry Failed!" |
| KeyPress | 60 ms down 3 px, 120 ms back. | Keycap and every keycap on the HUD |
| TabSwitch | 90 ms out, 160 ms in, 12 px. | PauseShell tabs, OptionsScreen sections |
| BannerFade | In: a 240 ms fade with a 16 px drop, ease-out cubic. Out: a 400 ms fade. | Banner |
| LowHealthPulse | The gauge plate's rim steps between `rim` and `vital` every 500 ms (1 Hz) at 25% Health or below. Reduce Motion holds it at `vital`. | GaugePlate |
| Throbber | A stroke every 160 ms, the slash holds 400 ms, then 160 ms blank: a 1200 ms loop. Drawn in `text`. It keeps looping under Reduce Motion, because it proves the game has not hung. | LoadingScreen |
| DamagePop | In: scale 0.6 to 1.15 to 1.0 over 120 ms (ease-out back); a crit 0.4 to 1.25 to 1.0 over 160 ms while its burst turns from −8° to 0°. Then it rises 100 cm/s in world space (scatter 30 cm; pops on one target alternate sides and drift outward at 60 cm/s) and fades over the last 300 ms of its 1.5 s. Reduce Motion: no overshoot, burst turn or drift; a 150 ms fade in, then the plain rise. | DamageNumber |
| PromptSlide | In: the tag slides 12 px left out of the hint pill and fades in over 120 ms, ease-out. Out: it collapses back toward the pill in 100 ms. A retarget keeps the right end and eases the width to the new length over 90 ms. | InteractPrompt |
| LabelLift | Focus: the tag lifts 10 px over 120 ms (ease-out back) as its string lengthens, and the keycap fades in over 80 ms. Losing focus reverses it in 100 ms. Reduce Motion: no lift; a 150 ms fade swaps the focused look. | PickupLabel |
| WorldTagFade | A whole-widget fade: in over 120 ms on first aggro or damage, out over 200 ms at the leash. | EnemyTag |
| BracketSnap | The critical brackets snap in: scale 1.15 to 1.0 over 120 ms. | EnemyTag |
| BossOut | The emptied bar holds 600 ms, then the whole unit fades out over 300 ms (a whole-widget fade; the plate fill stays opaque). | BossBar |
| ThreatPipFade | A threat pip fades in over 120 ms, then tracks continuously. It never pulses; Reduce Motion leaves it as is. | WorldMarkers |
| ReticleCancel | The ground-target reticle shrinks to 55% and fades out in 150 ms. | WorldMarkers |
| ReadyGlint | One 140 ms sweep when a cooldown ends: two `rim` bands cross the well while its lip turns `rim`, then settle. Reduce Motion: no sweep; the lip fades to `rim` over 150 ms. | SpellSlot |

Reveals and typing run on an overlay layer. The real TextBlock is set once, to the final string.

## Layout

- **HUD zoning:**
  - **Left is you:** the gauge plate, spell slots and status chips.
  - **Top right is news:** the rail head and its tickets.
  - **The centre is now:** callouts and the combo counter.
  - **Bottom centre is interaction:** the prompt and the Inventory hint.
  - **Bottom right is the loadout:** the weapon and quick items.
  - **Top centre is for big moments:** plates, the boss bar and banners.
- The edge margin is 32 px, or 96 px with the HUD margin option. The full map is in HUD and pacing.
- **The HUDLayout card** draws the whole HUD at 1920 x 1080 from the real components: a fight (the combat hold, the "+N" badge, callouts, enemy tags and damage numbers), the receipt after it (tickets and a plate), a boss fight (the boss bar up and the plate dropped to (0, 168)), and the map with every box as a guide. Check a new HUD piece against it before it ships.
- **The HUD is one widget.** Its root is pushed once on `UI.Layer.Game`; every HUD region is a child of it. Menus and dialogs draw over it on their own layers, and the death screen covers it (see Building it in Unreal).
- **Menus** live in one shell centred on the `veil`: the "Paused" nameplate hung on two strings, a 1000 x 640 column under it (a 56 px bar of five folder tabs over the 1000 x 584 content plate, content box 948 x 540), a hint bar below, and the Tally spike on the right. The spike shows the same ticket kinds as the rail, compressed to one line.
- **Menus sit over the toy diorama.** The live world shows through the veil, darkened, never blurred.
- **One device per screen.** Keyboard and mouse is the default: the pause hint bar reads "Esc Back | I Close" and the tab bar has no bumpers at its ends. The gamepad version is described beside the screen, not mixed into it.
- **The front end** hangs its choices as tags from a rail over a diorama vignette.

## Accessibility

- **High Contrast theme:** every text pair holds at least 7:1, outlines are 4 px, the focus ring is yellow, and pressed and unlit states keep their whole outline.
- **Focus:** every control is reachable by keyboard and gamepad and shows the ink | focus | ink ring (Interaction states). The ring carries focus on plates and dark floors; the ink keyline carries it on blockout, sand and snow, where white alone is 1.1-1.3:1.
- **Colour is never the only signal:** tally badges, notches, pips, pills, patterns, glyphs, badge silhouettes and words always carry it too.
- **Settings:** the OptionsScreen card's Accessibility table is the single list of names, values and defaults. In short:
  - High Contrast Off / On; Reduce motion Off / On.
  - Tally remarks On / Off; Tally notifications Full / Calm / Minimal; Ticket duration ×1 / ×1.5 / ×2 (base: slip 3 s, ticket 5 s).
  - Callout size Normal / Large / XL (40 / 48 / 56 px); Damage numbers All / Mine / Off.
  - UI scale 80-150%; HUD margin 32 / 96 px.
- **Text stays real UTextBlocks.** The playtest bridge and future screen narration read them.

## Previews

These rules are for the cards in this system, not the game.
- Spec notes sit BELOW the screen, in a band under a rule (`.tt-specband`), in plain `tt-label` text. Never inside a 1920 x 1080 frame, and never in chip or pill shapes, which read as game UI.
- The HUD drawn behind a pause veil or the death screen is the real HUD, built from the HUDLayout card's markup and classes, never a stand-in.
- Pause, death and options veils sit over a diorama stand-in (`.tt-diorama`: floor tiles and a few blocky props in the `world-*` and `biome-*` tokens), never over a flat grid, and never blurred.
- Each screen is drawn in one device; the other device goes in the spec band.
- Invented data is labelled "example" in the card's README. Systems that do not exist yet carry `TODO(DECISION)`.
