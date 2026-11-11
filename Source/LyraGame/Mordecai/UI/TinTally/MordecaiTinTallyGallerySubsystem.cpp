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

	UCommonTextBlock* AddText(
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
		UCommonTextBlock* Text = Tree->ConstructWidget<UCommonTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Justification);
		Text->SetAutoWrapText(true);
		const FString ClassPath = FString::Printf(
			TEXT("/MordecaiCore/UI/TinTally/Fonts/Styles/TS_Tally_%s.TS_Tally_%s_C"),
			StyleName,
			StyleName);
		if (UClass* StyleClass = LoadClass<UCommonTextStyle>(nullptr, *ClassPath))
		{
			Text->SetStyle(StyleClass);
		}
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
			AddText(
				Tree,
				Canvas,
				Label,
				TEXT("num_s"),
				Colors.Ink,
				X,
				Y + 1.f,
				Width,
				Height - 5.f,
				ETextJustify::Center);
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
			48.f,
			44.f,
			EMordecaiKeycapShape::Pill);
		for (int32 Section = 0; Section < 3; ++Section)
		{
			const float SectionX = X + 5.f + Section * 12.f;
			AddBox(
				Tree,
				Canvas,
				Section == LitSection ? Colors.Ink : Colors.EnamelShade,
				SectionX,
				Y + 6.f,
				11.f,
				16.f,
				Section == 1 ? 1.f : 4.f);
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
			FillFrames.Add(TallyAtlasBrush(Atlas, Frame, FrameSize));
			FSlateBrush Ghost = TallyAtlasBrush(Atlas, 1, FrameSize);
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
		AddBox(Tree, Canvas, Colors.WorldNight, 224.f, 60.f, 200.f, 236.f, 10.f);
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
			Tree, Canvas, TEXT("KILLS: 12"), TEXT("num_l"), Colors.Text,
			108.f, 135.f, 230.f, 55.f, ETextJustify::Center);
		AddText(
			Tree, Canvas,
			TEXT("1  INK OUTLINE\n2  CREAM RIM\n3  RIM SHADE\n4  PLATE FILL\n5  HARD PRINT\n6  RADIUS 12 / 15"),
			TEXT("ui_label"), Colors.Text, 456.f, 65.f, 450.f, 220.f);

		AddText(
			Tree, Canvas, TEXT("DEPTH LADDER · HUD RAIL AND MENU LEDGER"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 326.f, 912.f, 28.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("PlateRail"),
			24.f,
			366.f,
			400.f,
			340.f);
		AddText(
			Tree, Canvas, TEXT("KILLS: 12"), TEXT("num_l"), Colors.Text,
			54.f, 388.f, 320.f, 48.f);
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
				466.f,
				78.f,
				78.f);
			AddPrimitive(
				Tree,
				Canvas,
				FMordecaiUIPrimitiveBuilder::Well(Colors, false),
				FString::Printf(TEXT("SpellWell%d"), Slot),
				SlotX + 8.f,
				474.f,
				62.f,
				62.f);
			AddKeycap(
				Tree,
				Canvas,
				Colors,
				FString::Printf(TEXT("SpellKey%d"), Slot),
				Slot == 0 ? TEXT("Q") : Slot == 1 ? TEXT("E") : TEXT("R"),
				SlotX - 4.f,
				530.f,
				32.f,
				32.f);
		}
		AddText(
			Tree, Canvas, TEXT("FIREBALL       BLINK       STONE SKIN"),
			TEXT("ui_label"), Colors.TextMuted, 54.f, 580.f, 340.f, 36.f);

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
			Tree, Canvas, TEXT("STORED RESOURCES"), TEXT("ui_title"), Colors.Text,
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
			Tree, Canvas, TEXT("IRON ORE                         x1\n\nTOWN STONE                    x1"),
			TEXT("ui_body"), Colors.Text, 490.f, 482.f, 238.f, 130.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Well(Colors, false),
			TEXT("OreWell"),
			780.f,
			462.f,
			112.f,
			112.f);
		UTexture2D* Hatch = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/MordecaiCore/UI/TinTally/Textures/T_UI_Hatch16.T_UI_Hatch16"));
		UImage* Pattern = AddTexture(
			Tree, Canvas, Hatch, Colors.Danger, 780.f, 594.f, 112.f, 70.f);
		FSlateBrush PatternBrush = Pattern->GetBrush();
		PatternBrush.Tiling = ESlateBrushTileType::Both;
		Pattern->SetBrush(PatternBrush);
	}

	void BuildTagFixture(
		UWidgetTree* Tree,
		UCanvasPanel* Canvas,
		const FMordecaiUIThemeColors& Colors,
		EMordecaiUITheme Theme)
	{
		AddText(
			Tree, Canvas, TEXT("ANATOMY AT 2X · EYELET, FACE, SOCKET AND LINE"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 22.f, 912.f, 28.f);
		AddBox(Tree, Canvas, Colors.WorldNight, 24.f, 60.f, 912.f, 184.f, 10.f);
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
			Tree, Canvas, TEXT("UPGRADE KEY"), TEXT("ui_title"), Colors.Ink,
			290.f, 128.f, 360.f, 48.f);
		AddBox(Tree, Canvas, Colors.Well, 702.f, 122.f, 58.f, 58.f, 29.f, Colors.Ink, 3.f);
		AddTally(
			Tree, Canvas, 2, 5, EMordecaiTallyUsage::RarityBadge,
			706.f, 127.f, 50.f, 48.f);
		AddText(
			Tree, Canvas,
			TEXT("1 EYELET    2 INK OUTLINE    3 ENAMEL FACE    4 RARITY SOCKET    5 MIN 52    6 PAD 10 / 16"),
			TEXT("ui_label"), Colors.Text, 38.f, 260.f, 884.f, 44.f);

		AddText(
			Tree, Canvas, TEXT("HUNG · RAIL TAGS, GAUGE AND EXHAUSTED HINT"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 330.f, 912.f, 28.f);
		AddBox(Tree, Canvas, Colors.PlateRaised, 24.f, 370.f, 400.f, 336.f, 10.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("TagRailHead"),
			56.f,
			394.f,
			320.f,
			56.f);
		AddText(
			Tree, Canvas, TEXT("KILLS: 12"), TEXT("num_l"), Colors.Text,
			74.f, 402.f, 278.f, 40.f);
		const TCHAR* Labels[] = { TEXT("IRON ORE      x3"), TEXT("HEALTH POTION"), TEXT("UPGRADE KEY") };
		for (int32 Row = 0; Row < 3; ++Row)
		{
			const float RowY = 472.f + Row * 68.f;
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
				116.f, RowY + 9.f, 222.f, 34.f);
		}

		AddBox(Tree, Canvas, Colors.WorldNight, 444.f, 370.f, 492.f, 336.f, 10.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("GaugePlate"),
			474.f,
			400.f,
			432.f,
			178.f);
		AddText(
			Tree, Canvas, TEXT("HP"), TEXT("ui_label"), Colors.TextMuted,
			500.f, 425.f, 50.f, 30.f);
		AddPrimitive(
			Tree, Canvas, FMordecaiUIPrimitiveBuilder::Well(Colors, true),
			TEXT("HPWell"), 548.f, 428.f, 316.f, 22.f);
		AddBox(Tree, Canvas, Colors.Vital, 552.f, 432.f, 126.f, 14.f, 5.f);
		AddText(
			Tree, Canvas, TEXT("ST"), TEXT("ui_label"), Colors.TextMuted,
			500.f, 474.f, 50.f, 30.f);
		AddPrimitive(
			Tree, Canvas, FMordecaiUIPrimitiveBuilder::Well(Colors, true),
			TEXT("STWell"), 548.f, 477.f, 316.f, 22.f);
		AddBox(Tree, Canvas, Colors.Danger, 552.f, 481.f, 220.f, 14.f, 5.f);
		AddText(
			Tree, Canvas, TEXT("x0.5"), TEXT("num_s"), Colors.DangerText,
			790.f, 470.f, 74.f, 34.f, ETextJustify::Right);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Tag(Colors, EMordecaiUIPrintDepth::HUD, Theme),
			TEXT("ExhaustedTag"),
			500.f,
			610.f,
			382.f,
			66.f);
		AddText(
			Tree, Canvas, TEXT("EXHAUSTED: EVERY ACTION AT x0.5"), TEXT("ui_label"), Colors.Ink,
			550.f, 625.f, 310.f, 38.f);
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
			Tree, Canvas, TEXT("KEYBOARD AND MOUSE · SQUARE CAPS"), TEXT("ui_title"), Colors.Text,
			46.f, 72.f, 470.f, 38.f);
		AddText(
			Tree, Canvas, TEXT("RADIUS 8 · 40 x 40 · WORDS USE --WIDE"),
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
			Tree, Canvas, TEXT("LMB          RMB          MMB"), TEXT("ui_label"), Colors.TextMuted,
			42.f, 224.f, 270.f, 28.f);
		AddText(
			Tree, Canvas, TEXT("MOUSE BUTTONS ARE DRAWN; THE LIT BUTTON IS SOLID INK."),
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
			Tree, Canvas, TEXT("GAMEPAD · ROUND, PILL, CROSS"), TEXT("ui_title"), Colors.Text,
			46.f, 292.f, 480.f, 38.f);
		AddText(
			Tree, Canvas, TEXT("PLAIN LETTERS, NO BRAND COLOURS"), TEXT("ui_label"), Colors.TextMuted,
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
			Tree, Canvas, TEXT("FACE BUTTONS"), TEXT("ui_label"), Colors.TextMuted,
			48.f, 390.f, 190.f, 28.f, ETextJustify::Center);
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
			Tree, Canvas, TEXT("SHOULDERS       TRIGGERS       STICK CLICKS"),
			TEXT("ui_label"), Colors.TextMuted, 270.f, 394.f, 470.f, 28.f, ETextJustify::Center);
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
			Tree, Canvas, TEXT("D-PAD UP, RIGHT, DOWN, LEFT      32 PX      START, SELECT"),
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
			Tree, Canvas, TEXT("NORMAL"), TEXT("ui_label"), Colors.TextMuted,
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
		const TCHAR* Titles[] = { TEXT("1  SKILL RANKS"), TEXT("2  RARITY"), TEXT("3  SEAL, THROBBER") };
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
				Tree, Canvas, Titles[Index], TEXT("ui_title"), Colors.Text,
				PanelX + 18.f, 74.f, 252.f, 38.f);
		}
		AddTally(
			Tree, Canvas, 12, 20, EMordecaiTallyUsage::SkillRank,
			44.f, 120.f, 188.f, 50.f);
		AddText(
			Tree, Canvas, TEXT("12 / 20"), TEXT("num_s"), Colors.Text,
			228.f, 128.f, 64.f, 34.f, ETextJustify::Right);
		for (int32 Rank = 0; Rank < 6; ++Rank)
		{
			AddTally(
				Tree, Canvas, Rank, 5, EMordecaiTallyUsage::RarityBadge,
				348.f + Rank * 39.f, 125.f, 34.f, 42.f);
		}
		AddTally(
			Tree, Canvas, 5, 5, EMordecaiTallyUsage::Seal,
			658.f, 120.f, 48.f, 48.f);
		for (int32 Frame = 0; Frame < 6; ++Frame)
		{
			AddTally(
				Tree, Canvas, Frame, 5, EMordecaiTallyUsage::LoadingThrobber,
				720.f + Frame * 34.f, 126.f, 30.f, 38.f);
		}
		AddText(
			Tree, Canvas, TEXT("20 STROKES · MILESTONES 5 / 10 / 15 / 20"),
			TEXT("ui_label"), Colors.TextMuted, 44.f, 184.f, 250.f, 52.f);
		AddText(
			Tree, Canvas, TEXT("RING, THEN 1–5 STROKES"),
			TEXT("ui_label"), Colors.TextMuted, 348.f, 184.f, 250.f, 40.f);
		AddText(
			Tree, Canvas, TEXT("SEAL + STATIC LOADING FRAMES"),
			TEXT("ui_label"), Colors.TextMuted, 652.f, 184.f, 252.f, 40.f);

		AddText(
			Tree, Canvas, TEXT("ANATOMY, ENLARGED · RANK 7 OF 10"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 274.f, 912.f, 28.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("TallyAnatomy"),
			24.f,
			312.f,
			330.f,
			200.f);
		AddTally(
			Tree, Canvas, 7, 10, EMordecaiTallyUsage::SkillRank,
			66.f, 356.f, 246.f, 104.f);
		AddText(
			Tree, Canvas,
			TEXT("1  CAP DOT ON RANK 1\n\n2  FOUR UPRIGHT STROKES\n\n3  SLASH = FIVE\n\n4  UNLIT GHOST DOTS SHOW CAPACITY"),
			TEXT("ui_label"), Colors.Text, 390.f, 318.f, 520.f, 190.f);

		AddText(
			Tree, Canvas, TEXT("SKILL RANKS · ROW 148 x 24 AND DETAIL 180 x 32"),
			TEXT("kicker"), Colors.TextMuted, 24.f, 540.f, 912.f, 28.f);
		AddPrimitive(
			Tree,
			Canvas,
			FMordecaiUIPrimitiveBuilder::Plate(Colors, EMordecaiUIPrintDepth::Menu, Theme),
			TEXT("RankList"),
			24.f,
			578.f,
			912.f,
			142.f);
		const TCHAR* Names[] = { TEXT("PARRYING"), TEXT("FIREBALL"), TEXT("BLINK"), TEXT("STONE SKIN") };
		const int32 Counts[] = { 0, 1, 4, 5 };
		for (int32 Row = 0; Row < 4; ++Row)
		{
			const float RowY = 590.f + Row * 30.f;
			AddText(
				Tree, Canvas, Names[Row], TEXT("ui_label"),
				Row == 0 ? Colors.TextDisabled : Colors.Text,
				46.f, RowY, 150.f, 28.f);
			AddTally(
				Tree, Canvas, Counts[Row], 20, EMordecaiTallyUsage::SkillRank,
				210.f, RowY - 5.f, 188.f, 30.f);
			AddText(
				Tree, Canvas, FString::Printf(TEXT("%d / 20"), Counts[Row]), TEXT("num_s"),
				Row == 0 ? Colors.TextDisabled : Colors.Text,
				410.f, RowY, 80.f, 28.f, ETextJustify::Right);
		}
		AddText(
			Tree, Canvas, TEXT("LONGSWORDS"), TEXT("ui_title"), Colors.Text,
			564.f, 598.f, 280.f, 34.f);
		AddTally(
			Tree, Canvas, 12, 20, EMordecaiTallyUsage::SkillRank,
			564.f, 638.f, 240.f, 42.f);
		AddText(
			Tree, Canvas, TEXT("12 / 20"), TEXT("num_m"), Colors.Text,
			816.f, 642.f, 88.f, 36.f, ETextJustify::Right);
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

	const FString Component = GalleryValue(TEXT("TinTallyGalleryComponent="), TEXT("Plate"));
	const FString ThemeName = GalleryValue(TEXT("TinTallyGalleryTheme="), TEXT("diorama"));
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
