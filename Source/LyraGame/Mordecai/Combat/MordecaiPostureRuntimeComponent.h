// Project Mordecai — authoritative player posture runtime (US-123, US-124)

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"

#include "MordecaiPostureRuntimeComponent.generated.h"

class UAbilitySystemComponent;
class UMordecaiPostureSystem;

/**
 * Observes the PlayerState ASC and owns authority-only player posture
 * regeneration, break, vulnerability, knockdown, lockout and recovery.
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
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void EnterPostureBrokenState();
	void ExitPostureBrokenState();
	void EnterKnockDownState();
	void ExitKnockDownState();
	void ClearPostureRuntimeStates();
	void SetPostureRuntimeState(FGameplayTag StateTag, bool bEnabled);
	void CancelActiveNonPassiveAbilities();
	void LockMovement();
	void UnlockMovement();
	bool IsEnemyDamage(const FOnAttributeChangeData& Data) const;
	double GetRuntimeSeconds() const;

	UPROPERTY(Transient)
	TObjectPtr<UMordecaiPostureSystem> PostureSystem;

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle PostureChangedHandle;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle DeadTagChangedHandle;
	FTimerHandle PostureBrokenTimerHandle;
	FTimerHandle KnockDownTimerHandle;
	bool bMovementLocked = false;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
};
