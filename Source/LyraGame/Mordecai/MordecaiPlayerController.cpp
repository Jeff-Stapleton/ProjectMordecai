// Project Mordecai — Player Controller (US-054: HUD + ability granting)

#include "MordecaiPlayerController.h"

#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPlayerController)

AMordecaiPlayerController::AMordecaiPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMordecaiPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AMordecaiPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalController())
	{
		CreateCombatHUD();
	}

	GrantCombatAbilities();
}

// ---------------------------------------------------------------------------
// HUD Creation (AC-054.8)
// ---------------------------------------------------------------------------

void AMordecaiPlayerController::CreateCombatHUD()
{
	if (CombatHUDWidget || !CombatHUDWidgetClass || !IsLocalController())
	{
		return;
	}

	CombatHUDWidget = CreateWidget<UMordecaiCombatHUDWidget>(this, CombatHUDWidgetClass);
	if (CombatHUDWidget)
	{
		CombatHUDWidget->AddToViewport();

		// Try to bind to the player's ASC (lives on PlayerState in Lyra)
		if (APlayerState* PS = PlayerState)
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
			{
				if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
				{
					CombatHUDWidget->BindToASC(ASC);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Ability Granting (AC-054.5)
// ---------------------------------------------------------------------------

void AMordecaiPlayerController::GrantCombatAbilities()
{
	if (bAbilitiesGranted || !HasAuthority())
	{
		return;
	}

	// Find the ASC (lives on PlayerState in Lyra)
	UAbilitySystemComponent* ASC = nullptr;
	if (APlayerState* PS = PlayerState)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
		{
			ASC = ASI->GetAbilitySystemComponent();
		}
	}

	if (!ASC)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultCombatAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		// Special handling for MeleeAttack: set AttackProfiles before granting
		if (AbilityClass->IsChildOf(UMordecaiGA_MeleeAttack::StaticClass()) && MeleeAttackProfiles.Num() > 0)
		{
			UMordecaiGA_MeleeAttack* MeleeCDO = AbilityClass->GetDefaultObject<UMordecaiGA_MeleeAttack>();
			if (MeleeCDO && MeleeCDO->AttackProfiles.Num() == 0)
			{
				MeleeCDO->AttackProfiles = MeleeAttackProfiles;
			}
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
		ASC->GiveAbility(Spec);
	}

	bAbilitiesGranted = true;
}
