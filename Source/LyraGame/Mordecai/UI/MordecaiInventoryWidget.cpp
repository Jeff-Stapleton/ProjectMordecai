// Project Mordecai — Inventory Widget (US-071)

#include "Mordecai/UI/MordecaiInventoryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Mordecai/Items/MordecaiIdentificationService.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiItemDefinition.h"
#include "Mordecai/Items/MordecaiItemLibrary.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/UI/MordecaiPauseMenuWidget.h"
#include "Mordecai/UI/MordecaiTabButton.h"

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

void UMordecaiInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	RefreshVisuals();
}

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
	RefreshVisuals();
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
	RefreshVisuals();
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
	RefreshVisuals();
}

void UMordecaiInventoryWidget::HandleResourceChanged(FName ItemId, int32 NewCount)
{
	RebuildLedgerCache();
	RefreshVisuals();
}

void UMordecaiInventoryWidget::HandleFilterButtonClicked(FName FilterId)
{
	// Map the button id back to the enum via FilterToName round-trip
	for (uint8 Value = 0; Value <= static_cast<uint8>(EMordecaiInventoryFilter::Magical); ++Value)
	{
		const EMordecaiInventoryFilter Filter = static_cast<EMordecaiInventoryFilter>(Value);
		if (FilterToName(Filter) == FilterId)
		{
			SetFilter(Filter);
			return;
		}
	}
}

void UMordecaiInventoryWidget::HandleIdentifyButtonClicked(FName InstanceIdString)
{
	FGuid InstanceId;
	if (FGuid::Parse(InstanceIdString.ToString(), InstanceId))
	{
		TryIdentify(InstanceId);
	}
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

// ---------------------------------------------------------------------------
// Programmatic fallback layout (US-079)
// ---------------------------------------------------------------------------

void UMordecaiInventoryWidget::BuildDefaultLayout()
{
	UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// --- Left column: filter bar over the item list ---
	UVerticalBox* ListColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ListColumn"));
	if (UHorizontalBoxSlot* ListColumnSlot = Root->AddChildToHorizontalBox(ListColumn))
	{
		ListColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ListColumnSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	FilterBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("FilterBar"));
	if (UVerticalBoxSlot* FilterSlot = ListColumn->AddChildToVerticalBox(FilterBar))
	{
		FilterSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	ItemListBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ItemListBox"));
	if (UVerticalBoxSlot* ListSlot = ListColumn->AddChildToVerticalBox(ItemListBox))
	{
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	ListPlaceholder = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ListPlaceholder"));
	ListPlaceholder->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.f)));
	ListColumn->AddChildToVerticalBox(ListPlaceholder);

	// --- Right column: auto-stored resource ledger ---
	UVerticalBox* LedgerColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LedgerColumn"));
	if (UHorizontalBoxSlot* LedgerColumnSlot = Root->AddChildToHorizontalBox(LedgerColumn))
	{
		LedgerColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	LedgerHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LedgerHeader"));
	LedgerHeader->SetText(LOCTEXT("LedgerHeader", "Stored Resources"));
	if (UVerticalBoxSlot* HeaderSlot = LedgerColumn->AddChildToVerticalBox(LedgerHeader))
	{
		HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	LedgerBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LedgerBox"));
	LedgerColumn->AddChildToVerticalBox(LedgerBox);

	LedgerPlaceholder = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LedgerPlaceholder"));
	LedgerPlaceholder->SetColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.f)));
	LedgerColumn->AddChildToVerticalBox(LedgerPlaceholder);
}

void UMordecaiInventoryWidget::RefreshVisuals()
{
	if (!WidgetTree || !ItemListBox)
	{
		return; // Headless (NewObject) widgets carry no visual tree — caches are the API
	}

	// --- Filter bar: one button per filter, active one highlighted ---
	if (FilterBar)
	{
		FilterBar->ClearChildren();
		for (uint8 Value = 0; Value <= static_cast<uint8>(EMordecaiInventoryFilter::Magical); ++Value)
		{
			const EMordecaiInventoryFilter Filter = static_cast<EMordecaiInventoryFilter>(Value);
			const bool bActive = Filter == ActiveFilter;

			UMordecaiTabButton* Button = WidgetTree->ConstructWidget<UMordecaiTabButton>(UMordecaiTabButton::StaticClass());
			Button->InitButton(FilterToName(Filter));
			Button->OnClickedWithId.AddUniqueDynamic(this, &UMordecaiInventoryWidget::HandleFilterButtonClicked);
			Button->SetBackgroundColor(bActive
				? FLinearColor(0.25f, 0.35f, 0.55f, 1.f)
				: FLinearColor(0.12f, 0.12f, 0.15f, 1.f));

			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Label->SetText(FText::FromName(FilterToName(Filter)));
			Label->SetColorAndOpacity(FSlateColor(bActive ? FLinearColor::White : FLinearColor(0.7f, 0.7f, 0.7f, 1.f)));
			Button->AddChild(Label);

			if (UHorizontalBoxSlot* ButtonSlot = FilterBar->AddChildToHorizontalBox(Button))
			{
				ButtonSlot->SetPadding(FMargin(2.f, 0.f));
			}
		}
	}

	// --- Item list: name (rarity-colored), quantity, "?" badge + identify button ---
	ItemListBox->ClearChildren();
	for (const FMordecaiInventoryRowModel& Row : GetVisibleRows())
	{
		UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		FString NameString = Row.DisplayName.ToString();
		if (Row.bShowQuantity)
		{
			NameString += FString::Printf(TEXT("  x%d"), Row.Quantity);
		}
		if (Row.bIsEquipped)
		{
			NameString += TEXT("  [equipped]");
		}
		if (Row.bIsUnidentified)
		{
			NameString = TEXT("? ") + NameString;
		}

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameText->SetText(FText::FromString(NameString));
		NameText->SetColorAndOpacity(FSlateColor(GetRarityColor(Row.Rarity)));
		if (UHorizontalBoxSlot* NameSlot = RowBox->AddChildToHorizontalBox(NameText))
		{
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NameSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (Row.bIsUnidentified)
		{
			UMordecaiTabButton* IdentifyButton = WidgetTree->ConstructWidget<UMordecaiTabButton>(UMordecaiTabButton::StaticClass());
			IdentifyButton->InitButton(FName(*Row.InstanceId.ToString()));
			IdentifyButton->OnClickedWithId.AddUniqueDynamic(this, &UMordecaiInventoryWidget::HandleIdentifyButtonClicked);

			UTextBlock* IdentifyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			IdentifyLabel->SetText(LOCTEXT("IdentifyButton", "Identify"));
			IdentifyButton->AddChild(IdentifyLabel);

			if (UHorizontalBoxSlot* IdentifySlot = RowBox->AddChildToHorizontalBox(IdentifyButton))
			{
				IdentifySlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
			}
		}

		ItemListBox->AddChild(RowBox);
	}

	if (ListPlaceholder)
	{
		const FText Placeholder = GetListPlaceholderText();
		ListPlaceholder->SetText(Placeholder);
		ListPlaceholder->SetVisibility(Placeholder.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	// --- Ledger panel ---
	if (LedgerBox)
	{
		LedgerBox->ClearChildren();
		for (const FMordecaiInventoryLedgerRow& LedgerRow : CachedLedgerRows)
		{
			UTextBlock* EntryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			EntryText->SetText(FText::Format(LOCTEXT("LedgerEntryFormat", "{0}  x{1}"),
				LedgerRow.DisplayName, FText::AsNumber(LedgerRow.Count)));
			LedgerBox->AddChildToVerticalBox(EntryText);
		}
	}

	if (LedgerPlaceholder)
	{
		const FText Placeholder = GetLedgerPlaceholderText();
		LedgerPlaceholder->SetText(Placeholder);
		LedgerPlaceholder->SetVisibility(Placeholder.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

#undef LOCTEXT_NAMESPACE
