// Project Mordecai — retained combat HUD child delegate lifecycle regressions (US-086)

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiPostureBarWidget.h"
#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiHUDRebindingTest
{
	template <typename WidgetT>
	WidgetT* FindChild(const UMordecaiCombatHUDWidget* HUD)
	{
		TArray<UWidget*> Widgets;
		if (HUD && HUD->WidgetTree)
		{
			HUD->WidgetTree->GetAllWidgets(Widgets);
		}
		for (UWidget* Widget : Widgets)
		{
			if (WidgetT* Match = Cast<WidgetT>(Widget))
			{
				return Match;
			}
		}
		return nullptr;
	}

	void Rebuild(UMordecaiCombatHUDWidget* HUD, TSharedPtr<SWidget>& SlateTree)
	{
		SlateTree.Reset();
		SlateTree = HUD->TakeWidget();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_RebindsChildrenToNewASC,
	"Mordecai.Recovery.HUD.RebindsChildrenToNewASC",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_RebindsChildrenToNewASC::RunTest(const FString& Parameters)
{
	using namespace MordecaiHUDRebindingTest;
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled production-style world set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	AMordecaiEnemyCharacter* SourceA = Env.SpawnEnemy(FVector(500.f, 0.f, 100.f), {});
	AMordecaiEnemyCharacter* SourceB = Env.SpawnEnemy(FVector(700.f, 0.f, 100.f), {});
	UAbilitySystemComponent* ASCA = SourceA ? SourceA->GetAbilitySystemComponent() : nullptr;
	UAbilitySystemComponent* ASCB = SourceB ? SourceB->GetAbilitySystemComponent() : nullptr;
	if (!TestNotNull("ASC A", ASCA) || !TestNotNull("ASC B", ASCB))
	{
		Env.TeardownWithController();
		return true;
	}

	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	UMordecaiHealthBarWidget* HealthBar = FindChild<UMordecaiHealthBarWidget>(HUD);
	TestNotNull("HUD health bar", HealthBar);
	HUD->BindToASC(ASCA);
	TSharedPtr<SWidget> RebuiltSlateTree = HUD->TakeWidget();
	Rebuild(HUD, RebuiltSlateTree);
	HUD->BindToASC(ASCB);

	ASCB->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 80.f);
	TestEqual("ASC B drives the rebuilt display to 80%", HealthBar->GetHealthPercent(), 0.8f);
	ASCA->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 20.f);
	TestEqual("stale ASC A cannot overwrite the display after rebinding to B",
		HealthBar->GetHealthPercent(), 0.8f);

	Env.TeardownWithController();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_HUD_RepeatedRebuildsKeepOneChildCallback,
	"Mordecai.Recovery.HUD.RepeatedRebuildsKeepOneChildCallback",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_HUD_RepeatedRebuildsKeepOneChildCallback::RunTest(const FString& Parameters)
{
	using namespace MordecaiHUDRebindingTest;
	using namespace MordecaiRecoveryTest;

	FRecoveryControlledTestEnv Env;
	if (!TestTrue("controlled production-style world set up", Env.SetupWithController()))
	{
		Env.TeardownWithController();
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	HUD->BindToASC(ASC);
	TSharedPtr<SWidget> RebuiltSlateTree = HUD->TakeWidget();
	Rebuild(HUD, RebuiltSlateTree);
	Rebuild(HUD, RebuiltSlateTree);
	Rebuild(HUD, RebuiltSlateTree);

	UMordecaiHealthBarWidget* Health = FindChild<UMordecaiHealthBarWidget>(HUD);
	UMordecaiStaminaBarWidget* Stamina = FindChild<UMordecaiStaminaBarWidget>(HUD);
	UMordecaiPostureBarWidget* Posture = FindChild<UMordecaiPostureBarWidget>(HUD);
	UMordecaiSpellPointsBarWidget* SpellPoints = FindChild<UMordecaiSpellPointsBarWidget>(HUD);
	UMordecaiStatusEffectBarWidget* Status = FindChild<UMordecaiStatusEffectBarWidget>(HUD);
	UMordecaiComboCounterWidget* Combo = FindChild<UMordecaiComboCounterWidget>(HUD);
	UMordecaiCombatFeedbackWidget* Feedback = FindChild<UMordecaiCombatFeedbackWidget>(HUD);
	TestNotNull("health child", Health);
	TestNotNull("stamina child", Stamina);
	TestNotNull("posture child", Posture);
	TestNotNull("spell-points child", SpellPoints);
	TestNotNull("status child", Status);
	TestNotNull("combo child", Combo);
	TestNotNull("feedback child", Feedback);

	TestEqual("one Health callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetHealthAttribute()).RemoveAll(Health), 1);
	TestEqual("one MaxHealth callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxHealthAttribute()).RemoveAll(Health), 1);
	TestEqual("one Stamina callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetStaminaAttribute()).RemoveAll(Stamina), 1);
	TestEqual("one MaxStamina callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxStaminaAttribute()).RemoveAll(Stamina), 1);
	TestEqual("one Posture callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetPostureAttribute()).RemoveAll(Posture), 1);
	TestEqual("one MaxPosture callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxPostureAttribute()).RemoveAll(Posture), 1);
	TestEqual("one SpellPoints callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetSpellPointsAttribute()).RemoveAll(SpellPoints), 1);
	TestEqual("one MaxSpellPoints callback after repeated rebuilds",
		ASC->GetGameplayAttributeValueChangeDelegate(UMordecaiAttributeSet::GetMaxSpellPointsAttribute()).RemoveAll(SpellPoints), 1);
	TestEqual("one PostureBroken callback after repeated rebuilds",
		ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_PostureBroken, EGameplayTagEventType::NewOrRemoved)
			.RemoveAll(Posture), 1);

	const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(TEXT("Mordecai.Status"));
	TestEqual("one status-root callback after repeated rebuilds",
		ASC->RegisterGameplayTagEvent(StatusRoot, EGameplayTagEventType::NewOrRemoved).RemoveAll(Status), 1);
	TestEqual("one combo callback after repeated rebuilds",
		ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_ComboWindow, EGameplayTagEventType::NewOrRemoved)
			.RemoveAll(Combo), 1);

	const FGameplayTag FeedbackTags[] = {
		MordecaiGameplayTags::State_PerfectDodge,
		MordecaiGameplayTags::State_PerfectBlock,
		MordecaiGameplayTags::State_Parried,
		MordecaiGameplayTags::State_ParryWhiff,
		MordecaiGameplayTags::State_PostureBroken,
	};
	for (const FGameplayTag& Tag : FeedbackTags)
	{
		TestEqual(*FString::Printf(TEXT("one feedback callback for %s"), *Tag.ToString()),
			ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(Feedback), 1);
	}

	Env.TeardownWithController();
	return true;
}
