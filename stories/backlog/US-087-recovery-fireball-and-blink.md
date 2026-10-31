# US-087: Recovery — Fireball Projectile Flies and Hits; Blink Moves and Ends Its Invulnerability

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none (the hit is observable on the invisible enemies; US-085 makes them visible)
**Paired story:** none

## Overview
In the packaged arena Fireball spends 3 SP and does nothing visible: no projectile actor appears and the enemy ahead keeps its health (N15, possibly N11). The exploratory agent also found that Blink can spend SP and move the player 0 cm when an enemy is ahead (N20) and that Blink's i-frame tag `Mordecai.State.Invulnerable` is never removed (N19) — a permanently invulnerable player. This story makes the two movement/damage spells of the magic slice behave. Features: spells, projectiles_aim, status_effects.

## Execution Mode
**HEADLESS**
All fixes are C++ (projectile spawn/initialisation and overlap filtering, Blink destination trace and tag lifetime). The spell DataAssets already exist and are unchanged.

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
- [ ] AC-087.1: A Fireball projectile moves at `SpellData->ProjectileSpeed` along the (assisted) aim direction from its first tick, and its lifetime/range come from the spell data — verified by `Mordecai.Recovery.Projectile.FliesAtSpecSpeedFromSpawn` and `Mordecai.Recovery.Projectile.LifetimeFromSpec`
- [ ] AC-087.2: A projectile that overlaps an actor without an ability system component (pickup trigger, prop) is not destroyed and deals no damage; it still stops on WorldStatic and on damageable targets — verified by `Mordecai.Recovery.Projectile.IgnoresNonDamageableOverlaps`
- [ ] AC-087.3: The projectile spawns in front of the caster's capsule and has a visible mesh (placeholder sphere is fine) — verified by `Mordecai.Recovery.Projectile.SpawnsAheadWithVisibleMesh`
- [ ] AC-087.4: In the packaged build, the player taps Q facing an enemy and a projectile appears — verified by check `fireball_projectile` of `Scripts/playtests/us057_magic.py` (Tier 4)
- [ ] AC-087.5: In the packaged build, that Fireball damages the enemy ahead within 2.5 s — verified by check `fireball_hits_enemy` of `Scripts/playtests/us057_magic.py` (Tier 4)
- [ ] AC-087.6: Blink's destination trace ignores Pawns (it clamps on world geometry only; the destination is adjusted to a free spot if it lands in a pawn), so Blink with an enemy ahead still moves the player; if no movement is possible the spell does not commit its SP cost — verified by `Mordecai.Recovery.Blink.EnemyAheadDoesNotBlock` and `Mordecai.Recovery.Blink.NoMoveNoCost`
- [x] AC-087.7: Blink's invulnerability is removed when its duration ends even if the ability has already ended, and never outlives it — verified by `Mordecai.Recovery.Blink.InvulnerabilityExpires` (Stone Skin's own Invulnerable grant is unaffected; prefer a duration GameplayEffect granting the tag over a loose tag + ability-owned timer)
  - **Delivered by US-101 (issue #53, `d490a2d`, 2026-10-30).** Once US-101 made damage honor `Mordecai.State.Invulnerable`, a stuck tag would have meant a permanently invulnerable player, so the fix could not wait for this story.
    - The fix is the one this AC prescribes: a duration GE grants the tag.
    - Root cause: `UGameplayAbility::EndAbility`'s `ClearAllTimersForObject` killed the ability-owned removal timer. The "actor info cleared" suspect above is not the mechanism.
    - Note: Stone Skin grants no Invulnerable; it grants `Status.StoneSkin` (`MordecaiGA_StoneSkin.cpp`).
    - `Mordecai.Recovery.Blink.InvulnerabilityExpires` passes; it failed before the fix.
    - AC-087.9's Tier 4 check `blink_invulnerability_ends` is still this story's.
- [ ] AC-087.8: In the packaged build, the player taps E with an enemy about 1 m ahead and moves > 300 cm — verified by NEW check `blink_past_enemy` added to `Scripts/playtests/us057_magic.py` (Tier 4)
- [ ] AC-087.9: In the packaged build, `Mordecai.State.Invulnerable` is gone from `player.tags` within 1.5 s after a Blink (no Stone Skin active) — verified by NEW check `blink_invulnerability_ends` added to `Scripts/playtests/us057_magic.py` (Tier 4)

## Technical Notes
- Fireball: use `SpawnActorDeferred<AMordecaiProjectile>` → `InitFromSpec` / `SetProjectileInstigator` / sphere radius → `FinishSpawning`, so `InitializeComponent` and `BeginPlay` see the spec; or set `ProjectileMovement->Velocity` explicitly. Apply the same to `MordecaiGA_MagicMissile.cpp:129-155`.
- Spawn origin: `Origin + AimForward * (capsule radius + sphere radius + small margin)`.
- Visible mesh: a `UStaticMeshComponent` with an engine basic shape referenced from C++ (hard ref, so it is cooked); no collision on the mesh.
- N11: filter in `OnProjectileOverlap` — skip actors with no ASC (`UAbilitySystemGlobals::GetAbilitySystemComponentFromActor`) unless they are WorldStatic blockers.
- Blink (N20): trace with a channel/object query that excludes Pawns, then if the destination overlaps a pawn, step back along the line to the first free capsule position; decide "no movement possible" before committing cost. (N19): grant invulnerability through a duration `UGameplayEffect` (granted tag, Duration = `GetInvulnerabilityDuration()`), so GAS removes it; Stone Skin's own grant stays separate.
- New us057 checks (script edit committed with the code BEFORE packaging; added to `PLAN` with `proves` = `US-087:AC-087.8` / `US-087:AC-087.9`, US-087 added to `COVERS`; offline test against `Scripts/tests/fake_bridge.py`):
  - `blink_past_enemy`: walk to ~100 cm -X of the TestDummy (`TARGET_ENEMY`, (600, 600)) on its line, about (500, 600) (found like `walk_shot_line` does), tap E → `player.location.x` rises > 300 within 2 s. Never use the TestAttacker (2100, 0): its 900 cm AggroRange and 150 cm BasicSlash would hit or kill the player, and us057's route stays out of its range (US-100). Mind the SP plan (10 → 7 → 5 → 1 today, N6): budget the extra Blink (it may need its own launch segment or a second run).
- Since US-100 us057 plays CombatTestMap (`MAP = kit.COMBAT_TEST_MAP`): it shoots the TestDummy from the shot spot (300, 600) and Blinks along the lane from (0, -600). CombatTestMap has no pickups, so N11 (pickup trigger spheres) is covered by AC-087.2's Tier 1 test only.
  - `blink_invulnerability_ends`: after the existing `blink` step, wait_until `Mordecai.State.Invulnerable` not in `player.tags` within 1.5 s (before Stone Skin is cast).
- Script bookkeeping: `fireball_projectile` and `fireball_hits_enemy` were NEW (no `known_issue`). `fireball_hits_enemy`'s `does` text says "pre-cast value (200)"; it compares with the pre-cast reading, keep it that way.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Projectile.FliesAtSpecSpeedFromSpawn`, `Mordecai.Recovery.Projectile.LifetimeFromSpec` — AC-087.1
- `Mordecai.Recovery.Projectile.IgnoresNonDamageableOverlaps` — AC-087.2
- `Mordecai.Recovery.Projectile.SpawnsAheadWithVisibleMesh` — AC-087.3
- `Mordecai.Recovery.Blink.EnemyAheadDoesNotBlock`, `Mordecai.Recovery.Blink.NoMoveNoCost` — AC-087.6
- `Mordecai.Recovery.Blink.InvulnerabilityExpires` — AC-087.7
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_magic.py`: `[LOGIC]` cast Fireball → a projectile actor exists and moves between two ticks; `[LOGIC]` Blink → Invulnerable removed after its duration
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us057_magic.py`: existing `fireball_projectile`, `fireball_hits_enemy`; new `blink_past_enemy`, `blink_invulnerability_ends`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience; us057's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-057` (or `US-087` once in `COVERS`) on the same package, which boots the script's map
- **Player steps:** walk (WASD) to the shot spot (300, 600), 300 cm -X of the TestDummy (600, 600), tap Q (Fireball); walk to (0, -600) and tap E (the existing `blink`); for `blink_past_enemy` walk to about (500, 600), ~100 cm -X of the TestDummy on its line, tap E (Blink); wait 1.5 s
- **Visible on screen:** `fireball_hit` screenshot: the projectile or its impact near the TestDummy (blue-grey cylinder) and the SP bar at "7 / 10"; after `blink_past_enemy` the player on the far (+X) side of the TestDummy
- **Proof:** Tier 4 checks `fireball_projectile`, `fireball_hits_enemy`, `blink_past_enemy`, `blink_invulnerability_ends` of `Scripts/playtests/us057_magic.py`
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` — run: `py Scripts/playtest_story.py US-057`; `py Scripts/playtest_story.py US-087` after US-087 is added to `COVERS`
- **Steps:** (from `py Scripts/playtest_story.py US-057 --plan`; paste the new rows after implementing)

| Script | Check | Player step (bridge calls) → expected | Sweep / explore result (8ea853b) |
|---|---|---|---|
| us057 | `fireball_projectile` | same poll -> an actors[] entry with kind=="projectile" within 1.5s of the tap | FAIL (NEW) |
| us057 | `fireball_hits_enemy` | same poll -> the nearest enemy's health drops below its pre-cast value within 2.5s | FAIL (NEW) |
| us057 | `blink_past_enemy` (new) | walk ~100 cm -X of the TestDummy on its line, tap E -> player.location.x rises > 300 within 2 s | explore: moved 0 cm (N20) |
| us057 | `blink_invulnerability_ends` (new) | after `blink`: wait_until Mordecai.State.Invulnerable not in player.tags within 1.5 s | explore: tag stuck 6+ s (N19) |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none expected; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + us057 script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-087` (Tier 3 + Tier 4) on that same fresh package: `fireball_projectile`, `fireball_hits_enemy` go from FAIL (sweep `Saved/Playtests/20261022-184702-sweep`) to PASS, the new `blink_past_enemy` and `blink_invulnerability_ends` PASS, and the summary's Acceptance criteria table shows AC-087.4/.5/.8/.9 accepted; no check that PASSed in the sweep now FAILs; `--lint` exits 0 for US-087 and US-057 — never `--allow-stale`
- [ ] Known issues N15, N11, N19, N20 marked FIXED (or N11 cleared) in `Specs/agent_playtest_plan_v1.md` with the run dir
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-087]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
