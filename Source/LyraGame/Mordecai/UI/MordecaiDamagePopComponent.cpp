// Project Mordecai — Floating Damage Number Component (US-065)

#include "MordecaiDamagePopComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Mordecai/MordecaiGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiDamagePopComponent)

// Default color for unknown/untagged damage (AC-065.5: Gray)
const FLinearColor UMordecaiDamagePopComponent::DefaultDamageColor(0.7f, 0.7f, 0.7f, 1.0f);

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

UMordecaiDamagePopComponent::UMordecaiDamagePopComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InitDefaultColors();
}

void UMordecaiDamagePopComponent::InitDefaultColors()
{
	// AC-065.5: Default color mapping by damage type
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Physical, FLinearColor(1.0f, 1.0f, 1.0f));        // White
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Physical_Slash, FLinearColor(1.0f, 1.0f, 1.0f));  // White (subtype)
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Physical_Pierce, FLinearColor(1.0f, 1.0f, 1.0f)); // White (subtype)
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Physical_Blunt, FLinearColor(1.0f, 1.0f, 1.0f));  // White (subtype)
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Fire, FLinearColor(1.0f, 0.5f, 0.0f));            // Orange
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Frost, FLinearColor(0.3f, 0.8f, 1.0f));           // Cyan
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Lightning, FLinearColor(1.0f, 1.0f, 0.3f));       // Yellow
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Poison, FLinearColor(0.3f, 0.9f, 0.3f));          // Green
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Arcane, FLinearColor(0.7f, 0.3f, 1.0f));          // Purple
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Radiant, FLinearColor(1.0f, 0.85f, 0.0f));        // Gold (Holy)
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Shadow, FLinearColor(0.6f, 0.1f, 0.1f));          // Dark Red
	DamageTypeColors.Add(MordecaiGameplayTags::Damage_Corrosive, FLinearColor(0.7f, 0.7f, 0.7f));       // Gray (default)
}

// ---------------------------------------------------------------------------
// Color Lookup (AC-065.5, AC-065.6)
// ---------------------------------------------------------------------------

FLinearColor UMordecaiDamagePopComponent::GetColorForDamageType(const FGameplayTag& DamageTypeTag) const
{
	if (DamageTypeTag.IsValid())
	{
		if (const FLinearColor* Found = DamageTypeColors.Find(DamageTypeTag))
		{
			return *Found;
		}
	}
	return DefaultDamageColor;
}

// ---------------------------------------------------------------------------
// Scatter (AC-065.9)
// ---------------------------------------------------------------------------

FVector UMordecaiDamagePopComponent::ApplyScatterOffset(const FVector& BaseLocation) const
{
	if (PopScatterRadius <= 0.f)
	{
		return BaseLocation;
	}

	// Random offset in X/Y plane within scatter radius
	float Angle = FMath::FRand() * 2.0f * UE_PI;
	float Distance = FMath::FRand() * PopScatterRadius;
	FVector Offset(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.f);
	return BaseLocation + Offset;
}

// ---------------------------------------------------------------------------
// Pop Entry Management (AC-065.10)
// ---------------------------------------------------------------------------

void UMordecaiDamagePopComponent::CleanupExpiredPops(float CurrentWorldTime)
{
	ActivePops.RemoveAll([this, CurrentWorldTime](const FMordecaiDamagePopEntry& Entry)
	{
		return (CurrentWorldTime - Entry.SpawnWorldTime) >= PopLifespanSec;
	});
}

void UMordecaiDamagePopComponent::RecycleOldestPop()
{
	if (ActivePops.Num() > 0)
	{
		ActivePops.RemoveAt(0);
	}
}

FMordecaiDamagePopEntry& UMordecaiDamagePopComponent::CreatePopEntry(
	int32 Amount, const FGameplayTag& DamageTypeTag,
	const FVector& WorldLocation, const FLinearColor& Color,
	bool bIsCritical, bool bIsHealing)
{
	// AC-065.10: Enforce max concurrent pops
	if (MaxConcurrentPops > 0 && ActivePops.Num() >= MaxConcurrentPops)
	{
		RecycleOldestPop();
	}

	FMordecaiDamagePopEntry& NewEntry = ActivePops.AddDefaulted_GetRef();
	NewEntry.DamageAmount = Amount;
	NewEntry.DamageTypeTag = DamageTypeTag;
	NewEntry.WorldLocation = ApplyScatterOffset(WorldLocation);
	NewEntry.Color = Color;
	NewEntry.bIsCritical = bIsCritical;
	NewEntry.bIsHealing = bIsHealing;
	NewEntry.Scale = bIsCritical ? (PopScale * CriticalHitScale) : PopScale;

	// Use world time if available, otherwise 0
	if (const UWorld* World = GetWorld())
	{
		NewEntry.SpawnWorldTime = World->GetTimeSeconds();
	}

	return NewEntry;
}

// ---------------------------------------------------------------------------
// Public API — Damage Pop (AC-065.3, AC-065.4)
// ---------------------------------------------------------------------------

void UMordecaiDamagePopComponent::RequestDamagePop(
	int32 DamageAmount, const FGameplayTag& DamageTypeTag,
	const FVector& WorldLocation, bool bIsCritical)
{
	FLinearColor Color = GetColorForDamageType(DamageTypeTag);
	CreatePopEntry(DamageAmount, DamageTypeTag, WorldLocation, Color, bIsCritical, /*bIsHealing=*/false);
}

// ---------------------------------------------------------------------------
// Public API — Healing Pop (AC-065.8)
// ---------------------------------------------------------------------------

void UMordecaiDamagePopComponent::RequestHealingPop(int32 HealAmount, const FVector& WorldLocation)
{
	CreatePopEntry(HealAmount, FGameplayTag(), WorldLocation, HealingColor, /*bIsCritical=*/false, /*bIsHealing=*/true);
}

// ---------------------------------------------------------------------------
// Lyra NumberPop Override
// ---------------------------------------------------------------------------

void UMordecaiDamagePopComponent::AddNumberPop(const FLyraNumberPopRequest& NewRequest)
{
	// Route Lyra-style requests through Mordecai's system.
	// Determine damage type from source tags.
	FGameplayTag DamageTypeTag;
	for (const FGameplayTag& Tag : NewRequest.SourceTags)
	{
		if (Tag.MatchesTag(MordecaiGameplayTags::Damage_Physical))
		{
			DamageTypeTag = Tag;
			break;
		}
	}

	if (NewRequest.NumberToDisplay >= 0)
	{
		RequestHealingPop(NewRequest.NumberToDisplay, NewRequest.WorldLocation);
	}
	else
	{
		RequestDamagePop(-NewRequest.NumberToDisplay, DamageTypeTag,
			NewRequest.WorldLocation, NewRequest.bIsCriticalDamage);
	}
}

// ---------------------------------------------------------------------------
// Static Broadcast Helpers (AC-065.4: vertical slice)
// ---------------------------------------------------------------------------

void UMordecaiDamagePopComponent::BroadcastDamagePop(
	const UWorld* World, int32 DamageAmount, const FGameplayTag& DamageTypeTag,
	const FVector& WorldLocation, bool bIsCritical)
{
	if (!World)
	{
		return;
	}

	// For vertical slice: fire on the first local player controller
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	UMordecaiDamagePopComponent* PopComp = PC->FindComponentByClass<UMordecaiDamagePopComponent>();
	if (PopComp)
	{
		PopComp->RequestDamagePop(DamageAmount, DamageTypeTag, WorldLocation, bIsCritical);
	}
}

void UMordecaiDamagePopComponent::BroadcastHealingPop(
	const UWorld* World, int32 HealAmount, const FVector& WorldLocation)
{
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	UMordecaiDamagePopComponent* PopComp = PC->FindComponentByClass<UMordecaiDamagePopComponent>();
	if (PopComp)
	{
		PopComp->RequestHealingPop(HealAmount, WorldLocation);
	}
}
