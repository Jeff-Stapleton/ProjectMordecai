# Mordecai UI design system — lead's brief (synthesis of the research reports in this folder)

## The ask
Jeff (solo dev of Project Mordecai) asked for a design system covering MAIN MENUS / FRONT END, IN-GAME MENUS and the GAMEPLAY HUD.
Creative brief, his words: "a mashup of the book Dungeon Crawler Carl and the Nintendo Switch remake of Link's Awakening."

## What the game is (from the specs)
- UE 5.7 ARPG on Lyra. Fixed diorama camera (pitch -50, never rotates). Twin-stick: WASD/left stick moves, mouse/right stick aims. GAS combat.
- High concept: "A diorama-style (Link's Awakening / Echoes of Wisdom) action RPG where player skill can clear the entire game at level 1 ... D&D-like attributes + granular skills + curated loot".
- DCC shows up in the docs ONLY as pillar 2 "Granular skills (DCC vibe): 'Everything you do is a skill.' Skills level to 20 and unlock techniques at milestones." Feats are "earned, not picked: unlocked via gameplay achievements" (e.g. "burn 6 enemies at once -> Pyromaniac"). No crawlers, floors, game show, viewers, loot boxes or System AI exist in the docs.
- World tone is serious: theme "Impermanence vs Control". Ages: Ancient Age (Automation) -> Current Age (Necromancy) -> Player Era (Acceptance). The robot race "Continuants" "maintain systems indefinitely"; "Automated systems still running without purpose". Final boss: the Lich King. Two endings (Control / Acceptance).
- 6 biomes: Verdant Wilds (Denial), Searing Expanse (Control), Mire of Echoes (Avoidance), Crystal Caverns (Preservation), Storm Peaks (Obsession), Withered Kingdom (Final Truth). Bosses: Thorn Guardian, Iron Warden, Drowned King, Crystal Seer, Tempest Ascendant, The Lich King.
- Town/village is the crafting system: Mage Tower (identify), Alchemist (potion stock), Blacksmith, Clinic, Keep/Walls, lumber yard, mine, market. Building tiers Functional/Improved/Advanced/Masterwork. Initiatives (1-3 active), jobs, Population/Housing/Support Capacity/Healthy Margin. Upgrade Keys: Philosopher's Stone, Heartwood Core, Deepcore Prism, Saint's Ember, Phase Lens, Siege Sigil.
- Pillar 8 "Clean UI: Always show Health (0–100%), tiered Stamina, Spell Points. Status is primarily VFX-driven, not icon soup." Pillar 9: low-friction, unlimited-carry inventory; no bags.

## Systems the UI must carry (exact vocabulary)
- Resources: Health (shown as %), Stamina tiers Green (>=66%, 1.25x) / Yellow (>=33%, 1.0x) / Red (>0, 0.75x) / Exhausted (<=0, 0.5x; floor -50), Posture (player bar; enemies break -> "Posture Broken" stagger 1 s x1.5 -> Knocked Down 2 s x2.0; critical window / riposte), Spell Points (discrete, "x / y", max 10 today; upcast doubles cost).
- Spells on HUD: Q Fireball (3 SP, 2.0 s CD, Fire), E Blink (2 SP, 4.0 s), R Stone Skin (4 SP, 15 s), 1 Restoration (3 SP, 10 s). Gamepad LB/RB/–/D-pad Up. Spec also defines Skill A/B with radial select, quick items A-D (1-4 / D-pad), Throw (G / Y), weapon swap (Tab / hold Y), Map (M / View).
- Weapons: Longsword, Greatsword, Shortsword, Dagger, Axe, Mace, Spear, Quarterstaff, Unarmed (+ bows, crossbow, throwable, wand). Light chain 2-5 hits, heavy = hold 0.4 s, combo window 800 ms.
- Defense feedback strings (shipping): "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!", combo "Hit N".
- Damage types: Physical (Slash/Pierce/Blunt), Fire, Frost, Lightning, Poison, Corrosive, Arcane, Shadow (Curse), Radiant (Holy). Healing "+N". Crits x1.5 scale.
- 16 launch statuses with one-line identities (USE THESE as UI copy): Burning "You're on a clock", Bleeding "Recovery is denied", Poisoned "You can't catch your breath", Frostbitten (stacks 1-5 -> Frozen) "Sluggish body, slower reactions", Shocked (stacks 1-3) "Timing disruption", Weakened "You can't break them", Brittle "You break easily", Silenced "Magic is shut off", Rooted "Position solved or punished", Blinded "Reads become unreliable", Fear "Composure collapse", Cursed "Resource rules broken", Exposed "Next mistake hurts more", Corroded "Gear advantage denied", Drenched "Elemental rules shift", Focused (buff) "Mastery streak reward". Categories DoT / Debuff / Control / Buff / Environmental. Spell buffs: Stone Skin, Fire Ward, Blessed, Restoration, Enchanted Weapon, Blurred, Resolved.
- Attributes: STR DEX END (Physical), CON RES DIS (Resilience), INT WIS CHA (Magical); Effective Mod "+9.4%"; 5 primary-derived + 9 secondary-derived stats; Level max 60.
- Skills: 5 categories (Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic), 42 skills in v1.1, rank 0-20, milestones 1/5/10/15/20, 1 skill point per rank, "Allocate".
- Feats: Common / Rare / Legendary (Rare+Legendary carry drawbacks); locked feats show "???" + per-condition progress; header "Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary".
- Items: types Weapon, Armor, Trinket, Consumable, Material, TownResource ("Resource"), UpgradeKey ("Upgrade Key"), QuestItem ("Quest Item"), MagicalItem ("Magical Item"), CurrencyProxy ("Currency"). Rarity Common < Green < Blue < Purple < Red < Gold (player-facing words ARE the colour words: "Unidentified Purple Amulet"; alias Uncommon/Rare/Epic/Legendary/Mythic). Unidentified -> Identify at the Mage Tower. Inventory filters: All, Weapons, Armor, Trinkets, Consumables, Materials, Quest, Magical; ledger "Stored Resources" rows "Iron Ore  x1".
- Enemies: tiers Basic, Elite, Champion, Miniboss, Boss, Summon; roles Frontliner, Harasser, Bruiser, Artillery, Controller, Support, Summoner, Tank, Assassin, Area Denial, Boss Phase Driver. Boss phases 1/2/3 (enrage). Telegraphs must read from the fixed camera.
- Pause menu (shipping): title "Paused", tabs Character / Skills / Feats / Inventory / Settings, LB/RB cycle with wrap, Esc/Start/B back, I opens on Inventory and closes on key-up. Content area 1000x640 design px at 1920x1080.
- Death screen: "You Died", countdown, "Kills: N". Loading: "PROJECT MORDECAI" / "Loading...". Front end is still Lyra (issues #57-#69): needs a Mordecai title screen, play/continue, options, credits, quit confirm, dialogs.

## Today's UI (placeholder, engine defaults)
Roboto Bold 24, default grey progress bars, pure #00FF00/#FF0000 fills, blue-grey tab tints. Everything is UMG built in C++ with BindWidgetOptional names; designers may replace with WBPs that keep those names. Colours in code are FLinearColor (linear!) — author sRGB hex and convert with FLinearColor::FromSRGBColor. Design at 1920x1080, ScaleToFit (0.666 at 720p). Text must stay real UTextBlocks (playtest bridge reads them).

## Constraints for the design system
- Original work. Inspired by LA's toy diorama feel and DCC's interface energy, never copying Nintendo HUD art (hearts, rupees, Nintendo fonts), Nintendo marks, or DCC cover art/characters.
- Fonts: Google Fonts only (OFL), so the game can import the TTFs into UE. At most 3 families. HUD numerals must not jitter (monospaced or tabular-by-default digits; UE Slate does not apply OpenType tnum).
- Every text/ground pair >= 4.5:1 (3:1 for 24px+ text, borders, icons, focus rings). A High Contrast theme (7:1) is an accessibility option the game should ship. Colour is never the only carrier (rarity already carries its colour WORD; stamina tier needs notches/label; statuses need name/glyph).
- The HUD sits over a bright, saturated, toy-like 3D diorama with light floors (pickup labels are already "faint on the light floor").
