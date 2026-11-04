# Tin Tally — build contract for component authors

You are building part of **Tin Tally**, the UI design system for Project Mordecai (UE 5.7 diorama ARPG on Lyra; fixed camera; twin-stick; a mashup of Dungeon Crawler Carl and the Switch remake of Link's Awakening). It is published as a Design System artifact: each component is a folder `project/components/<Comp>/` holding `preview.html` (a live, static HTML rendition) and `README.md` (guidelines an Unreal implementer and later agents follow). Everything here is design-spec material for a UMG/CommonUI game, rendered in HTML for review. This file is the contract: where a component, a brand-book section or an older direction disagrees with it, this file wins and the other file is the one to fix. Decisions are tagged (D1)-(D27) so a reviewer can trace them.

Paths (relative to `Design/TinTally/` in the game repo; see `Design/TinTally/README.md` for the workflow):
- `ds/project/tokens.json` — THE colour, type, spacing, radius, shadow tokens. Read it. Never invent hex values in component CSS. Every token carries a usage note.
- `ds/css/_core.css` — the shared primitives (read it fully before writing anything). Owned by the core owner: component authors do NOT edit it; ask for a primitive instead of hand-rolling one.
- `ds/css/<Comp>.css` — YOUR component CSS, one file per component you own. It is concatenated into `components/bundle.css` after `_core.css`.
- `ds/js/core.js` — `window.TinTally` helpers (glyphs, tally strokes, rarity badges). Owned by the core owner.
- `ds/glyphs/*.json` — the icon set (`{name: "<svg inner markup on a 24x24 grid>"}`), owned by the icon authors.
- `ds/project/README.md` and `01-voice-and-copy.md`, `02-hud-and-pacing.md`, `03-building-in-unreal.md`, `04-open-decisions.md` — the brand book. `03` is the canonical asset manifest (D21).
- `research/BRIEF.md` + `research/read_*.md` — the game facts (systems, exact strings, numbers) as gathered on 2026-10-31. The repo (`Specs/`, `Source/LyraGame/Mordecai/`, `Config/DefaultInput.ini`) is the truth for numbers, names and bindings.
- Generated previews: PauseShell, CharacterTab, SkillsTab and FeatsTab come from `pause_gen/`; InventoryTab, MageTower, WorldMap, SpellCard, ReceiptTimeline, WorldMarkers, Dialog and OptionsScreen from `generators/`. Edit the generator and re-run it, never the HTML.
- Render: `cd tools` then `py -I render.py render <Comp>[,<Comp>]` writes `ds/build/shots/<Comp>.diorama.png` and `<Comp>.high-contrast.png`. Read BOTH PNGs and LOOK. `py -I render.py check` lists static problems (marker line, unknown tt- classes, unknown glyph names, missing README). Keep throwaway helper scripts out of `ds/`; never overwrite or delete another agent's files or temp folders.

## The idea (keep every component inside it)
The interface is **the Tally**: a cheerful pressed-tin counting machine left running since the Automation Age (the game's lore: "automated systems still running without purpose"). It counts everything you do (pillar: "Everything you do is a skill") and hands you the result as **tickets** hung from a **rail** at top right, **plates** for big moments, and a **receipt** after each fight. Dungeon Crawler Carl supplies that relentless, dry-witted progress stream. Link's Awakening (Switch) supplies the restraint and the toy: few chunky rounded pieces on screen, flat lithograph colour behind a heavy ink outline, a cosy toy-box subscreen over a toy diorama. On the Acceptance ending the Tally fails: plates go dark, the rail stops, and it prints its last ticket, "Count closed."

## Construction rules
1. **Dual edge.** Anything that sits on the world (HUD plates, tags, keycaps, chips, bars over enemies, callouts, damage numbers) has the 3 px ink outline OUTSIDE and a light edge INSIDE (the cream `rim` on plates, the `lip` on wells). On any floor one of the two clears 3:1. Use `.tt-plate`, `.tt-tag`, `.tt-chip`, `.tt-bar--world`.
2. **Two grounds.** Dark `plate` (text: `text`, `text-muted`, `text-disabled`, rarity/feat colours, `danger-text`, `good`) and light `enamel` tags (text: `ink`, `ink-muted`). Coloured fills (pills, bands, badges) always carry `ink` text. Never light text on enamel or ink text on plate. Never text straight on the world or on the veil: always on a plate or tag (world-space callouts and damage numbers are the exception: letters with the ink outline, `.tt-outlined` / `.tt-outlined-4`).
3. **Hard print shadows only.** No blur, no glow, no gradients, no glass on any plate. A plate's FILL is always 100% opaque; a whole widget may fade (opacity on the widget) for show/hide and for Reduce Motion (D13). The low-health edge frame is not a plate: an 8 px opaque `vital` band with a 3 px `ink` inner line, shown at 25% Health or below, not pulsing (D21).
4. **Radii:** plate 12, tag 10, key 8, pills full. **Strokes:** outline 3 (4 in High Contrast, automatic via `--tt-outline`), rim/lip 2, focus 3, focus keyline 2.
5. **Focus: ink | focus | ink** (see Focus below). Every focusable control shows it in at least one preview state.
6. **Colour is never the only signal.** Rarity = colour + rarity badge (`data-rarity`) + the colour WORD in names ("Unidentified Purple Amulet"). Stamina tier = colour + notches at 33/66 + multiplier pill (`×1.25`, `×1.0`, `×0.75`, `×0.5`) + fill pattern (solid / `.is-striped` / debt hatch). Status = name + glyph + chip polarity (buff: light enamel chip with a raised notch on top; debuff: dark plate chip with a pinked sawtooth bottom edge and a `debuff` rim). Feat tier = badge silhouette (circle / FLAT-TOP hexagon / 8-point starburst) + tier word (D14). Enemy tier = count of gold diamonds. Damage type = colour + type glyph, except Physical, which carries no glyph (D15); Shadow prints inverted; Radiant has a gold halo; heals carry "+". Lit vs unlit tally strokes = solid vs dotted. Reached vs ahead pips = filled vs hollow.
7. **Arrows, checks, crosses, triangles are glyphs** (`data-glyph="arrow-up"` etc.), never font characters: the fonts lack them and so would UE.
8. **Type.** Use the token classes from tokens.json: Tally voice `tally-display`, `tally-title`, `tally-heading`, `callout`, `kicker`, `pop` (Rowdies); World voice `world-title`, `world-heading` (Atkinson Next 800, tracked caps); Interface `ui-title`, `ui-body`, `ui-label`; Numerals `num-xl`, `num-l`, `num-m`, `num-s` (Atkinson Mono: tabular). **Nothing below 20 px.** Every number that changes is in a `num-*` class. Caps come from `.tt-caps` (in UE: TextTransformPolicy ToUpper, so GetText keeps the shipped string).
9. **Tone-bleed rule.** Death, boss bar and boss intro, biome cards, loading, endings and status chips use the World voice and carry NO Tally remarks or playful ornament (no rivets, no tally strokes). The Tally voice is for progress (tickets, plates, receipts, the rail, the title screen).
10. **Sizes are design px at 1920x1080** (UMG point size = px x 0.75). The HUD edge margin is 32 px (96 with the TV margin option). Live numbers sit in fixed-width boxes sized for their longest value (Mono digit = 0.632 em).
11. **Contrast, both themes.** Text 4.5:1 (3:1 at 24 px and up); borders, focus rings and icons 3:1. Check Diorama and High Contrast.
12. **High Contrast keeps outlines (D22).** Sunk, pressed and unlit states keep their full white outline in High Contrast; sinking shows by the print offset only, never by darkening the rim or covering the lip with a `well` inset. (Core does this for `.tt-plate.is-unlit`; components with their own pressed/sunk styles must too.)
13. **Rivets (D13)** only on Tally plates and front-end menu plates. Never on HUD plates or World-voice plates (death, banners, boss bar, loading).
14. **Dashed means unavailable, nothing else (D24):** locked, disabled, unbound, a locked "Coming Soon" option. Summons are marked with the `summon` glyph and a lighter fill, never a dashed rim.
15. **Data.** Shipping strings exactly as listed below. Invented data (extra feats, a second boss phase, tips, version strings) is labelled "example" in the README. Systems that do not exist yet get `TODO(DECISION)` in the README, never a silent invention.

## Focus (core)
- **The ring.** A 3 px `focus` ring (white; yellow in High Contrast) sits right outside the piece's own ink outline (offset = `--tt-outline`), and a 2 px `ink` keyline runs outside the ring: ink | focus | ink. The ring carries focus on plates, the veil and dark floors (13:1 on plate); the keyline carries it on blockout, sand and snow, where the ring alone is 1.1-1.3:1. Focus never removes a piece's own outline or print. Use `.is-focused` in static previews (`:focus-visible` live; `.tt-focusable` opts a non-core element into `:focus-visible`).
- **Variants** (add the class to the focusable piece; no local focus rules):
  - default — outlined pieces whose ink is an outer outline: plates, tags, chips, pills, nameplates, cards with an ink outline.
  - `.tt-focus--border` — pieces whose ink is their BORDER (keycaps, folder tabs, key-like cells): the ring sits at offset 0, right on the border.
  - `.tt-focus--fill-gap` — unoutlined pieces inside a plate (cards, rows, cells): an ink ring fills the gap, the focus ring sits outside it, no keyline (the plate is the ground). Put the piece's own box-shadows in `--tt-shadow` to keep them. REQUIRED on any transparent piece, because the default keyline would also edge its text.
  - `.tt-focus--own` — the piece draws its own ring (an open-bottom folder tab fused to its plate, a map cursor): core draws neither ring nor keyline.
  - `.tt-btn` — built in (2 px ink gap ring, the focus ring, the keyline).
- **In UMG:** one ring Image (9-slice, ink 2 | focus 3), placed `--tt-outline` outside the piece in an Overlay, Visible only while the widget has keyboard/gamepad focus. Same brush for every piece; the variants only change its offset and whether the ink keyline layer shows.

## Core primitives (`_core.css`, `core.js`)
| Class | What it is | Use it for |
|---|---|---|
| `.tt-plate` (`--menu`, `--inset`), `.tt-pad`, `.tt-pad-lg` | The pressed-tin plate (fill, rim, ink outline, print) | Every dark panel. `--menu` prints 6 px; `--inset` is a plate-raised card inside a plate. |
| `.tt-tag` (`--plain`, `--block`), `.tt-sub` | Enamel ticket with the grommet eyelet (12 px well hole, 3 px enamel-shade ring, 2 px ink ring) | Tickets, tags, prompts, nameplates. |
| `.tt-string`, `.tt-string--light` | 2 px ink string; the light string (4 px `rim` core, 1 px ink sides) | Hanging tags. Light strings hold on every floor and on the veil. Pseudo-element copies use `--tt-string-light-w`, `--tt-string-light-core`, `--tt-string-light-shadow`. |
| `.tt-key` (`--pad`, `--shoulder`, `--wide`, `--sm`), `.is-pressed`, `[aria-disabled]` | Keycaps: square = keyboard, round = gamepad face, pill = shoulder/trigger | Every input hint. Disabled/unbound keycaps are dashed (D24). |
| `.tt-hint` | Keycap + label, 8 px apart, keycap first | Inside a hint bar or a button. |
| `.tt-hintbar`, `.tt-hintbar__sep` | **The hint bar (D25):** keycap leads its label, "\|" separators, fixed gaps, on its own plate | Every screen footer and front-end hint. Placement may vary; the look does not. |
| `.tt-btn` (`--primary`, `--danger`, `--block`) | Menu buttons | `--danger` only for destructive actions (delete save), never Quit (D4). |
| `.tt-selectable` + `.is-selected` | **Selected = cream (`enamel`) fill, ink text (D6)** | Tabs, filter chips, categories, plate cards. The focus ring goes on top when focused. |
| `.tt-chip`, `.tt-pill` (+ tier variants) | Plate pills for statuses; token-fill pills with ink text | Status chips; multiplier and state pills. |
| `.tt-bar` (+ `__fill`, `__trail`, `__lead`, `__notch`, `__debt`, colour variants, `--world`, `--thin`, `--hair`, `.is-striped`) | Sunk well + lip meters | Every meter. Debt rule below (D2). |
| `.tt-pips`, `.tt-pip`, `.is-on` (`--lg`, `--enamel`) | **Pips (D1):** filled disc = reached, hollow ring = ahead | Boss phases, status stacks, anything counted that is not a rank or rarity. |
| `.tt-divider` (`--enamel`, `--v`) | **A quiet 2 px rule (D1)** | Menu and front-end dividers, title and credits rules. Never a row of tally strokes. |
| `.tt-empty`, `.tt-empty-box`, `.tt-empty__sub` | **The one empty state (D8):** a plain enamel tag, no eyelet, no radio dot, ink text, centred in its content box | "Inventory is empty.", "No items match this filter.", "No resources stored.", "Nothing to identify." (example), "Coming Soon". Never a radio dot on non-interactive text. |
| `.tt-drawback` (`__text`, `__label`, `--enamel`) | **The one Drawback box (D14):** the `.tt-hatch` strip, then "Drawback" inline and the cost | Feat plates, feat detail, tickets. |
| `.tt-hatch`, `--tt-hatch` | The one drawback / warning hatch (`danger` / `ink`, 7 px stripes) | Drawbacks and error bands. |
| `.tt-railbar` | **The one rail family (D23):** a teal tin bar with an end cap at each end holding an eyelet | The title-menu rail, the MageTower rod, the spike's base plate. Set its width; caps overhang 16 px. |
| `.tt-specband`, `__item`, `__title` | **Spec notes (D7):** a band under a rule, plain `.tt-label` text | Every annotation. Below the screen, never inside a 1920x1080 frame, never in chip or pill shapes. |
| `.tt-diorama` (`--blockout`, `--sand`, `--mire`, `--snow`, `--night`; props `__tree`, `__bush`, `__rock`, `__crate`, `__post`, `__pond`, `__path`, `__patch`) | **The toy world behind menus (D12):** floor tiles in a 96 px checker + blocky props, no blur | Behind the pause, death and options veils (with `.tt-veil` on top). |
| `.tt-stage`, `.tt-floor-blockout` / `-grass` / `-sand` / `-mire` / `-night` / `-snow`, `.tt-floors`, `.tt-veil` | World stand-ins for HUD previews | Prove every HUD edge on light and dark ground. |
| `.tt-glyph` (`--20`...`--64`), `.tt-tally`, `.tt-rarity`, `.tt-socket`, `.tt-r-*` | Glyphs, tally strokes, rarity badges (drawn by `TinTally.mount`) | See Tally below and Glyph names. |
| `.tt-outlined`, `.tt-outlined-4` | Ink-outlined lettering | World-space callouts and damage numbers only. |
| `.tt-label`, `.tt-caps`, `.tt-num`, `.tt-row`, `.tt-col`, `.tt-sr` | Small helpers | Labels, caps, layout, screen-reader text. |

`core.js`: `TinTally.mount(el)` fills `[data-glyph]`, `[data-tally]` (`data-max`, `data-height` 20, `data-stroke` 3, `data-pitch` 6, `data-cap`) and `[data-rarity]` (`data-height`). `TinTally.tally(n, max, {height, stroke, pitch, gap, capFirst})` returns the SVG.

## The tally mark (D1)
- **Exactly three jobs:** skill ranks (20 strokes, slashes on 5, 10, 15, 20, a cap dot on rank 1), rarity badges, and the seal / loading throbber (throbber on the loading screen only). Nothing else draws strokes:
  - combo chain → CombatCallout shows only "Hit N"; LoadoutPlate draws no strokes under the weapon name;
  - boss phases and status stacks → `.tt-pips`;
  - feat conditions → a thin bar (`.tt-bar--thin`) with a "6 / 10" count;
  - options slider → a notched track (a `.tt-bar` with `__notch` marks);
  - title and credits dividers → `.tt-divider`;
  - LoadingScreen → the wordmark + the throbber only (no tally field, no tally rule, no seal medallion);
  - Credits "Printing..." → text only, no throbber;
  - the rail's pending badge → the number only ("+3");
  - the world map's charted count → numerals.
- **Colour:** lit rank strokes are `text` on plate and `ink` on enamel, never `good`; Rank 20 is a full bundle in `text`. A gold full bundle means rarity Gold and nothing else.
- **Unlit strokes are dotted** in every theme (core.js): an unlit upright is a thinner column of dots fitted end to end; an unlit slash is three dots on the slash line that never land on an upright. Dots are `lip` on plate and `ink-muted` on enamel (automatic inside `.tt-tag`; elsewhere add `.tt-tally--enamel`). Never restyle `.tt-tally-empty` locally.
- **Sizes** (the Tally card is the source): skill row 24 tall, stroke 3, pitch 7 (148 px); detail 32 / 4 / 9 (180 px).

## Meters and resources
- **Stamina debt (D2).** Exhausted debt is left-anchored: width = |Stamina| / MaxStamina; regen eats it from its right edge, then the fill grows from the same left edge (one scale for debt and fill). StaminaFloor is an ABSOLUTE -50 in code (`UMordecaiStaminaSystem::StaminaFloor`), so the deepest debt is 50 / MaxStamina: 37% at the default MaxStamina 134 (MaxStamina = 100 x (1 + 0.04 x EffEnd)). One widget: `StaminaDebtBar` (UProgressBar, LeftToRight, `M_UI_PatternFill` hatch, percent clamped 0..1). Core: `.tt-bar__debt` with `--d`.
- **Player posture broken (D3):** the `posture` fill with an `ink` hatch, the rim turned to `rim`; no red, no chip. The player has no posture-break logic yet: `TODO(DECISION)`. Red / `danger` means threat only.
- **Spell Points (D17):** a token row designed for 10-16 tokens (16 fit in the 268 px gauge track). The default-state preview is "13 / 13" (MaxSpellPoints = 10 x (1 + 0.04 x EffInt) = 13 at default INT). The bar fallback (`SpellPointsProgressBar`) only above 16.
- **Gauge geometry:** track 268 px; HP 268 x 24, ST 268 x 18, posture 268 x 8 (GaugePlate is the source).
- **Spell slot well (D13):** 64 x 64 (SpellSlot's).
- **Damage numbers (D15):** Physical numbers carry NO type glyph (the `dmg-physical` note stands); `TODO(DECISION)` in DamageNumber whether busy melee fights need the subtype.

## Devices (D5)
- Every screen is drawn in ONE device. The default is keyboard and mouse: the pause footer reads "Esc Back · I Close", with no LB / RB bumpers at the tab-bar ends. The gamepad version ("B Back · LB / RB Tabs · A ...", bumpers at the bar ends) is shown only in the spec band below the screen.
- Keyboard tab cycling is `TODO(DECISION)` (proposal Q / E, mirroring LB / RB). Never draw it as bound: the Tier 4 playtests press real keys.
- Keycap shape tells the device: square = keyboard, round = gamepad face, pill = shoulder/trigger; mouse buttons are drawn as mouse keycaps, never "LMB" on a square.

## Menus and screens
- **Pause shell (D13):** a 56 px tab bar (y 220) over the shell plate, 1000 x 584 at (460, 276); the content box is 948 x 540 (padding 20 / 24). The footer hint bar sits under it (y 892); the spike is at x 1500-1880.
- **Selected (D6):** cream (`enamel`) fill everywhere (tabs, filter chips, categories, plate cards), with the focus ring added on top when focused. The one exception: enamel tickets in a list (the Mage Tower's sealed tags, the ServiceScreen instance whose items are tickets) are enamel already, so they select by sliding 12 px right, not by fill. Tab-state sheets show Idle, Hover, Press, Focus, Selected, Selected+Focused.
- **The spike (D9):** shows the SAME ticket kinds as TallyRail, compressed to one line: keep the stub colour and the kicker vocabulary (SKILL UP, ITEM · PURPLE, STORED, FEAT PROGRESS, IDENTIFIED). Max 8 visible; a ScrollBox holds the rest (12 kept). Its base is a `.tt-railbar` foot plate with ink outline and print (D23).
- **The HUD behind the veil (D10):** the HUD shown behind the pause veil and the death screen is built from the real HUD components' markup and classes (copy it from `components/HUDLayout/preview.html`), never a stand-in.
- **The world behind the veil (D12):** behind the pause, death and options veils draw the toy diorama (`.tt-diorama` floor tiles + a few props), not a flat grey grid. No blur.
- **Death screen (D11):** keeps the World voice ("You Died", no remarks) and gets a family frame: the Banner inlay mat in `foe` / `danger`, or hung on two strings like the "Paused" nameplate. The countdown ring gets the seal treatment: a dashed inner ring, ink outline, print shadow.
- **Buttons and dialogs (D4):** Dialog's rule wins. Quit is a plain `tt-btn`; `--danger` only for destructive actions (delete save). Row order: risky action left, safe action right, default focus on the safe action. The keycap LEADS its label everywhere ("[Esc] Stay", "[Enter] Identify"), matching the shipping "[F] Pick up ..." order.
- **Hint bars (D25):** `.tt-hintbar` everywhere (keycap first, "|" separators, fixed gaps, on a plate); placement may vary per screen.
- **Empty states (D8):** `.tt-empty` only.
- **Spec notes (D7):** `.tt-specband` below the screen, plain `tt-label` text; never chip or pill shapes, never inside a 1920x1080 frame.
- **Rails (D23):** one family, `.tt-railbar`: the title rail, the MageTower rod, the spike's base.
- **Feats (D14):** one Drawback box (`.tt-drawback`). A feat medallion carries the feat's own glyph everywhere (Pyromaniac = `fire`). The Rare hexagon is FLAT-TOP everywhere. Feat text fields are proposed once, on `UMordecaiFeatDataAsset`: `CauseText`, `DrawbackText`, and a `DisplayLabel` per unlock condition (all three proposed, `TODO(DECISION)`).

## Voice
- **Plain voice (the game):** sentence case, verbs on buttons ("Identify", "Allocate", "Stay", "Quit"), digits for numbers, exact system vocabulary. No exclamation marks except the shipping callouts.
- **The Tally (remarks, optional second line):** an Automation-Age counting engine. Deadpan, literal, indifferent rather than cruel; funny because it cares precisely about the wrong thing. At most 9 words. Never in combat (a receipt forced during combat prints no remarks), never on death/boss/biome/ending/status lines, never about story, NPC loss or the Lich King. The headline is always plain and complete without the remark (Settings can turn remarks off). Do NOT use "Filed under ..." jokes.
- **Copy canon (D26):** TallyRail's kinds table is the ticket copy (Slip; SKILL UP "Skill up" + "6 / 20" + skill name, footer "+N ranks" when merged; FEAT PROGRESS; STORED; ITEM "Item · Purple"; IDENTIFIED "Identified" with the rarity in the footer). Banner biome cards carry the biome name alone. The Iron Warden intro line is one string, data (`IntroLine`): "Enforces strict patterns. Punishes deviation." (example).
- **Shipping strings are kept EXACTLY** (playtests compare them; restyle, never reword): "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!", "Hit N", "Kills: N", "x / y" (SP), "[F] Pick up {item}" (rendered as an F keycap + "Pick up {item}"; the TextBlock still reads "[F] Pick up …" until device glyphs ship), "[I] Inventory", "You Died", "PROJECT MORDECAI" (ToUpper of "Project Mordecai"), "Loading...", "Paused", tab labels "Character" "Skills" "Feats" "Inventory" "Settings", "Coming Soon", "Inventory is empty.", "No items match this filter.", "No resources stored.", "Identify", "Stored Resources", filters "All" "Weapons" "Armor" "Trinkets" "Consumables" "Materials" "Quest" "Magical", feats header "Feats: N unlocked — a Common, b Rare, c Legendary", ledger rows "Iron Ore  x1", "Unidentified <Rarity> <Type>", "Unknown Item", "Unarmed", rank "rank / 20".
- Status names: show display names ("Stone Skin") in mockups and note in the README that StatusNameText returns the tag segment ("StoneSkin") until US-091 changes it with the playtests.
- Status taglines (use verbatim): Burning "You're on a clock", Bleeding "Recovery is denied", Poisoned "You can't catch your breath", Frostbitten "Sluggish body, slower reactions", Shocked "Timing disruption", Weakened "You can't break them", Brittle "You break easily", Silenced "Magic is shut off", Rooted "Position solved or punished", Blinded "Reads become unreliable", Fear "Composure collapse", Cursed "Resource rules broken", Exposed "Next mistake hurts more", Corroded "Gear advantage denied", Drenched "Elemental rules shift", Focused "Mastery streak reward".
- Real data to use in previews: spells Q Fireball (3 SP, 2.0 s, Fire), E Blink (2 SP, 4.0 s), R Stone Skin (4 SP, 15 s), 1 Restoration (3 SP, 10 s); gamepad LB / RB / none / D-pad Up. Weapons Longsword (3-hit chain, Slash), Dagger (5, Pierce), Greatsword (2), Spear (3, Pierce), Mace (2, Blunt). Items: Health Potion (Common, stack 10), Iron Ore, Town Stone, Upgrade Key (Blue), Mysterious Amulet (Purple, starts "Unidentified Purple Amulet"), Rusty Sword (Common), Emberfang (Purple magical sword, Fire, from the item schema). Feat: Pyromaniac (Common, "Burn 6 enemies at once", glyph `fire`). Enemies: Frontliner (Basic), Goblin Skirmisher (Basic), Skeletal Archer (Basic, Artillery), Corrupted Knight (Elite, Bruiser), Iron Warden (boss, Searing Expanse). Anything you must invent is labelled "example" in the README.
- Systems that do not exist yet (Level/XP, skills levelling from use, potion cap, identify cost, currency, player posture break, keyboard tab cycling, the spike's history) appear only with a note `TODO(DECISION)` in the README. Ranks are bought with skill points (Allocate) today.

## HUD architecture (D20)
- The HUD root (`UMordecaiCombatHUDWidget`) is reparented to a `ULyraActivatableWidget` (a Lyra HUD layout) and pushed ONCE on `UI.Layer.Game`. TallyRail, TallyPlate, Banner, BossBar, QuickItems, LoadoutPlate, the damage pops and the low-health edge frame are children of that root canvas, never separate pushes (a second push on a layer stack would deactivate and hide the HUD).
- Only the death screen is pushed onto the Game stack, which hides the HUD while the player is dead. The pause menu (`UI.Layer.Menu`) and dialogs (`UI.Layer.Modal`) draw their veil over it.
- **Always visible during play:** the gauge plate, the spell slots, the rail head, the loadout plate and the "[I] Inventory" hint pill. Everything else is contextual.

## HUD map (1920x1080, collision-free; use these exact boxes; HUDLayout is the composite)
| Element | Widget (keep the name) | Anchor | Offset | Size | Box | Shown |
|---|---|---|---|---|---|---|
| Gauge plate (HP, ST, posture, SP) | new `GaugePlate` hosting `HealthBar`, `StaminaBar`, `PostureBar`, `SpellPointsBar` | top-left | (32, 32) | 456 x 152 | x 32-488, y 32-184 | always |
| Spell slots Q E R 1 | `SpellCooldownA`-`D` | top-left | x 32 / 120 / 208 / 296, y 216 | 78 x 78 each | x 32-374, y 216-294 (rivets to 207, keycaps to 304) | always |
| Status chips (max 8, 2 rows) | `StatusEffectBar` (`IndicatorContainer`) | top-left | (32, 318) | 456 x 112 | x 32-488, y 318-430 | when any |
| Rail head (Kills + "+N") | new `TallyRailHead` hosting `KillCounter` | top-right | (-32, 32) | 320 x 56 | x 1568-1888, y 32-88 | always |
| Tickets (max 4 hanging) | new `TallyRail` | top-right | (-32, 104) | 320 wide, 8 gap | x 1568-1888, y 104-480 | out of combat; a pressed pickup's 2 s piece in combat |
| Plate (big moment) | new `TallyPlate` | top-centre | (0, 40); (0, 168) under a boss bar | 640 x 128 (184 with a Drawback) | x 640-1280 | out of combat |
| Boss bar | new `BossBar` | top-centre | (0, 32) | 880 x 96 | x 520-1400, y 32-128 | boss fights |
| Banner (biome / boss intro) | new `Banner` | top-centre | (0, 296) | 928 x 132 | x 496-1424, y 296-428 | entering a biome, boss intro |
| Combat callout | `CombatFeedback` | centre | (0, +128) | 520 x 52, letters only | x 700-1220, y 668-720 | 1.0 s |
| Combo "Hit N" | `ComboCounter` | centre | (0, +184) | 220 x 36 | x 850-1070, y 724-760 | during the combo window |
| Interact prompt | `PickupPrompt` (`PromptText`) | bottom-centre | (0, -36) | 540 x 60 | x 690-1230, y 984-1044 | when something is focused |
| Inventory hint pill | `PickupPrompt` (`InventoryHintText`) | docked 12 px right of the prompt box | — | 151 x 40 | x 1242-1393, y 994-1034 | always |
| Quick items (Throw + A-D) | new `QuickItems` | bottom-right | (-32, -132) | 336 x 56 | x 1552-1888, y 892-948 (keycaps perch to 868) | when stocked |
| Loadout plate (weapon) | `UMordecaiEquippedWeaponWidget` | bottom-right | (-32, -32) | 340 x 84 | x 1548-1888, y 964-1048 | always |
| Low-health edge frame | child of the HUD root | full screen | 0 | 8 px band + 3 px ink line | the screen edge | Health 25% or less |

World space: enemy tags (`UMordecaiEnemyIndicatorWidget`, Z +120, 150 wide), damage numbers (`UMordecaiDamagePopComponent`, pooled UMG pops), pickup labels. The player is the screen centre (960, 540): the callout lane is under their feet. The callout box keeps its bottom edge at +180 (4 px above the combo); High Contrast plates and the Large / XL callout sizes grow it upward only, so the combo slot never moves. TV margin: every edge offset becomes 96.

## Pacing (D19; ReceiptTimeline is the source of truth)
- **Lanes:** Callout (centre, never queued, the newest replaces the last) · Ticket (the rail: slips and tickets) · Plate (top-centre, one at a time) · Banner (biome and boss intros; plates wait for it).
- **Combat hold:** while the player dealt or took damage in the last 4 s, or an aggroed enemy is within 1500 uu, only callouts, damage numbers and the rail's "+N" pending badge change. A pickup the player pressed for still prints at once for 2 s in its usual form: a slip (Common) or an ITEM ticket (Green and up). The always-visible pieces keep updating: they are state, not news.
- **Receipt:** after 2.5 s of calm, waiting pieces print 350 ms apart: slips, then tickets, then plates one at a time. On screen: slip 3 s, ticket 5 s, plate 4 s (Legendary 5.5 s), banner 1.5-3.5 s (Banner's states); the Ticket duration setting multiplies the slip and ticket times.
- **Hard cap:** 20 s counted from the OLDEST waiting entry; it forces slips and tickets only. Plates are never forced: a big moment never lands mid-fight.
- **Pause:** files the waiting tickets to the spike (they never print later; the badge clears). Waiting plates print after the menu closes, once calm.
- **Merging:** identical tickets merge ("Iron Ore x3"); merged rank-ups print "+2 ranks" in the ticket footer; a feat's unlock replaces its progress ticket.
- **Overflow:** when more than 4 pieces would hang, 3 tickets show plus "N more on the spike".
- **History:** the pause menu's spike keeps the last 12 (8 visible, ScrollBox for the rest).
- **Density:** see Settings.

## Motion (D18; one row each in the brand book, cited by name, no local numbers)
| Animation | Timing |
|---|---|
| TicketDrop | One 350 ms swing about the eyelet (+8 deg to 0, -1.5 deg overshoot) |
| TicketRetract | 180 ms ease-in into the slot |
| DamageTrail | The ghost holds 350 ms, then drains over 450 ms, ease-in-out |
| HealLead | The `heal` lead snaps to the new value; the fill follows in 400 ms, ease-out; the lead hides when they meet (GaugePlate's) |
| StampReveal | 230 ms strike, then a 260 ms band wipe |
| TabSwitch | 90 ms out, 160 ms in, 12 px |
| KeyPress | 60 ms down 3 px, 120 ms back |
| CalloutPop | 160 ms in, 640 ms hold, 200 ms fade |
| Throbber | 160 ms per stroke, the slash holds 400 ms, a 1200 ms loop, drawn in `text`; keeps looping under Reduce Motion (it proves the game has not hung) |
| DamagePop | 120 ms pop in (a crit 160 ms, its burst −8 deg to 0); rise 100 cm/s, scatter 30 cm, a 60 cm/s outward drift on alternating sides; fade over the last 300 ms of 1.5 s (DamageNumber's) |
| PromptSlide | 12 px slide out of the hint pill with a 120 ms fade in, ease-out; back in 100 ms; a retarget eases the width over 90 ms (InteractPrompt's) |
| LabelLift | 10 px lift over 120 ms (ease-out back), keycap fade 80 ms; reversed in 100 ms (PickupLabel's) |
| WorldTagFade | Whole-widget fade: in 120 ms on first aggro or damage, out 200 ms at the leash (EnemyTag's) |
| BracketSnap | Scale 1.15 to 1.0 over 120 ms (EnemyTag's critical brackets) |
| BossOut | The emptied bar holds 600 ms, then a 300 ms whole-widget fade (BossBar's) |
| ThreatPipFade | 120 ms fade in, then continuous tracking; never pulses (WorldMarkers' threat pips) |
| ReticleCancel | Shrink to 55% and fade out in 150 ms (WorldMarkers' reticle) |
| ReadyGlint | One 140 ms sweep of two `rim` bands across the well, the lip turning `rim` (SpellSlot's) |
During combat, animations last at most 160 ms in. Reduce Motion turns swings, stamps and shakes into 150 ms fades; nothing flashes faster than 3 Hz. Only the throbber and the gauge plate's 1 Hz low-health rim step loop.

## Settings (D16; OptionsScreen's Accessibility table is the single list)
| Setting | Values |
|---|---|
| High Contrast | Off / On |
| Tally notifications | Full / Calm / Minimal (Full: everything; Calm: no remarks, slips only out of combat; Minimal: plates only, tickets to the spike) |
| Tally remarks | On / Off |
| Ticket duration | ×1 / ×1.5 / ×2 of the base hang times: slip 3 s, ticket 5 s (TallyRail's) |
| Callout size | Normal / Large / XL = 40 / 48 / 56 px letters (CombatCallout's) |
| Damage numbers | All / Mine / Off |
| UI scale | 80-150% |
| HUD margin | 32 / 96 px (TV safe) |
| Reduce Motion | Off / On |

## Unreal asset manifest (D21; `03-building-in-unreal.md` is canonical, components cite its names)
- **Plates:** a RoundedBox brush stack (fill, rim, ink outline, print offset), no plate textures.
- **Patterns:** `M_UI_PatternFill` for every patterned fill (stamina Red stripes, the Exhausted debt hatch on `StaminaDebtBar`, the posture-broken hatch, the drawback hatch), sampling `T_UI_Hatch16`.
- **Cooldown:** the SpellSlot slat shutter: a UProgressBar, TopToBottom, `plate-raised` slats over the glyph.
- **Damage numbers:** pooled UMG pops; no Niagara text.
- **Low-health edge frame:** an 8 px opaque `vital` band with a 3 px `ink` inner line at 25% Health or below, not pulsing; a child of the HUD root.
- **Focus:** one ring brush (ink 2 | focus 3), see Focus.
- **Theme:** `UMordecaiUIThemeDataAsset` per theme (`DA_UITheme_Diorama`, `DA_UITheme_HighContrast`), every colour from the sRGB hex in tokens.json via `FLinearColor::FromSRGBColor`.

## Preview file format (project/components/<Comp>/preview.html)
```
<!-- @dsCard group="HUD" height=320 width=960 subtitle="One line" -->
<!doctype html>
<html lang="en">
<head><meta charset="utf-8"><title><Comp></title></head>
<body>
<div class="tt-root ...">  ...  </div>
<script>/* optional; TinTally.mount() runs automatically on load. Call TinTally.mount(el) after inserting markup. */</script>
</body>
</html>
```
- Line 1 is the marker, exactly `<!-- @dsCard group="..." height=N width=N subtitle="..." -->` with single spaces between attributes, in that order. Groups: `Foundations`, `HUD`, `World`, `Tally`, `Menus`, `Front end`. `height` is the frame height in px at `width` (default 960; a full-screen mockup uses width=1920 and is scaled to fit). Make the height fit the content, no big empty bands.
- The page preloads tokens.css, bundle.css (core + all partials) and bundle.js, and sets `<html data-theme="diorama|high-contrast">`. Do not set data-theme yourself. Do not link any other script or stylesheet. No iframes/objects/embeds. No external images. No `<style>` unless a rule is truly preview-only (layout scaffolding); component styling belongs in your `css/<Comp>.css`.
- Show the component in its real states (ready / cooling / disabled / focused / selected / empty / overflow...), with real data, over a world stand-in where it lives on the world (`.tt-stage` + `.tt-floor-blockout` | `-grass` | `-sand` | `-mire` | `-night` | `-snow`; show at least the light blockout and one dark floor for HUD pieces), or over `.tt-diorama` + `.tt-veil` for menus. Label states with small `.tt-label` captions on a plate (never on the floor). Annotations go in a `.tt-specband` below the screen.
- Both themes must work with no extra rules (everything through tokens). Read both PNGs.

## README format (project/components/<Comp>/README.md)
```
# <Comp>

<First sentence: what it is, one line — it becomes the summary.> <Second sentence: when to use it.>

## Anatomy
## States
## Data and implementation   (the UMG/CommonUI widget, existing C++ class and BindWidgetOptional names to keep, the GAS attributes/tags/events it reads, sizes and anchors at 1920x1080, which tokens, which 03 asset names)
## Copy   (exact strings; Tally remarks if any, with the rule)
## Do / Don't
```
Plain, imperative, specific. The first sentence is a one-line summary. Name tokens in backticks (`plate`, `vital`). Name existing classes (`UMordecaiStaminaBarWidget`) and widget names (`StaminaProgressBar`). Cite motion rows by name (DamageTrail), never local numbers. Mark open items `TODO(DECISION)`. No marketing tone, no emoji.

## CSS partial rules (ds/css/<Comp>.css)
- Prefix every class with `tt-` plus a component stem (e.g. `.tt-gauge`, `.tt-gauge__row`). Reuse core classes; do not restyle core classes globally.
- Colours only through `var(--token)` (and `color-mix()` of tokens). No literal colours at all: scenery uses the theme-invariant `world-*` tokens (`world-blockout`, `world-blockout-line`, `world-grass`, `world-grass-dot`, `world-sand`, `world-sand-dot`, `world-mire`, `world-mire-dot`, `world-night`, `world-night-dot`, `world-snow`, `world-snow-dot`, `world-light`, `world-dark`, `world-shade`) and the `biome-*` tokens.
- No local focus rings, tally-stroke hacks (`line[stroke=...]`), empty states, drawback boxes, hint bars, dividers or rails: use the core primitive.
- No `@import`, no fonts, no `</style`.

## Glyph names (use exactly these; the icon authors draw them)
status: burning bleeding poisoned frostbitten frozen shocked weakened brittle silenced rooted blinded fear cursed exposed corroded drenched focused stoneskin fireward blessed restoration enchanted blurred resolved sleeping
damage: physical slash pierce blunt fire frost lightning poison corrosive arcane shadow radiant heal crit block immune
spells: fireball blink cone-of-cold magic-missile
weapons: longsword greatsword shortsword dagger axe mace spear quarterstaff unarmed bow crossbow throwable wand
items: all weapon armor trinket consumable material quest magical potion ore stone upgrade-key amulet chest currency bandage tonic
system: lock unknown check cross arrow-up arrow-down arrow-left arrow-right drawback pin skill-point diamond map gate player character skills feats inventory settings new seal search speech summon sprint reticle
settings sections: display audio controls accessibility
tools and town: explosives magnet shock-device invisibility phase glider mage-tower alchemist blacksmith clinic keep
(D27: bandage, tonic, search, speech, summon, sprint, reticle, display, audio, controls and accessibility are new this round; quarterstaff, dagger, shortsword, corroded, bleeding, stoneskin, inventory, magic-missile and map were redrawn.) Shapes worth knowing: `shadow` is a spiral; `corrosive` is a drop falling into a basin; `physical` is never drawn on a damage number.
Use: `<span class="tt-glyph tt-glyph--32" data-glyph="burning" aria-label="Burning"></span>`; tally `<span class="tt-tally" data-tally="12" data-max="20" data-cap></span>`; rarity `<span class="tt-rarity tt-r-purple" data-rarity="purple"></span>` (on enamel, wrap it in `<span class="tt-socket">`); pips `<span class="tt-pips" role="img" aria-label="Phase 2 of 3"><span class="tt-pip is-on"></span><span class="tt-pip is-on"></span><span class="tt-pip"></span></span>`; empty state `<div class="tt-empty-box"><p class="tt-empty">Inventory is empty.</p></div>`; drawback `<div class="tt-drawback"><p class="tt-drawback__text"><span class="tt-drawback__label">Drawback</span>Fire damage taken +10% (example).</p></div>`; hint bar `<div class="tt-hintbar"><span class="tt-hint"><span class="tt-key tt-key--sm">Esc</span>Back</span><span class="tt-hintbar__sep"></span><span class="tt-hint"><span class="tt-key tt-key--sm">I</span>Close</span></div>`.
