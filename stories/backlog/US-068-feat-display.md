# US-068: Feat Display

## Overview
Build the C++ feat display widget that shows unlocked feats with their tier, description, and applied effects, plus a preview of locked feats with unlock condition progress. This is the "Feats" tab content for the Pause Menu (US-069). The widget reads from `UMordecaiFeatComponent` (US-012). Per game_design_v2, feats are "earned, not picked" — unlocked via gameplay achievements with Common/Rare/Legendary tiers, and strong feats can include drawbacks.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `game_design_v2.md` Section 5 — Feats: unlocked by achievements/patterns, rarity tiers with tradeoffs (Common: modest no downside, Rare: strong + light downside, Legendary: build-defining + meaningful drawback)
- Agent rules: `agent_rules_v2.md` — GAS for abilities/effects
- Existing code: `UMordecaiFeatComponent` (US-012) — `HasFeat()`, `GetUnlockedFeats()`, `GetFeatAppliedRecord()`, `GetFeatStatValue()`, `OnFeatUnlockedBP` delegate
- Existing code: `UMordecaiFeatDataAsset` — `FeatName`, `DisplayName` (FText), `Description` (FText), `Tier` (EMordecaiFeatTier), `UnlockConditions` (TArray<FMordecaiFeatCondition>), `GrantedEffects`, `DrawbackEffects`
- Existing code: `EMordecaiFeatTier` — Common, Rare, Legendary
- Existing code: `FMordecaiFeatCondition` — ConditionType (StatThreshold/EventCount/TagPresent/SkillRank), StatName, ThresholdValue, RequiredTag
- Depends on: US-069 (Pause Menu Framework — tab registration API), US-012 (Feat System)

---

## Acceptance Criteria

### Widget Structure
- [ ] AC-068.1: `UMordecaiFeatDisplayWidget` (UUserWidget) exists in `Mordecai/UI/`. It is the content widget for the Pause Menu "Feats" tab. It registers itself with the pause menu via `RegisterTab("feats", ...)`.
- [ ] AC-068.2: The widget has a `BindToFeatComponent(UMordecaiFeatComponent*)` method that reads current feat state and subscribes to the `OnFeatUnlockedBP` delegate for live unlock notifications.

### Unlocked Feats List
- [ ] AC-068.3: Displays a list of all unlocked feats from `GetUnlockedFeats()`. Each feat row shows: display name (FText from DataAsset), tier badge (Common/Rare/Legendary), and a short description excerpt.
- [ ] AC-068.4: Feat rows are color-coded by tier: Common → White/Gray (`FLinearColor(0.8, 0.8, 0.8)`), Rare → Blue (`FLinearColor(0.3, 0.5, 1.0)`), Legendary → Gold (`FLinearColor(1.0, 0.75, 0.0)`). Colors are configurable via UPROPERTY.
- [ ] AC-068.5: Selecting an unlocked feat shows a detail panel with: full description (FText), tier, number of granted effects, number of granted abilities, and number of drawback effects (from `GetFeatAppliedRecord()`).

### Locked Feats Preview
- [ ] AC-068.6: Displays a separate section for locked (not-yet-unlocked) registered feats. Each locked feat row shows: display name (visible), description (hidden or teaser text like "???"), tier, and an unlock progress summary.
- [ ] AC-068.7: Unlock progress summary shows condition progress for each `FMordecaiFeatCondition` on the feat. For `StatThreshold` and `EventCount` conditions: shows "StatName: CurrentValue / ThresholdValue". For `SkillRank` conditions: shows "SkillName Rank: CurrentRank / RequiredRank". For `TagPresent` conditions: shows "Requires: TagName" with a check/cross for whether the tag is present. Current values are read from `GetFeatStatValue()` and the SkillComponent.

### Counters & Summary
- [ ] AC-068.8: The widget displays a summary header showing total unlocked feats count and breakdown by tier (e.g., "Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary").

### Live Updates
- [ ] AC-068.9: When a feat is unlocked during gameplay (via `OnFeatUnlockedBP` delegate), the feat moves from the locked section to the unlocked section and the summary counter updates. If the pause menu is open when this happens, the change is visible immediately.

## Technical Notes
- Place in `Source/LyraGame/Mordecai/UI/`.
- The widget finds the FeatComponent from the local player's PlayerState (where it's attached per US-012).
- **Registered feats discovery:** The FeatComponent's `RegisteredFeats` map contains all known feats (both locked and unlocked). Use `GetUnlockedFeats()` to partition into unlocked vs. locked. Locked feats = registered feats minus unlocked feats.
- **Condition progress:** For StatThreshold/EventCount conditions, use `FeatComponent->GetFeatStatValue(StatName)` to get the current tracked value. For SkillRank conditions, find the SkillComponent and call `GetSkillRank(SkillName)`. For TagPresent, check if the tag is in the FeatComponent's active tags.
- **Tier colors:** Store as a `TMap<EMordecaiFeatTier, FLinearColor>` UPROPERTY for designer override.
- **No feat respec:** Feats cannot be unlearned. Once unlocked, they stay.
- **NullRHI testing:** All widget state, component binding, feat queries, and delegate handling can be tested headlessly.
- The detail panel for unlocked feats can show the `FMordecaiFeatAppliedRecord` counts (e.g., "2 Effects, 1 Ability, 1 Drawback") — the exact effect names are not exposed without additional DataAsset queries. This is sufficient for the C++ framework; richer tooltips are EDITOR/UI polish.

## Tests Required
- [ ] `Mordecai.UI.FeatDisplay.ShowsUnlockedFeats` — Widget lists all unlocked feats with correct names and tiers (AC-068.3)
- [ ] `Mordecai.UI.FeatDisplay.TierColorCoding` — Feat rows use correct colors for Common, Rare, Legendary tiers (AC-068.4)
- [ ] `Mordecai.UI.FeatDisplay.DetailPanelShowsDescription` — Selecting an unlocked feat shows its full description and applied record (AC-068.5)
- [ ] `Mordecai.UI.FeatDisplay.ShowsLockedFeats` — Widget lists registered but not-yet-unlocked feats in a locked section (AC-068.6)
- [ ] `Mordecai.UI.FeatDisplay.LockedFeatShowsProgress` — Locked feat row shows condition progress (current/threshold) for each unlock condition (AC-068.7)
- [ ] `Mordecai.UI.FeatDisplay.SummaryCountsCorrect` — Summary header shows correct total and per-tier counts (AC-068.8)
- [ ] `Mordecai.UI.FeatDisplay.LiveUnlockMovesToUnlocked` — Unlocking a feat via the component moves it from locked to unlocked section in real-time (AC-068.9)
- [ ] `Mordecai.UI.FeatDisplay.GracefulWithoutComponent` — Widget with no FeatComponent bound shows empty state (AC-068.2)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-068]` prefix
