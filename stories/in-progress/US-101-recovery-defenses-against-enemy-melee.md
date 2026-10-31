# US-101: Recovery — Block, Parry, Dodge, Blink and Jump Defend Against Enemy Melee

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** US-100 (CombatTestMap and its TestAttacker), US-083 (block, parry, dodge and jump input)
**Paired story:** none (C++ only; no content changes)

## Overview
None of the player's defenses reduce enemy melee damage: a player who blocks, parries, dodges, Blinks or jumps through an enemy swing takes the full 15 damage of a BasicSlash, and only leaving the 150 cm reach avoids it (known issue N32, GitHub issue #53). This story routes every melee hit through the target's defenses when it is applied. It also grants `Mordecai.State.Airborne` while a character is falling, so jump-avoidable sweeps can be jumped. Blink's invulnerability now ends on time (N19), because once damage honors `Mordecai.State.Invulnerable` a stuck tag would make the player permanently invulnerable. Features: block_parry, dodge, stamina, enemies, movement.

## Execution Mode
**HEADLESS**
All changes are C++ (the melee damage path, block, Blink, the player and enemy characters). No asset is created or changed. The Tier 4 playtest uses the existing CombatTestMap TestAttacker (US-100).

## References
- GitHub issue #53. `Specs/agent_playtest_plan_v1.md`: N32 (the source audit), N19 (Invulnerable sticks after Blink), N27 (dodge distance), N30 (stamina never regenerates).
- Done stories whose damage-pipeline ACs were never wired, which this story makes true:
  - US-003 AC-003.7/003.8 (jump-avoid)
  - US-005 AC-005.3 (dodge i-frames)
  - US-006 AC-006.3/006.4/006.5/006.9/006.10/006.13 (block, guard break, perfect block, parry)
  - US-007 AC-007.5 (blocked posture ×0.5)
  - US-021 AC-021.3 (Blink invulnerability)
- `stories/backlog/US-087-recovery-fireball-and-blink.md` AC-087.7. This story delivers that AC's fix (a duration GameplayEffect grants Invulnerable) and its Tier 1 test `Mordecai.Recovery.Blink.InvulnerabilityExpires`. US-087 keeps its Tier 4 check `blink_invulnerability_ends` (AC-087.9).
- `Specs/design_decisions_log.md` 2026-10-30 "Defenses against enemy melee (issue #53)": the decisions this story takes on Jeff's behalf.
- `Specs/control_bindings_v1.1.md`:
  - Block: LT (hold) / Right Mouse (hold)
  - Parry: LT + RT / Right Mouse + Left Mouse (timed)
  - Dodge: B / Left Alt
  - Jump: A / Space
  - Move: WASD
- Existing code:
  - `Source/LyraGame/Mordecai/Combat/MordecaiGA_MeleeAttack.cpp` `ApplyDamageToTarget` (the enemy's and the player's melee, light and heavy)
  - `MordecaiGA_Block.cpp` `ReceiveBlockedHit`, `MordecaiGA_Parry.cpp` `ReceiveParriedHit`
  - `MordecaiHitDetectionSubsystem.cpp` `IsTargetAirborne` / `ShouldExcludeTarget`
  - `Magic/MordecaiGA_Blink.cpp`, `MordecaiCharacter.cpp`, `Enemy/MordecaiEnemyCharacter.cpp`
- Content values (from `Scripts/us054_01_create_data_assets.py`, `Scripts/us100_01_create_test_enemies.py`):
  - `DA_EnemyAttack_BasicSlash`: ArcSector 150 cm / 90°, windup 500 ms, Slash BasePower 15, PostureDamageScalar 0.3, JumpAvoidable true, HitsAirborne false
  - TestAttacker: one swing about every 2.6 s from (2100, 0) on CombatTestMap

## Acceptance Criteria
- [ ] AC-101.1: An enemy melee hit on a player who is not defending lands in full: Health −BasePower, Posture −BasePower × PostureDamageScalar. The player's own melee hits on enemies are unchanged — verified by `Mordecai.Recovery.Defense.UndefendedHitLandsInFull` and `Mordecai.Recovery.Defense.PlayerHitsOnEnemiesUnchanged`
- [ ] AC-101.2: Block (AC-006.3, AC-006.4, AC-007.5): an enemy melee hit on a player whose block is active, after the perfect-block window, applies Health damage × (1 − Mitigation) exactly once, drains Stamina by damage × (1 − Stability), and applies Posture damage × `BlockPostureDamageMultiplier`. With the code defaults a BasicSlash costs 7.5 health, 10.5 stamina and 2.25 posture — verified by `Mordecai.Recovery.Defense.BlockMitigatesEnemyHit` and `Mordecai.Recovery.Defense.TestAttackerSwingIsBlocked`
- [ ] AC-101.3: Perfect block (AC-006.9, AC-006.10): a hit within `PerfectBlockWindowMs` of raising the block is mitigated as in AC-101.2, but drains no stamina and deals `PerfectBlockPostureDamage` to the attacker's posture. `Mordecai.State.PerfectBlock` goes away when its window ends, even after a perfect block — verified by `Mordecai.Recovery.Defense.PerfectBlockSparesStaminaAndHurtsAttacker`
- [ ] AC-101.4: Guard break (AC-006.5): a blocked hit that leaves stamina at or below 0 breaks the guard (`Mordecai.State.GuardBroken`), and hits during the guard break land in full — verified by `Mordecai.Recovery.Defense.GuardBrokenTakesFullHits`
- [ ] AC-101.5: Parry (AC-006.13): an enemy melee hit during an active parry window has no effect on the player: no health or posture loss, no on-hit payloads or status effects. The attacker takes `ParryPostureDamage` and `Mordecai.State.Parried`, the player gets `Mordecai.State.Riposte`, and the parry ends without a whiff — verified by `Mordecai.Recovery.Defense.ParryNegatesEnemyHit`
- [ ] AC-101.6: Dodge i-frames (AC-005.3): an enemy melee hit while `Mordecai.State.Dodging` is on has no effect: no health or posture loss, no on-hit payloads or status effects, no damage number. The first hit after the dodge lands in full — verified by `Mordecai.Recovery.Defense.DodgeIFramesNegateEnemyHit`
- [ ] AC-101.7: Blink (AC-021.3, N19): an enemy melee hit while Blink's `Mordecai.State.Invulnerable` is on has no effect. The tag is removed when Blink's invulnerability duration ends, even when the spell ended first (BuffDuration 0.3 s > RecoveryTime 0.2 s, as in `DA_Spell_Blink`). The next hit lands — verified by `Mordecai.Recovery.Defense.BlinkInvulnerabilityNegatesEnemyHit` and `Mordecai.Recovery.Blink.InvulnerabilityExpires` (US-087 AC-087.7)
- [ ] AC-101.8: Airborne (AC-003.7): the player and every enemy carry `Mordecai.State.Airborne` exactly while their movement mode is Falling. That covers a jump, a drop off a ledge and a launch. The tag is seeded when the player's ASC is attached and cleared when it is detached — verified by `Mordecai.Recovery.Defense.FallingGrantsAirborne` and `Mordecai.Recovery.Defense.EnemyFallingGrantsAirborne`
- [ ] AC-101.9: A falling player is not hit by the TestAttacker's `DA_EnemyAttack_BasicSlash` (JumpAvoidable). The same player on the ground is hit, and a HitsAirborne swing hits it in the air — verified by `Mordecai.Recovery.Defense.JumpAvoidsBasicSlash`
- [ ] AC-101.10: In the packaged build on CombatTestMap, a player standing in the TestAttacker's reach who does nothing takes a 15-damage swing (the control) — verified by check `undefended_hit` of `Scripts/playtests/us101_defenses.py` (Tier 4)
- [ ] AC-101.11: In the packaged build, the player holds LT through a TestAttacker swing and loses 7.5 health and 10.5 stamina, not 15 health — verified by check `block_mitigates` of `Scripts/playtests/us101_defenses.py` (Tier 4)
- [ ] AC-101.12: In the packaged build, the player raises LT within 250 ms before the swing lands and loses 7.5 health and no stamina — verified by check `perfect_block` of `Scripts/playtests/us101_defenses.py` (Tier 4)
- [ ] AC-101.13: In the packaged build, the player taps Left Alt into a TestAttacker swing, holding W so the dodge stays inside the reach, and loses no health — verified by check `dodge_evades` of `Scripts/playtests/us101_defenses.py` (Tier 4)
- [ ] AC-101.14: In the packaged build, the player taps Space into a TestAttacker swing: `player.tags` shows `Mordecai.State.Airborne` while falling and no health is lost — verified by check `jump_evades` of `Scripts/playtests/us101_defenses.py` (Tier 4)
- [ ] AC-101.15: In the packaged build, the player holds LT and presses RT just before a TestAttacker swing lands: no health is lost, `Mordecai.State.Riposte` is on the player and `Mordecai.State.Parried` is on the attacker — verified by check `parry_negates` of `Scripts/playtests/us101_defenses.py` (Tier 4)
- [ ] AC-101.16: No content is created or changed; `py Scripts/check_content_committed.py` exits 0

## Technical Notes
**Damage path**
- New `FMordecaiHitDefense::ResolveIncomingHit(TargetASC, AttackerASC, IncomingHealthDamage, HitTime)` (`Source/LyraGame/Mordecai/Combat/MordecaiHitDefense.{h,cpp}`) returns an `EMordecaiHitDefense`:
  - `Evaded`: Invulnerable or Dodging
  - `Parried`: an active parry instance whose `ReceiveParriedHit` succeeds
  - `Blocked` / `PerfectBlocked`: an active block instance in the Active phase; `ReceiveBlockedHit` applies the mitigated health, the stamina drain, the guard break and the perfect-block posture
  - `None`
- `UMordecaiGA_MeleeAttack::ApplyDamageToTarget` calls the resolver first, so the light and heavy attack go through it in both directions:
  - Evaded or Parried: it returns before any health, posture, enchant, on-hit payload, status effect or damage number.
  - Blocked: it skips the health GE (the block applied the mitigated damage), scales the posture GE by `UMordecaiPostureSystem::BlockPostureDamageMultiplier`, and shows the mitigated number.
- The active Block/Parry instance is found by scanning `GetActivatableAbilities()` with `IsA` (Blueprint subclasses included), and its receive function is called after the scan, because `ReceiveParriedHit` ends the ability.
- `ReceiveBlockedHit` no longer adds a second `Mordecai.State.PerfectBlock` count on a perfect block. The window already holds the tag, and the extra count never came off.
- Every defended hit is logged on `LogMordecaiDefense` (Log), so a playtest's `game.log` says what happened.

**Airborne**
- `AMordecaiCharacter` and `AMordecaiEnemyCharacter` override `OnMovementModeChanged` and set the `Mordecai.State.Airborne` loose-tag count to 1 or 0 from `CMC->IsFalling()`, through `UMordecaiHitDetectionSubsystem::SyncAirborneTag`. Setting the count, as Lyra does for `Movement.Mode.*`, means it cannot drift.
- The player seeds the tag in `OnAbilitySystemInitialized` and clears it in `OnAbilitySystemUninitialized`. The enemy seeds it in `InitializeAbilitySystem`. `IsTargetAirborne` is unchanged.

**Blink (N19)**
- The root cause: `UGameplayAbility::EndAbility` calls `ClearAllTimersForObject(this)`. Blink's recovery (0.2 s) ends the ability before its 0.3 s invulnerability timer fires, so the tag was never removed.
- Invulnerability is now a HasDuration GameplayEffect granting `Mordecai.State.Invulnerable` (`UTargetTagsGameplayEffectComponent`, the Stone Skin pattern), applied to the caster. GAS removes it on expiry, whatever the ability's lifetime.

**Other**
- Melee only. Projectiles (`AMordecaiProjectile::ApplyDamageToTarget`), Cone of Cold and Burning ticks do not consult the defenses yet. No enemy uses them against the player today.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

**Decisions taken for Jeff (2026-10-30, `Specs/design_decisions_log.md`); he may revisit:**
1. Precedence: i-frames (Invulnerable, Dodging), then parry, then block. Airborne is decided earlier, in the hit query.
2. I-frames and a successful parry negate the whole hit: health, posture, enchant bonus, on-hit payloads, status effects and the damage number. This follows "incoming damage is negated" (AC-005.3), "immune to all damage" (the Invulnerable tag) and `ReceiveParriedHit` taking no damage argument.
3. A perfect block still mitigates health, as `ReceiveBlockedHit` already did; AC-006.10 is silent.
4. Blocked hits take posture × 0.5 (AC-007.5).
5. "Airborne" means any Falling movement mode, not only a jump.
6. Blink's fix lands here, ahead of US-087.

**TODO(DECISION) (left as they are, in the code)**
- Is block frontal only? `skill_sheet_v1.1.md` shield ranks say "frontal"; the code and US-006 have no direction.
- Do blocked hits still apply on-hit payloads, status effects and the enchant bonus? They do today, and nothing that enemies swing has any.
- Durations of `Mordecai.State.Parried` and `Mordecai.State.Riposte`. Both are loose tags that nothing removes (filed as known issue N33).
- Should projectiles, spells and DoT ticks also honor i-frames and block?
- AC-005.9: the perfect-dodge stamina refund still has no caller; the resolver's Evaded branch is its hook.

## Tests Required
### Tier 1 — Automation (headless)
`Source/LyraGame/Tests/Recovery/MordecaiRecoveryDefenseTests.cpp`:
- `Mordecai.Recovery.Defense.UndefendedHitLandsInFull`, `Mordecai.Recovery.Defense.PlayerHitsOnEnemiesUnchanged` — AC-101.1
- `Mordecai.Recovery.Defense.BlockMitigatesEnemyHit`, `Mordecai.Recovery.Defense.TestAttackerSwingIsBlocked` — AC-101.2
- `Mordecai.Recovery.Defense.PerfectBlockSparesStaminaAndHurtsAttacker` — AC-101.3
- `Mordecai.Recovery.Defense.GuardBrokenTakesFullHits` — AC-101.4
- `Mordecai.Recovery.Defense.ParryNegatesEnemyHit` — AC-101.5
- `Mordecai.Recovery.Defense.DodgeIFramesNegateEnemyHit` — AC-101.6
- `Mordecai.Recovery.Defense.BlinkInvulnerabilityNegatesEnemyHit`, `Mordecai.Recovery.Blink.InvulnerabilityExpires` — AC-101.7
- `Mordecai.Recovery.Defense.FallingGrantsAirborne`, `Mordecai.Recovery.Defense.EnemyFallingGrantsAirborne` — AC-101.8
- `Mordecai.Recovery.Defense.JumpAvoidsBasicSlash` — AC-101.9

Every test applies a real enemy swing: the enemy's `UMordecaiGA_MeleeAttack` activates, and its windup timer runs the hit query. Each defense is raised through the player's real combat abilities and input tags; Blink is cast. None of these tests is tag-only.

### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_defenses.py`:
  - the grounded player has no `Mordecai.State.Airborne`
  - its ASC has the Block, Parry and Dodge abilities
  - `[LOGIC]` `jump()`, then on the next phase the player is Falling and has `Mordecai.State.Airborne`

### Tier 3 — Packaged playtest
- See Playtest

### Tier 4 — Real-input playtest
- `Scripts/playtests/us101_defenses.py` (CombatTestMap): one check per defense against the TestAttacker's swing, synchronised to its Recover → Attack edge, plus an undefended control swing.
  - Offline tests: `Scripts/tests/test_us101_playtest.py`, against `Scripts/tests/fake_bridge.py`.
- Real input only (key/axis through the bridge); no `debug_*` or direct calls.

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience). Tier 4: `py Scripts/playtest_story.py US-101` on the same package; its script's `MAP` is CombatTestMap.
- **Player steps:**
  1. Walk (WASD) to the fight spot (2000, 0), 100 cm −X of the TestAttacker (2100, 0).
  2. Stand through one swing.
  3. Hold LT (`Gamepad_LeftTriggerAxis`) through the next swing.
  4. Raise LT about 0.30 s after a swing starts (inside the 250 ms before the hit at about 0.5 s).
  5. Press W about 0.15 s after a swing starts and tap Left Alt at about 0.25 s, so the dodge goes +X into the attacker and stays in reach; release W.
  6. Tap Space about 0.15 s after a swing starts.
  7. Hold LT and press RT (`Gamepad_RightTriggerAxis`) about 0.25 s after a swing starts.
- **Visible on screen:** the player next to the red TestAttacker on the CombatTestMap floor. The HUD health and stamina bars drop after the control and block swings and stay put after the dodge, jump and parry swings. "Perfect Block!" / "Parry" feedback may flash.
- **Proof:** Tier 3 boot of CombatTestMap shows the arena, the HUD and the TestAttacker. Tier 4 checks `undefended_hit`, `block_mitigates`, `perfect_block`, `dodge_evades`, `jump_evades` and `parry_negates` in `Scripts/playtests/us101_defenses.py`.
- **Tier 4 script:** `Scripts/playtests/us101_defenses.py`, run with `py Scripts/playtest_story.py US-101`
- **Steps:** (generated: `py Scripts/playtest_story.py US-101 --plan`; checked: `--lint`)

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `walk_to_fight_spot` | walk_to(2000, 0) -> within 25 cm (100 cm -X of the TestAttacker at (2100, 0)); state actors (radius 3000) -> the TestAttacker (by label, else class) alive, within its 150 cm reach | - |
| 2 | `undefended_hit` | no input: wait_until the TestAttacker's actors[] ai_state leaves 'Attack', read player, wait_until it is 'Attack' (the edge, poll_s 0), wait_until player.attributes.health drops; after the swing -> health -15 and posture -4.5 (+-0.5): the control, the swing lands at this spot | AC-101.10 |
| 3 | `block_mitigates` | axis Gamepad_LeftTriggerAxis=1 held from the attacker's Recover ('Mordecai.State.Blocking' seen before the edge) through its swing -> health -7.5, not -15, and stamina -10.5 (+-0.5); then Gamepad_LeftTriggerAxis=0 | AC-101.11, US-006:AC-006.3, US-006:AC-006.4 |
| 4 | `perfect_block` | Gamepad_LeftTriggerAxis up at the edge; ~0.3s after it (shifted by the measured hit time) axis Gamepad_LeftTriggerAxis=1, wait_until health drops -> health -7.5 and stamina unchanged (+-0.5); the edge, LT and hit times are in the detail | AC-101.12 |
| 5 | `dodge_evades` | press W ~0.15s and tap LeftAlt ~0.25s after the edge (a dodge +X, into the attacker), release W; poll state -> 'Mordecai.State.Dodging' seen, health unchanged (+-0.5), and the player within 150 cm of the attacker at the reads around the expected hit (needs the control) | AC-101.13, US-005:AC-005.3 |
| 6 | `jump_evades` | tap SpaceBar ~0.15s after the edge; poll state -> 'Mordecai.State.Airborne' in player.tags at a read with player.movement_mode=="Falling", health unchanged (+-0.5), within reach around the expected hit, and no 'Mordecai.State.Airborne' once Walking again (needs the control) | AC-101.14, US-003:AC-003.7 |
| 7 | `parry_negates` | axis Gamepad_LeftTriggerAxis=1 held from Recover ('Mordecai.State.Blocking' seen), axis Gamepad_RightTriggerAxis=1 for 0.1s ~0.25s after the edge (the parry chord) -> health unchanged (+-0.5), 'Mordecai.State.Riposte' in player.tags and 'Mordecai.State.Parried' in the attacker's actors[] tags; then Gamepad_LeftTriggerAxis=0 (last: N33; needs the control) | AC-101.15 |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none expected; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] Input-driven ACs: `py Scripts/playtest_story.py US-101` (Tier 3 plus the Tier 4 `py Scripts/playtests/us101_defenses.py`) exits 0 on that same fresh package, and its summary's Acceptance criteria table shows AC-101.10–AC-101.15 accepted
  - `py Scripts/playtest_story.py US-101 --lint` exits 0
  - never `--allow-stale`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Known issue N32 marked FIXED and N19 updated in `Specs/agent_playtest_plan_v1.md` with the run dir
- [ ] Evidence cited: run dirs (Tier 3 `Saved/Playtests/<run>-boot/report.md`, Tier 4 `Saved/Playtests/<run>-us101/report.md`) in Completion Notes AND in the body of the `[US-101]` commit that closes the story
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass. A failing playtest is reported honestly and the story stays in progress

## Completion Notes
<Filled in by the coding agent.>
