# US-122: Stamina Runtime

**Epic:** Core combat recovery
**Priority:** P0
**Depends on:** US-008, US-083, US-121
**Paired story:** none

## Overview
Issue #143 connects the shipped US-008 stamina mathematics to the real player runtime. The PlayerState ASC owns authoritative regeneration, spend delay, tier tags, sprint drain, and the tier multipliers used by melee, dodge, block, sprint, and the HUD.

## Execution Mode
**HEADLESS**
The production work is C++ GAS/runtime logic plus automation and packaged real-input tooling; no content assets are required.

## References
- `Specs/combat_system_v1.md` — stamina is consumed by attacks, dodge, block, and sprint; low stamina reduces effectiveness without preventing actions
- `Specs/game_design_v2.md` — low stamina never blocks actions
- `Specs/design_decisions_log.md` — Green/Yellow/Red/Exhausted multipliers are 1.25/1.0/0.75/0.5
- `stories/done/US-008-stamina-tier-system.md` — thresholds, regeneration delay, sprint drain, and action effects
- `Specs/control_bindings_v1.1.md` — Left Shift toggles sprint; LMB attacks; Left Alt dodges; RMB blocks
- Existing code: `Source/LyraGame/Mordecai/Combat/MordecaiStaminaSystem.*`, `MordecaiAttributeSet.cpp`, `MordecaiCharacter.cpp`, and `MordecaiStaminaBarWidget.cpp`

## Acceptance Criteria
- [x] AC-122.1: On the authority, the real player runtime passively regenerates stamina after the US-008 delay, using `UMordecaiStaminaSystem::ComputeRegenAmount` scaled by the ASC's `StaminaRegenMultiplier`.
- [x] AC-122.2: Every authoritative stamina decrease (attack, dodge, blocked hit, parry, or sprint) resets one centralized regeneration delay; stamina does not regenerate during that delay.
- [x] AC-122.3: Regeneration stops while `Mordecai.State.Dead` is present; the existing respawn path still restores stamina to MaxStamina.
- [x] AC-122.4: Exactly one of `Mordecai.Stamina.Tier.Green`, `.Yellow`, `.Red`, or `.Exhausted` is present on the ASC and changes immediately whenever Stamina or MaxStamina changes.
- [x] AC-122.5: Green/Yellow/Red/Exhausted tier multipliers 1.25/1.0/0.75/0.5 scale real melee health damage, dodge distance, and block stability/stamina mitigation.
- [x] AC-122.6: Real attack, dodge, and block abilities still activate at zero stamina; low stamina changes effectiveness and never gates activation.
- [x] AC-122.7: Toggling sprint applies `Mordecai.State.Sprinting` on the authoritative ASC and continuously drains stamina with `ComputeSprintDrain`; stopping sprint removes the tag and permits regeneration after the most recent drain delay.
- [x] AC-122.8: Sprint remains active at zero stamina and uses the Exhausted 0.5 multiplier; no automatic stop or stamina gate is introduced.
- [x] AC-122.9: The HUD stamina bar gets its tier from the authoritative ASC tier tags, rather than deriving a second tier from the attribute ratio.
- [x] AC-122.10: In a clean packaged build, real Left Shift input drains the visible stamina bar while sprinting, the bar changes to the authoritative low tier colour/tag, and after sprint stops and the delay elapses the bar refills to Green.

## Technical Notes
- Add one player runtime owner that binds after `OnAbilitySystemInitialized`, because the player's ASC lives on `AMordecaiPlayerState`; execute stamina mutation only on authority.
- Observe the ASC stamina delegate for all decreases so individual abilities do not duplicate spend-delay notifications.
- Preserve the current full-stamina respawn behavior.
- Sprint-at-zero decision for issue #143: continue sprinting and apply the Exhausted 0.5 effectiveness multiplier, matching US-008 AC-008.16 and the “never blocks actions” rule. Record this in `Specs/design_decisions_log.md`.
- Do not change capsule radius 40, half-height 90, or step height 45.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Stamina.Runtime.RegenAfterSpendDelay` — real authority runtime tick regenerates after a real attack spend (AC-122.1, AC-122.2)
- `Mordecai.Stamina.Runtime.NoRegenDuringSpendDelay` — real dodge spend resets and holds the delay (AC-122.2)
- `Mordecai.Stamina.Runtime.RegenUsesAttributeMultiplier` — runtime regeneration scales with `StaminaRegenMultiplier` (AC-122.1)
- `Mordecai.Stamina.Runtime.TierTagsTrackAttribute` — ASC has one authoritative tag while crossing all thresholds (AC-122.4)
- `Mordecai.Stamina.Runtime.LowTierScalesMeleeAndDodge` — real low-tier melee damage and dodge launch are reduced (AC-122.5)
- `Mordecai.Stamina.Runtime.LowTierScalesBlockStability` — a real blocked hit drains stamina using tier-scaled stability (AC-122.5)
- `Mordecai.Stamina.Runtime.SprintDrainsAndTags` — real Character sprint toggle applies the authority tag and drains over runtime ticks (AC-122.7)
- `Mordecai.Stamina.Runtime.ZeroStaminaActionsActivate` — granted attack, dodge, and block activate through their real input tags at zero (AC-122.6)
- `Mordecai.Stamina.Runtime.ZeroStaminaSprintContinues` — sprint remains on and uses the exhausted multiplier after crossing zero (AC-122.8)
- `Mordecai.Stamina.Runtime.DeadDoesNotRegenerate` — dead players do not regenerate (AC-122.3)
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_stamina_runtime.py` — verifies the live PlayerState ASC owns one tier tag and the HUD reads that tag; direct stamina mutations are labelled `[LOGIC]`
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us122_stamina_runtime.py` — real Left Shift sprint drains stamina, applies sprint/tier tags, stops, waits through the delay, observes regeneration, and captures drained/refilled HUD frames
- Real input only (key/axis/mouse/click through the bridge); no `debug_*` or direct calls.

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/MovementTestMap` (MovementTestMap → `B_MordecaiArenaExperience`); Tier 4: `py Scripts/playtests/us122_stamina_runtime.py` on the same package
- **Player steps:** walk with WASD to lane L1; tap Left Shift to start sprint; hold W until stamina reaches a low tier; capture the drained HUD; tap Left Shift to stop; wait through the spend delay and until stamina refills; capture the refilled HUD.
- **Visible on screen:** the Mordecai HUD stamina bar is visibly drained and Red/Exhausted in the first capture, then visibly refilled and Green in the second. Image verification is performed by script and recorded in `image-verification.json`; per issue #143 instructions, PNGs/video are not opened with the file-read tool.
- **Proof:** Tier 3 proves the packaged map and HUD boot; Tier 4 checks `sprint_drains`, `low_tier_authoritative`, `regen_delay`, and `refills_green`, and captures both frames.
- **Tier 4 script:** `Scripts/playtests/us122_stamina_runtime.py` — run: `py Scripts/playtest_story.py US-122`
- **Steps:**

| # | Check | Player step (bridge calls) → expected | Proves |
|---|---|---|---|
| 1 | `walk_lane` | `walk_to` three open-floor waypoints ending at `(-300, -800)` on lane L1 | - |
| 2 | `sprint_drains` | tap LeftShift, hold W in real-input pulses → stamina decreases and `Mordecai.State.Sprinting` is present | AC-122.7 |
| 3 | `low_tier_authoritative` | continue real sprint until an authoritative Red/Exhausted tier tag is present; capture drained HUD | AC-122.4, AC-122.8, AC-122.9, AC-122.10 |
| 4 | `regen_delay` | tap LeftShift to stop → sprint tag is removed and stamina stays unchanged during the configured delay | AC-122.2, AC-122.7 |
| 5 | `refills_green` | wait without input → stamina increases to Green; capture refilled HUD | AC-122.1, AC-122.9, AC-122.10 |
| 6 | `image_verification` | Pillow verifies both frames are non-uniform and detects Red in the drained HUD and Green in the refilled HUD; write `image-verification.json` | AC-122.10 |

- **Playable alone:** YES

## Definition of Done
- [x] All ACs checked off
- [x] Tier 1: project compiles; targeted tests and ALL `Mordecai.*` automation tests pass
- [x] Tier 2: PIE smoke tests pass; direct-call checks labelled LOGIC
- [x] Content guard: `py Scripts/check_content_committed.py` exits 0
- [x] Work committed first, then `py Scripts/package_playtest.py` produces `dirty=false` and `source_changed_during_build=false`
- [x] `py Scripts/playtest_story.py US-122` exits 0 on that fresh package with every input-driven AC accepted
- [x] Drained/refilled screenshots are verified by script only and `image-verification.json` records the checks
- [x] A fresh-main packaged sweep baseline and delivered-package full sweep compare with persistent `pass_to_fail: []`; one same-package rerun is allowed only for #135/#141 transients
- [x] Evidence is bundled under the agent store as `core-143-<shortsha>/` with SHA-256 manifest
- [x] Pushed; story moves to `stories/done/` only after all evidence passes

## Completion Notes
- Delivered code commit: `920209dc3b7285be47badaf15d3aa31f38ed2b5a`.
- Tier 1: Development Editor build PASS; `Mordecai.Stamina.Runtime` 11/11 PASS; full `Mordecai.*` 893/893 PASS. Python playtest tooling 716/716 PASS after the sweep-harness updates.
- Tier 2: full PIE verification PASS; stamina runtime smoke 8/8 PASS. Direct attribute mutations are labelled `[LOGIC]`.
- Content/LFS guards: PASS; no uncommitted content and no LFS pointers.
- Package: `Saved/Playtests/package-20261118-070505.log`; `PackagedBuilds/Playtest/build_info.json` records commit `920209dc3b`, `dirty=false`, and `source_changed_during_build=false`.
- Tier 3: `Saved/Playtests/20261118-070835-boot` PASS on MovementTestMap: fresh build, correct Mordecai experience/loading screen, cooked content, no crash, and 3/3 engine frames. Desktop captures were covered and are advisory only.
- Tier 4: `Saved/Playtests/20261118-070859-us122` PASS (15 PASS, 0 WARN/FAIL/SKIP). Real Left Shift + W drained stamina 100.0 → 93.667, reached Red at 23.997, stopped sprint, held 20.907 throughout the measured 0.75 s delay, then refilled to 95.16 with the authoritative Green tag. All input-driven ACs were accepted.
- Script-only image verification: `Saved/Playtests/20261118-070859-us122/image-verification.json` PASS. No PNG/video was opened with a file-read tool. `engine_loading.png` was non-uniform; `engine_01.png`, `engine_02.png`, and `engine_03.png` were non-uniform with 2,149 Green HUD pixels each; `bridge_01.png` (drained) was non-uniform with 734 Red HUD pixels; `bridge_02.png` (refilled) was non-uniform with 2,079 Green HUD pixels. The three covered desktop captures were non-uniform but advisory.
- Sweep baseline: fresh `origin/main` commit `e89bec569d`, `Saved/Playtests/20261118-051929-sweep`. Final first sweep: `Saved/Playtests/20261118-071005-sweep`; final permitted same-package rerun: `Saved/Playtests/20261118-071955-sweep`.
- Sweep comparison: `Saved/Playtests/core143-comparison-920209dc.json` records persistent `pass_to_fail: []`. The first run's #135 `deep_bank_blocks` transient passed the rerun. `enemies_present` and `cycle_prev_shift_tab` fail on the baseline as well. Sprint movement checks remained PASS; no movement result was waived. US-101 remains PASS with the intentional Green-tier block drain of 9.375 and runtime refill.
- Evidence bundle: agent store `core-143-920209dc/`.
