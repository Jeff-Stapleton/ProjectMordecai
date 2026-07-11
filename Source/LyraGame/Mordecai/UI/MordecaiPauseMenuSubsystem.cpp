// Project Mordecai — Pause Menu Subsystem (US-069)

#include "Mordecai/UI/MordecaiPauseMenuSubsystem.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "PrimaryGameLayout.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPauseMenuSubsystem)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Mordecai_UI_Layer_Menu, "UI.Layer.Menu");

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UMordecaiPauseMenuSubsystem::TogglePauseMenu()
{
	FName CurrentTab = NAME_None;
	if (ActiveMenuWidget.IsValid())
	{
		CurrentTab = ActiveMenuWidget->GetActiveTabId();
	}

	if (MenuState.Toggle(CurrentTab))
	{
		if (MenuState.bIsOpen)
		{
			if (bCanPauseGame)
			{
				UGameplayStatics::SetGamePaused(this, true);
			}
			PushMenuWidget();
		}
		else
		{
			if (bCanPauseGame)
			{
				UGameplayStatics::SetGamePaused(this, false);
			}
			PopMenuWidget();
		}
	}
}

void UMordecaiPauseMenuSubsystem::TogglePauseMenuToTab(FName TabId)
{
	if (MenuState.bIsOpen)
	{
		ClosePauseMenu();
		return;
	}

	// Land on the requested tab instead of the remembered one (US-079)
	MenuState.LastActiveTabId = TabId;
	OpenPauseMenu();
	if (ActiveMenuWidget.IsValid() && TabId != NAME_None)
	{
		ActiveMenuWidget->SetActiveTab(TabId);
	}
}

void UMordecaiPauseMenuSubsystem::OpenPauseMenu()
{
	if (!MenuState.Open())
	{
		return; // Already open — no-op (AC-069.12)
	}

	MenuState.bCanPauseGame = bCanPauseGame;

	if (bCanPauseGame)
	{
		UGameplayStatics::SetGamePaused(this, true);
	}

	PushMenuWidget();
}

void UMordecaiPauseMenuSubsystem::ClosePauseMenu()
{
	FName CurrentTab = NAME_None;
	if (ActiveMenuWidget.IsValid())
	{
		CurrentTab = ActiveMenuWidget->GetActiveTabId();
	}

	if (!MenuState.Close(CurrentTab))
	{
		return; // Already closed — no-op (AC-069.12)
	}

	if (bCanPauseGame)
	{
		UGameplayStatics::SetGamePaused(this, false);
	}

	PopMenuWidget();
}

bool UMordecaiPauseMenuSubsystem::IsMenuOpen() const
{
	return MenuState.bIsOpen;
}

FName UMordecaiPauseMenuSubsystem::GetLastActiveTabId() const
{
	return MenuState.LastActiveTabId;
}

// ---------------------------------------------------------------------------
// Widget lifecycle
// ---------------------------------------------------------------------------

void UMordecaiPauseMenuSubsystem::PushMenuWidget()
{
	UPrimaryGameLayout* Layout = UPrimaryGameLayout::GetPrimaryGameLayoutForPrimaryPlayer(this);
	if (!Layout)
	{
		return;
	}

	UMordecaiPauseMenuWidget* Widget = Layout->PushWidgetToLayerStack<UMordecaiPauseMenuWidget>(
		TAG_Mordecai_UI_Layer_Menu,
		UMordecaiPauseMenuWidget::StaticClass());

	if (Widget)
	{
		// Restore last active tab (AC-069.11)
		if (MenuState.LastActiveTabId != NAME_None)
		{
			Widget->SetActiveTab(MenuState.LastActiveTabId);
		}

		ActiveMenuWidget = Widget;
	}
}

void UMordecaiPauseMenuSubsystem::PopMenuWidget()
{
	if (ActiveMenuWidget.IsValid())
	{
		ActiveMenuWidget->DeactivateWidget();
		ActiveMenuWidget.Reset();
	}
}
