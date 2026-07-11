// Project Mordecai — Id-Payload Button (US-079)

#pragma once

#include "Components/Button.h"

#include "MordecaiTabButton.generated.h"

/** OnClicked with the button's id attached — solves UButton's payload-less click. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnMordecaiTabButtonClicked,
	FName, ButtonId);

/**
 * UMordecaiTabButton
 *
 * UButton carrying an FName id, re-broadcast on click via OnClickedWithId.
 * Used for pause-menu tabs, inventory filter buttons, and per-row identify
 * buttons — anywhere many C++-constructed buttons share one handler.
 */
UCLASS()
class LYRAGAME_API UMordecaiTabButton : public UButton
{
	GENERATED_BODY()

public:
	/** Assign the id and hook the click relay. Call once after ConstructWidget. */
	void InitButton(FName InButtonId);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mordecai|UI")
	FName GetButtonId() const { return ButtonId; }

	UPROPERTY(BlueprintAssignable, Category = "Mordecai|UI")
	FOnMordecaiTabButtonClicked OnClickedWithId;

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	FName ButtonId;

	bool bClickRelayBound = false;
};
