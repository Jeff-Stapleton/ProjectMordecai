// Project Mordecai — Spell Data Asset (US-019)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MordecaiSpellTypes.h"

#include "MordecaiSpellDataAsset.generated.h"

/**
 * UMordecaiSpellDataAsset
 *
 *	Data-driven description of a spell. Designers can create and tune
 *	spell definitions without code changes. Used by UMordecaiGA_SpellBase
 *	and its subclasses for all spell parameters.
 *
 *	See: ability_schema_v1.md, ability_system_v1.md
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiSpellDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// --- Identity ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Spell")
	FName SpellId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Spell")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Spell")
	FGameplayTag School;

	// --- Cost & Cooldown ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Cost")
	int32 SpellPointCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Cooldown")
	float CooldownDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Cooldown")
	FGameplayTag CooldownTag;

	// --- Timing ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	float CastTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	float WindupTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	float RecoveryTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	bool bInterruptible = true;

	// --- Movement ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Movement")
	EMordecaiMovementPolicy MovementPolicy = EMordecaiMovementPolicy::FreeMove;

	// --- Targeting ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Targeting")
	EMordecaiSpellTargetingType TargetingType = EMordecaiSpellTargetingType::Self;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Targeting")
	float Range = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Targeting")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Targeting")
	float ArcDegrees = 0.f;

	// --- Power & Scaling ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Power")
	float BasePower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Power")
	TArray<FMordecaiSpellScalingStat> ScalingStats;

	// --- Delivery ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Delivery")
	EMordecaiSpellDeliveryType DeliveryType = EMordecaiSpellDeliveryType::InstantApply;

	// --- Upcasting ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Upcast")
	float UpcastCostMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Upcast")
	float UpcastPowerMultiplier = 1.5f;
};
