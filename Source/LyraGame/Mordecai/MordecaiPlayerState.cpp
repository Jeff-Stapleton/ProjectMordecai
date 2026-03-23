// Project Mordecai

#include "MordecaiPlayerState.h"
#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPlayerState)

AMordecaiPlayerState::AMordecaiPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMordecaiAbilitySystemComponent>(TEXT("AbilitySystemComponent")))
{
	// Add Mordecai attribute set alongside the existing Lyra sets (HealthSet, CombatSet).
	// The ASC will auto-detect this subobject during InitializeComponent.
	MordecaiAttributeSet = CreateDefaultSubobject<UMordecaiAttributeSet>(TEXT("MordecaiAttributeSet"));
	SkillComponent = CreateDefaultSubobject<UMordecaiSkillComponent>(TEXT("MordecaiSkillComponent"));
}
