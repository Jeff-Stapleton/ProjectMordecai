# US-065: Floating Damage Numbers

## Overview
Integrate Lyra's NumberPops system to display floating damage numbers when attacks hit. Numbers appear at the hit location in world-space, rise upward, and fade out. Color-coded by Mordecai damage type for instant readability from the diorama camera. Critical hits get emphasized treatment. Healing shows as green positive numbers.

## Execution Mode
**Mode:** HEADLESS

## References
- Lyra NumberPops: `Source/LyraGame/Feedback/NumberPops/` — `LyraNumberPopComponent`, `FLyraNumberPopRequest`, `LyraDamagePopStyle`, `LyraNumberPopComponent_MeshText`
- Design doc: `damage_types_v1.md` — damage type taxonomy (Physical, Fire, Frost, Lightning, Poison, Arcane, Holy, Dark)
- Design doc: `game_design_v2.md` Section 9 — clean UI
- Existing code: `UMordecaiGA_MeleeAttack` (US-004), `AMordecaiProjectile` (US-009), Mordecai damage execution/application pipeline
- Agent rules: `agent_rules_v2.md` — fixed diorama camera

---

## Acceptance Criteria

### Damage Pop Component
- [x] AC-065.1: `UMordecaiDamagePopComponent` (UControllerComponent) exists in `Mordecai/UI/`. Attached to the player controller. Manages floating number display for all damage/healing events visible to this player.
- [x] AC-065.2: Extends or wraps Lyra's `ULyraNumberPopComponent_MeshText`. Configurable properties: `PopLifespanSec` (float, default 1.5), `PopRiseSpeed` (float, default 100.0 cm/s), `PopScale` (float, default 1.0).

### Damage Pipeline Integration
- [x] AC-065.3: When any actor takes damage from a Mordecai attack (melee hit, projectile hit, DoT tick), a damage pop request is fired. The request includes: damage amount (int32), damage type tag (`Mordecai.Damage.*`), world location (hit point or target center mass), and whether it was a critical hit (`bIsCriticalDamage`).
- [x] AC-065.4: Integration hook: the damage pop is triggered from the target's damage-received handler or the Mordecai damage execution. Fires on the server/authority and is communicated to the owning player controller's pop component. For vertical slice: fire directly on the local player controller (replication deferred).

### Color Coding
- [x] AC-065.5: Damage numbers are color-coded by damage type tag:
  - `Mordecai.Damage.Physical` → White (1.0, 1.0, 1.0)
  - `Mordecai.Damage.Fire` → Orange (1.0, 0.5, 0.0)
  - `Mordecai.Damage.Frost` → Cyan (0.3, 0.8, 1.0)
  - `Mordecai.Damage.Lightning` → Yellow (1.0, 1.0, 0.3)
  - `Mordecai.Damage.Poison` → Green (0.3, 0.9, 0.3)
  - `Mordecai.Damage.Arcane` → Purple (0.7, 0.3, 1.0)
  - `Mordecai.Damage.Holy` → Gold (1.0, 0.85, 0.0)
  - `Mordecai.Damage.Dark` → Dark Red (0.6, 0.1, 0.1)
  - Default/Unknown → Gray (0.7, 0.7, 0.7)
- [x] AC-065.6: Color mapping is stored in a configurable `TMap<FGameplayTag, FLinearColor> DamageTypeColors` UPROPERTY on the pop component. Designers can override colors without code changes.

### Visual Behavior
- [x] AC-065.7: Critical hits display at `CriticalHitScale` (default 1.5x) with a configurable `CriticalHitColor` tint (default: keep damage type color but add a bright outline/glow effect, or simply use larger scale). The `bIsCriticalDamage` flag from the request drives this.
- [x] AC-065.8: Healing numbers display in a distinct `HealingColor` (default Green: 0.2, 1.0, 0.2) with a "+" prefix to distinguish from damage numbers.
- [x] AC-065.9: Numbers have slight random horizontal scatter: `PopScatterRadius` (default 30.0cm) applied as a random X/Y offset to the world location so multiple simultaneous hits don't stack on the same pixel.

### Performance
- [x] AC-065.10: Maximum concurrent pops capped at configurable `MaxConcurrentPops` (int32, default 20). When the limit is exceeded, the oldest active pop is recycled to display the new number. Pops are pooled and reused after their lifespan expires.

## Technical Notes
- **Lyra's NumberPops architecture:** `FLyraNumberPopRequest` has `WorldLocation`, `SourceTags`, `TargetTags`, `NumberToDisplay`, `bIsCriticalDamage`. The SourceTags can carry the damage type tag for color routing.
- **MeshText rendering:** `LyraNumberPopComponent_MeshText` renders digits using 3D static meshes with dynamic material instances. It has built-in pooling. Extend this for Mordecai's color-coding needs.
- **Integration point:** The cleanest hook is a gameplay event `Mordecai.Event.DamageDealt` fired from the damage execution or `OnDamageReceived` handler. The pop component listens for this event and creates pop requests. Alternatively, a direct function call from the damage pipeline.
- **Horizontal scatter:** Add `FMath::VRandCone` or simple random XY offset within `PopScatterRadius` to the request's WorldLocation.
- **Healing detection:** If `NumberToDisplay` is negative or a `Mordecai.Event.HealingReceived` event is used, display as healing. Alternatively, a boolean flag `bIsHealing` on the request.
- **NullRHI compatibility:** MeshText rendering won't visually display under NullRHI, but the pop request creation, color lookup, pooling, and lifecycle logic can all be tested headlessly by verifying component state.
- Place code in `Source/LyraGame/Mordecai/UI/`.

## Tests Required
- [x] `Mordecai.UI.DamagePop.FiresOnDamage` — Taking damage triggers a damage pop request on the pop component (AC-065.3)
- [x] `Mordecai.UI.DamagePop.CorrectDamageAmount` — Pop request contains the correct damage number (AC-065.3)
- [x] `Mordecai.UI.DamagePop.ColorMatchesPhysical` — Physical damage maps to White color (AC-065.5)
- [x] `Mordecai.UI.DamagePop.ColorMatchesFire` — Fire damage maps to Orange color (AC-065.5)
- [x] `Mordecai.UI.DamagePop.ColorMatchesFrost` — Frost damage maps to Cyan color (AC-065.5)
- [x] `Mordecai.UI.DamagePop.CustomColorOverride` — Setting a custom color in the map overrides the default (AC-065.6)
- [x] `Mordecai.UI.DamagePop.CriticalHitScalesUp` — Critical hit flag produces pop at CriticalHitScale (AC-065.7)
- [x] `Mordecai.UI.DamagePop.HealingShowsGreen` — Healing events produce green pops with positive display (AC-065.8)
- [x] `Mordecai.UI.DamagePop.ScatterOffset` — Multiple pops at the same target have different world locations within ScatterRadius (AC-065.9)
- [x] `Mordecai.UI.DamagePop.MaxConcurrentRecycles` — Exceeding MaxConcurrentPops recycles the oldest pop (AC-065.10)

---

## Definition of Done
- [x] All tests written and failing (red phase confirmed)
- [x] All implementation complete
- [x] All tests passing (green)
- [x] Project compiles with zero errors
- [x] Code committed and pushed with `[US-065]` prefix
