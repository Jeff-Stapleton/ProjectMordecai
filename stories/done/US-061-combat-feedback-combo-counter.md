# US-061: Combat Feedback — Combo Counter & Action Indicators

## Overview
Add visual feedback widgets for existing combat mechanics: a combo counter showing the current hit in a melee chain, and text flashes for perfect actions (dodge, block, parry) and failures (parry whiff, posture broken).

## Execution Mode
**Mode:** HEADLESS

## References
- Existing code: `UMordecaiGA_MeleeAttack` (combo system), `UMordecaiGA_Dodge` (perfect dodge), `UMordecaiGA_Block` (perfect block), `UMordecaiGA_Parry` (parry/whiff)
- Gameplay tags: `Mordecai.State.ComboWindow`, `Mordecai.State.PerfectDodge`, `Mordecai.State.PerfectBlock`, `Mordecai.State.Parried`, `Mordecai.State.ParryWhiff`, `Mordecai.State.PostureBroken`

---

## Acceptance Criteria
- [x] AC-061.1: `UMordecaiComboCounterWidget` exists in `Mordecai/UI/`. Shows current combo hit (Hit 1/2/3) during melee combos. Listens for `Mordecai.State.ComboWindow` tag. Auto-hides when combo window closes.
- [x] AC-061.2: `UMordecaiCombatFeedbackWidget` exists in `Mordecai/UI/`. Shows brief text flashes for: Perfect Dodge (gold), Perfect Block (gold), Parried (gold), Parry Failed (red), Posture Broken (gold). Listens to 5 state tags. Auto-fades after ~1s.
- [x] AC-061.3: `UMordecaiCombatHUDWidget` extended with `BindWidgetOptional` slots for ComboCounter and CombatFeedback. `BindToASC` chains to both.
- [x] AC-061.4: `FormatComboHit(int32)` and `GetFeedbackTextForTag(FGameplayTag)` are static, testable without widget tree.

## Tests Required
- [x] `Mordecai.UI.ComboCounter.FormatComboHit` — Index 0→"Hit 1", 1→"Hit 2", 2→"Hit 3"
- [x] `Mordecai.UI.ComboCounter.FormatComboHitNegative` — Negative index → empty
- [x] `Mordecai.UI.CombatFeedback.GetFeedbackTextForTag` — Maps 5 tags to display text
- [x] `Mordecai.UI.CombatFeedback.GetFeedbackColorForTag` — Gold for positive, red for negative
- [x] `Mordecai.UI.CombatFeedback.UnknownTagReturnsEmpty` — Unknown tags → empty

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-061]` prefix
