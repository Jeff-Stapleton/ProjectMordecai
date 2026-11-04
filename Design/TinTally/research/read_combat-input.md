# Project Mordecai: combat, attacks, abilities, spells, damage and controls (UI research)

Scope: I read these files in full: `Specs/combat_system_v1.md`, `Specs/attack_taxonomy_v1.md`, `Specs/ability_schema_v1.md`, `Specs/ability_system_v1.md`, `Specs/player_attacks_agent_brief_v1.md`, `Specs/damage_types_v1.md`, `Specs/new_spells_proposal.md`, `Specs/control_bindings_v1.1.md`, `Specs/control_bindings_v1.md`, `Source/LyraGame/Mordecai/Combat/MordecaiCombatTypes.h` and `Source/LyraGame/Mordecai/MordecaiGameplayTags.h`.

I checked the numbers against the code:
- combat abilities, stamina and posture systems
- spell base, spell data and the shipped spell data values
- weapon profile factory
- HUD widgets
- runtime input map
- `Specs/design_decisions_log.md`
- the known-issues list in `Specs/agent_playtest_plan_v1.md`
- the related backlog and done stories

Status words: **implemented** means it exists in code. **Specified** means a spec describes it. **Implied** means a system needs it but no UI is specified. **Planned-story** means a story file plans it.

Big picture: the specs are thin on numbers, and most real values live in code. Several HUD widgets exist but never trigger in the game; those are flagged below. Two design themes run through the specs:
- `game_design_v2.md` §2 pillar 8 and §9: "Clean UI: Always show Health (0–100%), tiered Stamina, Spell Points. Status is primarily VFX-driven, not icon soup."
- Its pillar 2 is "Granular skills (DCC vibe): 'Everything you do is a skill.'"

---

## Resources the player manages (Health, Stamina, Posture, Spell Points: ranges, regen, break/exhaust states, costs)

The specs list three core resources: "Health (0--100%) Stamina (tiered resource) Spell Points (SP)" (`combat_system_v1.md`). Posture is a fourth meter, specified for enemies and present on the player HUD. Defaults are in `MordecaiAttributeSet.cpp:9-17`: Health 100/100, Stamina 100/100, SpellPoints 10/10, Posture 100/100. All nine primary stats default to 10.

### Health
- **Spec:** "Represents survivability. Reaching 0 → death or downed state" (`combat_system_v1.md`). Shown "as % (0–100)" (`game_design_v2.md` §4). Always visible: "Health%" (§9).
- **Scaling:** MaxHealth = BaseMaxHealth × (1 + 0.06 × EffCon). A `HealthRegenMultiplier` attribute exists (1 + 0.015 × EffCon) (`MordecaiAttributeSet.cpp:94,122`).
- **HUD (implemented):** `UMordecaiHealthBarWidget`, a plain green progress bar (`FLinearColor::Green`), 250×20 px at the top-left (20,20).
  - The fill is Health/MaxHealth clamped 0–1.
  - **No numeric or % text is drawn**, although AC-052.2 says "displays Health as a percentage (0–100%)".
- **Death (implemented):** `UMordecaiDeathScreenWidget` shows:
  - "You Died" (font 48, colour 0.8, 0.1, 0.1)
  - a respawn countdown ("%d", font 32)
  - "Kills: %d" (font 20)
  - all over a black dim at 0.5 alpha.
- No "downed" state exists in code.

### Stamina (tiered)
- **Spec:**
  - "Consumed by: attacks, dodge, block, sprint. Low stamina reduces effectiveness but does not fully prevent actions" (`combat_system_v1.md`).
  - "Tiered; low stamina reduces effectiveness, never blocks actions" (`game_design_v2.md`).
  - Dodge is "Always possible; effectiveness depends on stamina tier" (`control_bindings_v1.1.md`).
- **Tiers** (`MordecaiStaminaSystem.h/.cpp`, design log 2026-03-19):

| Tier | Range (combat logic) | Range (HUD `CalcStaminaTier`) | Effectiveness multiplier | HUD bar colour (`MordecaiStaminaBarWidget.h`) |
|---|---|---|---|---|
| Green | ratio > 0.66 | ≥ 66% | **1.25×** ("25% bonus — reward for managing stamina well") | (0, 1, 0) |
| Yellow | 0.33 – 0.66 | ≥ 33% | **1.0×** ("the standard operating range") | (1, 1, 0) |
| Red | < 0.33 | > 0% | **0.75×** | (1, 0, 0) |
| Exhausted | Stamina ≤ 0 (overlay) | ≤ 0 | **0.5×** | (0.5, 0, 0) |

- Design intent (design log): "Full stamina should feel rewarding. Normal range is a big chunk of the bar. Penalties ramp up at the bottom."
- **Floor:** stamina can go **negative, down to −50** (`StaminaFloor = -50`). The HUD clamps its fill to 0–1, so any negative value reads as an empty bar.
- **Regen:** 15/s (`StaminaRegenRate`) after a **1000 ms** delay following any spend; each new spend resets the delay. The END scaling (END × 2.0%) is a `TODO(DECISION)` stub.
- **Sprint drain:** 10/s (`SprintStaminaDrainRate`). Tags `Mordecai.State.StaminaRegenPaused` and `Mordecai.State.Sprinting` are declared.
- **Max:** MaxStamina = BaseMaxStamina × (1 + 0.04 × EffEnd). StaminaRegenMultiplier = 1 + 0.02 × EffEnd.
- **Costs:**
  - Dodge 15 (taken even at 0 stamina)
  - Parry 20
  - Block drain per blocked hit = IncomingDamage × (1 − Stability 0.3); a perfect block drains nothing
  - Attacks cost 2–30 depending on weapon and step (table below)
  - Spells cost SP, not stamina
- **Reality check (N30, `agent_playtest_plan_v1.md`):** "The player's stamina never regenerates." Nothing calls the regen or the sprint drain.
  - Measured in play: 100 → 80 → 55 → 30 → 22 → −3 → −18 → −26 → −50 in about 15 s of fighting, then it stayed at −50.
  - "The HUD's stamina bar shows it empty (grey) from then on."
  - Also, no code under `Source/LyraGame/Mordecai` adds the `Mordecai.Stamina.Tier.*` tags (grep; only `GetTierMultiplierFromASC` reads them). The tier multiplier therefore falls back to 1.0 in play.
- **Tags:** `Mordecai.Stamina.Tier.Green / .Yellow / .Red / .Exhausted`. Their comments read "Stamina above 66% — full effectiveness + bonus", "33-66% — normal effectiveness", "below 33% — reduced effectiveness" and "at or below 0 — heavily reduced effectiveness".

### Posture
- **Spec:** "Enemies have posture meters. Posture damage comes from: heavy attacks, perfect parries, certain abilities. When posture breaks: enemy enters **critical window** - player can perform riposte" (`combat_system_v1.md`).
  - Also: "Enemy Posture: built by heavy hits + perfect defense + certain spells; break → critical/riposte window" (`game_design_v2.md`).
  - Also: "Player Stability: determines stagger/interrupt resistance; increased by armor + DIS + gear + skills + feats."
- **Numbers** (`MordecaiPostureSystem.h`, design log):
  - Break at Posture ≤ 0 enters `PostureBroken` (stagger) for **1000 ms**, and damage taken is **×1.5**.
  - If **≥ 50** damage lands during the stagger, it escalates to `KnockedDown` for **2000 ms** at **×2.0** damage. Knock-down "replaces Stagger (escalation, not stacked)".
  - After either state, posture resets to max.
  - Regen is **10/s** after a **3000 ms** delay, and never during broken or knocked-down states.
  - A blocking target takes posture × **0.5**.
  - Posture damage = BasePower × PostureDamageScalar (× Weakened/Brittle stubs, currently 1.0).
  - A perfect block deals **30** posture to the attacker; a parry deals **50**.
- **Max:** MaxPosture = BaseMaxPosture × (1 + 0.04 × EffDis). PostureRecoveryMultiplier = 1 + 0.02 × EffDis.
- **Who it applies to:**
  - Only enemies run break/regen logic (`MordecaiEnemyCharacter.cpp`).
  - The player has a Posture attribute that enemy swings lower, but no break logic.
  - The **player** HUD posture bar is implemented: 250×16 px at (20,76), normal (0.2, 0.6, 1.0) blue and (1.0, 0.2, 0.2) red while `Mordecai.State.PostureBroken`.
  - **No enemy posture meter is drawn anywhere.** The enemy indicator shows only name, health and status icons.
  - `new_spells_proposal.md` mentions a "boss posture bar" (Enfeeble R20 "boss posture bar takes a chunk on cast"; Snare R20 "Boss posture damage on cast").

### Spell Points (SP)
- **Spec:**
  - "Used for abilities and spells. Regenerates slower than stamina. INT improves regeneration" (`combat_system_v1.md`).
  - "Discrete points; slower regen than stamina; INT improves regen; spells can upcast by spending more points" (`game_design_v2.md`).
  - The town can "Improve SP regen/cost via research lines".
- **Code:**
  - Default 10/10. MaxSpellPoints = Base × (1 + 0.04 × EffInt). SpellPointsRegenMultiplier = 1 + 0.02 × EffInt.
  - **No SP regen is implemented**, and respawn does not restore SP (N6). The arena loadout costs 12 SP (Fireball 3, Blink 2, Stone Skin 4, Restoration 3) against a max of 10. This is blocked on a design decision (US-090).
- **Upcast:** cost × `UpcastCostMultiplier` 2.0 and power × `UpcastPowerMultiplier` 1.5. No input is bound for upcasting.
- **HUD (implemented):** `UMordecaiSpellPointsBarWidget`, purple (0.4, 0.2, 0.8) fill, 250×16 px at (20,100), with centred text "`%d / %d`" (font 10). AC-055.3: "since SP is a discrete resource unlike health percentage."
- **Other resource-like attributes a HUD may need:**
  - `FireWardShieldHP` (Fire Ward shield)
  - `DamageReduction` (Stone Skin)
  - `EnchantWeaponBonusDamage`
  - `RangedEvasionChance` (Blur)

---

## Attacks and combos (light/heavy, combo counts, charge, timing windows a HUD could show)

**Taxonomy** (`attack_taxonomy_v1.md`, `player_attacks_agent_brief_v1.md`):
- "Melee attacks are simplified into 3 canonical shapes":
  - **Sweep**: an arc sector. "Pro: multi-target, good for crowds"; counterplay is "jump if tagged low".
  - **Thrust**: a narrow capsule lane. Counterplay is "horizontal dodge / spacing; cannot be jumped".
  - **Slam**: a point circle with an optional shockwave ring. "Pro: posture impact / AoE at point"; counterplay is "exit the point; cannot be jumped".
- Jump rule: "Sweeps: `JumpAvoidable=true` (low sweep); Thrusts/Slams: `HitsAirborne=true`."
- **Projectiles:** soft aim assist (fan-out at "0°, ±2°, ±4°, ±6°"; "No snapping/lock-on"; code `AssistRange` 3000). Supports pierce, ricochet, AoE on hit and status payloads.
  - Design log: pierce has no falloff, and "Nothing should travel much further than the edge of the screen."
  - `MaxRange` defaults to 2000 UU.
- **AoE origin modes:** Self-origin, Attached aura, Ground-targeted ("player becomes rooted, moves a reticle, releases to cast").

**Enums (code):**
- `EMordecaiAttackType` {MeleeSweep, MeleeThrust, MeleeSlam, Projectile, AoE}
- `EMordecaiInputSlot` {Light, Heavy, SkillA, SkillB, Throw}
- `EMordecaiRootedMode` {None, Windup, Active, Full}
- `EMordecaiHitShapeType` {ArcSector, Capsule, Box, Circle}
- `EMordecaiAttackPhase` {None, Windup, Active, Recovery}

**AttackProfile fields:**
- WindupTimeMs, ActiveTimeMs, RecoveryTimeMs, RootedDuring
- HitShape, HitsAirborne, JumpAvoidable
- DamageScalar / DamageProfile, PostureDamageScalar, StaminaCost
- OnHitPayload, OnUsePayload, StatusEffectsOnHit (chance 0–1)
- ComboIndex "1..N (light chains)", CancelableInto Dodge/Block ("default: committed")

**Light vs heavy input (code, `MordecaiHeroComponent`):** one button, LMB or RT.
- Released in under **0.4 s** (`HeavyAttackHoldSeconds`) it is a light attack (Tap).
- Held **≥ 0.4 s**, the heavy attack fires **on release** (HoldAndRelease).
- `TODO(DECISION)`: the spec says "Charge time depends on weapon", but 0.4 s is used for every weapon. A charge meter is therefore implied.

**Combo window:** `ComboWindowTimeoutMs` = **800 ms**, opened during the light attack's Recovery phase.
- The tag `Mordecai.State.ComboWindow` ("Combo input window is open during Recovery phase") is declared but never added by game code.
- **N24:** "Light attacks never combo": `NotifyComboInput` has no caller.
- **N25:** cancel windows (`CanCancelIntoDodge/Block`) are not wired.

**Weapons and combo counts.** The values below come from `MordecaiWeaponProfileFactory.cpp`. Timings are windup/active/recovery in ms. BP = base power, St = stamina cost, PS = posture scalar.

| Weapon (slot, dmg type, base dmg, speed ×, range) | Light chain | Heavy |
|---|---|---|
| **Longsword** (MainHand, Slash, 4, 1.0, 200) | **3** sweeps: L1 r200/120° 200/150/250 BP12 St8 PS0.5; L2 r200/150° 180/170/250 BP14 St10; L3 r220/180° 250/200/350 BP18 St14 PS0.8 | 360° spin sweep r220, 500/250/500, BP28 St25 PS1.5, rooted Active, no cancel |
| **Greatsword** (TwoHand, Slash, 8, 0.75, 250) | **2** sweeps: BP20 St14; BP26 St18 | Slam circle r180, 600/200/600, BP35 St30 PS2.0, rooted |
| **Shortsword** (MainHand, Slash, 2, 1.2, 150) | **4**: 3 sweeps (BP8/9/10, St5/6/7) + thrust finisher (BP14 St10) | Thrust 180×70, 300/150/350, BP20 St18 PS1.0 |
| **Dagger** (MainHand, Pierce, 1, 1.4, 120) | **5** thrusts: BP5/5/6/6/10, St3/3/4/4/8 | Thrust 160×50, 250/120/300, BP22 St15 PS1.2 |
| **Axe** (MainHand, Slash, 6, 0.85, 180) | **2** sweeps: BP16 St12; BP22 St16 | Slam r160, 500/200/550, BP30 St24 PS1.8, rooted |
| **Mace** (MainHand, Blunt, 5, 0.8, 160) | **2**: sweep BP14 St12 PS1.0; slam r140 BP18 St14 PS1.2 | Slam r180, 600/200/600, BP28 St28 PS2.5, rooted |
| **Spear** (TwoHand, Pierce, 5, 0.9, 280) | **3**: thrust 280×50 BP12 St8; thrust 290×55 BP14 St10; sweep r250/160° BP18 St14 | Thrust 320×60, 400/180/450, BP26 St22 PS1.3, rooted |
| **Quarterstaff** (TwoHand, Blunt, 3, 1.0, 240) | **3**: sweep, thrust, sweep (BP10/11/15) | 360° sweep r260, BP22 St20, rooted |
| **Unarmed** (MainHand, Blunt, 0, 1.3, 100) | **3** jabs (BP3/3/5) | Slam r80, BP8 St8 (not wired in game, N29) |

- **Arena starting loadout** (`MordecaiCharacter.cpp:57-61`): Longsword, Dagger, Greatsword, Spear, Mace, as 5 `DA_Weapon_*` assets with matching `DA_AP_*` profiles. The first one is auto-equipped.
- Tab / Gamepad Y cycles to the next weapon and BackSlash to the previous (N9: Shift+Tab is specified but not mapped).
- **Spec combo counts differ from the code:**
  - `attack_taxonomy_v1.md`: Longsword "Light chain (3)" and Spear "Light chain (5)". The older brief calls the 5-chain weapon a "Rapier".
  - `skill_sheet_v1.1.md` milestone combos: Longsword 3, Greatsword 2, Shortsword 4, Dagger 4, Axe 3, Mace 3, Spear 3, Quarterstaff 3, Unarmed 5.
- **Weapon-triggered secondary effects:**
  - Examples: "Hammer slam → shockwave AoE at impact", "Sword spin → persistent fire tornado hazard".
  - `TriggerOn` = OnUse / OnHit / OnPerfect (future).
  - Enchant Weapon R10 "Elemental Burst — combo finisher hits (3rd light, charged heavy)".
- **Timing a HUD could show:**
  - the combo step ("Hit N")
  - the combo window (800 ms)
  - the heavy-charge threshold (0.4 s)
  - rooted state (`Mordecai.State.Rooted`)
  - the attacking state (`Mordecai.State.Attacking`)
- **Implemented HUD:** `UMordecaiComboCounterWidget` shows "**Hit %d**" (ComboIndex+1), gold (1, 0.84, 0), font 24, bottom-centre 200×40 at (0,−120).
  - It is visible only while `State.ComboWindow` is present, which never happens in the game (see above).

---

## Defense (block, parry, dodge, i-frames, posture break, stagger)

**Precedence for an incoming melee hit** (`MordecaiHitDefense.cpp`, decision log 2026-10-30, `EMordecaiHitDefense`):
- **Evaded:** i-frames (`State.Invulnerable` or `State.Dodging`). Nothing lands.
- **Parried:** an active parry window takes the hit. Nothing lands.
- **Blocked** or **PerfectBlocked:** an active guard mitigates the hit.
- **None:** the hit lands in full.
- Jump-avoid (`State.Airborne` + `JumpAvoidable`) is decided earlier, in the hit query.
- "I-frames and a successful parry negate the whole hit: health, posture, the enchant bonus, on-hit payloads, status effects and the damage number."
- Open: whether dodge or Blink protect against projectiles, spells and DoT ticks.

**Dodge** (`MordecaiGA_Dodge.h`), Left Alt / Gamepad B:
- Distance 400 cm over 400 ms. **Measured only about 110 cm (N27).**
- Stamina 15.
- Perfect-dodge window: the first **150 ms** (placeholder, `TODO(DECISION)`). The refund is **10** stamina (placeholder).
- Cooldown **300 ms** (tag `State.DodgeCooldown`).
- Direction is the movement input, or backward when there is none. The direction is locked once the dodge starts.
- Blocked during Dodging, DodgeCooldown, PostureBroken, KnockedDown, ParryWhiff and Dead.
- Spec: "perfect dodge bonus (stamina refund or slow‑motion window)".
- Attack taxonomy: "wide arcs may clip if dodge ends inside the active arc (intended difficulty)".

**Block** (`MordecaiGA_Block.h`), RMB / LT held (analog actuation 0.5):
- Mitigation 0.5. Stability 0.3, so the stamina drain is damage × 0.7.
- Move speed × 0.5 while blocking.
- Perfect-block window **250 ms** from raising the guard: no stamina drain, and 30 posture to the attacker. A perfect block still halves the health damage.
- **Guard break** when stamina ≤ 0 while blocking: `State.GuardBroken` for **800 ms**. Hits during a guard break land in full.
- Block covers every direction (open: shields in `skill_sheet_v1.1` say "frontal").
- Phases: None / Active / Guard Broken.
- Measured against a 15-damage enemy swing (US-101 run):

| Defense | Health lost | Stamina lost |
|---|---|---|
| Undefended | 15 | 0 |
| Block | 7.5 | 10.5 |
| Perfect block | 7.5 | 0 |
| Dodge | 0 | – |
| Jump | 0 | – |
| Parry | 0 | – |

**Parry** (`MordecaiGA_Parry.h`): LMB / RT pressed fresh while the block is held ("Timed click while blocking; high risk/high reward"). It drops the guard.
- Window **500 ms**. Posture to the attacker **50**. Stamina **20**.
- Whiff penalty: **600 ms** lock (`State.ParryWhiff`). Design: "animation-locked (cannot activate any abilities) for ~600ms … The punishment is being a sitting duck, not taking bonus damage."
- Success adds `State.Riposte` (punish window) to the player and `State.Parried` (stagger) to the attacker.
  - **N33:** neither tag is ever removed, and no spec gives the stagger or punish-window durations.
- Design log: "Standard parry window: 500ms; Perfect parry/block window: 250ms — Perfect window is entirely inside the standard window." The code has no separate perfect-parry state.
- Phases: None / Active / Whiff.

**Posture break, stagger and knock-down:** see Resources. Spec: break opens a "critical window" in which the player "can perform riposte". No dedicated riposte attack exists in code.

**Blink:** grants `State.Invulnerable` for `DA_Spell_Blink.BuffDuration` **0.3 s** through a duration GameplayEffect (N19 fixed).

**State tags a HUD or feedback layer can read** (`MordecaiGameplayTags.h`):
- Airborne, Attacking, ComboWindow, Rooted
- Dodging, PerfectDodge, DodgeCooldown
- PostureBroken, KnockedDown
- Blocking, PerfectBlock, GuardBroken
- Parrying, Parried, Riposte, ParryWhiff
- Sprinting, StaminaRegenPaused
- Casting, CastingSlow, CastingRooted
- Invulnerable, Dead

---

## Spells and abilities (slots, which keys, cooldowns, charges, costs, every spell named with its damage type)

**Ability types** (`ability_system_v1.md`): "Weapon Techniques, Spells, Mobility abilities, Passive feats."

**Ability schema** (`ability_schema_v1.md`):
- Types: WeaponTechnique, Spell, Mobility, Defensive, Utility, Summon, Passive, Triggered, ItemGranted, EnemyAbility.
- Activation: Active, Passive, Triggered, Channeled, Toggled.
- Movement policies: FreeMove, SlowWhileCasting, RootWhileCasting, CommittedLunge, AnimationDriven.
- Targeting: Self, MeleeArc, Cone, Circle, Line, Projectile, Dash, PersistentArea, SummonLocation, GlobalScripted.
- Effect phases: OnActivate, OnCommit, OnHit, OnBlock, OnParry, OnCrit, OnExpire, OnTick, OnEnd.
- Identity fields a menu or tooltip needs:
  - `AbilityId`, `DisplayName`, `AbilityType`, `Description` (required)
  - `ShortDescription`, `Icon` (e.g. `T_UI_Ability_Fireball`), `SortOrder`, `LoreText` (recommended)
- Cost and cooldown fields: Stamina, SpellPoints, Health; Duration, CooldownTag, **Charges**.
- Presentation: Animation, VFX, SFX, CameraShake (Light/Medium), **TelegraphType** (e.g. `CastGlow`, `WindupPose`).
- Design rule: every ability must answer "what does it cost? how often can it be used? how is it aimed? how does it scale? what effects does it apply? **how is it read by the player?**"
- Examples:
  - Fireball "Cooldown: 5s Cost: 3 SP Radius: 4m" (`ability_system_v1.md`).
  - Whirlwind: 25 stamina, CD 8.0, radius 3.25, 360°.

**Spell casting (code, `MordecaiGA_SpellBase`):**
- Phases: Windup → Cast → Recovery. The owned tag is `State.Casting`, with movement tags `State.CastingSlow` / `State.CastingRooted`.
- Damage interrupts a cast during Windup or Cast if the spell is interruptible. "SP is already consumed. Cooldown is NOT applied." Event tag: `Mordecai.Event.CastInterrupted`.
- Spells are blocked by `Status.Silenced`, PostureBroken and KnockedDown. They are not blocked by Dead or ParryWhiff (N26).
- SpellPower = BasePower × (1 + Σ coef × EffMod) × (1 + Rank × 0.05) × (1.5 when upcast). Rank runs 0–20.

**Slots and keys in the game.** Four direct slots: `DefaultSpellAbilities` and `MordecaiHeroComponent.cpp:200-218`; values from `Scripts/us057_01_create_spell_data_assets.py` / AC-057.1-4.

| Slot | KBM | Gamepad | Spell | SP | Cooldown | Cast / Windup / Recovery (s) | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Q | LB | **Fireball** | 3 | 2.0 s | 0.3 / 0.2 / 0.3 | Projectile, BP 25, range 1500, speed 2000, lifetime 3 s, radius 30, SlowWhileCasting, **Fire**, CanCrit true. N15: "spends SP but does nothing visible" |
| 2 | E | RB | **Blink** | 2 | 4.0 s | 0 / 0 / 0.2 | Dash teleport 500, 0.3 s i-frames, FreeMove, wall buffer 50 cm. N20: can spend SP and not move |
| 3 | R | none | **Stone Skin** | 4 | 15.0 s | 0.5 / 0.3 / 0.3 | Self, BP 50 → `DamageReduction`, 10 s, RootWhileCasting, buff `Status.StoneSkin` |
| 4 | 1 | D-pad Up | **Restoration** | 3 | 10.0 s | 0.4 / 0.2 / 0.2 | Self HoT, 40 total heal over 5 s, ticks every 1.0 s, SlowWhileCasting, `Status.Restoration` |

- **Cooldowns are not applied in the game:** no `DA_Spell_*` sets a `CooldownTag`, and the HUD cooldown widgets are never bound. Planned in US-088 and US-089.
- The HUD has only 2 cooldown widgets (N8). US-088 AC-088.3 plans 4, "labelled with the slot key Q / E / R / 1", in a row under the SP bar.
- Cooldown widget (implemented): light-blue (0.3, 0.6, 1.0) fill, remaining time "%.1f" (font 9). In the ready state the bar is full and the text is empty.
- Each slot is 120×16 px, at (20,124) and (150,124).

**Spec slot model (different):** `control_bindings_v1.1.md` defines **two** equipped skills.
- **Skill A** = RB tap / Q tap; **Skill B** = LB tap / E tap.
- **Select Skill A/B (Radial)** = hold RB / LB, or hold Q / E: "Option B: radial does NOT slow/pause time; selection is committed."
- v1 adds: "Mouse Wheel OR 1–8 while held."
- The input assets `IA_Mordecai_SkillA/SkillB` exist but are not mapped in the runtime IMC.

**Every spell named in the sources, with damage type** (from code where implemented, otherwise from the docs):
- **Fireball:** Fire (`Damage.Fire`, CanCrit). Skill sheet text (copy-paste) reads "Channelled spell. Fires {x}*{rank} magic projectiles … force dmg". R20 "Charged casts now create a much larger explosion".
- **Cone of Cold** (code `Ability.Spell.ConeOfCold`; "Frost Nova" in the proposal's existing list): **Frost**. Cone arc and range come from data. Skill sheet R10: "slowed … can now be briefly frozen".
- **Magic Missile** ("Magic Missle" in the skill sheet): **Arcane** ("Arcane/Force"), homing, CanCrit false. Missiles are channelled over the cast time; per-missile damage = power / count.
- **Stone Skin:** buff (flat DamageReduction). No damage type.
- **Sleep** ("Sleep (Wind/Gravity)" in the proposal): `Status.Sleeping` plus `State.Rooted`. Wakes on damage.
- **Blink:** mobility with i-frames. Proposal R20 "Rift Walk — Blink gains **2 charges**"; skill sheet R20 "stores a second charge".
- **Fire Ward:** a shield of `FireWardShieldHP` = BasePower that absorbs **only `Damage.Fire`** (`Status.FireWard`). The proposal's version absorbs "magic damage", R5 "Elemental Attune", R10 "Reflect Pulse", R20 "Aegis of Denial … full spell immunity burst (2s)".
- **Bless:** buff `Status.Blessed`. +BasePower/100 to PhysicalDamageMultiplier and MagicDamageMultiplier; 1 stack, refreshes.
- **Restoration** (renamed from "Healing Light"): heal over time. Proposal R5 "Cleanse", R10 "Sanctuary" ground heal zone, R20 "Renewal" revive to 30% HP.
- **Snare:** `Status.Rooted`; the target can still attack and block. Proposal R15 "Petrify" (upcast for extra SP), R20 "Living Prison".
- **Enfeeble:** `Status.Weakened`; PhysicalDamageMultiplier − BasePower/100. Proposal adds Corroded (R5), Exposed (R10), "Hex Mark" (R15), "Unravel" (R20).
- **Enchant Weapon:** `Status.EnchantedWeapon`; bonus **Fire** damage on melee hits. Proposal: "matches your highest element"; R15 "Adaptive Edge" lets the player choose the element; R20 "every 5th hit triggers Elemental Burst".
- **Illusion:** a summoned decoy (`Team.Ally`) whose HP comes from BasePower. Proposal R15 "Phantom Swap".
- **Blur:** `Status.Blurred`; `RangedEvasionChance` = BasePower/100. Proposal R10 "Flicker" invisibility, R20 "Untraceable".
- **Skill-sheet-only spells (not in code):** Frost Ward (absorbs frost), Float (hover after jump), Counter Spell ("quick beam … interrupting the casting").
- **DLC reserved:** **Hex** (Dark/Curse: Cursed, Fear, Soul Leech), **Venom** (Nature/Poison: Poison, Corrode, Plague), **Conjure** (Summoning: Familiars, Elemental Servants).
- **Ranks:**
  - Every spell in `new_spells_proposal.md` has a Rank 1–20 table. Ranks 1/5/10/15/20 are named unlocks; other ranks are "+Duration", "−SP cost", "−Cooldown" and similar.
  - `game_design_v2.md`: "Milestones: Rank 1 / 5 / 10 / 15 / 20 unlock techniques/passives/interactions."
  - Proposal category is "Magic" for all of them.

---

## Damage types (full list, any colors or icons implied, resistances/weakness feedback, crits)

**Canonical list** (`damage_types_v1.md`; `EMordecaiDamageType`; tags `Mordecai.Damage.*`):

| Type | Fantasy (spec) | Typical delivery | Common status (spec §3) | Damage-number colour (code, `MordecaiDamagePopComponent.cpp:27-38`) |
|---|---|---|---|---|
| **Physical** (subtypes **Slash**, **Pierce**, **Blunt**; tags `Damage.Physical.Slash/.Pierce/.Blunt`) | "direct bodily harm from force, cuts, punctures, and impacts" | melee, arrows, thrown, techniques | Slash → Bleeding | White (1, 1, 1) for all four |
| **Fire** | "heat, burning, combustion" | fire spells, ignited weapons, burning zones, explosives | Burning | Orange (1, 0.5, 0) |
| **Frost** | "cold, freezing force, chill, and brittle pressure" | ice spells, frost procs, freezing zones | Frostbitten (also slows, Brittle, synergy vs Drenched) | Cyan (0.3, 0.8, 1) |
| **Lightning** | "electricity, shock, and rapid burst energy" | chain attacks, strikes from above | Shocked (bonus vs Drenched or metal) | Yellow (1, 1, 0.3) |
| **Poison** | "toxins, venoms, rot" | clouds, coated weapons, darts, pools | Poisoned | Green (0.3, 0.9, 0.3) |
| **Corrosive / Acid** | "material erosion and armor-eating damage" | acid spit, alchemical bombs | Corroded (armor reduction) | Gray (0.7, 0.7, 0.7), commented "default" |
| **Arcane** | "raw magical force … non-elemental" | force bolts, beams, pulses | (Silence, Focused, wards) | Purple (0.7, 0.3, 1) |
| **Shadow / Curse** (spec tags `Damage.Shadow`, `Damage.Curse`; code has only Shadow) | "corruption, affliction, entropy, hexes" | curses, corrupted melee, boss attacks | Cursed (Weakened, Fear) | Dark Red (0.6, 0.1, 0.1) |
| **Holy / Radiant** (`Damage.Radiant`) | "sanctified, purifying, or light-based force" | consecrated strikes, anti-undead | (cleansing, anti-curse) | Gold (1, 0.85, 0) "Gold (Holy)" |

- Unknown or untagged damage: Gray (0.7, 0.7, 0.7). Healing: green (0.2, 1.0, 0.2) with a "+" prefix.
- AC-065.5 lists `Damage.Holy` and `Damage.Dark`; the code maps those colours to Radiant and Shadow.
- Spec design goal: "avoid redundant 'just recolored damage' categories"; "statuses must have distinct gameplay identity (not 'damage colors')".
- **Damage number behaviour (implemented, US-065):**
  - 1.5 s lifespan, rises 100 cm/s, scale 1.0, random scatter 30 cm, at most 20 at once (the oldest is recycled).
  - The number shown is "the health actually taken" (a blocked hit shows the mitigated amount).
  - The bridge sees no damage-pop events (B2).
- **Resistances** (§4 schema, e.g. Physical 0.10, Fire −0.20, Frost 0.25, Poison 0.50): "positive values reduce damage taken; negative values indicate vulnerability; `0.25` means 25% mitigation; `-0.20` means 20% extra damage taken." No UI is specified for weakness or resist feedback. Immunities exist as `Mordecai.Immunity.*` tags per status.
- **Delivery values:** MeleeHit, Projectile, Beam, Explosion, PersistentArea, GroundHazard, Chain, Pulse, Aura, ScriptedEvent.
- **Per-type counterplay (§6), condensed:**
  - Physical: armor, block, parry, posture, mobility
  - Fire: resistance, move out of hazards, anti-burn, water/drenched
  - Frost: resistance, mobility timing, cleanse, anti-slow
  - Lightning: spacing, anti-chain, avoid conductive setups
  - Poison: cleanse, attrition mitigation
  - Corrosive: fast kill, avoid pools
  - Arcane: wards, interrupts
  - Shadow: cleanse, radiant tools
  - Radiant: avoid line/channel attacks
- **Formulas** (`combat_system_v1.md`):
  - "Damage = BaseDamage × SkillModifier × AttributeScaling × CriticalModifier × StatusModifier"
  - "FinalDamage = IncomingDamage × (1 − Mitigation)" (armor, buffs, resistances)
- **Crits:**
  - "CriticalChance = WeaponCrit + SkillBonus + Buffs"; "Default multiplier: **1.5x**".
  - Code: PhysicalCritChance = 0.0075 × EffDex; MagicCritChance = 0.0075 × EffCha; `CanCrit` per damage profile; damage pop `CriticalHitScale` 1.5×.
  - **No crit roll exists:** every damage pop is fired with `bIsCritical=false`. AC-065.7 suggests a crit "bright outline/glow effect, or simply use larger scale".
- **Vulnerability multipliers** a HUD might show on an enemy: staggered ×1.5, knocked down ×2.0.

---

## Controls (full keyboard+mouse and gamepad map from control_bindings_v1.1; which actions need on-screen button prompts; menu navigation keys)

**control_bindings_v1.1.md (authoritative spec), verbatim actions:**

| Action | Category | Controller | MKB | Notes (spec) | In the game today (`MordecaiHeroComponent.cpp`) |
|---|---|---|---|---|---|
| Move | Movement | Left Stick | W/A/S/D | 360° analog vs digital. Movement always available. | WASD works. The stick key name `Gamepad_LeftStick2D` is invalid, so gamepad movement is dead (US-097) |
| Sprint (Toggle) | Movement | L3 (click) | Left Shift (toggle) | may increase stamina drain/noise | Shift works; L3 name invalid |
| Look / Aim Direction | Camera/Aim | Right Stick | Mouse movement | "No target lock; aim sets facing and attack direction." | Mouse2D bound, but facing is pinned to +X (US-098, blocked on a decision) |
| Sneak (Toggle) | Stealth | R3 (click) | Ctrl (v1: **C**) | Reduces detection + noise; reduces move speed | Not bound (decided out of scope) |
| Light Attack | Combat | RT (tap) | Left Mouse (tap) | Primary attack. | Bound (tap < 0.4 s) |
| Heavy / Charged Attack | Combat | RT (hold) | Left Mouse (hold) | Charge time depends on weapon. | Bound (≥ 0.4 s, fires on release) |
| Block | Combat | LT (hold) | Right Mouse (hold) | drains stamina; stability scales | Bound |
| Parry | Combat | LT + RT | Right Mouse + Left Mouse (timed) | Timed click while blocking | Bound (chord) |
| Use Skill A | Skills | RB (tap) | Q (tap) | Executes equipped Skill A. | Q = Spell 1 (Fireball), but on **LB** |
| Select Skill A (Radial) | Skills | RB (hold) | Q (hold) | radial does NOT slow/pause time; selection is committed | Not implemented |
| Use Skill B | Skills | LB (tap) | E (tap) | Executes equipped Skill B. | E = Spell 2 (Blink), but on **RB** |
| Select Skill B (Radial) | Skills | LB (hold) | E (hold) | same | Not implemented |
| Jump | Movement | A | Space | Traversal + combat spacing. | Bound |
| Dodge | Combat | B | Left Alt | Always possible; effectiveness depends on stamina tier. | Bound |
| Interact | World | X | F | Talk, loot, activate, climb prompts. | Bound (pickup) |
| Throw (equipped throwable) | Combat/Utility | Y (tap) | G (tap) (v1: Middle Click optional) | knife/bomb/vial | Not bound (`IA_Mordecai_Throw` exists) |
| Weapon Set Swap (Set 1 ↔ Set 2) | Combat/Loadout | Y (hold) | Tab | hold on controller to avoid accidental swaps | Tab / **Y tap** = next of 5 weapons; BackSlash = previous |
| Use Item A | Items | D-pad Down | 1 | Quickslot item use. | Not bound; **1 is Spell 4** |
| Use Item B | Items | D-pad Left | 2 | Quickslot | Not bound |
| Use Item C | Items | D-pad Up | 3 | Quickslot | Not bound; **D-pad Up is Spell 4** |
| Use Item D | Items | D-pad Right | 4 | Quickslot | Not bound |
| Pause / Menu | System | Start/Menu | Esc | Inventory, character sheet, settings. | Bound (Esc / Start) |
| Map / Objective | System | View/Select | M | Metroidvania navigation support. | Not bound (`IA_Mordecai_Map` exists) |

- **Extra binding in the game:** **I** opens the pause menu on the Inventory tab, and closes the open menu on key-up. It is not yet in the bindings doc (`TODO(DECISION)`).
- **Spell 3 (R)** has no gamepad button. AC-057.5 specified LB+RB, which is not mapped.
- Open `TODO(DECISION)` (US-097): which mapping is authoritative, AC-057.5 (LB = Q) or the bindings doc (RB = Q)?

**Ground-targeting controls (spec conflict):**
- `attack_taxonomy_v1.md` §6.2: "Controller: press skill → enter reticle mode; move reticle with Right Stick; press A to cast, press B to cancel. KBM: press skill → reticle follows mouse; left-click to cast, right-click to cancel."
- `player_attacks_agent_brief_v1.md` §6.2: "hold skill → reticle mode … release to cast." Optional "dodge to cancel".
- Not implemented.

**Menu navigation (implemented, CommonUI, `Config/DefaultInput.ini`):**
- `UI.Action.Escape` "Back" = Escape / Gamepad Start (Special_Right). Gamepad B is also CommonUI Back (code comment).
- `UI.Action.Mordecai.PauseMenu.PreviousTab` "Previous Tab" = **LB**; `.NextTab` "Next Tab" = **RB**. Both wrap Character ↔ Settings.
- `.Inventory` "Close" = **I**.
- There is no keyboard tab switching; "the mouse switches tabs by clicking the tab headers".
- Pause-menu tabs: Character, Skills, Feats, Inventory, Settings (default Character). Skills, Feats and Settings show "Coming Soon" (N5).
- Menus run in CommonUI Menu input mode, which blocks game input, so LB/RB do not also cast spells.

**On-screen button prompts:**
- **Implemented:** "[F] Pick up {item}" and "[I] Inventory" (`MordecaiPickupPromptWidget.cpp`). Decision log: "Device-aware key glyphs (controller glyphs when on gamepad) deferred — prompts show keyboard keys for the slice."
- **Planned:** cooldown widgets labelled Q / E / R / 1 (US-088).
- **Implied by the specs:**
  - Interact prompts: "Talk, loot, activate, climb prompts"
  - skill slot key glyphs (Skill A/B, RB/LB, Q/E)
  - the radial selection hint ("Mouse Wheel OR 1–8 while held")
  - ground-target cast/cancel (A/B, LMB/RMB)
  - weapon set swap (Tab / hold Y)
  - quickslot items (1–4 / D-pad)
  - throwable (G / Y tap)
  - a riposte prompt in the critical window
  - pause-menu LB/RB tab glyphs and Back (Esc/B) in menus

---

## Combat feedback the player must read (hit, crit, block, parry, posture break, low health, out of stamina, interrupts)

**Implemented, `UMordecaiCombatFeedbackWidget`:**
- Bottom-centre, 300×30 at (0,−80), font 20, shown for 1.0 s.
- Gold (1, 0.84, 0) for positive feedback, red (1, 0.2, 0.2) for negative:

| Text | Colour | Trigger tag |
|---|---|---|
| "Perfect Dodge!" | gold | `State.PerfectDodge` |
| "Perfect Block!" | gold | `State.PerfectBlock` |
| "Parried!" | gold | `State.Parried` |
| "Parry Failed!" | **red** | `State.ParryWhiff` |
| "Posture Broken!" | gold | `State.PostureBroken` |

- **How it fires in the game:**
  - The PerfectDodge and PerfectBlock tags are added when the window **opens**, at the start of every dodge and every block raise. The flash therefore follows the input, not success.
  - "Parried!" listens on the player's ASC, but the Parried tag goes on the attacker (N33), so it never shows.
  - "Posture Broken!" listens on the player's ASC, but only enemies break posture.

**Other implemented feedback:**
- **Damage numbers:** coloured by type; healing "+" green; crit scale 1.5× (crits never flagged).
- **Combo "Hit N":** gold, never shown (see Attacks).
- **Kill counter:** "Kills: %d", top-right, font 14. N1: it never increments.
- **Status indicators:** bottom-left 400×40. Each shows name + "%.1f" countdown, green tint for buffs and red for debuffs (BuffTags e.g. Focused, Blessed, StoneSkin). N16: shows "Status" instead of the name.
- **Stamina bar:** tier colours; Exhausted (0.5, 0, 0).
- **Posture bar:** turns red while broken.
- **Death screen:** "You Died".
- **Enemy indicator:** health bar green (0, 0.8, 0) above 50%, yellow (1, 0.85, 0) from 25%, red (0.9, 0.1, 0.1) below 25%, plus status icons. N17: never appears in the arena.

**Needed but with no UI yet (implied):**
- hit confirm and "hit stop", camera shake (schema: CameraShake Light/Medium)
- **guard broken** (800 ms stagger), **out of stamina / exhausted** (−50 floor), **low health**
- **cast interrupted** (`Event.CastInterrupted`; SP lost), **not enough SP** (N20: Blink spent SP "with no feedback")
- **spell on cooldown / ready**, **riposte / critical window open** on a broken enemy, **knocked down**
- **enemy posture meter**
- **telegraphs** (TelegraphType CastGlow / WindupPose)
- **weakness / resistance hit** (positive or negative resistances)
- **immune**, **evaded** (i-frames negate the hit and its damage number)
- **Fire Ward absorbed / shield broken**, **perfect action** (`Event.PerfectAction` exists for perfect dodge, block and parry)

---

## Terminology

**Resources and states:**
- Health
- Stamina, with tiers Green / Yellow / Red / **Exhausted**
- Posture
- Spell Points (**SP**)
- **Posture Broken** (= stagger)
- **Knocked Down**
- **Critical window**
- **Riposte**
- **Guard Broken**
- **Stability** (block stamina-drain reduction; "Player Stability" = stagger resistance)
- **Mitigation**

**Defense:**
- Dodge, **Perfect Dodge**, **i-frames**
- Block, **Perfect Block**
- Parry, Parried, **Parry Failed / whiff**

**Attacks:**
- Light Attack
- Heavy / Charged Attack
- **combo** / **combo window** / **combo finisher**
- **Sweep** / **Thrust** / **Slam**
- **JumpAvoidable** / **HitsAirborne**
- **Windup / Active / Recovery**
- **Rooted**
- **soft aim assist** ("no lock-on")
- **Self-origin / Attached aura / Ground-targeted**
- **reticle**

**Spells:**
- Spell, cast, **upcast**, **cooldown**, **charges**
- **rank** (1–20; milestones 1/5/10/15/20)
- School
- **Skill A / Skill B**, **radial**
- **Weapon Set 1 ↔ Set 2**
- **quickslot** Item A–D
- **throwable**

**Damage:**
- Damage types Physical (Slash/Pierce/Blunt), Fire, Frost, Lightning, Poison, Corrosive, Arcane, Shadow (Curse), Radiant (Holy)
- **resistances** / **vulnerability**
- **crit** (1.5×)

**Status examples:** Burning, Bleeding, Poisoned, Frostbitten, Shocked, Weakened, Brittle, Silenced, Rooted, Blinded, Fear, Cursed, Exposed, Corroded, Drenched, Focused (buff), Frozen, MicroStunned, Sleeping, StoneSkin, FireWard, Blessed, Restoration, EnchantedWeapon, Blurred, Resolved.

**Source files cited:**
- `Specs/*.md` listed above
- `Source/LyraGame/Mordecai/Combat/*`, `Magic/*`, `UI/*`, `Weapons/MordecaiWeaponProfileFactory.cpp`, `MordecaiHeroComponent.cpp`, `MordecaiPlayerController.cpp`, `AbilitySystem/MordecaiAttributeSet.cpp`
- `Config/DefaultInput.ini`
- `Scripts/us057_01_create_spell_data_assets.py`
- `Specs/design_decisions_log.md`, `Specs/agent_playtest_plan_v1.md`
- `stories/backlog/US-088, US-090, US-091, US-092, US-097, US-098`
- `stories/done/US-052, US-055, US-057, US-061, US-065, US-083`

## UI INVENTORY
- [HUD|implemented] **Health bar**: Always-visible survivability readout | data: Health/MaxHealth fill 0-1 (default 100/100; MaxHealth = Base x (1+0.06xEffCon)); spec wants Health shown as % (0-100); 250x20 px top-left at (20,20), green fill | states: full, damaged, empty/0 (death); no % text drawn today (Source/LyraGame/Mordecai/UI/MordecaiHealthBarWidget.cpp; MordecaiCombatHUDWidget.cpp:57-61; game_design_v2.md §2 pillar 8, §9; AC-052.2)
- [HUD|specified] **Health percentage readout**: Spec requires Health displayed as a percentage, not just a bar | data: Health % 0-100 | states: normal, low health (game_design_v2.md §4 'Health: Displayed as % (0–100)', §9 'Always visible: Health%'; combat_system_v1.md 'Health (0--100%)')
- [HUD|implemented] **Stamina bar with tier colour**: Show stamina fill and effectiveness tier | data: Stamina/MaxStamina fill (default 100; floor -50); tier Green >=66% (1.25x), Yellow >=33% (1.0x), Red >0% (0.75x), Exhausted <=0 (0.5x); colours (0,1,0)/(1,1,0)/(1,0,0)/(0.5,0,0); 250x20 at (20,48) | states: Green, Yellow, Red, Exhausted (negative value reads empty), regen paused (1000 ms delay), sprint draining (MordecaiStaminaBarWidget.h/.cpp; MordecaiStaminaSystem.h; design_decisions_log.md 2026-03-19; N30 in agent_playtest_plan_v1.md (no regen in game))
- [HUD|implemented] **Player posture bar**: Show the player's posture meter | data: Posture/MaxPosture fill (default 100); normal blue (0.2,0.6,1), broken red (1,0.2,0.2); 250x16 at (20,76) | states: normal, depleting, Posture Broken (red) (MordecaiPostureBarWidget.cpp/.h; AC-052.4)
- [HUD|implemented] **Spell Points bar**: Show discrete SP available for spells | data: SP fill + text 'cur / max' (e.g. '10 / 10'); purple (0.4,0.2,0.8); font 10; 250x16 at (20,100) | states: full, partial, empty/insufficient for next spell; regen (spec only) (MordecaiSpellPointsBarWidget.cpp; AC-055.3; N6)
- [HUD|implemented] **Spell cooldown indicators (per slot)**: Show cooldown remaining / ready per spell slot | data: fill % of cooldown elapsed, remaining seconds '%.1f', light blue (0.3,0.6,1); today 2 widgets 120x16 at (20,124),(150,124); planned 4 labelled Q/E/R/1; durations Fireball 2.0, Blink 4.0, Stone Skin 15.0, Restoration 10.0 s | states: ready (full bar, empty text), on cooldown (counting), unbound (always ready today) (MordecaiSpellCooldownWidget.cpp; MordecaiCombatHUDWidget.cpp:81-92; stories/backlog/US-088 AC-088.3; N8)
- [HUD|implied] **Spell loadout slots (icon, name, key, SP cost)**: Show which spell is on Q/LB, E/RB, R, 1/DPadUp and whether it is castable | data: slot 1 Fireball 3 SP, slot 2 Blink 2 SP, slot 3 Stone Skin 4 SP, slot 4 Restoration 3 SP; ability Icon (e.g. T_UI_Ability_Fireball), DisplayName, key glyph | states: ready, on cooldown, not enough SP, casting, silenced/blocked, charges (MordecaiPlayerController.cpp:54-57,193-197; MordecaiHeroComponent.cpp:200-218; ability_schema_v1.md Identity (Icon))
- [HUD|specified] **Skill A / Skill B equipped indicators**: Show the two equipped skills per the bindings spec | data: Skill A (RB/Q), Skill B (LB/E), equipped skill name/icon | states: ready, cooldown, unaffordable (control_bindings_v1.1.md rows Use Skill A/B)
- [Notification / overlay|specified] **Skill selection radial**: Choose which skill sits in Skill A/B while holding the button | data: radial options (v1: select with Mouse Wheel or 1–8 while held) | states: open (time NOT slowed or paused), highlighted option, committed selection (control_bindings_v1.1.md 'Option B: radial does NOT slow/pause time; selection is committed'; control_bindings_v1.md KBM Notes)
- [HUD|implemented] **Combo counter**: Show current light-combo step | data: 'Hit N' (ComboIndex+1), gold (1,0.84,0), font 24, bottom-centre 200x40 at (0,-120) | states: hidden, visible during combo window (800 ms); never shows today (ComboWindow tag never added; N24 combos never chain) (MordecaiComboCounterWidget.cpp; AC-061.1; MordecaiGA_MeleeAttack.h ComboWindowTimeoutMs)
- [Notification / overlay|implemented] **Combat feedback text flash**: Brief callouts for defensive outcomes | data: 'Perfect Dodge!', 'Perfect Block!', 'Parried!', 'Posture Broken!' gold (1,0.84,0); 'Parry Failed!' red (1,0.2,0.2); font 20; 1.0 s; bottom-centre 300x30 at (0,-80) | states: hidden, showing (1 s). Today Perfect Dodge/Block fire when the window opens, Parried never shows (N33), Posture Broken never shows on the player (MordecaiCombatFeedbackWidget.cpp; AC-061.2)
- [World-space|implemented] **Floating damage numbers**: Show damage dealt/taken and healing at the hit point | data: integer amount; colour by damage type (Physical white, Fire orange, Frost cyan, Lightning yellow, Poison green, Arcane purple, Radiant gold, Shadow dark red, Corrosive/unknown gray); healing green (0.2,1,0.2) with '+'; 1.5 s life, rise 100 cm/s, scatter 30 cm, max 20 | states: normal, critical (1.5x scale; never flagged today), healing, blocked (mitigated amount), evaded/parried (no number) (MordecaiDamagePopComponent.h/.cpp; US-065 AC-065.5-065.10)
- [HUD|implemented] **Status effect bar and indicators**: Show active buffs/debuffs on the player | data: status name (last tag segment) font 10 + remaining '%.1f' font 9; green tint buff, red tint debuff; bottom-left 400x40 at (20,-60) | states: empty, one or more indicators, expiring; bug N16 reads 'Status' (MordecaiStatusEffectIndicatorWidget.cpp; MordecaiStatusEffectBarWidget.cpp; AC-055.4-055.7; US-091)
- [HUD|implemented] **Kill counter**: Count enemy kills this life | data: 'Kills: %d', white, font 14, top-right 150x24 | states: counting, reset on player death; never increments today (N1) (MordecaiKillCounterWidget.cpp; US-063; N1)
- [HUD|planned-story] **Equipped weapon display**: Show which weapon is active and update on swap | data: weapon display name (Longsword, Dagger, Greatsword, Spear, Mace in arena loadout) | states: current weapon, changed (after Tab/Y); draws nothing today (N3) (MordecaiEquippedWeaponWidget.cpp; MordecaiCharacter.cpp:57-61,193-206; stories/backlog/US-091 AC-091.3/091.4; AC-078.11 (bottom-center or bottom-right))
- [HUD|specified] **Weapon set indicator (Set 1 / Set 2)**: Show the active weapon set for the Set 1 ↔ Set 2 swap | data: Set 1 / Set 2 and their weapons | states: Set 1 active, Set 2 active, swapping (controller hold Y) (control_bindings_v1.1.md 'Weapon Set Swap (Set 1 ↔ Set 2)')
- [HUD|specified] **Quickslot items A–D**: Show consumables mapped to 1-4 / D-pad | data: Item A (D-pad Down/1), B (Left/2), C (Up/3), D (Right/4), item icon and count | states: empty slot, stocked, used/cooldown (control_bindings_v1.1.md Use Item A-D; IA_Mordecai_ItemA-D assets exist unmapped)
- [HUD|specified] **Throwable selector**: Show the currently selected throwable | data: knife / bomb / vial, count | states: selected, none equipped (control_bindings_v1.1.md Throw 'Throws selected throwable (knife/bomb/vial)')
- [HUD|implied] **Heavy attack charge indicator**: Show the hold-to-charge heavy threshold | data: charge progress to 0.4 s threshold (spec: charge time depends on weapon); bow 'hold to charge' | states: charging, charged (fires on release), cancelled (MordecaiHeroComponent.h HeavyAttackHoldSeconds 0.4 TODO(DECISION); control_bindings_v1.1.md Heavy/Charged; attack_taxonomy_v1.md Longbow)
- [HUD|implied] **Spell cast bar / casting state**: Show windup/cast progress and interruptibility | data: phases Windup -> Cast -> Recovery (e.g. Fireball 0.2/0.3/0.3 s); movement policy slow/rooted | states: winding up, casting, recovering, interrupted (SP lost, no cooldown) (MordecaiGA_SpellBase.h/.cpp; ability_schema_v1.md Cast; Event.CastInterrupted tag)
- [World-space|specified] **Ground-target reticle**: Aim ground-targeted AoE spells while rooted | data: reticle shape and range; cast/cancel prompts | states: entering targeting, aiming, cast committed, cancelled (attack_taxonomy_v1.md §6.2 (A cast / B cancel, LMB/RMB); player_attacks_agent_brief_v1.md §6.2 (release to cast))
- [World-space|planned-story] **Facing / aim indicator**: Show the aim direction attacks and spells go (no lock-on) | data: facing arrow from mouse or right stick | states: aiming, idle; today pinned to +X and arrow hidden (N12) (stories/backlog/US-098 AC-098.6; control_bindings_v1.1.md Look/Aim)
- [World-space|implemented] **Enemy health bar / indicator**: Show enemy health and statuses above head | data: name, health bar green >50% (0,0.8,0), yellow >=25% (1,0.85,0), red <25% (0.9,0.1,0.1), status icons | states: full, wounded, critical, dead; indicator missing in arena (N17) (MordecaiEnemyIndicatorWidget.cpp:28-30,247-261; MordecaiEnemyHealthBarWidget; US-092)
- [World-space|implied] **Enemy posture meter**: Show build-up toward posture break / critical window | data: enemy Posture/MaxPosture (break at 0; regen 10/s after 3 s) | states: building, broken (1 s stagger, x1.5 dmg), knocked down (2 s, x2.0), regenerating (combat_system_v1.md 'Enemies have posture meters'; MordecaiPostureSystem.h; no enemy UI shows posture)
- [HUD|implied] **Boss posture bar**: Show a boss's posture | data: boss posture fill | states: normal, chunked by spell, broken (new_spells_proposal.md Enfeeble R20 'boss posture bar takes a chunk on cast', Snare R20)
- [Notification / overlay|implied] **Riposte / critical window prompt**: Tell the player a broken or parried enemy can be punished | data: riposte opportunity on target | states: open, expired (durations unspecified, N33) (combat_system_v1.md Posture System; State.Riposte tag; game_design_v2.md 'punish window')
- [Notification / overlay|implied] **Guard broken feedback**: Show that the block broke at 0 stamina | data: guard broken state (800 ms) | states: guard broken, recovered (MordecaiGA_Block.h GuardBrokenDurationMs; State.GuardBroken)
- [Notification / overlay|implied] **Exhausted / low stamina warning**: Show that actions are at 0.5x effectiveness | data: Exhausted state, negative stamina | states: red tier, exhausted (MordecaiStaminaSystem.h ExhaustedMultiplier 0.5, StaminaFloor -50)
- [Notification / overlay|implied] **Low health warning**: Show danger before death | data: health % threshold | states: low, critical (combat_system_v1.md Health; game_design_v2.md §9)
- [Notification / overlay|implied] **Not enough SP / cast refused / interrupted feedback**: Explain why a spell did not go off | data: insufficient SP, on cooldown, silenced, interrupted | states: refused, interrupted (MordecaiGA_SpellBase.cpp CheckSpellPointCost, OnDamageTakenCallback; N20 'no feedback')
- [HUD|implied] **Spell charges pips**: Show multi-charge abilities | data: charges remaining (schema Cooldown.Charges; Blink R20 '2 charges') | states: full, partial, recharging (ability_schema_v1.md Costs and Cooldowns; new_spells_proposal.md Blink R20; skill_sheet_v1.1.md Blink R20)
- [HUD|implied] **Shield / ward readout**: Show Fire Ward absorb HP and Stone Skin reduction | data: FireWardShieldHP (= BasePower, absorbs only Damage.Fire), DamageReduction (Stone Skin BP 50, 10 s) | states: active, depleting, broken/expired (MordecaiGA_FireWard.cpp; MordecaiGA_StoneSkin.cpp; MordecaiAttributeSet.cpp:54-57)
- [HUD|implied] **Sprint / Sneak toggle state**: Show toggled movement modes | data: Sprinting (Shift/L3 toggle), Sneaking (Ctrl/R3 toggle) | states: on, off (control_bindings_v1.1.md Sprint (Toggle), Sneak (Toggle); State.Sprinting tag)
- [World-space|specified] **Enemy attack telegraphs**: Readable windups and shapes from the fixed camera | data: TelegraphType (CastGlow, WindupPose); sweep arc vs thrust lane vs slam circle; jump-avoidable vs hits-airborne | states: windup, active, recovery (ability_schema_v1.md Presentation; attack_taxonomy_v1.md §1.1; 'Targeting must remain readable from the fixed diorama perspective')
- [World-space|implied] **Weakness / resistance / immune hit feedback**: Show that a hit was resisted, a weakness hit, or immune | data: resistance value sign (positive mitigation, negative vulnerability), Immunity.* tags | states: weak hit, resisted, immune, evaded (damage_types_v1.md §4 Resistance Schema; MordecaiGameplayTags.h Immunity_*)
- [HUD|implemented] **Interact / pickup prompt**: Prompt for the interact key on pickups and other world interactions | data: '[F] Pick up {item}', '[I] Inventory'; spec also lists Talk, loot, activate, climb | states: no target, focused target; keyboard glyphs only (device-aware glyphs deferred) (MordecaiPickupPromptWidget.cpp:22,27; design_decisions_log.md 2026-07-10; control_bindings_v1.1.md Interact)
- [Notification / overlay|implemented] **Death screen**: Player death and respawn countdown | data: 'You Died' (48, red 0.8,0.1,0.1), countdown seconds (32), 'Kills: %d' (20), black dim 0.5 | states: dead, counting down, respawn (MordecaiDeathScreenWidget.cpp; US-086)
- [In-game menu|implemented] **Pause menu tab bar with shoulder navigation**: Navigate Character / Skills / Feats / Inventory / Settings | data: tabs Character, Skills, Feats, Inventory, Settings; LB previous / RB next (wraps), I closes, Esc/Start/B back, mouse click headers | states: open (game paused, game input blocked), active tab, Coming Soon tabs (MordecaiPauseMenuTypes.cpp:15-21; Config/DefaultInput.ini:39-43; design_decisions_log.md 2026-10-30)
- [In-game menu|specified] **Spell / ability detail card**: Explain an ability: cost, cooldown, aim, scaling, effects | data: DisplayName, Description/ShortDescription, Icon, LoreText, School, cost (SP/Stamina/Health), cooldown, charges, cast time, targeting type/range/radius, damage type, scaling stats | states: locked, learned, equipped, upcastable (ability_schema_v1.md §3 and §7 design notes)
- [In-game menu|specified] **Spell rank progression (1–20)**: Show rank unlocks per spell | data: rank tables with named milestones at 5/10/15/20 (e.g. Blink R5 Blink Strike, R10 Phase Step, R15 Recall Anchor, R20 Rift Walk) | states: rank achieved, next milestone, locked (new_spells_proposal.md; skill_sheet_v1.1.md Magic rows; game_design_v2.md §5)
- [In-game menu|implied] **Weapon / attack profile info**: Help the player read a weapon's identity | data: damage type (Slash/Pierce/Blunt), light combo count (e.g. Dagger 5, Longsword 3, Greatsword 2), heavy type (spin, slam, lunge), base damage, speed multiplier, range, posture bonus, stamina costs | states: equipped, in loadout (MordecaiWeaponProfileFactory.cpp; attack_taxonomy_v1.md §4)
- [In-game menu|implied] **Controls / key bindings screen**: Show the KBM and gamepad map; Settings tab | data: full binding table from control_bindings_v1.1 plus I (inventory) and in-menu LB/RB | states: keyboard view, gamepad view; Settings shows 'Coming Soon' today (control_bindings_v1.1.md; agent_playtest_plan_v1.md 'Settings tab also shows Coming Soon')
- [Other|specified] **Device-aware button glyph system**: Show controller glyphs when on gamepad, keys on KBM | data: key/button icons for every prompt | states: KBM, gamepad (design_decisions_log.md 2026-07-10 'Device-aware key glyphs … deferred')
- [In-game menu|specified] **Map / Objective screen**: Metroidvania navigation support | data: map, objectives | states: open/closed (control_bindings_v1.1.md Map / Objective (View/Select, M); IA_Mordecai_Map exists unmapped)
- [In-game menu|implied] **Damage type and status legend**: Teach the colour and type language | data: 12 damage types with colours, status relationships (Fire→Burning, Frost→Frostbitten, Lightning→Shocked, Poison→Poisoned, Corrosive→Corroded, Shadow→Cursed, Slash→Bleeding) | states: n/a (damage_types_v1.md §2-§3; MordecaiDamagePopComponent.cpp:27-38)

## KEY TERMS
- Health: survivability resource, spec says shown as % 0–100; 0 = death ('You Died')
- Stamina: tiered resource spent by attacks, dodge, block, sprint; can go to -50
- Stamina tiers: Green (>66%, 1.25x), Yellow (33–66%, 1.0x), Red (<33%, 0.75x), Exhausted (<=0, 0.5x)
- Exhausted: stamina at or below 0; actions still allowed at 50% effectiveness
- Spell Points (SP): discrete spell resource, default 10; upcast doubles cost
- Posture: meter worn down by heavy hits, perfect defense and certain spells; enemies break at 0
- Posture Broken: stagger state on break, 1 s, takes 1.5x damage
- Knocked Down: escalation if >=50 damage lands during a stagger, 2 s, 2.0x damage
- Critical window: the punish window opened by a posture break
- Riposte: the punish attack/window after a parry or break (State.Riposte)
- Dodge: Left Alt / B; i-frames; 15 stamina; 300 ms cooldown
- Perfect Dodge: first 150 ms of a dodge; refunds 10 stamina (placeholder)
- i-frames: invulnerability frames (State.Dodging / State.Invulnerable) that negate a hit
- Block: RMB / LT held; 50% mitigation; drains stamina; move speed x0.5
- Perfect Block: hit within 250 ms of raising the guard; no stamina drain, 30 posture to attacker
- Guard Broken: block collapses at 0 stamina; 800 ms stagger
- Parry: LMB/RT pressed while blocking; 500 ms window; 50 posture to attacker; 20 stamina
- Parry Failed / whiff: missed parry; ~600 ms lock, 'sitting duck', no bonus damage
- Stability: block stamina-drain reduction (0.3); also 'Player Stability' = stagger/interrupt resistance
- Light Attack: LMB / RT tap (<0.4 s)
- Heavy / Charged Attack: LMB / RT held >=0.4 s, fires on release
- Combo / combo window: light-attack chain; next input accepted within 800 ms; HUD reads 'Hit N'
- Sweep: arc-sector melee, hits groups, can be jumped (JumpAvoidable)
- Thrust: narrow lane melee, hits airborne, side-dodge to avoid
- Slam: point/circle impact with optional shockwave, hits airborne
- Windup / Active / Recovery: the three attack and cast phases
- Rooted: movement locked during part of an attack or cast
- Soft aim assist: few-degree fan-out correction; explicitly no lock-on
- Ground-targeted: rooted reticle aiming for AoE; release or press to cast, cancel available
- Skill A / Skill B: the two equipped skills in the bindings spec (RB/Q, LB/E) with a hold radial to select
- Radial: skill selection wheel; does not slow or pause time; selection is committed
- Spell slots 1–4: in-game Q/LB Fireball, E/RB Blink, R Stone Skin, 1/D-pad Up Restoration
- Upcast: spend more SP (x2.0) for more power (x1.5)
- Cooldown: per-spell lockout (Fireball 2 s, Blink 4 s, Stone Skin 15 s, Restoration 10 s)
- Charges: multiple stored uses of an ability (schema field; Blink R20 gets 2)
- Rank: skill/spell level 1–20 with milestones at 1/5/10/15/20
- Weapon Set Swap: Set 1 ↔ Set 2 (Tab / hold Y); game currently cycles 5 weapons
- Quickslot Items A–D: consumables on 1–4 / D-pad
- Throwable: selected knife/bomb/vial thrown with G / Y tap
- Damage types: Physical (Slash, Pierce, Blunt), Fire, Frost, Lightning, Poison, Corrosive, Arcane, Shadow (Curse), Radiant (Holy)
- Resistance / vulnerability: positive value = % mitigation, negative = % extra damage taken
- Critical hit: 1.5x default multiplier; damage number shown at 1.5x scale
- Interact: F / X; 'Talk, loot, activate, climb prompts'; pickup prompt '[F] Pick up {item}'
- Pause / Menu: Esc / Start; tabs Character, Skills, Feats, Inventory, Settings; LB/RB switch tabs; I opens Inventory / closes menu

## OPEN QUESTIONS
- Which spell/skill model should the HUD design for: the bindings spec's two equipped skills (Skill A/B with hold-radial selection, up to 8 options) or the game's four direct spell slots (Q, E, R, 1)?
- Gamepad spell mapping is undecided (US-097 TODO(DECISION)): spec says RB = Skill A/Q and LB = Skill B/E; game maps LB = Q and RB = E; spell 3 (R) has no gamepad button (AC-057.5 said LB+RB).
- Key conflicts: the game uses 1 and D-pad Up for Spell 4, but the spec reserves them for quickslot Item A and Item C. Y is Throw (tap) and Weapon Set Swap (hold) in the spec but next-weapon tap in the game.
- Should Health show a numeric % (spec) or stay a bar only (current HUD)?
- Is there a distinct 'perfect parry' (design log: 250 ms inside the 500 ms window) with its own feedback? The code has only one parry window.
- Riposte / critical window and Parried stagger durations are unspecified (N33); what should the punish prompt show and for how long?
- Should enemies (and bosses) show a posture meter? The spec says enemies have posture meters and mentions a 'boss posture bar', but no enemy UI shows posture.
- Does the player have a breakable posture? The HUD shows a player posture bar, but only enemies run break logic; game_design_v2 talks of 'Player Stability' instead.
- Ground-target controls conflict: attack_taxonomy says press to enter, A/LMB to cast and B/RMB to cancel; the agent brief says hold and release to cast.
- Heavy-attack charge time is 0.4 s for every weapon (spec: 'depends on weapon'); is a visible charge meter wanted?
- Crits are specified (1.5x, crit chance from DEX/CHA) but never rolled; what is the crit presentation (scale only, or a glow/outline per AC-065.7)?
- Damage colour naming: AC-065.5 uses Holy/Dark while code and spec use Radiant/Shadow; Corrosive and Curse have no defined colour (gray default).
- How should resistance, weakness and immunity hits be signalled? The schema has signed resistances and Immunity tags, but no UI spec.
- Status presentation: game_design_v2 says status is 'primarily VFX-driven, not icon soup', yet a status-indicator bar exists. How much status UI is wanted?
- SP economy is undecided (N6/US-090: regen, max, respawn restore, costs), which affects how the SP bar reads.
- Upcasting is specified but has no input. How does the player choose to upcast, and how does the HUD show it?
- Combo counts disagree between sources (e.g. Dagger 5 in code vs 4 in skill sheet; Axe and Mace 2 vs 3; Unarmed 3 vs 5; Spear 3 vs 5 in attack_taxonomy). Which is canonical for UI copy?
- Facing and aim are pinned to +X pending a decision (US-098); is a facing/aim indicator part of the HUD?
- 'I' (Inventory) and in-menu LB/RB are not yet in control_bindings_v1.1; Sneak is C in v1 but Ctrl in v1.1.
- Should the old world-space enemy health bar be removed in favour of the enemy indicator (US-092 TODO(DECISION))?