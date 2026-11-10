# US-088: Recovery — Spell Cooldown Tags + Bound HUD Cooldowns (code)

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none
**Paired story:** US-089 (EDITOR) — DA_Spell_* cooldown tags and durations

## Overview
Spells have no cooldowns in the packaged game: no `DA_Spell_*` sets a `CooldownTag`, so `ApplySpellCooldown` returns early, and the HUD's cooldown widgets are never bound (`BindToCooldownTag` has no callers), so they always read "ready". This story provides the native cooldown tags and binds the existing Fireball/Blink HUD widgets; US-089 puts the tags and durations into the spell DataAssets. The separate four-widget HUD expansion remains issue #13 / AC-088.3. Features: spells, spell_hud.

## Execution Mode
**HEADLESS**
Native gameplay tags and widget binding for the existing A/B cooldown widgets; the DataAsset values are US-089. Adding C/D widgets or labels is outside issue #35 and remains issue #13.

## References
- `Specs/agent_playtest_plan_v1.md` — "Spells have no cooldowns" (`MordecaiGA_SpellBase.cpp:445`; `UMordecaiSpellCooldownWidget::BindToCooldownTag` has no callers), N8 (`MordecaiCombatHUDWidget.cpp:81-92`)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184841-us057`: `cooldown_widgets` FAIL known ("2 SpellCooldown widget(s) on screen (expected 4)"), `fireball_cooldown_state` FAIL known ("cooldown_remaining_s stayed 0.00 ... cooldown_tag ''"), `cooldown_hud` FAIL known (no numeric remaining time); `spell_loadout` shows `cooldown_tag=''` for all four spells
- Done stories: US-057 AC-057.1-057.4 (CooldownDuration Fireball 2.0, Blink 4.0, Stone Skin 15.0, Restoration 10.0), AC-057.9 (4 spell cooldown indicators); US-055 AC-055.8, AC-055.9; US-019 (spell framework)
- Existing code: `Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp:81-92` (SpellCooldownA/B only), `Source/LyraGame/Mordecai/UI/MordecaiSpellCooldownWidget.*`, `Source/LyraGame/Mordecai/Magic/MordecaiGA_SpellBase.cpp:445` (`ApplySpellCooldown`), `Source/LyraGame/Mordecai/MordecaiGameplayTags.*`, `Source/LyraGame/Mordecai/MordecaiPlayerController.cpp:174-` (spell granting with SpellData)

## Acceptance Criteria
- [x] AC-088.1: Native tags `Mordecai.Cooldown.Spell.Fireball`, `.Blink`, `.StoneSkin`, `.Restoration` exist (and a parent `Mordecai.Cooldown.Spell`) — verified by `Mordecai.Recovery.Cooldown.NativeSpellCooldownTags`
- [x] AC-088.2: With a spell whose data has a `CooldownTag` and `CooldownDuration`, activating it applies a cooldown that blocks re-activation for the duration and reports the remaining time — verified by `Mordecai.Recovery.Cooldown.SpellBlockedDuringCooldown`
- [ ] AC-088.3: The default combat HUD has 4 `UMordecaiSpellCooldownWidget`s (one per loadout slot, labelled with the slot key Q / E / R / 1) — remains issue #13; packaged check `cooldown_widgets` truthfully FAILs with 2 widgets
- [x] AC-088.4: After the spells are granted, the existing A/B cooldown widgets are bound (`BindToCooldownTag`) to Fireball/Blink; a slot whose spell has no tag logs a warning. C/D binding remains issue #13 with AC-088.3 — verified by `Mordecai.Recovery.Cooldown.WidgetsBoundToSlotTags`
- [x] AC-088.5: A spell with an empty `CooldownTag` logs `LogAbilitySystem` Warning once at grant time (so missing content shows in `game.log`) — verified by `Mordecai.Recovery.Cooldown.WarnsOnMissingCooldownTag`

## Technical Notes
- Do not add `SpellCooldownC` / `SpellCooldownD` or their labels in issue #35; that work remains issue #13.
- Binding: after `GrantCombatAbilities` grants spells (or when the HUD binds to the ASC), resolve each slot's `UMordecaiSpellDataAsset::CooldownTag` and call `BindToCooldownTag`.
- The bridge's `player.spells[i].cooldown_tag` / `cooldown_remaining_s` read the same data; no bridge change expected.
- Script bookkeeping: keep N8 / issue #13 on `cooldown_widgets`; issue #35 removes the stale known-issue classification from `fireball_cooldown_state` and `cooldown_hud`.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Cooldown.NativeSpellCooldownTags` — AC-088.1
- `Mordecai.Recovery.Cooldown.SpellBlockedDuringCooldown` — AC-088.2
- `Mordecai.Recovery.Cooldown.HUDHasFourCooldownWidgets` — AC-088.3 remains open under issue #13
- `Mordecai.Recovery.Cooldown.WidgetsBoundToSlotTags` — AC-088.4
- `Mordecai.Recovery.Cooldown.WarnsOnMissingCooldownTag` — AC-088.5
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_magic.py`: A/B are bound to Fireball/Blink after grants
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us057_magic.py` checks `fireball_cooldown_state` and `cooldown_hud`; `cooldown_widgets` remains a known issue #13 FAIL

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience; us057's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-057` on the same package
- **Player steps:** walk with WASD to the Fireball line and tap Q
- **Visible on screen:** the existing Fireball/Blink cooldown indicators under the SP bar; Fireball counts down after Q
- **Proof:** Tier 4 checks `fireball_cooldown_state` and `cooldown_hud`; `cooldown_widgets` remains the expected issue #13 FAIL
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` — run: `py Scripts/playtest_story.py US-057`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us057 | `cooldown_widgets` | state widgets -> 4 SpellCooldown widgets on the HUD (one per spell) | FAIL (known issue #13): 2 |
| us057 | `fireball_cooldown_state` | tap Q -> Fireball remaining cooldown is positive within 1 s | PASS at `81f8cad` |
| us057 | `cooldown_hud` | the bound Fireball widget renders a numeric countdown | PASS at `81f8cad` |

- **Playable alone:** NO — paired content is US-089; together they fix issue #35 for A/B while issue #13 remains

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edit), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-057`: `fireball_cooldown_state` and `cooldown_hud` PASS; `cooldown_widgets` remains the known issue #13 FAIL; no baseline PASS becomes FAIL; `--lint` exits 0 — never `--allow-stale`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the closing commit; planner log records issue #35 fixed while issue #13 remains
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
- Issue #35 scope follows Jeff's decision: native tags and cooldown application for all four spells, with the existing A/B HUD widgets bound to Fireball/Blink. No C/D widgets or labels were added; AC-088.3 stays open under issue #13.
- Tier 1: 805/805 Mordecai tests PASS. Python tooling: 625/625 PASS. Fresh-editor PIE PASS. Development Editor/Game and Shipping builds PASS. Mutation proofs independently failed for native tags, cooldown application, A/B binding, delegate unbind, missing-tag warning, and serialized asset values, then were restored.
- Clean package: `81f8cadd79f930077e8cbce43262f42a598be616`, built 2026-11-10T01:25:54-07:00 in 565.07 s (`Saved/Playtests/package-20261110-011601.log`), clean tree, no source changes during build.
- Tier 3: `Saved/Playtests/20261110-012620-boot`, CombatTestMap PASS. `engine_loading.png` shows the Project Mordecai loading screen. `engine_01.png`–`engine_03.png` show the fixed-camera CombatTestMap grid, player, TestDummy, Mordecai HUD and the two existing cooldown slots. `shot_01.png`–`shot_03.png` are covered by an unrelated fortress window and are not visual evidence.
- Tier 4: `Saved/Playtests/20261110-012719-us057`, fresh package. `fireball_cooldown_state` PASS (2.0 s at 0.31 s after Q) and `cooldown_hud` PASS (bound widget text `2.0`). `cooldown_widgets` FAIL is expected issue #13 (2 rather than 4). `stone_skin_indicator` FAIL renders generic `Status 10.0`; it is N16 and predates this branch in baseline `Saved/Playtests/20261109-214811-sweep` at `22e809a`.
- Tier 4 screenshots: `bridge_01.png` shows the player and damaged TestDummy with SP 7/10 and a small cooldown number under the SP bar; `bridge_02.png` shows the post-Blink lane position with SP 5/10; `bridge_03.png` shows SP 1/10 and generic `Status 10.0` (not `StoneSkin`); `bridge_04.png` shows SP 1/10, the two cooldown slots, and the long Stone Skin cooldown still counting.
- Full sweep: `Saved/Playtests/20261110-012826-sweep` at `81f8cad`, compared with `Saved/Playtests/20261109-214811-sweep` at `22e809a`: only `fireball_cooldown_state` and `cooldown_hud` changed FAIL→PASS; zero PASS→FAIL.
- Final docs-head validation uses `Saved/Playtests/issue35-r1-final-boot` and `Saved/Playtests/issue35-r1-final-us057` on the repackaged documentation commit.
