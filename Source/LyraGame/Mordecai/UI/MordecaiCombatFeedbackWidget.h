// Project Mordecai — Combat Feedback Widget (US-061)

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"

#include "MordecaiCombatFeedbackWidget.generated.h"

class UAbilitySystemComponent;
class UTextBlock;

/**
 * UMordecaiCombatFeedbackWidget
 *
 * Shows brief text flashes for combat actions:
 * "Perfect Dodge!", "Perfect Block!", "Parried!", "Parry Failed!", "Posture Broken!"
 *
 * Listens for the corresponding gameplay tags on the bound ASC. When a tag is
 * added, displays the feedback text briefly (~1s) then fades/hides.
 */
UCLASS()
class LYRAGAME_API UMordecaiCombatFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Get the display text for a combat feedback tag. Empty if not a feedback tag. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FString GetFeedbackTextForTag(const FGameplayTag& Tag);

	/** Get the display color for a combat feedback tag. Gold for positive, red for negative. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	static FLinearColor GetFeedbackColorForTag(const FGameplayTag& Tag);

	/** Bind to ASC to listen for combat feedback tags. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void BindToASC(UAbilitySystemComponent* ASC);

	/** Manually trigger a feedback flash. */
	UFUNCTION(BlueprintCallable, Category = "Mordecai|HUD")
	void ShowFeedback(const FString& Text, const FLinearColor& Color);

	/** Duration (seconds) that feedback text remains visible before fading. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mordecai|HUD")
	float FeedbackDisplayDuration = 1.0f;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FeedbackText;

private:
	void OnFeedbackTagChanged(const FGameplayTag Tag, int32 NewCount);

	float RemainingDisplayTime = 0.f;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	/** Returns true if tag is a negative feedback (red color). */
	static bool IsNegativeFeedback(const FGameplayTag& Tag);
};
