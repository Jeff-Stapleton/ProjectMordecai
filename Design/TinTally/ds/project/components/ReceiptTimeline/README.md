# ReceiptTimeline

The Tally's pacing rules drawn as one fight on a printed receipt, and the source of truth for them: the combat hold, the "+N" badge, the receipt, the 20 s cap, overflow, pause and the Tally notifications setting. Use it as the reference for the Tally queue that feeds TallyRail and TallyPlate; where any other card or section summarises these rules differently, this card wins. It is a spec diagram, not a screen in the game.

## Anatomy

The diagram (preview):

| Part | What it shows |
|---|---|
| Head | A menu plate with the title and a slot along its bottom rim; the receipt paper comes out of it (`enamel`, `ink` text only, 3 px `ink` outline, print (0, 6)). |
| Lanes | One row per lane, split by 2 px dashed rules in `ink-muted` mixed 65% into `enamel` (3.1:1 in Diorama, 4.3:1 in High Contrast, where `enamel` and `enamel-shade` are both white). Lane names in `kicker` `ink-muted`. |
| Seconds | The time axis: a tick per second, a label every 2 s (`num-s`). Marks are placed with `--t` (seconds) and `--d` (duration) at `--pps` 40 px per second. |
| Hits | One tick per damage event (a timeline mark, not a tally stroke): dealt in `ink`, taken in `vital` with an `ink` edge. |
| Earned | Numbered `ink` stamps (28 px, radius 4, `text` numeral in Mono 700) where something was earned; the legend under the paper says what. They are not keycaps: a keycap "1" would read as the 1 key. |
| Badge | The rail's pending badge (an `enamel` pill, 2 px `ink` border, the number only: "+3") at each change; the line shows how long it is visible. |
| Hold | The combat hold as an `ink-muted` hatch, then the calm in `heal`. Dimension brackets with their notes under them mark "4 s, no hits" and "2.5 s". |
| Slips / Tickets / Plate | When each piece is on screen: a pressed slip (2 s, during the hold), two tickets (5 s, 350 ms apart; the merged one reads "Dodging, +2 ranks"), the plate (4 s, Pyromaniac with its own `fire` glyph). |
| Guides | Dashed `ink` lines across all lanes at the last hit, the end of the hold, and the receipt; their notes sit in their own bottom row. |
| Notes | Words on the paper are plain `ink` text knocked out of what they sit on by an `enamel` ground: no outline, no rounded chip or pill shape, so a note never reads as game UI. |
| Lower plates | Print order with the real tickets (TallyRail's classes) at +0 / +350 / +700 ms; pause; the hard cap as a 30 s fight; overflow with the real "6 more on the spike" stub; and the Tally notifications table with Full selected (core Selected: the cream fill) and focused (the ring on the tab's border). |

### The pacing system

Lanes:

| Lane | Where | Waits for the hold? |
|---|---|---|
| Callout | Centre (0, +128), letters only | Never. Never queued; the newest replaces the last. |
| Damage numbers | World | Never. |
| Ticket (slips and tickets) | The rail, top right | Yes, except pickups the player pressed for, and the 20 s cap. |
| Plate | Top centre (0, 40) | Yes, and the cap never forces it. |
| Banner | (0, 296) | No: it marks a place, starts a boss fight or announces a boss phase. Plates wait for it. |

Timers and rules (tunable values):

| Rule | Value |
|---|---|
| In combat | The player dealt or took damage in the last 4 s, or an aggroed enemy is within 1500 uu. |
| While held | Only callouts, damage numbers and the rail's "+N" badge change. The always-visible pieces (gauge plate, spell slots, rail head, loadout plate, the "[I] Inventory" hint) keep updating: they are state, not news. |
| Pressed pickups | A pickup the player pressed F for prints at once, even in the hold, for 2 s: a slip if it is Common, an ITEM ticket if it is Green or up. It is not printed again. |
| Calm | 2.5 s out of combat before the receipt prints. A hit or new aggro during the calm returns to the hold. |
| Print spacing | 350 ms between pieces: slips first, then tickets, then plates one at a time. |
| Hard cap | 20 s, counted from the OLDEST waiting entry. If the player is still in combat then, the waiting slips and tickets print anyway, 350 ms apart, with no remarks. It forces slips and tickets only: plates are never forced. A hold with nothing waiting forces nothing. |
| Pause | Opening the pause menu files every waiting slip and ticket onto the spike, unread. They never print afterwards; the badge clears. Waiting plates print after the menu closes, once it is calm (they are on the spike too). |
| On screen | Slip 3 s and ticket 5 s (TallyRail's base hang times; 2 s for a pressed pickup during the hold), multiplied by Ticket duration ×1 / ×1.5 / ×2. Plate 4 s, Legendary feat plate 5.5 s. Banner 1.5-3.5 s (Banner's states). |
| Merging | Identical items add counts ("Iron Ore x3"); rank-ups of one skill keep the latest rank and the ticket footer reads "+2 ranks"; a feat's unlock replaces its progress ticket; two different auto-stored resources within 4 s become one STORED ticket (a third starts another). Merging never grows the badge. |
| Overflow | At most 4 pieces hang. When more than 4 would hang, 3 hang and the 4th place is the stub "N more on the spike". |
| Spike | The pause menu keeps the last 12 entries, newest first: at most 8 visible, a ScrollBox for the rest. It shows the same ticket kinds as the rail, compressed to one line. |

## States

The queue's states, in the order of the example fight:

| State | Enter when | Behaviour |
|---|---|---|
| Idle | Nothing waiting, out of combat | Entries print as they arrive (slips and tickets at once, plates one at a time). |
| Hold | In combat (above) | Earned entries wait. The badge shows the count after merging, as the number only ("+3"). A pressed pickup prints now for 2 s in its usual form (a slip if Common, an ITEM ticket if Green or up) and is not printed again. |
| Merge | An entry matches one that is waiting | See Merging above. The badge does not grow. |
| Calm | Out of combat with entries waiting | A 2.5 s countdown; any hit or new aggro returns to Hold. |
| Receipt | The calm completes | Waiting entries print 350 ms apart, each with TicketDrop and a print tick; the slot shows paper; the badge counts down and goes. If combat starts mid-receipt, what has not printed goes back to waiting. |
| Capped | 20 s after the oldest waiting entry, still in combat | Its slips and tickets print anyway, 350 ms apart, without remarks. Plates keep waiting for calm: a big moment never lands mid-fight. |
| Overflow | More than 4 pieces would hang | 3 hang; the 4th place is "N more on the spike". |
| Paused | The pause menu opens | Waiting slips and tickets are filed onto the spike, unread, and never print; the badge clears. Waiting plates stay waiting and print after the menu closes, once calm. |

Tally notifications (Settings > Accessibility; default Full):

| | Full | Calm | Minimal |
|---|---|---|---|
| Pickup slips | Always (2 s during the hold) | Out of combat only | Spike only |
| Tickets and the +N badge | Receipt | Receipt | Spike only; no badge |
| Plates | 4 s / 5.5 s | 4 s / 5.5 s | 4 s / 5.5 s |
| Tally remarks | If Tally remarks is On | Off | Off |
| Spike | Everything | Everything | Everything |

In short: Full shows everything; Calm drops remarks and shows slips only out of combat; Minimal shows plates only and sends tickets to the spike. Callouts, damage numbers and banners are not Tally lanes and ignore this setting.

Motion is cited by name from the brand book's Motion table: pieces print with TicketDrop and leave with TicketRetract; plates use PlateDrop and StampReveal. The 350 ms print spacing above is pacing, not a motion.

## Data and implementation

- **Queue (proposed):** `UMordecaiTallySubsystem`, a local-player subsystem. It owns the pending list, merging, the hold and calm timers, the cap, the receipt and the spike history (last 12). It only reads game state and drives the HUD widgets (`TallyRailHead`, `TallyTicketStack`, `TallyPlate`, children of the one HUD root on `UI.Layer.Game`); it changes nothing in gameplay, so it is safe for the playtest bridge.
- **In-combat signal:** damage dealt or taken by the local player's ASC (the same events that drive damage numbers), timestamped; plus aggro: an enemy AI controller in Approach, Attack or Recover targeting the player within 1500 uu. Out of combat = neither for 4 s.
- **Entries:** { kind, key for merging, count, rank, rarity, text fields, earned time }. The earned time of the oldest waiting entry drives the cap. Kinds and their sources are in TallyRail (tickets) and TallyPlate (plates).
- **Settings:** OptionsScreen's Accessibility table is the single list of names and values. The ones this queue reads: "Tally notifications" Full / Calm / Minimal, "Tally remarks" On / Off, "Ticket duration" ×1 / ×1.5 / ×2 (of TallyRail's base hang times: slip 3 s, ticket 5 s). TODO(DECISION): the Settings tab is "Coming Soon" today, so these need a real Settings tab (and a save slot) before they ship.
- **TODO(DECISION):** a "Hold plates until I pause" option (plates file to the spike instead of printing) for players who want no interruptions.
- **TODO(DECISION):** ranks are bought with Allocate in the pause menu today, so in the current game SKILL UP entries arrive while paused and go straight to the spike. The timeline shows how they behave if ranks arrive in play.
- **TODO(DECISION):** the queue itself, its history and the spike's history system do not exist yet.
- **Spike:** owned by the pause-menu author (PauseShell); it reads this queue's history. Unread entries keep the peeled corner there.
- **Sound:** slip tick, ticket print, plate stamp, and one short run for a receipt. Each lane has its own sound, so a player with the HUD hidden still hears the receipt.
- **Tokens (diagram):** `enamel`, `enamel-shade`, `ink`, `ink-muted`, `vital`, `heal`, `good`, `plate`, `plate-raised`, `rim`, `lip`, `text`, `text-muted`, `danger-text`, `rarity-common`, `feat-common`, `focus`, `tally-heading`, `ui-title`, `ui-body`, `ui-label`, `kicker`, `num-s`, `num-m`.

## Copy

- Setting names and values: "Tally notifications" with "Full", "Calm", "Minimal"; "Tally remarks" with "On", "Off"; "Ticket duration" with "×1", "×1.5", "×2".
- The overflow reads "{N} more on the spike". Badge: "+{N}". A merged rank-up's footer: "+{N} ranks".
- The diagram's own words ("Combat hold", "Calm", "Receipt prints", "4 s, no hits") are documentation, not game strings.
- Remarks follow the Tally voice rules: optional, never in combat (a capped receipt prints none), never on callouts, banners, death, endings or status lines.

## Do / Don't

- Do let callouts and damage numbers through at once, always; the hold only delays the Tally.
- Do print pickups the player pressed for at once, even during the hold. Pressing F must show a result.
- Do merge before counting: the badge counts entries, not events.
- Do count the cap from the oldest waiting entry, not from the start of the fight.
- Do keep everything on the spike, whatever the setting, so nothing can be missed.
- Don't print a plate in combat, even at the cap.
- Don't re-print tickets that were filed on the spike during a pause.
- Don't hang more than 4 pieces: 3 and the "N more on the spike" stub.
- Don't let the receipt run longer than it needs: 350 ms per piece and nothing in between.
