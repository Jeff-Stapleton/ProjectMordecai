// Project Mordecai — Spell Cooldown Widget (US-055)

#include "Mordecai/UI/MordecaiSpellCooldownWidget.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeBinding.h"
#include "Mordecai/UI/TinTally/MordecaiUITypography.h"
#include "Mordecai/Magic/MordecaiSpellDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiSpellCooldownWidget)

namespace
{
	FLinearColor ResolveSpellGlyphTint(
		const FName GlyphName,
		const FMordecaiUIThemeColors& Colors)
	{
		if (GlyphName == TEXT("fireball"))
		{
			return Colors.DamageFire;
		}
		if (GlyphName == TEXT("restoration"))
		{
			return Colors.Heal;
		}
		return Colors.Text;
	}
}

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!CooldownProgressBar && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
	SetSlotKeyLabel(SlotKeyLabel);
}

void UMordecaiSpellCooldownWidget::NativeConstruct()
{
	Super::NativeConstruct();
	MordecaiUIThemeBinding::Bind(
		this, BoundThemeSubsystem, ThemeChangedHandle,
		this, &UMordecaiSpellCooldownWidget::ApplyTheme);
}

void UMordecaiSpellCooldownWidget::NativeDestruct()
{
	UnbindFromCooldownTag();
	MordecaiUIThemeBinding::Unbind(BoundThemeSubsystem, ThemeChangedHandle);
	Super::NativeDestruct();
}

void UMordecaiSpellCooldownWidget::BuildDefaultContent()
{
	const FMordecaiUIThemeColors Colors = FMordecaiUIThemeColors::MakeDiorama();
	UOverlay* Root = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree,
		FMordecaiUIPrimitiveBuilder::Plate(
			Colors, EMordecaiUIPrintDepth::HUD, EMordecaiUITheme::Diorama),
		TEXT("SpellSlotPlate"));
	WidgetTree->RootWidget = Root;
	UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("SpellSlotContent"));
	if (UOverlaySlot* ContentSlot = Root->AddChildToOverlay(Content))
	{
		ContentSlot->SetPadding(FMargin(0.f));
	}
	auto Place = [Content](UWidget* Widget, FVector2D Position, FVector2D Size, int32 ZOrder = 0)
	{
		if (UCanvasPanelSlot* Slot = Content->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
			Slot->SetAutoSize(false);
			Slot->SetZOrder(ZOrder);
		}
	};

	UOverlay* Well = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Well(Colors, false), TEXT("SpellSlotWell"));
	Place(Well, FVector2D(7.f), FVector2D(64.f), 0);

	SpellIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SpellIcon"));
	FSlateBrush IconBrush;
	IconBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	IconBrush.TintColor = FSlateColor(Colors.Text);
	IconBrush.OutlineSettings = FSlateBrushOutlineSettings(
		FVector4(8.f), FSlateColor(FLinearColor::Transparent), 0.f);
	SpellIcon->SetBrush(IconBrush);
	SpellIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(SpellIcon, FVector2D(15.f), FVector2D(48.f), 1);

	CooldownProgressBar = WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("CooldownProgressBar"));
	FProgressBarStyle ProgressStyle = CooldownProgressBar->GetWidgetStyle();
	FSlateBrush Background;
	Background.DrawAs = ESlateBrushDrawType::NoDrawType;
	FSlateBrush Fill;
	Fill.DrawAs = ESlateBrushDrawType::Image;
	UMaterialInterface* Pattern = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/MordecaiCore/UI/TinTally/Materials/M_UI_PatternFill.M_UI_PatternFill"));
	if (Pattern)
	{
		ShutterMaterial = UMaterialInstanceDynamic::Create(Pattern, this);
		ShutterMaterial->SetVectorParameterValue(TEXT("Base"), Colors.PlateRaised);
		ShutterMaterial->SetVectorParameterValue(TEXT("Line"), Colors.Well);
		ShutterMaterial->SetScalarParameterValue(TEXT("Angle"), 0.f);
		ShutterMaterial->SetScalarParameterValue(TEXT("Period"), 6.f);
		ShutterMaterial->SetScalarParameterValue(TEXT("LineWidth"), 2.f);
		Fill.SetResourceObject(ShutterMaterial);
	}
	ProgressStyle.SetBackgroundImage(Background);
	ProgressStyle.SetFillImage(Fill);
	CooldownProgressBar->SetWidgetStyle(ProgressStyle);
	CooldownProgressBar->SetBarFillType(EProgressBarFillType::TopToBottom);
	CooldownProgressBar->SetPercent(0.f);
	CooldownProgressBar->SetFillColorAndOpacity(FLinearColor::White);
	Place(CooldownProgressBar, FVector2D(7.f), FVector2D(64.f), 2);

	CooldownShutterLip = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("CooldownShutterLip"));
	CooldownShutterLip->SetBrushColor(Colors.Rim);
	CooldownShutterLip->SetPadding(FMargin(0.f));
	CooldownShutterLip->SetVisibility(ESlateVisibility::Collapsed);
	Place(CooldownShutterLip, FVector2D(7.f, 68.f), FVector2D(64.f, 3.f), 3);

	CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CooldownText"));
	CooldownText->SetText(FText::GetEmpty());
	CooldownText->SetColorAndOpacity(FSlateColor(Colors.Text));
	CooldownText->SetJustification(ETextJustify::Center);
	FSlateFontInfo FontInfo = CooldownText->GetFont();
	FontInfo.Size = 18;
	FontInfo.OutlineSettings.OutlineSize = 2;
	FontInfo.OutlineSettings.OutlineColor = Colors.Ink.ToFColorSRGB();
	CooldownText->SetFont(FontInfo);
	MordecaiUITypography::Apply(CooldownText, TEXT("num_m"));
	Place(CooldownText, FVector2D(7.f, 24.f), FVector2D(64.f, 30.f), 4);

	SlotKey = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree,
		FMordecaiUIPrimitiveBuilder::Keycap(Colors, EMordecaiKeycapShape::Square),
		TEXT("SlotKey"));
	Place(SlotKey, FVector2D(-6.f, 56.f), FVector2D(32.f), 5);
	SlotKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotKeyText"));
	SlotKeyText->SetText(SlotKeyLabel);
	SlotKeyText->SetColorAndOpacity(FSlateColor(Colors.Ink));
	SlotKeyText->SetJustification(ETextJustify::Center);
	FontInfo.OutlineSettings.OutlineSize = 0;
	FontInfo.Size = 15;
	SlotKeyText->SetFont(FontInfo);
	MordecaiUITypography::Apply(SlotKeyText, TEXT("num_s"));
	FSlateFontInfo KeyFont = SlotKeyText->GetFont();
	KeyFont.TypefaceFontName = TEXT("ExtraBold");
	SlotKeyText->SetFont(KeyFont);
	Place(SlotKeyText, FVector2D(-6.f, 60.f), FVector2D(32.f, 24.f), 6);

	CostRivets = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(), TEXT("CostRivets"));
	Place(CostRivets, FVector2D(72.f, -7.f), FVector2D(0.f, 14.f), 6);

	LockChip = FMordecaiUIPrimitiveBuilder::Build(
		WidgetTree, FMordecaiUIPrimitiveBuilder::Pill(Colors), TEXT("LockChip"));
	// TODO(DECISION): define the lock-chip state and copy before making this layer live.
	LockChip->SetVisibility(ESlateVisibility::Collapsed);
	Place(LockChip, FVector2D(24.f, 27.f), FVector2D(30.f, 24.f), 7);

	MuzzleBadge = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(), TEXT("MuzzleBadge"));
	// TODO(DECISION): define the muzzle-badge trigger and glyph before making this layer live.
	MuzzleBadge->SetVisibility(ESlateVisibility::Collapsed);
	Place(MuzzleBadge, FVector2D(16.f), FVector2D(46.f), 7);
}

// ---------------------------------------------------------------------------
// Static Helpers (AC-055.8)
// ---------------------------------------------------------------------------

float UMordecaiSpellCooldownWidget::CalcCooldownPercent(float ElapsedTime, float TotalDuration)
{
	if (TotalDuration <= 0.f)
	{
		return 0.f; // No cooldown = fully open shutter
	}
	return 1.f - FMath::Clamp(ElapsedTime / TotalDuration, 0.f, 1.f);
}

FString UMordecaiSpellCooldownWidget::FormatRemainingTime(float RemainingSeconds)
{
	if (RemainingSeconds <= 0.f)
	{
		return FString(); // Ready state = empty string
	}
	return FString::Printf(TEXT("%.1f"), RemainingSeconds);
}

// ---------------------------------------------------------------------------
// Binding (AC-055.9)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::SetSlotKeyLabel(const FText& Label)
{
	SlotKeyLabel = Label;
	if (SlotKeyText)
	{
		SlotKeyText->SetText(SlotKeyLabel);
	}
}

void UMordecaiSpellCooldownWidget::ConfigureFromSpellData(UMordecaiSpellDataAsset* SpellData)
{
	if (!SpellData || !SpellIcon || !CostRivets)
	{
		return;
	}
	FString GlyphName = SpellData->SpellId.ToString().ToLower();
	GlyphName.ReplaceInline(TEXT("_"), TEXT(""));
	GlyphName.ReplaceInline(TEXT(" "), TEXT(""));
	ConfiguredSpellGlyph = FName(GlyphName);
	const FString TexturePath = FString::Printf(
		TEXT("/MordecaiCore/UI/TinTally/Textures/Glyphs/T_UI_Glyph_%s.T_UI_Glyph_%s"),
		*GlyphName,
		*GlyphName);
	UTexture2D* GlyphTexture = LoadObject<UTexture2D>(nullptr, *TexturePath);
	FSlateBrush IconBrush = SpellIcon->GetBrush();
	IconBrush.DrawAs = GlyphTexture ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	IconBrush.SetResourceObject(GlyphTexture);
	IconBrush.TintColor = FSlateColor(
		ResolveSpellGlyphTint(ConfiguredSpellGlyph, CurrentThemeColors));
	SpellIcon->SetBrush(IconBrush);

	CostRivets->ClearChildren();
	const int32 RivetCount = FMath::Max(0, SpellData->SpellPointCost);
	const float RivetRowWidth = RivetCount > 0
		? RivetCount * 14.f + (RivetCount - 1) * 3.f
		: 0.f;
	if (UCanvasPanelSlot* RivetRowSlot = Cast<UCanvasPanelSlot>(CostRivets->Slot))
	{
		RivetRowSlot->SetPosition(FVector2D(72.f - RivetRowWidth, -7.f));
		RivetRowSlot->SetSize(FVector2D(RivetRowWidth, 14.f));
	}
	for (int32 Index = 0; Index < RivetCount; ++Index)
	{
		UImage* Rivet = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("CostRivet_%02d"), Index)));
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.ImageSize = FVector2D(14.f);
		Brush.TintColor = FSlateColor(CurrentThemeColors.SpellPoints);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(
			FVector4(7.f), FSlateColor(CurrentThemeColors.Ink), 2.f);
		Rivet->SetBrush(Brush);
		Rivet->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* RivetSlot = CostRivets->AddChildToHorizontalBox(Rivet))
		{
			RivetSlot->SetPadding(FMargin(Index > 0 ? 3.f : 0.f, 0.f, 0.f, 0.f));
			RivetSlot->SetVerticalAlignment(VAlign_Center);
			RivetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}
}

void UMordecaiSpellCooldownWidget::SetPreviewCooldownFraction(float Fraction)
{
	PreviewCooldownFraction = FMath::Clamp(Fraction, 0.f, 1.f);
	bPreviewCooldownHeld = PreviewCooldownFraction > 0.f;
	bIsOnCooldown = bPreviewCooldownHeld;
	ApplyCooldownFraction(PreviewCooldownFraction);
	if (CooldownText)
	{
		CooldownText->SetText(bPreviewCooldownHeld
			? FText::FromString(TEXT("2.4"))
			: FText::GetEmpty());
	}
}

void UMordecaiSpellCooldownWidget::BindToCooldownTag(UAbilitySystemComponent* ASC, FGameplayTag CooldownTag, float TotalCooldown)
{
	UnbindFromCooldownTag();

	BoundCooldownTag = CooldownTag;
	TotalCooldownDuration = TotalCooldown;
	if (!ASC || !CooldownTag.IsValid())
	{
		UpdateDisplay();
		return;
	}

	BoundASC = ASC;
	CooldownTagChangedHandle =
		ASC->RegisterGameplayTagEvent(BoundCooldownTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UMordecaiSpellCooldownWidget::OnCooldownTagChanged);

	OnCooldownTagChanged(BoundCooldownTag, ASC->GetTagCount(BoundCooldownTag));
}

void UMordecaiSpellCooldownWidget::UnbindFromCooldownTag()
{
	if (CooldownTagChangedHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = BoundASC.Get())
		{
			ASC->UnregisterGameplayTagEvent(
				CooldownTagChangedHandle,
				BoundCooldownTag,
				EGameplayTagEventType::NewOrRemoved);
		}
		CooldownTagChangedHandle.Reset();
	}
	BoundASC.Reset();
	bPreviewCooldownHeld = false;
	PreviewCooldownFraction = 0.f;
	bIsOnCooldown = false;
}

// ---------------------------------------------------------------------------
// Tag Change Callback (AC-055.9)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::OnCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	bPreviewCooldownHeld = false;
	PreviewCooldownFraction = 0.f;
	if (NewCount > 0)
	{
		// Cooldown started
		bIsOnCooldown = true;
		CooldownStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

		// Try to get actual remaining time from the active GE with this cooldown tag
		if (UAbilitySystemComponent* ASC = BoundASC.Get())
		{
			FGameplayEffectQuery Query;
			Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(BoundCooldownTag));
			TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffects(Query);
			for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
			{
				const float Remaining = ASC->GetActiveGameplayEffect(Handle)->GetTimeRemaining(GetWorld()->GetTimeSeconds());
				const float Duration = ASC->GetActiveGameplayEffect(Handle)->GetDuration();
				if (Duration > 0.f)
				{
					TotalCooldownDuration = Duration;
					CooldownStartTime = GetWorld()->GetTimeSeconds() - (Duration - Remaining);
					break;
				}
			}
		}
	}
	else
	{
		// Cooldown ended
		bIsOnCooldown = false;
	}

	UpdateDisplay();
}

// ---------------------------------------------------------------------------
// Tick (AC-055.8)
// ---------------------------------------------------------------------------

void UMordecaiSpellCooldownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsOnCooldown)
	{
		UpdateDisplay();
	}
}

void UMordecaiSpellCooldownWidget::UpdateDisplay()
{
	if (bPreviewCooldownHeld)
	{
		bIsOnCooldown = true;
		ApplyCooldownFraction(PreviewCooldownFraction);
		if (CooldownText)
		{
			CooldownText->SetText(FText::FromString(TEXT("2.4")));
		}
		return;
	}

	if (bIsOnCooldown && GetWorld())
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		const float Elapsed = CurrentTime - CooldownStartTime;
		const float Remaining = FMath::Max(0.f, TotalCooldownDuration - Elapsed);
		const float Percent = CalcCooldownPercent(Elapsed, TotalCooldownDuration);

		if (CooldownProgressBar)
		{
			ApplyCooldownFraction(Percent);
		}
		if (CooldownText)
		{
			CooldownText->SetText(FText::FromString(FormatRemainingTime(Remaining)));
		}

		// Auto-clear if cooldown should have expired
		if (Remaining <= 0.f)
		{
			bIsOnCooldown = false;
		}
	}
	else
	{
		// Ready state
		if (CooldownProgressBar)
		{
			ApplyCooldownFraction(0.f);
		}
		if (CooldownText)
		{
			CooldownText->SetText(FText::GetEmpty());
		}
	}
}

void UMordecaiSpellCooldownWidget::ApplyCooldownFraction(float Fraction)
{
	const float Clamped = FMath::Clamp(Fraction, 0.f, 1.f);
	if (CooldownProgressBar)
	{
		CooldownProgressBar->SetPercent(Clamped);
	}
	if (SpellIcon)
	{
		FLinearColor Tint = SpellIcon->GetColorAndOpacity();
		Tint.A = Clamped > 0.f ? 0.4f : 1.f;
		SpellIcon->SetColorAndOpacity(Tint);
	}
	if (CooldownShutterLip)
	{
		CooldownShutterLip->SetVisibility(
			Clamped > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* LipSlot = Cast<UCanvasPanelSlot>(CooldownShutterLip->Slot))
		{
			LipSlot->SetPosition(FVector2D(7.f, 7.f + 64.f * Clamped - 3.f));
		}
	}
}

void UMordecaiSpellCooldownWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	if (UOverlay* Plate = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("SpellSlotPlate")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Plate,
			FMordecaiUIPrimitiveBuilder::Plate(
				Colors, EMordecaiUIPrintDepth::HUD, Theme));
	}
	if (UOverlay* Well = WidgetTree
		? Cast<UOverlay>(WidgetTree->FindWidget(TEXT("SpellSlotWell")))
		: nullptr)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			Well, FMordecaiUIPrimitiveBuilder::Well(Colors, false));
	}
	if (SlotKey)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			SlotKey,
			FMordecaiUIPrimitiveBuilder::Keycap(Colors, EMordecaiKeycapShape::Square));
	}
	if (LockChip)
	{
		FMordecaiUIPrimitiveBuilder::Apply(
			LockChip, FMordecaiUIPrimitiveBuilder::Pill(Colors));
	}
	if (ShutterMaterial)
	{
		ShutterMaterial->SetVectorParameterValue(TEXT("Base"), Colors.PlateRaised);
		ShutterMaterial->SetVectorParameterValue(TEXT("Line"), Colors.Well);
	}
	if (CooldownShutterLip)
	{
		CooldownShutterLip->SetBrushColor(Colors.Rim);
	}
	if (SpellIcon)
	{
		FSlateBrush Brush = SpellIcon->GetBrush();
		Brush.TintColor = FSlateColor(
			ResolveSpellGlyphTint(ConfiguredSpellGlyph, Colors));
		SpellIcon->SetBrush(Brush);
	}
	MordecaiUITypography::Apply(CooldownText, TEXT("num_m"));
	if (CooldownText)
	{
		FSlateFontInfo Font = CooldownText->GetFont();
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = Colors.Ink.ToFColorSRGB();
		CooldownText->SetFont(Font);
		CooldownText->SetColorAndOpacity(FSlateColor(Colors.Text));
	}
	MordecaiUITypography::Apply(SlotKeyText, TEXT("num_s"));
	if (SlotKeyText)
	{
		FSlateFontInfo Font = SlotKeyText->GetFont();
		Font.TypefaceFontName = TEXT("ExtraBold");
		SlotKeyText->SetFont(Font);
		SlotKeyText->SetColorAndOpacity(FSlateColor(Colors.Ink));
	}
	for (int32 Index = 0; CostRivets && Index < CostRivets->GetChildrenCount(); ++Index)
	{
		if (UImage* Rivet = Cast<UImage>(CostRivets->GetChildAt(Index)))
		{
			FSlateBrush Brush = Rivet->GetBrush();
			Brush.TintColor = FSlateColor(Colors.SpellPoints);
			Brush.OutlineSettings.Color = FSlateColor(Colors.Ink);
			Rivet->SetBrush(Brush);
		}
	}
}
