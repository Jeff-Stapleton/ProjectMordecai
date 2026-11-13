# Design Decisions Log

Decisions made by Jeff, recorded for reference across stories and epics.

---

## 2026-03-19 — Combat Timing & Tuning

### Parry / Block Windows
- **Standard parry window**: 500ms
- **Perfect parry/block window**: 250ms
- Perfect window is entirely inside the standard window — all perfect parries are parries, but not all parries are perfect parries
- Source: Jeff via Discord (#coder-cron-debug)

### Failed Parry Consequences
- **No extra damage multiplier** on failed parry
- Player is **animation-locked** (cannot activate any abilities) for ~600ms
- Design intent: "helpless to avoid or block incoming hits" — like Super Smash Bros dodge/counter recovery
- The punishment is being a sitting duck, not taking bonus damage

### Posture Break / Stagger / Knock Down
- **Stagger duration**: 1 second (1000ms) when posture breaks
- **Stagger damage multiplier**: 1.5x
- **Knock Down**: If enough damage is dealt during the stagger window, enemy transitions to "knocked down" state
  - Extends the vulnerability window (longer than 1s stagger)
  - **Knock Down damage multiplier**: 2.0x
  - Knock Down replaces Stagger (escalation, not stacked)

### Stamina Tier Multipliers
- **Green (full stamina)**: 1.25x (25% bonus — reward for managing stamina well)
- **Yellow (mid range)**: 1.0x (normal — this is the "standard" operating range)
- **Red (low)**: 0.75x (25% penalty)
- **Exhausted (depleted)**: 0.5x (50% penalty)
- Design intent: Full stamina should feel rewarding. Normal range is a big chunk of the bar. Penalties ramp up at the bottom.

### Pierce / Projectile Range
- **Pierce count**: Per-projectile (e.g., arrow can pierce up to 2 targets)
- **Pierce damage falloff**: NONE — 100% damage to all pierced targets
- **Max range**: All projectiles and attacks must have a max range/lifetime. Nothing should travel much further than the edge of the screen. Despawns after max range or lifetime (whichever first).
- **Damage consistency**: Full damage to every target hit, whether first or last in a pierce chain

---

## 2026-07-10 — Inventory Vertical Slice ("initial-inventory" request)

### Press-to-Pickup (supersedes US-079 overlap-auto-pickup note)
- Items on the ground are picked up with the **Interact** input (**F** keyboard / **X** gamepad, per control_bindings_v1.1 "Talk, loot, activate"), not by walking over them
- Driven by Jeff's "initial-inventory" request: UI must prompt the player with the pickup key, which requires a deliberate press
- Auto-store routing (materials/town resources → ledger) is unchanged — only the trigger moved from overlap to key press
- Source: Jeff via "initial-inventory" pipeline request (2026-07-10)

### Inventory / Pause Menu Key Bindings
- **Esc / Gamepad Start** toggles the pause menu (last-active tab), matching control_bindings_v1.1 "Pause / Menu"
- **I** additionally toggles the pause menu open directly on the **Inventory** tab
  - Rationale: Esc stops PIE in-editor, so the slice needs a PIE-usable key; "I" is the genre-standard inventory key
  - TODO(DECISION): "I" is not yet in control_bindings_v1.1 — confirm and fold into the next bindings doc revision
- UI assist hint shows "[I] Inventory"; pickup prompt shows "[F] Pick up <item>"
- Device-aware key glyphs (controller glyphs when on gamepad) deferred — prompts show keyboard keys for the slice
- Source: Jeff via "initial-inventory" pipeline request (2026-07-10)

---

## 2026-10-29 — Test maps (T1, issue #46)

Context: T1 (`Specs/agent_playtest_plan_v1.md`) gives the Tier 4 playtests their own cooked maps. MovementTestMap has no enemies; CombatTestMap has a dummy and a fixed-cadence attacker. Implemented by US-100. The choice of T1 over moving the Frontliners out of aggro was made earlier (Jeff, 2026-10-28, issue #19; plan entry N14).

### Crouch is out of scope
- Issue #46 asked whether the movement test map should test crouch. Decided: **no**. T1 builds no crouch geometry or check.
- Mordecai's crouch is the spec'd **Sneak (Toggle)**: Ctrl / R3, "reduces detection + footstep noise; reduces move speed" (`control_bindings_v1.1.md`). The Sneaking skill is crouched movement (`skill_sheet_v1.1.md`).
- It belongs to US-034 (Movement Skills), which is blocked on placeholder skill values. No crouch control is bound today: Lyra's crouch binding was removed in US-083 (N23).
- Source: Jeff via Claude Code session (2026-10-29)

### Which playtest runs on which map
- **us069 (pause menu) moves to MovementTestMap**, alongside us001 (movement) and us083 (combat input), which issue #46 and US-100 already put there. Issue #46 had not listed us069.
- **us079 (inventory) stays on DevTestMap.** Its six test pickups stay where US-079 placed them, and US-079's "reuse the existing arena, do not create a new level" note stands. Consequence: the inventory route still races the Frontliner swarm, so US-094, US-079 and US-071 still close at Jeff's call (one exit-0 run so far: `Saved/Playtests/20261028-232550-us079`).
- Source: Jeff via Claude Code session (2026-10-29)
- For reference (from issue #46 and US-100, not part of this decision): us054's melee/dodge/death/respawn chain (new script `us054_combat.py`), us057 (magic) and us078 (weapons) run on CombatTestMap. us054's enemy-AI checks (`us054_arena.py`) stay on DevTestMap.

### Close the inventory slice on the US-100 us079 run
- **US-094, US-079 and US-071 are closed** on `Saved/Playtests/20261029-175841-us079` (exit 0, 56/56, fresh package of `9904cde`). The player won the race against the swarm in that run.
- Accepted with the closing: AC-079.2's overlap pickup is superseded by Press-to-Pickup (above). AC-079.10's `BP_MordecaiItemPickup` was never made; the native `AMordecaiItemPickup::NameLabel` serves its purpose.
- Source: Jeff via Claude Code session (2026-10-29)

---

## 2026-10-30 — Defenses against enemy melee (issue #53, US-101)

Context: nothing applied block, parry, dodge i-frames, Blink invulnerability or jump-avoid to an enemy melee hit (N32). US-101 routes every melee hit through the target's defenses in `UMordecaiGA_MeleeAttack::ApplyDamageToTarget` (`FMordecaiHitDefense`). The specs leave the points below open. They were **taken by the coding agent for Jeff** so the fix could land; each one is reversible, and Jeff may revisit any of them.

### Precedence
- I-frames first (`Mordecai.State.Invulnerable`, `Mordecai.State.Dodging`), then an active parry window, then an active block. Whether a jump avoids the hit is decided earlier, in the hit query (`Mordecai.State.Airborne` + `JumpAvoidable`, AC-003.7).
- Only AC-006.17 orders anything (parry over block). The rest is new.

### What a negated hit skips
- I-frames and a successful parry negate the whole hit: health, posture, the enchant bonus, on-hit payloads, status effects and the damage number.
- Sources:
  - "incoming damage is negated" (AC-005.3)
  - "avoiding all damage" (`skill_sheet_v1.1.md`, Dodge)
  - "immune to all damage" (the Invulnerable tag)
  - `ReceiveParriedHit` takes no damage argument
- Not decided: whether a dodge or Blink should also protect from projectiles, spells and DoT ticks. Today only melee consults the defenses.

### Block
- A perfect block still mitigates health (×(1 − Mitigation)), as `ReceiveBlockedHit` already did. AC-006.10 only names the stamina and attacker-posture rewards.
- Blocked hits, perfect ones included, take posture × `BlockPostureDamageMultiplier` 0.5 (AC-007.5).
- The hit that breaks the guard is still mitigated. Hits during the guard break land in full.
- The damage number shows the health actually taken.
- **Still open, left as before:**
  - Block covers every direction. `skill_sheet_v1.1.md`'s shields say "frontal", but US-006 has no direction.
  - A blocked hit still applies on-hit payloads, status effects and the enchant bonus in full. Nothing that enemies swing today has any.

### Airborne
- `Mordecai.State.Airborne` is on exactly while the character's movement mode is Falling: a jump, a drop off a ledge, a launch (including a dodge's few launch frames, which are inside its i-frames anyway).
- This applies to the player and to every enemy.
- Not "a jump only" and no height threshold; neither is specified.

### Blink's invulnerability is fixed here (N19)
- US-101 fixes N19 ahead of US-087, because once damage honors Invulnerable, a stuck tag means a permanently invulnerable player.
- The fix is the one US-087 AC-087.7 prescribes: a duration GameplayEffect grants the tag.
- The duration stays `DA_Spell_Blink`'s BuffDuration (0.3 s; AC-021.3 says "default 0.2s").

- Source: decisions taken by the coding agent for Jeff (Claude Code session, 2026-10-30), pending his review

---

## 2026-10-30 — Pause menu input (issues #23 and #32, US-095)

### LB/RB switch pause-menu tabs (N18)
- **Kept.** AC-069.4's controller tab switching stands. While the pause menu is open, LB selects the previous tab and RB the next, wrapping at either end (Character ↔ Settings).
- It does not conflict with LB/RB = Skill B / Skill A in game (`control_bindings_v1.1.md`): CommonUI's Menu input mode blocks game input while the menu is open, and the menu consumes the shoulder presses.
- No keyboard equivalent was added (none is specified); the mouse switches tabs by clicking the tab headers.
- Source: Jeff via Claude Code session (2026-10-30): "Fix this issue" on issue #23, whose open question was keep or drop.

### I closes the open pause menu
- I already toggled the menu open on the Inventory tab (2026-07-10 above). Since US-095, I closes the open menu through CommonUI UI action bindings, like Escape and Gamepad B.
- The close happens on the **key-up** of an I press made in the open menu (the press arms it). The key-up and repeats of the press that opened the menu do not close it.
- Why the key-up: the menu consumes the whole press. Closing on the press would let the OS key repeats of a held I reach the game after the menu is gone, and the Inventory action would reopen the menu about 0.5 s later. Escape still closes on the press through CommonUI's back action, so a held Escape can still do that. That is pre-existing and left as is.
- Still open: the 2026-07-10 TODO(DECISION) to add "I — Inventory" to `control_bindings_v1.1.md`. The bindings doc also does not list in-menu LB/RB yet.
- Source: issue #32 / US-095 AC-095.1 (fixed with #23 in the same story, Claude Code session, 2026-10-30)

---

## 2026-11-04 — What the kill counter counts, and when it resets (issue #6, US-086)

### What counts
- Only an enemy death the player caused counts. The credit comes from the killing blow's GAS effect context: its instigator is the player's PlayerState (whose ASC made the effect), or its effect causer is the player's pawn. That covers the player's melee, projectiles, spells and DoT ticks.
- An enemy that dies to itself, to another enemy, to a Health change with no effect context (the environment, a direct set) or to a direct `EnterDeathState` call counts 0. Nothing in the game kills enemies that way today.
- A dead enemy counts once. It can count again only after `ResetForArena` revives it.
- A kill that lands while its player is dead (a projectile or DoT from the life that just ended) scores for no one. Jeff: "Ideally the kill isn't awarded to anyone", and minor.
- The 2026-11-02 note that a refreshed Burning keeps its original applier's credit (closed PR #76) is not built. The case needs an enemy's Burning and a player's on the same enemy, and that cannot happen today: enemies never hit each other (friendly fire is off between `Mordecai.Team.Enemy` holders). Today the credit follows the context of the active Burning spec.

### When it resets
- **At the player's death.** `AMordecaiGameMode::NotifyPlayerDeath` saves the life's total for the death screen and resets the live count to 0 (AC-063.2). `HandlePlayerDeath` calls it before `Mordecai.State.Dead` goes on, so the death screen reads the saved total and does not depend on the order of its tag listener.
- **Not at the respawn.** The respawn's automatic arena reset revives the enemies and leaves the count alone.
- **At a manual arena reset.** A manual `ResetArena` call resets the count too.
- AC-062.2's "session kill count" on the death screen therefore means the kills of the life that just ended.
- AC-063.1 and AC-063.2 named the mechanism, `Mordecai.Event.EnemyKill` / `Event.PlayerDeath` tag events, and it never worked. The behaviour stands: the count goes up on a kill and resets at the death. The reset runs in the same `HandlePlayerDeath` call that sends `Event.PlayerDeath`.

### One count
- `AMordecaiGameMode::GetEnemyKillCount()` is the one count. The HUD counter shows it through `OnEnemyKillCountChanged`, and the agent bridge reports it as `player.kills`. The dead `Mordecai.Event.EnemyKill` / `Event.PlayerDeath` tag-count listeners are gone; the `Event_EnemyKill` tag stays defined, unused.
- Single-player: one count for the local player of a standalone or listen-server game. Per-player replicated counts, and a HUD and death screen for remote clients, are issue #77.

- Source: Jeff via Claude Code session (2026-11-04). He confirmed "reset at death" and "only player-caused kills", first recorded on the unmerged branch of closed PR #76, and chose that a death-window kill scores nothing.

---

## 2026-11-05 — Enemy indicator dual-bar interim hold (issue #22, US-092)

- **Interim decision for US-092:** keep both the legacy `WBP_EnemyHealthBar` and the new IndicatorSystem enemy readout. Do not remove the old bar or rewrite `enemy_health_bars` in this story.
- Packaged screenshots now show both readouts together and confirm that their bars and labels overlap heavily.
- **Final decision remains open:** Jeff still needs to choose whether to remove `WBP_EnemyHealthBar` and migrate `enemy_health_bars` to the indicator, or keep both readouts.
- Source: Jeff via Cursor session (2026-11-05): hold dual-bar removal until packaged screenshots show both; the screenshots now exist, so the hold becomes an explicit `TODO(DECISION)` rather than permission to remove either bar.

## 2026-11-05 — Enemy indicator replaces legacy health bar (issue #22, US-092)

- **Final decision (supersedes the interim hold above):** remove `WBP_EnemyHealthBar` entirely.
- The EnemyIndicator health bar is always red and keeps the retired bar's 150 px width.
- `enemy_health_bars` migrates to the single EnemyIndicator readout and verifies that no legacy actor-attached bar remains.
- Source: Jeff via Cursor session (2026-11-05), after reviewing the packaged dual-bar screenshots.

## 2026-11-05 — Enemy indicators are always on from spawn (issue #22, US-092)

- **Final decision:** every living enemy shows its EnemyIndicator from spawn, like the retired legacy bar. Registration is not gated on aggro or damage.
- Death still unregisters the indicator. Arena reset restores it and rebinds the widget to current health.
- The Idle CombatTestMap TestDummy (`AggroRange = 0`) therefore shows its one red, 150 px EnemyIndicator throughout its living state.
- Source: Jeff via Cursor session (2026-11-05), resolving the AC-100.3 product call.

---

## 2026-11-08 — Isolated pickup-projectile test fixture (issue #16, US-087)

- Jeff's approval, verbatim: **“Yes, add the isolated test map and expand the change budget as needed for that coverage and validation.”**
- Authorized fence: add one enemy-free isolated test map for real-pickup projectile coverage and validation.
  All existing maps and gameplay remain unchanged.
- The characterization does not assume or establish a native-pickup bug. An inconclusive shared-arena failure
  with enemy interference is not collision proof.
- The saved fixture was a one-time EDITOR authoring step. Remaining US-087 Blink work stays HEADLESS, and the
  fixture does not change acceptance ownership: `us087_fireball.py` solely owns AC-087.4/.5;
  `us087_pickup_lane.py` owns no AC (`COVERS=()`, every `proves=()`).
- Source: Jeff via Cursor session (2026-11-08).

---

## 2026-11-09 — Facing arrows on dead enemies (issue #17)

- **Jeff's exact decision:** “Keep the arrow visible on dead enemies.”
- **Implementation context (not Jeff's words):** `AMordecaiEnemyCharacter::EnterDeathState` hides the issue #19 placeholder `BodyMesh` but leaves the green facing arrow registered and visible. `ResetForArena` later revives the enemy and restores the body; the same arrow component remains registered throughout.
- Source: Jeff via Cursor session (2026-11-09, about 15:30 MST).

---

## 2026-11-09 — Weapon effects reach the ASC (issue #36, US-096)

- Jeff: "Repair the five weapon tags in this PR and add the packaged us078 tag check"
- Jeff: "Prove stat modifiers with a test-only modifier in automated tests, no balance change"
- Jeff: "Only AC-096.3 in this PR; leave previous-weapon N9 open"
- Jeff: "File a separate issue for multiplayer equipment replication"
- **Implementation context (not Jeff's words):** issue #95 tracks multiplayer equipment replication.
  This change keeps replication out of scope, repairs only the five existing weapon type tags,
  and adds no stat modifiers to game assets.
- Source: Jeff via Cursor session (2026-11-09).

---

## 2026-11-12 — Combat callout grows upward from a fixed bottom (US-106)

- **Final decision:** the combat callout's bottom edge remains fixed at centre `+180` for Normal, Large and XL text.
- Large and XL increase the callout slot height upward from that fixed edge.
- The combo remains fixed immediately below it at centre `+184`; the two regions do not overlap at any supported text size.
- Source: Jeff via the US-106 fix-round-2 coordinator decision (2026-11-12).

---

## 2026-11-13 — Starting skill registrations (US-093 round 2)

- Zero registered skills is a real content gap, not an acceptable empty state for AC-093.4.
- Register every nonblank row of `Specs/skill_sheet_v1.1.md` with its exact skill name and mapped existing `EMordecaiSkillCategory`. Do not add a category enum for an unmatched sheet category.
- All starting skills begin at rank 0 with max rank 20. Registration itself grants no abilities and changes no attributes or skill points.
- Keep registration data-driven through `UMordecaiSkillDataAsset` and `UMordecaiSkillComponent::RegisterSkillDataAsset`; a compiled native table sourced from the sheet is acceptable and must work in a packaged build.
- Settings remains the accepted `Coming Soon` placeholder; a Tin Tally restyle remains outside US-093.
- Source: US-093 round-2 coordinator decisions for Jeff (2026-11-13).

---

## 2026-11-13 — US-093 round-3 review decisions

### Magic Missile spelling
- **Final decision:** correct the skill-sheet typo `Magic Missle` to `Magic Missile` everywhere: stable skill ID, localized display name, Markdown sheets, the editable v1.1 workbook, and downstream design research.
- This matches the existing ability and glyph spelling.
- Source: US-093 round-3 coordinator decision for Jeff (2026-11-13).

### Laptop-builder screenshot review
- For laptop-agent runs, builders never open PNG/image files with the file-read tool because doing so crashes the agent.
- Builders validate captures with PIL/scripted image analysis, preserve the original images in the checksummed evidence bundle, and do not claim that a person viewed them.
- Mordecai Bot opens and describes the bundled captures before merge. This coordinator rule overrides the builder image-opening steps in `CLAUDE.md` 4c.4 and 4d.4; all other Tier 3/Tier 4 evidence rules remain in force.
- Source: US-093 round-3 coordinator decision for Jeff (2026-11-13).

---

## 2026-11-13 — Verdant Wilds v3 tile footprints and fixture orientation (US-120)

- For 1x1 cliff, pit, and pond edge pieces, the local origin is the midpoint of the authored grid edge between tiles. The body/high-or-land side is local +Y, and collision uses the high/land-side tile footprint: X -100..100, Y 0..200, with Z from the piece.
- Corners use the high/land-side tile quadrant or quadrants implied by their bounds. Ramps use the catalog ramp pivot and `run_tiles`/`width_cm`.
- Measure every piece's visual overhang past the face into local -Y. An overhang above 15 cm is a per-piece art defect to report with its measured value; it does not stop the import and must not be fixed by editing the mesh.
- `L_VW_TileTest` uses camera yaw 0: screen-up/north is Unreal +X and screen-right/east is Unreal +Y. Report the imported FBX face/downhill local axis for a straight cliff and the 2T ramp.
- Source: US-120 coordinator clarification for Jeff (2026-11-13).
