# Common GAS Patterns

## Damage Pipeline

The standard damage flow through GAS:

```
GameplayAbility → creates GE Spec → Execution Calculation (reads Attack, Armor)
    → outputs to Damage meta attribute → PostGameplayEffectExecute()
    → distributes to Shield/Health → triggers death/cues
```

### 1. Damage GE with Execution Calculation

```cpp
// GE_Damage Blueprint:
// - Duration: Instant
// - Executions: Add UMyDamageExecCalc
// - Gameplay Cues: GameplayCue.Damage.Hit

// In ExecCalc:
void UMyDamageExecCalc::Execute_Implementation(...) const
{
    float BaseDamage = 0.f;
    // Get SetByCaller damage
    BaseDamage = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), false, 0.f);

    float SourceAttack = 0.f, TargetArmor = 0.f;
    Params.AttemptCalculateCapturedAttributeMagnitude(AttackDef, EvalParams, SourceAttack);
    Params.AttemptCalculateCapturedAttributeMagnitude(ArmorDef, EvalParams, TargetArmor);

    float FinalDamage = FMath::Max(BaseDamage + SourceAttack - TargetArmor, 0.f);

    OutOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
        UMyAttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
}
```

### 2. AttributeSet processes damage

```cpp
void UMyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        float Damage = GetDamage();
        SetDamage(0.f);

        // Absorb with shield first
        float CurrentShield = GetShield();
        if (CurrentShield > 0.f)
        {
            float ShieldAbsorb = FMath::Min(CurrentShield, Damage);
            SetShield(CurrentShield - ShieldAbsorb);
            Damage -= ShieldAbsorb;
        }

        float NewHealth = FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth());
        SetHealth(NewHealth);

        if (NewHealth <= 0.f)
        {
            // Trigger death via gameplay event
            FGameplayEventData Payload;
            Payload.EventTag = FGameplayTag::RequestGameplayTag("Event.Death");
            UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
                GetOwningActor(), Payload.EventTag, Payload);
        }
    }
}
```

## Stun Effect

```
GE_Stun (Duration: 3 seconds)
├── Granted Tags: State.Debuff.Stun
├── Gameplay Cue: GameplayCue.Status.Stun

GA_* abilities:
├── Activation Blocked Tags: State.Debuff.Stun  (can't use abilities while stunned)
```

Listen for stun in character:
```cpp
ASC->RegisterGameplayTagEvent(
    FGameplayTag::RequestGameplayTag("State.Debuff.Stun"),
    EGameplayTagEventType::NewOrRemoved)
    .AddUObject(this, &AMyCharacter::OnStunChanged);

void AMyCharacter::OnStunChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (NewCount > 0)
    {
        // Disable input, play stun animation
        DisableInput(GetController<APlayerController>());
    }
    else
    {
        EnableInput(GetController<APlayerController>());
    }
}
```

## Sprint (Stamina Drain)

```
GA_Sprint:
- Activation Required Tags: (none — always available)
- Activation Blocked Tags: State.Debuff.Stun
- Activation Owned Tags: State.Sprinting
- Cancel Abilities With Tag: State.AimDownSights

On Activate:
1. Apply GE_SprintSpeed (Infinite, +MoveSpeed modifier)
2. Apply GE_StaminaDrain (Infinite, Periodic every 0.1s, -Stamina)

On Input Released or Stamina <= 0:
1. Remove GE_SprintSpeed
2. Remove GE_StaminaDrain
3. EndAbility
```

## Aim Down Sights

```
GA_ADS:
- Activation Owned Tags: State.AimDownSights
- Cancel Abilities With Tag: State.Sprinting

On Activate:
1. Apply GE_ADSSlow (Infinite, MoveSpeed * 0.5)
2. Set camera FOV

On Input Released:
1. Remove GE_ADSSlow
2. Reset camera FOV
3. EndAbility
```

## Lifesteal

In your damage ExecCalc or PostGameplayEffectExecute:

```cpp
// After calculating final damage:
float LifestealPercent = 0.15f; // 15%
float HealAmount = FinalDamage * LifestealPercent;

// Apply heal to source
UAbilitySystemComponent* SourceASC = Context.GetOriginalInstigatorAbilitySystemComponent();
if (SourceASC)
{
    FGameplayEffectSpecHandle HealSpec = SourceASC->MakeOutgoingSpec(GE_Heal, 1, Context);
    HealSpec.Data->SetSetByCallerMagnitude(HealTag, HealAmount);
    SourceASC->ApplyGameplayEffectSpecToSelf(*HealSpec.Data.Get());
}
```

## Cooldown Tracking (for UI)

```cpp
// Blueprint AsyncTask pattern:
UAsyncTask_CooldownChanged* Task = UAsyncTask_CooldownChanged::ListenForCooldownChange(ASC, CooldownTags);
Task->OnCooldownBegin.AddDynamic(this, &UMyWidget::OnCooldownBegin);
Task->OnCooldownEnd.AddDynamic(this, &UMyWidget::OnCooldownEnd);
Task->Activate();

// Manual check:
float TimeRemaining, Duration;
GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, TimeRemaining, Duration);
```

## Dynamic GE at Runtime

For one-off effects not worth creating a Blueprint for:

```cpp
UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), FName("DynHeal"));
GE->DurationPolicy = EGameplayEffectDurationType::Instant;

FGameplayModifierInfo Mod;
Mod.Attribute = UMyAttributeSet::GetHealthAttribute();
Mod.ModifierMagnitude = FScalableFloat(HealAmount);
Mod.ModifierOp = EGameplayModOp::Additive;
GE->Modifiers.Add(Mod);

ASC->ApplyGameplayEffectToSelf(GE, 1.f, ASC->MakeEffectContext());
```

## Debugging Checklist

1. `showdebug abilitysystem` — see active GEs, attributes, tags in viewport
2. Gameplay Debugger (apostrophe `) — NumPad 3 for abilities page
3. `LogAbilitySystem` — enable Verbose for detailed GAS logging
4. Check `InitAbilityActorInfo` called on both server and client
5. Verify ASC replication mode matches your use case
6. Confirm `GetLifetimeReplicatedProps` includes all replicated attributes
7. Ensure `REPNOTIFY_Always` for prediction rollback

## Acronyms

| Acronym | Meaning |
|---------|---------|
| ASC | AbilitySystemComponent |
| GA | GameplayAbility |
| GE | GameplayEffect |
| GC | GameplayCue |
| AT | AbilityTask |
| MMC | ModifierMagnitudeCalculation |
| EC | ExecutionCalculation |
