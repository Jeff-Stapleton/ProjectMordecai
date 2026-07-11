// Project Mordecai — Inventory Widget (US-071)

#include "Mordecai/UI/MordecaiInventoryWidget.h"

#include "Engine/GameInstance.h"
#include "Mordecai/Items/MordecaiIdentificationService.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiInventoryWidget)

#define LOCTEXT_NAMESPACE "MordecaiInventory"

// ---------------------------------------------------------------------------
// Tab registration
// ---------------------------------------------------------------------------

FText UMordecaiInventoryWidget::GetInventoryTabDisplayName()
{
	return LOCTEXT("InventoryTabName", "Inventory");
}

void UMordecaiInventoryWidget::RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiInventoryWidget> WidgetClass)
{
	if (!PauseMenu)
	{
		return;
	}
	TSubclassOf<UUserWidget> ContentClass = WidgetClass ? *WidgetClass : UMordecaiInventoryWidget::StaticClass();
	PauseMenu->RegisterTab(GetInventoryTabId(), GetInventoryTabDisplayName(), ContentClass);
}

// ---------------------------------------------------------------------------
// Rarity colors
// ---------------------------------------------------------------------------

FLinearColor UMordecaiInventoryWidget::GetRarityColor(EMordecaiItemRarity Rarity)
{
	switch (Rarity)
	{
	case EMordecaiItemRarity::Common: return RarityColorCommon;
	case EMordecaiItemRarity::Green:  return RarityColorGreen;
	case EMordecaiItemRarity::Blue:   return RarityColorBlue;
	case EMordecaiItemRarity::Purple: return RarityColorPurple;
	case EMordecaiItemRarity::Red:    return RarityColorRed;
	case EMordecaiItemRarity::Gold:   return RarityColorGold;
	default:                          return RarityColorCommon;
	}
}

// ---------------------------------------------------------------------------
// Filter helpers
// ---------------------------------------------------------------------------

FName UMordecaiInventoryWidget::FilterToName(EMordecaiInventoryFilter Filter)
{
	switch (Filter)
	{
	case EMordecaiInventoryFilter::All:         return FName(TEXT("All"));
	case EMordecaiInventoryFilter::Weapons:     return FName(TEXT("Weapons"));
	case EMordecaiInventoryFilter::Armor:       return FName(TEXT("Armor"));
	case EMordecaiInventoryFilter::Trinkets:    return FName(TEXT("Trinkets"));
	case EMordecaiInventoryFilter::Consumables: return FName(TEXT("Consumables"));
	case EMordecaiInventoryFilter::Materials:   return FName(TEXT("Materials"));
	case EMordecaiInventoryFilter::Quest:       return FName(TEXT("Quest"));
	case EMordecaiInventoryFilter::Magical:     return FName(TEXT("Magical"));
	default:                                    return NAME_None;
	}
}

bool UMordecaiInventoryWidget::FilterMatchesType(EMordecaiInventoryFilter Filter, EMordecaiItemType Type)
{
	switch (Filter)
	{
	case EMordecaiInventoryFilter::All:
		return true;
	case EMordecaiInventoryFilter::Weapons:
		return Type == EMordecaiItemType::Weapon;
	case EMordecaiInventoryFilter::Armor:
		return Type == EMordecaiItemType::Armor;
	case EMordecaiInventoryFilter::Trinkets:
		return Type == EMordecaiItemType::Trinket;
	case EMordecaiInventoryFilter::Consumables:
		return Type == EMordecaiItemType::Consumable;
	case EMordecaiInventoryFilter::Materials:
		return Type == EMordecaiItemType::Material || Type == EMordecaiItemType::TownResource;
	case EMordecaiInventoryFilter::Quest:
		return Type == EMordecaiItemType::QuestItem;
	case EMordecaiInventoryFilter::Magical:
		return Type == EMordecaiItemType::MagicalItem || Type == EMordecaiItemType::UpgradeKey;
	default:
		return false;
	}
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiInventoryWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Binding
// ---------------------------------------------------------------------------

void UMordecaiInventoryWidget::BindToInventory(UMordecaiInventoryComponent* Inventory, UMordecaiResourceLedger* Ledger)
{
	// Rebind-safe: drop any existing subscriptions first.
	Unbind();

	if (Inventory)
	{
		BoundInventory = Inventory;
		Inventory->OnInventoryChanged.AddDynamic(this, &UMordecaiInventoryWidget::HandleInventoryChanged);
	}

	if (Ledger)
	{
		BoundLedger = Ledger;
		Ledger->OnResourceChanged.AddDynamic(this, &UMordecaiInventoryWidget::HandleResourceChanged);
	}

	RebuildRowCache();
	RebuildLedgerCache();
}

void UMordecaiInventoryWidget::Unbind()
{
	// Clears subscriptions only — the row caches intentionally survive so the
	// last-known state remains displayable after the source goes away.
	if (UMordecaiInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UMordecaiInventoryWidget::HandleInventoryChanged);
	}
	BoundInventory.Reset();

	if (UMordecaiResourceLedger* Ledger = BoundLedger.Get())
	{
		Ledger->OnResourceChanged.RemoveDynamic(this, &UMordecaiInventoryWidget::HandleResourceChanged);
	}
	BoundLedger.Reset();
}

// ---------------------------------------------------------------------------
// Filter state
// ---------------------------------------------------------------------------

void UMordecaiInventoryWidget::SetFilter(EMordecaiInventoryFilter NewFilter)
{
	ActiveFilter = NewFilter;
	OnFilterChanged.Broadcast(FilterToName(NewFilter));
}

// ---------------------------------------------------------------------------
// Inventory list
// ---------------------------------------------------------------------------

TArray<FMordecaiInventoryRowModel> UMordecaiInventoryWidget::GetVisibleRows() const
{
	TArray<FMordecaiInventoryRowModel> Result;
	Result.Reserve(CachedRows.Num());
	for (const FMordecaiInventoryRowModel& Row : CachedRows)
	{
		if (FilterMatchesType(ActiveFilter, Row.Type))
		{
			Result.Add(Row);
		}
	}
	return Result;
}

int32 UMordecaiInventoryWidget::GetVisibleRowCount() const
{
	int32 Count = 0;
	for (const FMordecaiInventoryRowModel& Row : CachedRows)
	{
		if (FilterMatchesType(ActiveFilter, Row.Type))
		{
			++Count;
		}
	}
	return Count;
}

// ---------------------------------------------------------------------------
// Placeholder text
// ---------------------------------------------------------------------------

FText UMordecaiInventoryWidget::GetListPlaceholderText() const
{
	if (!BoundInventory.IsValid())
	{
		return LOCTEXT("ListUnbound", "--");
	}
	if (CachedRows.Num() == 0 && CachedLedgerRows.Num() == 0)
	{
		return LOCTEXT("ListEmpty", "Inventory is empty.");
	}
	if (CachedRows.Num() > 0 && GetVisibleRowCount() == 0)
	{
		return LOCTEXT("ListFiltered", "No items match this filter.");
	}
	return FText::GetEmpty();
}

FText UMordecaiInventoryWidget::GetLedgerPlaceholderText() const
{
	if (!BoundLedger.IsValid())
	{
		return LOCTEXT("LedgerUnbound", "--");
	}
	if (CachedLedgerRows.Num() == 0)
	{
		return LOCTEXT("LedgerEmpty", "No resources stored.");
	}
	return FText::GetEmpty();
}

// ---------------------------------------------------------------------------
// Identify action
// ---------------------------------------------------------------------------

bool UMordecaiInventoryWidget::TryIdentify(const FGuid& InstanceId)
{
	UMordecaiInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return false;
	}

	const FMordecaiItemInstance* Instance = Inventory->FindInstance(InstanceId);
	if (!Instance || !Instance->ItemDefinition)
	{
		return false;
	}
	if (!Instance->ItemDefinition->UsesIdentification || Instance->IsIdentified())
	{
		return false;
	}

	UMordecaiIdentificationService* Service = ResolveIdentificationService();
	if (!Service)
	{
		return false;
	}

	// The service flips state through the component, whose OnInventoryChanged
	// (delta=0) triggers the row rebuild — no manual refresh here.
	return Service->IdentifyInstance(Inventory, InstanceId);
}

void UMordecaiInventoryWidget::SetIdentificationServiceOverride(UMordecaiIdentificationService* InService)
{
	ServiceOverride = InService;
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void UMordecaiInventoryWidget::HandleInventoryChanged(const FGuid& InstanceId, int32 QuantityDelta)
{
	RebuildRowCache();
}

void UMordecaiInventoryWidget::HandleResourceChanged(FName ItemId, int32 NewCount)
{
	RebuildLedgerCache();
}

void UMordecaiInventoryWidget::RebuildRowCache()
{
	CachedRows.Reset();

	const UMordecaiInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory)
	{
		return;
	}

	const TArray<FMordecaiItemInstance> Sorted = Inventory->GetSortedItems();
	CachedRows.Reserve(Sorted.Num());
	for (const FMordecaiItemInstance& Instance : Sorted)
	{
		const UMordecaiItemDefinition* Def = Instance.ItemDefinition;
		if (!Def)
		{
			continue;
		}

		FMordecaiInventoryRowModel Row;
		Row.InstanceId = Instance.InstanceId;
		Row.DisplayName = UMordecaiItemLibrary::GetDisplayName(Instance);
		Row.Description = UMordecaiItemLibrary::GetDescription(Instance);
		Row.Quantity = Instance.Quantity;
		Row.bShowQuantity = Def->IsStackable() && Instance.Quantity > 1;
		Row.Type = Def->ItemType;
		Row.Rarity = Def->Rarity;
		Row.bIsUnidentified = Def->UsesIdentification && !Instance.IsIdentified();
		Row.bIsEquipped = Instance.IsEquipped;
		Row.Icon = Def->Icon;
		CachedRows.Add(MoveTemp(Row));
	}
}

void UMordecaiInventoryWidget::RebuildLedgerCache()
{
	CachedLedgerRows.Reset();

	const UMordecaiResourceLedger* Ledger = BoundLedger.Get();
	if (!Ledger)
	{
		return;
	}

	const TArray<FMordecaiResourceEntry> Entries = Ledger->GetAllResources();
	CachedLedgerRows.Reserve(Entries.Num());
	for (const FMordecaiResourceEntry& Entry : Entries)
	{
		FMordecaiInventoryLedgerRow Row;
		Row.ItemId = Entry.ItemId;
		Row.DisplayName = Entry.Def ? Entry.Def->DisplayName : FText::FromName(Entry.ItemId);
		Row.Count = Entry.Count;
		CachedLedgerRows.Add(MoveTemp(Row));
	}

	CachedLedgerRows.Sort([](const FMordecaiInventoryLedgerRow& A, const FMordecaiInventoryLedgerRow& B)
	{
		return A.DisplayName.ToString().Compare(B.DisplayName.ToString(), ESearchCase::IgnoreCase) < 0;
	});
}

UMordecaiIdentificationService* UMordecaiInventoryWidget::ResolveIdentificationService() const
{
	if (UMordecaiIdentificationService* Override = ServiceOverride.Get())
	{
		return Override;
	}

	// UUserWidget::GetGameInstance() returns null (no crash) when the widget
	// has no world — headless test widgets resolve to no service here.
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UMordecaiIdentificationService>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
