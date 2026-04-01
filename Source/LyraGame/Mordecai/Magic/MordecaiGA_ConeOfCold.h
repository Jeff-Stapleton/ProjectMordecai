// Project Mordecai — Cone of Cold Spell (US-020)

#pragma once

#include "CoreMinimal.h"
#include "MordecaiGA_SpellBase.h"
#include "Mordecai/Combat/MordecaiCombatTypes.h"

#include "MordecaiGA_ConeOfCold.generated.h"

/**
 * UMordecaiGA_ConeOfCold
 *
 *	Cone of Cold spell: performs an arc sector query hitting all enemies
 *	within ArcDegrees and Range. Applies cold damage GE to each hit target.
 *
 *	See: US-020, AC-020.4, AC-020.5, AC-020.6
 */
UCLASS()
class LYRAGAME_API UMordecaiGA_ConeOfCold : public UMordecaiGA_SpellBase
{
	GENERATED_BODY()

public:
	UMordecaiGA_ConeOfCold(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Public Getters (testable API) ---

	/** Get the damage type for this spell (AC-020.5). */
	EMordecaiDamageType GetSpellDamageType() const { return EMordecaiDamageType::Frost; }

	/** Get cone arc degrees from data asset (AC-020.4). */
	float GetConeArcDegrees() const;

	/** Get cone range from data asset (AC-020.4). */
	float GetConeRange() const;

protected:
	virtual void OnSpellCast() override;

	/** Apply cold damage to a single target via instant GE. */
	void ApplyColdDamageToTarget(AActor* TargetActor, float DamageMagnitude);
};
