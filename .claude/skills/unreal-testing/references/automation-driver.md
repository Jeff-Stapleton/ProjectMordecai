# Automation Driver — Full Reference

Simulates user input (mouse, keyboard) for UI functional tests. Platform-agnostic, Slate-aware.

## Setup

```cpp
#include "IAutomationDriverModule.h"

IAutomationDriverModule::Get().Enable();   // blocks real input
FAutomationDriverPtr Driver = IAutomationDriverModule::Get().CreateDriver();
// ... simulate ...
IAutomationDriverModule::Get().Disable();  // restores real input
```

**Important:** Synchronous API blocks — do NOT call on GameThread. Use Spec's `EAsyncExecution::ThreadPool`.

## Finding Elements

### By::Id() — Preferred, least brittle

Tag widgets with metadata:
```cpp
SNew(STextBlock)
    .Text(ViewModel, &IViewModel::GetName)
    .AddMetaData(FDriverMetaData::Id("SignUpFormNameField"))
```

Find:
```cpp
FDriverElementRef Elem = Driver->FindElement(By::Id("SignUpFormNameField"));
```

### By::Path() — Powerful but brittle

```cpp
Driver->FindElement(By::Path("#SignUpFormNameField"))     // by Id
Driver->FindElement(By::Path("FormField"))                // by Tag
Driver->FindElement(By::Path("<STextBlock>"))              // by widget type
Driver->FindElement(By::Path("#Form//<SEditableText>"))   // descendant
Driver->FindElement(By::Path("#Form/#Field"))              // direct child
```

Path syntax:
- `#name` — match by Id
- `name` — match by Tag
- `<SWidgetType>` — match by type
- `/` — direct child
- `//` — any descendant

### By::Cursor()

```cpp
FDriverElementRef Elem = Driver->FindElement(By::Cursor());
```

### By::Delegate()

```cpp
FDriverElementRef Elem = Driver->FindElement(By::Delegate([](TArray<TSharedRef<SWidget>>& OutWidgets) {
    OutWidgets.Add(MyCachedWidget);
}));
```

### Collections

```cpp
FDriverElementCollectionRef Fields = Driver->FindElements(By::Id("FormField"));
TArray<FDriverElementRef> Elements = Fields->GetElements();  // triggers discovery
```

## Actions

### On Elements

```cpp
Elem->Click();
Elem->DoubleClick();
Elem->Type(TEXT("Hello"));
Elem->Focus();
Elem->ScrollIntoView();
```

Actions auto-wait for element to exist/become visible (up to `ImplicitWait` timeout), scroll into view if needed, then act.

### Sequences (Multi-Element)

```cpp
FDriverSequenceRef Seq = Driver->CreateSequence();
Seq->Actions()
    .MoveToElement(By::Id("Username"))
    .Click(EMouseButtons::Left)
    .Type(TEXT("admin"))
    .MoveToElement(By::Id("Password"))
    .Click(EMouseButtons::Left)
    .Type(TEXT("secret"))
    .Click(By::Id("Submit"));
Seq->Perform();
```

Sequences are reusable, fail-fast (any action failure stops the chain).

### Full Action List

Mouse: Move, Click, DoubleClick, Press/Release, Scroll
Keyboard: Type, TypeChord (Ctrl+Shift+S), Press/Release Key, Focus
Supplemental: ScrollIntoView, GetText, GetSize, GetLocation

Source: `IDriverElement.h`, `IDriverSequence.h`

## Waiting

```cpp
// Explicit wait
Driver->Wait(FTimespan::FromSeconds(2));

// Conditional waits
Driver->Wait(Until::ElementExists(Elem, FWaitTimeout::InSeconds(5)));
Driver->Wait(Until::ElementIsVisible(Elem, FWaitTimeout::InSeconds(5)));
Driver->Wait(Until::ElementIsInteractable(Elem, FWaitTimeout::InSeconds(5)));
Driver->Wait(Until::ElementIsScrolledToBeginning(ScrollBox, FWaitTimeout::InSeconds(3)));

// Custom condition
Driver->Wait(Until::Condition([&]() { return MyFlag; }, FWaitTimeout::InSeconds(10)));
```

Configure `ImplicitWait` on the driver for default action timeouts.

## Best Practices with Specs

```cpp
BEGIN_DEFINE_SPEC(FMyUISpec, "Project.UI.MyFeature", ...)
    FAutomationDriverPtr Driver;
    TSharedPtr<SMyWidget> Widget;
END_DEFINE_SPEC(FMyUISpec)

void FMyUISpec::Define()
{
    BeforeEach([this]() {
        // GameThread: setup widgets
        IAutomationDriverModule::Get().Enable();
        Driver = IAutomationDriverModule::Get().CreateDriver();
        // create/show widgets...
    });

    Describe("Login Form", [this]() {
        It("should submit when clicking Submit", EAsyncExecution::ThreadPool, [this]() {
            // Off GameThread: simulate input
            Driver->FindElement(By::Id("Username"))->Type(TEXT("admin"));
            Driver->FindElement(By::Id("Submit"))->Click();
        });
    });

    AfterEach([this]() {
        Driver.Reset();
        IAutomationDriverModule::Get().Disable();
    });
}
```

**Thread safety:** Don't clone non-thread-safe SharedPtrs off GameThread. Cache them on the test class. Do simulation in `BeforeEach` blocks, check expectations in `It()` on GameThread.
