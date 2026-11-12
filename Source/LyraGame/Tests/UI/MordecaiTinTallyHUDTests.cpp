// Project Mordecai — Tin Tally final combat HUD contract (US-105).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiEquippedWeaponWidget.h"
#include "Mordecai/UI/MordecaiHealthBarWidget.h"
#include "Mordecai/UI/MordecaiPickupPromptWidget.h"
#include "Mordecai/UI/MordecaiPostureBarWidget.h"
#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "Mordecai/UI/MordecaiSpellPointsBarWidget.h"
#include "Mordecai/UI/MordecaiStaminaBarWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
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

	bool HasAncestorNamed(const UWidget* Widget, const FName AncestorName)
	{
		for (const UPanelWidget* Parent = Widget ? Widget->GetParent() : nullptr;
			Parent;
			Parent = Parent->GetParent())
		{
			if (Parent->GetFName() == AncestorName)
			{
				return true;
			}
		}
		return false;
	}

	bool InvokeFloat(UObject* Target, const FName FunctionName, const float Value)
	{
		UFunction* Function = Target ? Target->GetClass()->FindFunctionByName(FunctionName) : nullptr;
		if (!Function)
		{
			return false;
		}
		struct FParams
		{
			float Value = 0.f;
		} Params;
		Params.Value = Value;
		Target->ProcessEvent(Function, &Params);
		return true;
	}

	bool InvokeSpellPresentation(UObject* Target, UMordecaiSpellDataAsset* Spell)
	{
		UFunction* Function = Target
			? Target->GetClass()->FindFunctionByName(TEXT("ConfigureFromSpellData"))
			: nullptr;
		if (!Function)
		{
			return false;
		}
		struct FParams
		{
			UMordecaiSpellDataAsset* SpellData = nullptr;
		} Params;
		Params.SpellData = Spell;
		Target->ProcessEvent(Function, &Params);
		return true;
	}

	FLinearColor BrushTint(const FSlateBrush& Brush)
	{
		return Brush.TintColor.GetSpecifiedColor();
	}

	void ExpectCanvasOffsets(
		FAutomationTestBase& Test,
		const UWidget* Widget,
		const TCHAR* Label,
		const FMargin& Expected)
	{
		const UCanvasPanelSlot* Slot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr;
		if (Test.TestNotNull(*FString::Printf(TEXT("%s has a canvas slot"), Label), Slot))
		{
			Test.TestEqual(*FString::Printf(TEXT("%s offsets"), Label), Slot->GetOffsets(), Expected);
		}
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

	TestTrue("GaugePlate is an ancestor of HealthBar",
		HasAncestorNamed(Find<UWidget>(HUD, TEXT("HealthBar")), TEXT("GaugePlate")));
	TestTrue("GaugePlate is an ancestor of StaminaBar",
		HasAncestorNamed(Find<UWidget>(HUD, TEXT("StaminaBar")), TEXT("GaugePlate")));
	TestTrue("GaugePlate is an ancestor of PostureBar",
		HasAncestorNamed(Find<UWidget>(HUD, TEXT("PostureBar")), TEXT("GaugePlate")));
	TestTrue("GaugePlate is an ancestor of SpellPointsBar",
		HasAncestorNamed(Find<UWidget>(HUD, TEXT("SpellPointsBar")), TEXT("GaugePlate")));
	TestTrue("TallyRailHead owns KillCounter", Find<UWidget>(HUD, TEXT("KillCounter"))
		&& Find<UWidget>(HUD, TEXT("KillCounter"))->GetParent() != HUD->WidgetTree->RootWidget);

	float TopCenterWidth = 0.f;
	TestTrue("HUD exposes the top-centre safe width", ReadTopCenterSafeWidth(HUD, TopCenterWidth));
	TestEqual("normal margin leaves boss/banner width to its owning story", TopCenterWidth, 0.f);
	TestTrue("HUD exposes the 32/96 margin contract", InvokeMargin(HUD, 96.f));
	TestTrue("HUD exposes the top-centre safe width", ReadTopCenterSafeWidth(HUD, TopCenterWidth));
	const float ExpectedTVSafeWidth = 1920.f - 2.f * (96.f + 456.f + 16.f);
	TestEqual("TV-safe top-centre proposal clears the gauge by 16px",
		TopCenterWidth, ExpectedTVSafeWidth);
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
	if (!TestNotNull("health widget", Health)
		|| !TestNotNull("stamina widget", Stamina)
		|| !TestNotNull("posture widget", Posture)
		|| !TestNotNull("spell-points widget", SpellPoints))
	{
		return true;
	}
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
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 22.2f);
	TestEqual("health percentage rounds a living value up", HealthPercent
		? HealthPercent->GetText().ToString() : FString(), FString(TEXT("23%")));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetHealthAttribute(), 100.f);
	// SetNumericAttributeBase does not broadcast when restoring this recovery fixture's base/current aggregator.
	// Re-prime from the real ASC before exercising damage and healing through gameplay effects.
	Health->BindToASC(ASC);
	Env.ApplyPlayerHealthDelta(-40.f);
	TestEqual("damage snaps the live fill", HealthFill ? HealthFill->GetPercent() : -1.f, 0.6f, 0.001f);
	TestEqual("damage trail begins at the pre-hit value", HealthTrail ? HealthTrail->GetPercent() : -1.f, 1.f, 0.001f);
	TestTrue("health exposes deterministic motion advancement", InvokeFloat(Health, TEXT("AdvanceVisuals"), 0.35f));
	TestEqual("damage trail holds for exactly 350ms", HealthTrail ? HealthTrail->GetPercent() : -1.f, 1.f, 0.001f);
	InvokeFloat(Health, TEXT("AdvanceVisuals"), 0.225f);
	TestEqual("damage trail is halfway through its 450ms ease-in-out", HealthTrail ? HealthTrail->GetPercent() : -1.f, 0.8f, 0.01f);
	InvokeFloat(Health, TEXT("AdvanceVisuals"), 0.225f);
	TestEqual("damage trail reaches the fill after 450ms", HealthTrail ? HealthTrail->GetPercent() : -1.f, 0.6f, 0.001f);
	Env.ApplyPlayerHealthDelta(20.f);
	TestEqual("heal lead snaps to the healed value", HealthLead ? HealthLead->GetPercent() : -1.f, 0.8f, 0.001f);
	TestEqual("heal fill begins at its old value", HealthFill ? HealthFill->GetPercent() : -1.f, 0.6f, 0.001f);
	InvokeFloat(Health, TEXT("AdvanceVisuals"), 0.2f);
	TestEqual("heal fill uses a 400ms ease-out", HealthFill ? HealthFill->GetPercent() : -1.f, 0.75f, 0.01f);
	InvokeFloat(Health, TEXT("AdvanceVisuals"), 0.2f);
	TestEqual("heal fill reaches the lead after 400ms", HealthFill ? HealthFill->GetPercent() : -1.f, 0.8f, 0.001f);
	TestFalse("heal lead hides when fill and lead meet", IsShown(HealthLead));

	UProgressBar* StaminaFill = Find<UProgressBar>(Stamina, TEXT("StaminaProgressBar"));
	UProgressBar* StaminaDebt = Find<UProgressBar>(Stamina, TEXT("StaminaDebtBar"));
	UTextBlock* StaminaTier = Find<UTextBlock>(Stamina, TEXT("StaminaTierText"));
	TestNotNull("StaminaProgressBar fallback", StaminaFill);
	TestNotNull("StaminaDebtBar fallback", StaminaDebt);
	TestNotNull("StaminaTierText fallback", StaminaTier);
	TestNotNull("33 percent stamina notch", Find<UWidget>(Stamina, TEXT("StaminaNotch33")));
	TestNotNull("66 percent stamina notch", Find<UWidget>(Stamina, TEXT("StaminaNotch66")));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxStaminaAttribute(), 134.f);
	UBorder* TierFill = Find<UBorder>(Stamina, TEXT("StaminaTierPill_Fill"));
	UBorder* TierOutline = Find<UBorder>(Stamina, TEXT("StaminaTierPill_Outline"));
	const FMordecaiUIThemeColors Diorama = FMordecaiUIThemeColors::MakeDiorama();
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), 100.f);
	TestEqual("Green copy", StaminaTier ? StaminaTier->GetText().ToString() : FString(), FString(TEXT("×1.25")));
	TestTrue("Green pill uses tier ground", TierFill && BrushTint(TierFill->Background).Equals(Diorama.StaminaGreen));
	TestTrue("Green pill uses ink text", StaminaTier
		&& StaminaTier->GetColorAndOpacity().GetSpecifiedColor().Equals(Diorama.Ink));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), 50.f);
	TestEqual("Yellow copy", StaminaTier ? StaminaTier->GetText().ToString() : FString(), FString(TEXT("×1.0")));
	TestTrue("Yellow pill uses tier ground", TierFill && BrushTint(TierFill->Background).Equals(Diorama.StaminaYellow));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), 20.f);
	TestEqual("Red copy", StaminaTier ? StaminaTier->GetText().ToString() : FString(), FString(TEXT("×0.75")));
	TestTrue("Red fill uses the stripe material", StaminaFill
		&& StaminaFill->GetWidgetStyle().FillImage.GetResourceObject() != nullptr);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), -26.f);
	TestEqual("negative stamina has no positive fill", StaminaFill ? StaminaFill->GetPercent() : -1.f, 0.f);
	TestEqual("debt width is abs(Stamina)/MaxStamina", StaminaDebt ? StaminaDebt->GetPercent() : -1.f,
		26.f / 134.f, 0.0001f);
	TestEqual("exhausted multiplier copy", StaminaTier
		? StaminaTier->GetText().ToString() : FString(), FString(TEXT("×0.5")));
	TestTrue("Exhausted pill inverts to ink ground",
		TierFill && BrushTint(TierFill->Background).Equals(Diorama.Ink));
	TestTrue("Exhausted pill has danger border",
		TierOutline && BrushTint(TierOutline->Background).Equals(Diorama.Danger));
	TestTrue("Exhausted copy uses danger text", StaminaTier
		&& StaminaTier->GetColorAndOpacity().GetSpecifiedColor().Equals(Diorama.DangerText));

	UProgressBar* PostureFill = Find<UProgressBar>(Posture, TEXT("PostureProgressBar"));
	UWidget* PostureWell = Find<UWidget>(Posture, TEXT("PostureWell"));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxPostureAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), 70.f);
	TestTrue("posture row below max is visible", IsShown(Posture));
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), 100.f);
	TestTrue("posture row remains visible during the refill delay", IsShown(Posture));
	TestTrue("posture well remains visible during the refill delay", IsShown(PostureWell));
	TestTrue("posture exposes deterministic delay advancement", InvokeFloat(Posture, TEXT("AdvanceVisuals"), 2.9f));
	TestTrue("posture row is still visible before three seconds", IsShown(Posture));
	InvokeFloat(Posture, TEXT("AdvanceVisuals"), 0.1f);
	TestEqual("the whole posture row becomes Hidden after three seconds",
		Posture->GetVisibility(), ESlateVisibility::Hidden);
	TestEqual("PostureWell follows the row Hidden state",
		PostureWell ? PostureWell->GetVisibility() : ESlateVisibility::Visible, ESlateVisibility::Hidden);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetPostureAttribute(), 70.f);
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_PostureBroken);
	TestTrue("posture broken fixture is active", Posture->IsPostureBroken());
	TestEqual("broken fixture floods the posture lane", PostureFill ? PostureFill->GetPercent() : -1.f, 1.f);
	UBorder* PostureWellLayer = Find<UBorder>(Posture, TEXT("PostureWell_Well"));
	TestTrue("posture broken switches the well lip to rim",
		PostureWellLayer
		&& PostureWellLayer->Background.OutlineSettings.Color.GetSpecifiedColor().Equals(Diorama.Rim));
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
	UWidget* FirstToken = Tokens && Tokens->GetChildrenCount() > 0 ? Tokens->GetChildAt(0) : nullptr;
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), 4.f);
	TestTrue("SP current-value changes retint existing tokens instead of rebuilding",
		Tokens && Tokens->GetChildrenCount() > 0 && Tokens->GetChildAt(0) == FirstToken);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetMaxSpellPointsAttribute(), 18.f);
	ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetSpellPointsAttribute(), 14.f);
	TestTrue("above 16 collapses tokens", Tokens
		&& Tokens->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue("above 16 uses the manifest visual fallback", IsShown(SpellBar));
	TestEqual("above-16 fallback keeps the resource ratio", SpellBar ? SpellBar->GetPercent() : -1.f,
		14.f / 18.f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_HUD_LiveThemeAndSpellPresentation,
	"Mordecai.UI.HUD.LiveThemeAndSpellPresentation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_HUD_LiveThemeAndSpellPresentation::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyHUDTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}
	Env.LocalPlayer->PlayerAdded(nullptr, FPlatformMisc::GetPlatformUserForUserIndex(0));
	ON_SCOPE_EXIT { Env.LocalPlayer->PlayerRemoved(); };
	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	if (!TestNotNull("default combat HUD", HUD))
	{
		return true;
	}
	TSharedRef<SWidget> HUDSlate = HUD->TakeWidget();
	UMordecaiUIThemeSubsystem* Themes =
		Env.LocalPlayer ? Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>() : nullptr;
	UBorder* PlateFill = Find<UBorder>(HUD, TEXT("GaugePlate_Fill"));
	UBorder* PlateRimShade = Find<UBorder>(HUD, TEXT("GaugePlate_RimShade"));
	if (!TestNotNull("local theme subsystem", Themes)
		|| !TestNotNull("GaugePlate fill layer", PlateFill)
		|| !TestNotNull("GaugePlate rim-shade layer", PlateRimShade))
	{
		return true;
	}
	const UOverlaySlot* DioramaInset = Cast<UOverlaySlot>(PlateRimShade->Slot);
	TestTrue("Diorama plate fill starts from the active theme",
		BrushTint(PlateFill->Background).Equals(FMordecaiUIThemeColors::MakeDiorama().Plate));
	TestEqual("Diorama outline geometry is 3px",
		DioramaInset ? DioramaInset->GetPadding().Left : 0.f, 3.f);
	Themes->SetActiveTheme(EMordecaiUITheme::HighContrast);
	const UOverlaySlot* HighContrastInset = Cast<UOverlaySlot>(PlateRimShade->Slot);
	TestTrue("live GaugePlate switches to High Contrast black",
		BrushTint(PlateFill->Background).Equals(FLinearColor::Black));
	TestEqual("live GaugePlate switches to the 4px High Contrast outline",
		HighContrastInset ? HighContrastInset->GetPadding().Left : 0.f, 4.f);
	TestTrue("CombatHUD subscribes exactly once to theme changes",
		Themes->OnThemeChangedNative().IsBoundToObject(HUD));
	UMordecaiHealthBarWidget* Health = Env.MakeWidget<UMordecaiHealthBarWidget>();
	UMordecaiStaminaBarWidget* Stamina = Env.MakeWidget<UMordecaiStaminaBarWidget>();
	UMordecaiPostureBarWidget* Posture = Env.MakeWidget<UMordecaiPostureBarWidget>();
	UMordecaiSpellPointsBarWidget* SpellPoints = Env.MakeWidget<UMordecaiSpellPointsBarWidget>();
	UMordecaiPickupPromptWidget* Prompt = Env.MakeWidget<UMordecaiPickupPromptWidget>();
	UMordecaiEquippedWeaponWidget* Loadout = Env.MakeWidget<UMordecaiEquippedWeaponWidget>();
	TArray<TSharedPtr<SWidget>> LiveSlateWidgets = {
		Health->TakeWidget(), Stamina->TakeWidget(), Posture->TakeWidget(),
		SpellPoints->TakeWidget(), Prompt->TakeWidget(), Loadout->TakeWidget()
	};
	for (UUserWidget* LiveWidget : {
		static_cast<UUserWidget*>(Health), static_cast<UUserWidget*>(Stamina),
		static_cast<UUserWidget*>(Posture), static_cast<UUserWidget*>(SpellPoints),
		static_cast<UUserWidget*>(Prompt), static_cast<UUserWidget*>(Loadout) })
	{
		TestTrue(*FString::Printf(TEXT("%s subscribes once to the owning player's theme"),
			*LiveWidget->GetClass()->GetName()),
			Themes->OnThemeChangedNative().IsBoundToObject(LiveWidget));
	}

	UMordecaiSpellCooldownWidget* Slot = Env.MakeWidget<UMordecaiSpellCooldownWidget>();
	if (!TestNotNull("spell slot", Slot))
	{
		return true;
	}
	UMordecaiSpellDataAsset* Fireball = NewObject<UMordecaiSpellDataAsset>();
	Fireball->SpellId = TEXT("Fireball");
	Fireball->SpellPointCost = 3;
	TestTrue("spell slot accepts shipped spell presentation data",
		InvokeSpellPresentation(Slot, Fireball));
	UImage* Glyph = Find<UImage>(Slot, TEXT("SpellIcon"));
	UHorizontalBox* Rivets = Find<UHorizontalBox>(Slot, TEXT("CostRivets"));
	UWidget* Lip = Find<UWidget>(Slot, TEXT("CooldownShutterLip"));
	TestNotNull("spell glyph image exists", Glyph);
	TestEqual("Fireball uses three SP cost rivets", Rivets ? Rivets->GetChildrenCount() : 0, 3);
	TestTrue("Fireball uses its shipped glyph texture", Glyph
		&& Glyph->GetBrush().GetResourceObject()
		&& Glyph->GetBrush().GetResourceObject()->GetName() == TEXT("T_UI_Glyph_fireball"));
	const UCanvasPanelSlot* LipSlot = Lip ? Cast<UCanvasPanelSlot>(Lip->Slot) : nullptr;
	TestEqual("cooldown shutter rim lip is 3px", static_cast<float>(LipSlot ? LipSlot->GetSize().Y : 0.0), 3.f);
	TestTrue("gallery can set a deterministic mid-cooldown state",
		InvokeFloat(Slot, TEXT("SetPreviewCooldownFraction"), 0.5f));
	TestEqual("glyph dims to 0.4 while cooling",
		Glyph ? Glyph->GetColorAndOpacity().A : 0.f, 0.4f, 0.001f);
	TestEqual("keycap hangs at in-slot y=56",
		Find<UWidget>(Slot, TEXT("SlotKey")) && Cast<UCanvasPanelSlot>(Find<UWidget>(Slot, TEXT("SlotKey"))->Slot)
			? static_cast<float>(Cast<UCanvasPanelSlot>(Find<UWidget>(Slot, TEXT("SlotKey"))->Slot)->GetPosition().Y) : -1.f,
		56.f);
	TestEqual("key label sits at in-slot y=60",
		Find<UWidget>(Slot, TEXT("SlotKeyText")) && Cast<UCanvasPanelSlot>(Find<UWidget>(Slot, TEXT("SlotKeyText"))->Slot)
			? static_cast<float>(Cast<UCanvasPanelSlot>(Find<UWidget>(Slot, TEXT("SlotKeyText"))->Slot)->GetPosition().Y) : -1.f,
		60.f);
	TestTrue("keycap label uses F_Tally_Mono",
		Find<UTextBlock>(Slot, TEXT("SlotKeyText"))
		&& Find<UTextBlock>(Slot, TEXT("SlotKeyText"))->GetFont().FontObject
		&& Find<UTextBlock>(Slot, TEXT("SlotKeyText"))->GetFont().FontObject->GetName() == TEXT("F_Tally_Mono"));
	TestEqual("keycap label uses weight 800",
		Find<UTextBlock>(Slot, TEXT("SlotKeyText"))
			? Find<UTextBlock>(Slot, TEXT("SlotKeyText"))->GetFont().TypefaceFontName : NAME_None,
		FName(TEXT("ExtraBold")));
	LiveSlateWidgets.Add(Slot->TakeWidget());
	TestTrue("spell slot subscribes once to the owning player's theme",
		Themes->OnThemeChangedNative().IsBoundToObject(Slot));

	UCanvasPanelSlot* PromptTagSlot = Prompt && Find<UWidget>(Prompt, TEXT("PromptTag"))
		? Cast<UCanvasPanelSlot>(Find<UWidget>(Prompt, TEXT("PromptTag"))->Slot)
		: nullptr;
	TestNotNull("prompt tag has a canvas slot", PromptTagSlot);
	TestTrue("prompt tag is right anchored against the hint pill",
		PromptTagSlot
		&& PromptTagSlot->GetAnchors() == FAnchors(1.f, 0.5f)
		&& PromptTagSlot->GetAlignment() == FVector2D(1.f, 0.5f)
		&& PromptTagSlot->GetAutoSize());
	UTextBlock* PromptText = Find<UTextBlock>(Prompt, TEXT("PromptText"));
	UTextBlock* HealthText = Find<UTextBlock>(Health, TEXT("HealthPercentText"));
	UTextBlock* WeaponText = Find<UTextBlock>(Loadout, TEXT("WeaponNameText"));
	TestTrue("prompt uses F_Tally_UI", PromptText && PromptText->GetFont().FontObject
		&& PromptText->GetFont().FontObject->GetName() == TEXT("F_Tally_UI"));
	TestTrue("live gauge number uses F_Tally_Mono", HealthText && HealthText->GetFont().FontObject
		&& HealthText->GetFont().FontObject->GetName() == TEXT("F_Tally_Mono"));
	TestTrue("loadout name uses F_Tally_UI", WeaponText && WeaponText->GetFont().FontObject
		&& WeaponText->GetFont().FontObject->GetName() == TEXT("F_Tally_UI"));
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
	UWidget* TopVital = Find<UWidget>(HUD, TEXT("LowHealthTopVital"));
	UWidget* TopInk = Find<UWidget>(HUD, TEXT("LowHealthTopInk"));
	UWidget* BottomVital = Find<UWidget>(HUD, TEXT("LowHealthBottomVital"));
	UWidget* BottomInk = Find<UWidget>(HUD, TEXT("LowHealthBottomInk"));
	UWidget* LeftInk = Find<UWidget>(HUD, TEXT("LowHealthLeftInk"));
	UWidget* RightInk = Find<UWidget>(HUD, TEXT("LowHealthRightInk"));
	TestNotNull("8px top vital band", TopVital);
	TestNotNull("3px top inner ink line", TopInk);
	TestNotNull("8px bottom vital band", BottomVital);
	TestNotNull("3px bottom inner ink line", BottomInk);
	ExpectCanvasOffsets(*this, TopVital, TEXT("top vital band"), FMargin(0.f, 0.f, 0.f, 8.f));
	ExpectCanvasOffsets(*this, TopInk, TEXT("top ink line"), FMargin(0.f, 8.f, 0.f, 3.f));
	ExpectCanvasOffsets(*this, BottomVital, TEXT("bottom vital band"), FMargin(0.f, -8.f, 0.f, 8.f));
	ExpectCanvasOffsets(*this, BottomInk, TEXT("bottom ink line"), FMargin(0.f, -11.f, 0.f, 3.f));
	ExpectCanvasOffsets(*this, LeftInk, TEXT("left ink line inset at corners"), FMargin(8.f, 8.f, 3.f, -8.f));
	ExpectCanvasOffsets(*this, RightInk, TEXT("right ink line inset at corners"), FMargin(-11.f, 8.f, 3.f, -8.f));
	UBorder* Rim = Find<UBorder>(HUD, TEXT("GaugePlate_Rim"));
	const float FrameOpacityBefore = Frame ? Frame->GetRenderOpacity() : -1.f;
	TestTrue("HUD exposes deterministic low-health rim stepping",
		InvokeFloat(HUD, TEXT("UpdateLowHealthRimForTime"), 0.25f));
	TestTrue("vital step changes the rim brush tint without multiplying it",
		Rim
		&& BrushTint(Rim->Background).Equals(FMordecaiUIThemeColors::MakeDiorama().Vital)
		&& Rim->GetBrushColor().Equals(FLinearColor::White));
	TestEqual("low-health frame does not pulse when the rim steps",
		Frame ? Frame->GetRenderOpacity() : -1.f, FrameOpacityBefore);
	InvokeFloat(HUD, TEXT("UpdateLowHealthRimForTime"), 0.75f);
	TestTrue("rest step restores the theme rim tint",
		Rim
		&& BrushTint(Rim->Background).Equals(FMordecaiUIThemeColors::MakeDiorama().Rim)
		&& Rim->GetBrushColor().Equals(FLinearColor::White));
	return true;
}

#endif // WITH_AUTOMATION_TESTS
