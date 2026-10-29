# US-088: Recovery — Spell Cooldown Tags + Four Bound Cooldown Widgets on the HUD (code)

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none
**Paired story:** US-089 (EDITOR) — DA_Spell_* cooldown tags and durations

## Overview
Spells have no cooldowns in the packaged game: no `DA_Spell_*` sets a `CooldownTag`, so `ApplySpellCooldown` returns early, and the HUD's cooldown widgets are never bound (`BindToCooldownTag` has no callers), so they always read "ready". The HUD also shows only 2 cooldown widgets for a 4-spell loadout (N8). This story provides the native cooldown tags, four HUD cooldown widgets (one per spell slot) and their binding; US-089 puts the tags and durations into the spell DataAssets. Features: spells, spell_hud.

## Execution Mode
**HEADLESS**
Native gameplay tags, the C++ default HUD layout (`WBP_CombatHUD` has no widget tree, so `UMordecaiCombatHUDWidget::BuildDefaultLayout` is what the player sees) and widget binding; the DataAsset values are US-089.

## References
- `Specs/agent_playtest_plan_v1.md` — "Spells have no cooldowns" (`MordecaiGA_SpellBase.cpp:445`; `UMordecaiSpellCooldownWidget::BindToCooldownTag` has no callers), N8 (`MordecaiCombatHUDWidget.cpp:81-92`)
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184841-us057`: `cooldown_widgets` FAIL known ("2 SpellCooldown widget(s) on screen (expected 4)"), `fireball_cooldown_state` FAIL known ("cooldown_remaining_s stayed 0.00 ... cooldown_tag ''"), `cooldown_hud` FAIL known (no numeric remaining time); `spell_loadout` shows `cooldown_tag=''` for all four spells
- Done stories: US-057 AC-057.1-057.4 (CooldownDuration Fireball 2.0, Blink 4.0, Stone Skin 15.0, Restoration 10.0), AC-057.9 (4 spell cooldown indicators); US-055 AC-055.8, AC-055.9; US-019 (spell framework)
- Existing code: `Source/LyraGame/Mordecai/UI/MordecaiCombatHUDWidget.cpp:81-92` (SpellCooldownA/B only), `Source/LyraGame/Mordecai/UI/MordecaiSpellCooldownWidget.*`, `Source/LyraGame/Mordecai/Magic/MordecaiGA_SpellBase.cpp:445` (`ApplySpellCooldown`), `Source/LyraGame/Mordecai/MordecaiGameplayTags.*`, `Source/LyraGame/Mordecai/MordecaiPlayerController.cpp:174-` (spell granting with SpellData)

## Acceptance Criteria
- [ ] AC-088.1: Native tags `Mordecai.Cooldown.Spell.Fireball`, `.Blink`, `.StoneSkin`, `.Restoration` exist (and a parent `Mordecai.Cooldown.Spell`) — verified by `Mordecai.Recovery.Cooldown.NativeSpellCooldownTags`
- [ ] AC-088.2: With a spell whose data has a `CooldownTag` and `CooldownDuration`, activating it applies a cooldown that blocks re-activation for the duration and reports the remaining time — verified by `Mordecai.Recovery.Cooldown.SpellBlockedDuringCooldown`
- [ ] AC-088.3: The default combat HUD has 4 `UMordecaiSpellCooldownWidget`s (one per loadout slot, labelled with the slot key Q / E / R / 1) — verified by `Mordecai.Recovery.Cooldown.HUDHasFourCooldownWidgets`; in the packaged build, check `cooldown_widgets` of `Scripts/playtests/us057_magic.py` (Tier 4, no input: state read) PASSes
- [ ] AC-088.4: After the spells are granted, each cooldown widget is bound (`BindToCooldownTag`) to its slot's spell `CooldownTag`; a slot whose spell has no tag logs `LogMordecai` Warning — verified by `Mordecai.Recovery.Cooldown.WidgetsBoundToSlotTags`
- [ ] AC-088.5: A spell with an empty `CooldownTag` logs `LogMordecai` Warning once at grant time (so missing content shows in `game.log`) — verified by `Mordecai.Recovery.Cooldown.WarnsOnMissingCooldownTag`

## Technical Notes
- Add `SpellCooldownC` / `SpellCooldownD` to `UMordecaiCombatHUDWidget` (UPROPERTY, BindWidgetOptional) and the default layout; keep them in a row under the SP bar.
- Binding: after `GrantCombatAbilities` grants spells (or when the HUD binds to the ASC), resolve each slot's `UMordecaiSpellDataAsset::CooldownTag` and call `BindToCooldownTag`.
- The bridge's `player.spells[i].cooldown_tag` / `cooldown_remaining_s` read the same data; no bridge change expected.
- Script bookkeeping: remove the N8 `known_issue` from `cooldown_widgets` in `us057_magic.py` (commit before packaging). `fireball_cooldown_state` and `cooldown_hud` keep theirs until US-089 lands.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.Cooldown.NativeSpellCooldownTags` — AC-088.1
- `Mordecai.Recovery.Cooldown.SpellBlockedDuringCooldown` — AC-088.2
- `Mordecai.Recovery.Cooldown.HUDHasFourCooldownWidgets` — AC-088.3
- `Mordecai.Recovery.Cooldown.WidgetsBoundToSlotTags` — AC-088.4
- `Mordecai.Recovery.Cooldown.WarnsOnMissingCooldownTag` — AC-088.5
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_hud.py`: 4 SpellCooldown widgets in the PIE HUD
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us057_magic.py` check `cooldown_widgets` (state read, no input)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience; us057's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-057` on the same package
- **Player steps:** none for this half (casting with cooldowns is US-089)
- **Visible on screen:** Tier 3 `engine_NN.png`: four cooldown indicators under the SP bar (top-left)
- **Proof:** Tier 3 screenshots; Tier 4 check `cooldown_widgets` of `Scripts/playtests/us057_magic.py`
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` — run: `py Scripts/playtest_story.py US-057`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us057 | `cooldown_widgets` | state widgets -> 4 SpellCooldown widgets on the HUD (one per spell) | FAIL (known N8): 2 |

- **Playable alone:** NO — cooldowns are playable after US-089 (EDITOR) lands

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edit), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-057`: `cooldown_widgets` goes from FAIL to PASS; no check that PASSed in the sweep now FAILs; `fireball_cooldown_state`/`cooldown_hud` still FAIL (known, pending US-089); `--lint` exits 0 — never `--allow-stale`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-088]` closing commit; planner log tracks cooldowns as `PENDING PAIR (waiting on US-089)`
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
