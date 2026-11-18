# US-121: Recovery — Directional Block and Parry

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** US-098 (aim-driven facing), US-101 (incoming-hit defense resolver)
**Paired story:** none

## Overview
Issue #71 lets a raised guard mitigate attacks from behind. Block and parry become directional defenses centred on the defender's aim-driven actor forward, while equipped shield data can widen the defensive arc. Melee and projectile spells use the same flattened source-direction rule.

## Execution Mode
**HEADLESS**
The resolver, abilities, equipment data, projectile path, automation tests and playtest tooling are C++/Python changes; no content asset is required.

## References
- GitHub issue #71 — omnidirectional block defect and source audit
- `Specs/skill_sheet_v1.1.md` — shield skills reduce frontal damage
- `stories/done/US-006-block-parry-system.md` — block/parry behavior
- `stories/done/US-101-recovery-defenses-against-enemy-melee.md` — defense precedence and resolver
- `Specs/design_decisions_log.md` — owner decisions and tunable defaults
- `Specs/control_bindings_v1.1.md` — LT block, LT+RT parry, right-stick aim
- Existing code: `Source/LyraGame/Mordecai/Combat/MordecaiHitDefense.*`, `MordecaiGA_Block.*`, `MordecaiGA_Parry.*`, `MordecaiProjectile.*`

## Acceptance Criteria
- [ ] AC-121.1: Without a shield, block covers a 120 degree arc centred on the defender's actor forward. Hits from 0 and 59 degrees are blocked; hits from 61 degrees and behind land as full, unblocked hits with no block stamina cost and normal posture damage.
- [ ] AC-121.2: Equipped shield data carries its configurable block arc. A shield configured to 150 degrees blocks a hit at 70 degrees that the default 120 degree guard does not. Tunable defaults are Buckler/small 150, standard 180 and tower 210 degrees.
- [ ] AC-121.3: Parry uses the same effective defensive arc as block. An out-of-arc hit lands normally and does not consume the active parry window as a successful parry.
- [ ] AC-121.4: Attack direction is the ground-plane direction from defender to attacker for melee. Projectiles and projectile spells use their source/origin, falling back to the inverse flattened velocity when source and defender coincide. A projectile arriving from behind is not blocked.
- [ ] AC-121.5: In the fresh packaged CombatTestMap build, a real TestAttacker swing from the player's front is blocked while LT is held, and after the player aims away so the attacker is behind, its swing lands for full health/posture damage without block stamina drain. Checks `block_mitigates` and `rear_unblocked` of `Scripts/playtests/us101_defenses.py` capture one frame after each hit.
- [ ] AC-121.6: Existing i-frame precedence, in-arc block/perfect-block/parry rewards, guard break, damage payload behavior and player attacks remain unchanged.
- [ ] AC-121.7: The owner decision and shield defaults are recorded in `Specs/design_decisions_log.md`; content guard passes with no asset changes.

## Technical Notes
- Compare normalized 2D defender forward with the normalized 2D direction toward the source against `cos(ArcDegrees / 2)`. Clamp configured arcs to 0–360.
- Keep the resolver server-authoritative. Actor rotation already follows the existing aim RPC/actor replication path from US-098.
- Shield data is a `UMordecaiShieldDataAsset` equipped through the existing off-hand equipment slot. The active block reads it from the avatar's equipment component.
- Parry remains active after an out-of-arc hit; that hit is not a successful parry.
- Projectile direct hits and projectile-spell hits share `AMordecaiProjectile`; AoE uses the impact/source position.
- No PIE-only paths.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.DefenseArc.FrontIsBlocked`
- `Mordecai.DefenseArc.FiftyNineDegreesIsBlocked`
- `Mordecai.DefenseArc.SixtyOneDegreesIsNotBlocked`
- `Mordecai.DefenseArc.BehindIsNotBlocked`
- `Mordecai.DefenseArc.EquippedShieldWidensArc`
- `Mordecai.DefenseArc.ProjectileFromBehindIsNotBlocked`
- Directional parry and out-of-arc full-damage regression checks under `Mordecai.DefenseArc`

### Tier 2 — PIE smoke (editor integration)
- Existing defense/spawn/HUD smoke checks remain green; no new content integration is introduced.

### Tier 3 — Packaged playtest
- See Playtest.

### Tier 4 — Real-input playtest
- Extend `Scripts/playtests/us101_defenses.py`: `block_mitigates` explicitly aims front before blocking, and new `rear_unblocked` aims away while guarding; real LT and aim input only.
- Offline fake-bridge tests model facing and directional defense.

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap`; Tier 4: `py Scripts/playtest_story.py US-101` on the same fresh package.
- **Player steps:** walk next to TestAttacker; aim toward it and hold LT through a swing; release LT, use right-stick aim to face away, hold LT through the next swing.
- **Visible on screen:** first capture shows the player facing the TestAttacker after a mitigated front hit with health and stamina reduced; second shows the player facing away after the rear hit landed for full health/posture damage while stamina stayed unchanged for that hit.
- **Proof:** Tier 4 checks `block_mitigates` (front) and `rear_unblocked` in `Scripts/playtests/us101_defenses.py`; scripted image analysis writes `image-verification.json`. The laptop builder does not open image files.
- **Tier 4 script:** `Scripts/playtests/us101_defenses.py` — run with `py Scripts/playtest_story.py US-101`.
- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off.
- [ ] RED log preserved; clean build, targeted GREEN and full `Mordecai.*` automation pass.
- [ ] Tier 2 PIE smoke passes.
- [ ] Python tooling tests and story lint pass.
- [ ] Content guard passes; implementation is committed and pushed before packaging.
- [ ] Fresh package records `dirty=false` and `source_changed_during_build=false`.
- [ ] Tier 3 and extended US-101 Tier 4 pass without `--allow-stale`.
- [ ] Scripted image verification passes and preserves the required front-block/rear-hit captures.
- [ ] Fresh-main and branch full packaged sweeps compare with persistent `pass_to_fail=[]`.
- [ ] Required evidence bundle and SHA256 manifest are copied to the agent store.
- [ ] Draft PR closes issue #71 and remains draft.

## Completion Notes
Pending implementation and evidence.
