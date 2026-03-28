# US-052: Combat HUD — Minimal C++ Framework

## Overview
Implement C++ widget classes for a minimal combat HUD that displays Health (%), Stamina Tier (Green/Yellow/Red), and Posture. These are the three core combat resources. Without visual feedback, Jeff cannot playtest any combat system. This story covers the C++ framework; the EDITOR story (US-054) creates the UMG Blueprint widget and adds it to the viewport.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `game_design_v2.md` (Section 9 — UI/Feedback: "Always visible: Health%, tiered stamina, spell points", "Status effects communicated primarily via VFX")
- Design doc: `combat_system_v1.md` (Health 0-100%, tiered stamina, posture meter)
- Design decisions: `design_decisions_log.md` (stamina tiers: Green 1.25x, Yellow 1.0x, Red 0.75x, Exhausted 0.5x)
- Agent rules: `agent_rules_v2.md` (GAS for attributes, Clean UI pillar)
- Existing code: `UMordecaiAttributeSet` (Health, MaxHealth, Stamina, MaxStamina, Posture, MaxPosture), `UMordecaiStaminaSystem` (tier thresholds), `EMordecaiStaminaTier`

---

## Acceptance Criteria
- [ ] AC-052.1: `UMordecaiCombatHUDWidget` (UUserWidget subclass) exists as the root HUD container that hosts all combat UI elements
- [ ] AC-052.2: `UMordecaiHealthBarWidget` displays Health as a percentage (0–100%) of MaxHealth, using a progress bar bound to the player's Health/MaxHealth attributes
- [ ] AC-052.3: `UMordecaiStaminaBarWidget` displays the stamina bar fill amount AND the current stamina tier as a color indicator (Green/Yellow/Red/Exhausted) using the existing `UMordecaiStaminaSystem` tier thresholds
- [ ] AC-052.4: `UMordecaiPostureBarWidget` displays the player's posture meter as a progress bar (Posture/MaxPosture), with visual distinction when `Mordecai.State.PostureBroken` tag is active
- [ ] AC-052.5: All widgets expose a `BindToASC(UAbilitySystemComponent*)` method that registers attribute change delegates and updates the display in real-time
- [ ] AC-052.6: `GetHealthPercent()` returns `FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f)` — returns 0 if MaxHealth is 0
- [ ] AC-052.7: `GetStaminaPercent()` returns `FMath::Clamp(Stamina / MaxStamina, 0.0f, 1.0f)` — returns 0 if MaxStamina is 0
- [ ] AC-052.8: `GetStaminaTier()` returns the correct `EMordecaiStaminaTier` based on current Stamina/MaxStamina ratio and the tier thresholds (Green ≥ 66%, Yellow ≥ 33%, Red > 0%, Exhausted ≤ 0)
- [ ] AC-052.9: `GetPosturePercent()` returns `FMath::Clamp(Posture / MaxPosture, 0.0f, 1.0f)` — returns 0 if MaxPosture is 0
- [ ] AC-052.10: An `UMordecaiEnemyHealthBarWidget` exists for displaying enemy health above their head (world-space widget component), bound to the enemy's ASC
- [ ] AC-052.11: All percentage/tier computation methods are static or const and independently testable without a live widget tree

## Technical Notes
- **Class locations:**
  - `Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.h/.cpp`
  - `Source/LyraGame/Mordecai/UI/MordecaiHealthBarWidget.h/.cpp`
  - `Source/LyraGame/Mordecai/UI/MordecaiStaminaBarWidget.h/.cpp`
  - `Source/LyraGame/Mordecai/UI/MordecaiPostureBarWidget.h/.cpp`
  - `Source/LyraGame/Mordecai/UI/MordecaiEnemyHealthBarWidget.h/.cpp`
- **Testing strategy:** The core logic (percent calculations, tier determination) lives in static helper functions or const methods that take raw float values. This makes them testable headless without needing UMG widget rendering.
- The widgets should use `UPROPERTY(meta=(BindWidget))` for UMG integration but the C++ class can be compiled and tested without the UMG Blueprint.
- `BindToASC()` should use `ASC->GetGameplayAttributeValueChangeDelegate()` to register callbacks for Health, MaxHealth, Stamina, MaxStamina, Posture, MaxPosture.
- For stamina tier colors, expose `UPROPERTY(EditAnywhere)` `FLinearColor` fields for each tier. Defaults: Green=(0,1,0), Yellow=(1,1,0), Red=(1,0,0), Exhausted=(0.5,0,0).
- The enemy health bar widget (AC-052.10) should be a simple world-space version that only shows health percent. It will be attached as a `UWidgetComponent` on the enemy BP in US-054.
- Do NOT create UMG Blueprint assets — this is C++ only. BP creation is US-054 (EDITOR).

## Tests Required
- [ ] `Mordecai.HUD.HealthPercentCalculation` — 75/100 → 0.75, 0/100 → 0.0, 100/100 → 1.0 (verifies AC-052.6)
- [ ] `Mordecai.HUD.HealthPercentClampsAboveOne` — 150/100 → 1.0 (verifies AC-052.6)
- [ ] `Mordecai.HUD.HealthPercentZeroMaxHealth` — 50/0 → 0.0 (verifies AC-052.6)
- [ ] `Mordecai.HUD.StaminaPercentCalculation` — 80/100 → 0.8, 0/100 → 0.0 (verifies AC-052.7)
- [ ] `Mordecai.HUD.StaminaTierGreen` — 70% stamina → Green (verifies AC-052.8)
- [ ] `Mordecai.HUD.StaminaTierYellow` — 50% stamina → Yellow (verifies AC-052.8)
- [ ] `Mordecai.HUD.StaminaTierRed` — 20% stamina → Red (verifies AC-052.8)
- [ ] `Mordecai.HUD.StaminaTierExhausted` — 0% stamina → Exhausted (verifies AC-052.8)
- [ ] `Mordecai.HUD.StaminaTierBoundaryGreenYellow` — exactly 66% → Green (verifies AC-052.8)
- [ ] `Mordecai.HUD.StaminaTierBoundaryYellowRed` — exactly 33% → Yellow (verifies AC-052.8)
- [ ] `Mordecai.HUD.PosturePercentCalculation` — 60/100 → 0.6 (verifies AC-052.9)
- [ ] `Mordecai.HUD.PosturePercentZeroMaxPosture` — 50/0 → 0.0 (verifies AC-052.9)

---

## Definition of Done
- [ ] All tests written and failing (red phase confirmed)
- [ ] All implementation complete
- [ ] All tests passing (green)
- [ ] Project compiles with zero errors
- [ ] Code committed and pushed with `[US-052]` prefix
