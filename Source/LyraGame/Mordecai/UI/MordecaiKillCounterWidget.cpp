// Project Mordecai — Kill Counter Widget (US-063)

#include "Mordecai/UI/MordecaiKillCounterWidget.h"
#include "Mordecai/MordecaiGameMode.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiKillCounterWidget)

// ---------------------------------------------------------------------------
// Programmatic Fallback (empty Blueprint support)
// ---------------------------------------------------------------------------

void UMordecaiKillCounterWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!KillCountText && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultContent();
	}
}

void UMordecaiKillCounterWidget::NativeDestruct()
{
	UnbindFromGameMode();
	Super::NativeDestruct();
}

void UMordecaiKillCounterWidget::BuildDefaultContent()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	KillCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KillCountText"));
	UCanvasPanelSlot* TextSlot = Root->AddChildToCanvas(KillCountText);
	TextSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	TextSlot->SetOffsets(FMargin(0.f));

	KillCountText->SetText(FText::FromString(FormatKillCount(0)));
	KillCountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	FSlateFontInfo FontInfo = KillCountText->GetFont();
	FontInfo.Size = 14;
	KillCountText->SetFont(FontInfo);
}

// ---------------------------------------------------------------------------
// Static Helper
// ---------------------------------------------------------------------------

FString UMordecaiKillCounterWidget::FormatKillCount(int32 Kills)
{
	return FString::Printf(TEXT("Kills: %d"), Kills);
}

// ---------------------------------------------------------------------------
// Authoritative Arena Count Binding
// ---------------------------------------------------------------------------

void UMordecaiKillCounterWidget::BindToASC(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		BindToGameMode(nullptr);
		return;
	}

	UWorld* World = GetWorld();
	BindToGameMode(World ? Cast<AMordecaiGameMode>(World->GetAuthGameMode()) : nullptr);
}

void UMordecaiKillCounterWidget::BindToGameMode(AMordecaiGameMode* GameMode)
{
	UnbindFromGameMode();
	BoundGameMode = GameMode;

	if (GameMode)
	{
		KillCountChangedHandle = GameMode->OnEnemyKillCountChanged().AddUObject(
			this, &UMordecaiKillCounterWidget::OnKillCountChanged);
		OnKillCountChanged(GameMode->GetEnemyKillCount());
	}
	else
	{
		OnKillCountChanged(0);
	}
}

void UMordecaiKillCounterWidget::UnbindFromGameMode()
{
	if (AMordecaiGameMode* GameMode = BoundGameMode.Get())
	{
		GameMode->OnEnemyKillCountChanged().Remove(KillCountChangedHandle);
	}
	KillCountChangedHandle.Reset();
	BoundGameMode.Reset();
}

void UMordecaiKillCounterWidget::OnKillCountChanged(int32 NewKillCount)
{
	KillCount = NewKillCount;
	UpdateDisplay();
}

void UMordecaiKillCounterWidget::UpdateDisplay()
{
	if (KillCountText)
	{
		KillCountText->SetText(FText::FromString(FormatKillCount(KillCount)));
	}
}
