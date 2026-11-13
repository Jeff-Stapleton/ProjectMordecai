// Project Mordecai — Combo Counter Widget (US-061)

#include "Mordecai/UI/MordecaiComboCounterWidget.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiComboCounterWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiComboCounterWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ComboText && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}

	ComboDisplayTextCache.Reserve(1024);
	for (int32 ComboIndex = 0; ComboIndex < 1024; ++ComboIndex)
	{
		ComboDisplayTextCache.Add(FText::Format(
			NSLOCTEXT("MordecaiComboCounter", "HitCount", "Hit {0}"),
			FText::AsNumber(ComboIndex + 1)));
	}
}

void UMordecaiComboCounterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &ThisClass::ApplyTheme);
}

void UMordecaiComboCounterWidget::NativeDestruct()
{
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiComboCounterWidget::BuildDefaultContent()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	USizeBox* PlateSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), TEXT("ComboPlateSize"));
	PlateSize->SetWidthOverride(108.f);
	PlateSize->SetHeightOverride(36.f);
	if (UOverlaySlot* PlateSizeSlot = Root->AddChildToOverlay(PlateSize))
	{
		PlateSizeSlot->SetHorizontalAlignment(HAlign_Center);
		PlateSizeSlot->SetVerticalAlignment(VAlign_Center);
	}

	ComboPlate = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree,
		FMordecaiUIPrimitiveBuilder::Plate(
			FMordecaiUIThemeColors::MakeDiorama(),
			EMordecaiUIPrintDepth::HUD,
			EMordecaiUITheme::Diorama),
		TEXT("ComboPlate"));
	PlateSize->SetContent(ComboPlate);
	ComboText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("ComboText"));
	ComboText->SetText(FText::GetEmpty());
	ComboText->SetJustification(ETextJustify::Center);
	ComboText->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
	MordecaiUITypography::Apply(ComboText, TEXT("num_m"));
	FSlateFontInfo FontInfo = ComboText->GetFont();
	FontInfo.TypefaceFontName = TEXT("Bold");
	FontInfo.Size = 18;
	ComboText->SetFont(FontInfo);
	if (UOverlaySlot* TextSlot = ComboPlate->AddChildToOverlay(ComboText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(8.f, 2.f));
	}
	ApplyTheme(EMordecaiUITheme::Diorama, FMordecaiUIThemeColors::MakeDiorama());
}

// ---------------------------------------------------------------------------
// Static Helper
// ---------------------------------------------------------------------------

FString UMordecaiComboCounterWidget::FormatComboHit(int32 ComboIndex)
{
	if (ComboIndex < 0)
	{
		return FString();
	}
	return FString::Printf(TEXT("Hit %d"), ComboIndex + 1);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

void UMordecaiComboCounterWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}

	if (UAbilitySystemComponent* Previous = BoundASC.Get())
	{
		Previous->RegisterGameplayTagEvent(MordecaiGameplayTags::State_ComboWindow,
			EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	}
	BoundASC = ASC;

	ASC->RegisterGameplayTagEvent(MordecaiGameplayTags::State_ComboWindow, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiComboCounterWidget::OnComboWindowTagChanged);

	// Re-read on activation: the combo window may have changed while another UI.Layer.Game widget was on top.
	SetComboVisible(ASC->HasMatchingGameplayTag(MordecaiGameplayTags::State_ComboWindow));
}

void UMordecaiComboCounterWidget::SetComboIndex(int32 ComboIndex)
{
	CachedComboIndex = ComboIndex;
	UpdateDisplay();
}

void UMordecaiComboCounterWidget::SetComboVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMordecaiComboCounterWidget::OnComboWindowTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		SetComboVisible(true);
	}
	else
	{
		// Combo window closed — hide after brief display
		SetComboVisible(false);
		CachedComboIndex = -1;
	}
}

void UMordecaiComboCounterWidget::UpdateDisplay()
{
	if (ComboText)
	{
		if (ComboDisplayTextCache.IsValidIndex(CachedComboIndex))
		{
			ComboText->SetText(ComboDisplayTextCache[CachedComboIndex]);
		}
		else if (CachedComboIndex < 0)
		{
			ComboText->SetText(FText::GetEmpty());
		}
		else
		{
			ComboText->SetText(FText::Format(
				NSLOCTEXT("MordecaiComboCounter", "HitCount", "Hit {0}"),
				FText::AsNumber(CachedComboIndex + 1)));
		}
	}
}

void UMordecaiComboCounterWidget::ApplyTheme(
	const EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentThemeColors = Colors;
	if (ComboPlate)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			ComboPlate,
			FMordecaiUIPrimitiveBuilder::Plate(
				Colors, EMordecaiUIPrintDepth::HUD, Theme));
	}
	if (ComboText)
	{
		ComboText->SetColorAndOpacity(FSlateColor(Colors.Good));
		FSlateFontInfo Font = ComboText->GetFont();
		Font.OutlineSettings.OutlineSize =
			Theme == EMordecaiUITheme::HighContrast ? 0 : 1;
		Font.OutlineSettings.OutlineColor = Colors.Ink.ToFColorSRGB();
		ComboText->SetFont(Font);
	}
}
