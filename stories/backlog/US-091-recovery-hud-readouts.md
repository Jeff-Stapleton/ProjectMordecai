# US-091: Recovery — HUD Readouts: Status Effect Names, Weapon Name, No Debug Text

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** none
**Paired story:** none

## Overview
Three HUD readouts are wrong in the packaged game. Every status-effect indicator reads "Status" instead of the status name (N16: Stone Skin shows "Status 10.0"). The weapon HUD draws nothing (N3), so the player never sees which of the five weapons is equipped. And developer debug text is printed on screen over the HUD ("MORDECAI: Programmatic IMC created and input bound successfully ...", "MORDECAI: Weapon loadout initialized — 5 weapons available", "MORDECAI: SetupPlayerInputComponent called…", plus the Steam socket deprecation warnings). Features: status_effects, combat_hud, weapon_hud.

## Execution Mode
**HEADLESS**
Native widget code, log levels and an ini change (`Config/DefaultEngine.ini`); no editor assets.

## References
- `Specs/agent_playtest_plan_v1.md` — N16, N3, "On-screen debug text is shown to players", "More on-screen debug text", "Steam socket config deprecation warnings print on screen"
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`:
  - run `Saved/Playtests/20261022-184841-us057`: `stone_skin_indicator` FAIL NEW ("no widget renders 'StoneSkin'; ... MordecaiStatusEffectIndicatorWidget ... texts=['Status', '10.0']")
  - run `Saved/Playtests/20261022-184940-us078`: `weapon_hud_text` FAIL known ("no visible widget text equals the weapon name 'Longsword'"); `weapon_hud_updates` SKIP (needs `weapon_hud_text`)
- Done stories: US-055 AC-055.4-055.6 (indicator shows the status name + countdown, tint), US-057 AC-057.11 ("Burning"), US-077 AC-077.7/077.8, US-078 AC-078.11/078.12
- Existing code:
  - `Source/LyraGame/Mordecai/UI/MordecaiStatusEffectBarWidget.cpp:89-92,110-126` — registers `RegisterGameplayTagEvent(Mordecai.Status, NewOrRemoved)`; that delegate reports the registered parent tag, so `AddIndicator` gets `Mordecai.Status` and `MordecaiStatusEffectIndicatorWidget.cpp:73-83` takes its last segment: "Status". It also means only one indicator can ever exist and buff/debuff tint is wrong
  - `Source/LyraGame/Mordecai/UI/MordecaiEquippedWeaponWidget.*` (no widget tree), created at `Source/LyraGame/Mordecai/MordecaiCharacter.cpp:63,184-188`
  - Debug output: `Source/LyraGame/Mordecai/MordecaiHeroComponent.cpp:261`, `Source/LyraGame/Mordecai/MordecaiCharacter.cpp:195,238` (`UE_LOG(LogTemp, Warning, ...)`), `Config/DefaultEngine.ini` `SocketSubsystemSteamIP` keys

## Acceptance Criteria
- [ ] AC-091.1: Gaining `Mordecai.Status.StoneSkin` creates an indicator reading "StoneSkin" (or its configured display name) with a green buff tint; two different statuses produce two indicators; each is removed when its own tag is removed — verified by `Mordecai.Recovery.StatusBar.IndicatorUsesChildTagName` and `Mordecai.Recovery.StatusBar.OneIndicatorPerStatus`
- [ ] AC-091.2: In the packaged build, the player taps R (Stone Skin) and a status indicator renders "StoneSkin" with a numeric countdown — verified by check `stone_skin_indicator` of `Scripts/playtests/us057_magic.py` (Tier 4); `stone_skin_expires` still PASSes
- [ ] AC-091.3: `UMordecaiEquippedWeaponWidget` builds a default widget tree (weapon display name text) and updates on `OnWeaponChanged` — verified by `Mordecai.Recovery.WeaponHUD.BuildsDefaultContent` and `Mordecai.Recovery.WeaponHUD.UpdatesOnWeaponChanged`
- [ ] AC-091.4: In the packaged build, the HUD shows "Longsword" at start — verified by check `weapon_hud_text` of `Scripts/playtests/us078_weapons.py` (Tier 4, state read); after the player taps Tab the HUD shows the new weapon's name — verified by check `weapon_hud_updates` of `Scripts/playtests/us078_weapons.py` (Tier 4)
- [ ] AC-091.5: No Mordecai developer message and no Steam socket deprecation warning is drawn on screen in the packaged Development build; the messages remain in `game.log` at `Log`/`Verbose` level under a `LogMordecai*` category — verified by Tier 3: every `engine_NN.png` of `py Scripts/playtest_boot.py` shows no yellow/red debug text over the arena (the agent reads each screenshot and states it), and `game.log` still contains the messages

## Technical Notes
- Status bar: register per known status tag, or use `ASC->RegisterGenericGameplayTagEvent()` / `OnGameplayEffectAppliedDelegateToSelf` and filter `Tag.MatchesTag(Mordecai.Status)` excluding `Mordecai.Status.Category.*`; key indicators by the child tag.
- Weapon HUD: follow the `BuildDefaultContent` pattern of `UMordecaiStatusEffectIndicatorWidget` (build only when `WidgetTree->RootWidget` is null); ensure it is added to the viewport/HUD in a visible slot (AC-078.11: bottom-center or bottom-right) and does not overlap the pickup prompt.
- Debug text: find out how these `LogTemp` Warnings reach the screen in a packaged Development build (e.g. an on-screen log/warning setting in config, or Lyra's on-screen message hook) and fix it at the source; demote the three messages to `LogMordecai*` `Log`. Fix or remove the deprecated `SocketSubsystemSteamIP` keys in `Config/DefaultEngine.ini`. Do not disable on-screen messages globally if that hides real engine errors — say what you changed.
- Script bookkeeping (before packaging): remove the N3 `known_issue` from `weapon_hud_text` and `weapon_hud_updates` in `us078_weapons.py`; `stone_skin_indicator` was NEW.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.StatusBar.IndicatorUsesChildTagName`, `Mordecai.Recovery.StatusBar.OneIndicatorPerStatus` — AC-091.1
- `Mordecai.Recovery.WeaponHUD.BuildsDefaultContent`, `Mordecai.Recovery.WeaponHUD.UpdatesOnWeaponChanged` — AC-091.3
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_hud.py`: the weapon widget is visible with text "Longsword"; `[LOGIC]` apply StoneSkin GE → indicator text "StoneSkin"
### Tier 3 — Packaged playtest
- See Playtest (AC-091.5 is a Tier 3 visual check)
### Tier 4 — Real-input playtest
- `Scripts/playtests/us057_magic.py` `stone_skin_indicator`; `Scripts/playtests/us078_weapons.py` `weapon_hud_text`, `weapon_hud_updates`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py` (DevTestMap → B_MordecaiArenaExperience); Tier 4: `py Scripts/playtest_story.py US-057` and `py Scripts/playtest_story.py US-078` on the same package
- **Player steps:** tap R (Stone Skin); tap Tab (next weapon)
- **Visible on screen:** Tier 3 `engine_NN.png`: the HUD with the weapon name "Longsword" and NO yellow "MORDECAI: ..." lines or Steam warnings. Tier 4: us057 `stone_skin` screenshot shows "StoneSkin" + countdown bottom-left; us078 `bridge_01.png` shows "Longsword", `bridge_02.png` "Mace"
- **Proof:** Tier 3 screenshots (AC-091.5); Tier 4 checks `stone_skin_indicator` (us057), `weapon_hud_text`, `weapon_hud_updates` (us078)
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` (`py Scripts/playtest_story.py US-057`), `Scripts/playtests/us078_weapons.py` (`py Scripts/playtest_story.py US-078`)
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us057 | `stone_skin_indicator` | state widgets -> a widget renders "StoneSkin" and a numeric remaining duration within 2s | FAIL (NEW): 'Status' |
| us078 | `weapon_hud_text` | state player, widgets -> a visible Mordecai widget text equals player.weapon.display_name | FAIL (known N3) |
| us078 | `weapon_hud_updates` | tap Tab -> a visible widget text equals the new display_name within 3 s | SKIP |

- **Playable alone:** YES

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: none; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code, config, script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py` exits 0 — never `--allow-stale`; every `engine_NN.png` free of debug text
- [ ] On that same fresh package: `stone_skin_indicator`, `weapon_hud_text`, `weapon_hud_updates` go from FAIL/SKIP to PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 for US-057 and US-078 — never `--allow-stale`
- [ ] Known issues N16, N3, the debug-text entries and the Steam warning marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-091]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
