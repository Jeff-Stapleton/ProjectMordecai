// Project Mordecai — Blink Spell (US-021)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"

#include "MordecaiGA_Blink.generated.h"

/**
 * UMordecaiGA_Blink
 *
 *	Short-range teleport spell. On cast, teleports the caster forward in
 *	their aim direction by Range cm. A line trace prevents teleporting
 *	through walls (clamped to hit - 50cm buffer). Grants brief
 *	invulnerability (Mordecai.State.Invulnerable tag) for InvulnerabilityDuration.
 *
 *	Delivery: DashMovement. Near-instant cast (WindupTime ≈ 0, CastTime ≈ 0).
 *
 *	See: US-021, AC-021.1 through AC-021.4
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_Blink : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_Blink(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Wall-clamp buffer distance in cm (AC-021.2). */
	static constexpr float WallBuffer = 50.f;

	// --- Public Getters (testable API) ---

	/** Get teleport range from SpellDataAsset (AC-021.1). */
	float GetTeleportRange() const;

	/** Get invulnerability duration from SpellDataAsset BuffDuration (AC-021.3). */
	float GetInvulnerabilityDuration() const;

	/** Get the invulnerability tag (AC-021.3). */
	FGameplayTag GetInvulnerabilityTag() const;

	/**
	 * Compute teleport destination with wall clamping (AC-021.1, AC-021.2).
	 * Pure static function for testability.
	 *
	 * @param Origin       Caster's current location
	 * @param AimDirection Normalized aim direction
	 * @param Range        Teleport range in cm
	 * @param bWallHit     Whether the pre-teleport trace hit geometry
	 * @param WallHitPoint Hit location (only valid if bWallHit is true)
	 * @return Final teleport destination
	 */
	static FVector ComputeTeleportDestination(
		const FVector& Origin,
		const FVector& AimDirection,
		float Range,
		bool bWallHit,
		const FVector& WallHitPoint);

protected:
	virtual void OnSpellCast() override;

private:
	void ApplyInvulnerabilityTag();
	void RemoveInvulnerabilityTag();
	void OnInvulnerabilityExpired();

	FTimerHandle InvulnerabilityTimerHandle;
};
