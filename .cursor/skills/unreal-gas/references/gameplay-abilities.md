# Gameplay Abilities — Full Reference

## Ability Lifecycle

1. **Grant** — `ASC->GiveAbility(Spec)` adds to ASC's activatable list
2. **CanActivate** — tag checks, cost check, cooldown check
3. **Activate** — `ActivateAbility()` called
4. **Commit** — `CommitAbility()` applies cost + starts cooldown
5. **Execute** — ability logic (tasks, effects, cues)
6. **End** — `EndAbility()` called (must always be called!)

## Creating Abilities

### C++ Ability

```cpp
UCLASS()
class UGA_FireProjectile : public UGameplayAbility
{
    GENERATED_BODY()
public:
    UGA_FireProjectile();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
    UPROPERTY(EditDefaultsOnly) TSubclassOf<UGameplayEffect> DamageEffect;
    UPROPERTY(EditDefaultsOnly) UAnimMontage* FireMontage;
};
```

### Blueprint Ability

Create Blueprint child of `UGameplayAbility`. Override:
- `ActivateAbility` (Event)
- `CanActivateAbility` (optional)
- Use `CommitAbility` node, `EndAbility` node

## Granting Abilities

```cpp
// Basic grant
FGameplayAbilitySpec Spec(AbilityClass, Level, InputID, SourceObject);
FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

// Grant and activate immediately
ASC->GiveAbilityAndActivateOnce(Spec);

// Remove
ASC->ClearAbility(Handle);
ASC->ClearAllAbilities(); // nuclear option
```

## Four Activation Methods

1. **By Handle/Class** — explicit C++/Blueprint call using `FGameplayAbilitySpecHandle`
2. **By Gameplay Event** — fires all abilities with a matching Ability Trigger tag; provides `FGameplayEventData` payload
3. **By Gameplay Effect** — GEs with matching tags trigger abilities with matching Ability Triggers
4. **By Input Code** — input IDs added to ASC trigger matching abilities

## Activating Abilities

```cpp
// By class
bool Success = ASC->TryActivateAbilityByClass(AbilityClass);

// By tag
ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));

// By handle
ASC->TryActivateAbility(Handle);

// By gameplay event
FGameplayEventData Payload;
Payload.EventTag = EventTag;
Payload.Instigator = Instigator;
Payload.Target = Target;
ASC->HandleGameplayEvent(EventTag, &Payload);

// Input-based
ASC->AbilityLocalInputPressed(InputID);
ASC->AbilityLocalInputReleased(InputID);
```

### ActivateAbilityFromEvent

When triggered by a Gameplay Event (not normal activation), the ability uses a different code path:

```cpp
// Override this instead of ActivateAbility when using event triggers:
virtual void ActivateAbilityFromEvent(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData);
```

The `TriggerEventData` carries the event payload (instigator, target, magnitude, etc.). Make sure to handle this code path if your ability responds to Gameplay Events.

## Commit Pattern

```cpp
void UGA_MyAbility::ActivateAbility(...)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // Can also commit separately:
    // CommitAbilityCost(...)  — just cost
    // CommitAbilityCooldown(...) — just cooldown

    // ... do work ...

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
```

## Canceling Abilities

```cpp
// Cancel specific ability
ASC->CancelAbility(AbilityClass);
ASC->CancelAbilityHandle(Handle);

// Cancel by tags
FGameplayTagContainer Tags;
Tags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Skill"));
ASC->CancelAbilities(&Tags);

// Cancel all
ASC->CancelAllAbilities();
```

## Instancing Policy

| Policy | Instance | Use Case |
|--------|----------|----------|
| `NonInstanced` | Uses CDO only | Jump, simple instant abilities |
| `InstancedPerActor` | One per ASC | Most abilities (recommended) |
| `InstancedPerExecution` | New each activation | Rare; expensive |

`NonInstanced`: Cannot use member variables (shares CDO). Most performant.
`InstancedPerActor`: Can use member variables, tasks, state. Best balance.

## Net Execution Policy

| Policy | Description |
|--------|-------------|
| `LocalPredicted` | Client predicts, server confirms. Standard for player abilities. |
| `LocalOnly` | Client only. UI sounds, cosmetic effects. |
| `ServerInitiated` | Server activates, replicates to client. AI abilities. |
| `ServerOnly` | Server only, no client representation. |

## Replication Policy

- `ReplicateNo` — Don't replicate. Default for most.
- `ReplicateYes` — Replicate ability state to all.

## Ability Tags

| Tag | Purpose |
|-----|---------|
| **Ability Tags** | Classify this ability |
| **Cancel Abilities With Tag** | On activate, cancel active abilities matching |
| **Block Abilities With Tag** | While active, block abilities matching |
| **Activation Owned Tags** | Tags added to ASC while ability is active |
| **Activation Required Tags** | ASC must have these to activate |
| **Activation Blocked Tags** | ASC must NOT have these to activate |
| **Source Required/Blocked Tags** | Check source actor's tags |
| **Target Required/Blocked Tags** | Check target actor's tags |

## Input Binding

### Enhanced Input (UE5)

```cpp
// In Character setup:
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(FireAction, ETriggerEvent::Triggered, this, &AMyCharacter::OnFirePressed);
    }
}

void AMyCharacter::OnFirePressed()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->TryActivateAbilityByClass(GA_FireClass);
    }
}
```

### Legacy Input ID Binding

```cpp
// Grant with input ID
FGameplayAbilitySpec Spec(AbilityClass, 1, static_cast<int32>(EAbilityInputID::Fire), this);
ASC->GiveAbility(Spec);

// On input:
ASC->AbilityLocalInputPressed(static_cast<int32>(EAbilityInputID::Fire));
```

## Passing Data to Abilities

1. **GameplayEventData** — `HandleGameplayEvent` with payload
2. **SetByCaller on GE Spec** — dynamic values on effects
3. **Gameplay Effect Context** — hit results, source/target info
4. **FGameplayAbilitySpec** — `DynamicAbilityTags`, custom data
5. **Ability Spec Handle** — look up spec from handle to read/write data

## Leveling Abilities

```cpp
// Set level on spec
FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
if (Spec)
{
    Spec->Level = NewLevel;
    ASC->MarkAbilitySpecDirty(*Spec); // replicate change
}
```

Access level in ability: `GetAbilityLevel()`

## Passive Abilities

Activate on grant, never end:

```cpp
void UGA_PassiveArmor::ActivateAbility(...)
{
    if (!CommitAbility(...)) { EndAbility(...); return; }
    // Apply infinite GE — don't call EndAbility
    ASC->ApplyGameplayEffectToSelf(ArmorGE, GetAbilityLevel(), MakeEffectContext());
}
```

Activate automatically via `GiveAbilityAndActivateOnce()` or trigger via `Ability.Passive` tag on `OnAvatarSet`.

## Ability Batching

For rapid-fire abilities (e.g., gun), batch activate+end+cue into one RPC:

```cpp
FScopedServerAbilityRPCBatcher Batcher(ASC, Handle);
// Activations within this scope are batched
```

## Net Security Policy

Controls what the client is allowed to do:
- `ClientOrServer` — anyone can trigger (default)
- `ServerOnlyExecution` — server must validate
- `ServerOnlyTermination` — only server can end
- `ServerOnly` — full server authority
