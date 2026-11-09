# US-087: Recovery — Fireball Projectile Flies and Hits; Blink Moves and Ends Its Invulnerability

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none (the hit is observable on the invisible enemies; US-085 makes them visible)
**Paired story:** none

## Overview
In the packaged arena Fireball spends 3 SP and does nothing visible: no projectile actor appears and the enemy ahead keeps its health (N15, possibly N11). The exploratory agent also found that Blink can spend SP and move the player 0 cm when an enemy is ahead (N20) and that Blink's i-frame tag `Mordecai.State.Invulnerable` is never removed (N19) — a permanently invulnerable player. This story makes the two movement/damage spells of the magic slice behave. Features: spells, projectiles_aim, status_effects.

## Execution Mode
**HEADLESS**
The remaining work is Blink C++ plus headless tests and packaged validation. Issue #16 used a one-time
EDITOR authoring step to add the saved isolated test fixture; that completed fixture work does not change
AC ownership or make the remaining story EDITOR-mode. Existing gameplay, spell assets and maps remain unchanged.

## References
- `Specs/agent_playtest_plan_v1.md` — N15, N11 (suspected), N19, N20 ("Found by the first exploratory-agent run")
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184841-us057`: `fireball_cast` PASS (SP 10 → 7 at 0.02 s), `fireball_projectile` FAIL NEW ("no actors[] entry with kind=="projectile" within 1.5s of the tap (52 polls, radius 3000)"), `fireball_hits_enemy` FAIL NEW ("health stayed 100 for 2.5s ... projectile seen: False")
- Exploratory run `Saved/Playtests/20261022-185207-explore/report.md`: N19 (`Mordecai.State.Invulnerable` still on the player 6+ s after Blink ended), N20 (Blink with an enemy ~90 cm ahead spent 2 SP, moved 0 cm; `bridge_04.png`)
- Done stories: US-057 AC-057.1 (Fireball: Range 1500, BasePower 25, SpawnProjectile), AC-057.2 (Blink: Range 500, DashMovement), AC-057.12; US-009 (projectiles); US-020 AC-020.1/020.2; US-021 AC-021.1-021.3 (Blink teleport, wall clamp, brief invulnerability)
- `Specs/control_bindings_v1.1.md`; spells on Q (Fireball) and E (Blink) per `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:200-209`
- Existing code:
  - `Source/LyraGame/Mordecai/Magic/MordecaiGA_Fireball.cpp:76-108` — spawns `AMordecaiProjectile` at the caster's actor location, THEN calls `InitFromSpec` (speed, lifetime)
  - `Source/LyraGame/Mordecai/Combat/MordecaiProjectile.cpp:52-67` — `InitFromSpec` sets `ProjectileMovement->InitialSpeed`/`MaxSpeed` after the component has initialised (velocity is computed from `InitialSpeed` in `UProjectileMovementComponent::InitializeComponent`, during `SpawnActor`); `:79-93` — `BeginPlay` reads `Spec.Lifetime` before `InitFromSpec` ran; `:32-33,137-152` — overlaps with any Pawn/WorldDynamic call `HandleHit` and `Destroy()` even when the actor has no ASC (pickup trigger spheres, N11); no visible mesh component
  - `Source/LyraGame/Mordecai/Combat/MordecaiCombatTypes.h:158-173` — `FMordecaiProjectileSpec` defaults (Speed 0, Lifetime 0)
  - `Source/LyraGame/Mordecai/Magic/MordecaiGA_Blink.cpp:95-124` — "wall" trace on `ECC_WorldStatic`, which a Pawn capsule blocks by default, so an enemy ahead clamps the teleport to ~0 (N20 suspect); `:126-166` — invulnerability added as a loose tag, removed by a timer on the ability instance (`OnInvulnerabilityExpired` → `GetAbilitySystemComponentFromActorInfo()`), which fails if the ability ended / its actor info is cleared first (N19 suspect)

## Acceptance Criteria
- [x] AC-087.1: A Fireball projectile moves at `SpellData->ProjectileSpeed` along the (assisted) aim direction from its first tick, and its lifetime/range come from the spell data — verified by `Mordecai.Recovery.Fireball.ProjectileMovesAndExpiresFromConfiguredContract`
- [x] AC-087.2: A projectile that overlaps an actor without an ability system component (pickup trigger, prop) is not destroyed and deals no damage; it still stops on WorldStatic and on damageable targets — verified by `Mordecai.Recovery.Fireball.NonASCOverlapPassesThroughBeforeNaturalDamageAndStatus` (issue #88: begun isolated world, reflected delegate observer, then a blocking ASC Pawn) and `Mordecai.Recovery.Fireball.WorldStaticBlocksAndNonzeroPierceStillHitsTwoTargets`
- [x] AC-087.3: The projectile spawns in front of the caster's capsule and has a visible mesh (placeholder sphere is fine) — verified by `Mordecai.Recovery.Fireball.GASCastTimersSpawnClearVisibleProjectileAndNaturallyDamageTarget`
- [x] AC-087.4: In the packaged build, the player taps Q facing an enemy and a projectile appears — accepted by owner checks `fireball_cast` / `projectile_visible` of `Scripts/playtests/us087_fireball.py`; shared `Scripts/playtests/us057_magic.py` check `fireball_projectile` is a regression cross-check, not owner acceptance (Tier 4)
- [x] AC-087.5: In the packaged build, that Fireball damages the same retained enemy ahead within 2.5 s — accepted by owner checks `target_integrity` / `same_target_damaged` of `Scripts/playtests/us087_fireball.py`; shared `Scripts/playtests/us057_magic.py` check `fireball_hits_enemy` is a regression cross-check, not owner acceptance (Tier 4)
- [ ] AC-087.6: Blink's destination trace ignores Pawns (it clamps on world geometry only; the destination is adjusted to a free spot if it lands in a pawn), so Blink with an enemy ahead still moves the player; if no movement is possible the spell does not commit its SP cost — **Tier 1 GREEN / packaged acceptance pending**, mapped to `Mordecai.Recovery.Blink.EnemyAheadDoesNotBlock`, `PawnAheadStillClampsAtWall`, `NoMoveNoCost`, `DestinationPawnAdjustedToFreeSpot`, `ShortPositiveMoveCommitsCast`, and `PreservesGeometryAndFloorSemantics`
- [x] AC-087.7: Blink's invulnerability is removed when its duration ends even if the ability has already ended, and never outlives it — verified by `Mordecai.Recovery.Blink.InvulnerabilityExpires` (Stone Skin's own Invulnerable grant is unaffected; prefer a duration GameplayEffect granting the tag over a loose tag + ability-owned timer)
  - **Delivered by US-101 (issue #53, `d490a2d`, 2026-10-30).** Once US-101 made damage honor `Mordecai.State.Invulnerable`, a stuck tag would have meant a permanently invulnerable player, so the fix could not wait for this story.
    - The fix is the one this AC prescribes: a duration GE grants the tag.
    - Root cause: `UGameplayAbility::EndAbility`'s `ClearAllTimersForObject` killed the ability-owned removal timer. The "actor info cleared" suspect above is not the mechanism.
    - Note: Stone Skin grants no Invulnerable; it grants `Status.StoneSkin` (`MordecaiGA_StoneSkin.cpp`).
    - `Mordecai.Recovery.Blink.InvulnerabilityExpires` passes; it failed before the fix.
    - AC-087.9's Tier 4 check `blink_invulnerability_ends` is still this story's.
- [ ] AC-087.8: In the packaged build, the player taps E with an enemy about 1 m ahead and moves > 300 cm — verified by `blink_past_enemy` / `blink_integrity` in `Scripts/playtests/us087_blink.py` (Tier 4)
- [ ] AC-087.9: In the packaged build, `Mordecai.State.Invulnerable` is first observed granted and then gone from `player.tags` within 1.5 s after a Blink (no Stone Skin active) — verified by `blink_invulnerability_ends` in `Scripts/playtests/us087_blink.py` (Tier 4)

## Technical Notes
- Fireball: use `SpawnActorDeferred<AMordecaiProjectile>` → `InitFromSpec` / `SetProjectileInstigator` / sphere radius → `FinishSpawning`, so `InitializeComponent` and `BeginPlay` see the spec; or set `ProjectileMovement->Velocity` explicitly. Apply the same to `MordecaiGA_MagicMissile.cpp:129-155`.
- Spawn origin: `Origin + AimForward * (capsule radius + sphere radius + small margin)`.
- Visible mesh: a `UStaticMeshComponent` with an engine basic shape referenced from C++ (hard ref, so it is cooked); no collision on the mesh.
- N11: filter in `OnProjectileOverlap` — skip actors with no ASC (`UAbilitySystemGlobals::GetAbilitySystemComponentFromActor`) unless they are WorldStatic blockers.
- Blink (N20): repeat the existing `ECC_WorldStatic` trace while locally ignoring each hit Pawn actor, then if the destination overlaps blocking geometry, step back along the line to the first free real-capsule position; decide "no movement possible" before committing cost. Actor-level ignored-trace continuation is the scoped engineering choice for current content: it preserves every non-Pawn component's existing channel response, but is not a component-level Pawn-object-type filter and does not claim semantics for a hypothetical Pawn-owned non-Pawn blocker. (N19): grant invulnerability through a duration `UGameplayEffect` (granted tag, Duration = `GetInvulnerabilityDuration()`), so GAS removes it; Stone Skin's own grant stays separate.
- Blink owner checks are in the separate `Scripts/playtests/us087_blink.py`; do not add US-087 to `us057_magic.py` `COVERS`, because the dedicated owner takes precedence:
  - `blink_past_enemy`: its own `us087_blink.py` launch retains the exact passive TestDummy at (600, 600), real-WASD walks to ~100 cm directly west, then presses/releases real Slate E. Continuous sampling must prove SP 10 → 8 and >300 cm +X movement within 2 s while that exact full-health target remains stationary and unchanged. This is independent of the older shared-us057 sequence (10 → 7 → 5 → 1) and its `wait_until` checks.
  - `blink_invulnerability_ends`: in that same dedicated launch, continuous sampling must first observe `Mordecai.State.Invulnerable` granted and then absent within 1.5 s of E; absence without a prior grant does not pass, and Stone Skin must never appear.
- CombatTestMap keeps its zero-pickup invariant. `us087_pickup_lane.py` separately characterizes one real 120 cm
  native pickup on the enemy-free `PickupProjectileTestMap`. The earlier exact-clean DevTestMap run
  `Saved/Playtests/20261108-005745-us087-pickup-lane` at `7dc9a1e` was inconclusive: all retained player states
  stayed at 55 HP, while `BP_MordecaiEnemy_Frontliner_C_0` changed 200→175 as the retained Fireball disappeared.
  That supports enemy interference, but no collision-event trace attributes the disappearance and it does not
  prove native-pickup collision. Diagnostic `20261108-010100-us087-pickup-lane-diagnostic-300` crossed the pickup
  but still failed the enemy-free guard. Neither is acceptance evidence. N11 has no `known_issue` to remove.
- Script bookkeeping: `fireball_projectile` and `fireball_hits_enemy` were NEW (no `known_issue`) and remain shared regression cross-checks only. `fireball_hits_enemy`'s `does` text says "pre-cast value (200)"; it compares with the pre-cast reading, keep it that way. `Scripts/playtests/us087_blink.py` is the present dedicated owner of AC-087.8/.9; adding US-087 to `us057_magic.py` `COVERS` would be shadowed by that dedicated owner and is not acceptance.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Fireball.ProjectileMovesAndExpiresFromConfiguredContract` — AC-087.1
- `Mordecai.Recovery.Fireball.NonASCOverlapPassesThroughBeforeNaturalDamageAndStatus` — AC-087.2; its issue #88 subfixture observes the real component delegate in a begun world before the projectile reaches a blocking ASC Pawn
- `Mordecai.Recovery.Fireball.WorldStaticBlocksAndNonzeroPierceStillHitsTwoTargets` — AC-087.2
- `Mordecai.Recovery.Fireball.GASCastTimersSpawnClearVisibleProjectileAndNaturallyDamageTarget` — AC-087.1, AC-087.3
- `Mordecai.Recovery.Blink.EnemyAheadDoesNotBlock`, `PawnAheadStillClampsAtWall`, `NoMoveNoCost`, `DestinationPawnAdjustedToFreeSpot`, `ShortPositiveMoveCommitsCast`, and `PreservesGeometryAndFloorSemantics` — AC-087.6 (Tier 1 only until the fresh package/input run)
- `Mordecai.Recovery.Blink.InvulnerabilityExpires` — AC-087.7
- `Mordecai.Level.TestMaps.PickupProjectileMapHasRealPickupAndClearLane` — issue #16 fixture provenance,
  saved-map pickup/collision-response contract and enemy-/blocker-free lane. Cooked behavior belongs to Tier 3/4.
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_magic.py`: `[LOGIC]` directly activates Fireball, then a post-tick watcher verifies a projectile actor exists and moves between editor ticks. This is integration smoke only, not real-input acceptance. No Blink direct-call check belongs to this Fireball slice.
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us087_fireball.py`: dedicated Fireball owner for AC-087.4/.5.
- `Scripts/playtests/us087_blink.py`: dedicated Blink owner for AC-087.8/.9; it does not weaken
  or replace the shared `Scripts/playtests/us057_magic.py` regression checks.
- `Scripts/playtests/us087_pickup_lane.py`: issue #16 actual-pickup characterization; owns no AC.

## Playtest
- **Launch path:** CombatTestMap and PickupProjectileTestMap → `B_MordecaiArenaExperience`; run
  `py Scripts/playtest_story.py US-087` on the fresh package.
- **Player steps (Fireball):** resolve and retain the passive TestDummy's exact actor name; require it live,
  full-health and stationary; walk (WASD) about 1400 cm -X of it, validate the +X shot line is
  within Fireball's 1500 range, then tap Q through Slate and continuously observe for 2.5 s.
- **Player steps (Blink):** in a separate launch, retain that exact passive TestDummy, real-WASD
  walk to 80–130 cm directly west of it, require a live/full/stationary player at exactly 10 SP
  with E/Blink in slot 1 and no Stone Skin/Invulnerable, then press/release E through Slate.
- **Visible on screen:** `projectile_visible` captures the visible asset-backed projectile in
  flight within 1.5 s. `same_target_damaged` is an immediate post-hit timing frame and may have
  the target off-camera; timely AC-087.5 acceptance comes from the continuous exact-name/health
  audit. After the 2.5 s timing window, `reduced_health_visual` approaches the retained target by
  real WASD, revalidates its identity/health/max/position and player life, and captures its reduced
  health bar as supplemental visual evidence (`proves=()`). Blink's `pre_blink` / `post_blink`
  game frames are supplemental; continuous retained player/target state proves motion and integrity.
- **Proof:** independent Tier 4 checks `fireball_cast`, `projectile_visible`,
  `target_integrity`, and `same_target_damaged` in `Scripts/playtests/us087_fireball.py`;
  `blink_past_enemy`, `blink_integrity`, and `blink_invulnerability_ends` in
  `Scripts/playtests/us087_blink.py`; plus
  the shared `us057_magic.py` `fireball_projectile` / `fireball_hits_enemy` regression checks
  in the full sweep.
- **Tier 4 scripts:** `Scripts/playtests/us087_fireball.py` owns AC-087.4/.5;
  `Scripts/playtests/us087_blink.py` owns AC-087.8/.9; `Scripts/playtests/us087_pickup_lane.py`
  characterizes issue #16 only.
- **Steps:** generated by `py Scripts/playtest_story.py US-087 --plan`.

<!-- Scripts/playtests/us087_fireball.py -->

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `target_preconditions` | state player,actors -> resolve the live TestDummy by label/name or nearest (600,600), retain exact name; require full health, no death, and stationary over a baseline watch; then walk_to about 1400 cm -X and validate a +X shot line inside 1500 range | - |
| 2 | `fireball_cast` | tap Q through Slate -> spell_points decreases by 3; clock starts before the tap and reports wall/game timing | AC-087.4, AC-087.5 |
| 3 | `projectile_visible` | continuous state polling -> actors[] kind=='projectile' with has_visible_mesh==true within 1.5s; independently screenshot it | AC-087.4 |
| 4 | `target_integrity` | through the full observation window the retained exact target name does not disappear or get replaced, move materially, reset/increase health, and the player stays alive | AC-087.5 |
| 5 | `same_target_damaged` | continuous polling -> the retained target's health decreases within 2.5s without a reset/replacement; capture an immediate timing frame that may have the target off-camera | AC-087.5 |
| 6 | `reduced_health_visual` | after the timing window, real-WASD `walk_to` 300 cm west of the retained target; revalidate exact name, reduced/current and max health, stationary position, no replacement and live player; capture its reduced health bar | - |

<!-- Scripts/playtests/us087_pickup_lane.py -->

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `pickup_lane_preconditions` | retain native TestPickup_MysteriousAmulet; require live player, yaw 0, Fireball cost 3 at 10 SP; real-WASD walk_to pickup.x-500, same y, within 35 cm; require an enemy-free +X lane | - |
| 2 | `fireball_cast` | tap Q through Slate -> verified Fireball cost 3, SP 10 -> 7 | - |
| 3 | `pickup_trigger_pass_through` | retain one exact visible projectile without a gap from x < pickup.x-150 through x > pickup.x+175; a sampled 3D segment intersects the pickup envelope; capture a game-owned frame | - |
| 4 | `pickup_unchanged` | retained exact pickup remains present at its original location throughout flight; no F input | - |
| 5 | `lane_integrity` | player remains alive/unchanged; no enemy enters the guarded segment | - |

<!-- Scripts/playtests/us087_blink.py -->

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `blink_preconditions` | state player,actors -> retain exact live/full-health stationary TestDummy; real-WASD walk_to about 100 cm -X; require a 80-130 cm +X gap, low Y/Z offset, yaw 0, live full-health stationary player, exact SP 10, granted slot-1 E/Blink cost 2, and neither Stone Skin nor Invulnerable; capture a supplemental pre-cast frame | - |
| 2 | `blink_past_enemy` | press E through Slate and release it in finally; continuously sample the retained player/target -> SP 10->8 and player +X movement > 300 cm within 2s with low Y/Z drift | AC-087.8 |
| 3 | `blink_integrity` | through the full timed audit the exact player stays live, the exact passive target is not replaced/moved/damaged/killed, and Stone Skin never appears | AC-087.8 |
| 4 | `blink_invulnerability_ends` | continuous player.tags sampling must first observe Mordecai.State.Invulnerable granted, then observe it absent within 1.5s of E; never granted does not pass; capture a supplemental post-cast frame | AC-087.9 |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [x] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [x] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [x] Content: isolated `PickupProjectileTestMap.umap` saved, committed and cooked; all existing maps unchanged;
  `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + dedicated owner script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/PickupProjectileTestMap` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-087` (Tier 3 + all dedicated US-087 Tier 4 owners) on that same fresh package accepts AC-087.4/.5 and, once implemented, AC-087.8/.9; the full sweep keeps shared `us057_magic.py` `fireball_projectile` / `fireball_hits_enemy` as regression cross-checks; no prior PASS becomes FAIL; `--lint` exits 0 for US-087 and US-057 — never `--allow-stale`
- [ ] Known issues N15, N11, N19, N20 marked FIXED (or N11 cleared) in `Specs/agent_playtest_plan_v1.md` with the run dir
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-087]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
### Issue #25 R1 guard pass (2026-11-08; package evidence still pending)

- R1 changed only tests, the dedicated Blink owner oracle, and documentation; the locally green Blink runtime and
  duration-GE invulnerability implementation are unchanged.
- Added real-collision coverage for a trace-blocking Pawn at 90 cm with a WorldStatic wall face at 300 cm:
  the Pawn is ignored, the wall still clamps the player to approximately 250 cm, 2 SP is spent, and the enemy is
  unchanged. Changing the Pawn-hit continuation to `break` produced the isolated expected RED
  (`PawnAheadStillClampsAtWall`, 6 PASS / 1 FAIL); the restored focused run is 7/7 PASS.
- `NoMoveNoCost` now also requires the Blink spec inactive and `State.Casting` absent. Deleting the failure-path
  `EndAbility` produced the isolated expected RED on exactly those two assertions (6 PASS / 1 FAIL).
- Positive endpoint adjustment and every geometry-profile case now assert SP 10 → 8. A wall face at 60 cm proves
  a 10 cm positive Blink still commits SP, cast event, cooldown and Invulnerable; an invented 20 cm minimum
  produced the isolated expected `ShortPositiveMoveCommitsCast` RED (6 PASS / 1 FAIL).
- The owner now enforces its 2 s movement deadline both when a sample qualifies and when results are evaluated.
  Its deterministic fake state call advances the owner clock past the deadline before returning qualifying
  movement. Restoring the old unchecked behavior false-passed the owner (`code 0` where the regression requires
  `code 1`); the restored targeted test and all 612 Python tests pass.
- Final local ladder: compile PASS; focused Blink 7/7 PASS; retained expiry 1/1 PASS; all Mordecai automation
  788/788 PASS; Python 612/612 PASS; US-087 and US-057 lints PASS; content guard PASS; default PIE 16/16
  PASS; PickupProjectileTestMap PIE 3/3 PASS. Package, Tier 3, Tier 4, comparison sweep and screenshots were
  not run in this bounded R1 phase. AC-087.6/.8/.9 remain unchecked pending the fresh package and real-input run.
- The implementation commit is intentionally made before the first fresh package. After that package and its
  input runs pass, the tracked AC ticks, N20 FIXED status, completion notes, and required story closing commit
  are added; the coordinator's freshness policy then requires a final exact-HEAD repackage. `CLAUDE.md` Step 6
  by itself exempts a stories-only closing commit from repackage, but that exemption does not override this
  recovery's coordinator-directed final-package policy.

### Issue #25 / N20 Blink local GREEN (2026-11-08; package evidence pending)

- The runtime change is Blink-only. Its existing `ECC_WorldStatic` path trace now locally ignores Pawn actors
  and continues to the first non-Pawn blocker, preserving the existing WorldStatic, blocking WorldDynamic,
  overlap-trigger, floor-contact and 50 cm wall-buffer behavior. The exact desired endpoint is checked with the
  real actor capsule and searched backward on the cast line only; it does not use `FindTeleportSpot`'s lateral/Z
  MTD. Any positive partial move remains valid.
- Blink's configured contract is instant (`WindupTime=0`, `CastTime=0`). It resolves and successfully applies
  the move before the spell base commits SP, cooldown, `Event.SpellCast` or the duration-GE i-frame. A numerical
  zero, unavailable destination or failed `SetActorLocation` ends without those side effects. All destination
  state is activation-local, so no cached result crosses a windup/re-entry boundary; local prediction and the
  authoritative server each resolve the same collision query.
- Strengthened RED: `Saved/TestLogs/issue25-strengthened-red.log` — 2 PASS / 3 FAIL. The isolated endpoint
  fixture made its Pawn ignore the path-trace channel while retaining real Pawn capsule blocking, and failed
  only the endpoint-adjusted/free assertions. `NoMoveNoCost` additionally proved the old zero move spent
  2 SP, broadcast one cast event, started cooldown and granted Invulnerable. The preceding relative-log attempt
  is preserved as infrastructure exit 2 because the editor produced no relative-path log.
- Mutation proof: with only endpoint resolution bypassed,
  `Saved/TestLogs/issue25-endpoint-mutation-red.log` was 4 PASS / 1 FAIL:
  `DestinationPawnAdjustedToFreeSpot` failed both endpoint-adjusted and free-capsule assertions. Restoring the
  resolver produced 5/5 PASS in `Saved/TestLogs/issue25-post-mutation-green.log`.
- Local GREEN: final compile exit 0; focused Blink 5/5 PASS; retained
  `Mordecai.Recovery.Blink.InvulnerabilityExpires` 1/1 PASS
  (`Saved/TestLogs/issue25-expiry-green.log`); all Mordecai automation 786/786 PASS
  (`Saved/TestLogs/issue25-full-green.log`); Python tooling 611/611 PASS; US-087 and US-057 lints PASS; content
  guard PASS; default PIE 16/16 PASS; explicit PickupProjectileTestMap PIE 3/3 PASS.
- Per the coordinator's bounded Phase B, no package, game, story Tier 3/4 or full sweep was run. AC-087.8 and
  AC-087.9 remain unchecked, N20 is not yet marked packaged-FIXED, and this story remains in progress pending
  fresh-commit package/input evidence and the separate visual audit.

### Issue #16 / N11 native pickup characterization (2026-11-08)

- Jeff authorized this fixture on 2026-11-08: “Yes, add the isolated test map and expand the change budget as
  needed for that coverage and validation.” The fence is an enemy-free isolated new map; all existing maps and
  gameplay remain unchanged, and the result does not establish a native-pickup bug. The decision is recorded in
  `Specs/design_decisions_log.md`.
- No runtime behavior changed. The branch adds only `PickupProjectileTestMap`, its editor-authoring/cook/PIE/
  Tier 1 coverage, and the non-AC Tier 4 characterization owner. Blob identity evidence records unchanged
  `DevTestMap`, `MovementTestMap`, and `CombatTestMap`; production projectile/pickup C++ and existing binary
  content are unchanged.
- Pre-final-documentation checks passed: 601 Python tests, 782 Mordecai automation tests, the 16-check default
  PIE suite, and the 3-check explicit PickupProjectileTestMap PIE run. These checks preceded final documentation
  commit `c9e51f2`; they are not represented as exact-final-HEAD reruns. The Tier 1 map test establishes the
  saved-map contract (native class, item/mesh/material provenance, 120 cm QueryOnly trigger, Pawn overlap,
  projectile-channel ignore, and enemy-/blocker-free lane); cooking and packaged behavior are Tier 3/4 proof.
- Final historical package evidence is the clean Development package of
  `c9e51f2bf1861a01c39d1ad3e736e1b060bcc8ff`, built 2026-11-08 08:10:33 MST with `dirty=false` and
  `source_changed_during_build=false`. Dedicated run `Saved/Playtests/20261108-081122-us087-fireball` PASS:
  Slate Q spent SP 10→7, a visible projectile was retained, and the exact stationary TestDummy changed
  200→175. Dedicated non-AC run `Saved/Playtests/20261108-081140-us087-pickup-lane` PASS: Slate Q spent
  SP 10→7; one exact projectile was retained for 10 contiguous samples, intersected the 3D 120+30 cm pickup
  envelope, and cleared the far plane; the pickup stayed stationary and the lane stayed enemy-free.
- The fresh `20261108-081210-sweep` is not all green: 7 owners PASS and 6 owners FAIL, with 14 failed checks.
  Thirteen checks align with supplied live-open issue mappings: movement `facing_mouse` (#34),
  `facing_stick` (#34/#33), `gamepad_move`/`sprint_l3` (#33); magic `cooldown_widgets` (#13),
  `fireball_cooldown_state`/`cooldown_hud` (#35), `stone_skin_indicator` (#21); pause
  `character_tab_content` (#9), `skills_tab_content`/`feats_tab_content` (#10); weapons
  `cycle_prev_shift_tab` (#14); combat input `jump_gamepad_a` (#85). Arena `enemies_present` is an untracked
  arena-spacing assertion failure: observed post-launch distances were about 263/263/228 cm while all three
  Frontliners were alive at 200/200 in Attack. Those positions do not prove authored spawn spacing, closed
  issue #18 is only the HP fix, and US-085 is not an open issue.
- Historical comparator `d22e11b243e5c318902ddc67f0c6afc144424e91` is not current base
  `15f59cde887eb10d0d3ce3c19423942908b05a4e`. Between them only the Recovery test helper header (+97),
  Recovery tests (+166/−34), and this story (+4/−4) changed; gameplay, scripts, content, and config did not.
  Development binaries are not claimed byte-identical because reflected test types affect them. Raw comparison
  across the 12 existing owners found identical failure sets, no PASS→non-PASS/missing transition, and no added
  check names; the pickup owner is new and has no baseline.
- Earlier FAILs remain visible. `Saved/Playtests/20261108-075724-us087-fireball` stopped before Q when
  `walk_to` ended 51 cm off-line against the existing 50 cm aim tolerance (the walk tolerance is 70 cm);
  no policy or owner change follows. In `Saved/Playtests/20261108-005745-us087-pickup-lane`, every retained
  player state stayed at 55 HP while a Frontliner changed 200→175 as the Fireball disappeared. With no direct
  collision-event trace, that run is enemy interference and inconclusive—not player damage and not proof of
  native-pickup collision.
- Historical frame inspection was already recorded for the c9 evidence. The still does not prove the full
  trajectory; the exact-identity sampled state audit does. US-087 remains in backlog: Blink AC-087.6/.8/.9
  are open, and `us087_fireball.py` remains the sole owner of AC-087.4/.5 while the pickup script has empty
  `COVERS` and `proves=()`.

### Issue #20 / N15 Fireball slice (2026-11-06)

Issue #20 / N15 Fireball slice complete at implementation commit `66bcbc82c54cf48984e879bb60cfc38722354e41`; US-087 remains open in backlog for Blink AC-087.6/.8/.9. AC-087.7 remains the prior US-101 completion. N19/N20 are untouched, and this slice does not claim that the wider spell feature is playable.

- R1 follow-up strengthened the delivered Fireball slice at the final PR head: UE 5.7 source and a red deferred-spawn yaw-90 test confirmed that the engine's default local-space initialization rotated an already-world-space velocity twice. The compatibility fix disables that transform only before deferred component initialization, preserving normal-spawn/post-init callers. The tests also captured signed spawn clearance before the first movement tick and proved a WorldStatic-stopped projectile cannot remain as a channel-7 pawn obstacle. Correction from issue #88: PR #87's unbegun fixture observed only the non-ASC overlap-list entry, not the gated component delegate; issue #88 adds that real callback proof separately. The added Tier 2 check is explicitly `[LOGIC]`; it temporarily isolates the pawn, directly activates Fireball, and observes a projectile moving across editor ticks.
- Process note: historical closing commit `5acc5c01` touched this story and `Specs/agent_playtest_plan_v1.md`, rather than only `stories/` as Step 6 prescribes. It had no runtime/content effect. Final exact-head package and playtest evidence is therefore recorded in PR #87 and exported artifacts without another tracked evidence-only commit.
- Red: `Saved/TestLogs/tests-20261106-211014.log` — all four compiling `Mordecai.Recovery.Fireball.*` regression tests failed for the pre-fix projectile behavior.
- Tier 1 green: `Saved/TestLogs/tests-20261106-213309.log` — 4/4 focused tests PASS; `Saved/TestLogs/tests-20261106-220852.log` — all 781 Mordecai tests PASS. Historical correction from issue #88: that PR #87 run recorded a registered WorldDynamic overlap-list contact, then damage/status through the later blocking Pawn path; because its world had not begun play, it did not prove delivery of the non-ASC overlap callback. The static/pierce test did preserve nonzero-pierce delivery.
- Tier 2 green: `py Scripts/run_pie_verify.py` PASS. Python tooling: 597 tests PASS. US-087 plan/lint and US-057 lint PASS. `py Scripts/check_content_committed.py` PASS; no content changed.
- Package: `Saved/Playtests/package-20261106-221358.log` PASS; `PackagedBuilds/Playtest/build_info.json` records clean commit `66bcbc82c5`, `dirty=false`, and `source_changed_during_build=false`.
- Dedicated Tier 3 + Tier 4: `Saved/Playtests/20261106-222925-story-us087/summary.md` PASS. Tier 3 `Saved/Playtests/20261106-222925-boot` PASS. Tier 4 `Saved/Playtests/20261106-222949-us087-fireball` PASS: Q reduced SP 10→7; the visible projectile was sampled at 0.825 s; retained `BP_MordecaiEnemy_TestDummy_C_1` stayed live/stationary and changed 200→175 at 1.568 s.
- Full fresh-package sweep: baseline `Saved/Playtests/20261106-205142-sweep` vs final `Saved/Playtests/20261106-223104-sweep`. Shared `us057_magic.py` checks `fireball_projectile` and `fireball_hits_enemy` changed FAIL→PASS; there were zero PASS→FAIL check transitions. The sweep remains exit 1 for pre-existing unrelated failures (arena spacing, Stone Skin indicator, gamepad jump, and filed known issues); no runtime timing was changed for them.
- Final sweep's dedicated rerun `Saved/Playtests/20261106-223536-us087-fireball` also PASS: visible projectile at 0.567 s and the same exact dummy 200→175 at 1.160 s. Shared run `Saved/Playtests/20261106-223342-us057` observed the projectile at 0.44 s and TestDummy 200→175 at 0.48 s.
- Screenshots opened: dedicated Tier 3 `engine_loading.png` shows the Project Mordecai loading screen; `engine_01.png`–`engine_03.png` show CombatTestMap, its HUD, player, and full-health TestDummy. Dedicated `bridge_01.png` shows SP at 7/10 and the small dark placeholder sphere in flight; `bridge_02.png` is the immediate post-hit frame, with the target off-camera, so exact identity and 200→175 health are established by the continuous state/bridge audit rather than pixels. Shared `Saved/Playtests/20261106-223342-us057/bridge_01.png` visibly shows that TestDummy with its reduced red health bar and SP 7/10.
- Full-sweep Tier 3 frames opened: DevTestMap shows the arena, pickups, Frontliners, HUD, and later the expected unattended death screen; MovementTestMap shows the player/HUD on the empty grid; CombatTestMap shows the player/HUD and full-health TestDummy. All three loading frames show the Project Mordecai loading screen. Final dedicated sweep frames again show the projectile in flight followed by the immediate post-hit frame.
