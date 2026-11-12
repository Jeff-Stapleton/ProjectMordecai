# US-089: Recovery — Spell DataAssets Carry Cooldown Tags and Durations (content)

**Epic:** Epic R: Playability Recovery
**Priority:** P1
**Depends on:** US-088
**Paired story:** US-088 (HEADLESS) — native cooldown tags + four labelled, bound HUD widgets

## Overview
With US-088's tags and bound widgets in place, the four arena spells need their `CooldownTag` and `CooldownDuration` set and saved in their DataAssets. Then a player who casts Fireball sees a 2-second countdown on its HUD slot and cannot recast it until it ends. Features: spells, spell_hud.

## Execution Mode
**EDITOR**
Edits and saves `/MordecaiCore/Spells/DA_Spell_*` DataAssets via the Python editor API.

## References
- `Specs/agent_playtest_plan_v1.md` — "Spells have no cooldowns"
- Sweep `Saved/Playtests/20261022-184702-sweep/summary.md`; run `Saved/Playtests/20261022-184841-us057`: `spell_loadout` reports `cooldown_tag=''` for Fireball, Blink, Stone Skin, Restoration; `fireball_cooldown_state` and `cooldown_hud` FAIL (known)
- `stories/done/US-057-playable-magic-arena.md` — AC-057.1 Fireball CooldownDuration=2.0; AC-057.2 Blink 4.0; AC-057.3 Stone Skin 15.0; AC-057.4 Restoration 10.0
- `Specs/control_bindings_v1.1.md` — Q / E / R / 1 (keyboard spell keys in use)
- Existing assets: `/MordecaiCore/Spells/DA_Spell_Fireball`, `DA_Spell_Blink`, `DA_Spell_StoneSkin`, `DA_Spell_Restoration`

## Acceptance Criteria
- [x] AC-089.1: The four DataAssets have `CooldownTag` = `Mordecai.Cooldown.Spell.<Name>` and `CooldownDuration` = 2.0 / 4.0 / 15.0 / 10.0 (AC-057.1-057.4); verified in PIE by `Scripts/verify_pie_magic.py` and in the packaged build by check `spell_loadout` of `Scripts/playtests/us057_magic.py` reporting the tags (state read)
- [x] AC-089.2: In the packaged build, the player taps Q and `player.spells[0].cooldown_remaining_s > 0` within 1 s — verified by check `fireball_cooldown_state` of `Scripts/playtests/us057_magic.py` (Tier 4)
- [x] AC-089.3: In the packaged build, after the tap a HUD cooldown widget renders a numeric remaining time — verified by check `cooldown_hud` of `Scripts/playtests/us057_magic.py` (Tier 4)
- [x] AC-089.4: No missing-cooldown-tag warning (US-088 AC-088.5) appears in the packaged `game.log`
- [x] AC-089.5: every asset is saved and committed; `py Scripts/check_content_committed.py` passes

## Technical Notes
- Script `Scripts/us089_set_spell_cooldowns.py`: load each DA, `set_editor_property('cooldown_tag', unreal.GameplayTag...)` (use the tag-from-name helper available in the project's Python API; verify it round-trips), `cooldown_duration`, then `EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)`. Print each asset's values after reload.
- Extend `spell_loadout`'s detail (already prints `cooldown_tag`) — optionally assert non-empty tags in that check; if so, update its `does` text and the US-057 Steps table.
- Script bookkeeping (commit before packaging): remove the "Spells have no cooldowns" `known_issue` from `fireball_cooldown_state` and `cooldown_hud` in `us057_magic.py`.
- Gamepad spell buttons are out of scope (US-097).
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- No new logic; ALL `Mordecai.*` tests still pass
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_magic.py`: the four DAs' `CooldownTag`/`CooldownDuration`; `[LOGIC]` activate Fireball → its cooldown widget shows a remaining time
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us057_magic.py` checks `spell_loadout`, `fireball_cooldown_state`, `cooldown_hud`

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience; us057's `MAP` since US-100); Tier 4: `py Scripts/playtest_story.py US-057` on the same package
- **Player steps:** walk (WASD) onto the Fireball line, tap Q, watch the Q cooldown slot count down
- **Visible on screen:** `spell_hud` screenshot (`bridge_04.png` of us057): the cooldown widgets under the SP bar with a numeric remaining time on at least one slot (Stone Skin's 15 s is still running at that point)
- **Proof:** Tier 4 checks `fireball_cooldown_state`, `cooldown_hud` (and `spell_loadout` for the tags) of `Scripts/playtests/us057_magic.py`
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` — run: `py Scripts/playtest_story.py US-057`
- **Steps:**

| Script | Check | Player step (bridge calls) → expected | Sweep result (8ea853b) |
|---|---|---|---|
| us057 | `fireball_cooldown_state` | same poll -> player.spells[0].cooldown_remaining_s>0 within 1s of the tap | FAIL (known) historically; PASS at `bc6d0ce` |
| us057 | `cooldown_hud` | same poll -> a SpellCooldown widget renders a numeric remaining time | FAIL (known) historically; PASS at `bc6d0ce` |

- **Playable alone:** YES (with US-088 landed first)

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: the four `DA_Spell_*.uasset` saved and committed; `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (content + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-057`: `fireball_cooldown_state` and `cooldown_hud` go from FAIL to PASS; no check that PASSed in the sweep now FAILs (watch: the SP plan and Blink/Stone Skin timing now interact with cooldowns); `--lint` exits 0 — never `--allow-stale`
- [ ] Known issues "Spells have no cooldowns" and N8 marked FIXED in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen vs. "Visible on screen"
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-089]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
- All four serialized assets carry their native cooldown tag and Jeff's durations: Fireball 2 s, Blink 4 s, Stone Skin 15 s, Restoration 10 s. The stale Fireball 5 s specification was corrected.
- Tier 1: 808/808 Mordecai tests PASS at `bc6d0ce`. Python tooling: 625/625 PASS. Fresh-editor PIE PASS. The content guard and US-057 lint PASS at `bc6d0ce`. The evidenced asset mutation changed Fireball's serialized cooldown duration from 2 s to 3 s; `verify_pie_magic.py` failed its exact-value check, and the mutation was restored.
- Clean package: `81f8cadd79f930077e8cbce43262f42a598be616`, built 2026-11-10T01:25:54-07:00 in 565.07 s (`Saved/Playtests/package-20261110-011601.log`), clean tree, no source changes during build.
- Tier 3: `Saved/Playtests/20261110-012620-boot`, CombatTestMap PASS: correct experience, no crash, all content cooked, Mordecai loading screen, and three valid engine frames. Desktop captures are covered by another window and are not evidence.
- Tier 4: `Saved/Playtests/20261110-012719-us057`, fresh package. `spell_loadout` PASS reports all four native tags; `fireball_cooldown_state` PASS reports 2.0 s at 0.31 s after Q; `cooldown_hud` PASS renders `2.0`. Final-head log: `Saved/Playtests/issue35-r2-us057/game.log`; `rg -F "but no CooldownTag" Saved/Playtests/issue35-r2-us057/game.log` returns zero matches. The Q cast spends SP 10→7 and cannot bypass GAS cooldown handling.
- Historical issue #35 evidence had `cooldown_widgets` FAIL with two A/B widgets. Issue #13 subsequently delivered four Q/E/R/1 widgets and all four bindings at `a7a098c`; `Saved/Playtests/20261112-055851-us057` has `cooldown_widgets` and `stone_skin_cooldown_hud` PASS. `stone_skin_indicator` remains N16 (`Status 10.0`) and predates both changes.
- Tier 4 screenshots: `bridge_01.png` shows the player and damaged TestDummy with SP 7/10 and the cooldown number; `bridge_02.png` shows Blink with SP 5/10; `bridge_03.png` shows Stone Skin's generic `Status 10.0` defect and SP 1/10; `bridge_04.png` shows the two existing cooldown slots while Stone Skin's long cooldown remains active.
- Full sweep: `Saved/Playtests/20261110-012826-sweep` at `81f8cad`, compared with `Saved/Playtests/20261109-214811-sweep` at `22e809a`: `fireball_cooldown_state` and `cooldown_hud` are the only status changes, both FAIL→PASS; zero PASS→FAIL.
- Final docs-head validation uses `Saved/Playtests/issue35-r1-final-boot` and `Saved/Playtests/issue35-r1-final-us057` on the repackaged documentation commit.
