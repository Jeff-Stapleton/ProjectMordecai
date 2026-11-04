# GaugePlate

The always-on resource plate at the top-left of the HUD: Health with its percentage, Stamina with its tier and the sprint toggle, posture while it is below max, and Spell Points as tokens. Use exactly one, in the combat HUD; it is the player's own state, and it never moves, fades or resizes.

## Anatomy

Plate 456 x 152 design px, anchor top-left (0,0), offset (32, 32): x 32-488, y 32-184. `.tt-plate`: `plate` fill (100% opaque), 2 px `rim` on top and left, `rim-shade` on bottom and right, 3 px `ink` outline outside (4 px in High Contrast), hard print shadow (0, 4). Radius 12. No rivets (HUD plate).

Inside the rim: 14 px padding, so the content box is 424 x 120. Three columns with 10 px gutters: **label 28 | track 268 | value 108** (28 + 10 + 268 + 10 + 108 = 424). Four rows: **HP 34**, gap 8, **ST 28**, gap 7, **posture 8**, gap 7, **SP 28** (= 120).

| Row | Label (`num-s`, `text-muted`) | Track (268 px) | Value column (108 px) |
|---|---|---|---|
| HP | "HP" | 24 px bar: `well` + 2 px `lip`; `vital-trail` ghost, `heal` lead, `vital` fill; notch at 25% | `HealthPercentText` "100%", `num-l` 30 px, `text`, right-aligned: 4 Mono cells = 76 px |
| ST | "ST" | 18 px bar: tier fill or the debt hatch; notches at 33% and 66% | the sprint pip (20 x 20), 4 px, then `StaminaTierText` in a fixed 84 x 28 pill (Mono 700 20, `ink` on the tier colour; "×1.25" is 5 cells = 63 px) |
| Posture | none | 8 px hairline (core `.tt-bar--hair`: `posture` fill, 1 px `lip`) | none |
| SP | "SP" | the token row (below), or the bar fallback above 16 | `SpellPointsText` "13 / 13", `num-m` 24 px, `text`: 7 Mono cells = 106 px, the longest value |

- **Scale ticks.** Every notch is two parts: a 2 px `ink` line inside the bar (core `.tt-bar__notch`) and a 2 px `rim` tick printed on the plate that overhangs the well by 5 px above and below.
- **Sprint pip.** A fixed 20 x 20 box left of the tier pill. Off: a hollow 14 px pip ring (2 px `lip`). On: the 20 px `sprint` glyph (a boot with speed lines) in `text`. Off and on differ by shape.
- **Spell Point tokens** are the Tally partial's `.tt-tally-sp` discs: `spell-points` fill, 2 px `ink` ring and an `ink` punch slot at 12 o'clock (a ticket punch, not a gem); empty ones are `well` with a `lip` ring. They fill from the left and empty from the right, in groups of five. The row is designed for 10 to 16 tokens and always fits the 268 px track (D17):

  | MaxSpellPoints | Token | Gaps (in a five / between fives) | Width |
  |---|---|---|---|
  | up to 10 | 22 px | 4 / 12 | 10 = 264 px |
  | 11-13 (13 is the default) | 16 px (`.tt-tally-sp--16`) | 3 / 8 | 13 = 254 px |
  | 14-16 | 14 px (`.tt-tally-sp--14`) | 2 / 6 | 16 = 266 px |
  | above 16 | none: `SpellPointsProgressBar`, an 18 px `spell-points` bar in the same track | — | 268 px |

## States

| State | Trigger | What changes |
|---|---|---|
| Full (default) | Health = Max, Stamina >= 66%, Posture = Max | HP fill to the end, Green, posture row Hidden, every token lit: "13 / 13" at the default INT |
| Damage taken | Health drops | the fill snaps; the `vital-trail` ghost holds and drains (Motion: DamageTrail) |
| Healing | Health rises (Restoration ticks, potions) | the `heal` lead snaps to the new value and the fill follows (Motion: HealLead; GaugePlate owns it) |
| Green | Stamina / MaxStamina >= 0.66 | solid `stamina-green`, pill "×1.25" |
| Yellow | >= 0.33 | solid `stamina-yellow`, pill "×1.0" |
| Red | > 0 | `stamina-red` with 35% `ink` stripes (`.is-striped`), pill "×0.75" |
| Exhausted | Stamina <= 0 | no fill. The debt rule (D2): `StaminaDebtBar` is anchored LEFT and its width is \|Stamina\| / MaxStamina (−26 of 134 = 19%). Regen eats it from its right edge; once it is gone the tier fill grows from the same left edge, so debt and fill share one scale. The floor is an absolute −50, so the deepest debt is 50 / MaxStamina: 37% at the default 134. The pill inverts: `ink` ground, `danger-text` letters, `danger` border, "×0.5" |
| Guard broken | `Mordecai.State.GuardBroken` (Stamina reached 0 while blocking) | for the guard break (800 ms): a 28 px stamp across the right end of the ST track, in the Exhausted pill's colours (`ink` ground, 2 px `danger` border, `danger-text`): the `block` glyph 20 + "Guard broken" (UI 700 20 px). The debt hatch keeps showing at the left. Removed with the tag |
| Sprint on / off | Left Shift or L3 toggles sprint | on: the boot glyph; off: the hollow ring. The pip never hides, so a player always knows which way the toggle sits |
| Posture shown | Posture < MaxPosture | the hairline appears; it hides again 3 s after it refills. Hidden, never Collapsed, so the SP row never jumps |
| Posture broken | `Mordecai.State.PostureBroken` on the player | D3: the posture fill floods 100% under an `ink` hatch (Bar's `.tt-bar.is-posture-broken`) and the lip turns `rim`. No red and no chip: red means threat only. Kept apart from the Red stamina tier by height (8 vs 18 px), the hatch vs the stripes, the pewter ground and the "×0.75" pill. The player has no posture-break logic yet: `TODO(DECISION)` |
| Low health | Health / MaxHealth <= 0.25 | `HealthPercentText` turns `danger-text`; the plate's rim steps between `rim` and `vital` (Motion: LowHealthPulse); LowHealthFrame shows (below); the 25% notch is the fourth carrier |
| No Spell Points | SpellPoints = 0 | all tokens empty, "0 / 13" |
| More than 16 SP (example) | MaxSpellPoints >= 17 | the tokens collapse and `SpellPointsProgressBar` shows; "14 / 18" stays in its 7-cell box. `TODO(DECISION)` (US-090): it breaks "Spell Points are tokens" |

Colour is never the only signal. Stamina tier = colour + where the fill ends against the 33 / 66 notches + the pill + the pattern (solid, striped, debt hatch). Low health = number colour + rim step + edge frame + notch. Posture broken = the hatch + the rim. Guard broken = the words. Sprint = the glyph's shape.

**LowHealthFrame** (D21; the asset manifest's name). A screen-sized, HitTestInvisible Image on the HUD root canvas (not inside the gauge plate): an 8 px opaque `vital` band round the screen edge with a 3 px `ink` line along its inner side, Box-drawn with an 8 px margin. Shown while Health <= 25%. It is not a plate (no rim, no print) and it does not pulse. The left preview crop shows it along the screen's top and left edges.

**Reduce Motion:** the rim holds at `vital` (no step), the trail drains in one 150 ms step, the guard stamp appears and goes without a fade.

## Data and implementation

- **Container.** A new decor widget `GaugePlate` (a UBorder over the RoundedBox plate stack from the asset manifest; no plate textures) in the root CanvasPanel of `UMordecaiCombatHUDWidget`: anchor (0,0), offset (32,32), size 456 x 152, `bAutoSize` false. Nest the existing slots inside it; binding is by name, so nesting keeps them working: `HealthBar`, `StaminaBar`, `PostureBar`, `SpellPointsBar`.
- **HUD root (D20).** The HUD root is reparented to `ULyraActivatableWidget` and pushed once on `UI.Layer.Game`; GaugePlate and LowHealthFrame are children of its canvas and are never pushed on their own. The death screen, the only other push on that stack, hides them; re-read every attribute in `NativeOnActivated`.
- **Names to keep** (BindWidgetOptional): `HealthProgressBar` (UMordecaiHealthBarWidget), `StaminaProgressBar` (UMordecaiStaminaBarWidget), `PostureProgressBar` (UMordecaiPostureBarWidget), `SpellPointsProgressBar` and `SpellPointsText` (UMordecaiSpellPointsBarWidget). `SpellPointsText` stays a visible UTextBlock reading "%d / %d": the bridge compares "x / y".
- **New optional widgets** (each needs a C++ fallback in `BuildDefaultContent`):
  - `HealthPercentText` (UTextBlock) in UMordecaiHealthBarWidget: `FMath::CeilToInt(Health / MaxHealth * 100)` + "%", so a living player never reads "0%". Satisfies AC-052.2 (Health shown as %).
  - `HealthTrailBar` and `HealthLeadBar` (UProgressBar under the fill) for the trail ghost and the heal lead.
  - `StaminaTierText` (UTextBlock in a pill UBorder) in UMordecaiStaminaBarWidget, driven by `CalcStaminaTier` and the `MordecaiHUDConstants` thresholds.
  - `StaminaDebtBar` (D2): a UProgressBar, `BarFillType` LeftToRight, `FillImage` = `M_UI_PatternFill` (the debt hatch: `danger` lines on `well`), `Percent` = clamp(\|Stamina\| / MaxStamina, 0, 1) while Stamina < 0, under the same left edge as the fill.
  - `GuardBrokenStamp` (UBorder + `block` Image + UTextBlock "Guard broken") in UMordecaiStaminaBarWidget, Collapsed until `Mordecai.State.GuardBroken` is added and again when it is removed (`UMordecaiGA_Block::GuardBrokenDurationMs` = 800). Its class starts with "Mordecai", so the bridge can read "Guard broken".
  - `SprintPip` (UImage, two brushes: the hollow ring and the `sprint` glyph) in UMordecaiStaminaBarWidget. It reads `AMordecaiCharacter::bIsSprinting`. `SetSprinting` neither adds `Mordecai.State.Sprinting` (the tag is declared, never applied) nor broadcasts a delegate, so the widget has nothing to listen to: `TODO(DECISION)` add the loose tag in `SetSprinting` (or an `OnSprintChanged` delegate) rather than poll each frame.
  - `SpellPointTokens` (UHorizontalBox of UImage tokens in groups of five; 22, 16 and 14 px on/off brushes) in UMordecaiSpellPointsBarWidget. While it shows, collapse `SpellPointsProgressBar` but keep it in the tree.
- **Attributes** (UMordecaiAttributeSet, GAS change delegates): Health, MaxHealth, Stamina, MaxStamina (= 100 x (1 + 0.04 x EffEnd), 134 at default attributes; `UMordecaiStaminaSystem::StaminaFloor` −50, absolute), Posture, MaxPosture, SpellPoints, MaxSpellPoints (= 10 x (1 + 0.04 x EffInt), 13 at the default INT). **Tags:** `Mordecai.State.GuardBroken`, `Mordecai.State.PostureBroken`.
- **Colours.** Author the token hex as sRGB and convert with `FLinearColor::FromSRGBColor`. Fill the existing EditAnywhere UPROPERTYs from the theme DataAsset: `GreenTierColor` = `stamina-green`, `YellowTierColor` = `stamina-yellow`, `RedTierColor` = `stamina-red` (+ stripes), `ExhaustedTierColor` = `danger` (the debt hatch on `well`), `NormalPostureColor` = `posture`, `BrokenPostureColor` = `posture` (+ the `ink` hatch).
- **Assets** (the manifest in Building it in Unreal): the plate stack; `M_UI_PatternFill` (sampling `T_UI_Hatch16`) for the Red-tier stripes, the `StaminaDebtBar` hatch and the posture-broken hatch; `LowHealthFrame`.
- **Type.** Every number in Atkinson Hyperlegible Mono in a fixed box, `Justification` Right, `bAutoSize` false. UMG points = px x 0.75: `num-l` 30 px = 22.5 pt, `num-m` 24 px = 18 pt, `num-s` 20 px = 15 pt.
- **Tokens:** `plate`, `rim`, `rim-shade`, `ink`, `well`, `lip`, `vital`, `vital-trail`, `heal`, `stamina-green`, `stamina-yellow`, `stamina-red`, `danger`, `danger-text`, `posture`, `spell-points`, `text`, `text-muted`.
- **Known gaps in the game:**
  - Stamina never regenerates (N30), so after a fight the bar sits at the −50 floor: a 37% debt and "×0.5".
  - Spell Points do not regenerate (N6).
  - Sprint costs no stamina yet: `UMordecaiStaminaSystem::ComputeSprintDrain` (`SprintStaminaDrainRate` 10 / s) has no caller outside the tests. Sprint is client-local (N28).
  - `TODO(DECISION)`: does the player's posture break? Only enemies run break logic today, so the broken state (and a "Down" variant for `Mordecai.State.KnockedDown`) cannot be reached yet.
  - `TODO(DECISION)` (US-090): MaxSpellPoints above 16 (the bar fallback).
- **Example data in the preview:** the 16 and 18 Spell Point plates, and every Health / Stamina value, are examples; MaxStamina 134 and MaxSpellPoints 13 are the defaults.

## Copy

- Row labels: "HP", "ST", "SP".
- `HealthPercentText`: "100%" down to "1%".
- `StaminaTierText`: "×1.25", "×1.0", "×0.75", "×0.5" (U+00D7, present in Atkinson Hyperlegible Mono).
- `SpellPointsText`: "13 / 13" (shipping format "x / y", kept exactly).
- `GuardBrokenStamp`: "Guard broken" (new string, plain voice; the block phase's enum display name is "Guard Broken").
- The sprint pip has no text; its accessible name is "Sprint on" / "Sprint off".
- No Tally remarks on the gauge plate, ever.

## Do / Don't

- Do keep every value in its fixed box so the plate never shifts as numbers change.
- Do keep the posture row's space when it is hidden, and keep the sprint pip in place when sprint is off.
- Do draw the Exhausted debt from the left on the fill's scale; never let negative stamina read as an empty bar (today's N30 look).
- Don't tint Health by amount (no green-to-red); low health is told by the number, the rim step, the frame and the notch.
- Don't use `danger` or a chip for the player's broken posture.
- Don't print numbers inside the bars, and don't add glows, gradients, rivets or plate transparency: the low-health pulse is a colour step, and LowHealthFrame is an opaque band, not a plate.
