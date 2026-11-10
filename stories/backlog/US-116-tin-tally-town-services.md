# US-116: Tin Tally Dialogue and Town Services (BLOCKED)

**Epic:** Epic Tin Tally: In-engine UI
**Priority:** P2
**Depends on:** US-109, US-113, US-114, TODO(DECISION) town/Identify/vendor rules
**Paired story:** none (split when source town stories exist if assets and native logic require separate delivery)

## Overview
Implement town Dialogue, MageTower, and reusable ServiceScreen surfaces. This remains a stub because US-072/074 are not on disk and dialogue, Identify, vendors/currency, potion restock, town management, and pause behavior are unresolved.

## Execution Mode
**EDITOR**
The eventual player path requires saved town/NPC/service content in addition to native activatable widgets.

## References
- `Design/TinTally/ds/project/components/{Dialogue,MageTower,ServiceScreen}/README.md`
- `Design/TinTally/ds/project/03-building-in-unreal.md` — proposed class names/layers
- `Design/TinTally/ds/project/04-open-decisions.md`
- `Specs/village_system_v1.md`
- `Specs/control_bindings_v1.1.md` — Interact/menu/service input

## Acceptance Criteria
- [ ] AC-116.1: source town stories/data and Jeff's dialogue, Identify, vendor, potion, pause, and management decisions exist before implementation.
- [ ] AC-116.2: `UMordecaiDialogueWidget` is pushed on `UI.Layer.GameMenu`; pause can open above it and back/focus/input lifecycle is correct.
- [ ] AC-116.3: `UMordecaiServiceScreenWidget` and `UMordecaiMageTowerWidget` use `UI.Layer.Menu`, bind/unbind on activation, and consume data rather than hard-coded shop/service copy.
- [ ] AC-116.4: approved Identify/restock actions change real inventory/currency exactly once and feed factual Tally events without fabricated cost or remark.
- [ ] AC-116.5: Blacksmith/Clinic/management remain absent unless their source contracts land; unavailable services are truthful.
- [ ] AC-116.6: the complete real-input NPC-to-service path and both-theme visual comparisons pass in a packaged town build.

## Technical Notes
- Proposed class/layer names are fixed by `03`; they do not currently exist.
- No HUD wallet or price until currency/vendor systems exist.
- Preserve current field Identify until the approved migration story removes it.
- Reassess HEADLESS/EDITOR paired split after blockers resolve.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- Layer, activation lifecycle, data projection and transaction tests.
### Tier 2 — PIE smoke (editor integration)
- Saved NPC/dialogue/service route and asset references.
### Tier 3 — Packaged playtest
- Cooked town map/NPC/service route in both themes.
### Tier 4 — Real-input playtest
- Interact, choose dialogue, identify/restock, back/pause/reopen, verify inventory.

## Playtest
- **Launch path:** approved packaged town map/experience
- **Player steps:** approach NPC, interact, choose service, perform approved action, close/reopen/pause
- **Visible on screen:** Dialogue and service layers with truthful values and updated inventory/Tally result
- **Proof:** new Tier 4 script and both-theme captures
- **Tier 4 script:** BLOCKED pending source content
- **Steps:** BLOCKED pending source content
- **Playable alone:** YES after blockers

## Definition of Done
- [ ] Source stories/decisions exist and pair added if required; all ACs checked; red tests fail first
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation and affected Python tests pass
- [ ] Tier 2: `py Scripts/run_pie_verify.py` passes; direct-call checks are labeled LOGIC
- [ ] Content: every asset is saved/committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST, then the exact clean commit packages with the approved town map
- [ ] Town Tier 3 boot exits 0 — never `--allow-stale`
- [ ] `py Scripts/playtest_story.py US-116` and `--lint` pass on that package; full sweep has no baseline PASS to FAIL
- [ ] Both-theme previews/diffs and every `engine_NN.png`/`bridge_NN.png` are opened/described
- [ ] Evidence run dirs appear in Completion Notes and closing `[US-116]` commit body; closing commit touches only `stories/`
- [ ] Pushed; story moves to `stories/done/` only after all checks pass

## Completion Notes
BLOCKED on missing source stories and town/service decisions.
