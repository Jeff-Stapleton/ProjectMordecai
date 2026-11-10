# US-114: Tin Tally Queue, Rail, Receipt, and Spike (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P1
**Depends on:** US-108, US-113, TODO(DECISION) Tally queue and copy
**Paired story:** none

## Overview
Implement ReceiptTimeline as a local-player queue that feeds TallyRail and the pause spike. This is blocked until Jeff approves the subsystem, rank-event source, settings/save behavior, plate-hold option, and remark tone.

## Execution Mode
**HEADLESS**
The proposed subsystem, state machine, and native widgets can use US-103 assets without new editor content.

## References
- `Design/TinTally/ds/project/components/{ReceiptTimeline,TallyRail,PauseShell}/README.md`
- `Design/TinTally/ds/project/02-hud-and-pacing.md` — pacing
- `Design/TinTally/ds/project/04-open-decisions.md` — Tally queue, ranks, remarks
- `Specs/control_bindings_v1.1.md` — pickup/pause/menu input

## Acceptance Criteria
- [ ] AC-114.1: Jeff approves `UMordecaiTallySubsystem` and all blockers named above before implementation.
- [ ] AC-114.2: local-player queue state exactly follows Idle/Hold/Merge/Calm/Receipt/Capped/Overflow/Paused, including 4 s combat, 2.5 s calm, 350 ms spacing and 20 s oldest-entry cap.
- [ ] AC-114.3: `TallyRailHead` hosts exact `"Kills: N"` plus `PendingBadge`/`PendingText`; up to four normal pieces hang, while overflow renders three plus the `"N more on the spike"` stub.
- [ ] AC-114.4: pressed pickups print once immediately; merge rules and no-remark-in-combat rules match ReceiptTimeline; queue never changes gameplay.
- [ ] AC-114.5: pause files waiting tickets to a last-12/newest-first spike, max eight visible, and clears their badge; waiting plates remain queued and print only after the menu closes and calm resumes.
- [ ] AC-114.6: Full/Calm/Minimal and duration settings behave as approved and all rail/spike states pass both-theme packaged comparison.

## Technical Notes
- Proposed entries include kind, merge key, count/rank/rarity/text, earned time, unread and target tab.
- Preserve/add fallbacks for `TallyRailHead`, `PendingBadge`, `PendingText`, `TicketSlot`, `TallyTicketStack`, `TicketString`, `StubImage`, `KindGlyph`, `KickerText`, `LineText`, `LineScaleBox`, `NumberText`, `RaritySocket`, `FootText`, `RemarkSeal`, and `PeelImage`.
- Keep queue deterministic/testable with injected time.
- Allocate-only default: rank allocations file to spike; do not invent use-based leveling.
- BLOCKED: leave remarks empty/off until tone approval.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.UI.TallyQueue.*` for every state, timer, merge and settings rule
- `Mordecai.UI.TallyRail.*` and `Mordecai.UI.PauseSpike.*`
### Tier 2 — PIE smoke (editor integration)
- HUD children, event binding and pause handoff.
### Tier 3 — Packaged playtest
- Deterministic fixture covering hold, pickup, receipt, cap, overflow and pause in both themes.
### Tier 4 — Real-input playtest
- Fight/pickup/pause/focus spike/jump-to-tab path with no direct calls.

## Playtest
- **Launch path:** DevTestMap plus deterministic combat fixture
- **Player steps:** fight, earn events, pick up, wait calm, sustain cap, overflow, pause and activate spike entry
- **Visible on screen:** pending counts, exact ticket order/times, overflow, spike history, no duplicate or combat remark
- **Proof:** new Tier 4 script, queue trace and both-theme comparisons
- **Tier 4 script:** BLOCKED pending approvals
- **Steps:** BLOCKED pending approvals
- **Playable alone:** YES after blockers

## Definition of Done
- [ ] Decisions logged; all ACs checked; red tests fail for intended reasons first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content guard: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then `py Scripts/package_playtest.py` exits 0 for that exact clean commit
- [ ] Required DevTestMap/CombatTestMap Tier 3 routes exit 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-114` and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] Both-theme rail/receipt/spike previews/diffs, timing traces and every gameplay screenshot are opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-114]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on queue, settings, rank, and remark decisions.
