# US-066: Character Sheet — Attributes & Derived Stats

## Overview
Build the C++ character sheet widget that displays primary attributes, core resources, and derived stats. This is the first content tab for the Pause Menu (US-069). The widget reads from the player's ASC (`UMordecaiAttributeSet`) and updates in real-time when attribute values change. The character sheet is the player's primary window into their build identity — per game_design_v2 Section 5, attributes are "D&D-like scalers" and the sheet must surface both raw values and their derived effects.

## Execution Mode
**Mode:** HEADLESS

## References
- Design doc: `character_attributes_v1.md` — 9 primary stats (STR/DEX/END/CON/RES/DIS/INT/WIS/CHA), primary effects, secondary effects
- Design doc: `stat_formulas_v1.md` — derived stat formulas (Health, Stamina, SpellPoints, SpellRegen, Posture)
- Design doc: `game_design_v2.md` Section 5 — Progression: attributes, skills, feats
- Design doc: `game_design_v2.md` Section 8 — Clean UI principle
- Agent rules: `agent_rules_v2.md` — GAS for attributes
- Existing code: `UMordecaiAttributeSet` (9 primary + 5 primary-derived + 9 secondary-derived + 4 core resources), `UMordecaiPauseMenuWidget::RegisterTab()` (US-069)
- Depends on: US-069 (Pause Menu Framework — tab registration API), US-010 (Attribute Scaling — derived stats exist)

---

## Acceptance Criteria

### Widget Structure
- [x] AC-066.1: `UMordecaiCharacterSheetWidget` (UUserWidget) exists in `Mordecai/UI/`. It is the content widget for the Pause Menu "Character" tab. It registers itself with the pause menu via `RegisterTab("character", ...)`.
- [x] AC-066.2: The widget has a `BindToASC(UAbilitySystemComponent*)` method that subscribes to attribute value change delegates for all displayed attributes. When an attribute changes, the corresponding UI element updates immediately (no polling).

### Primary Attributes Display
- [x] AC-066.3: Displays all 9 primary attributes in a vertical list or grid: STR, DEX, END, CON, RES, DIS, INT, WIS, CHA. Each row shows: attribute name (FText), base value (int32), and effective modifier bonus (formatted as "+X%" where X is the derived multiplier contribution per `character_attributes_v1.md`).
- [x] AC-066.4: Primary attributes are grouped into three thematic columns: Physical (STR, DEX, END), Resilience (CON, RES, DIS), Magical (INT, WIS, CHA). Group labels are configurable FText.

### Core Resources Display
- [x] AC-066.5: Displays the 4 core resource pools: Health (current/max), Stamina (current/max), Spell Points (current/max), Posture (current/max). Each shows as "CurrentValue / MaxValue" text. Values update in real-time from the ASC.

### Derived Stats Display
- [x] AC-066.6: Displays primary-derived multipliers: Physical Damage, Attack Speed, Affliction Resist, Magic Damage, Cast Speed. Each shows as a percentage (e.g., "Physical Damage: +15%"). The percentage is `(MultiplierValue - 1.0) * 100`, since the base multiplier is 1.0.
- [x] AC-066.7: Displays secondary-derived stats: Armor Penetration, Physical Crit Chance, Stamina Regen, Health Regen, Affliction Recovery, Posture Recovery, SP Regen, Resistance Penetration, Magic Crit Chance. Same percentage format as primary-derived.

### Data Binding
- [x] AC-066.8: `BindToASC` uses `GetGameplayAttributeValueChangeDelegate()` on the ASC to register for changes on each displayed attribute. When a delegate fires, only the affected UI element is updated (not the entire widget). If no ASC is provided, all values display as "--" (graceful null handling).
- [x] AC-066.9: When the widget is destroyed or the ASC is unbound, all attribute change delegate handles are properly cleared (no dangling delegates).

## Technical Notes
- Place in `Source/LyraGame/Mordecai/UI/`.
- The widget binds to the player's ASC via `BindToASC()`. The pause menu subsystem (or the widget itself in `NativeConstruct`) finds the local player's ASC.
- **Attribute grouping:** The three-column layout (Physical/Resilience/Magical) matches the D&D attribute flavor from game_design_v2. This is a C++ layout — visual polish is EDITOR work.
- **Percentage formatting:** Primary-derived multipliers default to 1.0 (meaning 0% bonus). Display formula: `FString::Printf(TEXT("+%.1f%%"), (Value - 1.0f) * 100.0f)`.
- **No level display yet:** Character level is not implemented (no leveling system). Add a placeholder `Level: --` text that can be wired later.
- **NullRHI testing:** Widget creation, ASC binding, delegate registration, and value reading can all be tested headlessly by verifying widget state and delegate counts without visual rendering.
- Follow the same ASC binding pattern used by `UMordecaiHealthBarWidget` (US-052).

## Tests Required
- [x] `Mordecai.UI.CharacterSheet.DisplaysPrimaryAttributes` — Widget shows all 9 primary attributes with correct values from ASC (AC-066.3)
- [x] `Mordecai.UI.CharacterSheet.PrimaryAttributesGrouped` — Attributes are organized into 3 groups: Physical, Resilience, Magical (AC-066.4)
- [x] `Mordecai.UI.CharacterSheet.DisplaysCoreResources` — Widget shows Health, Stamina, SP, Posture as current/max (AC-066.5)
- [x] `Mordecai.UI.CharacterSheet.DisplaysDerivedMultipliers` — Widget shows primary-derived stats as percentages (AC-066.6)
- [x] `Mordecai.UI.CharacterSheet.DisplaysSecondaryDerived` — Widget shows all 9 secondary-derived stats (AC-066.7)
- [x] `Mordecai.UI.CharacterSheet.UpdatesOnAttributeChange` — Changing an attribute on the ASC triggers the widget to update the corresponding display value (AC-066.2, AC-066.8)
- [x] `Mordecai.UI.CharacterSheet.GracefulWithoutASC` — Widget with no ASC bound displays "--" placeholder values (AC-066.8)
- [x] `Mordecai.UI.CharacterSheet.ClearsDelegatesOnDestroy` — Destroying the widget or unbinding the ASC properly clears all delegate handles (AC-066.9)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-066]` prefix
