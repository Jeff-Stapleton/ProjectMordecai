# Actor & Component Lifecycle — Full Reference

## Actor Lifecycle

### Spawn Sequence

```
1. Constructor (CDO creation — shared defaults)
2. PostInitProperties()
3. PostLoad() (if loaded from disk) / PostActorCreated() (if spawned at runtime)
4. PostRegisterAllComponents()
5. PostInitializeComponents()
6. BeginPlay()
7. Tick() (every frame if enabled)
...
8. EndPlay(Reason)
9. Destroyed()
10. BeginDestroy() → FinishDestroy() (GC)
```

### Constructor

Called once to create the Class Default Object (CDO), then for each instance. Set defaults and create components here.

```cpp
AMyActor::AMyActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    bReplicates = true;
    NetUpdateFrequency = 10.f;

    // Create components (MUST be in constructor for CDO)
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Root);
}
```

**Rules:**
- No `GetWorld()` — world doesn't exist yet
- No `GetOwner()` — not set yet
- No spawning other actors
- `CreateDefaultSubobject` ONLY in constructor
- Don't call virtual functions on `this`

### PostInitializeComponents

All components registered and initialized. Safe to query components, set up references between them.

```cpp
void AMyActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    // Components are ready, set up inter-component references
}
```

### BeginPlay

World is ready, game has started. Safe for all gameplay initialization.

```cpp
void AMyActor::BeginPlay()
{
    Super::BeginPlay();
    // Spawn other actors, start timers, bind delegates, etc.
}
```

### Tick

```cpp
void AMyActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// Disable/enable ticking
PrimaryActorTick.bCanEverTick = false; // in constructor
SetActorTickEnabled(false);             // at runtime
```

### EndPlay

Called when actor is removed from world. Reason tells you why.

```cpp
void AMyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up: unbind delegates, clear timers, release resources
    Super::EndPlay(EndPlayReason);
}
```

Reasons: `Destroyed`, `LevelTransition`, `EndPlayInEditor`, `RemovedFromWorld`, `Quit`

### Destroyed

Called after `EndPlay`. Actor is being removed. Override for final cleanup that `EndPlay` doesn't cover.

## Component Lifecycle

```
1. Constructor (via CreateDefaultSubobject in owning Actor)
2. OnRegister()
3. InitializeComponent() (if bWantsInitializeComponent = true)
4. BeginPlay()
5. TickComponent() (every frame if registered)
...
6. EndPlay()
7. UninitializeComponent()
8. OnUnregister()
9. DestroyComponent()
```

### Key Component Functions

```cpp
// Override for component initialization
virtual void InitializeComponent() override;

// Override for per-frame logic
virtual void TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) override;

// Overlap/hit events (PrimitiveComponent)
UFUNCTION()
void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult);

// Register in constructor:
BoxComp->OnComponentBeginOverlap.AddDynamic(this, &AMyActor::OnOverlapBegin);
BoxComp->OnComponentEndOverlap.AddDynamic(this, &AMyActor::OnOverlapEnd);
BoxComp->OnComponentHit.AddDynamic(this, &AMyActor::OnHit);
```

## Runtime Component Creation

```cpp
// In BeginPlay or later (NOT constructor)
UStaticMeshComponent* DynMesh = NewObject<UStaticMeshComponent>(this, TEXT("DynMesh"));
DynMesh->SetupAttachment(RootComponent);
DynMesh->RegisterComponent(); // MUST register manually
```

## Spawning Actors

```cpp
// Basic spawn
FActorSpawnParameters Params;
Params.Owner = this;
Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

AMyProjectile* Proj = GetWorld()->SpawnActor<AMyProjectile>(
    ProjectileClass, SpawnLocation, SpawnRotation, Params);

// Deferred spawn (configure before BeginPlay)
AMyActor* Deferred = GetWorld()->SpawnActorDeferred<AMyActor>(
    ActorClass, Transform, Owner, Instigator);
Deferred->CustomProperty = Value;
Deferred->FinishSpawning(Transform);
```

## Destruction

```cpp
Actor->Destroy();                    // marks for destruction
Actor->SetLifeSpan(5.0f);          // auto-destroy after 5s
Actor->SetActorHiddenInGame(true);  // hide but keep alive
```

## Common Gotchas

1. **Never call `CreateDefaultSubobject` outside the constructor** — use `NewObject` + `RegisterComponent` instead
2. **Always call `Super::` for lifecycle functions** — `BeginPlay`, `EndPlay`, `Tick`, etc.
3. **`GetWorld()` returns null in constructors** — use it only in `BeginPlay` and later
4. **Components must be registered** to appear in the world — `CreateDefaultSubobject` does this automatically, `NewObject` does not
5. **`Destroy()` is not immediate** — actor is destroyed at end of frame
6. **`IsValid(Actor)` is preferred** over `Actor != nullptr` — also checks pending kill
