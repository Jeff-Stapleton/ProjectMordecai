// Project Mordecai — Stamina Tier System (US-008)

#include "Mordecai/Combat/MordecaiStaminaSystem.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiStaminaSystem)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiStaminaSystem::UMordecaiStaminaSystem()
{
}

// ---------------------------------------------------------------------------
// Computation Methods
// ---------------------------------------------------------------------------

EMordecaiStaminaTier UMordecaiStaminaSystem::GetTierForStamina(float CurrentStamina, float MaxStamina) const
{
	// AC-008.3: Green > 66%, Yellow 33%-66%, Red < 33%
	if (MaxStamina <= 0.f)
	{
		return EMordecaiStaminaTier::Red;
	}

	const float Ratio = CurrentStamina / MaxStamina;

	if (Ratio > GreenThresholdPercent)
	{
		return EMordecaiStaminaTier::Green;
	}
	else if (Ratio >= YellowThresholdPercent)
	{
		return EMordecaiStaminaTier::Yellow;
	}
	else
	{
		return EMordecaiStaminaTier::Red;
	}
}

bool UMordecaiStaminaSystem::IsExhausted(float CurrentStamina) const
{
	// AC-008.6: Exhausted when Stamina <= 0
	return CurrentStamina <= 0.f;
}

float UMordecaiStaminaSystem::GetMultiplierForTier(EMordecaiStaminaTier Tier) const
{
	// AC-008.7: Per-tier effectiveness multipliers
	switch (Tier)
	{
	case EMordecaiStaminaTier::Green:  return GreenMultiplier;
	case EMordecaiStaminaTier::Yellow: return YellowMultiplier;
	case EMordecaiStaminaTier::Red:    return RedMultiplier;
	default:                           return YellowMultiplier;
	}
}

float UMordecaiStaminaSystem::GetTierMultiplier(float CurrentStamina, float MaxStamina) const
{
	// AC-008.8: Exhausted overrides tier multiplier
	if (IsExhausted(CurrentStamina))
	{
		return ExhaustedMultiplier;
	}

	const EMordecaiStaminaTier Tier = GetTierForStamina(CurrentStamina, MaxStamina);
	return GetMultiplierForTier(Tier);
}

float UMordecaiStaminaSystem::ComputeRegenAmount(float DeltaTime) const
{
	// AC-008.10: Regen = StaminaRegenRate x DeltaTime
	return StaminaRegenRate * DeltaTime;
}

float UMordecaiStaminaSystem::ComputeSprintDrain(float DeltaTime) const
{
	// AC-008.14, AC-008.15: Continuous drain = SprintStaminaDrainRate x DeltaTime
	return SprintStaminaDrainRate * DeltaTime;
}

// ---------------------------------------------------------------------------
// State Queries
// ---------------------------------------------------------------------------

bool UMordecaiStaminaSystem::IsRegenAllowed(double CurrentTime) const
{
	// AC-008.11: Regen paused for StaminaRegenDelayMs after consumption
	const double DelaySeconds = StaminaRegenDelayMs / 1000.0;
	const double TimeSinceConsumption = CurrentTime - LastConsumptionTime;
	return TimeSinceConsumption > DelaySeconds;
}

// ---------------------------------------------------------------------------
// State Management
// ---------------------------------------------------------------------------

void UMordecaiStaminaSystem::NotifyStaminaConsumed(double CurrentTime)
{
	// AC-008.11, AC-008.12: Track consumption time; each new consumption resets the delay
	LastConsumptionTime = CurrentTime;
}

void UMordecaiStaminaSystem::Reset()
{
	LastConsumptionTime = -999.0;
}

// ---------------------------------------------------------------------------
// Static Helper
// ---------------------------------------------------------------------------

float UMordecaiStaminaSystem::GetTierMultiplierFromASC(UAbilitySystemComponent* ASC)
{
	// AC-008.8: Read tier tags from ASC and return appropriate multiplier
	if (!ASC)
	{
		return 1.0f;
	}

	// Exhausted overrides all tier multipliers
	if (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Exhausted))
	{
		// Use CDO defaults for multiplier values
		const UMordecaiStaminaSystem* CDO = GetDefault<UMordecaiStaminaSystem>();
		return CDO->ExhaustedMultiplier;
	}

	const UMordecaiStaminaSystem* CDO = GetDefault<UMordecaiStaminaSystem>();

	if (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Green))
	{
		return CDO->GreenMultiplier;
	}
	else if (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Yellow))
	{
		return CDO->YellowMultiplier;
	}
	else if (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::Stamina_Tier_Red))
	{
		return CDO->RedMultiplier;
	}

	// Default if no tier tag is present
	return 1.0f;
}
