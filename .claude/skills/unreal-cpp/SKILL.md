---
name: unreal-cpp
description: >
  Unreal Engine C++ programming patterns, types, macros, and conventions. Use when:
  (1) writing any UE C++ class (Actor, Component, GameMode, etc.),
  (2) using UE-specific types (FString, FName, FText, TArray, TMap, TSet, etc.),
  (3) working with the UObject system (UCLASS, UPROPERTY, UFUNCTION, reflection),
  (4) smart pointers (TSharedPtr, TSharedRef, TWeakPtr, TWeakObjectPtr, TSoftObjectPtr),
  (5) delegates (single, multicast, dynamic),
  (6) logging, assertions, debugging macros,
  (7) replication and networking code,
  (8) memory management and garbage collection,
  (9) Actor/Component lifecycle,
  (10) module and plugin architecture.
  NOT for: Blueprint-only work, GAS-specific code (use unreal-gas), Python editor scripting (use unreal-python-editor).
---

# Unreal Engine C++ Programming

Engine source reference: `C:/Users/jeffd/Documents/Gamedev/UnrealEngine`

## Naming Conventions

| Prefix | Meaning | Example |
|--------|---------|---------|
| `U` | UObject-derived class | `UActorComponent`, `UGameplayAbility` |
| `A` | AActor-derived class | `ACharacter`, `APlayerController` |
| `F` | Struct or non-UObject class | `FVector`, `FString`, `FHitResult` |
| `T` | Template | `TArray<>`, `TMap<>`, `TSharedPtr<>` |
| `E` | Enum | `EMovementMode`, `ENetRole` |
| `I` | Interface | `IAbilitySystemInterface` |
| `S` | Slate widget | `SButton`, `STextBlock` |
| `b` | Boolean variable prefix | `bIsActive`, `bCanJump` |

## Reflection Macros

### UCLASS

```cpp
UCLASS(BlueprintType, Blueprintable, Config=Game, meta=(DisplayName="My Actor"))
class MYMODULE_API AMyActor : public AActor
{
    GENERATED_BODY()
public:
    AMyActor();
};
```

**Common specifiers:** `Abstract`, `Blueprintable`, `BlueprintType`, `Config=X`, `Transient`, `NotBlueprintable`, `MinimalAPI`, `Within=OuterClass`, `HideCategories=(Cat)`, `ShowCategories=(Cat)`, `ComponentWrapperClass`

### UPROPERTY

```cpp
// Editable in editor + visible in Blueprints
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
float Damage = 10.f;

// Visible but not editable
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="State")
bool bIsAlive = true;

// Replicated
UPROPERTY(ReplicatedUsing=OnRep_Health)
float Health;

// Config from .ini
UPROPERTY(Config)
int32 MaxPlayers;

// Expose to spawn dialog
UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ExposeOnSpawn="true"))
FVector SpawnOffset;
```

**Access specifiers:** `EditAnywhere`, `EditDefaultsOnly`, `EditInstanceOnly`, `VisibleAnywhere`, `VisibleDefaultsOnly`, `VisibleInstanceOnly`, `BlueprintReadOnly`, `BlueprintReadWrite`

**Lifetime:** `Transient`, `DuplicateTransient`, `SkipSerialization`, `SaveGame`

**Networking:** `Replicated`, `ReplicatedUsing=FuncName`, `NotReplicated`

**Meta:** `DisplayName`, `ClampMin/Max`, `UIMin/Max`, `ExposeOnSpawn`, `AllowPrivateAccess`, `MakeEditWidget`, `EditCondition`, `HideAlphaChannel`, `ToolTip`

### UFUNCTION

```cpp
// Blueprint callable
UFUNCTION(BlueprintCallable, Category="Combat")
void DealDamage(AActor* Target, float Amount);

// Blueprint implementable event
UFUNCTION(BlueprintImplementableEvent, Category="Events")
void OnDeath();

// Native with Blueprint override
UFUNCTION(BlueprintNativeEvent, Category="Events")
void OnHit(const FHitResult& Hit);
void OnHit_Implementation(const FHitResult& Hit);

// Server RPC
UFUNCTION(Server, Reliable, WithValidation)
void ServerFire(FVector Direction);
void ServerFire_Implementation(FVector Direction);
bool ServerFire_Validate(FVector Direction);

// Client RPC
UFUNCTION(Client, Reliable)
void ClientShowDamage(float Amount);

// Multicast RPC
UFUNCTION(NetMulticast, Unreliable)
void MulticastPlayEffect();

// Pure function (no side effects)
UFUNCTION(BlueprintPure, Category="Utils")
float GetHealthPercent() const;

// Exec (console command)
UFUNCTION(Exec)
void DebugKillAll();
```

**Call specifiers:** `BlueprintCallable`, `BlueprintPure`, `BlueprintImplementableEvent`, `BlueprintNativeEvent`, `Exec`

**Network:** `Server`, `Client`, `NetMulticast`, `Reliable`, `Unreliable`, `WithValidation`

### USTRUCT

```cpp
USTRUCT(BlueprintType)
struct FDamageInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseDamage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UDamageType> DamageType;
};
```

### UENUM

```cpp
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
    Idle        UMETA(DisplayName="Idle"),
    Firing      UMETA(DisplayName="Firing"),
    Reloading   UMETA(DisplayName="Reloading"),
};
```

## String Types

For full reference: [references/string-types.md](references/string-types.md)

| Type | Use Case | Mutable | Localized |
|------|----------|---------|-----------|
| `FString` | General purpose string manipulation | Yes | No |
| `FName` | Asset names, identifiers, case-insensitive lookup keys | No (immutable pool) | No |
| `FText` | UI display, localization | Yes (localized) | Yes |

```cpp
FString Str = TEXT("Hello");
FName Name = FName(TEXT("WeaponSocket"));
FText DisplayText = NSLOCTEXT("MyNamespace", "Key", "Localized Hello");
FText FromString = FText::FromString(Str);

// Formatting
FString Formatted = FString::Printf(TEXT("Player %s has %d HP"), *PlayerName, Health);
FText FormattedText = FText::Format(NSLOCTEXT("", "", "{0} has {1} HP"), PlayerNameText, Health);

// Conversions
FString FromName = Name.ToString();
FName ToName(*Str);
```

## Container Types

For full reference: [references/containers.md](references/containers.md)

### TArray

```cpp
TArray<int32> Numbers;
Numbers.Add(42);
Numbers.Emplace(100);
Numbers.AddUnique(42);        // won't add duplicate
Numbers.Remove(42);           // remove first match
Numbers.RemoveAt(0);
Numbers.Num();                // count
Numbers.IsEmpty();
Numbers.Contains(100);
Numbers.Find(100);            // returns index or INDEX_NONE
Numbers.Sort();               // in-place sort
Numbers.FindByPredicate([](int32 V) { return V > 50; });

// Range-based for
for (int32 N : Numbers) { }
for (int32 i = 0; i < Numbers.Num(); i++) { }
```

### TMap

```cpp
TMap<FName, int32> Inventory;
Inventory.Add(FName("Sword"), 1);
Inventory.FindOrAdd(FName("Shield")) = 2;
int32* Found = Inventory.Find(FName("Sword"));
Inventory.Contains(FName("Sword"));
Inventory.Remove(FName("Sword"));
Inventory.Num();

for (auto& [Key, Value] : Inventory) { }
for (auto& Pair : Inventory) { Pair.Key; Pair.Value; }
```

### TSet

```cpp
TSet<FName> Tags;
Tags.Add(FName("Enemy"));
Tags.Contains(FName("Enemy"));
Tags.Remove(FName("Enemy"));
Tags.Num();

// Set operations
TSet<FName> Union = Tags.Union(OtherTags);
TSet<FName> Intersect = Tags.Intersect(OtherTags);
TSet<FName> Diff = Tags.Difference(OtherTags);
```

### Other Containers

- `TQueue<T>` — thread-safe FIFO queue
- `TDeque<T>` — double-ended queue
- `TSortedMap<K,V>` — ordered by key
- `TMultiMap<K,V>` — allows duplicate keys
- `TStaticArray<T,N>` — fixed-size array
- `TBitArray<>` — compact bit storage
- `TSparseArray<T>` — array with gaps (stable indices)
- `TCircularBuffer<T>` — ring buffer
- `TIndirectArray<T>` — array of heap pointers

## Smart Pointers

For full reference: [references/smart-pointers.md](references/smart-pointers.md)

### Non-UObject Smart Pointers

```cpp
// Shared pointer (nullable)
TSharedPtr<FMyClass> Ptr = MakeShared<FMyClass>(Args...);

// Shared reference (never null)
TSharedRef<FMyClass> Ref = MakeShared<FMyClass>(Args...);

// Weak pointer (observes, doesn't own)
TWeakPtr<FMyClass> Weak = Ptr;
if (TSharedPtr<FMyClass> Pinned = Weak.Pin()) { }

// Unique pointer (exclusive ownership)
TUniquePtr<FMyClass> Unique = MakeUnique<FMyClass>(Args...);
```

### UObject Pointers

```cpp
// Raw pointer (GC-managed, OK for UPROPERTY)
UPROPERTY() UMyObject* Obj;

// Weak object pointer (safe, can go stale)
TWeakObjectPtr<UMyObject> WeakObj = Obj;
if (WeakObj.IsValid()) { WeakObj.Get()->DoThing(); }

// Soft object pointer (asset reference by path, lazy load)
UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> MeshRef;
UStaticMesh* Mesh = MeshRef.LoadSynchronous();

// Soft class pointer
UPROPERTY(EditAnywhere) TSoftClassPtr<AActor> ActorClass;
TSubclassOf<AActor> Loaded = ActorClass.LoadSynchronous();

// TSubclassOf (class filter for UPROPERTY)
UPROPERTY(EditAnywhere) TSubclassOf<UDamageType> DamageTypeClass;

// TObjectPtr (UE5 replacement for raw pointers in UPROPERTY)
UPROPERTY() TObjectPtr<UStaticMesh> Mesh;
```

## Delegates

For full reference: [references/delegates.md](references/delegates.md)

```cpp
// Single-cast
DECLARE_DELEGATE(FOnDeath);
DECLARE_DELEGATE_OneParam(FOnDamage, float /*Damage*/);
DECLARE_DELEGATE_RetVal(bool, FCanAttack);

// Multi-cast
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float /*NewHealth*/);

// Dynamic (Blueprint-compatible, requires UFUNCTION binding)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChangedBP, float, NewHealth);
// Exposed:
UPROPERTY(BlueprintAssignable) FOnHealthChangedBP OnHealthChanged;

// Binding
Delegate.BindUObject(this, &AMyActor::OnDeath);
Delegate.BindLambda([](){ });
Delegate.BindSP(SharedPtr, &FMyClass::Func);
MulticastDelegate.AddUObject(this, &AMyActor::OnHealthChanged);
DynamicDelegate.AddDynamic(this, &AMyActor::OnHealthChangedHandler);

// Executing
Delegate.ExecuteIfBound();
MulticastDelegate.Broadcast(NewHealth);
```

## Logging & Assertions

```cpp
// Declare log category
DECLARE_LOG_CATEGORY_EXTERN(LogMyGame, Log, All);  // .h
DEFINE_LOG_CATEGORY(LogMyGame);                      // .cpp

// Log messages
UE_LOG(LogMyGame, Log, TEXT("Player %s joined"), *PlayerName);
UE_LOG(LogMyGame, Warning, TEXT("Health is low: %f"), Health);
UE_LOG(LogMyGame, Error, TEXT("Failed to load asset %s"), *AssetPath);
UE_LOG(LogMyGame, Verbose, TEXT("Tick delta: %f"), DeltaTime);

// Assertions
check(Ptr != nullptr);              // fatal in non-shipping
checkf(Value > 0, TEXT("Bad: %d"), Value);
verify(SomeFunction());             // always executes, asserts on false
ensure(Ptr != nullptr);             // non-fatal, logs callstack once
ensureMsgf(Ptr, TEXT("Null!"));     // non-fatal with message
ensureAlways(Condition);            // logs every time, not just first
```

## Actor Lifecycle

For full reference: [references/actor-lifecycle.md](references/actor-lifecycle.md)

```
Constructor → PostInitializeComponents → BeginPlay → Tick → EndPlay → Destroyed
```

Key functions:
- **Constructor** — set defaults, create components with `CreateDefaultSubobject`
- **PostInitializeComponents()** — all components initialized, safe to query them
- **BeginPlay()** — game started, safe for gameplay initialization
- **Tick(float DeltaTime)** — per-frame update
- **EndPlay(EEndPlayReason)** — cleanup before destruction

## Component Pattern

```cpp
AMyActor::AMyActor()
{
    // Scene root
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    // Mesh
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComp->SetupAttachment(SceneRoot);

    // Collision
    BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    BoxComp->SetupAttachment(SceneRoot);
    BoxComp->OnComponentBeginOverlap.AddDynamic(this, &AMyActor::OnOverlapBegin);

    // Defaults
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
}
```

## Replication

For full reference: [references/replication.md](references/replication.md)

```cpp
// In constructor
bReplicates = true;

// Property replication
void AMyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMyActor, Health);
    DOREPLIFETIME_CONDITION(AMyActor, Ammo, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION_NOTIFY(AMyActor, Score, COND_None, REPNOTIFY_Always);
}

// RepNotify
UFUNCTION()
void OnRep_Health();
```

## Casting & Type Checks

```cpp
// Safe cast (returns nullptr on failure)
AMyCharacter* Char = Cast<AMyCharacter>(OtherActor);
if (Char) { }

// Checked cast (asserts on failure — use when you're certain)
AMyCharacter* Char = CastChecked<AMyCharacter>(OtherActor);

// Interface check
if (OtherActor->Implements<UMyInterface>())
{
    IMyInterface::Execute_MyFunction(OtherActor, Args);
}

// IsA check
if (OtherActor->IsA<AMyCharacter>()) { }
if (OtherActor->IsA(AMyCharacter::StaticClass())) { }
```

## Timers

```cpp
// Set timer
FTimerHandle Handle;
GetWorldTimerManager().SetTimer(Handle, this, &AMyActor::OnTimerFired, 2.0f, /*bLoop=*/false);

// Lambda timer
GetWorldTimerManager().SetTimer(Handle, [this]() { DoThing(); }, 1.0f, true);

// Clear
GetWorldTimerManager().ClearTimer(Handle);
GetWorldTimerManager().ClearAllTimersForObject(this);

// Pause / unpause
GetWorldTimerManager().PauseTimer(Handle);
GetWorldTimerManager().UnPauseTimer(Handle);
```

## Async & Latent Actions

```cpp
// Async on game thread (next frame)
AsyncTask(ENamedThreads::GameThread, [this]() { DoThing(); });

// Async on background thread
Async(EAsyncExecution::ThreadPool, [this]() { HeavyComputation(); });

// FRunnable (custom thread)
class FMyRunnable : public FRunnable
{
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
};
```

## Module / Build System

```cpp
// MyModule.Build.cs
PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
PrivateDependencyModuleNames.Add("InputCore");

// API export macro — use in every class header
class MYMODULE_API AMyActor : public AActor { };
```

## Detailed References

- [references/string-types.md](references/string-types.md) — FString, FName, FText deep dive
- [references/containers.md](references/containers.md) — TArray, TMap, TSet and all container types
- [references/smart-pointers.md](references/smart-pointers.md) — TSharedPtr, TWeakObjectPtr, TSoftObjectPtr
- [references/delegates.md](references/delegates.md) — All delegate types and binding patterns
- [references/actor-lifecycle.md](references/actor-lifecycle.md) — Actor & Component lifecycle
- [references/replication.md](references/replication.md) — Property replication, RPCs, conditions
