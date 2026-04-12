// Project Mordecai — Player Controller (US-054: HUD + ability granting)

#include "MordecaiPlayerController.h"

#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiDamagePopComponent.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiGA_StoneSkin.h"
#include "Mordecai/Magic/MordecaiGA_Restoration.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPlayerController)

AMordecaiPlayerController::AMordecaiPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// US-065: Create floating damage number component
	DamagePopComponent = CreateDefaultSubobject<UMordecaiDamagePopComponent>(TEXT("DamagePopComponent"));

	// AC-057.6: Default spell loadout (Fireball, Blink, StoneSkin, Restoration)
	DefaultSpellAbilities.Add(UMordecaiGA_Fireball::StaticClass());
	DefaultSpellAbilities.Add(UMordecaiGA_Blink::StaticClass());
	DefaultSpellAbilities.Add(UMordecaiGA_StoneSkin::StaticClass());
	DefaultSpellAbilities.Add(UMordecaiGA_Restoration::StaticClass());
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
	if (CombatHUDWidget || !IsLocalController())
	{
		return;
	}

	// If CombatHUDWidgetClass is not set (e.g., no Blueprint subclass), try to load WBP_CombatHUD
	if (!CombatHUDWidgetClass)
	{
		CombatHUDWidgetClass = LoadClass<UMordecaiCombatHUDWidget>(
			nullptr,
			TEXT("/MordecaiCore/UI/WBP_CombatHUD.WBP_CombatHUD_C"));

		if (!CombatHUDWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("MordecaiPlayerController: Failed to load WBP_CombatHUD. No combat HUD will be displayed."));
			return;
		}
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

	// --- AC-057.6: Grant spell abilities with SpellData assignment ---
	for (int32 i = 0; i < DefaultSpellAbilities.Num(); ++i)
	{
		TSubclassOf<UGameplayAbility> SpellClass = DefaultSpellAbilities[i];
		if (!SpellClass)
		{
			continue;
		}

		// Set SpellData on CDO before granting (if SpellDataAssets array is populated)
		if (SpellDataAssets.IsValidIndex(i) && SpellDataAssets[i])
		{
			UMordecaiGA_SpellBase* SpellCDO = SpellClass->GetDefaultObject<UMordecaiGA_SpellBase>();
			if (SpellCDO)
			{
				SpellCDO->SpellData = SpellDataAssets[i];
			}
		}
		else
		{
			// Try to load SpellData from well-known content paths
			static const TCHAR* SpellDataPaths[] = {
				TEXT("/MordecaiCore/Spells/DA_Spell_Fireball.DA_Spell_Fireball"),
				TEXT("/MordecaiCore/Spells/DA_Spell_Blink.DA_Spell_Blink"),
				TEXT("/MordecaiCore/Spells/DA_Spell_StoneSkin.DA_Spell_StoneSkin"),
				TEXT("/MordecaiCore/Spells/DA_Spell_Restoration.DA_Spell_Restoration"),
			};

			if (i < UE_ARRAY_COUNT(SpellDataPaths))
			{
				UMordecaiGA_SpellBase* SpellCDO = SpellClass->GetDefaultObject<UMordecaiGA_SpellBase>();
				if (SpellCDO && !SpellCDO->SpellData)
				{
					UMordecaiSpellDataAsset* LoadedData = Cast<UMordecaiSpellDataAsset>(
						FSoftObjectPath(SpellDataPaths[i]).TryLoad());
					if (LoadedData)
					{
						SpellCDO->SpellData = LoadedData;
					}
				}
			}
		}

		FGameplayAbilitySpec Spec(SpellClass, 1, INDEX_NONE, this);
		ASC->GiveAbility(Spec);
	}

	bAbilitiesGranted = true;
}

TSubclassOf<UGameplayAbility> AMordecaiPlayerController::GetSpellAbilityClass(int32 SlotIndex) const
{
	if (DefaultSpellAbilities.IsValidIndex(SlotIndex))
	{
		return DefaultSpellAbilities[SlotIndex];
	}
	return nullptr;
}
