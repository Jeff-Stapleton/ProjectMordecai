// Project Mordecai — weapon effects must follow the player-state ASC lifecycle (US-096, issue #36)

#include "Misc/AutomationTest.h"

#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiCharacter.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerState.h"
#include "Mordecai/Weapons/MordecaiEquipmentComponent.h"
#include "Mordecai/Weapons/MordecaiWeaponDataAsset.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"
#include "Player/LyraPlayerController.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "UObject/UObjectGlobals.h"

namespace MordecaiRecoveryWeaponEffectsTest
{
	using namespace MordecaiRecoveryTest;

	UMordecaiWeaponDataAsset* MakeWeapon(
		const FName Id,
		const FGameplayTag TypeTag,
		const float StrengthBonus)
	{
		UMordecaiWeaponDataAsset* Weapon = NewObject<UMordecaiWeaponDataAsset>();
		Weapon->WeaponId = Id;
		Weapon->DisplayName = FText::FromName(Id);
		Weapon->WeaponType = EMordecaiWeaponType::Longsword;
		Weapon->EquipSlot = EMordecaiEquipSlot::MainHand;
		Weapon->GrantedTags.AddTag(TypeTag);

		FMordecaiStatModifier& Modifier = Weapon->StatModifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UMordecaiAttributeSet::GetStrengthAttribute();
		Modifier.Operation = EMordecaiModifierOp::Add;
		Modifier.Value = StrengthBonus;
		return Weapon;
	}

	FMordecaiWeaponInstance MakeInstance(UMordecaiWeaponDataAsset* Weapon)
	{
		FMordecaiWeaponInstance Instance;
		Instance.InstanceId = FGuid::NewGuid();
		Instance.WeaponDataAsset = Weapon;
		return Instance;
	}

	float Strength(const UAbilitySystemComponent* ASC)
	{
		return ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStrengthAttribute());
	}

	ALyraPlayerController* PossessWithPlayerState(FRecoveryTestEnv& Env)
	{
		ALyraPlayerController* PC =
			Env.World->SpawnActor<ALyraPlayerController>(ALyraPlayerController::StaticClass());
		if (!PC)
		{
			return nullptr;
		}

		PC->PlayerState = Env.PlayerState;
		PC->Possess(Env.Player);
		return PC->GetPawn() == Env.Player && Env.Player->GetPlayerState() == Env.PlayerState
			? PC
			: nullptr;
	}
}

using namespace MordecaiRecoveryTest;
using namespace MordecaiRecoveryWeaponEffectsTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_EffectsAppliedToPlayerStateASC,
	"Mordecai.Recovery.Weapons.EffectsAppliedToPlayerStateASC",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_EffectsAppliedToPlayerStateASC::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!Env.Setup(FVector::ZeroVector))
	{
		AddError(TEXT("Failed to create the recovery player"));
		return false;
	}

	ULyraAbilitySystemComponent* PlayerStateASC = Env.PlayerState->GetLyraAbilitySystemComponent();
	UMordecaiEquipmentComponent* Equipment = Env.Player->GetEquipmentComponent();
	TestNotNull(TEXT("PlayerState owns an ASC"), PlayerStateASC);
	TestNotNull(TEXT("Player pawn owns equipment"), Equipment);
	if (!PlayerStateASC || !Equipment)
	{
		Env.Teardown();
		return false;
	}

	const float OriginalStrength = Strength(PlayerStateASC);
	UMordecaiWeaponDataAsset* Weapon = MakeWeapon(
		TEXT("RecoverySword"),
		MordecaiGameplayTags::Weapon_Type_Sword,
		5.f);
	FMordecaiWeaponInstance Instance = MakeInstance(Weapon);

	TestTrue(TEXT("Equip succeeds without a test ASC override"),
		Equipment->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("Synthetic +5 Strength modifier reaches the PlayerState ASC"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));
	TestTrue(TEXT("Synthetic weapon tag reaches the PlayerState ASC"),
		PlayerStateASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));

	TestTrue(TEXT("Unequip succeeds"), Equipment->UnequipWeapon(EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("Unequip restores Strength"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength, 0.01f));
	TestFalse(TEXT("Unequip removes the weapon tag"),
		PlayerStateASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_PlayerStateFallbackRemainsSingleAcrossASCInitialization,
	"Mordecai.Recovery.Weapons.PlayerStateFallbackRemainsSingleAcrossASCInitialization",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_PlayerStateFallbackRemainsSingleAcrossASCInitialization::RunTest(
	const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!Env.Setup(FVector::ZeroVector))
	{
		AddError(TEXT("Failed to create the recovery player"));
		return false;
	}

	ALyraPlayerController* PC = PossessWithPlayerState(Env);
	ULyraPawnExtensionComponent* PawnExt =
		ULyraPawnExtensionComponent::FindPawnExtensionComponent(Env.Player);
	ULyraAbilitySystemComponent* PlayerStateASC = Env.PlayerState->GetLyraAbilitySystemComponent();
	UMordecaiEquipmentComponent* Equipment = Env.Player->GetEquipmentComponent();
	if (!PC || !PawnExt || !PlayerStateASC || !Equipment)
	{
		AddError(TEXT("Recovery player is missing controller, pawn extension, ASC, or equipment"));
		Env.Teardown();
		return false;
	}

	const float OriginalStrength = Strength(PlayerStateASC);
	const int32 OriginalTagCount =
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword);
	PawnExt->UninitializeAbilitySystem();

	FMordecaiWeaponInstance Instance = MakeInstance(MakeWeapon(
		TEXT("FallbackSword"), MordecaiGameplayTags::Weapon_Type_Sword, 5.f));
	TestTrue(TEXT("Equip succeeds after the pawn extension releases the ASC"),
		Equipment->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("PlayerState fallback applies +5 Strength before pawn ASC initialization"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));
	TestEqual(TEXT("PlayerState fallback adds exactly one weapon tag"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword),
		OriginalTagCount + 1);

	PawnExt->InitializeAbilitySystem(PlayerStateASC, Env.PlayerState);
	TestTrue(TEXT("ASC initialization does not stack the existing modifier"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));
	TestEqual(TEXT("ASC initialization does not stack the existing tag"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword),
		OriginalTagCount + 1);

	Equipment->RefreshWeaponEffects();
	Equipment->RefreshWeaponEffects();
	TestTrue(TEXT("Repeated refreshes keep one modifier"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));
	TestEqual(TEXT("Repeated refreshes keep one weapon tag"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword),
		OriginalTagCount + 1);

	Equipment->ClearWeaponEffects();
	PC->PlayerState = nullptr;
	PC->Destroy();
	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_EffectsAppliedAfterLateASCInitialization,
	"Mordecai.Recovery.Weapons.EffectsAppliedAfterLateASCInitialization",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_EffectsAppliedAfterLateASCInitialization::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!Env.Setup(FVector::ZeroVector))
	{
		AddError(TEXT("Failed to create the recovery player"));
		return false;
	}

	ULyraPawnExtensionComponent* PawnExt =
		ULyraPawnExtensionComponent::FindPawnExtensionComponent(Env.Player);
	ULyraAbilitySystemComponent* PlayerStateASC = Env.PlayerState->GetLyraAbilitySystemComponent();
	UMordecaiEquipmentComponent* Equipment = Env.Player->GetEquipmentComponent();
	if (!PawnExt || !PlayerStateASC || !Equipment)
	{
		AddError(TEXT("Recovery player is missing pawn extension, ASC, or equipment"));
		Env.Teardown();
		return false;
	}

	const float OriginalStrength = Strength(PlayerStateASC);
	PawnExt->UninitializeAbilitySystem();

	UMordecaiWeaponDataAsset* Weapon = MakeWeapon(
		TEXT("LateRecoverySword"),
		MordecaiGameplayTags::Weapon_Type_Sword,
		5.f);
	FMordecaiWeaponInstance Instance = MakeInstance(Weapon);
	TestTrue(TEXT("Equip succeeds while the pawn ASC is unavailable"),
		Equipment->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("No modifier is applied before ASC initialization"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength, 0.01f));

	PawnExt->InitializeAbilitySystem(PlayerStateASC, Env.PlayerState);
	PawnExt->InitializeAbilitySystem(PlayerStateASC, Env.PlayerState);
	TestTrue(TEXT("Late ASC initialization applies +5 Strength exactly once"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));
	TestEqual(TEXT("Late ASC initialization adds one weapon tag"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword), 1);

	PawnExt->UninitializeAbilitySystem();
	TestTrue(TEXT("ASC detach cleans the modifier from its recorded ASC"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength, 0.01f));
	TestFalse(TEXT("ASC detach cleans the tag from its recorded ASC"),
		PlayerStateASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));

	PawnExt->InitializeAbilitySystem(PlayerStateASC, Env.PlayerState);
	TestTrue(TEXT("Reinitialization reapplies +5 Strength exactly once"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));
	TestEqual(TEXT("Reinitialization still has one weapon tag"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword), 1);

	ULyraAbilitySystemComponent* DifferentASC =
		NewObject<ULyraAbilitySystemComponent>(Env.PlayerState);
	Equipment->SetASCOverride(DifferentASC);
	Equipment->ClearWeaponEffects();
	TestTrue(TEXT("Clear removes the modifier from the originally recorded ASC"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength, 0.01f));
	TestFalse(TEXT("Clear removes the tag from the originally recorded ASC"),
		PlayerStateASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));

	Equipment->SetASCOverride(nullptr);
	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_PawnDestroyClearsRecordedPlayerStateEffects,
	"Mordecai.Recovery.Weapons.PawnDestroyClearsRecordedPlayerStateEffects",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_PawnDestroyClearsRecordedPlayerStateEffects::RunTest(
	const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!Env.Setup(FVector::ZeroVector))
	{
		AddError(TEXT("Failed to create the recovery player"));
		return false;
	}

	ALyraPlayerController* PC = PossessWithPlayerState(Env);
	ULyraAbilitySystemComponent* PlayerStateASC = Env.PlayerState->GetLyraAbilitySystemComponent();
	UMordecaiEquipmentComponent* Equipment = Env.Player->GetEquipmentComponent();
	if (!PC || !PlayerStateASC || !Equipment)
	{
		AddError(TEXT("Recovery player is missing controller, ASC, or equipment"));
		Env.Teardown();
		return false;
	}

	const float OriginalStrength = Strength(PlayerStateASC);
	const int32 OriginalTagCount =
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword);
	FMordecaiWeaponInstance Instance = MakeInstance(MakeWeapon(
		TEXT("DestroyedPawnSword"), MordecaiGameplayTags::Weapon_Type_Sword, 5.f));
	TestTrue(TEXT("Equip succeeds"),
		Equipment->EquipWeapon(Instance, EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("Modifier is active before unpossess"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));
	TestEqual(TEXT("Tag is active before unpossess"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword),
		OriginalTagCount + 1);

	PC->UnPossess();
	TestNull(TEXT("Lyra controller clears the surviving ASC avatar"), PlayerStateASC->GetAvatarActor());
	Env.Player->Destroy();

	TestTrue(TEXT("Pawn component teardown restores PlayerState Strength"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength, 0.01f));
	TestEqual(TEXT("Pawn component teardown restores the weapon tag count"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword),
		OriginalTagCount);

	Equipment->ClearWeaponEffects();
	TestTrue(TEXT("A second clear is a no-op for Strength"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength, 0.01f));
	TestEqual(TEXT("A second clear is a no-op for the tag"),
		PlayerStateASC->GetTagCount(MordecaiGameplayTags::Weapon_Type_Sword),
		OriginalTagCount);

	PC->PlayerState = nullptr;
	PC->Destroy();
	Env.Player = nullptr;
	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_SwapDoesNotStackPreviousEffects,
	"Mordecai.Recovery.Weapons.SwapDoesNotStackPreviousEffects",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_SwapDoesNotStackPreviousEffects::RunTest(const FString& Parameters)
{
	FRecoveryTestEnv Env;
	if (!Env.Setup(FVector::ZeroVector))
	{
		AddError(TEXT("Failed to create the recovery player"));
		return false;
	}

	ULyraAbilitySystemComponent* PlayerStateASC = Env.PlayerState->GetLyraAbilitySystemComponent();
	UMordecaiEquipmentComponent* Equipment = Env.Player->GetEquipmentComponent();
	if (!PlayerStateASC || !Equipment)
	{
		AddError(TEXT("Recovery player is missing ASC or equipment"));
		Env.Teardown();
		return false;
	}

	const float OriginalStrength = Strength(PlayerStateASC);
	FMordecaiWeaponInstance Sword = MakeInstance(MakeWeapon(
		TEXT("SwapSword"), MordecaiGameplayTags::Weapon_Type_Sword, 5.f));
	FMordecaiWeaponInstance Dagger = MakeInstance(MakeWeapon(
		TEXT("SwapDagger"), MordecaiGameplayTags::Weapon_Type_Dagger, 2.f));

	TestTrue(TEXT("Equip first weapon"),
		Equipment->EquipWeapon(Sword, EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("First modifier is active"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 5.f, 0.01f));

	TestTrue(TEXT("Swap to second weapon"),
		Equipment->EquipWeapon(Dagger, EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("Only the current modifier remains after swap"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength + 2.f, 0.01f));
	TestFalse(TEXT("Previous weapon tag is absent"),
		PlayerStateASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Sword));
	TestTrue(TEXT("Current weapon tag is present"),
		PlayerStateASC->HasMatchingGameplayTag(MordecaiGameplayTags::Weapon_Type_Dagger));

	TestTrue(TEXT("Unequip current weapon"),
		Equipment->UnequipWeapon(EMordecaiEquipSlot::MainHand));
	TestTrue(TEXT("No modifier remains after unequip"),
		FMath::IsNearlyEqual(Strength(PlayerStateASC), OriginalStrength, 0.01f));

	Env.Teardown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMordecai_Recovery_Weapons_SerializedAssetsHaveGrantedTags,
	"Mordecai.Recovery.Weapons.SerializedAssetsHaveGrantedTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Weapons_SerializedAssetsHaveGrantedTags::RunTest(const FString& Parameters)
{
	struct FExpectedWeaponTag
	{
		const TCHAR* AssetPath;
		FGameplayTag Tag;
	};
	const FExpectedWeaponTag Expectations[] = {
		{TEXT("/MordecaiCore/Weapons/DA_Weapon_Longsword.DA_Weapon_Longsword"),
			MordecaiGameplayTags::Weapon_Type_Sword},
		{TEXT("/MordecaiCore/Weapons/DA_Weapon_Dagger.DA_Weapon_Dagger"),
			MordecaiGameplayTags::Weapon_Type_Dagger},
		{TEXT("/MordecaiCore/Weapons/DA_Weapon_Greatsword.DA_Weapon_Greatsword"),
			MordecaiGameplayTags::Weapon_Type_Sword},
		{TEXT("/MordecaiCore/Weapons/DA_Weapon_Spear.DA_Weapon_Spear"),
			MordecaiGameplayTags::Weapon_Type_Spear},
		{TEXT("/MordecaiCore/Weapons/DA_Weapon_Mace.DA_Weapon_Mace"),
			MordecaiGameplayTags::Weapon_Type_Mace},
	};

	for (const FExpectedWeaponTag& Expected : Expectations)
	{
		UMordecaiWeaponDataAsset* Weapon =
			LoadObject<UMordecaiWeaponDataAsset>(nullptr, Expected.AssetPath);
		TestNotNull(FString::Printf(TEXT("%s loads"), Expected.AssetPath), Weapon);
		if (Weapon)
		{
			TestTrue(
				FString::Printf(TEXT("%s serializes %s"),
					Expected.AssetPath, *Expected.Tag.ToString()),
				Weapon->GrantedTags.HasTagExact(Expected.Tag));
		}
	}
	return true;
}
