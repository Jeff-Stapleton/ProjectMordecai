# Bar

The meter: a sunk `well` behind a 2 px lip, with a fill plus the trail, heal lead, notches and patterns that let each resource read without its colour. Use it for player Health, Stamina and Posture, and for enemy and boss health and posture; Spell Points are counted as tokens (see Tally), with a bar only as the fallback above 16.

## Anatomy
- **Well**: `well`, radius 6. Empty space always shows, so a bar's full length is visible at any value.
- **Lip**: 2 px `lip` on plates (3.45:1 on `plate`, 4.74:1 on `well`). Thin bars use a 1 px lip.
- **World edge** (core `.tt-bar--world`): a bar standing straight on a floor with no frame of its own takes `rim` as its lip and a 2 px `ink` ring outside it, so one of the two holds on every floor. No shipping meter does this today: enemy lanes sit in EnemyTag's framed gauge and boss bars in BossBar's plate.
- **Fill**: radius 3, one colour per resource:
  - `vital`: player Health
  - `stamina-green`, `stamina-yellow`, `stamina-red`: the Stamina tiers
  - `posture`: Posture
  - `foe`: enemy and boss health. One colour at every level; low health shows by the trail and the 25% notch, not a hue shift.
- **Trail** (`.tt-bar__trail`, `--t`): `vital-trail`, behind the fill on Health, enemy and boss health only. Motion: DamageTrail.
- **Heal lead** (`.tt-bar__lead`, `--l`): `heal`, ahead of the fill while healing. Player Health only. Motion: HealLead.
- **Notch** (`.tt-bar__notch`, `--at`): a 2 px `ink` line with 1 px `lip` sides, full height.
  - Health (player, enemy, boss): 25%.
  - Stamina: 33% and 66%.
- **Stripes** (`.is-striped`): `ink` at 35%, −45°, 6 on 4. Stamina Red tier only.
- **Debt hatch** (core `.tt-bar__debt`, `--d`; widget `StaminaDebtBar`): thin `danger` lines at +45° on `well`, anchored LEFT, closed by a solid 2 px `danger` edge on its right, where regen bites.
- **Player posture broken** (`.is-posture-broken`): the `posture` fill floods 100% under an `ink` hatch at +45° and the lip turns `rim`. No red, no chip.
- **Break flood** (`.is-broken`): `good` and `ink` stripes at −45° fill an enemy or boss posture lane.

### Sizes at 1920 x 1080 (the well, lip included; frames and outlines sit outside)
The component that places a meter owns its size and its edge. This table copies them; when they change, the owner wins.

| Meter | Size | Edge | Owner |
|---|---|---|---|
| Player Health (`--hud`) | 268 x 24 | 2 px `lip` | GaugePlate, HP row |
| Player Stamina (`--hud-st`) | 268 x 18 | 2 px `lip` | GaugePlate, ST row |
| Player Posture (`--hud-thin`) | 268 x 8 | 1 px `lip` (`rim` while broken) | GaugePlate, posture row |
| Enemy health lane | 136 x 10 | 2 px `lip`, inside the tag's framed gauge (3 px `ink` outline, 2 px `rim` line, 2 px `ink` round the lanes) | EnemyTag gauge (144 x 25) |
| Enemy posture lane | 136 x 5 (8 while broken) | 1 px `lip`, same frame | EnemyTag gauge |
| Boss health | 848 x 22 | 2 px `lip`: a plain well inside the plate | BossBar plate (880 wide, x 520 to 1400) |
| Boss posture | 848 x 8 | 1 px `lip`: a plain well inside the plate | BossBar plate |

Inside a plate or a framed gauge the meter never takes the world edge: the plate or frame already carries the dual edge, and a cream rim around the well would swallow the trail's edges.

## States
- **Damage** (Motion: DamageTrail):
  - The fill snaps to the new value.
  - The trail holds at the old value, then drains to the fill.
  - A hit during the hold restarts the hold from where the trail is; the trail never jumps down.
- **Heal** (player Health, Motion: HealLead): the `heal` lead snaps to the new value, the fill follows, and the lead hides once the fill meets it.
- **Stamina tiers**: the colour, the notches, the multiplier pill and the pattern change together, instantly, at the HUD thresholds:

| Tier | Range | Fill | Pill |
|---|---|---|---|
| Green | 66% and up | `stamina-green`, solid | ×1.25 |
| Yellow | 33% to 66% | `stamina-yellow`, solid | ×1.0 |
| Red | above 0, under 33% | `stamina-red`, striped | ×0.75 |
| Exhausted | 0 and below | no fill, the debt hatch (`.tt-bar--exhausted`) | ×0.5 (`.tt-pill--exhausted`) |

  - Stamina has no trail: it moves with the attribute.
- **Exhausted debt** (D2, the system rule):
  - The hatch is anchored LEFT; its width is |Stamina| / MaxStamina (−26 of 134 = 19%).
  - Regen eats it from its right edge. When it is gone, the fill grows from the same left edge: one scale for debt and fill.
  - The floor is an ABSOLUTE −50 (`UMordecaiStaminaSystem::StaminaFloor`, and the attribute clamp in `UMordecaiAttributeSet`), so the deepest debt is 50 / MaxStamina: 37% at the default MaxStamina of 134 (MaxStamina = 100 x (1 + 0.04 x EffEnd)).
  - Stamina does not regenerate today (N30), so after a fight the bar sits at the floor: 37% at the default.
  - It is a dark ground with thin red lines, the reverse of the Red tier's bright fill with dark stripes, so the two never read alike.
- **Posture**:
  - Player (D3): while `Mordecai.State.PostureBroken` is on the player, the posture hairline floods with `posture` under an `ink` hatch and its lip turns `rim` (`.is-posture-broken`). No red, no chip: red means threat only. `TODO(DECISION)`: the player has no posture-break logic yet; only enemies break.
  - Enemy and boss: a break floods the posture lane with the `good` / `ink` stripes (`.is-broken`), which drain over the stagger duration: 1 s Posture Broken, 2 s Knocked Down. One rule for both; EnemyTag and BossBar draw it in their lanes.
- **Boss phases**: boss phases 1 / 2 / 3 exist, but their health thresholds are not defined. `TODO(DECISION)`; add no phase notches until they are. Phases are BossBar's pips, never marks on the bar.

## Data and implementation
- **Existing widgets and the names to keep**:
  - `UMordecaiHealthBarWidget`: `HealthProgressBar`.
  - `UMordecaiStaminaBarWidget`: `StaminaProgressBar`.
    - Fill its `GreenTierColor`, `YellowTierColor`, `RedTierColor` and `ExhaustedTierColor` (= `danger`, the debt lines) from the theme asset.
    - `EMordecaiStaminaTier` and the `MordecaiHUDConstants` thresholds stay.
  - `UMordecaiPostureBarWidget`: `PostureProgressBar`, with `NormalPostureColor` = `posture` and `BrokenPostureColor` = `posture` (the broken look is the hatch and the rim lip, not a colour change).
  - `UMordecaiEnemyIndicatorWidget` (EnemyTag): `HealthProgressBar`. Its green / yellow / red health thresholds are retired: `foe` at every level. EnemyTag specifies its lanes.
- **Replace the default UProgressBar look** (light grey track, tinted fill). Each named UProgressBar becomes the FILL layer only:
  - `WidgetStyle.BackgroundImage`: DrawAs NoDrawType.
  - `FillImage`: RoundedBox radius 3, white.
  - `FillColorAndOpacity`: the token.
  - `BarFillType`: LeftToRight. No marquee.
- **Build each meter as an Overlay**, bottom to top:
  1. Well: the `03` Well (a Border, RoundedBox radius 6, `well` fill, 2 px `lip` outline).
  2. An inner Overlay with `Clipping = ClipToBounds` and 2 px padding (1 px on thin bars). It holds:
     - the trail: new UProgressBar `HealthTrailBar`, `vital-trail`;
     - the lead: new UProgressBar `HealthLeadBar`, `heal`;
     - the named fill bar;
     - the debt: new UProgressBar `StaminaDebtBar`, `BarFillType` LeftToRight, `FillImage` = `M_UI_PatternFill` (the debt hatch), `Percent` = clamp(|Stamina| / MaxStamina, 0, 1) while Stamina < 0, else hidden;
     - the notches: Images 2 px wide in a CanvasPanel, anchors min.x = max.x = 0.25 / 0.33 / 0.66.
- **Patterns**: every pattern is `M_UI_PatternFill` (the `03` asset manifest), sampling `T_UI_Hatch16` and tiling in screen pixels (`GetPixelPosition`), not UVs, so the stripes never stretch. Parameters: Base, Line, Angle, Period, LineWidth. Set it as the FillImage of a UProgressBar.
  - Red tier: Base `stamina-red`, Line `ink` 35%, −45°.
  - Debt: Base `well`, Line `danger`, +45°.
  - Player posture broken: Base `posture`, Line `ink`, +45°.
  - Break flood: Base `good`, Line `ink`, −45°.
- **GAS**: `UMordecaiAttributeSet` Health / MaxHealth, Stamina / MaxStamina, Posture / MaxPosture, through the attribute-change delegates the widgets already bind.
  - Trail and lead timing runs in `NativeTick` from the cached previous value (DamageTrail, HealLead).
  - Posture broken comes from the `Mordecai.State.PostureBroken` tag event; Knocked Down from `Mordecai.State.KnockedDown`.
- **Tokens**: `well`, `lip`, `rim`, `ink`, `vital`, `vital-trail`, `heal`, `stamina-green`, `stamina-yellow`, `stamina-red`, `danger`, `posture`, `foe`, `good`.

## Copy
Bars carry no text. The HP percentage, the stamina multiplier pill ("×1.25", "×1.0", "×0.75", "×0.5") and the SP "x / y" belong to the gauge plate. Enemy names belong to the enemy tag.

## Do / Don't
- Do keep the lip visible at every value, including empty and full.
- Do keep `foe` one colour at every enemy health level.
- Do pair every stamina tier with its pill and pattern, not colour alone.
- Do draw the debt from the left on the fill's own scale, and the deepest debt at 37% (default MaxStamina), not 50%.
- Do cite DamageTrail and HealLead; never write local timings.
- Don't shift health colour with value (no green, yellow, red health bars).
- Don't use red or `danger` for player posture.
- Don't put a trail on stamina or posture, or a lead on anything but player Health.
- Don't give a meter inside a plate or a framed gauge the world edge.
- Don't draw Spell Points as a bar while MaxSpellPoints is 16 or less (the `SpellPointsProgressBar` fallback is above 16, `TODO(DECISION)` US-090), and don't add gloss, gradients or glow to a fill. Patterns come only from `M_UI_PatternFill`.
