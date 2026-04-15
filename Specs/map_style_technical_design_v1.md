# Project Mordecai — Map Style Technical Design v1

Date: 2026-04-12
Purpose: Define how we will build a diorama-style world inspired by the **spatial readability, compression, and authored pathing** seen in the provided Link's Awakening screenshots, while keeping the implementation highly compatible with an **agentic development workflow**.

---

# 1. Intent

We are **not** trying to copy Nintendo assets or exact layouts.
We **are** trying to reproduce the following design qualities:

- compact, toy-like diorama spaces
- very readable paths and combat pockets
- strong separation between traversable floor, walls, ledges, hazards, and secrets
- handcrafted "micro-rooms" connected into a broader metroidvania world
- fast environmental comprehension from a fixed camera
- deceptively simple geometry with high gameplay density

This map philosophy must fit Project Mordecai's locked rules:

- UE **5.7** target
- **3rd-person diorama camera** that follows the character and **does not rotate**
- **metroidvania** world structure
- local-only build/test for now
- Iris replication
- data-driven systems that support AI agents building content safely and repeatedly

See the authoritative stack and game rules in `agent_rules_v2`, `game_design_v2`, and the world themes in `world_design_v1`.

---

# 2. Visual / Spatial Characteristics to Reproduce

From the screenshots, the target style is defined by the following:

## 2.1 Compressed scale
Spaces are physically small, but feel legible and intentional.

- corridors are short
- arenas are compact
- landmarks are always close
- walls are thick and readable
- traversal decisions happen every few seconds

### Design rule
Prefer **dense authored space** over realistic scale.

A 20-second traversal in this game should usually include at least one of:
- decision point
- enemy pocket
- traversal gate
- secret hint
- interactable
- landmark

---

## 2.2 Strong floor/wall readability
The environment is easy to parse because geometry communicates gameplay immediately.

- walkable floor is broad and uncluttered
- cliffs/walls have chunky silhouettes
- hazards use obvious texture/material contrast
- water, pits, brush, and blockers read instantly
- decorative detail is concentrated near edges, not in the middle of traversal lanes

### Design rule
The player should understand the collision story of a room in under 1 second.

---

## 2.3 "Pocket room" composition
Outdoor space behaves like a chain of micro-rooms, even when there are no literal walls.

Examples:
- a beach bend becomes a combat pocket
- a grove becomes a stealth pocket
- a rock ring becomes a puzzle pocket
- a castle landing becomes a tactical pocket

### Design rule
Each pocket should have a clear identity:
- traversal
- combat
- puzzle
- secret
- town/service
- boss lead-in

Do not build large vague fields with no authored purpose.

---

## 2.4 One-screen legibility
Even if the game is not literally screen-by-screen, each local area should feel readable within one camera view.

### Design rule
Any encounter-critical information should be understandable without camera manipulation.

That includes:
- enemy positions
- dodge lanes
- climbable edges
- bombable walls
- phase barriers
- interact prompts
- exits / shortcuts

This directly aligns with the locked camera rule.

---

## 2.5 Layered but shallow verticality
The screenshots show height differences, but not realistic multi-story complexity.

Verticality is used for:
- route control
- visual framing
- secrets
- jump/fall restrictions
- path looping

### Design rule
Use **clear tiers of elevation**, not noisy freeform terrain.

Recommended tier set:
- Tier 0: main walkable ground
- Tier 1: raised ledge
- Tier 2: major overlook / wall-top / structure edge
- Tier -1: pit, trench, lower route, water edge

Agents should avoid arbitrary slope sculpting unless the biome explicitly needs it.

---

# 3. Core World-Building Model

We should build the world from **authored map cells**.

## 3.1 Map cell
A map cell is the smallest authored world chunk that can stand on its own.

A cell usually contains:
- one traversal idea
- one combat or puzzle idea
- 2–4 exits
- 1 focal landmark
- optional secret or return hook

### Recommended size
Use a **standard gameplay footprint** for outdoor cells.

Suggested starting target:
- approximately **1 camera-readable micro-space per cell**
- roughly the amount of content a player can visually parse without moving the camera

Do not define cells by art dimensions first.
Define them by **readability footprint**.

---

## 3.2 Room graph, not open sprawl
The full overworld should be authored like a graph of connected cells.

Node types:
- Path
- Arena
- Gate
- Vista
- Secret
- Service
- DungeonEntrance
- ShortcutReturn
- Transition

### Rule
When agents create or modify world layout, they must update the **room graph data**, not just place geometry.

This is critical for:
- progression logic
- route planning
- test generation
- content validation
- AI-assisted editing

---

## 3.3 Metroidvania gating inside compact cells
The world docs already establish town-driven route unlocks and metroidvania return structure.

Map design should therefore assume each biome contains:
- primary route
- visible future route(s)
- shortcut unlocks
- town-upgrade gates
- optional side pockets

### Gate examples
- explosives wall
- magnetic device rail/object
- invisibility-required bypass
- phase barrier
- glider gap

The gate should be legible even before it is usable.

---

# 4. UE5 Technical Representation

## 4.1 Recommended map stack
Each playable region should be composed from the following layers:

1. **World Partition / persistent world shell**
2. **Biome region map(s)**
3. **Cell sublevels or level instances**
4. **Gameplay markup layer**
5. **Set dressing layer**
6. **Encounter layer**
7. **Gate / secret / scripting layer**

### Practical recommendation
For the current scope, favor **Level Instances** or a similarly modular composition approach for reusable authored cells.

Why:
- clean ownership boundaries
- easy for agents to modify isolated spaces
- safer diffs
- easier validation
- repeatable biome kit workflows

---

## 4.2 Separate gameplay shape from art mesh
Every cell should have two different representations:

### A) Gameplay geometry
Simple, deliberate, low-ambiguity collision and navigation surfaces.

### B) Visual geometry
Stylized walls, rocks, foliage, props, trims, decals, clutter.

### Rule
Agents must never treat decorative mesh placement as the source of truth for:
- walkability
- cover
- ledge logic
- jump rules
- projectile blocking
- path width

The source of truth must be gameplay markup / blockout geometry.

---

## 4.3 Grid-assisted, not grid-prison
The target look is handmade, but implementation should still use a **snap-friendly structural grid** for consistency.

Recommended use of grid:
- floor plate alignment
- wall segment placement
- ledge height tiers
- doorway widths
- collision volumes
- gate markers

Allowed to break grid slightly for:
- natural rock edges
- organic foliage silhouettes
- shoreline variation
- prop dressing

### Rule
Gameplay footprint should remain standardized even when visuals feel organic.

---

## 4.4 Height tier system
Define a small explicit height vocabulary in data.

Example:
- `HeightTier_0_Ground`
- `HeightTier_1_Ledge`
- `HeightTier_2_WallTop`
- `HeightTier_Minus1_Lower`

Each cell should declare the tiers it uses.

Why this matters:
- consistent climb/fall rules
- clean nav reasoning
- easier agent edits
- predictable camera framing
- simpler combat readability

---

# 5. Gameplay Markup Layer (Most Important for Agents)

The map must include a **formal markup layer** that can be edited and validated independent of the art.

## 5.1 Required markup actors / data assets
Each cell should support explicit placement of the following gameplay markup:

- `PM_WalkableArea`
- `PM_Wall`
- `PM_Ledge`
- `PM_Drop`
- `PM_ClimbEdge`
- `PM_JumpGap`
- `PM_Hazard`
- `PM_TallGrass`
- `PM_ShallowWater`
- `PM_DeepWater`
- `PM_SecretHint`
- `PM_Breakable`
- `PM_Gate`
- `PM_Interact`
- `PM_SpawnGroup`
- `PM_PatrolPath`
- `PM_LootAnchor`
- `PM_CameraInterest`
- `PM_ReturnShortcut`

These can be lightweight actors, components, or data-driven marker instances.

### Rule
Gameplay markup must be queryable by tools and tests.

That means agents should be able to answer questions like:
- Which cells contain explosive gates?
- Which path pockets have no landmark?
- Which arenas have no spawn group?
- Which exits are one-way drops?
- Which cells are missing a secret or return hook?

---

## 5.2 Cell metadata schema
Every cell should have a metadata asset.

Minimum fields:
- `CellId`
- `BiomeId`
- `CellType`
- `ThemeTags`
- `HeightTiersUsed`
- `Exits`
- `PrimaryPathRole`
- `GateRequirements`
- `ContainsSecret`
- `ContainsCombat`
- `ContainsPuzzle`
- `EncounterBudget`
- `LandmarkType`
- `RevisitValue`
- `Dependencies`
- `TestChecklist`

This metadata becomes the contract between:
- design agents
- level-building agents
- encounter agents
- validation agents
- world-graph agents

---

# 6. Kit-Based Environment Construction

## 6.1 Build biome kits, not one-off maps
Each biome should have a modular kit containing:

- floor plates
- wall edges
- corner pieces
- ledge caps
- cliff faces
- path trims
- bridge pieces
- blockers
- foliage clusters
- landmark props
- gate props
- secret props

### Rule
The art kit must be designed around **recombining authored gameplay cells**, not around freeform scenic assembly.

---

## 6.2 Kit categories
Split the kit into these categories:

### Structural kit
Defines readable gameplay boundaries.
- cliffs
- walls
- ledges
- stairs/ramps
- pits
- bridge modules

### Traversal kit
Communicates what the player can do.
- climb markers
- jump edges
- bombable panels
- magnetic anchors
- glide launch points
- phase barriers

### Readability kit
Makes routes obvious.
- path borders
- signposts
- torch lines
- flowers / shrubs / stones used as path framing
- contrast decals

### Dressing kit
Adds life without harming readability.
- bushes
- crates
- ruins
- carts
- fences
- ambient props

### Landmark kit
Makes each pocket memorable.
- shrine
- statue
- giant tree root
- broken automaton
- crystal outcrop
- tower fragment

---

# 7. Encounter-Friendly Map Rules

The combat docs emphasize readable attacks, jump-avoidance rules, narrow thrust lanes, and AoE positioning. Map design must support that. Sweeps must be readable, thrust lanes must exist, and combat pockets must have meaningful dodge/jump geometry. See `attack_taxonomy_v1` for the combat shape rules.

## 7.1 Floor clarity for combat
Combat pockets must preserve:
- clean dodge lanes
- readable enemy approach lines
- enough width for sweep attacks to matter
- enough narrowness in some spaces for thrusts/slams to shine

### Rule
Never over-dress the center of combat space.
Clutter belongs at edges unless it is intentional gameplay clutter.

---

## 7.2 Intentional pocket archetypes
Standardize a few reusable combat-space archetypes.

Recommended launch set:
- `OpenPocket`
- `LanePocket`
- `RingPocket`
- `CrossroadsPocket`
- `AmbushPocket`
- `HazardPocket`
- `ElevationPocket`
- `BossAntechamber`

Agents should assemble cells from known archetypes instead of inventing new layouts every time.

---

## 7.3 Enemy placement principles
- place enemies where the player can read them early
- preserve reaction space at entrances
- do not hide critical melee enemies in unreadable foliage
- use brush/cover for optional stealth, not cheap surprise spam
- use height or blockers to create tactical identity, not confusion

---

# 8. Camera-Conscious Level Design

The locked camera is a major technical constraint and should become a design advantage.

## 8.1 Author for the fixed viewing angle
Each cell should be tested from the real gameplay camera, not a free editor camera.

### Rule
A space is not approved until it is readable from:
- player entry
- center of pocket
- expected combat movement
- exit discovery angle

---

## 8.2 Occlusion discipline
Avoid placing tall decorative assets where they:
- hide enemies unfairly
- cover interactables
- obscure gate affordances
- block projectile readability

Use tall foreground elements sparingly and intentionally.

---

## 8.3 Framing landmarks
Because the camera does not rotate, landmarks must be placed where they naturally sit inside the fixed composition.

Recommended uses:
- exit framing
- quest destination hinting
- biome identity
- return-route memory anchors

---

# 9. Secrets, Shortcuts, and Revisit Design

## 9.1 Every region needs return value
A metroidvania map should reward memory.

Each region should include some combination of:
- visible unreachable chest
- suspicious cracked wall
- shortcut door opened later
- cliff ledge seen from below first
- alternate route hidden by foliage/water/pathing trick

---

## 9.2 Secret readability rule
Secrets should be hidden, not arbitrary.

Use readable hints:
- odd wall seam
- unusual prop arrangement
- suspicious dead end
- landmark alignment
- environmental repetition break

---

## 9.3 Shortcut pattern rule
For every major route branch, prefer this rhythm:
- long forward push
- challenge / pocket / gate
- reward or reveal
- unlock return shortcut

This keeps exploration satisfying and reduces tedious backtracking.

---

# 10. Agentic Development Workflow

This section is the most important implementation guidance.

## 10.1 Source-of-truth hierarchy
Agents need a strict order of truth.

### Source of truth priority
1. **Cell metadata asset**
2. **Gameplay markup layer**
3. **Blockout geometry**
4. **Encounter data**
5. **Art dressing**

If art conflicts with gameplay markup, gameplay markup wins.

---

## 10.2 Agent roles
Recommended world-building agent split:

### World Architect Agent
Owns:
- biome graph
- route planning
- gate placement
- revisit logic
- macro progression

### Cell Designer Agent
Owns:
- cell purpose
- exits
- pocket composition
- landmark selection
- secret placement

### Blockout Builder Agent
Owns:
- gameplay geometry
- wall/floor/ledge shapes
- traversal widths
- collision volumes

### Markup Agent
Owns:
- gate markers
- hazards
- climb edges
- interact anchors
- spawn groups
- metadata correctness

### Encounter Agent
Owns:
- enemy compositions
- patrols
- arena pressure
- reward placement

### Dressing Agent
Owns:
- foliage
- props
- decals
- micro-storytelling
- readability-safe polish

### Validation Agent
Owns:
- naming
- metadata completeness
- exit connectivity
- gate consistency
- pathing checks
- camera/readability checks

---

## 10.3 Required agent contract per cell
Before a cell can be considered complete, the following files/assets should exist:

- `DA_Cell_<CellId>` metadata asset
- `LI_Cell_<CellId>` level instance
- `DA_Encounter_<CellId>` encounter asset if combat exists
- `DA_Loot_<CellId>` loot anchors if needed
- validation report output
- screenshot pack from gameplay camera

### Rule
No agent should "just build a room in the editor" without the metadata asset.

---

## 10.4 Validation checklist automation
Build commandlets or editor utility validation for:

- missing metadata fields
- invalid exit links
- gate requirement mismatch
- unreachable anchors
- overlapping blockers
- hidden interactables from gameplay camera
- missing landmark in required cell types
- combat pocket with insufficient movement width
- secret cell with no hint marker
- return shortcut declared but not instantiated

These checks are ideal for agent loops because they produce deterministic feedback.

---

## 10.5 Safe edit zones
To reduce agent collisions in shared levels:

- each cell has a single owner at a time
- shared biome graph edits happen through metadata, not ad hoc geometry edits
- cross-cell boundaries use standardized connection sockets
- cell exits snap to approved dimensions

---

# 11. Recommended Folder / Asset Structure

```text
/Content/World
  /Biomes
    /VerdantWilds
      /Cells
      /Kits
      /Encounters
      /Data
      /Validation
    /SearingExpanse
    /MireOfEchoes
    /CrystalCaverns
    /StormPeaks
    /WitheredKingdom

/Content/WorldShared
  /CellFramework
  /TraversalMarkers
  /GateMarkers
  /Landmarks
  /Validation
```

Recommended naming:
- `DA_Cell_VW_01`
- `LI_Cell_VW_01`
- `DA_Encounter_VW_01`
- `PM_Gate_Explosives_A`
- `PM_ReturnShortcut_VW_01_A`

---

# 12. Initial Technical Milestones

## Milestone 1 — Cell Framework
Build:
- cell metadata asset type
- gameplay markup actors
- validation pass
- connection socket standard
- blockout template cell

## Milestone 2 — One Biome Vertical Slice
Build one small biome strip with:
- 6–10 cells
- one gate type
- one shortcut
- one secret
- two combat pocket archetypes
- one landmark cell

## Milestone 3 — Reusable Kit
Create first modular biome kit with:
- structural pieces
- traversal markers
- dressing set
- landmark props

## Milestone 4 — Agent Loop Enablement
Create tools for:
- generate cell from template
- validate cell metadata
- screenshot cell from gameplay camera
- produce missing-elements report

---

# 13. Definition of Done for a Map Cell

A map cell is done when:

- purpose is clear in metadata
- exits are linked and validated
- gameplay geometry is readable
- landmark exists if required
- combat pocket supports intended enemy behavior
- secrets have readable hints
- gates are legible and tagged
- room reads correctly from real gameplay camera
- dressing does not compromise readability
- validation passes cleanly

---

# 14. Non-Goals

Do not optimize for:
- giant realistic landscapes
- procedural open-world sprawl
- hyper-detailed terrain sculpting as the primary workflow
- freeform art-first placement with gameplay retrofitted later
- visually noisy scenes that obscure combat reads

---

# 15. Final Principle

The Link's Awakening-inspired target is successful when the world feels:

- small but adventurous
- simple but rich
- readable but secretive
- toy-like but dangerous
- handcrafted at every step

Technically, that means:

> **Build the world as a graph of authored, metadata-driven, camera-readable gameplay cells — then let art and polish sit on top of that structure.**
