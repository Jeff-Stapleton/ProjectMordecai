# HUDLayout

The whole HUD at 1920 x 1080, every HUD component in its HUD map box. Use it to place widgets in the HUD root, to check that a new HUD piece fits the map before it ships, to see what is on screen in each pacing state (its frames: a fight, the receipt after it, a boss fight at low health, and the map with the TV margin), and as the source of the HUD that PauseShell and DeathScreen draw behind their veils (D10: copy this card's markup, never a stand-in).

## Anatomy

### Zones

- **Left is you:** state, always on (gauge plate, spell slots), plus the status chips under them.
- **Top right is news:** the Tally's rail (rail head, tickets).
- **The centre is now:** callouts and the combo under the player's feet, where the eye already is.
- **Bottom centre is interaction:** the interact prompt and the "[I] Inventory" hint.
- **Bottom right is the loadout:** the loadout plate and the quick items.
- **Top centre is for big moments:** plates, the boss bar, banners.

### The map

Design px at 1920 x 1080 with ScaleToFit (0.666 at 720p). Edge margin 32 px (96 with the TV margin, below). Every box is collision-free at 32; keep them.

| Element | Widget (keep the name) | Anchor | Offset | Alignment | Size | Box at 1920 x 1080 | Shown | Card |
|---|---|---|---|---|---|---|---|---|
| Gauge plate | new `GaugePlate` hosting `HealthBar`, `StaminaBar`, `PostureBar`, `SpellPointsBar` | top-left | (32, 32) | (0, 0) | 456 x 152 | x 32-488, y 32-184 | always | GaugePlate |
| Spell slots Q E R 1 | `SpellCooldownA`-`D` | top-left | x 32 / 120 / 208 / 296, y 216 | (0, 0) | 78 x 78 each | x 32-374, y 216-294 (cost rivets rise to 207, keycaps hang to 304) | always | SpellSlot |
| Status chips | `StatusEffectBar` (`IndicatorContainer`, a WrapBox) | top-left | (32, 318) | (0, 0) | 456 x 112, 2 rows, max 8 | x 32-488, y 318-430 | when any status is active | StatusChip |
| Rail head | new `TallyRailHead` hosting `KillCounter` and `PendingBadge` | top-right | (-32, 32) | (1, 0) | 320 x 56 | x 1568-1888, y 32-88 | always | TallyRail |
| Tickets | new `TallyTicketStack` | top-right | (-32, 104) | (1, 0) | 320 wide, 8 gap, max 4 | x 1568-1888, y 104-480 | the receipt, the 20 s cap, a pressed pickup at once | TallyRail |
| Plate | new `TallyPlate` | top-centre | (0, 40); (0, 168) under a boss bar | (0.5, 0) | 640 x 128 (184 with a Drawback box) | x 640-1280, y 40-168 (168-296) | out of combat only | TallyPlate |
| Boss bar | new `BossBar` | top-centre | (0, 32) | (0.5, 0) | 880 x 96 | x 520-1400, y 32-128 | boss fights | BossBar |
| Banner | new `Banner` | top-centre | (0, 296) | (0.5, 0) | 928 x 132; phase card 560 x 96 | x 496-1424, y 296-428 (phase card x 680-1240, y 296-392) | biome entry, boss intro, boss phase | Banner |
| Combat callout | `CombatFeedback` | centre | (0, +128) | (0.5, 0) | 520 x 52, letters only | x 700-1220, y 668-720 | 1.0 s | CombatCallout |
| Combo | `ComboCounter` | centre | (0, +184) | (0.5, 0) | 220 x 36 | x 850-1070, y 724-760 | during the combo window | CombatCallout |
| Interact prompt | `PickupPrompt` (`PromptText`) | bottom-centre | (0, -36) | (0.5, 1) | 540 x 60, tag right-aligned | x 690-1230, y 984-1044 | when something is focused for Interact | InteractPrompt |
| Inventory hint | `PickupPrompt` (`InventoryHintText`) | docked 12 px right of the prompt box | (552, 10) inside it | — | 151 x 40 | x 1242-1393, y 994-1034 | always during play; never moves | InteractPrompt |
| Quick items | new `QuickItems` (Throw + Item A-D) | bottom-right | (-32, -132) | (1, 1) | 336 x 56 | x 1552-1888, y 892-948; reserve y 868-948 (keycaps perch to 868) | when stocked; an empty slot is Hidden | QuickItems |
| Loadout plate | `UMordecaiEquippedWeaponWidget` | bottom-right | (-32, -32) | (1, 1) | 340 x 84 | x 1548-1888, y 964-1048 | always | LoadoutPlate |
| Low-health edge frame | new `LowHealthFrame` | full screen | 0 | — | 8 px band + 3 px inner line | the screen edge, 0-11 px in | Health 25% or less | GaugePlate |

World space (no box; they follow their actors, and the HUD's screen pieces should draw over them):

| Element | Widget | Where | Shown | Card |
|---|---|---|---|---|
| Enemy tags | `UMordecaiEnemyIndicatorWidget` (world) | Z +120 over the enemy, 150 wide | after aggro or damage; Elite and up always | EnemyTag |
| Damage numbers | `UMordecaiDamagePopComponent`, pooled UMG pops on the HUD root's damage-pop canvas | the hit point, fanning up and out under the tag | 1.5 s | DamageNumber |
| Pickup labels | new, replaces the TextRender `NameLabel` | on a light cord over the item | near; focused when F will take it | PickupLabel |
| World markers | decals and VFX (the off-screen pips are UMG) | on the floor; pips on a lane 56 px inside the edge | aim chevron always (TODO below); telegraphs per attack; reticle while targeting | WorldMarkers |

The player is the screen centre (960, 540): the camera is fixed and follows the player, so the callout at (0, +128) always lands under their feet. The callout keeps its bottom edge at +180; Large / XL sizes and the High Contrast plate grow it upward only, so the combo never moves.

### Frames in the card

All four frames are 1:1 at 1920 x 1080, keyboard and mouse. Notes sit in the spec band under each frame (D7).

1. **In a fight (the combat hold),** on the light blockout diorama with two grass zones. Gauge plate mid fight (HP 64% with the DamageTrail ghost from 76, Stamina 70 / 134 Yellow ×1.0, sprint off, posture 70, 5 / 13 SP); Blink cooling 2.4 s; Frostbitten (2 of 4 stacks as pips) and Focused; "Kills: 7" with the "+3" badge and no tickets; three enemies (an Elite Corrupted Knight taking Physical hits with no glyph, a posture-broken Frontliner with the Riposte window and a mouse keycap, a burning Goblin Skirmisher whose fire tick carries the fire stamp); the aim chevron (today, +X) and the Goblin's jumpable low sweep at 60% of its windup with the player standing in it; "Perfect Block!" under the player; quick items and loadout; the Inventory hint alone at bottom centre.
2. **After the fight (the receipt),** the same floor, calm. The three waiting entries printed 350 ms apart: the "Iron Ore x3" slip, SKILL UP Longswords 6 / 20 (newest on top, peeled, its remark in the footer), then the Pyromaniac FEAT UNLOCKED plate (riveted: a Tally plate) with its remark tag on the light cord. No status chips, posture hidden at max, spells ready. The Rusty Sword's label carries the core focus ring and the prompt reads F + "Pick up Rusty Sword".
3. **Boss fight at low health,** a basalt arena (the dark night stand-in) round a sand floor. The Iron Warden's bar at (0, 32) counts Phase 2 with pips; the Banner's phase card "PHASE 2" lands at (0, 296). The 20 s cap forced the waiting slip and SKILL UP ticket through with no remarks; the Parrying technique plate is never forced, so the badge still reads "+1". Health 22%: danger-text number, the rim on its vital step, and LowHealthFrame round the screen. Burning; Stone Skin locked at 3 / 13 SP; Fireball cooling; a crit "42!" with no glyph; "Parried!" and "Hit 2" 4 px apart.
4. **Map,** a diagram on the spec plate, not a screen: every box above at 1:1 with its coordinates. Always visible = a lit `rim` bar along the top, a 3 px `rim` edge and a `plate` fill; contextual = a 2 px `lip` outline. Solid lines only (dashed means unavailable, D24). The TV margin band (32-96 px) is shaded and the 96 px safe line is drawn in `rim`; the screen edge wears the LowHealthFrame look.

## States

**Always visible during play (D20):** the gauge plate, the spell slots, the rail head, the loadout plate and the "[I] Inventory" hint pill. They keep updating in combat: they are state, not news. Everything else is contextual.

**Contextual:**

| Region | Appears when | Goes when |
|---|---|---|
| Status chips | a status is applied | the last one expires (the row collapses; nothing is anchored below it) |
| Posture lane (in the gauge plate) | posture is below max | posture is back at max (Hidden, keeps its space) |
| Tickets | the receipt (2.5 s of calm), the 20 s cap, or a pressed pickup | each piece's hang time ends (slip 3 s, ticket 5 s, a pressed pickup 2 s during the hold) |
| Pending badge "+N" | the hold starts with something waiting | nothing is waiting (the receipt prints the last piece, or the pause menu files tickets to the spike) |
| Plate | a big moment prints, once calm (feat unlocked, technique, upgrade key, tool) | 4 s (Legendary 5.5 s) |
| Boss bar | the boss fight starts (after the boss intro banner) | the boss fight ends |
| Banner | first entry to a biome, boss intro, boss phase | 2.5-3.5 s; phase card 1.5 s ("Enraged" 2 s) |
| Combat callout | Perfect Dodge / Perfect Block / Parried / Parry Failed / Posture Broken | 1.0 s, or the next callout replaces it |
| Combo | a light hit lands | the combo window closes |
| Interact prompt tag | something is focused for Interact | nothing is focused (the tag Collapses; the hint stays) |
| Quick items | a consumable is assigned | never as a row; an empty slot is Hidden so the rest keep their places |
| Enemy tags | aggro or damage; Elite and up always | the enemy dies or drops aggro |
| LowHealthFrame | Health 25% or less | Health above 25% |

**The combat hold (D19; ReceiptTimeline is the source of truth).** The player is in combat when they dealt or took damage in the last 4 s, or an aggroed enemy is within 1500 uu.
- While held, only the callout lane, damage numbers and the rail's "+N" badge change (the number only, after merging). Tickets and plates wait. The Banner is its own lane: the hold does not delay a boss intro or a phase card.
- A pickup the player pressed F for still prints at once for 2 s in its usual form: a slip if Common, an ITEM ticket if Green or above.
- **Receipt:** after 2.5 s of calm the waiting entries print 350 ms apart: slips, then tickets, then plates one at a time. Newest on top.
- **Hard cap:** 20 s after the OLDEST waiting entry, if the player is still in combat, its slips and tickets print anyway, without Tally remarks. Plates are never forced: they keep waiting for calm and stay on the badge.
- **Pause:** opening the pause menu files every waiting slip and ticket onto the spike (they never print later; the badge clears). Waiting plates print after the menu closes, once calm.
- **Overflow:** when more than 4 pieces would hang, 3 hang plus "N more on the spike".

**Top-centre stacking.** One plate at a time. With a boss bar up, the plate moves from (0, 40) to (0, 168) and ends at y 296, where the banner box starts (its outline and print reach y 303). A Drawback plate (184) at (0, 168) ends at y 352, inside the banner box. The phase card (560 x 96) is top-centred in the banner slot at y 296. They rarely meet: a plate never prints in combat, and with an aggroed boss within 1500 uu the player is in combat for the whole fight. `TODO(DECISION)` (Banner, TallyPlate): if the player leaves the boss for 2.5 s, a plate prints under the boss bar, and a phase change lands within its 4 s, the two overlap. Proposal: the plate retracts early when a phase card fires (the fight wins); plates already wait for a banner that is up.

**Low health.** At 25% Health or below: `HealthPercentText` turns `danger-text`, the gauge rim steps at 1 Hz (LowHealthPulse), the HP bar's 25% notch is passed, and LowHealthFrame shows: an 8 px opaque `vital` band with a 3 px `ink` inner line round the screen edge, not pulsing, HitTestInvisible. It is not a plate (no rim, no print). It ends 11 px in, 21 px short of every HUD box at the 32 px margin (85 px short with the TV margin).

**TV margin (HUD margin 96).** Every edge-anchored offset moves 64 px inward; centre-anchored pieces do not move.

| Element | At 32 | At 96 (TV) |
|---|---|---|
| Gauge plate | x 32-488, y 32-184 | x 96-552, y 96-248 |
| Spell slots | y 216-294, x 32 / 120 / 208 / 296 | y 280-358, x 96 / 184 / 272 / 360 |
| Status chips | x 32-488, y 318-430 | x 96-552, y 382-494 |
| Rail head; tickets | x 1568-1888, y 32-88; from y 104 | x 1504-1824, y 96-152; from y 168 |
| Boss bar | x 520-1400, y 32-128 | x 520-1400, y 96-192 |
| Plate; under a boss bar | y 40-168; y 168-296 | y 104-232; y 232-360 |
| Banner; phase card | y 296-428; y 296-392 | y 360-492; y 360-456 |
| Callout; combo | (0, +128); (0, +184) | unchanged |
| Interact prompt; hint | y 984-1044; y 994-1034 | y 920-980; y 930-970 |
| Quick items | x 1552-1888, y 892-948 (868) | x 1488-1824, y 828-884 (804) |
| Loadout plate | x 1548-1888, y 964-1048 | x 1484-1824, y 900-984 |

`TODO(DECISION)`: at 96 two boxes collide. The boss bar (x 520-1400, y 96-192) overlaps the gauge plate (x 96-552) by 32 px, and the banner (x 496-1424, y 360-492) overlaps the status strip (x 96-552, y 382-494). Scaling them down is not an option (their text would drop under 20 px). Proposal: with the TV margin, the boss bar and the banner take a 784 px width (x 568-1352, 16 px clear of the left column); BossBar and Banner own the call. Moving only the top-centre pieces' y does not help: the collision is horizontal.

## Data and implementation

- **Root (D20).** `UMordecaiCombatHUDWidget` (`WBP_CombatHUD`) is the HUD root. It is reparented to `ULyraActivatableWidget` (modelled on `ULyraHUDLayout`, not derived from it: that binds `UI.Action.Escape` to Lyra's escape menu) and pushed ONCE on `UI.Layer.Game` when the local player's pawn is ready, instead of `AddToViewport()`. Its explicit input config is Game so the persistent HUD leaves gameplay input enabled.
- **Children, never separate pushes.** Every region above is a slot in the root CanvasPanel with the map's anchor, offset, alignment and size, `bAutoSize` false: `GaugePlate`, `SpellCooldownA`-`D`, `StatusEffectBar`, `TallyRailHead`, `TallyTicketStack`, `TallyPlate`, `Banner`, `BossBar`, `CombatFeedback`, `ComboCounter`, `PickupPrompt`, `QuickItems`, the root-owned loadout plate (`UMordecaiEquippedWeaponWidget`), the damage-pop canvas and `LowHealthFrame`. A layer is a stack that shows only its top widget, so a second push would hide the HUD.
- **Draw order inside the root:** the damage-pop canvas first (it belongs to the world), then the regions (their boxes never overlap, so their order does not matter), then `LowHealthFrame` last, HitTestInvisible.
- **Layering.** The death screen is the only other push on `UI.Layer.Game`: as the top of the stack it hides the HUD (and the hint) while the player is dead, and popping it shows the HUD again. The pause menu, Options and service screens (`UI.Layer.Menu`) and dialogs (`UI.Layer.Modal`) draw their veil over the HUD, so "[I] Inventory" no longer floats over menus. A hidden HUD keeps its GAS bindings and re-reads every value in `NativeOnActivated`.
- **Names:** the BindWidgetOptional names in Building in Unreal; a WBP that replaces a fallback must keep them, or the piece silently disappears. The old screen-space `WBP_EnemyHealthBar` retires once enemy tags ship (US-092).
- **Moves from today's slots** (each card has the detail): KillCounter TopRight (-170, 20) to the rail head; CombatFeedback BottomCenter (0, -80) and ComboCounter BottomCenter (0, -120) to the centre lane; PickupPrompt BottomCenter (0, -30) 420 x 56 to the 540 x 60 box plus the hint pill.
- **TV margin:** one setting (HUD margin 32 / 96) moves every edge-anchored offset by 64 px; see the table above and its open collision.
- **Pacing:** a local-player Tally subsystem (proposed `UMordecaiTallySubsystem`, ReceiptTimeline) owns the hold, the receipt, the cap and the spike. It reads state and never changes gameplay. The always-visible widgets bind to GAS attributes directly and are never held.
- **Text stays text:** every string in every region is a real UTextBlock (the agent bridge reads them by name); casing comes from `TextTransformPolicy = ToUpper`.
- **Tokens used by the layout itself:** `space-6` (the 32 px edge margin) and `space-8` (the 96 px TV margin); the Map diagram uses `plate-raised`, `plate`, `rim`, `lip`, `well`, `vital`, `ink`. Every other colour and type token belongs to the component cards. The worlds are core's `.tt-diorama` stand-ins (`world-*`, `biome-*`), never UI.
- **Real data:** spells Q Fireball 3 SP, E Blink 2 SP, R Stone Skin 4 SP, 1 Restoration 3 SP; MaxStamina 134 and MaxSpellPoints 13 at default attributes; Longsword (Slash); Health Potion, Iron Ore, Rusty Sword; Pyromaniac (`fire` glyph, "Burn 6 enemies at once."); Frontliner, Goblin Skirmisher, Corrupted Knight (Elite), Iron Warden.
- **Example data:** Throwing Knife, Antidote and Bandage (QuickItems' examples); both Tally remarks; the Pyromaniac cause line (TallyPlate's proposed `CauseText`); the Iron Warden's phase 2 (no phase system yet); the Goblin's sweep reach (WorldMarkers' Frontliner sweep scaled 1.15); kill counts; the stamina refill in frame 2 (stamina does not regenerate today, N30; SP does not either, N6, so SP stays 5 / 13).
- **TODO(DECISION):**
  - The TV margin collisions (above).
  - SKILL UP tickets after a fight assume ranks that rise from use; today ranks are bought with Allocate (TallyRail).
  - Quick item A's spec key is 1, which casts Restoration today, so Item A shows the dashed unbound cap (QuickItems, US-097).
  - The aim chevron points up the screen while facing is pinned to +X; whether to show it before US-098 (WorldMarkers). Who builds the decals, UI or VFX (WorldMarkers).
  - On gamepad, Inventory has no direct button; the hint's gamepad form is InteractPrompt's call.
  - "Kills: N" is the arena's counter; whether it stays the always-visible rail head in the campaign is open (04).

## Copy

No strings of its own. Every string belongs to a component card and is kept exactly: "Kills: N", "+N", "Perfect Block!", "Parried!", "Hit N", "[F] Pick up {item}" (drawn as an F keycap + "Pick up {item}"), "[I] Inventory" (drawn as an I keycap + "Inventory"), "x / y" SP, the ticket kickers "Skill up" and the slip "Iron Ore x3", "Feat unlocked · Common", the Banner's "Phase 2". Status names show display names ("Stone Skin"); StatusNameText returns the tag segment ("StoneSkin") until US-091.

Tally remarks appear only on tickets and plates printed out of combat (frame 2). None in combat (a capped receipt prints none), none on the boss bar, banners, callouts or status chips.

## Do / Don't

- Do place every region at its map box; a new HUD piece gets a box in this map before it ships, and must not overlap another box at 32 or at 96.
- Do keep the always-visible set small: gauge plate, spell slots, rail head, loadout plate, Inventory hint.
- Do hold tickets and plates during combat; only the callout lane, numbers and the "+N" badge speak, and the Banner keeps its own lane.
- Do drop the plate to (0, 168) whenever the boss bar is up.
- Do size every live number for its longest value so no region changes size while it ticks.
- Do build the HUD behind the pause veil and the death screen from this card's markup (D10).
- Don't push any HUD piece on its own: one root, pushed once on UI.Layer.Game.
- Don't put anything in the centre lane except the callout and the combo; the player's feet are there.
- Don't add HUD text to the world or the world's text to the HUD: enemy tags, damage numbers, pickup labels and world markers stay in world space, and the floor never carries text.
- Don't stack two plates, or print a plate in combat, even at the cap.
- Don't draw a dashed box on the map or the HUD for anything but an unavailable state.
