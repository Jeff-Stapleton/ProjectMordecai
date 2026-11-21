// Project Mordecai — The target's defenses against an incoming melee hit (US-101, issue #53)

#include "Mordecai/Combat/MordecaiHitDefense.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"

DEFINE_LOG_CATEGORY_STATIC(LogMordecaiDefense, Log, All);

namespace
{
	/** The primary instance of the first matching ability spec (InstancedPerActor). */
	template <typename AbilityT>
	AbilityT* FindInstance(const UAbilitySystemComponent* ASC, bool bRequireActive)
	{
		if (!ASC)
		{
			return nullptr;
		}
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if ((!bRequireActive || Spec.IsActive()) && Spec.Ability && Spec.Ability->IsA<AbilityT>())
			{
				if (AbilityT* Instance = Cast<AbilityT>(Spec.GetPrimaryInstance()))
				{
					return Instance;
				}
			}
		}
		return nullptr;
	}

	FString DescribeOwner(const UAbilitySystemComponent* ASC)
	{
		return ASC ? GetNameSafe(ASC->GetAvatarActor()) : FString(TEXT("None"));
	}

	bool IsSourceInsideDefenseArc(
		const UAbilitySystemComponent* TargetASC,
		const UAbilitySystemComponent* AttackerASC,
		const FMordecaiIncomingHitSource& HitSource,
		float ArcDegrees)
	{
		const AActor* Defender = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
		if (!Defender)
		{
			return false;
		}

		FVector SourceLocation = HitSource.SourceLocation;
		bool bHasSourceLocation = HitSource.bHasSourceLocation;
		if (!bHasSourceLocation)
		{
			if (const AActor* Attacker = AttackerASC ? AttackerASC->GetAvatarActor() : nullptr)
			{
				SourceLocation = Attacker->GetActorLocation();
				bHasSourceLocation = true;
			}
		}

		FVector TowardSource = bHasSourceLocation
			? SourceLocation - Defender->GetActorLocation()
			: FVector::ZeroVector;
		TowardSource.Z = 0.f;
		if (TowardSource.IsNearlyZero() && !HitSource.IncomingTravelDirection.IsNearlyZero())
		{
			// Travel points toward the defender; defense direction points back toward where it came from.
			TowardSource = -HitSource.IncomingTravelDirection;
			TowardSource.Z = 0.f;
		}
		if (!TowardSource.Normalize())
		{
			return false;
		}

		FVector DefenderForward = Defender->GetActorForwardVector();
		DefenderForward.Z = 0.f;
		if (!DefenderForward.Normalize())
		{
			return false;
		}

		const float ClampedArc = FMath::Clamp(ArcDegrees, 0.f, 360.f);
		if (ClampedArc >= 360.f - KINDA_SMALL_NUMBER)
		{
			return true;
		}
		if (ClampedArc <= 0.f)
		{
			return false;
		}
		const float MinimumDot = FMath::Cos(FMath::DegreesToRadians(ClampedArc * 0.5f));
		return FVector::DotProduct(DefenderForward, TowardSource) >= MinimumDot - KINDA_SMALL_NUMBER;
	}

	float GetEffectiveDefenseArc(const UAbilitySystemComponent* TargetASC)
	{
		if (const UMordecaiGA_Block* Block = FindInstance<UMordecaiGA_Block>(TargetASC, false))
		{
			return Block->GetEffectiveBlockArcDegrees();
		}
		return 120.f;
	}
}

UMordecaiGA_Parry* FMordecaiHitDefense::FindActiveParry(const UAbilitySystemComponent* ASC)
{
	return FindInstance<UMordecaiGA_Parry>(ASC, true);
}

UMordecaiGA_Block* FMordecaiHitDefense::FindActiveBlock(const UAbilitySystemComponent* ASC)
{
	return FindInstance<UMordecaiGA_Block>(ASC, true);
}

FMordecaiHitDefenseResult FMordecaiHitDefense::ResolveIncomingHit(
	UAbilitySystemComponent* TargetASC,
	UAbilitySystemComponent* AttackerASC,
	float IncomingHealthDamage,
	double HitTime,
	const FMordecaiIncomingHitSource& HitSource)
{
	FMordecaiHitDefenseResult Result;
	if (!TargetASC)
	{
		return Result;
	}

	// 1. i-frames: the hit never connects (AC-005.3 dodge, AC-021.3 Blink).
	// Only a real hit evaded by an active Dodge can earn its one perfect-action reward.
	const bool bDodging =
		TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dodging);
	if (bDodging)
	{
		if (UMordecaiGA_Dodge* Dodge =
			FindInstance<UMordecaiGA_Dodge>(TargetASC, true))
		{
			Dodge->NotifySuccessfulEvade();
		}
	}
	if (TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Invulnerable)
		|| bDodging)
	{
		Result.Defense = EMordecaiHitDefense::Evaded;
		UE_LOG(LogMordecaiDefense, Log, TEXT("%s evaded %s's hit (%.1f) with i-frames"),
			*DescribeOwner(TargetASC), *DescribeOwner(AttackerASC), IncomingHealthDamage);
		return Result;
	}

	const float DefenseArcDegrees = GetEffectiveDefenseArc(TargetASC);
	const bool bInsideDefenseArc =
		IsSourceInsideDefenseArc(TargetASC, AttackerASC, HitSource, DefenseArcDegrees);

	// 2. An in-arc parry window takes the whole hit (AC-006.13). Found first, called after the scan: a parry ends its ability.
	if (UMordecaiGA_Parry* Parry = FindActiveParry(TargetASC))
	{
		if (bInsideDefenseArc && Parry->ReceiveParriedHit(AttackerASC, !HitSource.bIsProjectile))
		{
			Result.Defense = EMordecaiHitDefense::Parried;
			UE_LOG(LogMordecaiDefense, Log, TEXT("%s parried %s's hit (%.1f)"),
				*DescribeOwner(TargetASC), *DescribeOwner(AttackerASC), IncomingHealthDamage);
			return Result;
		}
	}

	// 3. An in-arc raised guard mitigates it and applies that damage itself. A guard-broken block (the ability still
	// runs its stagger) and an out-of-arc hit are no guard: the hit lands in full.
	if (UMordecaiGA_Block* Block = FindActiveBlock(TargetASC))
	{
		if (bInsideDefenseArc && Block->GetCurrentBlockPhase() == EMordecaiBlockPhase::Active)
		{
			const bool bPerfect = Block->IsPerfectBlockWindowActive() && Block->IsHitWithinPerfectBlockWindow(HitTime);
			Result.Defense = bPerfect ? EMordecaiHitDefense::PerfectBlocked : EMordecaiHitDefense::Blocked;
			Result.BlockedHealthDamage = Block->ReceiveBlockedHit(IncomingHealthDamage, HitTime, AttackerASC);
			UE_LOG(LogMordecaiDefense, Log, TEXT("%s %s %s's hit: %.1f -> %.1f health%s"),
				*DescribeOwner(TargetASC), bPerfect ? TEXT("perfect-blocked") : TEXT("blocked"), *DescribeOwner(AttackerASC),
				IncomingHealthDamage, Result.BlockedHealthDamage,
				Block->GetCurrentBlockPhase() == EMordecaiBlockPhase::GuardBroken ? TEXT(", guard broken") : TEXT(""));
			return Result;
		}
	}

	return Result;
}
