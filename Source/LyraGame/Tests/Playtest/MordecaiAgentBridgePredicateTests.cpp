// Project Mordecai — Agent bridge wait_until predicate tests (Specs/agent_bridge_v1.md §4 wait_until)
// Headless tests for the condition grammar and its evaluation over state JSON.

#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "Mordecai/Playtest/MordecaiAgentBridgePredicate.h"

namespace MordecaiAgentBridgePredicateTestHelpers
{
	/** A state object like the bridge produces for wait_until. */
	static TSharedPtr<FJsonObject> MakeState()
	{
		const FString Json = TEXT(R"JSON({
			"inventory": {"valid": true, "count": 4, "items": [
				{"item_id": "TestItem_UpgradeKey", "display_name": "Upgrade Key", "identified": true},
				{"item_id": "TestItem_MysteriousAmulet", "display_name": "Unidentified Purple Amulet", "identified": false}
			], "ledger_count": 2, "ledger": []},
			"pause_menu": {"valid": true, "open": true, "active_tab": "inventory",
				"inventory_tab": {"visible": true, "rows": [{"text": "a"}, {"text": "b"}, {"text": "c"}, {"text": "d"}]}},
			"player": {"valid": true, "location": [400.5, -2, 92], "is_dead": false, "speed": 0, "name": "", "tags": [], "weapon": null,
				"attributes": {"health": 100, "stamina": -12.5}},
			"pickups": {"valid": true, "focused": null, "in_range_count": 0}
		})JSON");
		TSharedPtr<FJsonObject> Object;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object);
		return Object;
	}

	/** Parse + evaluate; a parse failure counts as false and is reported through bParsed. */
	static bool Eval(const FString& Condition, bool& bParsed, TSharedPtr<FJsonObject>* OutValues = nullptr)
	{
		FString Error;
		const TSharedPtr<FMordecaiAgentBridgePredicate> Predicate = FMordecaiAgentBridgePredicate::Parse(Condition, Error);
		bParsed = Predicate.IsValid();
		return bParsed && Predicate->Evaluate(MakeState(), OutValues);
	}
}

#define MORDECAI_BRIDGE_EXPECT(Condition, bExpected) \
	{ bool bParsed = false; const bool bResult = MordecaiAgentBridgePredicateTestHelpers::Eval(TEXT(Condition), bParsed); \
	  TestTrue(TEXT("Parses: " Condition), bParsed); TestEqual(TEXT("Value of: " Condition), bResult, bExpected); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_Comparisons,
	"Mordecai.Playtest.AgentBridge.Predicate.Comparisons",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_Comparisons::RunTest(const FString& Parameters)
{
	MORDECAI_BRIDGE_EXPECT("inventory.count>=4", true);
	MORDECAI_BRIDGE_EXPECT("inventory.count >= 5", false);
	MORDECAI_BRIDGE_EXPECT("inventory.count==4", true);
	MORDECAI_BRIDGE_EXPECT("inventory.count == 4.0", true);
	MORDECAI_BRIDGE_EXPECT("inventory.count!=4", false);
	MORDECAI_BRIDGE_EXPECT("inventory.count<5", true);
	MORDECAI_BRIDGE_EXPECT("inventory.count<=3", false);
	MORDECAI_BRIDGE_EXPECT("inventory.count>3.5", true);
	MORDECAI_BRIDGE_EXPECT("player.attributes.stamina < -10", true);
	MORDECAI_BRIDGE_EXPECT("player.attributes.stamina == -12.5", true);
	MORDECAI_BRIDGE_EXPECT("4 == inventory.count", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.active_tab==\"inventory\"", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.active_tab=='inventory'", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.active_tab==\"Inventory\"", false);
	MORDECAI_BRIDGE_EXPECT("pause_menu.active_tab != 'character'", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.open==true", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.open==false", false);
	MORDECAI_BRIDGE_EXPECT("player.is_dead==false", true);
	MORDECAI_BRIDGE_EXPECT("pickups.focused==null", true);
	MORDECAI_BRIDGE_EXPECT("player.weapon == null", true);
	MORDECAI_BRIDGE_EXPECT("'abc' < 'abd'", true);
	MORDECAI_BRIDGE_EXPECT("\"b\" >= \"a\"", true);
	MORDECAI_BRIDGE_EXPECT("1e2 == 100", true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_LogicalOperators,
	"Mordecai.Playtest.AgentBridge.Predicate.LogicalOperators",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_LogicalOperators::RunTest(const FString& Parameters)
{
	MORDECAI_BRIDGE_EXPECT("pause_menu.open==true && pause_menu.active_tab==\"inventory\" && len(pause_menu.inventory_tab.rows)==4", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.open && inventory.count==3", false);
	MORDECAI_BRIDGE_EXPECT("inventory.count==3 || inventory.count==4", true);
	MORDECAI_BRIDGE_EXPECT("inventory.count==3 || inventory.count==5", false);
	MORDECAI_BRIDGE_EXPECT("!pause_menu.open", false);
	MORDECAI_BRIDGE_EXPECT("!!pause_menu.open", true);
	MORDECAI_BRIDGE_EXPECT("!(inventory.count==3)", true);
	MORDECAI_BRIDGE_EXPECT("! (inventory.count==4)", false);
	// && binds tighter than ||
	MORDECAI_BRIDGE_EXPECT("true || false && false", true);
	MORDECAI_BRIDGE_EXPECT("(true || false) && false", false);
	MORDECAI_BRIDGE_EXPECT("((inventory.count==4))", true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_LenIndexesAndTruthiness,
	"Mordecai.Playtest.AgentBridge.Predicate.LenIndexesAndTruthiness",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_LenIndexesAndTruthiness::RunTest(const FString& Parameters)
{
	MORDECAI_BRIDGE_EXPECT("len(pause_menu.inventory_tab.rows)==4", true);
	MORDECAI_BRIDGE_EXPECT("len(inventory.items) == 2", true);
	MORDECAI_BRIDGE_EXPECT("len(pause_menu.active_tab)==9", true);     // string length
	MORDECAI_BRIDGE_EXPECT("len(player.attributes)==2", true);         // object field count
	MORDECAI_BRIDGE_EXPECT("len(pickups.focused)==0", true);           // null
	MORDECAI_BRIDGE_EXPECT("len(no.such.path)==0", true);
	MORDECAI_BRIDGE_EXPECT("inventory.items[1].identified==false", true);
	MORDECAI_BRIDGE_EXPECT("inventory.items[0].display_name=='Upgrade Key'", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.inventory_tab.rows[3].text=='d'", true);
	MORDECAI_BRIDGE_EXPECT("player.location[0] > 400", true);
	MORDECAI_BRIDGE_EXPECT("player.location[2]==92", true);

	// Bare operands: falsy are null, false, 0, "", [] and {}
	MORDECAI_BRIDGE_EXPECT("pause_menu.open", true);
	MORDECAI_BRIDGE_EXPECT("inventory.count", true);
	MORDECAI_BRIDGE_EXPECT("inventory.items", true);
	MORDECAI_BRIDGE_EXPECT("inventory", true);
	MORDECAI_BRIDGE_EXPECT("player.is_dead", false);
	MORDECAI_BRIDGE_EXPECT("player.speed", false);
	MORDECAI_BRIDGE_EXPECT("player.name", false);
	MORDECAI_BRIDGE_EXPECT("player.tags", false);
	MORDECAI_BRIDGE_EXPECT("pickups.focused", false);
	MORDECAI_BRIDGE_EXPECT("inventory.ledger", false);
	MORDECAI_BRIDGE_EXPECT("'x'", true);
	MORDECAI_BRIDGE_EXPECT("0", false);
	MORDECAI_BRIDGE_EXPECT("null", false);

	TestTrue("Empty object is falsy", !FMordecaiAgentBridgePredicate::IsTruthy(MakeShared<FJsonValueObject>(MakeShared<FJsonObject>())));
	TestTrue("Null pointer is falsy", !FMordecaiAgentBridgePredicate::IsTruthy(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_MissingPathsAndTypeMismatch,
	"Mordecai.Playtest.AgentBridge.Predicate.MissingPathsAndTypeMismatch",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_MissingPathsAndTypeMismatch::RunTest(const FString& Parameters)
{
	// Missing paths are null
	MORDECAI_BRIDGE_EXPECT("inventory.nothing==null", true);
	MORDECAI_BRIDGE_EXPECT("no_section.count==null", true);
	MORDECAI_BRIDGE_EXPECT("inventory.items[5].identified==null", true);
	MORDECAI_BRIDGE_EXPECT("inventory.count.deeper==null", true);    // field of a number
	MORDECAI_BRIDGE_EXPECT("inventory[0]==null", true);              // index into an object
	MORDECAI_BRIDGE_EXPECT("inventory.items.first==null", true);     // field of an array
	MORDECAI_BRIDGE_EXPECT("inventory.nothing", false);
	MORDECAI_BRIDGE_EXPECT("inventory.nothing>=0", false);

	// Mismatched types are unequal; ordering needs two numbers or two strings
	MORDECAI_BRIDGE_EXPECT("inventory.count=='4'", false);
	MORDECAI_BRIDGE_EXPECT("inventory.count!='4'", true);
	MORDECAI_BRIDGE_EXPECT("pause_menu.open==1", false);
	MORDECAI_BRIDGE_EXPECT("pause_menu.open!=1", true);
	MORDECAI_BRIDGE_EXPECT("player.weapon==false", false);
	MORDECAI_BRIDGE_EXPECT("player.weapon==0", false);
	MORDECAI_BRIDGE_EXPECT("inventory.count>'3'", false);
	MORDECAI_BRIDGE_EXPECT("inventory.count<'5'", false);
	MORDECAI_BRIDGE_EXPECT("pause_menu.open>false", false);
	MORDECAI_BRIDGE_EXPECT("null<1", false);
	MORDECAI_BRIDGE_EXPECT("null>=null", false);
	MORDECAI_BRIDGE_EXPECT("null==null", true);

	TestTrue("Numbers equal numerically", FMordecaiAgentBridgePredicate::ValuesEqual(MakeShared<FJsonValueNumber>(4.0), MakeShared<FJsonValueNumberString>(TEXT("4"))));
	TestFalse("String vs number", FMordecaiAgentBridgePredicate::ValuesEqual(MakeShared<FJsonValueString>(TEXT("4")), MakeShared<FJsonValueNumber>(4.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_ParseErrors,
	"Mordecai.Playtest.AgentBridge.Predicate.ParseErrors",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_ParseErrors::RunTest(const FString& Parameters)
{
	const TCHAR* Bad[] = {
		TEXT(""), TEXT("   "), TEXT("inventory.count>="), TEXT(">= 4"), TEXT("inventory.count = 4"), TEXT("a & b"), TEXT("a | b"),
		TEXT("(inventory.count==4"), TEXT("inventory.count==4)"), TEXT("len(inventory.items"), TEXT("len()"), TEXT("len(4)"),
		TEXT("len('a')"), TEXT("inventory..count"), TEXT("inventory.count[x]"), TEXT("inventory.items[-1]"), TEXT("inventory.items[1"),
		TEXT("'unterminated"), TEXT("\"unterminated"), TEXT("a == b c"), TEXT("1abc"), TEXT("a == 1 == 2"), TEXT("@"), TEXT("a.1"),
		TEXT("!"), TEXT("a &&"), TEXT("|| a"),
	};
	for (const TCHAR* Condition : Bad)
	{
		FString Error;
		const TSharedPtr<FMordecaiAgentBridgePredicate> Predicate = FMordecaiAgentBridgePredicate::Parse(Condition, Error);
		TestFalse(FString::Printf(TEXT("Rejects '%s'"), Condition), Predicate.IsValid());
		TestFalse(FString::Printf(TEXT("Error message for '%s'"), Condition), Error.IsEmpty());
	}

	// Reserved words are literals, not paths; 'len' without '(' is a plain identifier
	MORDECAI_BRIDGE_EXPECT("true", true);
	MORDECAI_BRIDGE_EXPECT("false", false);
	MORDECAI_BRIDGE_EXPECT("len==null", true);
	MORDECAI_BRIDGE_EXPECT("'it\\'s' == \"it's\"", true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_RootSectionsAndValues,
	"Mordecai.Playtest.AgentBridge.Predicate.RootSectionsAndValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_RootSectionsAndValues::RunTest(const FString& Parameters)
{
	FString Error;
	const TSharedPtr<FMordecaiAgentBridgePredicate> Predicate = FMordecaiAgentBridgePredicate::Parse(
		TEXT("pause_menu.open==true && (inventory.count>=4 || len(pause_menu.inventory_tab.rows)==4) && !player.is_dead && 'x'=='x'"), Error);
	if (!TestTrue("Parses", Predicate.IsValid()))
	{
		return true;
	}

	TArray<FString> Roots = Predicate->GetRootIdentifiers();
	Roots.Sort();
	TestEqual("Unique root sections", Roots, TArray<FString>({ TEXT("inventory"), TEXT("pause_menu"), TEXT("player") }));

	TSharedPtr<FJsonObject> Values;
	TestTrue("Evaluates true", Predicate->Evaluate(MordecaiAgentBridgePredicateTestHelpers::MakeState(), &Values));
	if (TestTrue("Values reported", Values.IsValid()))
	{
		TestTrue("pause_menu.open", Values->HasTypedField<EJson::Boolean>(TEXT("pause_menu.open")) && Values->GetBoolField(TEXT("pause_menu.open")));
		TestEqual("inventory.count", Values->GetNumberField(TEXT("inventory.count")), 4.0);
		TestEqual("len(...)", Values->GetNumberField(TEXT("len(pause_menu.inventory_tab.rows)")), 4.0);
		TestTrue("player.is_dead", Values->HasTypedField<EJson::Boolean>(TEXT("player.is_dead")));
		TestEqual("Only operand paths, no literals", Values->Values.Num(), 4);
	}

	// Values of a false condition, including a missing path as null
	const TSharedPtr<FMordecaiAgentBridgePredicate> Missing = FMordecaiAgentBridgePredicate::Parse(TEXT("inventory.count>=10 || game.ready"), Error);
	if (TestTrue("Second condition parses", Missing.IsValid()))
	{
		TSharedPtr<FJsonObject> MissingValues;
		TestFalse("Evaluates false", Missing->Evaluate(MordecaiAgentBridgePredicateTestHelpers::MakeState(), &MissingValues));
		TestTrue("Missing path reported as null", MissingValues.IsValid() && MissingValues->HasTypedField<EJson::Null>(TEXT("game.ready")));
		TestEqual("Roots", Missing->GetRootIdentifiers().Num(), 2);
	}

	// Path resolution helper
	const TSharedPtr<FJsonObject> State = MordecaiAgentBridgePredicateTestHelpers::MakeState();
	TestEqual("ResolvePathString", FMordecaiAgentBridgePredicate::ResolvePathString(State, TEXT("inventory.items[1].item_id"))->AsString(),
		FString(TEXT("TestItem_MysteriousAmulet")));
	TestTrue("ResolvePathString missing", FMordecaiAgentBridgePredicate::ResolvePathString(State, TEXT("inventory.items[9]"))->IsNull());
	TestTrue("Null root", FMordecaiAgentBridgePredicate::ResolvePathString(nullptr, TEXT("inventory"))->IsNull());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_NestingDepthIsLimited,
	"Mordecai.Playtest.AgentBridge.Predicate.NestingDepthIsLimited",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_NestingDepthIsLimited::RunTest(const FString& Parameters)
{
	auto Repeat = [](const TCHAR* Text, int32 Count)
	{
		FString Out;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Out += Text;
		}
		return Out;
	};
	auto Parses = [](const FString& Condition, FString& OutError)
	{
		return FMordecaiAgentBridgePredicate::Parse(Condition, OutError).IsValid();
	};
	FString Error;

	// 64 levels of '!' or '(' are fine (an even number of '!' keeps the value)
	TestTrue("64 '!' parse", Parses(Repeat(TEXT("!"), 64) + TEXT("true"), Error));
	{
		bool bParsed = false;
		TestTrue("64 '!' evaluate", MordecaiAgentBridgePredicateTestHelpers::Eval(Repeat(TEXT("!"), 64) + TEXT("pause_menu.open"), bParsed) && bParsed);
	}
	TestTrue("64 '(' parse", Parses(Repeat(TEXT("("), 64) + TEXT("inventory.count==4") + Repeat(TEXT(")"), 64), Error));

	// One more level is a bad_request, not a stack overflow on the game thread
	TestFalse("65 '!' rejected", Parses(Repeat(TEXT("!"), 65) + TEXT("true"), Error));
	TestTrue("'!' depth message", Error.Contains(TEXT("condition nested too deeply")));
	TestFalse("65 '(' rejected", Parses(Repeat(TEXT("("), 65) + TEXT("true") + Repeat(TEXT(")"), 65), Error));
	TestTrue("'(' depth message", Error.Contains(TEXT("condition nested too deeply")));
	TestFalse("'!(' counts both", Parses(Repeat(TEXT("!("), 33) + TEXT("true") + Repeat(TEXT(")"), 33), Error));
	TestTrue("mixed depth message", Error.Contains(TEXT("condition nested too deeply")));

	// Depth is nesting, not count: long flat chains are fine
	TestTrue("100 sibling groups parse", Parses(TEXT("(true)") + Repeat(TEXT(" && (true)"), 99), Error));
	TestTrue("100 sibling negations parse", Parses(TEXT("!false") + Repeat(TEXT(" && !false"), 99), Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMordecai_Playtest_AgentBridge_Predicate_ConditionLengthIsLimited,
	"Mordecai.Playtest.AgentBridge.Predicate.ConditionLengthIsLimited",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMordecai_Playtest_AgentBridge_Predicate_ConditionLengthIsLimited::RunTest(const FString& Parameters)
{
	FString Error;
	const FString Base = TEXT("inventory.count>=4");

	FString AtLimit = Base;
	AtLimit += FString::ChrN(4096 - Base.Len(), TEXT(' '));
	TestEqual("4096 characters", AtLimit.Len(), 4096);
	TestTrue("a 4096-character condition parses", FMordecaiAgentBridgePredicate::Parse(AtLimit, Error).IsValid());

	const FString OverLimit = AtLimit + TEXT(" ");
	TestFalse("4097 characters are rejected", FMordecaiAgentBridgePredicate::Parse(OverLimit, Error).IsValid());
	TestTrue("length message", Error.Contains(TEXT("condition longer than 4096 characters")));

	// The review's crash input: about 200k '!' (under the 1 MiB line limit) is rejected up front
	const FString Bomb = FString::ChrN(200000, TEXT('!')) + TEXT("game.ready");
	TestFalse("a 200k '!' condition is rejected", FMordecaiAgentBridgePredicate::Parse(Bomb, Error).IsValid());
	TestTrue("rejected for its length", Error.Contains(TEXT("condition longer than 4096 characters")));
	return true;
}

#undef MORDECAI_BRIDGE_EXPECT
