# US-111: Tin Tally World Markers (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P2
**Depends on:** US-103, US-110, TODO(DECISION) world-marker ownership and targeting
**Paired story:** none (split HEADLESS/EDITOR when unblocked if native logic and assets cannot stay coherent)

## Overview
Implement floor telegraphs/hazards, aim chevron, ground-target reticle, and off-screen threat pips. This is a blocked stub because ownership, attack coverage, aim/facing, targeting validity, and pip architecture are unresolved.

## Execution Mode
**EDITOR**
The eventual surface requires `M_UI_GroundDecal` and related saved material assets.

## References
- `Design/TinTally/ds/project/components/WorldMarkers/README.md`
- `Design/TinTally/ds/project/03-building-in-unreal.md` — proposed `M_UI_GroundDecal`
- Attack profile/hit-shape code and `stories/backlog/US-098-recovery-facing-and-aim.md`
- `Specs/control_bindings_v1.1.md` — aim/target/cast/cancel input
- Issue #34

## Acceptance Criteria
- [ ] AC-111.1: Jeff decides UI versus VFX ownership, attacks with markers, aim/facing behavior, invalid-target rules, shockwave data, and threat-pip implementation before work starts.
- [ ] AC-111.2: each telegraph uses the same shape/values as its hit query; charge reaches the edge at windup end and interruption removes it immediately.
- [ ] AC-111.3: dual edges remain screen-constant/readable on every representative floor and High Contrast swaps marker colors from the theme.
- [ ] AC-111.4: jump band appears only on `JumpAvoidable` sweeps; hazards and invalid reticles use their distinct non-color signals; no floor marker carries text.
- [ ] AC-111.5: aim/reticle respond to approved real input, and threat pips avoid all active HUD boxes and disappear on-screen/death/leash.
- [ ] AC-111.6: all assets are saved/committed/cooked and packaged comparison passes.

## Technical Notes
- Never infer a telegraph separately from the attack query.
- Pool decals/pips; do not spawn every frame.
- BLOCKED: do not ship a fixed +X chevron as if aiming works.
- After decisions, reassess PLANNER split requirements and create a paired story if asset and native halves are independently deliverable.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- Shape parity, phase lifecycle, pip lane/avoidance, targeting-validity tests.
### Tier 2 — PIE smoke (editor integration)
- Material parameters and attack/reticle integration.
### Tier 3 — Packaged playtest
- Every marker state on light/mid/dark floors in both themes.
### Tier 4 — Real-input playtest
- Aim, target, cast/cancel, enemy windup/interrupt/off-screen paths.

## Playtest
- **Launch path:** CombatTestMap plus a deterministic ground-target fixture
- **Player steps:** aim, enter/cancel/commit targeting, bait/interupt attacks, move enemies off-screen
- **Visible on screen:** truthful shapes, readable dual edges, correct pip/reticle states and no text on floor
- **Proof:** new Tier 4 script and both-theme gallery/full-frame comparison
- **Tier 4 script:** BLOCKED pending design
- **Steps:** BLOCKED pending design
- **Playable alone:** YES after blockers

## Definition of Done
- [ ] Blockers resolved/logged and paired split created if required; all ACs checked; red tests fail first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content: every asset is saved/committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] Required combat/targeting Tier 3 routes exit 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-111` and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] Every floor/theme preview/diff and `engine_NN.png`/`bridge_NN.png` is opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-111]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on the decisions named above.
