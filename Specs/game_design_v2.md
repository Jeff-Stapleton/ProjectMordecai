# Game Design
v2.0.0 Last updated: 2026-03-17

## 1) High Concept
A diorama-style (Link’s Awakening / Echoes of Wisdom) action RPG where player skill can clear the entire game at level 1 with no upgrades, while D&D-like attributes + granular skills + curated loot create deep build identity. Between expeditions, a self-sufficient town provides services, research, provisioning, and Metroidvania-style world gates—as an upgrade vector, not a chore.

## 2) Non-Negotiable Pillars
1. **Skill-first combat:** Mastery (dodge/block/parry/spacing) beats stats.
2. **Granular skills (DCC vibe):** “Everything you do is a skill.” Skills level to 20 and unlock techniques at milestones.
3. **Feats are earned, not picked:** Unlocked via gameplay achievements. Tiered (Common/Rare/Legendary). Strong feats can include drawbacks (DOS2-like).
4. **Common-sense loot:** Wolves drop pelts/teeth; bandits drop gold + plausible items. No grinding for random rare weapons.
5. **Curated magic items:** Found via bosses/dungeons/story (BG3-style gating/prereqs). Rarity: Green/Blue/Purple/Red(Gold).
6. **Town is strategic and delegated:** NPCs gather/produce. Player assigns jobs, selects initiatives, upgrades buildings, and occasionally defends assets.
7. **Metroidvania world:** 4–6 biomes. Town upgrades unlock new routes in old zones.
8. **Clean UI:** Always show Health (0–100%), tiered Stamina, Spell Points. Status is primarily VFX-driven, not icon soup.
9. **Low-friction inventory:** Inventory management is not a core source of challenge; expedition pressure should come from combat, preparation, identification, and town services.

## 3) Core Loops

### Moment-to-moment
Explore → Read threats → Engage skill combat → Gain meaningful loot/resources/keys → Continue.

### Session loop
Quest/dungeon → miniboss/boss → signature item or upgrade key → return to town → identify loot / refill services / select initiative → unlock new routes → repeat.

### Campaign loop
Town grows → specialists arrive → new gates open → build-defining items acquired → region bosses defeated → final act.

## 4) Combat (Elden Ring DNA, top-down readability)

### Core resources
- **Health:** Displayed as % (0–100).
- **Stamina:** Tiered; low stamina reduces effectiveness, never blocks actions.
- **Spell Points:** Discrete points; slower regen than stamina; INT improves regen; spells can upcast by spending more points.

### Defense mechanics
- **Dodge:** i-frames; perfect dodge rewards (e.g., stamina refund or brief advantage).
- **Block:** drains stamina; stability matters; perfect block can reward posture damage/charge mechanics.
- **Parry:** high-risk/high-reward; perfect parry creates large posture damage + punish window; failed parry has meaningful punish.

### Posture model
- **Enemy Posture:** built by heavy hits + perfect defense + certain spells; break → critical/riposte window.
- **Player Stability:** determines stagger/interrupt resistance; increased by armor + DIS + gear + skills + feats.

### Skill promise enforcement
Enemies are not sponges. Difficulty comes from timing, spacing, patterns, stamina management, and build tradeoffs—not stat checks.

## 5) Progression

### Levels
- **Max level:** 60 (WoW nod).
- Leveling grants skill points and minor baseline growth; not the primary power driver.

### Attributes (D&D-like scalers)
- Generally static (rarely changed).
- Scale derived stats (HP, stamina, SP, regen, posture, resist).

### Skills (granular, max 20)
- Weapon, defense/armor, movement/exploration, utility/interaction, magic schools.
- Milestones: Rank 1 / 5 / 10 / 15 / 20 unlock techniques/passives/interactions.

### Feats (Outer Worlds + DOS2)
- Unlocked by achievements/patterns (e.g., burn 6 enemies at once → Pyromaniac).
- Rarity tiers with tradeoffs:
  - **Common:** modest, no downside
  - **Rare:** strong specialization, light downside/condition
  - **Legendary:** build-defining, meaningful drawback

## 6) Loot & Economy

### Common-sense loot rules (hard constraints)
- Enemies drop believable items/resources (no “wolves drop gold”).
- No mob grinding for random rare weapons.
- Boss/elite wielding a magic weapon drops it 100%.
- Most equipment is found in environment containers (weapon racks, armor stands, chests) and doubles as decor.

### Loot types
1. **Common gear:** ubiquitous, readable, decorative, serviceable.
2. **Crafted gear:** town-produced utility/sidegrades (repair, sharpen, oils, resist sets).
3. **Magical items (Green/Blue/Purple/Red(Gold)):** curated, named, build hooks, often gated by prerequisites.
4. **Upgrade Keys:** unique resources for town upgrades (e.g., Philosopher’s Stone unlocks Mage Tower tier). Not wearable gear.
5. **Materials / Town Resources:** monster parts, herbs, ore, hides, and similar progression loot primarily used to improve village systems and unlock long-term capabilities.

### Inventory rule
- Player inventory is effectively **unlimited** for normal gameplay.
- Inventory management should **not** be part of what makes progression difficult.
- Players should be able to pick up and keep all meaningful loot they find during expeditions.
- Materials and town resources should be **auto-stored**.
- The game should never expect new players to know which monster parts or materials will matter later and punish them for guessing wrong.

### Loot friction rule
Loot friction should come from **use gating**, not carry-space gating.

Primary constraints:
- **Magical items may require identification** at the **Mage Tower** before they can be fully used.
- **Healing sustainability is limited by Alchemist stock**, not by backpack size.
- **Upgrade pacing** comes from what players discover and bring home, not from inventory slot optimization.

## 7) Town / City Management (delegated, not survival chores)

### Player controls
- Assign jobs (miners/lumber/research/guards/etc.).
- Pick a small number of initiatives (1–3) at a time.
- Upgrade buildings: blacksmith, alchemist, mage tower, walls/keep, lumber yard, mine, market, clinic.

### Outputs (QoL + passive bonuses)
- Repair, sharpen (temporary damage/posture bonuses)
- Identify magical items
- Research spells/mods
- Produce potions/consumables
- Unlock fast travel routes/shortcuts
- Improve SP regen/cost via research lines (not raw damage creep)

### Events (occasional)
Protect workers, reclaim mine from goblins, defend caravan, etc. One mission/decision—never chores.

## 8) World Structure (Metroidvania)
- 4–6 biomes with main dungeon + optional micro-dungeons + roaming elites.
- Certain routes are inaccessible until town upgrades unlock them (bridges, scaffolding, phase barriers, drainage, siege tools).
- Returning to older biomes is expected and rewarding.

## 9) UI / Feedback
- Always visible: Health%, tiered stamina, spell points.
- Status effects communicated primarily via VFX + animation; minimal critical UI only.
- Inventory should be low clutter:
  - materials auto-stored
  - signature items prioritized in sorting
  - no loot vomit
  - no bag-slot micromanagement
- Unidentified magical items should be clearly signposted in inventory and town UI.

## 10) Status Effects (Launch Set)
Principle: statuses must have distinct gameplay identity (not “damage colors”).

Launch examples include:
- Burning
- Bleeding
- Poisoned
- Frostbitten
- Shocked
- Weakened
- Brittle
- Silenced
- Rooted
- Blinded
- Fear
- Cursed
- Exposed
- Corroded
- Drenched
- Focused (Buff)

## 11) File Relationships
- `skill_sheet_v1` — full skill spec sheets (scaling placeholders, town/world hooks, feat hooks)
- `character_attributes_v1` — attribute → derived stat scaling table
- `status_effects_v1` — launch status set
- `village_system_v1` — village progression and provisioning rules
- `item_schema_v2` — canonical item schema with identification and auto-storage support
- `agent_rules_v2` — implementation guardrails for systems and agents

## 12) Open Decisions (safe to iterate later)
- Exact numeric curves for BaseHP/BaseStamina/BaseSP by level.
- Exact stamina tier multipliers and perfect-window timings.
- Final skill count for launch (target ~40–70; current catalog is 60).
- Whether magic schools remain “by school” vs refactor to “spell type.”
- Exact identification UX (single-item, batch, timed, or service queue).
- Exact potion field-cap model from the Alchemist.
