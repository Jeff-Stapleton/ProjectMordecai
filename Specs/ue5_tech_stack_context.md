# UE5 Multiplayer Action RPG — Implementation Stack Context (Locked Decisions)

Date: 2026-03-03  
Purpose: Provide concise, implementation-oriented context for AI developer agents building a multiplayer action RPG in Unreal Engine 5.

## Game goals (high level)

We are building a **multiplayer, action RPG** with:
- **Deep player stats**
- **Wide array of skills/abilities**
- **Many buffs/debuffs**
- **Items that augment/modify stats, skills, buffs/debuffs**
- **1st + 3rd person support**
- **Server-authoritative multiplayer**

## Locked decision: Backend = PlayFab ✅

We are committing to **Microsoft PlayFab** as the primary backend service for:
- Player identity/auth and account profile
- Player profiles + progression/meta data (out-of-match)
- Inventory ownership and economy data (virtual items/currency if needed)
- Lobby / Matchmaking / Party (managed multiplayer services)

**Guiding rule:** PlayFab is for **persistent meta systems**. Unreal dedicated servers remain authoritative for **live match/combat state**.

---

# Core engine + gameplay foundation

## Ability System (Core) — GAS ✅

**Use Unreal Gameplay Ability System (GAS)** as the backbone for combat/math/state:
- **Attributes / Attribute Sets** for stats (Health, Mana/Stamina/Resource, Offense/Defense, Resistances, etc.)
- **Gameplay Abilities** (`UGameplayAbility`) for active + passive skills
- **Gameplay Effects** (`GameplayEffect`) for buffs/debuffs, DOT/HOT, temporary modifiers
- **Gameplay Tags** for status flags, requirements, immunities, synergy gates, proc state, CC flags, elemental flags, etc.

### Modeling principles (important)
- **Items should not implement bespoke gameplay math everywhere.**
  - Prefer items that **grant abilities**, **apply effects**, **add tags**, or **modify parameters** used by abilities/effects.
- Favor **data-driven** tuning: tags + effect specs + scalable floats/curves over special-casing.

---

# Implementation recommendations by area

## 1) User Interface ✅

Primary stack:
- **UMG** for rendering
- **Common UI** for screen flow / layered UI / input routing
- **UMG MVVM (Viewmodel)** for data-heavy UI binding
- **Enhanced Input** for action mapping + remapping

Implementation notes:
- Use Common UI for: main menu, character select, inventory, skill tree, overlays/pause menu, modal flows.
- Use MVVM for: character sheet, buff/debuff tray, party frames, cooldown bars, item tooltips & compare panels.
- Keep UI “thin”: UI reads replicated gameplay state + profile/meta state; UI does not own gameplay rules.

## 2) Ability system ✅ (GAS details)

Core objects:
- Attribute Sets (C++): `UAttributeSet_*`
- Abilities: `UGameplayAbility_*`
- Effects: `UGameplayEffect` assets
- Tags: central tag dictionary + conventions (e.g., `Status.*`, `Damage.*`, `Immune.*`, `Weapon.*`, `Skill.*`)

Patterns to standardize early:
- Cooldowns & costs always via Gameplay Effects
- Buff/debuff stacking rules standardized (stack count, duration refresh, magnitude stacking, unique tags)
- Status effects represented by tags + effects, not replicated booleans
- Damage pipeline: use GameplayEffectExecutionCalculation / modifiers

## 3) Player character with 1st & 3rd person support ✅

Approach:
- Use standard **`ACharacter` + `CharacterMovementComponent`** first.
- Treat 1st/3rd person as **camera/presentation modes**, not separate gameplay implementations.

Implementation notes:
- Build a small “camera mode” abstraction:
  - 3rd-person over-shoulder
  - combat zoom or top-down-ish option (optional)
  - true first-person (optional)
- Combat rules (targeting, hit validation, ability activation) must be camera-agnostic.
- Avoid adopting experimental systems early (e.g., Mover / Gameplay Camera System) unless needed.

## 4) Database / Persistence ✅ (PlayFab)

**What belongs in PlayFab:**
- Account + auth-linked player identity
- Character roster
- Profile/progression data (XP, levels, skill unlocks)
- Inventory ownership, currencies, cosmetics
- Saved builds/loadouts
- Out-of-match economy and progression state
- Social meta (if used)

**What does NOT belong in PlayFab:**
- Real-time combat state while a match is running
- Authoritative damage, buffs, or movement outcomes

Recommended split:
- **Unreal Dedicated Server:** authoritative match + combat state
- **PlayFab backend:** persistent meta state + matchmaking + lobby/party

## 5) Multiplayer networking ✅

Baseline:
- **Dedicated servers**
- **Server-authoritative combat** and GAS outcome resolution
- Clients may predict *feel* (animations, local responsiveness) but server decides outcomes.

Scaling options (choose later if needed):
- Start with standard Unreal replication.
- If actor counts / relevancy scaling becomes a pain: adopt **Replication Graph**.
- Avoid **Iris** initially unless there’s a clear need and willingness to adopt experimental tech.
  - Note: Iris and Replication Graph are mutually exclusive on the same net driver.

Netcode principles:
- Replicate *state*, not *derivations*.
- Prefer tags/effects for statuses rather than lots of bespoke replicated variables.
- Be deliberate about what replicates and at what frequency.

## 6) Player profiles ✅

In-engine glue:
- Use **Common User** (Lyra pattern) for a stable user abstraction (local user, login flow, online integration).

Persistent profile storage:
- Use **PlayFab** for long-term profile state.

## 7) Matchmaking ✅ (PlayFab)

Use PlayFab’s multiplayer services:
- Lobby / Party (pre-match grouping, invites, roster)
- Matchmaking (queueing and match formation)
- Session handoff to Unreal dedicated servers

---

# Architecture & reference project

## Lyra as reference ✅

Use **Lyra Sample Game** as an architectural reference (patterns, plugin usage, Common UI/Common User, modular setup).
- Do not treat Lyra as a drop-in “our game,” but as a well-tested blueprint for modern UE project structure.

---

# Agentic development hooks (optional but recommended)

## Editor automation (to accelerate content creation)

- Use **Python Editor Scripting** for editor-time automation:
  - bulk asset creation, generation of Blueprint assets, batch edits, import pipelines
- Use **Editor Utility Widgets/Blueprints** for in-editor tool UX

Important: Python is editor-only (not shipped runtime gameplay scripting).

## Testing stack (for agent-driven TDD)

Recommended testing pyramid:
1. **Low-Level Tests** for fast TDD-style checks
2. **Automation Test Framework / Specs** for integration/feature checks
3. **Functional Tests** for level/gameplay scenario validation
4. **Automation Driver** for UI interaction simulation
5. **Gauntlet** for end-to-end session-based validation (packaged builds, multiplayer, device runs)

## Horde (scale option)

If/when we need studio-scale build/test operations:
- Consider **Horde** for distributed build + CI + test result aggregation (especially for Gauntlet runs).
- Not required for early development, but a scaling path.

---

# Practical implementation “north stars”

- **GAS owns gameplay math and state transitions.**
- **Gameplay Tags are the vocabulary** of statuses, gates, immunities, and synergy.
- **Effects are the source of truth** for buffs/debuffs and timed modifiers.
- **Items compose gameplay** by granting abilities/effects/tags and parameter modifiers.
- **Dedicated server is authoritative** for all meaningful outcomes.
- **PlayFab owns persistence + matchmaking**, not in-match combat state.
- Keep systems **data-driven** and avoid special-case sprawl.

---

# Open items (non-blocking)

- Exact camera mode UX and how/when we allow switching in-match
- Inventory schema details (stacking, rarity rolls, affix system)
- BuildGraph/CI plan and when to introduce Horde
- Replication scaling: standard replication vs Replication Graph (decide after profiling)

