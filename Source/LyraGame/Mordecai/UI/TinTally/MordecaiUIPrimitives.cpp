// Project Mordecai — shared Tin Tally RoundedBox primitives (US-102)

#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiUIPrimitives)

namespace MordecaiUIPrimitives
{
	FSlateBrush Rounded(
		const FLinearColor& Fill,
		const FVector4& Radius,
		const FLinearColor& Outline = FLinearColor::Transparent,
		float OutlineWidth = 0.f,
		bool bHalfHeight = false)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings = bHalfHeight
			? FSlateBrushOutlineSettings(FSlateColor(Outline), OutlineWidth)
			: FSlateBrushOutlineSettings(Radius, FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	FMordecaiUIPrimitiveLayer Layer(
		FName Name,
		const FSlateBrush& Brush,
		const FMargin& Padding = FMargin(0.f),
		const FVector2D& Translation = FVector2D::ZeroVector)
	{
		FMordecaiUIPrimitiveLayer Result;
		Result.Name = Name;
		Result.Brush = Brush;
		Result.Padding = Padding;
		Result.Translation = Translation;
		return Result;
	}

	float PrintOffset(EMordecaiUIPrintDepth Depth)
	{
		return Depth == EMordecaiUIPrintDepth::Menu ? 6.f : 4.f;
	}

	FLinearColor PrintColor(const FMordecaiUIThemeColors& Colors)
	{
		FLinearColor Result = Colors.Ink;
		Result.A *= 0.55f;
		return Result;
	}

	void MakeHalfHeight(FMordecaiUIPrimitiveSpec& Spec)
	{
		for (FMordecaiUIPrimitiveLayer& PrimitiveLayer : Spec.Layers)
		{
			PrimitiveLayer.Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			PrimitiveLayer.Brush.OutlineSettings.CornerRadii = FVector4::Zero();
		}
	}
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Plate(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth)
{
	using namespace MordecaiUIPrimitives;
	FMordecaiUIPrimitiveSpec Spec;
	Spec.OuterOutlineWidth = 3.f;
	Spec.InnerStrokeWidth = 2.f;
	Spec.Layers = {
		Layer(TEXT("Print"), Rounded(PrintColor(Colors), FVector4(15.f)), FMargin(0.f),
			FVector2D(0.f, PrintOffset(PrintDepth))),
		Layer(TEXT("Outline"), Rounded(Colors.Ink, FVector4(15.f))),
		Layer(TEXT("RimShade"), Rounded(Colors.RimShade, FVector4(12.f)), FMargin(3.f)),
		Layer(TEXT("Rim"), Rounded(Colors.Rim, FVector4(12.f)), FMargin(3.f, 3.f, 5.f, 5.f)),
		Layer(TEXT("Fill"), Rounded(Colors.Plate, FVector4(10.f)), FMargin(5.f))
	};
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Pill(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth)
{
	FMordecaiUIPrimitiveSpec Spec = Plate(Colors, PrintDepth);
	MordecaiUIPrimitives::MakeHalfHeight(Spec);
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Tag(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth)
{
	using namespace MordecaiUIPrimitives;
	const FVector4 OuterRadius(7.f, 13.f, 13.f, 7.f);
	const FVector4 FaceRadius(4.f, 10.f, 10.f, 4.f);
	FMordecaiUIPrimitiveSpec Spec;
	Spec.OuterOutlineWidth = 3.f;
	Spec.Layers = {
		Layer(TEXT("Print"), Rounded(PrintColor(Colors), OuterRadius), FMargin(0.f),
			FVector2D(0.f, PrintOffset(PrintDepth))),
		Layer(TEXT("Outline"), Rounded(Colors.Ink, OuterRadius)),
		Layer(TEXT("Face"), Rounded(Colors.Enamel, FaceRadius), FMargin(3.f))
	};

	auto AddGrommet = [&Spec](FName Name, float Size, const FLinearColor& Color)
	{
		FMordecaiUIPrimitiveLayer Grommet =
			Layer(Name, Rounded(Color, FVector4::Zero(), FLinearColor::Transparent, 0.f, true));
		Grommet.DesiredSize = FVector2D(Size);
		Grommet.Center = FVector2D(22.f, 11.f);
		Spec.Layers.Add(Grommet);
	};
	AddGrommet(TEXT("GrommetInk"), 22.f, Colors.Ink);
	AddGrommet(TEXT("GrommetShade"), 18.f, Colors.EnamelShade);
	AddGrommet(TEXT("GrommetWell"), 12.f, Colors.Well);
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Well(
	const FMordecaiUIThemeColors& Colors,
	bool bBar)
{
	using namespace MordecaiUIPrimitives;
	FMordecaiUIPrimitiveSpec Spec;
	Spec.InnerStrokeWidth = 2.f;
	const float Radius = bBar ? 6.f : 8.f;
	Spec.Layers = {
		Layer(TEXT("Well"), Rounded(Colors.Well, FVector4(Radius), Colors.Lip, 2.f))
	};
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Keycap(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiKeycapShape Shape)
{
	using namespace MordecaiUIPrimitives;
	const bool bHalfHeight = Shape != EMordecaiKeycapShape::Square;
	const FVector4 Radius(8.f);
	FMordecaiUIPrimitiveSpec Spec;
	Spec.OuterOutlineWidth = 2.f;
	Spec.Layers = {
		Layer(TEXT("Lip"), Rounded(Colors.EnamelShade, Radius, Colors.Ink, 2.f, bHalfHeight),
			FMargin(0.f), FVector2D(0.f, 4.f)),
		Layer(TEXT("Face"), Rounded(Colors.Enamel, Radius, Colors.Ink, 2.f, bHalfHeight))
	};
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Focus(
	const FMordecaiUIThemeColors& Colors,
	float PieceRadius,
	EMordecaiUITheme Theme)
{
	using namespace MordecaiUIPrimitives;
	FMordecaiUIPrimitiveSpec Spec;
	Spec.FocusStrokeWidth = 3.f;
	Spec.FocusKeylineWidth = 2.f;
	Spec.FocusOffset = Theme == EMordecaiUITheme::HighContrast ? 4.f : 3.f;
	const float RingRadius = PieceRadius + Spec.FocusOffset;
	const float KeylineRadius = RingRadius + Spec.FocusStrokeWidth;
	Spec.Layers = {
		Layer(TEXT("Keyline"), Rounded(FLinearColor::Transparent, FVector4(KeylineRadius), Colors.Ink, 2.f)),
		Layer(TEXT("Focus"), Rounded(FLinearColor::Transparent, FVector4(RingRadius), Colors.Focus, 3.f),
			FMargin(Spec.FocusKeylineWidth))
	};
	return Spec;
}

UOverlay* FMordecaiUIPrimitiveBuilder::Build(
	UWidgetTree* WidgetTree,
	const FMordecaiUIPrimitiveSpec& Spec,
	FName RootName)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), RootName);
	for (const FMordecaiUIPrimitiveLayer& PrimitiveLayer : Spec.Layers)
	{
		const FName LayerName(*FString::Printf(TEXT("%s_%s"), *RootName.ToString(), *PrimitiveLayer.Name.ToString()));
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), LayerName);
		Border->SetBrush(PrimitiveLayer.Brush);
		Border->SetRenderTranslation(PrimitiveLayer.Translation);
		Border->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (!PrimitiveLayer.DesiredSize.IsNearlyZero())
		{
			const FName BoxName(*FString::Printf(TEXT("%s_Box"), *LayerName.ToString()));
			USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), BoxName);
			SizeBox->SetWidthOverride(PrimitiveLayer.DesiredSize.X);
			SizeBox->SetHeightOverride(PrimitiveLayer.DesiredSize.Y);
			SizeBox->AddChild(Border);
			if (UOverlaySlot* Slot = Root->AddChildToOverlay(SizeBox))
			{
				Slot->SetHorizontalAlignment(HAlign_Left);
				Slot->SetVerticalAlignment(VAlign_Top);
				Slot->SetPadding(FMargin(
					PrimitiveLayer.Center.X - PrimitiveLayer.DesiredSize.X * 0.5f,
					PrimitiveLayer.Center.Y - PrimitiveLayer.DesiredSize.Y * 0.5f,
					0.f,
					0.f));
			}
		}
		else if (UOverlaySlot* Slot = Root->AddChildToOverlay(Border))
		{
			Slot->SetPadding(PrimitiveLayer.Padding);
		}
	}
	return Root;
}
