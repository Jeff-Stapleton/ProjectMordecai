# HUD and pacing

The HUD is designed at 1920 x 1080 with ScaleToFit, which is 0.666 at 720p. The edge margin is 32 px, or 96 px with the HUD margin option. Every box below is collision-free. Keep them. The HUDLayout card draws all of them together.

## Zoning

- **Left is you:** state, always on.
- **Top right is news:** the Tally's rail.
- **The centre is now:** callouts under the player's feet, where the eye already is.
- **Bottom centre is interaction.**
- **Bottom right is the loadout.**
- **Top centre is for big moments.**

## The map

| Element | Widget (keep the name) | Anchor | Offset | Size | Shown |
|---|---|---|---|---|---|
| Gauge plate | `HealthBar`, `StaminaBar`, `PostureBar`, `SpellPointsBar` inside a new `GaugePlate` | top-left | (32, 32) | 456 x 152 | always |
| Spell slots Q E R 1 | `SpellCooldownA`-`D` | top-left | x 32 / 120 / 208 / 296, y 216 | 78 x 78 (cost rivets rise to y 207, keycaps hang to y 304) | always |
| Status chips | `StatusEffectBar` / `IndicatorContainer` (a WrapBox, max 8) | top-left | (32, 318) | 456 x 112, 2 rows | when any |
| Rail head | new `TallyRailHead` hosting `KillCounter` and the pending badge | top-right | (-32, 32) | 320 x 56 | always |
| Tickets | new `TallyTicketStack` | top-right | (-32, 104) | 320 wide, 8 gap, max 4 | the receipt; pressed pickups at once |
| Plate | new `TallyPlate` | top-centre | (0, 40); (0, 168) under a boss bar | 640 x 128 (184 with a Drawback box) | out of combat only |
| Boss bar | new `BossBar` | top-centre | (0, 32) | 880 x 96 (x 520-1400) | boss fights |
| Banner | new `Banner` | top-centre | (0, 296) | 928 x 132 | entering a biome, boss intro, boss phase |
| Combat callout | `CombatFeedback` | centre | (0, +128) | 520 x 52 | 1.0 s |
| Combo | `ComboCounter` | centre | (0, +184) | 220 x 36 | during the combo window |
| Interact prompt | `PickupPrompt` (`PromptText`) | bottom-centre | (0, -36) | 540 x 60, tag right-aligned | when something is focused |
| Inventory hint | `PickupPrompt` (`InventoryHintText`) | docked 12 px right of the prompt box | x 1242-1393, y 994-1034 | 151 x 40 | always |
| Quick items | new `QuickItems` (Throw + Item A-D) | bottom-right | (-32, -132) | 336 x 56 (keycaps perch to y 868) | when stocked |
| Loadout plate | `UMordecaiEquippedWeaponWidget` | bottom-right | (-32, -32) | 340 x 84 | always |
| Low-health frame | new `LowHealthFrame` | full screen | — | 8 px band | Health 25% or below |

World space (no box; they follow their actors):

| Element | Widget | Where | Size | Shown |
|---|---|---|---|---|
| Enemy tags | `UMordecaiEnemyIndicatorWidget` (world) | Z +120 over the enemy | 150 wide | after aggro or damage; Elite and up always |
| Damage numbers | `UMordecaiDamagePopComponent`, drawn by pooled UMG pops on the HUD root | the hit point, fanning up and out under the tag | `pop` 30 px, crit 45 | 1.5 s |
| Pickup labels | new, replaces the TextRender `NameLabel` | on a string over the item | — | near; focused when F will take it |
| World markers | WorldMarkers card (aim, ground target, telegraphs) | on the floor and at the screen edge | see the card | see the card |

The player is the screen centre (960, 540): the camera is fixed and follows the player, so the callout at (0, +128) always lands under their feet.

**Always visible during play:** the gauge plate, the spell slots, the rail head, the loadout plate and the "[I] Inventory" hint pill. The hint is part of the bottom-centre unit and never moves, with or without a prompt. Everything else is contextual. The always-visible pieces keep updating in combat: they are state, not news.

## Layering

- **One HUD root.** `UMordecaiCombatHUDWidget` becomes an activatable widget and is pushed ONCE on `UI.Layer.Game`.
  - Every region in the map is a child of its root canvas: the gauge plate, spell slots, status chips, `TallyRailHead`, `TallyTicketStack`, `TallyPlate`, `Banner`, `BossBar`, the callout and combo, the prompt and hint, `QuickItems`, the loadout plate (`UMordecaiEquippedWeaponWidget`, today a separate `AddToViewport(50)`), the damage-pop canvas and the low-health frame.
  - None of them is pushed on its own. A layer is a stack that shows only its top widget, so a second push would hide the HUD.
- **The death screen** is the only other widget pushed onto `UI.Layer.Game`. It becomes the top of the stack, so the HUD (and the "[I] Inventory" hint) hides while the player is dead and returns when the death screen is removed.
- **Menus and dialogs** draw over the HUD on their own layers: the pause menu, Options and service screens on `UI.Layer.Menu`, dialogs on `UI.Layer.Modal`. Their veil covers the HUD, and "[I] Inventory" no longer floats over menus.
- **Town dialogue** (Dialogue) goes on `UI.Layer.GameMenu`, under `UI.Layer.Menu`, so the pause menu can open above it; the service screens it leads to (MageTower, ServiceScreen) push on `UI.Layer.Menu`, as the pause menu does.
- The old screen-space `WBP_EnemyHealthBar` retires once enemy tags ship (US-092).
- How to build this is in Building it in Unreal; what it costs is in Open decisions.

## Low health

At 25% Health or below:
- `HealthPercentText` turns `danger-text`;
- the gauge plate's rim pulses at 1 Hz (LowHealthPulse);
- the edge frame shows: an 8 px opaque `vital` band round the screen edge with a 3 px `ink` line along its inner side. It does not pulse. It is not a plate, so it has no rim and no print. It is HitTestInvisible and lives on the HUD root canvas;
- the HP bar's 25% notch is the fourth carrier.

## The Tally's pacing

ReceiptTimeline is the source of truth for the Tally queue: its timer table and state table win over any summary, this one included. TallyRail and TallyPlate draw what it prints.

**Lanes.**
- **Callout:** centre, never queued, always immediate. Damage numbers too.
- **Ticket:** the rail (slips and tickets).
- **Plate:** top centre, one at a time.
- **Banner:** biome entry, boss intro and boss phase. Its own lane: the hold does not delay it, and plates wait for it.

**Combat hold.** The player is "in combat" when they dealt or took damage in the last 4 s, or an aggroed enemy is within 1500 uu.
- While held, only callouts, damage numbers and the rail's "+N" pending badge change. The badge counts entries after merging and shows the number alone ("+3").
- A pickup the player pressed F for still prints at once, for 2 s: a slip if it is Common, an ITEM ticket if it is Green or above (or too long for a slip). It is not printed again later.

**Receipt.** After 2.5 s of calm, waiting entries print 350 ms apart: slips, then tickets, then plates one at a time. Any hit or new aggro during the calm returns to the hold; what has not printed goes back to waiting.

**Hard cap.** 20 s after the OLDEST waiting entry, if the player is still in combat, its slips and tickets print anyway, 350 ms apart, with no Tally remarks (remarks are never in combat). The cap never forces a plate: plates keep waiting for calm, so a big moment never lands mid-fight. A hold with nothing waiting forces nothing.

**Pause.** Opening the pause menu files every waiting slip and ticket onto the spike as unread, and the badge clears. They never print afterwards. Waiting plates stay waiting and print after the menu closes, once it is calm; they are on the spike as well.

**On screen.** Slip 3 s (2 s for a pressed pickup during the hold), ticket 5 s, plate 4 s, Legendary plate 5.5 s, banner 1.5-3.5 s (Banner's states). The Ticket duration setting multiplies the slip and ticket times by ×1, ×1.5 or ×2.

**Merging.**
- Identical items add counts ("Iron Ore x3").
- Rank-ups of one skill keep the latest rank, and the ticket's footer reads "+2 ranks".
- A feat's unlock replaces its progress ticket.
- Two different auto-stored resources within 4 s become one STORED ticket; a third starts another.
- Merging never grows the badge.

**Overflow.** At most 4 pieces hang. When more than 4 would hang, 3 hang and the 4th place is the stub "N more on the spike".

**The spike.** The pause menu keeps the last 12 entries, newest first. It draws the same ticket kinds as the rail, compressed to one line: the stub colour and the kicker vocabulary stay (SKILL UP, ITEM · PURPLE, STORED, FEAT PROGRESS, IDENTIFIED). At most 8 show; the rest scroll. Selecting one jumps to its tab, so nothing can be missed.

**Tally notifications** (Settings > Accessibility; default Full):

| | Full | Calm | Minimal |
|---|---|---|---|
| Pickup slips | Always (2 s during the hold) | Out of combat only | Spike only |
| Tickets and the "+N" badge | Receipt | Receipt | Spike only; no badge |
| Plates | 4 s / 5.5 s | 4 s / 5.5 s | 4 s / 5.5 s |
| Tally remarks | If "Tally remarks" is On | Off | Off |
| Spike | Everything | Everything | Everything |

Callouts, damage numbers and banners are not Tally lanes and ignore this setting.

**Sounds.** Each lane has its own sound, so the stream is legible without looking: a slip tick, a ticket print, a plate stamp, a negative thunk, and the receipt run. The banner has its own low cue.

**Ranks today.** Skill ranks are bought with skill points (Allocate) in the pause menu. A rank bought there is filed straight onto the spike, because the HUD is under the veil; its Technique plate prints after the menu closes. Use-based levelling is `TODO(DECISION)`; if it lands, SKILL UP tickets become the main stream.

## Combat callouts and numbers

- **Callouts** are letters only (`callout`, 4 px ink outline) under the player's feet: `good` for Perfect Dodge / Perfect Block / Parried / Posture Broken, `danger-text` for Parry Failed.
  - The latest replaces the previous one. They never queue.
  - High Contrast puts a plate behind them.
  - Callout size: Normal 40, Large 48, XL 56 px. `TODO(DECISION)`: at Large and XL the callout slot grows to 60 and 68 px high and would reach the combo slot at (0, +184); the combo has to move down by the same amount, or the map has to open the gap.
- **Combo:** "Hit N" on a small plate in `good`, during the combo window. No strokes, here or under the weapon name.
- **Damage numbers** show the Health actually taken.
  - Evaded and parried hits show no number.
  - Blocked hits are 80% scale with the `block` glyph.
  - Crits are 1.5x with a starburst and "!".
  - Every non-Physical number carries its type glyph; Physical numbers carry none.
  - Damage numbers setting: All / Mine / Off.
