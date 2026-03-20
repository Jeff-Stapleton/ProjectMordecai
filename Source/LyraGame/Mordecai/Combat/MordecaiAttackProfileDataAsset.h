// Project Mordecai — Attack Profile Data Asset (US-002)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MordecaiCombatTypes.h"

#include "MordecaiAttackProfileDataAsset.generated.h"

class UGameplayEffect;

/**
 * UMordecaiAttackProfileDataAsset
 *
 * Data-driven description of an attack. Designers can create and tune
 * attack profiles without code changes. Used by melee, projectile, and
 * AoE attacks alike.
 *
 * See: attack_taxonomy_v1.md Section 2.1
 */
UCLASS(BlueprintType)
class LYRAGAME_API UMordecaiAttackProfileDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// --- Core identity (AC-002.9) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Attack")
	EMordecaiAttackType AttackType = EMordecaiAttackType::MeleeSweep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Attack")
	EMordecaiInputSlot InputSlot = EMordecaiInputSlot::Light;

	// --- Timing ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	float WindupTimeMs = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	float ActiveTimeMs = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	float RecoveryTimeMs = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Timing")
	EMordecaiRootedMode RootedDuring = EMordecaiRootedMode::None;

	// --- Hit shape ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|HitShape")
	EMordecaiHitShapeType HitShapeType = EMordecaiHitShapeType::ArcSector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|HitShape")
	FMordecaiHitShapeParams HitShapeParams;

	// --- Airborne interaction ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Interaction")
	bool HitsAirborne = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Interaction")
	bool JumpAvoidable = false;

	// --- Damage ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage")
	FMordecaiDamageProfile DamageProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage")
	float PostureDamageScalar = 1.f;

	// --- Cost ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Cost")
	float StaminaCost = 0.f;

	// --- Combo ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Combo")
	int32 ComboIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Combo")
	bool CancelableIntoDodge = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Combo")
	bool CancelableIntoBlock = false;

	// --- Payloads (AC-002.10) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Payload")
	TArray<TSubclassOf<UGameplayEffect>> OnHitPayload;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Payload")
	TArray<TSubclassOf<UGameplayEffect>> OnUsePayload;

	// --- Projectile (only relevant when AttackType == Projectile) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	FMordecaiProjectileSpec ProjectileSpec;
};
