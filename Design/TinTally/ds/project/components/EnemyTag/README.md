# EnemyTag

The world tag over an enemy: tier diamonds and name on a plate pill, foe health over posture in one dual-edged gauge, a state chip and up to three status tokens. Use it for every non-boss enemy once it has aggroed or been hit; it replaces the plain red `WBP_EnemyHealthBar`, and bosses use the BossBar instead.

## Anatomy

Pivot: bottom-centre, anchored at Z +120 above the enemy origin. The gauge is the fixed core of the map's 150 px box (144 px, 150 with its outline); the name pill hugs its name and centres over the gauge, so it may be wider (measured: Frontliner 117, Skeletal Archer 169, Goblin Skirmisher 193, Corrupted Knight with its diamond 211, a Miniboss ribbon 323). The tag is 67 high without statuses, 107 with them (108 with the state chip). Rows top to bottom, 10 px apart (room for both outlines):

| Row | Spec |
|---|---|
| Name pill | 32 high, radius pill, `plate` fill, 2 px `rim` (top and left) / `rim-shade` (bottom and right), 3 px `ink` outline + print shadow. Holds the tier diamonds, then NameText in `ui-label` (20/26, 600), `text`. |
| Tier diamonds | 11 px squares turned 45 deg, `good` with a 2 px `ink` edge, 7 px apart, left of the name. Basic: none. Elite: 1. Champion: 2. Miniboss: 3, and the pill rides a cream ribbon (`rim` with a 3 px `ink` edge, swallowtail ends, 24 px beyond each side). Summon: no diamonds (see Summon mark). Boss: no tag. |
| Summon mark | The `summon` glyph (20 px, `text-muted`) before the name, and a lighter pill fill: `plate` mixed 18% toward `rim` (text 7.6:1 on it, 13:1 in High Contrast). The rim stays solid: a dashed edge means unavailable (locked, disabled, unbound) and nothing else. |
| Gauge | One framed piece with the dual edge: a 3 px `ink` outline outside (4 in High Contrast), a 2 px `rim` line inside it, then 2 px of `ink` round two lanes 2 px apart. Radius 8, print shadow below. 144 x 25 (150 x 31 with the outline), 144 x 28 while broken. On blockout, sand and snow the ink outline holds; on mire and night, where ink alone is 2.5:1 and 1.2:1 and the lanes' `lip` 1.9:1 on mire, the `rim` line holds. |
| Health lane | 136 x 10, `well` with a 2 px `lip`. Fill `foe` at every health level. Damage trail `vital-trail`. A 2 px `ink` notch at 25%. |
| Posture lane | 136 x 5, `well` with a 1 px `lip`, fill `posture`. Opens to 8 px while broken or down (see States). |
| State chip | 28 high, radius pill, `good` fill, `ink` text (`ui-label` weight 700, multiplier in Mono), 3 px `ink` edge. |
| Status tokens | 30 px discs, glyph 20 px, 5 px apart, at most 3 + a "+N" token in `num-s`. Debuff: `plate` fill, 2 px `debuff` rim, `text` glyph. Buff: `enamel` fill, `ink` glyph, 2 px inner `buff` ring. Light vs dark ground carries the polarity without colour. |
| Critical brackets | Four 20 px corners, 5 px `good` strokes with a 2 px `ink` edge, framing the enemy's projected bounds + 12 px. |
| Riposte tag | A plain `enamel` tag, 40 high: the light-attack keycap + "Riposte" in `ui-label`, `ink`. On keyboard and mouse the cap is Keycap's drawn mouse (`.tt-key--mouse`, left button lit), never the letters "LMB"; on gamepad it is the RT trigger cap (control_bindings_v1.1: Light Attack = RT / Left Mouse). It sits right of the brackets, vertically centred. |

## States

| State | When | What shows |
|---|---|---|
| Hidden | Basic enemy, not aggroed and never hit | Nothing. |
| Basic after aggro | First aggro (AI enters Approach) or first damage | Name pill + gauge. |
| Hit | Health drops | The trail segment follows the DamageTrail motion (the same as the player's bar and the boss bar). |
| Under 25% | Health below the notch | No hue change; the fill sits left of the notch. |
| Statuses | Any of the 16 watched statuses | Up to 3 tokens, sorted Control > DoT > Debuff > Environmental > Buff, then "+N". |
| Elite and up | Tier Elite, Champion or Miniboss | Visible whenever within range, aggroed or not, with its diamonds (and the ribbon for Miniboss). |
| Summon | Tier Summon | The `summon` glyph before the name, the lighter pill. |
| Posture Broken | `Mordecai.State.PostureBroken` (1000 ms) | The posture lane opens to 8 px and floods with `good` / `ink` stripes that drain over the 1 s. Chip "Broken ×1.5". |
| Knocked Down | `Mordecai.State.KnockedDown` (50+ damage during the stagger; 2000 ms) | Same stripes, draining over 2 s. Chip "Down ×2.0". |
| Critical window | The enemy can be riposted (see the open item below) | Broken state + critical brackets + Riposte tag. |
| Far | Beyond 1400 cm from the player | Tier diamonds + the health lane only (112 px). No name, posture, chip or statuses. |
| Dead | `Mordecai.State.Dead` | The tag is removed at once. An empty bar never stays on a corpse. |

Motion: the trail is DamageTrail. The tag fades in on first aggro and out at the leash with WorldTagFade (whole-widget fades). Diamonds never animate. The stripes drain linearly over the stagger. The brackets snap in with BracketSnap. Reduce Motion: the system's 150 ms fades only.

## Data and implementation

- **Widget:** `UMordecaiEnemyIndicatorWidget` (implements `IIndicatorWidgetInterface`).
  - Keep the BindWidgetOptional names `NameText`, `HealthProgressBar` and `StatusIconContainer`.
  - Add optional `TierMarks` (HBox of diamond images), `PostureProgressBar`, `StateChip` with `StateText`, `CriticalBrackets` and `RiposteTag`.
- **Component:** `UMordecaiEnemyIndicatorComponent`.
  - `IndicatorHeightOffset` 120, `IndicatorVisibilityRange` 2000 (checked every 0.25 s), HAlign Center / VAlign Bottom. Add the far threshold at 1400 cm.
  - It needs US-092 to appear at all (N17): a `ULyraIndicatorManagerComponent` on the player controller, a `UIndicatorLayer` on the HUD, `IndicatorWidgetClass` set, and a caller for `NotifyAggroActivated`.
- **Retire** `WBP_EnemyHealthBar` (the 150 x 20 screen-space `UWidgetComponent` from `CreateHealthBarWidget()`), so each enemy has one health readout. US-092 lists this as TODO(DECISION); this design assumes removal.
- **Name:** `EnemyDisplayName` only, never `GetName()`.
  - No enemy sets a display name in content today, so the actor name ("BP_MordecaiEnemy_Frontliner_C_0") would show.
  - Until the names are set, collapse the pill and show diamonds + gauge.
- **Health:** Health / MaxHealth from the enemy ASC.
  - Replace `HealthColorGreen`, `HealthColorYellow` and `HealthColorRed` with the single `foe` token. Low health reads from the notch and the trail.
- **Posture:** Posture / MaxPosture (`UMordecaiAttributeSet`). The numbers come from `MordecaiPostureSystem`:
  - Break at 0: `PostureBrokenDurationMs` 1000, `StaggeredDamageMultiplier` 1.5.
  - Knock-down: 50 damage during the stagger, `KnockDownDurationMs` 2000, x2.0.
  - Regen 10/s after 3000 ms.
  - The chip text is built from these values, not typed in.
- **Tier:** from the enemy definition's `Tier` (Basic, Elite, Champion, Miniboss, Boss, Summon). Today only Basic Frontliners and the test dummies exist.
  - The Champion, Miniboss and Summon names in the preview (Sentinel Frame, Drowned Automaton, Echo Clone) are taken from the world roster, but their tiers are examples.
- **Statuses:** the 16 watched tags (Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched, Focused).
  - `StatusIconMap` (empty today) maps each to the glyph of the same name.
  - Tokens replace the 16 x 16 icons. Pillar 8 ("not icon soup") is why the cap is 3.
- **Critical window:** TODO(DECISION). `State.Riposte` and `State.Parried` are never removed (N33), no duration is specified, and no dedicated riposte attack exists. Proposed rule:
  - Show while the local player has `State.Riposte` and this enemy is the parried or posture-broken target, within 300 cm (example value).
  - The keycap shows the light attack (the drawn left-mouse cap / RT). "Riposte" is example copy until the riposte exists.
- **Sizes and type:**
  - At 1920 x 1080: name 15 pt (20 px), chip 15 pt, "+N" Mono 15 pt.
  - Brushes (03's manifest): the name pill and state chip are the pill stack; the gauge is a RoundedBox stack (ink outline, rim line, ink face) around two wells; the break stripes are `M_UI_PatternFill` (`good` and `ink`). Many tags on screen may use 03's SDF rounded-rect material instead of the Overlay stack. The tier diamond is a 45-degree RoundedBox Image (`good` face, `ink` edge), no texture. The Miniboss ribbon is a two-layer silhouette like 03's Badges (an `ink` edge layer, a `rim` face), proposed for 03's Badges row.
- **Tokens:** `plate`, `rim`, `rim-shade`, `ink`, `text`, `text-muted`, `good`, `foe`, `vital-trail`, `well`, `lip`, `posture`, `debuff`, `buff`, `enamel`, `ui-label`, `num-s`.
- **Glyphs:** the 16 status glyphs, `summon`.

## Copy

- NameText: the enemy's display name, Title Case as authored ("Corrupted Knight", "Goblin Skirmisher", "Skeletal Archer", "Frontliner").
- State chip: "Broken ×1.5", "Down ×2.0". The multiplier is Mono, built from data.
- Riposte tag: "Riposte" (example copy).
- Overflow token: "+N" (N = hidden statuses).
- No Tally remarks on enemy tags.

## Do / Don't

- Do let the number of diamonds carry the tier; their colour is decoration.
- Do keep the gauge one piece: both lanes inside one ink frame.
- Do drop to the far form past 1400 cm instead of shrinking the whole tag.
- Don't change the health fill colour with health.
- Don't print status names or durations on enemy tags; the token glyph is enough, and the player's own chips carry names.
- Don't show more than 3 status tokens.
- Don't give a boss an EnemyTag; it gets the BossBar.
- Don't show the actor's internal name, ever.
- Don't mark a summon with a dashed rim; dashed means unavailable.
- Don't spell a mouse button ("LMB"); draw the mouse cap.
