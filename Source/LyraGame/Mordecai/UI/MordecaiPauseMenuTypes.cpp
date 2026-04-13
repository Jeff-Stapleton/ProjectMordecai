// Project Mordecai — Pause Menu Types (US-069)

#include "Mordecai/UI/MordecaiPauseMenuTypes.h"

#define LOCTEXT_NAMESPACE "MordecaiPauseMenu"

// ---------------------------------------------------------------------------
// FMordecaiPauseMenuTabModel
// ---------------------------------------------------------------------------

void FMordecaiPauseMenuTabModel::InitDefaults(TSubclassOf<UUserWidget> PlaceholderClass)
{
	Tabs.Reset();

	Tabs.Add({ FName("character"), LOCTEXT("Tab_Character", "Character"), PlaceholderClass });
	Tabs.Add({ FName("skills"),    LOCTEXT("Tab_Skills",    "Skills"),    PlaceholderClass });
	Tabs.Add({ FName("feats"),     LOCTEXT("Tab_Feats",     "Feats"),     PlaceholderClass });
	Tabs.Add({ FName("inventory"), LOCTEXT("Tab_Inventory", "Inventory"), PlaceholderClass });
	Tabs.Add({ FName("settings"),  LOCTEXT("Tab_Settings",  "Settings"),  PlaceholderClass });

	ActiveTabId = FName("character");
}

int32 FMordecaiPauseMenuTabModel::GetTabCount() const
{
	return Tabs.Num();
}

bool FMordecaiPauseMenuTabModel::HasTab(FName TabId) const
{
	return FindTab(TabId) != nullptr;
}

const FMordecaiPauseMenuTabEntry* FMordecaiPauseMenuTabModel::FindTab(FName TabId) const
{
	return Tabs.FindByPredicate([TabId](const FMordecaiPauseMenuTabEntry& Entry)
	{
		return Entry.TabId == TabId;
	});
}

void FMordecaiPauseMenuTabModel::RegisterTab(FName TabId, FText DisplayName, TSubclassOf<UUserWidget> ContentClass)
{
	for (FMordecaiPauseMenuTabEntry& Entry : Tabs)
	{
		if (Entry.TabId == TabId)
		{
			Entry.DisplayName = DisplayName;
			Entry.ContentWidgetClass = ContentClass;
			return;
		}
	}

	// Tab doesn't exist yet — add it
	Tabs.Add({ TabId, DisplayName, ContentClass });
}

bool FMordecaiPauseMenuTabModel::UnregisterTab(FName TabId)
{
	const int32 Removed = Tabs.RemoveAll([TabId](const FMordecaiPauseMenuTabEntry& Entry)
	{
		return Entry.TabId == TabId;
	});

	if (Removed > 0 && ActiveTabId == TabId)
	{
		// Active tab was removed — fall back to first tab
		ActiveTabId = Tabs.Num() > 0 ? Tabs[0].TabId : NAME_None;
	}

	return Removed > 0;
}

bool FMordecaiPauseMenuTabModel::SetActiveTab(FName TabId)
{
	if (!HasTab(TabId))
	{
		return false;
	}

	ActiveTabId = TabId;
	return true;
}

// ---------------------------------------------------------------------------
// FMordecaiPauseMenuState
// ---------------------------------------------------------------------------

bool FMordecaiPauseMenuState::Open()
{
	if (bIsOpen)
	{
		return false;
	}

	bIsOpen = true;
	return true;
}

bool FMordecaiPauseMenuState::Close(FName CurrentActiveTabId)
{
	if (!bIsOpen)
	{
		return false;
	}

	LastActiveTabId = CurrentActiveTabId;
	bIsOpen = false;
	return true;
}

bool FMordecaiPauseMenuState::Toggle(FName CurrentActiveTabId)
{
	return bIsOpen ? Close(CurrentActiveTabId) : Open();
}

bool FMordecaiPauseMenuState::ShouldBePaused() const
{
	return bIsOpen && bCanPauseGame;
}

#undef LOCTEXT_NAMESPACE
