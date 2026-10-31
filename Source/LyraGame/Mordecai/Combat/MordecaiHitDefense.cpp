// Project Mordecai — The target's defenses against an incoming melee hit (US-101, issue #53)

#include "Mordecai/Combat/MordecaiHitDefense.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"

DEFINE_LOG_CATEGORY_STATIC(LogMordecaiDefense, Log, All);

namespace
{
	/** The primary instance of the first active ability spec whose ability is an AbilityT (InstancedPerActor). */
	template <typename AbilityT>
	AbilityT* FindActiveInstance(const UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return nullptr;
		}
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.IsActive() && Spec.Ability && Spec.Ability->IsA<AbilityT>())
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
}

UMordecaiGA_Parry* FMordecaiHitDefense::FindActiveParry(const UAbilitySystemComponent* ASC)
{
	return FindActiveInstance<UMordecaiGA_Parry>(ASC);
}

UMordecaiGA_Block* FMordecaiHitDefense::FindActiveBlock(const UAbilitySystemComponent* ASC)
{
	return FindActiveInstance<UMordecaiGA_Block>(ASC);
}

FMordecaiHitDefenseResult FMordecaiHitDefense::ResolveIncomingHit(
	UAbilitySystemComponent* TargetASC,
	UAbilitySystemComponent* AttackerASC,
	float IncomingHealthDamage,
	double HitTime)
{
	FMordecaiHitDefenseResult Result;
	if (!TargetASC)
	{
		return Result;
	}

	// 1. i-frames: the hit never connects (AC-005.3 dodge, AC-021.3 Blink)
	if (TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Invulnerable)
		|| TargetASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dodging))
	{
		Result.Defense = EMordecaiHitDefense::Evaded;
		UE_LOG(LogMordecaiDefense, Log, TEXT("%s evaded %s's hit (%.1f) with i-frames"),
			*DescribeOwner(TargetASC), *DescribeOwner(AttackerASC), IncomingHealthDamage);
		return Result;
	}

	// 2. A parry window takes the whole hit (AC-006.13). Found first, called after the scan: a parry ends its ability.
	if (UMordecaiGA_Parry* Parry = FindActiveParry(TargetASC))
	{
		if (Parry->ReceiveParriedHit(AttackerASC))
		{
			Result.Defense = EMordecaiHitDefense::Parried;
			UE_LOG(LogMordecaiDefense, Log, TEXT("%s parried %s's hit (%.1f)"),
				*DescribeOwner(TargetASC), *DescribeOwner(AttackerASC), IncomingHealthDamage);
			return Result;
		}
	}

	// 3. A raised guard mitigates it and applies that damage itself. A guard-broken block (the ability still runs its
	// stagger) is no guard: the hit lands in full.
	// TODO(DECISION): block covers every direction; skill_sheet_v1.1's shields say "frontal".
	if (UMordecaiGA_Block* Block = FindActiveBlock(TargetASC))
	{
		if (Block->GetCurrentBlockPhase() == EMordecaiBlockPhase::Active)
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
