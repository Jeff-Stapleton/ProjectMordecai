// Project Mordecai — packaged Tin Tally visual acceptance gallery (US-103)

#include "Mordecai/UI/TinTally/MordecaiTinTallyGallerySubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Mordecai/UI/TinTally/MordecaiTallyWidget.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
#include "Mordecai/UI/TinTally/MordecaiUIThemeSubsystem.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiTinTallyGallerySubsystem)

namespace
{
	FString GalleryValue(const TCHAR* Key, const TCHAR* DefaultValue)
	{
		FString Value;
		return FParse::Value(FCommandLine::Get(), Key, Value) ? Value : FString(DefaultValue);
	}

	UCanvasPanelSlot* Place(
		UCanvasPanel* Canvas,
		UWidget* Widget,
		float X,
		float Y,
		float Width,
		float Height)
	{
		if (UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget))
		{
			Slot->SetPosition(FVector2D(X, Y));
			Slot->SetSize(FVector2D(Width, Height));
			return Slot;
		}
		return nullptr;
	}

	UTextBlock* AddText(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FString& Value,
		const TCHAR* StyleName,
		const FLinearColor& Color,
		float X,
		float Y,
		float Width,
		float Height,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetJustification(Justification);
		Text->SetAutoWrapText(false);
		const FString ClassPath = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Fonts/Styles/TS_Tally_%s.TS_Tally_%s_C"),
			StyleName,
			StyleName);
		if (UClass* StyleClass = LoadClass<UCommonTextStyle>(nullptr, *ClassPath))
		{
			const UCommonTextStyle* Style = GetDefault<UCommonTextStyle>(StyleClass);
			FSlateFontInfo Font;
			Style->GetFont(Font);
			Text->SetFont(Font);
			Text->SetLineHeightPercentage(Style->GetLineHeightPercentage());
		}
		Text->SetColorAndOpacity(FSlateColor(Color));
		Place(Canvas, Text, X, Y, Width, Height);
		return Text;
	}

	FSlateBrush RoundedBrush(
		const FLinearColor& Fill,
		float Radius,
		const FLinearColor& Outline = FLinearColor::Transparent,
		float OutlineWidth = 0.f)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings =
			FSlateBrushOutlineSettings(FVector4(Radius), FSlateColor(Outline), OutlineWidth);
		return Brush;
	}

	UBorder* AddBox(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FLinearColor& Fill,
		float X,
		float Y,
		float Width,
		float Height,
		float Radius = 0.f,
		const FLinearColor& Outline = FLinearColor::Transparent,
		float OutlineWidth = 0.f)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>();
		Border->SetBrush(RoundedBrush(Fill, Radius, Outline, OutlineWidth));
		Border->SetPadding(FMargin(0.f));
		Border->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Border, X, Y, Width, Height);
		return Border;
	}

	void AddMarker(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		int32 Number,
		float X,
		float Y,
		bool bLightGround = true)
	{
		const FLinearColor Fill = bLightGround ? Colors.Enamel : Colors.Plate;
		const FLinearColor TextColor = bLightGround ? Colors.Ink : Colors.Text;
		AddBox(Tree, Canvas, Fill, X, Y, 30.f, 30.f, 15.f, Colors.Ink, 3.f);
		AddText(
			Tree,
			Canvas,
			FString::FromInt(Number),
			TEXT("num_s"),
			TextColor,
			X,
			Y + 3.f,
			30.f,
			24.f,
			ETextJustify::Center);
	}

	UOverlay* AddPrimitive(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIPrimitiveSpec& Spec,
		const FString& Name,
		float X,
		float Y,
		float Width,
		float Height)
	{
		UOverlay* Primitive = FMordecaiUIPrimitiveBuilder::Build(Tree, Spec, FName(*Name));
		Place(Canvas, Primitive, X, Y, Width, Height);
		return Primitive;
	}

	void AddKeycap(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		const FString& Name,
		const FString& Label,
		float X,
		float Y,
		float Width = 40.f,
		float Height = 40.f,
		EMordecaiKeycapShape Shape = EMordecaiKeycapShape::Square)
	{
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Keycap(Colors, Shape),
			Name,
			X,
			Y,
			Width,
			Height);
		if (!Label.IsEmpty())
		{
			UTextBlock* LabelText = AddText(
				Tree,
				Canvas,
				Label,
				TEXT("num_s"),
				Colors.Ink,
				X,
				Y + 7.f,
				Width,
				24.f,
				ETextJustify::Center);
			FSlateFontInfo Font = LabelText->GetFont();
			Font.TypefaceFontName = TEXT("ExtraBold");
			Font.Size = Height <= 32.f ? 15 : 17;
			LabelText->SetFont(Font);
		}
	}

	void AddMouseCap(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		const FString& Name,
		float X,
		float Y,
		int32 LitSection)
	{
		AddKeycap(
			Tree,
			Canvas,
			Colors,
			Name,
			TEXT(""),
			X,
			Y,
			40.f,
			40.f,
			EMordecaiKeycapShape::Square);
		const float MouseX = X + 10.f;
		const float MouseY = Y + 4.f;
		AddBox(Tree, Canvas, FLinearColor::Transparent, MouseX, MouseY, 20.f, 27.f, 10.f, Colors.Ink, 2.f);
		AddBox(Tree, Canvas, Colors.Ink, MouseX, MouseY + 10.f, 20.f, 2.f);
		AddBox(Tree, Canvas, Colors.Ink, MouseX + 9.f, MouseY, 2.f, 11.f);
		if (LitSection == 0)
		{
			AddBox(Tree, Canvas, Colors.Ink, MouseX + 2.f, MouseY + 2.f, 7.f, 8.f, 6.f);
		}
		else if (LitSection == 2)
		{
			AddBox(Tree, Canvas, Colors.Ink, MouseX + 11.f, MouseY + 2.f, 7.f, 8.f, 6.f);
		}
		else
		{
			AddBox(Tree, Canvas, Colors.Ink, MouseX + 7.f, MouseY + 2.f, 6.f, 8.f, 3.f);
		}
	}

	void AddFacePosition(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		float X,
		float Y)
	{
		AddKeycap(
			Tree, Canvas, Colors, TEXT("FacePosition"), TEXT(""), X, Y, 40.f, 40.f,
			EMordecaiKeycapShape::Round);
		const FVector2D Positions[] = {
			FVector2D(15.f, 5.f), FVector2D(23.f, 13.f),
			FVector2D(15.f, 21.f), FVector2D(7.f, 13.f)
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			AddBox(
				Tree,
				Canvas,
				Index == 2 ? Colors.Ink : FLinearColor::Transparent,
				X + Positions[Index].X,
				Y + Positions[Index].Y,
				10.f,
				10.f,
				5.f,
				Colors.Ink,
				2.f);
		}
	}

	void AddDPad(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		float X,
		float Y,
		float Size)
	{
		const float Arm = Size * 0.38f;
		AddBox(Tree, Canvas, Colors.Ink, X + (Size - Arm) * 0.5f, Y, Arm, Size, 2.f);
		AddBox(Tree, Canvas, Colors.Ink, X, Y + (Size - Arm) * 0.5f, Size, Arm, 2.f);
		AddBox(
			Tree,
			Canvas,
			Colors.Enamel,
			X + (Size - Arm + 4.f) * 0.5f,
			Y + 2.f,
			Arm - 2.f,
			Size - 4.f,
			1.f);
		AddBox(
			Tree,
			Canvas,
			Colors.Enamel,
			X + 2.f,
			Y + (Size - Arm + 4.f) * 0.5f,
			Size - 4.f,
			Arm - 2.f,
			1.f);
		AddBox(
			Tree,
			Canvas,
			Colors.Ink,
			X + Size * 0.40f,
			Y + 3.f,
			Size * 0.20f,
			Size * 0.20f,
			1.f);
	}

	UImage* AddTexture(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		UObject* Resource,
		const FLinearColor& Tint,
		float X,
		float Y,
		float Width,
		float Height)
	{
		UImage* Image = Tree->ConstructWidget<UImage>();
		FSlateBrush Brush;
		Brush.SetResourceObject(Resource);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(Width, Height);
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Image, X, Y, Width, Height);
		return Image;
	}

	UImage* AddBrush(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FSlateBrush& Brush,
		const FLinearColor& Tint,
		float X,
		float Y,
		float Width,
		float Height)
	{
		UImage* Image = Tree->ConstructWidget<UImage>();
		Image->SetBrush(Brush);
		Image->SetColorAndOpacity(Tint);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Image, X, Y, Width, Height);
		return Image;
	}

	UImage* AddGlyph(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FString& GlyphName,
		const FLinearColor& Tint,
		float X,
		float Y,
		float Size)
	{
		const FString AssetName = GlyphName.Replace(TEXT("-"), TEXT("_"));
		const FString Path = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Textures/Glyphs/T_UI_Glyph_%s.T_UI_Glyph_%s"),
			*AssetName,
			*AssetName);
		return AddTexture(
			Tree, Canvas, LoadObject<UTexture2D>(nullptr, *Path), Tint, X, Y, Size, Size);
	}

	void AddPattern(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FLinearColor& Tint,
		float X,
		float Y,
		float Width,
		float Height)
	{
		UObject* Pattern = LoadObject<UObject>(
			nullptr,
			TEXT("/MordecaiCore/UI/TinTally/Materials/M_UI_PatternFill.M_UI_PatternFill"));
		AddTexture(Tree, Canvas, Pattern, Tint, X, Y, Width, Height);
	}

	FSlateBrush TallyAtlasBrush(UTexture2D* Atlas, int32 Frame, const FVector2D& Size)
	{
		FSlateBrush Brush;
		if (Frame <= 0)
		{
			Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
			Brush.ImageSize = Size;
			return Brush;
		}
		Brush.SetResourceObject(Atlas);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = Size;
		Brush.SetUVRegion(FBox2D(
			FVector2D(static_cast<float>(Frame) / 6.f, 0.f),
			FVector2D(static_cast<float>(Frame + 1) / 6.f, 1.f)));
		return Brush;
	}

	void AddAtlasFrame(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		int32 Frame,
		const FLinearColor& Tint,
		float X,
		float Y,
		float Width,
		float Height)
	{
		UTexture2D* Atlas = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/MordecaiCore/UI/TinTally/Textures/T_UI_TallyAtlas.T_UI_TallyAtlas"));
		AddBrush(
			Tree, Canvas, TallyAtlasBrush(Atlas, Frame, FVector2D(Width, Height)),
			Tint, X, Y, Width, Height);
	}

	void AddRarityBadge(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		int32 StrokeCount,
		const FLinearColor& Color,
		const FMordecaiUIThemeColors& Colors,
		float X,
		float Y,
		float Size)
	{
		AddBox(
			Tree, Canvas, FLinearColor::Transparent, X, Y, Size, Size,
			Size * 0.5f, Color, 3.f);
		if (StrokeCount > 0)
		{
			AddAtlasFrame(
				Tree, Canvas, FMath::Clamp(StrokeCount, 1, 5), Color,
				X + Size * 0.18f, Y + Size * 0.20f, Size * 0.64f, Size * 0.60f);
		}
		if (StrokeCount == 0)
		{
			AddBox(
				Tree, Canvas, FLinearColor::Transparent,
				X + Size * 0.20f, Y + Size * 0.20f, Size * 0.60f, Size * 0.60f,
				Size * 0.3f, Colors.Lip, 2.f);
		}
	}

	void AddSeal(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		float X,
		float Y,
		float Size)
	{
		AddBox(Tree, Canvas, Colors.Plate, X, Y, Size, Size, Size * 0.5f, Colors.Rim, 4.f);
		AddBox(
			Tree, Canvas, FLinearColor::Transparent,
			X + 7.f, Y + 7.f, Size - 14.f, Size - 14.f,
			(Size - 14.f) * 0.5f, Colors.Rim, 2.f);
		AddAtlasFrame(
			Tree, Canvas, 5, Colors.Rim,
			X + Size * 0.22f, Y + Size * 0.25f, Size * 0.56f, Size * 0.50f);
	}

	void AddCapDot(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FLinearColor& Tint,
		float X,
		float Y,
		float Radius = 4.f)
	{
		AddBox(Tree, Canvas, Tint, X - Radius, Y - Radius, Radius * 2.f, Radius * 2.f, Radius);
	}

	UMordecaiTallyWidget* AddTally(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		int32 Count,
		int32 Max,
		EMordecaiTallyUsage Usage,
		float X,
		float Y,
		float Width,
		float Height)
	{
		UTexture2D* Atlas = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/MordecaiCore/UI/TinTally/Textures/T_UI_TallyAtlas.T_UI_TallyAtlas"));
		const int32 BundleCount = FMath::Max(1, FMath::DivideAndRoundUp(Max, 5));
		const FVector2D FrameSize(Width / BundleCount, Height);
		TArray<FSlateBrush> FillFrames;
		TArray<FSlateBrush> GhostFrames;
		FillFrames.Reserve(6);
		GhostFrames.Reserve(6);
		for (int32 Frame = 0; Frame < 6; ++Frame)
		{
			FSlateBrush Fill = TallyAtlasBrush(Atlas, Frame, FrameSize);
			if (Frame == 0)
			{
				Fill.DrawAs = ESlateBrushDrawType::NoDrawType;
			}
			FillFrames.Add(Fill);
			FSlateBrush Ghost = TallyAtlasBrush(Atlas, 0, FrameSize);
			Ghost.SetUVRegion(FBox2D(FVector2D(0.f, 0.f), FVector2D(1.f / 6.f, 1.f)));
			GhostFrames.Add(Ghost);
		}
		FSlateBrush CapDot;
		CapDot.DrawAs = ESlateBrushDrawType::NoDrawType;
		UMordecaiTallyWidget* Tally = Tree->ConstructWidget<UMordecaiTallyWidget>();
		Tally->SetUsage(Usage);
		Tally->SetFrameBrushes(FillFrames, GhostFrames, CapDot);
		Tally->SetCountAndMax(Count, Max);
		Place(Canvas, Tally, X, Y, Width, Height);
		return Tally;
	}

	void BuildPlateFixture(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUITheme Theme)
	{
		AddText(
			Tree, Canvas, TEXT("ANATOMY AT 4X · ONE PLATE ACROSS A LIGHT AND A DARK FLOOR"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 22.f, 912.f, 28.f);
		AddBox(Tree, Canvas, Colors.Enamel, 24.f, 60.f, 200.f, 236.f, 10.f);
		AddPattern(Tree, Canvas, Colors.WorldBlockoutLine, 24.f, 60.f, 200.f, 236.f);
		AddBox(Tree, Canvas, Colors.WorldNight, 224.f, 60.f, 200.f, 236.f, 10.f);
		AddPattern(Tree, Canvas, Colors.WorldNightDot, 224.f, 60.f, 200.f, 236.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("PlateAnatomy"),
			80.f,
			82.f,
			288.f,
			168.f);
		AddText(
			Tree, Canvas, TEXT("Kills: 12"), TEXT("num_l"), Colors.Text,
			108.f, 135.f, 230.f, 55.f, ETextJustify::Center);
		const FVector2D MarkerPositions[] = {
			FVector2D(53.f, 151.f), FVector2D(159.f, 71.f), FVector2D(349.f, 227.f),
			FVector2D(209.f, 151.f), FVector2D(141.f, 259.f), FVector2D(81.f, 83.f)
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(MarkerPositions); ++Index)
		{
			AddMarker(
				Tree, Canvas, Colors, Index + 1,
				MarkerPositions[Index].X, MarkerPositions[Index].Y);
		}
		const TCHAR* Legend[] = {
			TEXT("Ink outline · 3 px outside"),
			TEXT("Rim · top and left"),
			TEXT("Rim shade · bottom and right"),
			TEXT("Fill · always opaque"),
			TEXT("Print · hard 0, 4 offset"),
			TEXT("Radius · 12, outline 15")
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legend); ++Index)
		{
			const float RowY = 64.f + Index * 36.f;
			AddMarker(Tree, Canvas, Colors, Index + 1, 456.f, RowY);
			AddText(
				Tree, Canvas, Legend[Index], TEXT("ui_label"), Colors.Text,
				498.f, RowY + 2.f, 416.f, 28.f);
		}

		AddText(
			Tree, Canvas, TEXT("DEPTH LADDER · WORLD, PLATE, INSET, WELL"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 326.f, 912.f, 28.f);
		AddBox(Tree, Canvas, Colors.Enamel, 24.f, 366.f, 400.f, 340.f, 10.f);
		AddPattern(Tree, Canvas, Colors.WorldBlockoutLine, 24.f, 366.f, 400.f, 340.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("PlateRailHead"),
			56.f,
			394.f,
			320.f,
			56.f);
		AddText(
			Tree, Canvas, TEXT("Kills: 12"), TEXT("num_l"), Colors.Text,
			74.f, 402.f, 278.f, 40.f);
		const TCHAR* SpellNames[] = { TEXT("Fireball"), TEXT("Blink"), TEXT("Stone Skin") };
		const TCHAR* SpellGlyphs[] = { TEXT("fireball"), TEXT("blink"), TEXT("stoneskin") };
		for (int32 Slot = 0; Slot < 3; ++Slot)
		{
			const float SlotX = 54.f + Slot * 106.f;
			AddPrimitive(
				Tree,
				Canvas,
				FMordecaiUIPrimitiveBuilder::Plate(
					Colors, EMordecaiUIPrintDepth::HUD, Theme),
				FString::Printf(TEXT("SpellPlate%d"), Slot),
				SlotX,
				472.f,
				78.f,
				78.f);
			AddPrimitive(
				Tree,
				Canvas,
				FMordecaiUIPrimitiveBuilder::Well(Colors, false),
				FString::Printf(TEXT("SpellWell%d"), Slot),
				SlotX + 8.f,
				480.f,
				62.f,
				62.f);
			AddGlyph(
				Tree, Canvas, SpellGlyphs[Slot], Colors.Text,
				SlotX + 15.f, 487.f, 48.f);
			AddKeycap(
				Tree,
				Canvas,
				Colors,
				FString::Printf(TEXT("SpellKey%d"), Slot),
				Slot == 0 ? TEXT("Q") : Slot == 1 ? TEXT("E") : TEXT("R"),
				SlotX - 4.f,
				536.f,
				32.f,
				32.f);
			AddText(
				Tree, Canvas, SpellNames[Slot], TEXT("ui_label"), Colors.TextMuted,
				SlotX - 8.f, 578.f, 96.f, 28.f, ETextJustify::Center);
		}

		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("StoredResources"),
			444.f,
			366.f,
			492.f,
			340.f);
		AddText(
			Tree, Canvas, TEXT("Stored Resources"), TEXT("ui_title"), Colors.Text,
			470.f, 390.f, 440.f, 40.f);
		AddBox(Tree, Canvas, Colors.Rim, 470.f, 438.f, 440.f, 2.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::InsetCardWell(Colors),
			TEXT("StoredInset"),
			470.f,
			462.f,
			278.f,
			180.f);
		AddText(
			Tree, Canvas, TEXT("Iron Ore"), TEXT("ui_body"), Colors.Text,
			490.f, 482.f, 160.f, 32.f);
		AddText(
			Tree, Canvas, TEXT("x1"), TEXT("num_s"), Colors.Text,
			672.f, 482.f, 54.f, 28.f, ETextJustify::Right);
		AddText(
			Tree, Canvas, TEXT("Town Stone"), TEXT("ui_body"), Colors.Text,
			490.f, 544.f, 160.f, 32.f);
		AddText(
			Tree, Canvas, TEXT("x1"), TEXT("num_s"), Colors.Text,
			672.f, 544.f, 54.f, 28.f, ETextJustify::Right);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Well(Colors, false),
			TEXT("OreWell"),
			780.f,
			462.f,
			112.f,
			112.f);
		AddGlyph(Tree, Canvas, TEXT("ore"), Colors.Text, 812.f, 494.f, 48.f);
	}

	void BuildTagFixture(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUITheme Theme)
	{
		AddText(
			Tree, Canvas, TEXT("ANATOMY AT 2X"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 22.f, 912.f, 28.f);
		AddBox(Tree, Canvas, Colors.WorldBlockout, 24.f, 60.f, 912.f, 184.f, 10.f);
		AddPattern(Tree, Canvas, Colors.WorldBlockoutLine, 24.f, 60.f, 912.f, 184.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Tag(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("TagAnatomy"),
			196.f,
			98.f,
			600.f,
			104.f);
		AddText(
			Tree, Canvas, TEXT("Upgrade Key"), TEXT("ui_title"), Colors.Ink,
			290.f, 128.f, 360.f, 48.f);
		AddBox(Tree, Canvas, Colors.Well, 702.f, 122.f, 58.f, 58.f, 29.f, Colors.Ink, 3.f);
		AddRarityBadge(Tree, Canvas, 2, Colors.RarityBlue, Colors, 712.f, 132.f, 38.f);
		const FVector2D MarkerPositions[] = {
			FVector2D(225.f, 67.f), FVector2D(521.f, 195.f), FVector2D(633.f, 135.f),
			FVector2D(723.f, 67.f), FVector2D(175.f, 191.f), FVector2D(819.f, 135.f)
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(MarkerPositions); ++Index)
		{
			AddMarker(
				Tree, Canvas, Colors, Index + 1,
				MarkerPositions[Index].X, MarkerPositions[Index].Y, false);
		}
		const TCHAR* Legend[] = {
			TEXT("Eyelet · well hole and double ring"),
			TEXT("Outline · ink plus hard print"),
			TEXT("Face · enamel, ink text"),
			TEXT("Socket · holds rarity badge"),
			TEXT("Corners · square eyelet end"),
			TEXT("Height · 52 min, padded")
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legend); ++Index)
		{
			const int32 Column = Index % 2;
			const int32 Row = Index / 2;
			const float MarkerX = 38.f + Column * 450.f;
			const float RowY = 256.f + Row * 30.f;
			AddMarker(Tree, Canvas, Colors, Index + 1, MarkerX, RowY, false);
			AddText(
				Tree, Canvas, Legend[Index], TEXT("ui_label"), Colors.Text,
				MarkerX + 42.f, RowY + 2.f, 390.f, 26.f);
		}

		AddText(
			Tree, Canvas, TEXT("HUNG · THREADED UNDER THE RAIL HEAD, OR FROM A PLATE"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 350.f, 912.f, 28.f);
		AddBox(Tree, Canvas, Colors.WorldBlockout, 24.f, 390.f, 400.f, 316.f, 10.f);
		AddPattern(Tree, Canvas, Colors.WorldBlockoutLine, 24.f, 390.f, 400.f, 316.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("TagRailHead"),
			56.f,
			408.f,
			320.f,
			56.f);
		AddText(
			Tree, Canvas, TEXT("Kills: 12"), TEXT("num_l"), Colors.Text,
			74.f, 416.f, 278.f, 40.f);
		AddBox(Tree, Canvas, Colors.Ink, 88.f, 458.f, 2.f, 218.f);
		const TCHAR* Labels[] = { TEXT("Iron Ore"), TEXT("Health Potion"), TEXT("Upgrade Key") };
		for (int32 Row = 0; Row < 3; ++Row)
		{
			const float RowY = 478.f + Row * 66.f;
			AddPrimitive(
				Tree,
				Canvas,
				FMordecaiUIPrimitiveBuilder::Tag(
					Colors, EMordecaiUIPrintDepth::HUD, Theme),
				FString::Printf(TEXT("HungTag%d"), Row),
				68.f,
				RowY,
				300.f,
				52.f);
			AddText(
				Tree, Canvas, Labels[Row], TEXT("ui_body"), Colors.Ink,
				116.f, RowY + 10.f, 174.f, 32.f);
			if (Row == 0)
			{
				AddText(
					Tree, Canvas, TEXT("x3"), TEXT("num_s"), Colors.Ink,
					306.f, RowY + 10.f, 42.f, 28.f, ETextJustify::Right);
			}
			else
			{
				AddBox(Tree, Canvas, Colors.Well, 314.f, RowY + 8.f, 36.f, 36.f, 6.f);
				AddRarityBadge(
					Tree, Canvas, Row == 1 ? 0 : 2,
					Row == 1 ? Colors.RarityCommon : Colors.RarityBlue,
					Colors, 319.f, RowY + 13.f, 26.f);
			}
		}

		AddBox(Tree, Canvas, Colors.WorldMire, 444.f, 390.f, 492.f, 316.f, 10.f);
		AddPattern(Tree, Canvas, Colors.WorldMireDot, 444.f, 390.f, 492.f, 316.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("GaugePlate"),
			474.f,
			410.f,
			432.f,
			178.f);
		AddText(
			Tree, Canvas, TEXT("HP"), TEXT("ui_label"), Colors.TextMuted,
			500.f, 435.f, 50.f, 30.f);
		AddPrimitive(
			Tree, Canvas, FMordecaiUIPrimitiveBuilder::Well(Colors, true),
			TEXT("HPWell"), 548.f, 438.f, 316.f, 22.f);
		AddBox(Tree, Canvas, Colors.Vital, 552.f, 442.f, 126.f, 14.f, 5.f);
		AddText(
			Tree, Canvas, TEXT("ST"), TEXT("ui_label"), Colors.TextMuted,
			500.f, 484.f, 50.f, 30.f);
		AddPrimitive(
			Tree, Canvas, FMordecaiUIPrimitiveBuilder::Well(Colors, true),
			TEXT("STWell"), 548.f, 487.f, 316.f, 22.f);
		AddBox(Tree, Canvas, Colors.Danger, 552.f, 491.f, 220.f, 14.f, 5.f);
		AddText(
			Tree, Canvas, TEXT("x0.5"), TEXT("num_s"), Colors.DangerText,
			790.f, 480.f, 74.f, 34.f, ETextJustify::Right);
		AddBox(Tree, Canvas, Colors.Rim, 530.f, 584.f, 4.f, 38.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Tag(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("ExhaustedTag"),
			500.f,
			614.f,
			382.f,
			66.f);
		AddText(
			Tree, Canvas, TEXT("Exhausted: every action at ×0.5"), TEXT("ui_label"), Colors.Ink,
			550.f, 630.f, 312.f, 30.f);
	}

	void BuildKeycapFixture(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUITheme Theme)
	{
		AddText(
			Tree, Canvas, TEXT("SHAPE TELLS THE DEVICE"), TEXT("kicker"), Colors.TextMuted,
			24.f, 22.f, 912.f, 28.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("KeyboardPanel"),
			24.f,
			58.f,
			912.f,
			202.f);
		AddText(
			Tree, Canvas, TEXT("Keyboard and mouse · square caps"), TEXT("ui_title"), Colors.Text,
			46.f, 72.f, 470.f, 38.f);
		AddText(
			Tree, Canvas, TEXT("Radius 8 · 40 x 40 · words use --wide"),
			TEXT("ui_label"), Colors.TextMuted, 520.f, 76.f, 380.f, 32.f, ETextJustify::Right);
		const TCHAR* Keys[] = {
			TEXT("F"), TEXT("I"), TEXT("Q"), TEXT("E"), TEXT("R"), TEXT("1"),
			TEXT("Tab"), TEXT("Esc"), TEXT("Shift"), TEXT("Alt"), TEXT("Space")
		};
		float KeyX = 48.f;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Keys); ++Index)
		{
			const bool bWide = Index >= 6;
			const float Width = Index == 10 ? 104.f : bWide ? 64.f : 40.f;
			AddKeycap(
				Tree, Canvas, Colors, FString::Printf(TEXT("KeyboardKey%d"), Index),
				Keys[Index], KeyX, 116.f, Width, 40.f);
			KeyX += Width + 12.f;
		}
		AddMouseCap(Tree, Canvas, Colors, TEXT("MouseLeft"), 50.f, 180.f, 0);
		AddMouseCap(Tree, Canvas, Colors, TEXT("MouseRight"), 132.f, 180.f, 2);
		AddMouseCap(Tree, Canvas, Colors, TEXT("MouseMiddle"), 214.f, 180.f, 1);
		AddText(
			Tree, Canvas, TEXT("LMB"), TEXT("ui_label"), Colors.TextMuted,
			42.f, 224.f, 56.f, 28.f, ETextJustify::Center);
		AddText(
			Tree, Canvas, TEXT("RMB"), TEXT("ui_label"), Colors.TextMuted,
			124.f, 224.f, 56.f, 28.f, ETextJustify::Center);
		AddText(
			Tree, Canvas, TEXT("MMB"), TEXT("ui_label"), Colors.TextMuted,
			206.f, 224.f, 56.f, 28.f, ETextJustify::Center);
		AddText(
			Tree, Canvas, TEXT("Mouse buttons are drawn, not spelled: the lit button is solid ink."),
			TEXT("ui_label"), Colors.TextMuted, 330.f, 184.f, 540.f, 40.f);

		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("GamepadPanel"),
			24.f,
			278.f,
			912.f,
			230.f);
		AddText(
			Tree, Canvas, TEXT("Gamepad · round, pill, cross"), TEXT("ui_title"), Colors.Text,
			46.f, 292.f, 480.f, 38.f);
		AddText(
			Tree, Canvas, TEXT("Plain letters, no brand colours"), TEXT("ui_label"), Colors.TextMuted,
			560.f, 296.f, 340.f, 32.f, ETextJustify::Right);
		const TCHAR* FaceKeys[] = { TEXT("A"), TEXT("B"), TEXT("X"), TEXT("Y") };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(FaceKeys); ++Index)
		{
			AddKeycap(
				Tree, Canvas, Colors, FString::Printf(TEXT("Face%d"), Index),
				FaceKeys[Index], 48.f + Index * 50.f, 344.f, 40.f, 40.f,
				EMordecaiKeycapShape::Round);
		}
		AddText(
			Tree, Canvas, TEXT("Face buttons"), TEXT("ui_label"), Colors.TextMuted,
			48.f, 390.f, 190.f, 28.f, ETextJustify::Center);
		AddFacePosition(Tree, Canvas, Colors, 242.f, 344.f);
		AddText(
			Tree, Canvas, TEXT("Position"), TEXT("ui_label"), Colors.TextMuted,
			232.f, 390.f, 60.f, 28.f, ETextJustify::Center);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("LB"), TEXT("LB"), 280.f, 344.f, 66.f, 40.f,
			EMordecaiKeycapShape::Pill);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("RB"), TEXT("RB"), 356.f, 344.f, 66.f, 40.f,
			EMordecaiKeycapShape::Pill);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("LT"), TEXT("LT"), 456.f, 338.f, 58.f, 48.f,
			EMordecaiKeycapShape::Pill);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("RT"), TEXT("RT"), 524.f, 338.f, 58.f, 48.f,
			EMordecaiKeycapShape::Pill);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("StickL"), TEXT("L"), 620.f, 338.f, 48.f, 48.f,
			EMordecaiKeycapShape::Round);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("StickR"), TEXT("R"), 678.f, 338.f, 48.f, 48.f,
			EMordecaiKeycapShape::Round);
		AddText(
			Tree, Canvas, TEXT("Shoulders"), TEXT("ui_label"), Colors.TextMuted,
			278.f, 394.f, 146.f, 28.f, ETextJustify::Center);
		AddText(
			Tree, Canvas, TEXT("Triggers"), TEXT("ui_label"), Colors.TextMuted,
			454.f, 394.f, 130.f, 28.f, ETextJustify::Center);
		AddText(
			Tree, Canvas, TEXT("Stick clicks"), TEXT("ui_label"), Colors.TextMuted,
			618.f, 394.f, 110.f, 28.f, ETextJustify::Center);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			AddDPad(Tree, Canvas, Colors, 48.f + Index * 54.f, 438.f, 40.f);
		}
		AddDPad(Tree, Canvas, Colors, 288.f, 446.f, 32.f);
		AddDPad(Tree, Canvas, Colors, 334.f, 446.f, 32.f);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("MenuKey"), TEXT("Menu"), 430.f, 438.f, 76.f, 40.f,
			EMordecaiKeycapShape::Pill);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("ViewKey"), TEXT("View"), 518.f, 438.f, 76.f, 40.f,
			EMordecaiKeycapShape::Pill);
		AddText(
			Tree, Canvas, TEXT("D-pad up, right, down, left     32 px     Start, Select"),
			TEXT("ui_label"), Colors.TextMuted, 48.f, 480.f, 650.f, 26.f);

		AddText(
			Tree, Canvas, TEXT("STATES"), TEXT("kicker"), Colors.TextMuted,
			24.f, 530.f, 912.f, 28.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("KeyStates"),
			24.f,
			566.f,
			912.f,
			154.f);
		AddText(
			Tree, Canvas, TEXT("Normal"), TEXT("ui_label"), Colors.TextMuted,
			44.f, 640.f, 100.f, 32.f);
		const TCHAR* StateHeaders[] = {
			TEXT("KEY"), TEXT("WIDE"), TEXT("MOUSE"), TEXT("FACE"),
			TEXT("SHOULDER"), TEXT("TRIGGER"), TEXT("STICK"), TEXT("D-PAD")
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(StateHeaders); ++Index)
		{
			const float HeaderX = 150.f + Index * 92.f;
			AddText(
				Tree, Canvas, StateHeaders[Index], TEXT("ui_label"), Colors.TextMuted,
				HeaderX, 586.f, 84.f, 30.f, ETextJustify::Center);
		}
		AddKeycap(Tree, Canvas, Colors, TEXT("StateQ"), TEXT("Q"), 172.f, 632.f);
		AddKeycap(Tree, Canvas, Colors, TEXT("StateEsc"), TEXT("Esc"), 252.f, 632.f, 64.f);
		AddMouseCap(Tree, Canvas, Colors, TEXT("StateMouse"), 348.f, 630.f, 0);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("StateA"), TEXT("A"), 450.f, 632.f, 40.f, 40.f,
			EMordecaiKeycapShape::Round);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("StateLB"), TEXT("LB"), 532.f, 632.f, 62.f, 40.f,
			EMordecaiKeycapShape::Pill);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("StateRT"), TEXT("RT"), 626.f, 628.f, 54.f, 46.f,
			EMordecaiKeycapShape::Pill);
		AddKeycap(
			Tree, Canvas, Colors, TEXT("StateL"), TEXT("L"), 720.f, 628.f, 46.f, 46.f,
			EMordecaiKeycapShape::Round);
		AddDPad(Tree, Canvas, Colors, 824.f, 632.f, 40.f);
	}

	void BuildTallyFixture(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUITheme Theme)
	{
		AddText(
			Tree, Canvas, TEXT("ONE MARK, EXACTLY THREE JOBS · NOTHING ELSE DRAWS STROKES"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 22.f, 912.f, 28.f);
		const TCHAR* Titles[] = { TEXT("Skill ranks"), TEXT("Rarity"), TEXT("Seal, throbber") };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const float PanelX = 24.f + Index * 304.f;
			AddPrimitive(
				Tree,
				Canvas,
				FMordecaiUIPrimitiveBuilder::Plate(
					Colors, EMordecaiUIPrintDepth::Menu, Theme),
				FString::Printf(TEXT("TallyUse%d"), Index),
				PanelX,
				58.f,
				288.f,
				188.f);
			AddText(
				Tree, Canvas, FString::FromInt(Index + 1), TEXT("num_l"), Colors.TextMuted,
				PanelX + 18.f, 72.f, 32.f, 40.f);
			AddText(
				Tree, Canvas, Titles[Index], TEXT("ui_title"), Colors.Text,
				PanelX + 56.f, 74.f, 210.f, 38.f);
		}
		AddTally(
			Tree, Canvas, 12, 20, EMordecaiTallyUsage::SkillRank,
			44.f, 124.f, 148.f, 32.f);
		AddCapDot(Tree, Canvas, Colors.Text, 48.f, 127.f, 3.f);
		AddText(
			Tree, Canvas, TEXT("12 / 20"), TEXT("num_m"), Colors.Text,
			198.f, 126.f, 88.f, 32.f, ETextJustify::Right);
		const FLinearColor RarityColors[] = {
			Colors.RarityCommon, Colors.RarityGreen, Colors.RarityBlue,
			Colors.RarityPurple, Colors.RarityRed, Colors.RarityGold
		};
		for (int32 Rank = 0; Rank < 6; ++Rank)
		{
			AddRarityBadge(
				Tree, Canvas, Rank, RarityColors[Rank], Colors,
				348.f + Rank * 40.f, 124.f, 30.f);
		}
		AddSeal(Tree, Canvas, Colors, 650.f, 116.f, 56.f);
		for (int32 Frame = 0; Frame < 6; ++Frame)
		{
			AddAtlasFrame(
				Tree, Canvas, 0, Colors.Lip,
				720.f + Frame * 32.f, 126.f, 26.f, 34.f);
			if (Frame > 0)
			{
				AddAtlasFrame(
					Tree, Canvas, Frame, Colors.Text,
					720.f + Frame * 32.f, 126.f, 26.f, 34.f);
			}
		}
		AddText(
			Tree, Canvas, TEXT("20 strokes; slashes mark 5 / 10 / 15 / 20."),
			TEXT("ui_label"), Colors.TextMuted, 44.f, 178.f, 244.f, 52.f);
		AddText(
			Tree, Canvas, TEXT("A ring, then 1–4 strokes; Gold is the full bundle."),
			TEXT("ui_label"), Colors.TextMuted, 348.f, 178.f, 244.f, 52.f);
		AddText(
			Tree, Canvas, TEXT("The seal and static loading frames."),
			TEXT("ui_label"), Colors.TextMuted, 652.f, 178.f, 252.f, 52.f);

		AddText(
			Tree, Canvas, TEXT("ANATOMY, ENLARGED · RANK 7 OF 10"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 264.f, 912.f, 28.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("TallyAnatomy"),
			24.f,
			302.f,
			330.f,
			174.f);
		AddTally(
			Tree, Canvas, 7, 10, EMordecaiTallyUsage::SkillRank,
			64.f, 336.f, 246.f, 96.f);
		AddCapDot(Tree, Canvas, Colors.Text, 71.f, 343.f, 6.f);
		const FVector2D AnatomyMarkers[] = {
			FVector2D(58.f, 324.f), FVector2D(123.f, 438.f),
			FVector2D(250.f, 438.f), FVector2D(310.f, 324.f)
		};
		const TCHAR* AnatomyText[] = {
			TEXT("Cap dot on rank 1"),
			TEXT("Four upright strokes"),
			TEXT("Slash closes every five"),
			TEXT("Dotted ghosts show capacity")
		};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(AnatomyText); ++Index)
		{
			const float RowY = 304.f + Index * 42.f;
			AddMarker(
				Tree, Canvas, Colors, Index + 1,
				AnatomyMarkers[Index].X, AnatomyMarkers[Index].Y);
			AddMarker(Tree, Canvas, Colors, Index + 1, 390.f, RowY);
			AddText(
				Tree, Canvas, AnatomyText[Index], TEXT("ui_label"), Colors.Text,
				432.f, RowY + 2.f, 460.f, 28.f);
		}

		AddText(
			Tree, Canvas, TEXT("SKILL RANKS · THE SKILLS TAB ROW (148 x 24) AND DETAIL (180 x 32)"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 500.f, 912.f, 28.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("RankList"),
			24.f,
			536.f,
			500.f,
			180.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("RankDetail"),
			544.f,
			536.f,
			392.f,
			180.f);
		AddText(
			Tree, Canvas, TEXT("Longswords"), TEXT("ui_title"), Colors.Text,
			566.f, 550.f, 250.f, 36.f);
		const TCHAR* Names[] = {
			TEXT("Parrying"), TEXT("Fireball"), TEXT("Blink"),
			TEXT("Stone Skin"), TEXT("Longswords"), TEXT("Dodging")
		};
		const int32 Counts[] = { 0, 1, 4, 5, 12, 20 };
		for (int32 Row = 0; Row < 6; ++Row)
		{
			const float RowY = 544.f + Row * 27.f;
			AddText(
				Tree, Canvas, Names[Row], TEXT("ui_body"),
				Row == 0 ? Colors.TextDisabled : Colors.Text,
				44.f, RowY, 148.f, 28.f);
			AddTally(
				Tree, Canvas, Counts[Row], 20, EMordecaiTallyUsage::SkillRank,
				202.f, RowY + 1.f, 148.f, 24.f);
			if (Counts[Row] > 0)
			{
				AddCapDot(Tree, Canvas, Colors.Text, 205.f, RowY + 4.f, 2.f);
			}
			AddText(
				Tree, Canvas, FString::Printf(TEXT("%d / 20"), Counts[Row]), TEXT("num_m"),
				Row == 0 ? Colors.TextDisabled : Colors.Text,
				376.f, RowY, 126.f, 28.f, ETextJustify::Right);
		}
		AddTally(
			Tree, Canvas, 12, 20, EMordecaiTallyUsage::SkillRank,
			566.f, 594.f, 180.f, 32.f);
		AddCapDot(Tree, Canvas, Colors.Text, 570.f, 598.f, 3.f);
		AddText(
			Tree, Canvas, TEXT("12 / 20"), TEXT("num_m"), Colors.Text,
			806.f, 594.f, 104.f, 32.f, ETextJustify::Right);
		AddText(
			Tree, Canvas, TEXT("1     5      10      15      20"), TEXT("num_s"), Colors.TextMuted,
			562.f, 636.f, 202.f, 26.f);
		AddText(
			Tree, Canvas, TEXT("Detail size · milestone numbers sit under the marks."),
			TEXT("ui_label"), Colors.TextMuted, 566.f, 674.f, 340.f, 30.f);
	}

	void BuildGlyphFixture(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors)
	{
		static const TCHAR* StatusNames[] = {
			TEXT("burning"), TEXT("bleeding"), TEXT("poisoned"), TEXT("frostbitten"),
			TEXT("frozen"), TEXT("shocked"), TEXT("weakened"), TEXT("brittle"),
			TEXT("silenced"), TEXT("rooted"), TEXT("blinded"), TEXT("fear"),
			TEXT("cursed"), TEXT("exposed"), TEXT("corroded"), TEXT("drenched"),
			TEXT("focused"), TEXT("stoneskin"), TEXT("fireward"), TEXT("blessed"),
			TEXT("restoration"), TEXT("enchanted"), TEXT("blurred"), TEXT("resolved")
		};
		AddText(
			Tree, Canvas, TEXT("STATUS · 25"), TEXT("kicker"), Colors.TextMuted,
			24.f, 22.f, 912.f, 28.f);
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(StatusNames); ++Index)
		{
			const int32 Column = Index % 6;
			const int32 Row = Index / 6;
			const float CellX = 24.f + Column * 152.f;
			const float CellY = 58.f + Row * 154.f;
			const FString GlyphName(StatusNames[Index]);
			const FString AssetName = GlyphName.Replace(TEXT("-"), TEXT("_"));
			const FString Path = FString::Printf(
				TEXT("/MordecaiCore/UI/TinTally/Textures/Glyphs/T_UI_Glyph_%s.T_UI_Glyph_%s"),
				*AssetName,
				*AssetName);
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
			AddPrimitive(
				Tree,
				Canvas,
				FMordecaiUIPrimitiveBuilder::InsetCardWell(Colors),
				FString::Printf(TEXT("GlyphDark%d"), Index),
				CellX,
				CellY,
				136.f,
				68.f);
			AddTexture(
				Tree, Canvas, Texture, Colors.Text,
				CellX + 52.f, CellY + 18.f, 32.f, 32.f);
			AddBox(
				Tree, Canvas, Colors.Enamel, CellX, CellY + 76.f, 136.f, 40.f,
				8.f, Colors.Ink, 2.f);
			AddTexture(
				Tree, Canvas, Texture, Colors.Ink,
				CellX + 58.f, CellY + 86.f, 20.f, 20.f);
			AddText(
				Tree, Canvas, GlyphName.ToUpper(), TEXT("num_s"), Colors.TextMuted,
				CellX, CellY + 120.f, 136.f, 30.f, ETextJustify::Center);
		}
		AddText(
			Tree, Canvas, TEXT("DAMAGE · 16"), TEXT("kicker"), Colors.TextMuted,
			24.f, 686.f, 912.f, 28.f);
	}
}

bool UMordecaiTinTallyGalleryWidget::Initialize()
{
	const bool bInitializationResult = Super::Initialize();
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return bInitializationResult;
	}

	const FString Component = FixtureComponentOverride.IsEmpty()
		? GalleryValue(TEXT("TinTallyGalleryComponent="), TEXT("Plate"))
		: FixtureComponentOverride;
	const FString ThemeName = FixtureThemeOverride.IsEmpty()
		? GalleryValue(TEXT("TinTallyGalleryTheme="), TEXT("diorama"))
		: FixtureThemeOverride;
	const bool bHighContrast = ThemeName.Equals(TEXT("high-contrast"), ESearchCase::IgnoreCase);
	const EMordecaiUITheme Theme =
		bHighContrast ? EMordecaiUITheme::HighContrast : EMordecaiUITheme::Diorama;
	FMordecaiUIThemeColors Colors = bHighContrast
		? FMordecaiUIThemeColors::MakeHighContrast()
		: FMordecaiUIThemeColors::MakeDiorama();
	const TCHAR* ThemePath = bHighContrast
		? TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_HighContrast.DA_UITheme_HighContrast")
		: TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_Diorama.DA_UITheme_Diorama");
	if (const UMordecaiUIThemeDataAsset* ThemeAsset =
		LoadObject<UMordecaiUIThemeDataAsset>(nullptr, ThemePath))
	{
		Colors = ThemeAsset->Colors;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("GalleryCanvas"));
	WidgetTree->RootWidget = Canvas;
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("GalleryBackdrop"));
	Backdrop->SetBrushColor(Colors.WorldNight);
	if (UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackdropSlot->SetOffsets(FMargin(0.f));
	}

	UBorder* Sheet = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("GallerySheet"));
	Sheet->SetBrushColor(Colors.Plate);
	Sheet->SetPadding(FMargin(0.f));
	if (UCanvasPanelSlot* SheetSlot = Canvas->AddChildToCanvas(Sheet))
	{
		SheetSlot->SetPosition(FVector2D(480.f, 120.f));
		SheetSlot->SetSize(FVector2D(960.f, 720.f));
	}
	UCanvasPanel* Fixture = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("GalleryFixture"));
	Sheet->SetContent(Fixture);

	if (Component.Equals(TEXT("Tag"), ESearchCase::IgnoreCase))
	{
		BuildTagFixture(WidgetTree, Fixture, Colors, Theme);
	}
	else if (Component.Equals(TEXT("Keycap"), ESearchCase::IgnoreCase))
	{
		BuildKeycapFixture(WidgetTree, Fixture, Colors, Theme);
	}
	else if (Component.Equals(TEXT("Tally"), ESearchCase::IgnoreCase))
	{
		BuildTallyFixture(WidgetTree, Fixture, Colors, Theme);
	}
	else if (Component.Equals(TEXT("Glyphs"), ESearchCase::IgnoreCase))
	{
		BuildGlyphFixture(WidgetTree, Fixture, Colors);
	}
	else
	{
		BuildPlateFixture(WidgetTree, Fixture, Colors, Theme);
	}
	return bInitializationResult;
}

void UMordecaiTinTallyGalleryWidget::SetFixtureSelectionForTest(
	const FString& InComponent,
	const FString& InTheme)
{
	FixtureComponentOverride = InComponent;
	FixtureThemeOverride = InTheme;
}

bool UMordecaiTinTallyGallerySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer);
#endif
}

void UMordecaiTinTallyGallerySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("TinTallyGallery")))
	{
		OutputPath = GalleryValue(
			TEXT("TinTallyGalleryOutput="),
			*(FPaths::ProjectSavedDir() / TEXT("TinTallyGallery/full.png")));
		WorldInitializedHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(
			this, &ThisClass::OnWorldInitialized);
	}
#endif
}

void UMordecaiTinTallyGallerySubsystem::Deinitialize()
{
	if (WorldInitializedHandle.IsValid())
	{
		FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitializedHandle);
		WorldInitializedHandle.Reset();
	}
	if (GalleryWidget)
	{
		GalleryWidget->RemoveFromParent();
		GalleryWidget = nullptr;
	}
	Super::Deinitialize();
}

void UMordecaiTinTallyGallerySubsystem::OnWorldInitialized(
	UWorld* World, const UWorld::InitializationValues InitializationValues)
{
	(void)InitializationValues;
	if (!World || !World->IsGameWorld() || GalleryWorld.IsValid())
	{
		return;
	}
	GalleryWorld = World;
	World->GetTimerManager().SetTimer(ShowTimer, this, &ThisClass::ShowGallery, 1.5f, false);
}

void UMordecaiTinTallyGallerySubsystem::ShowGallery()
{
	UWorld* World = GalleryWorld.Get();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller)
	{
		return;
	}
	if (UMordecaiUIThemeSubsystem* ThemeSubsystem =
		UMordecaiUIThemeSubsystem::ResolveForWorld(World))
	{
		UMordecaiUIThemeDataAsset* Diorama = LoadObject<UMordecaiUIThemeDataAsset>(
			nullptr,
			TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_Diorama.DA_UITheme_Diorama"));
		UMordecaiUIThemeDataAsset* HighContrast = LoadObject<UMordecaiUIThemeDataAsset>(
			nullptr,
			TEXT("/MordecaiCore/UI/TinTally/Themes/DA_UITheme_HighContrast.DA_UITheme_HighContrast"));
		ThemeSubsystem->SetThemeAssets(Diorama, HighContrast);
		const FString ThemeName =
			GalleryValue(TEXT("TinTallyGalleryTheme="), TEXT("diorama"));
		ThemeSubsystem->SetActiveTheme(
			ThemeName.Equals(TEXT("high-contrast"), ESearchCase::IgnoreCase)
				? EMordecaiUITheme::HighContrast
				: EMordecaiUITheme::Diorama);
	}
	GalleryWidget = CreateWidget<UMordecaiTinTallyGalleryWidget>(Controller);
	if (GalleryWidget)
	{
		GalleryWidget->AddToViewport(10000);
		World->GetTimerManager().SetTimer(CaptureTimer, this, &ThisClass::CaptureGallery, 1.f, false);
	}
}

void UMordecaiTinTallyGallerySubsystem::CaptureGallery()
{
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
	// The gallery itself is Slate/UMG. Omitting UI would capture only the map
	// behind it and could produce a plausible, non-black false positive.
	FScreenshotRequest::RequestScreenshot(OutputPath, true, false);

	TSharedRef<FJsonObject> Metadata = MakeShared<FJsonObject>();
	Metadata->SetNumberField(TEXT("schema"), 1);
	Metadata->SetStringField(TEXT("component"),
		GalleryValue(TEXT("TinTallyGalleryComponent="), TEXT("Plate")));
	Metadata->SetStringField(TEXT("theme"),
		GalleryValue(TEXT("TinTallyGalleryTheme="), TEXT("diorama")));
	Metadata->SetStringField(TEXT("state"),
		GalleryValue(TEXT("TinTallyGalleryState="), TEXT("sheet")));
	Metadata->SetStringField(TEXT("commit"),
		GalleryValue(TEXT("TinTallyGalleryCommit="), TEXT("unknown")));
	Metadata->SetArrayField(TEXT("resolution"), {
		MakeShared<FJsonValueNumber>(1920), MakeShared<FJsonValueNumber>(1080) });
	Metadata->SetArrayField(TEXT("crop_rect"), {
		MakeShared<FJsonValueNumber>(480), MakeShared<FJsonValueNumber>(120),
		MakeShared<FJsonValueNumber>(960), MakeShared<FJsonValueNumber>(720) });
	FString Json;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
	FJsonSerializer::Serialize(Metadata, Writer);
	FFileHelper::SaveStringToFile(Json, *(FPaths::ChangeExtension(OutputPath, TEXT("json"))));
}
