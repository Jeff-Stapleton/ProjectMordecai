// Project Mordecai

#include "MordecaiPlayerState.h"
#include "Mordecai/AbilitySystem/MordecaiAbilitySystemComponent.h"
#include "Mordecai/AbilitySystem/Effects/MordecaiGE_SpellPointRegen.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiPostureRuntimeComponent.h"
#include "Mordecai/Combat/MordecaiStaminaRuntimeComponent.h"
#include "Mordecai/Skills/MordecaiSkillComponent.h"
#include "Mordecai/Feats/MordecaiFeatComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPlayerState)

AMordecaiPlayerState::AMordecaiPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMordecaiAbilitySystemComponent>(TEXT("AbilitySystemComponent")))
{
	// Add Mordecai attribute set alongside the existing Lyra sets (HealthSet, CombatSet).
	// The ASC will auto-detect this subobject during InitializeComponent.
	MordecaiAttributeSet = CreateDefaultSubobject<UMordecaiAttributeSet>(TEXT("MordecaiAttributeSet"));
	SkillComponent = CreateDefaultSubobject<UMordecaiSkillComponent>(TEXT("MordecaiSkillComponent"));
	FeatComponent = CreateDefaultSubobject<UMordecaiFeatComponent>(TEXT("MordecaiFeatComponent"));
	StaminaRuntimeComponent =
		CreateDefaultSubobject<UMordecaiStaminaRuntimeComponent>(TEXT("StaminaRuntimeComponent"));
	PostureRuntimeComponent =
		CreateDefaultSubobject<UMordecaiPostureRuntimeComponent>(TEXT("PostureRuntimeComponent"));
}

void AMordecaiPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (HasAuthority() && !SpellPointRegenEffectHandle.IsValid())
	{
		if (UMordecaiAbilitySystemComponent* AbilitySystem =
				Cast<UMordecaiAbilitySystemComponent>(GetAbilitySystemComponent()))
		{
			FGameplayEffectContextHandle EffectContext =
				AbilitySystem->MakeEffectContext();
			EffectContext.AddSourceObject(this);
			SpellPointRegenEffectHandle = AbilitySystem->ApplyGameplayEffectToSelf(
				GetDefault<UMordecaiGE_SpellPointRegen>(),
				/*Level=*/1.0f,
				EffectContext);
		}
	}

	if (SkillComponent)
	{
		SkillComponent->RegisterStartingSkillDataAssets();
	}
}
