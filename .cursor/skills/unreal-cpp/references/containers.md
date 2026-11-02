# Container Types — Full Reference

Source: `Engine/Source/Runtime/Core/Public/Containers/`

## TArray<T>

Dynamic array. The workhorse container.

### Construction & Initialization
```cpp
TArray<int32> Empty;
TArray<int32> Initialized = { 1, 2, 3 };
TArray<int32> Reserved; Reserved.Reserve(100);
TArray<int32> Filled; Filled.Init(0, 100); // 100 zeros
TArray<int32> Copy(Other);
TArray<int32> Moved(MoveTemp(Other));
```

### Adding
```cpp
Array.Add(Item);              // copy
Array.Emplace(Args...);       // construct in-place (preferred)
Array.AddUnique(Item);        // no-op if already present
Array.Insert(Item, Index);
Array.Append(OtherArray);
Array.Push(Item);             // alias for Add
Array.SetNum(50);             // resize (fills with default)
Array.SetNumUninitialized(50);
Array.SetNumZeroed(50);
```

### Removing
```cpp
Array.Remove(Item);           // remove first match, shift down
Array.RemoveSingle(Item);     // alias
Array.RemoveAt(Index);        // remove at index, shift
Array.RemoveAtSwap(Index);    // remove at index, swap with last (O(1))
Array.RemoveSwap(Item);       // remove first match, swap
Array.RemoveAll(Predicate);   // remove all matching
Array.Pop(/*bAllowShrink=*/false);
Array.Empty();                // clear, optionally keep allocation
Array.Reset();                // clear, keep allocation
```

### Querying
```cpp
Array.Num();                  // element count
Array.Max();                  // allocated capacity
Array.IsEmpty();
Array.IsValidIndex(Index);
Array.Contains(Item);
Array.Find(Item);             // returns INDEX_NONE or index
Array.FindLast(Item);
Array.FindByPredicate([](const T& V) { return V > 5; });
Array.FilterByPredicate([](const T& V) { return V > 5; });
Array.ContainsByPredicate(Pred);
Array.IndexOfByPredicate(Pred);
Array.Top();                  // last element ref
Array.Last(Offset=0);        // from end
Array[Index];                 // unchecked access
```

### Sorting
```cpp
Array.Sort();                 // default operator<
Array.Sort([](const T& A, const T& B) { return A.Score > B.Score; });
Array.StableSort(Predicate);  // preserves equal-element order
Array.HeapSort(Predicate);
Algo::Reverse(Array);
```

### Iteration
```cpp
for (auto& Item : Array) { }
for (int32 i = 0; i < Array.Num(); i++) { }
for (auto It = Array.CreateIterator(); It; ++It) { *It; }
// Safe removal during iteration:
for (int32 i = Array.Num() - 1; i >= 0; --i) { if (Cond) Array.RemoveAt(i); }
```

## TMap<K, V>

Hash map. Unordered. One value per key.

```cpp
TMap<FString, int32> Map;
Map.Add(Key, Value);
Map.Emplace(Key, Args...);
Map.FindOrAdd(Key);           // returns ref, inserts default if missing
int32* Ptr = Map.Find(Key);   // nullptr if not found
Map.Contains(Key);
Map.Remove(Key);
Map.Empty();
Map.Num();

// Access
int32& Val = Map[Key];        // asserts if not found!
int32& Val = Map.FindOrAdd(Key); // safe

// Keys and values
TArray<FString> Keys; Map.GetKeys(Keys);
TArray<int32> Values; Map.GenerateValueArray(Values);

// Iteration
for (auto& [K, V] : Map) { }
for (auto& Pair : Map) { Pair.Key; Pair.Value; }
```

## TMultiMap<K, V>

Like TMap but allows multiple values per key.

```cpp
TMultiMap<FName, AActor*> TaggedActors;
TaggedActors.Add(Tag, Actor);
TArray<AActor*> Found;
TaggedActors.MultiFind(Tag, Found);
TaggedActors.RemoveSingle(Tag, Actor);
```

## TSet<T>

Hash set. Unordered, unique elements.

```cpp
TSet<FName> Tags;
Tags.Add(Tag);
Tags.Contains(Tag);
Tags.Remove(Tag);
Tags.Num();
Tags.Empty();
TArray<FName> AsArray = Tags.Array();

// Set operations
Tags.Union(Other);
Tags.Intersect(Other);
Tags.Difference(Other);
Tags.Includes(Other); // true if Other is subset
```

## TSortedMap<K, V>

Ordered by key (uses `Algo::Sort`). Same API as TMap but iteration is in key order.

## TQueue<T>

Thread-safe FIFO queue (SPSC or MPSC mode).

```cpp
TQueue<FString> Queue;
Queue.Enqueue(TEXT("Hello"));
FString Item;
if (Queue.Dequeue(Item)) { }
if (Queue.Peek(Item)) { }
Queue.IsEmpty();
```

## TCircularBuffer<T>

Fixed-size ring buffer.

```cpp
TCircularBuffer<float> Buffer(256);
Buffer[Buffer.GetNextIndex(CurrentIdx)] = Value;
```

## TStaticArray<T, N>

Compile-time fixed-size array with UE container interface.

## TSparseArray<T>

Array with holes — elements can be removed without shifting. Indices remain stable.

## TBitArray<>

Compact array of bits. Useful for flags/masks.

## TIndirectArray<T>

Array of heap-allocated pointers. Owns the objects.

## TArrayView<T>

Non-owning view into a contiguous array. Like `std::span`.

```cpp
void ProcessItems(TArrayView<const int32> Items);
ProcessItems(MakeArrayView(Array));
ProcessItems(MakeArrayView(RawPtr, Count));
```
