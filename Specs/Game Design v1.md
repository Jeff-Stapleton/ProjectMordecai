# Game Design V1

*Converted from Game Design v1.docx*

Game Design

Version 1.0 Last updated: 2026-03-02

### 1) High Concept

A diorama-style (Link’s Awakening / Echoes of Wisdom) action RPG where **player skill can clear the entire game at level 1 with no upgrades**, while **D&D-like attributes + granular skills + curated loot** create deep build identity. Between expeditions, a **self-sufficient town** provides services, research, crafting, and **Metroidvania-style world gates**—as an upgrade vector, not a chore.


### 2) Non-Negotiable Pillars

**Skill-first combat:** Mastery (dodge/block/parry/spacing) beats stats.

**Granular skills (DCC vibe):** “Everything you do is a skill.” Skills level to 20 and unlock techniques at milestones.

**Feats are earned, not picked:** Unlocked via gameplay achievements. Tiered (Common/Rare/Legendary). Strong feats can include drawbacks (DOS2-like).

**Common-sense loot:** Wolves drop pelts/teeth; bandits drop gold + plausible items. No grinding for random rare weapons.

**Curated magic items:** Found via bosses/dungeons/story (BG3-style gating/prereqs). Rarity: **Green/Blue/Purple/Red(Gold)**.

**Town is strategic and delegated:** NPCs gather/produce. Player assigns jobs, selects initiatives, upgrades buildings, and occasionally defends assets.

**Metroidvania world:** 4–6 biomes. Town upgrades unlock new routes in old zones.

**Clean UI:** Always show Health (0–100%), tiered Stamina, Spell Points. Status is primarily VFX-driven, not icon soup.


### 3) Core Loops

#### Moment-to-moment

Explore → Read threats → Engage skill combat → Gain meaningful loot/resources/keys → Continue.


#### Session loop

Quest/dungeon → miniboss/boss → signature item or upgrade key → return to town → select initiative/services → unlock new routes → repeat.


#### Campaign loop

Town grows → specialists arrive → new gates open → build-defining items acquired → region bosses defeated → final act.


### 4) Combat (Elden Ring DNA, top-down readability)

#### Core resources

**Health:** Displayed as % (0–100).

**Stamina:** Tiered; low stamina reduces effectiveness, never blocks actions.

**Spell Points:** Discrete points; slower regen than stamina; INT+CON improve regen; spells can **upcast** by spending more points.


#### Defense mechanics

**Dodge:** i-frames; perfect dodge rewards (e.g., stamina refund or brief advantage).

**Block:** drains stamina; stability matters; perfect block can reward posture damage/charge mechanics.

**Parry:** high-risk/high-reward; perfect parry creates large posture damage + punish window; failed parry has meaningful punish.


#### Posture model

**Enemy Posture:** built by heavy hits + perfect defense + certain spells; break → critical/riposte window.

**Player Stability:** determines stagger/interrupt resistance; increased by armor skills + CON + gear/feats.


#### Skill promise enforcement

Enemies are not sponges. Difficulty comes from timing, spacing, patterns, stamina management, and build tradeoffs—not stat checks.


### 5) Progression

#### Levels

**Max level:** 60 (WoW nod).

Leveling grants **skill points** and minor baseline growth; not the primary power driver.


#### Attributes (D&D-like scalers)

Generally static (rarely changed).

Scale derived stats (HP, stamina, SP, regen, posture, resist). See **attribute_scaling_v1.csv** for the current table.


#### Skills (granular, max 20)

Weapon, defense/armor, movement/exploration, utility/interaction, magic schools.

**Milestones:** Rank 1 / 5 / 10 / 15 / 20 unlock techniques/passives/interaction

Full catalog: **skill_spec_sheets_v1.csv** and **skill_catalog_v1.csv**.


#### Feats (Outer Worlds + DOS2)

Unlocked by achievements/patterns (e.g., burn 6 enemies at once → Pyromaniac).

Rarity tiers with tradeoffs:

**Common:** modest, no downside

**Rare:** strong specialization, light downside/condition

**Legendary:** build-defining, meaningful drawback


### 6) Loot & Economy

#### Common-sense loot rules (hard constraints)

Enemies drop believable items/resources (no “wolves drop gold”).

No mob grinding for random rare weapons.

Boss/elite wielding a magic weapon drops it 100%.

Most equipment is found in **environment containers** (weapon racks, armor stands, chests) and doubles as decor.


#### Loot types

**Common gear:** ubiquitous, readable, decorative, serviceable.

**Crafted gear:** town-produced utility/sidegrades (repair, sharpen, oils, resist sets).

**Magical items (Green/Blue/Purple/Red(Gold)):** curated, named, build hooks, often gated by prerequisites.

**Upgrade Keys (formerly “Relics”):** unique resources for town upgrades (e.g., Philosopher’s Stone unlocks Mage Tower tier). Not wearable gear.


### 7) Town / City Management (delegated, not survival chores)

#### Player controls

Assign jobs (miners/lumber/research/guards/etc.).

Pick a small number of **initiatives** (1–3) at a time.

Upgrade buildings: blacksmith, alchemist, mage tower, walls/keep, lumber yard, mine, market, clinic.


#### Outputs (QoL + passive bonuses)

Repair, sharpen (temporary damage/posture bonuses)

Identify magical items

Research spells/mods

Produce potions/consumables

Unlock fast travel routes/shortcuts

Improve SP regen/cost via research lines (not raw damage creep)


#### Events (occasional)

Protect workers, reclaim mine from goblins, defend caravan, etc. One mission/decision—never chores.


### 8) World Structure (Metroidvania)

**4–6 biomes** with main dungeon + optional micro-dungeons + roaming elites.

Certain routes are inaccessible until town upgrades unlock them (bridges, scaffolding, phase barriers, drainage, siege tools).

Returning to older biomes is expected and rewarding.


### 9) UI / Feedback

Always visible: **Health%**, **tiered stamina**, **spell points**.

Status effects communicated primarily via **VFX + animation**; minimal critical UI only.

Inventory should be low clutter:

materials auto-stored

signature items prioritized in sorting

no “loot vomit”


### 10) Status Effects (Launch Set)

Principle: statuses must have distinct gameplay identity (not “damage colors”). Launch set CSV:

- **status_effects_launch_set_v1.csv**

Includes: Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched, Focused (Buff).


### 11) Files (Project Sources)

- **skill_spec_sheets_v1.csv** — full skill spec sheets (scaling placeholders, town/world hooks, feat hooks)

- **skill_catalog_v1.csv** — compact skill milestones

- **attribute_scaling_v1.csv** — attribute → derived stat scaling table (v1)

- **status_effects_launch_set_v1.csv** — launch status set


### 12) Open Decisions (safe to iterate later)

- Exact numeric curves for BaseHP/BaseStamina/BaseSP by level.

- Exact stamina tier multipliers and perfect-window timings.

- Final skill count for launch (target ~40–70; current catalog is 60).

- Whether magic schools remain “by school” vs refactor to “spell type.”

