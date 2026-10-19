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
