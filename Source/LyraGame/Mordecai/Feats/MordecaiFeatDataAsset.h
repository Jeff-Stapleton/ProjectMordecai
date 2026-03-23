// Project Mordecai — Feat Data Asset (US-012)

#pragma once

#include "Engine/DataAsset.h"
#include "Mordecai/Feats/MordecaiFeatTypes.h"

#include "MordecaiFeatDataAsset.generated.h"

class UGameplayEffect;
class UGameplayAbility;

/**
 * UMordecaiFeatDataAsset
 *
 *	Static data for a single feat. One asset per feat.
 *	Contains tier, unlock conditions, rewards, and drawbacks.
 *	Extends UPrimaryDataAsset for runtime querying via the Asset Manager.
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiFeatDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique feat identifier (e.g. "Pyromaniac"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	FName FeatName;

	/** Localized display name shown in UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	FText DisplayName;

	/** Localized description of the feat's effects. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	FText Description;

	/** Rarity tier — determines drawback application rules. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	EMordecaiFeatTier Tier = EMordecaiFeatTier::Common;

	/** Gameplay effects granted when the feat is unlocked (positive). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	TArray<TSubclassOf<UGameplayEffect>> GrantedEffects;

	/** Gameplay abilities granted when the feat is unlocked. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

	/** Drawback effects applied for Rare/Legendary feats (skipped for Common). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	TArray<TSubclassOf<UGameplayEffect>> DrawbackEffects;

	/** Gameplay tag granted to the character when this feat is unlocked (Mordecai.Feat.<Name>). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	FGameplayTag FeatTag;

	/** All conditions that must be met (AND logic) for the feat to auto-unlock. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Feats")
	TArray<FMordecaiFeatCondition> UnlockConditions;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
