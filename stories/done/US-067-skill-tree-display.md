# US-067: Skill Tree Display

## Overview
Build the C++ skill tree widget that displays all skills grouped by category, shows current ranks and milestone progress, and allows the player to allocate skill points. This is the "Skills" tab content for the Pause Menu (US-069). The widget reads from `UMordecaiSkillComponent` (US-011) and provides the primary interface for the player's skill progression — per game_design_v2, "Everything you do is a skill" with ranks 1–20 and milestones at 1/5/10/15/20.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `skill_sheet_v1.1.md` — all skills by category (Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic) with milestone descriptions at ranks 1/5/10/15/20
- Design doc: `game_design_v2.md` Section 5 — Skills: granular, max 20, milestones unlock techniques/passives
- Agent rules: `agent_rules_v2.md` — GAS for abilities
- Existing code: `UMordecaiSkillComponent` (US-011) — `GetSkillRank()`, `GetAllSkillsByCategory()`, `TryAllocateSkillPoint()`, `GetAvailableSkillPoints()`, `OnSkillMilestoneReachedBP` delegate
- Existing code: `UMordecaiSkillDataAsset` — `SkillName`, `Category`, `RankDescriptions` (TMap<int32, FText>), `MilestoneAbilities`
- Existing code: `EMordecaiSkillCategory` — Weapon, DefenseArmor, MovementExploration, UtilityInteraction, Magic
- Depends on: US-069 (Pause Menu Framework — tab registration API), US-011 (Skill Framework)

---

## Acceptance Criteria

### Widget Structure
- [x] AC-067.1: `UMordecaiSkillTreeWidget` (UUserWidget) exists in `Mordecai/UI/`. It is the content widget for the Pause Menu "Skills" tab. It registers itself with the pause menu via `RegisterTab("skills", ...)`.
- [x] AC-067.2: The widget has a `BindToSkillComponent(UMordecaiSkillComponent*)` method that reads current skill state and subscribes to the `OnSkillMilestoneReachedBP` delegate for live updates.

### Category Tabs / Sections
- [x] AC-067.3: Skills are displayed grouped by `EMordecaiSkillCategory`. Each category is a section or sub-tab: Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic. The active category can be switched by the player.
- [x] AC-067.4: Each category section lists all registered skills in that category via `GetAllSkillsByCategory()`. Each skill row shows: skill name (FName → FText), current rank (int32), and max rank (20).

### Rank & Milestone Display
- [x] AC-067.5: Each skill row displays a rank bar or numeric indicator showing current rank out of 20. Milestone ranks (1, 5, 10, 15, 20) are visually distinguished (e.g., marked nodes or different formatting).
- [x] AC-067.6: Selecting a skill shows a detail panel with milestone descriptions from `UMordecaiSkillDataAsset::RankDescriptions`. Each milestone (1/5/10/15/20) shows its description text and whether it has been reached (current rank >= milestone rank). Unreached milestones show the description but in a "locked" presentation.
- [x] AC-067.7: The detail panel indicates which milestones have granted abilities (via `HasGrantedMilestoneAbility()`), distinguishing "unlocked and active" from "not yet reached."

### Skill Point Allocation
- [x] AC-067.8: The widget displays available skill points prominently (from `GetAvailableSkillPoints()`). Updates when points change.
- [x] AC-067.9: Each skill row has an "Allocate" action (button or interaction). When activated, calls `TryAllocateSkillPoint(SkillName)` on the component. On success, the skill's displayed rank increments and available points decrements. On failure (no points or max rank), no change occurs.
- [x] AC-067.10: The allocate action is disabled (grayed out / not interactable) when available skill points are 0 or the skill is already at max rank (20).

### Live Updates
- [x] AC-067.11: When a skill milestone is reached (via the `OnSkillMilestoneReachedBP` delegate), the widget updates the corresponding skill's milestone indicator to show "unlocked" state.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/UI/`.
- The widget finds the SkillComponent from the local player's PlayerState (where it's attached per US-011).
- **Category switching:** Use a simple index or enum to track the active category. Switching categories refreshes the skill list from `GetAllSkillsByCategory()`. No need for a full CommonUI tab system — a simple button group or list selector.
- **Skill detail panel:** A child widget or expandable section that shows milestone info for the selected skill. Reads `RankDescriptions` from the registered `UMordecaiSkillDataAsset`. If no DataAsset is registered for a skill, milestone descriptions show "No data available."
- **Allocation feedback:** `TryAllocateSkillPoint` returns bool. On success, the UI refreshes the skill rank display and available points. No need for server round-trip UI — the component handles replication.
- **NullRHI testing:** Widget creation, skill component binding, category filtering, allocation calls, and state queries can all be tested headlessly.
- **No skill respec:** There is no "unlearn" or respec mechanic. Skill points are one-way allocation. `TODO(DECISION)` — respec system is not in scope.

## Tests Required
- [x] `Mordecai.UI.SkillTree.DisplaysSkillsByCategory` — Widget shows skills filtered by the active category (AC-067.3, AC-067.4)
- [x] `Mordecai.UI.SkillTree.ShowsRankAndMaxRank` — Each skill shows current rank / 20 (AC-067.4)
- [x] `Mordecai.UI.SkillTree.MilestonesDistinguished` — Milestone ranks (1/5/10/15/20) are marked differently from regular ranks (AC-067.5)
- [x] `Mordecai.UI.SkillTree.DetailPanelShowsMilestoneDescriptions` — Selecting a skill shows its milestone descriptions from DataAsset (AC-067.6)
- [x] `Mordecai.UI.SkillTree.DisplaysAvailablePoints` — Widget shows available skill points from component (AC-067.8)
- [x] `Mordecai.UI.SkillTree.AllocatePointIncrementsRank` — Allocating a point via the UI increases the skill rank by 1 and decrements available points (AC-067.9)
- [x] `Mordecai.UI.SkillTree.AllocateDisabledWhenNoPoints` — Allocate action is disabled when available skill points are 0 (AC-067.10)
- [x] `Mordecai.UI.SkillTree.AllocateDisabledAtMaxRank` — Allocate action is disabled when skill is at rank 20 (AC-067.10)
- [x] `Mordecai.UI.SkillTree.MilestoneReachedUpdatesUI` — Reaching a milestone rank triggers the widget to update the milestone's visual state (AC-067.11)
- [x] `Mordecai.UI.SkillTree.GracefulWithoutComponent` — Widget with no SkillComponent bound shows empty state (AC-067.2)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-067]` prefix
