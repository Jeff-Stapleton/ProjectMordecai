# Project Mordecai — Agentic Map Production Pipeline v1

Date: 2026-04-12
Purpose: Define the production workflow, ownership model, validation strategy, and automation hooks for building Link's Awakening-style map content with AI agents in Unreal Engine 5.7.

---

# 1. Production Philosophy

For this project, AI agents should excel at:
- creating repeatable map cells
- applying consistent rules
- validating metadata and connectivity
- generating blockouts from templates
- flagging readability problems early

AI agents should **not** be trusted to freestyle the macro world or invent product rules in isolation.

Therefore the workflow should be:

1. user defines or approves design rules
2. docs define cell system + metadata schema
3. agents create constrained content inside that framework
4. validators reject broken or unreadable work
5. humans review only the highest-value decisions

---

# 2. Pipeline Stages

## Stage A — World Intent
Human/user or high-level design agent defines:
- biome purpose
- gating sequence
- emotional arc
- landmark set
- route topology

Output:
- biome graph
- gate dependency chart
- landmark plan

---

## Stage B — Cell Planning
Cell designer agent produces a plan for each cell:
- cell type
- exits
- encounter intent
- secret intent
- landmark intent
- traversal notes

Output:
- `DA_Cell_*` assets or equivalent source definitions

---

## Stage C — Gameplay Blockout
Blockout agent builds:
- floor shape
- walls
- ledges
- drops
- gaps
- basic collision

Output:
- playable greybox cell

---

## Stage D — Gameplay Markup
Markup agent adds:
- gate markers
- secrets
- spawn groups
- interact anchors
- hazards
- camera interest markers

Output:
- testable gameplay-ready cell

---

## Stage E — Encounter and Loot Pass
Encounter agent adds:
- enemy sets
- patrols
- wave or ambush logic if relevant
- loot anchors
- reward tags

Output:
- mechanically complete cell

---

## Stage F — Dressing Pass
Dressing agent adds:
- foliage
- props
- decals
- biome flavor
- micro-storytelling

Output:
- visually readable polished cell

---

## Stage G — Validation
Validation agent runs automated checks and produces:
- pass/fail summary
- missing requirements
- camera readability warnings
- connectivity warnings
- naming violations

---

# 3. Cell Templates

Agents work better from templates than blank canvases.

Recommended default templates:
- PathCell
- CombatPocketCell
- SecretCell
- GateCell
- LandmarkCell
- ShortcutCell
- TransitionCell
- DungeonEntryCell

Each template should pre-populate:
- metadata fields
- default markup anchors
- connection sockets
- screenshot test points
- validation checklist

---

# 4. Asset Contracts

## Required per-cell artifacts
- metadata asset
- level instance / map asset
- encounter definition if combat exists
- validation result
- camera screenshots

## Nice-to-have generated artifacts
- minimap preview
- top-down blockout preview
- graph node preview
- dependency summary

---

# 5. Validation Rules That Matter Most

Priority order:

## Critical
- exit connectivity valid
- no unreachable required path
- gate tags valid
- no blocked spawn inside collision
- required encounter fits play space

## High
- landmark present when required
- secret has hint
- camera view does not hide critical interactables
- path width supports intended combat type

## Medium
- naming conventions pass
- dressing budget within biome target
- clutter density acceptable

---

# 6. Recommended Automation Hooks

## Editor utilities
Create utilities for:
- new cell from template
- bulk validate selected cells
- snapshot gameplay camera views
- show missing markup overlays
- report all gates by requirement type

## Commandlets / scripts
Create local scripts for:
- validate world graph
- validate cell metadata
- generate CSV/JSON reports
- export review image sheets

This aligns with the project rule to keep build/test workflows local-only for now.

---

# 7. Review Loop for Agents

Preferred loop:

1. agent creates or modifies one cell
2. validator runs
3. screenshot generator runs
4. reviewer agent summarizes problems
5. fix agent resolves only flagged problems
6. final human review only if cell impacts progression or camera readability

This keeps the loop deterministic and cheap.

---

# 8. Human Approval Gates

Require explicit approval for:
- new biome graph changes
- new gate type introduction
- changes to height tier rules
- changes to connection socket standards
- changes that affect progression sequence

Let agents act autonomously for:
- blockout refinement
- dressing polish
- metadata completion
- local validation fixes
- anchor placement adjustments

---

# 9. Suggested First Slice

Build a small test biome strip with:
- 1 entry cell
- 2 path cells
- 2 combat pockets
- 1 gate cell
- 1 secret cell
- 1 return shortcut
- 1 landmark cell

Goal:
prove the cell pipeline, not just the art style.

---

# 10. Final Principle

A good agentic map workflow is not just "AI builds rooms."
It is:

> **AI agents operating inside a strict cell framework with strong metadata, automated validation, and camera-aware review.**
