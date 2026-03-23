# US-011: Skill Framework & Rank Progression

## Overview
Build the data infrastructure and runtime systems for the 50-skill rank progression system. Each skill levels from Rank 1 to 20, with milestone unlocks at Ranks 1, 5, 10, 15, and 20. This story implements the **framework only** — the data model, rank storage, skill point allocation, and milestone query/grant system. Actual skill milestone implementations (weapon techniques, spell upgrades, etc.) are deferred to their respective epics (Epic 5: Magic, Epic 6: Weapons).

The skill system is a core progression vector per game_design_v2.md: "Skill-first combat — mastery beats stats." Skills are the primary power driver, not levels or attributes.

## Execution Mode
**Mode:** HEADLESS

## References
- **Primary:** `skill_sheet_v1.1.md` — full 50-skill table with categories and milestone descriptions
- **Design context:** `game_design_v2.md` §5 (Progression), §2 (Pillar: "granular skills, DCC vibe")
- **Agent rules:** `agent_rules_v2.md` §6 (GAS for abilities/attributes/effects/tags)

---

## Acceptance Criteria

### Skill Data Model
- [x] AC-011.1: `UMordecaiSkillDataAsset` exists as a `UPrimaryDataAsset` with fields: `SkillName` (FName), `Category` (enum), `RankDescriptions` (TMap<int32, FText> for milestones at 1/5/10/15/20), `MilestoneAbilities` (TMap<int32, TSubclassOf<UGameplayAbility>> for abilities granted at each milestone rank)
- [x] AC-011.2: `EMordecaiSkillCategory` enum defines 5 categories: `Weapon`, `DefenseArmor`, `MovementExploration`, `UtilityInteraction`, `Magic`
- [x] AC-011.3: Skill data assets can be loaded and queried at runtime — given a skill ID, retrieve its category, current rank description, and milestone data

### Skill Tracking Component
- [x] AC-011.4: `UMordecaiSkillComponent` (ActorComponent on PlayerState) stores per-character skill ranks as a TMap<FName, int32> (skill name → current rank, 0 = not learned)
- [x] AC-011.5: Skill ranks are clamped to [0, 20] — cannot exceed max rank or go below 0
- [x] AC-011.6: Skill ranks are replicated to clients
- [x] AC-011.7: `GetSkillRank(FName SkillName)` returns the current rank (0 if not learned)
- [x] AC-011.8: `GetAllSkillsByCategory(EMordecaiSkillCategory)` returns all skills in that category with their current ranks

### Skill Point Allocation
- [x] AC-011.9: `AvailableSkillPoints` attribute tracks unspent points (replicated, clamped ≥ 0)
- [x] AC-011.10: `TryAllocateSkillPoint(FName SkillName)` succeeds if: skill exists, current rank < 20, available points > 0. On success: decrements available points, increments skill rank by 1, returns true
- [x] AC-011.11: `TryAllocateSkillPoint` fails gracefully (returns false, no state change) if: skill doesn't exist, already max rank, or no points available
- [x] AC-011.12: Allocating a point to a skill at Rank 0 sets it to Rank 1 (learning a new skill costs 1 point)

### Milestone System
- [x] AC-011.13: When a skill reaches a milestone rank (1, 5, 10, 15, 20), the system fires a `Mordecai.Event.SkillMilestone` gameplay event with the skill name and rank as payload
- [x] AC-011.14: When a skill reaches a milestone rank and the skill data asset specifies a `MilestoneAbility` for that rank, the ability is automatically granted to the character's ASC
- [x] AC-011.15: When a skill rank is set (e.g., by a debug command), milestone abilities for all ranks ≤ current rank are granted, and abilities for ranks > current rank are removed
- [x] AC-011.16: Milestone gameplay tags (`Mordecai.Skill.<SkillName>.Rank<N>`) are applied when a skill reaches that rank and removed if rank drops below

### Sample Skill Validation
- [x] AC-011.17: At least one sample `UMordecaiSkillDataAsset` (Longswords) can be created in code with placeholder milestone data for ranks 1, 5, 10, 15, 20 — validates the full pipeline from data → allocation → milestone grant

## Technical Notes

### Architecture
- **`UMordecaiSkillDataAsset`**: Extends `UPrimaryDataAsset`. One asset per skill. Contains all static data about the skill (category, descriptions, milestone mappings). The skill_sheet_v1.1.md defines 50 skills — actual DataAsset creation happens in EDITOR stories. For this HEADLESS story, validate with code-constructed test assets.
- **`UMordecaiSkillComponent`**: `UActorComponent` attached to `AMordecaiPlayerState`. Owns the mutable skill rank data. Replicates via standard UE property replication. Provides the public API for querying and modifying skills.
- **`EMordecaiSkillCategory`**: 5 categories matching skill_sheet_v1.1.md: Weapon (17 skills), Defense & Armor (8 skills), Movement & Exploration (6 skills), Utility & Interaction (4 skills), Magic (11 skills). Note: skill counts total 46 from the sheet — the remaining 4 may be added later.
- **Milestone abilities**: Each milestone rank can optionally grant a `UGameplayAbility`. The ability is given via `GiveAbility` on the ASC. When rank drops (if ever), abilities for that rank are removed via `ClearAbility`. This is the hook for weapon epics to wire in actual combat techniques.
- **Gameplay tags for milestones**: Pattern `Mordecai.Skill.Longswords.Rank5`, `Mordecai.Skill.Dodging.Rank10`, etc. Tags are added/removed via the ASC's owned tag interface. Other systems can query these tags to check if a player has reached a milestone.

### Skill Categories (from skill_sheet_v1.1.md)
| Category | Skills |
|---|---|
| Weapon | Longswords, Greatswords, Shortswords, Daggers, Axes, Maces, Spears, Quarterstaff, Unarmed, Longbow, Shortbow, Crossbows, Two-Weapon Fighting |
| Defense & Armor | Dodging, Parrying, Buckler Shield, Scutum Shield, Heater Shield, Leather Armor, Chainmail Armor, Plate Armor |
| Movement & Exploration | Sneaking, Climbing, Swimming, Perception, Tracking, Cartography |
| Utility & Interaction | Traps, Lockpicking, Pickpocket, First Aid |
| Magic | Fireball, Cone of Cold, Magic Missile, Stone Skin, Sleep, Blink, Illusion, Fire Ward, Frost Ward, Float, Counter Spell |

### What's NOT in scope
- **Actual milestone ability implementations**: This story builds the framework. Weapon milestones (e.g., "Longswords Rank 5: charged spinning attack") come in Epic 6. Magic milestones come in Epic 5.
- **Creating 50 DataAssets in the editor**: Asset creation is EDITOR work. This story validates the pipeline with code-constructed test data.
- **Skill point sources (leveling, quests)**: How players earn skill points is a progression/leveling concern. This story only provides the `AvailableSkillPoints` attribute and the allocation API.
- **Attribute prerequisites for skills**: Not defined in current design docs. If needed, add later.

### TODO(DECISION): Skill point economy
How many skill points per level? How fast do skills progress? The design says max level 60 with 50 skills at max rank 20 (= 1000 total ranks). Skill point distribution needs design input. This story just provides the infrastructure.

### TODO(DECISION): Skill rank-down
Can skill ranks decrease (e.g., from curses or respec)? AC-011.15 handles the case, but the actual trigger is undefined. Default: no rank-down mechanic, but the system supports it.

## Tests Required
- [x] `Mordecai.Skills.SkillDataAssetHasRequiredFields` — verifies AC-011.1
- [x] `Mordecai.Skills.SkillCategoryEnumMatchesDesign` — verifies AC-011.2
- [x] `Mordecai.Skills.SkillDataAssetQueryable` — verifies AC-011.3
- [x] `Mordecai.Skills.ComponentStoresSkillRanks` — verifies AC-011.4
- [x] `Mordecai.Skills.RankClampedToRange` — verifies AC-011.5
- [x] `Mordecai.Skills.SkillRanksReplicated` — verifies AC-011.6
- [x] `Mordecai.Skills.GetSkillRankReturnsZeroForUnlearned` — verifies AC-011.7
- [x] `Mordecai.Skills.GetSkillsByCategoryFiltersCorrectly` — verifies AC-011.8
- [x] `Mordecai.Skills.AvailableSkillPointsTracked` — verifies AC-011.9
- [x] `Mordecai.Skills.AllocatePointSucceedsWhenValid` — verifies AC-011.10
- [x] `Mordecai.Skills.AllocatePointFailsGracefully` — verifies AC-011.11
- [x] `Mordecai.Skills.LearningNewSkillCostsOnePoint` — verifies AC-011.12
- [x] `Mordecai.Skills.MilestoneFiresGameplayEvent` — verifies AC-011.13
- [x] `Mordecai.Skills.MilestoneGrantsAbility` — verifies AC-011.14
- [x] `Mordecai.Skills.SetRankGrantsAllMilestonesBelowOrEqual` — verifies AC-011.15
- [x] `Mordecai.Skills.MilestoneTagsAppliedAndRemoved` — verifies AC-011.16
- [x] `Mordecai.Skills.LongswordsSamplePipeline` — verifies AC-011.17

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-011]` prefix
