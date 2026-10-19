# Player Attacks — Agent Implementation Brief v1
_Last updated: 2026-03-03 (America/Denver)_

## 0) Goal
Implement player attacks for a **top-down diorama ARPG** where readability comes from **simplified swing paths** and clear counterplay:

- **Sweeps**: wide arcs, best vs groups, can be **jumped over**.
- **Thrusts & Slams**: narrow lanes/points, cannot be jumped (they hit airborne), easier to **side-dodge**.
- Weapons/spells can trigger **secondary effects** on use or on hit.
- Projectiles use **soft aim assist** to “do what the player meant” without lock-on.
- AoE supports **self-origin**, **attached aura**, and **ground-targeted** (rooted targeting mode).

## 1) Attack Taxonomy (Core Types)

### 1.1 Melee Attacks (Diorama Shapes)
Melee attacks are simplified into 3 canonical shapes:

- **Sweep**: arc sector in front of attacker  
  - Pro: multi-target, good for crowds  
  - Counterplay: **jump** if tagged low; dodge is possible but wide arcs can clip dodge end
- **Thrust**: narrow rectangle/capsule forward (lane)  
  - Pro: precise, strong vs single target/lines  
  - Counterplay: **horizontal dodge** / spacing; cannot be jumped
- **Slam**: point impact (circle), optional shockwave ring  
  - Pro: posture impact / AoE at point  
  - Counterplay: exit the point; cannot be jumped

**Rule: Jump interaction**  
- Sweeps: `JumpAvoidable=true` (low sweep)  
- Thrusts/Slams: `HitsAirborne=true`

### 1.2 Projectile Attacks
Projectiles include arrows, bolts, fireballs, lightning, etc.

- Use **soft aim assist** (no lock-on).
- Can support **pierce**, **ricochet**, **AoE on hit**, and **status payloads**.
- Profiles vary strongly by family (longbow vs shortbow vs lightning bolt, etc.).

### 1.3 AoE Attacks
AoE origin modes:

1. **Self-origin**: spawns at character position (e.g., stomp).
2. **Attached aura**: follows the character for duration (e.g., storm field).
3. **Ground-targeted**: player becomes **rooted**, moves a reticle, releases to cast.

## 2) Required Data Model (Build This First)

### 2.1 `AttackProfile` (data-driven)
All attacks should be describable by data so designers can tune without code changes.

**Minimum fields**
- `AttackType`: `MeleeSweep | MeleeThrust | MeleeSlam | Projectile | AoE`
- `Input`: `Light | Heavy | SkillA | SkillB | Throw`
- `WindupTimeMs`, `ActiveTimeMs`, `RecoveryTimeMs`
- `RootedDuring`: `None | Windup | Active | Full`
- `HitShape`: `ArcSector | Capsule | Box | Circle | ProjectileSpec | AoESpec`
- `HitsAirborne`: bool
- `JumpAvoidable`: bool
- `DamageScalar`
- `PostureDamageScalar`
- `StaminaCost`
- `OnHitPayload`: statuses, AoE-on-hit, spawn effects, etc.
- `OnUsePayload`: effects triggered on use (even on whiff if desired)
- `ComboIndex`: 1..N (light chains)
- `CancelableInto`: `Dodge? Block?` (default: committed)

### 2.2 Hit resolution / Target filtering
When resolving hits:
1. Gather targets in shape.
2. Apply: damage, posture damage, statuses, hit reactions (stagger/knockback).

Filters:
- enemies only / destructibles / interactables
- friendly fire toggle (default off)

## 3) Melee Implementation Details

### 3.1 Sweep (Arc Sector)
- Shape: sector defined by `radius`, `angleDegrees`, `startAngleOffset`.
- Tagged as low sweep: `JumpAvoidable=true`.
- Dodging uses i-frames, but wide arcs may clip if dodge ends inside the active arc (intended difficulty).

**Agent tasks**
- Implement `ArcSectorOverlapQuery(origin, forward, radius, angle)`.
- If `JumpAvoidable` and target `IsAirborne`, ignore hit.

### 3.2 Thrust (Narrow Lane)
- Shape: capsule/box forward.
- `HitsAirborne=true` → hits jumping targets.
- Counterplay: side dodge / spacing.

**Agent tasks**
- Implement `CapsuleOverlapQuery(origin, forward, length, width)`.
- Apply hits to airborne targets.

### 3.3 Slam (Point / Shockwave)
- Primary: circle at impact point.
- Optional: expanding ring shockwave.

**Agent tasks**
- Implement `CircleOverlapQuery(impactPoint, radius)`.
- For shockwave: expanding ring collider or stepped-radius pulses.

### 3.4 Secondary effects (Weapon-triggered abilities)
Weapons can spawn abilities on use or on hit:

- Hammer slam → shockwave AoE at impact
- Sword spin → persistent fire tornado hazard

**Trigger rules**
- `TriggerOn=OnUse` (always fires)
- `TriggerOn=OnHit` (only on hit connect)
- `TriggerOn=OnPerfect` (optional future extension)

## 4) Weapon Class Attack Profiles (Examples)

### Longsword
- **Light chain (3):** fast **sweeps**
  1) medium arc sweep  
  2) slightly wider arc sweep  
  3) forward step + sweep finisher
- **Heavy:** charged **360 spin sweep**
  - large posture, longer recovery
  - optional `OnUse` elemental followup from weapon affix

### Rapier
- **Light chain (5):** quick **thrust** series (narrow lane)
- **Heavy:** charged **lunge thrust**
  - includes forward movement impulse
  - intended commitment (no easy cancel) for skill-based risk

**Agent tasks**
- Create initial data assets for these weapon classes.
- Hook hit windows via animation notifies during `ActiveTime`.

## 5) Projectiles + Soft Aim Assist (No Lock-On)

### 5.1 Soft aim assist — fan-out traces
Purpose: if the player aims “basically at” an enemy, projectiles go there.

**Algorithm**
1. Build candidate directions around aim forward:
   - 0°, ±2°, ±4°, ±6° (tunable)
2. Raycast each direction out to `AssistRange`.
3. If valid targets hit:
   - choose best by smallest angle offset, then closest target.
4. Slightly rotate firing direction toward chosen candidate **before spawning projectile**.
5. If no candidates, fire straight.

**Constraints**
- Subtle assistance only (few degrees).
- No snapping/lock-on behavior.

### 5.2 `ProjectileSpec`
- `Speed`, `GravityDrop` (optional), `Lifetime`
- `PierceCount`
- `RicochetCount`, `RicochetRange`, `RicochetAngleLimit`
- `OnHitAoE` (radius + damage scalar)
- `OnHitStatuses`

### Examples
- **Longbow:** slow, piercing; rooted while drawing; hold to charge.
- **Shortbow:** rapid, non-piercing; hold to root + unleash flurry.
- **Lightning bolt:** ricochets to nearby targets; charge increases ricochet count.

**Agent tasks**
- Implement aim-assist module + tests.
- Implement projectile pipeline: spawn → move → collide → apply hit → optional pierce/ricochet.

## 6) AoE Attacks (Self / Aura / Ground-Targeted)

### 6.1 Origin modes
A) **Self-origin**: spawns at player instantly.  
B) **Attached aura**: follows player, ticks/pulses.  
C) **Ground-targeted**: enter targeting mode (rooted), move reticle, release to cast.

### 6.2 Ground targeting controls
- **Controller:** hold skill → reticle mode; move reticle with **Right Stick**; release to cast.
- **KBM:** hold key → reticle mode; reticle follows mouse; release to cast.

**Agent tasks**
- Implement `TargetingMode` state machine:
  - `EnterTargeting(mode, range, reticleShape)`
  - `UpdateReticle(input)`
  - `CommitCast(onRelease)`
  - `CancelCast` (optional: dodge to cancel)

## 7) First Playtest Slice (Minimum)
Implement for initial playtest:
- Longsword (3 sweep chain + charged spin)
- Rapier (5 thrust chain + charged lunge)
- One projectile (Longbow charged pierce)
- One ground AoE (bomb) + optional aura AoE
- Aim assist (fan-out traces)
- Jump rules: sweep avoidable; thrust/slam hits airborne

## 8) Acceptance Criteria
- Sweeps reliably hit groups and are **jump-avoidable** when tagged.
- Thrusts/slams hit airborne targets and are **side-dodge friendly**.
- Aim assist subtly corrects shots within a few degrees; never feels like lock-on.
- Weapon classes feel distinct from frame 1.
- Data-driven profiles allow tuning without code changes.
