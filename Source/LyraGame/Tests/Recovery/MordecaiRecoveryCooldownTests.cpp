// Project Mordecai -- Recovery: spell cooldown tags, authoritative GAS blocking, and HUD binding
// (US-088 / US-089, issue #35).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "GameplayEffect.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Magic/MordecaiGA_Blink.h"
#include "Mordecai/Magic/MordecaiGA_Fireball.h"
#include "Mordecai/Magic/MordecaiGA_SpellBase.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/MordecaiPlayerController.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "UObject/UnrealType.h"

namespace MordecaiRecoveryCooldownTest
{
	using namespace MordecaiRecoveryTest;

	constexpr float InitialSP = 10.f;
	constexpr float SpellCost = 3.f;
	constexpr float CooldownSeconds = 0.2f;
	constexpr float FrameSeconds = 0.05f;

	FGameplayTag Tag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name), false);
	}

	UMordecaiSpellDataAsset* MakeSpellData(
		UObject* Outer,
		const FName SpellId,
		const FGameplayTag CooldownTag,
		const float CooldownDuration)
	{
		UMordecaiSpellDataAsset* Data = NewObject<UMordecaiSpellDataAsset>(Outer);
		Data->SpellId = SpellId;
		Data->DisplayName = FText::FromName(SpellId);
		Data->SpellPointCost = static_cast<int32>(SpellCost);
		Data->CooldownTag = CooldownTag;
		Data->CooldownDuration = CooldownDuration;
		Data->WindupTime = 0.f;
		Data->CastTime = 0.f;
		Data->RecoveryTime = 0.f;
		Data->MovementPolicy = EMordecaiMovementPolicy::FreeMove;
		return Data;
	}

	void Tick(FRecoveryTestEnv& Env, const int32 Frames)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			++GFrameCounter;
			Env.World->TimeSeconds += FrameSeconds;
			Env.World->GetTimerManager().Tick(FrameSeconds);
		}
	}

	float MaxRemaining(UAbilitySystemComponent* ASC, const FGameplayTag CooldownTag)
	{
		if (!ASC || !CooldownTag.IsValid())
		{
			return 0.f;
		}
		const FGameplayEffectQuery Query =
			FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
		float Remaining = 0.f;
		for (const TPair<float, float>& TimeAndDuration : ASC->GetActiveEffectsTimeRemainingAndDuration(Query))
		{
			Remaining = FMath::Max(Remaining, TimeAndDuration.Key);
		}
		return Remaining;
	}

	UMordecaiSpellCooldownWidget* CooldownWidget(
		UMordecaiCombatHUDWidget* HUD,
		const TCHAR* PropertyName)
	{
		const FObjectProperty* Property =
			FindFProperty<FObjectProperty>(UMordecaiCombatHUDWidget::StaticClass(), FName(PropertyName));
		return Property ? Cast<UMordecaiSpellCooldownWidget>(
			Property->GetObjectPropertyValue_InContainer(HUD)) : nullptr;
	}

	FGameplayTag BoundTag(const UMordecaiSpellCooldownWidget* Widget)
	{
		const FStructProperty* Property =
			FindFProperty<FStructProperty>(UMordecaiSpellCooldownWidget::StaticClass(), TEXT("BoundCooldownTag"));
		const FGameplayTag* Value = Property && Widget
			? Property->ContainerPtrToValuePtr<FGameplayTag>(Widget) : nullptr;
		return Value ? *Value : FGameplayTag();
	}

	void SetHUD(AMordecaiPlayerController* Controller, UMordecaiCombatHUDWidget* HUD)
	{
		FObjectProperty* Property =
			FindFProperty<FObjectProperty>(AMordecaiPlayerController::StaticClass(), TEXT("CombatHUDWidget"));
		if (Property)
		{
			Property->SetObjectPropertyValue_InContainer(Controller, HUD);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Cooldown_NativeSpellCooldownTags,
	"Mordecai.Recovery.Cooldown.NativeSpellCooldownTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Cooldown_NativeSpellCooldownTags::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryCooldownTest;
	for (const TCHAR* Name : {
		TEXT("Mordecai.Cooldown.Spell"),
		TEXT("Mordecai.Cooldown.Spell.Fireball"),
		TEXT("Mordecai.Cooldown.Spell.Blink"),
		TEXT("Mordecai.Cooldown.Spell.StoneSkin"),
		TEXT("Mordecai.Cooldown.Spell.Restoration") })
	{
		const FGameplayTag CooldownTag = Tag(Name);
		TestTrue(FString::Printf(TEXT("%s is a registered native gameplay tag"), Name), CooldownTag.IsValid());
		if (CooldownTag.IsValid())
		{
			TestEqual(FString::Printf(TEXT("%s has the exact requested name"), Name),
				CooldownTag.GetTagName(), FName(Name));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Cooldown_SpellBlockedDuringCooldown,
	"Mordecai.Recovery.Cooldown.SpellBlockedDuringCooldown",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Cooldown_SpellBlockedDuringCooldown::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryCooldownTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("real player and PlayerState ASC", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	const FGameplayTag RequestedFireballCooldown = Tag(TEXT("Mordecai.Cooldown.Spell.Fireball"));
	TestTrue("Fireball cooldown tag is registered", RequestedFireballCooldown.IsValid());
	// Keep exercising the real GAS behavior during RED even before the requested native tag exists.
	const FGameplayTag FireballCooldown = RequestedFireballCooldown.IsValid()
		? RequestedFireballCooldown : MordecaiGameplayTags::Ability_Spell_Fireball;
	if (!ASC)
	{
		AddError(TEXT("Player ASC is null"));
		return true;
	}
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), InitialSP);
	UMordecaiSpellDataAsset* Data =
		MakeSpellData(ASC, TEXT("CooldownTest"), FireballCooldown, CooldownSeconds);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(
		FGameplayAbilitySpec(UMordecaiGA_SpellBase::StaticClass(), 1, INDEX_NONE, Data));

	TestTrue("first real ASC activation succeeds", ASC->TryActivateAbility(Handle));
	TestEqual("first activation spends exactly 3 SP",
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetSpellPointsAttribute()), InitialSP - SpellCost);
	TestTrue("the timed cooldown tag is owned by the ASC", ASC->HasMatchingGameplayTag(FireballCooldown));
	TestTrue("the active cooldown effect reports remaining time > 0",
		MaxRemaining(ASC, FireballCooldown) > 0.f);

	TestFalse("GAS rejects a second activation while the cooldown tag is active",
		ASC->TryActivateAbility(Handle));
	TestEqual("the rejected activation spends no SP",
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetSpellPointsAttribute()), InitialSP - SpellCost);

	Tick(Env, 6);
	TestFalse("the timed cooldown tag expires", ASC->HasMatchingGameplayTag(FireballCooldown));
	TestTrue("activation succeeds after expiry", ASC->TryActivateAbility(Handle));
	TestEqual("the post-expiry activation spends SP once",
		ASC->GetNumericAttribute(UMordecaiAttributeSet::GetSpellPointsAttribute()),
		InitialSP - (2.f * SpellCost));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Cooldown_WidgetsBoundToSlotTags,
	"Mordecai.Recovery.Cooldown.WidgetsBoundToSlotTags",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Cooldown_WidgetsBoundToSlotTags::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryCooldownTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player with a PlayerState ASC", Env.SetupWithController()))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	const FGameplayTag RequestedFireballCooldown = Tag(TEXT("Mordecai.Cooldown.Spell.Fireball"));
	const FGameplayTag RequestedBlinkCooldown = Tag(TEXT("Mordecai.Cooldown.Spell.Blink"));
	TestTrue("Fireball cooldown tag is registered", RequestedFireballCooldown.IsValid());
	TestTrue("Blink cooldown tag is registered", RequestedBlinkCooldown.IsValid());
	// Existing ability tags isolate HUD/delegate RED behavior until the cooldown tags are added.
	const FGameplayTag FireballCooldown = RequestedFireballCooldown.IsValid()
		? RequestedFireballCooldown : MordecaiGameplayTags::Ability_Spell_Fireball;
	const FGameplayTag BlinkCooldown = RequestedBlinkCooldown.IsValid()
		? RequestedBlinkCooldown : MordecaiGameplayTags::Ability_Spell_Blink;

	AMordecaiPlayerController* GrantController = NewObject<AMordecaiPlayerController>();
	GrantController->PlayerState = Env.PlayerState;
	GrantController->DefaultCombatAbilities.Reset();
	GrantController->DefaultSpellAbilities.Reset();
	GrantController->DefaultSpellAbilities.Add(UMordecaiGA_Fireball::StaticClass());
	GrantController->DefaultSpellAbilities.Add(UMordecaiGA_Blink::StaticClass());
	GrantController->SpellDataAssets.Reset();
	GrantController->SpellDataAssets.Add(MakeSpellData(
		GrantController, TEXT("Fireball"), FireballCooldown, 2.f));
	GrantController->SpellDataAssets.Add(MakeSpellData(
		GrantController, TEXT("Blink"), BlinkCooldown, 4.f));

	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	SetHUD(GrantController, HUD);
	HUD->BindToASC(ASC); // The live HUD is created and ASC-bound before the controller grants spells.
	GrantController->GrantCombatAbilities();

	UMordecaiSpellCooldownWidget* WidgetA = CooldownWidget(HUD, TEXT("SpellCooldownA"));
	UMordecaiSpellCooldownWidget* WidgetB = CooldownWidget(HUD, TEXT("SpellCooldownB"));
	if (!TestNotNull("slot A cooldown widget", WidgetA)
		|| !TestNotNull("slot B cooldown widget", WidgetB))
	{
		return true;
	}
	TestEqual("slot A is bound to slot 0 Fireball", BoundTag(WidgetA), FireballCooldown);
	TestEqual("slot B is bound to slot 1 Blink", BoundTag(WidgetB), BlinkCooldown);

	// Rebinding the same live widget must replace its delegate, not register another callback.
	WidgetA->BindToCooldownTag(ASC, FireballCooldown, 2.f);
	WidgetA->BindToCooldownTag(ASC, FireballCooldown, 2.f);
	const int32 SameTagBindings =
		ASC->RegisterGameplayTagEvent(FireballCooldown, EGameplayTagEventType::NewOrRemoved)
			.RemoveAll(WidgetA);
	TestEqual("same-tag rebind leaves one delegate registration", SameTagBindings, 1);

	// Rebinding a respawned/reused widget to another slot must release the prior tag's delegate.
	WidgetA->BindToCooldownTag(ASC, FireballCooldown, 2.f);
	WidgetA->BindToCooldownTag(ASC, BlinkCooldown, 4.f);
	TestFalse("old Fireball delegate is unregistered after a Blink rebind",
		ASC->RegisterGameplayTagEvent(FireballCooldown, EGameplayTagEventType::NewOrRemoved)
			.IsBoundToObject(WidgetA));
	TestTrue("new Blink delegate is registered",
		ASC->RegisterGameplayTagEvent(BlinkCooldown, EGameplayTagEventType::NewOrRemoved)
			.IsBoundToObject(WidgetA));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Recovery_Cooldown_WarnsOnMissingCooldownTag,
	"Mordecai.Recovery.Cooldown.WarnsOnMissingCooldownTag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Recovery_Cooldown_WarnsOnMissingCooldownTag::RunTest(const FString& Parameters)
{
	using namespace MordecaiRecoveryCooldownTest;
	FRecoveryTestEnv Env;
	ON_SCOPE_EXIT { Env.Teardown(); };
	if (!TestTrue("real player and PlayerState ASC", Env.Setup(FVector::ZeroVector)))
	{
		return true;
	}

	AMordecaiPlayerController* GrantController = NewObject<AMordecaiPlayerController>();
	GrantController->PlayerState = Env.PlayerState;
	GrantController->DefaultCombatAbilities.Reset();
	GrantController->DefaultSpellAbilities.Reset();
	GrantController->DefaultSpellAbilities.Add(UMordecaiGA_SpellBase::StaticClass());
	GrantController->SpellDataAssets.Reset();
	GrantController->SpellDataAssets.Add(MakeSpellData(
		GrantController, TEXT("MissingCooldown"), FGameplayTag(), 2.f));

	AddExpectedErrorPlain(
		TEXT("spell slot 0 'MissingCooldown' has CooldownDuration 2.00 but no CooldownTag"),
		EAutomationExpectedErrorFlags::Contains,
		1);
	GrantController->GrantCombatAbilities();
	GrantController->GrantCombatAbilities(); // Idempotent grant: the warning is emitted exactly once.
	return true;
}

#endif // WITH_AUTOMATION_TESTS
