// Project Mordecai

#pragma once

#include "AbilitySystem/LyraAbilitySystemComponent.h"

#include "MordecaiAbilitySystemComponent.generated.h"

/**
 * UMordecaiAbilitySystemComponent
 *
 *	Custom ability system component for Project Mordecai.
 */
UCLASS()
class LYRAGAME_API UMordecaiAbilitySystemComponent : public ULyraAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UMordecaiAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

#if WITH_AUTOMATION_TESTS
	void ExecutePeriodicEffectForTesting(FActiveGameplayEffectHandle Handle)
	{
		ExecutePeriodicEffect(Handle);
	}
#endif
};
