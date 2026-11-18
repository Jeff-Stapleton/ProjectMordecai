// Project Mordecai — Shield block-arc data (US-121, issue #71)

#pragma once

#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"

#include "MordecaiShieldDataAsset.generated.h"

/**
 * A shield is off-hand equipment in the existing weapon/equipment framework.
 * Its arc is data-driven so individual shield assets can tune their frontal coverage.
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiShieldDataAsset : public UMordecaiWeaponDataAsset
{
	GENERATED_BODY()

public:
	UMordecaiShieldDataAsset();

	static constexpr float BucklerDefaultBlockArcDegrees = 150.f;
	static constexpr float StandardDefaultBlockArcDegrees = 180.f;
	static constexpr float TowerDefaultBlockArcDegrees = 210.f;

	/** Full defensive arc centred on actor forward, in degrees. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Shield", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float BlockArcDegrees = StandardDefaultBlockArcDegrees;
};
