// Project Mordecai — Tin Tally status chips and centre-lane feedback (US-106).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "UObject/UnrealType.h"

namespace MordecaiTinTallyStatusFeedbackTest
{
	using namespace MordecaiRecoveryTest;

	template <typename T>
	T* Find(const UUserWidget* Owner, const TCHAR* Name)
	{
		return Owner && Owner->WidgetTree
			? Cast<T>(Owner->WidgetTree->FindWidget(FName(Name)))
			: nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_StatusChip_WrapsAndCapsAtEight,
	"Mordecai.UI.StatusChip.WrapsAndCapsAtEight",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_StatusChip_WrapsAndCapsAtEight::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyStatusFeedbackTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	const FGameplayTag Statuses[] = {
		MordecaiGameplayTags::Status_Burning,
		MordecaiGameplayTags::Status_Bleeding,
		MordecaiGameplayTags::Status_Poisoned,
		MordecaiGameplayTags::Status_Frostbitten,
		MordecaiGameplayTags::Status_Shocked,
		MordecaiGameplayTags::Status_Weakened,
		MordecaiGameplayTags::Status_Brittle,
		MordecaiGameplayTags::Status_Silenced,
		MordecaiGameplayTags::Status_Rooted,
	};
	for (const FGameplayTag& Status : Statuses)
	{
		ASC->AddLooseGameplayTag(Status);
	}

	UMordecaiStatusEffectBarWidget* Bar =
		Env.MakeWidget<UMordecaiStatusEffectBarWidget>();
	Bar->BindToASC(ASC);
	UPanelWidget* Container = Find<UPanelWidget>(Bar, TEXT("IndicatorContainer"));
	UWrapBox* Wrap = Cast<UWrapBox>(Container);
	TestNotNull("IndicatorContainer is panel-compatible at runtime", Container);
	TestNotNull("fallback IndicatorContainer is a WrapBox", Wrap);
	TestEqual("WrapBox uses the 456px strip width",
		Wrap ? Wrap->GetWrapSize() : 0.f, 456.f);
	TestTrue("visible children never exceed eight",
		Container && Container->GetChildrenCount()
			<= UMordecaiStatusEffectBarWidget::MaxVisibleIndicators);
	TestEqual("nine statuses still materialize exactly eight visible chip slots",
		Container ? Container->GetChildrenCount() : 0,
		UMordecaiStatusEffectBarWidget::MaxVisibleIndicators);
	TestNotNull("OverflowChip native fallback exists",
		Find<UWidget>(Bar, TEXT("OverflowChip")));

	const FObjectPropertyBase* ContainerProperty =
		FindFProperty<FObjectPropertyBase>(
			UMordecaiStatusEffectBarWidget::StaticClass(), TEXT("IndicatorContainer"));
	TestTrue("IndicatorContainer UPROPERTY accepts any panel implementation",
		ContainerProperty
		&& ContainerProperty->PropertyClass == UPanelWidget::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_StatusChip_DisplayNameAndPolarity,
	"Mordecai.UI.StatusChip.DisplayNameAndPolarity",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_StatusChip_DisplayNameAndPolarity::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyStatusFeedbackTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}

	UMordecaiStatusEffectIndicatorWidget* Buff =
		Env.MakeWidget<UMordecaiStatusEffectIndicatorWidget>();
	UMordecaiStatusEffectIndicatorWidget* Debuff =
		Env.MakeWidget<UMordecaiStatusEffectIndicatorWidget>();
	Buff->InitializeIndicator(MordecaiGameplayTags::Status_StoneSkin, true, 10.f);
	Debuff->InitializeIndicator(MordecaiGameplayTags::Status_Burning, false, 5.f);

	const UTextBlock* BuffName = Find<UTextBlock>(Buff, TEXT("StatusNameText"));
	const UTextBlock* DebuffName = Find<UTextBlock>(Debuff, TEXT("StatusNameText"));
	TestEqual("StoneSkin uses approved player-facing copy",
		BuffName ? BuffName->GetText().ToString() : FString(),
		FString(TEXT("Stone Skin")));
	TestEqual("Burning uses approved player-facing copy",
		DebuffName ? DebuffName->GetText().ToString() : FString(),
		FString(TEXT("Burning")));
	for (const TPair<UMordecaiStatusEffectIndicatorWidget*, FString>& Chip : {
		TPair<UMordecaiStatusEffectIndicatorWidget*, FString>(Buff, TEXT("buff")),
		TPair<UMordecaiStatusEffectIndicatorWidget*, FString>(Debuff, TEXT("debuff")) })
	{
		TestNotNull(*FString::Printf(TEXT("%s StatusGlyph fallback"), *Chip.Value),
			Find<UWidget>(Chip.Key, TEXT("StatusGlyph")));
		TestNotNull(*FString::Printf(TEXT("%s StackPips fallback"), *Chip.Value),
			Find<UWidget>(Chip.Key, TEXT("StackPips")));
		TestNotNull(*FString::Printf(TEXT("%s TintImage retained"), *Chip.Value),
			Find<UWidget>(Chip.Key, TEXT("TintImage")));
	}
	TestNotNull("buff polarity has a raised tab independent of colour",
		Find<UWidget>(Buff, TEXT("BuffTab")));
	TestNotNull("debuff polarity has a toothed hem independent of colour",
		Find<UWidget>(Debuff, TEXT("DebuffTeeth")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_CombatCallout_ReplacesAndExpires,
	"Mordecai.UI.CombatCallout.ReplacesAndExpires",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_CombatCallout_ReplacesAndExpires::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyStatusFeedbackTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}
	UMordecaiCombatFeedbackWidget* Callout =
		Env.MakeWidget<UMordecaiCombatFeedbackWidget>();
	TSharedRef<SWidget> Slate = Callout->TakeWidget();
	UTextBlock* FeedbackText = Find<UTextBlock>(Callout, TEXT("FeedbackText"));
	TestNotNull("CalloutPlate native fallback exists",
		Find<UWidget>(Callout, TEXT("CalloutPlate")));
	TestEqual("shipped duration is exactly one second",
		Callout->FeedbackDisplayDuration, 1.f);

	Callout->ShowFeedback(TEXT("Perfect Dodge!"), FLinearColor::Green);
	Callout->ShowFeedback(TEXT("Parry Failed!"), FLinearColor::Red);
	TestEqual("latest callout replaces the previous one immediately",
		FeedbackText ? FeedbackText->GetText().ToString() : FString(),
		FString(TEXT("Parry Failed!")));
	TestEqual("replacement remains visible",
		Callout->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Slate->Tick(FGeometry(), 1.01, 1.01f);
	TestEqual("callout collapses after one second",
		Callout->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Combo_SurvivesHUDReactivation,
	"Mordecai.UI.Combo.SurvivesHUDReactivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Combo_SurvivesHUDReactivation::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyStatusFeedbackTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	UMordecaiComboCounterWidget* Combo = Cast<UMordecaiComboCounterWidget>(
		HUD->GetNamedWidget(HUD, TEXT("ComboCounter")));
	if (!TestNotNull("HUD owns ComboCounter", Combo))
	{
		return true;
	}
	TestNotNull("ComboPlate native fallback exists",
		Find<UWidget>(Combo, TEXT("ComboPlate")));
	Combo->SetComboIndex(1);
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_ComboWindow);
	HUD->BindToASC(ASC);
	HUD->ActivateWidget();
	HUD->DeactivateWidget();
	HUD->ActivateWidget();

	const UTextBlock* ComboText = Find<UTextBlock>(Combo, TEXT("ComboText"));
	TestTrue("combo uses the shipped num-m Mono font",
		ComboText && ComboText->GetFont().FontObject
		&& ComboText->GetFont().FontObject->GetName() == TEXT("F_Tally_Mono"));
	TestEqual("combo uses Mono Bold",
		ComboText ? ComboText->GetFont().TypefaceFontName : NAME_None,
		FName(TEXT("Bold")));
	TestEqual("24 design px converts to 18 pt",
		ComboText ? static_cast<float>(ComboText->GetFont().Size) : 0.f,
		18.f);
	TestEqual("active combo keeps Hit N through HUD reactivation",
		ComboText ? ComboText->GetText().ToString() : FString(),
		FString(TEXT("Hit 2")));
	TestEqual("active combo remains visible after HUD reactivation",
		Combo->GetVisibility(), ESlateVisibility::HitTestInvisible);
	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_ComboWindow);
	TestEqual("combo hides when the active window closes",
		Combo->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
