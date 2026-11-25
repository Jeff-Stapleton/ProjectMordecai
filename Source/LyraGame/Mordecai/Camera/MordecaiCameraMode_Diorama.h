// Project Mordecai

#pragma once

#include "Camera/LyraCameraMode.h"

#include "MordecaiCameraMode_Diorama.generated.h"

class UPrimitiveComponent;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mordecai|Camera|Occlusion", Meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float OcclusionFadeDuration;

	/** Pure helpers shared by runtime occlusion and headless tests. */
	static float AdvanceOcclusionFade(float CurrentFade, bool bBlockingPlayer, float DeltaTime, float FadeDuration);
	static bool IsOcclusionFadeCandidate(const UPrimitiveComponent* Primitive);

protected:
	virtual void UpdateView(float DeltaTime) override;

private:
	FVector SmoothedPivotLocation;
	bool bFirstUpdate;
};
