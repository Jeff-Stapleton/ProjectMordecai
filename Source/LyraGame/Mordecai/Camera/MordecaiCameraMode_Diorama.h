// Project Mordecai

#pragma once

#include "Camera/LyraCameraMode.h"

#include "MordecaiCameraMode_Diorama.generated.h"

/**
 * UMordecaiCameraMode_Diorama
 *
 *	Fixed diorama-style camera for Project Mordecai.
 *	Camera follows player position smoothly but NEVER rotates.
 *	Locked design decision per agent_rules_v2.
 */
UCLASS(Blueprintable)
class LYRAGAME_API UMordecaiCameraMode_Diorama : public ULyraCameraMode
{
	GENERATED_BODY()

public:
	UMordecaiCameraMode_Diorama();

	// Designer-exposed camera properties (AC-1.2.3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Camera", Meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float PitchAngle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Camera", Meta = (ClampMin = "100.0", ClampMax = "5000.0"))
	float Distance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Camera", Meta = (ClampMin = "0.1", ClampMax = "50.0"))
	float FollowSpeed;

protected:
	virtual void UpdateView(float DeltaTime) override;

private:
	FVector SmoothedPivotLocation;
	bool bFirstUpdate;
};
