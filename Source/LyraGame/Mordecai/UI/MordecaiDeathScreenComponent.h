// Project Mordecai — Death Screen Component (US-086, issue #7)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "MordecaiDeathScreenComponent.generated.h"

class AMordecaiGameMode;
class UAbilitySystemComponent;
class UMordecaiDeathScreenWidget;
class UPrimaryGameLayout;

/**
 * UMordecaiDeathScreenComponent
 *
 * Shows the death screen (US-062) while the player is dead. Attached to the player controller, it listens for
 * Mordecai.State.Dead on the PlayerState ASC: when the tag is added (AMordecaiCharacter::HandlePlayerDeath) it pushes
 * UMordecaiDeathScreenWidget onto the local player's UI.Layer.Game stack, set up with the game mode's respawn delay
 * and session kill count; when the tag is removed (HandlePlayerRespawn) it removes the screen.
 *
 * Bound by AMordecaiPlayerController::OnPossess for a local controller, where the combat HUD binds. Like the HUD it
 * serves the local player of a standalone or listen-server game: the Dead tag is a loose tag and does not replicate.
 */
UCLASS()
class LYRAGAME_API UMordecaiDeathScreenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiDeathScreenComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Listen for the Dead tag on the owning controller's PlayerState ASC; shows the screen at once if already dead. */
	void BindToPlayerState();

	/** Stop listening and remove a death screen this component shows. */
	void Unbind();

	UFUNCTION(BlueprintPure, Category = "Mordecai|HUD")
	bool IsBoundToPlayerState() const;

	/** The death screen on screen, or null while alive. */
	UFUNCTION(BlueprintPure, Category = "Mordecai|HUD")
	UMordecaiDeathScreenWidget* GetDeathScreenWidget() const;

	/** Tests: push to this layout instead of the local player's primary game layout (headless has no UI policy). */
	void SetLayoutOverride(UPrimaryGameLayout* Layout);

	/** Tests: take the respawn delay and kill count from this game mode (a headless world has no auth game mode). */
	void SetGameModeOverride(const AMordecaiGameMode* GameMode);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void ShowDeathScreen();
	void HideDeathScreen();
	UPrimaryGameLayout* GetLayout() const;
	const AMordecaiGameMode* GetGameMode() const;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle DeadTagChangedHandle;
	TWeakObjectPtr<UMordecaiDeathScreenWidget> DeathScreen;
	TWeakObjectPtr<UPrimaryGameLayout> LayoutOverride;
	TWeakObjectPtr<const AMordecaiGameMode> GameModeOverride;
};
