# Open decisions

Tin Tally takes a position wherever the specs are silent, so the screens can be drawn. Each call below is reversible and is listed here so it can be confirmed or overturned. Everything under "Still open" is drawn as a clearly marked placeholder (`TODO(DECISION)`), and everything under "Also not drawn" has no screen yet.

## Calls this system makes

| Call | Why | Undo cost |
|---|---|---|
| The Dungeon Crawler Carl half is the Tally: an Automation-Age counting machine with a dry voice. There is no game-show audience, no viewers and no loot boxes. | The specs use DCC only for "everything you do is a skill" and achievement-style feats. The world bible is serious. The Ancient Age's still-running systems give DCC's System AI a home in this lore. | Copy only. Remarks are already optional. |
| The HUD becomes one activatable root pushed once on `UI.Layer.Game`; the rail, plate, banner, boss bar, quick items, loadout plate, damage pops and low-health frame are children of its canvas. Only the death screen is pushed onto the same stack. | Today the HUD is added with `AddToViewport`, so it draws over the pause veil and the death screen, and "[I] Inventory" floats over menus. | **Not small.** `UMordecaiCombatHUDWidget` is a `UUserWidget`, and `PushWidgetToLayerStack` needs a `UCommonActivatableWidget`, so the class is reparented to `ULyraActivatableWidget` (not `ULyraHUDLayout`, whose Escape binding pushes Lyra's escape menu, #68). Its spawn moves from `AMordecaiPlayerController` to a layer push; `UMordecaiEquippedWeaponWidget` leaves its own `AddToViewport(50)`; every new HUD region is built as a child, never a push; the HUD must refresh its values when the death screen pops. Every Tier 4 playtest that reads HUD text is re-run. Undoing it later means moving every child back to the viewport and losing the veil and death-screen cover. |
| Every enemy tag shows the enemy's posture lane, from the first hit. | Posture breaks open the riposte window; the player needs to see it coming on any enemy. The specs are silent. | EnemyTag hides the lane by tier. |
| Feat tiers get their own hues (Rare teal, Legendary magenta) and silhouettes (circle, flat-top hexagon, starburst). A feat medallion carries the feat's own glyph. | Gold already means rarity Gold and reward. Feats and items must not share a word for different things. | Theme values and badge textures. |
| Six rarity tiers: Common, Green, Blue, Purple, Red and Gold, with Red and Gold separate. | The code's rarity enum and the inventory colours have six; `item_system_v1` writes the top tier as "Red / Gold". | Theme values and the badge rule (Gold = the full bundle). |
| Health is `vital` rose. Enemy health, stamina Red, the Exhausted debt and errors share `danger`; red means threat only. Player posture broken is `posture` under an `ink` hatch, never red. | Health keeps its own identity next to the stamina tiers. "Red means threat" stays one family. | Theme values. |
| Health shows as a percentage on the HUD ("100%") and as Current / Max on the Character tab. | Pillar 8 and AC-052.2 ask for Health as 0-100% on the HUD; the Character tab is where exact values are read. The specs do not say which the menu uses. | Copy and one TextBlock format. |
| The stamina debt (Exhausted) is left-anchored, its width the absolute Stamina over MaxStamina, on the same scale as the fill. | One scale for debt and fill; the hatch reads as the reverse of the Red tier. The floor is an absolute −50, so the deepest debt is 37% at the default MaxStamina of 134. | One widget (`StaminaDebtBar`). |
| "Kills: N" stays a permanent rail head in the campaign, not only in the arena. | It is the shipping counter the playtests read, and the rail head needs a live count. | The rail head can host another count, or hide outside the arena. |
| The tally mark has exactly three jobs: skill ranks, rarity badges, and the seal / throbber (the throbber on the loading screen only). Everything else counts with pips, bars, numerals or "Hit N". | One motif, one meaning: a gold full bundle must only ever mean rarity Gold. | Per component; the replacements are listed in the README. |
| Selected is a cream `enamel` fill everywhere, with the focus ring added on top. | Tabs, filter chips, categories and cards were drawn four ways. | Theme values and brushes. |
| Focus is ink, focus, ink: the 3 px ring right outside the piece's outline, with a 2 px `ink` keyline outside the ring. | The white ring alone is 1.1-1.3:1 on blockout and snow floors, where front-end tags and pickup labels take focus. | One ring brush (a CommonUI focus brush). |
| Every screen is drawn in one device. Keyboard and mouse is the default; the gamepad version is described beside it. | Mixed keyboard and gamepad caps in one hint bar are wrong for both players. | Preview only; in game, CommonUI swaps the caps by the last input. |
| The pause menu's spike shows the same ticket kinds as the rail, compressed to one line (at most 8 visible, 12 kept). | Pause files the waiting tickets onto the spike, so they are the same objects. | PauseShell only. |
| The Tally holds progress during combat and prints a receipt after it (ReceiptTimeline): the 20 s cap counts from the oldest waiting entry and forces slips and tickets only; pause files tickets to the spike. | Combat stays readable; rewards are never swallowed by an endless fight. | Tuning values in the proposed `UMordecaiTallySubsystem`. |
| Physical, Slash, Pierce and Blunt damage numbers carry no type glyph. | Melee is the common case; the `dmg-physical` colour alone marks it. DamageNumber keeps a `TODO(DECISION)` to review it in busy melee fights. | DamageNumber's stamp rule. |
| Status chips show display names ("Stone Skin"). | Player-facing text should be words, not tag segments. It lands with US-091 and the playtests that compare "StoneSkin". | Coordinated with the playtests. |
| Death speaks in the World voice, centred, without a Tally remark, but in the family's frame (a `foe` inlay mat or two strings, and a stamp-ring countdown). | The tone-bleed rule: death, bosses, biomes and endings keep their weight, without looking like another game's overlay. | Copy and DeathScreen's frame. |
| Rowdies, Atkinson Hyperlegible Next and Atkinson Hyperlegible Mono. | A toy-tin display face, a legibility-first UI face, and digits that never jitter in Slate. | Font assets. |
| The interact prompt draws the key as a keycap in front of its label. Its TextBlock keeps "[F] Pick up …" until device-aware glyphs ship. | The keycap is the look. The string is what the playtests read. | None. |
| **Spell model:** the HUD draws the game's four direct spell slots (Q, E, R, 1). The bindings spec (`control_bindings_v1.1`) instead has two equipped skills, Skill A and Skill B, chosen from a hold radial that does not slow time. | The game ships four direct slots today; SpellSlot draws what plays. | A Skill A / B HUD needs two slots and a new radial component (up to 8 options, open while held, commit on release). The decision stays open with US-097. |
| **Gamepad spell mapping:** the slots show the game's mapping, LB = Q (Fireball), RB = E (Blink), D-pad Up = 1 (Restoration), and an unbound cap on R (Stone Skin). | That is what `MordecaiHeroComponent` binds. The bindings spec puts Skill A on RB, and AC-057.5 gave Spell 3 LB + RB. | Keycap swaps only. US-097 decides. |
| **Front-end flow:** Title → Play → ExpeditionSelect (the playlist cards) → LoadingScreen. Options, Credits and a quit Dialog hang from the title rail. Lyra's hosting, joining, replay and ADD PLAYER screens, its logo, machine name, online ID and build stamp are removed from the front end. | Issues #57-#61 leave "what Play does" open, including booting straight into the arena. This flow reaches the arena through a Mordecai screen and none of Lyra's. | TitleScreen and ExpeditionSelect; booting straight into the arena drops ExpeditionSelect. |
| **Single-player scope:** no party frames, lobby, matchmaking or character select are drawn. | The game is played solo today, although `ue5_tech_stack_context` specifies PlayFab lobby, party and matchmaking. | Party frames need a new box on the HUD map; lobby and character select are new front-end screens. |
| **Feat text fields** proposed once, on `UMordecaiFeatDataAsset`: `CauseText` (the plate's cause line), `DrawbackText` (the Drawback box) and a `DisplayLabel` per unlock condition (the ticket footer and the Feats tab's condition bar). | TallyPlate, FeatsTab and TallyRail each needed player-facing feat text and proposed different fields. | Data fields; until they exist the screens fall back to the condition in plain words or the Description. |

## Still open (TODO(DECISION))

- **Level and XP.** There is no level attribute or XP curve. The Character tab shows the Level slot and the LEVEL UP plate is drawn as a placeholder.
- **Use-based skill ranks.** Ranks are bought with Allocate today. If skills level from use, which is the DCC promise, SKILL UP tickets become the main stream. If they don't, tickets print only on milestones and allocations, and from the pause menu they go straight to the spike.
- **Identify.** The cost (free, gold, resource or time) and the flow (single, batch, timed queue) are undecided. The Mage Tower screen shows "Free" and single-item Identify. Whether field Identify in the inventory survives is open too.
- **Identify remark.** "It had a name all along." is the one identify remark, but TallyRail's IDENTIFIED ticket carries facts in its footer and no remark. Where it prints, if anywhere, is open.
- **Potion field cap.** Hard cap, soft stock or refill decides what the quick-item count means ("x3" vs "3 / 5").
- **Quick-item keys.** Key 1 is Spell 4 (Restoration) today and Use Item A in the bindings doc, so the HUD shows two "1" keycaps. D-pad Up has the same clash. Stone Skin (R) has no gamepad button.
- **Keyboard tab cycling.** Previous / Next Tab are bound to LB / RB only. Proposal: Q / E (they sit where LB / RB do; Menu input mode blocks casting). Drawn nowhere as bound until it is decided.
- **Player posture break.** Only enemies break today. The player's broken look (`posture` under `ink`) is specified but unreachable, and so is a "Down" variant.
- **More than 16 Spell Points.** The token row holds 10 to 16. Above 16 the gauge falls back to a bar, which breaks "Spell Points are tokens" (US-090).
- **Callout size and the combo slot.** At Large (48 px) and XL (56 px) the callout slot grows to 60 and 68 px and reaches the combo at (0, +184). Move the combo down, or open the gap in the HUD map.
- **World map form** (US-073). The tin tile board is a proposal. A screen-grid map or a room-graph map are the alternatives.
- **Settings tab.** A real Settings tab (the Options sections) or the "Coming Soon" placeholder. Every accessibility setting needs it, and a save slot, before it ships.
- **Device-aware glyphs.** Use CommonInput controller data with Mordecai keycap art for KBM, Xbox and PlayStation. This needs the playtests updated in the same change.
- **Campaign death.** What is lost and where the player respawns. The arena death screen respawns automatically; the campaign variant may need a button.
- **Currency and vendors.** "Bandits drop gold" and `CurrencyProxy` exist, but there is no wallet or vendor screen. Nothing here shows a currency on the HUD.
- **Biomes, areas and bosses in data.** Nothing marks "entered biome" or "entered area", and no boss exists yet; the boss `IntroLine` field is proposed.
- **The Tally queue.** `UMordecaiTallySubsystem` (hold, receipt, cap, spike history) is proposed, not built. A "Hold plates until I pause" option is open.
- **Town management** (US-074). The job, initiative, building-tier and population screens are not drawn. They would reuse the menu shell, ledgers and ServiceScreen.
- **Remark copy.** Every sample Tally line needs a tone read before any is written into data.
- **The Acceptance ending sequence.** Which screens go dark, in what order, and whether the HUD returns afterwards.

## Covered this round

Drawn, so no longer gaps: WorldMarkers (aim, ground-target reticle, telegraphs), Dialogue, ServiceScreen with the Alchemist as its second instance, StatusTooltip, SpellCard, a sprint pip and a Guard broken state on the gauge plate, Controls rows in Options, an inventory search field, a Phase state on the Banner, and a Tool kind on TallyPlate.

## Also not drawn

These are specified or implied in the specs and have no screen yet:
- the equipment panel (US-072: Set 1 / Set 2, MainHand / OffHand / TwoHand);
- quickslot assignment;
- Blacksmith and Clinic services (they would be ServiceScreen instances);
- a quest or objective tracker;
- the controller-disconnected overlay and subtitles (Lyra widgets today, #62 / #68);
- the Control ending screen (only the Acceptance ending's "Count closed." exists);
- a heavy-attack charge indicator;
- a Fire Ward absorb and Stone Skin readout;
- a Rooted break-free prompt.
