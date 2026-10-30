// Project Mordecai — Loading Screen Widget (issue #39)

#include "Mordecai/UI/MordecaiLoadingScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Throbber.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiLoadingScreenWidget)

DEFINE_LOG_CATEGORY_STATIC(LogMordecaiLoadingScreen, Log, All);

#define LOCTEXT_NAMESPACE "MordecaiLoadingScreen"

namespace MordecaiLoadingScreen
{
	// TODO(DECISION): the loading screen's art (logo, key art, palette) has no direction yet: the game's name in the
	// engine's default font on a near-black screen
	static const FLinearColor BackgroundColor(0.010f, 0.010f, 0.012f, 1.f);
	static const FLinearColor TitleColor(0.86f, 0.80f, 0.68f, 1.f);
	static const FLinearColor StatusColor(0.55f, 0.55f, 0.55f, 1.f);
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

FText UMordecaiLoadingScreenWidget::GetTitleText()
{
	return LOCTEXT("Title", "Project Mordecai");
}

FText UMordecaiLoadingScreenWidget::GetStatusText()
{
	return LOCTEXT("Status", "Loading...");
}

TSubclassOf<UUserWidget> UMordecaiLoadingScreenWidget::GetConfiguredLoadingScreenWidgetClass()
{
	// UCommonLoadingScreenSettings is declared in its plugin's Private folder
	const UClass* SettingsClass = FindObject<UClass>(nullptr, TEXT("/Script/CommonLoadingScreen.CommonLoadingScreenSettings"));
	const FStructProperty* WidgetProperty = SettingsClass ? FindFProperty<FStructProperty>(SettingsClass, TEXT("LoadingScreenWidget")) : nullptr;
	if (!WidgetProperty || WidgetProperty->Struct != TBaseStructure<FSoftClassPath>::Get())
	{
		return nullptr;
	}
	const FSoftClassPath& WidgetPath = *WidgetProperty->ContainerPtrToValuePtr<FSoftClassPath>(SettingsClass->GetDefaultObject());
	return WidgetPath.TryLoadClass<UUserWidget>();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool UMordecaiLoadingScreenWidget::Initialize()
{
	const bool bInitializedNow = Super::Initialize();

	// Here and not in NativeOnInitialized, which needs a player context: the loading screen manager creates this from
	// the game instance, with a context only if the first local player has a player controller. Without a tree the
	// screen is blank.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
	return bInitializedNow;
}

void UMordecaiLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// The manager built this widget's Slate tree to put it on screen. Scripts/playtest_boot.py checks for this line.
	UE_LOG(LogMordecaiLoadingScreen, Log, TEXT("MordecaiLoadingScreen: shown (%s)"), *GetClass()->GetName());
}

void UMordecaiLoadingScreenWidget::BuildDefaultContent()
{
	using namespace MordecaiLoadingScreen;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// First child, so under everything, and opaque: the manager renders the world again while it holds the screen up
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrushColor(BackgroundColor);
	if (UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackgroundSlot->SetOffsets(FMargin(0.f));
	}

	// Centered column: the game's name / "Loading..." / a throbber
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MessageColumn"));
	if (UCanvasPanelSlot* ColumnSlot = Root->AddChildToCanvas(Column))
	{
		ColumnSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		ColumnSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ColumnSlot->SetAutoSize(true);
	}

	auto AddLine = [this, Column](FName WidgetName, const FText& Text, int32 FontSize, FName Typeface,
		int32 LetterSpacing, const FLinearColor& Color)
	{
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
		Line->SetText(Text);
		Line->SetJustification(ETextJustify::Center);
		Line->SetColorAndOpacity(FSlateColor(Color));
		// The engine's default font (Roboto, an engine startup package): nothing to cook
		FSlateFontInfo FontInfo = Line->GetFont();
		FontInfo.Size = FontSize;
		FontInfo.TypefaceFontName = Typeface;
		FontInfo.LetterSpacing = LetterSpacing;
		Line->SetFont(FontInfo);
		if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(Line))
		{
			LineSlot->SetHorizontalAlignment(HAlign_Center);
			LineSlot->SetPadding(FMargin(0.f, 6.f));
		}
		return Line;
	};
	TitleText = AddLine(TEXT("TitleText"), GetTitleText(), 56, TEXT("Bold"), 150, TitleColor);
	TitleText->SetTextTransformPolicy(ETextTransformPolicy::ToUpper);
	StatusText = AddLine(TEXT("StatusText"), GetStatusText(), 20, TEXT("Regular"), 0, StatusColor);

	UThrobber* Throbber = WidgetTree->ConstructWidget<UThrobber>(UThrobber::StaticClass(), TEXT("LoadingThrobber"));
	FSlateBrush ThrobberBrush = Throbber->GetImage();
	ThrobberBrush.TintColor = FSlateColor(StatusColor);
	Throbber->SetImage(ThrobberBrush);
	if (UVerticalBoxSlot* ThrobberSlot = Column->AddChildToVerticalBox(Throbber))
	{
		ThrobberSlot->SetHorizontalAlignment(HAlign_Center);
		ThrobberSlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
}

#undef LOCTEXT_NAMESPACE
