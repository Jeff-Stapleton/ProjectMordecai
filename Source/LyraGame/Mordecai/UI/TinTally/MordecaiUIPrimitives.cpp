// Project Mordecai — shared Tin Tally RoundedBox primitives (US-102)

#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiUIPrimitives)

namespace
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

	FLinearColor PrintColor(const FMordecaiUIThemeColors& Colors, EMordecaiUITheme Theme)
	{
		FLinearColor Result = Colors.Ink;
		Result.A = Theme == EMordecaiUITheme::HighContrast ? 1.f : 0.55f;
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
	return Plate(Colors, PrintDepth, EMordecaiUITheme::Diorama);
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Plate(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth,
	EMordecaiUITheme Theme)
{
	const bool bHighContrast = Theme == EMordecaiUITheme::HighContrast;
	const float GeometryShift = bHighContrast ? 1.f : 0.f;
	FMordecaiUIPrimitiveSpec Spec;
	Spec.OuterOutlineWidth = bHighContrast ? 4.f : 3.f;
	Spec.InnerStrokeWidth = 2.f;
	Spec.Layers = {
		Layer(TEXT("Print"), Rounded(PrintColor(Colors, Theme), FVector4(15.f + GeometryShift)), FMargin(0.f),
			FVector2D(0.f, PrintOffset(PrintDepth))),
		Layer(TEXT("Outline"), Rounded(Colors.Ink, FVector4(15.f + GeometryShift))),
		Layer(TEXT("RimShade"), Rounded(Colors.RimShade, FVector4(12.f)), FMargin(3.f + GeometryShift)),
		Layer(TEXT("Rim"), Rounded(Colors.Rim, FVector4(12.f)),
			FMargin(3.f + GeometryShift, 3.f + GeometryShift, 5.f + GeometryShift, 5.f + GeometryShift)),
		Layer(TEXT("Fill"), Rounded(Colors.Plate, FVector4(10.f)), FMargin(5.f + GeometryShift))
	};
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Pill(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth)
{
	return Pill(Colors, PrintDepth, EMordecaiUITheme::Diorama);
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Pill(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth,
	EMordecaiUITheme Theme)
{
	FMordecaiUIPrimitiveSpec Spec = Plate(Colors, PrintDepth, Theme);
	MakeHalfHeight(Spec);
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Tag(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth)
{
	return Tag(Colors, PrintDepth, EMordecaiUITheme::Diorama);
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Tag(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiUIPrintDepth PrintDepth,
	EMordecaiUITheme Theme)
{
	const bool bHighContrast = Theme == EMordecaiUITheme::HighContrast;
	const float GeometryShift = bHighContrast ? 1.f : 0.f;
	const FVector4 OuterRadius(
		7.f + GeometryShift,
		13.f + GeometryShift,
		13.f + GeometryShift,
		7.f + GeometryShift);
	const FVector4 FaceRadius(4.f, 10.f, 10.f, 4.f);
	FMordecaiUIPrimitiveSpec Spec;
	Spec.OuterOutlineWidth = bHighContrast ? 4.f : 3.f;
	Spec.Layers = {
		Layer(TEXT("Print"), Rounded(PrintColor(Colors, Theme), OuterRadius), FMargin(0.f),
			FVector2D(0.f, PrintOffset(PrintDepth))),
		Layer(TEXT("Outline"), Rounded(Colors.Ink, OuterRadius)),
		Layer(TEXT("Face"), Rounded(Colors.Enamel, FaceRadius), FMargin(3.f + GeometryShift))
	};

	auto AddGrommet = [&Spec](FName Name, float Size, const FLinearColor& Color)
	{
		FMordecaiUIPrimitiveLayer Grommet =
			Layer(Name, Rounded(Color, FVector4::Zero(), FLinearColor::Transparent, 0.f, true));
		Grommet.DesiredSize = FVector2D(Size);
		Grommet.Center = FVector2D(22.f, 0.f);
		Grommet.HorizontalAlignment = HAlign_Left;
		Grommet.VerticalAlignment = VAlign_Center;
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
	FMordecaiUIPrimitiveSpec Spec;
	Spec.InnerStrokeWidth = 2.f;
	const float Radius = bBar ? 6.f : 8.f;
	Spec.Layers = {
		Layer(TEXT("Well"), Rounded(Colors.Well, FVector4(Radius), Colors.Lip, 2.f))
	};
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::InsetCardWell(
	const FMordecaiUIThemeColors& Colors)
{
	FMordecaiUIPrimitiveSpec Spec;
	Spec.InnerStrokeWidth = 2.f;
	Spec.Layers = {
		Layer(TEXT("InsetCard"), Rounded(Colors.PlateRaised, FVector4(12.f), Colors.Lip, 2.f))
	};
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Keycap(
	const FMordecaiUIThemeColors& Colors,
	EMordecaiKeycapShape Shape)
{
	const bool bHalfHeight = Shape != EMordecaiKeycapShape::Square;
	const FVector4 Radius(8.f);
	FMordecaiUIPrimitiveSpec Spec;
	Spec.OuterOutlineWidth = 2.f;
	Spec.Layers = {
		Layer(TEXT("Face"), Rounded(Colors.Enamel, Radius, Colors.Ink, 2.f, bHalfHeight))
	};
	FMordecaiUIPrimitiveLayer Lip =
		Layer(TEXT("Lip"), Rounded(Colors.EnamelShade, FVector4(2.f)));
	Lip.Padding = FMargin(2.f, 0.f, 2.f, 2.f);
	Lip.DesiredSize = FVector2D(0.f, 4.f);
	Lip.HorizontalAlignment = HAlign_Fill;
	Lip.VerticalAlignment = VAlign_Bottom;
	Spec.Layers.Add(Lip);
	return Spec;
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Focus(
	const FMordecaiUIThemeColors& Colors,
	float PieceRadius,
	EMordecaiUITheme Theme)
{
	return Focus(Colors, PieceRadius, Theme, EMordecaiUIFocusVariant::Default);
}

FMordecaiUIPrimitiveSpec FMordecaiUIPrimitiveBuilder::Focus(
	const FMordecaiUIThemeColors& Colors,
	float PieceRadius,
	EMordecaiUITheme Theme,
	EMordecaiUIFocusVariant Variant)
{
	FMordecaiUIPrimitiveSpec Spec;
	Spec.FocusStrokeWidth = 3.f;
	Spec.FocusKeylineWidth = 2.f;
	const float PieceOutline = Theme == EMordecaiUITheme::HighContrast ? 4.f : 3.f;
	Spec.FocusOffset = Variant == EMordecaiUIFocusVariant::Border ? 0.f : PieceOutline;
	const float RingRadius = PieceRadius + Spec.FocusOffset + Spec.FocusStrokeWidth;
	const float KeylineRadius = RingRadius + Spec.FocusKeylineWidth;

	if (Variant == EMordecaiUIFocusVariant::FillGap)
	{
		Spec.Layers = {
			Layer(TEXT("Gap"), Rounded(FLinearColor::Transparent,
				FVector4(PieceRadius + Spec.FocusOffset), Colors.Ink, PieceOutline)),
			Layer(TEXT("Focus"), Rounded(FLinearColor::Transparent,
				FVector4(RingRadius), Colors.Focus, Spec.FocusStrokeWidth))
		};
	}
	else
	{
		Spec.Layers = {
			Layer(TEXT("Keyline"), Rounded(FLinearColor::Transparent,
				FVector4(KeylineRadius), Colors.Ink, Spec.FocusKeylineWidth)),
			Layer(TEXT("Focus"), Rounded(FLinearColor::Transparent,
				FVector4(RingRadius), Colors.Focus, Spec.FocusStrokeWidth))
		};
	}
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
		Border->SetPadding(FMargin(0.f));
		Border->SetRenderTranslation(PrimitiveLayer.Translation);
		Border->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (!PrimitiveLayer.DesiredSize.IsNearlyZero())
		{
			const FName BoxName(*FString::Printf(TEXT("%s_Box"), *LayerName.ToString()));
			USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), BoxName);
			if (PrimitiveLayer.DesiredSize.X > 0.f)
			{
				SizeBox->SetWidthOverride(PrimitiveLayer.DesiredSize.X);
			}
			if (PrimitiveLayer.DesiredSize.Y > 0.f)
			{
				SizeBox->SetHeightOverride(PrimitiveLayer.DesiredSize.Y);
			}
			SizeBox->SetVisibility(ESlateVisibility::HitTestInvisible);
			SizeBox->AddChild(Border);
			if (UOverlaySlot* Slot = Root->AddChildToOverlay(SizeBox))
			{
				Slot->SetHorizontalAlignment(PrimitiveLayer.HorizontalAlignment);
				Slot->SetVerticalAlignment(PrimitiveLayer.VerticalAlignment);
				FMargin SlotPadding = PrimitiveLayer.Padding;
				if (PrimitiveLayer.HorizontalAlignment == HAlign_Left && PrimitiveLayer.DesiredSize.X > 0.f)
				{
					SlotPadding.Left += PrimitiveLayer.Center.X - PrimitiveLayer.DesiredSize.X * 0.5f;
				}
				if (PrimitiveLayer.VerticalAlignment == VAlign_Top && PrimitiveLayer.DesiredSize.Y > 0.f)
				{
					SlotPadding.Top += PrimitiveLayer.Center.Y - PrimitiveLayer.DesiredSize.Y * 0.5f;
				}
				Slot->SetPadding(SlotPadding);
			}
		}
		else if (UOverlaySlot* Slot = Root->AddChildToOverlay(Border))
		{
			Slot->SetHorizontalAlignment(PrimitiveLayer.HorizontalAlignment);
			Slot->SetVerticalAlignment(PrimitiveLayer.VerticalAlignment);
			Slot->SetPadding(PrimitiveLayer.Padding);
		}
	}
	return Root;
}

void FMordecaiUIPrimitiveBuilder::Apply(
	UOverlay* Overlay,
	const FMordecaiUIPrimitiveSpec& Spec)
{
	// RED3 scaffold: behavior is implemented after the contract test fails.
	(void)Overlay;
	(void)Spec;
}
