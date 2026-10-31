// Project Mordecai — The target's defenses against an incoming melee hit (US-101, issue #53)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiCombatTypes.h"

class UAbilitySystemComponent;
class UMordecaiGA_Block;
class UMordecaiGA_Parry;

/** What the target's defenses did with a hit. */
struct FMordecaiHitDefenseResult
{
	EMordecaiHitDefense Defense = EMordecaiHitDefense::None;

	/** Health damage the block already applied (Blocked / PerfectBlocked); the caller must not apply health again. */
	float BlockedHealthDamage = 0.f;

	/** Evaded or Parried: no part of the hit lands (health, posture, payloads, status effects, damage number). */
	bool IsNegated() const { return Defense == EMordecaiHitDefense::Evaded || Defense == EMordecaiHitDefense::Parried; }

	bool IsBlocked() const { return Defense == EMordecaiHitDefense::Blocked || Defense == EMordecaiHitDefense::PerfectBlocked; }
};

/**
 * FMordecaiHitDefense
 *
 * Runs an incoming melee hit through the target's defenses before any of it is applied, in this order:
 *   1. i-frames: Mordecai.State.Invulnerable (Blink) or Mordecai.State.Dodging (AC-005.3, AC-021.3) — Evaded
 *   2. an active parry window (UMordecaiGA_Parry::ReceiveParriedHit, AC-006.13) — Parried
 *   3. an active block (UMordecaiGA_Block::ReceiveBlockedHit: mitigation, stamina drain, guard break, perfect block;
 *      AC-006.3/006.4/006.5/006.9/006.10) — Blocked / PerfectBlocked
 * Jump-avoidable sweeps are decided earlier, in the hit query (Mordecai.State.Airborne, AC-003.7).
 *
 * TODO(DECISION): melee only. Projectiles, Cone of Cold and DoT ticks do not consult the defenses yet.
 */
struct LYRAGAME_API FMordecaiHitDefense
{
	/**
	 * Resolve a hit of IncomingHealthDamage (positive) from AttackerASC on TargetASC landing at HitTime (world seconds).
	 * Parry and block apply their own effects here: the parry's attacker posture and tags, the block's mitigated
	 * health damage, stamina drain, guard break and perfect-block posture.
	 */
	static FMordecaiHitDefenseResult ResolveIncomingHit(
		UAbilitySystemComponent* TargetASC,
		UAbilitySystemComponent* AttackerASC,
		float IncomingHealthDamage,
		double HitTime);

	/** The ASC's active parry instance (Blueprint subclasses included), or null. */
	static UMordecaiGA_Parry* FindActiveParry(const UAbilitySystemComponent* ASC);

	/** The ASC's active block instance (Blueprint subclasses included), or null. */
	static UMordecaiGA_Block* FindActiveBlock(const UAbilitySystemComponent* ASC);
};
