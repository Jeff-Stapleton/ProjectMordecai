# agent_rules_v2.md — UE5 Tech Stack (Project Mordecai)

These rules are **authoritative**. If a tradeoff exists, follow this doc unless the user explicitly overrides it.

---

## 0) Engine Version

### DO
- Target **Unreal Engine 5.7** for all implementation decisions.
- Prefer UE 5.7-native APIs and recommended patterns.

### DO NOT
- Introduce version-specific workarounds for earlier UE versions unless explicitly requested.

### WHEN UNSURE
- Add a short comment: `// UE5.7: verify API name/signature if it differs from 5.5/5.6`

---

## 1) Camera & Controls (Locked)

### Camera Style
- **3rd-person diorama perspective**
- Camera **follows the character**
- Camera **does not rotate** (no free-look; no yaw/pitch driven by player input)

### DO
- Implement movement/aiming/targeting assuming a fixed-orientation camera.
- Ensure interaction selection works without camera rotation (e.g., auto-target, cone checks, closest-in-front logic, or explicit target cycling).

### DO NOT
- Add free-look, orbit camera, shoulder swap, right-stick rotation, or “hold-to-rotate camera.”
- Depend on camera yaw to define movement direction unless clearly documented and consistent.

### ALLOWED
- Screen shake / camera effects that do **not** change camera facing (orientation).
- Minor positional smoothing/lag to follow the character (no rotation).

---

## 2) Inventory & Items (Unlimited Carry / Town Gating) (Locked)

### Core Model
- Player inventory is **effectively unlimited** for normal adventuring.
- Inventory management is **not** intended to be a difficulty source.
- Players should be able to **pick up and keep everything** that is reasonably collectible during expeditions.

### Canonical Loot Friction
Loot pressure should come from **town-facing systems**, not bag space.

The primary return-to-town forcing functions are:
1. **Magical item identification** at the **Mage Tower**
2. **Healing potion stock limits** governed by the **Alchemist**
3. Delivering **materials / town resources / upgrade keys** into long-term progression loops

### Material Rule
- Most non-gear loot should be classified as **Material** or **TownResource**.
- Materials and town resources should be **auto-stored** into a shared progression inventory / resource ledger.
- Players should **not** be asked to drop useful monster parts, herbs, ore, hides, or upgrade materials because of inventory limits.

### Magical Item Rule
- Magical items may drop in an **Unidentified** state.
- Unidentified magical items cannot be fully used until processed by the **Mage Tower**.
- Identification is an intentional pacing mechanism and replaces “bag full” as a major looting constraint.

### Consumable Rule
- Consumable carry logic should support readable limits where needed, but **healing scarcity should primarily come from village production / stock**, not inventory capacity.
- The Alchemist determines how many health potions the player can reliably take into the field at a time.

### DO
- Implement inventory as a **flat, player-friendly carry model** with strong sorting/filtering.
- Support categories such as:
  - `Weapon`
  - `Armor`
  - `Trinket`
  - `Consumable`
  - `Material`
  - `TownResource`
  - `UpgradeKey`
  - `QuestItem`
  - `MagicalItem`
- Auto-route materials and town resources into persistent storage.
- Surface unidentified magical items clearly in UI and tooltips.
- Preserve strong organization, filtering, and search so unlimited carry remains readable.

### DO NOT
- Implement bag slots, container tetris, or specialized bags as a core system.
- Force players to discard strategically important materials due to lack of space.
- Use inventory capacity as the primary limiter on healing, crafting, or town progression.

### DEFAULTS (until user decides otherwise)
- Materials auto-store immediately on pickup.
- Town resources auto-store immediately on pickup.
- Magical items are individually tracked and may require identification.
- Sorting should prioritize: quest items, upgrade keys, magical items, then common gear.

---

## 3) Build, CI, and Testing (Local-Only for Now) (Locked)

### DO
- Keep build + test workflows **local**:
  - simple scripts (bat/sh)
  - basic commandlets
  - local automation tests that run without infrastructure

### DO NOT
- Introduce BuildGraph
- Introduce Horde
- Build distributed CI/test pipelines
- Add heavyweight infra dependencies for building/testing

### GOOD PRACTICE
- Keep scripts in a predictable location (e.g., `/BuildScripts` or `/Scripts`) and document usage in `README.md`.

### PIE SMOKE TESTS (Locked)
Headless tests (NullRHI) cannot catch visual or runtime integration issues. PIE smoke tests **must** be run for **every story, regardless of Execution Mode** — no exceptions.

- Run `py Scripts/run_pie_verify.py` after headless tests pass.
- If you create a new feature with runtime/visual behavior, **add a `Scripts/verify_pie_<feature>.py` smoke test** that verifies it works during PIE.
- Smoke tests query live game state via Python remote execution — they check widget visibility, spawn chain integrity, component existence, etc.
- A story is not done if PIE smoke tests fail.

---

## 4) Networking Replication: Iris (Locked)

### DO
- Use **Iris replication**.
- Design gameplay as **server-authoritative**.
- Optimize replication via Iris-compatible mechanisms:
  - replicated state minimization
  - update frequency control
  - relevance/scoping mechanisms supported by Iris
  - explicit replication fragments / state descriptors (as appropriate)

### DO NOT
- Implement or propose Replication Graph as the scaling strategy.
- Build systems that require RepGraph routing semantics.

---

## 5) Persistence Boundary (PlayFab Default)

### DO
- Use PlayFab for **persistent meta**:
  - identity/auth
  - profiles
  - account-level inventory/economy meta
  - matchmaking/lobby/party (as applicable)
- Keep **in-match** combat state in UE server runtime.

### DO NOT
- Add a second custom persistence database unless explicitly requested.

### NOTE
- Exact write timing for loot/xp/inventory deltas is still an open decision (see Open Items).

---

## 6) GAS Defaults (Applies Everywhere)

### DO
- Use GAS for:
  - Attributes
  - Abilities
  - Status effects (buffs/debuffs)
  - Gameplay Tags as shared vocabulary
- Prefer data-driven configuration (DataAssets / tables) over hardcoding.

### DO NOT
- Create parallel “status effect systems” outside GAS unless explicitly directed.

---

## 8) Map & World Building (Locked)

### Core Model
- The world is built from **authored map cells**, not open terrain or freeform scenic assembly.
- A cell is the smallest authored world chunk that stands on its own (one traversal idea, one combat/puzzle idea, 2–4 exits, optional landmark/secret).
- The overworld is a **room graph of connected cells**, not open sprawl.
- See `map_style_technical_design_v1.md` for full spatial rules and `agentic_map_pipeline_v1.md` for the production pipeline.

### Source-of-Truth Hierarchy
When editing world content, agents must respect this priority (highest first):
1. **Cell metadata asset** (`DA_Cell_*`)
2. **Gameplay markup layer** (`PM_*` actors)
3. **Blockout geometry**
4. **Encounter data**
5. **Art dressing**

If art conflicts with gameplay markup, **gameplay markup wins**.

### DO
- Create a `DA_Cell_<CellId>` metadata asset before placing any geometry for a new cell.
- Use the gameplay markup layer (`PM_WalkableArea`, `PM_Wall`, `PM_Gate`, etc.) as the source of truth for walkability, collision, ledges, and cover.
- Use **clear height tiers** (`Tier 0: Ground`, `Tier 1: Ledge`, `Tier 2: WallTop`, `Tier -1: Lower`) — each cell declares which tiers it uses.
- Follow the 7-stage pipeline: World Intent → Cell Planning → Blockout → Markup → Encounter → Dressing → Validation.
- Use standardized combat pocket archetypes (`OpenPocket`, `LanePocket`, `RingPocket`, `CrossroadsPocket`, `AmbushPocket`, `HazardPocket`, `ElevationPocket`, `BossAntechamber`).
- Keep combat pocket centers **clear** — clutter goes at edges.
- Use snap-friendly structural grids for gameplay footprints; allow organic variation only for visual dressing.

### DO NOT
- Build a room or cell without the metadata asset first.
- Treat decorative mesh placement as the source of truth for walkability, cover, ledges, jump rules, or path width.
- Build large vague fields with no authored purpose — every space needs a clear identity.
- Use arbitrary slope sculpting unless the biome explicitly requires it.
- Place tall decorative assets where they hide enemies, cover interactables, or obscure gates from the fixed camera.

### Human Approval Required
Agents must **not** autonomously change:
- biome graph structure
- gate type introduction
- height tier rules
- connection socket standards
- progression sequence

Agents **may** act autonomously for:
- blockout refinement within a cell
- dressing polish
- metadata completion
- local validation fixes
- anchor placement adjustments

### Naming Conventions (World Assets)
- Cell metadata: `DA_Cell_<BiomeAbbrev>_<Number>` (e.g., `DA_Cell_VW_01`)
- Level instance: `LI_Cell_<BiomeAbbrev>_<Number>`
- Encounter data: `DA_Encounter_<BiomeAbbrev>_<Number>`
- Gameplay markup: `PM_<Type>_<Variant>` (e.g., `PM_Gate_Explosives_A`)

### Content Folder Structure
```
/Content/World/Biomes/<BiomeName>/Cells/
/Content/World/Biomes/<BiomeName>/Kits/
/Content/World/Biomes/<BiomeName>/Encounters/
/Content/World/Biomes/<BiomeName>/Data/
/Content/World/Biomes/<BiomeName>/Validation/
/Content/WorldShared/CellFramework/
/Content/WorldShared/TraversalMarkers/
/Content/WorldShared/GateMarkers/
/Content/WorldShared/Landmarks/
/Content/WorldShared/Validation/
```

---

## Open Items — Do Not Guess (Scaffold Only)

Agents may create interfaces and TODOs, but must **not finalize behavior** without user input:

1) **Gameplay tag taxonomy & naming rules**
   - namespaces, ownership, review/approval flow

2) **Buff/debuff stacking policy**
   - refresh vs stack behavior
   - unique tags
   - magnitude stacking rules

3) **PlayFab ownership details**
   - authoritative inventory mutation path
   - match end writeback timing
   - loot/xp grant flow

4) **Unidentified item flow details**
   - whether appraisal can happen per-item, batch, or via time-based town process
   - whether identification has a gold/resource/service cost
   - whether unidentified items expose partial hints before identification

5) **Potion field-cap rule exactness**
   - whether potion carry is a hard cap, soft stock cap, or refill-style provisioning rule

---

## 7) Persistence of Changes (Locked)

### DO
- Always make changes **permanent** — write to config files, save assets to disk, or put settings in C++ code.
- When modifying editor settings or blueprint assets via Python scripts, always call the appropriate save method (`EditorAssetLibrary.save_asset`, config file writes, etc.).
- If a change cannot be persisted (e.g., `save_config()` not exposed to Python), immediately implement an alternative permanent solution (C++ code, direct config file edit, etc.).

### DO NOT
- Leave changes as **in-memory-only** fixes that revert on editor/tool restart.
- Rely on Python CDO hacks or runtime-only settings that won't survive a session restart.
- Declare a fix "done" if it only works until the next restart.

---

## Implementation Behavior When Requirements Are Missing

### DO
- Add explicit `TODO(DECISION)` markers and keep the design flexible.
- Provide 1–2 minimal options in comments if needed, but do not choose.

### DO NOT
- Invent product rules or “pick what seems best” for open items.

---

## Quick Compliance Checklist (for PRs / agent output)

- [ ] UE version assumed: 5.7
- [ ] Camera follows character and does not rotate
- [ ] Inventory uses unlimited carry + auto-stored materials + town gating
- [ ] No bag slot system or specialized bag restrictions
- [ ] Build/test remains local-only (no BuildGraph/Horde)
- [ ] Replication uses Iris (no RepGraph)
- [ ] Persistence respects PlayFab boundary
- [ ] GAS used for abilities/attributes/effects/tags
- [ ] Open items left as TODO(DECISION), not guessed
- [ ] Map cells have metadata asset before geometry
- [ ] Gameplay markup is source of truth for collision/walkability (not art meshes)
- [ ] Height tiers used (not arbitrary slopes)
- [ ] Biome graph / gate / progression changes approved by human
