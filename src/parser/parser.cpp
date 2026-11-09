#include "parser.h"

#include "../ast/ast.h"
#include "../common/arena.h"
#include "../common/span.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"
#include "../lexer/tokens.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	constexpr std::size_t MAX_DEPTH{ 300 };
	constexpr int NO_OP_PREC{ -1 };

	struct ParserState
	{
		const std::vector<Token>& tokens;
		std::size_t cursor{ 0 };
		Diagnostics& diag;
		Arena& arena;
		FileId fileId{};
		std::size_t lastError{ SIZE_MAX };
		std::size_t exprDepth{ 0 };
	};

	std::size_t PrevEnd(const ParserState& state)
	{
		if (state.cursor == 0) return 0;
		return state.tokens[state.cursor - 1].span.end;
	}

	Span SpanFrom(std::size_t startToken, const ParserState& state)
	{
		std::size_t begin{ state.tokens[startToken].span.begin };
		return { state.fileId, begin, std::max(begin, PrevEnd(state)) };
	}

	const Token& Peek(std::size_t offset, const ParserState& state)
	{
		std::size_t index{ std::min(state.cursor + offset, state.tokens.size() - 1) };
		return state.tokens[index];
	}

	const Token& Advance(ParserState& state)
	{
		const Token& token{ state.tokens[state.cursor] };

		if (state.cursor + 1 < state.tokens.size())
		{
			++state.cursor;
		}

		return token;
	}

	bool At(TokenType type, const ParserState& state)
	{
		return state.tokens[state.cursor].type == type;
	}

	bool Accept(TokenType type, ParserState& state)
	{
		if (At(type, state))
		{
			Advance(state);
			return true;
		}

		return false;
	}

	std::string DescribeToken(TokenType type)
	{
		std::string out{ "found " };

		bool quote{ IsOperator(type) || IsDelimiter(type) };

		if (quote) out.append("'");
		out.append(GetSpelling(type));
		if (quote) out.append("'");

		return out;
	}

	void Report(ParserState& state, Span span, std::string_view message)
	{
		if (state.lastError == state.cursor || At(TokenType::Error, state)) return;

		state.diag.Error(span, std::string{ message });
		state.lastError = state.cursor;
	}

	void ReportWithNote(ParserState& state, Span span, std::string_view message, const Note& note)
	{
		if (state.lastError == state.cursor || At(TokenType::Error, state)) return;

		Diagnostic diag{ Severity::Error, span, std::string{ message }, { note } };
		state.diag.Emit(std::move(diag));
		state.lastError = state.cursor;
	}

	bool Expect(TokenType type, ParserState& state, std::string_view message)
	{
		if (!Accept(type, state))
		{
			std::size_t end{ PrevEnd(state) };

			std::string log{ message };
			log.append(", " + DescribeToken(Peek(0, state).type));

			Report(state, { state.fileId, end, end }, log);

			return false;
		}

		return true;
	}

	AstExpr* MakeExpr(ExprKind kind, Span span, ParserState& state)
	{
		AstExpr* expr{ state.arena.Create<AstExpr>() };
		expr->kind = kind;
		expr->span = span;
		
		return expr;
	}

	AstExpr* ErrorExpr(Span span, ParserState& state)
	{
		return MakeExpr(ExprKind::Error, span, state);
	}

	AstStmt* MakeStmt(StmtKind kind, Span span, ParserState& state)
	{
		AstStmt* stmt{ state.arena.Create<AstStmt>() };
		stmt->kind = kind;
		stmt->span = span;

		return stmt;
	}

	AstStmt* ErrorStmt(Span span, ParserState& state)
	{
		return MakeStmt(StmtKind::Error, span, state);
	}

	AstDecl* MakeDecl(DeclKind kind, Span span, ParserState& state)
	{
		AstDecl* decl{ state.arena.Create<AstDecl>() };
		decl->kind = kind;
		decl->span = span;

		return decl;
	}

	template<typename T>
	List<T*> ToList(const std::vector<T*>& vec, ParserState& state)
	{
		return { state.arena.CopyArray<T*>(vec), vec.size() };
	}

	bool LooksLikeFunction(const ParserState& state)
	{
		TokenType current{ Peek(0, state).type };
		TokenType next{ Peek(1, state).type };

		return (current == TokenType::Identifier || current == TokenType::KwVoid) && next == TokenType::Identifier;
	}

	void SyncStatement(ParserState& state)
	{
		while (!At(TokenType::Semicolon, state))
		{
			TokenType token{ Peek(0, state).type };

			if (token == TokenType::RBrace || token == TokenType::Eof || token == TokenType::KwReturn) return;

			Advance(state);
		}
		Advance(state);
	}

	void SyncTopLevel(ParserState& state)
	{
		std::size_t braceDepth{ 0 };
		if (At(TokenType::LBrace, state)) braceDepth = 1;

		Advance(state);
		while (!At(TokenType::Eof, state))
		{
			if (braceDepth == 0 && LooksLikeFunction(state)) return;

			if (At(TokenType::LBrace, state)) ++braceDepth;
			else if (At(TokenType::RBrace, state) && braceDepth > 0) --braceDepth;

			Advance(state);
		}
	}

	int PrecedenceOf(TokenType type)
	{
		switch (type)
		{
		case TokenType::Plus:
			return 10;
		default:
			return NO_OP_PREC;
		}
	}

	AstExpr* ParseTerm(ParserState& state);

	AstExpr* ParseBinary(ParserState& state, int minPrec)
	{
		std::size_t start{ state.cursor };

		AstExpr* lhs{ ParseTerm(state) };
		while (PrecedenceOf(Peek(0, state).type) >= minPrec)
		{
			TokenType op{ Advance(state).type };
			AstExpr* rhs{ ParseBinary(state, PrecedenceOf(op) + 1) };

			AstExpr* binary{ MakeExpr(ExprKind::Binary, SpanFrom(start, state), state) };
			binary->op = op;
			binary->lhs = lhs;
			binary->rhs = rhs;

			lhs = binary;
		}

		return lhs;
	}

	AstExpr* ParseExpression(ParserState& state)
	{
		if (state.exprDepth >= MAX_DEPTH)
		{
			Span span{ Peek(0, state).span };
			Report(state, span, "expression nested too deeply");
			return ErrorExpr({ state.fileId, span.begin, span.begin }, state);
		}

		++state.exprDepth;
		AstExpr* expr{ ParseBinary(state, 0) };
		--state.exprDepth;

		return expr;
	}

	AstExpr* ParseCall(ParserState& state)
	{
		std::size_t startToken{ state.cursor };
		Span nameSpan{ Peek(0, state).span };

		Advance(state); // identifier
		Advance(state); // '('

		std::vector<AstExpr*> args{};
		if (!At(TokenType::RParen, state))
		{
			args.push_back(ParseExpression(state));

			while (Accept(TokenType::Comma, state))
			{
				args.push_back(ParseExpression(state));
			}
		}

		Expect(TokenType::RParen, state, "expected ')' to close argument list");

		AstExpr* call{ MakeExpr(ExprKind::Call, SpanFrom(startToken, state), state) };
		call->name = nameSpan;
		call->args = ToList<AstExpr>(args, state);

		return call;
	}

	AstExpr* ParseTerm(ParserState& state)
	{
		const Token& token{ Peek(0, state) };
		TokenType type{ token.type };
		Span span{ token.span };

		switch (type)
		{
		case TokenType::Error:
			Advance(state);
			return ErrorExpr(span, state);
		case TokenType::Identifier:
			if (Peek(1, state).type == TokenType::LParen)
			{
				return ParseCall(state);
			}
			else
			{
				Advance(state);
				AstExpr* expr{ MakeExpr(ExprKind::Name, span, state) };
				expr->name = span;
				return expr;
			}
		case TokenType::IntLiteral:
			Advance(state);
			return MakeExpr(ExprKind::IntLiteral, span, state);
		case TokenType::StringLiteral:
			Advance(state);
			return MakeExpr(ExprKind::StringLiteral, span, state);
		case TokenType::LParen:
		{
			Advance(state);
			AstExpr* expr{ ParseExpression(state) };
			Expect(TokenType::RParen, state, "expected ')' to close expression");
			return expr;
		}
		default:
			Report(state, span, "expected expression, " + DescribeToken(type));
			return ErrorExpr({ state.fileId, span.begin, span.begin }, state);
		}
	}

	AstStmt* ParseReturn(ParserState& state)
	{
		std::size_t start{ state.cursor };

		Advance(state); // 'return'

		AstExpr* expr{ ParseExpression(state) };
		Expect(TokenType::Semicolon, state, "expected ';' after return value");

		AstStmt* stmt{ MakeStmt(StmtKind::Return, SpanFrom(start, state), state) };
		stmt->expr = expr;

		return stmt;
	}

	AstStmt* ParseVarDecl(ParserState& state)
	{
		assert(At(TokenType::Identifier, state));
		assert(Peek(1, state).type == TokenType::Identifier);

		std::size_t start{ state.cursor };
		Span typeSpan{ Peek(0, state).span };
		Advance(state);
		Span nameSpan{ Peek(0, state).span };
		Advance(state);

		Span declSpan;

		AstExpr* init{ nullptr };
		if (Expect(TokenType::Assign, state, "expected '=' after variable name"))
		{
			init = ParseExpression(state);
			Expect(TokenType::Semicolon, state, "expected ';' after initializer");
			declSpan = SpanFrom(start, state);
		}
		else
		{
			std::size_t tokStart{ Peek(0, state).span.begin };
			init = ErrorExpr({ state.fileId, tokStart, tokStart }, state);
			declSpan = SpanFrom(start, state);
			SyncStatement(state);
		}

		AstStmt* decl{ MakeStmt(StmtKind::VarDecl, declSpan, state) };
		decl->name = nameSpan;
		decl->typeRef = typeSpan;
		decl->expr = init;

		return decl;
	}

	AstStmt* ParseExprStmt(ParserState& state)
	{
		std::size_t start{ state.cursor };

		AstExpr* expr{ ParseExpression(state) };

		Expect(TokenType::Semicolon, state, "expected ';' after expression");

		AstStmt* stmt{ MakeStmt(StmtKind::ExprStmt, SpanFrom(start, state), state) };
		stmt->expr = expr;
		
		return stmt;
	}

	AstStmt* ParseStatement(ParserState& state)
	{
		TokenType type{ Peek(0, state).type };

		switch (type)
		{
		case TokenType::Identifier:
			if (Peek(1, state).type == TokenType::Identifier)
			{
				return ParseVarDecl(state);
			}
			return ParseExprStmt(state);
		case TokenType::KwReturn:
			return ParseReturn(state);
		default:
			return ParseExprStmt(state);
		}
	}

	AstStmt* ParseBlock(ParserState& state)
	{
		assert(At(TokenType::LBrace, state));

		std::size_t lBracePos{ state.cursor };
		Span lBraceSpan{ Peek(0, state).span };

		Advance(state); // '{'

		std::vector<AstStmt*> stmts{};
		while (!At(TokenType::RBrace, state) && !At(TokenType::Eof, state))
		{
			std::size_t iterCursor{ state.cursor };

			stmts.push_back(ParseStatement(state));

			if (state.cursor == iterCursor) Advance(state);
		}

		if (!Accept(TokenType::RBrace, state))
		{
			std::size_t prevEnd{ PrevEnd(state) };
			ReportWithNote(state, { state.fileId, prevEnd, prevEnd }, "expected '}' to close block, " + DescribeToken(Peek(0, state).type), { lBraceSpan, "block opened here" });
		}

		AstStmt* block{ MakeStmt(StmtKind::Block, SpanFrom(lBracePos, state), state) };
		block->stmts = ToList<AstStmt>(stmts, state);

		return block;
	}

	AstDecl* ParseParam(ParserState& state)
	{
		std::size_t start{ state.cursor };

		Span typeSpan{ Peek(0, state).span };
		Expect(TokenType::Identifier, state, "expected parameter type");

		Span nameSpan{ Peek(0, state).span };
		Expect(TokenType::Identifier, state, "expected parameter name");

		AstDecl* param{ MakeDecl(DeclKind::Param, SpanFrom(start, state), state) };
		param->typeRef = typeSpan;
		param->name = nameSpan;

		return param;
	}

	AstDecl* ParseFunction(ParserState& state)
	{
		std::size_t start{ state.cursor };

		Span typeSpan{ Peek(0, state).span };
		Advance(state);

		Span nameSpan{ Peek(0, state).span };
		Advance(state);

		Expect(TokenType::LParen, state, "expected '(' after function name");

		std::vector<AstDecl*> params{};
		if (!At(TokenType::RParen, state))
		{
			params.push_back(ParseParam(state));

			while (Accept(TokenType::Comma, state))
			{
				params.push_back(ParseParam(state));
			}
		}

		Expect(TokenType::RParen, state, "expected ')' after parameters");

		AstStmt* body{ nullptr };
		if (At(TokenType::LBrace, state))
		{
			body = ParseBlock(state);
		}
		else
		{
			while (!At(TokenType::LBrace, state) && !At(TokenType::Semicolon, state) && !At(TokenType::RBrace, state) && !At(TokenType::Eof, state)) Advance(state);

			if (At(TokenType::LBrace, state))
			{
				body = ParseBlock(state);
			}
			else
			{
				const Token& token{ Peek(0, state) };
				Report(state, SpanFrom(start, state), "expected function body, " + DescribeToken(token.type));
				body = ErrorStmt({ state.fileId, token.span.begin, token.span.begin }, state);

				SyncTopLevel(state);
			}
		}

		AstDecl* func{ MakeDecl(DeclKind::Function, SpanFrom(start, state), state) };
		func->typeRef = typeSpan;
		func->name = nameSpan;
		func->params = ToList<AstDecl>(params, state);
		func->body = body;

		return func;
	}

	AstProgram* ParseProgram(ParserState& state)
	{
		std::vector<AstDecl*> decls{};

		while (!At(TokenType::Eof, state))
		{
			std::size_t iterCursor{ state.cursor };

			if (LooksLikeFunction(state))
			{
				decls.push_back(ParseFunction(state));
			}
			else
			{
				const Token& token{ Peek(0, state) };

				std::string msg{ "expected a function declaration, " };
				msg.append(DescribeToken(token.type));
				
				Report(state, token.span, msg);
				SyncTopLevel(state);
			}

			if (state.cursor == iterCursor) Advance(state);
		}

		AstProgram* program{ state.arena.Create<AstProgram>() };
		program->fileId = state.fileId;

		AstDecl** declList{ state.arena.CopyArray<AstDecl*>(decls) };
		program->body = { declList, decls.size() };

		return program;
	}
}

AstProgram* Parse(FileId fileId, const std::vector<Token>& tokens, Diagnostics& diag, Arena& arena)
{
	assert(!tokens.empty());

	ParserState state{ .tokens = tokens, .diag = diag, .arena = arena, .fileId = fileId };
	
	return ParseProgram(state);
}