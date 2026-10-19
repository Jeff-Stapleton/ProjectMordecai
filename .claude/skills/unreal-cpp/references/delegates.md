# Delegates — Full Reference

Source: `Engine/Source/Runtime/Core/Public/Delegates/Delegate.h`

## Delegate Types

| Type | Macro Prefix | Multiple Bindings | Blueprint | Serializable |
|------|-------------|-------------------|-----------|-------------|
| Single-cast | `DECLARE_DELEGATE` | No | No | No |
| Multi-cast | `DECLARE_MULTICAST_DELEGATE` | Yes | No | No |
| Dynamic | `DECLARE_DYNAMIC_DELEGATE` | No | Yes | Yes |
| Dynamic Multi-cast | `DECLARE_DYNAMIC_MULTICAST_DELEGATE` | Yes | Yes | Yes |

## Declaration

```cpp
// No params
DECLARE_DELEGATE(FOnDeath);
DECLARE_MULTICAST_DELEGATE(FOnDeathMulti);
DECLARE_DYNAMIC_DELEGATE(FOnDeathDynamic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathDynamicMulti);

// With params (OneParam, TwoParams, ThreeParams, ... up to NineParams)
DECLARE_DELEGATE_OneParam(FOnDamage, float /*Damage*/);
DECLARE_DELEGATE_TwoParams(FOnHit, AActor* /*Instigator*/, float /*Damage*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float /*NewHealth*/);

// With return value
DECLARE_DELEGATE_RetVal(bool, FCanAttack);
DECLARE_DELEGATE_RetVal_OneParam(float, FCalcDamage, float /*BaseDamage*/);

// Dynamic delegates require named parameters
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnDamageDyn, float, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedDyn, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitDyn, AActor*, Instigator, float, Damage);
```

## Binding — Single-Cast

| Method | Target | Safety |
|--------|--------|--------|
| `BindStatic(&GlobalFunc)` | Free/static function | None |
| `BindUObject(UObj, &UClass::Func)` | UObject member | Weak ref (safe if GC'd) |
| `BindSP(SharedPtr, &FClass::Func)` | Shared pointer | Weak ref |
| `BindThreadSafeSP(SharedPtr, &Func)` | Thread-safe shared ptr | Weak ref |
| `BindRaw(RawPtr, &FClass::Func)` | Raw pointer | **NONE — you must Unbind!** |
| `BindLambda(Lambda)` | Lambda | **NONE — captures may dangle!** |
| `BindSPLambda(SharedPtr, Lambda)` | Lambda guarded by shared ptr | Weak ref on shared ptr |
| `BindWeakLambda(UObj, Lambda)` | Lambda guarded by UObject | Weak ref on UObject |
| `BindUFunction(UObj, FName)` | UFUNCTION by name | Weak ref |

```cpp
FOnDamage OnDamage;
OnDamage.BindUObject(this, &AMyActor::HandleDamage);
OnDamage.BindLambda([](float D) { UE_LOG(LogTemp, Log, TEXT("Damage: %f"), D); });
OnDamage.BindStatic(&UMyLibrary::StaticHandleDamage);

// Execute
OnDamage.ExecuteIfBound(50.f);
if (OnDamage.IsBound()) { OnDamage.Execute(50.f); }
OnDamage.Unbind();
```

## Binding — Multi-Cast

Replace `Bind` with `Add`, returns `FDelegateHandle` for removal.

```cpp
FOnHealthChanged OnHealthChanged;
FDelegateHandle Handle = OnHealthChanged.AddUObject(this, &AMyActor::HandleHealthChanged);
OnHealthChanged.AddLambda([](float H) { });
OnHealthChanged.AddStatic(&GlobalFunc);

// Broadcast (calls all bound delegates)
OnHealthChanged.Broadcast(NewHealth);

// Remove
OnHealthChanged.Remove(Handle);
OnHealthChanged.RemoveAll(this); // remove all bindings for object
OnHealthChanged.Clear();
```

## Dynamic Delegates — Blueprint Compatible

Must bind to `UFUNCTION`s. Parameter names must match declaration.

```cpp
// Declaration
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedBP, float, NewHealth);

// In class
UPROPERTY(BlueprintAssignable, Category="Events")
FOnHealthChangedBP OnHealthChanged;

// Binding in C++
OnHealthChanged.AddDynamic(this, &AMyActor::HandleHealthChanged);
// The bound function MUST be a UFUNCTION:
UFUNCTION()
void HandleHealthChanged(float NewHealth);

// Remove
OnHealthChanged.RemoveDynamic(this, &AMyActor::HandleHealthChanged);

// Broadcast
OnHealthChanged.Broadcast(CurrentHealth);

// Check
OnHealthChanged.IsBound();
```

## Payload Data

Single-cast and multi-cast (non-dynamic) delegates support payload — extra args stored at bind time:

```cpp
DECLARE_DELEGATE_OneParam(FOnItemUsed, FName /*ItemName*/);

// Bind with payload (int32 slot is extra data)
Delegate.BindUObject(this, &AMyActor::OnItemUsed, FName("HealthPotion"));

void AMyActor::OnItemUsed(FName ItemName)
{
    // ItemName == "HealthPotion" (from payload)
}
```

## Creating Delegate Instances for Deferred Add

```cpp
// Create a delegate instance separately from adding it
FOnHealthChanged::FDelegate MyDelegate;
MyDelegate.BindUObject(this, &AMyActor::HandleHealth);
FDelegateHandle Handle = OnHealthChanged.Add(MyDelegate);

// Or using CreateUObject:
auto Handle = OnHealthChanged.Add(
    FOnHealthChanged::FDelegate::CreateUObject(this, &AMyActor::HandleHealth));
```

## Common Patterns

```cpp
// UPROPERTY delegate for Blueprint assignment
UPROPERTY(BlueprintAssignable) FOnDeathDynamicMulti OnDeath;

// C++ multicast for internal systems
FOnHealthChanged OnHealthChangedNative;

// Notify pattern
void SetHealth(float NewHealth)
{
    Health = NewHealth;
    OnHealthChangedNative.Broadcast(Health);
    OnHealthChanged.Broadcast(Health); // Blueprint
}
```
