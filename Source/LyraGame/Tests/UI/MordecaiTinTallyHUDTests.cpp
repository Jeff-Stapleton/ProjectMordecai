// Project Mordecai — Tin Tally final combat HUD contract (US-105).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiPostureBarWidget.h"
#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"

namespace MordecaiTinTallyHUDTest
{
	using namespace MordecaiRecoveryTest;

	template <typename T>
	T* Find(const UUserWidget* Owner, const TCHAR* Name)
	{
		return Owner && Owner->WidgetTree
			? Cast<T>(Owner->WidgetTree->FindWidget(FName(Name)))
			: nullptr;
	}

	bool IsShown(const UWidget* Widget)
	{
		return Widget
			&& Widget->GetVisibility() != ESlateVisibility::Collapsed
			&& Widget->GetVisibility() != ESlateVisibility::Hidden;
	}

	void ExpectCanvasBox(
		FAutomationTestBase& Test,
		const UWidget* Widget,
		const TCHAR* Label,
		const FAnchors& Anchors,
		const FVector2D& Position,
		const FVector2D& Size,
		const FVector2D& Alignment)
	{
		const UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s has a canvas slot"), Label), Slot))
		{
			return;
		}
		Test.TestEqual(*FString::Printf(TEXT("%s minimum anchor"), Label),
			Slot->GetAnchors().Minimum, Anchors.Minimum);
		Test.TestEqual(*FString::Printf(TEXT("%s maximum anchor"), Label),
			Slot->GetAnchors().Maximum, Anchors.Maximum);
		Test.TestEqual(*FString::Printf(TEXT("%s offset"), Label), Slot->GetPosition(), Position);
		Test.TestEqual(*FString::Printf(TEXT("%s size"), Label), Slot->GetSize(), Size);
		Test.TestEqual(*FString::Printf(TEXT("%s alignment"), Label), Slot->GetAlignment(), Alignment);
		Test.TestFalse(*FString::Printf(TEXT("%s is fixed-size"), Label), Slot->GetAutoSize());
	}

	bool InvokeMargin(UObject* Target, const float Margin)
	{
		UFunction* Function = Target
			? Target->GetClass()->FindFunctionByName(TEXT("SetHUDMargin"))
			: nullptr;
		if (!Function)
		{
			return false;
		}
		struct FParams
		{
			float InMargin = 0.f;
		} Params;
		Params.InMargin = Margin;
		Target->ProcessEvent(Function, &Params);
		return true;
	}

	bool ReadTopCenterSafeWidth(UObject* Target, float& OutWidth)
	{
		UFunction* Function = Target
			? Target->GetClass()->FindFunctionByName(TEXT("GetTopCenterSafeWidth"))
			: nullptr;
		if (!Function)
		{
			return false;
		}
		struct FParams
		{
			float ReturnValue = 0.f;
		} Params;
		Target->ProcessEvent(Function, &Params);
		OutWidth = Params.ReturnValue;
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_HUDLayout_FinalMapBoxes,
	"Mordecai.UI.HUDLayout.FinalMapBoxes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_HUDLayout_FinalMapBoxes::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyHUDTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}

	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	if (!TestNotNull("default combat HUD", HUD))
	{
		return true;
	}
	TestTrue("single combat HUD root is a canvas",
		HUD->WidgetTree && HUD->WidgetTree->RootWidget->IsA<UCanvasPanel>());

	UWidget* GaugePlate = Find<UWidget>(HUD, TEXT("GaugePlate"));
	UWidget* SlotA = Find<UWidget>(HUD, TEXT("SpellCooldownA"));
	UWidget* SlotB = Find<UWidget>(HUD, TEXT("SpellCooldownB"));
	UWidget* SlotC = Find<UWidget>(HUD, TEXT("SpellCooldownC"));
	UWidget* SlotD = Find<UWidget>(HUD, TEXT("SpellCooldownD"));
	UWidget* Status = Find<UWidget>(HUD, TEXT("StatusEffectBar"));
	UWidget* RailHead = Find<UWidget>(HUD, TEXT("TallyRailHead"));
	UWidget* Callout = Find<UWidget>(HUD, TEXT("CombatFeedback"));
	UWidget* Combo = Find<UWidget>(HUD, TEXT("ComboCounter"));
	UWidget* Prompt = Find<UWidget>(HUD, TEXT("PickupPrompt"));
	UWidget* Loadout = Find<UWidget>(HUD, TEXT("EquippedWeaponWidget"));

	TestNotNull("GaugePlate manifest name exists", GaugePlate);
	TestNotNull("TallyRailHead hosts the existing kill counter", RailHead);
	ExpectCanvasBox(*this, GaugePlate, TEXT("GaugePlate"), FAnchors(0.f, 0.f),
		FVector2D(32.f, 32.f), FVector2D(456.f, 152.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, SlotA, TEXT("SpellCooldownA"), FAnchors(0.f, 0.f),
		FVector2D(32.f, 216.f), FVector2D(78.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, SlotB, TEXT("SpellCooldownB"), FAnchors(0.f, 0.f),
		FVector2D(120.f, 216.f), FVector2D(78.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, SlotC, TEXT("SpellCooldownC"), FAnchors(0.f, 0.f),
		FVector2D(208.f, 216.f), FVector2D(78.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, SlotD, TEXT("SpellCooldownD"), FAnchors(0.f, 0.f),
		FVector2D(296.f, 216.f), FVector2D(78.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, Status, TEXT("StatusEffectBar"), FAnchors(0.f, 0.f),
		FVector2D(32.f, 318.f), FVector2D(456.f, 112.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, RailHead, TEXT("TallyRailHead"), FAnchors(1.f, 0.f),
		FVector2D(-32.f, 32.f), FVector2D(320.f, 56.f), FVector2D(1.f, 0.f));
	ExpectCanvasBox(*this, Callout, TEXT("CombatFeedback"), FAnchors(0.5f, 0.5f),
		FVector2D(0.f, 128.f), FVector2D(520.f, 52.f), FVector2D(0.5f, 0.f));
	ExpectCanvasBox(*this, Combo, TEXT("ComboCounter"), FAnchors(0.5f, 0.5f),
		FVector2D(0.f, 184.f), FVector2D(220.f, 36.f), FVector2D(0.5f, 0.f));
	ExpectCanvasBox(*this, Prompt, TEXT("PickupPrompt unit"), FAnchors(0.5f, 1.f),
		FVector2D(90.f, -36.f), FVector2D(720.f, 60.f), FVector2D(0.5f, 1.f));
	ExpectCanvasBox(*this, Loadout, TEXT("EquippedWeaponWidget"), FAnchors(1.f, 1.f),
		FVector2D(-32.f, -32.f), FVector2D(340.f, 84.f), FVector2D(1.f, 1.f));

	TestTrue("GaugePlate owns HealthBar", Find<UWidget>(HUD, TEXT("HealthBar"))
		&& Find<UWidget>(HUD, TEXT("HealthBar"))->GetParent() != HUD->WidgetTree->RootWidget);
	TestTrue("GaugePlate owns StaminaBar", Find<UWidget>(HUD, TEXT("StaminaBar"))
		&& Find<UWidget>(HUD, TEXT("StaminaBar"))->GetParent() != HUD->WidgetTree->RootWidget);
	TestTrue("GaugePlate owns PostureBar", Find<UWidget>(HUD, TEXT("PostureBar"))
		&& Find<UWidget>(HUD, TEXT("PostureBar"))->GetParent() != HUD->WidgetTree->RootWidget);
	TestTrue("GaugePlate owns SpellPointsBar", Find<UWidget>(HUD, TEXT("SpellPointsBar"))
		&& Find<UWidget>(HUD, TEXT("SpellPointsBar"))->GetParent() != HUD->WidgetTree->RootWidget);
	TestTrue("TallyRailHead owns KillCounter", Find<UWidget>(HUD, TEXT("KillCounter"))
		&& Find<UWidget>(HUD, TEXT("KillCounter"))->GetParent() != HUD->WidgetTree->RootWidget);

	float TopCenterWidth = 0.f;
	TestTrue("HUD exposes the 32/96 margin contract", InvokeMargin(HUD, 96.f));
	TestTrue("HUD exposes the top-centre safe width", ReadTopCenterSafeWidth(HUD, TopCenterWidth));
	TestEqual("top-centre width is 784 at the 96px margin", TopCenterWidth, 784.f);
	ExpectCanvasBox(*this, GaugePlate, TEXT("GaugePlate at 96"), FAnchors(0.f, 0.f),
		FVector2D(96.f, 96.f), FVector2D(456.f, 152.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, SlotA, TEXT("SpellCooldownA at 96"), FAnchors(0.f, 0.f),
		FVector2D(96.f, 280.f), FVector2D(78.f), FVector2D::ZeroVector);
	ExpectCanvasBox(*this, RailHead, TEXT("TallyRailHead at 96"), FAnchors(1.f, 0.f),
		FVector2D(-96.f, 96.f), FVector2D(320.f, 56.f), FVector2D(1.f, 0.f));
	ExpectCanvasBox(*this, Prompt, TEXT("PickupPrompt at 96"), FAnchors(0.5f, 1.f),
		FVector2D(90.f, -100.f), FVector2D(720.f, 60.f), FVector2D(0.5f, 1.f));
	ExpectCanvasBox(*this, Loadout, TEXT("EquippedWeaponWidget at 96"), FAnchors(1.f, 1.f),
		FVector2D(-96.f, -96.f), FVector2D(340.f, 84.f), FVector2D(1.f, 1.f));
	ExpectCanvasBox(*this, Callout, TEXT("CombatFeedback stays centred"), FAnchors(0.5f, 0.5f),
		FVector2D(0.f, 128.f), FVector2D(520.f, 52.f), FVector2D(0.5f, 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Gauge_HealthStaminaPostureSpellPoints,
	"Mordecai.UI.Gauge.HealthStaminaPostureSpellPoints",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Gauge_HealthStaminaPostureSpellPoints::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyHUDTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	if (!TestNotNull("player ASC", ASC))
	{
		return true;
	}

	UMordecaiHealthBarWidget* Health = Env.MakeWidget<UMordecaiHealthBarWidget>();
	UMordecaiStaminaBarWidget* Stamina = Env.MakeWidget<UMordecaiStaminaBarWidget>();
	UMordecaiPostureBarWidget* Posture = Env.MakeWidget<UMordecaiPostureBarWidget>();
	UMordecaiSpellPointsBarWidget* SpellPoints = Env.MakeWidget<UMordecaiSpellPointsBarWidget>();
	Health->BindToASC(ASC);
	Stamina->BindToASC(ASC);
	Posture->BindToASC(ASC);
	SpellPoints->BindToASC(ASC);

	UProgressBar* HealthFill = Find<UProgressBar>(Health, TEXT("HealthProgressBar"));
	UProgressBar* HealthTrail = Find<UProgressBar>(Health, TEXT("HealthTrailBar"));
	UProgressBar* HealthLead = Find<UProgressBar>(Health, TEXT("HealthLeadBar"));
	UTextBlock* HealthPercent = Find<UTextBlock>(Health, TEXT("HealthPercentText"));
	TestNotNull("HealthProgressBar fallback", HealthFill);
	TestNotNull("HealthTrailBar fallback", HealthTrail);
	TestNotNull("HealthLeadBar fallback", HealthLead);
	TestNotNull("HealthPercentText fallback", HealthPercent);
	TestNotNull("25 percent health notch", Find<UWidget>(Health, TEXT("HealthNotch25")));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 22.6f);
	TestEqual("health percentage rounds a living value up", HealthPercent
		? HealthPercent->GetText().ToString() : FString(), FString(TEXT("23%")));

	UProgressBar* StaminaFill = Find<UProgressBar>(Stamina, TEXT("StaminaProgressBar"));
	UProgressBar* StaminaDebt = Find<UProgressBar>(Stamina, TEXT("StaminaDebtBar"));
	UTextBlock* StaminaTier = Find<UTextBlock>(Stamina, TEXT("StaminaTierText"));
	TestNotNull("StaminaProgressBar fallback", StaminaFill);
	TestNotNull("StaminaDebtBar fallback", StaminaDebt);
	TestNotNull("StaminaTierText fallback", StaminaTier);
	TestNotNull("33 percent stamina notch", Find<UWidget>(Stamina, TEXT("StaminaNotch33")));
	TestNotNull("66 percent stamina notch", Find<UWidget>(Stamina, TEXT("StaminaNotch66")));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxStaminaAttribute(), 134.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), -26.f);
	TestEqual("negative stamina has no positive fill", StaminaFill ? StaminaFill->GetPercent() : -1.f, 0.f);
	TestEqual("debt width is abs(Stamina)/MaxStamina", StaminaDebt ? StaminaDebt->GetPercent() : -1.f,
		26.f / 134.f, 0.0001f);
	TestEqual("exhausted multiplier copy", StaminaTier
		? StaminaTier->GetText().ToString() : FString(), FString(TEXT("×0.5")));

	UProgressBar* PostureFill = Find<UProgressBar>(Posture, TEXT("PostureProgressBar"));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxPostureAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), 100.f);
	TestTrue("posture at max is Hidden, not Collapsed",
		PostureFill && PostureFill->GetVisibility() == ESlateVisibility::Hidden);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), 70.f);
	TestTrue("posture below max is visible", IsShown(PostureFill));
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_PostureBroken);
	TestTrue("posture broken fixture is active", Posture->IsPostureBroken());
	TestEqual("broken fixture floods the posture lane", PostureFill ? PostureFill->GetPercent() : -1.f, 1.f);
	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_PostureBroken);

	UHorizontalBox* Tokens = Find<UHorizontalBox>(SpellPoints, TEXT("SpellPointTokens"));
	UProgressBar* SpellBar = Find<UProgressBar>(SpellPoints, TEXT("SpellPointsProgressBar"));
	UTextBlock* SpellText = Find<UTextBlock>(SpellPoints, TEXT("SpellPointsText"));
	TestNotNull("SpellPointTokens fallback", Tokens);
	TestNotNull("SpellPointsProgressBar retained for above-16 fallback", SpellBar);
	TestNotNull("SpellPointsText retained", SpellText);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxSpellPointsAttribute(), 13.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), 5.f);
	TestTrue("10-16 SP uses tokens", IsShown(Tokens));
	TestEqual("13 max SP materializes 13 token images", Tokens ? Tokens->GetChildrenCount() : 0, 13);
	TestTrue("10-16 SP hides the progress fallback",
		SpellBar && SpellBar->GetVisibility() == ESlateVisibility::Collapsed);
	TestEqual("SP keeps the shipped x / y text", SpellText
		? SpellText->GetText().ToString() : FString(), FString(TEXT("5 / 13")));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxSpellPointsAttribute(), 18.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), 14.f);
	TestTrue("above 16 collapses tokens", Tokens
		&& Tokens->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue("above 16 uses the manifest visual fallback", IsShown(SpellBar));
	TestEqual("above-16 fallback keeps the resource ratio", SpellBar ? SpellBar->GetPercent() : -1.f,
		14.f / 18.f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_HUD_LowHealthFrameThreshold,
	"Mordecai.UI.HUD.LowHealthFrameThreshold",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_HUD_LowHealthFrameThreshold::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyHUDTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}
	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	HUD->BindToASC(ASC);

	UWidget* Frame = Find<UWidget>(HUD, TEXT("LowHealthFrame"));
	TestNotNull("LowHealthFrame manifest name exists", Frame);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 25.1f);
	TestFalse("frame is hidden above 25 percent", IsShown(Frame));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 25.f);
	TestTrue("frame shows at exactly 25 percent", IsShown(Frame));
	TestEqual("visible frame is HitTestInvisible", Frame
		? Frame->GetVisibility() : ESlateVisibility::Visible, ESlateVisibility::HitTestInvisible);
	TestEqual("frame opacity stays fixed instead of pulsing", Frame ? Frame->GetRenderOpacity() : 0.f, 1.f);
	TestNotNull("8px top vital band", Find<UWidget>(HUD, TEXT("LowHealthTopVital")));
	TestNotNull("3px top inner ink line", Find<UWidget>(HUD, TEXT("LowHealthTopInk")));
	TestNotNull("8px bottom vital band", Find<UWidget>(HUD, TEXT("LowHealthBottomVital")));
	TestNotNull("3px bottom inner ink line", Find<UWidget>(HUD, TEXT("LowHealthBottomInk")));
	TestNotNull("8px left vital band", Find<UWidget>(HUD, TEXT("LowHealthLeftVital")));
	TestNotNull("3px left inner ink line", Find<UWidget>(HUD, TEXT("LowHealthLeftInk")));
	TestNotNull("8px right vital band", Find<UWidget>(HUD, TEXT("LowHealthRightVital")));
	TestNotNull("3px right inner ink line", Find<UWidget>(HUD, TEXT("LowHealthRightInk")));
	return true;
}

#endif // WITH_AUTOMATION_TESTS
