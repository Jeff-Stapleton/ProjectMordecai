// Project Mordecai

#include "MordecaiAbilitySystemComponent.h"
#include "AbilitySystem/LyraGameplayEffectContext.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Mordecai/MordecaiGameMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiAbilitySystemComponent)

UMordecaiAbilitySystemComponent::UMordecaiAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayEffectContextHandle UMordecaiAbilitySystemComponent::MakeEffectContext() const
{
	FGameplayEffectContextHandle Context = Super::MakeEffectContext();
	FLyraGameplayEffectContext* LyraContext = FLyraGameplayEffectContext::ExtractEffectContext(Context);
	if (!LyraContext)
	{
		return Context;
	}

	APlayerState* SourcePlayerState = Cast<APlayerState>(GetOwnerActor());
	if (!SourcePlayerState)
	{
		if (const APawn* SourcePawn = Cast<APawn>(GetAvatarActor()))
		{
			SourcePlayerState = SourcePawn->GetPlayerState();
		}
	}

	const UWorld* World = GetWorld();
	const AMordecaiGameMode* GameMode = World ? World->GetAuthGameMode<AMordecaiGameMode>() : nullptr;
	LyraContext->SetSourcePlayerLifeId(
		GameMode ? GameMode->GetCurrentPlayerLifeId(SourcePlayerState) : INDEX_NONE);
	return Context;
}
