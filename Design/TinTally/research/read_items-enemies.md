# Project Mordecai: items, loot, inventory, enemies and AI (UI research)

All paths are relative to `C:\Users\jeffd\Documents\Gamedev\ProjectMordecai`. Linear colors are UE `FLinearColor` (0–1). "Code" means it exists in `Source/LyraGame/Mordecai/`.

## Item categories and equipment slots

**The specs list types at two levels of detail:**
- `Specs/item_system_v1.md` lists 5 categories: Weapons, Armor, Trinkets, Consumables, Upgrade Keys.
- `Specs/item_schema_v2.md` §3 lists 10 recommended item types: `Weapon`, `Armor`, `Trinket`, `Consumable`, `Material`, `TownResource`, `MagicalItem`, `QuestItem`, `UpgradeKey`, `CurrencyProxy`.
  - It explicitly removes `Bag`: "Bag items are not part of the default launch inventory model."
- `Specs/agent_rules_v2.md` §2 lists the same categories except CurrencyProxy.

**The code enum is `EMordecaiItemType`** (`Items/MordecaiItemTypes.h`). Order: Weapon, Armor, Trinket, Consumable, Material, TownResource, UpgradeKey, QuestItem, MagicalItem, CurrencyProxy.

**Player-facing type strings** (`Items/MordecaiItemLibrary.cpp` `ItemTypeDisplayString`):

| Enum value | String shown |
|---|---|
| Weapon | "Weapon" |
| Armor | "Armor" |
| Trinket | "Trinket" |
| Consumable | "Consumable" |
| Material | "Material" |
| TownResource | "Resource" |
| UpgradeKey | "Upgrade Key" |
| QuestItem | "Quest Item" |
| MagicalItem | "Magical Item" |
| CurrencyProxy | "Currency" |
| (fallback) | "Item" |

**Tags and helpers:**
- Tags: `Mordecai.Item.Type.{Weapon,Armor,Trinket,Consumable,Material,TownResource,UpgradeKey,QuestItem,MagicalItem}`. CurrencyProxy has no tag.
- `IsEquippable()` is true for Weapon, Armor, Trinket and MagicalItem.
- `Subtype` is a free FName, for example "Sword", "Herb", "Amulet".

**Equip slots:**
- The schema example uses `EquipSlot: MainHand`.
- Code `EMordecaiEquipSlot` (`Weapons/MordecaiWeaponTypes.h`): MainHand, OffHand, TwoHand. These are weapon slots only.
- `UMordecaiEquipmentComponent` holds MainHandWeapon and OffHandWeapon.
- No armor or trinket slots exist anywhere.
- Placeholder stories that need design input: US-027 (Ranged), US-028 (Two-Weapon), US-029 (Armor: Leather, Chain, Plate), US-030 (Shield).
- US-072 "Equipment Panel — Weapon Sets & Armor" is planned for after Epic 6+7 (`stories/PLAN.md`).

**Weapons:**
- `EMordecaiWeaponType`: Longsword, Greatsword, Shortsword, Dagger, Axe, Mace, Spear, Quarterstaff, Unarmed, Longbow, Shortbow, Crossbow, Throwable, Wand.
- `UMordecaiWeaponDataAsset` fields usable for comparison: WeaponId, DisplayName, WeaponType, EquipSlot, Rarity, BaseDamage, AttackSpeedMultiplier (1.0 = normal), Range (cm), PostureDamageBonus, LightAttackProfiles (combo chain), HeavyAttackProfile, StatModifiers (Attribute + Op `Add`/`Multiply`/`PercentAdd` + Value), GrantedAbilities, GrantedTags.
- HUD `UMordecaiEquippedWeaponWidget` shows the weapon name and type, or "Unarmed".
- Control bindings specify "Weapon Set Swap (Set 1 ↔ Set 2)" on Tab / Y (hold). It is deferred; the slice cycles weapons linearly (US-077).

**Item content named in the specs:**
- Alchemist consumables: health potions, antidotes / cures, resistance tonics, specialty expedition consumables (`Specs/village_system_v1.md` §4.1).
- Cleanse and mitigation items (`Specs/status_effects_launch_set_v1.md`): Douse item, Bandage/First Aid, Antidote, Heat item, Grounding charm, Fortify tonic, Eye drops, Courage charm, anti-curse talisman, Neutralizer item, anti-silence charm, towel charm.
- Throwables: "knife/bomb/vial" (Throw: G tap / Y tap).
- Crafted gear: "repair, sharpen, oils, resist sets".
- Biome unlock tools (`Specs/world_design_v1.md`): Explosives, Magnetic Device, Invisibility Potion, Phase Talisman (HP drain), Glider Cloak, plus the Shock Device. No spec gives these an item type.

**Quickslots:**
- "Use Item A/B/C/D" are on D-pad Down/Left/Up/Right and keys 1/2/3/4: "Quickslot item use" (`Specs/control_bindings_v1.1.md`).
- Not implemented.
- Key `One` is bound to Spell4 in `MordecaiHeroComponent.cpp:216`, which conflicts with Use Item A.

**Materials** (`village_system_v1.md` §1):
- Common Materials: Wood, Stone, Iron, Fiber, Herbs, Hides, Monster Parts.
- Refined Materials: Lumber, Cut Stone, Iron Ingots, Alchemical Extract, Arcane Ink.
- Upgrade Keys / Relics: Philosopher's Stone, Heartwood Core, Deepcore Prism, Saint's Ember, Phase Lens, Siege Sigil.
- game_design_v2 gives an example: "Philosopher's Stone unlocks Mage Tower tier". Upgrade keys are "Not wearable gear".

**Item definition fields:**
- In code (`UMordecaiItemDefinition`): ItemId, DisplayName, Description, ShortDescription, Icon; ItemType, Subtype, Rarity, Tags; CarryModel, AutoStoreOnPickup, StorageDomain, SortPriority; Stackable, MaxStackSize; BindType, DropOnDeath, Tradable; UsesIdentification, DefaultIdentificationState, RequiresIdentificationToEquip, ShowPartialInfoBeforeIdentify, IdentificationService ("MageTower").
- Schema only, not in code:
  - EquipRules: RequiredLevel (example 10), RequiredTags, ForbiddenTags, AllowedClasses.
  - StatModifiers. Examples: WeaponDamage Add 18, CritChance Add 0.05, FireDamageScalar Add 0.12. Ops: Add, Multiply, Override, PercentAdd.
  - GrantedAbilities, GrantedEffects, GrantedTags, ProcRules, Affixes.
  - Durability: UsesDurability false, MaxDurability 0.
  - SpecialRules: BossDropGuaranteed, WorldPlaced, ContainerPlaced.
  - Economy: SellValue 250, BuyValue 0, SalvageTableId `salvage_weapon_purple`.
  - Presentation: RarityColor Purple, EquipFX `NS_Weapon_Embers`, InspectText.
  - Optional fields: WorldMesh, SortOrder, LoreText.

**Test items in the build** (`Scripts/create_test_pickups.py`, `/MordecaiCore/Items/TestPickups/`):

| Item | Type | Rarity | Notes |
|---|---|---|---|
| Health Potion | Consumable | Common | Stack 10. "Restores a chunk of health. The Alchemist limits how many you can field." |
| Iron Ore | Material | Common | Auto-stored, stack 99 |
| Town Stone | TownResource | Common | Auto-stored |
| Upgrade Key | UpgradeKey | Blue | SortPriority Critical |
| Mysterious Amulet | MagicalItem | Purple | Subtype Amulet, starts Unidentified, needs identification to equip. Short description "An amulet of unknown power." Description "A faintly humming amulet. The Mage Tower can identify it." |
| Rusty Sword | Weapon | Common | Subtype Sword. "A worn blade. Barely holds an edge, but it is honest work." |

## Rarity tiers (exact names and order; any colors named)

**What the specs say:**
- `item_system_v1.md`: "Common Green Blue Purple Red / Gold". "Higher tiers introduce: stronger modifiers, unique abilities". "Affixes scale with item rarity."
- `game_design_v2.md` pillar 5 and §6: magical items are "Green/Blue/Purple/Red(Gold)". Common is not listed for magical items.

**What the code does** (`EMordecaiItemRarity` in `Weapons/MordecaiWeaponTypes.h`):
- Ordinals: Common=0, Green=1, Blue=2, Purple=3, Red=4, Gold=5.
- Comment: "Common(Gray) < Green(Uncommon) < Blue(Rare) < Purple(Epic) < Red(Legendary) < Gold(Mythic). Gold added by US-071 per item_system_v1 'Red / Gold' top tiers". So the code splits "Red / Gold" into two tiers.
- Rarity "affects affix count, stat budget, and visual treatment".
- Tags: `Mordecai.Item.Rarity.{Common,Green,Blue,Purple,Red,Gold}`.
- Player-facing rarity words are the color words "Common", "Green", "Blue", "Purple", "Red", "Gold", as in "Unidentified Purple Amulet".
- Story text uses the alias names: US-071 AC-071.6 says "Uncommon/Green… Rare/Blue… Epic/Purple… Legendary/Red… Mythic/Gold". US-079 calls the Upgrade Key "Rare" and the amulet "Epic".

**Rarity colors** (`UI/MordecaiInventoryWidget.h`, static constexpr, AC-071.6):

| Rarity | Color | Linear value |
|---|---|---|
| Common | Gray | (0.55, 0.55, 0.55) |
| Green | Green | (0.10, 0.80, 0.10) |
| Blue | Blue | (0.15, 0.40, 1.00) |
| Purple | Purple | (0.60, 0.20, 0.90) |
| Red | Red | (0.90, 0.10, 0.10) |
| Gold | Gold | (1.00, 0.84, 0.15) |

- The color is applied to the row text only. AC-079.11 asked for row backgrounds or borders; the accepted build tints the name text.
- The schema's Presentation field is `RarityColor: Purple`.

**Colors that clash across systems:**
- Rarity Red (0.9, 0.1, 0.1) is exactly the enemy-indicator low-health red (0.9, 0.1, 0.1).
- Rarity Gold (1.0, 0.84, 0.15) is close to the enemy mid-health yellow (1.0, 0.85, 0.0) and the Radiant damage color (1.0, 0.85, 0.0).
- Rarity Green (0.1, 0.8, 0.1) is close to the enemy healthy green (0.0, 0.8, 0.0), healing pops (0.2, 1.0, 0.2) and Poison damage (0.3, 0.9, 0.3).
- Rarity Purple is close to Arcane damage (0.7, 0.3, 1.0).
- Feat tiers use a different vocabulary: Common (0.8, 0.8, 0.8), Rare (0.3, 0.5, 1.0), Legendary (1.0, 0.75, 0.0) (`UI/MordecaiFeatDisplayWidget.cpp`).

## Affixes, identification (unidentified items, identify flow), item comparison data

**Affixes:**
- `item_system_v1.md` modifier types: StatModifier, AbilityModifier, ProcEffect, PassiveBonus.
  - Example: "Flameblade +15% fire damage Attacks apply Burning".
  - Example affixes: +AttackSpeed, +SpellPower, +BleedChance, +StaminaRegen.
- `item_schema_v2.md`: `Affixes: Prefixes: [flaming] Suffixes: [of_cinders]`. Rule: affixes are "built from known modifier primitives".
- Code: `FMordecaiItemInstance.AffixRolls` and `FMordecaiWeaponInstance.AffixRolls` are `TArray<FName>` placeholders. No affix data exists.
- `ue5_tech_stack_context.md` lists "Inventory schema details (stacking, rarity rolls, affix system)" as open.

**Proc triggers** (schema): OnHit, OnCrit, OnKill, OnBlock, OnPerfectBlock, OnParry, OnDodge, OnAbilityUse, OnStatusApplied, OnLowHealth, OnEnterCombat. Example `emberfang_burn_proc`: OnHit, Chance 0.25, InternalCooldown 2.0, applies `ge_burning_buildup_medium`.

**Example item, Emberfang:**
- "A named sword that kindles fire with every strike."
- MagicalItem, Sword, Purple, MainHand.
- Tags Item.Weapon, Item.Magical, Weapon.Sword, Damage.Fire.
- Grants `emberfang_flame_arc`.
- InspectText "The blade glows with a restrained inner furnace."

**Identification rules** (schema):
- Fields: UsesIdentification; DefaultState (Identified/Unidentified); IdentificationService (usually MageTower); ShowPartialInfoBeforeIdentify ("teaser text, rarity hinting, silhouette hints").
- Defaults: most common gear does not use identification; curated magical items may.
- "Identification should gate usage clarity and sometimes usage permission, not storage."
- The MagicalItem default is Unidentified with RequiresIdentificationToEquip true.
- `agent_rules_v2.md` §2:
  - "Unidentified magical items cannot be fully used until processed by the Mage Tower."
  - Identification "replaces 'bag full' as a major looting constraint".
  - "Surface unidentified magical items clearly in UI and tooltips."
- `game_design_v2.md` §9: "Unidentified magical items should be clearly signposted in inventory and town UI."

**Display rules in code** (`MordecaiItemLibrary.cpp`):

| State | Name shown | Description shown | Tags shown |
|---|---|---|---|
| Identified | DisplayName | Description | All tags |
| Unidentified, partial info on | "Unidentified <Rarity> <Subtype or type string>" | ShortDescription (or empty) | Only `Mordecai.Item.Rarity.*` and `Mordecai.Item.Type.*` |
| Unidentified, partial info off | "Unknown Item" | Empty | Only `Mordecai.Item.Rarity.*` and `Mordecai.Item.Type.*` |

`CanEquipInstance` is false when identification is required and the item is unidentified, or when the type is not equippable.

**Identification service:**
- `UMordecaiIdentificationService::IdentifyInstance(Inventory, InstanceId, ServiceName="MageTower")` returns a bool and fires `OnItemIdentified`.
- It is free and instant: "TODO(DECISION): Identification cost model unspecified; currently free."
- Batch identify and identification scrolls are out of scope (US-033).

**Identify flow as built** (US-071/US-079, accepted on `Saved/Playtests/20261029-175841-us079`):
1. Open the pause menu Inventory tab (I).
2. The unidentified row reads "? Unidentified Purple Amulet" in purple, with an "Identify" button.
3. Click Identify. The row rebuilds as "Mysterious Amulet" and the button disappears.
4. This works anywhere in the field. US-079: "Don't build town/Mage-Tower UI — out of scope."

**Still open:**
- `agent_rules_v2.md` Open Item 4: per-item, batch or timed town process; gold/resource/service cost; whether partial hints show.
- `game_design_v2.md` §12: "Exact identification UX (single-item, batch, timed, or service queue)."

**Item comparison data:**
- No spec defines a compare rule or layout.
- `ue5_tech_stack_context.md`: "Use MVVM for: … item tooltips & compare panels." and "Use Common UI for: … inventory".
- Comparable fields that exist: the weapon stats above; schema StatModifiers, RequiredLevel, GrantedAbilities, ProcRules, Durability, SellValue.
- AC-071.5 says "Row tooltip/description uses GetDescription". The row model carries Description, but the shipped C++ layout never renders it, so there is no tooltip today.

## Loot sources (drops, boxes/chests, vendors) and pickup flow

**Loot rules** (`game_design_v2.md` §2, §6):
- "Common-sense loot: Wolves drop pelts/teeth; bandits drop gold + plausible items. No grinding for random rare weapons."
- "Enemies drop believable items/resources (no 'wolves drop gold')."
- "Boss/elite wielding a magic weapon drops it 100%." The schema has `SpecialRules.BossDropGuaranteed`.
- "Most equipment is found in environment containers (weapon racks, armor stands, chests) and doubles as decor." The schema has `WorldPlaced` and `ContainerPlaced`.
- "Curated magic items: Found via bosses/dungeons/story (BG3-style gating/prereqs)."
- Session loop: "Quest/dungeon → miniboss/boss → signature item or upgrade key → return to town → identify loot / refill services / select initiative → unlock new routes → repeat."
- §9: "no loot vomit".

**Enemy loot fields** (`enemy_archetypes_v1.md`): `CommonTableId: loot_goblin_basic`, `GuaranteedDrops: []`, `SignatureDropId: null`. No loot tables or enemy drops exist in code.

**Map side** (`map_style_technical_design_v1.md`, `agentic_map_pipeline_v1.md`):
- Markup actor `PM_LootAnchor`.
- `DA_Loot_<CellId>` "loot anchors if needed".
- Stage E "Encounter and Loot Pass" adds "loot anchors, reward tags".
- Revisit hooks include "visible unreachable chest".
- Shortcut rhythm: "long forward push → challenge / pocket / gate → reward or reveal → unlock return shortcut".

**Skills that touch loot** (`skill_sheet_v1.1.md`):
- Lockpicking: "Pick simple locks with a {x}*{rank}% wider success window and {y}*{rank}% faster progress". Rank 20: "Successfully opening a lock briefly reveals nearby hidden containers."
- Perception: reveals traps and secrets; "Spotting an enemy now briefly highlights weak points"; "Hidden interactables now shimmer briefly when nearby".
- Cartography: "automatically noting merchants, shrines, and hazards".

**Vendors:**
- `village_system_v1.md` speaks of "all building vendors" (Alchemist, Blacksmith, Clinic, Mage Tower, Keep).
- `game_design_v2.md` lists the buildings "blacksmith, alchemist, mage tower, walls/keep, lumber yard, mine, market, clinic".
- CHA covers "town attraction, dialogue leverage, vendor outcomes" (`attribute_scaling_v1.md`).
- No vendor UI or buy/sell spec exists.

**Loot boxes:** no spec or story mentions loot boxes.

**Pickup flow as built** (decision "Press-to-Pickup", `design_decisions_log.md` 2026-07-10, which supersedes the walk-over pickup):
1. `AMordecaiItemPickup` (`Items/MordecaiItemPickup.h/.cpp`):
   - Sphere trigger, default radius 120 cm.
   - Optional display mesh. The test pickups use engine shapes at 0.35 scale with flat colors: Health Potion red cylinder (0.9, 0.1, 0.1), Iron Ore grey cube, Town Stone tan cube, Upgrade Key blue cone (0.15, 0.4, 1.0), Amulet purple sphere (0.6, 0.2, 0.9), Rusty Sword brown cylinder.
   - In-world name label: a `UTextRenderComponent`, white, WorldSize 22, 70 cm above, centered, billboarded to the camera every frame. It shows the identification-aware name, so the amulet reads "Unidentified Purple Amulet". The label is white for every rarity.
2. Overlap registers the pickup with `UMordecaiPickupInteractionComponent`. The nearest in-range pickup is "focused", re-evaluated on a 0.15 s tick while pickups are in range.
3. The HUD shows the prompt "[F] Pick up <item name>" (white, centered, collapsed when nothing is focused) and a persistent "[I] Inventory" hint (0.8 grey, 0.9 alpha), at bottom-center.
   - Keyboard glyphs only. "Device-aware key glyphs … deferred" (Gamepad Interact = X).
4. Interact (F / Gamepad X) runs `ExecutePickup` on the server, then `PickupItem`:
   - Auto-stored items go to the Resource Ledger.
   - Other items go to `AddItem`, which merges stacks up to MaxStackSize and puts overflow in a new stack.
   - The actor is destroyed.
5. There is no acquisition toast or feedback beyond the actor disappearing. Auto-stored materials enter the ledger silently.

The Interact binding note reads "Talk, loot, activate, climb prompts."

## Inventory rules (unlimited carry, town gating, sorting/filters)

**Locked rules** (`agent_rules_v2.md` §2):
- Inventory is "effectively unlimited" and "not intended to be a difficulty source"; players "pick up and keep everything".
- Loot friction comes from town-facing systems:
  1. Magical item identification at the Mage Tower.
  2. Healing potion stock limits governed by the Alchemist.
  3. Delivering materials, town resources and upgrade keys.
- DO: "flat, player-friendly carry model with strong sorting/filtering"; "Preserve strong organization, filtering, and search".
- DO NOT: "bag slots, container tetris, or specialized bags".
- Defaults: materials and town resources auto-store immediately; magical items are individually tracked. "Sorting should prioritize: quest items, upgrade keys, magical items, then common gear."
- Consumables: "healing scarcity should primarily come from village production / stock, not inventory capacity. The Alchemist determines how many health potions the player can reliably take into the field." The potion field-cap model is Open Item 5.
- `village_system_v1.md`: "The player should not return to town because their bags are full."

**Storage** (schema and code):
- CarryModel: Unlimited, SpecialRule.
- StorageDomain: PlayerInventory, TownStorage, QuestState.
- SortPriority: Low, Normal, High, Signature, Critical.
- BindType: None, BindOnPickup, BindOnEquip, QuestBound. Also DropOnDeath and Tradable.

Type defaults:

| Type | Storage | SortPriority | Auto-store | Identification |
|---|---|---|---|---|
| Material | TownStorage | Normal | yes | no |
| TownResource | TownStorage | High | yes | no |
| UpgradeKey | PlayerInventory | Critical | no | no |
| MagicalItem | PlayerInventory | Signature | no | Unidentified, MageTower |

**Sort order in code** (`CompareSortPriority`):
1. SortPriority, highest first: Critical → Signature → High → Normal → Low.
2. Ties broken by type rank: Weapon 0, Armor 1, Trinket 2, MagicalItem 3, UpgradeKey 4, QuestItem 5, Consumable 6, Material/TownResource 7, CurrencyProxy 8.
3. Then by ItemId.

This tie-break puts gear before quest items, which is the reverse of agent_rules' "quest items, upgrade keys, magical items, then common gear". Only the SortPriority defaults bring keys and magical items to the top.

**Stacks:** quantity shows only when `IsStackable()` and Quantity > 1, as "  x<N>".

**Inventory tab** (`UI/MordecaiInventoryWidget.*`, US-071/US-079; implemented, accepted at Tier 4):
- It is the pause menu's "Inventory" tab. Header "Paused"; tabs Character, Skills, Feats, Inventory, Settings.
- Input:
  - I opens the menu directly on Inventory; Esc / Gamepad Start toggle the menu on the last-active tab.
  - I closes on key-up; Escape / Gamepad B close it.
  - LB/RB switch tabs (decided 2026-10-30).
- Filters (`EMordecaiInventoryFilter`), default All. Labels are exactly: All, Weapons, Armor, Trinkets, Consumables, Materials (= Material + TownResource), Quest, Magical (= MagicalItem + UpgradeKey).
- Filter buttons: active background (0.25, 0.35, 0.55) with a white label; inactive background (0.12, 0.12, 0.15) with a 0.7-grey label.
- Rows show the rarity-colored name, "  x<N>", "  [equipped]", a "? " prefix when unidentified, and an "Identify" button. The description and icon are in the row model but not drawn.
- Right column, "Stored Resources": the ledger, sorted alphabetically, one row "<Name>  x<Count>" each. In the playtest it read "Iron Ore  x1" and "Town Stone  x1".
- Placeholder text (0.6 grey):

| State | List text | Ledger text |
|---|---|---|
| Not bound | "--" | "--" |
| Inventory and ledger both empty | "Inventory is empty." | |
| Filter hides every item | "No items match this filter." | |
| Ledger empty | | "No resources stored." |

- Known cosmetic bug: "The inventory filter bar overlaps the 'Stored Resources' header."
- Out of scope today: drop, equip and use. TODO(DECISION) N22 (`stories/PLANNER_LOG.md`): "picked-up items have no Use or Equip action (Health Potion unusable, Rusty Sword cannot be equipped…)".
- The ledger is per-character: "TODO(DECISION): Migrate to PlayerState when town persistence ships." `GetAllResources()` is "for UI / town bank queries".

## Currencies

No currency system is specified. Every mention:
- "bandits drop gold" (`game_design_v2.md`).
- The identification cost might be "gold/time/reagent" (US-033, TODO(DECISION)).
- Schema `Economy: SellValue 250, BuyValue 0, SalvageTableId`.
- Item type `CurrencyProxy`: display string "Currency", sort rank 8, no gameplay tag.
- PlayFab holds "Inventory ownership and economy data (virtual items/currency if needed)" and "Inventory ownership, currencies, cosmetics" (`ue5_tech_stack_context.md`).
- A "market" building; CHA affects "vendor outcomes".

Town progression is paid in ledger materials, refined materials and upgrade keys (village §1). No wallet, HUD readout or price display is specified.

## Enemy archetypes (every archetype: role, tier, telegraphs, special states the player must read)

**Enemy schema** (`Specs/enemy_archetypes_v1.md`):
- Required fields: EnemyId, DisplayName, Archetype, Tier.
- Recommended tiers: `Basic`, `Elite`, `Champion`, `Boss`, `Summon`, `Miniboss`.
- `enemy_ai_framework_v1.md` lists the archetypes "Basic Elite Boss".
- Classification: Faction, Family, BiomeTags, DamageTags.
- CoreStats (goblin example): HealthScalar 1.0, DamageScalar 1.0, MoveSpeed 1.15, Posture 0.8, Stability 0.7, AggroRange 9.0, LeashRange 18.0.
- Resistances (example): Fire 0.0, Frost -0.15, Poison 0.25, Lightning 0.0.
- StatusRules: ImmuneTags, ResistantTags (Status.Fear), VulnerableTags (Status.Rooted).
- Behavior: ControllerType, BehaviorTree, Blackboard, PreferredRange (Close/Far).
- Aggression styles: `DirectRush`, `FlankAndPressure`, `HoldLine`, `KiteAndChip`, `AreaDenial`, `Ambush`, `SupportFromRear`, `SiegePressure`.
- Telegraph profile: `WindupClarity` (High), `FXClarity` (Medium), `AudioCues` (true), `GroundMarkers` (false). Rule: "Telegraphs must remain readable from the fixed diorama camera."
- EncounterRole: Role, ThreatPriority (Medium/High), IdealGroupSize ("2-4"), SynergyTags (`EnemyRole.Frontliner`, `EnemyRole.Support`).
- Roles: `Frontliner`, `Harasser`, `Bruiser`, `Artillery`, `Controller`, `Support`, `Summoner`, `Tank`, `Assassin`, `AreaDenial`, `BossPhaseDriver`.
- SpawnRules: SpawnWeight 1.0, MinimumRegionTier 1, MaxSimultaneousRecommended 6.
- Philosophy: challenge through "readable patterns, positioning, cadence, area control, synergy, status pressure, encounter composition". Not through "oversized health pools, cheap unreadable one-shots, excessive permanent crowd control, random behavior". Each enemy needs "a readable role, a clear threat profile, a weakness or counterplay angle".
- `ability_schema_v1.md`: Presentation `TelegraphType: CastGlow` or `WindupPose`. AIHints include `AvoidIfTargetHasTag: Immune.Fire`.

**Archetype library:**

| Archetype | Purpose | Traits | Counterplay |
|---|---|---|---|
| Frontliner | absorb attention, define battle lines, force spacing | medium/high health, stable posture, moderate damage, readable melee | posture breaks, flanking, mobility |
| Harasser / Skirmisher | pressure movement, interrupt comfort zones, punish tunnel vision | high mobility, low/medium health, leap/dash/disengage | roots, spacing discipline, burst after commitment |
| Bruiser | high-threat melee presence, dangerous but committed attacks | heavy swings, posture damage, slower punishing cadence | dodge/parry timing, baiting commitments, windows after misses |
| Artillery | force movement from afar, punish stationary players | ranged attacks, **area markers**, low defenses | close distance, line-of-sight abuse, interruption |
| Controller | shape battlefield; roots, slows, silence, fear, zone denial | lower direct damage, high annoyance via status | target priority, cleanse, interrupt, resistance builds |
| Support | sustain/empower allies | buffs, heals, **shields**, summons | target priority, disruption, burst |
| Summoner | pressure over time, alter composition | spawns adds, low direct threat unless unchecked | pressure summoner, AoE add clear, anti-summon |
| Tank / Sentinel | anchor encounter, guard lanes/objectives | high health, high stability, taunt-like/protective, often paired with artillery/support | ignore and reposition, exploit weak supports, **armor-break/posture tools** |
| Assassin | burst vs backline/distracted | **stealth/opening burst**, mobility, fragile if exposed | awareness, anti-stealth, punish after reveal |
| Area Denial | make safe zones unsafe | hazards, puddles, fire walls, lightning zones, corrupted terrain | mobility, timing, status resistance, control space early |
| Boss Phase Driver | boss ecosystems; changes cadence/rules | "scripted but readable phase transitions", summons hazards/support, alters arena pressures | learn phase language, preserve resources, target priority |

**Example definitions:**
- **Goblin Skirmisher** (`goblin_skirmisher`):
  - Skirmisher, Basic; Faction Goblin, Family Humanoid; Biome.Forest and Biome.Caves.
  - FlankAndPressure, Close.
  - Abilities: goblin_quick_slash, goblin_leap_strike, goblin_evade.
  - Role Harasser, ThreatPriority Medium.
- **Skeletal Archer:**
  - Artillery, Basic; Undead/Skeleton.
  - Stats: HealthScalar 0.8, DamageScalar 1.1, MoveSpeed 0.95, Posture 0.7, Stability 0.6, AggroRange 12.0, LeashRange 22.0.
  - DefaultRangedAI, Far, KiteAndChip.
  - Abilities: skeleton_arrow_shot, skeleton_rapid_volley, skeleton_retreat_step.
  - ThreatPriority High, group size 1-3.
- **Corrupted Knight:**
  - Bruiser, **Elite**; Corrupted/Humanoid.
  - Stats: HealthScalar 2.4, DamageScalar 1.8, MoveSpeed 0.9, Posture 1.6, Stability 1.5, AggroRange 10.0, LeashRange 20.0.
  - EliteMeleeAI, DirectRush.
  - Abilities: corrupted_knight_cleave, corrupted_knight_guard_break, corrupted_knight_dark_wave.
  - ThreatPriority High, group size 1.

Validation rules include "telegraph clarity is appropriate to threat level", "ranged enemies have real counterplay", "controller enemies do not over-stack frustration" and "bosses are readable despite complexity".

**World roster** (`world_design_v1.md`; game_design_v2 says "4–6 biomes … roaming elites"):

| Biome (theme) | Enemies | Boss and its traits | Unlock |
|---|---|---|---|
| Verdant Wilds (Denial) | "Machines overtaken by nature" | Thorn Guardian: regenerates, creates barriers, uses overgrowth to trap the player | Explosives |
| Searing Expanse (Control) | Dust Automatons, Sentinel Frames | Iron Warden: enforces strict patterns, punishes deviation, grid-based attacks | Magnetic Device |
| Mire of Echoes (Avoidance) | Bog Wraiths, Rot Beasts, Drowned Automatons | Drowned King: hidden attacks, phases in water, delayed damage patterns | Invisibility Potion |
| Crystal Caverns (Preservation) | Crystal Phantoms, Echo clones | Crystal Seer: repeats attacks, freezes zones, creates delayed echoes | Phase Talisman (HP drain) |
| Storm Peaks (Obsession) | Storm Constructs, Aerial Wraiths | Tempest Ascendant: aerial combat, constant movement, anti-ground mechanics | Glider Cloak |
| Withered Kingdom (Final Truth) | Undead knights, Royal wraiths | none listed | |

- Final Boss: **The Lich King**. "Uses player abilities, Controls time and space, Resets fight states".
- The robot race "Continuants": "Some broken and hostile, Some sentient".

**Boss phases** (`enemy_ai_framework_v1.md`): "Phase 1: Basic attacks, Phase 2: AoE attacks unlocked, Phase 3: Enrage mechanics." Enrage is mentioned only here.

**AI as built** (`Enemy/MordecaiEnemyAIController.h`, US-051):
- A tick-driven state machine: `Idle`, `Approach`, `Attack`, `Recover`, `Staggered`, `Leash`, `Dead`.
- Defaults: AggroRange 900 cm, AttackRange 200 cm (only a fallback; the real range is the attack profile's reach, 150 cm for BasicSlash, issue #47), LeashRange 1800 cm, AttackCooldownMs 1500.
- It targets decoys (`Team_Ally`, the Illusion spell) before the player.
- The spec loop is "Idle → Detect Player → Engage → Attack Patterns → Recover → Repeat"; enemies evaluate distance, cooldowns and posture state.

**Enemies in the build:**
- `BP_MordecaiEnemy_Frontliner`:
  - Dark red cylinder, `MI_Enemy_Frontliner` (0.55, 0.04, 0.04), scale 0.8 x 0.8 x 1.8.
  - MaxHealth 200, MaxPosture 100, MoveSpeed 400.
  - Profiles BasicSlash and FireSlash.
- `BP_MordecaiEnemy_TestDummy`: blue-grey (~0.25, 0.35, 0.75), AggroRange 0, MoveSpeed 0, 200 HP.
- `BP_MordecaiEnemy_TestAttacker`: orange (~1.0, 0.45, 0.05), MoveSpeed 0, BasicSlash only.
- `DA_EnemyAttack_BasicSlash`:
  - MeleeSweep, WindupMs 500 ("readable telegraph"), ActiveMs 200, RecoveryMs 400.
  - ArcAngle 90°, Radius 150 cm, BasePower 15, PostureDamageScalar 0.3.
  - Sweeps are `JumpAvoidable=true` ("low sweep").
- FireSlash applies Burning at 40%.
- PLANNER_LOG TODO(DECISION): "Enemy attack feel — 500ms … too generous? Too punishing?"

**Special states the player must read:**
- **Posture:**
  - "Enemies have posture meters." Posture damage comes from heavy attacks, perfect parries and certain abilities. "When posture breaks: enemy enters **critical window** — player can perform riposte" (`combat_system_v1.md`).
  - Code (`Combat/MordecaiPostureSystem.h`): stagger PostureBrokenDurationMs 1000, StaggeredDamageMultiplier 1.5x.
  - Knock Down: KnockDownDamageThreshold 50 dealt during the stagger, KnockDownDurationMs 2000, 2.0x. "Knock Down replaces Stagger (escalation, not stacked)" (`design_decisions_log.md`).
  - Posture regen: PostureRegenRate 10 after PostureRegenDelayMs 3000. Blocked hits deal 0.5x posture.
  - Tags: `Mordecai.State.PostureBroken`, `Mordecai.State.KnockedDown`, `Mordecai.State.Parried`, `Mordecai.State.Riposte` (N33: never removed), `Mordecai.State.GuardBroken`.
  - The AI Staggered state "pauses all actions".
- **Dead:** `Mordecai.State.Dead`. The body is hidden with its collision. "TODO(DECISION) (Jeff): corpse visuals (death animation, ragdoll, fade-out). Until then a dead enemy shows only its empty bar."
- **Airborne:** `Mordecai.State.Airborne` while Falling, for enemies too.
- **Tier:** Elite, Champion, Miniboss, Boss and Summon have no visual or UI spec.
- **Armored:** no enemy state. It is referenced only by Maces "Crack Bones (bonus vs armored)", Crossbows "Armor Bolt (bonus vs plate)", and Tank counterplay "armor-break/posture tools".
- **Shielded / guarding:** Support "shields". Maces "Guard Breaker (heavy vs blocking foes)". `corrupted_knight_guard_break`. No shielded state.
- **Enraged:** boss Phase 3 only.
- **Stealthed:** Assassin.
- **Statuses on enemies:** the 16 launch statuses (Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched, Focused). Each has a "Visual/Audio Read", for example Exposed: "Glowing target sigil hovering in-world"; its sources include "boss tells".
- **Resistances, immunities and threat priority** exist as data with no UI.

## Enemy-facing UI (health bars, indicators, off-screen markers, aggro, telegraph/danger zones, boss bars)

**Implemented:**
1. **Enemy health bar**: `WBP_EnemyHealthBar` / `UMordecaiEnemyHealthBarWidget` (US-052/US-054).
   - A `UWidgetComponent` in Screen space, draw size 150 x 20, 120 cm above the enemy.
   - A Health/MaxHealth progress bar. The C++ fallback fill is pure red; there is no number and no name.
   - This is the "red health bar" in every packaged screenshot. It stays, empty, after death.
   - AC-054.9: "showing their health percentage".
2. **Enemy indicator**: `UMordecaiEnemyIndicatorComponent` / `UMordecaiEnemyIndicatorWidget` (US-064).
   - Uses Lyra's IndicatorSystem. Registers on damage or aggro, unregisters on death.
   - IndicatorHeightOffset 120 cm, IndicatorVisibilityRange 2000 cm (checked every 0.25 s), HAlign Center / VAlign Bottom.
   - Shows a name (`EnemyDisplayName`, falling back to the actor name), a health bar and a status icon row.
   - Health bar colors: Green (0.0, 0.8, 0.0) above 50%, Yellow (1.0, 0.85, 0.0) from 25% to 50%, Red (0.9, 0.1, 0.1) below 25%.
   - Status icons: 16x16, one per tag in `StatusIconMap`, with a fallback placeholder.
   - **Not on screen in the package (N17).** Nothing calls `NotifyAggroActivated`, and there is no indicator manager or `UIndicatorLayer`. Fix planned in backlog US-092.
   - TODO(DECISION): with indicators working, each enemy would show two health readouts. Should the old bar be removed?
3. **Floating damage numbers**: `UMordecaiDamagePopComponent` (US-065).
   - Lifespan 1.5 s, rise 100 cm/s, scatter 30 cm, at most 20 at once.
   - Crits at 1.5x scale; healing shows "+" in (0.2, 1.0, 0.2); default color grey (0.7, 0.7, 0.7).
   - Damage colors: Physical white, Fire orange (1, 0.5, 0), Frost cyan (0.3, 0.8, 1), Lightning yellow (1, 1, 0.3), Poison green (0.3, 0.9, 0.3), Arcane purple (0.7, 0.3, 1), Radiant gold (1, 0.85, 0), Shadow dark red (0.6, 0.1, 0.1), Corrosive grey.
4. **Kill counter**: "Kills: 0", top-right. N1: it never increments; the fix is in US-086, in progress.

**Not implemented and not specified** (needs the design system):
- **Enemy posture meter and the Posture Broken / Knocked Down / critical-window / riposte cue.** The spec says meters exist. `new_spells_proposal.md` refers to a "boss posture bar".
- **Boss bar** (name, health, phases 1/2/3, enrage).
- **Tier badges** for Elite, Champion, Miniboss and Summon.
- **Aggro/alert cue.** AI enters Approach at 900 cm; Sneak "reduces detection".
- **Telegraphs:**
  - The data fields above exist (`WindupClarity`, `FXClarity`, `AudioCues`, `GroundMarkers`, `TelegraphType: CastGlow/WindupPose`) but nothing implements them.
  - Artillery "area markers"; Area Denial hazards; the low-sweep `JumpAvoidable` read.
  - `map_style_technical_design_v1.md`: "Any encounter-critical information should be understandable without camera manipulation"; "place enemies where the player can read them early".
- **Off-screen markers:** not specified anywhere, and the indicator is not clamped to the screen edge.
- **Status philosophy limits icons:**
  - game_design_v2 pillar 8: "Clean UI … Status is primarily VFX-driven, not icon soup".
  - §9: "Status effects communicated primarily via VFX + animation; minimal critical UI only".
  - The indicator's status icon row cuts against this.

**Camera and readability:**
- The camera follows the player and never rotates. The diorama camera has yaw 0 (looking along +X), pitched -50° (issue #38 analysis).
- The packaged screenshots and bridge coordinates are 1280x720.

## Terminology

| Term | Meaning |
|---|---|
| Unlimited carry | Flat inventory with no capacity |
| Town gating | Loot friction comes from town services |
| Auto-store / AutoStoreOnPickup | Materials and town resources go straight to the ledger |
| Resource Ledger / "Stored Resources" | The auto-stored resource list |
| Mage Tower | Identification |
| Alchemist | Potion stock |
| Identified / Unidentified / "Unknown Item" | Identification states and the no-hint name |
| partial info | The "Unidentified <Rarity> <Type>" name |
| Signature (sort priority) / signature item | Sorts near the top; a boss's notable drop |
| Critical (sort priority) | Highest sort bucket |
| Upgrade Key (formerly "Relic") | Unique town-upgrade resource |
| CurrencyProxy | Item type shown as "Currency" |
| Press-to-Pickup / Interact (F / X) | Pickups need a key press |
| focused pickup | Nearest pickup in range |
| common-sense loot | Believable drops (no "wolves drop gold") |
| curated magic items | Named, gated magical items |
| environment containers | Weapon racks, armor stands, chests |
| loot anchor (`PM_LootAnchor`) | Map marker for loot placement |
| no loot vomit | Design rule against loot clutter |

Enemy terms: tiers, roles, archetypes, aggression styles, AI states, AggroRange / LeashRange / AttackRange, Posture / Posture Broken / Stagger / Knock Down / critical window / riposte, Telegraph profile, CastGlow / WindupPose, JumpAvoidable (low sweep), ThreatPriority, Frontliner / TestDummy / TestAttacker, and the boss and biome names above.


## UI INVENTORY
- [HUD|implemented] **Pickup prompt**: Tell the player which ground item is focused and which key picks it up | data: "[F] Pick up <item name>"; name is identification-aware (e.g. "Unidentified Purple Amulet", "Unknown Item"); white, centered, bottom-center | states: hidden (nothing focused, collapsed); visible with identified name; visible with partial/unknown name; keyboard glyph only (gamepad X glyph deferred) (Source/LyraGame/Mordecai/UI/MordecaiPickupPromptWidget.h/.cpp; Specs/design_decisions_log.md 2026-07-10; US-079)
- [HUD|implemented] **Inventory assist hint**: Persistent reminder of the inventory key | data: "[I] Inventory" in (0.8,0.8,0.8,0.9) under the pickup prompt | states: always visible during play (MordecaiPickupPromptWidget.cpp:27,107; design_decisions_log.md 2026-07-10)
- [HUD|implemented] **Equipped weapon readout**: Show the current weapon after cycling | data: Weapon DisplayName + EMordecaiWeaponType; "Unarmed" when none | states: unarmed; weapon equipped; changes on Tab/BackSlash cycle (spec'd Weapon Set Swap Set 1 <-> Set 2 deferred) (Source/LyraGame/Mordecai/UI/MordecaiEquippedWeaponWidget.h; US-077; Specs/control_bindings_v1.1.md)
- [HUD|implied] **Item quickslots A-D**: Use consumables mid-combat | data: 4 slots: Use Item A (D-pad Down / 1), B (D-pad Left / 2), C (D-pad Up / 3), D (D-pad Right / 4); item icon + stack count implied | states: empty; filled; count 0/depleted; on use (Specs/control_bindings_v1.1.md rows 26-29 ("Quickslot item use"); key 1 currently bound to Spell4 in MordecaiHeroComponent.cpp:216)
- [HUD|implied] **Selected throwable**: Show which throwable G / Y-tap will throw | data: throwable type (knife/bomb/vial) and count | states: none equipped; selected; depleted (Specs/control_bindings_v1.1.md (Throw (equipped throwable)))
- [HUD|implied] **Healing potion stock counter**: Show how many Alchemist-provisioned potions remain in the field | data: potions carried vs field cap (cap model undecided) | states: full; low; empty (Specs/agent_rules_v2.md §2 Consumable Rule + Open Item 5; Specs/village_system_v1.md §4.1; Health Potion test item (stack 10))
- [HUD|implemented] **Kill counter**: Count enemies killed this run | data: "Kills: N" top-right | states: 0 at start; increments on kill (currently never increments, N1); resets on death (UMordecaiKillCounterWidget; US-063; stories/in-progress/US-086; Specs/agent_playtest_plan_v1.md N1)
- [HUD|implied] **Boss health bar**: Track boss health and phase in boss fights | data: boss name (Thorn Guardian, Iron Warden, Drowned King, Crystal Seer, Tempest Ascendant, The Lich King), health, phase 1/2/3, possibly a boss posture bar | states: phase 1 basic; phase 2 AoE unlocked; phase 3 enrage; posture broken; defeated (Specs/enemy_ai_framework_v1.md Boss Phases; Specs/world_design_v1.md; Specs/new_spells_proposal.md:182 ("boss posture bar"))
- [HUD|implied] **Off-screen threat markers**: Warn of aggroed or ranged enemies outside the fixed camera frame | data: direction to enemy / incoming projectile | states: not specified (Not specified; enemy indicator not clamped to screen (MordecaiEnemyIndicatorComponent.cpp:125-128); Artillery 'force movement from afar' (enemy_archetypes_v1.md))
- [World-space|implemented] **Ground pickup name label**: Tell pickups apart at a glance | data: identification-aware item name; TextRender, white, WorldSize 22, +70 cm, centered, billboarded to camera each frame | states: identified name; "Unidentified <Rarity> <Type>"; "Unknown Item"; same white for all rarities (Source/LyraGame/Mordecai/Items/MordecaiItemPickup.h/.cpp; issues #30/#38; US-079 AC-079.10)
- [World-space|implied] **Ground loot body and rarity/identification cue**: Show rarity and unidentified status of loot before pickup | data: placeholder engine shapes at 0.35 scale with flat colors (potion red, ore grey, stone tan, key blue cone, amulet purple sphere, sword brown); no rarity glow or beam | states: by rarity Common..Gold; unidentified (Scripts/place_test_pickups.py; Specs/item_schema_v2.md ShowPartialInfoBeforeIdentify ('rarity hinting, silhouette hints'))
- [World-space|implied] **Focused-pickup highlight**: Show which of several overlapping pickups F will take | data: nearest pickup within 120 cm trigger, re-evaluated every 0.15 s | states: unfocused; focused; swapping between two (MordecaiPickupInteractionComponent.h (only the HUD prompt text shows focus today))
- [World-space|implied] **Container / loot source interact**: Open chests, weapon racks, armor stands; Interact on loot | data: container type, locked/unlocked, prompt key F/X | states: closed; locked (Lockpicking); opened/empty; hidden (revealed by Perception/Lockpicking R20) (Specs/game_design_v2.md §6; map_style_technical_design_v1.md §9.1, PM_LootAnchor; skill_sheet_v1.1.md Lockpicking/Perception)
- [World-space|implemented] **Enemy health bar (WBP_EnemyHealthBar)**: Show each enemy's remaining health | data: Health/MaxHealth progress bar, red fill, Screen-space widget component 150x20 at +120 cm; no number, no name | states: full; damaged; empty (stays visible on dead enemy) (Source/LyraGame/Mordecai/UI/MordecaiEnemyHealthBarWidget.*; MordecaiEnemyCharacter.cpp:532-534; US-052 AC-052.10, US-054 AC-054.9)
- [World-space|implemented] **Enemy indicator (name + color-coded health)**: Name, health tier and statuses above aggroed or damaged enemies | data: EnemyDisplayName (falls back to actor name); health bar Green >50% (0,0.8,0), Yellow 25-50% (1,0.85,0), Red <25% (0.9,0.1,0.1); +120 cm; visible within 2000 cm | states: unregistered; registered on damage/aggro; hidden beyond range; removed on death. Not on screen in package (N17) (UI/MordecaiEnemyIndicatorWidget.*, MordecaiEnemyIndicatorComponent.*; US-064; fix planned in stories/backlog/US-092)
- [World-space|implemented] **Enemy status icon row**: Show active debuffs on an enemy | data: 16x16 icons for Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched, Focused (StatusIconMap); fallback placeholder | states: none; one or more active; add/remove on tag change (MordecaiEnemyIndicatorWidget.cpp:148-165, 279-311; tension with game_design_v2 pillar 8 'not icon soup')
- [World-space|implemented] **Floating damage numbers**: Hit feedback on enemies and player | data: amount colored by damage type (Physical white, Fire orange, Frost cyan, Lightning yellow, Poison green, Arcane purple, Radiant gold, Shadow dark red, Corrosive grey); crit 1.5x scale; healing '+' green; 1.5 s life, 100 cm/s rise, 30 cm scatter, max 20 | states: normal; critical; healing; negated hits show none (UI/MordecaiDamagePopComponent.h/.cpp; US-065; design_decisions_log 2026-10-30)
- [World-space|implied] **Enemy posture meter and break state**: Let the player see posture building and the critical/riposte window | data: posture vs MaxPosture (100); Posture Broken = stagger 1000 ms at 1.5x damage; Knocked Down after 50 damage in stagger = 2000 ms at 2.0x; regen 10/s after 3000 ms | states: building; regenerating; Posture Broken (stagger); Knocked Down; recovered (Specs/combat_system_v1.md ('Enemies have posture meters'); Combat/MordecaiPostureSystem.h; design_decisions_log.md 2026-03-19)
- [World-space|implied] **Riposte / critical-window prompt**: Signal the punish window after a posture break or parry | data: window open; State.Riposte / State.Parried | states: window open; window closed (Specs/combat_system_v1.md; game_design_v2.md §4; MordecaiGameplayTags (State_Riposte, State_Parried; N33 tags never removed))
- [World-space|implied] **Enemy tier / rank badge**: Mark Elite, Champion, Miniboss, Boss and Summon enemies | data: tier name or frame treatment; threat priority | states: Basic; Elite; Champion; Miniboss; Boss; Summon (Specs/enemy_archetypes_v1.md §2 recommended tiers; game_design_v2.md 'roaming elites')
- [World-space|specified] **Enemy attack telegraphs**: Make windups and attack areas readable from the fixed camera | data: windup pose / cast glow; optional ground markers; jump-avoidable low sweep; BasicSlash windup 500 ms, 90° arc, 150 cm | states: windup; active; recovery; ground-marker on/off per enemy (Specs/enemy_archetypes_v1.md Telegraph Profile (WindupClarity, FXClarity, AudioCues, GroundMarkers); ability_schema_v1.md TelegraphType CastGlow/WindupPose; US-054 AC-054.3)
- [World-space|implied] **Danger / hazard zones**: Show unsafe floor (Artillery area markers, Area Denial hazards) | data: puddles, fire walls, lightning zones, corrupted terrain, AoE areas | states: warning/arming; active; expiring (Specs/enemy_archetypes_v1.md Artillery ('area markers'), Area Denial; map_style_technical_design_v1.md PM_Hazard)
- [World-space|specified] **Exposed target sigil**: Show that a target's next hit will be punished | data: "Glowing target sigil hovering in-world" | states: active until 1 hit or short timer (Specs/status_effects_launch_set_v1.md (Exposed; sources include 'boss tells'))
- [World-space|implied] **Aggro / alert indicator**: Show when an enemy has noticed the player (Idle -> Approach) or lost them (Leash) | data: AI state: Idle, Approach, Attack, Recover, Staggered, Leash, Dead; AggroRange 900 cm, LeashRange 1800 cm | states: unaware; alerted; engaged; leashing home (Enemy/MordecaiEnemyAITypes.h; MordecaiEnemyAIController.h; control_bindings Sneak 'reduces detection'; Assassin counterplay 'awareness')
- [World-space|implied] **Dead enemy / corpse and drop**: Show a kill and any dropped loot | data: body hidden on death, empty health bar remains; no loot drop exists yet | states: alive; dead; loot dropped (future) (US-085 TODO(DECISION) corpse visuals; game_design_v2 §6 boss/elite drop rule)
- [In-game menu|implemented] **Inventory tab (pause menu)**: Flat list of carried items plus stored resources | data: Pause menu "Paused"; tabs Character, Skills, Feats, Inventory, Settings; left: filter bar + item list; right: "Stored Resources" | states: opened with I (directly on Inventory) or Esc/Start (last tab); closes with Esc/B or I key-up; LB/RB switch tabs (UI/MordecaiInventoryWidget.*; US-071, US-079, US-094, US-095; design_decisions_log 2026-07-10, 2026-10-30)
- [In-game menu|implemented] **Inventory category filter bar**: Filter the flat list by category | data: All, Weapons, Armor, Trinkets, Consumables, Materials (Material+TownResource), Quest, Magical (MagicalItem+UpgradeKey); active bg (0.25,0.35,0.55) white label, inactive (0.12,0.12,0.15) 0.7-grey label | states: one active filter (default All); known bug: overlaps 'Stored Resources' header (MordecaiInventoryWidget.h EMordecaiInventoryFilter; .cpp RefreshVisuals; agent_playtest_plan_v1.md)
- [In-game menu|implemented] **Inventory item row**: One carried item | data: name colored by rarity (Common grey, Green, Blue, Purple, Red, Gold); '  xN' when stackable and N>1; '  [equipped]'; '? ' prefix when unidentified; model also has Description and Icon (not drawn) | states: normal; stacked; equipped; unidentified; identified (MordecaiInventoryWidget.h FMordecaiInventoryRowModel, RarityColor* constants; .cpp:489-535)
- [In-game menu|implemented] **Unidentified badge**: Signpost unidentified magical items | data: '? ' text prefix plus partial name 'Unidentified <Rarity> <Subtype|Type>' or 'Unknown Item' | states: unidentified; removed after identification (MordecaiInventoryWidget.cpp:504-507; agent_rules_v2 §2 'Surface unidentified magical items clearly in UI and tooltips')
- [In-game menu|implemented] **Identify button (field slice)**: Identify an unidentified item (stand-in for the Mage Tower) | data: 'Identify' button on unidentified rows | states: shown only on unidentified rows; click -> row renames (e.g. 'Mysterious Amulet'), button disappears; free and instant (MordecaiInventoryWidget::TryIdentify; UMordecaiIdentificationService (TODO(DECISION) cost); US-079 Tier 4 amulet_identified)
- [In-game menu|implemented] **Stored Resources (ledger) panel**: Show auto-stored materials and town resources | data: 'Stored Resources' header; rows '<Name>  x<Count>' sorted alphabetically | states: unbound '--'; empty 'No resources stored.'; populated (MordecaiInventoryWidget.cpp:432-451, 544-562; Items/MordecaiResourceLedger.h)
- [In-game menu|implemented] **Inventory empty / placeholder states**: Explain an empty or filtered-out list | data: '--' (unbound), 'Inventory is empty.', 'No items match this filter.' in 0.6 grey | states: unbound; empty; filtered to zero (MordecaiInventoryWidget.cpp GetListPlaceholderText; US-071 AC-071.15)
- [In-game menu|specified] **Item tooltip / detail panel**: Full item info: description, stats, procs, requirements | data: Description or ShortDescription (unidentified), visible tags (unidentified: rarity and type only), StatModifiers, GrantedAbilities, ProcRules, RequiredLevel, BindType, SellValue, InspectText, LoreText | states: identified; unidentified partial; unknown; equip-blocked (US-071 AC-071.5 ('Row tooltip/description'); ue5_tech_stack_context.md ('item tooltips & compare panels'); item_schema_v2.md §3)
- [In-game menu|specified] **Item compare panel**: Compare a candidate item against the equipped one | data: weapon BaseDamage, AttackSpeedMultiplier, Range, PostureDamageBonus, StatModifiers; no compare rule specified | states: no equipped item; better/worse per stat; unidentified (no compare?) (Specs/ue5_tech_stack_context.md:57; Weapons/MordecaiWeaponDataAsset.h)
- [In-game menu|specified] **Inventory search**: Keep unlimited carry readable | data: text search over items | states: empty query; results; no results (Specs/agent_rules_v2.md §2 ('organization, filtering, and search'))
- [In-game menu|implied] **Sort control**: Change list ordering beyond the default | data: default: SortPriority Critical>Signature>High>Normal>Low, then type rank, then ItemId | states: default sort only today (Items/MordecaiItemLibrary.cpp CompareSortPriority; agent_rules_v2 'strong sorting/filtering')
- [In-game menu|implied] **Item actions (Use / Equip / Drop)**: Act on carried items | data: Use (consumable), Equip/Unequip (gear), Drop/Trash | states: available; blocked (unidentified + RequiresIdentificationToEquip); not equippable (PLANNER_LOG TODO(DECISION) N22; US-071 tech notes (drop/equip out of scope); MordecaiItemLibrary::CanEquipInstance)
- [In-game menu|planned-story] **Equipment panel (weapon sets and armor)**: Manage equipped gear | data: MainHand, OffHand, TwoHand weapon slots; Weapon Set 1/2; armor and trinket slots undefined | states: empty slot; equipped; two-hand blocks off-hand (stories/PLAN.md US-072; Weapons/MordecaiWeaponTypes.h EMordecaiEquipSlot; US-029/US-030 placeholders)
- [In-game menu|implied] **Quickslot assignment**: Assign consumables to Use Item A-D | data: 4 slots and assignable consumables | states: empty; assigned (Specs/control_bindings_v1.1.md Use Item A-D)
- [Village / service screen|specified] **Mage Tower identification service**: Identify unidentified magical items in town | data: unidentified items, cost (undecided), result reveal | states: per-item, batch, timed or service queue (undecided); free today (Specs/village_system_v1.md §4.2; agent_rules_v2.md §2 + Open Item 4; game_design_v2.md §9, §12)
- [Village / service screen|specified] **Alchemist provisioning / restock**: Restock health potions and expedition consumables | data: health potions, antidotes/cures, resistance tonics, specialty consumables; stock and restock rate | states: in stock; low; restocking; by building tier 1-4 (Functional, Improved, Advanced, Masterwork) (Specs/village_system_v1.md §4.1, §2)
- [Village / service screen|implied] **Town storage / resource bank**: View and spend auto-stored materials on buildings | data: Common materials (Wood, Stone, Iron, Fiber, Herbs, Hides, Monster Parts), refined, upgrade keys/relics with counts | states: enough / not enough for an upgrade (UMordecaiResourceLedger::GetAllResources ('for UI / town-bank queries'); village_system_v1.md §1)
- [Village / service screen|implied] **Vendor / market (buy, sell, salvage)**: Trade items | data: SellValue, BuyValue, SalvageTableId; currency undefined | states: buy; sell; salvage; cannot afford (item_schema_v2.md Economy; game_design_v2.md (market building); attribute_scaling_v1.md CHA 'vendor outcomes')
- [Village / service screen|specified] **Blacksmith services**: Repair, sharpen, reinforce gear | data: reinforcement, repair, sharpening (temporary damage/posture bonuses), equipment preparation, resist sets | states: service available; applied; disabled by Corroded (Specs/village_system_v1.md §4.3; game_design_v2.md §7; status_effects_launch_set_v1.md Corroded)
- [Notification / overlay|implied] **Auto-stored resource toast**: Confirm that a material went to the ledger (today it disappears silently) | data: '+N <Material>' routed to Stored Resources | states: single; batched (UMordecaiResourceLedger::OnResourceChanged; UMordecaiInventoryComponent::PickupItem)
- [Notification / overlay|implied] **Item acquired / signature drop / upgrade key banner**: Celebrate important loot (boss drop, upgrade key) | data: item name, rarity, type; boss/elite magic weapon drops 100% | states: common pickup; magical (unidentified); upgrade key; signature/boss drop (game_design_v2.md §3 session loop, §6; item_schema_v2 BossDropGuaranteed)
- [Notification / overlay|implied] **Identification reveal**: Moment the true item is revealed | data: old partial name -> DisplayName, Description, full tags | states: before; revealed (UMordecaiIdentificationService::OnItemIdentified)
- [Notification / overlay|implied] **Boss phase / enrage banner**: Signal readable phase transitions | data: phase number; enrage | states: phase 1; phase 2 (AoE unlocked); phase 3 (enrage) (Specs/enemy_ai_framework_v1.md; enemy_archetypes_v1.md Boss Phase Driver ('scripted but readable phase transitions'))
- [Other|implied] **Lockpicking minigame**: Open locked containers | data: success window width and progress speed scale with rank | states: in progress; success; tool break (rank 15 reduces it); reveal hidden containers (rank 20) (Specs/skill_sheet_v1.1.md Lockpicking)

## KEY TERMS
- Unlimited carry: inventory has no capacity and is never a difficulty source (agent_rules_v2 §2, locked)
- Town gating: loot friction comes from town services (Mage Tower identification, Alchemist potion stock, delivering materials), not bag space
- Auto-store / AutoStoreOnPickup: Materials and TownResources go straight to the Resource Ledger on pickup
- Resource Ledger / 'Stored Resources': per-character list of auto-stored resource counts, shown as '<Name>  x<Count>'
- Mage Tower: town building that identifies magical items (IdentificationService default 'MageTower')
- Alchemist: town building whose stock limits the health potions you can take into the field
- Identified / Unidentified: per-instance identification state (EMordecaiIdentificationState)
- Partial info: unidentified name 'Unidentified <Rarity> <Subtype|Type>' (e.g. 'Unidentified Purple Amulet') when ShowPartialInfoBeforeIdentify is on
- 'Unknown Item': name of an unidentified item when partial info is off
- Identify: action that flips an item to Identified; today a free, instant button in the Inventory tab
- Rarity tiers (code order): Common (Gray), Green, Blue, Purple, Red, Gold; specs write 'Red / Gold' / 'Red(Gold)' as the top tier
- Rarity aliases (code comments / stories only): Green=Uncommon, Blue=Rare, Purple=Epic, Red=Legendary, Gold=Mythic
- Item types: Weapon, Armor, Trinket, Consumable, Material, TownResource ('Resource'), UpgradeKey ('Upgrade Key'), QuestItem ('Quest Item'), MagicalItem ('Magical Item'), CurrencyProxy ('Currency')
- Inventory filters: All, Weapons, Armor, Trinkets, Consumables, Materials, Quest, Magical
- Sort priority: Critical > Signature > High > Normal > Low
- Signature item: curated or boss drop that sorts high (MagicalItem default SortPriority)
- Upgrade Key (formerly 'Relic'): unique town-upgrade unlocker, not wearable (Philosopher's Stone, Heartwood Core, Deepcore Prism, Saint's Ember, Phase Lens, Siege Sigil)
- Common / Refined Materials: Wood, Stone, Iron, Fiber, Herbs, Hides, Monster Parts / Lumber, Cut Stone, Iron Ingots, Alchemical Extract, Arcane Ink
- Storage domains: PlayerInventory, TownStorage, QuestState
- Bind types: None, BindOnPickup, BindOnEquip, QuestBound
- Affixes: Prefixes (e.g. 'flaming') and Suffixes (e.g. 'of_cinders') built from known modifier primitives
- Modifier types: StatModifier, AbilityModifier, ProcEffect, PassiveBonus
- Proc triggers: OnHit, OnCrit, OnKill, OnBlock, OnPerfectBlock, OnParry, OnDodge, OnAbilityUse, OnStatusApplied, OnLowHealth, OnEnterCombat
- Common-sense loot: believable drops ('no wolves drop gold'); no grinding for random rare weapons
- Curated magic items: named items found via bosses/dungeons/story, often gated by prerequisites
- Environment containers: weapon racks, armor stands, chests (main source of equipment)
- Loot anchor (PM_LootAnchor / DA_Loot_<CellId>): map markup for loot placement
- No loot vomit: UI/feedback rule against loot clutter
- Press-to-Pickup / Interact: F (keyboard) / X (gamepad) picks up the focused pickup
- Focused pickup: nearest pickup within the 120 cm trigger
- Equip slots: MainHand, OffHand, TwoHand (weapons only so far)
- Weapon Set Swap: Set 1 <-> Set 2 on Tab / hold Y (deferred; slice cycles weapons)
- Use Item A-D: quickslots on D-pad / keys 1-4
- Enemy tiers: Basic, Elite, Champion, Boss, Summon, Miniboss
- Encounter roles: Frontliner, Harasser, Bruiser, Artillery, Controller, Support, Summoner, Tank, Assassin, AreaDenial, BossPhaseDriver
- Aggression styles: DirectRush, FlankAndPressure, HoldLine, KiteAndChip, AreaDenial, Ambush, SupportFromRear, SiegePressure
- ThreatPriority: per-enemy priority (Medium, High)
- AI states: Idle, Approach, Attack, Recover, Staggered, Leash, Dead
- AggroRange / LeashRange / AttackRange: 900 cm / 1800 cm / 200 cm fallback (real range = attack profile reach, 150 cm)
- Posture: enemy meter; breaking it opens the critical window
- Posture Broken / Stagger: 1000 ms, 1.5x damage taken (Mordecai.State.PostureBroken)
- Knock Down: escalation from stagger after 50 damage; 2000 ms, 2.0x damage taken (Mordecai.State.KnockedDown)
- Critical window / Riposte: punish window after a posture break or perfect parry
- Telegraph profile: WindupClarity, FXClarity, AudioCues, GroundMarkers
- TelegraphType: CastGlow or WindupPose (ability presentation)
- JumpAvoidable / low sweep: sweeps that a jump avoids
- Boss phases: Phase 1 basic attacks, Phase 2 AoE attacks unlocked, Phase 3 Enrage mechanics
- Frontliner: the only real enemy in the build (BP_MordecaiEnemy_Frontliner, dark red cylinder, 200 HP)
- TestDummy / TestAttacker: CombatTestMap test enemies (blue-grey passive dummy / orange stationary attacker)
- Enemy Indicator: Lyra IndicatorSystem overlay with name, color-coded health and status icons
- Continuants: the robot race; some broken and hostile, some sentient
- Bosses: Thorn Guardian, Iron Warden, Drowned King, Crystal Seer, Tempest Ascendant, The Lich King (final)
- Biomes: Verdant Wilds, Searing Expanse, Mire of Echoes, Crystal Caverns, Storm Peaks, Withered Kingdom

## OPEN QUESTIONS
- Rarity count: specs give 'Red / Gold' / 'Red(Gold)' as one top tier, but code has 6 tiers with Red (Legendary) and Gold (Mythic) separate (Gold appended by US-071). Which is canonical for the UI?
- Rarity naming: player-facing text uses color words ('Unidentified Purple Amulet'); stories and code comments use Uncommon/Rare/Epic/Legendary/Mythic. Which vocabulary does the player see? Feats separately use Common/Rare/Legendary.
- Does Common (Gray) apply to magical items? game_design_v2 lists magical rarity as Green/Blue/Purple/Red(Gold) only.
- Several colors collide across systems: rarity Red equals enemy low-health red (0.9,0.1,0.1); rarity Gold is near enemy mid-health yellow and Radiant damage; rarity Green is near enemy healthy green, healing pops and Poison; rarity Purple is near Arcane. The design system has to decide on semantic separation.
- Identification UX is open (agent_rules_v2 Open Item 4, game_design_v2 §12): per-item, batch, timed or service queue; cost (gold/time/reagent); how much partial info shows. The current slice lets the player Identify anywhere from the pause-menu Inventory tab. Should that stay, or move to a Mage Tower screen?
- Potion field-cap model (hard cap, soft stock cap or refill-style provisioning, Open Item 5) decides what the HUD potion counter shows.
- Item Use / Equip / Drop actions do not exist (TODO(DECISION) N22). Are they in scope before the US-072 equipment panel?
- Quickslot key conflict: control_bindings_v1.1 maps key 1 to Use Item A, but code binds 1 ('One') to Spell4.
- 'I — Inventory' is bound in code but missing from control_bindings_v1.1.md (TODO(DECISION)); in-menu LB/RB is not listed either. Device-aware gamepad glyphs for prompts are deferred.
- Two enemy health readouts (WBP_EnemyHealthBar widget component and the US-064 indicator): should the old bar be removed once US-092 makes indicators appear (TODO(DECISION) in US-092)?
- Enemy indicator name falls back to the actor's object name (e.g. 'BP_MordecaiEnemy_Frontliner_C_0') when EnemyDisplayName is empty. No enemy has a DisplayName set in content.
- No spec says whether enemy posture is shown (meter, break flash, knock-down marker) or how the riposte/critical window is signalled. new_spells_proposal mentions a 'boss posture bar'.
- No spec for boss health bars, phase or enrage presentation, or tier badges for Elite/Champion/Miniboss/Summon.
- Status display tension: game_design_v2 says status is 'primarily VFX-driven, not icon soup', yet the enemy indicator has a 16-icon status row. How much enemy status UI is wanted?
- Off-screen enemy markers and aggro/alert cues are unspecified; the Lyra indicator is not clamped to the screen edge.
- Telegraph ground markers are a per-enemy flag (GroundMarkers false in the example). Is there a shared visual language for windups, AoE areas, low sweeps (jump-avoidable) and hazard zones?
- Currency is undefined: 'bandits drop gold', SellValue/BuyValue exist, CurrencyProxy has no gameplay tag, PlayFab may hold currencies. Is there a gold wallet, and where is it shown?
- Loot boxes: no spec or story mentions loot boxes or a system-announcer style of reward notification. Loot comes from containers, enemy drops and bosses. Whether the Dungeon Crawler Carl part of the brief should add loot-box or achievement-reward presentation is a design call, not in any spec.
- Corpse visuals are a TODO(DECISION). A dead enemy currently leaves only its empty health bar. Loot drops from enemies are not implemented.
- Armor and trinket equip slots are undefined (US-029 Armor and US-030 Shield need design input). Weapon Set Swap (Set 1/2) is deferred in favor of linear cycling, and the previous-weapon key (Shift+Tab vs BackSlash) is open (US-096).
- Durability: schema default UsesDurability false, but the Blacksmith offers repair and Corroded affects gear. Is durability ever shown?
- Sort order: agent_rules_v2 says quest items, upgrade keys, magical items, then common gear, but the code's tie-break ranks Weapon/Armor/Trinket first and only SortPriority defaults lift keys and magical items. Which order should the UI promise?
- The Resource Ledger is per-character today (TODO(DECISION) migrate to PlayerState). Does the inventory's 'Stored Resources' panel become a town-storage view?