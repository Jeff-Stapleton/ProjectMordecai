# Attributes, stats, scaling, status effects, skills and feats: research for the UI design system

Sources read in full: `Specs/character_attributes_v1.md` (2028 lines, mostly empty table rows; content at lines 1-31 and 1025-1038), `Specs/attribute_scaling_v1.md`, `Specs/stat_formulas_v1.md`, `Specs/status_effects_v1.md` (1012 lines; content at lines 1-21 and 1009-1011), `Specs/status_effects_launch_set_v1.md`, `Specs/skill_sheet_v1.1.md` (997 lines; 42 skills on lines 9-50, the rest empty), `Specs/skill_sheet_v1.md` (diffed). For progression, feats and UI rules I also read `Specs/game_design_v2.md`, `Specs/ability_system_v1.md`, `Specs/design_decisions_log.md`, the relevant parts of `Specs/agent_rules_v2.md`, `Specs/damage_types_v1.md` and `Specs/control_bindings_v1.1.md`, and `Specs/Game Design v1.md` (for differences only). I checked the code under `Source/LyraGame/Mordecai/` (AbilitySystem, StatusEffects, Skills, Feats, UI) and the stories US-011, 012, 013-018, 055, 059, 066, 067, 068, 091 and 093.

**Big picture for the designer:**
- The specs give **three different attribute models**. The code implements the 9-attribute `character_attributes_v1` model.
- Every in-game menu screen in this area (Character, Skills, Feats) has a C++ data model, but **none renders in the packaged game yet**:
  - N4: the Character tab is blank.
  - N5: Skills and Feats show "Coming Soon".
  - Backlog story US-093 fixes both.
- No skill or feat content assets exist, so even when rendered, both lists would be empty.
- Status display philosophy (`game_design_v2.md` pillar 8 and §9): **"Status is primarily VFX-driven, not icon soup"** and "minimal critical UI only".

---

## Primary attributes

### The implemented 9 (`character_attributes_v1.md`, `MordecaiAttributeSet.h`, `MordecaiCharacterSheetTypes.cpp`)
The code groups them into three columns with the labels "Physical", "Resilience" and "Magical" (US-066 AC-066.4).

| # | Name | Abbr | Group | Primary effect (primary_stats) | Secondary effect (secondary_stats) |
|---|---|---|---|---|---|
| 0 | Strength | STR | Physical | Physical Dmg 3.0% x Effective Mod | Armor Penetration 1.5% x Effective Mod |
| 1 | Dexterity | DEX | Physical | Attack Speed 1.5% x Effective Mod | Physical Crit Chance 0.75% x Effective Mod |
| 2 | Endurance | END | Physical | Stamina 4.0% x Effective Mod | Stamina Regen 2.0% x Effective Mod |
| 3 | Constitution | CON | Resilience | Health 6.0% x Effective Mod | Health Regen 1.5% x Effective Mod |
| 4 | Resistance | RES | Resilience | Affliction Resist 3.0% x Effective Mod | Affliction Recovery 1.5% x Effective Mod |
| 5 | Discipline | DIS | Resilience | Posture 4.0% x Effective Mod | Posture Recovery 2.0% x Effective Mod |
| 6 | Intelligence | INT | Magical | Spell Points 4.0% x Effective Mod | Spell Points Regen 2.0% x Effective Mod |
| 7 | Wisdom | WIS | Magical | Magic Dmg 3.0% x Effective Mod | Resistance Peneration [sic] 1.25% x Effective Mod |
| 8 | Charisma | CHA | Magical | Cast Speed 1.5% x Effective Mod | Magic Crit Chance 0.75% x Effective Mod |

### Effective Mod: diminishing returns (`character_attributes_v1.md` scaling_rules, `MordecaiAttributeScaling.cpp`)
"Use Effective Mod instead of raw Mod for all attribute scaling."

| Band | Mods | Counts at |
|---|---|---|
| Band 1 | 1-5 | 100% |
| Band 2 | 6-10 | 70% |
| Band 3 | 11-15 | 45% |
| Band 4 | 16+ | 25% |

- Formula: `Effective Mod = min(Mod,5) + max(min(Mod-5,5),0)*0.7 + max(min(Mod-10,5),0)*0.45 + max(Mod-15,0)*0.25`
- Spec examples:
  - Mod 12 → 5 + 3.5 + 0.9 = 9.4
  - STR 12 → Physical Dmg 28.2%, Armor Penetration 14.1%
  - DEX 12 → Attack Speed 14.1%, Physical Crit Chance 7.05%
- Values computed from the formula: Mod 5 = 5.0, 10 = 8.5, 12 = 9.4, 15 = 10.75, 20 = 12.0.
- Balancing note in the spec: "If penetration or resistance proves too volatile in testing, convert those two families to rating instead of raw %."
- The code passes the raw attribute value in as "Mod".

### Ranges, caps and defaults
- No spec gives a minimum, maximum or cap for primary attributes.
- Code: each primary attribute defaults to **10.0**, and the attribute set has no clamp on them.
- The code comment says "Raw base value (e.g. 10 at character creation)".

### How attribute points are gained
- `game_design_v2.md` §5: "Attributes (D&D-like scalers) — Generally static (rarely changed). Scale derived stats (HP, stamina, SP, regen, posture, resist)."
- No spec names a source of attribute points.
- Leveling grants "skill points and minor baseline growth", not attribute points.
- Character creation is not specified anywhere.

### Conflicting attribute models in the specs
- **`attribute_scaling_v1.md`** has only 6 attributes: STR, DEX, CON, INT, WIS, CHA. It uses "% × Mod" with different columns: Health (MaxHP multiplier), Stamina (Max pool multiplier), Stamina Regen, Stamina Tier Effectiveness, Spell Points (Max SP), Spell Regen, Posture Damage (deal), Block Stability, Parry Forgiveness, Cast Stability, Element DR% (Fire/Frost/Shock), Status Resist (buildup threshold). Per attribute:
  - STR: Posture Damage +1.5% × Mod (melee/heavy); Block Stability +2.0% × Mod.
  - DEX: Stamina Tier Effectiveness +0.5% × Mod ("dodge/parry consistency when tired"); Posture Damage +1.0% × Mod (finesse/ranged); Parry Forgiveness +1.0% × Mod.
  - CON: Health +3.0%, Stamina +2.0%, Stamina Regen +2.0%, Tier Effectiveness +0.5% ("reduces low-tier penalties"), Spell Regen +1.0%, Block Stability +1.0%, Element DR +0.5%, Status Resist +2.0%. Note: "main scaler for HP/stamina".
  - INT: Max SP +0.5 SP × Mod (rounded), Spell Regen +3.0%, Posture Damage +1.0% (spell), Cast Stability +2.0%, Status Resist +1.0% (vs magic statuses).
  - WIS: Stamina Regen +1.0% ("recovery under pressure"), Tier Effectiveness +0.5% ("defense actions more consistent"), Max SP +0.25 SP × Mod (optional; rounded), Spell Regen +1.5%, Cast Stability +1.0% ("less interruption"), Element DR +0.5%, Status Resist +3.0%.
  - CHA: no combat columns. "Charisma focuses on town attraction, dialogue leverage, vendor outcomes; minimal combat math impact." This conflicts with character_attributes, where CHA drives Cast Speed and Magic Crit.
- **`stat_formulas_v1.md`** lists 5 primaries ("Strength Dexterity Intelligence Constitution Discipline") with flat formulas:
  - Health = BaseHealth + (CON × 10)
  - Stamina = BaseStamina + (STR × 2) + (DEX × 1)
  - SpellPoints = BaseSP + (INT × 5)
  - SpellRegen = BaseSPRegen + (INT × 0.1)
  - Posture = BasePosture + (STR × 2) + (DIS × 1)
  - DamageTaken = IncomingDamage × (1 − Resistance). Example: FireResistance 25%, IncomingFireDamage 100 → FinalDamage 75.
- `game_design_v2.md` §11 names `character_attributes_v1` as "attribute → derived stat scaling table". That is the implemented one.
- Game Design v1 said "INT+CON improve regen" and "Player Stability … armor skills + CON + gear/feats". v2 says "INT improves regen" and "armor + DIS + gear + skills + feats".

---

## Derived stats

### Core resources (the 4 pools)
Code: `MordecaiAttributeSet.h/.cpp`. Max = Base × (1 + pct × EffMod).

| Resource | Base | Max formula | Clamp | Display |
|---|---|---|---|---|
| Health | 100 (`BaseMaxHealthValue`) | 100 × (1 + 0.06 × Eff CON) | 0..MaxHealth | HUD: "Displayed as % (0–100)" (`game_design_v2` §4, §9). Character sheet: "Current / Max" |
| Stamina | 100 | 100 × (1 + 0.04 × Eff END) | **−50**..MaxStamina (AC-008.1: "Stamina can go negative (floor at -50 to prevent infinite debt)") | "Tiered; low stamina reduces effectiveness, never blocks actions" |
| Spell Points | **10** | 10 × (1 + 0.04 × Eff INT) | 0..Max | "Discrete points; slower regen than stamina; INT improves regen; spells can upcast by spending more points". SP bar shows "12 / 20" style text (AC-055.3) |
| Posture | 100 | 100 × (1 + 0.04 × Eff DIS) | 0..Max | Player "Stability" in design terms |

Every Max is floored at 1. The code comment says the bases are "fixed until level progression exists".

### Primary-derived multipliers (5 rows on the character sheet)
Each defaults to 1.0. Display = (V − 1) × 100%, signed, one decimal, for example "+15.0%".
- Physical Damage = 1 + 0.03 × Eff STR
- Attack Speed = 1 + 0.015 × Eff DEX
- Affliction Resist = 1 + 0.03 × Eff RES
- Magic Damage = 1 + 0.03 × Eff WIS
- Cast Speed = 1 + 0.015 × Eff CHA

### Secondary-derived (9 rows)
- Armor Penetration = 1 + 0.015 × Eff STR
- Physical Crit Chance = 0.0075 × Eff DEX. This is a raw chance with default 0.0, not a multiplier.
- Stamina Regen = 1 + 0.02 × Eff END
- Health Regen = 1 + 0.015 × Eff CON
- Affliction Recovery = 1 + 0.015 × Eff RES
- Posture Recovery = 1 + 0.02 × Eff DIS
- Spell Points Regen = 1 + 0.02 × Eff INT (sheet label "Spell Points Regen"; AC text "SP Regen")
- Resistance Penetration = 1 + 0.0125 × Eff WIS
- Magic Crit Chance = 0.0075 × Eff CHA (raw chance, default 0.0)

### Stamina tiers
Sources: `MordecaiStaminaSystem.h`, `MordecaiStaminaBarWidget.h`, `design_decisions_log.md` 2026-03-19.

| Tier | Threshold | Multiplier | HUD color |
|---|---|---|---|
| Green (full) | ≥ 66% | **1.25x** ("reward for managing stamina well") | (0,1,0) |
| Yellow (mid) | ≥ 33% | **1.0x** ("the standard operating range") | (1,1,0) |
| Red (low) | > 0% | **0.75x** | (1,0,0) |
| Exhausted | ≤ 0 | **0.5x** | (0.5,0,0) |

- Regen 15/s after a 1000 ms delay. Sprint drains 10/s.
- TODO(DECISION) in code: END scaling of regen is stubbed.
- Design intent: "Full stamina should feel rewarding. Normal range is a big chunk of the bar. Penalties ramp up at the bottom."

### Hidden modifier attributes (GAS; drive statuses, not on the sheet)
HealingReceivedMultiplier, MoveSpeedMultiplier, DodgeRecoveryMultiplier, BlockStaminaCostMultiplier, OutgoingPostureDamageMultiplier, IncomingPostureDamageMultiplier, IncomingDamageMultiplier, BlockStabilityMultiplier, ArmorEfficiencyMultiplier, DamageReduction (Stone Skin), FireWardShieldHP, EnchantWeaponBonusDamage, RangedAccuracyMultiplier, AimAssistMultiplier, StealthDetectionMultiplier, StaminaTierPenaltyMultiplier, RangedEvasionChance (0..1, Blur), FireDamageReceivedMultiplier.

### Derived stats named in specs but not implemented
- From `attribute_scaling_v1`: Block Stability, Parry Forgiveness, Cast Stability, Element DR% (Fire/Frost/Shock), Status Resist (buildup threshold), Stamina Tier Effectiveness.
- From `game_design_v2`: **Player Stability** ("determines stagger/interrupt resistance; increased by armor + DIS + gear + skills + feats").
- From `damage_types_v1.md`: per-type Resistances for Physical, Fire, Frost, Lightning, Poison, Corrosive, Arcane, Shadow, Radiant.
  - Positive values reduce damage. Negative means vulnerability ("0.25 means 25% mitigation", "-0.20 means 20% extra damage taken").
- A full character sheet would need a resistances block. Today no per-element resistance attribute exists except FireDamageReceivedMultiplier.

---

## Leveling/XP/progression currency

**Levels** (`game_design_v2.md` §5):
- "**Max level:** 60 (WoW nod)."
- "Leveling grants skill points and minor baseline growth; not the primary power driver."
- High concept: "player skill can clear the entire game at level 1 with no upgrades".
- Open decision §12: "Exact numeric curves for BaseHP/BaseStamina/BaseSP by level."

**XP:**
- No XP formula or curve exists in any spec.
- `agent_rules_v2.md` Open Item 3 lists "loot/xp grant flow" (PlayFab) as undecided, and the NOTE says "Exact write timing for loot/xp/inventory deltas is still an open decision."
- No Level or XP attribute exists in code.

**Skill points** (`MordecaiSkillComponent.h`, US-011):
- `AvailableSkillPoints` is an int, replicated, clamped ≥ 0.
- One point buys +1 rank. Learning a new skill (Rank 0 → 1) costs 1 point.
- An allocation fails if the skill is unknown, already at rank 20, or there are 0 points.
- US-011 TODO(DECISION) "Skill point economy": "How many skill points per level? … max level 60 with 50 skills at max rank 20 (= 1000 total ranks)."
- Skill point sources are undefined ("leveling, quests" mentioned as out of scope).
- No respec or rank-down mechanic exists ("Default: no rank-down mechanic, but the system supports it").

**Other progression vectors** (`game_design_v2` §6, §7; detail belongs to other areas):
- Curated magic items (Green/Blue/Purple/Red(Gold))
- Upgrade Keys (e.g., "Philosopher's Stone unlocks Mage Tower tier")
- Town research lines that "Improve SP regen/cost … (not raw damage creep)"
- Feats (earned, not a currency)
- Gold is mentioned only as loot ("bandits drop gold").

---

## Status effects

### Rules
- `game_design_v2` §10: "statuses must have distinct gameplay identity (not 'damage colors')."
- §9: "Status effects communicated primarily via VFX + animation; minimal critical UI only."
- Pillar 8: "Status is primarily VFX-driven, not icon soup."
- `agent_rules_v2` §6: GAS only; no parallel status systems.
- Open Item 2: buff/debuff stacking policy (refresh vs stack, unique tags, magnitude) is undecided.

### Framework (code, US-013)
- **Tags:** `Mordecai.Status.<Name>`, `Mordecai.Immunity.<Name>` (an immunity tag blocks application), `Mordecai.Status.Category.<DoT|Debuff|Control|Buff|Environmental>`.
- **Cleanse:** `RemoveStatusEffectsByCategory` (e.g., cleansing DoT removes Burning, Bleeding and Poisoned).
- **Stacking default:** Refresh (StackLimitCount 1, AggregateByTarget, refresh duration on application, clear the entire stack on expiry). Only Frostbitten (max 5) and Shocked (max 3) stack. `EMordecaiStackingPolicy` {Refresh, Stack Count, Stack Duration, Independent} is marked TODO(DECISION).

**Category mapping** (`MordecaiStatusEffectTypes.cpp`):

| Category | Statuses |
|---|---|
| DoT | Burning, Bleeding, Poisoned |
| Debuff | Frostbitten, Weakened, Brittle, Cursed, Corroded, Exposed |
| Control | Shocked, Silenced, Rooted, Blinded, Fear, Frozen, MicroStunned |
| Buff | Focused |
| Environmental | Drenched |

Unknown tags default to Debuff.

### Launch set: 16 statuses
Sources:
- Identity, visual/audio read, sources and cleanses are quoted verbatim from `status_effects_launch_set_v1.md`.
- The numbers are code defaults, marked "placeholder" in the stories.
- The damage-type link comes from `damage_types_v1.md` §2-3, "guidance mappings, not absolute hard locks". The rule: "Not every fire effect must burn. Not every slash must bleed."

| Status | Category | Identity | Mechanics (spec → implemented default) | Stacks | Damage-type link | Visual/Audio Read (spec) | Typical sources | Cleanses/Mitigation |
|---|---|---|---|---|---|---|---|---|
| Burning | DoT | "You're on a clock" | DoT; minor cast interrupt; standing in flames refreshes → 5.0 s, tick 1.0 s, 3.0 fire dmg/tick (`Mordecai.Damage.Fire`), 15% cast-interrupt per tick; doused by Drenched | refresh | Fire → Burning | "Flames + ember trail + sizzling" | Fire spells, flame traps, lava/embers | Douse item; water/ice; Restoration cleanse; fire resist gear |
| Bleeding | DoT | "Recovery is denied" | Healing received reduced/blocked until clotted; hits refresh → 6.0 s, healing −50%, any hit refreshes, clots after 4.0 s with no damage | refresh | Physical.Slash → Bleeding ("bleed buildup") | "Dark-red seep + heartbeat thump" | Blades, barbs, beast bites, bleed weapons | Bandage/First Aid; Plate/Chain resist; Restoration cleanse |
| Poisoned | DoT | "You can't catch your breath" | Stamina regen suppressed; low tiers feel worse; movement labored → 8.0 s, stamina regen −60%, move −15% | refresh | Poison → Poisoned | "Green haze + labored breathing" | Venom beasts, darts, swamp hazards | Antidote; Resilience skill; Clinic prep buff; poison resist gear |
| Frostbitten | Debuff | "Sluggish body, slower reactions" | Move/attack speed down, dodge recovery slower, freeze at high stacks → 6.0 s, move −25%, attack speed −20%, dodge recovery +30%; **at 5 stacks → Frozen 2.0 s** then stacks clear; perfect dodge breaks stacks | **1-5** | Frost → Frostbitten | "Ice crust + shiver + frosty footsteps" | Ice spells, cold zones, frost traps | Heat item; proximity to fire; Cryo resist gear; Restoration cleanse |
| Shocked | Control | "Timing disruption" | Micro-stun on repeated hits; block costs more; interrupts long casts → 5.0 s; micro-stun 15% per stack on hit → MicroStunned 0.3 s; block stamina +20% per stack; cast interrupt 25% per stack (casts > 1 s); perfect block clears 1 stack | **1-3** | Lightning → Shocked | "Crackling arcs + sharp zap pops" | Lightning spells, storm traps, conductive water hazards | Grounding charm; Warding dispel; shock resist gear; avoid Drenched synergy |
| Weakened | Debuff | "You can't break them" | Your posture damage reduced → 8.0 s, outgoing posture −40%; heavy attacks suffer only 50% of the penalty | refresh | Shadow/Curse lists Weakened | "Dim aura + dulled impact sound" | Curse casters, debuff traps, certain bosses | Restoration cleanse; Fortify tonic; town clinic blessing |
| Brittle | Debuff | "You break easily" | Extra posture taken, stagger threshold lowered → 7.0 s, incoming posture +35%, MaxPosture −20%; perfect block halves the increase | refresh | Frost lists Brittle | "Hairline crack sheen + glassy tinks" | Ice/stone debuffs, heavy blunt enemies | Geomancy ward; stability/plate gear; Fortified buff |
| Silenced | Control | "Magic is shut off" | Cannot cast; SP regen paused (optional); weapon skills OK unless magical → 5.0 s, SP regen ×0; abilities tagged `Mordecai.Ability.Category.Magical` blocked | refresh | Arcane "may interact with Silence" | "Void ring/muzzle effect + muffled SFX" | Anti-mage enemies, null zones, ward traps | Warding dispel; Restoration cleanse; anti-silence charm |
| Rooted | Control | "Position solved or punished" | Cannot move; can attack/block/cast; refresh in root zone → 3.0 s; **break free costs 25 stamina** (input binding deferred) | refresh | none | "Vines/ice gripping legs + scraping" | Vines, ice binds, trap plates | Fire; cleanse; engineering tool |
| Blinded | Control | "Reads become unreliable" | Ranged accuracy, weakpoint aid, detection, lock-on down → 4.0 s, accuracy −40%, aim assist −50%, stealth detection −60%; perfect dodge gives a 1000 ms clarity window | refresh | none | "Dark vignette + smeared highlights" (screen-space) | Illusion magic, sand bursts, ink traps | Eye drops; WIS scaling; Restoration cleanse |
| Fear | Control | "Composure collapse" | Tier penalties amplified; guard stability down; aim drift → 5.0 s, StaminaTierPenalty ×1.50, guard stability −25%, aim drift 0.15; perfect block/parry grants **Resolved** which cancels Fear | refresh | Shadow/Curse lists Fear | "Shaky stance + low rumble + heartbeat" | Necro/eldritch enemies, intimidation auras | Courage charm; WIS/CHA perks; town morale buff |
| Cursed | Debuff | "Resource rules broken" | Spell regen down; healing down; some buffs disabled (optional) → 10.0 s, SP regen −50%, healing −30% (stacks with Bleeding); which buffs are disabled is TODO(DECISION); Focused is not blocked | refresh | Shadow → Cursed; Radiant has "anti-curse hooks" | "Black smoke + whispers" | Undead, forbidden relics, curse traps | Restoration/holy cleanse; town wards upgrade; anti-curse talisman |
| Exposed | Debuff | "Next mistake hurts more" | Next hit bonus damage; expires after 1 hit or a short timer → 5.0 s or 1 hit, +30% damage, consumed on first hit; perfect block removes it | refresh | none | "Glowing target sigil hovering in-world" | Snipers, marked traps, boss tells | Warding; smoke/illusion; block mastery |
| Corroded | Debuff | "Gear advantage denied" | Sharpen/repair disabled; block stability and armor down → 10.0 s, block stability −30%, armor efficiency −25% | refresh | Corrosive → Corroded | "Pitting/steam + sizzling" | Slimes, acid pools, corrosion traps | Neutralizer item; smith service; corrosion resist gear |
| Drenched | Environmental | "Elemental rules shift" | Fire taken down; Shock up; Frost buildup up → 15.0 s, fire taken −25%, Shocked micro-stun ×1.50, +1 Frostbitten stack per application; douses Burning; fire on a Drenched target removes Drenched and skips Burning | refresh | water (Fire and Lightning interactions) | "Water dripping + squelch footsteps" | Rain, water spells, flooded zones | Heat/dry sources; towel charm; change zones |
| Focused (Buff) | Buff | "Mastery streak reward" | Perfect-action streak gives stamina tier forgiveness + posture bonus; breaks on hit → granted after **3** consecutive perfect dodge/block/parry; **infinite** until hit; tier penalties ×0; +20% outgoing posture; streak resets on any hit or non-perfect action; self-only | n/a | Arcane "may interact with … Focused" | "Clean whoosh + subtle time dilation" | Perfect dodge/block/parry chains | N/A (self-buff) |

### Other implemented status tags that can reach the status bar
Comments come from `MordecaiGameplayTags.cpp`.
- **Frozen:** "full incapacitation from max frost stacks"; cannot move, attack, block or dodge; 2.0 s.
- **MicroStunned:** "brief interrupt from Shocked on-hit"; 0.3 s.
- **StoneSkin:** "damage reduction applied". HUD showed "Status 10.0".
- **Sleeping:** "incapacitated, wakes on damage".
- **FireWard:** "absorbing fire damage"; FireWardShieldHP.
- **Blessed:** "increased physical and magic damage multipliers".
- **Restoration:** "healing over time active".
- **EnchantedWeapon:** "melee attacks deal bonus elemental damage".
- **Resolved:** "Fear cancelled by perfect block/parry during Fear".
- **Blurred:** "ranged projectile attacks have a chance to miss".
- State tags a player should notice:
  - `Mordecai.State.PostureBroken` ("staggered"; 1000 ms, ×1.5 damage taken per the design log)
  - `Mordecai.State.KnockedDown` (×2.0; "replaces Stagger")
  - `Mordecai.State.Invulnerable`
  - `Mordecai.State.Airborne`

### `status_effects_v1.md` diverges from the launch set
- It has a `negative_effects` sheet: Burning, Bleeding, Poisoned, **Dazed** ("You can't think straight", otherwise blank), **Slowed**, **Asleep**, Silenced, Rooted, **Staggered**, **Knocked Down**, **Frozen**, **Stunned**.
- Its `positive_effects` sheet is "*(empty sheet)*".
- The six new names reuse other rows' mechanics text verbatim:
  - Slowed carries Frostbitten's text
  - Asleep carries Weakened's
  - Staggered carries Blinded's
  - Knocked Down carries Fear's
  - Frozen carries Cursed's
  - Stunned carries Exposed's
- This looks like a rename in progress.
- `game_design_v2` §11 still calls `status_effects_v1` "launch status set". The code implements the launch_set 16.

### Status HUD as implemented (US-055; defects N16 and US-091)
- `UMordecaiStatusEffectBarWidget`: horizontal row, **max 8** indicators, one per `Mordecai.Status.*` tag.
- `UMordecaiStatusEffectIndicatorWidget`:
  - Name text: 10 pt, white.
  - Countdown: 9 pt, (0.8, 0.8, 0.8), format "%.1f" seconds; empty when there is no duration.
  - Tint image: green (0,1,0) for buff, red (1,0,0) for debuff.
- `BuffTags` = Focused, Blessed, StoneSkin, FireWard, Restoration. Every other status shows red, which includes Blurred, EnchantedWeapon and Resolved.
- No stack count is shown.
- **N16 defect:** every indicator reads "Status" (registered on the parent tag), so only one indicator can exist and tints are wrong. Seen bottom-left as "Status 10.0".
- US-091 (backlog) plans: name shown, "one indicator per status".

### Buildup (implied, not implemented)
These specs describe accumulation toward a status, but the code applies statuses instantly:
- `attribute_scaling_v1` "Status Resist (buildup threshold)"
- `damage_types_v1` "BuildsStatus: Status: Burning, Magnitude: 12"
- the launch set's "frost buildup"
- skill text "bonus bleed buildup"

---

## Skills

### Structure (`game_design_v2` §2, §5; `skill_sheet_v1.1.md`; US-011, US-067)
- "Granular skills (DCC vibe): 'Everything you do is a skill.' Skills level to 20 and unlock techniques at milestones."
- **5 categories** (the "trees"):

| Display label | Enum |
|---|---|
| Weapon | `Weapon` |
| Defense & Armor | `DefenseArmor` |
| Movement & Exploration | `MovementExploration` |
| Utility & Interaction | `UtilityInteraction` |
| Magic | `Magic` |

- **Ranks:** 0 (not learned) to 20. **Milestones:** Rank 1 / 5 / 10 / 15 / 20 "unlock techniques/passives/interactions".
- On a milestone:
  - Tag `Mordecai.Skill.<SkillName>.Rank<N>` is applied.
  - Event `Mordecai.Event.SkillMilestone` fires.
  - The milestone ability from the data asset is granted.
- **No prerequisites between skills, no branching and no unlock costs other than 1 skill point per rank** are specified. The "tree" is a flat list per category.
- Magic: open decision "Whether magic schools remain 'by school' vs refactor to 'spell type.'" In v1.1 each spell is its own skill.
- **Active vs passive:** not labeled in the sheet.
  - Rank 1 text makes weapon skills active attacks ("Wielder performs…"), Dodging/Parrying/Shields active defenses, and Magic active spells.
  - Armor skills are passive ("While worn…").
  - Most Movement & Exploration and Utility skills are passive modifiers, except Traps (place), First Aid (use), Lockpicking and Pickpocket (interactions).
  - `ability_system_v1.md` types: Weapon Techniques, Spells, Mobility abilities, Passive feats.
  - Ability properties: cooldown, resource cost, cast time, targeting type (Self, Cone, Circle, Line, Projectile, Persistent Area), magnitude.
  - Example: "Fireball Cooldown: 5s Cost: 3 SP Radius: 4m".
  - `AbilityPower = BaseAbilityPower + AttributeScaling + SkillScaling`.
- **Equipped active skills on the HUD** (`control_bindings_v1.1.md`):
  - Use Skill A = RB tap / Q tap; Select Skill A (Radial) = RB hold / Q hold.
  - Use Skill B = LB tap / E tap; Select Skill B (Radial) = LB hold / E hold.
  - "radial does NOT slow/pause time; selection is committed."
- **Scaling placeholders:** Rank 1 text uses `{x} * {rank}`, `{y} * {rank}`, `{z} * {rank}`, `{k} * {rank}`. A tooltip would need to resolve these.
- **Counts:**

| Source | Count |
|---|---|
| v1.1 | **42** (Weapon 13, Defense & Armor 8, Movement & Exploration 6, Utility & Interaction 4, Magic 11) |
| v1 | 40 |
| Code comment / US-011 | "50 total" |
| `game_design_v2` §12 | "target ~40–70; current catalog is 60" |

- **No skill data assets exist** in `Plugins/GameFeatures/MordecaiCore/Content`, and nothing registers any at runtime.

### v1.1 skill list
Format: R1 base / R5 / R10 / R15 / R20.

**Weapon (13)**
- **Longswords** — R1 arcing sweep, {x}·rank° arc, {y}·rank range, physical {z}·rank / charged spinning heavy / 3-swing light combo / heavy leaves a 2 s cyclone / combo fires short projectiles at full health
- **Greatswords** — wide cleave / charged overhead slam (heavy posture) / 2-swing combo + cleaving finisher / heavy shockwave on impact / finisher launches a crescent of force
- **Shortswords** — quick sweeping strike / channeled flurry heavy / 4-hit combo + lunging finisher / flurry moves with target / finishers grant a speed burst
- **Daggers** — rapid jab / shadow poke (bonus from behind) / 4-hit combo + bleed buildup / heavy becomes a stab flurry passing light targets / finisher cloaks on kill
- **Axes** — overhead swing / hooking pull / 3-hit + armor-breaking chop / wider heavy arc that staggers / finisher bonus vs wounded
- **Maces** — crushing overhead / guard-breaking smash (posture) / 3-hit + crushing overhead / heavy shockwave / finisher bonus vs armored
- **Spears** — long thrust / sweeping brace / 3-hit + pinning thrust / impaling charge / finisher thrust flurry
- **Quarterstaff** — sweeping strike / vaulting overhead (gap close) / flowing 3-hit, more range / spinning guard / finisher repositioning step
- **Unarmed** — rapid jab / palm strike (heavy posture) / 5-hit + rising blow / grapple and throw light targets / finishers boost attack speed
- **Longbow** — powerful projectile / power draw / charged shots pierce / heavy splits into a volley / full charge reveals weak points
- **Shortbow** — rapid projectile / snap shot / dodge empowers next shot / short volley / moving hits stack attack speed
- **Crossbows** — instant bolt, armor-piercing {y} plus physical {z} / cranked bolt (posture) / bonus vs armored / exploding bolts / faster reload after a kill
- **Two-Weapon Fighting** — dual stance for {x}·rank s / off-hand deflect / rushing 4-hit / spinning cross-slash / finishers strike with both weapons

**Defense & Armor (8)**
- **Dodging** — "Dive and roll {x}·rank meters…, avoiding all damage for {y}·rank seconds" / perfect dodge speed burst / dodge cancels light recovery / afterimage distracts / next hit damage bonus
- **Parrying** — parry window {x}·rank s, {y}·rank posture dmg / parries restore stamina / reflect projectiles / shockwave (posture) / perfect parry empowers next attack
- **Buckler Shield** — small shield, frontal dmg −{y}, posture −{z} / shield jab / counter bash / move faster while blocking / bash ricochets projectiles
- **Scutum Shield** — large shield, −{y} frontal, −{z}% move speed / shield push stagger / protects allies behind / bigger push / planted wall stance
- **Heater Shield** — balanced shield; counters deal physical {z} plus posture {k} / shield bash / block empowers next light / bash stuns light targets / blocks restore stamina
- **Leather Armor** — movement penalties −{x}%, stealth +{y}% / cheaper dodge / speed on damage / less crit damage taken / perfect dodge attack speed
- **Chainmail Armor** — defense +{x}, keeps {y}% move speed / cheaper block / bleed and slash resistance / posture resistance on hit / blocks deal posture
- **Plate Armor** — defense +{x}, posture resistance +{y} / less knockback / stagger resistance during heavy / armor up on damage / walk through light attacks

**Movement & Exploration (6)**
- **Sneaking** — crouched; detection radius −{x}%, noise −{y}%
- **Climbing** — speed +{x}%, stamina drain −{y}%
- **Swimming** — swim speed +{x}%, breath +{y} s
- **Perception** — notice range {x}, reveal {y} s
- **Tracking** — range {x}, trails from last {y} s
- **Cartography** — "revealing {x}·rank% more detail in explored areas"; landmarks, secrets, merchants/shrines/hazards noted on the map

**Utility & Interaction (4)**
- **Traps** — arms after {x} s, physical {y}; later two active traps
- **Lockpicking** — "{x}% wider success window and {y}% faster progress"
- **Pickpocket** — "{x}% success chance within {y} range"
- **First Aid** — restore {x} health over {y} s; R5 "also stops bleed effects"

**Magic (11)**
- **Fireball** — "Channelled spell. Fires {x} magic projectiles … force {z} dmg"; burning ground at R10
- **Cone of Cold** — R1 text is a copy of Stone Skin's; R10 "Targets slowed … can now be briefly frozen"
- **Magic Missle** (spelled this way in the sheet) — channelled projectiles
- **Stone Skin** — armor {x} for {y} s
- **Sleep** — target sleeps {x} s
- **Blink** — teleport {x} m; R20 second charge
- **Illusion** — mirrored copy, {y} HP
- **Fire Ward** — absorbs fire {x} s
- **Frost Ward** — absorbs frost {x} s
- **Float** — float {x} s when jumping
- **Counter Spell** — interrupt beam {x} range; R10 "Interrupted targets are now briefly silenced"

### v1 → v1.1 differences
- **v1 layout:** 20 rank columns, but only Rank 5/10/15/20 were filled, with **named techniques**. Examples:
  - Longswords: Pommel Strike / Deflect Slash / Stance Shift / Master Riposte
  - Dodging: "Perfect Dodge reward enabled" / Quick Recovery / Ghost Step / Untouchable
  - Lockpicking capstone: "Master Key (one free open/day)"
  - Cartography capstone: "Metroid Key Sense (hint gates)"
- **v1.1 layout:** Rank 1/5/10/15/20 columns with descriptive text. Rank 1 is the base ability.
- **Removed in v1.1:** Throwables, Wands (Focus), Blocking, Shields (split into Buckler, Scutum and Heater), Frost Nova (now Cone of Cold), Bless, Healing Light.
- **Added in v1.1:** Buckler, Scutum and Heater Shield, Cone of Cold, Illusion, Fire Ward, Frost Ward, Float, Counter Spell.
- **Moved:** Traps and Lockpicking went from Movement & Exploration to Utility & Interaction.
- **Data defects in v1.1:**
  - Cone of Cold R1 duplicates Stone Skin's text.
  - Fireball R1 reuses the Magic Missle "force dmg" text.
  - The Shortbow and Magic Missle R1 cells contain CSV artifacts ("Shortbow,\"Wielder…").
  - Several cells have stray trailing quotes.

---

## Feats

### What they are (`game_design_v2.md` pillar 3, §5)
- "Feats are earned, not picked: Unlocked via gameplay achievements. Tiered (Common/Rare/Legendary). Strong feats can include drawbacks (DOS2-like)."
- "(Outer Worlds + DOS2) Unlocked by achievements/patterns (e.g., burn 6 enemies at once → Pyromaniac)."
- Tiers:
  - **Common**: "modest, no downside"
  - **Rare**: "strong specialization, light downside/condition"
  - **Legendary**: "build-defining, meaningful drawback"
- Feats also raise Player Stability. `ability_system_v1` lists "Passive feats".

### Code (US-012)
- `UMordecaiFeatDataAsset` fields: FeatName, DisplayName, Description, Tier, GrantedEffects, GrantedAbilities, DrawbackEffects, FeatTag (`Mordecai.Feat.<FeatName>`), UnlockConditions.
- Conditions use AND logic and come in four types:
  - **StatThreshold** (e.g., "EnemiesBurnedSimultaneously" ≥ 6)
  - **EventCount** (e.g., "TotalPerfectParries" ≥ 50)
  - **TagPresent**
  - **SkillRank**
- Unlock behavior:
  - Feats auto-unlock once, are replicated, and fire `Mordecai.Event.FeatUnlocked`.
  - Common feats apply GrantedEffects only.
  - Rare and Legendary feats also apply DrawbackEffects, which are "non-optional".
- Only sample: "Pyromaniac" (Common; burn 6 enemies simultaneously), in tests. **No feat content assets exist.**

### How feats display (US-068; in the Pause Menu "Feats" tab; not visible yet, N5)
- **Summary header:** "Feats: 5 unlocked — 3 Common, 1 Rare, 1 Legendary".
- **Unlocked rows:** display name, tier badge, short description excerpt.
- **Locked rows:** name visible, description "???", tier, and progress per condition:
  - "StatName: CurrentValue / ThresholdValue"
  - "SkillName Rank: CurrentRank / RequiredRank"
  - "Requires: TagName" with a check or cross
- **Detail panel:** full description, tier, number of granted effects, granted abilities and drawback effects. It shows drawback counts, not drawback text.
- **Tier colors** (configurable):

| Tier | Color | Value |
|---|---|---|
| Common | White/Gray | (0.8, 0.8, 0.8) |
| Rare | Blue | (0.3, 0.5, 1.0) |
| Legendary | Gold | (1.0, 0.75, 0.0) |

- A live unlock moves the row from the locked to the unlocked section immediately.
- No unlock toast or notification is specified anywhere.
- Color conflict: loot rarity in `game_design_v2` uses Green/Blue/Purple/Red(Gold), so Gold means Legendary feat in one system and Red(Gold) in the other.

---

## What a character sheet, skill tree and status bar must show

**Pause menu shell** (`MordecaiPauseMenuTypes.cpp`, US-069/095):
- Title "Paused".
- Tabs in order, with Character as the default tab:

| Tab | Id |
|---|---|
| Character | `character` |
| Skills | `skills` |
| Feats | `feats` |
| Inventory | `inventory` |
| Settings | `settings` (placeholder "Coming Soon") |

- Content area 1000×640, with a scroll box for long skill lists.
- LB/RB switch tabs and wrap; mouse clicks tab headers; Esc / Start / Gamepad B / I close; I opens on Inventory.

**Character sheet**
- **Specified** (US-066):
  - 9 primary attributes in 3 groups (Physical: STR DEX END; Resilience: CON RES DIS; Magical: INT WIS CHA). Each row shows name, base value (int) and an Effective Mod bonus "+X%".
  - 4 resources as "Current / Max": Health, Stamina, Spell Points, Posture.
  - 5 primary-derived as signed %: Physical Damage, Attack Speed, Affliction Resist, Magic Damage, Cast Speed.
  - 9 secondary-derived: Armor Penetration, Physical Crit Chance, Stamina Regen, Health Regen, Affliction Recovery, Posture Recovery, Spell Points Regen, Resistance Penetration, Magic Crit Chance.
  - "--" when unbound. Live per-row updates.
- **Implied:**
  - Level (max 60) and XP progress
  - Unspent skill points
  - Effective Mod band explainer (100/70/45/25%)
  - Stamina tier thresholds
  - Player Stability
  - Per-damage-type resistances (Physical, Fire, Frost, Lightning, Poison, Corrosive, Arcane, Shadow, Radiant) and immunities
  - Unlocked feats summary
  - Active statuses with their counterplay
- **Formatting traps:**
  - Crit chances are raw chances with default 0, so the "(V−1)×100%" format would print −100%.
  - `FormatEffectiveModBonus` prints the Effective Mod number itself as "%" (e.g., 8.5 → "+8.5%").

**Skill tree** (US-067)
- **Specified:**
  - Category switcher for the 5 categories.
  - Per skill: name, "rank / 20", a rank bar with milestone ranks 1/5/10/15/20 visually distinguished, and an Allocate action that is greyed out at 0 points or rank 20.
  - "Available skill points" shown prominently.
  - Detail panel with 5 milestone rows: description, reached or "locked" presentation, and whether the ability was granted ("unlocked and active" vs "not yet reached").
  - Live update on milestone.
- **Implied:**
  - Unlearned (rank 0) state
  - Resolved {x}·rank numbers
  - Active vs passive marker
  - Equipped Skill A/B slots and radial
  - Milestone-reached celebration

**Status bar / status feedback**
- **Specified:**
  - Primarily VFX + animation per status (the 16 "Visual/Audio Read" cues), "minimal critical UI only".
  - HUD row of up to 8 chips with name, countdown in seconds and buff/debuff tint.
- **Implied:**
  - Stack pips (Frostbitten 1-5 → Frozen; Shocked 1-3)
  - Infinite-duration state (Focused) and Focused streak 0/3
  - Bleeding clot timer (4 s)
  - Exposed hit-consumed state
  - Rooted break-free prompt (25 stamina)
  - Frozen, MicroStunned and Sleeping incapacitation
  - Immunity / resisted feedback
  - Status buildup meters
  - The same reads on enemies (Weakened, Brittle and Sleep applied by the player)
  - A codex explaining Identity, Counterplay and Cleanses

---

## Terminology
Use these exactly as the sources do:
- **STR/DEX/END/CON/RES/DIS/INT/WIS/CHA** (Strength, Dexterity, Endurance, Constitution, Resistance, Discipline, Intelligence, Wisdom, Charisma)
- **Effective Mod**, **Mod**, **Band 1-4**
- Group labels **Physical / Resilience / Magical**
- **Health**, **Stamina**, **Spell Points (SP)**, **Posture**
- **Affliction Resist / Affliction Recovery** (the attribute-side name for status resistance)
- **Stamina tiers Green / Yellow / Red / Exhausted**
- **Perfect dodge / perfect block / perfect parry**, **perfect action**, **streak**
- **Stagger**, **Knock Down**, **Posture break**, **Player Stability**
- **Skill**, **Rank** (0-20), **Milestone** (1/5/10/15/20), **skill points**, **Allocate**, **technique**
- Categories **Weapon / Defense & Armor / Movement & Exploration / Utility & Interaction / Magic**
- **Skill A / Skill B**, **Radial**
- **Feat**, tiers **Common / Rare / Legendary**, **drawback**, **unlock condition**
- **Status**, categories **DoT / Debuff / Control / Buff / Environmental**
- **Immunity**, **cleanse**, **buildup**, **stacks**, **clot**, **douse**, **break free**, **Resolve/Resolved**, **clarity window**, **Focused**, **Frozen**, **MicroStunned**
- **Upcast**
- "**Everything you do is a skill**" (the DCC pillar)
- "**Feats are earned, not picked**"
- "**not icon soup**"
- Spec spellings that are probably typos but appear verbatim: "Magic Missle", "Resistance Peneration".


## UI INVENTORY
- [In-game menu|implemented] **Pause menu shell and tab bar**: Container for the Character, Skills, Feats, Inventory and Settings screens | data: Title 'Paused'; tabs Character (default), Skills, Feats, Inventory, Settings; content area 1000x640 | states: open/closed; active tab; last-active tab remembered; LB/RB wrap between tabs; Settings shows a 'Coming Soon' placeholder (Source/LyraGame/Mordecai/UI/MordecaiPauseMenuTypes.cpp:15-21; stories/backlog/US-093 (1000x640); design_decisions_log.md 2026-10-30)
- [In-game menu|implemented] **Character sheet: primary attributes panel**: Show the 9 D&D-like attributes that scale derived stats | data: STR, DEX, END (Physical); CON, RES, DIS (Resilience); INT, WIS, CHA (Magical); per row: full name, base value (int, default 10), Effective Mod bonus '+X%' | states: unbound ('--'); bound/live; a single row refreshes on change (MordecaiCharacterSheetTypes.h/.cpp; US-066 AC-066.3/066.4; not rendered in package (N4), fix planned in US-093)
- [In-game menu|implemented] **Character sheet: core resources block**: Show the current and maximum of the four pools | data: Health, Stamina, Spell Points, Posture as 'Current / Max' (floored ints); bases 100/100/10/100; Stamina can be negative down to -50 | states: unbound '--'; live; stamina negative (Exhausted) (MordecaiCharacterSheetTypes.cpp:52-56, FormatResource; MordecaiAttributeSet.cpp:250-284; US-066 AC-066.5)
- [In-game menu|implemented] **Character sheet: primary-derived multipliers**: Show the percentage bonuses that attributes give | data: Physical Damage (STR 3.0%), Attack Speed (DEX 1.5%), Affliction Resist (RES 3.0%), Magic Damage (WIS 3.0%), Cast Speed (CHA 1.5%) x Effective Mod; displayed as (V-1)*100% e.g. '+15.0%' | states: unbound '--'; positive/negative sign (MordecaiAttributeSet.cpp:85-110; MordecaiCharacterSheetTypes.cpp:32-37; US-066 AC-066.6)
- [In-game menu|implemented] **Character sheet: secondary-derived stats**: Show penetration, crit, regen and recovery stats | data: Armor Penetration, Physical Crit Chance (raw 0.75% x EffMod), Stamina Regen, Health Regen, Affliction Recovery, Posture Recovery, Spell Points Regen, Resistance Penetration, Magic Crit Chance (raw 0.75% x EffMod) | states: unbound '--'; crit chance is a raw chance (default 0), not a multiplier (MordecaiAttributeSet.cpp:113-138; MordecaiCharacterSheetTypes.cpp:40-49; US-066 AC-066.7)
- [In-game menu|implied] **Effective Mod / diminishing returns explainer (tooltip)**: Explain why raising an attribute gives less each time | data: Bands: Mods 1-5 100%, 6-10 70%, 11-15 45%, 16+ 25%; e.g. Mod 12 -> Effective Mod 9.4 | states: hover/focus tooltip per attribute (Specs/character_attributes_v1.md lines 1025-1038)
- [In-game menu|implied] **Level and XP readout**: Show character level progress toward max level 60 | data: Level (1-60), XP to next level (no formula exists), unspent skill points | states: normal; level-up pending; max level (Specs/game_design_v2.md §5 Levels; agent_rules_v2.md Open Item 3 (loot/xp flow undecided); no Level/XP attribute in code)
- [In-game menu|implied] **Resistances, immunities and stability panel**: Show damage-type mitigation, status immunities and Player Stability | data: Resistances per type: Physical, Fire, Frost, Lightning, Poison, Corrosive, Arcane, Shadow, Radiant (positive = mitigation, negative = vulnerability); Mordecai.Immunity.<Status>; Player Stability; spec-only Element DR% (Fire/Frost/Shock) and Status Resist | states: resistant / neutral / vulnerable; immune (Specs/damage_types_v1.md §4; Specs/attribute_scaling_v1.md; Specs/game_design_v2.md §4 Posture model; Specs/stat_formulas_v1.md Resistance)
- [In-game menu|implemented] **Skills tab: category switcher**: Switch between the 5 skill trees | data: Weapon, Defense & Armor, Movement & Exploration, Utility & Interaction, Magic | states: selected category; empty category (no skill assets exist yet) (MordecaiSkillTreeWidget.h; MordecaiSkillTypes.h; US-067 AC-067.3; shows 'Coming Soon' in package (N5), planned US-093)
- [In-game menu|implemented] **Skill row with rank bar and Allocate button**: Show a skill's rank and spend a point on it | data: Skill name, 'rank / 20', rank bar with milestones 1/5/10/15/20 marked, Allocate action | states: unlearned (rank 0); learning (0->1 costs 1 point); in progress; milestone reached; max rank 20; Allocate disabled when 0 points or rank 20 (FMordecaiSkillTreeRow; US-067 AC-067.4/.5/.9/.10; US-011 AC-011.10-.12)
- [In-game menu|implemented] **Skill detail panel (milestones)**: Describe what each milestone rank unlocks | data: 5 rows (Rank 1/5/10/15/20): description text with {x}*{rank} placeholders, reached flag, ability granted flag | states: locked presentation; reached; 'unlocked and active' (ability granted); no description (FMordecaiSkillTreeMilestoneRow; US-067 AC-067.6/.7/.11; Specs/skill_sheet_v1.1.md)
- [In-game menu|implemented] **Available skill points counter**: Show unspent skill points prominently | data: AvailableSkillPoints (int >= 0) | states: zero (allocation disabled); points available (MordecaiSkillComponent.h; US-067 AC-067.8)
- [HUD|specified] **Equipped Skill A / Skill B slots**: Show the two equipped active skills and their input | data: Skill A (RB tap / Q tap), Skill B (LB tap / E tap); cooldown and SP cost per ability_system_v1 | states: ready; on cooldown; insufficient SP; Silenced (magical skills blocked) (Specs/control_bindings_v1.1.md lines 17-20; Specs/ability_system_v1.md)
- [HUD|specified] **Skill radial selector**: Pick which skill fills slot A or B | data: Equipped/available skills around a radial | states: open while RB/Q or LB/E held; selection committed; does NOT slow or pause time (Specs/control_bindings_v1.1.md lines 18, 20)
- [In-game menu|implemented] **Feats tab: summary header**: Count earned feats by tier | data: 'Feats: N unlocked — X Common, Y Rare, Z Legendary' | states: zero feats; live update on unlock (FMordecaiFeatDisplaySummary; US-068 AC-068.8; tab shows 'Coming Soon' in package (N5), planned US-093 AC-093.5)
- [In-game menu|implemented] **Unlocked feat row**: List earned feats | data: Display name, tier badge (Common gray 0.8/0.8/0.8, Rare blue 0.3/0.5/1.0, Legendary gold 1.0/0.75/0.0), short description | states: Common / Rare / Legendary; selected (MordecaiFeatDisplayWidget.cpp:41-43; US-068 AC-068.3/.4)
- [In-game menu|implemented] **Locked feat row with unlock progress**: Tease feats not yet earned and show progress toward them | data: Name visible, description '???', tier, per condition: 'StatName: Current / Threshold', 'SkillName Rank: Current / Required', 'Requires: TagName' with check/cross | states: locked; conditions partly met; becomes unlocked live (FMordecaiFeatConditionProgress; US-068 AC-068.6/.7/.9)
- [In-game menu|implemented] **Feat detail panel**: Full detail of a selected unlocked feat | data: Full description, tier, counts of granted effects, granted abilities, drawback effects | states: no selection; selected; Rare/Legendary has a drawback (only the count is shown, not the drawback text) (FMordecaiFeatAppliedRecord; US-068 AC-068.5)
- [Notification / overlay|implied] **Feat unlocked toast (achievement-style)**: Announce an earned feat when Mordecai.Event.FeatUnlocked fires | data: Feat name, tier color, short description, drawback for Rare/Legendary | states: Common / Rare / Legendary (US-012 AC-012.20; game_design_v2.md pillar 3 'Unlocked via gameplay achievements')
- [Notification / overlay|implied] **Skill milestone reached toast**: Announce a new technique at rank 1/5/10/15/20 | data: Skill name, milestone rank, technique description | states: per milestone rank (US-011 AC-011.13 (Mordecai.Event.SkillMilestone); OnSkillMilestoneReachedBP)
- [Notification / overlay|implied] **Level-up / skill points gained notice**: Tell the player a level was gained and how many skill points were added | data: New level, skill points gained | states: single; multiple levels (Specs/game_design_v2.md §5 'Leveling grants skill points')
- [HUD|implemented] **Status effect bar**: Minimal critical readout of active statuses (VFX is the primary channel) | data: Horizontal row of up to 8 indicators, bottom-left | states: empty; 1-8 indicators; overflow beyond 8 is dropped (MordecaiStatusEffectBarWidget.h (MaxVisibleIndicators = 8); US-055 AC-055.5/.6; N16 defect, fix planned in US-091)
- [HUD|implemented] **Status effect indicator chip**: Show one active status | data: Status name (10 pt white), remaining seconds '%.1f' (9 pt, 0.8 gray), tint image green (0,1,0) buff / red (1,0,0) debuff; buffs = Focused, Blessed, StoneSkin, FireWard, Restoration | states: buff; debuff; timed (countdown); infinite (no countdown, e.g. Focused); currently reads 'Status' (N16 bug) (MordecaiStatusEffectIndicatorWidget.cpp:20-108; MordecaiStatusEffectBarWidget.cpp:44-73; US-091 AC-091.1)
- [HUD|implied] **Status stack count pips**: Show escalating stacks before a threshold effect | data: Frostbitten stacks 1-5 (5 -> Frozen 2.0 s); Shocked stacks 1-3 (15% micro-stun and +20% block cost per stack) | states: per stack level; at max; cleared (MordecaiGE_Frostbitten.h (FrostbittenMaxStacks=5); MordecaiGE_Shocked.h (ShockedMaxStacks=3); US-015)
- [HUD|implied] **Status buildup meter**: Show accumulation toward a status, since the specs describe buildup thresholds | data: Buildup amount vs threshold per status (threshold scaled by CON/WIS/INT per attribute_scaling) | states: filling; triggered; decaying (Specs/attribute_scaling_v1.md 'Status Resist (buildup threshold)'; Specs/damage_types_v1.md 'BuildsStatus: Magnitude: 12'; launch set 'frost buildup')
- [HUD|implied] **Focused streak meter**: Show progress to the Focused buff and its active state | data: Consecutive perfect actions 0/3; Focused active (infinite until hit) | states: building; Focused active; broken on hit; reset on non-perfect action (MordecaiPerfectActionTracker.h (FocusedStreakThreshold = 3); US-018 AC-018.8-.14)
- [HUD|implied] **Rooted break-free prompt**: Offer the stamina-cost escape from Root | data: Break free action, cost 25 stamina | states: available; not enough stamina; input binding not yet decided (MordecaiGE_Rooted.h (RootedBreakFreeStaminaCost = 25); US-017 AC-017.9)
- [HUD|implied] **Bleeding clot timer**: Show that avoiding hits for 4 s ends Bleeding | data: Time since last hit vs 4.0 s clot time; healing -50% while bleeding | states: bleeding; clotting; refreshed by hit; clotted (MordecaiGE_Bleeding.h; US-014 AC-014.8/.9)
- [HUD|implemented] **Stamina bar tier coloring**: Show the stamina effectiveness tier | data: Fill %; tier Green >=66% (1.25x), Yellow >=33% (1.0x), Red >0% (0.75x), Exhausted <=0 (0.5x); stamina can go to -50 | states: Green (0,1,0); Yellow (1,1,0); Red (1,0,0); Exhausted (0.5,0,0); Fear amplifies penalties x1.5; Focused forgives penalties (MordecaiStaminaBarWidget.h; MordecaiStaminaSystem.h; design_decisions_log.md 2026-03-19)
- [World-space|specified] **Per-status VFX/audio reads (16)**: Primary channel for status communication ('not icon soup') | data: Burning 'Flames + ember trail + sizzling'; Bleeding 'Dark-red seep + heartbeat thump'; Poisoned 'Green haze + labored breathing'; Frostbitten 'Ice crust + shiver + frosty footsteps'; Shocked 'Crackling arcs + sharp zap pops'; Weakened 'Dim aura + dulled impact sound'; Brittle 'Hairline crack sheen + glassy tinks'; Silenced 'Void ring/muzzle effect + muffled SFX'; Rooted 'Vines/ice gripping legs + scraping'; Blinded 'Dark vignette + smeared highlights'; Fear 'Shaky stance + low rumble + heartbeat'; Cursed 'Black smoke + whispers'; Exposed 'Glowing target sigil hovering in-world'; Corroded 'Pitting/steam + sizzling'; Drenched 'Water dripping + squelch footsteps'; Focused 'Clean whoosh + subtle time dilation' | states: on player; on enemies; Blinded is screen-space (vignette) (Specs/status_effects_launch_set_v1.md 'Visual/Audio Read' column; game_design_v2.md §9)
- [World-space|specified] **Exposed target sigil**: Mark that the next hit deals +30% and is consumed | data: Glowing sigil above the target | states: active (5.0 s or 1 hit); consumed; removed by perfect block (Specs/status_effects_launch_set_v1.md Exposed row; MordecaiGE_Exposed.h)
- [World-space|implied] **Incapacitation reads: Frozen / MicroStunned / Sleeping / Staggered / Knocked Down**: Show that an actor cannot act and is open to punishment | data: Frozen 2.0 s (cannot move, attack, block or dodge); MicroStunned 0.3 s; Sleeping (wakes on damage); PostureBroken stagger 1000 ms x1.5; KnockedDown x2.0 | states: player-side; enemy-side (punish window) (MordecaiGameplayTags.cpp:19,31,87-88,119; design_decisions_log.md 2026-03-19)
- [Notification / overlay|implied] **Immune / resisted feedback**: Tell the player a status did not apply | data: 'Immune' or a resisted marker per status | states: immune (Mordecai.Immunity.<Status>); doused (Drenched removes Burning); cleansed (US-013 AC-013.8/.11; US-018 AC-018.5/.6)
- [In-game menu|implied] **Status codex / glossary**: Teach each status's identity, counterplay and cleanses | data: Per status: Identity, Mechanical Effect, Skillful Counterplay, Typical Sources, Cleanses/Mitigation | states: discovered / undiscovered (not specified) (Specs/status_effects_launch_set_v1.md columns)
- [Main menu / front end|implied] **Character creation attribute setup**: Set the starting attributes | data: 9 attributes, default 10 each | states: not specified anywhere (MordecaiCharacterSheetTypes.h comment 'e.g. 10 at character creation'; game_design_v2.md §5 'Generally static')

## KEY TERMS
- STR / DEX / END / CON / RES / DIS / INT / WIS / CHA — the 9 primary attributes (Strength, Dexterity, Endurance, Constitution, Resistance, Discipline, Intelligence, Wisdom, Charisma); default 10 each
- Physical / Resilience / Magical — the three character-sheet groups (STR DEX END / CON RES DIS / INT WIS CHA)
- Effective Mod — diminishing-returns value used for all attribute scaling; bands 1-5 100%, 6-10 70%, 11-15 45%, 16+ 25%
- Health — pool shown on the HUD as % (0–100); base 100
- Stamina — tiered pool; low stamina reduces effectiveness but never blocks actions; can go negative to -50
- Stamina tiers Green / Yellow / Red / Exhausted — >=66% 1.25x, >=33% 1.0x, >0% 0.75x, <=0 0.5x
- Spell Points (SP) — discrete points, slower regen than stamina; base 10; spells can upcast
- Upcast — spend extra Spell Points to empower a spell
- Posture — break meter; enemy posture break gives Stagger (1 s, x1.5) and Knock Down (x2.0)
- Player Stability — the player's stagger/interrupt resistance (armor + DIS + gear + skills + feats)
- Affliction Resist / Affliction Recovery — attribute-side names for status resistance and recovery (RES)
- Skill — one of the granular skills ('Everything you do is a skill'); 42 in skill_sheet_v1.1
- Rank — skill level 0-20 (0 = not learned)
- Milestone — Ranks 1/5/10/15/20 that unlock techniques/passives/interactions
- Skill points — currency spent 1 per rank; granted by leveling
- Allocate — spending a skill point on a skill
- Weapon / Defense & Armor / Movement & Exploration / Utility & Interaction / Magic — the 5 skill categories
- Skill A / Skill B — the two equipped active skill slots (RB/Q, LB/E); hold for the Radial
- Feat — earned (not picked) passive reward unlocked by gameplay achievements/patterns
- Common / Rare / Legendary — feat tiers; Rare and Legendary carry drawbacks; colors gray / blue / gold
- Drawback — the mandatory downside effect of a Rare or Legendary feat
- Unlock condition — StatThreshold, EventCount, TagPresent or SkillRank (all must be met)
- Status — a GAS status effect (Mordecai.Status.<Name>)
- DoT / Debuff / Control / Buff / Environmental — the status categories
- Immunity — tag Mordecai.Immunity.<Status> that blocks a status
- Cleanse — removal of statuses (by category, item, spell or town service)
- Buildup — accumulation toward a status (spec concept; not implemented)
- Stacks — Frostbitten 1-5 (max -> Frozen), Shocked 1-3
- Frozen — full incapacitation for 2 s at 5 frost stacks
- MicroStunned — 0.3 s interrupt from Shocked
- Focused — buff after 3 consecutive perfect actions; lasts until hit
- Perfect dodge / perfect block / perfect parry — the mastery actions (perfect window 250 ms inside a 500 ms parry window)
- Resolve / Resolved — perfect block/parry during Fear cancels Fear
- Clot — Bleeding ends after 4 s without taking damage
- Douse — Drenched removes Burning
- Break free — 25-stamina action that ends Rooted early
- Clarity window — 1000 ms of relief from Blinded after a perfect dodge
- Magic Missle — the spell name as spelled in the skill sheet
- Resistance Peneration — the stat name as misspelled in character_attributes_v1 (code: Resistance Penetration)

## OPEN QUESTIONS
- Which attribute model is canonical? Code and character_attributes_v1 use 9 attributes with Effective Mod; attribute_scaling_v1 uses 6 attributes with different stats (Block Stability, Parry Forgiveness, Cast Stability, Element DR%, Status Resist) and makes CHA a non-combat town/dialogue stat; stat_formulas_v1 uses 5 attributes with flat formulas.
- How does the player gain or change attribute points, and what are the min/max/caps? No spec says (only 'Generally static (rarely changed)'); the code has no clamp and defaults each to 10.
- What is the XP curve, and how many skill points per level? Max level 60 is fixed; the US-011 TODO(DECISION) notes 50 skills x 20 ranks = 1000 ranks.
- Should the character sheet show Level/XP, resistances per damage type, immunities and Player Stability? None of these are in the implemented sheet.
- Health is 'Displayed as % (0–100)' per game_design_v2, while the character sheet shows Health as 'Current / Max'. Which wins on which surface?
- Derived stats recalculate only when a primary attribute changes (PostAttributeChange). At startup the sheet may show +0.0% and Max Health 100, while STR..CHA = 10 would compute Effective Mod 8.5 (e.g. Max Health 151). Is an initial recalculation or init GE intended?
- Crit chances are raw chances (default 0), so the sheet's '(V-1)*100%' format would show -100%. FormatEffectiveModBonus prints the Effective Mod number as a percent. Display formats need a decision.
- status_effects_v1.md renames six launch statuses (Dazed, Slowed, Asleep, Staggered, Knocked Down, Stunned) using copied text, while game_design_v2 and the code use the launch_set 16. Which list should the status iconography cover?
- Stacking policy (agent_rules_v2 Open Item 2) is still TODO(DECISION). The HUD chip does not show stack counts for Frostbitten (1-5) or Shocked (1-3).
- Are status buildup meters wanted (specs describe buildup thresholds), or does the game keep instant application?
- How far should HUD status UI go given 'not icon soup'? The current HUD shows up to 8 text chips. Should chips use icons, and are they shown for enemies too?
- The BuffTags set omits Blurred, EnchantedWeapon and Resolved, so they would tint red as debuffs. Is that intended?
- Which buffs does Cursed disable (TODO(DECISION))?
- Skill count: 42 in v1.1, 'one asset per skill (50 total)' in code, and 'current catalog is 60' / target ~40–70 in game_design_v2. No skill data assets exist yet.
- Is the skill 'tree' a flat ranked list per category (as implemented), or should it be a branching visual tree with prerequisites? No prerequisites are specified.
- Magic: keep each spell as its own skill, or group 'by school' vs 'spell type' (open decision in game_design_v2 §12)?
- Feat content does not exist (only the Pyromaniac sample in tests). How should drawbacks be described in UI (the detail panel only counts them), and should there be an unlock toast/announcement (DCC-style achievement)?
- Feat tier colors (Common gray, Rare blue, Legendary gold) overlap with loot rarity colors (Green/Blue/Purple/Red(Gold)). Should the two systems share or separate palettes?
- skill_sheet_v1.1 data defects: Cone of Cold R1 copies Stone Skin; Fireball R1 copies Magic Missle's 'force' text; CSV artifacts in Shortbow and Magic Missle. The v1 named techniques (e.g. 'Master Riposte', 'Metroid Key Sense') were dropped. Should named techniques return for milestone UI?