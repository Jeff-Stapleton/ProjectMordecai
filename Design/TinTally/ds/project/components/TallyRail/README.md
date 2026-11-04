# TallyRail

The Tally's news lane at top right: the rail head with "Kills: N" and the tickets threaded on one light cord under it. Use it for every small piece of progress (pickups, rank-ups, feat progress, stored resources, items, identifications); big moments go to TallyPlate, and the pacing rules are ReceiptTimeline's (the source of truth).

## Anatomy

| Part | Spec |
|---|---|
| Rail head | `.tt-plate.tt-plate--rail` (Plate): 320 x 56, anchor top-right (1, 0), offset (-32, 32), alignment (1, 0). `plate` fill, `rim` / `rim-shade` 2 px, radius 12, 3 px `ink` outline (4 in High Contrast), print (0, 4). Row padding 0 8 4 14. No rivets (a HUD plate). Always visible. |
| Kills | KillCountText "Kills: 12" in `num-l` (Mono 700, 30/34), `text`, left-aligned in a fixed 190 px box (sized for "Kills: 999"). |
| Pending badge | Shown while entries wait (the combat hold) and while the receipt counts them down. An `enamel` pill, 32 high, 2 px `ink` border, padding 0 8, holding the number only: "+3" in `num-m` `ink`, centred in a fixed 46 px box ("+99"); 66 px wide in all. No tally bundle: the tally mark has exactly three jobs (ranks, rarity, the seal and throbber). Right end of the row. |
| Ticket slot | A 240 x 6 `ink` slit cut through the bottom rim, starting at x 10, radius 3 on top, with a 2 px `lip` edge above it. While a receipt prints, a 3 px `enamel` sliver shows in it. The cord drops out of its left end. |
| Cord | Always the light cord: Tag's `.tt-tag-stack.tt-tag-stack--light`, built from core `.tt-string--light` (a 4 px `rim` core with 1 px `ink` sides), x 20-24, centred on the eyelets at x 22. TallyRail.css also makes any stack under a rail head the light cord, so a copy that forgets the modifier still holds. A 2 px `ink` string vanishes on night (1.2:1) and mire (2.5:1) floors; the light cord holds on every floor (the `rim` core on dark ground, the `ink` sides on light ground). It runs from the slot, behind the tickets (visible in the 8 px gaps), and crosses each face from its top edge into its eyelet. |
| Stack | From screen y 104 (16 px under the head), 8 px gaps, right-aligned, 320 wide, at most 4 pieces (lowest edge about y 480). Newest on top. |
| Ticket body | `.tt-tag` (Tag) + `.tt-ticket`: `enamel` face, radius 4 at the eyelet end and 10 at the far end, 3 px `ink` outline, print (0, 4). Text is `ink` or `ink-muted` only. |
| Stub and eyelet | The left 44 px, filled with the kind colour (see Kinds). The core grommet is punched through it, centred at x 22: a 12 px `well` hole, a 3 px `enamel-shade` ring and a 2 px `ink` ring (22 px across). A perforation follows at x 44: 2 px `ink-muted`, 4 on / 4 off, so the stub edge still reads in High Contrast, where `rarity-common` and `feat-common` are near-white. |
| Slip, 56 px | Common pickups. Glyph (24, `ink`) at x 54, then the name in `ui-title` (Next 700, 26/32, one line) in a 176 px box, then the count in `num-s` (Mono 600, 20/24) in a fixed 38 px box ("x99"), or a 32 px rarity socket (182 px name box) when the item does not stack. Right padding 16. |
| Ticket, 88 px | Text column x 58-306 (248 px), right padding 14. Row 1: kicker (`kicker`, Rowdies 400, 20/24, tracked 0.08 em, ToUpper, `ink-muted`) with an optional end item (rarity socket, or the SKILL UP rank); it keeps 14 px clear on the right for the peel. Row 2: the line in `ui-title`. Rows centre in the 88 without a footer; with one they sit from y 4 and end at y 60. |
| Footer band | Optional, 28 px, from x 46 to the right edge: `enamel-shade`, a 2 px dashed tear line on top, `ui-label` (Next 600, 20/24) in `ink-muted`. It holds the plain detail line; when a kind has none and remarks are On, it holds the Tally remark, which starts with the 20 px `seal` glyph. Text box 248 px, 222 px after the seal; the text ellipsizes inside it. |
| Peel | The newest ticket prints with its top-right corner peeled: a 20 px square, the corner half `ink` (the void), the other half `enamel-shade` (the flap) with 2 px `ink` legs. The corner radius drops to 3 while peeled. |
| Overflow | "6 more on the spike": a 48 px `plate` stub (2 px `lip` border, 3 px `ink` outline, tag corners), `pin` glyph at x 10 (centre x 22, where the cord ends), count in `num-s` `text`, words in `ui-label` `text-muted`. It takes the 4th place. |

Text fit (every width below was measured in the shipped fonts at 1920 x 1080):

| Where | Box | Rule |
|---|---|---|
| Slip name | 176 px (182 with a socket) | One line, never cut. A pickup whose name is wider prints as an ITEM ticket instead. "Health Potion" is 166 px. |
| SKILL UP name | 248 px (the rank sits on row 1 in a 108 px box) | One line. Wider names scale down in a ScaleBox (Stretch ScaleToFit, StretchDirection DownOnly) to a 22 px floor, which takes names up to 293 px at 26; past that, ellipsis. Every v1.1 skill fits: the longest, "Two-Weapon Fighting", is 264 px and lands at 24 px. Row 1 holds "SKILL UP" (106 px) + 8 + the rank box (108 px; "20 / 20" is 106) inside 234 px. |
| FEAT PROGRESS name | 248 px minus the count and 8 (164 px beside "4 / 6", 134 beside "50 / 50") | One line, same ScaleBox with the 22 px floor, then ellipsis. "Pyromaniac" is 142 px. Keep feat display names to about 158 px at 26 px so they fit beside any count. |
| ITEM name | 248 px, two lines at 30 px leading | AutoWrapText in a 60 px SizeBox, OverflowPolicy MultilineEllipsis. "Unidentified Purple Amulet" (333 px) wraps after "Purple". A cut never loses the rarity: the kicker repeats the colour word. |
| IDENTIFIED name | 248 px | One line, ScaleBox to 22 px, then ellipsis. "Mysterious Amulet" is 229 px. |
| STORED rows | 202 px name + 38 px count | Two rows at most; a third resource starts a new STORED ticket. |
| Footer | 248 px (222 after the seal) | One line, OverflowPolicy Ellipsis. Remarks are written to fit 222 px; detail lines to fit 248 ("Burn 6 enemies at once." is 223 px). |

## States

The queue rules below are ReceiptTimeline's, restated for this component; where they differ, ReceiptTimeline wins.

| State | Trigger | What shows |
|---|---|---|
| Idle | Nothing waiting | The rail head with Kills. No badge, no tickets, no cord. |
| Pickup, out of combat | Player picks up an item | A slip (Common) or ITEM ticket (Green and up, or too long for a slip) prints at once. Identical slips merge while showing: "Iron Ore x3". |
| Combat hold | Damage dealt or taken in the last 4 s, or an aggroed enemy within 1500 uu | Earned entries wait. The badge shows how many, after merging ("+3"). A pickup the player pressed F for still prints at once for 2 s in its usual form (a slip if Common, an ITEM ticket if Green or up) and is not printed again. The rail head itself does not change: the badge is the hold's only signal, so there is no "held" class or look. |
| Receipt | 2.5 s of calm | Waiting entries print 350 ms apart, slips then tickets; the slot shows paper; the badge counts down and goes. Plates follow on TallyPlate, one at a time. |
| Capped | 20 s after the OLDEST waiting entry, still in combat | Its slips and tickets print anyway, 350 ms apart, without remarks. Plates are never forced. |
| Paused | The pause menu opens | Every waiting slip and ticket is filed onto the spike, unread; they never print afterwards. The badge clears. |
| New | A ticket just printed | The peeled corner, until the next ticket prints or 1.5 s pass. |
| Merged | An identical entry arrives while one is waiting or showing | One ticket: counts add ("x3"); rank-ups of one skill keep the latest rank and the footer reads "+2 ranks" (plain detail, so it replaces a remark). The badge does not grow. |
| Overflow | More than 4 pieces would hang | 3 tickets + "N more on the spike" in the 4th place. |
| Tally remarks Off | Settings > Accessibility > Tally remarks | Footers that held a remark disappear; row 1 and row 2 centre in the 88. Plain detail footers ("+2 ranks", conditions, rarity and type) stay. |
| Tally notifications Calm / Minimal | Settings > Accessibility > Tally notifications | Calm: no remarks, slips only out of combat. Minimal: plates only; slips and tickets go to the spike, no badge. Full (the default) shows everything. |
| Stopped | Acceptance ending | The rail head takes Plate's `.is-unlit`; the count stays; nothing prints again. The last ticket is on TallyPlate. |

Kinds (the stub colour is the kind; the kicker word carries the meaning, so colour is never the only signal):

| Kind | Size | Stub | Row 1 | Row 2 | Footer |
|---|---|---|---|---|---|
| Slip | 56 | `rarity-common` | none | glyph + "Iron Ore" + "x3", or a socket | none |
| SKILL UP | 88 | `good` | "Skill up" + rank "6 / 20" (`num-m`, right) | "Dodging" | "+2 ranks" when merged, else the remark if On |
| FEAT PROGRESS | 88 | `feat-common` / `feat-rare` / `feat-legendary` | "Feat progress" | "Pyromaniac" + "4 / 6" | the condition's `DisplayLabel`, plain: "Burn 6 enemies at once." |
| STORED | 88 | `rarity-common` | "Stored" | two rows: "Iron Ore" x2, "Town Stone" x1 | none |
| ITEM | 88 | rarity colour | "Item · Purple" + socket | "Unidentified Purple Amulet" (wraps) | none |
| IDENTIFIED | 88 | rarity colour | "Identified" | "Mysterious Amulet" | socket + "Purple · Magical Item" |

This table is the ticket copy canon (01 Voice and copy follows it). The pause menu's spike (PauseShell) shows the same kinds compressed to one line: the same stub colour and the same kicker vocabulary (SKILL UP, ITEM · PURPLE, STORED, FEAT PROGRESS, IDENTIFIED), at most 8 visible, 12 kept.

Motion (cited by name from the brand book's Motion table; no local timings):
- **Print:** TicketDrop, about the eyelet. One print tick sound per piece.
- **Leave:** TicketRetract, into the slot.
- **Hang times:** slip 3 s, ticket 5 s (2 s for a pressed pickup during the hold). These are the base times the Ticket duration setting multiplies (×1 / ×1.5 / ×2). Tunable.
- **Badge:** the number changes in place (fixed box); the badge shows when the hold starts with something waiting and hides when it reaches 0, with no swing.
- **Reduce Motion:** TicketDrop and TicketRetract become the system's fades.

## Data and implementation

- **Keep:** `UMordecaiKillCounterWidget`, slot `KillCounter`, text `KillCountText`, format `"Kills: %d"`. It shows `AMordecaiGameMode::GetEnemyKillCount()` (the bridge's `player.kills`) and follows its `OnEnemyKillCountChanged` delegate: it goes up when the player kills an enemy and back to 0 when the player dies (issue #6, US-086; rules in `Specs/design_decisions_log.md` 2026-11-04).
- **Move:** the KillCounter slot from TopRight (-170, 20) 150 x 24 into the new rail head at (-32, 32), 320 x 56, alignment (1, 0). Binding is by name, so it may sit inside the decor widget.
- **Where it lives:** `TallyRailHead` and `TallyTicketStack` are children of the HUD root canvas (`UMordecaiCombatHUDWidget`, reparented to `ULyraActivatableWidget` and pushed ONCE on `UI.Layer.Game`). Never push them on their own: a second push on a layer stack hides the HUD. The pause veil and the death screen cover them with the rest of the HUD; the rail head is always visible during play.
- **New optional widgets (BindWidgetOptional on the HUD, C++ fallbacks like the rest; names from Building it in Unreal):**
  - `TallyRailHead`: the plate stack, `KillCounter`, `PendingBadge` (a pill holding `PendingText` `"+%d"`), `TicketSlot` (image).
  - `TallyTicketStack`: a VerticalBox (8 px spacing) at (-32, 104), plus `TicketString` (the light cord, behind it).
  - `UMordecaiTallyTicketWidget` (pooled, 4 + the overflow): `StubImage` (tinted), `KindGlyph`, `KickerText`, `LineText` (inside `LineScaleBox`), `NumberText`, `RaritySocket`, `FootText`, `RemarkSeal`, `PeelImage`.
- **Assets (03, the asset manifest):** the ticket is the **Tag stack** (RoundedBox overlay with the grommet) plus a stub tint layer (a RoundedBox with left corner radii only) and the perforation as a tiled 2 x 8 Image; the rail head is the **Plate stack**; the cord is the **Strings** light cord (a 4 px `rim` Image between two 1 px `ink` Images). Pivot each ticket at its eyelet for TicketDrop; the cord is never a child of the ticket, so it stays plumb while the ticket swings.
- **Queue:** a local-player Tally subsystem (proposed `UMordecaiTallySubsystem`) owns pending entries, merging, the combat hold, the receipt, the cap and the spike history. It only reads state; it never changes gameplay. Rules: ReceiptTimeline.
- **Sources:**
  - Pickups: `UMordecaiPickupInteractionComponent` / `UMordecaiInventoryComponent::PickupItem` (slip or ITEM by the fit rule); auto-stored resources: `UMordecaiResourceLedger::OnResourceChanged` (slip, or STORED when 2 different resources arrive within 4 s).
  - Feat progress: the condition values of `UMordecaiFeatDataAsset::UnlockConditions` (StatThreshold, EventCount, SkillRank).
  - Identified: `UMordecaiIdentificationService` `OnItemIdentified`.
  - Names come from `UMordecaiItemLibrary` (identification-aware: "Unidentified Purple Amulet", "Unknown Item").
- **TODO(DECISION):**
  - SKILL UP: ranks are bought with Allocate in the Skills tab today, and skills do not level from use. There is no rank-changed event, only `Mordecai.Event.SkillMilestone`. Until ranks arrive as gameplay events, a rank bought in the menu is filed straight to the spike (the HUD is under the pause veil).
  - FEAT PROGRESS footer: the condition's player-facing `DisplayLabel`, one of the three feat text fields proposed once on `UMordecaiFeatDataAsset` (`CauseText`, `DrawbackText`, a `DisplayLabel` per unlock condition; all proposed). Until it exists, the ticket prints with no footer: the Feats tab format "EnemiesBurnedSimultaneously: 4 / 6" is 341 px and does not fit. When it prints is also open; proposal: on a new best at 50% of the threshold or more, at most once per fight.
  - IDENTIFIED: identification happens in the pause menu today, so the ticket is filed to the spike. Once the Mage Tower screen exists, it prints on the rail when that screen closes.
- **Text stays text:** every string is a real UTextBlock so the bridge can read it. Each ticket's accessible text is kicker + line + number (+ footer), for example "Skill up. Dodging, 6 / 20, +2 ranks." STORED reads "Stored: Iron Ore x2, Town Stone x1". The badge reads "3 waiting".
- **Tokens:** `plate`, `rim`, `rim-shade`, `ink`, `lip`, `enamel`, `enamel-shade`, `well`, `text`, `text-muted`, `ink-muted`, `good`, `feat-common`, `feat-rare`, `feat-legendary`, `rarity-*`, `num-l`, `num-m`, `num-s`, `kicker`, `ui-title`, `ui-label`, `radius-plate`, `radius-tag`, `stroke-rim`, `space-2`.

## Copy

Shipping, kept exactly: "Kills: N" (KillCountText, `"Kills: %d"`). Ledger counts follow the ledger's "x1" form.

Kickers (sentence case in data, ToUpper on screen): "Skill up", "Feat progress", "Stored", "Item · {Rarity}", "Identified".

Lines and numbers: "Dodging" + "6 / 20" (rank as "rank / 20"); "Pyromaniac" + "4 / 6"; "Iron Ore" + "x3"; "Unidentified Purple Amulet"; "Mysterious Amulet"; "{N} more on the spike"; the badge "+{N}". Footer details: "+{N} ranks", "Burn 6 enemies at once.", "Purple · Magical Item".

Settings named here (OptionsScreen's Accessibility table is the single list): "Tally notifications" Full / Calm / Minimal, "Tally remarks" On / Off, "Ticket duration" ×1 / ×1.5 / ×2.

Tally remarks (optional second line, Settings > Accessibility > Tally remarks):
- At most 9 words, and they must fit the footer: 222 px after the seal, about 22 characters at 20 px. Examples (example copy, awaiting a tone read): "Six. Up from five." (159 px), "Three. Up from two." (181 px).
- Only on SKILL UP tickets that have no plain detail. Never on FEAT PROGRESS, STORED, ITEM or IDENTIFIED: their footers carry facts.
- Never in combat: a receipt forced by the 20 s cap prints without remarks.
- Deadpan and literal; the Tally cares about the count, not about you. Never about a failure, death, story, NPCs or the Lich King. Never on combat callouts. No "Filed under ..." jokes.
- The headline must make sense with remarks Off.

## Do / Don't

- Do keep every piece 320 wide and right-aligned under the head; the cord must meet every eyelet at x 22.
- Do use the light cord for the rail on every floor; never the 2 px ink string.
- Do merge identical entries instead of stacking duplicates.
- Do size KillCountText (190 px), PendingText (46 px) and the SKILL UP rank (108 px) for their longest values so nothing shifts as they tick.
- Do keep plain detail (merges, conditions, rarity, type) over a remark when both want the footer.
- Don't draw tally strokes on the badge: it shows the number only.
- Don't ellipsize a slip: send a long name to an ITEM ticket.
- Don't put text on the stub or on the floor; stubs are colour only.
- Don't hang more than 4 pieces, and don't let the rail print during the combat hold except pressed pickups and the 20 s cap.
- Don't print tickets that were filed on the spike during a pause.
- Don't use the rail for big moments (feat unlocked, technique, upgrade key, tool): those are TallyPlate.
- Don't colour ticket text by rarity; on enamel it is `ink`, and rarity rides on the stub, the socket and the word.
