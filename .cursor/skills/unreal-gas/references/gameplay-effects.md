# Gameplay Effects — Full Reference

GEs are data-only Blueprints (subclass `UGameplayEffect`). They modify Attributes, apply Tags, grant Abilities, and trigger Cues.

## Duration Types

| Type | BaseValue | CurrentValue | Persistence |
|------|-----------|-------------|-------------|
| `Instant` | ✅ Permanently modified | N/A | No active handle |
| `Duration` | ❌ | ✅ Temporarily modified | Active for set time |
| `Infinite` | ❌ | ✅ Temporarily modified | Active until removed |
| `Periodic` (on Duration/Infinite) | ✅ Each tick modifies BaseValue | N/A | Repeats at interval |

## Applying

```cpp
// 1. Create context
FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
Context.AddSourceObject(this);
Context.AddInstigator(Instigator, Instigator);

// 2. Make spec
FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, Level, Context);

// 3. Optional: set dynamic values
Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), 50.f);

// 4. Apply
FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

// Apply to another target:
FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
```

## Removing

```cpp
// By handle
ASC->RemoveActiveGameplayEffect(Handle);
ASC->RemoveActiveGameplayEffect(Handle, StacksToRemove);

// By class (remove all instances)
ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(EffectTag));

// By granted tags
ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GrantedTag));

// By source
ASC->RemoveActiveEffectsWithSourceObject(SourceObject, SourceASC);
```

## Modifiers

Each modifier specifies: **Attribute**, **Operation** (Add/Multiply/Divide/Override), **Magnitude Type**.

### Magnitude Types

| Type | Description |
|------|-------------|
| **Scalable Float** | Static value, optionally scaled by curve table + level |
| **Attribute Based** | Based on source or target attribute value |
| **Custom Calculation (MMC)** | C++ class for complex math |
| **SetByCaller** | Value set at runtime via tag or name on the spec |

### Modifier Aggregation Formula

```
((CurrentValue + Additive) * Multiplicative) / Division
```

Multiply modifiers stack additively: two ×1.5 = (1 + 0.5 + 0.5) = 2.0× total.

### SetByCaller

```cpp
// Set on spec:
Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"), 50.f);

// In GE modifier: set Magnitude Type to "SetByCaller", specify matching tag
```

## Modifier Magnitude Calculation (MMC)

Custom C++ class for complex modifier math. Can capture attributes from source/target.

```cpp
UMyDamageMMC::UMyDamageMMC()
{
    // Capture source's Attack attribute
    FGameplayEffectAttributeCaptureDefinition AttackDef;
    AttackDef.AttributeToCapture = UMyAttributeSet::GetAttackAttribute();
    AttackDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
    AttackDef.bSnapshot = false; // live value, not snapshot at apply time

    RelevantAttributesToCapture.Add(AttackDef);
}

float UMyDamageMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvalParams;
    EvalParams.SourceTags = SourceTags;
    EvalParams.TargetTags = TargetTags;

    float Attack = 0.f;
    GetCapturedAttributeMagnitude(AttackDef, Spec, EvalParams, Attack);

    return FMath::Max(Attack * 1.5f, 0.f);
}
```

## Execution Calculation

Most powerful modifier — can read multiple attributes, do complex math, modify multiple output attributes.

```cpp
UMyDamageExecCalc::UMyDamageExecCalc()
{
    // Declare attributes to capture
    RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(
        UMyAttributeSet::GetAttackAttribute(),
        EGameplayEffectAttributeCaptureSource::Source, true));
    RelevantAttributesToCapture.Add(FGameplayEffectAttributeCaptureDefinition(
        UMyAttributeSet::GetArmorAttribute(),
        EGameplayEffectAttributeCaptureSource::Target, true));
}

void UMyDamageExecCalc::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& Params,
    FGameplayEffectCustomExecutionOutput& OutOutput) const
{
    float Attack = 0.f, Armor = 0.f;
    Params.AttemptCalculateCapturedAttributeMagnitude(AttackDef, EvalParams, Attack);
    Params.AttemptCalculateCapturedAttributeMagnitude(ArmorDef, EvalParams, Armor);

    float Damage = FMath::Max(Attack - Armor, 0.f);

    OutOutput.AddOutputModifier(
        FGameplayModifierEvaluatedData(UMyAttributeSet::GetDamageAttribute(),
            EGameplayModOp::Additive, Damage));
}
```

## Stacking

Configure on the GE Blueprint:

| Setting | Options |
|---------|---------|
| Stacking Type | None, Aggregate by Source, Aggregate by Target |
| Stack Limit | Max number of stacks |
| Duration Refresh | Reset duration on new stack |
| Period Reset | Reset periodic timer on new stack |
| Stack Expiration | Clear all stacks, remove single, refresh duration |
| Overflow | Deny, apply overflow GE, clear all stacks |

## Granted Abilities

GEs can grant GameplayAbilities while active (Duration/Infinite only):
- Configure in GE → "Granted Abilities" array
- Specify ability class, level, input ID, removal policy

## GE Tags (8 Categories)

| Category | Purpose |
|----------|---------|
| **Asset Tags** | Identify the GE itself |
| **Granted Tags** | Applied to target while GE active |
| **Ongoing Tag Reqs** | GE turns off if target loses required tags |
| **Application Tag Reqs** | GE can't apply if target fails tag check |
| **Remove GEs with Tags** | On apply, remove other active GEs matching |
| **Immunity Tags** | Block GEs with matching Asset Tags |
| **Source Required/Blocked** | Filter on source actor's tags |
| **Target Required/Blocked** | Filter on target actor's tags |

## Immunity

Set on the ASC or via GE tags. Blocks application of GEs with matching tags:

```cpp
// Via GE: set "Immunity Tags" → "Gameplay Effect Asset Tags to Immune"
// Programmatic:
ASC->ApplyModToAttribute(Attribute, EGameplayModOp::Override, Value); // bypass GE system
```

## Dynamic GEs at Runtime

```cpp
UGameplayEffect* DynGE = NewObject<UGameplayEffect>(GetTransientPackage(), FName("DynamicDamage"));
DynGE->DurationPolicy = EGameplayEffectDurationType::Instant;

FGameplayModifierInfo Modifier;
Modifier.ModifierMagnitude = FScalableFloat(DamageAmount);
Modifier.ModifierOp = EGameplayModOp::Additive;
Modifier.Attribute = UMyAttributeSet::GetDamageAttribute();
DynGE->Modifiers.Add(Modifier);

ASC->ApplyGameplayEffectToSelf(DynGE, Level, ASC->MakeEffectContext());
```

## Cooldown

Cooldowns are Duration GEs with a specific GameplayTag. Check cooldown:

```cpp
// Get remaining time
float TimeRemaining, Duration;
FGameplayTagContainer CooldownTags;
CooldownTags.AddTag(CooldownTag);
ASC->GetCooldownTimeRemainingAndDuration(CooldownTags, TimeRemaining, Duration);

// Listen for cooldown changes
ASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &AMyClass::OnGEApplied);
ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved)
    .AddUObject(this, &AMyClass::OnCooldownTagChanged);
```

## Gameplay Effect Components (GEComponents)

GEs contain modular components that define behavior. These are configured in the GE Blueprint.

| Component | Description |
|-----------|-------------|
| `UChanceToApplyGameplayEffectComponent` | Probability that the GE is applied |
| `UBlockAbilityTagsGameplayEffectComponent` | Block ability activation by tags on target |
| `UAssetTagsGameplayEffectComponent` | Tags the GE asset owns (don't transfer to actors) |
| `UAdditionalEffectsGameplayEffectComponent` | Additional GEs to apply conditionally |
| `UTargetTagsGameplayEffectComponent` | Grant tags to the target while GE is active |
| `UTargetTagRequirementsGameplayEffectComponent` | Tag requirements for GE to apply/continue |
| `URemoveOtherGameplayEffectComponent` | Remove other GEs based on conditions |
| `UCustomCanApplyGameplayEffectComponent` | Custom application requirement function |
| `UImmunityGameplayEffectComponent` | Block application of other GE specs |

You can create custom GEComponents in C++ — extend the base and register callbacks into the GE lifecycle.

**Note:** GEComponents are data-only (live within the GE asset). Only one exists per GE class, shared across all applied instances.

## Gameplay Effect Specs

`FGameplayEffectSpec` is the runtime instance of a GE. When working with GEs at runtime (Blueprint or C++), you work with Specs, not the GE asset directly.

```cpp
FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GEClass, Level, Context);
// Spec.Data is the FGameplayEffectSpec*
Spec.Data->SetSetByCallerMagnitude(Tag, Value);
```

The Ability System Blueprint Library functions use GE Specs extensively.

## Gameplay Effect Context

Carries metadata about the GE application: instigator, causer, hit result, etc.

```cpp
FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
Context.AddSourceObject(SourceActor);
Context.AddInstigator(InstigatorActor, CauserActor);
Context.AddHitResult(HitResult);

// Custom context (subclass FGameplayEffectContext):
// Override AllocGameplayEffectContext() on your ASC subclass
```
