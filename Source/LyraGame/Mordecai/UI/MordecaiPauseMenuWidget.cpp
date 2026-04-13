// Project Mordecai — Pause Menu Widget (US-069)

#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuPlaceholderWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPauseMenuWidget)

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiPauseMenuWidget::UMordecaiPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Menu input mode — blocks game input, shows mouse cursor (AC-069.1)
	InputConfig = ELyraWidgetInputMode::Menu;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	InitDefaultTabs();
}

void UMordecaiPauseMenuWidget::NativeOnDeactivated()
{
	// When CommonUI back action deactivates this widget, notify the subsystem (AC-069.10)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMordecaiPauseMenuSubsystem* Sub = GI->GetSubsystem<UMordecaiPauseMenuSubsystem>())
		{
			Sub->ClosePauseMenu();
		}
	}

	Super::NativeOnDeactivated();
}

// ---------------------------------------------------------------------------
// Tab API — delegates to TabModel
// ---------------------------------------------------------------------------

void UMordecaiPauseMenuWidget::RegisterTab(FName TabId, FText TabDisplayName, TSubclassOf<UUserWidget> ContentWidgetClass)
{
	TabModel.RegisterTab(TabId, TabDisplayName, ContentWidgetClass);
	RebuildContentArea();
}

void UMordecaiPauseMenuWidget::UnregisterTab(FName TabId)
{
	TabModel.UnregisterTab(TabId);
	RebuildContentArea();
}

FName UMordecaiPauseMenuWidget::GetActiveTabId() const
{
	return TabModel.ActiveTabId;
}

void UMordecaiPauseMenuWidget::SetActiveTab(FName TabId)
{
	TabModel.SetActiveTab(TabId);
	RebuildContentArea();
}

int32 UMordecaiPauseMenuWidget::GetTabCount() const
{
	return TabModel.GetTabCount();
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void UMordecaiPauseMenuWidget::InitDefaultTabs()
{
	TabModel.InitDefaults(UMordecaiPauseMenuPlaceholderWidget::StaticClass());
}

void UMordecaiPauseMenuWidget::RebuildContentArea()
{
	// Visual rebuild is handled in BP or PIE — headless tests don't render.
	// The tab model is the source of truth; the subsystem reads ActiveTabId.
}
