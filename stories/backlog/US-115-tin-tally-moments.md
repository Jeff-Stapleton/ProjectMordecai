# US-115: Tin Tally Plates, Banners, and Boss Bar (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P2
**Depends on:** US-114, TODO(DECISION) progression and world/boss data
**Paired story:** none

## Overview
Add the top-centre big-moment surfaces: TallyPlate, Banner, and BossBar. This is blocked because level/XP, feat text fields, biome/area/boss data, boss phases, and remark copy are not implemented or approved.

## Execution Mode
**HEADLESS**
After data decisions, native HUD children can consume real gameplay events and US-103 assets.

## References
- `Design/TinTally/ds/project/components/{TallyPlate,Banner,BossBar}/README.md`
- `Design/TinTally/ds/project/components/HUDLayout/README.md`
- `Design/TinTally/ds/project/04-open-decisions.md`
- `Specs/control_bindings_v1.1.md` — gameplay input used to trigger accepted states

## Acceptance Criteria
- [ ] AC-115.1: progression/world/boss fields and event sources are approved and implemented before UI work.
- [ ] AC-115.2: one TallyPlate shows at a time, never during combat, for the documented durations; Drawback/tier/type states use real data only.
- [ ] AC-115.3: plate is `(0,40)` normally and `(0,168)` under an `880x96` BossBar; Banner uses `(0,296)`, always wins its lane, and plates wait until it clears.
- [ ] AC-115.4: Banner independently presents biome, boss intro and phase states in World voice with no Tally remarks.
- [ ] AC-115.5: BossBar presents real health trail/posture/phase data and exits on the boss lifecycle; no fake boss is shipped.
- [ ] AC-115.6: every approved state passes both-theme packaged comparison and queue/combat regressions.

## Technical Notes
- Proposed classes are `UMordecaiTallyPlateWidget`, `UMordecaiBannerWidget`, and `UMordecaiBossBarWidget`; optional HUD names remain `TallyPlate`, `Banner`, and `BossBar`.
- Level/XP placeholder is hidden in shipping until real progression exists.
- Remarks remain omitted until tone approval.
- BLOCKED: do not manufacture biome/boss events from map names or proximity.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- Plate hold/order/duration, banner preemption, boss binding/lifecycle/map-box tests.
### Tier 2 — PIE smoke (editor integration)
- Real event-source binding and HUD ownership.
### Tier 3 — Packaged playtest
- Deterministic real-data fixtures for approved plate/banner/boss states.
### Tier 4 — Real-input playtest
- Earn plate, enter biome/boss, trigger phase, leave/kill boss.

## Playtest
- **Launch path:** map containing approved progression and boss fixtures
- **Player steps:** trigger each real event through gameplay
- **Visible on screen:** non-overlapping top-centre states with correct voice/data/timing
- **Proof:** Tier 4 events/state plus both-theme comparisons
- **Tier 4 script:** BLOCKED pending gameplay/data
- **Steps:** BLOCKED pending gameplay/data
- **Playable alone:** YES after blockers

## Definition of Done
- [ ] Blockers resolved/logged; all ACs checked; red tests fail for intended reasons first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] Approved progression/boss map Tier 3 route exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-115` and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] No placeholder gameplay data ships; every state/theme preview/diff and gameplay screenshot is opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-115]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on progression, world/boss data, and copy decisions.
