# Village System v1.0

## Core Pillar

The village is the game's crafting system, but the player never crafts in the traditional sense.

Players do **not** manually combine herbs, ore, wood, or other resources into one-off consumables or equipment through crafting menus. Instead, players invest those resources into **village infrastructure**, and the infrastructure becomes a **renewable provisioning network**.

Upgrading the Alchemist means the village can now reliably provide better potions, in larger quantities, with faster restocks. Upgrading the Blacksmith means better reinforcement, repair, and weapon preparation services. Upgrading the Clinic means better recovery kits and blessings. The same principle applies across all building vendors.

This creates several desired outcomes:

- no inventory clutter
- no endless material hoarding anxiety
- no menu-crafting busywork
- progression feels permanent and legible
- returning to town feels meaningful and rewarding
- the village has a strong gameplay identity instead of being a cosmetic side activity

## Village Service Rule

**If a player unlocks a village service through construction or upgrade, that service should remain available in a renewable form.**  
Players should not need to repeatedly re-craft the same baseline tools using raw ingredients.

## Return-to-Town Rule

Town should pull the player back through **meaningful service friction**, not inventory pressure.

The main reasons to return are:
- identify magical items at the **Mage Tower**
- restock healing and expedition supplies through the **Alchemist**
- convert gathered materials into permanent village growth
- start, complete, or redirect village initiatives
- unlock new routes, services, and world capabilities

The player should **not** return to town because their bags are full.

---

## High-Level Village Philosophy

The village should be:

- a strategic progression system
- a renewable provisioning system
- a source of world and route unlocking
- a source of services, blessings, preparation, and institutional support
- a place where player investment permanently improves future expeditions

The village should **not** become:

- a survival sim
- a chore-heavy settlement micromanager
- a traditional crafting economy
- a place where players hoard hundreds of ingredients for fear of wasting them
- a place where inventory slot pressure creates accidental progression mistakes

---

# 1. Core Resource Model

Resources are divided into three categories.

## 1.1 Common Materials
Used in large quantities for building and upgrading structures.

- Wood
- Stone
- Iron
- Fiber
- Herbs
- Hides
- Monster Parts

## 1.2 Refined Materials
Created by village infrastructure once the appropriate buildings are online.

- Lumber
- Cut Stone
- Iron Ingots
- Alchemical Extract
- Arcane Ink

## 1.3 Upgrade Keys / Relics
Rare progression unlockers used to break upgrade caps or unlock special structures.

- Philosopher's Stone
- Heartwood Core
- Deepcore Prism
- Saint's Ember
- Phase Lens
- Siege Sigil

## Resource Handling Rule

Resources are primarily used for:

- building structures
- upgrading structures
- unlocking higher service tiers
- restoration projects
- research
- town expansion
- attracting or sustaining specialists

Resources are **not** primarily used for:

- repeated one-off consumable crafting
- routine gear crafting
- reagent juggling
- manual item production loops

### Auto-Storage Rule
- Common materials, refined materials, and town resources should be **auto-stored** when collected.
- Players should not be asked to manually protect important upgrade materials from accidental loss.
- If a material matters for future town progression, the design should assume the player can safely keep it.

---

# 2. Village Building Tier Structure

Most major buildings use a 4-tier progression.

- **Tier 1:** Functional
- **Tier 2:** Improved
- **Tier 3:** Advanced
- **Tier 4:** Masterwork

Not every building must launch with every tier fully implemented, but this should be the intended long-term framework.

---

# 3. Population, Housing, and Upkeep

The village must support its population. More villagers means more labor, more throughput, and more specialization, but also more upkeep burden.

## 3.1 Core Population Concepts

### Population
Total villagers living in the settlement.

### Housing Capacity
How many villagers current housing can support.

### Support Capacity
A derived value representing how well the village can sustain its current population.

Support Capacity is influenced by:

- housing
- food
- health / sanitation
- safety / order
- economy / livelihood

### Healthy Margin
The difference between village support and village population demand.

- if support is meaningfully above demand, villagers are attracted
- if support is only barely meeting demand, growth stalls
- if support falls below demand, the village becomes strained
- if support falls too far below demand, villagers begin leaving or services degrade

The system should be understandable and low-friction, not a spreadsheet simulator.

---

# 4. Building Roles

## 4.1 Alchemist
The Alchemist is the village's primary expedition provisioning node.

Provides:
- health potions
- antidotes / cures
- resistance tonics
- specialty expedition consumables

### Alchemist Rule
Healing pressure should come from **Alchemist stock and replenishment**, not carry capacity.

Desired behavior:
- the village can only prepare so many health potions at a time
- taking damage and consuming stock creates real expedition pressure
- low potion reserves naturally encourage returning to town
- potion scarcity should feel like a preparation problem, not a bag problem

## 4.2 Mage Tower
The Mage Tower is the appraisal, identification, and arcane analysis center.

Provides:
- magical item identification
- arcane research
- spell modification / insight systems
- magical barrier / route unlocking support

### Mage Tower Rule
Magical items may be recovered in an **Unidentified** state.

Desired behavior:
- players can carry unidentified magical items without inventory stress
- those items are not fully usable until identified at the Mage Tower
- magical loot therefore remains exciting and immediately collectible, while still reinforcing the town loop

## 4.3 Blacksmith
Provides:
- reinforcement
- repair
- sharpening
- equipment preparation
- sidegrade support gear / resist sets

## 4.4 Clinic
Provides:
- recovery services
- cleansing / blessing support
- survivability preparation
- passive health and resilience improvements through village quality

## 4.5 Keep / Walls / Defenses
Provides:
- village safety
- event resilience
- higher-tier civic expansion support
- symbolic proof of long-term settlement advancement

---

# 5. Intended Player Experience

The village loop should feel like this:

1. go on expedition
2. gather gear, magical items, materials, and keys freely
3. return because healing stock is getting low, magical items need identification, or an upgrade is now possible
4. invest in buildings / initiatives / specialists
5. leave town stronger, better supplied, and with new strategic options

The village loop should **not** feel like this:

1. go on expedition
2. stop looting because inventory is full
3. throw away useful materials
4. return to town just to empty bags

---

# 6. Design Guardrails

### DO
- make town services renewable and legible
- let players keep important progression materials
- use village output as the source of long-term convenience and strength
- create reasons to return that feel strategic and satisfying

### DO NOT
- create survival-style storage pressure
- require material babysitting
- turn building progression into repeated reagent crafting
- use inventory friction as the main source of expedition tension
