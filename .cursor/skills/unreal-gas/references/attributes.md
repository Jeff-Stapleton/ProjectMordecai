# Attributes & AttributeSets — Full Reference

## Defining Attributes

Attributes are `FGameplayAttributeData` floats living in a `UAttributeSet`. Use the `ATTRIBUTE_ACCESSORS` macro for boilerplate getters/setters.

```cpp
// Header
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class UMyAttributeSet : public UAttributeSet
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Vitals")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Vitals")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, MaxHealth)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Mana, Category="Vitals")
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Mana)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxMana, Category="Vitals")
    FGameplayAttributeData MaxMana;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, MaxMana)

    UPROPERTY(BlueprintReadOnly, Category="Damage")
    FGameplayAttributeData Damage;  // Meta attribute — not replicated
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Damage)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Armor, Category="Defense")
    FGameplayAttributeData Armor;
    ATTRIBUTE_ACCESSORS(UMyAttributeSet, Armor)
};
```

## Replication

```cpp
void UMyAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMyAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    // ... same for all replicated attributes
}

void UMyAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, Health, OldHealth);
}
```

Use `REPNOTIFY_Always` to ensure prediction rollback works correctly.

## Initialization

**Via DataTable:**

Create a `DataTable` of `FAttributeMetaData` rows. Apply to ASC:
```cpp
ASC->InitStats(UMyAttributeSet::StaticClass(), DataTable);
```

**Via Instant GameplayEffect:**

Create a GE with `Instant` duration that sets initial attribute values. Apply on spawn.

**In C++ constructor:**

```cpp
UMyAttributeSet::UMyAttributeSet()
{
    InitHealth(100.f);
    InitMaxHealth(100.f);
}
```

## PreAttributeChange() / PreAttributeBaseChange()

Called before CurrentValue (or BaseValue) changes. Use ONLY for clamping — not for gameplay logic.

```cpp
void UMyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetManaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
    }
}
```

**Warning:** Changes here only affect CurrentValue returned from queries. The actual modifier value is unchanged. For BaseValue clamping, use `PostGameplayEffectExecute`.

## PreGameplayEffectExecute()

Called just before a GE modifies an Attribute's value. Can reject or alter the proposed modification.

```cpp
bool UMyAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
    // Return false to reject the modification entirely
    // Modify Data.EvaluatedData.Magnitude to alter the value
    return true;
}
```

## PostGameplayEffectExecute()

Called after an Instant GE (or periodic tick) modifies BaseValue. This is where you:
- Process damage meta attributes
- Clamp health/mana BaseValue
- Trigger death
- Apply lifesteal
- Distribute damage to shields

```cpp
void UMyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
    UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
    AActor* TargetActor = nullptr;
    if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
    {
        TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
    }

    // Process Damage meta attribute
    if (Data.EvaluatedData.Attribute == GetDamageAttribute())
    {
        float LocalDamage = GetDamage();
        SetDamage(0.f);  // Reset meta attribute

        if (LocalDamage > 0.f)
        {
            // Subtract from shields first, then health
            float NewHealth = GetHealth() - LocalDamage;
            SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
        }
    }

    // Clamp health
    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    }
}
```

## OnAttributeAggregatorCreated()

Override to control how modifiers stack on a specific attribute. Default is all modifiers stack. You can change to use only the highest modifier:

```cpp
void UMyAttributeSet::OnAttributeAggregatorCreated(const FGameplayAttribute& Attribute,
    FAggregator* NewAggregator) const
{
    Super::OnAttributeAggregatorCreated(Attribute, NewAggregator);

    if (Attribute == GetMoveSpeedAttribute())
    {
        NewAggregator->EvaluationMetaData->SetMostNegativeMagnitude();
    }
}
```

## Listening for Attribute Changes

```cpp
ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
    .AddUObject(this, &AMyClass::HealthChanged);

void AMyClass::HealthChanged(const FOnAttributeChangeData& Data)
{
    float NewHealth = Data.NewValue;
    float OldHealth = Data.OldValue;
    // Update UI, trigger events, etc.
}
```

## Derived Attributes

Use an `Infinite` GE with `Attribute Based` modifiers. The derived attribute auto-updates when source attributes change.

Formula: `((CurrentValue + Additive) * Multiplicative) / Division`

## Design Guidelines

- **One AttributeSet per logical group** (combat, movement, UI)
- **Meta attributes** (Damage, Healing) for intermediate values — not replicated
- **Max values as separate attributes** (MaxHealth, MaxMana)
- **Don't use BaseValue as max** — beginners' mistake
- **Subclass AttributeSets** for specialized characters (add Shield attribute)
- **Can add/remove AttributeSets at runtime** on ASC for equipment-based attributes
