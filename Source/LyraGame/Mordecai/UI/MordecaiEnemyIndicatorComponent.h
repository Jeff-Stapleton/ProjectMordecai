// Project Mordecai — Enemy Indicator Component (US-064)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"

#include "MordecaiEnemyIndicatorComponent.generated.h"

class UIndicatorDescriptor;
class ULyraIndicatorManagerComponent;
class UMordecaiEnemyIndicatorWidget;

/**
 * UMordecaiEnemyIndicatorComponent
 *
 * Attached to enemy characters. Manages registration/unregistration of a
 * world-space indicator with Lyra's IndicatorSystem. The indicator shows
 * health, name, and status icons above the enemy's head.
 *
 * Registration triggers:
 *   - Enemy takes damage (NotifyDamageReceived)
 *   - Enemy enters aggro range (NotifyAggroActivated)
 *
 * Unregistration triggers:
 *   - Enemy dies (NotifyDeath)
 *   - Component destroyed
 *
 * Distance-based visibility:
 *   - Indicator is hidden when beyond IndicatorVisibilityRange.
 *   - Checked on a low-frequency timer (every 0.25s).
 *
 * See: US-064, AC-064.1 through AC-064.4
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiEnemyIndicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiEnemyIndicatorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// --- Configuration ---

	/** Z offset above enemy for indicator placement (AC-064.2). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Indicator")
	float IndicatorHeightOffset = 120.0f;

	/** Maximum distance from player for indicator visibility (AC-064.4). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Indicator")
	float IndicatorVisibilityRange = 2000.0f;

	/** Widget class for the enemy indicator. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mordecai|Indicator")
	TSoftClassPtr<UMordecaiEnemyIndicatorWidget> IndicatorWidgetClass;

	// --- Registration Triggers ---

	/** Called when the enemy takes damage. Registers indicator if not already registered (AC-064.2). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Indicator")
	void NotifyDamageReceived();

	/** Called when the enemy enters aggro range. Registers indicator if not already registered (AC-064.2). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Indicator")
	void NotifyAggroActivated();

	/** Called when the enemy dies. Unregisters indicator (AC-064.3). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Indicator")
	void NotifyDeath();

	// --- Queries ---

	/** Returns true if the indicator is currently registered with the IndicatorSystem. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Indicator")
	bool IsIndicatorRegistered() const { return bIndicatorRegistered; }

	/** Returns true if the given distance is within IndicatorVisibilityRange (AC-064.4). */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|Indicator")
	bool IsWithinVisibilityRange(float Distance) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RegisterIndicator();
	void UnregisterIndicator();
	void UpdateVisibility();

	/** Find the local player's IndicatorManagerComponent. */
	ULyraIndicatorManagerComponent* FindIndicatorManager() const;

	UPROPERTY()
	TObjectPtr<UIndicatorDescriptor> ActiveDescriptor;

	FTimerHandle VisibilityTimerHandle;
	bool bIndicatorRegistered = false;
};
