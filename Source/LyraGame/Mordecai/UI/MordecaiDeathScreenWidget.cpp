// Project Mordecai — Death Screen Widget (US-062)

#include "Mordecai/UI/MordecaiDeathScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiDeathScreenWidget)

namespace MordecaiDeathScreen
{
	static const TCHAR* const DeathMessage = TEXT("You Died");
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiDeathScreenWidget::UMordecaiDeathScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// GameAndMenu so respawn flow is not blocked by modal input capture
	InputConfig = ELyraWidgetInputMode::GameAndMenu;

	// The viewport keeps this capture mode after the screen goes (CommonUI restores none): the project's default
	// (DefaultInput.ini), so a click that refocuses the game still attacks after a death
	GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiDeathScreenWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Pushed by class, the widget has no designer tree: build one, or the screen is blank (US-086)
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiDeathScreenWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	// An overlay: clicks go through to the game view
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Root;

	// Dim the game behind the message
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundDim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	// Centered column: "You Died" / countdown / kills. The texts sit in this widget's own tree, where the agent
	// bridge reads them.
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MessageColumn"));
	if (UCanvasPanelSlot* ColumnSlot = Root->AddChildToCanvas(Column))
	{
		ColumnSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		ColumnSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ColumnSlot->SetAutoSize(true);
	}

	auto AddLine = [this, Column](FName WidgetName, int32 FontSize, const FLinearColor& Color)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(Color));
		FSlateFontInfo FontInfo = Text->GetFont();
		FontInfo.Size = FontSize;
		Text->SetFont(FontInfo);
		if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(Text))
		{
			LineSlot->SetHorizontalAlignment(HAlign_Center);
			LineSlot->SetPadding(FMargin(0.f, 4.f));
		}
		return Text;
	};
	DeathText = AddLine(TEXT("DeathText"), 48, FLinearColor(0.8f, 0.1f, 0.1f));
	CountdownText = AddLine(TEXT("CountdownText"), 32, FLinearColor::White);
	KillCountText = AddLine(TEXT("KillCountText"), 20, FLinearColor::White);

	DeathText->SetText(FText::FromString(MordecaiDeathScreen::DeathMessage));
}

// ---------------------------------------------------------------------------
// Static Helpers
// ---------------------------------------------------------------------------

FString UMordecaiDeathScreenWidget::FormatRespawnCountdown(float TimeRemaining)
{
	if (TimeRemaining <= 0.f)
	{
		return FString();
	}
	const int32 CeilSeconds = FMath::CeilToInt32(TimeRemaining);
	return FString::Printf(TEXT("%d"), CeilSeconds);
}

FString UMordecaiDeathScreenWidget::FormatKillCount(int32 Kills)
{
	return FString::Printf(TEXT("Kills: %d"), Kills);
}

// ---------------------------------------------------------------------------
// Instance Methods
// ---------------------------------------------------------------------------

void UMordecaiDeathScreenWidget::InitializeDeathScreen(float RespawnDelay, int32 KillCount)
{
	RemainingRespawnTime = RespawnDelay;
	CachedKillCount = KillCount;

	if (DeathText)
	{
		DeathText->SetText(FText::FromString(MordecaiDeathScreen::DeathMessage));
	}

	if (KillCountText)
	{
		KillCountText->SetText(FText::FromString(FormatKillCount(CachedKillCount)));
	}

	UpdateCountdownDisplay();
}

// ---------------------------------------------------------------------------
// Tick — countdown
// ---------------------------------------------------------------------------

void UMordecaiDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TickCountdown(InDeltaTime);
}

void UMordecaiDeathScreenWidget::TickCountdown(float DeltaSeconds)
{
	// The respawn is a world timer, which stops while the game is paused (the pause menu opens while dead): so does
	// the countdown, which Slate ticks on regardless
	const UWorld* World = GetWorld();
	if (RemainingRespawnTime <= 0.f || (World && World->IsPaused()))
	{
		return;
	}

	RemainingRespawnTime -= DeltaSeconds;
	UpdateCountdownDisplay();
}

void UMordecaiDeathScreenWidget::UpdateCountdownDisplay()
{
	if (CountdownText)
	{
		CountdownText->SetText(FText::FromString(FormatRespawnCountdown(RemainingRespawnTime)));
	}
}
