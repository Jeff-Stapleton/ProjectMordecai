// Project Mordecai — Tin Tally bundle widget (US-102)

#include "Mordecai/UI/TinTally/MordecaiTallyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiTallyWidget)

UMordecaiTallyWidget::UMordecaiTallyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BundleFrames.SetNum(6);
	for (FSlateBrush& Frame : BundleFrames)
	{
		Frame.DrawAs = ESlateBrushDrawType::NoDrawType;
	}
	CapDotBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	CurrentTint = FMordecaiUIThemeColors::MakeDiorama().Text;
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
	// TODO(US-103): render separate lit-fill and dotted-ghost atlas layers;
	// tint the fill per Usage and tint the ghost with the theme's Lip token.
	if (!BundleBox || !WidgetTree)
	{
		return;
	}

	if (BundleFrames.Num() != 6)
	{
		BundleFrames.SetNum(6);
	}

	const TArray<int32> Frames = GetRenderedBundleFrames();
	if (RenderedBundleImages.Num() != Frames.Num())
	{
		BundleBox->ClearChildren();
		RenderedBundleImages.Reset(Frames.Num());
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			UImage* BundleImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), NAME_None);
			BundleImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			BundleBox->AddChildToHorizontalBox(BundleImage);
			RenderedBundleImages.Add(BundleImage);
		}
	}

	for (int32 Index = 0; Index < Frames.Num(); ++Index)
	{
		RenderedBundleImages[Index]->SetBrush(BundleFrames[Frames[Index]]);
		RenderedBundleImages[Index]->SetColorAndOpacity(CurrentTint);
	}
}

void UMordecaiTallyWidget::ApplyTheme(
	EMordecaiUITheme Theme,
	const FMordecaiUIThemeColors& Colors)
{
	(void)Theme;
	CurrentTint = Colors.Text;
	for (UImage* BundleImage : RenderedBundleImages)
	{
		if (BundleImage)
		{
			BundleImage->SetColorAndOpacity(CurrentTint);
		}
	}
}
