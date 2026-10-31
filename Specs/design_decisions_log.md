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
