# DamageNumber

A floating damage or heal number at the hit point: the amount in pop letters inside an ink outline, coloured by damage type, with a type stamp after the digits for every non-Physical type. Use it for every hit that changes Health; evaded and parried hits show no number.

## Anatomy

| Part | Spec |
|---|---|
| Digits | `pop` (Rowdies 700, 30/30), fill = the type token, 3 px `ink` outline (4 px in High Contrast), hard (0, 3) `ink` print shadow. Proportional digits are fine: pops never tick in place. |
| Stamp | An `ink` capsule, 30 px high, min 30 wide, radius pill. It sits 3 px after the digits and holds the type glyph (22 px) in the type colour, plus at most one modifier glyph in `text`. Physical, Slash, Pierce and Blunt have no type glyph, so a plain Physical pop has no stamp at all, and a blocked, weak or resisted Physical pop shows its modifier glyph alone. |
| Crit | Digits at 45 px (x1.5) with a "!" suffix, on a 96 px 12-point starburst: `good` face with a 6 px `ink` edge, centred behind the digits. A non-Physical crit's stamp keeps its 30 px size and sits 8 px further out, clear of the burst's points. |
| Heal | `heal` fill, always prefixed "+", stamp glyph `heal`. |
| Blocked | The mitigated amount at 24 px (80%) with the `block` glyph in the stamp. |
| Weakness / resisted | The stamp adds `arrow-up` (the target is weak to the type, so the hit did more) or `arrow-down` (it resisted). |
| Immune | The word "Immune" in `text-muted` letters, with the attempted type glyph and the `immune` glyph. |
| Shadow (Curse) | Inverted print: `dmg-shadow` fill inside a 3 px `dmg-shadow-edge` outline (4 px in High Contrast), closed by a 2 px `ink` keyline outside it so the light edge never melts into a light floor: the dual edge, inverted. The stamp inverts too: a `dmg-shadow-edge` capsule, `dmg-shadow` glyph, 2 px `ink` ring. Its silhouette is unlike any other number. |
| Radiant (Holy) | `dmg-radiant` fill, a 2 px `good` halo, then the 3 px `ink` outline. The stamp gets a 2 px inner `good` ring. |

Type map (colour token / stamp glyph):

| Damage tag | Colour | Glyph |
|---|---|---|
| `Mordecai.Damage.Physical` and its subtypes `.Slash`, `.Pierce`, `.Blunt`, and untagged damage | `dmg-physical` | none (no stamp) |
| `Mordecai.Damage.Fire` | `dmg-fire` | `fire` |
| `Mordecai.Damage.Frost` | `dmg-frost` | `frost` |
| `Mordecai.Damage.Lightning` | `dmg-lightning` | `lightning` |
| `Mordecai.Damage.Poison` | `dmg-poison` | `poison` |
| `Mordecai.Damage.Corrosive` | `dmg-corrosive` | `corrosive` |
| `Mordecai.Damage.Arcane` | `dmg-arcane` | `arcane` |
| `Mordecai.Damage.Shadow` (spec: Curse, Dark) | `dmg-shadow` + `dmg-shadow-edge` | `shadow` |
| `Mordecai.Damage.Radiant` (spec: Holy) | `dmg-radiant` + `good` halo | `radiant` |
| Healing | `heal` | `heal` |

Physical numbers carry no type glyph (the `dmg-physical` token note and the brand book's colour rule): melee is the common case, and the `dmg-physical` colour (`text`) with no stamp marks it, which is itself a shape signal against every stamped type. The `physical`, `slash`, `pierce` and `blunt` glyphs are never drawn on a number. TODO(DECISION): review in busy melee fights whether the Slash / Pierce / Blunt subtype should show after all (for example, against enemies that resist one subtype).

## States

| State | When | What shows |
|---|---|---|
| Normal | A hit lands | Digits, plus the type stamp for a non-Physical hit. |
| Crit | `bIsCritical` | 45 px digits, "!", burst. |
| Heal | Healing applied (Restoration ticks +8 every 1.0 s, 40 over 5 s) | "+N" in `heal`. |
| Blocked | The target was guarding (`EMordecaiHitDefense::Blocked` or `PerfectBlocked`) | The mitigated amount at 80% with `block`. A perfect block still shows the halved amount. |
| Weakness | Resolved resistance for the type is below 0 | `arrow-up` in the stamp. |
| Resisted | Resolved resistance for the type is above 0 | `arrow-down` in the stamp. |
| Immune | The hit's type is fully negated (resistance 1.0) | "Immune" + type + `immune`. |
| Evaded / parried | i-frames (`State.Dodging`, `State.Invulnerable`) or an active parry | **No number.** The hit is negated in full (design log 2026-10-30), and the callout lane reports the perfect action instead. |

Motion: DamagePop, the brand book's Motion row, times the pop in, the crit and its burst, the rise, the fan's drift and the fade, and gives the Reduce Motion form. It keeps the brand book's combat rule (at most 160 ms in).
- **Fan (several pops on one target, the first preview scene):** a light chain lands a hit every few hundred ms, and rise plus scatter alone pile the numbers into one column. So pops on the same target alternate sides of its centre line (left, right, left...) and drift outward (DamagePop's drift) while they rise. A combo opens into a V: the newest number sits at the hit point, the older ones further out and higher on alternating sides.
- **Never over the tag:** the fan carries pops past the edges of the target's EnemyTag gauge (146 px). A pop that would still cross the tag stops rising 8 px under the tag's bottom edge and fades there. Pops may cross the enemy's body; they never cover a health readout.
- **Reduce Motion:** DamagePop's Reduce Motion form: no overshoot, burst turn or drift.

## Data and implementation

- **Component:** `UMordecaiDamagePopComponent` (extends `ULyraNumberPopComponent`, lives on the player controller).
  - It already stores `FMordecaiDamagePopEntry` records, and its `PopLifespanSec`, `PopRiseSpeed` and `PopScatterRadius` defaults already match DamagePop's lifespan, rise and scatter. Also `PopScale` 1.0, `CriticalHitScale` 1.5 and `MaxConcurrentPops` 20 (the oldest is recycled).
  - It renders nothing today: the Lyra `AddNumberPop` base is empty.
  - Add for the fan: `PopDriftSpeed` (cm/s, outward; DamagePop's drift), a per-target side flag that flips on every pop (key the target actor in a `TMap<TWeakObjectPtr<AActor>, bool>`), and at most 5 live pops per target (a sixth recycles that target's oldest). The values are starting points for tuning in the arena.
- **Render path (03, "Damage pops"):** a pool of 20 `UMordecaiDamagePopWidget` (new, UMG) on the damage-pop canvas, a child of the HUD root (`UMordecaiCombatHUDWidget`, pushed once on `UI.Layer.Game`; never a separate push).
  - Each frame: project the entry's world position (rising, scattered, drifting) with `ProjectWorldLocationToScreen`, then set the render translation. Clamp the screen y under the target's EnemyTag (its indicator widget's cached geometry) when the pop overlaps it horizontally.
  - Each widget: an HBox holding an Overlay [Burst image, DigitsText, HaloText] and a StampBorder [type Image, modifier Image]. StampBorder is Collapsed when both Images are empty (a plain Physical hit).
  - Pool and reuse; never construct widgets per hit.
  - No Niagara text and no MeshText (03): they cannot draw the stamp, the burst or the inverted Shadow print.
- **DigitsText:**
  - F_Tally_Display Bold, 22.5 pt (30 px). Crit 34 pt (45 px). Blocked 18 pt (24 px).
  - `OutlineSettings.OutlineSize` 3 (4 in High Contrast). OutlineColor `ink`, or `dmg-shadow-edge` for Shadow.
  - ShadowOffset (0, 3), `ink`.
- **HaloText** (Radiant and Shadow): a second TextBlock behind DigitsText with the same string, outline 5 in `ink` (6 in High Contrast), print (0, 3) `ink`. DigitsText then drops its own print.
  - Radiant: DigitsText outline 2 in `good`, so the ink shows 3 px outside the gold halo.
  - Shadow: DigitsText outline 3 in `dmg-shadow-edge` (4 in High Contrast), so the ink shows as a 2 px keyline outside the light edge.
- **Burst:** a 96 x 96 twelve-point starburst built like 03's Badges: an `ink` edge layer under a face layer tinted `good` (the face inset 6 px). Proposed for 03's Badges row as the crit burst; no separate texture name until it is listed there.
- **Stamp:** a Border with the pill stack (03) in `ink` (Shadow: `dmg-shadow-edge`). Glyph images 22 x 22 (20 x 20 when blocked), tinted per the type map.
- **Colours:** replace `DamageTypeColors` and `HealingColor` (linear literals today, so Fire displays #FFBC00) with the tokens above, from the theme asset. Author the sRGB hex and convert with `FLinearColor::FromSRGBColor`. Untagged damage uses the Physical style, not grey.
- **Entry fields to add:** bBlocked, ResistanceSign (-1 / 0 / +1), bImmune.
  - The number stays "the health actually taken".
  - Crits exist as a flag, but no crit roll exists yet: every pop is fired with `bIsCritical=false`. The crit look is ready for when crits roll.
  - TODO(DECISION): show the arrows for every non-zero resistance, or only from a threshold (example: 10%)?
  - TODO(DECISION): which immunity counts as "Immune": full damage-type negation only, or also `Mordecai.Immunity.*` status immunities on the payload.
- **Whose numbers:** the local player's outgoing hits and heals. TODO(DECISION): whether damage taken by the player pops too; if so, it uses the same styles.
- **Setting:** Settings > Accessibility > "Damage numbers" All / Mine / Off (OptionsScreen's table is the single list). Mine shows only the local player's hits and heals. The setting does not exist yet (Settings is "Coming Soon"); All is the default.
- The agent bridge sees no pop events today (B2), so this component is verified by reading screenshots.
- **Tokens:** `dmg-physical`, `dmg-fire`, `dmg-frost`, `dmg-lightning`, `dmg-poison`, `dmg-corrosive`, `dmg-arcane`, `dmg-shadow`, `dmg-shadow-edge`, `dmg-radiant`, `heal`, `good`, `ink`, `text`, `text-muted`, `pop`.
- **Glyphs:** `fire`, `frost`, `lightning`, `poison`, `corrosive`, `arcane`, `shadow`, `radiant`, `heal`, `block`, `immune`, `arrow-up`, `arrow-down`. Never `physical`, `slash`, `pierce` or `blunt` on a number.

## Copy

- Digits only, formatted as integers ("14", "+40"). Round half up; never show decimals.
- Crit appends "!" ("42!").
- Heals always start with "+". Damage never carries a sign.
- "Immune" is the only word a pop may show.
- No Tally remarks.

## Do / Don't

- Do keep the stamp attached to the digits; it is part of the number, not a separate icon.
- Don't put a type glyph on a Physical number; colour and the missing stamp mark melee.
- Do let the burst be the only crit backing; never add glows or a second outline colour.
- Do recycle the oldest pop past 20, as the component already does.
- Don't show a number (or "0", or "Miss") for evaded or parried hits.
- Don't use arrow, check or cross characters from the font; use the glyphs.
- Don't print light digits on a light edge for Shadow; it is always dark inside a light edge.
- Don't scale numbers by damage amount; only crit (x1.5) and blocked (x0.8) change the size.
