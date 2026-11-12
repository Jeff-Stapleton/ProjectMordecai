// Project Mordecai — Tin Tally bundle widget (US-102)

#include "Mordecai/UI/TinTally/MordecaiTallyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiTallyWidget)

UMordecaiTallyWidget::UMordecaiTallyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BundleFrames.SetNum(6);
	GhostBundleFrames.SetNum(6);
	for (FSlateBrush& Frame : BundleFrames)
	{
		Frame.DrawAs = ESlateBrushDrawType::NoDrawType;
	}
	for (FSlateBrush& Frame : GhostBundleFrames)
	{
		Frame.DrawAs = ESlateBrushDrawType::NoDrawType;
	}
	CapDotBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	CurrentThemeColors = FMordecaiUIThemeColors::MakeDiorama();
	CurrentTint = CurrentThemeColors.Text;
	CurrentGhostTint = CurrentThemeColors.Lip;
}

bool UMordecaiTallyWidget::Initialize()
{
	const bool bInitializedNow = Super::Initialize();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	ClampValues();
	Refresh();
	return bInitializedNow;
}

void UMordecaiTallyWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ClampValues();
	Refresh();
}

#if WITH_EDITOR
void UMordecaiTallyWidget::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ClampValues();
	Refresh();
}
#endif

void UMordecaiTallyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BoundThemeSubsystem = UMordecaiUIThemeSubsystem::ResolveForWidget(this);
	if (UMordecaiUIThemeSubsystem* ThemeSubsystem = BoundThemeSubsystem.Get())
	{
		if (!ThemeChangedHandle.IsValid())
		{
			ThemeChangedHandle = ThemeSubsystem->OnThemeChangedNative().AddUObject(
				this, &ThisClass::ApplyTheme);
		}
		ApplyTheme(ThemeSubsystem->GetActiveTheme(), ThemeSubsystem->GetActiveColors());
	}
}

void UMordecaiTallyWidget::NativeDestruct()
{
	if (UMordecaiUIThemeSubsystem* ThemeSubsystem = BoundThemeSubsystem.Get();
		ThemeSubsystem && ThemeChangedHandle.IsValid())
	{
		ThemeSubsystem->OnThemeChangedNative().Remove(ThemeChangedHandle);
	}
	ThemeChangedHandle.Reset();
	BoundThemeSubsystem.Reset();

	Super::NativeDestruct();
}

void UMordecaiTallyWidget::SetCount(int32 NewCount)
{
	Count = FMath::Clamp(NewCount, 0, Max);
	Refresh();
}

void UMordecaiTallyWidget::SetMax(int32 NewMax)
{
	Max = NewMax;
	ClampValues();
	Refresh();
}

void UMordecaiTallyWidget::SetCountAndMax(int32 NewCount, int32 NewMax)
{
	Max = NewMax;
	Count = NewCount;
	ClampValues();
	Refresh();
}

void UMordecaiTallyWidget::SetUsage(EMordecaiTallyUsage NewUsage)
{
	if (IsAllowedUsage(NewUsage))
	{
		Usage = NewUsage;
	}
	ApplyTheme(CurrentTheme, CurrentThemeColors);
	Refresh();
}

void UMordecaiTallyWidget::SetRarityTier(EMordecaiItemRarity NewRarity)
{
	Rarity = NewRarity;
	ApplyTheme(CurrentTheme, CurrentThemeColors);
	Refresh();
}

void UMordecaiTallyWidget::SetSealOnEnamel(bool bNewSealOnEnamel)
{
	bSealOnEnamel = bNewSealOnEnamel;
	ApplyTheme(CurrentTheme, CurrentThemeColors);
	Refresh();
}

void UMordecaiTallyWidget::SetFrameBrushes(
	const TArray<FSlateBrush>& NewBundleFrames,
	const TArray<FSlateBrush>& NewGhostBundleFrames,
	const FSlateBrush& NewCapDotBrush)
{
	if (NewBundleFrames.Num() == 6)
	{
		BundleFrames = NewBundleFrames;
	}
	if (NewGhostBundleFrames.Num() == 6)
	{
		GhostBundleFrames = NewGhostBundleFrames;
	}
	CapDotBrush = NewCapDotBrush;
	Refresh();
}

TArray<int32> UMordecaiTallyWidget::GetRenderedBundleFrames() const
{
	const int32 BundleCount = FMath::Max(1, FMath::DivideAndRoundUp(Max, 5));
	TArray<int32> Frames;
	Frames.Reserve(BundleCount);
	int32 Remaining = FMath::Clamp(Count, 0, Max);
	for (int32 BundleIndex = 0; BundleIndex < BundleCount; ++BundleIndex)
	{
		const int32 BundleCapacity = FMath::Clamp(Max - BundleIndex * 5, 0, 5);
		const int32 Frame = FMath::Clamp(Remaining, 0, BundleCapacity);
		Frames.Add(Frame);
		Remaining -= Frame;
	}
	return Frames;
}

bool UMordecaiTallyWidget::IsAllowedUsage(EMordecaiTallyUsage Candidate)
{
	switch (Candidate)
	{
	case EMordecaiTallyUsage::SkillRank:
	case EMordecaiTallyUsage::RarityBadge:
	case EMordecaiTallyUsage::Seal:
	case EMordecaiTallyUsage::LoadingThrobber:
		return true;
	default:
		return false;
	}
}

void UMordecaiTallyWidget::BuildDefaultLayout()
{
	BundleBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BundleBox"));
	WidgetTree->RootWidget = BundleBox;
}

void UMordecaiTallyWidget::ClampValues()
{
	Max = FMath::Clamp(Max, 0, MaxSupportedCount);
	Count = FMath::Clamp(Count, 0, Max);
}

void UMordecaiTallyWidget::Refresh()
{
	if (!BundleBox || !WidgetTree)
	{
		return;
	}

	if (BundleFrames.Num() != 6)
	{
		BundleFrames.SetNum(6);
	}
	if (GhostBundleFrames.Num() != 6)
	{
		GhostBundleFrames.SetNum(6);
	}

	const TArray<int32> Frames = GetRenderedBundleFrames();
	if (RenderedBundleImages.Num() != Frames.Num())
	{
		BundleBox->ClearChildren();
		RenderedBundleImages.Reset(Frames.Num());
		RenderedGhostImages.Reset(Frames.Num());
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			UOverlay* Bundle = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(), *FString::Printf(TEXT("TallyBundle_%d"), Index));
			UImage* GhostImage = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(), *FString::Printf(TEXT("TallyGhost_%d"), Index));
			UImage* FillImage = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(), *FString::Printf(TEXT("TallyFill_%d"), Index));
			GhostImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			FillImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			Bundle->AddChildToOverlay(GhostImage);
			Bundle->AddChildToOverlay(FillImage);
			BundleBox->AddChildToHorizontalBox(Bundle);
			RenderedGhostImages.Add(GhostImage);
			RenderedBundleImages.Add(FillImage);
		}
	}

	for (int32 Index = 0; Index < Frames.Num(); ++Index)
	{
		const int32 Capacity = FMath::Clamp(Max - Index * 5, 0, 5);
		RenderedGhostImages[Index]->SetBrush(GhostBundleFrames[Capacity]);
		RenderedGhostImages[Index]->SetColorAndOpacity(CurrentGhostTint);
		RenderedBundleImages[Index]->SetBrush(BundleFrames[Frames[Index]]);
		RenderedBundleImages[Index]->SetColorAndOpacity(CurrentTint);
	}
}

void UMordecaiTallyWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	CurrentTheme = Theme;
	CurrentThemeColors = Colors;
	switch (Usage)
	{
	case EMordecaiTallyUsage::RarityBadge:
		switch (Rarity)
		{
		case EMordecaiItemRarity::Green:
			CurrentTint = Colors.RarityGreen;
			break;
		case EMordecaiItemRarity::Blue:
			CurrentTint = Colors.RarityBlue;
			break;
		case EMordecaiItemRarity::Purple:
			CurrentTint = Colors.RarityPurple;
			break;
		case EMordecaiItemRarity::Red:
			CurrentTint = Colors.RarityRed;
			break;
		case EMordecaiItemRarity::Gold:
			CurrentTint = Colors.RarityGold;
			break;
		case EMordecaiItemRarity::Common:
		default:
			CurrentTint = Colors.RarityCommon;
			break;
		}
		break;
	case EMordecaiTallyUsage::LoadingThrobber:
		CurrentTint = Colors.Text;
		break;
	case EMordecaiTallyUsage::Seal:
		CurrentTint = bSealOnEnamel ? Colors.Ink : Colors.Rim;
		break;
	case EMordecaiTallyUsage::SkillRank:
	default:
		CurrentTint = Colors.Text;
		break;
	}
	CurrentGhostTint = Colors.Lip;
	for (UImage* GhostImage : RenderedGhostImages)
	{
		if (GhostImage)
		{
			GhostImage->SetColorAndOpacity(CurrentGhostTint);
		}
	}
	for (UImage* BundleImage : RenderedBundleImages)
	{
		if (BundleImage)
		{
			BundleImage->SetColorAndOpacity(CurrentTint);
		}
	}
}
