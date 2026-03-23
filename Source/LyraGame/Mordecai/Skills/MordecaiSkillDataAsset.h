// Project Mordecai — Skill Data Asset (US-011)

#pragma once

#include "Engine/DataAsset.h"
#include "Mordecai/Skills/MordecaiSkillTypes.h"

#include "MordecaiSkillDataAsset.generated.h"

class UGameplayAbility;

/**
 * UMordecaiSkillDataAsset
 *
 *	Static data for a single skill. One asset per skill (50 total).
 *	Contains category, milestone descriptions, and milestone ability mappings.
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiSkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique skill identifier (e.g. "Longswords", "Dodging"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Skills")
	FName SkillName;

	/** Which of the 5 skill categories this belongs to. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Skills")
	EMordecaiSkillCategory Category = EMordecaiSkillCategory::Weapon;

	/** Milestone rank descriptions (keys: 1, 5, 10, 15, 20). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Skills")
	TMap<int32, FText> RankDescriptions;

	/** Gameplay abilities granted at each milestone rank (keys: 1, 5, 10, 15, 20). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Skills")
	TMap<int32, TSubclassOf<UGameplayAbility>> MilestoneAbilities;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
