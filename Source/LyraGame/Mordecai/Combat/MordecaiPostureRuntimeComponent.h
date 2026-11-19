// Project Mordecai — authoritative player posture regeneration (US-123)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"

#include "MordecaiPostureRuntimeComponent.generated.h"

class UAbilitySystemComponent;
class UMordecaiPostureSystem;

/**
 * Observes every player posture spend on the PlayerState ASC and owns the
 * authority-only regeneration delay. Player posture cannot break until US-124.
 */
UCLASS(ClassGroup = (Mordecai), meta = (BlueprintSpawnableComponent))
class LYRAGAME_API UMordecaiPostureRuntimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMordecaiPostureRuntimeComponent(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void BindToAbilitySystem();
	void UnbindFromAbilitySystem();
	void OnPostureChanged(const FOnAttributeChangeData& Data);
	double GetRuntimeSeconds() const;

	UPROPERTY(Transient)
	TObjectPtr<UMordecaiPostureSystem> PostureSystem;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle PostureChangedHandle;
};
