// Project Mordecai — Agent bridge protocol tests (Specs/agent_bridge_v1.md §2-§5)
// Headless tests for the pure helpers of the in-game agent bridge: command-line config, NDJSON
// framing, request parsing and response building, key-name resolution, command arguments,
// viewport <-> absolute coordinates, build_info.json lookup and experience-name normalization.
// The socket and the input injection are proven at Tier 4, not here.

#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"

namespace MordecaiAgentBridgeProtocolTestHelpers
{
	static TSharedPtr<FJsonObject> ParseObject(const FString& Json)
	{
		TSharedPtr<FJsonObject> Object;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		FJsonSerializer::Deserialize(Reader, Object);
		return Object;
	}

	static void AppendString(FMordecaiAgentBridgeLineFramer& Framer, const FString& Text, TArray<FMordecaiAgentBridgeFramedLine>& Out)
	{
		const FTCHARToUTF8 Utf8(*Text);
		Framer.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), Out);
	}

	/** Resolve and require success; returns the canonical name ("" on failure). */
	static FString ResolveName(const FString& Name, MordecaiAgentBridge::EKeyUse Use, bool bUnsafe = false)
	{
		const MordecaiAgentBridge::FKeyResolution R = MordecaiAgentBridge::ResolveKey(Name, Use, bUnsafe);
		return R.bOk ? R.Name : FString();
	}
}

// ===========================================================================
// Config (§2 command line)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Config_DisabledWithoutExactFlag,
	"Mordecai.Playtest.AgentBridge.Config.DisabledWithoutExactFlag",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Config_DisabledWithoutExactFlag::RunTest(const FString& Parameters)
{
	TestFalse("Normal launch has no bridge",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("/MordecaiCore/Game/Maps/DevTestMap -windowed -log")).bEnabled);
	TestFalse("-AgentBridgePort= alone does not enable it",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("-AgentBridgePort=47801 -AgentBridgeToken=x -AgentBridgeShotDir=C:/Shots")).bEnabled);
	TestFalse("A longer token is not the flag",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("-AgentBridgeX -log")).bEnabled);
	TestTrue("The exact token enables it",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("/Map -AgentBridge -log")).bEnabled);
	TestTrue("The token at the end of the line enables it",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("-log -AgentBridge")).bEnabled);
	TestFalse("Null command line", FMordecaiAgentBridgeConfig::FromCommandLine(nullptr).bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Config_ParsesSettings,
	"Mordecai.Playtest.AgentBridge.Config.ParsesSettings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Config_ParsesSettings::RunTest(const FString& Parameters)
{
	const FMordecaiAgentBridgeConfig Config = FMordecaiAgentBridgeConfig::FromCommandLine(
		TEXT("/MordecaiCore/Game/Maps/DevTestMap -AgentBridge -AgentBridgePort=47801 -AgentBridgeToken=smoke-1234 -AgentBridgeShotDir=\"C:/Play Tests/Run 1\" -log"));
	TestTrue("Enabled", Config.bEnabled);
	TestEqual("Port", Config.Port, 47801);
	TestEqual("Token", Config.Token, FString(TEXT("smoke-1234")));
	TestEqual("Quoted shot dir with spaces", Config.ShotDir, FString(TEXT("C:/Play Tests/Run 1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Config_Defaults,
	"Mordecai.Playtest.AgentBridge.Config.Defaults",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Config_Defaults::RunTest(const FString& Parameters)
{
	const FMordecaiAgentBridgeConfig Config = FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("-AgentBridge"));
	TestEqual("Default port", Config.Port, 47777);
	TestTrue("Default token is empty", Config.Token.IsEmpty());
	TestTrue("Default shot dir is <ProjectSavedDir>/Playtests/Bridge",
		Config.ShotDir.StartsWith(FPaths::ProjectSavedDir()) && Config.ShotDir.EndsWith(TEXT("Playtests/Bridge")));

	TestEqual("Port 0 falls back to the default",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("-AgentBridge -AgentBridgePort=0")).Port, 47777);
	TestEqual("Port above 65535 falls back to the default",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("-AgentBridge -AgentBridgePort=70000")).Port, 47777);
	TestEqual("Non-numeric port falls back to the default",
		FMordecaiAgentBridgeConfig::FromCommandLine(TEXT("-AgentBridge -AgentBridgePort=abc")).Port, 47777);
	return true;
}

// ===========================================================================
// Framer (§3)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Framer_SplitsLines,
	"Mordecai.Playtest.AgentBridge.Framer.SplitsLines",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Framer_SplitsLines::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	FMordecaiAgentBridgeLineFramer Framer;
	TArray<FMordecaiAgentBridgeFramedLine> Lines;
	AppendString(Framer, TEXT("{\"id\":1,\"cmd\":\"ping\"}\n{\"id\":2,\"cmd\":\"state\"}\n"), Lines);
	if (!TestEqual("Two lines", Lines.Num(), 2))
	{
		return true;
	}
	TestEqual("First line", Lines[0].Text, FString(TEXT("{\"id\":1,\"cmd\":\"ping\"}")));
	TestEqual("Second line", Lines[1].Text, FString(TEXT("{\"id\":2,\"cmd\":\"state\"}")));
	TestFalse("Not oversize", Lines[0].bOversize || Lines[1].bOversize);
	TestEqual("Nothing pending", Framer.GetPendingBytes(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Framer_AcceptsCrLfAndSkipsBlankLines,
	"Mordecai.Playtest.AgentBridge.Framer.AcceptsCrLfAndSkipsBlankLines",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Framer_AcceptsCrLfAndSkipsBlankLines::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	FMordecaiAgentBridgeLineFramer Framer;
	TArray<FMordecaiAgentBridgeFramedLine> Lines;
	AppendString(Framer, TEXT("first\r\n\n\r\n   \t \nsecond\r\n"), Lines);
	if (!TestEqual("Blank lines are dropped", Lines.Num(), 2))
	{
		return true;
	}
	TestEqual("CRLF stripped from the first line", Lines[0].Text, FString(TEXT("first")));
	TestEqual("CRLF stripped from the second line", Lines[1].Text, FString(TEXT("second")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Framer_BuffersPartialLines,
	"Mordecai.Playtest.AgentBridge.Framer.BuffersPartialLines",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Framer_BuffersPartialLines::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	FMordecaiAgentBridgeLineFramer Framer;
	TArray<FMordecaiAgentBridgeFramedLine> Lines;

	AppendString(Framer, TEXT("{\"cmd\":"), Lines);
	TestEqual("No line before the newline", Lines.Num(), 0);
	TestEqual("Partial bytes buffered", Framer.GetPendingBytes(), 7);

	AppendString(Framer, TEXT("\"ping\"}\n{\"cmd\""), Lines);
	if (TestEqual("Line completes when the newline arrives", Lines.Num(), 1))
	{
		TestEqual("Joined line", Lines[0].Text, FString(TEXT("{\"cmd\":\"ping\"}")));
	}
	TestEqual("The next partial line stays buffered", Framer.GetPendingBytes(), 6);

	// A multibyte UTF-8 character split across two receives (e = U+00E9: C3 A9)
	FMordecaiAgentBridgeLineFramer Utf8Framer;
	TArray<FMordecaiAgentBridgeFramedLine> Utf8Lines;
	const uint8 PartA[] = { 'h', 0xC3 };
	const uint8 PartB[] = { 0xA9, 'l', 'l', 'o', '\n' };
	Utf8Framer.Append(PartA, UE_ARRAY_COUNT(PartA), Utf8Lines);
	Utf8Framer.Append(PartB, UE_ARRAY_COUNT(PartB), Utf8Lines);
	if (TestEqual("UTF-8 line", Utf8Lines.Num(), 1))
	{
		TestEqual("UTF-8 decoded after the split", Utf8Lines[0].Text, FString(TEXT("h\u00E9llo")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Framer_OversizeLineIsDiscarded,
	"Mordecai.Playtest.AgentBridge.Framer.OversizeLineIsDiscarded",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Framer_OversizeLineIsDiscarded::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;

	{
		FMordecaiAgentBridgeLineFramer Framer(8);
		TArray<FMordecaiAgentBridgeFramedLine> Lines;
		AppendString(Framer, TEXT("0123456789"), Lines);
		if (TestEqual("One oversize marker while the line is still arriving", Lines.Num(), 1))
		{
			TestTrue("Marked oversize", Lines[0].bOversize);
		}
		TestEqual("Oversize bytes are not kept", Framer.GetPendingBytes(), 0);

		AppendString(Framer, TEXT("abc\nok\n"), Lines);
		if (TestEqual("The rest of the long line is discarded up to the newline", Lines.Num(), 2))
		{
			TestFalse("Next line is normal", Lines[1].bOversize);
			TestEqual("Next line text", Lines[1].Text, FString(TEXT("ok")));
		}
	}

	{
		FMordecaiAgentBridgeLineFramer Framer(8);
		TArray<FMordecaiAgentBridgeFramedLine> Lines;
		AppendString(Framer, TEXT("0123456789AB\n01234567\nok\n"), Lines);
		if (TestEqual("Oversize, exact-limit and normal lines", Lines.Num(), 3))
		{
			TestTrue("A complete over-long line is oversize", Lines[0].bOversize);
			TestFalse("A line exactly at the limit is accepted", Lines[1].bOversize);
			TestEqual("Exact-limit text", Lines[1].Text, FString(TEXT("01234567")));
			TestEqual("Following line", Lines[2].Text, FString(TEXT("ok")));
		}
	}
	return true;
}

// ===========================================================================
// Requests and responses (§3)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Request_EchoesIds,
	"Mordecai.Playtest.AgentBridge.Request.EchoesIds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Request_EchoesIds::RunTest(const FString& Parameters)
{
	FMordecaiAgentBridgeRequest Request;
	FString Message;

	TestTrue("Numeric id parses", MordecaiAgentBridge::ParseRequest(TEXT("{\"id\":7,\"cmd\":\"ping\"}"), Request, Message));
	TestEqual("Cmd", Request.Cmd, FString(TEXT("ping")));
	TestEqual("Numeric id echoed",
		MordecaiAgentBridge::ToJsonLine(MordecaiAgentBridge::MakeOkResponse(Request.Id, Request.Cmd)),
		FString(TEXT("{\"id\":7,\"ok\":true,\"cmd\":\"ping\"}")));

	TestTrue("Fractional id parses", MordecaiAgentBridge::ParseRequest(TEXT("{\"id\":1.5,\"cmd\":\"ping\"}"), Request, Message));
	TestEqual("Fractional id echoed verbatim",
		MordecaiAgentBridge::ToJsonLine(MordecaiAgentBridge::MakeOkResponse(Request.Id, Request.Cmd)),
		FString(TEXT("{\"id\":1.5,\"ok\":true,\"cmd\":\"ping\"}")));

	TestTrue("String id parses", MordecaiAgentBridge::ParseRequest(TEXT("{\"cmd\":\"ping\",\"id\":\"abc-1\"}"), Request, Message));
	TestEqual("String id echoed",
		MordecaiAgentBridge::ToJsonLine(MordecaiAgentBridge::MakeOkResponse(Request.Id, Request.Cmd)),
		FString(TEXT("{\"id\":\"abc-1\",\"ok\":true,\"cmd\":\"ping\"}")));

	TestTrue("Missing id parses", MordecaiAgentBridge::ParseRequest(TEXT("{\"cmd\":\"ping\"}"), Request, Message));
	TestEqual("Missing id is null",
		MordecaiAgentBridge::ToJsonLine(MordecaiAgentBridge::MakeOkResponse(Request.Id, Request.Cmd)),
		FString(TEXT("{\"id\":null,\"ok\":true,\"cmd\":\"ping\"}")));

	TestTrue("Arguments sit next to cmd",
		MordecaiAgentBridge::ParseRequest(TEXT("{\"id\":3,\"cmd\":\"key\",\"key\":\"F\",\"action\":\"tap\"}"), Request, Message));
	TestTrue("Args object kept", Request.Args.IsValid() && Request.Args->GetStringField(TEXT("key")) == TEXT("F"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Request_RejectsMalformed,
	"Mordecai.Playtest.AgentBridge.Request.RejectsMalformed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Request_RejectsMalformed::RunTest(const FString& Parameters)
{
	FString Message;
	{
		FMordecaiAgentBridgeRequest Request;
		TestFalse("Bad JSON", MordecaiAgentBridge::ParseRequest(TEXT("{not json"), Request, Message));
		TestFalse("Bad JSON has a message", Message.IsEmpty());
		TestTrue("Bad JSON id is null", !Request.Id.IsValid() || Request.Id->IsNull());
	}
	{
		FMordecaiAgentBridgeRequest Request;
		TestFalse("Array is not a request", MordecaiAgentBridge::ParseRequest(TEXT("[1,2,3]"), Request, Message));
	}
	{
		FMordecaiAgentBridgeRequest Request;
		TestFalse("Missing cmd", MordecaiAgentBridge::ParseRequest(TEXT("{\"id\":3}"), Request, Message));
		TestTrue("Missing cmd keeps the id for the reply", Request.Id.IsValid() && !Request.Id->IsNull());
		const FString Line = MordecaiAgentBridge::ToJsonLine(
			MordecaiAgentBridge::MakeErrorResponse(Request.Id, Request.Cmd, MordecaiAgentBridge::ErrBadRequest, Message));
		TestTrue("Error reply echoes the id and nulls cmd",
			Line.StartsWith(TEXT("{\"id\":3,\"ok\":false,\"cmd\":null,\"error\":\"bad_request\",\"message\":\"")));
	}
	{
		FMordecaiAgentBridgeRequest Request;
		TestFalse("cmd must be a string", MordecaiAgentBridge::ParseRequest(TEXT("{\"id\":1,\"cmd\":5}"), Request, Message));
	}
	{
		FMordecaiAgentBridgeRequest Request;
		TestFalse("Empty cmd", MordecaiAgentBridge::ParseRequest(TEXT("{\"id\":1,\"cmd\":\"\"}"), Request, Message));
	}
	{
		FMordecaiAgentBridgeRequest Request;
		TestFalse("id must be a number or string", MordecaiAgentBridge::ParseRequest(TEXT("{\"id\":true,\"cmd\":\"ping\"}"), Request, Message));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Response_ShapeAndNoRawNewlines,
	"Mordecai.Playtest.AgentBridge.Response.ShapeAndNoRawNewlines",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Response_ShapeAndNoRawNewlines::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;

	const TSharedRef<FJsonObject> Ok = MordecaiAgentBridge::MakeOkResponse(MakeShared<FJsonValueNumber>(9), TEXT("state"));
	Ok->SetStringField(TEXT("text"), TEXT("line one\nline two\r\n\ttabbed"));
	const FString OkLine = MordecaiAgentBridge::ToJsonLine(Ok);
	TestFalse("No raw LF", OkLine.Contains(TEXT("\n")));
	TestFalse("No raw CR", OkLine.Contains(TEXT("\r")));
	TestTrue("Starts with id, ok, cmd", OkLine.StartsWith(TEXT("{\"id\":9,\"ok\":true,\"cmd\":\"state\"")));
	const TSharedPtr<FJsonObject> Reparsed = ParseObject(OkLine);
	TestTrue("Round trip keeps the newlines", Reparsed.IsValid() && Reparsed->GetStringField(TEXT("text")) == TEXT("line one\nline two\r\n\ttabbed"));

	const TSharedRef<FJsonObject> Err = MordecaiAgentBridge::MakeErrorResponse(MakeShared<FJsonValueString>(TEXT("x")), TEXT("key"),
		MordecaiAgentBridge::ErrBadKey, TEXT("unknown key 'Foo'"));
	TestEqual("Error reply",
		MordecaiAgentBridge::ToJsonLine(Err),
		FString(TEXT("{\"id\":\"x\",\"ok\":false,\"cmd\":\"key\",\"error\":\"bad_key\",\"message\":\"unknown key 'Foo'\"}")));

	TestEqual("Compact numbers", MordecaiAgentBridge::ToJsonLine(MakeShared<FJsonObject>()), FString(TEXT("{}")));
	{
		const TSharedRef<FJsonObject> Numbers = MakeShared<FJsonObject>();
		Numbers->SetField(TEXT("a"), MordecaiAgentBridge::MakeNumber(600.0));
		Numbers->SetField(TEXT("b"), MordecaiAgentBridge::MakeNumber(0.1));
		Numbers->SetField(TEXT("c"), MordecaiAgentBridge::MakeNumber(-12.34567));
		Numbers->SetField(TEXT("d"), MordecaiAgentBridge::MakeNumber(-0.0001));
		Numbers->SetField(TEXT("e"), MordecaiAgentBridge::MakeNumber(TNumericLimits<double>::Max() * 2.0));
		Numbers->SetField(TEXT("v"), MordecaiAgentBridge::MakeVector(FVector(400.0, 0.0, 30.25)));
		Numbers->SetField(TEXT("r"), MordecaiAgentBridge::MakeRotator(FRotator(-50.0, 0.0, 0.0)));
		TestEqual("Numbers are compact and non-finite is null",
			MordecaiAgentBridge::ToJsonLine(Numbers),
			FString(TEXT("{\"a\":600,\"b\":0.1,\"c\":-12.346,\"d\":0,\"e\":null,\"v\":[400,0,30.25],\"r\":[-50,0,0]}")));
	}
	return true;
}

// ===========================================================================
// Keys (§5)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Keys_ResolvesNamesCaseInsensitively,
	"Mordecai.Playtest.AgentBridge.Keys.ResolvesNamesCaseInsensitively",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Keys_ResolvesNamesCaseInsensitively::RunTest(const FString& Parameters)
{
	using MordecaiAgentBridge::EKeyUse;
	const struct { const TCHAR* Input; FKey Expected; } Cases[] = {
		{ TEXT("W"), EKeys::W }, { TEXT("w"), EKeys::W }, { TEXT("F"), EKeys::F }, { TEXT("i"), EKeys::I },
		{ TEXT("Escape"), EKeys::Escape }, { TEXT("escape"), EKeys::Escape }, { TEXT("SpaceBar"), EKeys::SpaceBar },
		{ TEXT("spacebar"), EKeys::SpaceBar }, { TEXT("Tab"), EKeys::Tab }, { TEXT("One"), EKeys::One },
		{ TEXT("BackSlash"), EKeys::Backslash }, { TEXT("LeftShift"), EKeys::LeftShift }, { TEXT("q"), EKeys::Q },
	};
	for (const auto& Case : Cases)
	{
		const MordecaiAgentBridge::FKeyResolution R = MordecaiAgentBridge::ResolveKey(Case.Input, EKeyUse::Key);
		TestTrue(FString::Printf(TEXT("'%s' resolves"), Case.Input), R.bOk);
		TestTrue(FString::Printf(TEXT("'%s' is %s"), Case.Input, *Case.Expected.ToString()), R.Key == Case.Expected);
		TestEqual(FString::Printf(TEXT("'%s' canonical name"), Case.Input), R.Name, Case.Expected.GetFName().ToString());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Keys_Aliases,
	"Mordecai.Playtest.AgentBridge.Keys.Aliases",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Keys_Aliases::RunTest(const FString& Parameters)
{
	using MordecaiAgentBridge::EKeyUse;
	const struct { const TCHAR* Input; FKey Expected; } Cases[] = {
		{ TEXT("Esc"), EKeys::Escape }, { TEXT("esc"), EKeys::Escape }, { TEXT("Space"), EKeys::SpaceBar },
		{ TEXT("Enter"), EKeys::Enter }, { TEXT("Return"), EKeys::Enter }, { TEXT("Shift"), EKeys::LeftShift },
		{ TEXT("Ctrl"), EKeys::LeftControl }, { TEXT("Alt"), EKeys::LeftAlt }, { TEXT("0"), EKeys::Zero },
		{ TEXT("1"), EKeys::One }, { TEXT("5"), EKeys::Five }, { TEXT("9"), EKeys::Nine },
		{ TEXT("Up"), EKeys::Up }, { TEXT("Down"), EKeys::Down }, { TEXT("left"), EKeys::Left }, { TEXT("Right"), EKeys::Right },
	};
	for (const auto& Case : Cases)
	{
		const MordecaiAgentBridge::FKeyResolution R = MordecaiAgentBridge::ResolveKey(Case.Input, EKeyUse::Key);
		TestTrue(FString::Printf(TEXT("Alias '%s' resolves"), Case.Input), R.bOk && R.Key == Case.Expected);
		TestEqual(FString::Printf(TEXT("Alias '%s' reports the FKey name"), Case.Input), R.Name, Case.Expected.GetFName().ToString());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Keys_RejectsInvalidAndWrongKind,
	"Mordecai.Playtest.AgentBridge.Keys.RejectsInvalidAndWrongKind",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Keys_RejectsInvalidAndWrongKind::RunTest(const FString& Parameters)
{
	using MordecaiAgentBridge::EKeyUse;

	const MordecaiAgentBridge::FKeyResolution Foo = MordecaiAgentBridge::ResolveKey(TEXT("Foo"), EKeyUse::Key);
	TestFalse("Unknown name", Foo.bOk);
	TestEqual("Unknown name is bad_key", Foo.Error.Code, FString(MordecaiAgentBridge::ErrBadKey));
	TestTrue("Message names the key", Foo.Error.Message.Contains(TEXT("Foo")));

	TestEqual("Empty name is bad_key", MordecaiAgentBridge::ResolveKey(TEXT(""), EKeyUse::Key).Error.Code, FString(MordecaiAgentBridge::ErrBadKey));
	TestFalse("The game's broken stick name is not a key", MordecaiAgentBridge::ResolveKey(TEXT("Gamepad_LeftStick2D"), EKeyUse::Key).bOk);
	TestFalse("'None' is not a key", MordecaiAgentBridge::ResolveKey(TEXT("None"), EKeyUse::Key).bOk);

	const MordecaiAgentBridge::FKeyResolution Lmb = MordecaiAgentBridge::ResolveKey(TEXT("LeftMouseButton"), EKeyUse::Key);
	TestFalse("Mouse buttons are rejected by key", Lmb.bOk);
	TestEqual("Mouse button rejection is bad_key", Lmb.Error.Code, FString(MordecaiAgentBridge::ErrBadKey));
	TestFalse("Mouse axes are rejected by key", MordecaiAgentBridge::ResolveKey(TEXT("MouseX"), EKeyUse::Key).bOk);
	TestFalse("Stick axes are rejected by key", MordecaiAgentBridge::ResolveKey(TEXT("Gamepad_LeftX"), EKeyUse::Key).bOk);
	TestFalse("2D axes are rejected by key", MordecaiAgentBridge::ResolveKey(TEXT("Gamepad_Left2D"), EKeyUse::Key).bOk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Keys_DenyListAndUnsafeOverride,
	"Mordecai.Playtest.AgentBridge.Keys.DenyListAndUnsafeOverride",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Keys_DenyListAndUnsafeOverride::RunTest(const FString& Parameters)
{
	using MordecaiAgentBridge::EKeyUse;
	const TCHAR* Denied[] = {
		TEXT("K"), TEXT("k"), TEXT("Insert"), TEXT("Delete"), TEXT("Home"), TEXT("End"), TEXT("PageUp"), TEXT("PageDown"),
		TEXT("NumPadFour"), TEXT("F1"), TEXT("F5"), TEXT("F9"), TEXT("F11"), TEXT("F12"), TEXT("Tilde"), TEXT("Semicolon"),
		TEXT("Gamepad_Special_Left"),
	};
	for (const TCHAR* Name : Denied)
	{
		const MordecaiAgentBridge::FKeyResolution Safe = MordecaiAgentBridge::ResolveKey(Name, EKeyUse::Key, /*bAllowUnsafe*/ false);
		TestFalse(FString::Printf(TEXT("'%s' denied by default"), Name), Safe.bOk);
		TestEqual(FString::Printf(TEXT("'%s' denial is bad_key"), Name), Safe.Error.Code, FString(MordecaiAgentBridge::ErrBadKey));
		TestTrue(FString::Printf(TEXT("'%s' allowed with unsafe"), Name), MordecaiAgentBridge::ResolveKey(Name, EKeyUse::Key, /*bAllowUnsafe*/ true).bOk);
	}
	TestTrue("K is on the deny list", MordecaiAgentBridge::IsDeniedDebugKey(EKeys::K));
	TestFalse("J is not", MordecaiAgentBridge::IsDeniedDebugKey(EKeys::J));
	TestFalse("F (interact) is not", MordecaiAgentBridge::IsDeniedDebugKey(EKeys::F));
	TestTrue("J needs no unsafe", MordecaiAgentBridge::ResolveKey(TEXT("J"), EKeyUse::Key).bOk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Keys_GamepadButtonsAxesAndMouseButtons,
	"Mordecai.Playtest.AgentBridge.Keys.GamepadButtonsAxesAndMouseButtons",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Keys_GamepadButtonsAxesAndMouseButtons::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	using MordecaiAgentBridge::EKeyUse;

	// Gamepad buttons go through key
	TestEqual("Face button bottom", ResolveName(TEXT("Gamepad_FaceButton_Bottom"), EKeyUse::Key), EKeys::Gamepad_FaceButton_Bottom.GetFName().ToString());
	TestEqual("Face button left", ResolveName(TEXT("gamepad_facebutton_left"), EKeyUse::Key), EKeys::Gamepad_FaceButton_Left.GetFName().ToString());
	TestEqual("Start", ResolveName(TEXT("Gamepad_Special_Right"), EKeyUse::Key), EKeys::Gamepad_Special_Right.GetFName().ToString());
	TestEqual("Left shoulder", ResolveName(TEXT("Gamepad_LeftShoulder"), EKeyUse::Key), EKeys::Gamepad_LeftShoulder.GetFName().ToString());
	TestEqual("DPad up", ResolveName(TEXT("Gamepad_DPad_Up"), EKeyUse::Key), EKeys::Gamepad_DPad_Up.GetFName().ToString());

	// The six analog axes go through axis, nothing else does
	const FKey AxisKeys[] = { EKeys::Gamepad_LeftX, EKeys::Gamepad_LeftY, EKeys::Gamepad_RightX, EKeys::Gamepad_RightY,
		EKeys::Gamepad_LeftTriggerAxis, EKeys::Gamepad_RightTriggerAxis };
	for (const FKey& AxisKey : AxisKeys)
	{
		TestEqual(FString::Printf(TEXT("Axis %s"), *AxisKey.ToString()), ResolveName(AxisKey.ToString(), EKeyUse::Axis), AxisKey.GetFName().ToString());
	}
	TestEqual("Axis names are case-insensitive", ResolveName(TEXT("gamepad_lefty"), EKeyUse::Axis), EKeys::Gamepad_LeftY.GetFName().ToString());
	TestFalse("W is not an axis", MordecaiAgentBridge::ResolveKey(TEXT("W"), EKeyUse::Axis).bOk);
	TestFalse("2D keys are not axis inputs", MordecaiAgentBridge::ResolveKey(TEXT("Gamepad_Left2D"), EKeyUse::Axis).bOk);
	TestFalse("Mouse axes are not axis inputs", MordecaiAgentBridge::ResolveKey(TEXT("MouseX"), EKeyUse::Axis).bOk);
	TestTrue("Left trigger is a trigger", MordecaiAgentBridge::IsTriggerAxis(EKeys::Gamepad_LeftTriggerAxis));
	TestTrue("Right trigger is a trigger", MordecaiAgentBridge::IsTriggerAxis(EKeys::Gamepad_RightTriggerAxis));
	TestFalse("Left X is a stick", MordecaiAgentBridge::IsTriggerAxis(EKeys::Gamepad_LeftX));

	// Click buttons
	TestEqual("Left", ResolveName(TEXT("Left"), EKeyUse::MouseButton), EKeys::LeftMouseButton.GetFName().ToString());
	TestEqual("right", ResolveName(TEXT("right"), EKeyUse::MouseButton), EKeys::RightMouseButton.GetFName().ToString());
	TestEqual("Middle", ResolveName(TEXT("Middle"), EKeyUse::MouseButton), EKeys::MiddleMouseButton.GetFName().ToString());
	const MordecaiAgentBridge::FKeyResolution Thumb = MordecaiAgentBridge::ResolveKey(TEXT("Thumb"), EKeyUse::MouseButton);
	TestFalse("Thumb is not a click button", Thumb.bOk);
	TestEqual("Bad click button is bad_key", Thumb.Error.Code, FString(MordecaiAgentBridge::ErrBadKey));
	TestFalse("W is not a click button", MordecaiAgentBridge::ResolveKey(TEXT("W"), EKeyUse::MouseButton).bOk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Keys_ModifierStateFromHeldKeys,
	"Mordecai.Playtest.AgentBridge.Keys.ModifierStateFromHeldKeys",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Keys_ModifierStateFromHeldKeys::RunTest(const FString& Parameters)
{
	TestTrue("LeftShift is a modifier", MordecaiAgentBridge::IsModifierKey(EKeys::LeftShift));
	TestTrue("RightCommand is a modifier", MordecaiAgentBridge::IsModifierKey(EKeys::RightCommand));
	TestFalse("W is not a modifier", MordecaiAgentBridge::IsModifierKey(EKeys::W));

	const FModifierKeysState None = MordecaiAgentBridge::MakeModifierKeysState({ EKeys::W, EKeys::F });
	TestFalse("No modifiers held", None.AnyModifiersDown());

	const FModifierKeysState Mods = MordecaiAgentBridge::MakeModifierKeysState({ EKeys::LeftShift, EKeys::RightControl, EKeys::LeftAlt, EKeys::W });
	TestTrue("Left shift", Mods.IsLeftShiftDown());
	TestFalse("Right shift", Mods.IsRightShiftDown());
	TestFalse("Left control", Mods.IsLeftControlDown());
	TestTrue("Right control", Mods.IsRightControlDown());
	TestTrue("Left alt", Mods.IsLeftAltDown());
	TestFalse("Command", Mods.IsCommandDown());
	TestFalse("Caps lock never set", Mods.AreCapsLocked());
	return true;
}

// ===========================================================================
// Command arguments (§4)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Args_KeyCommand,
	"Mordecai.Playtest.AgentBridge.Args.KeyCommand",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Args_KeyCommand::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	using MordecaiAgentBridge::EKeyAction;
	using MordecaiAgentBridge::ERoute;
	FMordecaiAgentBridgeError Error;

	{
		MordecaiAgentBridge::FKeyCommandArgs Args;
		TestTrue("Single key", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"cmd\":\"key\",\"key\":\"F\"}")), Args, Error));
		TestTrue("Key F", Args.Keys.Num() == 1 && Args.Keys[0] == EKeys::F);
		TestEqual("Key name", Args.KeyNames.Num() == 1 ? Args.KeyNames[0] : FString(), FString(TEXT("F")));
		TestTrue("Default action is tap", Args.Action == EKeyAction::Tap);
		TestEqual("Default hold", Args.HoldSeconds, 0.1);
		TestTrue("Default route is slate", Args.Route == ERoute::Slate);
		TestFalse("Default not unsafe", Args.bUnsafe);
		TestFalse("Single-key syntax", Args.bListSyntax);
	}
	{
		MordecaiAgentBridge::FKeyCommandArgs Args;
		TestTrue("Key list", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"cmd\":\"key\",\"keys\":[\"W\",\"d\"],\"action\":\"tap\",\"hold_s\":0.3}")), Args, Error));
		TestTrue("Two keys in order", Args.Keys.Num() == 2 && Args.Keys[0] == EKeys::W && Args.Keys[1] == EKeys::D);
		TestEqual("Hold", Args.HoldSeconds, 0.3);
		TestTrue("List syntax", Args.bListSyntax);
	}
	{
		MordecaiAgentBridge::FKeyCommandArgs Args;
		TestTrue("Press", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"key\":\"W\",\"action\":\"press\"}")), Args, Error));
		TestTrue("Press action", Args.Action == EKeyAction::Press);
		TestTrue("Release", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"key\":\"W\",\"action\":\"release\"}")), Args, Error));
		TestTrue("Release action", Args.Action == EKeyAction::Release);
		TestTrue("Viewport route", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"key\":\"W\",\"route\":\"viewport\"}")), Args, Error));
		TestTrue("Viewport route parsed", Args.Route == ERoute::Viewport);
		TestTrue("Unsafe K", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"key\":\"K\",\"unsafe\":true}")), Args, Error));
		TestTrue("Hold 30 is the maximum", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"key\":\"W\",\"hold_s\":30}")), Args, Error));
		TestTrue("Hold 0 allowed", MordecaiAgentBridge::ParseKeyArgs(*ParseObject(TEXT("{\"key\":\"W\",\"hold_s\":0}")), Args, Error));
	}

	const struct { const TCHAR* Json; const TCHAR* Code; } Bad[] = {
		{ TEXT("{\"cmd\":\"key\"}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"W\",\"keys\":[\"D\"]}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"keys\":[]}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"keys\":[\"W\",\"A\",\"S\",\"D\",\"F\"]}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"keys\":[\"W\",\"w\"]}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"keys\":\"W\"}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":5}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"W\",\"action\":\"hold\"}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"W\",\"hold_s\":30.5}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"W\",\"hold_s\":-1}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"W\",\"hold_s\":\"0.3\"}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"W\",\"route\":\"os\"}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"W\",\"unsafe\":\"yes\"}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"key\":\"Foo\"}"), MordecaiAgentBridge::ErrBadKey },
		{ TEXT("{\"key\":\"K\"}"), MordecaiAgentBridge::ErrBadKey },
		{ TEXT("{\"key\":\"LeftMouseButton\"}"), MordecaiAgentBridge::ErrBadKey },
		{ TEXT("{\"keys\":[\"W\",\"F1\"]}"), MordecaiAgentBridge::ErrBadKey },
	};
	for (const auto& Case : Bad)
	{
		MordecaiAgentBridge::FKeyCommandArgs Args;
		FMordecaiAgentBridgeError CaseError;
		TestFalse(FString::Printf(TEXT("Rejects %s"), Case.Json), MordecaiAgentBridge::ParseKeyArgs(*ParseObject(Case.Json), Args, CaseError));
		TestEqual(FString::Printf(TEXT("Error code for %s"), Case.Json), CaseError.Code, FString(Case.Code));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Args_AxisCommand,
	"Mordecai.Playtest.AgentBridge.Args.AxisCommand",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Args_AxisCommand::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	FMordecaiAgentBridgeError Error;
	{
		MordecaiAgentBridge::FAxisCommandArgs Args;
		TestTrue("Axis with hold", MordecaiAgentBridge::ParseAxisArgs(*ParseObject(TEXT("{\"cmd\":\"axis\",\"values\":{\"Gamepad_LeftX\":0.0,\"Gamepad_LeftY\":1.0},\"hold_s\":0.5}")), Args, Error));
		TestEqual("Two axes", Args.Keys.Num(), 2);
		const int32 YIndex = Args.Keys.IndexOfByKey(EKeys::Gamepad_LeftY);
		TestTrue("LeftY = 1", YIndex != INDEX_NONE && Args.Values[YIndex] == 1.f);
		TestTrue("Hold set", Args.HoldSeconds.IsSet() && Args.HoldSeconds.GetValue() == 0.5);
	}
	{
		MordecaiAgentBridge::FAxisCommandArgs Args;
		TestTrue("Persistent axis", MordecaiAgentBridge::ParseAxisArgs(*ParseObject(TEXT("{\"values\":{\"gamepad_lefty\":-1,\"Gamepad_RightTriggerAxis\":1}}")), Args, Error));
		TestFalse("No hold", Args.HoldSeconds.IsSet());
		TestTrue("Canonical name", Args.KeyNames.Contains(EKeys::Gamepad_LeftY.GetFName().ToString()));
	}
	const struct { const TCHAR* Json; const TCHAR* Code; } Bad[] = {
		{ TEXT("{\"cmd\":\"axis\"}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":[1]}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":{}}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":{\"Gamepad_LeftY\":1.5}}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":{\"Gamepad_LeftY\":-1.01}}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":{\"Gamepad_LeftTriggerAxis\":-0.5}}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":{\"Gamepad_LeftY\":\"1\"}}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":{\"Gamepad_LeftY\":1},\"hold_s\":31}"), MordecaiAgentBridge::ErrBadRequest },
		{ TEXT("{\"values\":{\"W\":1}}"), MordecaiAgentBridge::ErrBadKey },
		{ TEXT("{\"values\":{\"Gamepad_Left2D\":1}}"), MordecaiAgentBridge::ErrBadKey },
	};
	for (const auto& Case : Bad)
	{
		MordecaiAgentBridge::FAxisCommandArgs Args;
		FMordecaiAgentBridgeError CaseError;
		TestFalse(FString::Printf(TEXT("Rejects %s"), Case.Json), MordecaiAgentBridge::ParseAxisArgs(*ParseObject(Case.Json), Args, CaseError));
		TestEqual(FString::Printf(TEXT("Error code for %s"), Case.Json), CaseError.Code, FString(Case.Code));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Args_PointerCommands,
	"Mordecai.Playtest.AgentBridge.Args.PointerCommands",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Args_PointerCommands::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	FMordecaiAgentBridgeError Error;
	{
		MordecaiAgentBridge::FMouseCommandArgs Args;
		TestTrue("Mouse", MordecaiAgentBridge::ParseMouseArgs(*ParseObject(TEXT("{\"cmd\":\"mouse\",\"x\":640,\"y\":360.5}")), Args, Error));
		TestEqual("Mouse position", Args.Pixel, FVector2D(640.0, 360.5));
		TestFalse("Mouse needs y", MordecaiAgentBridge::ParseMouseArgs(*ParseObject(TEXT("{\"x\":640}")), Args, Error));
		TestEqual("Missing y is bad_request", Error.Code, FString(MordecaiAgentBridge::ErrBadRequest));
		TestFalse("Mouse x must be a number", MordecaiAgentBridge::ParseMouseArgs(*ParseObject(TEXT("{\"x\":\"640\",\"y\":1}")), Args, Error));
	}
	{
		MordecaiAgentBridge::FClickCommandArgs Args;
		TestTrue("Click with position", MordecaiAgentBridge::ParseClickArgs(*ParseObject(TEXT("{\"cmd\":\"click\",\"x\":812,\"y\":402,\"button\":\"Left\",\"hold_s\":0.08}")), Args, Error));
		TestTrue("Click position", Args.Pixel.IsSet() && Args.Pixel.GetValue() == FVector2D(812.0, 402.0));
		TestTrue("Left button", Args.Button == EKeys::LeftMouseButton);
		TestEqual("Click hold", Args.HoldSeconds, 0.08);
	}
	{
		MordecaiAgentBridge::FClickCommandArgs Args;
		TestTrue("Click at the cursor", MordecaiAgentBridge::ParseClickArgs(*ParseObject(TEXT("{\"cmd\":\"click\"}")), Args, Error));
		TestFalse("No position", Args.Pixel.IsSet());
		TestTrue("Default button is Left", Args.Button == EKeys::LeftMouseButton);
		TestEqual("Default hold", Args.HoldSeconds, 0.05);
		TestTrue("Right button", MordecaiAgentBridge::ParseClickArgs(*ParseObject(TEXT("{\"button\":\"Right\"}")), Args, Error) && Args.Button == EKeys::RightMouseButton);
	}
	{
		MordecaiAgentBridge::FClickCommandArgs Args;
		TestFalse("x without y", MordecaiAgentBridge::ParseClickArgs(*ParseObject(TEXT("{\"x\":5}")), Args, Error));
		TestEqual("x without y is bad_request", Error.Code, FString(MordecaiAgentBridge::ErrBadRequest));
		TestFalse("Unknown button", MordecaiAgentBridge::ParseClickArgs(*ParseObject(TEXT("{\"button\":\"Thumb\"}")), Args, Error));
		TestEqual("Unknown button is bad_key", Error.Code, FString(MordecaiAgentBridge::ErrBadKey));
		TestFalse("Hold too long", MordecaiAgentBridge::ParseClickArgs(*ParseObject(TEXT("{\"hold_s\":31}")), Args, Error));
		TestEqual("Hold too long is bad_request", Error.Code, FString(MordecaiAgentBridge::ErrBadRequest));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Args_WaitStateAndScreenshot,
	"Mordecai.Playtest.AgentBridge.Args.WaitStateAndScreenshot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Args_WaitStateAndScreenshot::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeProtocolTestHelpers;
	FMordecaiAgentBridgeError Error;

	// wait
	{
		MordecaiAgentBridge::FWaitCommandArgs Args;
		TestTrue("Wait", MordecaiAgentBridge::ParseWaitArgs(*ParseObject(TEXT("{\"cmd\":\"wait\",\"seconds\":1.5}")), Args, Error));
		TestEqual("Wait seconds", Args.Seconds, 1.5);
		TestTrue("Wait 0", MordecaiAgentBridge::ParseWaitArgs(*ParseObject(TEXT("{\"seconds\":0}")), Args, Error));
		TestTrue("Wait 600", MordecaiAgentBridge::ParseWaitArgs(*ParseObject(TEXT("{\"seconds\":600}")), Args, Error));
		TestFalse("Wait needs seconds", MordecaiAgentBridge::ParseWaitArgs(*ParseObject(TEXT("{}")), Args, Error));
		TestFalse("Wait above 600", MordecaiAgentBridge::ParseWaitArgs(*ParseObject(TEXT("{\"seconds\":600.5}")), Args, Error));
		TestFalse("Negative wait", MordecaiAgentBridge::ParseWaitArgs(*ParseObject(TEXT("{\"seconds\":-1}")), Args, Error));
		TestEqual("Bad wait is bad_request", Error.Code, FString(MordecaiAgentBridge::ErrBadRequest));
	}
	// wait_until
	{
		MordecaiAgentBridge::FWaitUntilCommandArgs Args;
		TestTrue("wait_until", MordecaiAgentBridge::ParseWaitUntilArgs(*ParseObject(TEXT("{\"cmd\":\"wait_until\",\"condition\":\"inventory.count>=4\",\"timeout_s\":20,\"poll_s\":0.25}")), Args, Error));
		TestEqual("Condition", Args.Condition, FString(TEXT("inventory.count>=4")));
		TestEqual("Timeout", Args.TimeoutSeconds, 20.0);
		TestEqual("Poll", Args.PollSeconds, 0.25);
		TestTrue("wait_until defaults", MordecaiAgentBridge::ParseWaitUntilArgs(*ParseObject(TEXT("{\"condition\":\"game.ready\"}")), Args, Error));
		TestEqual("Default timeout", Args.TimeoutSeconds, 10.0);
		TestEqual("Default poll", Args.PollSeconds, 0.1);
		TestFalse("Needs a condition", MordecaiAgentBridge::ParseWaitUntilArgs(*ParseObject(TEXT("{}")), Args, Error));
		TestFalse("Empty condition", MordecaiAgentBridge::ParseWaitUntilArgs(*ParseObject(TEXT("{\"condition\":\"  \"}")), Args, Error));
		TestFalse("Timeout above 600", MordecaiAgentBridge::ParseWaitUntilArgs(*ParseObject(TEXT("{\"condition\":\"a\",\"timeout_s\":601}")), Args, Error));
		TestFalse("Negative timeout", MordecaiAgentBridge::ParseWaitUntilArgs(*ParseObject(TEXT("{\"condition\":\"a\",\"timeout_s\":-1}")), Args, Error));
		TestFalse("Negative poll", MordecaiAgentBridge::ParseWaitUntilArgs(*ParseObject(TEXT("{\"condition\":\"a\",\"poll_s\":-0.1}")), Args, Error));
		TestEqual("Bad wait_until is bad_request", Error.Code, FString(MordecaiAgentBridge::ErrBadRequest));
	}
	// state
	{
		MordecaiAgentBridge::FStateCommandArgs Args;
		TestTrue("State defaults", MordecaiAgentBridge::ParseStateArgs(*ParseObject(TEXT("{\"cmd\":\"state\"}")), Args, Error));
		TestEqual("All sections by default", Args.Sections, MordecaiAgentBridge::GetStateSectionNames());
		TestEqual("Default radius", Args.Radius, 1500.0);
		TestFalse("Level geometry excluded by default", Args.bIncludeAllActors);
		TestTrue("Section subset", MordecaiAgentBridge::ParseStateArgs(*ParseObject(TEXT("{\"sections\":[\"inventory\",\"pickups\"],\"radius\":800,\"include_all_actors\":true}")), Args, Error));
		TestEqual("Two sections", Args.Sections, TArray<FString>({ TEXT("inventory"), TEXT("pickups") }));
		TestEqual("Radius", Args.Radius, 800.0);
		TestTrue("Include all", Args.bIncludeAllActors);
		TestTrue("Empty list means all", MordecaiAgentBridge::ParseStateArgs(*ParseObject(TEXT("{\"sections\":[]}")), Args, Error) && Args.Sections.Num() == 9);
		TestFalse("Unknown section", MordecaiAgentBridge::ParseStateArgs(*ParseObject(TEXT("{\"sections\":[\"bogus\"]}")), Args, Error));
		TestFalse("Sections must be an array", MordecaiAgentBridge::ParseStateArgs(*ParseObject(TEXT("{\"sections\":\"inventory\"}")), Args, Error));
		TestFalse("Radius must be positive", MordecaiAgentBridge::ParseStateArgs(*ParseObject(TEXT("{\"radius\":0}")), Args, Error));
		TestEqual("Bad state is bad_request", Error.Code, FString(MordecaiAgentBridge::ErrBadRequest));
	}
	// screenshot
	{
		MordecaiAgentBridge::FScreenshotCommandArgs Args;
		TestTrue("Default screenshot path", MordecaiAgentBridge::ParseScreenshotArgs(*ParseObject(TEXT("{\"cmd\":\"screenshot\"}")), Args, Error) && Args.Path.IsEmpty());
		TestTrue("Explicit screenshot path", MordecaiAgentBridge::ParseScreenshotArgs(*ParseObject(TEXT("{\"path\":\"C:/Runs/bridge_07.png\"}")), Args, Error));
		TestEqual("Path kept", Args.Path, FString(TEXT("C:/Runs/bridge_07.png")));
		TestFalse("Path must be a string", MordecaiAgentBridge::ParseScreenshotArgs(*ParseObject(TEXT("{\"path\":5}")), Args, Error));
	}
	// section names and routes
	TestEqual("Section names", MordecaiAgentBridge::GetStateSectionNames(), TArray<FString>({ TEXT("game"), TEXT("focus"), TEXT("player"), TEXT("input"),
		TEXT("pickups"), TEXT("inventory"), TEXT("pause_menu"), TEXT("widgets"), TEXT("actors") }));
	bool bValid = false;
	TestTrue("slate route", MordecaiAgentBridge::ParseRouteName(TEXT("Slate"), bValid) == MordecaiAgentBridge::ERoute::Slate && bValid);
	TestTrue("viewport route", MordecaiAgentBridge::ParseRouteName(TEXT("viewport"), bValid) == MordecaiAgentBridge::ERoute::Viewport && bValid);
	MordecaiAgentBridge::ParseRouteName(TEXT("os"), bValid);
	TestFalse("Unknown route", bValid);
	TestEqual("Route names", FString(MordecaiAgentBridge::RouteToString(MordecaiAgentBridge::ERoute::Viewport)), FString(TEXT("viewport")));
	TestEqual("Action names", FString(MordecaiAgentBridge::KeyActionToString(MordecaiAgentBridge::EKeyAction::Tap)), FString(TEXT("tap")));
	return true;
}

// ===========================================================================
// Coordinates (§4 units)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Coords_ViewportAndAbsolute,
	"Mordecai.Playtest.AgentBridge.Coords.ViewportAndAbsolute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Coords_ViewportAndAbsolute::RunTest(const FString& Parameters)
{
	// A 1280x720 viewport drawn into a 1920x1080 client area at desktop (320,224): 150% DPI scaling
	const FVector2D VpPos(320.0, 224.0);
	const FVector2D VpSize(1920.0, 1080.0);
	const FIntPoint Viewport(1280, 720);

	TestEqual("Centre to absolute", MordecaiAgentBridge::ViewportToAbsolute(FVector2D(640.0, 360.0), VpPos, VpSize, Viewport), FVector2D(1280.0, 764.0));
	TestEqual("Origin to absolute", MordecaiAgentBridge::ViewportToAbsolute(FVector2D::ZeroVector, VpPos, VpSize, Viewport), VpPos);
	TestEqual("Absolute to viewport", MordecaiAgentBridge::AbsoluteToViewport(FVector2D(1280.0, 764.0), VpPos, VpSize, Viewport), FVector2D(640.0, 360.0));
	TestEqual("Bottom-right corner", MordecaiAgentBridge::AbsoluteToViewport(VpPos + VpSize, VpPos, VpSize, Viewport), FVector2D(1280.0, 720.0));

	const FVector2D Samples[] = { FVector2D(812.0, 402.0), FVector2D(1.5, 719.0), FVector2D(-10.0, 50.0) };
	for (const FVector2D& Pixel : Samples)
	{
		const FVector2D Back = MordecaiAgentBridge::AbsoluteToViewport(
			MordecaiAgentBridge::ViewportToAbsolute(Pixel, VpPos, VpSize, Viewport), VpPos, VpSize, Viewport);
		TestTrue(FString::Printf(TEXT("Round trip %s"), *Pixel.ToString()), Back.Equals(Pixel, 1e-6));
	}

	// Unscaled: viewport pixels are absolute pixels offset by the window position
	TestEqual("Unscaled", MordecaiAgentBridge::ViewportToAbsolute(FVector2D(10.0, 20.0), FVector2D(100.0, 200.0), FVector2D(1280.0, 720.0), Viewport), FVector2D(110.0, 220.0));

	// Degenerate geometry never yields NaN
	const FVector2D ZeroAbs = MordecaiAgentBridge::AbsoluteToViewport(FVector2D(5.0, 5.0), VpPos, FVector2D::ZeroVector, Viewport);
	const FVector2D ZeroVp = MordecaiAgentBridge::ViewportToAbsolute(FVector2D(5.0, 5.0), VpPos, VpSize, FIntPoint::ZeroValue);
	TestTrue("Zero absolute size stays finite", FMath::IsFinite(ZeroAbs.X) && FMath::IsFinite(ZeroAbs.Y));
	TestTrue("Zero viewport size stays finite", FMath::IsFinite(ZeroVp.X) && FMath::IsFinite(ZeroVp.Y));
	return true;
}

// ===========================================================================
// build_info.json (§4 ping)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_BuildInfo_FoundInParentDirectories,
	"Mordecai.Playtest.AgentBridge.BuildInfo.FoundInParentDirectories",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_BuildInfo_FoundInParentDirectories::RunTest(const FString& Parameters)
{
	const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") /
		FString::Printf(TEXT("AgentBridgeBuildInfo_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	// Root/Playtest/build_info.json ; deepest dir is 7 levels below Playtest
	const FString Archive = Root / TEXT("Playtest");
	const FString Win64 = Archive / TEXT("Windows/ProjectMordecai/Binaries/Win64");        // 4 below the archive
	const FString SixBelow = Archive / TEXT("a/b/c/d/e/f");
	const FString SevenBelow = SixBelow / TEXT("g");
	IFileManager::Get().MakeDirectory(*Win64, true);
	IFileManager::Get().MakeDirectory(*SevenBelow, true);

	const FString InfoPath = Archive / TEXT("build_info.json");
	FFileHelper::SaveStringToFile(
		TEXT("{\"commit\":\"abc123def\",\"branch\":\"feature/playtest-phase2\",\"dirty\":false,\"dirty_files\":[],\"built_at\":\"2026-10-20T10:00:00-06:00\",\"config\":\"Development\"}"),
		*InfoPath);

	TestEqual("Found 4 levels up (packaged layout)", FPaths::ConvertRelativePathToFull(MordecaiAgentBridge::FindBuildInfoFile(Win64)), InfoPath);
	TestEqual("Found in the start directory", FPaths::ConvertRelativePathToFull(MordecaiAgentBridge::FindBuildInfoFile(Archive)), InfoPath);
	TestEqual("Found 6 levels up", FPaths::ConvertRelativePathToFull(MordecaiAgentBridge::FindBuildInfoFile(SixBelow)), InfoPath);
	TestTrue("Not found 7 levels up", MordecaiAgentBridge::FindBuildInfoFile(SevenBelow).IsEmpty());
	TestTrue("Not found with a smaller limit", MordecaiAgentBridge::FindBuildInfoFile(Win64, 3).IsEmpty());
	TestTrue("Empty start directory", MordecaiAgentBridge::FindBuildInfoFile(FString()).IsEmpty());

	const TSharedPtr<FJsonObject> Build = MordecaiAgentBridge::ReadBuildInfo(InfoPath);
	if (TestTrue("build_info.json parsed", Build.IsValid()))
	{
		TestEqual("commit", Build->GetStringField(TEXT("commit")), FString(TEXT("abc123def")));
		TestEqual("branch", Build->GetStringField(TEXT("branch")), FString(TEXT("feature/playtest-phase2")));
		TestFalse("dirty", Build->GetBoolField(TEXT("dirty")));
		TestEqual("built_at", Build->GetStringField(TEXT("built_at")), FString(TEXT("2026-10-20T10:00:00-06:00")));
		TestFalse("Only the ping subset is kept", Build->HasField(TEXT("dirty_files")) || Build->HasField(TEXT("config")));
	}

	const FString BadPath = Root / TEXT("bad_build_info.json");
	FFileHelper::SaveStringToFile(TEXT("{not json"), *BadPath);
	TestFalse("Unreadable JSON is null", MordecaiAgentBridge::ReadBuildInfo(BadPath).IsValid());
	TestFalse("Missing file is null", MordecaiAgentBridge::ReadBuildInfo(Root / TEXT("missing.json")).IsValid());

	IFileManager::Get().DeleteDirectory(*Root, /*RequireExists*/ false, /*Tree*/ true);
	return true;
}

// ===========================================================================
// Ids (§4: FName-derived ids clients compare exactly)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Ids_NameIdsAreLowercase,
	"Mordecai.Playtest.AgentBridge.Ids.NameIdsAreLowercase",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Ids_NameIdsAreLowercase::RunTest(const FString& Parameters)
{
	// A cooked game does not preserve FName case: the tab id FName("inventory") prints as the
	// spelling registered first ("Inventory", from the BindToInventory parameter). This editor build
	// preserves case, so the cooked spelling is used directly to stand in for it.
	const FName FirstRegistered(TEXT("Inventory"));
	const FName TabId(TEXT("inventory"));
	TestTrue("FName compares case-insensitively", FirstRegistered == TabId);
	TestEqualSensitive("cooked-build spelling -> canonical id", MordecaiAgentBridge::CanonicalNameId(FirstRegistered), FString(TEXT("inventory")));
	TestEqualSensitive("already lowercase", MordecaiAgentBridge::CanonicalNameId(TabId), FString(TEXT("inventory")));
	TestEqualSensitive("Character", MordecaiAgentBridge::CanonicalNameId(FName(TEXT("Character"))), FString(TEXT("character")));
	TestEqualSensitive("None", MordecaiAgentBridge::CanonicalNameId(NAME_None), FString(TEXT("none")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Ids_ButtonIdsMatchInstanceIds,
	"Mordecai.Playtest.AgentBridge.Ids.ButtonIdsMatchInstanceIds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Ids_ButtonIdsMatchInstanceIds::RunTest(const FString& Parameters)
{
	const FGuid Guid = FGuid::NewGuid();
	const FString InstanceId = Guid.ToString(); // inventory.items[].instance_id
	FString Canonical;
	TestTrue("an uppercase guid parses", MordecaiAgentBridge::TryCanonicalGuid(InstanceId, Canonical));
	TestEqualSensitive("uppercase guid unchanged", Canonical, InstanceId);
	TestTrue("a lowercase guid parses", MordecaiAgentBridge::TryCanonicalGuid(InstanceId.ToLower(), Canonical));
	TestEqualSensitive("lowercase guid -> instance-id form", Canonical, InstanceId);
	TestFalse("a word is not a guid", MordecaiAgentBridge::TryCanonicalGuid(TEXT("Inventory"), Canonical));
	TestFalse("empty is not a guid", MordecaiAgentBridge::TryCanonicalGuid(FString(), Canonical));

	TestEqualSensitive("identify button id (any case) == instance_id", MordecaiAgentBridge::CanonicalButtonId(InstanceId.ToLower()), InstanceId);
	TestEqualSensitive("tab button id lowercased", MordecaiAgentBridge::CanonicalButtonId(TEXT("Inventory")), FString(TEXT("inventory")));
	TestEqualSensitive("filter button id lowercased", MordecaiAgentBridge::CanonicalButtonId(TEXT("All")), FString(TEXT("all")));
	return true;
}

// ===========================================================================
// Axis threshold buttons (§4 axis, XInputInterface.cpp)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Axis_ThresholdButtonsMirrorXInput,
	"Mordecai.Playtest.AgentBridge.Axis.ThresholdButtonsMirrorXInput",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Axis_ThresholdButtonsMirrorXInput::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridge;
	auto Down = [](const FKey& Axis, float Value) { return GetAxisThresholdKeysDown(Axis, Value); };
	auto Only = [](const FKey& Key) { return TArray<FKey>{ Key }; };

	FAxisThresholdButtons Buttons;
	TestTrue("LeftY has threshold buttons", GetAxisThresholdButtons(EKeys::Gamepad_LeftY, Buttons));
	TestTrue("LeftY: up / down", Buttons.Positive == EKeys::Gamepad_LeftStick_Up && Buttons.Negative == EKeys::Gamepad_LeftStick_Down);
	TestEqual("left stick dead zone", Buttons.Threshold, 7849);
	TestTrue("RightX: right / left", GetAxisThresholdButtons(EKeys::Gamepad_RightX, Buttons)
		&& Buttons.Positive == EKeys::Gamepad_RightStick_Right && Buttons.Negative == EKeys::Gamepad_RightStick_Left && Buttons.Threshold == 8689);
	TestTrue("left trigger: one button", GetAxisThresholdButtons(EKeys::Gamepad_LeftTriggerAxis, Buttons)
		&& Buttons.bTrigger && Buttons.Positive == EKeys::Gamepad_LeftTrigger && !Buttons.Negative.IsValid() && Buttons.Threshold == 30);
	TestFalse("W is not an axis", GetAxisThresholdButtons(EKeys::W, Buttons));

	// Raw values: the inverse of ShortToNormalizedFloat (32767 up, 32768 down) and 0..255 for triggers
	TestEqual("stick +1", AxisValueToXInputRaw(EKeys::Gamepad_LeftY, 1.f), 32767);
	TestEqual("stick -1", AxisValueToXInputRaw(EKeys::Gamepad_LeftY, -1.f), -32768);
	TestEqual("trigger 1", AxisValueToXInputRaw(EKeys::Gamepad_RightTriggerAxis, 1.f), 255);

	// Strictly past the threshold, like XInputInterface.cpp (sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	TestEqual("LeftY 0.239 (raw 7831) is inside the dead zone", Down(EKeys::Gamepad_LeftY, 0.239f).Num(), 0);
	TestEqual("LeftY 0.24 (raw 7864) -> LeftStick_Up", Down(EKeys::Gamepad_LeftY, 0.24f), Only(EKeys::Gamepad_LeftStick_Up));
	TestEqual("LeftY -0.24 -> LeftStick_Down", Down(EKeys::Gamepad_LeftY, -0.24f), Only(EKeys::Gamepad_LeftStick_Down));
	TestEqual("LeftX 1 -> LeftStick_Right", Down(EKeys::Gamepad_LeftX, 1.f), Only(EKeys::Gamepad_LeftStick_Right));
	TestEqual("LeftX -1 -> LeftStick_Left", Down(EKeys::Gamepad_LeftX, -1.f), Only(EKeys::Gamepad_LeftStick_Left));
	TestEqual("RightY 0.265 (raw 8683) is inside the right dead zone", Down(EKeys::Gamepad_RightY, 0.265f).Num(), 0);
	TestEqual("RightY 0.266 -> RightStick_Up", Down(EKeys::Gamepad_RightY, 0.266f), Only(EKeys::Gamepad_RightStick_Up));
	TestEqual("RightX -0.5 -> RightStick_Left", Down(EKeys::Gamepad_RightX, -0.5f), Only(EKeys::Gamepad_RightStick_Left));
	TestEqual("left trigger 0.117 (raw 30) is not past 30", Down(EKeys::Gamepad_LeftTriggerAxis, 0.117f).Num(), 0);
	TestEqual("left trigger 0.12 (raw 31) -> Gamepad_LeftTrigger", Down(EKeys::Gamepad_LeftTriggerAxis, 0.12f), Only(EKeys::Gamepad_LeftTrigger));
	TestEqual("right trigger 1 -> Gamepad_RightTrigger", Down(EKeys::Gamepad_RightTriggerAxis, 1.f), Only(EKeys::Gamepad_RightTrigger));
	TestEqual("0 -> nothing", Down(EKeys::Gamepad_RightTriggerAxis, 0.f).Num(), 0);
	TestEqual("not an axis -> nothing", Down(EKeys::W, 1.f).Num(), 0);
	return true;
}

// ===========================================================================
// Auto-repeat (§4 key: real keyboard typematic, XInput button repeat)
// ===========================================================================

namespace MordecaiAgentBridgeRepeatTestHelpers
{
	/** Repeats per key while stepping frames of Step seconds from From (exclusive) to To (inclusive). */
	static TMap<FKey, int32> Run(FMordecaiAgentBridgeAutoRepeat& Repeat, double From, double To, double Step = 1.0 / 60.0)
	{
		TMap<FKey, int32> Counts;
		for (double Now = From + Step; Now <= To + 1e-9; Now += Step)
		{
			for (const FKey& Key : Repeat.CollectRepeats(Now))
			{
				++Counts.FindOrAdd(Key);
			}
		}
		return Counts;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Repeat_KeyboardRepeatsOnlyTheLastPressedKey,
	"Mordecai.Playtest.AgentBridge.Repeat.KeyboardRepeatsOnlyTheLastPressedKey",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Repeat_KeyboardRepeatsOnlyTheLastPressedKey::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeRepeatTestHelpers;
	FMordecaiAgentBridgeAutoRepeat Repeat(0.5, 0.1, 0.2, 0.1);

	Repeat.KeyDown(EKeys::W, 0.0);
	TestEqual("no repeat before the typematic delay", Repeat.CollectRepeats(0.49).Num(), 0);
	TestEqual("first repeat at the delay", Repeat.CollectRepeats(0.5), TArray<FKey>{ EKeys::W });
	TestEqual("not again in the same interval", Repeat.CollectRepeats(0.55).Num(), 0);
	TestEqual("then every interval", Repeat.CollectRepeats(0.61), TArray<FKey>{ EKeys::W });

	// D goes down while W is held: the repeat moves to D, W stops for good
	Repeat.KeyDown(EKeys::D, 0.65);
	TestTrue("D is the typematic key", Repeat.GetTypematicKey() == EKeys::D);
	TMap<FKey, int32> Counts = Run(Repeat, 0.65, 1.149);
	TestFalse("W does not repeat after D went down", Counts.Contains(EKeys::W));
	TestFalse("D waits for its own delay", Counts.Contains(EKeys::D));
	Counts = Run(Repeat, 1.149, 1.5);
	TestTrue("D repeats after its delay", Counts.FindRef(EKeys::D) >= 3);
	TestFalse("W still silent", Counts.Contains(EKeys::W));

	// D released while W is still held: W does not resume
	Repeat.KeyUp(EKeys::D);
	TestFalse("no typematic key after the repeating key went up", Repeat.GetTypematicKey().IsValid());
	TestEqual("W does not resume", Run(Repeat, 1.5, 4.0).Num(), 0);

	// Releasing a key that is not repeating leaves the repeat alone
	Repeat.KeyDown(EKeys::A, 4.0);
	Repeat.KeyUp(EKeys::W);
	TestTrue("A keeps repeating after W went up", Run(Repeat, 4.0, 4.6).FindRef(EKeys::A) >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Repeat_GamepadButtonsRepeatIndependently,
	"Mordecai.Playtest.AgentBridge.Repeat.GamepadButtonsRepeatIndependently",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Repeat_GamepadButtonsRepeatIndependently::RunTest(const FString& Parameters)
{
	using namespace MordecaiAgentBridgeRepeatTestHelpers;
	FMordecaiAgentBridgeAutoRepeat Repeat; // project defaults: keyboard 0.5 / 1/30, gamepad 0.2 / 0.1

	Repeat.KeyDown(EKeys::Gamepad_DPad_Down, 0.0);
	TestEqual("no repeat before 0.2 s", Repeat.CollectRepeats(0.199).Num(), 0);
	TestEqual("first repeat at 0.2 s", Repeat.CollectRepeats(0.2), TArray<FKey>{ EKeys::Gamepad_DPad_Down });

	// A second button and a keyboard key do not interrupt the first button's repeat
	Repeat.KeyDown(EKeys::Gamepad_FaceButton_Bottom, 0.25);
	Repeat.KeyDown(EKeys::W, 0.25);
	const TMap<FKey, int32> Counts = Run(Repeat, 0.2, 1.0, 1.0 / 120.0);
	// Like XInput, the next repeat is scheduled from the frame that sent the last one, so a repeat can
	// slip by up to a frame: DPad at ~0.3 .. 1.0 (7-8 more, about 9 in the first second as on a real pad)
	const int32 DPad = Counts.FindRef(EKeys::Gamepad_DPad_Down);
	TestTrue(FString::Printf(TEXT("DPad repeats every 0.1 s after the first (%d)"), DPad), DPad >= 7 && DPad <= 8);
	// Face button: ~0.45, then every 0.1 s to 0.95 (5-6)
	const int32 Face = Counts.FindRef(EKeys::Gamepad_FaceButton_Bottom);
	TestTrue(FString::Printf(TEXT("face button on its own schedule (%d)"), Face), Face >= 5 && Face <= 6);
	// W: keyboard typematic from 0.75 s, every 1/30 s (6-8 by 1.0 s)
	const int32 W = Counts.FindRef(EKeys::W);
	TestTrue(FString::Printf(TEXT("W repeats from 0.75 s at the keyboard rate (%d)"), W), W >= 6 && W <= 8);

	Repeat.KeyUp(EKeys::Gamepad_DPad_Down);
	const TMap<FKey, int32> After = Run(Repeat, 1.0, 1.5, 1.0 / 120.0);
	TestFalse("a released button stops", After.Contains(EKeys::Gamepad_DPad_Down));
	TestTrue("the other button keeps repeating", After.FindRef(EKeys::Gamepad_FaceButton_Bottom) >= 4);

	// Timing from Input.ini
	FMordecaiAgentBridgeAutoRepeat Custom;
	Custom.SetGamepadTiming(0.3, 0.05);
	Custom.KeyDown(EKeys::Gamepad_FaceButton_Right, 0.0);
	TestEqual("custom initial delay", Custom.CollectRepeats(0.29).Num(), 0);
	TestEqual("custom first repeat", Custom.CollectRepeats(0.301).Num(), 1);
	TestEqual("not before the custom repeat delay", Custom.CollectRepeats(0.34).Num(), 0);
	TestEqual("custom repeat delay", Custom.CollectRepeats(0.36).Num(), 1);

	Custom.Reset();
	TestEqual("reset forgets every key", Custom.CollectRepeats(10.0).Num(), 0);
	return true;
}

// ===========================================================================
// Input union (§2 Connections: Slate sees one keyboard, pad and mouse)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Union_ReferenceCountsKeys,
	"Mordecai.Playtest.AgentBridge.Union.ReferenceCountsKeys",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Union_ReferenceCountsKeys::RunTest(const FString& Parameters)
{
	using MordecaiAgentBridge::ERoute;
	FMordecaiAgentBridgeInputUnion Union;
	bool bWasHeld = false;

	TestTrue("A presses W: 0 -> 1 (down event)", Union.AddKey(1, EKeys::W, ERoute::Slate));
	TestTrue("A holds W", Union.HoldsKey(1, EKeys::W));
	TestFalse("B presses W: no second down", Union.AddKey(2, EKeys::W, ERoute::Slate));
	TestFalse("A pressing W again is not a transition", Union.AddKey(1, EKeys::W, ERoute::Slate));
	TestEqual("two holders", Union.CountKeyHolders(EKeys::W, false), 2);
	TestEqual("W listed once", Union.GetDownKeys(), TArray<FKey>{ EKeys::W });

	TestFalse("B releases W: A still holds it (no up)", Union.RemoveKey(2, EKeys::W, bWasHeld));
	TestTrue("B did hold it", bWasHeld);
	TestTrue("still down", Union.IsKeyDown(EKeys::W));
	TestFalse("B releasing again: not held by B", Union.RemoveKey(2, EKeys::W, bWasHeld));
	TestFalse("was_held false for B", bWasHeld);
	TestTrue("A releases W: 1 -> 0 (up event)", Union.RemoveKey(1, EKeys::W, bWasHeld) && bWasHeld);
	TestFalse("W is up", Union.IsKeyDown(EKeys::W));
	TestEqual("no holders", Union.CountKeyHolders(EKeys::W, true), 0);

	// Synthesized holds (threshold buttons) count for the transition but not as a connection holder
	TestTrue("synthesized LeftTrigger down", Union.AddKey(FMordecaiAgentBridgeInputUnion::SynthesizedHolder, EKeys::Gamepad_LeftTrigger, ERoute::Slate));
	TestFalse("a connection pressing it adds no down", Union.AddKey(3, EKeys::Gamepad_LeftTrigger, ERoute::Slate));
	TestEqual("connection holders exclude the synthesized hold", Union.CountKeyHolders(EKeys::Gamepad_LeftTrigger, false), 1);
	TestEqual("all holders", Union.CountKeyHolders(EKeys::Gamepad_LeftTrigger, true), 2);
	TestFalse("the connection releasing leaves the synthesized hold", Union.RemoveKey(3, EKeys::Gamepad_LeftTrigger, bWasHeld));
	TestTrue("synthesized release goes up", Union.RemoveKey(FMordecaiAgentBridgeInputUnion::SynthesizedHolder, EKeys::Gamepad_LeftTrigger, bWasHeld));

	// Per-holder key lists keep press order; the first holder's route is the key's route
	Union.AddKey(1, EKeys::D, ERoute::Viewport);
	Union.AddKey(1, EKeys::A, ERoute::Slate);
	Union.AddKey(2, EKeys::D, ERoute::Slate);
	TestEqual("A's keys in press order", Union.GetKeysHeldBy(1), TArray<FKey>({ EKeys::D, EKeys::A }));
	TestEqual("B's keys", Union.GetKeysHeldBy(2), TArray<FKey>{ EKeys::D });
	TestTrue("route of the first holder", Union.GetKeyRoute(EKeys::D) == ERoute::Viewport);
	Union.Reset();
	TestEqual("reset", Union.GetDownKeys().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Union_AxesUseTheLargestMagnitude,
	"Mordecai.Playtest.AgentBridge.Union.AxesUseTheLargestMagnitude",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Union_AxesUseTheLargestMagnitude::RunTest(const FString& Parameters)
{
	using MordecaiAgentBridge::ERoute;
	FMordecaiAgentBridgeInputUnion Union;

	TestEqual("nobody holds it", Union.GetAxisValue(EKeys::Gamepad_LeftY), 0.f);
	Union.SetAxis(1, EKeys::Gamepad_LeftY, 0.4f, ERoute::Slate);
	TestEqual("one holder", Union.GetAxisValue(EKeys::Gamepad_LeftY), 0.4f);
	Union.SetAxis(2, EKeys::Gamepad_LeftY, -0.9f, ERoute::Slate);
	TestEqual("largest magnitude wins", Union.GetAxisValue(EKeys::Gamepad_LeftY), -0.9f);
	TestEqual("two holders", Union.CountAxisHolders(EKeys::Gamepad_LeftY), 2);
	Union.SetAxis(1, EKeys::Gamepad_LeftY, 0.9f, ERoute::Slate);
	TestEqual("tie: the earliest holder wins", Union.GetAxisValue(EKeys::Gamepad_LeftY), 0.9f);
	Union.SetAxis(2, EKeys::Gamepad_RightTriggerAxis, 1.f, ERoute::Slate);
	TestEqual("held axes in first-hold order", Union.GetHeldAxes(), TArray<FKey>({ EKeys::Gamepad_LeftY, EKeys::Gamepad_RightTriggerAxis }));
	TestEqual("B's axes", Union.GetAxesHeldBy(2), TArray<FKey>({ EKeys::Gamepad_LeftY, EKeys::Gamepad_RightTriggerAxis }));

	Union.SetAxis(2, EKeys::Gamepad_LeftY, 0.f, ERoute::Slate);
	TestEqual("0 removes only B's hold", Union.GetAxisValue(EKeys::Gamepad_LeftY), 0.9f);
	TestEqual("one holder left", Union.CountAxisHolders(EKeys::Gamepad_LeftY), 1);
	Union.SetAxis(1, EKeys::Gamepad_LeftY, 0.f, ERoute::Slate);
	TestEqual("released", Union.GetAxisValue(EKeys::Gamepad_LeftY), 0.f);
	TestEqual("only the trigger is still held", Union.GetHeldAxes(), TArray<FKey>{ EKeys::Gamepad_RightTriggerAxis });
	return true;
}

// ===========================================================================
// Focus restore (§1 environment emulation)
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Focus_RestoreOnlyWhenNothingHasFocus,
	"Mordecai.Playtest.AgentBridge.Focus.RestoreOnlyWhenNothingHasFocus",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Focus_RestoreOnlyWhenNothingHasFocus::RunTest(const FString& Parameters)
{
	using MordecaiAgentBridge::ERoute;
	using MordecaiAgentBridge::ShouldRestoreUserFocus;
	TestTrue("empty focus path in game mode (locked-desktop launch)", ShouldRestoreUserFocus(ERoute::Slate, false, false));
	TestFalse("the focused leaf died but the path is still live", ShouldRestoreUserFocus(ERoute::Slate, true, false));
	TestFalse("never while a CommonUI menu is up", ShouldRestoreUserFocus(ERoute::Slate, false, true));
	TestFalse("never for the viewport route", ShouldRestoreUserFocus(ERoute::Viewport, false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Experience_NormalizesNames,
	"Mordecai.Playtest.AgentBridge.Experience.NormalizesNames",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Experience_NormalizesNames::RunTest(const FString& Parameters)
{
	const FString Expected = TEXT("B_MordecaiArenaExperience");
	TestEqual("Primary asset id", MordecaiAgentBridge::NormalizeExperienceName(TEXT("LyraExperienceDefinition:B_MordecaiArenaExperience")), Expected);
	TestEqual("Class path", MordecaiAgentBridge::NormalizeExperienceName(TEXT("/MordecaiCore/System/Experiences/B_MordecaiArenaExperience.B_MordecaiArenaExperience_C")), Expected);
	TestEqual("CDO name", MordecaiAgentBridge::NormalizeExperienceName(TEXT("Default__B_MordecaiArenaExperience_C")), Expected);
	TestEqual("Class name", MordecaiAgentBridge::NormalizeExperienceName(TEXT("B_MordecaiArenaExperience_C")), Expected);
	TestEqual("Plain name", MordecaiAgentBridge::NormalizeExperienceName(Expected), Expected);
	TestEqual("Empty", MordecaiAgentBridge::NormalizeExperienceName(FString()), FString());
	return true;
}
