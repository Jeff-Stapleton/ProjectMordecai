// Project Mordecai — Character Sheet Widget (US-066)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "Mordecai/UI/MordecaiCharacterSheetTypes.h"

#include "MordecaiCharacterSheetWidget.generated.h"

class UAbilitySystemComponent;
class UMordecaiPauseMenuWidget;

/**
 * UMordecaiCharacterSheetWidget
 *
 * Character tab content for the Pause Menu (US-069). Binds to a player's ASC
 * and surfaces:
 *   - 9 primary attributes grouped by Physical / Resilience / Magical
 *   - 4 core resource pools (Health, Stamina, Spell Points, Posture)
 *   - 5 primary-derived multipliers (Physical Damage, Attack Speed, Affliction
 *     Resist, Magic Damage, Cast Speed)
 *   - 9 secondary-derived stats (Armor Penetration, Crit chances, regens, etc.)
 *
 * All live values come from UMordecaiAttributeSet via GAS attribute change
 * delegates — the widget refreshes only the affected row when an attribute
 * changes (no polling). Without an ASC, rows display "--".
 *
 * Visual layout is intentionally C++-minimal; Blueprint polish is EDITOR work
 * (US-076 / Phase 4). Pure data logic lives in FMordecaiCharacterSheetData and
 * is independently testable.
 */
UCLASS()
class LYRAGAME_API UMordecaiCharacterSheetWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Tab id used when registering with UMordecaiPauseMenuWidget. */
	static FName GetCharacterTabId() { return FName(TEXT("character")); }

	/** Default display name for the Character tab. */
	static FText GetCharacterTabDisplayName();

	/** Register this widget class under the "character" tab on the given pause menu. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|CharacterSheet")
	static void RegisterWithPauseMenu(UMordecaiPauseMenuWidget* PauseMenu, TSubclassOf<UMordecaiCharacterSheetWidget> WidgetClass = nullptr);

	/** Subscribe to attribute-change delegates on the given ASC and prime current values. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|CharacterSheet")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Unsubscribe from all attribute-change delegates and mark data unbound. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|CharacterSheet")
	void UnbindFromASC();

	/** True if currently subscribed to a live ASC. */
	bool IsBoundToASC() const { return BoundASC.IsValid(); }

	/** Number of attribute-change delegate handles currently registered. */
	int32 GetRegisteredDelegateCount() const { return DelegateHandles.Num(); }

	/** Read access for tests and BP. */
	const FMordecaiCharacterSheetData& GetData() const { return Data; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** Model populated by InitDefaults() and refreshed from the bound ASC. */
	FMordecaiCharacterSheetData Data;

private:
	void RefreshAllFromASC();
	void OnPrimaryAttributeChanged(const FOnAttributeChangeData& ChangeData, int32 PrimaryIndex);
	void OnPrimaryDerivedChanged(const FOnAttributeChangeData& ChangeData, int32 DerivedIndex);
	void OnSecondaryDerivedChanged(const FOnAttributeChangeData& ChangeData, int32 SecondaryIndex);
	void OnResourceChanged(const FOnAttributeChangeData& ChangeData, int32 ResourceIndex, bool bMax);

	void RegisterAttributeDelegate(const FGameplayAttribute& Attribute, TFunction<void(const FOnAttributeChangeData&)> Callback);

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TArray<TPair<FGameplayAttribute, FDelegateHandle>> DelegateHandles;
};
