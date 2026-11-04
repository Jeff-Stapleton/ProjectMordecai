# Project Mordecai: planned UI work and known UI problems (research for the UI design system)

Sources read in full:
- `stories/PLAN.md`
- every UI-touching story in `stories/{backlog,in-progress,done}`
- `Specs/agent_playtest_plan_v1.md` (Known issues backlog, lines 174-518)
- `Specs/design_decisions_log.md` (UI entries)
- `stories/PLANNER.md` §5.1
- `stories/PLANNER_LOG.md` (Playable Status lines)
- GitHub issues #6-#71 (`gh issue view`, read-only)

Facts about the as-built code come from `Source/LyraGame/Mordecai/UI/*`.

**No story, plan or known issue mentions Dungeon Crawler Carl, Link's Awakening, loot boxes, an announcer, sponsors, achievements UI, toasts or a minimap.** The brief is not reflected anywhere in the planned work yet.

Today the in-game UI has no style of its own:
- It is native C++ widgets using the engine default font (Roboto) and hard-coded `FLinearColor`s.
- The front end is unmodified Lyra: Orbitron, cyan "blue glass" panels.

Issue #62 calls this "two visual identities".

---

## UI stories

Status words used below:
- **done-old-rules**: closed under the old, PIE-only rules. Per PLAN.md, that does not mean playable.
- **PLAYABLE**: accepted on a packaged build.

### Epic 10, Phase 1: Combat HUD (all done-old-rules)

**US-052 Combat HUD C++ Framework** (done)
- `UMordecaiCombatHUDWidget` is the root container.
- Health bar: Health % (0-100%) of MaxHealth (AC-052.2). `GetHealthPercent` = Clamp(Health/MaxHealth, 0, 1), and 0 if Max is 0.
- Stamina bar: fill plus a tier colour (AC-052.3/052.8).
  - Green ≥ 66%, Yellow ≥ 33%, Red > 0%, Exhausted ≤ 0.
  - Default colours: Green (0,1,0), Yellow (1,1,0), Red (1,0,0), Exhausted (0.5,0,0).
  - Tier multipliers from the design log: Green 1.25x, Yellow 1.0x, Red 0.75x, Exhausted 0.5x.
- Posture bar: Posture/MaxPosture (AC-052.4), drawn differently while `Mordecai.State.PostureBroken` is active.
  - As built: Normal (0.2,0.6,1.0), Broken (1,0.2,0.2).
- `UMordecaiEnemyHealthBarWidget`: a world-space enemy health bar (AC-052.10).
- Design reference cited: game_design_v2 §9, "Always visible: Health%, tiered stamina, spell points"; "Status effects communicated primarily via VFX".

**US-054 Playable Arena Integration** (done, EDITOR)
- AC-054.7: `WBP_CombatHUD` shows Health (top-left), Stamina with tier colour below it, Posture below that.
- AC-054.9: `WBP_EnemyHealthBar` world-space bar above each enemy.
- Tier 4 `hud_layout`: HealthBar top-left (x<300, y<60), StaminaBar below, PostureBar below.
- Tier 4 `hud_visible`: HealthBar, StaminaBar, PostureBar, SpellPointsBar and KillCounter on screen, with text "Kills: 0".

**US-055 Spell HUD & Status Effect Indicators** (done)
- SP bar shows fill plus numeric "12 / 20" (AC-055.1-055.3).
- `StatusEffectIndicator` (AC-055.4): tag/display name, a countdown, and a tint (red debuff, green buff).
- `StatusEffectBar` (AC-055.5-055.7): a horizontal row that adds and removes indicators on `Mordecai.Status.*`.
  - `BuffTags` (e.g. Focused, Blessed, StoneSkin) are green; everything else is a debuff.
- `SpellCooldownWidget` (AC-055.8/055.9): remaining seconds as text plus a radial/linear fill, and a "ready" state.
- Cites game_design_v2 §8: "Status is primarily VFX-driven, not icon soup."

**US-061 Combo Counter & Combat Feedback** (done)
- Combo counter: "Hit 1/2/3" while `Mordecai.State.ComboWindow` is active; it auto-hides.
- Text flashes that fade after about 1 s:
  - Perfect Dodge, Perfect Block, Parried and Posture Broken in gold.
  - Parry Failed in red.
- As built:
  - Texts: "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!".
  - Colours: Gold (1,0.84,0), Red (1,0.2,0.2).

**US-062 Death Screen & Respawn Countdown** (done; wiring was missing until US-086)
- "You Died", a countdown (ceiling integer: 2.5 s→"3", 1.1→"2", 0.5→"1", ≤0→empty) and "Kills: N".
- Input mode GameAndMenu.
- Its "UI.Layer.Modal" note is superseded: the screen now pushes to `UI.Layer.Game`.

**US-063 Kill Counter** (done)
- A persistent counter in the HUD corner: "Kills: 0" / "Kills: 1" / "Kills: 99".
- Increments on `Mordecai.Event.EnemyKill`; resets on `Mordecai.Event.PlayerDeath`.

### Epic 10, Phase 2: world-space indicators (done-old-rules)

**US-064 Enemy Indicator System** (uses the Lyra IndicatorSystem)
- Shows the enemy display name, a health bar (0-1) and a row of status icons.
- Registers on damage or aggro, at `IndicatorHeightOffset` 120 cm.
- Shown only within `IndicatorVisibilityRange` 2000 cm.
- Health colour: Green > 50%, Yellow 25-50%, Red < 25%.
- Status icons come from a `TMap<tag, TSoftObjectPtr<UTexture2D>>`, with a placeholder fallback when a texture is missing.
- One indicator per enemy.

**US-065 Floating Damage Numbers** (Lyra NumberPops / MeshText)

| Setting | Default |
|---|---|
| `PopLifespanSec` | 1.5 |
| `PopRiseSpeed` | 100 cm/s |
| `PopScale` | 1.0 |
| `CriticalHitScale` | 1.5x |
| `PopScatterRadius` | 30 cm |
| `MaxConcurrentPops` | 20 (the oldest is recycled) |

Colours by damage type:

| Damage type | Colour |
|---|---|
| Physical | White (1,1,1) |
| Fire | Orange (1,0.5,0) |
| Frost | Cyan (0.3,0.8,1) |
| Lightning | Yellow (1,1,0.3) |
| Poison | Green (0.3,0.9,0.3) |
| Arcane | Purple (0.7,0.3,1) |
| Holy | Gold (1,0.85,0) |
| Dark | Dark Red (0.6,0.1,0.1) |
| Unknown | Gray (0.7,0.7,0.7) |
| Healing | `HealingColor` (0.2,1,0.2), with a "+" prefix |

### Epic 10, Phase 3: menus and progression (done-old-rules)

**US-069 Pause Menu Framework** (CommonUI `ULyraActivatableWidget`)
- Menu input mode blocks game input and shows the cursor.
- Five default tabs:

| Id | Label |
|---|---|
| `character` | Character |
| `skills` | Skills |
| `feats` | Feats |
| `inventory` | Inventory |
| `settings` | Settings |

- Behaviour:
  - Every tab starts as the placeholder "Coming Soon".
  - LB/RB or clicking a header switches tabs.
  - Esc/Start opens; Back (B/Esc) closes.
  - The game is paused while open (`bCanPauseGame`, default true; multiplayer is a TODO(DECISION)).
  - The last tab is remembered within a session.
  - Opening twice and closing twice are no-ops.
  - API: `RegisterTab(FName, FText, class)` / `UnregisterTab`.
- As built (`MordecaiPauseMenuWidget.cpp`):
  - Full-screen dim (0,0,0,0.65) behind a centred 1000×640 column.
  - Title "Paused".
  - Tab buttons: active (0.25,0.35,0.55), inactive (0.12,0.12,0.15). Labels white when active, 0.7 grey otherwise.
  - Content border (0.05,0.05,0.08,0.9).
  - Pushed on `UI.Layer.Menu`.

**US-066 Character Sheet** (Character tab)
- The 9 primary attributes STR DEX END CON RES DIS INT WIS CHA. Each shows its base value and a "+X%" modifier.
- They sit in 3 columns: Physical (STR, DEX, END), Resilience (CON, RES, DIS), Magical (INT, WIS, CHA).
- Core resources as "Current / Max": Health, Stamina, Spell Points, Posture.
- Primary-derived stats: Physical Damage, Attack Speed, Affliction Resist, Magic Damage, Cast Speed. Formatted "+%.1f%%" of (Value−1)×100.
- Secondary-derived stats: Armor Penetration, Physical Crit Chance, Stamina Regen, Health Regen, Affliction Recovery, Posture Recovery, SP Regen, Resistance Penetration, Magic Crit Chance.
- Shows "--" with no ASC, and a placeholder "Level: --".

**US-067 Skill Tree Display** (Skills tab)
- Five categories: Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic. The player can switch between them.
- Each row: skill name and rank / 20. The milestone ranks 1/5/10/15/20 are visually distinguished.
- Detail panel: the milestone descriptions, with unreached milestones "locked" and "unlocked and active" shown differently. It reads "No data available." when there is no DataAsset.
- Available skill points are shown prominently.
- An "Allocate" action per row, greyed out at 0 points or rank 20.
- No respec.

**US-068 Feat Display** (Feats tab)
- Unlocked list: name, tier badge and a description excerpt.
- Tier colours:
  - Common White/Gray (0.8,0.8,0.8)
  - Rare Blue (0.3,0.5,1.0)
  - Legendary Gold (1.0,0.75,0.0)
- Detail panel: full description, tier, and counts of effects, abilities and drawbacks ("2 Effects, 1 Ability, 1 Drawback").
- Locked section: name visible, description "???" (teaser), tier, and progress lines:
  - "StatName: Current / Threshold"
  - "SkillName Rank: X / Y"
  - "Requires: TagName" with a check or cross
- Summary header, e.g. "Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary".
- Live unlocks move a feat from the locked to the unlocked section.

### Epic 7.5: inventory slice (PLAYABLE, closed at Jeff's call 2026-10-29, run `Saved/Playtests/20261029-175841-us079`, 56/56)

**US-071 Inventory UI** (Inventory tab)
- Rows show icon, display name, quantity (only when stackable and > 1), rarity colour and an identification badge.
- Unidentified names follow `GetDisplayName`:
  - "Unidentified <Rarity> <Subtype/Type>", e.g. "Unidentified Purple Amulet"
  - or "Unknown Item"
- Rarity colours (`MordecaiInventoryWidget.h`):

| Rarity | Colour |
|---|---|
| Common | Gray (0.55,0.55,0.55) |
| Green | (0.10,0.80,0.10) |
| Blue | (0.15,0.40,1.00) |
| Purple | (0.60,0.20,0.90) |
| Red | (0.90,0.10,0.10) |
| Gold | (1.00,0.84,0.15) |

- Filters: All, Weapons, Armor, Trinkets, Consumables, Materials (= Material + TownResource), Quest, Magical (= MagicalItem + UpgradeKey).
- Ledger panel "Stored Resources", entries "{name}  x{n}", sorted alphabetically.
- Identify button on unidentified rows.
- Placeholder texts:
  - "Inventory is empty."
  - "No items match this filter."
  - "No resources stored."
  - "--" when unbound
- Row text as built: "? " prefix for unidentified, "  x%d" quantity, "  [equipped]".
- Out of scope: drop and equip.

**US-079 Playable Inventory Arena**
- Six test pickups: Health Potion, Iron Ore, Town Stone, Upgrade Key, Mysterious Amulet (shows "Unidentified Purple Amulet"), Rusty Sword.
- Pickup prompt "[F] Pick up <name>" and hint "[I] Inventory".
- Each pickup has an in-world billboard name label.
- Minimum polish (AC-079.11): filter buttons visible and clickable, rarity colours, a "?" badge, and the Identify button.
- Deviations accepted at closing:
  - The rarity colour is on the row text, not a background or border.
  - No `BP_MordecaiItemPickup` exists; the native `NameLabel` is used.
  - Press-to-Pickup (F) replaced overlap pickup.

**US-094 Inventory refresh on reopen** (done): the CommonUI pool fix.

### Epic 6.5: weapons (done-old-rules)

**US-077 Equipped Weapon Widget**
- Shows the weapon `DisplayName` and `WeaponType`, or "Unarmed" when nothing is equipped.
- Updates through `OnWeaponChanged`.
- The full "Weapon Set Swap" (Set 1/Set 2) is deferred.

**US-078 Playable Weapon Arena**
- AC-078.11: weapon widget at bottom-center or bottom-right.
- Five weapons: Longsword, Dagger, Greatsword, Spear, Mace.
- Cycling: Tab / Gamepad Y next; Shift+Tab previous (not working, N9); BackSlash previous as built.

### Epic 5.5: magic slice (done-old-rules)

**US-057 Playable Magic Arena**
- AC-057.9: SP bar below the posture bar, a status indicator bar below the SP bar, and 4 spell cooldown indicators (bottom-center or a side panel).
- Spells:

| Spell | Key | SP | Cooldown |
|---|---|---|---|
| Fireball | Q/LB | 3 | 2.0 s |
| Blink | E/RB | 2 | 4.0 s |
| Stone Skin | R/LB+RB | 4 | 15 s |
| Restoration | 1/DPadUp | 3 | 10 s |

- Fresh character: SP "10 / 10".

### Epic R: Playability Recovery (current priority)

**US-086 Death Screen + Kill Counter** (IN PROGRESS)
- Death-screen half done (issue #7):
  - `UMordecaiDeathScreenComponent` pushes onto `UI.Layer.Game`, which is below `Menu` and `Modal`.
  - Full-screen dim (0,0,0,0.5).
  - "You Died" 48 pt red (0.8,0.1,0.1); countdown 32 pt white; "Kills: N" 20 pt white.
  - HitTestInvisible.
  - The countdown holds while the pause menu has the world paused.
- Kill-counter half (AC-086.3/086.5, issue #6) still open. Goal: "Kills: 1" top-right after killing the TestDummy.
- Remote clients see no death screen.

**US-095 Pause Menu Input** (IN PROGRESS; every US-095 check PASSes, but us069 exits 1 on N4/N5)
- I closes the menu on the key-up of a press made inside the menu.
- LB/RB cycle tabs with wrap (Character ↔ Settings).
- Done through UI action tags `UI.Action.Mordecai.PauseMenu.{PreviousTab,NextTab,Inventory}`.
- No keyboard equivalent for tab switching; the mouse clicks the headers.
- `bridge_04.png` shows the filter bar overlapping "Stored Resources".

**US-088 Spell Cooldown HUD** (backlog)
- Four cooldown widgets, one per loadout slot, labelled with the slot key Q / E / R / 1, in a row under the SP bar.
- Bound to `Mordecai.Cooldown.Spell.{Fireball,Blink,StoneSkin,Restoration}`.
- "Visible on screen": four cooldown indicators under the SP bar (top-left).

**US-089 Spell Cooldown Content** (backlog, EDITOR): tags and durations, so the Q slot shows a 2 s countdown.

**US-091 HUD Readouts** (backlog)
- Status indicator shows "StoneSkin" (or a configured display name) with a green tint, one indicator per status.
- Weapon HUD shows "Longsword", then the new name after Tab. It must not overlap the pickup prompt.
- Remove all yellow debug text and Steam warnings from the screen.

**US-092 Enemy Indicators** (backlog)
- Add an `ULyraIndicatorManagerComponent` on the PC and an `UIndicatorLayer` in the HUD.
- Register on aggro, with retry.
- TODO(DECISION): remove the old widget-component bar? Today each enemy would show two health readouts.

**US-093 Pause Menu Tab Contents** (backlog)
- Build the default trees for the Character, Skills and Feats tabs inside the 1000×640 content area, with a scroll box for long skill lists.
- Settings stays "Coming Soon". TODO(DECISION): a real Settings tab?

**US-097 Gamepad key names** (backlog)
- `Gamepad_Left2D`, `Gamepad_Right2D`, `Gamepad_LeftThumbstick`.
- TODO(DECISION): the gamepad spell mapping. AC-057.5 has Spell 1=LB/Q, Spell 2=RB/E; control_bindings has Skill A = RB/Q, Skill B = LB/E.

**US-098 Facing and Aim** (backlog, BLOCKED): fixed facing vs mouse/right-stick aim; the facing arrow is hidden (N12).

**US-099 Player visible behind walls** (backlog, BLOCKED): cutaway decision N21 (fade, silhouette or lower walls).

**US-090 SP budget** (backlog, BLOCKED N6): MaxSP 10 vs loadout cost 12; no regen; no restore on respawn.

**US-096 Previous-weapon key** (backlog, N9): Shift+Tab vs BackSlash. The weapon HUD should read "Spear".

**US-087 Fireball/Blink** (backlog): projectile needs a visible mesh; Blink feedback.

**US-085 Frontliner content** (IN PROGRESS)
- Enemies are dark red cylinders with red bars.
- TODO(DECISION): corpse visuals. A dead enemy shows only its empty bar.

**US-001** (done): AC-2.2.5 asks for a basic HUD with Health%, Stamina, SP. AC-1.3.1 lists input actions including "Items A-D", "Pause", "Map" and "Throw".

### As-built HUD layout (`MordecaiCombatHUDWidget::BuildDefaultLayout`)

`WBP_CombatHUD` has no widget tree, so this C++ layout is what ships. All positions are on a canvas, in pixels.

| Element | Anchor | Position | Size |
|---|---|---|---|
| HealthBar | top-left | (20,20) | 250×20 |
| StaminaBar | top-left | (20,48) | 250×20 |
| PostureBar | top-left | (20,76) | 250×16 |
| SpellPointsBar | top-left | (20,100) | 250×16 |
| SpellCooldownA | top-left | (20,124) | 120×16 |
| SpellCooldownB | top-left | (150,124) | 120×16 |
| KillCounter | top-right | (−170,20) | 150×24 |
| ComboCounter | bottom-center | (0,−120) | 200×40 |
| CombatFeedback | bottom-center | (0,−80) | 300×30 |
| StatusEffectBar | bottom-left | (20,−60) | 400×40 |
| PickupPrompt | bottom-center | (0,−30) | 420×56 |

PickupPrompt text: "[F] Pick up {0}" in white; "[I] Inventory" in (0.8,0.8,0.8,0.9).

There is no weapon widget slot in this layout. The weapon widget is created by `MordecaiCharacter.cpp`.

### Loading screen (issue #39, FIXED)

`UMordecaiLoadingScreenWidget`:
- Background (0.010,0.010,0.012).
- "PROJECT MORDECAI" in 56 pt Bold, letter spacing 150, colour (0.86,0.80,0.68).
- "Loading..." in 20 pt, colour (0.55,0.55,0.55).
- A throbber.
- Held 2 s after the experience loads.
- Final art is a TODO(DECISION).

---

## Known UI issues

| Id | What is wrong | Owner / decision needed |
|---|---|---|
| N1 / #6 | The kill counter never increments: it waits for an `EnemyKill` tag nothing grants, while the GameMode counts kills elsewhere. | US-086, second half |
| N2 / #7 | Death screen never created. **FIXED** (`8486aa1`). | Remote clients still see none |
| N3 / #8 | Weapon HUD draws nothing (no widget tree). | US-091 |
| N4 / #9 | Character tab is blank. | US-093 |
| N5 / #10 | Skills, Feats and Settings show "Coming Soon". | US-093. Decision: a real Settings tab? |
| N8 / #13 | The HUD has 2 spell cooldown widgets, not 4. | US-088 |
| #35 | Spells have no cooldowns. The cooldown widgets are never bound, so they always read "ready". | US-088/089 |
| N16 / #21 | Status indicator reads "Status 10.0" instead of the status name. Only one indicator can exist and the buff/debuff tint is wrong. | US-091 |
| N17 / #22 | Enemy indicators never appear (no manager/layer, nothing calls `NotifyAggroActivated`). | US-092. Decision: remove the duplicate `WBP_EnemyHealthBar`? |
| #29 | Yellow debug text on screen ("MORDECAI: Programmatic IMC created…", "Weapon loadout initialized — 5 weapons available", "SetupPlayerInputComponent called…") plus Steam socket warnings, in every frame. | US-091 |
| #37 | Inventory filter bar overlaps the "Stored Resources" header. | No recovery story |
| N22 / #27 | Items have no Use/Equip action (potion cannot be used, sword cannot be equipped). | Decision: in scope now? |
| N21 / #26 | The near wall hides the player; there is no cutaway or fade. | US-099. Decision: fade vs silhouette vs lower walls |
| #34 + N12 / #17 | Facing is pinned to +X and the facing arrow is hidden in game. | US-098. Decision: fixed vs aim facing |
| N6 / #11 | SP budget: 10 SP vs a 12 SP loadout, no regen. The SP bar runs dry for the session. | US-090. Decision |
| N9 / #14 | Previous weapon is BackSlash only; Shift+Tab conflicts with sprint on Shift. | US-096. Decision. Note: control_bindings defines Tab as "Weapon Set Swap (Set 1 ↔ Set 2)" |
| #33 | Gamepad stick and L3 key names are invalid. | US-097. Plus the gamepad spell-button decision |
| N18 / #23 | LB/RB tab switching. **Decided "keep" and FIXED** (US-095). | — |
| #32 | I does not close the menu. **FIXED** (US-095, key-up close). | Held Escape probably reopens the menu after ~0.5 s (not reproduced) |
| N30 | Stamina never regenerates. After about 15 s of fighting the stamina bar sits empty or grey at the −50 floor for good. | No story |
| N33 | `Parried`/`Riposte` tags never removed. "Parried!" feedback never shows (the widget listens on the player's own ASC). | Durations unspecified |
| N24 | Light attacks never combo, so the combo counter never shows Hit 2/3. | No story |
| — | Pickup label text is light grey and far labels are faint on the light floor. | Readability, not filed |
| — | Enemy floating bar may not redraw after `ResetForArena` (TestDummy bar at ~92% while state says 200/200). | US-086 notes |
| — | Corpse visuals (death animation, ragdoll, fade-out). | TODO(DECISION), US-085 |
| — | The pause menu can open while dead (Escape is bound on the controller). The death countdown holds while paused. | Handled in US-086 |
| — | `control_bindings_v1.1.md` lists neither "I — Inventory" nor in-menu LB/RB. | TODO(DECISION), doc change by Jeff |
| — | Device-aware key glyphs (controller glyphs on gamepad) deferred; prompts show keyboard keys (design log 2026-07-10). Lyra's controller-data key-icon sets are still the CommonUI defaults (#62). | Deferred |
| B2 / #31 | The bridge cannot read progress-bar fill/colour, rarity colours, world-space widget texts or damage-pop events, so those UI facts cannot be playtest-asserted. | Bridge gap |

Playable Status (PLANNER_LOG, sweep `20261029-175504` @ `9904cde`):
- NOT PLAYABLE: combat_hud, kill_counter, enemy_indicators, spell_hud, status_effects, pause_menu, character_sheet, skill_tree_ui, feat_ui, weapon_hud.
- PLAYABLE (input): inventory, pickups, identification.

---

## Lyra branding still to replace

From the 2026-10-30 audit of the fresh package `1d33354`. All issues are open and `needs-decision`.

- **#57 Main menu** (`W_LyraFrontEnd`)
  - Glowing blue LYRA wordmark with an intro animation (`T_UI_LyraLogo`, `T_UI_LyraLogo_2K`, `M_UI_Base_Logo`, `MI_UI_Logo*`).
  - Buttons PLAY LYRA / OPTIONS / CREDITS / SHOW REPLAY / QUIT LYRA.
  - The UE "U" roundel (`MI_UI_UE5Logo_Small`).
  - "Exit Game" / "Are you sure?" quit confirmation.
  - The press-start screen `W_LyraStartup` ("Start" plus the LYRA logo; skipped on Windows).
  - Interim fix proposed: a text wordmark "PROJECT MORDECAI".
- **#58 Front-end background**: Lyra's white sci-fi lobby atrium (`L_ShooterFrontendBackground` via `B_LoadRandomLobbyBackground` / `ShooterGameLobbyBG`). Options: a Mordecai diorama vignette level, or a scene built into the map.
- **#59 Lyra modes playable from the front end**: Elimination (Expanse), Control (Convolution), Exploder (Checkered), Exploder Multiplayer, Test (Shooter Test). Five sample plugins and 17 Lyra maps ship.
- **#60 PLAY leads to Lyra's online-shooter screens**
  - `W_ExperienceSelectionScreen`: Quickplay / Start a Game / Browse tiles with mannequin art.
  - `W_HostSessionScreen`: "Host a game", AI Combatants, Network Offline/LAN/Online.
  - `W_SessionBrowserScreen`: "Join a game", ping, "Refresh Search".
  - Replay browser ("Select a Saved Replay") and the Options > Gameplay "Replays" settings.
  - The only Mordecai entry is the tile "Mordecai Arena" / "Dev test arena" / "Project Mordecai diorama arena (DevTestMap)". It has no icon and is not a default experience.
- **#61 Main menu watermarks**: "Player 1: <machine name+ID>", the NULL net ID, "UE5-CL-0 (Development)", "Player 2: Not logged in" and ADD PLAYER (split-screen login). No ProjectVersion is set.
- **#62 Lyra UI style**
  - Orbitron, uppercase, cyan glass buttons, a dark-blue rounded translucent panel, the `CA_UI_Frontend` curve atlas, `MI_UI_MenuBorder_*`, `MI_UI_MenuButton_*`, `MI_UI_FontColor_*`, `TextStyle-*` (NotoSans) and `ButtonStyle-*`.
  - Lyra screens affected: OPTIONS (`W_LyraSettingScreen` and its editors), dialogs (`W_ConfirmationDialog`, `W_ConfirmationDefault`/`Error`, `W_ControllerDisconnected`), `W_GammaEditor` (UE logo test image).
  - The confirmation/error dialog can appear in game on `UI.Layer.Modal`.
  - The throbber, rich-text icon set and controller key-icon sets are Lyra's.
  - Proposed: a native Mordecai dialog and one shared Mordecai text style.
- **#63 Credits**: `W_Credits` shows the LYRA logo, the UE5 logo and placeholder rows "CREDIT TITLE : Credit Person Name N". It needs real credits plus Orbitron's OFL attribution, or a hidden button.
- **#64 Localization**: 14 staged cultures of Lyra's Game target ("Lyra spielen", "Lancer Lyra", …) and zero Mordecai strings. A language picker exists in Options.
- **#65 Audio**: front-end music (`mx_System` MetaSound, `mx_menu_*` stems) and Lyra UI highlight/select sounds. The Mordecai pause menu and inventory have no UI sounds; the arena has no music.
- **#66 Executable metadata**: ProductName/FileDescription "Lyra", CompanyName "Epic Games, Inc.", placeholder copyright, the Unreal "U" icon (no `Build/Windows/Application.ico`), `ProjectName=Lyra`, Description "Sample starter game for Unreal Engine 5". Window title: "ProjectMordecai (64-bit Development PCD3D_SM6)".
- **#67** `LyraGame.exe` file name.
- **#68 Fallback experience**: `B_LyraDefaultExperience` → `W_DefaultHUDLayout`, which brings `W_LyraGameMenu` ("Return to Front End", Options, "Exit Game"), `W_ControllerDisconnected` and `W_SubtitleDisplayHost`. Also `DA_ExamplePlaylist`, `L_DefaultEditorOverview` and the old Lyra loading widgets (`W_LoadingScreen_Host`, `W_LyraLogo_LoadingScreen`).
- **#69** The `LyraGame` module, `Lyra.*` CVars and `LogLyra`. Not player-visible; recommended to defer.
- **#29** On-screen engine messages.
- Loading screen: **FIXED** (#39). The Mordecai text-only screen ships; its art is TODO(DECISION).

---

## Planned screens not yet built

From PLAN.md; no story files exist yet:
- **US-070** Combat HUD Blueprint Polish (EDITOR)
- **US-076** Pause Menu Blueprint & Tab Wiring (EDITOR)
- **US-075** Status Effect VFX (Niagara)
- **US-072** Equipment Panel: Weapon Sets & Armor (after Epics 6+7)
- **US-073** Map: Metroidvania World Map (after Epic 8; the "Map" input action already exists per AC-1.3.1)
- **US-074** Town Management UI (after Epic 9: US-037 Town Building, US-038 NPC Job Assignment, US-039 Initiatives & Upgrades, US-040 Town Events)

Implied by stories:
- Mage Tower identification NPC/UI (US-033, future town story; identification is currently free and instant, cost model a TODO(DECISION)).
- Town-storage UI (AC-031.16) and crafting (`ConsumeResource`).
- Item Use/Equip actions (N22).
- Drop/trash actions (out of scope in US-071).
- Weapon Set 1/Set 2 swap (control_bindings; deferred by US-077).
- Ranged weapons, armor and shields UI (US-027-030, which need design input).
- "Items A-D" quick slots and Throw (AC-1.3.1 input actions).
- Character level display ("Level: --").
- Feat-unlock and skill-milestone notifications: only delegates exist (`OnFeatUnlockedBP`, `OnSkillMilestoneReachedBP`).
- A real Settings tab.
- A Mordecai front end and its Tier 4 "choose the arena tile" playtest (PLANNER.md §5.1 item 1).
- Mordecai confirmation dialog (#62); credits (#63); front-end background (#58).
- Occlusion cutaway or silhouette (US-099); facing/aim indicator (US-098).
- Device-aware glyphs.
- Corpse visuals.

---

## Terminology

- **Core resources:** Health (shown as %), Stamina (tiered Green/Yellow/Red/Exhausted), Posture ("Posture Broken"), Spell Points (SP, "x / y").
- **Primary attributes:** STR, DEX, END, CON, RES, DIS, INT, WIS, CHA, grouped Physical / Resilience / Magical.
- **Skills:** categories Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic. Rank 1-20, milestones at 1/5/10/15/20. Skill points, "Allocate".
- **Feats:** "earned, not picked". Tiers Common / Rare / Legendary; drawbacks.
- **Items:**
  - Item types: Weapon, Armor, Trinket, Consumable, Material, TownResource, UpgradeKey, QuestItem, MagicalItem, CurrencyProxy.
  - Rarity: Common, Green, Blue, Purple, Red (plus Gold in the widget map); also called Uncommon/Rare/Epic/Legendary/Mythic in US-071.
  - Sort priority: Low, Normal, High, Signature, Critical.
  - Unidentified / Identified; Identify; Mage Tower.
  - Unlimited carry; auto-store; resource ledger ("Stored Resources"); town storage; town gating.
- **Pause menu:** "Paused"; tabs Character / Skills / Feats / Inventory / Settings; "Coming Soon".
- **Prompts and messages:** "[F] Pick up <item>", "[I] Inventory", "You Died", "Kills: N", "Hit 1/2/3", "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!", "Unarmed".
- **Spells:** Fireball (Q), Blink (E), Stone Skin (R), Restoration (1).
- **Weapons:** Longsword, Dagger, Greatsword, Spear, Mace.
- **Damage types:** Physical, Fire, Frost, Lightning, Poison, Arcane, Holy, Dark.
- **Statuses:** e.g. Burning, StoneSkin, Focused, Blessed.
- **Enemies:** Frontliner; TestDummy / TestAttacker.
- **Maps and playlist:** DevTestMap / MovementTestMap / CombatTestMap; playlist "Mordecai Arena".
- **UI layers:** `UI.Layer.Game` / `Menu` / `Modal`. CommonUI Menu input mode.
- **Camera:** diorama camera (fixed pitch −50, yaw 0, never rotates).


## UI INVENTORY
- [HUD|implemented] **Combat HUD root (UMordecaiCombatHUDWidget)**: Canvas that hosts every gameplay HUD element; its C++ BuildDefaultLayout is what ships because WBP_CombatHUD has no widget tree | data: Children: HealthBar, StaminaBar, PostureBar, SpellPointsBar, SpellCooldownA/B, KillCounter, ComboCounter, CombatFeedback, StatusEffectBar, PickupPrompt | states: bound to ASC / unbound; local player of standalone or listen server only (remote clients get none) (stories/done/US-052-combat-hud-framework.md; Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp:34-127)
- [HUD|implemented] **Health bar**: Always-visible player health | data: Health as 0-100% of MaxHealth (default 100); top-left (20,20), 250x20 | states: full, draining, empty (dead) (US-052 AC-052.2/052.6; US-054 AC-054.7; MordecaiCombatHUDWidget.cpp:61)
- [HUD|implemented] **Stamina bar with tier colour**: Stamina fill plus effectiveness tier | data: Stamina/MaxStamina (default 100); tiers Green >=66% (0,1,0) 1.25x, Yellow >=33% (1,1,0) 1.0x, Red >0% (1,0,0) 0.75x, Exhausted <=0 (0.5,0,0) 0.5x; floor -50; (20,48) 250x20 | states: Green, Yellow, Red, Exhausted; stuck empty because stamina never regenerates (N30) (US-052 AC-052.3/052.8; Specs/design_decisions_log.md stamina tiers; agent_playtest_plan_v1.md N30)
- [HUD|implemented] **Posture bar**: Player posture meter | data: Posture/MaxPosture; Normal colour (0.2,0.6,1.0), Broken (1,0.2,0.2); (20,76) 250x16 | states: normal, Posture Broken (Mordecai.State.PostureBroken) (US-052 AC-052.4; MordecaiPostureBarWidget.h:63-67)
- [HUD|implemented] **Spell Points bar**: Discrete SP resource | data: Fill plus numeric 'Current / Max' (e.g. '10 / 10', '7 / 10'); MaxSP 10; costs Fireball 3, Blink 2, Stone Skin 4, Restoration 3; (20,100) 250x16 | states: full, partial, too low to cast (no regen, N6) (US-055 AC-055.1-055.3; US-057 AC-057.10; US-090 (N6))
- [HUD|implemented] **Spell cooldown slots**: Per-spell cooldown readout | data: Remaining seconds text plus radial/linear fill; planned 4 slots labelled Q / E / R / 1 in a row under the SP bar; durations Fireball 2.0 s, Blink 4.0 s, Stone Skin 15 s, Restoration 10 s | states: ready, cooling down (counting), unbound (always 'ready' today); only 2 exist (N8) (US-055 AC-055.8/055.9; US-057 AC-057.9; stories/backlog/US-088, US-089; MordecaiCombatHUDWidget.cpp:81-92)
- [HUD|implemented] **Kill counter**: Session kills | data: 'Kills: N' (0..99 tested); top-right (-170,20) 150x24 | states: 0, incrementing, reset on player death; never increments today (N1) (US-063; US-086 AC-086.3/086.5; issue #6)
- [HUD|implemented] **Combo counter**: Shows current hit in a melee chain | data: 'Hit 1' / 'Hit 2' / 'Hit 3'; bottom-center (0,-120) 200x40 | states: hidden, visible during Mordecai.State.ComboWindow, auto-hide; Hit 2/3 never reached (N24) (US-061 AC-061.1; agent_playtest_plan_v1.md N24)
- [HUD|implemented] **Combat feedback flash**: Brief text for perfect actions and failures | data: 'Perfect Dodge!', 'Perfect Block!', 'Parried!', 'Posture Broken!' in gold (1,0.84,0); 'Parry Failed!' in red (1,0.2,0.2); bottom-center (0,-80) 300x30; fades ~1 s | states: hidden, flashing, fading; 'Parried!' never shows (N33) (US-061 AC-061.2; MordecaiCombatFeedbackWidget.cpp:51-80; US-101 notes)
- [HUD|implemented] **Status effect bar and indicators**: Active buffs and debuffs with remaining time | data: Per indicator: status name (e.g. 'StoneSkin', 'Burning'), remaining duration '%.1f', tint green (buff) / red (debuff); BuffTags Focused, Blessed, StoneSkin; bottom-left (20,-60) 400x40 | states: empty, one or more indicators, expiring; bug: reads 'Status', only one indicator, wrong tint (N16) (US-055 AC-055.4-055.7; US-091 AC-091.1/091.2; issue #21)
- [HUD|implemented] **Pickup prompt and inventory hint**: Press-to-pickup prompt and assist hint | data: '[F] Pick up <display name>' white (identification-aware, e.g. 'Unidentified Purple Amulet'); '[I] Inventory' grey (0.8,0.8,0.8,0.9); bottom-center (0,-30) 420x56 | states: no pickup focused (hint only), pickup focused (prompt visible); keyboard glyphs only (device-aware glyphs deferred) (Specs/design_decisions_log.md 2026-07-10; US-079; MordecaiPickupPromptWidget.cpp:22-27)
- [HUD|implemented] **Equipped weapon display**: Show the current weapon | data: Weapon DisplayName and WeaponType; 'Unarmed' when none; Longsword, Dagger, Greatsword, Spear, Mace; bottom-center or bottom-right (AC-078.11) | states: weapon equipped, unarmed, changing on Tab/Y/BackSlash; draws nothing today (N3) (US-077 AC-077.7-077.9; US-078 AC-078.11/078.12; US-091 AC-091.3/091.4; issue #8)
- [Notification / overlay|planned-story] **On-screen debug text (to remove)**: Developer messages that must not reach players | data: 'MORDECAI: Programmatic IMC created and input bound successfully...', 'MORDECAI: Weapon loadout initialized — 5 weapons available', 'MORDECAI: SetupPlayerInputComponent called…', Steam socket deprecation warnings (yellow) | states: shown in every Development frame today; to be log-only (US-091 AC-091.5; issue #29)
- [World-space|implemented] **Enemy health bar (widget component)**: Health above each enemy | data: Enemy health percent; red bar above the head (WBP_EnemyHealthBar) | states: full, damaged, empty (dead); may not redraw after arena reset; possibly to be removed once indicators work (TODO(DECISION)) (US-052 AC-052.10; US-054 AC-054.9; US-092 Technical Notes; US-086 completion notes)
- [World-space|implemented] **Enemy indicator (Lyra IndicatorSystem)**: Screen-space tag tracking each enemy | data: Enemy display name, health bar 0-1 (Green >50%, Yellow 25-50%, Red <25%), row of status icons (tag->texture map, placeholder fallback); 120 cm above head; visible within 2000 cm | states: unregistered, registered on damage/aggro, hidden beyond range, removed on death; never appears today (N17) (US-064 AC-064.1-064.10; US-092; issue #22)
- [World-space|implemented] **Floating damage numbers**: Hit and heal feedback | data: Integer amount coloured by damage type: Physical white, Fire orange (1,0.5,0), Frost cyan (0.3,0.8,1), Lightning yellow (1,1,0.3), Poison green (0.3,0.9,0.3), Arcane purple (0.7,0.3,1), Holy gold (1,0.85,0), Dark dark red (0.6,0.1,0.1), unknown grey (0.7); healing '+' green (0.2,1,0.2); lifespan 1.5 s, rise 100 cm/s, scatter 30 cm, max 20 concurrent | states: normal, critical (1.5x scale), healing, recycled (US-065 AC-065.1-065.10)
- [World-space|implemented] **Pickup name label**: Tell pickups apart on the floor | data: Item display name (identification-aware: 'Unidentified Purple Amulet'); billboard facing the camera | states: identified name, partial unidentified name, picked up (gone); light grey, faint on light floor (not filed) (US-079 AC-079.10; agent_playtest_plan_v1.md issue #38 entry)
- [World-space|planned-story] **Player facing / aim indicator**: Show where attacks and spells go | data: Facing arrow (AC-2.1.5); yaw following mouse cursor (projected onto the ground) or right stick | states: hidden in game (N12); facing pinned to +X pending decision (US-001 AC-2.1.2/2.1.5; US-098; issues #17, #34)
- [World-space|planned-story] **Occlusion cutaway / player silhouette**: Keep the player visible behind near walls | data: Fade/hide occluders, or a silhouette through walls (custom depth), or lower near-side walls | states: player occluded vs visible; approach undecided (N21) (US-099; issue #26)
- [World-space|implied] **Enemy corpse visuals**: Show a dead enemy | data: Death animation / ragdoll / fade-out (undecided); today a dead enemy shows only its empty bar and its body is hidden | states: alive, dead (stories/in-progress/US-085 Technical Notes TODO(DECISION))
- [Notification / overlay|implemented] **Death screen**: Death feedback and respawn timing | data: Full-screen dim (0,0,0,0.5); 'You Died' 48 pt red (0.8,0.1,0.1); countdown ceiling integer from RespawnDelaySeconds 3.0 (32 pt white); 'Kills: N' (20 pt white); on UI.Layer.Game | states: shown on Mordecai.State.Dead, counting down, holding while paused, removed on respawn; none for remote clients (US-062; US-086 AC-086.1/086.2/086.4; MordecaiDeathScreenWidget.cpp:58-113)
- [Loading / transition|implemented] **Mordecai loading screen**: Shown on every map/experience load | data: Near-black background (0.010,0.010,0.012); 'PROJECT MORDECAI' 56 pt Bold, letter spacing 150, (0.86,0.80,0.68); 'Loading...' 20 pt (0.55); throbber; held 2 s after load | states: visible during load; final art TODO(DECISION) (Specs/agent_playtest_plan_v1.md issue #39 entry; MordecaiLoadingScreenWidget.cpp:25-132)
- [In-game menu|implemented] **Pause menu shell**: Tabbed game menu that pauses play | data: Dim (0,0,0,0.65); centred 1000x640 column; title 'Paused'; tab bar Character / Skills / Feats / Inventory / Settings; active tab (0.25,0.35,0.55) white label, inactive (0.12,0.12,0.15) grey label; content border (0.05,0.05,0.08,0.9) | states: closed, open (game paused, Menu input mode, cursor shown), tab active; opened by Esc/Start (last tab) or I (Inventory tab); closed by Esc/Gamepad B/I key-up; LB/RB previous/next tab with wrap; click headers; remembers last tab per session (US-069; US-095; Specs/design_decisions_log.md 2026-10-30; MordecaiPauseMenuWidget.cpp:244-315)
- [In-game menu|implemented] **Character tab (character sheet)**: Build identity: attributes and derived stats | data: STR DEX END CON RES DIS INT WIS CHA (base value + '+X%') in Physical / Resilience / Magical columns; Health/Stamina/Spell Points/Posture as 'Current / Max'; Physical Damage, Attack Speed, Affliction Resist, Magic Damage, Cast Speed; Armor Penetration, Physical Crit Chance, Stamina Regen, Health Regen, Affliction Recovery, Posture Recovery, SP Regen, Resistance Penetration, Magic Crit Chance; 'Level: --' | states: bound, unbound ('--'), live update; blank today (N4) (US-066 AC-066.1-066.9; US-093 AC-093.1/093.3; issue #9)
- [In-game menu|implemented] **Skills tab (skill tree)**: Skill ranks, milestones and point allocation | data: Categories Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic; rows 'name rank / 20'; milestone ranks 1/5/10/15/20 marked; detail panel with milestone descriptions; available skill points; Allocate per row | states: category selected, skill selected, milestone locked / reached / unlocked-and-active, Allocate enabled / disabled (0 points or rank 20), no component (empty), 'No data available.'; 'Coming Soon' today (N5) (US-067 AC-067.1-067.11; US-093 AC-093.2/093.4; issue #10)
- [In-game menu|implemented] **Feats tab (feat display)**: Unlocked and locked feats | data: Header 'Feats: N unlocked — X Common, Y Rare, Z Legendary'; rows with name, tier badge, excerpt; tier colours Common (0.8,0.8,0.8), Rare (0.3,0.5,1.0), Legendary (1.0,0.75,0.0); detail panel with description and counts of effects/abilities/drawbacks; locked rows with '???' and progress ('Stat: cur / threshold', 'Skill Rank: x / y', 'Requires: Tag' with check or cross) | states: unlocked, locked, live unlock moves a row; no component (empty); 'Coming Soon' today (N5) (US-068 AC-068.1-068.9; US-093 AC-093.2/093.5; issue #10)
- [In-game menu|implemented] **Inventory tab**: Flat unlimited-carry list, filters, ledger, identify | data: Filter bar All / Weapons / Armor / Trinkets / Consumables / Materials / Quest / Magical; rows with name, '  xN', '  [equipped]', '? ' prefix for unidentified, rarity colour on the text (Common grey (0.55), Green (0.10,0.80,0.10), Blue (0.15,0.40,1.00), Purple (0.60,0.20,0.90), Red (0.90,0.10,0.10), Gold (1.00,0.84,0.15)); Identify button; 'Stored Resources' ledger '{name}  x{n}' | states: 'Inventory is empty.', 'No items match this filter.', 'No resources stored.', '--' unbound, unidentified vs identified row; filter bar overlaps the ledger header (#37); no Use/Equip/Drop actions (N22) (US-071; US-079 AC-079.8-079.11; US-094; MordecaiInventoryWidget.h:159-164, .cpp:221-551)
- [In-game menu|specified] **Settings tab**: In-game settings | data: Currently 'Coming Soon' | states: placeholder; whether a real Settings tab is wanted is TODO(DECISION) (US-069 AC-069.3; US-093 Technical Notes; issue #10)
- [In-game menu|implied] **Item use / equip actions**: Use consumables, equip gear from the inventory | data: Use (Health Potion), Equip (Rusty Sword into the weapon cycle); equip gated by identification (RequiresIdentificationToEquip) | states: available, unavailable (unidentified) (issue #27 (N22); US-033 AC-033.9/033.10; US-071 Technical Notes)
- [In-game menu|planned-story] **Equipment panel (weapon sets and armor)**: Manage weapon sets and armor | data: Weapon Set 1 / Set 2, armor slots (Leather, Chain, Plate per Epic 6) | states: not built (stories/PLAN.md Epic 10 Future US-072; US-077 (Weapon Set Swap deferred))
- [In-game menu|planned-story] **World map**: Metroidvania world map | data: Not specified in stories; 'Map' input action exists (AC-1.3.1) | states: not built (stories/PLAN.md US-073; US-001 AC-1.3.1)
- [HUD|implied] **Item quick slots (Items A-D) and Throw**: Quick-use items | data: Four item slots A-D; Throw | states: not built; input actions exist (US-001 AC-1.3.1)
- [Notification / overlay|implied] **Feat unlocked notification**: Tell the player a feat was earned through play | data: Feat name, tier (Common/Rare/Legendary), drawback if any; e.g. 'Pyromaniac' (burn 6 enemies at once) | states: not built; only the OnFeatUnlockedBP delegate exists (US-012; US-068 AC-068.2/068.9)
- [Notification / overlay|implied] **Skill milestone reached notification**: Announce rank milestones 1/5/10/15/20 | data: Skill name, milestone rank, unlocked technique/passive | states: not built; only OnSkillMilestoneReachedBP exists (US-011; US-067 AC-067.11)
- [Village / service screen|implied] **Mage Tower identification service**: Identify unidentified magical items in town | data: Unidentified items, identify action; cost model undecided (free today) | states: not built; the inventory Identify button stands in (US-033 Technical Notes; US-079 Technical Notes)
- [Village / service screen|implied] **Town storage / resource ledger screen**: View auto-stored materials in town; crafting | data: FMordecaiResourceEntry list (ItemId, Count, Def) | states: not built (US-031 AC-031.16, AC-031.15)
- [Village / service screen|planned-story] **Town Management UI**: Delegated town systems | data: Town buildings, NPC job assignment, initiatives and upgrades, town events (Epic 9: US-037-040) | states: not built (stories/PLAN.md US-074, Epic 9)
- [Main menu / front end|implemented] **Main menu (Lyra W_LyraFrontEnd, to replace)**: First screen after the loading screen | data: LYRA wordmark; PLAY LYRA / OPTIONS / CREDITS / SHOW REPLAY / QUIT LYRA; UE 'U' roundel; 'Exit Game' / 'Are you sure?' confirmation | states: PLAY focused on boot; to be rebranded (wording, logo, what PLAY does are Jeff's decisions) (issue #57; issue #60)
- [Main menu / front end|implemented] **Press-start screen (W_LyraStartup)**: Platform press-start | data: LYRA logo, 'Start' | states: skipped on Windows (Platform.Trait.SingleOnlineUser) (issue #57)
- [Main menu / front end|implemented] **Front-end background scene**: 3D scene behind the main menu | data: Lyra white sci-fi lobby atrium (L_ShooterFrontendBackground); options: a Mordecai diorama vignette or a scene in the map | states: to replace (issue #58)
- [Main menu / front end|implemented] **Experience / mode selection**: Choose what to play | data: Quickplay / Start a Game / Browse tiles; Lyra playlists Elimination, Control, Exploder, Exploder Multiplayer, Test; Mordecai tile 'Mordecai Arena' / 'Dev test arena' / 'Project Mordecai diorama arena (DevTestMap)', no icon | states: Lyra modes reachable today; Tier 4 front-end playtest (choose the arena tile) not yet written (issues #59, #60; stories/PLANNER.md §5.1 item 1)
- [Main menu / front end|implemented] **Host session / session browser screens**: Lyra online hosting and joining | data: 'Host a game', AI Combatants (Bots Enabled/Disabled), Network Offline/LAN/Online; 'Join a game', Game / Map, Players, ping, 'Refresh Search' | states: keep and reskin, or remove (Jeff's decision) (issue #60)
- [Main menu / front end|implemented] **Replay browser**: Lyra replays | data: 'Select a Saved Replay', REFRESH, 'Play replay?'; Options > Gameplay 'Record Replays', 'Keep Replay Limit' | states: remove or reskin (decision) (issue #60)
- [Main menu / front end|implemented] **Options / settings screen (Lyra W_LyraSettingScreen)**: Game settings in the front end | data: Lyra settings editors (W_SettingsListEntry_*, W_SafeZoneEditor, W_PressAnyKey, W_KeyAlreadyBoundWarning); Language picker; Video > Brightness slider; Gamma editor shipped but unreachable | states: Lyra-styled; whether it stays player-facing is undecided (issues #62, #64)
- [Main menu / front end|implemented] **Credits screen**: Credits and attributions | data: Today LYRA logo, UE5 logo, placeholder rows 'CREDIT TITLE : Credit Person Name N'; needs real credits plus third-party attributions (Orbitron SIL OFL) | states: to replace or hide (issue #63)
- [Main menu / front end|implemented] **Dev watermarks and ADD PLAYER**: Lyra developer and split-screen widgets | data: 'Player 1: <machine name+ID>', NULL net ID, 'UE5-CL-0 (Development)', 'Player 2: Not logged in', ADD PLAYER | states: to remove or dev-only; version scheme undecided (issue #61)
- [Notification / overlay|implemented] **Confirmation / error dialog**: Quit confirmation, language apply, system errors | data: Lyra W_ConfirmationDialog (Orbitron, uppercase, Lyra borders) on UI.Layer.Modal; proposed native UMordecaiConfirmationDialog | states: yes/no confirm, error; can appear in game (issue #62)
- [Notification / overlay|implemented] **Controller disconnected / subtitles (Lyra)**: Lyra HUD-layout overlays | data: W_ControllerDisconnected, W_SubtitleDisplayHost | states: only via the Lyra fallback HUD or front end (issue #68; issue #62)
- [In-game menu|implemented] **Lyra fallback escape menu (W_LyraGameMenu)**: Escape menu if the Mordecai experience fails to load | data: 'Return to Front End', Options, 'Exit Game', 'Are you sure?' | states: shipped, not seen on Mordecai maps (issue #68)
- [Other|specified] **Input prompt glyphs (device-aware)**: Show controller glyphs on gamepad | data: Prompts show keyboard keys only ('[F]', '[I]'); Lyra controller-data key-icon sets (KBM, XSX, PS5) and rich-text icon set are the CommonUI defaults | states: deferred (Specs/design_decisions_log.md 2026-07-10; issue #62 item 6)
- [Other|implemented] **Front-end music and UI sounds**: Audio identity of menus | data: Lyra mx_System MetaSound, mx_menu_* stems, Lyra_UI_MainMenu/SubMenu/Lobby highlight/select; Mordecai pause menu and inventory have no UI sounds, the arena has no music | states: to replace (audio direction undecided) (issue #65)
- [Other|implemented] **Executable identity (name, icon, version, window title)**: OS-level branding | data: LyraGame.exe; ProductName 'Lyra', CompanyName 'Epic Games, Inc.', Unreal 'U' icon; window title 'ProjectMordecai (64-bit Development PCD3D_SM6)' | states: to replace (issues #66, #67)
- [Other|implemented] **Localization / language**: Player-facing text in other cultures | data: 14 staged Lyra cultures (en, ar, es, es-419, fr, it, ja, pl, pt-BR, ru, tr, zh-Hans, ko, de) with Lyra translations, 0 Mordecai strings | states: recommended: stage en only until translated (issue #64)

## KEY TERMS
- Combat HUD — UMordecaiCombatHUDWidget; the gameplay HUD root (health, stamina, posture, SP, cooldowns, kills, feedback, status, prompt)
- Health % — health shown as 0-100% of MaxHealth (default 100)
- Stamina Tier — Green (>=66%, 1.25x), Yellow (>=33%, 1.0x), Red (>0%, 0.75x), Exhausted (<=0, 0.5x); stamina effectiveness band
- Posture / Posture Broken — stagger meter; Mordecai.State.PostureBroken is the broken state
- Spell Points (SP) — discrete spell resource shown 'Current / Max' (fresh 10 / 10)
- Kills: N — kill counter and death-screen kill text format
- You Died — death screen headline; respawn countdown is a ceiling integer from RespawnDelaySeconds 3.0
- Perfect Dodge! / Perfect Block! / Parried! / Parry Failed! / Posture Broken! — combat feedback flashes (gold except Parry Failed in red)
- Hit 1 / Hit 2 / Hit 3 — combo counter text
- [F] Pick up <item> — press-to-pickup prompt (Interact = F / Gamepad X)
- [I] Inventory — HUD assist hint; I opens the pause menu on the Inventory tab
- Paused — pause menu title
- Character / Skills / Feats / Inventory / Settings — pause menu tabs (ids character, skills, feats, inventory, settings)
- Coming Soon — pause-menu placeholder text
- STR, DEX, END, CON, RES, DIS, INT, WIS, CHA — the 9 primary attributes
- Physical / Resilience / Magical — character-sheet attribute groups
- Rank / 20 and milestones 1/5/10/15/20 — skill progression display
- Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic — skill categories
- Feats: earned, not picked — achievement-unlocked perks; tiers Common / Rare / Legendary, with drawbacks
- Unlimited carry — no bag slots or weight; never build bag-based inventory
- Auto-store / Resource ledger / Stored Resources — materials and town resources go straight to town storage
- Town gating — some item functions (e.g. identification at the Mage Tower) happen in town
- Unidentified / Identify / Mage Tower — magical items show partial info ('Unidentified Purple Amulet' or 'Unknown Item') until identified
- ? prefix — inventory badge for an unidentified row
- Rarity: Common, Green, Blue, Purple, Red (plus Gold) — item rarity colour tiers (US-071 also calls them Uncommon, Rare, Epic, Legendary, Mythic)
- Item types: Weapon, Armor, Trinket, Consumable, Material, TownResource, UpgradeKey, QuestItem, MagicalItem, CurrencyProxy
- Inventory filters: All, Weapons, Armor, Trinkets, Consumables, Materials, Quest, Magical
- Sort priority: Low, Normal, High, Signature, Critical
- Unarmed — weapon display text when nothing is equipped
- Weapon Set Swap — control_bindings Tab / Y hold (Set 1 vs Set 2); today a 5-weapon cycle: Longsword, Dagger, Greatsword, Spear, Mace
- Fireball (Q) / Blink (E) / Stone Skin (R) / Restoration (1) — the arena spell loadout
- Damage types: Physical, Fire, Frost, Lightning, Poison, Arcane, Holy, Dark — each has a damage-number colour
- Status effects (Mordecai.Status.*) — e.g. Burning, StoneSkin, Focused, Blessed; buffs tinted green, debuffs red
- Enemy indicator — Lyra IndicatorSystem tag over an enemy: name, health bar, status icons
- Frontliner — the arena enemy archetype (dark red cylinder placeholder)
- TestDummy / TestAttacker — CombatTestMap targets
- Diorama camera — fixed overhead camera, pitch -50, yaw 0, never rotates
- DevTestMap / MovementTestMap / CombatTestMap — the cooked playtest maps
- Mordecai Arena — the front-end playlist tile (DA_Mordecai_Arena), subtitle 'Dev test arena'
- PROJECT MORDECAI / Loading... — current loading-screen text
- UI.Layer.Game / UI.Layer.Menu / UI.Layer.Modal — CommonUI layers (death screen / pause menu / dialogs)
- Menu input mode — CommonUI mode that blocks game input and shows the cursor while the pause menu is open
- Lyra branding — leftover Lyra sample UI, art, audio and metadata (issues #57-#69)

## OPEN QUESTIONS
- No story, plan or known issue references Dungeon Crawler Carl or Link's Awakening (nor loot boxes, an announcer, sponsors, achievement popups or a minimap). How should the brief map onto the specified elements: feats as achievements, the floating numbers, the death screen, the Mage Tower and town?
- Final title, logo art and wordmark (loading screen art TODO(DECISION); #57 main-menu logo; #63 credits title art); product name, company, copyright and version scheme (#66, #61)
- Main-menu button wording, and what PLAY does: straight into the arena, or a Mordecai mode/character select (#57, #60)
- Whether any Lyra mode, hosting/joining or replays stay in the game (#59, #60)
- Front-end background scene: a Mordecai diorama vignette level, or a scene in the front-end map (#58)
- Mordecai UI style (font, palette, panel and button art) to replace both Lyra's Orbitron/blue glass and the current Roboto/native colours; does Lyra's Options screen stay player-facing (#62)?
- Real credits now, or hide the CREDITS button (#63)
- Which cultures to support at this stage (#64)
- Audio direction for menu music and UI sounds (#65)
- Is a real in-game Settings tab wanted, or does it stay 'Coming Soon' (US-093 TODO(DECISION))?
- With enemy indicators working, remove the old WBP_EnemyHealthBar widget-component bar so enemies do not show two health readouts (US-092 TODO(DECISION))?
- Corpse visuals for dead enemies: death animation, ragdoll or fade-out (US-085 TODO(DECISION))
- Cutaway approach for walls hiding the player: fade occluders, silhouette, or lower near walls (US-099, N21)
- Fixed facing vs aim-driven facing, and the visibility of the facing arrow / aim indicator (US-098, N12)
- Are item Use/Equip (and Drop) actions in scope (N22, #27)?
- SP budget rule: raise MaxSP, regen, restore on respawn, or lower costs (US-090, N6). It changes what the SP bar shows.
- Previous-weapon key: Shift+Tab vs BackSlash; should the 5-weapon cycle become the specified Weapon Set Swap (US-096, N9)?
- Gamepad spell buttons: AC-057.5 (LB/Q, RB/E, LB+RB/R, DPadUp/1) vs control_bindings (Skill A = RB/Q, Skill B = LB/E) (US-097 TODO(DECISION))
- Add 'I — Inventory' and in-menu LB/RB to control_bindings_v1.1.md (TODO(DECISION) open since 2026-07-10)
- Device-aware key glyphs (controller prompts) are deferred: when, and in which art style?
- Durations of the Parried/Riposte states (N33). Until they are removed, the 'Parried!' feedback never shows.
- Identification cost model at the Mage Tower (free today; TODO(DECISION) in US-033)
- Multiplayer pause behaviour (bCanPauseGame, AC-069.13); remote clients get no HUD or death screen today
- Character level display: the sheet shows 'Level: --' because no leveling system exists
- Remove the dev watermarks, ADD PLAYER and the split-screen config (#61); is local co-op ever planned?