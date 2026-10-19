# Ability Tasks — Full Reference

Asynchronous building blocks used inside GameplayAbilities. They tick, support latent actions, and handle replication.

## Using Built-in Tasks

```cpp
void UGA_MyAbility::ActivateAbility(...)
{
    // Play montage and wait
    UAbilityTask_PlayMontageAndWait* MontageTask =
        UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, FireMontage, 1.0f, NAME_None, false, 1.0f);
    MontageTask->OnCompleted.AddDynamic(this, &UGA_MyAbility::OnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_MyAbility::OnMontageBlendOut);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_MyAbility::OnMontageCancelled);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_MyAbility::OnMontageCancelled);
    MontageTask->ReadyForActivation();
}
```

**Key:** Always call `ReadyForActivation()` after binding delegates.

## Common Built-in Tasks

| Task | Description |
|------|-------------|
| `PlayMontageAndWait` | Play anim montage, fire on complete/cancel/interrupt |
| `WaitGameplayEvent` | Wait for gameplay event by tag |
| `WaitGameplayTagAdded` | Wait for tag to be added to ASC |
| `WaitGameplayTagRemoved` | Wait for tag to be removed |
| `WaitGameplayEffectApplied_Self` | Wait for GE applied to self |
| `WaitGameplayEffectApplied_Target` | Wait for GE applied to target |
| `WaitGameplayEffectRemoved` | Wait for GE to be removed |
| `WaitAbilityActivate` | Wait for another ability to activate |
| `WaitAbilityCommit` | Wait for another ability to commit |
| `WaitAbilityEnd` | Wait for another ability to end |
| `WaitCancel` | Wait for cancel input |
| `WaitConfirm` | Wait for confirm input |
| `WaitConfirmCancel` | Wait for either confirm or cancel |
| `WaitDelay` | Wait for duration |
| `WaitInputPress` | Wait for input press |
| `WaitInputRelease` | Wait for input release |
| `WaitMovementModeChange` | Wait for character movement mode change |
| `WaitOverlap` | Wait for primitive overlap |
| `WaitTargetData` | Wait for targeting data from target actor |
| `WaitVelocityChange` | Wait for velocity threshold |
| `ApplyRootMotionConstantForce` | Apply constant root motion |
| `ApplyRootMotionRadialForce` | Apply radial root motion |
| `ApplyRootMotionMoveToForce` | Move to location via root motion |
| `ApplyRootMotionJumpForce` | Jump arc via root motion |
| `MoveToLocation` | Interpolate actor to location |
| `NetworkSyncPoint` | Wait for server/client sync |
| `Repeat` | Repeat action N times with delay |
| `SpawnActor` | Deferred spawn with confirmation |
| `VisualizeTargeting` | Show targeting visualization |

## Creating Custom Tasks

```cpp
UCLASS()
class UAbilityTask_WaitForHealth : public UAbilityTask
{
    GENERATED_BODY()

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthThreshold, float, CurrentHealth);

public:
    UPROPERTY(BlueprintAssignable)
    FOnHealthThreshold OnThresholdReached;

    UPROPERTY(BlueprintAssignable)
    FOnHealthThreshold OnThresholdLeft;

    UFUNCTION(BlueprintCallable, Category="Ability|Tasks",
        meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="TRUE"))
    static UAbilityTask_WaitForHealth* WaitForHealthThreshold(
        UGameplayAbility* OwningAbility,
        float Threshold,
        bool bTriggerOnce = true);

    virtual void Activate() override;
    virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
    virtual void TickTask(float DeltaTime) override;

    float HealthThreshold;
    bool bTriggerOnce;
    bool bWasAboveThreshold;

    FDelegateHandle AttributeChangeDelegateHandle;
    void OnHealthChanged(const FOnAttributeChangeData& Data);
};
```

### Implementation

```cpp
UAbilityTask_WaitForHealth* UAbilityTask_WaitForHealth::WaitForHealthThreshold(
    UGameplayAbility* OwningAbility, float Threshold, bool bOnce)
{
    UAbilityTask_WaitForHealth* Task = NewAbilityTask<UAbilityTask_WaitForHealth>(OwningAbility);
    Task->HealthThreshold = Threshold;
    Task->bTriggerOnce = bOnce;
    return Task;
}

void UAbilityTask_WaitForHealth::Activate()
{
    Super::Activate();

    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (ASC)
    {
        AttributeChangeDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
            UMyAttributeSet::GetHealthAttribute())
            .AddUObject(this, &UAbilityTask_WaitForHealth::OnHealthChanged);

        // Check initial state
        float CurrentHealth = ASC->GetNumericAttribute(UMyAttributeSet::GetHealthAttribute());
        bWasAboveThreshold = CurrentHealth >= HealthThreshold;
    }
}

void UAbilityTask_WaitForHealth::OnHealthChanged(const FOnAttributeChangeData& Data)
{
    bool bNowAbove = Data.NewValue >= HealthThreshold;

    if (bWasAboveThreshold && !bNowAbove)
    {
        OnThresholdReached.Broadcast(Data.NewValue);
        if (bTriggerOnce) { EndTask(); }
    }
    else if (!bWasAboveThreshold && bNowAbove)
    {
        OnThresholdLeft.Broadcast(Data.NewValue);
    }

    bWasAboveThreshold = bNowAbove;
}

void UAbilityTask_WaitForHealth::OnDestroy(bool bInOwnerFinished)
{
    if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
    {
        ASC->GetGameplayAttributeValueChangeDelegate(UMyAttributeSet::GetHealthAttribute())
            .Remove(AttributeChangeDelegateHandle);
    }
    Super::OnDestroy(bInOwnerFinished);
}
```

### Key Rules for Custom Tasks

1. Use `NewAbilityTask<T>()` factory — never `NewObject`
2. Call `ReadyForActivation()` after setup
3. Clean up delegates in `OnDestroy()`
4. Use `ShouldBroadcastAbilityTaskDelegates()` before broadcasting
5. Override `TickTask()` if you need per-frame logic (set `bTickingTask = true` in constructor)
6. For replication, override `GetReplicationMode()` and relevant functions

## Root Motion Source Tasks

Apply forces to the character via root motion:

```cpp
UAbilityTask_ApplyRootMotionConstantForce* ForceTask =
    UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
        this, NAME_None, WorldDirection, Strength, Duration,
        bIsAdditive, StrengthOverTime, ERootMotionFinishVelocityMode::ClampVelocity,
        FVector::ZeroVector, ClampVelocity, bEnableGravity);
ForceTask->OnFinish.AddDynamic(this, &UGA_Dash::OnDashFinished);
ForceTask->ReadyForActivation();
```

These integrate with `CharacterMovementComponent` and replicate properly.
