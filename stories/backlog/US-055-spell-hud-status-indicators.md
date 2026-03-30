# US-055: Spell HUD & Status Effect Indicators

## Overview
Extend the combat HUD C++ framework (from US-052) with spell-related UI elements: a Spell Points bar, active buff/debuff status indicators with remaining duration, and a basic spell cooldown display. These C++ widget base classes enable the EDITOR story (US-057) to wire up spell UI in the playable arena.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `game_design_v2.md` — Section 8 ("Clean UI: Always show Health 0-100%, tiered Stamina, Spell Points. Status is primarily VFX-driven, not icon soup.")
- Design doc: `combat_system_v1.md` — SP as a core resource
- Design doc: `character_attributes_v1.md` — SpellPoints, MaxSpellPoints attributes
- Agent rules: `agent_rules_v2.md` — GAS for attributes
- Existing code: `UMordecaiCombatHUDWidget` (US-052), `UMordecaiHealthBarWidget`, `UMordecaiStaminaBarWidget`, `UMordecaiPostureBarWidget`, `UMordecaiAttributeSet` (SpellPoints, MaxSpellPoints)

---

## Acceptance Criteria

### SP Bar
- [ ] AC-055.1: `UMordecaiSpellPointsBarWidget` exists in `Mordecai/UI/`, inheriting from `UUserWidget`. Displays current SpellPoints as a percentage of MaxSpellPoints (same pattern as HealthBarWidget)
- [ ] AC-055.2: `UMordecaiSpellPointsBarWidget::BindToASC(UAbilitySystemComponent*)` binds attribute change delegates for `SpellPoints` and `MaxSpellPoints`. Bar updates in real-time when SP changes
- [ ] AC-055.3: SP bar displays the numeric SP value (e.g., "12 / 20") in addition to the percentage fill, since SP is a discrete resource unlike health percentage

### Status Effect Indicators
- [ ] AC-055.4: `UMordecaiStatusEffectIndicatorWidget` exists in `Mordecai/UI/`. Displays a single active status effect: tag name (or configurable display name), remaining duration countdown, and a visual indicator (colored tint: red for debuff, green for buff)
- [ ] AC-055.5: `UMordecaiStatusEffectBarWidget` exists in `Mordecai/UI/`. Manages a horizontal row of `UMordecaiStatusEffectIndicatorWidget` children. Dynamically adds/removes indicators when gameplay tags matching `Mordecai.Status.*` are added/removed from the bound ASC
- [ ] AC-055.6: `UMordecaiStatusEffectBarWidget::BindToASC(UAbilitySystemComponent*)` registers a gameplay tag change delegate for `Mordecai.Status` (and children). On tag added: create indicator. On tag removed: destroy indicator
- [ ] AC-055.7: Status indicators distinguish buffs from debuffs. Tags in a configurable `BuffTags` set (e.g., `Mordecai.Status.Focused`, `Mordecai.Status.Blessed`, `Mordecai.Status.StoneSkin`) show green tint. All others show red tint. Default: all `Mordecai.Status.*` are debuffs unless listed in `BuffTags`

### Spell Cooldown Display
- [ ] AC-055.8: `UMordecaiSpellCooldownWidget` exists in `Mordecai/UI/`. Displays cooldown remaining for a single spell (bound to a specific cooldown gameplay tag). Shows remaining seconds as text and a radial/linear fill representing percentage of total cooldown elapsed
- [ ] AC-055.9: `UMordecaiSpellCooldownWidget::BindToCooldownTag(UAbilitySystemComponent*, FGameplayTag CooldownTag, float TotalCooldown)` monitors the ASC for the presence of the cooldown tag. While tag is active, decrements display. When tag is removed, shows "ready" state

### Integration
- [ ] AC-055.10: `UMordecaiCombatHUDWidget` is extended with UPROPERTY slots for: `SpellPointsBar`, `StatusEffectBar`, and an array of `SpellCooldownWidgets`. The `BindToASC` method chains binding to these new child widgets

## Technical Notes
- **Follow US-052 pattern exactly.** The existing HUD widgets (Health, Stamina, Posture) all follow the same pattern: C++ base class with `BindToASC`, attribute change delegates, and UPROPERTY fields for UMG sub-widgets. The new widgets should be identical in structure.
- **SP bar:** SpellPoints is an integer resource (not percentage-based like health). Display both the fill bar AND the numeric value "Current / Max".
- **Status indicators:** The key challenge is tracking active GE durations. Options: (a) query `ASC->GetActiveGameplayEffects()` filtered by tag to get remaining duration, (b) use a timer based on the known duration. Recommend (a) — query the ASC each tick/timer for remaining time of the first active GE matching the tag.
- **Cooldown widget:** Uses the same pattern as GAS cooldown queries: `ASC->GetCooldownTimeRemaining()` or check for the cooldown tag + query remaining time on the GE.
- **Do NOT create UMG Blueprints** in this story — that's US-057 (EDITOR). This story creates the C++ base classes only.
- Place all files in `Source/LyraGame/Mordecai/UI/`.

## Tests Required
- [ ] `Mordecai.UI.SpellPointsBar.BindsToASC` — SP bar widget binds and reads initial SP value (verifies AC-055.2)
- [ ] `Mordecai.UI.SpellPointsBar.UpdatesOnSPChange` — SP bar updates when SpellPoints attribute changes (verifies AC-055.2)
- [ ] `Mordecai.UI.StatusEffectBar.AddsIndicatorOnTagAdded` — Adding a `Mordecai.Status.*` tag creates a child indicator widget (verifies AC-055.5, AC-055.6)
- [ ] `Mordecai.UI.StatusEffectBar.RemovesIndicatorOnTagRemoved` — Removing the tag destroys the indicator (verifies AC-055.5, AC-055.6)
- [ ] `Mordecai.UI.StatusEffectBar.DistinguishesBuffsFromDebuffs` — Buff tags show green, debuff tags show red (verifies AC-055.7)
- [ ] `Mordecai.UI.SpellCooldown.ShowsCooldownWhileTagActive` — Cooldown widget shows remaining time while cooldown tag is present (verifies AC-055.8, AC-055.9)
- [ ] `Mordecai.UI.SpellCooldown.ShowsReadyWhenCooldownExpires` — Widget shows ready state when cooldown tag removed (verifies AC-055.9)
- [ ] `Mordecai.UI.CombatHUD.BindsChainsToNewWidgets` — CombatHUDWidget binds SP bar, status bar, and cooldown widgets via BindToASC (verifies AC-055.10)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-055]` prefix
