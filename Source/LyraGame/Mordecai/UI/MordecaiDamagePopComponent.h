// Project Mordecai — Floating Damage Number Component (US-065)

#pragma once

#include "CoreMinimal.h"
#include "Feedback/NumberPops/LyraNumberPopComponent.h"
#include "GameplayTagContainer.h"

#include "MordecaiDamagePopComponent.generated.h"

/**
 * Tracks a single active damage pop for pool management and testing.
 */
USTRUCT(BlueprintType)
struct FMordecaiDamagePopEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY()
	int32 DamageAmount = 0;

	UPROPERTY()
	FGameplayTag DamageTypeTag;

	UPROPERTY()
	FLinearColor Color = FLinearColor::White;

	UPROPERTY()
	bool bIsCritical = false;

	UPROPERTY()
	bool bIsHealing = false;

	UPROPERTY()
	float Scale = 1.f;

	UPROPERTY()
	float SpawnWorldTime = 0.f;
};

/**
 * UMordecaiDamagePopComponent
 *
 * Manages floating damage/healing numbers. Extends Lyra's NumberPop system
 * with Mordecai-specific color coding by damage type, critical hit emphasis,
 * healing display, random scatter, and a concurrent pop pool cap.
 *
 * Attached to the player controller. For the vertical slice, damage pops
 * are fired directly on the local player controller (replication deferred).
 *
 * See: US-065, damage_types_v1.md, game_design_v2.md Section 9
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiDamagePopComponent : public ULyraNumberPopComponent
{
	GENERATED_BODY()

public:
	UMordecaiDamagePopComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ ULyraNumberPopComponent interface
	virtual void AddNumberPop(const FLyraNumberPopRequest& NewRequest) override;
	//~ End ULyraNumberPopComponent interface

	// --- Mordecai Damage Pop API ---

	/** Request a damage number pop. Called from the damage pipeline. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|DamagePop")
	void RequestDamagePop(int32 DamageAmount, const FGameplayTag& DamageTypeTag,
		const FVector& WorldLocation, bool bIsCritical);

	/** Request a healing number pop. Displayed in HealingColor with "+" prefix. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|DamagePop")
	void RequestHealingPop(int32 HealAmount, const FVector& WorldLocation);

	/** Resolve color for a given damage type tag from the DamageTypeColors map. */
	UFUNCTION(BlueprintPure, Category = "Mordecai|DamagePop")
	FLinearColor GetColorForDamageType(const FGameplayTag& DamageTypeTag) const;

	/** Apply random scatter offset within PopScatterRadius. */
	FVector ApplyScatterOffset(const FVector& BaseLocation) const;

	// --- Static utility for damage pipeline integration ---

	/**
	 * Fire a damage pop on the local player controller's pop component.
	 * Call from melee/projectile/DoT damage application.
	 * For vertical slice: fires directly on the first local player controller.
	 */
	static void BroadcastDamagePop(const UWorld* World, int32 DamageAmount,
		const FGameplayTag& DamageTypeTag, const FVector& WorldLocation, bool bIsCritical);

	/** Fire a healing pop on the local player controller's pop component. */
	static void BroadcastHealingPop(const UWorld* World, int32 HealAmount,
		const FVector& WorldLocation);

	// --- Testability ---

	const TArray<FMordecaiDamagePopEntry>& GetActivePops() const { return ActivePops; }
	int32 GetActivePopCount() const { return ActivePops.Num(); }

	// --- Configuration (AC-065.2, AC-065.5-10) ---

	/** How long each pop lives before being recycled (AC-065.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	float PopLifespanSec = 1.5f;

	/** Vertical rise speed in cm/s (AC-065.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	float PopRiseSpeed = 100.f;

	/** Base scale of pops (AC-065.2). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	float PopScale = 1.0f;

	/** Scale multiplier for critical hits (AC-065.7). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	float CriticalHitScale = 1.5f;

	/** Color for healing numbers (AC-065.8). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	FLinearColor HealingColor = FLinearColor(0.2f, 1.0f, 0.2f);

	/** Random horizontal scatter radius in cm (AC-065.9). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	float PopScatterRadius = 30.f;

	/** Maximum concurrent pops before oldest is recycled (AC-065.10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	int32 MaxConcurrentPops = 20;

	/**
	 * Color mapping from Mordecai.Damage.* tags to display colors (AC-065.5, AC-065.6).
	 * Designers can override these without code changes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|DamagePop")
	TMap<FGameplayTag, FLinearColor> DamageTypeColors;

protected:
	/** Initialize default damage type color mapping (AC-065.5). */
	void InitDefaultColors();

	/** Remove expired pops from the active list. */
	void CleanupExpiredPops(float CurrentWorldTime);

	/** Recycle the oldest pop to make room for a new one (AC-065.10). */
	void RecycleOldestPop();

	/** Internal: create and track a pop entry. */
	FMordecaiDamagePopEntry& CreatePopEntry(int32 Amount, const FGameplayTag& DamageTypeTag,
		const FVector& WorldLocation, const FLinearColor& Color, bool bIsCritical, bool bIsHealing);

	/** Color for unknown/untagged damage. */
	static const FLinearColor DefaultDamageColor;

	UPROPERTY(Transient)
	TArray<FMordecaiDamagePopEntry> ActivePops;
};
