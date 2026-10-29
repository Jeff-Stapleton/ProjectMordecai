# US-090: Recovery — Spell Point Budget: Cast the Whole Loadout (BLOCKED on decision N6)

**Epic:** Epic R: Playability Recovery
**Priority:** P2
**Depends on:** TODO(DECISION) N6 (Jeff); US-089
**Paired story:** none

## Overview
A fresh character has 10 SP, the arena loadout costs 12 (Fireball 3, Blink 2, Stone Skin 4, Restoration 3), nothing regenerates SP and respawning does not restore it. So the player can never cast all four spells, and after a few casts the magic slice is over for the session. This is a design question, not a code bug: this story implements Jeff's answer. Features: spells.

## Execution Mode
**HEADLESS**
Attribute defaults / a regen GameplayEffect defined natively and respawn restoration are C++ (if Jeff's answer needs a tuned DataAsset/GE asset instead, split an EDITOR pair at that point).

## References
- `Specs/agent_playtest_plan_v1.md` — N6 (`MordecaiAttributeSet.cpp:14-15` MaxSpellPoints 10; `MordecaiCharacter.cpp:357-359` respawn does not restore SP)
- Sweep run `Saved/Playtests/20261022-184841-us057` — plan "10 -> 7 -> 5 -> 1 (Restoration refused at 1)"; Notes: "a fresh character cannot cast all four spells once (design question N6)"; us054 `restoration_heals` can never get SP after the player has cast anything
- `Specs/character_attributes_v1.md`, `Specs/stat_formulas_v1.md`, `Specs/attribute_scaling_v1.md` (SpellPoints derivation), `Specs/ability_system_v1.md`
- Done stories: US-057 AC-057.1-057.4 (costs), AC-057.12 (full magic loop: Fireball → ... → Restoration → Blink → Stone Skin in one session)

## Acceptance Criteria
- [ ] AC-090.1: TODO(DECISION) N6 is answered by Jeff and recorded in `Specs/design_decisions_log.md` (options below) before any code is written
- [ ] AC-090.2: The chosen SP rule is implemented natively (exact values from the decision) — verified by `Mordecai.Recovery.SpellPoints.<Rule>` tests named after the decision (e.g. `RegenPerSecond`, `RestoredOnRespawn`, `MaxFromAttributes`)
- [ ] AC-090.3: In the packaged build, the player casts Fireball (Q), Blink (E), Stone Skin (R) and Restoration (1) in one session, each accepted — verified by NEW check `all_spells_castable` added to `Scripts/playtests/us057_magic.py` (Tier 4), which replaces the `restoration_refused_low_sp` expectation (or keeps a refusal check at whatever SP the new rule leaves)
- [ ] AC-090.4: If the decision includes respawn restoration: after a respawn the SP bar reads max — verified by NEW check `sp_restored_on_respawn` in `Scripts/playtests/us054_combat.py` (CombatTestMap, after `respawn`; `restoration_heals` has spent SP before the death there). Since US-100, `us054_arena.py` (DevTestMap) has no death or respawn.

## Technical Notes
- TODO(DECISION) N6 — options for Jeff (do not pick one):
  1. Raise base MaxSpellPoints (e.g. ≥ 12) or derive it from INT/WIS per `attribute_scaling_v1.md`.
  2. Passive SP regeneration (rate, out-of-combat only?).
  3. Restore SP on respawn (and/or at shrines/town).
  4. Lower spell costs.
  Combinations allowed. The decision also changes `us057_magic.py`'s SP plan and its `restoration_refused_low_sp` step.
- Keep SP rules in GAS (attribute defaults + infinite periodic GE for regen); no SP logic outside GAS.
- No PIE-only paths: nothing depends on `IsPlayInEditor()`, `WITH_EDITOR`, `LyraDeveloperSettings`, or editor-only state.

## Tests Required
### Tier 1 — Automation (headless)
- `Mordecai.Recovery.SpellPoints.*` — one per rule decided (AC-090.2)
### Tier 2 — PIE smoke (editor integration)
- `Scripts/verify_pie_magic.py`: SP max/regen as decided
### Tier 3 — Packaged playtest
- See Playtest
### Tier 4 — Real-input playtest
- `Scripts/playtests/us057_magic.py` new `all_spells_castable`; `Scripts/playtests/us054_combat.py` new `sp_restored_on_respawn` after `respawn` (if decided)

## Playtest
- **Launch path:** `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` (CombatTestMap → B_MordecaiArenaExperience; us057 and us054_combat play it since US-100); Tier 4: `py Scripts/playtest_story.py US-057` (and US-054 if respawn restoration is decided) on the same package
- **Player steps:** tap Q, E, R, 1 in order (with the movement the us057 plan already does)
- **Visible on screen:** the SP bar never blocks the 4th spell; `spell_hud` screenshot shows the SP bar value implied by the decision
- **Proof:** Tier 4 checks `all_spells_castable` (us057), `sp_restored_on_respawn` (us054_combat, if decided)
- **Tier 4 script:** `Scripts/playtests/us057_magic.py` — run: `py Scripts/playtest_story.py US-057`
- **Steps:** paste the new rows from `py Scripts/playtest_story.py US-057 --plan` after the decision
- **Playable alone:** YES (after the decision)

## Definition of Done
- [ ] All ACs checked off
- [ ] Tier 1: project compiles; ALL `Mordecai.*` automation tests pass (`py Scripts/run_tests.py` exits 0)
- [ ] Tier 2: PIE smoke tests pass (`py Scripts/run_pie_verify.py`); direct-call checks labeled LOGIC
- [ ] Content: `py Scripts/check_content_committed.py` exits 0
- [ ] Work committed FIRST (code + script edits), then a fresh package of that commit: `py Scripts/package_playtest.py` exits 0
- [ ] `py Scripts/playtest_boot.py --map /MordecaiCore/Game/Maps/CombatTestMap` exits 0 — never `--allow-stale`
- [ ] On that same fresh package `py Scripts/playtest_story.py US-057` (and US-054): the new checks PASS; no check that PASSed in the sweep now FAILs; `--lint` exits 0 — never `--allow-stale`
- [ ] Known issue N6 marked resolved in `Specs/agent_playtest_plan_v1.md`
- [ ] Every screenshot (`engine_NN.png`, `bridge_NN.png`) opened with the Read tool; Completion Notes state what is actually on screen
- [ ] Evidence cited: run dirs in Completion Notes AND in the body of the `[US-090]` closing commit
- [ ] Pushed; story moved to `stories/done/` only after all of the above pass

## Completion Notes
<Filled in by the coding agent.>
