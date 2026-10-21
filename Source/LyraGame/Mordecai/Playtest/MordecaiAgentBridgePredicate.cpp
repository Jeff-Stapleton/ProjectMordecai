// Project Mordecai — Agent bridge wait_until predicates (Specs/agent_bridge_v1.md §4 wait_until)

#include "Mordecai/Playtest/MordecaiAgentBridgePredicate.h"

#include "Mordecai/Playtest/MordecaiAgentBridgeProtocol.h"

struct FMordecaiAgentBridgePredicateNode
{
	enum class EKind : uint8 { Or, And, Not, Compare, Operand };
	enum class EOp : uint8 { Eq, Ne, Lt, Le, Gt, Ge };
	enum class EOperand : uint8 { Path, Len, Literal };

	EKind Kind = EKind::Operand;
	EOp Op = EOp::Eq;
	EOperand OperandKind = EOperand::Literal;
	TArray<TSharedPtr<FMordecaiAgentBridgePredicateNode>> Children;
	TArray<FMordecaiAgentBridgePathSegment> Path;
	FString PathText;
	TSharedPtr<FJsonValue> Literal;
};

namespace MordecaiAgentBridgePredicatePrivate
{
	using FNode = FMordecaiAgentBridgePredicateNode;

	enum class ETokenType : uint8
	{
		End, LParen, RParen, LBracket, RBracket, Dot, Not, And, Or, Op, Ident, Number, String
	};

	struct FToken
	{
		ETokenType Type = ETokenType::End;
		FString Text;           // ident / string contents / number text / operator
		int32 Position = 0;
	};

	static bool IsIdentStart(TCHAR C) { return FChar::IsAlpha(C) || C == TEXT('_'); }
	static bool IsIdentChar(TCHAR C) { return FChar::IsAlnum(C) || C == TEXT('_'); }

	static bool Tokenize(const FString& Source, TArray<FToken>& OutTokens, FString& OutError)
	{
		const int32 Len = Source.Len();
		int32 I = 0;
		while (I < Len)
		{
			const TCHAR C = Source[I];
			if (FChar::IsWhitespace(C))
			{
				++I;
				continue;
			}

			FToken Token;
			Token.Position = I;
			const TCHAR Next = I + 1 < Len ? Source[I + 1] : TEXT('\0');

			if (C == TEXT('(')) { Token.Type = ETokenType::LParen; ++I; }
			else if (C == TEXT(')')) { Token.Type = ETokenType::RParen; ++I; }
			else if (C == TEXT('[')) { Token.Type = ETokenType::LBracket; ++I; }
			else if (C == TEXT(']')) { Token.Type = ETokenType::RBracket; ++I; }
			else if (C == TEXT('.') && !FChar::IsDigit(Next)) { Token.Type = ETokenType::Dot; ++I; }
			else if (C == TEXT('&'))
			{
				if (Next != TEXT('&'))
				{
					OutError = FString::Printf(TEXT("expected '&&' at position %d"), I);
					return false;
				}
				Token.Type = ETokenType::And;
				I += 2;
			}
			else if (C == TEXT('|'))
			{
				if (Next != TEXT('|'))
				{
					OutError = FString::Printf(TEXT("expected '||' at position %d"), I);
					return false;
				}
				Token.Type = ETokenType::Or;
				I += 2;
			}
			else if (C == TEXT('=') || C == TEXT('!') || C == TEXT('<') || C == TEXT('>'))
			{
				if (Next == TEXT('='))
				{
					Token.Type = ETokenType::Op;
					Token.Text = FString::Printf(TEXT("%c="), C);
					I += 2;
				}
				else if (C == TEXT('!'))
				{
					Token.Type = ETokenType::Not;
					++I;
				}
				else if (C == TEXT('<') || C == TEXT('>'))
				{
					Token.Type = ETokenType::Op;
					Token.Text = FString::Printf(TEXT("%c"), C);
					++I;
				}
				else
				{
					OutError = FString::Printf(TEXT("'=' at position %d: use '=='"), I);
					return false;
				}
			}
			else if (C == TEXT('"') || C == TEXT('\''))
			{
				const TCHAR Quote = C;
				++I;
				FString Value;
				bool bClosed = false;
				while (I < Len)
				{
					const TCHAR S = Source[I];
					if (S == Quote)
					{
						bClosed = true;
						++I;
						break;
					}
					if (S == TEXT('\\') && I + 1 < Len)
					{
						const TCHAR E = Source[I + 1];
						switch (E)
						{
						case TEXT('n'): Value.AppendChar(TEXT('\n')); break;
						case TEXT('t'): Value.AppendChar(TEXT('\t')); break;
						case TEXT('r'): Value.AppendChar(TEXT('\r')); break;
						default: Value.AppendChar(E); break;
						}
						I += 2;
						continue;
					}
					Value.AppendChar(S);
					++I;
				}
				if (!bClosed)
				{
					OutError = FString::Printf(TEXT("unterminated string starting at position %d"), Token.Position);
					return false;
				}
				Token.Type = ETokenType::String;
				Token.Text = MoveTemp(Value);
			}
			else if (FChar::IsDigit(C) || (C == TEXT('-') && (FChar::IsDigit(Next) || Next == TEXT('.'))) || (C == TEXT('.') && FChar::IsDigit(Next)))
			{
				int32 J = I;
				if (Source[J] == TEXT('-'))
				{
					++J;
				}
				while (J < Len && FChar::IsDigit(Source[J])) { ++J; }
				if (J < Len && Source[J] == TEXT('.'))
				{
					++J;
					if (J >= Len || !FChar::IsDigit(Source[J]))
					{
						OutError = FString::Printf(TEXT("malformed number at position %d"), I);
						return false;
					}
					while (J < Len && FChar::IsDigit(Source[J])) { ++J; }
				}
				if (J < Len && (Source[J] == TEXT('e') || Source[J] == TEXT('E')))
				{
					int32 K = J + 1;
					if (K < Len && (Source[K] == TEXT('+') || Source[K] == TEXT('-'))) { ++K; }
					if (K < Len && FChar::IsDigit(Source[K]))
					{
						while (K < Len && FChar::IsDigit(Source[K])) { ++K; }
						J = K;
					}
				}
				Token.Type = ETokenType::Number;
				Token.Text = Source.Mid(I, J - I);
				I = J;
			}
			else if (IsIdentStart(C))
			{
				int32 J = I;
				while (J < Len && IsIdentChar(Source[J])) { ++J; }
				Token.Type = ETokenType::Ident;
				Token.Text = Source.Mid(I, J - I);
				I = J;
			}
			else
			{
				OutError = FString::Printf(TEXT("unexpected character '%c' at position %d"), C, I);
				return false;
			}
			OutTokens.Add(MoveTemp(Token));
		}

		FToken EndToken;
		EndToken.Type = ETokenType::End;
		EndToken.Position = Len;
		OutTokens.Add(EndToken);
		return true;
	}

	static FString PathToText(const TArray<FMordecaiAgentBridgePathSegment>& Path)
	{
		FString Text;
		for (int32 Index = 0; Index < Path.Num(); ++Index)
		{
			if (Path[Index].Field.IsEmpty())
			{
				Text += FString::Printf(TEXT("[%d]"), Path[Index].Index);
			}
			else
			{
				if (Index > 0)
				{
					Text += TEXT(".");
				}
				Text += Path[Index].Field;
			}
		}
		return Text;
	}

	class FParser
	{
	public:
		FParser(const TArray<FToken>& InTokens) : Tokens(InTokens) {}

		TSharedPtr<FNode> ParseAll(FString& OutError)
		{
			TSharedPtr<FNode> Root = ParseOr();
			if (Root.IsValid() && Peek().Type != ETokenType::End)
			{
				Fail(TEXT("unexpected token"));
				Root.Reset();
			}
			if (!Root.IsValid())
			{
				OutError = Error.IsEmpty() ? FString(TEXT("syntax error")) : Error;
			}
			return Root;
		}

		/** Parse a bare path (for ResolvePathString). */
		bool ParsePathOnly(TArray<FMordecaiAgentBridgePathSegment>& OutPath)
		{
			if (Peek().Type != ETokenType::Ident)
			{
				return false;
			}
			return ParsePath(OutPath) && Peek().Type == ETokenType::End;
		}

	private:
		const FToken& Peek(int32 Ahead = 0) const
		{
			return Tokens[FMath::Min(Cursor + Ahead, Tokens.Num() - 1)];
		}

		const FToken& Advance()
		{
			const FToken& Token = Tokens[FMath::Min(Cursor, Tokens.Num() - 1)];
			if (Cursor < Tokens.Num() - 1)
			{
				++Cursor;
			}
			return Token;
		}

		void Fail(const TCHAR* What)
		{
			if (Error.IsEmpty())
			{
				const FToken& Token = Peek();
				const FString Near = Token.Type == ETokenType::End ? FString(TEXT("end of condition")) : FString::Printf(TEXT("'%s'"), *DescribeToken(Token));
				Error = FString::Printf(TEXT("%s at position %d (near %s)"), What, Token.Position, *Near);
			}
		}

		static FString DescribeToken(const FToken& Token)
		{
			switch (Token.Type)
			{
			case ETokenType::LParen: return TEXT("(");
			case ETokenType::RParen: return TEXT(")");
			case ETokenType::LBracket: return TEXT("[");
			case ETokenType::RBracket: return TEXT("]");
			case ETokenType::Dot: return TEXT(".");
			case ETokenType::Not: return TEXT("!");
			case ETokenType::And: return TEXT("&&");
			case ETokenType::Or: return TEXT("||");
			default: return Token.Text;
			}
		}

		TSharedPtr<FNode> ParseOr()
		{
			TSharedPtr<FNode> Left = ParseAnd();
			if (!Left.IsValid())
			{
				return nullptr;
			}
			if (Peek().Type != ETokenType::Or)
			{
				return Left;
			}
			TSharedPtr<FNode> Node = MakeShared<FNode>();
			Node->Kind = FNode::EKind::Or;
			Node->Children.Add(Left);
			while (Peek().Type == ETokenType::Or)
			{
				Advance();
				TSharedPtr<FNode> Right = ParseAnd();
				if (!Right.IsValid())
				{
					return nullptr;
				}
				Node->Children.Add(Right);
			}
			return Node;
		}

		TSharedPtr<FNode> ParseAnd()
		{
			TSharedPtr<FNode> Left = ParseUnary();
			if (!Left.IsValid())
			{
				return nullptr;
			}
			if (Peek().Type != ETokenType::And)
			{
				return Left;
			}
			TSharedPtr<FNode> Node = MakeShared<FNode>();
			Node->Kind = FNode::EKind::And;
			Node->Children.Add(Left);
			while (Peek().Type == ETokenType::And)
			{
				Advance();
				TSharedPtr<FNode> Right = ParseUnary();
				if (!Right.IsValid())
				{
					return nullptr;
				}
				Node->Children.Add(Right);
			}
			return Node;
		}

		/** One more level of '!' or '(' nesting; false (and a parse error) past MaxConditionDepth. */
		bool EnterNesting()
		{
			if (++Depth > MordecaiAgentBridge::MaxConditionDepth)
			{
				// Bounds the parser, EvaluateNode and the node destructor, which all recurse per level
				Error = TEXT("condition nested too deeply");
				return false;
			}
			return true;
		}

		TSharedPtr<FNode> ParseUnary()
		{
			if (Peek().Type == ETokenType::Not)
			{
				Advance();
				if (!EnterNesting())
				{
					return nullptr;
				}
				TSharedPtr<FNode> Operand = ParseUnary();
				--Depth;
				if (!Operand.IsValid())
				{
					return nullptr;
				}
				TSharedPtr<FNode> Node = MakeShared<FNode>();
				Node->Kind = FNode::EKind::Not;
				Node->Children.Add(Operand);
				return Node;
			}
			return ParsePrimary();
		}

		TSharedPtr<FNode> ParsePrimary()
		{
			if (Peek().Type == ETokenType::LParen)
			{
				Advance();
				if (!EnterNesting())
				{
					return nullptr;
				}
				TSharedPtr<FNode> Inner = ParseOr();
				--Depth;
				if (!Inner.IsValid())
				{
					return nullptr;
				}
				if (Peek().Type != ETokenType::RParen)
				{
					Fail(TEXT("expected ')'"));
					return nullptr;
				}
				Advance();
				return Inner;
			}
			return ParseComparison();
		}

		TSharedPtr<FNode> ParseComparison()
		{
			TSharedPtr<FNode> Left = ParseOperand();
			if (!Left.IsValid())
			{
				return nullptr;
			}
			if (Peek().Type != ETokenType::Op)
			{
				return Left;
			}
			const FString OpText = Advance().Text;
			TSharedPtr<FNode> Right = ParseOperand();
			if (!Right.IsValid())
			{
				return nullptr;
			}
			TSharedPtr<FNode> Node = MakeShared<FNode>();
			Node->Kind = FNode::EKind::Compare;
			if (OpText == TEXT("==")) { Node->Op = FNode::EOp::Eq; }
			else if (OpText == TEXT("!=")) { Node->Op = FNode::EOp::Ne; }
			else if (OpText == TEXT("<")) { Node->Op = FNode::EOp::Lt; }
			else if (OpText == TEXT("<=")) { Node->Op = FNode::EOp::Le; }
			else if (OpText == TEXT(">")) { Node->Op = FNode::EOp::Gt; }
			else { Node->Op = FNode::EOp::Ge; }
			Node->Children.Add(Left);
			Node->Children.Add(Right);
			return Node;
		}

		TSharedPtr<FNode> ParseOperand()
		{
			const FToken& Token = Peek();
			TSharedPtr<FNode> Node = MakeShared<FNode>();
			Node->Kind = FNode::EKind::Operand;

			if (Token.Type == ETokenType::Number)
			{
				double Value = 0.0;
				if (!LexTryParseString(Value, *Token.Text) || !FMath::IsFinite(Value))
				{
					Fail(TEXT("malformed number"));
					return nullptr;
				}
				Advance();
				Node->OperandKind = FNode::EOperand::Literal;
				Node->Literal = MakeShared<FJsonValueNumber>(Value);
				return Node;
			}
			if (Token.Type == ETokenType::String)
			{
				Node->OperandKind = FNode::EOperand::Literal;
				Node->Literal = MakeShared<FJsonValueString>(Advance().Text);
				return Node;
			}
			if (Token.Type != ETokenType::Ident)
			{
				Fail(TEXT("expected a path, len(path) or a literal"));
				return nullptr;
			}

			if (Token.Text == TEXT("true") || Token.Text == TEXT("false"))
			{
				Node->OperandKind = FNode::EOperand::Literal;
				Node->Literal = MakeShared<FJsonValueBoolean>(Advance().Text == TEXT("true"));
				return Node;
			}
			if (Token.Text == TEXT("null"))
			{
				Advance();
				Node->OperandKind = FNode::EOperand::Literal;
				Node->Literal = MakeShared<FJsonValueNull>();
				return Node;
			}
			if (Token.Text == TEXT("len") && Peek(1).Type == ETokenType::LParen)
			{
				Advance();
				Advance();
				if (Peek().Type != ETokenType::Ident || IsKeyword(Peek().Text))
				{
					Fail(TEXT("len() needs a path"));
					return nullptr;
				}
				if (!ParsePath(Node->Path))
				{
					return nullptr;
				}
				if (Peek().Type != ETokenType::RParen)
				{
					Fail(TEXT("expected ')' after len(path"));
					return nullptr;
				}
				Advance();
				Node->OperandKind = FNode::EOperand::Len;
				Node->PathText = PathToText(Node->Path);
				return Node;
			}

			if (!ParsePath(Node->Path))
			{
				return nullptr;
			}
			Node->OperandKind = FNode::EOperand::Path;
			Node->PathText = PathToText(Node->Path);
			return Node;
		}

		static bool IsKeyword(const FString& Text)
		{
			return Text == TEXT("true") || Text == TEXT("false") || Text == TEXT("null");
		}

		bool ParsePath(TArray<FMordecaiAgentBridgePathSegment>& OutPath)
		{
			FMordecaiAgentBridgePathSegment First;
			First.Field = Advance().Text;
			OutPath.Add(First);
			while (true)
			{
				if (Peek().Type == ETokenType::Dot)
				{
					Advance();
					if (Peek().Type != ETokenType::Ident)
					{
						Fail(TEXT("expected a field name after '.'"));
						return false;
					}
					FMordecaiAgentBridgePathSegment Segment;
					Segment.Field = Advance().Text;
					OutPath.Add(Segment);
				}
				else if (Peek().Type == ETokenType::LBracket)
				{
					Advance();
					const FToken& IndexToken = Peek();
					int32 Index = INDEX_NONE;
					if (IndexToken.Type != ETokenType::Number || !IndexToken.Text.IsNumeric() || IndexToken.Text.Contains(TEXT("."))
						|| IndexToken.Text.StartsWith(TEXT("-")) || !LexTryParseString(Index, *IndexToken.Text) || Index < 0)
					{
						Fail(TEXT("expected a non-negative integer index"));
						return false;
					}
					Advance();
					if (Peek().Type != ETokenType::RBracket)
					{
						Fail(TEXT("expected ']'"));
						return false;
					}
					Advance();
					FMordecaiAgentBridgePathSegment Segment;
					Segment.Index = Index;
					OutPath.Add(Segment);
				}
				else
				{
					return true;
				}
			}
		}

		const TArray<FToken>& Tokens;
		int32 Cursor = 0;
		int32 Depth = 0;
		FString Error;
	};

	static double LengthOf(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid())
		{
			return 0.0;
		}
		switch (Value->Type)
		{
		case EJson::Array: return Value->AsArray().Num();
		case EJson::String: return Value->AsString().Len();
		case EJson::Object: return Value->AsObject().IsValid() ? Value->AsObject()->Values.Num() : 0;
		default: return 0.0;
		}
	}

	static TSharedPtr<FJsonValue> OperandValue(const FNode& Node, const TSharedPtr<FJsonObject>& Root)
	{
		switch (Node.OperandKind)
		{
		case FNode::EOperand::Path: return FMordecaiAgentBridgePredicate::ResolvePath(Root, Node.Path);
		case FNode::EOperand::Len: return MakeShared<FJsonValueNumber>(LengthOf(FMordecaiAgentBridgePredicate::ResolvePath(Root, Node.Path)));
		default: return Node.Literal;
		}
	}

	static bool Compare(FNode::EOp Op, const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B)
	{
		if (Op == FNode::EOp::Eq)
		{
			return FMordecaiAgentBridgePredicate::ValuesEqual(A, B);
		}
		if (Op == FNode::EOp::Ne)
		{
			return !FMordecaiAgentBridgePredicate::ValuesEqual(A, B);
		}

		const EJson TypeA = A.IsValid() ? A->Type : EJson::Null;
		const EJson TypeB = B.IsValid() ? B->Type : EJson::Null;
		int32 Order = 0;
		if (TypeA == EJson::Number && TypeB == EJson::Number)
		{
			double X = 0.0;
			double Y = 0.0;
			if (!A->TryGetNumber(X) || !B->TryGetNumber(Y))
			{
				return false;
			}
			Order = X < Y ? -1 : (X > Y ? 1 : 0);
		}
		else if (TypeA == EJson::String && TypeB == EJson::String)
		{
			Order = A->AsString().Compare(B->AsString(), ESearchCase::CaseSensitive);
		}
		else
		{
			return false;
		}

		switch (Op)
		{
		case FNode::EOp::Lt: return Order < 0;
		case FNode::EOp::Le: return Order <= 0;
		case FNode::EOp::Gt: return Order > 0;
		default: return Order >= 0;
		}
	}

	static bool EvaluateNode(const FNode& Node, const TSharedPtr<FJsonObject>& Root)
	{
		switch (Node.Kind)
		{
		case FNode::EKind::Or:
			for (const TSharedPtr<FNode>& Child : Node.Children)
			{
				if (EvaluateNode(*Child, Root))
				{
					return true;
				}
			}
			return false;
		case FNode::EKind::And:
			for (const TSharedPtr<FNode>& Child : Node.Children)
			{
				if (!EvaluateNode(*Child, Root))
				{
					return false;
				}
			}
			return true;
		case FNode::EKind::Not:
			return !EvaluateNode(*Node.Children[0], Root);
		case FNode::EKind::Compare:
			return Compare(Node.Op, OperandValue(*Node.Children[0], Root), OperandValue(*Node.Children[1], Root));
		default:
			return FMordecaiAgentBridgePredicate::IsTruthy(OperandValue(Node, Root));
		}
	}

	static void CollectOperands(const FNode& Node, TArray<const FNode*>& OutOperands)
	{
		if (Node.Kind == FNode::EKind::Operand)
		{
			if (Node.OperandKind != FNode::EOperand::Literal)
			{
				OutOperands.Add(&Node);
			}
			return;
		}
		for (const TSharedPtr<FNode>& Child : Node.Children)
		{
			CollectOperands(*Child, OutOperands);
		}
	}
}

FMordecaiAgentBridgePredicate::FMordecaiAgentBridgePredicate() = default;
FMordecaiAgentBridgePredicate::~FMordecaiAgentBridgePredicate() = default;

TSharedPtr<FMordecaiAgentBridgePredicate> FMordecaiAgentBridgePredicate::Parse(const FString& InExpression, FString& OutError)
{
	using namespace MordecaiAgentBridgePredicatePrivate;

	OutError.Reset();
	if (InExpression.Len() > MordecaiAgentBridge::MaxConditionLength)
	{
		OutError = FString::Printf(TEXT("condition longer than %d characters"), MordecaiAgentBridge::MaxConditionLength);
		return nullptr;
	}
	if (InExpression.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("empty condition");
		return nullptr;
	}

	TArray<FToken> Tokens;
	if (!Tokenize(InExpression, Tokens, OutError))
	{
		return nullptr;
	}

	FParser Parser(Tokens);
	const TSharedPtr<FNode> Root = Parser.ParseAll(OutError);
	if (!Root.IsValid())
	{
		return nullptr;
	}

	TSharedPtr<FMordecaiAgentBridgePredicate> Predicate = MakeShared<FMordecaiAgentBridgePredicate>();
	Predicate->Expression = InExpression;
	Predicate->Root = Root;
	return Predicate;
}

bool FMordecaiAgentBridgePredicate::Evaluate(const TSharedPtr<FJsonObject>& InRoot, TSharedPtr<FJsonObject>* OutValues) const
{
	using namespace MordecaiAgentBridgePredicatePrivate;

	if (!Root.IsValid())
	{
		return false;
	}
	const bool bResult = EvaluateNode(*Root, InRoot);

	if (OutValues)
	{
		// Every referenced operand, whether or not evaluation short-circuited past it
		TSharedPtr<FJsonObject> Values = MakeShared<FJsonObject>();
		TArray<const FNode*> Operands;
		CollectOperands(*Root, Operands);
		for (const FNode* Operand : Operands)
		{
			const FString Key = Operand->OperandKind == FNode::EOperand::Len
				? FString::Printf(TEXT("len(%s)"), *Operand->PathText)
				: Operand->PathText;
			Values->SetField(Key, OperandValue(*Operand, InRoot));
		}
		*OutValues = Values;
	}
	return bResult;
}

TArray<FString> FMordecaiAgentBridgePredicate::GetRootIdentifiers() const
{
	using namespace MordecaiAgentBridgePredicatePrivate;

	TArray<FString> Roots;
	if (!Root.IsValid())
	{
		return Roots;
	}
	TArray<const FNode*> Operands;
	CollectOperands(*Root, Operands);
	for (const FNode* Operand : Operands)
	{
		if (Operand->Path.Num() > 0 && !Operand->Path[0].Field.IsEmpty())
		{
			Roots.AddUnique(Operand->Path[0].Field);
		}
	}
	return Roots;
}

TSharedRef<FJsonValue> FMordecaiAgentBridgePredicate::ResolvePath(const TSharedPtr<FJsonObject>& InRoot, const TArray<FMordecaiAgentBridgePathSegment>& Path)
{
	if (!InRoot.IsValid())
	{
		return MakeShared<FJsonValueNull>();
	}

	TSharedPtr<FJsonValue> Current = MakeShared<FJsonValueObject>(InRoot);
	for (const FMordecaiAgentBridgePathSegment& Segment : Path)
	{
		if (!Current.IsValid())
		{
			break;
		}
		if (!Segment.Field.IsEmpty())
		{
			const TSharedPtr<FJsonObject> Object = Current->Type == EJson::Object ? Current->AsObject() : nullptr;
			Current = Object.IsValid() ? Object->TryGetField(Segment.Field) : nullptr;
		}
		else
		{
			if (Current->Type != EJson::Array)
			{
				Current.Reset();
				break;
			}
			const TArray<TSharedPtr<FJsonValue>>& Array = Current->AsArray();
			Current = Array.IsValidIndex(Segment.Index) ? Array[Segment.Index] : nullptr;
		}
	}

	if (!Current.IsValid())
	{
		return MakeShared<FJsonValueNull>();
	}
	return Current.ToSharedRef();
}

TSharedRef<FJsonValue> FMordecaiAgentBridgePredicate::ResolvePathString(const TSharedPtr<FJsonObject>& InRoot, const FString& Path)
{
	using namespace MordecaiAgentBridgePredicatePrivate;

	TArray<FToken> Tokens;
	FString Error;
	TArray<FMordecaiAgentBridgePathSegment> Segments;
	if (!Tokenize(Path, Tokens, Error))
	{
		return MakeShared<FJsonValueNull>();
	}
	FParser Parser(Tokens);
	if (!Parser.ParsePathOnly(Segments))
	{
		return MakeShared<FJsonValueNull>();
	}
	return ResolvePath(InRoot, Segments);
}

bool FMordecaiAgentBridgePredicate::IsTruthy(const TSharedPtr<FJsonValue>& Value)
{
	if (!Value.IsValid())
	{
		return false;
	}
	switch (Value->Type)
	{
	case EJson::Boolean: return Value->AsBool();
	case EJson::Number:
	{
		double Number = 0.0;
		return Value->TryGetNumber(Number) && Number != 0.0;
	}
	case EJson::String: return !Value->AsString().IsEmpty();
	case EJson::Array: return Value->AsArray().Num() > 0;
	case EJson::Object: return Value->AsObject().IsValid() && Value->AsObject()->Values.Num() > 0;
	default: return false;
	}
}

bool FMordecaiAgentBridgePredicate::ValuesEqual(const TSharedPtr<FJsonValue>& A, const TSharedPtr<FJsonValue>& B)
{
	const EJson TypeA = A.IsValid() ? A->Type : EJson::Null;
	const EJson TypeB = B.IsValid() ? B->Type : EJson::Null;
	if (TypeA != TypeB)
	{
		return false;
	}
	switch (TypeA)
	{
	case EJson::Null:
	case EJson::None:
		return true;
	case EJson::Number:
	{
		double X = 0.0;
		double Y = 0.0;
		return A->TryGetNumber(X) && B->TryGetNumber(Y) && X == Y;
	}
	case EJson::String: return A->AsString().Equals(B->AsString(), ESearchCase::CaseSensitive);
	case EJson::Boolean: return A->AsBool() == B->AsBool();
	default: return FJsonValue::CompareEqual(*A, *B);
	}
}
