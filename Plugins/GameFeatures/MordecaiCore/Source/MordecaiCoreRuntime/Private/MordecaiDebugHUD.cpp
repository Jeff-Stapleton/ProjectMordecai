// Copyright Project Mordecai. All Rights Reserved.

#include "MordecaiDebugHUD.h"
#include "MordecaiAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiDebugHUD)

UMordecaiDebugHUD::UMordecaiDebugHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentHealth = 0.0f;
	CurrentMaxHealth = 0.0f;
	CurrentStamina = 0.0f;
	CurrentMaxStamina = 0.0f;
	CurrentSpellPoints = 0.0f;
	CurrentMaxSpellPoints = 0.0f;
	CurrentPosture = 0.0f;
	CurrentMaxPosture = 0.0f;
	RefreshInterval = 0.1f;
	TimeSinceLastRefresh = 0.0f;
}

void UMordecaiDebugHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TimeSinceLastRefresh += InDeltaTime;
	if (TimeSinceLastRefresh >= RefreshInterval)
	{
		TimeSinceLastRefresh = 0.0f;
		RefreshAttributeDisplay();
	}
}

void UMordecaiDebugHUD::RefreshAttributeDisplay()
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (!OwningPawn)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningPawn);
	if (!ASC)
	{
		return;
	}

	// Read vital attributes from MordecaiAttributeSet.
	CurrentHealth = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetHealthAttribute());
	CurrentMaxHealth = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxHealthAttribute());
	CurrentStamina = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaAttribute());
	CurrentMaxStamina = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxStaminaAttribute());
	CurrentSpellPoints = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetSpellPointsAttribute());
	CurrentMaxSpellPoints = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxSpellPointsAttribute());
	CurrentPosture = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetPostureAttribute());
	CurrentMaxPosture = ASC->GetNumericAttribute(UMordecaiAttributeSet::GetMaxPostureAttribute());

	// If used as a simple PrintString debug HUD (no widget bindings), log to screen.
	if (GEngine)
	{
		const float HealthPct = (CurrentMaxHealth > 0.0f) ? (CurrentHealth / CurrentMaxHealth * 100.0f) : 0.0f;
		const FString DebugString = FString::Printf(
			TEXT("HP: %.0f/%.0f (%.0f%%)  STA: %.0f/%.0f  SP: %.0f/%.0f  POST: %.0f/%.0f"),
			CurrentHealth, CurrentMaxHealth, HealthPct,
			CurrentStamina, CurrentMaxStamina,
			CurrentSpellPoints, CurrentMaxSpellPoints,
			CurrentPosture, CurrentMaxPosture
		);
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, DebugString);
	}
}
