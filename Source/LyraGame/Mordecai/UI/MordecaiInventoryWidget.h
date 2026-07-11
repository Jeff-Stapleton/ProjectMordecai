// Project Mordecai — Inventory Widget (US-071)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Mordecai/Items/MordecaiItemTypes.h"
#include "Mordecai/Weapons/MordecaiWeaponTypes.h"

#include "MordecaiInventoryWidget.generated.h"

class UHorizontalBox;
class UMordecaiIdentificationService;
class UMordecaiInventoryComponent;
class UMordecaiPauseMenuWidget;
class UMordecaiResourceLedger;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

/**
 * EMordecaiInventoryFilter
 *
 * Category filter for the inventory flat list (AC-071.7). Materials includes
 * both Material and TownResource; Magical includes MagicalItem and UpgradeKey;
 * Quest includes QuestItem.
 */
UENUM(BlueprintType)
enum class EMordecaiInventoryFilter : uint8
{
	All = 0,
	Weapons,
	Armor,
	Trinkets,
	Consumables,
	Materials,
	Quest,
	Magical
};

/**
 * FMordecaiInventoryRowModel
 *
 * Lightweight row cache entry computed from a single FMordecaiItemInstance.
 * The widget caches an array of these on every inventory change delegate;
 * filtering operates on the cache (AC-071.8) — no re-fetch from the component.
 */
USTRUCT(BlueprintType)
struct FMordecaiInventoryRowModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	FGuid InstanceId;

	/** Via UMordecaiItemLibrary::GetDisplayName — honors identification rules (AC-071.5). */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	FText DisplayName;

	/** Via UMordecaiItemLibrary::GetDescription — honors identification rules (AC-071.5). */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	int32 Quantity = 1;

	/** True only when the definition IsStackable() and Quantity > 1 (AC-071.4). */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	bool bShowQuantity = false;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	EMordecaiItemType Type = EMordecaiItemType::Material;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	EMordecaiItemRarity Rarity = EMordecaiItemRarity::Common;

	/** Identification badge state: definition UsesIdentification and not yet identified (AC-071.4). */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	bool bIsUnidentified = false;

	/** Display-only equip state — this widget exposes no equip action (out of scope). */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	bool bIsEquipped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * FMordecaiInventoryLedgerRow
 *
 * One auto-stored resource ledger entry for the ledger panel (AC-071.10).
 */
USTRUCT(BlueprintType)
struct FMordecaiInventoryLedgerRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	FName ItemId;

	/** Entry.Def->DisplayName, or ItemId.ToString() when Def is null (AC-071.10). */
	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Mordecai|Inventory")
	int32 Count = 0;
};

/** Fires once per SetFilter call with the new filter id (AC-071.9). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnMordecaiInventoryFilterChanged,
	FName, NewFilterId);

/**
 * UMordecaiInventoryWidget
 *
 * Content widget for the Pause Menu "Inventory" tab (US-069/US-071). Binds to
 * the local player's UMordecaiInventoryComponent and sibling
 * UMordecaiResourceLedger, and surfaces:
 *   - Flat item list in GetSortedItems() order with name/description via
 *     UMordecaiItemLibrary (identification-aware), quantity, rarity color,
 *     and an unidentified badge flag
 *   - Category filtering over the cached row set (All/Weapons/Armor/Trinkets/
 *     Consumables/Materials/Quest/Magical)
 *   - Auto-stored resource ledger panel, sorted alphabetically
 *   - TryIdentify() action that dispatches to UMordecaiIdentificationService
 *
 * Rebuild pattern: on any OnInventoryChanged / OnResourceChanged fire, the
 * row cache is cleared and rebuilt in a single pass (no polling, no diffing) —
 * matches the US-067/US-068 widget pattern.
 *
 * Drop and equip actions are OUT OF SCOPE (read + identify only).
 * Visual layout is intentionally C++-minimal; Blueprint polish is EDITOR work.
 */
UCLASS()
class LYRAGAME_API UMordecaiInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Tab registration (AC-071.1) ---

	/** Tab id used when registering with UMordecaiPauseMenuWidget. */
	static FName GetInventoryTabId() { return FName(TEXT("inventory")); }

	/** Default display name for the Inventory tab. */
	static FText GetInventoryTabDisplayName();

	/** Register this widget class under the "inventory" tab on the given pause menu. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|InventoryUI")
	static void RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiInventoryWidget> WidgetClass = nullptr);

	// --- Rarity colors (AC-071.6) ---
	// Exact values declared static constexpr here so US-079 Blueprint polish
	// can reference or override the palette from one place.

	static constexpr FLinearColor RarityColorCommon = FLinearColor(0.55f, 0.55f, 0.55f, 1.0f); // Gray
	static constexpr FLinearColor RarityColorGreen  = FLinearColor(0.10f, 0.80f, 0.10f, 1.0f); // Green
	static constexpr FLinearColor RarityColorBlue   = FLinearColor(0.15f, 0.40f, 1.00f, 1.0f); // Blue
	static constexpr FLinearColor RarityColorPurple = FLinearColor(0.60f, 0.20f, 0.90f, 1.0f); // Purple
	static constexpr FLinearColor RarityColorRed    = FLinearColor(0.90f, 0.10f, 0.10f, 1.0f); // Red
	static constexpr FLinearColor RarityColorGold   = FLinearColor(1.00f, 0.84f, 0.15f, 1.0f); // Gold

	/** Rarity → display color map lookup (Common=Gray … Gold=Gold). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	static FLinearColor GetRarityColor(EMordecaiItemRarity Rarity);

	// --- Filter helpers ---

	/** Stable FName id for a filter value (broadcast by OnFilterChanged). */
	static FName FilterToName(EMordecaiInventoryFilter Filter);

	/** True if an item of the given type is visible under the given filter (AC-071.8). */
	static bool FilterMatchesType(EMordecaiInventoryFilter Filter, EMordecaiItemType Type);

	// --- Binding (AC-071.2 / AC-071.3) ---

	/**
	 * Bind to an inventory component and resource ledger. Subscribes to
	 * OnInventoryChanged / OnResourceChanged and rebuilds the row caches.
	 * Null pointers are tolerated: that source shows "--" placeholders and
	 * is skipped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|InventoryUI")
	void BindToInventory(UMordecaiInventoryComponent* Inventory, UMordecaiResourceLedger* Ledger);

	/** Clear all delegate subscriptions (no dangling bindings). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|InventoryUI")
	void Unbind();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	bool IsBoundToInventory() const { return BoundInventory.IsValid(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	bool IsBoundToLedger() const { return BoundLedger.IsValid(); }

	// --- Filter state (AC-071.7 / AC-071.9) ---

	/** Change the category filter. Fires OnFilterChanged exactly once per call. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|InventoryUI")
	void SetFilter(EMordecaiInventoryFilter NewFilter);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	EMordecaiInventoryFilter GetFilter() const { return ActiveFilter; }

	UPROPERTY(BlueprintAssignable, Category = "Mordecai|InventoryUI")
	FOnMordecaiInventoryFilterChanged OnFilterChanged;

	// --- Inventory list (AC-071.4 / AC-071.8) ---

	/** Cached rows passing the active filter, in GetSortedItems() order. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	TArray<FMordecaiInventoryRowModel> GetVisibleRows() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	int32 GetVisibleRowCount() const;

	/** Total cached rows regardless of filter (test/BP introspection). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	int32 GetCachedRowCount() const { return CachedRows.Num(); }

	// --- Ledger panel (AC-071.10 / AC-071.11) ---

	/** Ledger entries sorted alphabetically by display name. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	TArray<FMordecaiInventoryLedgerRow> GetLedgerRows() const { return CachedLedgerRows; }

	// --- Placeholder / empty-state text (AC-071.2 / AC-071.11 / AC-071.15) ---

	/**
	 * Status text for the item list area:
	 *   unbound inventory            → "--"
	 *   empty inventory + ledger     → "Inventory is empty."
	 *   filter hides all items       → "No items match this filter."
	 *   otherwise                    → empty FText
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	FText GetListPlaceholderText() const;

	/**
	 * Status text for the ledger panel:
	 *   unbound ledger → "--", empty ledger → "No resources stored.",
	 *   otherwise empty FText.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|InventoryUI")
	FText GetLedgerPlaceholderText() const;

	// --- Identify action (AC-071.12 / AC-071.13 / AC-071.14) ---

	/**
	 * Identify an unidentified instance via UMordecaiIdentificationService.
	 * Returns the service result. No-op (false) for identified instances,
	 * unknown ids, definitions that don't use identification, or when the
	 * service is unavailable. The service flips state through the inventory
	 * component, whose OnInventoryChanged (delta=0) triggers the row rebuild —
	 * no manual refresh required.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|InventoryUI")
	bool TryIdentify(const FGuid& InstanceId);

	/**
	 * Test/DI seam: provide the identification service directly, bypassing
	 * the GetGameInstance() subsystem lookup (headless widgets have no world).
	 */
	void SetIdentificationServiceOverride(UMordecaiIdentificationService* InService);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	// --- Programmatic fallback layout (US-079) — BP polish can override ---

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> FilterBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ItemListBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ListPlaceholder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LedgerHeader;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> LedgerBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LedgerPlaceholder;

private:
	UFUNCTION()
	void HandleInventoryChanged(const FGuid& InstanceId, int32 QuantityDelta);

	UFUNCTION()
	void HandleResourceChanged(FName ItemId, int32 NewCount);

	UFUNCTION()
	void HandleFilterButtonClicked(FName FilterId);

	UFUNCTION()
	void HandleIdentifyButtonClicked(FName InstanceIdString);

	/** Build the fallback widget tree when no Blueprint layout exists (US-079). */
	void BuildDefaultLayout();

	/** Repopulate the visual item list, filter bar highlight, and ledger panel from the caches. */
	void RefreshVisuals();

	/** Single-pass rebuild of the item row cache from GetSortedItems(). */
	void RebuildRowCache();

	/** Single-pass rebuild of the ledger row cache from GetAllResources(). */
	void RebuildLedgerCache();

	/** Override if set, else GameInstance subsystem lookup. May return null. */
	UMordecaiIdentificationService* ResolveIdentificationService() const;

	TWeakObjectPtr<UMordecaiInventoryComponent> BoundInventory;
	TWeakObjectPtr<UMordecaiResourceLedger> BoundLedger;
	TWeakObjectPtr<UMordecaiIdentificationService> ServiceOverride;

	EMordecaiInventoryFilter ActiveFilter = EMordecaiInventoryFilter::All;

	TArray<FMordecaiInventoryRowModel> CachedRows;
	TArray<FMordecaiInventoryLedgerRow> CachedLedgerRows;
};
