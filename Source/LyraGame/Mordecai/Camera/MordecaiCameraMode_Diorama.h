// Project Mordecai

#pragma once

#include "Camera/LyraCameraMode.h"
#include "Engine/EngineTypes.h"

#include "MordecaiCameraMode_Diorama.generated.h"

class AActor;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UMeshComponent;

struct FMordecaiOccluderFadeState
{
	float Progress = 1.0f;
	float LastAppliedOpacity = 1.0f;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>, TInlineAllocator<2>> Materials;
};

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

	/** ULyraCameraMode lifecycle hook. Public so the world automation test can exercise cleanup directly. */
	virtual void OnDeactivation() override;

protected:
	virtual void UpdateView(float DeltaTime) override;

private:
	void UpdateOcclusionFades(float DeltaTime, AActor* TargetActor);
	void RestoreOccluders();
	void SetOcclusionTarget(AActor* TargetActor);
	bool CacheOccluderMaterials(UMeshComponent* Mesh, FMordecaiOccluderFadeState& OutState);
	static void ApplyOccluderOpacity(FMordecaiOccluderFadeState& State, float Opacity);

	UFUNCTION()
	void HandleOcclusionTargetEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	FVector SmoothedPivotLocation;
	bool bFirstUpdate;
	TWeakObjectPtr<AActor> OcclusionTarget;
	TMap<TWeakObjectPtr<UMeshComponent>, FMordecaiOccluderFadeState> OccluderFadeStates;
	TSet<TWeakObjectPtr<UMeshComponent>> WarnedInvalidOccluders;
};
