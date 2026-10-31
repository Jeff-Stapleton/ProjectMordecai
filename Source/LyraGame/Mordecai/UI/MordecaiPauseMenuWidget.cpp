// Project Mordecai — Pause Menu Widget (US-069, visuals + inventory tab US-079)

#include "Mordecai/UI/MordecaiPauseMenuWidget.h"

#include "AbilitySystemInterface.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Input/CommonUIInputTypes.h"
#include "Mordecai/Items/MordecaiInventoryComponent.h"
#include "Mordecai/Items/MordecaiResourceLedger.h"
#include "Mordecai/MordecaiGameplayTags.h"
#include "Mordecai/UI/MordecaiCharacterSheetWidget.h"
#include "Mordecai/UI/MordecaiInventoryWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuPlaceholderWidget.h"
#include "Mordecai/UI/MordecaiPauseMenuSubsystem.h"
#include "Mordecai/UI/MordecaiTabButton.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MordecaiPauseMenuWidget)

#define LOCTEXT_NAMESPACE "MordecaiPauseMenu"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

UMordecaiPauseMenuWidget::UMordecaiPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Menu input mode — blocks game input, shows mouse cursor (AC-069.1)
	InputConfig = ELyraWidgetInputMode::Menu;

	// Menu mode blocks every key without a UI action binding, so the Escape / gamepad-back that closes
	// the menu must be CommonUI's default back action (DT_UniversalActions DefaultBack: Escape,
	// Gamepad_FaceButton_Right), registered in NativeConstruct for back handlers. Its default handling
	// deactivates the widget, and NativeOnDeactivated closes the menu through the subsystem (AC-069.10).
	bIsBackHandler = true;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UMordecaiPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	InitializeTabs();
	EnsureDefaultLayout();
	RebuildTabBar();
	RebuildContentArea();

	// Registered once (Lyra's pattern, LyraHUDLayout): the handles live as long as this pooled widget, and
	// UCommonUserWidget re-attaches them to the action router on every Slate rebuild (each reopen). Registering
	// again in NativeConstruct would duplicate them. No action router headless: the handles stay invalid there.
	for (const FBindUIActionArgs& BindArgs : MakeUIActionBindingArgs())
	{
		RegisterUIActionBinding(BindArgs);
	}
}

void UMordecaiPauseMenuWidget::InitializeTabs()
{
	InitDefaultTabs();

	// Replace placeholders with real content widgets:
	// "character" tab — character sheet (US-066)
	UMordecaiCharacterSheetWidget::RegisterWithPauseMenu(this);
	// "inventory" tab — inventory flat list + ledger (US-071/US-079)
	UMordecaiInventoryWidget::RegisterWithPauseMenu(this);
}

void UMordecaiPauseMenuWidget::NativeOnActivated()
{
	// CommonUI pools this widget on UI.Layer.Menu: a reopen reuses this instance without NativeOnInitialized, and
	// SetActiveTab keeps the content instance of an unchanged tab. Rebind the active tab to the current pawn on
	// every activation, or a reopened inventory shows the last open's rows (US-094, issue #28).
	BindTabContent(CurrentContentWidget);

	// Only an I press made in this open menu closes it (US-095)
	bInventoryCloseArmed = false;

	Super::NativeOnActivated();
}

void UMordecaiPauseMenuWidget::NativeOnDeactivated()
{
	bInventoryCloseArmed = false;

	// A closed menu does not listen to the pawn; the next activation rebinds (US-094)
	UnbindTabContent(CurrentContentWidget);

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
	RebuildTabBar();
	RebuildContentArea();
}

void UMordecaiPauseMenuWidget::UnregisterTab(FName TabId)
{
	TabModel.UnregisterTab(TabId);
	RebuildTabBar();
	RebuildContentArea();
}

FName UMordecaiPauseMenuWidget::GetActiveTabId() const
{
	return TabModel.ActiveTabId;
}

void UMordecaiPauseMenuWidget::SetActiveTab(FName TabId)
{
	TabModel.SetActiveTab(TabId);
	RebuildTabBar();
	RebuildContentArea();
}

int32 UMordecaiPauseMenuWidget::GetTabCount() const
{
	return TabModel.GetTabCount();
}

void UMordecaiPauseMenuWidget::SelectPreviousTab()
{
	if (TabModel.CycleActiveTab(-1))
	{
		RebuildTabBar();
		RebuildContentArea();
	}
}

void UMordecaiPauseMenuWidget::SelectNextTab()
{
	if (TabModel.CycleActiveTab(1))
	{
		RebuildTabBar();
		RebuildContentArea();
	}
}

// ---------------------------------------------------------------------------
// CommonUI UI actions (US-095)
// ---------------------------------------------------------------------------

TArray<FBindUIActionArgs> UMordecaiPauseMenuWidget::MakeUIActionBindingArgs()
{
	// Defaults: fire on IE_Pressed, Menu input mode, consume the key, not persistent (live only while this menu is the
	// active root). Keys come from the tags' CommonUIInputSettings InputActions entries (Config/DefaultInput.ini); no
	// action bar shows them.
	TArray<FBindUIActionArgs> Args;
	Args.Emplace(FUIActionTag::ConvertChecked(MordecaiGameplayTags::UIAction_PauseMenu_PreviousTab), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::SelectPreviousTab));
	Args.Emplace(FUIActionTag::ConvertChecked(MordecaiGameplayTags::UIAction_PauseMenu_NextTab), false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::SelectNextTab));

	// I closes on the key-up of a press made in the open menu. Closing on the press would let the OS repeats of a held
	// I reach the game once the menu is gone, where the Inventory action's Started reopens it. Closing on any key-up
	// would close the menu on the key-up of the I press that opened it.
	const FUIActionTag InventoryTag = FUIActionTag::ConvertChecked(MordecaiGameplayTags::UIAction_PauseMenu_Inventory);
	Args.Emplace(InventoryTag, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleInventoryPressed));
	FBindUIActionArgs& InventoryReleased = Args.Emplace_GetRef(InventoryTag, false,
		FSimpleDelegate::CreateUObject(this, &ThisClass::HandleInventoryReleased));
	InventoryReleased.KeyEvent = IE_Released;
	return Args;
}

TArray<FName> UMordecaiPauseMenuWidget::GetUIActionBindingNames() const
{
	TArray<FName> Names;
	for (const FUIActionBindingHandle& Handle : GetActionBindings())
	{
		if (Handle.IsValid())
		{
			Names.Add(Handle.GetActionName());
		}
	}
	return Names;
}

void UMordecaiPauseMenuWidget::HandleInventoryPressed()
{
	bInventoryCloseArmed = true;
}

void UMordecaiPauseMenuWidget::HandleInventoryReleased()
{
	if (!bInventoryCloseArmed)
	{
		return; // the key-up of the I press that opened the menu
	}
	bInventoryCloseArmed = false;

	// What the default back action does (UCommonActivatableWidget::NativeOnHandleBackAction): NativeOnDeactivated
	// then closes the menu through the subsystem (unpause, remember the tab, pop)
	DeactivateWidget();
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void UMordecaiPauseMenuWidget::HandleTabButtonClicked(FName TabId)
{
	SetActiveTab(TabId);
}

void UMordecaiPauseMenuWidget::InitDefaultTabs()
{
	TabModel.InitDefaults(UMordecaiPauseMenuPlaceholderWidget::StaticClass());
}

void UMordecaiPauseMenuWidget::EnsureDefaultLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return; // Blueprint provided a tree — BindWidgetOptional members drive it
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// Dim the game behind the menu
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundDim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.65f));
	if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	// Centered menu column: title / tab bar / content
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuColumn"));
	if (UCanvasPanelSlot* ColumnSlot = Root->AddChildToCanvas(Column))
	{
		ColumnSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		ColumnSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ColumnSlot->SetAutoSize(false);
		ColumnSlot->SetSize(FVector2D(1000.f, 640.f));
		ColumnSlot->SetPosition(FVector2D::ZeroVector);
	}

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(LOCTEXT("PauseMenuTitle", "Paused"));
	TitleText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	TabBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabBar"));
	if (UVerticalBoxSlot* TabBarSlot = Column->AddChildToVerticalBox(TabBar))
	{
		TabBarSlot->SetHorizontalAlignment(HAlign_Center);
		TabBarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	TabContentBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TabContentBorder"));
	TabContentBorder->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.08f, 0.9f));
	TabContentBorder->SetPadding(FMargin(16.f));
	if (UVerticalBoxSlot* ContentSlot = Column->AddChildToVerticalBox(TabContentBorder))
	{
		ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UMordecaiPauseMenuWidget::RebuildTabBar()
{
	if (!WidgetTree || !TabBar)
	{
		return; // Headless (NewObject) path — model only
	}

	TabBar->ClearChildren();
	for (const FMordecaiPauseMenuTabEntry& Tab : TabModel.Tabs)
	{
		UMordecaiTabButton* Button = WidgetTree->ConstructWidget<UMordecaiTabButton>(UMordecaiTabButton::StaticClass());
		Button->InitButton(Tab.TabId);
		Button->OnClickedWithId.AddUniqueDynamic(this, &UMordecaiPauseMenuWidget::HandleTabButtonClicked);

		const bool bActive = Tab.TabId == TabModel.ActiveTabId;
		Button->SetBackgroundColor(bActive
			? FLinearColor(0.25f, 0.35f, 0.55f, 1.f)
			: FLinearColor(0.12f, 0.12f, 0.15f, 1.f));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Tab.DisplayName);
		Label->SetColorAndOpacity(FSlateColor(bActive ? FLinearColor::White : FLinearColor(0.7f, 0.7f, 0.7f, 1.f)));
		Button->AddChild(Label);

		if (UHorizontalBoxSlot* ButtonSlot = TabBar->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(4.f, 0.f));
		}
	}
}

void UMordecaiPauseMenuWidget::RebuildContentArea()
{
	if (!WidgetTree || !TabContentBorder)
	{
		return; // Headless (NewObject) path — the tab model stays the source of truth
	}

	const FMordecaiPauseMenuTabEntry* ActiveTab = TabModel.FindTab(TabModel.ActiveTabId);
	if (!ActiveTab || !ActiveTab->ContentWidgetClass)
	{
		UnbindTabContent(CurrentContentWidget);
		TabContentBorder->ClearChildren();
		CurrentContentWidget = nullptr;
		return;
	}

	// Keep the existing instance when the class is unchanged (tab re-click)
	if (CurrentContentWidget && CurrentContentWidget->GetClass() == *ActiveTab->ContentWidgetClass)
	{
		return;
	}

	UnbindTabContent(CurrentContentWidget); // the replaced tab stops listening
	TabContentBorder->ClearChildren();
	CurrentContentWidget = CreateWidget<UUserWidget>(this, ActiveTab->ContentWidgetClass);
	if (CurrentContentWidget)
	{
		TabContentBorder->SetContent(CurrentContentWidget);
		BindTabContent(CurrentContentWidget);
	}
}

void UMordecaiPauseMenuWidget::BindTabContent(UUserWidget* ContentWidget)
{
	APlayerController* OwningPC = GetOwningPlayer();
	APawn* OwningPawn = OwningPC ? OwningPC->GetPawn() : nullptr;

	if (UMordecaiInventoryWidget* Inventory = Cast<UMordecaiInventoryWidget>(ContentWidget))
	{
		UMordecaiInventoryComponent* InventoryComp = OwningPawn ? OwningPawn->FindComponentByClass<UMordecaiInventoryComponent>() : nullptr;
		UMordecaiResourceLedger* Ledger = OwningPawn ? OwningPawn->FindComponentByClass<UMordecaiResourceLedger>() : nullptr;
		Inventory->BindToInventory(InventoryComp, Ledger);
	}
	else if (UMordecaiCharacterSheetWidget* CharacterSheet = Cast<UMordecaiCharacterSheetWidget>(ContentWidget))
	{
		// ASC lives on the PlayerState (Lyra pattern)
		UAbilitySystemComponent* ASC = nullptr;
		if (APlayerState* PS = OwningPC ? OwningPC->PlayerState : nullptr)
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
			{
				ASC = ASI->GetAbilitySystemComponent();
			}
		}
		CharacterSheet->BindToASC(ASC);
	}
}

void UMordecaiPauseMenuWidget::UnbindTabContent(UUserWidget* ContentWidget)
{
	if (UMordecaiInventoryWidget* Inventory = Cast<UMordecaiInventoryWidget>(ContentWidget))
	{
		Inventory->Unbind();
	}
	else if (UMordecaiCharacterSheetWidget* CharacterSheet = Cast<UMordecaiCharacterSheetWidget>(ContentWidget))
	{
		CharacterSheet->UnbindFromASC();
	}
}

#undef LOCTEXT_NAMESPACE
