// Project Mordecai

#pragma once

#include "Player/LyraPlayerState.h"

#include "MordecaiPlayerState.generated.h"

class UMordecaiAttributeSet;
class UMordecaiAbilitySystemComponent;
class UMordecaiSkillComponent;
class UMordecaiFeatComponent;
class UMordecaiStaminaRuntimeComponent;
class UMordecaiPostureRuntimeComponent;

/**
 * AMordecaiPlayerState
 *
 *	Player state for Project Mordecai. Extends Lyra's base player state.
 *	Adds UMordecaiAttributeSet and swaps in UMordecaiAbilitySystemComponent.
 */
UCLASS()
class LYRAGAME_API AMordecaiPlayerState : public ALyraPlayerState
{
	GENERATED_BODY()

public:
	AMordecaiPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "Mordecai|PlayerState")
	UMordecaiAttributeSet* GetMordecaiAttributeSet() const { return MordecaiAttributeSet; }

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Skills")
	UMordecaiSkillComponent* GetSkillComponent() const { return SkillComponent; }

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Feats")
	UMordecaiFeatComponent* GetFeatComponent() const { return FeatComponent; }

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Stamina")
	UMordecaiStaminaRuntimeComponent* GetStaminaRuntimeComponent() const { return StaminaRuntimeComponent; }

	UFUNCTION(BlueprintCallable, Category = "Mordecai|Posture")
	UMordecaiPostureRuntimeComponent* GetPostureRuntimeComponent() const { return PostureRuntimeComponent; }

private:
	UPROPERTY()
	TObjectPtr<UMordecaiAttributeSet> MordecaiAttributeSet;

	UPROPERTY()
	TObjectPtr<UMordecaiSkillComponent> SkillComponent;

	UPROPERTY()
	TObjectPtr<UMordecaiFeatComponent> FeatComponent;

	UPROPERTY(VisibleAnywhere, Category = "Mordecai|Stamina")
	TObjectPtr<UMordecaiStaminaRuntimeComponent> StaminaRuntimeComponent;

	UPROPERTY(VisibleAnywhere, Category = "Mordecai|Posture")
	TObjectPtr<UMordecaiPostureRuntimeComponent> PostureRuntimeComponent;
};
