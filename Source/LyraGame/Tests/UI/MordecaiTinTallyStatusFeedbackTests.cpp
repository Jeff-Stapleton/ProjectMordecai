// Project Mordecai — Tin Tally status chips and centre-lane feedback (US-106).

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/App.h"
#include "Misc/ScopeExit.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Mordecai/AbilitySystem/MordecaiAttributeSet.h"
#include "Mordecai/Combat/MordecaiGA_MeleeAttack.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/StatusEffects/MordecaiStatusEffectTypes.h"
#include "Mordecai/UI/MordecaiCombatFeedbackWidget.h"
#include "Mordecai/UI/MordecaiCombatHUDWidget.h"
#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectBarWidget.h"
#include "Mordecai/UI/MordecaiStatusEffectIndicatorWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
#include "Input/HittestGrid.h"
#include "Rendering/DrawElements.h"
#include "Tests/Recovery/MordecaiRecoveryTestEnv.h"
#include "TimerManager.h"
#include "Types/PaintArgs.h"
#include "UObject/UnrealType.h"
#include "Widgets/SVirtualWindow.h"

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

	bool IsShown(const UWidget* Widget)
	{
		return Widget
			&& Widget->GetVisibility() != ESlateVisibility::Collapsed
			&& Widget->GetVisibility() != ESlateVisibility::Hidden;
	}

	int32 CountShownChildren(const UPanelWidget* Panel)
	{
		int32 Count = 0;
		for (int32 Index = 0; Panel && Index < Panel->GetChildrenCount(); ++Index)
		{
			Count += IsShown(Panel->GetChildAt(Index)) ? 1 : 0;
		}
		return Count;
	}

	UMordecaiStatusEffectIndicatorWidget* FirstIndicator(const UPanelWidget* Panel)
	{
		for (int32 Index = 0; Panel && Index < Panel->GetChildrenCount(); ++Index)
		{
			if (UMordecaiStatusEffectIndicatorWidget* Indicator =
				Cast<UMordecaiStatusEffectIndicatorWidget>(Panel->GetChildAt(Index)))
			{
				return Indicator;
			}
		}
		return nullptr;
	}

	void PaintForCachedGeometry(UUserWidget* Widget, const FVector2D& DrawSize)
	{
		Widget->TakeWidget();
		TSharedRef<SWidget> SlateWidget =
			Widget->WidgetTree->RootWidget->TakeWidget();
		TSharedRef<SVirtualWindow> Window = SNew(SVirtualWindow).Size(DrawSize);
		Window->SetContent(SlateWidget);
		Window->Resize(DrawSize);
		Widget->ForceLayoutPrepass();
		SlateWidget->SlatePrepass(1.f);

		const FGeometry RootGeometry = FGeometry::MakeRoot(
			DrawSize, FSlateLayoutTransform(1.f));
		const FSlateRect CullingRect = RootGeometry.GetLayoutBoundingRect();
		FHittestGrid HitTestGrid;
		HitTestGrid.SetHittestArea(CullingRect.GetTopLeft(), CullingRect.GetSize());
		FSlateWindowElementList DrawElements(Window);
		const FPaintArgs PaintArgs(
			nullptr, HitTestGrid, FVector2D::ZeroVector,
			FApp::GetCurrentTime(), 0.f);
		SlateWidget->Paint(
			PaintArgs, RootGeometry, CullingRect, DrawElements, 0,
			FWidgetStyle(), true);
	}

	FSlateRect CachedRect(const UWidget* Widget)
	{
		const FGeometry Geometry = Widget->GetCachedGeometry();
		const FVector2D Position = Geometry.GetAbsolutePosition();
		const FVector2D Size = Geometry.GetLocalSize();
		return FSlateRect(
			Position.X, Position.Y, Position.X + Size.X, Position.Y + Size.Y);
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
	UMordecaiStatusEffectBarWidget* Bar =
		Env.MakeWidget<UMordecaiStatusEffectBarWidget>();
	Bar->BindToASC(ASC);
	UPanelWidget* Container = Find<UPanelWidget>(Bar, TEXT("IndicatorContainer"));
	UWrapBox* Wrap = Cast<UWrapBox>(Container);
	TestNotNull("IndicatorContainer is panel-compatible at runtime", Container);
	TestNotNull("fallback IndicatorContainer is a WrapBox", Wrap);
	TestEqual("WrapBox uses the 456px strip width",
		Wrap ? Wrap->GetWrapSize() : 0.f, 456.f);
	TestNotNull("status strip reserves 12px above and 8px below the WrapBox",
		Find<UBorder>(Bar, TEXT("StatusWrapPadding")));

	// Bind first: live AddLooseGameplayTag callbacks must incrementally add chips.
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::Status_Burning);
	UMordecaiStatusEffectIndicatorWidget* FirstLive = FirstIndicator(Container);
	UPanelSlot* FirstLiveSlot = FirstLive ? FirstLive->Slot : nullptr;
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::Status_Rooted);
	TestNotNull("a live tag added after BindToASC creates its chip", FirstLive);
	TestTrue("adding another live tag preserves an existing chip slot",
		FirstLive && FirstLive->Slot == FirstLiveSlot);
	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::Status_Burning);
	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::Status_Rooted);

	// Eight narrow compact chips fit in two rows. The ninth is capped silently:
	// +N belongs only to the case where compact geometry would need a third row.
	const FGameplayTag NarrowStatuses[] = {
		MordecaiGameplayTags::Status_Burning,
		MordecaiGameplayTags::Status_Bleeding,
		MordecaiGameplayTags::Status_Poisoned,
		MordecaiGameplayTags::Status_Rooted,
		MordecaiGameplayTags::Status_Silenced,
		MordecaiGameplayTags::Status_Blinded,
		MordecaiGameplayTags::Status_Fear,
		MordecaiGameplayTags::Status_Weakened,
		MordecaiGameplayTags::Status_Focused,
	};
	for (const FGameplayTag& Status : NarrowStatuses)
	{
		ASC->AddLooseGameplayTag(Status);
	}
	UTextBlock* Overflow = Find<UTextBlock>(Bar, TEXT("OverflowChip"));
	UWidget* OverflowSize = Find<UWidget>(Bar, TEXT("OverflowSize"));
	TestNotNull("OverflowChip native fallback exists", Overflow);
	TestNotNull("permanent overflow child exists", OverflowSize);
	TestTrue("visible children never exceed eight",
		CountShownChildren(Container)
			<= UMordecaiStatusEffectBarWidget::MaxVisibleIndicators);
	TestEqual("nine narrow statuses still show eight compact chips",
		CountShownChildren(Container),
		UMordecaiStatusEffectBarWidget::MaxVisibleIndicators);
	TestEqual("+N stays collapsed when eight compact chips fit two rows",
		OverflowSize ? OverflowSize->GetVisibility() : ESlateVisibility::Visible,
		ESlateVisibility::Collapsed);
	TestTrue("overflow remains the permanent last child while collapsed",
		Container && Container->GetChildrenCount() > 0
		&& Container->GetChildAt(Container->GetChildrenCount() - 1) == OverflowSize);
	UMordecaiStatusEffectIndicatorWidget* First = FirstIndicator(Container);
	TestNotNull("first visible child is a status indicator", First);
	TestTrue("display sort puts a Control chip first",
		First && (
			First->GetStatusTag() == MordecaiGameplayTags::Status_Rooted
			|| First->GetStatusTag() == MordecaiGameplayTags::Status_Silenced
			|| First->GetStatusTag() == MordecaiGameplayTags::Status_Blinded
			|| First->GetStatusTag() == MordecaiGameplayTags::Status_Fear));
	TestEqual("compact mode collapses every status name",
		First ? Find<UTextBlock>(First, TEXT("StatusNameText"))->GetVisibility()
			: ESlateVisibility::Visible,
		ESlateVisibility::Collapsed);
	Bar->ForceLayoutPrepass();
	TestTrue("two-row status strip desired height stays within 112px",
		Bar->GetDesiredSize().Y <= 112.f);

	for (const FGameplayTag& Status : NarrowStatuses)
	{
		ASC->RemoveLooseGameplayTag(Status);
	}
	const FGameplayTag WideStatuses[] = {
		MordecaiGameplayTags::Status_Frostbitten,
		MordecaiGameplayTags::Status_Shocked,
		MordecaiGameplayTags::Status_Rooted,
		MordecaiGameplayTags::Status_Silenced,
		MordecaiGameplayTags::Status_Cursed,
		MordecaiGameplayTags::Status_Corroded,
		MordecaiGameplayTags::Status_Drenched,
		MordecaiGameplayTags::Status_StoneSkin,
	};
	for (const FGameplayTag& Status : WideStatuses)
	{
		ASC->AddLooseGameplayTag(Status);
	}
	TestEqual("+N is HitTestInvisible when compact chips would need a third row",
		OverflowSize ? OverflowSize->GetVisibility() : ESlateVisibility::Collapsed,
		ESlateVisibility::HitTestInvisible);
	TestTrue("overflow text is a real +N value",
		Overflow && Overflow->GetText().ToString().StartsWith(TEXT("+"))
		&& Overflow->GetText().ToString() != TEXT("+0"));
	TestTrue("overflow remains the permanent last child while visible",
		Container && Container->GetChildrenCount() > 0
		&& Container->GetChildAt(Container->GetChildrenCount() - 1) == OverflowSize);

	const FObjectPropertyBase* ContainerProperty =
		FindFProperty<FObjectPropertyBase>(
			UMordecaiStatusEffectBarWidget::StaticClass(), TEXT("IndicatorContainer"));
	TestTrue("IndicatorContainer UPROPERTY accepts any panel implementation",
		ContainerProperty
		&& ContainerProperty->PropertyClass == UPanelWidget::StaticClass());

	// A gameplay bind must discard gallery-only order rather than leaking preview chips.
	for (const FGameplayTag& Status : WideStatuses)
	{
		ASC->RemoveLooseGameplayTag(Status);
	}
	UMordecaiStatusEffectBarWidget* Rebound =
		Env.MakeWidget<UMordecaiStatusEffectBarWidget>();
	Rebound->SetGalleryPreviewStatuses({
		MordecaiGameplayTags::Status_Focused,
		MordecaiGameplayTags::Status_Burning });
	Rebound->BindToASC(ASC);
	TestEqual("BindToASC prunes gallery/stale ActiveTagOrder on rebind",
		Rebound->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_StatusChip_DistinctNamesForConcurrentStatuses,
	"Mordecai.UI.StatusChip.DistinctNamesForConcurrentStatuses",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_StatusChip_DistinctNamesForConcurrentStatuses::RunTest(
	const FString& Parameters)
{
	using namespace MordecaiTinTallyStatusFeedbackTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	UMordecaiStatusEffectBarWidget* Bar =
		Env.MakeWidget<UMordecaiStatusEffectBarWidget>();
	Bar->BindToASC(ASC);
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::Status_StoneSkin);
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::Status_Burning);

	UPanelWidget* Container = Find<UPanelWidget>(Bar, TEXT("IndicatorContainer"));
	TMap<FGameplayTag, FString> NamesByTag;
	for (int32 Index = 0; Container && Index < Container->GetChildrenCount(); ++Index)
	{
		if (const UMordecaiStatusEffectIndicatorWidget* Indicator =
			Cast<UMordecaiStatusEffectIndicatorWidget>(Container->GetChildAt(Index)))
		{
			const UTextBlock* Name =
				Find<UTextBlock>(Indicator, TEXT("StatusNameText"));
			NamesByTag.Add(
				Indicator->GetStatusTag(),
				Name ? Name->GetText().ToString() : FString());
		}
	}

	TestEqual("two active statuses create two indicators", NamesByTag.Num(), 2);
	TestEqual("Stone Skin indicator uses its player-facing name",
		NamesByTag.FindRef(MordecaiGameplayTags::Status_StoneSkin),
		FString(TEXT("Stone Skin")));
	TestEqual("Burning indicator uses its distinct player-facing name",
		NamesByTag.FindRef(MordecaiGameplayTags::Status_Burning),
		FString(TEXT("Burning")));

	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::Status_StoneSkin);
	TestEqual("ending Stone Skin removes only its indicator",
		CountShownChildren(Container), 1);
	const UMordecaiStatusEffectIndicatorWidget* Remaining =
		FirstIndicator(Container);
	TestTrue("Burning remains after Stone Skin ends",
		Remaining
		&& Remaining->GetStatusTag() == MordecaiGameplayTags::Status_Burning);
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
	UMordecaiStatusEffectIndicatorWidget* Frost =
		Env.MakeWidget<UMordecaiStatusEffectIndicatorWidget>();
	UMordecaiStatusEffectIndicatorWidget* Shocked =
		Env.MakeWidget<UMordecaiStatusEffectIndicatorWidget>();
	UMordecaiStatusEffectIndicatorWidget* Focused =
		Env.MakeWidget<UMordecaiStatusEffectIndicatorWidget>();
	UMordecaiStatusEffectIndicatorWidget* Exposed =
		Env.MakeWidget<UMordecaiStatusEffectIndicatorWidget>();
	UMordecaiStatusEffectIndicatorWidget* Fear =
		Env.MakeWidget<UMordecaiStatusEffectIndicatorWidget>();
	Buff->InitializeIndicator(MordecaiGameplayTags::Status_StoneSkin, true, 10.f);
	Debuff->InitializeIndicator(MordecaiGameplayTags::Status_Burning, false, 5.f);
	Frost->InitializeIndicator(MordecaiGameplayTags::Status_Frostbitten, false, 6.f);
	Shocked->InitializeIndicator(MordecaiGameplayTags::Status_Shocked, false, 5.f);
	Shocked->SetCompact(true);
	Focused->InitializeIndicator(MordecaiGameplayTags::Status_Focused, true, 0.f);
	Exposed->InitializeIndicator(MordecaiGameplayTags::Status_Exposed, false, 5.f);
	Fear->InitializeIndicator(MordecaiGameplayTags::Status_Fear, false, 5.f);

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
	TestEqual("buff tab is visible",
		Find<UWidget>(Buff, TEXT("BuffTab"))->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	TestEqual("buff hides debuff teeth",
		Find<UWidget>(Buff, TEXT("DebuffTeeth"))->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual("debuff hides buff tab",
		Find<UWidget>(Debuff, TEXT("BuffTab"))->GetVisibility(),
		ESlateVisibility::Collapsed);
	TestEqual("debuff teeth are visible",
		Find<UWidget>(Debuff, TEXT("DebuffTeeth"))->GetVisibility(),
		ESlateVisibility::HitTestInvisible);

	const FMordecaiUIThemeColors Diorama = FMordecaiUIThemeColors::MakeDiorama();
	const UImage* BuffBody = Find<UImage>(Buff, TEXT("TintImage"));
	const UImage* DebuffBody = Find<UImage>(Debuff, TEXT("TintImage"));
	TestTrue("buff body uses enamel fill",
		BuffBody && BuffBody->GetBrush().TintColor.GetSpecifiedColor().Equals(Diorama.Enamel));
	TestTrue("debuff body uses plate fill",
		DebuffBody && DebuffBody->GetBrush().TintColor.GetSpecifiedColor().Equals(Diorama.Plate));
	TestTrue("chip body has the 3px ink outline",
		BuffBody
		&& BuffBody->GetBrush().OutlineSettings.Width == 3.f
		&& BuffBody->GetBrush().OutlineSettings.Color.GetSpecifiedColor().Equals(Diorama.Ink));

	const UTextBlock* BuffDuration = Find<UTextBlock>(Buff, TEXT("DurationText"));
	const UTextBlock* DebuffDuration = Find<UTextBlock>(Debuff, TEXT("DurationText"));
	TestEqual("10 second chip renders the complete 10.0 value",
		BuffDuration ? BuffDuration->GetText().ToString() : FString(), FString(TEXT("10.0")));
	TestEqual("5 second chip renders 5.0",
		DebuffDuration ? DebuffDuration->GetText().ToString() : FString(), FString(TEXT("5.0")));
	const UCanvasPanelSlot* BuffDurationSlot = BuffDuration
		? Cast<UCanvasPanelSlot>(BuffDuration->GetParent()->Slot) : nullptr;
	const UCanvasPanelSlot* DebuffDurationSlot = DebuffDuration
		? Cast<UCanvasPanelSlot>(DebuffDuration->GetParent()->Slot) : nullptr;
	TestEqual("duration box is 51px from 10 seconds",
		BuffDurationSlot ? static_cast<float>(BuffDurationSlot->GetSize().X) : 0.f, 51.f);
	TestEqual("duration box is 38px under 10 seconds",
		DebuffDurationSlot ? static_cast<float>(DebuffDurationSlot->GetSize().X) : 0.f, 38.f);
	TestEqual("Frostbitten owns four pips",
		Find<UHorizontalBox>(Frost, TEXT("StackPips"))->GetChildrenCount(), 4);
	TestEqual("Focused uses the approved per-row value",
		Find<UTextBlock>(Focused, TEXT("DurationText"))->GetText().ToString(),
		FString(TEXT("3/3")));
	TestEqual("Exposed uses the approved per-row value",
		Find<UTextBlock>(Exposed, TEXT("DurationText"))->GetText().ToString(),
		FString(TEXT("1 hit")));
	Focused->SetCompact(true);
	const UImage* FocusedGlyph = Find<UImage>(Focused, TEXT("StatusGlyph"));
	TestTrue("compact Focused retains a visible loaded glyph",
		IsShown(FocusedGlyph)
		&& FocusedGlyph->GetBrush().GetResourceObject() != nullptr);
	const UImage* FearGlyph = Find<UImage>(Fear, TEXT("StatusGlyph"));
	TestTrue("Fear resolves dmg-shadow-edge",
		FearGlyph && FearGlyph->GetColorAndOpacity().Equals(Diorama.DamageShadowEdge));

	PaintForCachedGeometry(Frost, FVector2D(Frost->GetChipWidth(false), 52.f));
	const UImage* FrostBody = Find<UImage>(Frost, TEXT("TintImage"));
	const UImage* FrostGlyph = Find<UImage>(Frost, TEXT("StatusGlyph"));
	const UTextBlock* FrostName = Find<UTextBlock>(Frost, TEXT("StatusNameText"));
	const UHorizontalBox* FrostPips = Find<UHorizontalBox>(Frost, TEXT("StackPips"));
	const UTextBlock* FrostDuration =
		Find<UTextBlock>(Frost, TEXT("DurationText"));
	const FSlateRect BodyRect = CachedRect(FrostBody);
	const FSlateRect GlyphRect = CachedRect(FrostGlyph);
	const FSlateRect NameRect = CachedRect(FrostName);
	const FSlateRect DurationRect = CachedRect(FrostDuration);
	TestTrue("real arranged body geometry is non-zero",
		BodyRect.Right > BodyRect.Left && BodyRect.Bottom > BodyRect.Top);
	TestTrue("real arranged glyph stays inside the body",
		GlyphRect.Left >= BodyRect.Left && GlyphRect.Right <= BodyRect.Right
		&& GlyphRect.Top >= BodyRect.Top && GlyphRect.Bottom <= BodyRect.Bottom);
	TestTrue("real arranged name stays inside the body",
		NameRect.Left >= BodyRect.Left && NameRect.Right <= BodyRect.Right
		&& NameRect.Top >= BodyRect.Top && NameRect.Bottom <= BodyRect.Bottom);
	TestTrue("real arranged duration stays inside the body",
		DurationRect.Left >= BodyRect.Left
		&& DurationRect.Right <= BodyRect.Right
		&& DurationRect.Top >= BodyRect.Top
		&& DurationRect.Bottom <= BodyRect.Bottom);
	TestTrue("real arranged name and duration vertical centres match within 1px",
		FMath::Abs(
			(NameRect.Top + NameRect.Bottom)
			- (DurationRect.Top + DurationRect.Bottom)) <= 2.f);

	FSlateRect PreviousPipRect;
	for (int32 Index = 0; FrostPips && Index < FrostPips->GetChildrenCount(); ++Index)
	{
		const FSlateRect PipRect = CachedRect(FrostPips->GetChildAt(Index));
		TestTrue(*FString::Printf(TEXT("real arranged pip %d stays inside the body"), Index),
			PipRect.Left >= BodyRect.Left && PipRect.Right <= BodyRect.Right
			&& PipRect.Top >= BodyRect.Top && PipRect.Bottom <= BodyRect.Bottom);
		if (Index > 0)
		{
			TestTrue(*FString::Printf(
				TEXT("real arranged pip %d has the specified 6px gap"), Index),
				PipRect.Left - PreviousPipRect.Right >= 6.f);
		}
		PreviousPipRect = PipRect;
	}
	const FSlateRect FirstPipRect = FrostPips && FrostPips->GetChildrenCount() > 0
		? CachedRect(FrostPips->GetChildAt(0)) : FSlateRect();
	const FSlateRect LastPipRect = FrostPips && FrostPips->GetChildrenCount() > 0
		? CachedRect(FrostPips->GetChildAt(FrostPips->GetChildrenCount() - 1))
		: FSlateRect();
	TestTrue("real arranged name-to-pips gap is at least 6px",
		FirstPipRect.Left - NameRect.Right >= 6.f);
	TestTrue("real arranged pips do not overlap duration and keep a 6px gap",
		DurationRect.Left - LastPipRect.Right >= 6.f);

	PaintForCachedGeometry(Buff, FVector2D(Buff->GetChipWidth(false), 52.f));
	const FSlateRect BuffBodyRect =
		CachedRect(Find<UImage>(Buff, TEXT("TintImage")));
	const FSlateRect BuffNameRect =
		CachedRect(Find<UTextBlock>(Buff, TEXT("StatusNameText")));
	const FSlateRect BuffDurationRect =
		CachedRect(Find<UTextBlock>(Buff, TEXT("DurationText")));
	TestTrue("real Stone Skin name-to-duration gap is at least 6px",
		BuffDurationRect.Left - BuffNameRect.Right >= 6.f);
	TestTrue("real Stone Skin name and duration stay inside the body",
		BuffNameRect.Left >= BuffBodyRect.Left
		&& BuffNameRect.Right <= BuffBodyRect.Right
		&& BuffDurationRect.Left >= BuffBodyRect.Left
		&& BuffDurationRect.Right <= BuffBodyRect.Right);
	TestTrue("real Stone Skin name and duration vertical centres match within 1px",
		FMath::Abs(
			(BuffNameRect.Top + BuffNameRect.Bottom)
			- (BuffDurationRect.Top + BuffDurationRect.Bottom)) <= 2.f);

	PaintForCachedGeometry(
		Shocked, FVector2D(Shocked->GetChipWidth(true), 52.f));
	const FSlateRect ShockedBodyRect =
		CachedRect(Find<UImage>(Shocked, TEXT("TintImage")));
	const FSlateRect ShockedDurationRect =
		CachedRect(Find<UTextBlock>(Shocked, TEXT("DurationText")));
	const UHorizontalBox* ShockedPips =
		Find<UHorizontalBox>(Shocked, TEXT("StackPips"));
	FSlateRect ShockedLastPipRect(0.f, 0.f, 0.f, 0.f);
	for (int32 Index = 0;
		ShockedPips && Index < ShockedPips->GetChildrenCount(); ++Index)
	{
		const FSlateRect PipRect = CachedRect(ShockedPips->GetChildAt(Index));
		TestTrue(*FString::Printf(
			TEXT("real compact Shocked pip %d stays inside the body"), Index),
			PipRect.Left >= ShockedBodyRect.Left
			&& PipRect.Right <= ShockedBodyRect.Right);
		ShockedLastPipRect = PipRect;
	}
	TestTrue("real compact Shocked pips keep 6px clear of duration",
		ShockedDurationRect.Left - ShockedLastPipRect.Right >= 6.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_StatusChip_GameplayCategoriesStayExact,
	"Mordecai.UI.StatusChip.GameplayCategoriesStayExact",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_StatusChip_GameplayCategoriesStayExact::RunTest(const FString& Parameters)
{
	using namespace MordecaiGameplayTags;
	TestEqual("MicroStunned remains gameplay Control",
		GetStatusEffectCategory(Status_MicroStunned),
		EMordecaiStatusEffectCategory::Control);
	TestEqual("Sleeping keeps the base gameplay default Debuff category",
		GetStatusEffectCategory(Status_Sleeping),
		EMordecaiStatusEffectCategory::Debuff);
	const TArray<FGameplayTag> Buffs =
		GetStatusTagsForCategory(EMordecaiStatusEffectCategory::Buff);
	TestEqual("gameplay Buff has exactly one member", Buffs.Num(), 1);
	TestTrue("Focused is the exact gameplay Buff membership",
		Buffs.Num() == 1 && Buffs[0] == Status_Focused);
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
	Env.LocalPlayer->PlayerAdded(nullptr, FPlatformMisc::GetPlatformUserForUserIndex(0));
	ON_SCOPE_EXIT { Env.LocalPlayer->PlayerRemoved(); };
	UMordecaiCombatFeedbackWidget* Callout =
		Env.MakeWidget<UMordecaiCombatFeedbackWidget>();
	TSharedRef<SWidget> Slate = Callout->TakeWidget();
	UTextBlock* FeedbackText = Find<UTextBlock>(Callout, TEXT("FeedbackText"));
	TestNotNull("CalloutPlate native fallback exists",
		Find<UWidget>(Callout, TEXT("CalloutPlate")));
	TestEqual("shipped duration is exactly one second",
		Callout->FeedbackDisplayDuration, 1.f);

	Callout->ShowFeedback(TEXT("Perfect Dodge!"), FLinearColor::Green);
	Slate->Tick(FGeometry(), 0.99, 0.99f);
	TestEqual("callout is still visible at 0.99 seconds",
		Callout->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Callout->ShowFeedback(TEXT("Parry Failed!"), FLinearColor::Red);
	TestEqual("latest callout replaces the previous one immediately",
		FeedbackText ? FeedbackText->GetText().ToString() : FString(),
		FString(TEXT("Parry Failed!")));
	TestEqual("replacement remains visible",
		Callout->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Slate->Tick(FGeometry(), 1.59, 0.6f);
	Callout->ShowFeedback(TEXT("Perfect Block!"), FLinearColor::Green);
	Slate->Tick(FGeometry(), 2.19, 0.6f);
	TestEqual("replacement owns a fresh timer after 0.6 seconds",
		Callout->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Slate->Tick(FGeometry(), 2.60, 0.41f);
	TestEqual("replacement collapses after its own one second",
		Callout->GetVisibility(), ESlateVisibility::Collapsed);

	UMordecaiUIThemeSubsystem* Themes =
		Env.LocalPlayer->GetSubsystem<UMordecaiUIThemeSubsystem>();
	TestNotNull("local theme subsystem", Themes);
	Callout->ShowFeedback(
		TEXT("Parry Failed!"), Themes->GetActiveColors().DangerText);
	Themes->SetActiveTheme(EMordecaiUITheme::HighContrast);
	TestTrue("direct negative callout survives a live theme switch",
		FeedbackText->GetColorAndOpacity().GetSpecifiedColor().Equals(
			Themes->GetActiveColors().DangerText));

	UAbilitySystemComponent* ASC = Env.GetPlayerASC();
	Callout->BindToASC(ASC);
	ASC->AddLooseGameplayTag(MordecaiGameplayTags::State_ParryWhiff);
	TestEqual("ParryWhiff tag drives exact negative copy",
		FeedbackText->GetText().ToString(), FString(TEXT("Parry Failed!")));
	Themes->SetActiveTheme(EMordecaiUITheme::Diorama);
	TestTrue("tag-driven danger colour survives another theme switch",
		FeedbackText->GetColorAndOpacity().GetSpecifiedColor().Equals(
			Themes->GetActiveColors().DangerText));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Combo_SurvivesHUDReactivation,
	"Mordecai.UI.Combo.SurvivesHUDReactivation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Combo_LiveMeleeAbilityUpdatesBoundCounter,
	"Mordecai.UI.Combo.LiveMeleeAbilityUpdatesBoundCounter",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_UI_Combo_BlockedTagsStopQueuedStep,
	"Mordecai.UI.Combo.BlockedTagsStopQueuedStep",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_UI_Combo_LiveMeleeAbilityUpdatesBoundCounter::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyStatusFeedbackTest;
	using namespace MordecaiRecoveryTest;
	FRecoveryControlledTestEnv Env;
	ON_SCOPE_EXIT { Env.TeardownWithController(); };
	if (!TestTrue("controlled player world", Env.SetupWithController()))
	{
		return true;
	}

	ULyraAbilitySystemComponent* ASC =
		CastChecked<ULyraAbilitySystemComponent>(Env.GetPlayerASC());
	UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
	UMordecaiComboCounterWidget* Combo = Cast<UMordecaiComboCounterWidget>(
		HUD->GetNamedWidget(HUD, TEXT("ComboCounter")));
	UTextBlock* ComboText = Find<UTextBlock>(Combo, TEXT("ComboText"));
	if (!TestNotNull("HUD owns bound combo counter", Combo)
		|| !TestNotNull("combo counter owns text", ComboText))
	{
		return true;
	}
	HUD->BindToASC(ASC);

	FGameplayAbilitySpec Spec(UMordecaiGA_MeleeAttack::StaticClass(), 1);
	Spec.GetDynamicSpecSourceTags().AddTag(MordecaiGameplayTags::Input_LightAttack);
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
	FGameplayAbilitySpec* Granted = ASC->FindAbilitySpecFromHandle(Handle);
	UMordecaiGA_MeleeAttack* Melee = Granted
		? Cast<UMordecaiGA_MeleeAttack>(Granted->GetPrimaryInstance())
		: nullptr;
	if (!TestNotNull("real instanced melee ability", Melee))
	{
		return true;
	}
	Melee->AttackProfiles = { MakeStubProfile(), MakeStubProfile() };

	auto PressLight = [ASC]()
	{
		ASC->AbilityInputTagPressed(MordecaiGameplayTags::Input_LightAttack);
		ASC->ProcessAbilityInput(0.f, false);
		ASC->AbilityInputTagReleased(MordecaiGameplayTags::Input_LightAttack);
		ASC->ProcessAbilityInput(0.f, false);
	};
	auto Tick = [&Env]()
	{
		++GFrameCounter;
		Env.World->TimeSeconds += 0.11f;
		Env.World->GetTimerManager().Tick(0.11f);
	};

	TestTrue("real melee ability activates", ASC->TryActivateAbility(Handle));
	TestEqual("hit 1 begins in Windup",
		Melee->GetCurrentPhase(), EMordecaiAttackPhase::Windup);
	for (int32 Frame = 0;
		Frame < 10 && Melee->GetCurrentPhase() != EMordecaiAttackPhase::Recovery;
		++Frame)
	{
		Tick();
	}
	TestEqual("hit 1 reaches Recovery",
		Melee->GetCurrentPhase(), EMordecaiAttackPhase::Recovery);
	TestEqual("hit 1 opens the bound counter",
		ComboText->GetText().ToString(), FString(TEXT("Hit 1")));

	PressLight(); // real second light input while the first hit is in Recovery
	for (int32 Frame = 0;
		Frame < 10
			&& (Melee->GetCurrentComboIndex() != 1
				|| Melee->GetCurrentPhase() != EMordecaiAttackPhase::Recovery);
		++Frame)
	{
		Tick();
	}
	TestEqual("hit 2 updates the bound counter",
		ComboText->GetText().ToString(), FString(TEXT("Hit 2")));

	for (int32 Frame = 0;
		Frame < 10 && Melee->GetCurrentPhase() != EMordecaiAttackPhase::None;
		++Frame)
	{
		Tick();
	}
	TestEqual("counter hides when the combo window closes",
		Combo->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

bool FMordecai_UI_Combo_BlockedTagsStopQueuedStep::RunTest(const FString& Parameters)
{
	using namespace MordecaiTinTallyStatusFeedbackTest;
	using namespace MordecaiRecoveryTest;

	auto RunBlockedCase = [this](const FGameplayTag BlockedTag, const TCHAR* CaseName,
		const bool bAddTagBeforeInput)
	{
		FRecoveryControlledTestEnv Env;
		ON_SCOPE_EXIT { Env.TeardownWithController(); };
		if (!TestTrue(*FString::Printf(TEXT("%s: controlled player world"), CaseName),
			Env.SetupWithController()))
		{
			return;
		}

		ULyraAbilitySystemComponent* ASC =
			CastChecked<ULyraAbilitySystemComponent>(Env.GetPlayerASC());
		UMordecaiCombatHUDWidget* HUD = Env.MakeWidget<UMordecaiCombatHUDWidget>();
		UMordecaiComboCounterWidget* Combo = Cast<UMordecaiComboCounterWidget>(
			HUD->GetNamedWidget(HUD, TEXT("ComboCounter")));
		UTextBlock* ComboText = Find<UTextBlock>(Combo, TEXT("ComboText"));
		if (!TestNotNull(*FString::Printf(TEXT("%s: combo counter"), CaseName), Combo)
			|| !TestNotNull(*FString::Printf(TEXT("%s: combo text"), CaseName), ComboText))
		{
			return;
		}
		HUD->BindToASC(ASC);

		FGameplayAbilitySpec Spec(UMordecaiGA_MeleeAttack::StaticClass(), 1);
		Spec.GetDynamicSpecSourceTags().AddTag(MordecaiGameplayTags::Input_LightAttack);
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		FGameplayAbilitySpec* Granted = ASC->FindAbilitySpecFromHandle(Handle);
		UMordecaiGA_MeleeAttack* Melee = Granted
			? Cast<UMordecaiGA_MeleeAttack>(Granted->GetPrimaryInstance())
			: nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("%s: real melee ability"), CaseName), Melee))
		{
			return;
		}

		UMordecaiAttackProfileDataAsset* First = MakeStubProfile();
		UMordecaiAttackProfileDataAsset* Second = MakeStubProfile();
		First->StaminaCost = 5.f;
		Second->StaminaCost = 5.f;
		Melee->AttackProfiles = { First, Second };
		ASC->SetNumericAttributeBase(UMordecaiAttributeSet::GetStaminaAttribute(), 100.f);

		auto Tick = [&Env]()
		{
			++GFrameCounter;
			Env.World->TimeSeconds += 0.11f;
			Env.World->GetTimerManager().Tick(0.11f);
		};
		auto PressLight = [ASC]()
		{
			ASC->AbilityInputTagPressed(MordecaiGameplayTags::Input_LightAttack);
			ASC->ProcessAbilityInput(0.f, false);
			ASC->AbilityInputTagReleased(MordecaiGameplayTags::Input_LightAttack);
			ASC->ProcessAbilityInput(0.f, false);
		};

		TestTrue(*FString::Printf(TEXT("%s: first step activates"), CaseName),
			ASC->TryActivateAbility(Handle));
		for (int32 Frame = 0;
			Frame < 10 && Melee->GetCurrentPhase() != EMordecaiAttackPhase::Recovery;
			++Frame)
		{
			Tick();
		}
		TestEqual(*FString::Printf(TEXT("%s: first step reaches Recovery"), CaseName),
			Melee->GetCurrentPhase(), EMordecaiAttackPhase::Recovery);
		const float StaminaAfterFirst = ASC->GetNumericAttribute(
			UMordecaiAttributeSet::GetStaminaAttribute());

		if (bAddTagBeforeInput)
		{
			ASC->AddLooseGameplayTag(BlockedTag);
		}
		PressLight();
		if (!bAddTagBeforeInput)
		{
			ASC->AddLooseGameplayTag(BlockedTag);
		}
		Tick(); // Finish hit 1 Recovery; a blocked hit 2 must not start.

		TestEqual(*FString::Printf(TEXT("%s: combo index stays on hit 1"), CaseName),
			Melee->GetCurrentComboIndex(), 0);
		TestFalse(*FString::Printf(TEXT("%s: ability ends after blocked queue"), CaseName),
			Melee->IsActive());
		TestEqual(*FString::Printf(TEXT("%s: blocked step spends no stamina"), CaseName),
			ASC->GetNumericAttribute(UMordecaiAttributeSet::GetStaminaAttribute()),
			StaminaAfterFirst);
		TestTrue(*FString::Printf(TEXT("%s: combo counter stays hidden"), CaseName),
			Combo->GetVisibility() == ESlateVisibility::Collapsed);
		TestNotEqual(*FString::Printf(TEXT("%s: combo counter never shows Hit 2"), CaseName),
			ComboText->GetText().ToString(), FString(TEXT("Hit 2")));
	};

	const TPair<FGameplayTag, const TCHAR*> BlockedCases[] = {
		{ MordecaiGameplayTags::State_PostureBroken, TEXT("PostureBroken before input") },
		{ MordecaiGameplayTags::State_KnockedDown, TEXT("KnockedDown before input") },
		{ MordecaiGameplayTags::State_Parrying, TEXT("Parrying before input") },
		{ MordecaiGameplayTags::State_ParryWhiff, TEXT("ParryWhiff before input") },
		{ MordecaiGameplayTags::State_Dead, TEXT("Dead before input") },
	};
	for (const TPair<FGameplayTag, const TCHAR*>& BlockedCase : BlockedCases)
	{
		RunBlockedCase(BlockedCase.Key, BlockedCase.Value, true);
	}
	RunBlockedCase(MordecaiGameplayTags::State_Dead, TEXT("Dead after queued input"), false);
	return true;
}

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
	Combo->SetComboVisible(false); // A rebuilt Slate widget starts collapsed before its ASC rebind.
	HUD->BindToASC(ASC);
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
	TestEqual("active combo keeps Hit N through HUD rebuild and rebind",
		ComboText ? ComboText->GetText().ToString() : FString(),
		FString(TEXT("Hit 2")));
	TestEqual("active combo becomes visible again after HUD rebuild and rebind",
		Combo->GetVisibility(), ESlateVisibility::HitTestInvisible);
	ASC->RemoveLooseGameplayTag(MordecaiGameplayTags::State_ComboWindow);
	TestEqual("combo hides when the active window closes",
		Combo->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}

#endif // WITH_AUTOMATION_TESTS
