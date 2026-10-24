// Project Mordecai — Player Controller (US-054: HUD + ability granting)

#include "MordecaiPlayerController.h"

#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiDamagePopComponent.h"
#include "Mordecai/UI/MordecaiPickupPromptWidget.h"
#include "Mordecai/Items/MordecaiPickupInteractionComponent.h"
#include "Mordecai/Combat/MordecaiAttackProfileDataAsset.h"
#include "Mordecai/Combat/MordecaiGA_Block.h"
#include "Mordecai/Combat/MordecaiGA_Dodge.h"
#include "Mordecai/Combat/MordecaiGA_HeavyAttack.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/Combat/MordecaiGA_Parry.h"
#include "Mordecai/MordecaiGameplayTags.h"
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

	// AC-054.5 / US-083: the player's combat abilities, each with the input tag its bindings press
	// (UMordecaiHeroComponent). Native class refs: no content, always cooked, never a CDO edit.
	auto AddCombatAbility = [this](TSubclassOf<UGameplayAbility> AbilityClass, const FGameplayTag& InputTag)
	{
		DefaultCombatAbilities.Add(AbilityClass);
		CombatAbilityInputTags.Add(AbilityClass, InputTag);
	};
	AddCombatAbility(UMordecaiGA_MeleeAttack::StaticClass(), MordecaiGameplayTags::Input_LightAttack);
	AddCombatAbility(UMordecaiGA_HeavyAttack::StaticClass(), MordecaiGameplayTags::Input_HeavyAttack);
	AddCombatAbility(UMordecaiGA_Dodge::StaticClass(), MordecaiGameplayTags::Input_Dodge);
	AddCombatAbility(UMordecaiGA_Block::StaticClass(), MordecaiGameplayTags::Input_Block);
	AddCombatAbility(UMordecaiGA_Parry::StaticClass(), MordecaiGameplayTags::Input_Parry);

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

		// Wire the pickup/inventory assist prompts to the pawn's interaction component (US-079)
		if (UMordecaiPickupPromptWidget* Prompt = CombatHUDWidget->GetPickupPrompt())
		{
			if (APawn* PossessedPawn = GetPawn())
			{
				if (UMordecaiPickupInteractionComponent* Interaction = PossessedPawn->FindComponentByClass<UMordecaiPickupInteractionComponent>())
				{
					Prompt->BindToInteraction(Interaction);
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

		// This controller is the grant's SourceObject; a melee ability instance takes MeleeAttackProfiles from it
		// in OnGiveAbility. Writing them into the CDO instead is a fatal GC error in packaged builds.
		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);

		// US-083: the input tag the hero component's bindings press to activate this spec (Lyra input path)
		if (const FGameplayTag* InputTag = CombatAbilityInputTags.Find(AbilityClass))
		{
			Spec.GetDynamicSpecSourceTags().AddTag(*InputTag);
		}
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

		UMordecaiSpellDataAsset* GrantedData = SpellDataAssets.IsValidIndex(i) ? SpellDataAssets[i].Get() : nullptr;
		if (!GrantedData)
		{
			// Try to load SpellData from well-known content paths (all of /MordecaiCore is cooked)
			static const TCHAR* SpellDataPaths[] = {
				TEXT("/MordecaiCore/Spells/DA_Spell_Fireball.DA_Spell_Fireball"),
				TEXT("/MordecaiCore/Spells/DA_Spell_Blink.DA_Spell_Blink"),
				TEXT("/MordecaiCore/Spells/DA_Spell_StoneSkin.DA_Spell_StoneSkin"),
				TEXT("/MordecaiCore/Spells/DA_Spell_Restoration.DA_Spell_Restoration"),
			};

			const UMordecaiGA_SpellBase* SpellCDO = SpellClass->GetDefaultObject<UMordecaiGA_SpellBase>();
			if (i < UE_ARRAY_COUNT(SpellDataPaths) && SpellCDO && !SpellCDO->SpellData)
			{
				GrantedData = Cast<UMordecaiSpellDataAsset>(FSoftObjectPath(SpellDataPaths[i]).TryLoad());
			}
		}

		// The spell data travels with the grant as its SourceObject; the ability instance picks it up
		// in OnGiveAbility. Writing it into the CDO instead is a fatal GC error in packaged builds.
		FGameplayAbilitySpec Spec(SpellClass, 1, INDEX_NONE, GrantedData ? static_cast<UObject*>(GrantedData) : this);
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
