---
name: unreal-gas
description: >
  Unreal Engine Gameplay Ability System (GAS) — framework for abilities, attributes, effects,
  and gameplay cues. Use when: (1) creating/modifying AbilitySystemComponents, (2) defining
  AttributeSets and Attributes, (3) writing GameplayAbilities in C++ or Blueprint,
  (4) creating GameplayEffects (damage, buffs, debuffs, costs, cooldowns),
  (5) implementing Gameplay Cues (VFX/SFX), (6) using Ability Tasks,
  (7) setting up GameplayTags for abilities, (8) prediction and replication of abilities,
  (9) targeting systems, (10) debugging GAS (showdebug, GameplayDebugger).
  NOT for: non-GAS gameplay code, movement/physics, UI (except GAS-driven HUD updates), or AI behavior trees.
---

# Gameplay Ability System (GAS)

A framework for building networked abilities, attributes, and status effects. Battle-tested in Fortnite and Paragon.

## Project Setup

1. Enable **GameplayAbilitySystem** plugin in Editor
2. Add modules to `Build.cs`:
   ```csharp
   PrivateDependencyModuleNames.AddRange(new[] {
       "GameplayAbilities", "GameplayTags", "GameplayTasks"
   });
   ```
3. Regenerate project files
4. UE 5.3+: `InitGlobalData()` is called automatically. For 5.2 and below, call `UAbilitySystemGlobals::Get().InitGlobalData()` in `AssetManager::StartInitialLoading()`

## Core Architecture

```
AbilitySystemComponent (ASC)
├── Attributes (via AttributeSets)
├── GameplayAbilities (granted/activated)
├── GameplayEffects (active effects)
├── GameplayTags (state tags)
└── GameplayCues (VFX/SFX)
```

**Naming conventions:**
- `GA_` — GameplayAbility
- `GE_` — GameplayEffect
- `GC_` — GameplayCue

## Ability System Component (ASC)

The heart of GAS. Every actor using abilities needs one ASC.

```cpp
// In constructor (must be C++)
AbilitySystemComponent = CreateDefaultSubobject<UMyAbilitySystemComponent>(TEXT("ASC"));
AbilitySystemComponent->SetIsReplicated(true);
```

**OwnerActor vs AvatarActor:**
- **OwnerActor** — actor that owns the ASC (often PlayerState for persistence across respawns)
- **AvatarActor** — physical representation (the Character/Pawn)
- Both must implement `IAbilitySystemInterface`

**ASC on PlayerState** (recommended for players): persists attributes/effects across respawns. Increase `NetUpdateFrequency` on PlayerState and enable Adaptive Network Update Frequency.

**ASC on Character** (OK for AI minions): simpler setup, no persistence needed.

### Initialization

```cpp
// Server — in Pawn::PossessedBy()
void AMyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    // If ASC on PlayerState:
    if (AMyPlayerState* PS = GetPlayerState<AMyPlayerState>())
    {
        AbilitySystemComponent = PS->GetAbilitySystemComponent();
        AbilitySystemComponent->InitAbilityActorInfo(PS, this);
    }
}

// Client — in Pawn::OnRep_PlayerState()
void AMyCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    if (AMyPlayerState* PS = GetPlayerState<AMyPlayerState>())
    {
        AbilitySystemComponent = PS->GetAbilitySystemComponent();
        AbilitySystemComponent->InitAbilityActorInfo(PS, this);
    }
}
```

### Replication Modes

| Mode | Use Case | Behavior |
|------|----------|----------|
| `Full` | Single player | All GEs replicated to all clients |
| `Mixed` | MP players | GEs to owning client only; Tags+Cues to all |
| `Minimal` | MP AI | No GE replication; Tags+Cues to all |

```cpp
AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
```

## Attributes & Attribute Sets

For full reference: [references/attributes.md](references/attributes.md)

### Defining an AttributeSet

```cpp
UCLASS()
class UMyAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Health")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Health")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, MaxHealth)

    // Meta attribute (not replicated) — damage placeholder
    UPROPERTY(BlueprintReadOnly, Category="Damage")
    FGameplayAttributeData Damage;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Damage)

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    UFUNCTION() void OnRep_Health(const FGameplayAttributeData& OldValue);
    UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
};
```

**Key callbacks:**
- `PreAttributeChange()` — clamp CurrentValue before it's applied (NOT for gameplay logic)
- `PostGameplayEffectExecute()` — respond to BaseValue changes; do damage distribution, clamping, death checks here
- `OnAttributeAggregatorCreated()` — control modifier stacking behavior

**BaseValue vs CurrentValue:**
- `Instant` GEs modify BaseValue permanently
- `Duration`/`Infinite` GEs modify CurrentValue temporarily
- Max values should be separate Attributes, not BaseValue

## Gameplay Tags

Hierarchical names (`State.Debuff.Stun`) registered with GameplayTagManager. Replace booleans/enums.

```cpp
FGameplayTag StunTag = FGameplayTag::RequestGameplayTag(FName("State.Debuff.Stun"));

// Listen for tag changes
ASC->RegisterGameplayTagEvent(StunTag, EGameplayTagEventType::NewOrRemoved)
    .AddUObject(this, &AMyClass::OnStunTagChanged);
```

Enable **Fast Replication** in project settings for efficient tag networking.

## Gameplay Effects (GE)

For full reference: [references/gameplay-effects.md](references/gameplay-effects.md)

Data-only Blueprints (subclass `UGameplayEffect`) that modify Attributes and apply Tags.

### Duration Types

| Type | Behavior |
|------|----------|
| `Instant` | Permanently changes BaseValue |
| `Duration` | Temporary; modifies CurrentValue; has set duration |
| `Infinite` | Temporary; lasts until explicitly removed |

### Applying / Removing

```cpp
// Make spec
FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DamageEffectClass, Level, Context);

// SetByCaller for dynamic values
Spec.Data->SetSetByCallerMagnitude(DamageTag, 50.0f);

// Apply
FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

// Remove
ASC->RemoveActiveGameplayEffect(Handle);
```

### Modifiers

| Op | Formula |
|----|---------|
| Add | `((Base + Additive) * Multiplicative) / Division` |
| Multiply | Stacks additively: two 1.5x = 2.0x multiplier |
| Divide | Stacks additively like Multiply |

**Modifier types:** Scalable Float, Attribute Based, Custom Calculation (MMC), SetByCaller

### Modifier Magnitude Calculation (MMC)

Custom C++ class for complex modifier math:

```cpp
UCLASS()
class UMyMMC : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()
public:
    UMyMMC();
    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
```

### Execution Calculation

Most powerful — can read multiple attributes, do complex math, modify multiple attributes:

```cpp
UCLASS()
class UMyExecCalc : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()
public:
    UMyExecCalc();
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Params,
        FGameplayEffectCustomExecutionOutput& OutOutput) const override;
};
```

### Cost & Cooldown

```cpp
// In GameplayAbility:
CostGameplayEffectClass = GE_AbilityCost;    // Instant GE that subtracts resources
CooldownGameplayEffectClass = GE_AbilityCooldown; // Duration GE with cooldown tag
```

### Stacking

Configure on the GE: stack by Source/Target, max stacks, duration refresh/reset, overflow behavior.

### GE Tags (8 categories)

| Tag Category | Purpose |
|-------------|---------|
| Gameplay Effect Asset Tags | Tags on the GE itself |
| Granted Tags | Tags applied to target while GE active |
| Ongoing Tag Requirements | GE removed if requirements not met |
| Application Tag Requirements | GE can't apply if requirements fail |
| Remove Gameplay Effects with Tags | Remove other GEs with matching tags |
| Immunity Tags | Block GEs with matching tags |
| Source/Target Required/Blocked Tags | Additional filtering |

## Gameplay Abilities (GA)

For full reference: [references/gameplay-abilities.md](references/gameplay-abilities.md)

### Creating in C++

```cpp
UCLASS()
class UGA_MyAbility : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UGA_MyAbility();
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags,
        const FGameplayTagContainer* TargetTags,
        FGameplayTagContainer* OptionalRelevantTags) const override;
};
```

### Granting & Activating

```cpp
// Grant
FGameplayAbilitySpec Spec(AbilityClass, Level, InputID, this);
ASC->GiveAbility(Spec);

// Activate by class
ASC->TryActivateAbilityByClass(AbilityClass);

// Activate by tag
ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));

// Activate by input (Enhanced Input)
ASC->AbilityLocalInputPressed(InputID);
```

### Commit Pattern

```cpp
void UGA_MyAbility::ActivateAbility(...)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }
    // Do ability work...
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
```

`CommitAbility()` checks and applies Cost + starts Cooldown. Always call `EndAbility()` when done.

### Instancing Policy

| Policy | Description |
|--------|-------------|
| `NonInstanced` | No instance created; uses CDO. Most efficient but limited. |
| `InstancedPerActor` | One instance per ASC. Recommended default. |
| `InstancedPerExecution` | New instance per activation. Expensive. |

### Net Execution Policy

| Policy | Description |
|--------|-------------|
| `LocalPredicted` | Predicted on client, confirmed by server. Default for player abilities. |
| `LocalOnly` | Client only (UI, cosmetics). |
| `ServerInitiated` | Server starts, replicates to client. |
| `ServerOnly` | Server only, no client notification. |

### Ability Tags

| Tag Category | Purpose |
|-------------|---------|
| Ability Tags | Tags that describe this ability |
| Cancel Abilities With Tag | Cancel other active abilities with these tags |
| Block Abilities With Tag | Block activation of abilities with these tags |
| Activation Owned Tags | Tags applied while ability is active |
| Activation Required Tags | Must be present to activate |
| Activation Blocked Tags | Must NOT be present to activate |
| Source/Target Required/Blocked | Filter based on source/target tags |

## Ability Tasks

For full reference: [references/ability-tasks.md](references/ability-tasks.md)

Asynchronous building blocks for abilities. Tick, have latent actions, replicate.

```cpp
// In an ability:
UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
    this, NAME_None, MontageToPlay, 1.0f);
Task->OnCompleted.AddDynamic(this, &UGA_MyAbility::OnMontageCompleted);
Task->OnCancelled.AddDynamic(this, &UGA_MyAbility::OnMontageCancelled);
Task->ReadyForActivation();
```

**Common built-in tasks:**
- `PlayMontageAndWait` — play animation montage
- `WaitGameplayEvent` — wait for gameplay event
- `WaitAbilityCommit` — wait for ability commit
- `WaitTargetData` — wait for targeting data
- `WaitInputPress/Release` — wait for input
- `WaitDelay` — wait for time
- `ApplyRootMotionConstantForce` — root motion
- `WaitGameplayTagAdded/Removed` — tag events
- `WaitGameplayEffectApplied` — effect events

### Custom Ability Tasks

```cpp
UCLASS()
class UMyAbilityTask : public UAbilityTask
{
    GENERATED_BODY()
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComplete);

public:
    UPROPERTY(BlueprintAssignable) FOnComplete OnComplete;

    UFUNCTION(BlueprintCallable, Category="AbilityTasks",
        meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility"))
    static UMyAbilityTask* CreateTask(UGameplayAbility* OwningAbility, FName TaskInstanceName);

    virtual void Activate() override;
    virtual void TickTask(float DeltaTime) override;
    virtual void OnDestroy(bool bInOwnerFinished) override;
};
```

## Gameplay Cues

Visual/audio feedback triggered by GAS. Tags must start with `GameplayCue.`

```cpp
// From ability or effect:
ASC->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.Hero.Fireball"));

// With parameters:
FGameplayCueParameters Params;
Params.Location = HitLocation;
ASC->ExecuteGameplayCue(CueTag, Params);

// Duration cues (from GEs):
ASC->AddGameplayCue(CueTag, Params);  // OnActive + WhileActive
ASC->RemoveGameplayCue(CueTag);       // OnRemove
```

**Types:**
- **Static** (Notify) — fire-and-forget: `GameplayCue_Execute`, `GameplayCue_OnActive`, `GameplayCue_OnRemove`
- **Actor** (looping) — spawns an actor that persists while effect is active

## Targeting

```cpp
// In ability:
UAbilityTask_WaitTargetData* Task = UAbilityTask_WaitTargetData::CreateWaitTargetDataProxy(
    this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
Task->ValidData.AddDynamic(this, &UGA_MyAbility::OnTargetDataReady);
Task->ReadyForActivation();
```

`FGameplayAbilityTargetData` — base struct for target data (hit results, actors, transforms).

## Prediction

GAS supports client-side prediction for:
- Ability activation
- Animation montages
- Attribute changes
- GameplayTag application
- GameplayCue spawning
- Root motion

**Cannot predict:** GE removal, cooldown reconciliation.

Use `LocalPredicted` net execution policy. The system uses `FPredictionKey` to match client predictions with server confirmations.

## Debugging

```
// Console commands:
showdebug abilitysystem    // Shows ASC state on HUD
AbilitySystem.Debug.NextTarget/PrevTarget  // Cycle targets

// Gameplay Debugger (apostrophe key):
// Shows tags, attributes, abilities, effects

// Logging:
LogAbilitySystem            // Main GAS log category
```

## Common Patterns

See: [references/common-patterns.md](references/common-patterns.md) for:
- Damage pipeline (Meta Attributes → Execution Calc → PostGameplayEffectExecute)
- Stun effect
- Sprint (stamina drain)
- Lifesteal
- Cooldown tracking
- Dynamic GE creation at runtime

## Detailed References

- [references/attributes.md](references/attributes.md) — AttributeSets, defining/initializing attributes, clamping
- [references/gameplay-effects.md](references/gameplay-effects.md) — GE details, modifiers, MMC, execution calcs, stacking
- [references/gameplay-abilities.md](references/gameplay-abilities.md) — GA lifecycle, input binding, instancing, networking
- [references/ability-tasks.md](references/ability-tasks.md) — Built-in and custom ability tasks
- [references/common-patterns.md](references/common-patterns.md) — Common ability/effect implementations
