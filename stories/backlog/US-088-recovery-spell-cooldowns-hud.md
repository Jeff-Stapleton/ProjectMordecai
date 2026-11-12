# US-088: Recovery — Spell Cooldown Tags + Bound HUD Cooldowns (code)

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none
**Paired story:** US-089 (EDITOR) — DA_Spell_* cooldown tags and durations

## Overview
Spells originally had no cooldowns in the packaged game: no `DA_Spell_*` set a `CooldownTag`, so `ApplySpellCooldown` returned early, and the HUD's cooldown widgets were never bound. This story provides the native cooldown tags and four labelled, bound Fireball/Blink/Stone Skin/Restoration HUD widgets; US-089 puts the tags and durations into the spell DataAssets. Features: spells, spell_hud.

## Execution Mode
**HEADLESS**
Native gameplay tags and data-driven A-D cooldown widgets/binding; the DataAsset values are US-089.

## References
- `Specs/agent_playtest_plan_v1.md` — "Spells have no cooldowns" (`MordecaiGA_SpellBase.cpp:445`; `UMordecaiSpellCooldownWidget::BindToCooldownTag` has no callers), N8 (`MordecaiCombatHUDWidget.cpp:81-92`)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184841-us057`: `cooldown_widgets` FAIL known ("2 SpellCooldown widget(s) on screen (expected 4)"), `fireball_cooldown_state` FAIL known ("cooldown_remaining_s stayed 0.00 ... cooldown_tag ''"), `cooldown_hud` FAIL known (no numeric remaining time); `spell_loadout` shows `cooldown_tag=''` for all four spells
- Done stories: US-057 AC-057.1-057.4 (CooldownDuration Fireball 2.0, Blink 4.0, Stone Skin 15.0, Restoration 10.0), AC-057.9 (4 spell cooldown indicators); US-055 AC-055.8, AC-055.9; US-019 (spell framework)
- Existing code: `Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp:81-92` (SpellCooldownA/B only), `Source/LyraGame/Mordecai/UI/MordecaiSpellCooldownWidget.*`, `Source/LyraGame/Mordecai/Magic/MordecaiGA_SpellBase.cpp:445` (`ApplySpellCooldown`), `Source/LyraGame/Mordecai/MordecaiGameplayTags.*`, `Source/LyraGame/Mordecai/MordecaiPlayerController.cpp:174-` (spell granting with SpellData)

## Acceptance Criteria
- [x] AC-088.1: Native tags `Mordecai.Cooldown.Spell.Fireball`, `.Blink`, `.StoneSkin`, `.Restoration` exist (and a parent `Mordecai.Cooldown.Spell`) — verified by `Mordecai.Recovery.Cooldown.NativeSpellCooldownTags`
- [x] AC-088.2: With a spell whose data has a `CooldownTag` and `CooldownDuration`, activating it applies a cooldown that blocks re-activation for the duration and reports the remaining time — verified by `Mordecai.Recovery.Cooldown.SpellBlockedDuringCooldown`
- [x] AC-088.3: The default combat HUD has 4 `UMordecaiSpellCooldownWidget`s (one per loadout slot, labelled with the slot key Q / E / R / 1) — verified by `Mordecai.Recovery.Cooldown.HUDHasFourCooldownWidgets`, PIE HUD verification, and packaged `cooldown_widgets`
- [x] AC-088.4: After the spells are granted, A-D are bound (`BindToCooldownTag`) to Fireball/Blink/Stone Skin/Restoration; a slot whose spell has no tag logs a warning — verified by `Mordecai.Recovery.Cooldown.WidgetsBoundToSlotTags`, PIE magic verification, and packaged `stone_skin_cooldown_hud`
- [x] AC-088.5: A spell with `CooldownDuration > 0` and an empty `CooldownTag` logs `LogAbilitySystem` Warning once at grant time (so missing content shows in `game.log`) — verified by `Mordecai.Recovery.Cooldown.WarnsOnMissingCooldownTag`

## Technical Notes
- The default HUD creates A-D from one slot definition table, with exact labels Q / E / R / 1. The compact prototype layout fits beneath the SP bar; Tin Tally restyling remains US-105.
- Binding: after `GrantCombatAbilities` grants spells (or when the HUD binds to the ASC), resolve each slot's `UMordecaiSpellDataAsset::CooldownTag` and call `BindToCooldownTag`.
- The bridge's `player.spells[i].cooldown_tag` / `cooldown_remaining_s` read the same data; no bridge change expected.
- Script bookkeeping: issue #13 removes N8 from `cooldown_widgets` and adds real-input proof for Stone Skin's R-slot countdown.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Cooldown.NativeSpellCooldownTags` — AC-088.1
- `Mordecai.Recovery.Cooldown.SpellBlockedDuringCooldown` — AC-088.2
- `Mordecai.Recovery.Cooldown.HUDHasFourCooldownWidgets` — AC-088.3
- `Mordecai.Recovery.Cooldown.WidgetsBoundToSlotTags` — AC-088.4
- `Mordecai.Recovery.Cooldown.WarnsOnMissingCooldownTag` — AC-088.5
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_hud.py`: A-D exist in slot order with labels Q / E / R / 1
- `Scripts/verify_pie_magic.py`: A-D are bound to Fireball/Blink/Stone Skin/Restoration after grants
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us057_magic.py` checks `cooldown_widgets`, `fireball_cooldown_state`, `cooldown_hud`, and `stone_skin_cooldown_hud`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience; us057's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-057` on the same package
- **Player steps:** walk with WASD to the Fireball line and tap Q; later tap R for Stone Skin
- **Visible on screen:** four cooldown indicators labelled Q / E / R / 1 under the SP bar; Fireball counts down after Q and Stone Skin counts down in R after its cast
- **Proof:** Tier 4 checks `cooldown_widgets`, `fireball_cooldown_state`, `cooldown_hud`, and `stone_skin_cooldown_hud`
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` — run: `py Scripts/playtest_story.py US-057`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us057 | `cooldown_widgets` | state widgets -> 4 SpellCooldown widgets labelled Q / E / R / 1 in slot order | PASS at `a7a098c` |
| us057 | `fireball_cooldown_state` | tap Q -> Fireball remaining cooldown is positive within 1 s | PASS at `a7a098c` |
| us057 | `cooldown_hud` | the bound Fireball widget renders a numeric countdown | PASS at `a7a098c` |
| us057 | `stone_skin_cooldown_hud` | tap R -> the R widget renders Stone Skin's numeric countdown | PASS at `a7a098c` |

- **Playable alone:** YES with paired US-089 content, which is already delivered

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edit), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-057`: `cooldown_widgets`, `fireball_cooldown_state`, `cooldown_hud`, and `stone_skin_cooldown_hud` PASS; no baseline PASS becomes FAIL; `--lint` exits 0 — never `--allow-stale`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) inspected by the issue #13 scripted-image constraint; Completion Notes state what the script and run state show
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the closing commit; planner log records issues #35 and #13 fixed
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
- Issue #35 delivered native tags and cooldown application for all four spells plus A/B binding. Issue #13 now adds C/D, exact labels Q / E / R / 1, and four-slot binding without changing Jeff's 2/4/15/10 s cooldowns, GAS rejection, or SP behavior.
- Tier 1: 808/808 Mordecai tests PASS at `bc6d0ce`. Python tooling: 625/625 PASS. Fresh-editor PIE PASS. Development Editor/Game and Shipping builds PASS. Mutation proofs independently failed for native tags, cooldown application, A/B binding, delegate unbind, missing-tag warning, and the evidenced Fireball duration asset mutation, then were restored.
- Clean package: `81f8cadd79f930077e8cbce43262f42a598be616`, built 2026-11-10T01:25:54-07:00 in 565.07 s (`Saved/Playtests/package-20261110-011601.log`), clean tree, no source changes during build.
- Tier 3: `Saved/Playtests/20261110-012620-boot`, CombatTestMap PASS. `engine_loading.png` shows the Project Mordecai loading screen. `engine_01.png`–`engine_03.png` show the fixed-camera CombatTestMap grid, player, TestDummy, Mordecai HUD and the two existing cooldown slots. `shot_01.png`–`shot_03.png` are covered by an unrelated fortress window and are not visual evidence.
- Historical issue #35 Tier 4: `Saved/Playtests/20261110-012719-us057`, fresh package. `fireball_cooldown_state` and `cooldown_hud` passed; `cooldown_widgets` still had two widgets at that commit. `stone_skin_indicator` rendered generic `Status 10.0`; it is N16.
- Tier 4 screenshots: `bridge_01.png` shows the player and damaged TestDummy with SP 7/10 and a small cooldown number under the SP bar; `bridge_02.png` shows the post-Blink lane position with SP 5/10; `bridge_03.png` shows SP 1/10 and generic `Status 10.0` (not `StoneSkin`); `bridge_04.png` shows SP 1/10, the two cooldown slots, and the long Stone Skin cooldown still counting.
- Full sweep: `Saved/Playtests/20261110-012826-sweep` at `81f8cad`, compared with `Saved/Playtests/20261109-214811-sweep` at `22e809a`: only `fireball_cooldown_state` and `cooldown_hud` changed FAIL→PASS; zero PASS→FAIL.
- Final docs-head validation uses `Saved/Playtests/issue35-r1-final-boot` and `Saved/Playtests/issue35-r1-final-us057` on the repackaged documentation commit.
- Issue #13 red proof: `Mordecai.Recovery.Cooldown.HUDHasFourCooldownWidgets` failed because A/B had no labels and C/D did not exist (`Saved/TestLogs/tests-20261112-044610.log`); `WidgetsBoundToSlotTags` failed because C/D did not exist (`Saved/TestLogs/tests-20261112-044740.log`). Red commit: `2a3c20273d`.
- Issue #13 verification at package commit `a7a098c5fc16ae82f41d0fdceaa1765bee771f97`: build PASS; Python tooling 656/656 PASS; Tier 1 826/826 PASS (`Saved/TestLogs/tests-20261112-054130.log`); fresh-editor Tier 2 20/20 PASS; content guard and US-057 lint PASS. Clean package log: `Saved/Playtests/package-20261112-054836.log`.
- Tier 3: `Saved/Playtests/20261112-055752-boot`, CombatTestMap PASS on the exact clean package commit (correct experience, no crash, content cooked, Mordecai loading screen, 3/3 engine frames). Pillow found `engine_loading.png` and `engine_01.png`-`engine_03.png` 1280x720 and non-uniform; their top-left HUD crops were non-uniform. Desktop shots were covered by another window and are not visual evidence.
- Tier 4: `Saved/Playtests/20261112-055851-us057`, exact fresh package. `cooldown_widgets` PASS with four widgets and labels `['Q','E','R','1']`; `fireball_cooldown_state` PASS at 1.983 s; `cooldown_hud` PASS with `['2.0','Q']`; real-input `stone_skin_cooldown_hud` PASS with `['14.9','R']`. The overall US-057 script exits 1 only because N16 `stone_skin_indicator` still renders `Status 10.0`; that same check failed on the exact-main baseline and is outside issue #13.
- Tier 4 frames, inspected by Pillow plus report/state data: all four `bridge_NN.png` are 1280x720, non-uniform, with non-uniform HUD crops. `bridge_01.png` records the Fireball hit at SP 7/10 and Q countdown; `bridge_02.png` records the post-Blink lane at SP 5/10; `bridge_03.png` records SP 1/10 and the known generic `Status 10.0`; `bridge_04.png` records the four Q/E/R/1 slots and R/Stone Skin countdown after Restoration is refused at 1 SP.
- Full regression: `Saved/Playtests/20261112-055932-sweep` at `a7a098c`, compared with exact-main `Saved/Playtests/20261112-043446-sweep` at `b84a2e1`: zero PASS→FAIL; `cooldown_widgets` is the only FAIL→PASS; `stone_skin_cooldown_hud` is a newly added PASS. Pillow inspected all 182 captured PNGs; every feature frame was non-uniform. Only 12 baseline desktop shots were uniform black. Bundle `Saved/Playtests/issue13-a7a098c/` contains 392 files, `image-analysis.{json,md}`, comparison data, all captured frames/logs, and a verified SHA-256 manifest with zero mismatches.
