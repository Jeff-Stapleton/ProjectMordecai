// Project Mordecai — packaged Tin Tally visual acceptance gallery (US-103)

#include "Mordecai/UI/TinTally/MordecaiTinTallyGallerySubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Mordecai/UI/TinTally/MordecaiUIPrimitives.h"
#include "Mordecai/UI/TinTally/MordecaiUITheme.h"
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

	UTextBlock* AddText(UWidgetTree* Tree, UVerticalBox* Parent, const FString& Value, int32 Size)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		if (UFont* TallyFont = LoadObject<UFont>(
			nullptr, TEXT("/MordecaiCore/UI/TinTally/Fonts/F_Tally_UI.F_Tally_UI")))
		{
			Font.FontObject = TallyFont;
		}
		Text->SetFont(Font);
		Parent->AddChildToVerticalBox(Text);
		return Text;
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
	const FString State = GalleryValue(TEXT("TinTallyGalleryState="), TEXT("sheet"));
	const bool bHighContrast = ThemeName.Equals(TEXT("high-contrast"), ESearchCase::IgnoreCase);
	const EMordecaiUITheme Theme =
		bHighContrast ? EMordecaiUITheme::HighContrast : EMordecaiUITheme::Diorama;
	const FMordecaiUIThemeColors Colors = bHighContrast
		? FMordecaiUIThemeColors::MakeHighContrast()
		: FMordecaiUIThemeColors::MakeDiorama();

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
	Sheet->SetPadding(FMargin(48.f));
	if (UCanvasPanelSlot* SheetSlot = Canvas->AddChildToCanvas(Sheet))
	{
		SheetSlot->SetPosition(FVector2D(480.f, 120.f));
		SheetSlot->SetSize(FVector2D(960.f, 720.f));
	}
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("GalleryColumn"));
	Sheet->SetContent(Column);
	AddText(WidgetTree, Column,
		FString::Printf(TEXT("TIN TALLY / %s / %s / %s"),
			*Component.ToUpper(), *ThemeName.ToUpper(), *State.ToUpper()), 30);

	auto AddPrimitive = [this, Column](const FMordecaiUIPrimitiveSpec& Spec, FName Name,
		float Width, float Height)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(Width);
		Box->SetHeightOverride(Height);
		Box->SetContent(FMordecaiUIPrimitiveBuilder::Build(WidgetTree, Spec, Name));
		Column->AddChildToVerticalBox(Box);
	};

	if (Component.Equals(TEXT("Tag"), ESearchCase::IgnoreCase))
	{
		AddPrimitive(FMordecaiUIPrimitiveBuilder::Tag(
			Colors, EMordecaiUIPrintDepth::HUD, Theme), TEXT("GalleryTag"), 680.f, 128.f);
		AddText(WidgetTree, Column, TEXT("FOUNDATION TAG / EYELET + HARD PRINT"), 20);
	}
	else if (Component.Equals(TEXT("Keycap"), ESearchCase::IgnoreCase))
	{
		AddPrimitive(FMordecaiUIPrimitiveBuilder::Keycap(
			Colors, EMordecaiKeycapShape::Square), TEXT("GalleryKeycap"), 96.f, 96.f);
		AddPrimitive(FMordecaiUIPrimitiveBuilder::Focus(
			Colors, 8.f, Theme), TEXT("GalleryFocus"), 118.f, 118.f);
		AddText(WidgetTree, Column, TEXT("F  /  INK | FOCUS | INK"), 20);
	}
	else if (Component.Equals(TEXT("Glyphs"), ESearchCase::IgnoreCase))
	{
		UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(
			UUniformGridPanel::StaticClass(), TEXT("GalleryGlyphGrid"));
		Column->AddChildToVerticalBox(Grid);
		TArray<FAssetData> GlyphAssets;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))
			.Get().GetAssetsByPath(
				TEXT("/MordecaiCore/UI/TinTally/Textures/Glyphs"), GlyphAssets, false);
		GlyphAssets.RemoveAll([](const FAssetData& Asset)
		{
			return !Asset.AssetName.ToString().StartsWith(TEXT("T_UI_Glyph_"));
		});
		GlyphAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.AssetName.LexicalLess(Right.AssetName);
		});
		for (int32 Index = 0; Index < GlyphAssets.Num(); ++Index)
		{
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			FSlateBrush Brush;
			Brush.SetResourceObject(Cast<UTexture2D>(GlyphAssets[Index].GetAsset()));
			Brush.ImageSize = FVector2D(40.f);
			Image->SetBrush(Brush);
			Image->SetColorAndOpacity(Colors.Text);
			if (UUniformGridSlot* GridSlot = Grid->AddChildToUniformGrid(
				Image, Index / 13, Index % 13))
			{
				GridSlot->SetHorizontalAlignment(HAlign_Center);
				GridSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}
	else if (Component.Equals(TEXT("Tally"), ESearchCase::IgnoreCase))
	{
		UHorizontalBox* Tally = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("GalleryTally"));
		Column->AddChildToVerticalBox(Tally);
		UTexture2D* Atlas = LoadObject<UTexture2D>(
			nullptr, TEXT("/MordecaiCore/UI/TinTally/Textures/T_UI_TallyAtlas.T_UI_TallyAtlas"));
		for (int32 Frame = 0; Frame < 6; ++Frame)
		{
			UImage* Image = WidgetTree->ConstructWidget<UImage>();
			FSlateBrush Brush;
			Brush.SetResourceObject(Atlas);
			Brush.ImageSize = FVector2D(96.f, 96.f);
			Image->SetBrush(Brush);
			Image->SetColorAndOpacity(Frame == 0 ? Colors.Lip : Colors.Text);
			Tally->AddChildToHorizontalBox(Image);
		}
		AddText(WidgetTree, Column, TEXT("0  1  2  3  4  5 / GHOST + FILL"), 20);
	}
	else
	{
		AddPrimitive(FMordecaiUIPrimitiveBuilder::Plate(
			Colors, EMordecaiUIPrintDepth::HUD, Theme), TEXT("GalleryPlate"), 760.f, 360.f);
		AddText(WidgetTree, Column, TEXT("PLATE / PRINT / OUTLINE / RIM / FACE"), 20);
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
