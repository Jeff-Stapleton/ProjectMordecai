# Smart Pointers — Full Reference

Source: `Engine/Source/Runtime/Core/Public/Templates/SharedPointer.h`, `UniquePtr.h`

## Two Worlds: UObject vs Non-UObject

UObjects are garbage-collected — **never** use `TSharedPtr` on them. Use `UPROPERTY()` raw pointers, `TWeakObjectPtr`, `TSoftObjectPtr`, or `TObjectPtr`.

Non-UObject types (F-prefix structs, plain C++ classes) use `TSharedPtr`, `TSharedRef`, `TWeakPtr`, `TUniquePtr`.

## Non-UObject Smart Pointers

### TSharedPtr<T> — Nullable Shared Ownership

```cpp
TSharedPtr<FMyData> Ptr = MakeShared<FMyData>(CtorArgs...);
TSharedPtr<FMyData> Null; // nullptr by default

if (Ptr.IsValid()) { Ptr->DoThing(); }
if (Ptr) { }  // implicit bool

FMyData& Ref = *Ptr;
FMyData* Raw = Ptr.Get();

Ptr.Reset(); // release ownership
int32 Count = Ptr.GetSharedReferenceCount();

// Thread-safe version
TSharedPtr<FMyData, ESPMode::ThreadSafe> ThreadSafePtr = MakeShared<FMyData, ESPMode::ThreadSafe>();
```

### TSharedRef<T> — Non-Nullable Shared Ownership

```cpp
TSharedRef<FMyData> Ref = MakeShared<FMyData>(Args...);
// Cannot be null — always dereferenceable
FMyData& Data = *Ref;
FMyData& Data = Ref.Get();

// Convert to/from TSharedPtr
TSharedPtr<FMyData> Ptr = Ref; // implicit
TSharedRef<FMyData> BackToRef = Ptr.ToSharedRef(); // asserts if null!
```

### TWeakPtr<T> — Non-Owning Observer

```cpp
TWeakPtr<FMyData> Weak = SharedPtr;

if (Weak.IsValid()) { } // check without pinning
if (TSharedPtr<FMyData> Pinned = Weak.Pin())
{
    // Safe to use — prevents destruction during scope
    Pinned->DoThing();
}
```

### TUniquePtr<T> — Exclusive Ownership

```cpp
TUniquePtr<FMyData> Unique = MakeUnique<FMyData>(Args...);
FMyData* Raw = Unique.Get();
FMyData* Released = Unique.Release(); // transfers ownership out
Unique.Reset(); // destroy

// Move only — cannot copy
TUniquePtr<FMyData> Moved = MoveTemp(Unique);
```

## Casting

```cpp
// Static cast (compile-time known safe)
TSharedPtr<FDerived> Derived = StaticCastSharedPtr<FDerived>(BasePtr);
TSharedRef<FDerived> DerivedRef = StaticCastSharedRef<FDerived>(BaseRef);

// Const cast
TSharedPtr<FMyData> Mutable = ConstCastSharedPtr<FMyData>(ConstPtr);
```

## UObject Pointers

### Raw Pointer (UPROPERTY)

```cpp
UPROPERTY()
UMyObject* Obj = nullptr;
// GC keeps alive as long as referenced by a UPROPERTY or added to root
// NEVER use TSharedPtr on UObjects!
```

### TObjectPtr<T> (UE5 — Editor only enhancement)

Drop-in replacement for raw `UObject*` in `UPROPERTY`. Adds access tracking in editor.

```cpp
UPROPERTY()
TObjectPtr<UStaticMesh> Mesh;
// Use like a raw pointer: Mesh->DoThing();
// Implicitly converts to/from raw pointer
```

### TWeakObjectPtr<T> — Safe Weak Reference

```cpp
TWeakObjectPtr<AActor> WeakActor = SomeActor;
if (WeakActor.IsValid()) // checks not GC'd AND not pending kill
{
    WeakActor->DoThing();
}
AActor* Raw = WeakActor.Get(); // nullptr if invalid
```

Use when you need to reference a UObject without preventing GC. Common in non-UPROPERTY contexts (local variables, non-UObject classes).

### TSoftObjectPtr<T> — Asset Reference by Path

```cpp
UPROPERTY(EditAnywhere)
TSoftObjectPtr<UStaticMesh> MeshAsset;

// Check if loaded
if (MeshAsset.IsValid()) { UStaticMesh* Mesh = MeshAsset.Get(); }

// Load synchronously
UStaticMesh* Mesh = MeshAsset.LoadSynchronous();

// Get path without loading
FSoftObjectPath Path = MeshAsset.ToSoftObjectPath();
FString PathStr = Path.ToString(); // "/Game/Meshes/SM_Cube.SM_Cube"

// Async load
FStreamableManager& Manager = UAssetManager::GetStreamableManager();
Manager.RequestAsyncLoad(MeshAsset.ToSoftObjectPath(),
    FStreamableDelegate::CreateUObject(this, &AMyActor::OnMeshLoaded));
```

### TSoftClassPtr<T> — Soft Reference to a UClass

```cpp
UPROPERTY(EditAnywhere)
TSoftClassPtr<AActor> ActorClass;

TSubclassOf<AActor> LoadedClass = ActorClass.LoadSynchronous();
```

### TSubclassOf<T> — Class Type Filter

```cpp
UPROPERTY(EditAnywhere)
TSubclassOf<UDamageType> DamageTypeClass;

// Use
UDamageType* CDO = DamageTypeClass.GetDefaultObject();
UObject* Instance = NewObject<UObject>(this, DamageTypeClass);
```

## Summary Table

| Type | Nullable | Owns | GC-Safe | Thread-Safe | Use For |
|------|----------|------|---------|-------------|---------|
| `TSharedPtr<T>` | Yes | Yes | N/A | Optional | Non-UObject shared |
| `TSharedRef<T>` | No | Yes | N/A | Optional | Non-UObject guaranteed |
| `TWeakPtr<T>` | Yes | No | N/A | Optional | Non-UObject observer |
| `TUniquePtr<T>` | Yes | Yes (exclusive) | N/A | No | Non-UObject exclusive |
| `UPROPERTY() T*` | Yes | GC root | Yes | No | UObject standard |
| `TObjectPtr<T>` | Yes | GC root | Yes | No | UObject (UE5 UPROPERTY) |
| `TWeakObjectPtr<T>` | Yes | No | Yes | No | UObject weak ref |
| `TSoftObjectPtr<T>` | Yes | No | Yes | No | UObject lazy load |
| `TSoftClassPtr<T>` | Yes | No | Yes | No | UClass lazy load |
| `TSubclassOf<T>` | Yes | No | Yes | No | UClass type filter |
