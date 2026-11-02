# String Types — Full Reference

Source: `Engine/Source/Runtime/Core/Public/Containers/UnrealString.h`, `UObject/NameTypes.h`, `Internationalization/Text.h`

## FString

General-purpose mutable string. Backed by `TArray<TCHAR>`. Use for string manipulation, file paths, formatting.

```cpp
FString Str = TEXT("Hello World");
FString Empty;

// Building
FString Built = FString::Printf(TEXT("Player %s (%d HP)"), *Name, Health);
FString Joined = FString::Join(Array, TEXT(", "));
FString Repeated = FString::ChrN(10, TEXT('='));

// Query
int32 Len = Str.Len();
bool bEmpty = Str.IsEmpty();
bool bContains = Str.Contains(TEXT("World"));
bool bStartsWith = Str.StartsWith(TEXT("Hello"));
bool bEndsWith = Str.EndsWith(TEXT("World"));
int32 Idx = Str.Find(TEXT("World")); // INDEX_NONE if not found

// Modification
Str.Append(TEXT(" Unreal"));
Str.InsertAt(5, TEXT(","));
Str.RemoveAt(0, 5);
Str.ReplaceInline(TEXT("World"), TEXT("Engine"));
Str.ToLowerInline();
Str.ToUpperInline();
Str.TrimStartAndEndInline();

// Splitting
FString Left, Right;
Str.Split(TEXT(" "), &Left, &Right);
TArray<FString> Parts;
Str.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/true);

// Substring
FString Sub = Str.Mid(0, 5);
FString L = Str.Left(5);
FString R = Str.Right(5);

// Comparison
bool bEqual = Str.Equals(Other, ESearchCase::IgnoreCase);
bool bMatch = Str.MatchesWildcard(TEXT("*.uasset"));

// Conversion
int32 IntVal; FString::Atoi(*Str);
float FloatVal = FCString::Atof(*Str);
FString FromInt = FString::FromInt(42);
FString FromFloat = FString::SanitizeFloat(3.14f);

// Raw TCHAR access
const TCHAR* Raw = *Str;
```

## FName

Immutable, case-insensitive identifier stored in a global pool. Optimized for comparison (integer compare, not string). Use for asset names, socket names, tag names, row names.

```cpp
FName Name(TEXT("WeaponSocket"));
FName FromString(*SomeString);
FName None = NAME_None; // the "null" name

// Comparison (fast — integer compare)
if (Name == FName(TEXT("WeaponSocket"))) { }
if (Name == NAME_None) { }
if (Name.IsNone()) { }

// Conversion
FString Str = Name.ToString();
Name.AppendString(OutString);

// Number suffix
FName Numbered(TEXT("Item"), 5); // "Item_5"
int32 Number = Name.GetNumber();

// Case
// FNames are CASE-PRESERVING but CASE-INSENSITIVE for comparison
// "Sword" and "sword" are the SAME FName
```

**Performance:** FName comparison is O(1) (integer). Creation is slower (hash lookup). Cache FNames as `static const FName` when used frequently.

```cpp
static const FName HealthName(TEXT("Health"));
```

## FText

Localized, display-ready text. Use for anything shown to the user (UI, HUD, messages). Supports formatting, plurals, gender.

```cpp
// Literal (localized)
FText Display = NSLOCTEXT("MyNamespace", "GreetingKey", "Hello, {PlayerName}!");
FText FromLoc = LOCTEXT("Key", "Text"); // requires LOCTEXT_NAMESPACE

// From other types (NOT for localization — only for debug/internal)
FText FromStr = FText::FromString(SomeString);
FText FromName = FText::FromName(SomeName);

// Formatting (preferred for localized text)
FText Formatted = FText::Format(
    NSLOCTEXT("", "", "{0} dealt {1} damage to {2}"),
    AttackerName, FText::AsNumber(Damage), TargetName);

// Named arguments
FFormatNamedArguments Args;
Args.Add(TEXT("PlayerName"), PlayerNameText);
Args.Add(TEXT("Score"), FText::AsNumber(Score));
FText Result = FText::Format(NSLOCTEXT("", "", "{PlayerName}: {Score}"), Args);

// Number/currency/percent
FText Num = FText::AsNumber(1234);
FText Pct = FText::AsPercent(0.85f);
FText Date = FText::AsDate(FDateTime::Now());

// Comparison
bool bSame = Display.EqualTo(Other); // culture-aware
bool bEmpty = Display.IsEmpty();
```

## Conversion Summary

| From → To | Method |
|-----------|--------|
| FString → FName | `FName(*Str)` |
| FString → FText | `FText::FromString(Str)` (not localized!) |
| FName → FString | `Name.ToString()` |
| FName → FText | `FText::FromName(Name)` |
| FText → FString | `Text.ToString()` (loses localization!) |
| int32 → FString | `FString::FromInt(Val)` |
| float → FString | `FString::SanitizeFloat(Val)` |
| FString → int32 | `FCString::Atoi(*Str)` |
| FString → float | `FCString::Atof(*Str)` |

## TCHAR, TEXT(), and Character Encoding

- `TCHAR` is UE's platform-independent character type (UTF-16 on Windows, UTF-32 on others)
- Always use `TEXT("literal")` for TCHAR string literals
- Use `TCHAR_TO_ANSI()`, `ANSI_TO_TCHAR()` for conversion (temporary buffers)
- Use `TCHAR_TO_UTF8()` for UTF-8 conversion
