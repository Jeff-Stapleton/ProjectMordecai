# BossBar

The boss's bar at top-centre: the boss name on a nameplate, phase pips, health with a damage trail, and posture. Show it from the boss intro to the boss's death; it is World voice, so it carries no Tally remark, no rivets and no tally strokes.

## Anatomy

Box: anchor top-centre (0.5, 0), offset (0, 32), alignment (0.5, 0), 880 x 96, so x 520-1400 at 1920 x 1080. It clears the gauge plate (x 32-488) and the rail head (x 1568-1888). While it shows, the Plate lane drops to (0, 168), and the Banner's phase card lands under it at (0, 296).

| Part | Spec |
|---|---|
| Bar plate | 880 x 62 at the bottom of the box. `plate`, 2 px `rim` / `rim-shade`, radius 12, 3 px `ink` outline + print shadow. Padding 19 top (room for the tabs' overlap) and 14 at the sides; the two lanes sit 5 px apart and end 4 px above the bottom rim. |
| Health | 848 x 22, `well` with a 2 px `lip`. Fill `foe`, trail `vital-trail`, a 2 px `ink` notch at 25%. A plain well (core `.tt-bar`): the bar plate already carries the dual edge, so neither lane takes the world edge (`.tt-bar--world`), whose cream rim would swallow the cream trail. |
| Posture | 848 x 8, `well` with a 1 px `lip`, fill `posture`. |
| Nameplate | A raised plate tab, 48 high, at x 22 from the bar's left edge, overlapping the bar plate by 14 px. BossNameText in `world-heading` (Atkinson Next 800, 40 px, tracking 0.05 em), ToUpper, `text`, line height 40. |
| Phase tab | A plate tab, 42 high, 22 from the right edge, top 6. The phase word ("PHASE 2", 20 px, 700, tracking 0.06 em, ToUpper, `text-muted`), then 3 pips (core `.tt-pips--lg`: 20 px, 8 apart, 76 px in all): a filled `text` disc for each phase reached, a hollow 3 px `lip` ring for each phase ahead, so reached and ahead differ by shape. Then at most one state chip. Never tally strokes: the tally mark is for ranks, rarity and the seal / throbber only. |
| Enraged chip | 28 high, radius pill, `danger` fill, `ink` text, 800 20 px tracked caps. Phase 3 only. |
| Broken chip | Same shape, `good` fill, "BROKEN ×1.5" or "DOWN ×2.0" (multiplier in Mono, not cased). While it shows it takes the Enraged chip's place and the phase word collapses (see Sizes); the phase still reads from the pips and, in phase 3, their `danger-text` colour. |

## States

| State | When | What shows |
|---|---|---|
| Phase 1 | Fight start | 1 of 3 pips filled. |
| Phase 2 | The boss enters phase 2 ("AoE attacks unlocked") | 2 of 3 pips. The new pip fills as the Banner's phase card ("PHASE 2") lands below. |
| Phase 3, enraged | The boss enters phase 3 ("Enrage mechanics") | 3 of 3, the pips turn `danger-text`, and the "ENRAGED" chip appears (the Banner shows "ENRAGED"). |
| Hit | Health drops | The `vital-trail` ghost follows the DamageTrail motion. |
| Posture broken | Boss `Mordecai.State.PostureBroken` | The posture lane floods with `good` / `ink` stripes that drain over the stagger (1000 ms), and the "BROKEN ×1.5" chip shows in the phase tab while the phase word collapses. Knocked Down uses the same lane with "DOWN ×2.0". In phase 3 the Broken or Down chip replaces "ENRAGED" until the stagger ends (see the last preview state). |
| Defeated | Boss Health reaches 0 | The bar drains to empty, then the whole unit leaves with BossOut (a hold, then a whole-widget fade; the plate fill stays opaque), and the Plate lane returns to (0, 40). |

Phase changes are announced by the Banner's phase card (560 x 96 at (0, 296), under the bar), not by this component. Motion: the trail is DamageTrail; the unit drops in on the intro with PlateDrop's in-motion, after the intro banner clears, and leaves with BossOut. Reduce Motion: the system's 150 ms fades.

## Data and implementation

- **Widget:** new `UMordecaiBossBarWidget` (`UUserWidget`), a child of `UMordecaiCombatHUDWidget` in a slot named `BossBar`, Collapsed by default.
  - BindWidgetOptional names: `BossNameText`, `BossHealthBar`, `BossPostureBar`, `PhaseText`, `PhasePips` (an HBox of three pip Images: a filled disc, or a 3 px ring), `EnragedChip`, `BrokenChip`. `EnragedChip` and `BrokenChip` are never visible together: while `PostureBroken` or `KnockedDown` is on the boss, `BrokenChip` shows, and `EnragedChip` and `PhaseText` are Collapsed.
  - Use the class prefix "Mordecai" so the agent bridge can read `BossNameText` and `PhaseText`. `PhaseText` keeps its string while Collapsed ("Phase 3"), so a test can still read the phase.
- **Binding:** to the boss's ASC: Health / MaxHealth, Posture / MaxPosture, and the `PostureBroken` / `KnockedDown` tags.
  - Show on the boss encounter start event; hide on the boss's `Mordecai.State.Dead`.
  - The boss is the actor whose enemy definition has Tier = Boss. Bosses never get an EnemyTag.
- **Phase:** there is no boss phase system in code yet. TODO(DECISION):
  - the phase source, for example a `Mordecai.Boss.Phase.1/2/3` tag or a replicated int on the boss;
  - whether the phase thresholds should also appear as notches on the health bar.
  - `enemy_ai_framework_v1.md` defines Phase 1 basic attacks, Phase 2 AoE attacks unlocked and Phase 3 enrage. "Enraged" appears only in phase 3.
- **Name:** the boss's DisplayName ("Iron Warden"), cased with TextTransformPolicy ToUpper, so GetText keeps "Iron Warden". The six bosses are Thorn Guardian, Iron Warden, Drowned King, Crystal Seer, Tempest Ascendant and The Lich King.
  - Text measured at 40 px: THORN GUARDIAN 393, IRON WARDEN 309, DROWNED KING 342, CRYSTAL SEER 313, TEMPEST ASCENDANT 472, THE LICH KING 328.
  - The nameplate is the text + 48 (22 padding and a 2 px rim each side) and may grow to 480, so the text may be up to 432.
  - A name wider than 432 at 40 px steps down to 36 px (`.tt-boss__title--long`). Only TEMPEST ASCENDANT does: 425 at 36 px, a 473 nameplate. Never truncate a boss name.
  - Phase tab widths, measured: "PHASE 1" + pips 199, "PHASE 2" + pips 202, "PHASE 3" + pips + "ENRAGED" 340, pips + "BROKEN ×1.5" (the phase word collapsed) 282.
  - Worst cases against TEMPEST ASCENDANT's 473 nameplate: Enraged, 22 + 473 + 340 + 22 = 857, leaves 23 px between the tabs (17 px of floor between their outlines); Broken, 22 + 473 + 282 + 22 = 799, leaves 81. With the phase word kept beside "BROKEN ×1.5" the tab would be 382 and overlap the nameplate by 19 px, which is why the word collapses; with both chips at once it would be wider still, which is why only one state chip shows.
- **Posture:** the boss posture bar is the one `new_spells_proposal.md` refers to: Enfeeble R20 "boss posture bar takes a chunk on cast", Snare R20. A chunk shows as a trail on the posture lane.
- **Sizes:**
  - Health 848 x 22, posture 848 x 8. Name font 30 pt (40 px), chips 15 pt (20 px).
  - Brushes (03's manifest): the bar plate and both tabs are the plate stack (RoundedBox, no plate textures); the chips are the pill stack; the break stripes are `M_UI_PatternFill` (`good` and `ink`); the pips are RoundedBox Images (a filled disc, a 3 px ring). The HUD root owns it: a child of `UMordecaiCombatHUDWidget`, never a separate push.
- **Tokens:** `plate`, `rim`, `rim-shade`, `ink`, `text`, `text-muted`, `well`, `lip`, `foe`, `vital-trail`, `posture`, `good`, `danger`, `danger-text`, `world-heading`.

## Copy

- BossNameText: the boss's display name, shown in caps ("IRON WARDEN").
- PhaseText: "Phase 1", "Phase 2", "Phase 3" (shown in caps).
- Chips: "Enraged", "Broken ×1.5", "Down ×2.0" (the words in caps; the multipliers are built from data).
- World voice: no Tally remarks, no jokes, no exclamation marks, no tally strokes. The phases count with pips.

## Do / Don't

- Do keep the bar inside x 520-1400 at every phase; chips grow into the phase tab, never past the box.
- Do show one state chip at a time; Broken or Down outranks Enraged while it lasts.
- Do drop the Plate lane to y 168 while the bar shows.
- Do keep `foe` for the fill at every health level and phase.
- Don't add ornament, glow, rivets or a portrait. The nameplate, the pips and one state chip are the whole identity.
- Don't draw the phases as tally strokes.
- Don't show the boss's numbers (no "4,210 / 6,000").
- Don't put a boss name on an EnemyTag over the boss's head.
- Don't announce phases in this component; the Banner's phase card does it.
