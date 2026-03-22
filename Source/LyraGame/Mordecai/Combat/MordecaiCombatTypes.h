// Project Mordecai — Combat Data Types (US-002)

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "MordecaiCombatTypes.generated.h"

// ---------------------------------------------------------------------------
// AC-002.1: EMordecaiAttackType
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiAttackType : uint8
{
	MeleeSweep   UMETA(DisplayName = "Melee Sweep"),
	MeleeThrust  UMETA(DisplayName = "Melee Thrust"),
	MeleeSlam    UMETA(DisplayName = "Melee Slam"),
	Projectile   UMETA(DisplayName = "Projectile"),
	AoE          UMETA(DisplayName = "AoE"),
};

// ---------------------------------------------------------------------------
// AC-002.2: EMordecaiInputSlot
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiInputSlot : uint8
{
	Light   UMETA(DisplayName = "Light"),
	Heavy   UMETA(DisplayName = "Heavy"),
	SkillA  UMETA(DisplayName = "Skill A"),
	SkillB  UMETA(DisplayName = "Skill B"),
	Throw   UMETA(DisplayName = "Throw"),
};

// ---------------------------------------------------------------------------
// AC-002.3: EMordecaiRootedMode
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiRootedMode : uint8
{
	None    UMETA(DisplayName = "None"),
	Windup  UMETA(DisplayName = "Windup"),
	Active  UMETA(DisplayName = "Active"),
	Full    UMETA(DisplayName = "Full"),
};

// ---------------------------------------------------------------------------
// AC-002.4: EMordecaiHitShapeType
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiHitShapeType : uint8
{
	ArcSector  UMETA(DisplayName = "Arc Sector"),
	Capsule    UMETA(DisplayName = "Capsule"),
	Box        UMETA(DisplayName = "Box"),
	Circle     UMETA(DisplayName = "Circle"),
};

// ---------------------------------------------------------------------------
// AC-002.5: EMordecaiDamageType
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiDamageType : uint8
{
	Physical   UMETA(DisplayName = "Physical"),
	Slash      UMETA(DisplayName = "Slash"),
	Pierce     UMETA(DisplayName = "Pierce"),
	Blunt      UMETA(DisplayName = "Blunt"),
	Fire       UMETA(DisplayName = "Fire"),
	Frost      UMETA(DisplayName = "Frost"),
	Lightning  UMETA(DisplayName = "Lightning"),
	Poison     UMETA(DisplayName = "Poison"),
	Corrosive  UMETA(DisplayName = "Corrosive"),
	Arcane     UMETA(DisplayName = "Arcane"),
	Shadow     UMETA(DisplayName = "Shadow"),
	Radiant    UMETA(DisplayName = "Radiant"),
};

// ---------------------------------------------------------------------------
// US-004: EMordecaiAttackPhase — phase of a melee attack sequence
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiAttackPhase : uint8
{
	None     UMETA(DisplayName = "None"),
	Windup   UMETA(DisplayName = "Windup"),
	Active   UMETA(DisplayName = "Active"),
	Recovery UMETA(DisplayName = "Recovery"),
};

// ---------------------------------------------------------------------------
// US-005: EMordecaiDodgePhase — phase of a dodge action
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiDodgePhase : uint8
{
	None     UMETA(DisplayName = "None"),
	Active   UMETA(DisplayName = "Active"),
	Cooldown UMETA(DisplayName = "Cooldown"),
};

// ---------------------------------------------------------------------------
// US-006: EMordecaiBlockPhase — phase of a block action
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiBlockPhase : uint8
{
	None         UMETA(DisplayName = "None"),
	Active       UMETA(DisplayName = "Active"),
	GuardBroken  UMETA(DisplayName = "Guard Broken"),
};

// ---------------------------------------------------------------------------
// US-006: EMordecaiParryPhase — phase of a parry attempt
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMordecaiParryPhase : uint8
{
	None    UMETA(DisplayName = "None"),
	Active  UMETA(DisplayName = "Active"),
	Whiff   UMETA(DisplayName = "Whiff"),
};

// ---------------------------------------------------------------------------
// AC-002.6: FMordecaiHitShapeParams
// Union-like: all fields present, relevant ones depend on HitShapeType.
// ArcSector: Radius, Angle, StartAngleOffset
// Capsule/Box: Length, Width
// Circle: Radius
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiHitShapeParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|HitShape")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|HitShape")
	float Angle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|HitShape")
	float StartAngleOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|HitShape")
	float Length = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|HitShape")
	float Width = 0.f;
};

// ---------------------------------------------------------------------------
// AC-002.7: FMordecaiProjectileSpec
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiProjectileSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	float Speed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	float GravityDrop = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	float Lifetime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile",
		Meta = (ClampMin = "0", ToolTip = "Max travel distance in UU. Default 2000. 0 = no range limit (use Lifetime only)."))
	float MaxRange = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	int32 PierceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	int32 RicochetCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	float RicochetRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	float RicochetAngleLimit = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	float OnHitAoERadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Projectile")
	float OnHitAoEDamageScalar = 0.f;
};

// ---------------------------------------------------------------------------
// AC-002.8: FMordecaiDamageProfile
// ---------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct LYRAGAME_API FMordecaiDamageProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage")
	EMordecaiDamageType DamageType = EMordecaiDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage", Meta = (Categories = "Mordecai.Damage.Delivery"))
	FGameplayTag Delivery;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage")
	float BasePower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage")
	bool CanCrit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage")
	bool AppliesPostureDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage", Meta = (Categories = "Mordecai.Status"))
	FGameplayTag BuildsStatus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mordecai|Damage")
	float StatusMagnitude = 0.f;
};
