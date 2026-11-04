# CharacterTab

The Character tab of the pause menu: Level, Skill Points and resources up top, the active statuses, nine attribute cards with band meters and a derived-stat receipt. Use it as the content of the `character` tab inside the PauseShell's 948 x 540 content box (it fills it exactly); its "Active statuses" row is where StatusTooltip opens.

## Anatomy
Positions are design px at 1920 x 1080 (content box at x 486-1434, y 298-838). States not on screen are in the spec band under the screen.

| Part | Box | Build |
|---|---|---|
| Header strip | 948 x 76 at (486, 298) | `plate-raised`, 2 px `lip`, radius 10, padding 0 14, gap 14. |
| Level badge | 84 x 58 at (502, 307) | Unavailable today, so it is drawn as unavailable (dashed means unavailable, D24): `plate` face, 2 px dashed `lip`, radius 8; `ui-label` "Level" in `text-muted` over `num-l` "--" in `text-disabled`. |
| Skill Points badge | 162 x 52 at (600, 310) | The ONE skill-point badge, shared with SkillsTab (`.tt-skills__points`): a 48 px `good` coin (2 px `ink` edge, an `ink` ring 3 px inside) holding the `skill-point` glyph in `ink`, then "Skill Points" (`ui-label`, `text-muted`) over the count (`num-l`, `good`, a 2-digit box). |
| Divider | 2 x 44 | Core `.tt-divider--v`. |
| Resource cells | Health, Stamina, Posture 138 x 66; Spell Points 170 x 66; gap 14; x 792 / 944 / 1096 / 1280 | `ui-label` name in `text-muted`, the value `num-m` in `text` ("118 / 156"), a meter under it. Health, Stamina, Posture: an 8 px hair bar (`vital`; the stamina tier fill; `posture`). Spell Points: tokens, never a bar (Tally's `.tt-tally-sp` at 8 px, 2 apart, 6 between fives; 16 fit the 170 px cell). 138 px holds "-50 / 172" (9 Mono cells = 137 px). |
| Active statuses row | 600 x 46 at (486, 388) | StatusTooltip's host. 10 px headroom for a buff chip's tab, then a 36 px line: the label "Statuses" (`ui-label`, `text-muted`, 79 px), 14 px (a focused chip's ring stays clear of the label), then StatusChip's own chips (its classes and look: glyph, name, stack pips, time), 10 px apart, in StatusChip's sort order, in the 507 px left. The debuff teeth hang into the 12 px gap above the group headings. Frostbitten (272) + Focused (169) take 451. |
| Group heading (x3) | 192 x 26 at y 446 | `ui-label` caps in `text-muted`, tracked 0.06em, then a core `.tt-divider` filling the rest of the column. |
| Attribute card (x9) | 192 x 112, columns x 486 / 690 / 894, rows y 480 / 602 / 724 | `plate-raised`, 2 px `lip`, radius 10, padding 10 / 10 / 12 (the three rows take 88 of the 90 px inside). Row 1 (34): the abbreviation keycap (min 58 x 32, Mono 800 20 px, `ink` on `enamel`) and the base value `num-l` right-aligned in a 2-digit box. Row 2: the name, `ui-label` `text`. Row 3 (28): the band meter and the Effective Mod `num-s` right-aligned in a 76 px box ("+12.0%"). |
| Band meter | 77 x 28 | Four wells (17 wide, gap 3, 2 px `lip`, `well` inside), heights 28 / 20 / 13 / 8 = 100 / 70 / 45 / 25 % of 28, bottom-aligned. Each well is one band of 5 Mod points; its `rim` fill grows left to right with the points in that band. |
| Receipt | 328 wide at (1106, 388), torn foot to y 837 | `enamel` paper, top radius 10, a 4 px `enamel-shade` band inside the top, padding 10 / 12 / 20, a zigzag foot (20 teeth, 10 px deep). Kickers "Derived · primary" and "Derived · secondary" in `ink-muted`, a core `.tt-divider--enamel` between them. Rows 24 tall from y 426: label `ui-label` `ink-muted`, a dotted leader (`ink-muted` dots every 7 px), value `num-s` `ink` right-aligned in a 76 px box. |

## States
- **Bound (live).** Every value updates per row when its attribute changes.
- **Unbound** (no ASC yet): every base value, Effective Mod and resource reads "--" and the band meters are empty (spec band).
- **Card focus.** Core focus with `.tt-focus--fill-gap`: the 3 px `focus` ring outside the card with an `ink` ring filling the gap (the card has no outline of its own). The card's derived rows on the receipt light up: `enamel-shade` band, label in `ink` at 700, and a 4 px `ink` tick at the receipt's left edge. Pairs: STR Physical Damage + Armor Penetration; DEX Attack Speed + Physical Crit Chance; END Stamina + Stamina Regen; CON Health + Health Regen; RES Affliction Resist + Affliction Recovery; DIS Posture + Posture Recovery; INT Spell Points + Spell Points Regen; WIS Magic Damage + Resistance Penetration; CHA Cast Speed + Magic Crit Chance.
- **Status chip focus.** Each chip in the Active statuses row is in the tab's focus order (before the STR card) and takes core focus (the 3 px `focus` ring with its `ink` keyline). Focus or mouse hover hangs that status's StatusTooltip under the chip on its light string (glyph, name, tagline, time left, stack pips, counterplay); it goes when focus or hover moves. The time is frozen at the pause value, as on the HUD chip behind the veil.
- **Statuses: many or none.** One row only: chips print full while they fit beside the label, then every chip drops its name at once (StatusChip's compact switch), then the last chips in sort order fold into one "+N" chip. No status: the label stays and the one empty state (core `.tt-empty`, D8) sits beside it at chip height (36 tall), reading "None." (spec band). The row never collapses, so the cards never move.
- **Low Health** (25% or less): the Health value turns `danger-text`.
- **Exhausted** (Stamina below 0, floor -50): the Stamina value in `danger-text` ("-12 / 134"), and the hair bar is `StaminaDebtBar` (D2, the system rule): the `M_UI_PatternFill` hatch fills from the LEFT by |Stamina| / MaxStamina (-12 = 9%); regen eats it from its right edge, then the tier fill grows from the same left edge. The floor is an absolute -50 (`UMordecaiStaminaSystem::StaminaFloor`), so the deepest debt is 50 / MaxStamina: 37% at the default 134. Beside the strip the `tt-pill--exhausted` "×0.5" pill and the word "Exhausted" (spec band).
- **0 skill points.** The coin turns `plate` with `lip` rings, the glyph and the count `text-disabled` (spec band).
- **Spell Points above 16.** The cell shows the number alone (no tokens, never a bar).

## Data and implementation
- Widget: `UMordecaiCharacterSheetWidget`, bound live to the ASC through `FMordecaiCharacterSheetData`. It has no widget tree today (N4, blank tab); US-093 (AC-093.1) builds it inside this box. Bind in `NativeOnActivated` (CommonUI pools the pause menu).
- GAS attributes (`UMordecaiAttributeSet`):
  - Resources: `Health` / `MaxHealth`, `Stamina` / `MaxStamina`, `SpellPoints` / `MaxSpellPoints`, `Posture` / `MaxPosture`.
  - Primary: `Strength`, `Dexterity`, `Endurance`, `Constitution`, `Resistance`, `Discipline`, `Intelligence`, `Wisdom`, `Charisma` (default 10, no clamp).
  - Primary-derived: `PhysicalDamageMultiplier`, `AttackSpeedMultiplier`, `AfflictionResistMultiplier`, `MagicDamageMultiplier`, `CastSpeedMultiplier`.
  - Secondary-derived: `ArmorPenetrationMultiplier`, `PhysicalCritChance`, `StaminaRegenMultiplier`, `HealthRegenMultiplier`, `AfflictionRecoveryMultiplier`, `PostureRecoveryMultiplier`, `SpellPointsRegenMultiplier`, `ResistancePenetrationMultiplier`, `MagicCritChance`.
- Proposed `BindWidgetOptional` names: `LevelText`, `SkillPointsText`, `HealthText`, `StaminaText`, `SpellPointsText`, `PostureText`, `StaminaDebtBar`, `SpellPointTokens`, `AttributeGrid` (3 columns), one `UMordecaiAttributeCardWidget` per attribute with `AbbrText`, `BaseText`, `NameText`, `ModText`, `BandMeter`; `DerivedPrimaryList` and `DerivedSecondaryList` with `LabelText` / `ValueText` rows. All text stays real TextBlocks.
- Active statuses (proposed `BindWidgetOptional`): `ActiveStatusesText` (the label "Statuses"), `ActiveStatusBox` (a `UHorizontalBox`, one row, 600 wide) and `ActiveStatusEmpty` (the empty-state Border + TextBlock). Fill the box from the same source as the HUD's `StatusEffectBar`: one `UMordecaiStatusEffectIndicatorWidget` per active child tag of `Mordecai.Status` on the player's ASC, with the HUD chip's `StatusNameText`, `DurationText`, `StatusGlyph`, `StackPips`, here wrapped so it is focusable (a `UCommonButtonBase`, or `IsFocusable` on the chip). On focus or hover it shows `UMordecaiStatusTooltipWidget` (StatusTooltip) on the top layer. Rebuild in `NativeOnActivated`; the pause freezes the times.
- `StaminaDebtBar`: a `UProgressBar`, BarFillType LeftToRight, fill brush `M_UI_PatternFill` (the hatch, `T_UI_Hatch16`), percent = clamp(-Stamina / MaxStamina, 0, 1), visible only while Stamina < 0. The same widget and rule as GaugePlate's.
- Spell Points tokens: an HBox of token images in groups of five (the Tally token brush at 8 px), MaxSpellPoints = 10 x (1 + 0.04 x EffInt), 13 at the default INT. Collapse it above 16.
- Model order: groups "Physical" (STR, DEX, END), "Resilience" (CON, RES, DIS), "Magical" (INT, WIS, CHA); 5 primary-derived and 9 secondary-derived rows in the order shown; resources Health, Stamina, Spell Points, Posture.
- Formats:
  - Base value: integer.
  - Effective Mod: `FormatEffectiveModBonus` prints the Effective Mod number with a percent sign ("+9.4%" for Mod 12). TODO(DECISION): it is a count of effective points, not a percent; decide between "+9.4%" (today) and "+9.4".
  - Primary-derived and most secondary-derived: (V - 1) x 100, signed, one decimal: "+28.2%".
  - Physical Crit Chance and Magic Crit Chance are raw chances (default 0): V x 100, unsigned, one decimal: "9.0%", "0.0%" before any recalculation. The current (V - 1) formatter would print "-100.0%"; US-093 must special-case them.
  - Resources: "%d / %d" (floored; Stamina may be negative). Placeholder: "--".
- Band meter: points in band i (0..3) = clamp(Mod - 5i, 0, 5); fill = points / 5. Band 4 shows Mod 16-20 and stays full above 20. Effective Mod = min(Mod, 5) + 0.7 x (6..10) + 0.45 x (11..15) + 0.25 x (16+).
- Level: no Level or XP exists. Show "Level" over "--" (the US-066 placeholder "Level: --") in the unavailable badge. TODO(DECISION): levelling and XP; when they exist the badge takes the bound look (enamel keycap-like badge, `ink` number).
- Skill Points: `UMordecaiSkillComponent::AvailableSkillPoints`.
- Example build in the preview (labelled example): STR 12, DEX 20, END 10, CON 12, RES 9, DIS 10, INT 6, WIS 6, CHA 7, giving Health 118 / 156 (MaxHealth = 100 x (1 + 0.06 x EffCon)), Stamina 134 / 134, Spell Points 7 / 12, Posture 134 / 134 and the derived values shown. The active statuses are HUDLayout's fight frame: Frostbitten at 2 of 4 stacks (5.1) and Focused (3/3). The HUD behind the veil shows the same moment (HP 76%, SP 7 / 12, the same two status chips, the stocked quick items). Shipping defaults are 10 in every attribute (Effective Mod 8.5, MaxSpellPoints 13).
- UMG sizes: `num-l` 22.5 pt, `num-m` 18 pt, `num-s` 15 pt, `ui-label` 15 pt, `kicker` 15 pt. Colours are sRGB; convert with `FLinearColor::FromSRGBColor`.
- Tokens: `plate`, `plate-raised`, `well`, `lip`, `rim`, `ink`, `enamel`, `enamel-shade`, `text`, `text-muted`, `text-disabled`, `ink-muted`, `good`, `danger`, `danger-text`, `vital`, `stamina-green`, `spell-points`, `posture`, `focus`; the status chips bring StatusChip's own (`buff`, `debuff`, `dmg-frost` and the other status tints).

## Copy
- Group labels: "Physical", "Resilience", "Magical" (ToUpper at display).
- Attribute codes and names: STR Strength, DEX Dexterity, END Endurance, CON Constitution, RES Resistance, DIS Discipline, INT Intelligence, WIS Wisdom, CHA Charisma.
- Resources: "Health", "Stamina", "Spell Points", "Posture". "Skill Points". "Level". "Exhausted".
- Statuses (proposed): the label "Statuses" (the row's accessible name is "Active statuses"); the empty state "None."; chip names and times are StatusChip's ("Frostbitten", "5.1", "Focused", "3/3"). World voice: no Tally remark on the row.
- Primary-derived: "Physical Damage", "Attack Speed", "Affliction Resist", "Magic Damage", "Cast Speed".
- Secondary-derived: "Armor Penetration", "Physical Crit Chance", "Stamina Regen", "Health Regen", "Affliction Recovery", "Posture Recovery", "Spell Points Regen", "Resistance Penetration", "Magic Crit Chance".
- Receipt kickers: "Derived · primary", "Derived · secondary" (Tally voice; no remark line on this screen). TODO(DECISION): friendlier player-facing names for the two groups.

## Do / Don't
- Do keep all nine cards the same size; the band meter, not the card, shows the diminishing returns.
- Do keep crit chances unsigned and raw; every other derived row is signed.
- Do size each number box for its longest value so nothing shifts as values change.
- Do draw the Stamina debt from the left, the same widget and rule as the HUD gauge.
- Do use the one skill-point badge (SkillsTab's) here; never a second coin style.
- Don't draw Spell Points as a bar, here or anywhere.
- Don't colour attributes by group; the headings carry the grouping.
- Don't put text straight on the receipt's torn foot or on the band wells.
- Don't underline linked receipt rows; the band, the weight and the tick carry the link.
- Don't show resistances, immunities or Player Stability rows until those stats exist in the attribute set.
