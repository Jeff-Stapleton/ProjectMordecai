# WorldMarkers

The world-space combat readability layer for the fixed diorama camera: floor decals for enemy telegraphs and hazard zones, the player's aim chevron and ground-target reticle, and screen-edge pips for aggroed enemies the camera cannot see. Use it wherever the player must read, from the fixed camera, where a hit will land, where they are aiming or where a threat is coming from; it never carries text.

These are decal materials and VFX with UI constraints, not widgets (the pips are the one UMG piece). This card fixes their colours, edges, line weights and states so they belong to the same family as the HUD. Who builds and owns them, the UI or the VFX side, is TODO(DECISION).

## Anatomy

**The camera and the scale.** `UMordecaiCameraMode_Diorama`: `PitchAngle` -50, `Distance` 1800 cm, `FieldOfView` 60, yaw 0 (it looks along +X and never rotates). At the screen centre 1 cm of floor is 0.92 px across and 0.71 px up the screen (the floor is foreshortened by sin 50 = 0.77). +X (the W key) is up the screen. Across the frame the floor's depth runs from 0.79 x (bottom edge) to 1.38 x (top edge) of the centre's, so a line authored in world units would be about 3.8 px at the bottom and 2.2 px at the top for 3 px at the centre. Every edge below is therefore given in screen px and held constant (see Data and implementation); fills and hatches are world-space and foreshorten with the floor.

**The edge (dual edge, on every decal).** An ink | rim | ink sandwich: 3 px `ink`, 2 px `rim`, 3 px `ink` (4 | 2 | 4 in High Contrast, from `--tt-outline`). The outer ink holds on blockout, sand and snow; the rim holds on mire and night, where ink alone is 2.5:1 and 1.2:1. The player's own markers carry a wider cream core: 3 | 4 | 3.

| Part | Spec |
|---|---|
| Telegraph (enemy) | One decal per hit shape of the attack (`EMordecaiHitShapeType`): `ArcSector` = a sweep sector (`Radius`, `Angle`, `StartAngleOffset`), `Capsule` / `Box` = a thrust lane (`Length`, `Width`, a round far end for a capsule), `Circle` = a slam disc (`Radius`), with an optional shockwave ring drawn as a second edge (no field for its radius yet: TODO(DECISION)). Fill: the `danger` hatch (5 px lines every 12 px at 45 deg, at the screen centre) over the floor. Charge: solid `danger` growing from the attacker (a sweep by radius, a lane by length; a slam from its point outward) until it meets the edge. Edge: the dual edge. |
| Jump band | Only on a sweep whose profile has `JumpAvoidable` true (a low sweep): a 10 px band of 7 px blocks, alternating `rim` and `ink`, 12 px inside the arc's outer edge, like a hurdle. Thrusts and slams have `HitsAirborne` and never carry it. It is a striped band, not a dashed edge: the arc's own edge stays solid. |
| Hazard zone | Persistent ground that hurts (Area Denial "puddles, fire walls, lightning zones, corrupted terrain"; the `GroundHazard` delivery in damage_types_v1). The `danger` hatch, a 10 px solid `danger` band just inside the edge (telegraphs have none, so a hazard never reads as a windup), the dual edge, and a floor stamp. Any outline shape (circle, blob). |
| Floor stamp | A 44 px `ink` disc, 2 px `rim` ring, ink outline, laid flat (0.77 tall). It holds a 28 px glyph: the hazard's damage type in its `dmg-*` colour (`fire`, `poison`, ...), or `cross` in `rim` on an invalid ground target. The only "label" on the floor. |
| Aim chevron (player) | An open "^", 52 cm wide, 6 px `rim` stroke inside the ink outline (ink 3 \| rim 6 \| ink 3), flat on the floor, 140 cm ahead of the feet in the aim direction. 140 cm clears the pawn's head when aiming +X (up the screen); any nearer and the body hides it. Open, not a filled arrowhead, so it never reads as the mouse cursor. |
| Ground-target reticle (player) | A circle at the spell's `Radius` (100 cm in the preview, example). Edge: the player's wider sandwich (3 \| 4 \| 3). Valid: a light `rim` hatch (3 px lines every 14 px) and a 24 px cream cross at the centre. Committed: a dense `rim` hatch (5 px every 10 px) and an 18 px cream pin. Invalid: no fill, the rim dashed 12 / 9 over the solid ink (dashed means unavailable, the system rule) and the `cross` stamp. |
| Threat pip (screen space) | A `foe` teardrop whose point aims at the enemy, with the dual edge (3 px `ink`, a 2 px `rim` line inside). Basic: 32 px disc (43 px tip to tail). Elite and up: 40 px with a 12 px `good` diamond (EnemyTag's tier mark). Winding up: an ink \| rim \| ink ring 10 px outside the disc. It sits on a lane 56 px inside the screen edge (120 px with the 96 px TV margin). |

## States

| Piece | State | When | What shows |
|---|---|---|---|
| Telegraph | Windup | `EMordecaiAttackPhase::Windup` | The hatch and the dual edge at once; the charge grows linearly over `WindupTimeMs` and meets the edge as the phase ends. |
| Telegraph | Active | `Active` | The whole shape solid `danger` for `ActiveTimeMs`. |
| Telegraph | Recovery | `Recovery` | Gone. A whiff leaves nothing on the floor. |
| Telegraph | Interrupted | The attacker is staggered (`Mordecai.State.PostureBroken`) or dies mid-windup | Gone at once. |
| Hazard | Live | The hazard actor exists | Hatch, band, edge and stamp, static. It never pulses. |
| Hazard | Ending | Its last second (example) | The whole decal fades over that second. |
| Aim chevron | Today | Facing and aim are pinned to +X (US-098 is blocked on a decision) | The chevron points up the screen, always. TODO(DECISION): show it at all while it cannot turn. |
| Aim chevron | Aiming (US-098) | Mouse or right stick sets the aim yaw | The chevron turns with the aim, at the same 140 cm. |
| Reticle | Valid | Targeting mode, the point is castable | Hatch, cross, wide edge. It follows the cursor (keyboard and mouse) or moves with the right stick (gamepad), clamped to the spell's `Range`. |
| Reticle | Invalid | The point is not castable (no floor, out of range or line: TODO(DECISION) which rules) | Dashed rim, no hatch, the `cross` stamp. A cast press does nothing (and plays the negative thunk). |
| Reticle | Committed | The cast is released (`CommitCast`) | Dense hatch and pin until the spell lands, then gone. |
| Reticle | Cancel | Right click / B (`CancelCast`) | It shrinks and fades out (ReticleCancel). |
| Threat pip | Shown | An enemy is aggroed (or has hit the player) and its body is outside the frame | A pip on the lane where the line from the player to the enemy crosses it. |
| Threat pip | Winding up | That enemy is in `Windup` | The ring is added (no pulse). |
| Threat pip | Displaced | Its lane point falls on a HUD box (gauge plate, spell slots, status chips, rail head, tickets, quick items, loadout, the prompt and hint, the boss bar while up) | It slides along the lane to the nearest free point. |
| Threat pip | Hidden | The enemy comes on screen, leashes or dies | Gone; the EnemyTag takes over on screen. |

The TargetingHint (the core hint bar "Cast \| Cancel", keycap first) sits in the interact prompt box while the reticle is up; see InteractPrompt's Targeting state.

Motion: the charge is a timer, not an animation (linear over `WindupTimeMs`). Pips appear with ThreatPipFade and then track continuously; the reticle's cancel is ReticleCancel (both rows in the brand book's Motion table). Nothing here pulses or blinks (the 3 Hz rule), and Reduce Motion changes nothing: every change here is information. A whole decal may fade (opacity on the decal), never its edge alone.

## Data and implementation

- **Existing code** (no telegraph, ground-marker, reticle or off-screen marker code exists; a grep of `Source/LyraGame/Mordecai` finds none):
  - `UMordecaiAttackProfileDataAsset`: `WindupTimeMs`, `ActiveTimeMs`, `RecoveryTimeMs`, `HitShapeType`, `HitShapeParams` (`FMordecaiHitShapeParams`: `Radius`, `Angle`, `StartAngleOffset`, `Length`, `Width`), `HitsAirborne`, `JumpAvoidable`. These drive the telegraph shape, the charge and the band, with no new data.
  - `EMordecaiAttackPhase` (`Windup`, `Active`, `Recovery`) and `UMordecaiHitDetectionSubsystem` (`ArcSectorOverlapQuery`, the capsule and circle queries): the decal must draw the same shape the query tests.
  - `DA_EnemyAttack_BasicSlash` (the Frontliner's attack): `MeleeSweep`, windup 500 ms, active 200, recovery 400, 90 deg, 150 cm, `JumpAvoidable`. The preview's low sweep is this attack. Everything else in the preview is example data: the Corrupted Knight's slam (its spec abilities `corrupted_knight_cleave`, `_guard_break` and `_dark_wave` have no shapes yet), the thrust lane (80 x 250 cm), the slam and its ring (90 / 125 cm), the burning patch and the poison pool.
  - `AMordecaiCharacter::FacingArrowComponent`: a `UArrowComponent`, green, Z +50, hidden in game (N12). The aim chevron replaces it.
  - `UMordecaiSpellDataAsset`: `TargetingType` (`Circle`, `PersistentArea`, ...), `Range`, `Radius`. No spell is ground-targeted today (Fireball is a projectile, Blink a dash, Stone Skin and Restoration are self); the reticle is ready for the first one (for example Sanctuary's "AoE heal zone on ground" in new_spells_proposal, example).
  - `UMordecaiEnemyIndicatorComponent` registers a Lyra `UIndicatorDescriptor` on aggro or damage and never sets `SetClampToScreen`, so today nothing marks an off-screen enemy.
- **Specs:** attack_taxonomy_v1 §1.1 (sweep / thrust / slam, the jump rule) and §6.2 (ground targeting: KBM reticle follows the mouse, left click casts, right click cancels; controller right stick, A casts, B cancels; the proposed `TargetingMode` with `EnterTargeting`, `UpdateReticle`, `CommitCast`, `CancelCast`). enemy_archetypes_v1 Telegraph Profile (`WindupClarity`, `FXClarity`, `AudioCues`, `GroundMarkers`; its example sets `GroundMarkers: false`) and its rule "Telegraphs must remain readable from the fixed diorama camera". ability_schema_v1 `TelegraphType` (`CastGlow`, `WindupPose`) stays on the body: these decals add the floor.
- **Which attacks get a floor telegraph:** those whose archetype's Telegraph Profile has `GroundMarkers` true. TODO(DECISION) which archetypes set it. Proposal: every `JumpAvoidable` sweep (a jump must be readable to be fair), every Artillery "area marker", and Elite tier and up; Basic melee without the flag relies on `WindupPose`.
- **Decals:** `M_UI_GroundDecal` (03's asset manifest; proposed, and whether the UI or the VFX side owns it is TODO(DECISION)), one deferred decal material for telegraphs, hazards, the chevron and the reticle, on `UDecalComponent`s (projected straight down, sorted under actors). Parameters: Shape (sector / lane / circle / mask texture for blobs), the shape params in cm, Charge (0-1), Fill (hatch, dense hatch, none), EdgeInk / EdgeRim (px), RimDash (bool), and the colours from `UMordecaiUIThemeDataAsset` (`DA_UITheme_Diorama` / `DA_UITheme_HighContrast`), so High Contrast swaps them with the HUD.
  - Screen-constant edges: width in cm = width in px x PixelDepth x 2 tan(FOV / 2) / ViewSize.x, which is about 1.08 cm per px at the screen centre. The hatch stays world-space.
  - The charge is a material parameter set each tick from the attack's elapsed windup; nothing is spawned per frame.
  - Floor stamps are a second small decal (or a flat mesh) with the glyph mask, tinted.
- **Aim chevron:** a decal attached to the pawn, yawed by the aim direction (US-098), 140 cm out. TODO(DECISION) with US-098: whether facing follows the aim at all.
- **Reticle:** spawned on `EnterTargeting`, moved on `UpdateReticle` (KBM: the cursor deprojected onto the floor; gamepad: the right stick at a set speed, example 900 cm/s), clamped to `Range`; the invalid test is TODO(DECISION). The TargetingHint is InteractPrompt's.
- **Threat pips (UMG):** a pool of 8 pip widgets on a `ThreatPips` canvas, a child of the HUD root (`UMordecaiCombatHUDWidget`, pushed once on `UI.Layer.Game`; never a separate push). Each frame: project each aggroed enemy, intersect the line from the screen centre with the lane rectangle (56 px in, 120 with the TV margin), slide it out of the HUD map's boxes, and set the render angle. Alternative: a second `UIndicatorDescriptor` per enemy with `SetClampToScreen(true)` (Lyra's `SActorCanvas` already clamps) and `SetShowClampToScreenArrow(false)` (the teardrop is the arrow); it would need the lane inset and the HUD-box avoidance added. TODO(DECISION) which.
- **Sizes at 1920 x 1080:** edges 3 \| 2 \| 3 px (player 3 \| 4 \| 3); hatch 5 / 12 px; jump band 10 px of 7 px blocks; hazard band 10 px; stamp 44 px with a 28 px glyph; chevron 52 cm, 140 cm out; pips 32 / 40 px, ring +10 px, lane 56 px in.
- **Tokens:** `danger` (telegraphs, hazards), `foe` (pips), `rim`, `ink`, `good` (the Elite pip diamond), `dmg-fire`, `dmg-poison` and the other `dmg-*` (hazard stamps). Every colour comes from the theme asset; none is a VFX literal.
- **Glyphs:** `fire`, `poison` (and the other damage-type glyphs on hazard stamps), `cross`.

## Copy

None. Nothing on the floor or at the screen edge carries text, numbers or remarks: shapes, the charge, the band and glyph stamps only. The TargetingHint's "Cast" and "Cancel" live in the HUD (InteractPrompt), not on the floor.

## Do / Don't

- Do keep `danger` for every enemy telegraph and hazard and cream (`rim` on `ink`) for the player's own markers: red is threat, cream is yours.
- Do draw the dual edge on every decal, screen-constant, so it holds on every floor and at every depth.
- Do draw the telegraph from the same shape and numbers the hit query uses; a decal that is bigger or smaller than the hit is a lie.
- Do put the jump band only on `JumpAvoidable` sweeps.
- Do slide pips off the HUD boxes rather than draw over a plate.
- Don't put text, numbers, keycaps or Tally remarks on the floor.
- Don't pulse, blink or glow any marker; no bloom, no emissive halo.
- Don't use a dashed edge for anything but an invalid (unavailable) reticle.
- Don't colour the player's markers by spell element; the hazard stamp is the only place a damage colour appears.
- Don't show a pip for an enemy that is on screen; its EnemyTag already marks it.
