// Project Mordecai — Bleeding Status Effect GE (US-014)

#pragma once

#include "Mordecai/StatusEffects/MordecaiStatusEffectGameplayEffect.h"

#include "MordecaiGE_Bleeding.generated.h"

/**
 * UMordecaiGE_Bleeding
 *
 *	Bleeding status effect: healing reduction + hit-refresh + clot mechanic.
 *	- Applies Mordecai.Status.Bleeding tag for duration
 *	- Reduces HealingReceivedMultiplier by BleedingHealReductionPct
 *	- Hit-refresh: taking damage resets Bleeding duration
 *	- Clot: if no damage for BleedingClotTimeSec, Bleeding is removed early
 *	- Blocked by Mordecai.Immunity.Bleeding
 *
 *	Hit-refresh and clot mechanics are managed by UMordecaiStatusEffectComponent.
 */
UCLASS()
class LYRAGAME_API UMordecaiGE_Bleeding : public UMordecaiStatusEffectGameplayEffect
{
	GENERATED_BODY()

public:
	UMordecaiGE_Bleeding();

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Bleeding")
	float BleedingDurationSec = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Bleeding")
	float BleedingHealReductionPct = 0.50f;

	UPROPERTY(EditDefaultsOnly, Category = "Mordecai|Bleeding")
	float BleedingClotTimeSec = 4.0f;
};
