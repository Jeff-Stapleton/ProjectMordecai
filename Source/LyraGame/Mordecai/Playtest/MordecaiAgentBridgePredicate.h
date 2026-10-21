// Project Mordecai — Agent bridge wait_until predicates (Specs/agent_bridge_v1.md §4 wait_until)
//
//   expr       := or
//   or         := and ( "||" and )*
//   and        := unary ( "&&" unary )*
//   unary      := "!" unary | primary
//   primary    := "(" expr ")" | comparison
//   comparison := operand ( ("=="|"!="|"<"|"<="|">"|">=") operand )?
//   operand    := path | "len(" path ")" | literal
//   path       := ident ( "." ident | "[" int "]" )*        ident := [A-Za-z_][A-Za-z0-9_]*
//   literal    := number | "..." | '...' | true | false | null
//
// Evaluated over the bridge's state JSON. Pure: compiles and is testable in every configuration.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

struct FMordecaiAgentBridgePredicateNode;

/** One step of a path: an object field, or an array index (Field empty). */
struct FMordecaiAgentBridgePathSegment
{
	FString Field;
	int32 Index = INDEX_NONE;
};

class FMordecaiAgentBridgePredicate
{
public:
	/** Parse a condition. Returns null and sets OutError on a syntax error. */
	static TSharedPtr<FMordecaiAgentBridgePredicate> Parse(const FString& Expression, FString& OutError);

	/**
	 * Evaluate against a state object ({"inventory":{...}, ...}). When OutValues is given it receives
	 * the value of every referenced operand: "a.b" -> value, "len(a.b)" -> number.
	 */
	bool Evaluate(const TSharedPtr<FJsonObject>& Root, TSharedPtr<FJsonObject>* OutValues = nullptr) const;

	/** Unique first identifiers of every path (the state sections the condition needs). */
	TArray<FString> GetRootIdentifiers() const;

	const FString& GetExpression() const { return Expression; }

	/** Resolve a parsed path; anything that does not resolve is a JSON null. */
	static TSharedRef<FJsonValue> ResolvePath(const TSharedPtr<FJsonObject>& Root, const TArray<FMordecaiAgentBridgePathSegment>& Path);

	/** Parse "a.b[2].c" and resolve it (test helper). Invalid paths resolve to null. */
	static TSharedRef<FJsonValue> ResolvePathString(const TSharedPtr<FJsonObject>& Root, const FString& Path);

	/** Truthiness: false for null, false, 0, "", [] and {}. */
	static bool IsTruthy(const TSharedPtr<FJsonValue>& Value);

	/** ==: numbers numerically, strings exactly, bools/null by value; mismatched types are unequal. */
	static bool ValuesEqual(const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B);

	FMordecaiAgentBridgePredicate();
	~FMordecaiAgentBridgePredicate();

private:
	FString Expression;
	TSharedPtr<FMordecaiAgentBridgePredicateNode> Root;
};
