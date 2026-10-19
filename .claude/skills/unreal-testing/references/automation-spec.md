# Automation Spec (BDD) — Full Reference

## Setup

Two ways to define a Spec:

```cpp
// Simple (no custom members)
DEFINE_SPEC(MySpec, "MyGame.MySpec", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
void MySpec::Define() { /* ... */ }

// With members
BEGIN_DEFINE_SPEC(MySpec, "MyGame.MySpec", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
    TSharedPtr<FMyClass> Instance;
END_DEFINE_SPEC(MySpec)
void MySpec::Define() { /* ... */ }
```

File naming: `MyClass.spec.cpp` (not "Test" in filename).

Implement `Define()` (not `RunTests()`).

## Core API

### Describe / It

```cpp
void MySpec::Define()
{
    Describe("Execute()", [this]() {
        It("should return true when successful", [this]() {
            TestTrue("Execute", Instance->Execute());
        });
        It("should return false when unsuccessful", [this]() {
            TestFalse("Execute", Instance->Execute());
        });
    });
}
```

Combined descriptions read as sentences:
- `Execute() should return true when successful`
- `Execute() should return false when unsuccessful`

Start `It()` descriptions with "should".

### BeforeEach / AfterEach

Run before/after each `It()` in their scope. Multiple calls execute in order. Scope-aware — only run for `It()` blocks within the same or nested `Describe()`.

```cpp
Describe("feature", [this]() {
    BeforeEach([this]() { /* setup */ });
    
    It("should work", [this]() { /* test */ });
    
    AfterEach([this]() { /* cleanup only — no expectations here */ });
    
    Describe("sub-feature", [this]() {
        BeforeEach([this]() { /* additional setup */ });
        It("should also work", [this]() { /* test */ });
    });
});
```

## AsyncExecution

Run code blocks on different threads:

```cpp
BeforeEach(EAsyncExecution::TaskGraph, [this]() { /* runs on TaskGraph */ });
It("test", EAsyncExecution::ThreadPool, [this]() { /* runs on thread pool */ });
AfterEach(EAsyncExecution::Thread, [this]() { /* runs on dedicated thread */ });
```

Options: `EAsyncExecution::TaskGraph`, `ThreadPool`, `Thread`

Essential for Automation Driver tests (must run off GameThread).

## Latent Completion

For multi-frame operations, use `LatentBeforeEach`, `LatentIt`, `LatentAfterEach`. They receive a `Done` delegate — execution pauses until `Done.Execute()` is called.

```cpp
LatentIt("should return items", [this](const FDoneDelegate& Done) {
    Service->QueryItems(FOnQueryComplete::CreateLambda([this, Done](const TArray<FItem>& Items) {
        TestEqual("Item count", Items.Num(), 5);
        Done.Execute();
    }));
});
```

Can combine with AsyncExecution:
```cpp
LatentIt("async test", EAsyncExecution::ThreadPool, [this](const FDoneDelegate& Done) { /* ... */ });
```

## Parameterized Tests

Use loops in `Define()`:

```cpp
Describe("Addition", [this]() {
    for (int32 i = 0; i < 5; i++) {
        It(FString::Printf(TEXT("should resolve %d + %d = %d"), i, i, i + i), [this, i]() {
            TestEqual(FString::Printf(TEXT("%d + %d"), i, i), i + i, i * 2);
        });
    }
});
```

Each iteration becomes a separate, isolated test.

## Redefine

Call `Redefine()` to re-execute `Define()` and regenerate tests. Useful for data-driven tests with external file inputs that change at runtime.

## Disabling Tests

Prefix any function with `x` to disable:
- `xDescribe()` — disables all contained tests
- `xBeforeEach()` / `xAfterEach()` — skips that block
- `xIt()` — disables that specific test

Easier than commenting out code during iteration.

## How It Works Internally

1. `Define()` runs once, collecting all lambdas
2. For each `It()`, a chain is built: `BeforeEach` → `It` → `AfterEach`
3. Each chain becomes latent commands executed sequentially
4. Each block waits for the previous to signal completion

## Examples in Engine

- `Engine/Plugins/Tests/AutomationDriverTests/Source/.../AutomationDriver.spec.cpp` — 120+ expectations
- BuildPatchServices specs (Launcher team)
