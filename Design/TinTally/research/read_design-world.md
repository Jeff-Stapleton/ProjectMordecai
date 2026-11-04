# Project Mordecai: core design, tone, world and meta loop (research for the UI design system)

Sources I read in full:
- `Specs/game_design_v2.md` (v2.0.0, 2026-03-17)
- `Specs/Game Design v1.md` (v1.0, 2026-03-02)
- `Specs/world_design_v1.md`
- `Specs/village_system_v1.md`
- `Specs/design_decisions_log.md`
- `Specs/agent_rules_v2.md`
- `Specs/map_style_technical_design_v1.md` (2026-04-12)
- `Specs/agentic_map_pipeline_v1.md` (2026-04-12)

Sources I read for cross-checks:
- `Specs/item_system_v1.md`
- `Specs/status_effects_launch_set_v1.md`
- `Specs/ue5_tech_stack_context.md`
- `stories/PLAN.md`
- `stories/PLANNER.md` §5.1
- `stories/done/US-033-unidentified-items-town-gating.md`
- `Specs/agent_playtest_plan_v1.md` (loading-screen and rebranding entries)
- `Source/LyraGame/Mordecai/UI/MordecaiLoadingScreenWidget.cpp`
- `MordecaiDeathScreenWidget.cpp`
- `MordecaiPauseMenuTypes.h`

I also grepped the whole repo (`*.md`) for DCC terms.

---

## Pillars and tone

### High concept (identical in v1 and v2)
> "A diorama-style (Link's Awakening / Echoes of Wisdom) action RPG where player skill can clear the entire game at level 1 with no upgrades, while D&D-like attributes + granular skills + curated loot create deep build identity. Between expeditions, a self-sufficient town provides services, research, provisioning, and Metroidvania-style world gates—as an upgrade vector, not a chore." (`game_design_v2.md` §1)

### Non-negotiable pillars (`game_design_v2.md` §2; v1 has the first 8)
1. **Skill-first combat:** "Mastery (dodge/block/parry/spacing) beats stats."
2. **Granular skills (DCC vibe):** "'Everything you do is a skill.' Skills level to 20 and unlock techniques at milestones."
3. **Feats are earned, not picked:** "Unlocked via gameplay achievements. Tiered (Common/Rare/Legendary). Strong feats can include drawbacks (DOS2-like)."
4. **Common-sense loot:** "Wolves drop pelts/teeth; bandits drop gold + plausible items. No grinding for random rare weapons."
5. **Curated magic items:** "Found via bosses/dungeons/story (BG3-style gating/prereqs). Rarity: Green/Blue/Purple/Red(Gold)."
6. **Town is strategic and delegated:** "NPCs gather/produce. Player assigns jobs, selects initiatives, upgrades buildings, and occasionally defends assets."
7. **Metroidvania world:** "4–6 biomes. Town upgrades unlock new routes in old zones."
8. **Clean UI:** "Always show Health (0–100%), tiered Stamina, Spell Points. Status is primarily VFX-driven, not icon soup."
9. **Low-friction inventory** (new in v2): "Inventory management is not a core source of challenge; expedition pressure should come from combat, preparation, identification, and town services."

### How Dungeon Crawler Carl shows up in the docs
DCC appears **only** as the parenthetical **"DCC vibe"** on pillar 2: granular "everything you do is a skill" progression. `stories/done/US-011-skill-framework.md` cites it again ("Pillar: 'granular skills, DCC vibe'").

The feat rule is DCC-like, though the docs never call it DCC: feats are "Unlocked by achievements/patterns (e.g., burn 6 enemies at once → Pyromaniac)" (`game_design_v2.md` §5).

A repo-wide search of every `*.md` found **zero** mentions of:
- Dungeon Crawler / Carl / Princess Donut / Borant / showrunner
- crawlers / the System AI / announcer / narrator
- viewers / sponsors / followers
- loot boxes, floors, stairwells, safe rooms, floor timers

**The docs never explain the name "Mordecai".** No doc says it refers to the DCC game-guide character, and no guide character is specified.

The world lore is original, not DCC: "Impermanence vs Control", automatons ("Continuants"), necromancy, a Lich King. DCC is a progression-feel reference, not a license, homage or setting.

### How Link's Awakening shows up
- High concept: "diorama-style (Link's Awakening / Echoes of Wisdom)".
- `map_style_technical_design_v1.md` §0: "inspired by the **spatial readability, compression, and authored pathing** seen in the provided Link's Awakening screenshots". It also says: "We are **not** trying to copy Nintendo assets or exact layouts."
- Target qualities: "compact, toy-like diorama spaces", "very readable paths and combat pockets", "handcrafted 'micro-rooms' connected into a broader metroidvania world", "deceptively simple geometry with high gameplay density".
- Final principle (§15): the world should feel "small but adventurous / simple but rich / readable but secretive / toy-like but dangerous / handcrafted at every step".
- `agentic_map_pipeline_v1.md`: "building Link's Awakening-style map content with AI agents".

### Other named influences
- "Elden Ring DNA, top-down readability" (combat, §4)
- "Max level: 60 (WoW nod)"
- "D&D-like attributes"
- "BG3-style gating/prereqs"
- "DOS2-like" drawbacks
- "Feats (Outer Worlds + DOS2)"
- Failed parry: "like Super Smash Bros dodge/counter recovery" (`design_decisions_log.md` 2026-03-19)

### World tone (`world_design_v1.md`)
The tone is serious and philosophical, with no humor guidance anywhere:
- Core theme: **"Impermanence vs Control"**: "Life is temporary / Growth requires letting go / Attempts to preserve or control everything lead to stagnation".
- "The villain is not death. The villain is refusing to accept it."
- Final statement: "Mastery is not controlling everything. It is knowing when to let go."

### Player-facing copy voice (the closest the docs come to a style guide)
The status "Identity" column in `status_effects_launch_set_v1.md` is written as terse taglines, most in second person:
- "You're on a clock" (Burning)
- "Recovery is denied" (Bleeding)
- "You can't catch your breath" (Poisoned)
- "You break easily" (Brittle)
- "Magic is shut off" (Silenced)
- "Position solved or punished" (Rooted)
- "Composure collapse" (Fear)
- "Next mistake hurts more" (Exposed)
- "Mastery streak reward" (Focused)

### Implemented UI tone so far
**Loading screen** (`MordecaiLoadingScreenWidget.cpp`):
- "PROJECT MORDECAI": uppercase, 56 pt bold, letter spacing 150, engine Roboto.
- "Loading..." at 20 pt regular, plus a throbber.
- Colors:
  - background (0.010, 0.010, 0.012) near-black
  - title (0.86, 0.80, 0.68), a warm parchment
  - status (0.55, 0.55, 0.55) gray
- Code comment: `TODO(DECISION): the loading screen's art (logo, key art, palette) has no direction yet`.

**Death screen:** "You Died", a respawn countdown and kills (`MordecaiDeathScreenWidget.cpp`).

**Front end is still Lyra.** The 2026-10-30 rebranding audit filed these, all `needs-decision` (`agent_playtest_plan_v1.md`):
- #57: LYRA logo, "PLAY LYRA" / "QUIT LYRA"
- #58: lobby background
- #59: Lyra shooter/party modes ship
- #60: PLAY leads to multiplayer/replay screens
- #61: machine name, online ID, build stamp, ADD PLAYER
- #62: "Lyra's UI style (Orbitron, blue glass)"
- #63: credits
- #64: translated menu text
- #65: menu music/sounds
- #68: Lyra fallback HUD and escape menu

---

## Core loop and meta loop

### Loops (`game_design_v2.md` §3)
- **Moment-to-moment:** "Explore → Read threats → Engage skill combat → Gain meaningful loot/resources/keys → Continue."
- **Session loop (v2):** "Quest/dungeon → miniboss/boss → signature item or upgrade key → return to town → identify loot / refill services / select initiative → unlock new routes → repeat." v1 had "select initiative/services".
- **Campaign loop:** "Town grows → specialists arrive → new gates open → build-defining items acquired → region bosses defeated → final act."

### Village loop (`village_system_v1.md` §5)
1. "go on expedition"
2. "gather gear, magical items, materials, and keys freely"
3. "return because healing stock is getting low, magical items need identification, or an upgrade is now possible"
4. "invest in buildings / initiatives / specialists"
5. "leave town stronger, better supplied, and with new strategic options"

The anti-pattern to avoid is "stop looting because inventory is full… return to town just to empty bags".

### Reasons to return to town
From the village "Return-to-Town Rule":
- identify magical items at the **Mage Tower**
- restock healing and expedition supplies through the **Alchemist**
- convert gathered materials into permanent village growth
- start, complete, or redirect village initiatives
- unlock new routes, services, and world capabilities

"The player should **not** return to town because their bags are full."

`agent_rules_v2.md` §2 names three "return-to-town forcing functions":
1. Mage Tower identification
2. Alchemist healing potion stock limits
3. delivering materials / town resources / upgrade keys

### Progression (`game_design_v2.md` §5)
- **Levels:** max level 60. Leveling grants skill points and minor baseline growth, "not the primary power driver".
- **Attributes:** "Generally static (rarely changed)". They scale HP, stamina, SP, regen, posture and resist.
- **Skills:** max rank 20, in groups "Weapon, defense/armor, movement/exploration, utility/interaction, magic schools". Milestones at Rank 1 / 5 / 10 / 15 / 20 unlock techniques. The catalog has 60 skills; the launch target is ~40–70.
- **Feat tiers:**
  - Common: "modest, no downside"
  - Rare: "strong specialization, light downside/condition"
  - Legendary: "build-defining, meaningful drawback"

### Core resources the player always sees
- **Health:** shown as % (0–100).
- **Stamina:** "Tiered; low stamina reduces effectiveness, never blocks actions". Tier multipliers (`design_decisions_log.md` 2026-03-19):
  - Green (full) 1.25x
  - Yellow (mid) 1.0x
  - Red (low) 0.75x
  - Exhausted (depleted) 0.5x
- **Spell Points:** "Discrete points; slower regen than stamina; INT improves regen; spells can upcast by spending more points."

### Death
None of the design docs define what death costs in the campaign: no corpse run, no loss, no town respawn. The only references:
- `combat_system_v1.md`: "Reaching 0 → death or downed state".
- What is implemented is arena-only (US-053/US-086): a "You Died" screen, a respawn countdown and the kill count, then the arena resets.

### Endings (`world_design_v1.md`)
- **Control Ending:** "Player becomes Lich King / World becomes static".
- **Acceptance Ending (True):** "Systems fail / World begins changing again / NPCs move on".

Core player journey: "Gain control → Understand systems → Question control → Let go". Pillars: "Mechanics reinforce theme / Progression mirrors emotional growth / Backtracking represents reflection".

---

## World and floors

### No floors, stairs, timers or safe rooms
No doc has DCC-style floors, stairwells, floor timers, collapse countdowns or safe rooms.

The world is a **Metroidvania of 4–6 biomes** (`world_design_v1` lists 6). Each biome has "main dungeon + optional micro-dungeons + roaming elites" (`game_design_v2.md` §8). "Returning to older biomes is expected and rewarding."

The town and "Service" cells are the only safe-hub analog.

### The ages (`world_design_v1.md`)
| Age | Method | Result |
|---|---|---|
| Ancient Age | Automation | Lifeless permanence |
| Current Age | Necromancy | Undead stagnation |
| Player Era | Acceptance | Living, changing world |

- **Automaton Age legacy:** "Ancient machinery across all biomes", "Automated systems still running without purpose". Player tools Magnetic Device and Shock Device come from it.
- **Robot race "Continuants":** "Maintain systems indefinitely". Some are broken and hostile, some sentient and questioning. Their conflict: "Should we continue... or stop?"

### Biomes
| # | Biome (emotional arc) | Theme quote | Environment | Enemies | Boss | Unlock |
|---|---|---|---|---|---|---|
| 1 | **Verdant Wilds** (Denial) | "Everything is fine" | Lush and vibrant, subtle unnatural repetition, overgrown ruins; machines overtaken by nature | — | **Thorn Guardian** (regenerates, creates barriers, overgrowth traps) | **Explosives** (break barriers, reveal hidden decay) |
| 2 | **Searing Expanse** (Control) | "We removed chaos... and everything else with it" | Desert of sand and metal, buried cities, exposed machinery | Dust Automatons, Sentinel Frames | **Iron Warden** (strict patterns, punishes deviation, grid-based attacks) | **Magnetic Device** |
| 3 | **Mire of Echoes** (Avoidance) | "Bury it" | Foggy swamp, rotting structures, submerged ruins | Bog Wraiths, Rot Beasts, Drowned Automatons | **Drowned King** (hidden attacks, phases in water, delayed damage) | **Invisibility Potion** (stealth paths) |
| 4 | **Crystal Caverns** (Preservation) | "Freeze this moment forever" | Crystalline structures, frozen scenes, time distortion | Crystal Phantoms, Echo clones | **Crystal Seer** (repeats attacks, freezes zones, delayed echoes) | **Phase Talisman (HP drain)**: phase through barriers, cost drains health |
| 5 | **Storm Peaks** (Obsession) | "Rise above everything" | Floating ruins, strong winds, vertical traversal | Storm Constructs, Aerial Wraiths | **Tempest Ascendant** (aerial, constant movement, anti-ground) | **Glider Cloak** |
| 6 | **Withered Kingdom** (Final Truth / Acceptance) | "This is what happens when you refuse to let go" | Preserved but lifeless city, citizens frozen in time | Undead knights, Royal wraiths | Final boss **The Lich King**: "A reflection of the player", "I chose to preserve what matters"; uses player abilities, controls time and space, resets fight states | — |

### Gates
- Game-design gates are town-driven: "bridges, scaffolding, phase barriers, drainage, siege tools".
- Map-doc gate examples: "explosives wall, magnetic device rail/object, invisibility-required bypass, phase barrier, glider gap".
- "The gate should be legible even before it is usable."

### Room graph and cells (`map_style_technical_design_v1.md`)
- **Room graph node types:** Path, Arena, Gate, Vista, Secret, Service, DungeonEntrance, ShortcutReturn, Transition.
- **Pocket identities:** traversal, combat, puzzle, secret, town/service, boss lead-in.
- **Combat pocket archetypes:** `OpenPocket`, `LanePocket`, `RingPocket`, `CrossroadsPocket`, `AmbushPocket`, `HazardPocket`, `ElevationPocket`, `BossAntechamber`.
- **A cell** has "one traversal idea, one combat or puzzle idea, 2–4 exits, 1 focal landmark, optional secret or return hook".
- **Return value:** "visible unreachable chest, suspicious cracked wall, shortcut door opened later, cliff ledge seen from below first".
- **Shortcut rhythm:** "long forward push → challenge / pocket / gate → reward or reveal → unlock return shortcut".
- **Cell templates:** PathCell, CombatPocketCell, SecretCell, GateCell, LandmarkCell, ShortcutCell, TransitionCell, DungeonEntryCell.
- **Nice-to-have per-cell artifacts:** "minimap preview", "top-down blockout preview", "graph node preview".

### Maps today
Only test maps exist:
- DevTestMap: arena ~2000x2000 cm, white blockout, Frontliner enemies.
- MovementTestMap: no enemies.
- CombatTestMap: TestDummy and TestAttacker.

No biome cell is built. Epic 8 Phase 2 and world gates (US-036) are blocked on human approval.

---

## Village services and screens

### Core pillar (`village_system_v1.md`)
"The village is the game's crafting system, but the player never crafts in the traditional sense." Resources go into infrastructure, which "becomes a **renewable provisioning network**".

Service rule: "If a player unlocks a village service through construction or upgrade, that service should remain available in a renewable form."

### Buildings (`game_design_v2.md` §7)
Blacksmith, alchemist, mage tower, walls/keep, lumber yard, mine, market, clinic.

**Alchemist**, "primary expedition provisioning node":
- Provides health potions, antidotes / cures, resistance tonics and specialty expedition consumables.
- Rule: "Healing pressure should come from Alchemist stock and replenishment, not carry capacity"; "the village can only prepare so many health potions at a time".
- Upgrading means "better potions, in larger quantities, with faster restocks".

**Mage Tower**, "appraisal, identification, and arcane analysis center":
- Provides magical item identification, arcane research, "spell modification / insight systems" and "magical barrier / route unlocking support".
- Items may be "Unidentified" and are "not fully usable until identified".

**Blacksmith:** reinforcement, repair, sharpening, equipment preparation, sidegrade support gear / resist sets. Sharpen gives "temporary damage/posture bonuses".

**Clinic:** recovery services, cleansing / blessing support, survivability preparation, passive health and resilience. Upgrading gives "better recovery kits and blessings".

**Keep / Walls / Defenses:** village safety, event resilience, higher-tier civic expansion, "symbolic proof of long-term settlement advancement".

**Lumber yard, mine, market:** listed, with no role description.

### What the player does in town
**Player controls:**
- "Assign jobs (miners/lumber/research/guards/etc.)"
- "Pick a small number of initiatives (1–3) at a time"
- Upgrade buildings

**Outputs:**
- repair and sharpen
- identify magical items
- research spells/mods
- produce potions/consumables
- "Unlock fast travel routes/shortcuts"
- "Improve SP regen/cost via research lines (not raw damage creep)"

**Events:** "Protect workers, reclaim mine from goblins, defend caravan, etc. One mission/decision—never chores."

**Specialists:** they "arrive" (campaign loop). Resources are used for "attracting or sustaining specialists". CHA "focuses on town attraction, dialogue leverage, vendor outcomes" (`attribute_scaling_v1.md`).

### Building tiers
- Tier 1: **Functional**
- Tier 2: **Improved**
- Tier 3: **Advanced**
- Tier 4: **Masterwork**

### Resources
- **Common Materials:** Wood, Stone, Iron, Fiber, Herbs, Hides, Monster Parts.
- **Refined Materials:** Lumber, Cut Stone, Iron Ingots, Alchemical Extract, Arcane Ink.
- **Upgrade Keys / Relics:** Philosopher's Stone, Heartwood Core, Deepcore Prism, Saint's Ember, Phase Lens, Siege Sigil. These are "Rare progression unlockers used to break upgrade caps or unlock special structures". v1 notes "formerly 'Relics'".
- Auto-storage: all materials and town resources "auto-stored when collected".

### Population (§3)
- **Population**
- **Housing Capacity**
- **Support Capacity**, influenced by housing, food, health / sanitation, safety / order, economy / livelihood
- **Healthy Margin**, with four states:
  - support well above demand: villagers are attracted
  - barely meeting: growth stalls
  - below: strained
  - far below: villagers leave or services degrade

"understandable and low-friction, not a spreadsheet simulator."

### What exists today
- Identification is headless: `UMordecaiIdentificationService::IdentifyInstance(..., "MageTower")`, "free and instant", with `TODO(DECISION)` on cost (US-033).
- An **Identify** button lives in the pause-menu Inventory tab.
- A **ledger** panel lists auto-stored materials (e.g. "Iron Ore x1", "Town Stone x1").
- No Mage Tower NPC, town map or service screen exists.
- Planned stories:
  - US-037 Town Building Framework
  - US-038 NPC Job Assignment
  - US-039 Initiatives & Upgrades
  - US-040 Town Events
  - US-074 Town Management UI
  - US-073 Metroidvania World Map
  - US-072 Equipment Panel

### Open decisions
- "Exact identification UX (single-item, batch, timed, or service queue)"
- "Exact potion field-cap model from the Alchemist"
- From agent_rules Open Items: identification cost (gold/resource/service), partial hints, whether the potion cap is "hard cap, soft stock cap, or refill-style provisioning rule"

---

## Visual style rules from the map docs

### Palette and lighting
**No doc defines a palette, hex colors, lighting mood or typography.** The loading-screen code says the palette "has no direction yet". The only lighting requirement is US-054's "adequate lighting for the diorama camera".

### Camera
- Locked: "Camera **follows the character**", "**does not rotate** (no free-look; no yaw/pitch driven by player input)".
- Allowed: "Screen shake / camera effects that do not change camera facing" and "Minor positional smoothing/lag".
- Implemented: pitch −50, yaw 0, looking along +X, so screen up = +X and screen right = +Y.

### Readability rules (`map_style_technical_design_v1.md`)
- "The player should understand the collision story of a room in under 1 second."
- "walkable floor is broad and uncluttered"; "cliffs/walls have chunky silhouettes"; "hazards use obvious texture/material contrast"; "water, pits, brush, and blockers read instantly"; "decorative detail is concentrated near edges".
- "A 20-second traversal… should usually include at least one of: decision point, enemy pocket, traversal gate, secret hint, interactable, landmark."
- One-screen legibility: understandable without camera manipulation are "enemy positions, dodge lanes, climbable edges, bombable walls, phase barriers, **interact prompts**, exits / shortcuts".
- Occlusion discipline: no tall decor that hides enemies, covers interactables, obscures gate affordances or blocks projectile readability.
- "Never over-dress the center of combat space."
- Non-goal: "visually noisy scenes that obscure combat reads".

### Scale and height tiers
- "Prefer dense authored space over realistic scale"; "corridors are short, arenas are compact, landmarks are always close, walls are thick and readable".
- Tier 0: Ground. Tier 1: Ledge. Tier 2: WallTop. Tier −1: Lower (pit, trench, lower route, water edge).
- "clear tiers of elevation, not noisy freeform terrain".

### Kits (§6.2)
- **Readability kit:** "path borders, signposts, torch lines, flowers / shrubs / stones used as path framing, contrast decals".
- **Landmark kit:** shrine, statue, giant tree root, broken automaton, crystal outcrop, tower fragment.
- **Traversal kit:** climb markers, jump edges, bombable panels, magnetic anchors, glide launch points, phase barriers.
- **Dressing kit:** bushes, crates, ruins, carts, fences.

### Status and rarity reads
- Status is shown through world VFX, e.g. Exposed: "Glowing target sigil hovering in-world"; Blinded: "Dark vignette + smeared highlights".
- **Rarity tiers:** "Common Green Blue Purple Red / Gold" (`item_system_v1.md`). Magical items are "Green/Blue/Purple/Red(Gold)".

### Known readability problems
- N21: the near wall hides the player; cutaway is undecided (US-099).
- Pickup labels "render light gray and the far labels are faint on the light floor".

---

## Constraints on UI from agent_rules

**Camera:**
- No camera-rotation UI or controls.
- Interaction selection must work without rotation: "auto-target, cone checks, closest-in-front logic, or explicit target cycling".

**Inventory (§2, Locked):**
- "effectively unlimited". Do NOT implement "bag slots, container tetris, or specialized bags".
- "flat, player-friendly carry model with strong sorting/filtering".
- Categories: `Weapon`, `Armor`, `Trinket`, `Consumable`, `Material`, `TownResource`, `UpgradeKey`, `QuestItem`, `MagicalItem`.
- Default sort priority: "quest items, upgrade keys, magical items, then common gear".
- Materials and town resources auto-store into a "shared progression inventory / resource ledger".
- "Surface unidentified magical items clearly in UI and tooltips."
- "Preserve strong organization, filtering, and search so unlimited carry remains readable."
- Consumables: "readable limits where needed", but healing scarcity comes from Alchemist stock.

**HUD (`game_design_v2.md` §9):**
- "Always visible: Health%, tiered stamina, spell points."
- Status "primarily via VFX + animation; minimal critical UI only".
- "materials auto-stored, signature items prioritized in sorting, no loot vomit, no bag-slot micromanagement".
- "Unidentified magical items should be clearly signposted in inventory and town UI."

**Open items (do not guess):** unidentified flow, potion cap, buff stacking, gameplay tag taxonomy, PlayFab writeback timing.

**UI tech stack** (`ue5_tech_stack_context.md`):
- Common UI for "main menu, character select, inventory, skill tree, overlays/pause menu, modal flows".
- MVVM for "character sheet, buff/debuff tray, party frames, cooldown bars, item tooltips & compare panels".
- "Keep UI 'thin'".
- PlayFab lobby / party / matchmaking implies a multiplayer front end.

**Input decisions (`design_decisions_log.md`):**
- Pickup prompt "[F] Pick up <item>"; UI assist hint "[I] Inventory".
- Esc / Gamepad Start toggles the pause menu to the last-active tab. I opens it on Inventory and closes it on key-up.
- LB/RB cycle tabs, wrapping "Character ↔ Settings".
- "Device-aware key glyphs (controller glyphs when on gamepad) deferred".
- Default pause tabs (`MordecaiPauseMenuTypes.h`): Character, Skills, Feats, Inventory, Settings.

**Testability (affects how new UI is built):**
- The Tier 4 bridge lists only `Mordecai*` widgets.
- `click_button` finds buttons by their text.
- `UMordecaiTabButton` ids are lowercase (`"inventory"`, `"all"`, `"weapons"`).
- Lyra front-end widgets cannot be clicked by name.

**Packaged-build rules:**
- No PIE-only UI.
- Front-end entries are `LyraUserFacingExperienceDefinition` playlists under `/MordecaiCore/System/Playlists` (e.g. `DA_Mordecai_Arena`).

---

## Terminology
See `key_terms`. Exact spellings matter: "Spell Points" (SP), "Upgrade Keys", "Mage Tower", "Unidentified", "initiatives", "Continuants", the biome names, Tier names "Functional/Improved/Advanced/Masterwork", stamina tiers "Green/Yellow/Red/Exhausted".

## UI INVENTORY
- [Main menu / front end|implemented] **Main menu (title screen)**: Player entry point; launch the game, quit | data: Currently Lyra: LYRA logo, 'PLAY LYRA' / 'QUIT LYRA', machine name, online ID, engine build stamp, ADD PLAYER, lobby background, Lyra music; Orbitron + 'blue glass' style | states: Lyra-branded (needs rebrand, issues #57/#58/#61/#62/#64/#65 needs-decision) (Specs/agent_playtest_plan_v1.md (issue #39 entry, rebranding audit #57-#69); Scripts/playtest_boot.py --frontend (L_LyraFrontEnd, B_LyraFrontEnd_Experience))
- [Main menu / front end|implemented] **Experience / playlist select (arena tile)**: Choose what to play; front-end path to the Mordecai arena | data: Playlist tiles (LyraUserFacingExperienceDefinition) e.g. DA_Mordecai_Arena; currently also Lyra shooter/party modes | states: PLAY currently leads to Lyra multiplayer/replay screens (#60); Lyra modes still ship (#59) (stories/PLANNER.md §5.1 item 1; Specs/agent_rules_v2.md NO PIE-ONLY BEHAVIOR; agent_playtest_plan_v1.md #59/#60)
- [Main menu / front end|specified] **Character select / lobby / party / matchmaking**: Multiplayer meta: pick character, group up, matchmake (PlayFab) | data: Character roster, party roster, lobby; 'party frames' | states: not built (Specs/ue5_tech_stack_context.md §1 (Common UI for main menu, character select), §4 (character roster), §7 (Lobby/Party/Matchmaking))
- [Main menu / front end|implemented] **Credits screen**: Credits | data: Currently Lyra credits | states: Lyra-branded (#63) (Specs/agent_playtest_plan_v1.md #63)
- [Loading / transition|implemented] **Loading screen**: Shown on every map/experience load | data: 'PROJECT MORDECAI' (uppercase, 56pt bold, letter spacing 150, Roboto, color 0.86/0.80/0.68), 'Loading...' (20pt, gray 0.55), throbber; near-black background 0.010/0.010/0.012 | states: shown / held ~2 s after experience load; final art TODO(DECISION) 'logo, key art, palette has no direction yet' (Source/LyraGame/Mordecai/UI/MordecaiLoadingScreenWidget.cpp; agent_playtest_plan_v1.md issue #39)
- [Loading / transition|implied] **Biome / area title card**: Tell the player which biome/region they entered; reinforce the emotional arc | data: Biome names: Verdant Wilds, Searing Expanse, Mire of Echoes, Crystal Caverns, Storm Peaks, Withered Kingdom (arc: Denial, Control, Avoidance, Preservation, Obsession, Acceptance) | states: per biome; possibly first visit vs revisit (Specs/world_design_v1.md Biomes)
- [HUD|implemented] **Core resource cluster (Health %, tiered Stamina, Spell Points)**: Always-visible core resources (pillar 8 'Clean UI') | data: Health as % 0-100; Stamina with tiers Green (1.25x) / Yellow (1.0x) / Red (0.75x) / Exhausted (0.5x); Spell Points as discrete points (current impl MaxSpellPoints 10); Posture bar also implemented | states: stamina tier changes; SP spent/upcast; low health (Specs/game_design_v2.md §2 pillar 8, §4, §9; Specs/design_decisions_log.md 2026-03-19; Source/LyraGame/Mordecai/UI/Mordecai{Health,Stamina,Posture,SpellPoints}BarWidget.h)
- [HUD|implemented] **Status effect readout (minimal critical UI)**: Statuses communicated primarily via VFX + animation; minimal critical UI only, 'not icon soup' | data: 16 launch statuses (Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched, Focused (Buff)); current indicator shows 'Status 10.0' not the name (N16) | states: active/expiring; buff vs debuff (Specs/game_design_v2.md §9, §10; Specs/status_effects_launch_set_v1.md; agent_playtest_plan_v1.md N16)
- [HUD|implied] **Healing potion count / consumable quick-use**: Show remaining field potions; healing scarcity comes from Alchemist stock | data: Potion count vs field cap (cap model undecided) | states: full / low / empty; restock in town (Specs/village_system_v1.md §4.1 Alchemist Rule; Specs/agent_rules_v2.md §2 Consumable Rule, Open Item 5)
- [HUD|implemented] **Interact / pickup prompt**: Prompt the deliberate pickup/talk/activate press | data: '[F] Pick up <item>' (keyboard glyphs only; X on gamepad per bindings) | states: focused pickup / none; device-aware glyphs deferred (Specs/design_decisions_log.md 2026-07-10 Press-to-Pickup; Source/LyraGame/Mordecai/UI/MordecaiPickupPromptWidget.h)
- [HUD|implemented] **UI assist hint**: Remind the inventory key | data: '[I] Inventory' | states: visible in play (Specs/design_decisions_log.md 2026-07-10)
- [World-space|implemented] **Pickup name labels**: Name items on the ground, readable from the fixed camera | data: Item display name (billboard UTextRenderComponent) | states: near/far (far labels faint on light floor, unfiled readability issue) (Specs/agent_playtest_plan_v1.md known issues (issues #30/#38))
- [World-space|implied] **Gate / traversal affordance markers**: 'The gate should be legible even before it is usable'; show which tool/town upgrade opens it | data: Gate types: explosives wall, magnetic device rail/object, invisibility bypass, phase barrier, glider gap; town gates: bridges, scaffolding, phase barriers, drainage, siege tools | states: locked (requirement unknown) / locked (requirement known) / usable / opened (Specs/map_style_technical_design_v1.md §3.3, §6.2; Specs/game_design_v2.md §8)
- [HUD|implied] **Boss health bar + name card**: Region boss fights | data: Thorn Guardian, Iron Warden, Drowned King, Crystal Seer, Tempest Ascendant, The Lich King; enemy Posture (break -> critical/riposte window) | states: phases (Drowned King phases in water; Lich King 'resets fight states'); posture broken / staggered / knocked down (Specs/world_design_v1.md; Specs/game_design_v2.md §4; Specs/design_decisions_log.md (stagger 1 s, knock down))
- [HUD|implied] **Traversal tool equip / use indicator**: Show the acquired biome tools and costs | data: Explosives, Magnetic Device, Shock Device, Invisibility Potion, Phase Talisman (cost: drains health), Glider Cloak | states: not acquired / acquired / active (Phase Talisman draining HP) (Specs/world_design_v1.md Unlocks, Automaton Age legacy)
- [Notification / overlay|implied] **Tool / key item acquired banner**: Celebrate the biome unlock or signature item / upgrade key (session loop payoff) | data: Item name + what it unlocks (e.g. Philosopher's Stone unlocks Mage Tower tier) | states: one-shot (Specs/game_design_v2.md §3 session loop, §6 Upgrade Keys; Specs/world_design_v1.md)
- [Notification / overlay|implied] **Feat unlocked (achievement-style) toast**: Feats are earned via gameplay achievements | data: Feat name, tier Common/Rare/Legendary, trigger (e.g. 'burn 6 enemies at once -> Pyromaniac'), drawback for Rare/Legendary | states: Common / Rare / Legendary (Specs/game_design_v2.md §2 pillar 3, §5 Feats)
- [Notification / overlay|implied] **Skill rank-up / milestone toast**: 'Everything you do is a skill' (DCC vibe); ranks increase from use | data: Skill name, rank (max 20), milestone unlock at Rank 1/5/10/15/20 | states: rank up / milestone technique unlocked (Specs/game_design_v2.md §2 pillar 2, §5 Skills)
- [Notification / overlay|implied] **Level up toast**: Character level progression | data: Level (max 60, 'WoW nod'), skill points granted | states: one-shot (Specs/game_design_v2.md §5 Levels)
- [Notification / overlay|implemented] **Death screen**: Show death and respawn (arena flow) | data: 'You Died', respawn countdown, kills | states: dead -> countdown -> respawn; campaign death consequence undefined (Source/LyraGame/Mordecai/UI/MordecaiDeathScreenWidget.cpp; agent_playtest_plan_v1.md N2; Specs/combat_system_v1.md ('death or downed state'))
- [In-game menu|implemented] **Pause menu (tabbed)**: Hub for character, skills, feats, inventory, settings | data: Tabs: Character, Skills, Feats, Inventory, Settings | states: open/closed; last-active tab remembered; Esc/Start toggle; I opens on Inventory and closes on key-up; LB/RB cycle tabs with wrap (Character <-> Settings); mouse clicks tab headers (Source/LyraGame/Mordecai/UI/MordecaiPauseMenuTypes.h; Specs/design_decisions_log.md 2026-07-10, 2026-10-30)
- [In-game menu|implemented] **Inventory (flat list, filter, ledger)**: Unlimited-carry, readable inventory with strong sort/filter/search | data: Categories Weapon, Armor, Trinket, Consumable, Material, TownResource, UpgradeKey, QuestItem, MagicalItem; sort priority quest items > upgrade keys > magical items > common gear; rarity Common/Green/Blue/Purple/Red(Gold); Identify button; ledger rows like 'Iron Ore  x1', 'Town Stone  x1' | states: empty; filtered; unidentified rows; no bag slots/weight (Specs/agent_rules_v2.md §2; Specs/game_design_v2.md §6, §9; stories/PLAN.md Epic 7.5 (US-071/US-079))
- [In-game menu|implemented] **Unidentified item signposting (inventory + tooltips + town)**: 'Surface unidentified magical items clearly in UI and tooltips' | data: Partial label 'Unidentified <RarityName> <SubtypeOrType>' (e.g. 'Unidentified Purple Sword') or 'Unknown Item'; only Rarity/Type tags visible; short description if partial info allowed | states: Unidentified / Identified; RequiresIdentificationToEquip blocks equip (stories/done/US-033-unidentified-items-town-gating.md; Specs/game_design_v2.md §9)
- [In-game menu|implemented] **Resource ledger / town storage**: Auto-stored materials and town resources; feeds village growth | data: Common materials (Wood, Stone, Iron, Fiber, Herbs, Hides, Monster Parts), refined (Lumber, Cut Stone, Iron Ingots, Alchemical Extract, Arcane Ink), Upgrade Keys (Philosopher's Stone, Heartwood Core, Deepcore Prism, Saint's Ember, Phase Lens, Siege Sigil) with counts | states: empty / counts; key used/unused (Specs/village_system_v1.md §1; Specs/agent_rules_v2.md §2 Material Rule)
- [In-game menu|planned-story] **World map (Metroidvania)**: Room-graph map of biomes and cells; Cartography skill adds detail | data: Explored routes; landmarks; secret passages and shortcuts once found; merchants, shrines, hazards (Cartography ranks); gates by requirement; node types Path/Arena/Gate/Vista/Secret/Service/DungeonEntrance/ShortcutReturn/Transition | states: unexplored / partially explored / explored; gate locked/open; shortcut unlocked (stories/PLAN.md US-073; Specs/skill_sheet_v1.1.md Cartography; Specs/map_style_technical_design_v1.md §3.2, §9)
- [Other|implied] **Fast travel route selection**: Use town-unlocked fast travel routes/shortcuts | data: Unlocked routes/destinations | states: locked / unlocked (Specs/game_design_v2.md §7 Outputs ('Unlock fast travel routes/shortcuts'))
- [HUD|implied] **Quest / objective tracker**: Session loop starts with 'Quest/dungeon'; QuestItem category; landmarks used for 'quest destination hinting' | data: Current quest/dungeon objective | states: active / complete (Specs/game_design_v2.md §3; Specs/agent_rules_v2.md §2 categories; Specs/map_style_technical_design_v1.md §8.3)
- [Village / service screen|specified] **Mage Tower: identification service**: Identify magical items (primary return-to-town forcing function); also arcane research, spell modification/insight, barrier/route support | data: Unidentified items list, identify action, result reveal; cost model undecided (currently free and instant) | states: single vs batch vs timed vs service queue (open decision); no items to identify (Specs/village_system_v1.md §4.2; Specs/agent_rules_v2.md Open Item 4; Specs/game_design_v2.md §12; stories/done/US-033 (service only, no UI))
- [Village / service screen|implied] **Alchemist: provisioning / restock**: Restock health potions, antidotes/cures, resistance tonics, specialty consumables | data: Stock levels, restock rate, field cap per potion type, building tier | states: stock full / low / empty; restocking (Specs/village_system_v1.md §4.1; Specs/agent_rules_v2.md §2 Consumable Rule)
- [Village / service screen|implied] **Blacksmith services**: Reinforcement, repair, sharpening (temporary damage/posture bonus), equipment preparation, sidegrade/resist sets | data: Equipment list, service options, bonus duration | states: available by building tier (Specs/village_system_v1.md §4.3; Specs/game_design_v2.md §7)
- [Village / service screen|implied] **Clinic services**: Recovery services, cleansing/blessing, survivability prep, passive resilience | data: Blessings/prep buffs offered (e.g. 'Clinic prep buff', 'town clinic blessing') | states: available by building tier (Specs/village_system_v1.md §4.4; Specs/status_effects_launch_set_v1.md (cleanses))
- [Village / service screen|planned-story] **Building upgrade screen**: Invest materials and upgrade keys into buildings | data: Buildings: blacksmith, alchemist, mage tower, walls/keep, lumber yard, mine, market, clinic; tier 1-4 Functional/Improved/Advanced/Masterwork; costs in common/refined materials + upgrade key requirements | states: locked / affordable / not affordable / under construction / max tier; 'upgrade now possible' must be noticeable (Specs/village_system_v1.md §2; Specs/game_design_v2.md §7; stories/PLAN.md Epic 9 US-037/US-039, Epic 10 US-074)
- [Village / service screen|planned-story] **Job assignment**: Assign villagers to jobs (delegated town) | data: Jobs: miners, lumber, research, guards, etc.; villager counts | states: assigned / idle (Specs/game_design_v2.md §7; stories/PLAN.md US-038)
- [Village / service screen|planned-story] **Initiatives selection**: Pick 1-3 initiatives at a time; start, complete, or redirect | data: Initiative list, active slots (1-3), progress | states: inactive / active / complete / redirected (Specs/game_design_v2.md §7; Specs/village_system_v1.md Return-to-Town Rule; stories/PLAN.md US-039)
- [Village / service screen|specified] **Village status / population panel**: Legible view of village health without a 'spreadsheet simulator' | data: Population, Housing Capacity, Support Capacity (housing, food, health/sanitation, safety/order, economy/livelihood), Healthy Margin | states: attracting (support well above demand) / stalled (barely meeting) / strained (below) / villagers leaving or services degrading (far below) (Specs/village_system_v1.md §3)
- [Notification / overlay|planned-story] **Town event / mission prompt**: Occasional one-decision events | data: Examples: protect workers, reclaim mine from goblins, defend caravan | states: offered / active / resolved (Specs/game_design_v2.md §7 Events; stories/PLAN.md US-040)
- [Notification / overlay|implied] **Specialist arrived / route unlocked notices**: Campaign loop beats: 'specialists arrive -> new gates open' | data: Specialist / route name, what it unlocks | states: one-shot (Specs/game_design_v2.md §3 Campaign loop; Specs/village_system_v1.md §1 Resource Handling)
- [Village / service screen|implied] **NPC dialogue**: Talk to villagers/specialists (Interact = 'Talk, loot, activate'); CHA affects dialogue leverage and vendor outcomes | data: Speaker, lines, choices | states: not specified (Specs/design_decisions_log.md 2026-07-10 (control_bindings 'Talk, loot, activate'); Specs/attribute_scaling_v1.md CHA)
- [Loading / transition|implied] **Ending screens**: Two endings | data: Control Ending ('Player becomes Lich King / World becomes static'); Acceptance Ending (True) ('Systems fail / World begins changing again / NPCs move on') | states: Control / Acceptance (Specs/world_design_v1.md Endings)

## KEY TERMS
- Diorama camera: fixed 3rd-person camera that follows the character and never rotates (pitch -50, yaw 0; screen up = +X)
- DCC vibe: the only Dungeon Crawler Carl reference in the docs; means 'Everything you do is a skill' granular skill progression
- Skill: granular use-based ability, max rank 20; milestones at Rank 1/5/10/15/20 unlock techniques/passives
- Feat: earned (not picked) via gameplay achievements; tiers Common / Rare / Legendary (Rare/Legendary carry drawbacks)
- Health %: health always shown as 0-100%
- Stamina tiers: Green (full, 1.25x) / Yellow (mid, 1.0x) / Red (low, 0.75x) / Exhausted (depleted, 0.5x); low stamina never blocks actions
- Spell Points (SP): discrete spell resource; slower regen than stamina; spells can upcast by spending more
- Posture / Stability: enemy Posture breaks into stagger (1 s, 1.5x) then Knock Down (2.0x); player Stability resists stagger
- Expedition: a trip out of town into the world (village docs' word for a run)
- Village / Town: delegated, renewable provisioning network; 'the village is the game's crafting system'
- Initiatives: 1-3 active town projects the player selects
- Upgrade Keys (formerly 'Relics'): rare unlockers for town upgrades/caps: Philosopher's Stone, Heartwood Core, Deepcore Prism, Saint's Ember, Phase Lens, Siege Sigil
- Common Materials: Wood, Stone, Iron, Fiber, Herbs, Hides, Monster Parts (auto-stored)
- Refined Materials: Lumber, Cut Stone, Iron Ingots, Alchemical Extract, Arcane Ink
- Resource ledger: persistent auto-store for Materials/TownResources (shown in the Inventory tab)
- Unidentified: magical-item state; not fully usable until identified at the Mage Tower; label 'Unidentified <Rarity> <Type>' or 'Unknown Item'
- Mage Tower: identification, arcane research, spell modification, barrier/route unlocking
- Alchemist: potion provisioning; healing scarcity comes from its stock, not bag space
- Blacksmith: reinforcement, repair, sharpening, equipment prep, resist sets
- Clinic: recovery, cleansing/blessing, survivability prep
- Keep / Walls / Defenses: village safety, event resilience
- Building tiers: Tier 1 Functional, Tier 2 Improved, Tier 3 Advanced, Tier 4 Masterwork
- Population / Housing Capacity / Support Capacity / Healthy Margin: village sustainability vocabulary
- Rarity: Common, Green, Blue, Purple, Red/Gold (magical items: Green/Blue/Purple/Red(Gold))
- Item categories: Weapon, Armor, Trinket, Consumable, Material, TownResource, UpgradeKey, QuestItem, MagicalItem
- Biomes: Verdant Wilds (Denial), Searing Expanse (Control), Mire of Echoes (Avoidance), Crystal Caverns (Preservation), Storm Peaks (Obsession), Withered Kingdom (Final Truth/Acceptance)
- Bosses: Thorn Guardian, Iron Warden, Drowned King, Crystal Seer, Tempest Ascendant, The Lich King (final)
- Unlock tools: Explosives, Magnetic Device, Shock Device, Invisibility Potion, Phase Talisman (HP drain), Glider Cloak
- Continuants: the robot race built to maintain systems indefinitely
- Ages: Ancient Age (Automation), Current Age (Necromancy), Player Era (Acceptance)
- Impermanence vs Control: core theme
- Control Ending / Acceptance Ending (True): the two endings
- Cell: smallest authored world chunk (one traversal idea, one combat/puzzle idea, 2-4 exits, 1 landmark)
- Room graph node types: Path, Arena, Gate, Vista, Secret, Service, DungeonEntrance, ShortcutReturn, Transition
- Combat pocket archetypes: OpenPocket, LanePocket, RingPocket, CrossroadsPocket, AmbushPocket, HazardPocket, ElevationPocket, BossAntechamber
- Height tiers: Tier 0 Ground, Tier 1 Ledge, Tier 2 WallTop, Tier -1 Lower
- Gate: route blocked until a tool or town upgrade (explosives wall, magnetic rail, invisibility bypass, phase barrier, glider gap, bridges, scaffolding, drainage, siege tools)
- Return shortcut: a route unlocked after a forward push to cut backtracking
- Pause menu tabs: Character, Skills, Feats, Inventory, Settings
- Interact (F / gamepad X): 'Talk, loot, activate'; prompt '[F] Pick up <item>'

## OPEN QUESTIONS
- Dungeon Crawler Carl usage: the docs contain only 'Granular skills (DCC vibe)'. There are no crawlers, floors, stairwells, safe rooms, floor timers, loot boxes, System AI announcer, viewers or sponsors, and no explanation of the name 'Mordecai' (in the books Mordecai is the crawlers' game guide). How literal should the DCC half of the mashup be in UI: a snarky System/announcer voice, achievement pop-ups, loot-box-style reward reveals, a guide character?
- Tone conflict: the world doc is serious and philosophical ('Impermanence vs Control', 'The villain is not death'), while DCC is satirical. Which voice should UI copy, notifications and achievements use?
- Palette, typography and key art: no doc defines colors, fonts or a logo. The loading screen has TODO(DECISION) 'logo, key art, palette has no direction yet', and the front-end rebrand issues #57-#69 are all needs-decision. What is the art direction (e.g. Link's Awakening Switch toy/clay palette)?
- Campaign death consequence is undefined: what is lost, where the player respawns (town?), whether there is a downed state (combat_system_v1: 'death or downed state'). Only the arena death screen ('You Died', countdown, kills) exists.
- Identification UX is an open decision: single item, batch, timed or service queue; whether it costs gold, resources or a service fee; whether partial hints show (agent_rules Open Item 4, game_design_v2 §12).
- Potion field-cap model is open: hard cap, soft stock cap or refill-style provisioning (agent_rules Open Item 5). This sets the HUD potion counter and the Alchemist screen.
- Lumber yard, mine and market are listed buildings with no role description. Is there a currency (bandits drop gold; item_schema has CurrencyProxy; CHA affects 'vendor outcomes') and a vendor/market screen?
- Is the game single-player or multiplayer at launch? The tech stack specifies PlayFab lobby, party, matchmaking, character select and party frames, and First Aid mentions 'reviving downed allies or companions'. This decides the scope of the front end and HUD party frames.
- Biome count: game_design says 4-6, world_design lists 6. Is there a dungeon or floor layer inside each biome that needs its own UI (dungeon map, keys, boss door)?
- World map form: US-073 is planned but unspecified (room-graph node map vs screen-grid map like Link's Awakening vs a minimap). Is there an in-HUD minimap?
- The 'I' inventory key and in-menu LB/RB are not yet in control_bindings_v1.1 (TODO(DECISION)). Device-aware gamepad glyphs are deferred.
- Rarity colors: item_system_v1 lists 'Common Green Blue Purple Red / Gold' but gives no color for Common and no exact values. Is the top tier 'Red' or 'Gold' in UI?
- Readability decisions pending that affect HUD and world-space UI: wall cutaway/fade so the player is never hidden (US-099), and whether facing/aim follows mouse/stick (US-098).