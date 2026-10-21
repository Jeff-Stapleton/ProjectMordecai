// Project Mordecai — Agent bridge protocol helpers (Specs/agent_bridge_v1.md §2-§5)

#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"

#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY(LogMordecaiAgentBridge);

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

FMordecaiAgentBridgeConfig FMordecaiAgentBridgeConfig::FromCommandLine(const TCHAR* CommandLine)
{
	FMordecaiAgentBridgeConfig Config;
	Config.ShotDir = FPaths::ProjectSavedDir() / TEXT("Playtests/Bridge");
	if (!CommandLine)
	{
		return Config;
	}

	// FParse::Param matches whole tokens only, so -AgentBridgePort=... does not enable the bridge
	Config.bEnabled = FParse::Param(CommandLine, TEXT("AgentBridge"));

	FString PortText;
	if (FParse::Value(CommandLine, TEXT("AgentBridgePort="), PortText))
	{
		int32 Port = 0;
		if (PortText.IsNumeric() && LexTryParseString(Port, *PortText) && Port >= 1 && Port <= 65535)
		{
			Config.Port = Port;
		}
	}

	FParse::Value(CommandLine, TEXT("AgentBridgeToken="), Config.Token);

	FString ShotDir;
	if (FParse::Value(CommandLine, TEXT("AgentBridgeShotDir="), ShotDir) && !ShotDir.IsEmpty())
	{
		Config.ShotDir = ShotDir;
	}
	return Config;
}

// ---------------------------------------------------------------------------
// Framer
// ---------------------------------------------------------------------------

FMordecaiAgentBridgeLineFramer::FMordecaiAgentBridgeLineFramer(int32 InMaxLineBytes)
	: MaxLineBytes(FMath::Max(1, InMaxLineBytes))
{
}

void FMordecaiAgentBridgeLineFramer::Append(const uint8* Data, int32 Num, TArray<FMordecaiAgentBridgeFramedLine>& OutLines)
{
	if (!Data || Num <= 0)
	{
		return;
	}

	int32 Start = 0;
	while (Start < Num)
	{
		// 0x0A never occurs inside a UTF-8 multibyte sequence, so splitting raw bytes is safe
		int32 NewLine = INDEX_NONE;
		for (int32 Index = Start; Index < Num; ++Index)
		{
			if (Data[Index] == '\n')
			{
				NewLine = Index;
				break;
			}
		}
		const int32 End = NewLine == INDEX_NONE ? Num : NewLine;

		if (bDiscarding)
		{
			// Skip the rest of an oversize line
			if (NewLine != INDEX_NONE)
			{
				bDiscarding = false;
			}
		}
		else
		{
			Pending.Append(Data + Start, End - Start);
			if (NewLine != INDEX_NONE)
			{
				if (Pending.Num() > MaxLineBytes)
				{
					OutLines.Add({ FString(), /*bOversize*/ true });
					Pending.Reset();
				}
				else
				{
					EmitPending(OutLines);
				}
			}
			else if (Pending.Num() > MaxLineBytes)
			{
				OutLines.Add({ FString(), /*bOversize*/ true });
				Pending.Reset();
				bDiscarding = true;
			}
		}

		if (NewLine == INDEX_NONE)
		{
			break;
		}
		Start = NewLine + 1;
	}
}

void FMordecaiAgentBridgeLineFramer::EmitPending(TArray<FMordecaiAgentBridgeFramedLine>& OutLines)
{
	int32 Length = Pending.Num();
	if (Length > 0 && Pending[Length - 1] == '\r')
	{
		--Length;
	}

	FString Text;
	if (Length > 0)
	{
		const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Pending.GetData()), Length);
		Text = FString(Converted.Length(), Converted.Get());
	}
	Pending.Reset();

	Text.TrimStartAndEndInline();
	if (!Text.IsEmpty())
	{
		OutLines.Add({ MoveTemp(Text), false });
	}
}

namespace MordecaiAgentBridge
{
	// -----------------------------------------------------------------------
	// Requests and responses
	// -----------------------------------------------------------------------

	bool ParseRequest(const FString& Line, FMordecaiAgentBridgeRequest& Out, FString& OutMessage)
	{
		Out = FMordecaiAgentBridgeRequest();
		Out.Id = MakeShared<FJsonValueNull>();

		TSharedPtr<FJsonValue> Parsed;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Line);
		// Numbers stay as their original text, so the id is echoed verbatim
		if (!FJsonSerializer::Deserialize(Reader, Parsed, FJsonSerializer::EFlags::StoreNumbersAsStrings) || !Parsed.IsValid())
		{
			OutMessage = FString::Printf(TEXT("malformed JSON: %s"), *Reader->GetErrorMessage());
			return false;
		}
		if (Parsed->Type != EJson::Object)
		{
			OutMessage = TEXT("a request must be a JSON object");
			return false;
		}

		Out.Args = Parsed->AsObject();
		if (const TSharedPtr<FJsonValue> Id = Out.Args->TryGetField(TEXT("id")))
		{
			if (Id->Type == EJson::Number || Id->Type == EJson::String)
			{
				Out.Id = Id;
			}
			else if (Id->Type != EJson::Null)
			{
				OutMessage = TEXT("'id' must be a number or a string");
				return false;
			}
		}

		const TSharedPtr<FJsonValue> Cmd = Out.Args->TryGetField(TEXT("cmd"));
		if (!Cmd.IsValid())
		{
			OutMessage = TEXT("missing 'cmd'");
			return false;
		}
		if (Cmd->Type != EJson::String || Cmd->AsString().IsEmpty())
		{
			OutMessage = TEXT("'cmd' must be a non-empty string");
			return false;
		}
		Out.Cmd = Cmd->AsString();
		return true;
	}

	TSharedRef<FJsonObject> MakeOkResponse(const TSharedPtr<FJsonValue>& Id, const FString& Cmd)
	{
		const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
		Response->SetField(TEXT("id"), Id.IsValid() ? Id : MakeShared<FJsonValueNull>());
		Response->SetBoolField(TEXT("ok"), true);
		Response->SetField(TEXT("cmd"), MakeStringOrNull(Cmd, Cmd.IsEmpty()));
		return Response;
	}

	TSharedRef<FJsonObject> MakeErrorResponse(const TSharedPtr<FJsonValue>& Id, const FString& Cmd, const FString& Code, const FString& Message)
	{
		const TSharedRef<FJsonObject> Response = MakeShared<FJsonObject>();
		Response->SetField(TEXT("id"), Id.IsValid() ? Id : MakeShared<FJsonValueNull>());
		Response->SetBoolField(TEXT("ok"), false);
		Response->SetField(TEXT("cmd"), MakeStringOrNull(Cmd, Cmd.IsEmpty()));
		Response->SetStringField(TEXT("error"), Code);
		Response->SetStringField(TEXT("message"), Message);
		return Response;
	}

	FString ToJsonLine(const TSharedRef<FJsonObject>& Object)
	{
		FString Out;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
		FJsonSerializer::Serialize(Object, Writer);
		// The condensed writer escapes control characters; this is a belt-and-braces guard for NDJSON
		Out.ReplaceInline(TEXT("\n"), TEXT("\\n"), ESearchCase::CaseSensitive);
		Out.ReplaceInline(TEXT("\r"), TEXT("\\r"), ESearchCase::CaseSensitive);
		return Out;
	}

	TSharedRef<FJsonValue> MakeNumber(double Value, int32 Decimals)
	{
		if (!FMath::IsFinite(Value))
		{
			return MakeShared<FJsonValueNull>();
		}
		FString Text;
		switch (Decimals)
		{
		case 0: Text = FString::Printf(TEXT("%.0f"), Value); break;
		case 1: Text = FString::Printf(TEXT("%.1f"), Value); break;
		case 2: Text = FString::Printf(TEXT("%.2f"), Value); break;
		case 3: Text = FString::Printf(TEXT("%.3f"), Value); break;
		default: Text = FString::Printf(TEXT("%.6f"), Value); break;
		}
		if (Text.Contains(TEXT(".")))
		{
			while (Text.EndsWith(TEXT("0")))
			{
				Text.LeftChopInline(1);
			}
			if (Text.EndsWith(TEXT(".")))
			{
				Text.LeftChopInline(1);
			}
		}
		if (Text == TEXT("-0"))
		{
			Text = TEXT("0");
		}
		return MakeShared<FJsonValueNumberString>(Text);
	}

	TSharedRef<FJsonValue> MakeVector(const FVector& Value)
	{
		return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeNumber(Value.X), MakeNumber(Value.Y), MakeNumber(Value.Z) });
	}

	TSharedRef<FJsonValue> MakeRotator(const FRotator& Value)
	{
		return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeNumber(Value.Pitch), MakeNumber(Value.Yaw), MakeNumber(Value.Roll) });
	}

	TSharedRef<FJsonValue> MakeVector2D(const FVector2D& Value)
	{
		return MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{ MakeNumber(Value.X, 1), MakeNumber(Value.Y, 1) });
	}

	TSharedRef<FJsonValue> MakeStringOrNull(const FString& Value, bool bNull)
	{
		if (bNull)
		{
			return MakeShared<FJsonValueNull>();
		}
		return MakeShared<FJsonValueString>(Value);
	}

	// -----------------------------------------------------------------------
	// Keys
	// -----------------------------------------------------------------------

	namespace ProtocolPrivate
	{
		static FKey FindAlias(const FString& Name)
		{
			struct FAlias { const TCHAR* Alias; const FKey* Key; };
			static const FAlias Aliases[] = {
				{ TEXT("Esc"), &EKeys::Escape },
				{ TEXT("Space"), &EKeys::SpaceBar },
				{ TEXT("Return"), &EKeys::Enter },
				{ TEXT("Shift"), &EKeys::LeftShift },
				{ TEXT("Ctrl"), &EKeys::LeftControl },
				{ TEXT("Control"), &EKeys::LeftControl },
				{ TEXT("Alt"), &EKeys::LeftAlt },
				{ TEXT("0"), &EKeys::Zero }, { TEXT("1"), &EKeys::One }, { TEXT("2"), &EKeys::Two }, { TEXT("3"), &EKeys::Three },
				{ TEXT("4"), &EKeys::Four }, { TEXT("5"), &EKeys::Five }, { TEXT("6"), &EKeys::Six }, { TEXT("7"), &EKeys::Seven },
				{ TEXT("8"), &EKeys::Eight }, { TEXT("9"), &EKeys::Nine },
			};
			for (const FAlias& Alias : Aliases)
			{
				if (Name.Equals(Alias.Alias, ESearchCase::IgnoreCase))
				{
					return *Alias.Key;
				}
			}
			return EKeys::Invalid;
		}

		static FKey FindClickButton(const FString& Name)
		{
			if (Name.Equals(TEXT("Left"), ESearchCase::IgnoreCase)) { return EKeys::LeftMouseButton; }
			if (Name.Equals(TEXT("Right"), ESearchCase::IgnoreCase)) { return EKeys::RightMouseButton; }
			if (Name.Equals(TEXT("Middle"), ESearchCase::IgnoreCase)) { return EKeys::MiddleMouseButton; }
			// Also accept the FKey names of the same three buttons
			if (Name.Equals(TEXT("LeftMouseButton"), ESearchCase::IgnoreCase)) { return EKeys::LeftMouseButton; }
			if (Name.Equals(TEXT("RightMouseButton"), ESearchCase::IgnoreCase)) { return EKeys::RightMouseButton; }
			if (Name.Equals(TEXT("MiddleMouseButton"), ESearchCase::IgnoreCase)) { return EKeys::MiddleMouseButton; }
			return EKeys::Invalid;
		}

		static bool IsAxisCommandKey(const FKey& Key)
		{
			return Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY || Key == EKeys::Gamepad_RightX
				|| Key == EKeys::Gamepad_RightY || Key == EKeys::Gamepad_LeftTriggerAxis || Key == EKeys::Gamepad_RightTriggerAxis;
		}

		static FMordecaiAgentBridgeError MakeBridgeError(const TCHAR* Code, const FString& Message)
		{
			FMordecaiAgentBridgeError Error;
			Error.Code = Code;
			Error.Message = Message;
			return Error;
		}

		/** Strictly typed optional number field; false (with error) if present but wrong. */
		static bool ReadOptionalNumber(const FJsonObject& Object, const TCHAR* Field, double& InOutValue, double Min, double Max, FMordecaiAgentBridgeError& OutError, bool& bOutPresent)
		{
			bOutPresent = false;
			const TSharedPtr<FJsonValue> Value = Object.TryGetField(Field);
			if (!Value.IsValid() || Value->Type == EJson::Null)
			{
				return true;
			}
			double Number = 0.0;
			if (Value->Type != EJson::Number || !Value->TryGetNumber(Number) || !FMath::IsFinite(Number))
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("'%s' must be a number"), Field));
				return false;
			}
			if (Number < Min || Number > Max)
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("'%s' must be in %g..%g (got %g)"), Field, Min, Max, Number));
				return false;
			}
			InOutValue = Number;
			bOutPresent = true;
			return true;
		}

		static bool ReadOptionalBool(const FJsonObject& Object, const TCHAR* Field, bool& InOutValue, FMordecaiAgentBridgeError& OutError)
		{
			const TSharedPtr<FJsonValue> Value = Object.TryGetField(Field);
			if (!Value.IsValid() || Value->Type == EJson::Null)
			{
				return true;
			}
			if (Value->Type != EJson::Boolean)
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("'%s' must be true or false"), Field));
				return false;
			}
			InOutValue = Value->AsBool();
			return true;
		}

		static bool ReadOptionalString(const FJsonObject& Object, const TCHAR* Field, FString& OutValue, bool& bOutPresent, FMordecaiAgentBridgeError& OutError)
		{
			bOutPresent = false;
			const TSharedPtr<FJsonValue> Value = Object.TryGetField(Field);
			if (!Value.IsValid() || Value->Type == EJson::Null)
			{
				return true;
			}
			if (Value->Type != EJson::String)
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("'%s' must be a string"), Field));
				return false;
			}
			OutValue = Value->AsString();
			bOutPresent = true;
			return true;
		}

		static bool ReadRoute(const FJsonObject& Object, ERoute& OutRoute, FMordecaiAgentBridgeError& OutError)
		{
			FString RouteName;
			bool bPresent = false;
			if (!ReadOptionalString(Object, TEXT("route"), RouteName, bPresent, OutError))
			{
				return false;
			}
			if (!bPresent)
			{
				OutRoute = ERoute::Slate;
				return true;
			}
			bool bValid = false;
			OutRoute = ParseRouteName(RouteName, bValid);
			if (!bValid)
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("unknown route '%s' (use \"slate\" or \"viewport\")"), *RouteName));
				return false;
			}
			return true;
		}

		/** Read required x and y viewport-pixel coordinates. */
		static bool ReadPixel(const FJsonObject& Object, FVector2D& OutPixel, FMordecaiAgentBridgeError& OutError)
		{
			double X = 0.0;
			double Y = 0.0;
			bool bHasX = false;
			bool bHasY = false;
			if (!ReadOptionalNumber(Object, TEXT("x"), X, -100000.0, 100000.0, OutError, bHasX)
				|| !ReadOptionalNumber(Object, TEXT("y"), Y, -100000.0, 100000.0, OutError, bHasY))
			{
				return false;
			}
			if (!bHasX || !bHasY)
			{
				OutError = MakeBridgeError(ErrBadRequest, TEXT("both 'x' and 'y' are required (viewport pixels)"));
				return false;
			}
			OutPixel = FVector2D(X, Y);
			return true;
		}
	}

	FKeyResolution ResolveKey(const FString& InName, EKeyUse Use, bool bAllowUnsafe)
	{
		FKeyResolution Result;
		const FString Name = InName.TrimStartAndEnd();

		FKey Key = EKeys::Invalid;
		if (Use == EKeyUse::MouseButton)
		{
			Key = ProtocolPrivate::FindClickButton(Name);
			if (!Key.IsValid())
			{
				Result.Error = ProtocolPrivate::MakeBridgeError(ErrBadKey, FString::Printf(TEXT("unknown button '%s' (use Left, Right or Middle)"), *InName));
				return Result;
			}
		}
		else
		{
			Key = ProtocolPrivate::FindAlias(Name);
			if (!Key.IsValid() && !Name.IsEmpty() && Name.Len() <= 64)
			{
				// FName lookup is case-insensitive; the registered details give the canonical key
				const FKey Candidate{ FName(*Name) };
				if (const TSharedPtr<FKeyDetails> Details = EKeys::GetKeyDetails(Candidate))
				{
					Key = Details->GetKey();
				}
			}
			if (!Key.IsValid())
			{
				Result.Error = ProtocolPrivate::MakeBridgeError(ErrBadKey, FString::Printf(TEXT("unknown key '%s'"), *InName));
				return Result;
			}

			if (Use == EKeyUse::Axis)
			{
				if (!ProtocolPrivate::IsAxisCommandKey(Key))
				{
					Result.Error = ProtocolPrivate::MakeBridgeError(ErrBadKey, FString::Printf(
						TEXT("'%s' is not an axis input (use Gamepad_LeftX, Gamepad_LeftY, Gamepad_RightX, Gamepad_RightY, Gamepad_LeftTriggerAxis or Gamepad_RightTriggerAxis)"), *InName));
					return Result;
				}
			}
			else
			{
				if (Key.IsMouseButton())
				{
					Result.Error = ProtocolPrivate::MakeBridgeError(ErrBadKey, FString::Printf(TEXT("'%s' is a mouse button: use click"), *InName));
					return Result;
				}
				if (Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D() || Key.IsTouch())
				{
					Result.Error = ProtocolPrivate::MakeBridgeError(ErrBadKey, FString::Printf(TEXT("'%s' is not a key or button: use axis or mouse"), *InName));
					return Result;
				}
				if (!bAllowUnsafe && IsDeniedDebugKey(Key))
				{
					Result.Error = ProtocolPrivate::MakeBridgeError(ErrBadKey, FString::Printf(
						TEXT("'%s' is a Development debug exec binding with side effects; pass \"unsafe\": true to send it"), *InName));
					return Result;
				}
			}
		}

		Result.bOk = true;
		Result.Key = Key;
		Result.Name = Key.GetFName().ToString();
		return Result;
	}

	bool IsDeniedDebugKey(const FKey& Key)
	{
		static const FKey* Denied[] = {
			&EKeys::K, &EKeys::Insert, &EKeys::Delete, &EKeys::Home, &EKeys::End, &EKeys::PageUp, &EKeys::PageDown,
			&EKeys::NumPadFour, &EKeys::F1, &EKeys::F2, &EKeys::F3, &EKeys::F4, &EKeys::F5, &EKeys::F6, &EKeys::F7, &EKeys::F8,
			&EKeys::F9, &EKeys::F10, &EKeys::F11, &EKeys::F12, &EKeys::Tilde, &EKeys::Semicolon, &EKeys::Gamepad_Special_Left,
		};
		for (const FKey* DeniedKey : Denied)
		{
			if (Key == *DeniedKey)
			{
				return true;
			}
		}
		return false;
	}

	bool IsModifierKey(const FKey& Key)
	{
		return Key == EKeys::LeftShift || Key == EKeys::RightShift || Key == EKeys::LeftControl || Key == EKeys::RightControl
			|| Key == EKeys::LeftAlt || Key == EKeys::RightAlt || Key == EKeys::LeftCommand || Key == EKeys::RightCommand;
	}

	FModifierKeysState MakeModifierKeysState(const TArray<FKey>& HeldKeys)
	{
		return FModifierKeysState(
			HeldKeys.Contains(EKeys::LeftShift), HeldKeys.Contains(EKeys::RightShift),
			HeldKeys.Contains(EKeys::LeftControl), HeldKeys.Contains(EKeys::RightControl),
			HeldKeys.Contains(EKeys::LeftAlt), HeldKeys.Contains(EKeys::RightAlt),
			HeldKeys.Contains(EKeys::LeftCommand), HeldKeys.Contains(EKeys::RightCommand),
			/*bAreCapsLocked*/ false);
	}

	bool IsTriggerAxis(const FKey& Key)
	{
		return Key == EKeys::Gamepad_LeftTriggerAxis || Key == EKeys::Gamepad_RightTriggerAxis;
	}

	// -----------------------------------------------------------------------
	// Coordinates
	// -----------------------------------------------------------------------

	namespace ProtocolPrivate
	{
		/** Absolute units per viewport pixel; 1 when the geometry is degenerate. */
		static FVector2D AbsolutePerPixel(const FVector2D& ViewportAbsSize, const FIntPoint& ViewportSize)
		{
			const double ScaleX = (ViewportSize.X > 0 && ViewportAbsSize.X > UE_SMALL_NUMBER) ? ViewportAbsSize.X / ViewportSize.X : 1.0;
			const double ScaleY = (ViewportSize.Y > 0 && ViewportAbsSize.Y > UE_SMALL_NUMBER) ? ViewportAbsSize.Y / ViewportSize.Y : 1.0;
			return FVector2D(ScaleX, ScaleY);
		}
	}

	FVector2D ViewportToAbsolute(const FVector2D& Pixel, const FVector2D& ViewportAbsPos, const FVector2D& ViewportAbsSize, const FIntPoint& ViewportSize)
	{
		return ViewportAbsPos + Pixel * ProtocolPrivate::AbsolutePerPixel(ViewportAbsSize, ViewportSize);
	}

	FVector2D AbsoluteToViewport(const FVector2D& Absolute, const FVector2D& ViewportAbsPos, const FVector2D& ViewportAbsSize, const FIntPoint& ViewportSize)
	{
		return (Absolute - ViewportAbsPos) / ProtocolPrivate::AbsolutePerPixel(ViewportAbsSize, ViewportSize);
	}

	// -----------------------------------------------------------------------
	// build_info.json
	// -----------------------------------------------------------------------

	FString FindBuildInfoFile(const FString& StartDir, int32 MaxParents)
	{
		if (StartDir.IsEmpty())
		{
			return FString();
		}
		FString Dir = FPaths::ConvertRelativePathToFull(StartDir);
		FPaths::NormalizeDirectoryName(Dir);
		for (int32 Level = 0; Level <= MaxParents && !Dir.IsEmpty(); ++Level)
		{
			const FString Candidate = Dir / TEXT("build_info.json");
			if (IFileManager::Get().FileExists(*Candidate))
			{
				return Candidate;
			}
			const FString Parent = FPaths::GetPath(Dir);
			if (Parent.IsEmpty() || Parent == Dir)
			{
				break;
			}
			Dir = Parent;
		}
		return FString();
	}

	TSharedPtr<FJsonObject> ReadBuildInfo(const FString& FilePath)
	{
		FString Text;
		if (FilePath.IsEmpty() || !FFileHelper::LoadFileToString(Text, *FilePath))
		{
			return nullptr;
		}
		TSharedPtr<FJsonObject> Info;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Info) || !Info.IsValid())
		{
			return nullptr;
		}

		const TSharedRef<FJsonObject> Build = MakeShared<FJsonObject>();
		const TCHAR* Fields[] = { TEXT("commit"), TEXT("branch"), TEXT("dirty"), TEXT("built_at") };
		for (const TCHAR* Field : Fields)
		{
			const TSharedPtr<FJsonValue> Value = Info->TryGetField(Field);
			Build->SetField(Field, Value.IsValid() ? Value : MakeShared<FJsonValueNull>());
		}
		return Build;
	}

	FString NormalizeExperienceName(const FString& Name)
	{
		FString Result = Name.TrimStartAndEnd();
		int32 Index = INDEX_NONE;
		if (Result.FindLastChar(TEXT(':'), Index))
		{
			Result.RightChopInline(Index + 1);
		}
		if (Result.FindLastChar(TEXT('/'), Index))
		{
			Result.RightChopInline(Index + 1);
		}
		if (Result.FindChar(TEXT('.'), Index))
		{
			Result.LeftInline(Index);
		}
		Result.RemoveFromStart(TEXT("Default__"));
		Result.RemoveFromEnd(TEXT("_C"));
		return Result;
	}

	// -----------------------------------------------------------------------
	// Command arguments
	// -----------------------------------------------------------------------

	ERoute ParseRouteName(const FString& Name, bool& bOutValid)
	{
		bOutValid = true;
		if (Name.Equals(TEXT("slate"), ESearchCase::IgnoreCase))
		{
			return ERoute::Slate;
		}
		if (Name.Equals(TEXT("viewport"), ESearchCase::IgnoreCase))
		{
			return ERoute::Viewport;
		}
		bOutValid = false;
		return ERoute::Slate;
	}

	const TCHAR* RouteToString(ERoute Route)
	{
		return Route == ERoute::Viewport ? TEXT("viewport") : TEXT("slate");
	}

	const TCHAR* KeyActionToString(EKeyAction Action)
	{
		switch (Action)
		{
		case EKeyAction::Press: return TEXT("press");
		case EKeyAction::Release: return TEXT("release");
		default: return TEXT("tap");
		}
	}

	bool ParseKeyArgs(const FJsonObject& Request, FKeyCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FKeyCommandArgs();
		using namespace ProtocolPrivate;

		if (!ReadOptionalBool(Request, TEXT("unsafe"), Out.bUnsafe, OutError) || !ReadRoute(Request, Out.Route, OutError))
		{
			return false;
		}

		FString ActionName;
		bool bHasAction = false;
		if (!ReadOptionalString(Request, TEXT("action"), ActionName, bHasAction, OutError))
		{
			return false;
		}
		if (bHasAction)
		{
			if (ActionName.Equals(TEXT("press"), ESearchCase::IgnoreCase)) { Out.Action = EKeyAction::Press; }
			else if (ActionName.Equals(TEXT("release"), ESearchCase::IgnoreCase)) { Out.Action = EKeyAction::Release; }
			else if (ActionName.Equals(TEXT("tap"), ESearchCase::IgnoreCase)) { Out.Action = EKeyAction::Tap; }
			else
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("unknown action '%s' (use press, release or tap)"), *ActionName));
				return false;
			}
		}

		bool bHasHold = false;
		if (!ReadOptionalNumber(Request, TEXT("hold_s"), Out.HoldSeconds, 0.0, 30.0, OutError, bHasHold))
		{
			return false;
		}

		const TSharedPtr<FJsonValue> KeyValue = Request.TryGetField(TEXT("key"));
		const TSharedPtr<FJsonValue> KeysValue = Request.TryGetField(TEXT("keys"));
		const bool bHasKey = KeyValue.IsValid() && KeyValue->Type != EJson::Null;
		const bool bHasKeys = KeysValue.IsValid() && KeysValue->Type != EJson::Null;
		if (bHasKey == bHasKeys)
		{
			OutError = MakeBridgeError(ErrBadRequest, bHasKey ? TEXT("give either 'key' or 'keys', not both") : TEXT("missing 'key' (or 'keys')"));
			return false;
		}

		TArray<FString> Names;
		if (bHasKey)
		{
			if (KeyValue->Type != EJson::String)
			{
				OutError = MakeBridgeError(ErrBadRequest, TEXT("'key' must be a string"));
				return false;
			}
			Names.Add(KeyValue->AsString());
		}
		else
		{
			Out.bListSyntax = true;
			if (KeysValue->Type != EJson::Array)
			{
				OutError = MakeBridgeError(ErrBadRequest, TEXT("'keys' must be an array of 1-4 key names"));
				return false;
			}
			const TArray<TSharedPtr<FJsonValue>>& Array = KeysValue->AsArray();
			if (Array.Num() < 1 || Array.Num() > 4)
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("'keys' must hold 1-4 key names (got %d)"), Array.Num()));
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Item : Array)
			{
				if (!Item.IsValid() || Item->Type != EJson::String)
				{
					OutError = MakeBridgeError(ErrBadRequest, TEXT("'keys' entries must be strings"));
					return false;
				}
				Names.Add(Item->AsString());
			}
		}

		for (const FString& Name : Names)
		{
			const FKeyResolution Resolved = ResolveKey(Name, EKeyUse::Key, Out.bUnsafe);
			if (!Resolved.bOk)
			{
				OutError = Resolved.Error;
				return false;
			}
			if (Out.Keys.Contains(Resolved.Key))
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("key '%s' is listed twice"), *Resolved.Name));
				return false;
			}
			Out.Keys.Add(Resolved.Key);
			Out.KeyNames.Add(Resolved.Name);
		}
		return true;
	}

	bool ParseAxisArgs(const FJsonObject& Request, FAxisCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FAxisCommandArgs();
		using namespace ProtocolPrivate;

		if (!ReadRoute(Request, Out.Route, OutError))
		{
			return false;
		}

		double Hold = 0.0;
		bool bHasHold = false;
		if (!ReadOptionalNumber(Request, TEXT("hold_s"), Hold, 0.0, 30.0, OutError, bHasHold))
		{
			return false;
		}
		if (bHasHold)
		{
			Out.HoldSeconds = Hold;
		}

		const TSharedPtr<FJsonValue> Values = Request.TryGetField(TEXT("values"));
		if (!Values.IsValid() || Values->Type != EJson::Object)
		{
			OutError = MakeBridgeError(ErrBadRequest, TEXT("'values' must be an object of axis name -> value"));
			return false;
		}
		const TSharedPtr<FJsonObject> ValueObject = Values->AsObject();
		if (!ValueObject.IsValid() || ValueObject->Values.Num() == 0)
		{
			OutError = MakeBridgeError(ErrBadRequest, TEXT("'values' is empty"));
			return false;
		}

		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : ValueObject->Values)
		{
			const FKeyResolution Resolved = ResolveKey(Pair.Key, EKeyUse::Axis);
			if (!Resolved.bOk)
			{
				OutError = Resolved.Error;
				return false;
			}
			double Number = 0.0;
			if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Number || !Pair.Value->TryGetNumber(Number) || !FMath::IsFinite(Number))
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("value for '%s' must be a number"), *Pair.Key));
				return false;
			}
			const double Min = IsTriggerAxis(Resolved.Key) ? 0.0 : -1.0;
			if (Number < Min || Number > 1.0)
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("value for '%s' must be in %g..1 (got %g)"), *Resolved.Name, Min, Number));
				return false;
			}
			if (Out.Keys.Contains(Resolved.Key))
			{
				OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("axis '%s' is listed twice"), *Resolved.Name));
				return false;
			}
			Out.Keys.Add(Resolved.Key);
			Out.KeyNames.Add(Resolved.Name);
			Out.Values.Add(static_cast<float>(Number));
		}
		return true;
	}

	bool ParseMouseArgs(const FJsonObject& Request, FMouseCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FMouseCommandArgs();
		return ProtocolPrivate::ReadPixel(Request, Out.Pixel, OutError);
	}

	bool ParseClickArgs(const FJsonObject& Request, FClickCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FClickCommandArgs();
		using namespace ProtocolPrivate;

		const bool bHasX = Request.HasField(TEXT("x")) && !Request.HasTypedField<EJson::Null>(TEXT("x"));
		const bool bHasY = Request.HasField(TEXT("y")) && !Request.HasTypedField<EJson::Null>(TEXT("y"));
		if (bHasX || bHasY)
		{
			FVector2D Pixel;
			if (!ReadPixel(Request, Pixel, OutError))
			{
				return false;
			}
			Out.Pixel = Pixel;
		}

		FString ButtonName = TEXT("Left");
		bool bHasButton = false;
		if (!ReadOptionalString(Request, TEXT("button"), ButtonName, bHasButton, OutError))
		{
			return false;
		}
		const FKeyResolution Button = ResolveKey(ButtonName, EKeyUse::MouseButton);
		if (!Button.bOk)
		{
			OutError = Button.Error;
			return false;
		}
		Out.Button = Button.Key;
		Out.ButtonName = Button.Key == EKeys::RightMouseButton ? TEXT("Right") : (Button.Key == EKeys::MiddleMouseButton ? TEXT("Middle") : TEXT("Left"));

		bool bHasHold = false;
		return ReadOptionalNumber(Request, TEXT("hold_s"), Out.HoldSeconds, 0.0, 30.0, OutError, bHasHold);
	}

	bool ParseWaitArgs(const FJsonObject& Request, FWaitCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FWaitCommandArgs();
		bool bHasSeconds = false;
		if (!ProtocolPrivate::ReadOptionalNumber(Request, TEXT("seconds"), Out.Seconds, 0.0, 600.0, OutError, bHasSeconds))
		{
			return false;
		}
		if (!bHasSeconds)
		{
			OutError = ProtocolPrivate::MakeBridgeError(ErrBadRequest, TEXT("missing 'seconds' (0..600)"));
			return false;
		}
		return true;
	}

	bool ParseWaitUntilArgs(const FJsonObject& Request, FWaitUntilCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FWaitUntilCommandArgs();
		using namespace ProtocolPrivate;

		bool bHasCondition = false;
		if (!ReadOptionalString(Request, TEXT("condition"), Out.Condition, bHasCondition, OutError))
		{
			return false;
		}
		Out.Condition.TrimStartAndEndInline();
		if (!bHasCondition || Out.Condition.IsEmpty())
		{
			OutError = MakeBridgeError(ErrBadRequest, TEXT("missing 'condition'"));
			return false;
		}
		bool bPresent = false;
		return ReadOptionalNumber(Request, TEXT("timeout_s"), Out.TimeoutSeconds, 0.0, 600.0, OutError, bPresent)
			&& ReadOptionalNumber(Request, TEXT("poll_s"), Out.PollSeconds, 0.0, 600.0, OutError, bPresent);
	}

	bool ParseStateArgs(const FJsonObject& Request, FStateCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FStateCommandArgs();
		using namespace ProtocolPrivate;

		const TSharedPtr<FJsonValue> Sections = Request.TryGetField(TEXT("sections"));
		if (Sections.IsValid() && Sections->Type != EJson::Null)
		{
			if (Sections->Type != EJson::Array)
			{
				OutError = MakeBridgeError(ErrBadRequest, TEXT("'sections' must be an array of section names"));
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Item : Sections->AsArray())
			{
				const FString Name = Item.IsValid() && Item->Type == EJson::String ? Item->AsString() : FString();
				if (!GetStateSectionNames().Contains(Name))
				{
					OutError = MakeBridgeError(ErrBadRequest, FString::Printf(TEXT("unknown state section '%s' (use %s)"),
						*Name, *FString::Join(GetStateSectionNames(), TEXT(", "))));
					return false;
				}
				Out.Sections.AddUnique(Name);
			}
		}
		if (Out.Sections.Num() == 0)
		{
			Out.Sections = GetStateSectionNames();
		}

		bool bPresent = false;
		if (!ReadOptionalNumber(Request, TEXT("radius"), Out.Radius, 0.0, 1.0e7, OutError, bPresent))
		{
			return false;
		}
		if (bPresent && Out.Radius <= 0.0)
		{
			OutError = MakeBridgeError(ErrBadRequest, TEXT("'radius' must be greater than 0"));
			return false;
		}
		return ReadOptionalBool(Request, TEXT("include_all_actors"), Out.bIncludeAllActors, OutError);
	}

	bool ParseScreenshotArgs(const FJsonObject& Request, FScreenshotCommandArgs& Out, FMordecaiAgentBridgeError& OutError)
	{
		Out = FScreenshotCommandArgs();
		bool bPresent = false;
		if (!ProtocolPrivate::ReadOptionalString(Request, TEXT("path"), Out.Path, bPresent, OutError))
		{
			return false;
		}
		Out.Path.TrimStartAndEndInline();
		return true;
	}

	const TArray<FString>& GetStateSectionNames()
	{
		static const TArray<FString> Names = {
			TEXT("game"), TEXT("focus"), TEXT("player"), TEXT("input"), TEXT("pickups"),
			TEXT("inventory"), TEXT("pause_menu"), TEXT("widgets"), TEXT("actors"),
		};
		return Names;
	}

	// -----------------------------------------------------------------------
	// Gamepad axis threshold buttons
	// -----------------------------------------------------------------------

	bool GetAxisThresholdButtons(const FKey& Axis, FAxisThresholdButtons& OutButtons)
	{
		OutButtons = FAxisThresholdButtons();
		// The pairs and thresholds XInputInterface.cpp SendControllerEvents uses
		if (Axis == EKeys::Gamepad_LeftX) { OutButtons = { EKeys::Gamepad_LeftStick_Left, EKeys::Gamepad_LeftStick_Right, XInputLeftThumbDeadZone, false }; }
		else if (Axis == EKeys::Gamepad_LeftY) { OutButtons = { EKeys::Gamepad_LeftStick_Down, EKeys::Gamepad_LeftStick_Up, XInputLeftThumbDeadZone, false }; }
		else if (Axis == EKeys::Gamepad_RightX) { OutButtons = { EKeys::Gamepad_RightStick_Left, EKeys::Gamepad_RightStick_Right, XInputRightThumbDeadZone, false }; }
		else if (Axis == EKeys::Gamepad_RightY) { OutButtons = { EKeys::Gamepad_RightStick_Down, EKeys::Gamepad_RightStick_Up, XInputRightThumbDeadZone, false }; }
		else if (Axis == EKeys::Gamepad_LeftTriggerAxis) { OutButtons = { EKeys::Invalid, EKeys::Gamepad_LeftTrigger, XInputTriggerThreshold, true }; }
		else if (Axis == EKeys::Gamepad_RightTriggerAxis) { OutButtons = { EKeys::Invalid, EKeys::Gamepad_RightTrigger, XInputTriggerThreshold, true }; }
		else
		{
			return false;
		}
		return true;
	}

	int32 AxisValueToXInputRaw(const FKey& Axis, float Value)
	{
		if (IsTriggerAxis(Axis))
		{
			return FMath::Clamp(FMath::RoundToInt(FMath::Clamp(Value, 0.f, 1.f) * 255.f), 0, 255);
		}
		// Inverse of XInputInterface.cpp ShortToNormalizedFloat: [-32768..32767] -> [-1..1]
		const float Clamped = FMath::Clamp(Value, -1.f, 1.f);
		return FMath::RoundToInt(Clamped * (Clamped <= 0.f ? 32768.f : 32767.f));
	}

	TArray<FKey> GetAxisThresholdKeysDown(const FKey& Axis, float Value)
	{
		TArray<FKey> Down;
		FAxisThresholdButtons Buttons;
		if (!GetAxisThresholdButtons(Axis, Buttons))
		{
			return Down;
		}
		const int32 Raw = AxisValueToXInputRaw(Axis, Value);
		if (Raw > Buttons.Threshold)
		{
			Down.Add(Buttons.Positive);
		}
		else if (!Buttons.bTrigger && Raw < -Buttons.Threshold)
		{
			Down.Add(Buttons.Negative);
		}
		return Down;
	}

	// -----------------------------------------------------------------------
	// Ids
	// -----------------------------------------------------------------------

	FString CanonicalNameId(const FName& Id)
	{
		return Id.ToString().ToLower();
	}

	bool TryCanonicalGuid(const FString& Text, FString& OutGuid)
	{
		OutGuid.Reset();
		FGuid Guid;
		if (Text.IsEmpty() || !FGuid::Parse(Text, Guid) || !Guid.IsValid())
		{
			return false;
		}
		// The same format inventory.items[].instance_id uses (FGuid::ToString(), EGuidFormats::Digits)
		OutGuid = Guid.ToString();
		return true;
	}

	FString CanonicalButtonId(const FString& IdText)
	{
		FString Guid;
		return TryCanonicalGuid(IdText, Guid) ? Guid : IdText.ToLower();
	}

	bool ShouldRestoreUserFocus(ERoute Route, bool bFocusPathHasLiveWidget, bool bMenuInputMode)
	{
		return Route == ERoute::Slate && !bFocusPathHasLiveWidget && !bMenuInputMode;
	}
}

// ---------------------------------------------------------------------------
// Auto-repeat
// ---------------------------------------------------------------------------

FMordecaiAgentBridgeAutoRepeat::FMordecaiAgentBridgeAutoRepeat(double InKeyboardDelay, double InKeyboardInterval, double InGamepadInitialDelay, double InGamepadRepeatDelay)
	: KeyboardDelay(InKeyboardDelay)
	, KeyboardInterval(InKeyboardInterval)
	, GamepadInitialDelay(InGamepadInitialDelay)
	, GamepadRepeatDelay(InGamepadRepeatDelay)
{
}

void FMordecaiAgentBridgeAutoRepeat::SetGamepadTiming(double InInitialDelay, double InRepeatDelay)
{
	GamepadInitialDelay = FMath::Max(0.0, InInitialDelay);
	GamepadRepeatDelay = FMath::Max(0.0, InRepeatDelay);
}

void FMordecaiAgentBridgeAutoRepeat::KeyDown(const FKey& Key, double Now)
{
	if (!Key.IsValid())
	{
		return;
	}
	if (Key.IsGamepadKey())
	{
		// XInput: every button on its own schedule, first after InitialButtonRepeatDelay
		GamepadNextTimes.RemoveAll([&](const TPair<FKey, double>& Entry) { return Entry.Key == Key; });
		GamepadNextTimes.Add({ Key, Now + GamepadInitialDelay });
		return;
	}
	// Typematic: the newest key takes over the repeat; the previous one never resumes
	TypematicKey = Key;
	TypematicNextTime = Now + KeyboardDelay;
}

void FMordecaiAgentBridgeAutoRepeat::KeyUp(const FKey& Key)
{
	if (Key.IsGamepadKey())
	{
		GamepadNextTimes.RemoveAll([&](const TPair<FKey, double>& Entry) { return Entry.Key == Key; });
		return;
	}
	if (TypematicKey.IsValid() && Key == TypematicKey)
	{
		TypematicKey = EKeys::Invalid;
	}
}

TArray<FKey> FMordecaiAgentBridgeAutoRepeat::CollectRepeats(double Now)
{
	TArray<FKey> Due;
	if (TypematicKey.IsValid() && Now >= TypematicNextTime)
	{
		Due.Add(TypematicKey);
		TypematicNextTime = Now + KeyboardInterval;
	}
	for (TPair<FKey, double>& Entry : GamepadNextTimes)
	{
		if (Now >= Entry.Value)
		{
			Due.Add(Entry.Key);
			// As XInputInterface.cpp: the next repeat is ButtonRepeatDelay after this one was sent
			Entry.Value = Now + GamepadRepeatDelay;
		}
	}
	return Due;
}

void FMordecaiAgentBridgeAutoRepeat::Reset()
{
	TypematicKey = EKeys::Invalid;
	TypematicNextTime = 0.0;
	GamepadNextTimes.Reset();
}

// ---------------------------------------------------------------------------
// Input union
// ---------------------------------------------------------------------------

bool FMordecaiAgentBridgeInputUnion::AddKey(int32 Holder, const FKey& Key, MordecaiAgentBridge::ERoute Route)
{
	FKeyHold* Hold = Keys.FindByPredicate([&](const FKeyHold& Entry) { return Entry.Key == Key; });
	if (!Hold)
	{
		FKeyHold& NewHold = Keys.AddDefaulted_GetRef();
		NewHold.Key = Key;
		NewHold.Route = Route;
		NewHold.Holders.Add(Holder);
		return true;
	}
	Hold->Holders.AddUnique(Holder);
	return false;
}

bool FMordecaiAgentBridgeInputUnion::RemoveKey(int32 Holder, const FKey& Key, bool& bOutWasHeld)
{
	bOutWasHeld = false;
	const int32 Index = Keys.IndexOfByPredicate([&](const FKeyHold& Entry) { return Entry.Key == Key; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	bOutWasHeld = Keys[Index].Holders.Remove(Holder) > 0;
	if (bOutWasHeld && Keys[Index].Holders.Num() == 0)
	{
		Keys.RemoveAt(Index);
		return true;
	}
	return false;
}

bool FMordecaiAgentBridgeInputUnion::HoldsKey(int32 Holder, const FKey& Key) const
{
	const FKeyHold* Hold = Keys.FindByPredicate([&](const FKeyHold& Entry) { return Entry.Key == Key; });
	return Hold && Hold->Holders.Contains(Holder);
}

bool FMordecaiAgentBridgeInputUnion::IsKeyDown(const FKey& Key) const
{
	return Keys.ContainsByPredicate([&](const FKeyHold& Entry) { return Entry.Key == Key; });
}

int32 FMordecaiAgentBridgeInputUnion::CountKeyHolders(const FKey& Key, bool bIncludeSynthesized) const
{
	const FKeyHold* Hold = Keys.FindByPredicate([&](const FKeyHold& Entry) { return Entry.Key == Key; });
	if (!Hold)
	{
		return 0;
	}
	int32 Count = 0;
	for (const int32 Holder : Hold->Holders)
	{
		Count += (bIncludeSynthesized || Holder != SynthesizedHolder) ? 1 : 0;
	}
	return Count;
}

MordecaiAgentBridge::ERoute FMordecaiAgentBridgeInputUnion::GetKeyRoute(const FKey& Key) const
{
	const FKeyHold* Hold = Keys.FindByPredicate([&](const FKeyHold& Entry) { return Entry.Key == Key; });
	return Hold ? Hold->Route : MordecaiAgentBridge::ERoute::Slate;
}

TArray<FKey> FMordecaiAgentBridgeInputUnion::GetKeysHeldBy(int32 Holder) const
{
	TArray<FKey> Out;
	for (const FKeyHold& Hold : Keys)
	{
		if (Hold.Holders.Contains(Holder))
		{
			Out.Add(Hold.Key);
		}
	}
	return Out;
}

TArray<FKey> FMordecaiAgentBridgeInputUnion::GetDownKeys() const
{
	TArray<FKey> Out;
	for (const FKeyHold& Hold : Keys)
	{
		Out.Add(Hold.Key);
	}
	return Out;
}

void FMordecaiAgentBridgeInputUnion::SetAxis(int32 Holder, const FKey& Axis, float Value, MordecaiAgentBridge::ERoute Route)
{
	const int32 Index = Axes.IndexOfByPredicate([&](const FAxisHold& Entry) { return Entry.Key == Axis; });
	if (Value == 0.f)
	{
		if (Index != INDEX_NONE)
		{
			Axes[Index].Values.RemoveAll([&](const TPair<int32, float>& Entry) { return Entry.Key == Holder; });
			if (Axes[Index].Values.Num() == 0)
			{
				Axes.RemoveAt(Index);
			}
		}
		return;
	}
	FAxisHold* Hold = Index != INDEX_NONE ? &Axes[Index] : nullptr;
	if (!Hold)
	{
		Hold = &Axes.AddDefaulted_GetRef();
		Hold->Key = Axis;
		Hold->Route = Route;
	}
	if (TPair<int32, float>* Existing = Hold->Values.FindByPredicate([&](const TPair<int32, float>& Entry) { return Entry.Key == Holder; }))
	{
		Existing->Value = Value;
	}
	else
	{
		Hold->Values.Add({ Holder, Value });
	}
}

float FMordecaiAgentBridgeInputUnion::GetAxisValue(const FKey& Axis) const
{
	const FAxisHold* Hold = Axes.FindByPredicate([&](const FAxisHold& Entry) { return Entry.Key == Axis; });
	float Best = 0.f;
	if (Hold)
	{
		for (const TPair<int32, float>& Entry : Hold->Values)
		{
			if (FMath::Abs(Entry.Value) > FMath::Abs(Best))
			{
				Best = Entry.Value;
			}
		}
	}
	return Best;
}

int32 FMordecaiAgentBridgeInputUnion::CountAxisHolders(const FKey& Axis) const
{
	const FAxisHold* Hold = Axes.FindByPredicate([&](const FAxisHold& Entry) { return Entry.Key == Axis; });
	return Hold ? Hold->Values.Num() : 0;
}

MordecaiAgentBridge::ERoute FMordecaiAgentBridgeInputUnion::GetAxisRoute(const FKey& Axis) const
{
	const FAxisHold* Hold = Axes.FindByPredicate([&](const FAxisHold& Entry) { return Entry.Key == Axis; });
	return Hold ? Hold->Route : MordecaiAgentBridge::ERoute::Slate;
}

TArray<FKey> FMordecaiAgentBridgeInputUnion::GetAxesHeldBy(int32 Holder) const
{
	TArray<FKey> Out;
	for (const FAxisHold& Hold : Axes)
	{
		if (Hold.Values.ContainsByPredicate([&](const TPair<int32, float>& Entry) { return Entry.Key == Holder; }))
		{
			Out.Add(Hold.Key);
		}
	}
	return Out;
}

TArray<FKey> FMordecaiAgentBridgeInputUnion::GetHeldAxes() const
{
	TArray<FKey> Out;
	for (const FAxisHold& Hold : Axes)
	{
		Out.Add(Hold.Key);
	}
	return Out;
}

void FMordecaiAgentBridgeInputUnion::Reset()
{
	Keys.Reset();
	Axes.Reset();
}
