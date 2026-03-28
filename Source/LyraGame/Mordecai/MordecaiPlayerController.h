// Project Mordecai — Player Controller (US-054: HUD integration)

#pragma once

#include "Player/LyraPlayerController.h"

#include "MordecaiPlayerController.generated.h"

class UMordecaiCombatHUDWidget;
class UGameplayAbility;
class UMordecaiAttackProfileDataAsset;

/**
 * AMordecaiPlayerController
 *
 *	Player controller for Project Mordecai. Extends Lyra's base controller.
 *	Creates and manages the combat HUD widget (AC-054.8).
 *	Grants combat abilities to the player (AC-054.5).
 */
UCLASS()
class LYRAGAME_API AMordecaiPlayerController : public ALyraPlayerController
{
	GENERATED_BODY()

public:
	AMordecaiPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Widget class for the combat HUD. Set in Blueprint or CDO. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|HUD")
	TSubclassOf<UMordecaiCombatHUDWidget> CombatHUDWidgetClass;

	/** Combat abilities to grant to the player on possession (AC-054.5). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Combat")
	TArray<TSubclassOf<UGameplayAbility>> DefaultCombatAbilities;

	/** Attack profiles for the melee attack ability combo (AC-054.4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Combat")
	TArray<TObjectPtr<UMordecaiAttackProfileDataAsset>> MeleeAttackProfiles;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	void CreateCombatHUD();
	void GrantCombatAbilities();

	UPROPERTY()
	TObjectPtr<UMordecaiCombatHUDWidget> CombatHUDWidget;

	bool bAbilitiesGranted = false;
};
