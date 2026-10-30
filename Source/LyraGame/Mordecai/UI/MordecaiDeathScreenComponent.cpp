// Project Mordecai — Death Screen Component (US-086, issue #7)

#include "Mordecai/UI/MordecaiDeathScreenComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Mordecai/MordecaiGameMode.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiDeathScreenWidget.h"
#include "NativeGameplayTags.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiDeathScreenComponent)

// Empty in Mordecai, and below UI.Layer.Menu (the pause menu) and UI.Layer.Modal (Lyra's dialogs)
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Mordecai_UI_Layer_Game, "UI.Layer.Game");

UMordecaiDeathScreenComponent::UMordecaiDeathScreenComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UMordecaiDeathScreenComponent::BindToPlayerState()
{
	const APlayerController* PC = GetOwner<APlayerController>();
	UAbilitySystemComponent* ASC = PC ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PC->PlayerState) : nullptr;
	if (!ASC || BoundASC.Get() == ASC)
	{
		return;
	}

	Unbind();
	// Added by HandlePlayerDeath, removed by HandlePlayerRespawn: the one signal for both (no respawn event)
	DeadTagChangedHandle = ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiDeathScreenComponent::OnDeadTagChanged);
	BoundASC = ASC;

	// Possessed while already dead: the tag was added before this listened
	if (ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_Dead))
	{
		ShowDeathScreen();
	}
}

void UMordecaiDeathScreenComponent::Unbind()
{
	HideDeathScreen();
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_Dead, EGameplayTagEventType::NewOrRemoved).Remove(DeadTagChangedHandle);
	}
	DeadTagChangedHandle.Reset();
	BoundASC.Reset();
}

bool UMordecaiDeathScreenComponent::IsBoundToPlayerState() const
{
	return BoundASC.IsValid();
}

UMordecaiDeathScreenWidget* UMordecaiDeathScreenComponent::GetDeathScreenWidget() const
{
	return DeathScreen.Get();
}

void UMordecaiDeathScreenComponent::SetLayoutOverride(UPrimaryGameLayout* Layout)
{
	LayoutOverride = Layout;
}

void UMordecaiDeathScreenComponent::SetGameModeOverride(const AMordecaiGameMode* GameMode)
{
	GameModeOverride = GameMode;
}

void UMordecaiDeathScreenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The root layout outlives this controller (it belongs to the local player): take the screen with us
	Unbind();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Death screen
// ---------------------------------------------------------------------------

void UMordecaiDeathScreenComponent::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		ShowDeathScreen();
	}
	else
	{
		HideDeathScreen();
	}
}

void UMordecaiDeathScreenComponent::ShowDeathScreen()
{
	if (DeathScreen.IsValid())
	{
		return;
	}

	UPrimaryGameLayout* Layout = GetLayout();
	if (!Layout)
	{
		return;
	}

	// The game mode's respawn delay and session kill count (player.kills); its defaults where there is none
	const AMordecaiGameMode* GameMode = GetGameMode();
	const float RespawnDelay = (GameMode ? GameMode : GetDefault<AMordecaiGameMode>())->RespawnDelaySeconds;
	const int32 KillCount = GameMode ? GameMode->GetEnemyKillCount() : 0;

	// CommonUI pools the layer's widgets: a later death gets this instance back without NativeOnInitialized, so it
	// is set up on every push
	DeathScreen = Layout->PushWidgetToLayerStack<UMordecaiDeathScreenWidget>(
		TAG_Mordecai_UI_Layer_Game,
		UMordecaiDeathScreenWidget::StaticClass(),
		[RespawnDelay, KillCount](UMordecaiDeathScreenWidget& Screen)
		{
			Screen.InitializeDeathScreen(RespawnDelay, KillCount);
		});
}

void UMordecaiDeathScreenComponent::HideDeathScreen()
{
	UMordecaiDeathScreenWidget* Screen = DeathScreen.Get();
	DeathScreen.Reset();
	if (!Screen)
	{
		return;
	}

	// Out of whichever layer holds it: the stack deactivates the displayed widget and releases it to its pool
	if (UPrimaryGameLayout* Layout = GetLayout())
	{
		Layout->FindAndRemoveWidgetFromLayer(Screen);
	}
}

UPrimaryGameLayout* UMordecaiDeathScreenComponent::GetLayout() const
{
	if (UPrimaryGameLayout* Layout = LayoutOverride.Get())
	{
		return Layout;
	}

	// The local player's root layout (B_LyraUIPolicy's W_OverallUILayout). This overload never asserts: it is null
	// for a controller without a UCommonLocalPlayer.
	return UPrimaryGameLayout::GetPrimaryGameLayout(GetOwner<APlayerController>());
}

const AMordecaiGameMode* UMordecaiDeathScreenComponent::GetGameMode() const
{
	if (const AMordecaiGameMode* GameMode = GameModeOverride.Get())
	{
		return GameMode;
	}
	return GetWorld() ? GetWorld()->GetAuthGameMode<AMordecaiGameMode>() : nullptr;
}
