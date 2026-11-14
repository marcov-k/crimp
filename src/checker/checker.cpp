#include "checker.h"

#include "names.h"
#include "scopes.h"
#include "symbols.h"
#include "types.h"
#include "../ast/ast.h"
#include "../common/arena.h"
#include "../common/utils.h"
#include "../diagnostics/diagnostics.h"
#include "../diagnostics/source_map.h"

#include <cassert>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
	struct CheckerState
	{
		const SourceMap& source;
		FileId fileId;
		Diagnostics& diag;
		Arena& arena;
		Names names;
		Scopes scopes;

		CheckerState(const SourceMap& source, FileId fileId, Diagnostics& diag, Arena& arena) :
			source(source), fileId(fileId), diag(diag), arena(arena), names(arena) {}
	};

	const Type* ExpectedMainReturn()
	{
		return IntType();
	}

	constexpr bool ValidEscapeChar(char c)
	{
		constexpr char validEscapes[] =
		{
			'n',
			't',
			'\\',
			'\"',
		};

		for (char valid : validEscapes)
		{
			if (c == valid) return true;
		}

		return false;
	}

	Symbol* DeclareType(CheckerState& state, const Type* type)
	{
		NameId id{ state.names.Intern(type->name) };
		Symbol* symbol{ state.arena.Create<Symbol>() };
		symbol->kind = SymbolKind::Type;
		symbol->name = state.names.Text(id);
		symbol->nameId = id;
		symbol->type = type;
		[[maybe_unused]] Symbol* res{ state.scopes.Declare(id, symbol) };
		assert(!res);

		return symbol;
	}

	void DeclareBuiltins(CheckerState& state)
	{
		DeclareType(state, VoidType());
		DeclareType(state, IntType());
		DeclareType(state, StringType());

		// print()
		NameId id{ state.names.Intern("print") };
		Symbol* symbol{ state.arena.Create<Symbol>() };
		symbol->kind = SymbolKind::Function;
		symbol->name = state.names.Text(id);
		symbol->nameId = id;
		const Type** params{ state.arena.CopyArray<const Type*>(std::vector<const Type*>{ StringType() }) };
		symbol->paramTypes = { params, 1 };
		symbol->isVariadic = true;
		symbol->returnType = VoidType();
		symbol->linkName = "printf";
		symbol->isBuiltin = true;
		[[maybe_unused]] Symbol* res{ state.scopes.Declare(id, symbol) };
		assert(!res);
	}

	bool IsEmptySpan(const Span& span)
	{
		return span.begin == span.end;
	}

	std::string_view SpanToStringView(const CheckerState& state, const Span& span)
	{
		const auto& text{ state.source.File(state.fileId).text };

		assert(span.begin <= span.end && span.end <= text.size());
		if (span.begin > span.end || span.end > text.size()) return {};

		return { text.substr(span.begin, span.end - span.begin) };
	}

	void Report(CheckerState& state, Span span, std::string_view message)
	{
		if (state.diag.LimitReached()) return;

		state.diag.Error(span, std::string{ message });
	}

	void ReportWithNote(CheckerState& state, Span span, std::string_view message, const Note& note)
	{
		if (state.diag.LimitReached()) return;

		Diagnostic diag{ Severity::Error, span, std::string{ message }, { note } };
		state.diag.Emit(std::move(diag));
	}

	const Type* ResolveType(CheckerState& state, const Span& nameSpan, bool voidAllowed = false)
	{
		if (IsEmptySpan(nameSpan)) return ErrorType();

		std::string_view name{ SpanToStringView(state, nameSpan) };
		NameId id{ state.names.Intern(name) };

		const Symbol* symbol{ state.scopes.Lookup(id) };
		if (!symbol)
		{
			Report(state, nameSpan, std::format("unknown type '{}'", name));
			return ErrorType();
		}

		if (symbol->kind != SymbolKind::Type)
		{
			Report(state, nameSpan, std::format("'{}' is not a type", name));
			return ErrorType();
		}

		if (!voidAllowed && symbol->type == VoidType())
		{
			Report(state, nameSpan, "'void' is not allowed here");
			return ErrorType();
		}

		return symbol->type;
	}

	void DeclareFunction(CheckerState& state, AstDecl* decl)
	{
		const Type* returnType{ ResolveType(state, decl->typeRef, true) };

		std::vector<const Type*> localTypes{};
		std::unordered_map<NameId, Symbol*> localSymbols{};
		for (AstDecl* param : decl->params)
		{
			const Type* paramType{ ResolveType(state, param->typeRef) };
			localTypes.push_back(paramType);

			if (!IsEmptySpan(param->name))
			{
				NameId paramId{ state.names.Intern(param->name, state.source) };
				Symbol* paramSymbol{ state.arena.Create<Symbol>() };
				paramSymbol->kind = SymbolKind::Param;
				paramSymbol->name = state.names.Text(paramId);
				paramSymbol->nameId = paramId;
				paramSymbol->type = paramType;
				paramSymbol->declSpan = param->name;
				paramSymbol->declNode = param;
				param->symbol = paramSymbol;

				auto paramRes{ localSymbols.try_emplace(paramId, paramSymbol) };
				if (!paramRes.second)
				{
					ReportWithNote(state, param->name, std::format("duplicate parameter name '{}'", paramSymbol->name),
						{ paramRes.first->second->declSpan, "name already declared here" });
				}
			}
		}

		NameId funcId{ state.names.Intern(decl->name, state.source) };
		Symbol* funcSymbol{ state.arena.Create<Symbol>() };
		funcSymbol->kind = SymbolKind::Function;
		funcSymbol->name = state.names.Text(funcId);
		funcSymbol->nameId = funcId;
		funcSymbol->returnType = returnType;
		funcSymbol->paramTypes = { state.arena.CopyArray<const Type*>(localTypes), localTypes.size() };
		funcSymbol->linkName = funcSymbol->name;
		funcSymbol->declSpan = decl->name;
		funcSymbol->declNode = decl;

		Symbol* existing{ state.scopes.Declare(funcId, funcSymbol) };
		if (existing)
		{
			if (existing->isBuiltin)
			{
				Report(state, decl->name, std::format("'{}' is a reserved name", funcSymbol->name));
			}
			else
			{
				ReportWithNote(state, decl->name, std::format("function '{}' is already defined", funcSymbol->name),
					{ existing->declSpan, "already defined here" });
			}
		}

		decl->symbol = funcSymbol;
	}

	void CheckMain(CheckerState& state)
	{
		NameId id{ state.names.Intern("main") };
		Symbol* symbol{ state.scopes.Lookup(id) };

		if (!symbol)
		{
			Report(state, { state.fileId, 0, 0 }, "program has no 'main' function");
		}
		else if (symbol->kind != SymbolKind::Function)
		{
			Report(state, symbol->declSpan, "'main' must be a function");
		}
		else if (symbol->returnType != ExpectedMainReturn() && symbol->returnType != ErrorType())
		{
			Report(state, symbol->declNode->typeRef, std::format("'main' must return '{}'", ExpectedMainReturn()->name));
		}
		else if (symbol->paramTypes.count > 0)
		{
			Report(state, symbol->declSpan, "'main' must take no parameters");
		}
	}

	const Type* ValidateIntLiteral(CheckerState& state, AstExpr* expr)
	{
		std::uint64_t acc{ 0 };
		std::string_view text{ SpanToStringView(state, expr->span) };

		for (char chara : text)
		{
			assert(IsDigit(chara));

			acc *= 10;
			acc += static_cast<std::uint64_t>(chara - '0');

			if (acc > 2147483647)
			{
				Report(state, expr->span, "integer literal out of range");
				break;
			}
		}

		return IntType();
	}

	const Type* ValidateStringLiteral(CheckerState& state, AstExpr* expr)
	{
		std::string_view text{ SpanToStringView(state, expr->span) };

		for (std::size_t i{ 1 }; i + 1 < text.size(); ++i)
		{
			if (text[i] == '\\')
			{
				char next{ text[i + 1] };
				if (!ValidEscapeChar(next))
				{
					Report(state, { state.fileId, expr->span.begin + i, expr->span.begin + i + 2 }, std::format("invalid escape sequence '\\{}'", next));
				}
				++i;
			}
		}

		return StringType();
	}

	const Type* ValidateName(CheckerState& state, AstExpr* expr)
	{
		NameId id{ state.names.Intern(expr->name, state.source) };
		Symbol* symbol{ state.scopes.Lookup(id) };
		if (!symbol)
		{
			Report(state, expr->name, std::format("undefined name '{}'", SpanToStringView(state, expr->name)));
			return ErrorType();
		}

		expr->symbol = symbol;

		switch (symbol->kind)
		{
		case SymbolKind::Function:
			Report(state, expr->name, std::format("'{}' is a function, not a variable", symbol->name));
			break;
		case SymbolKind::Param:
			[[fallthrough]];
		case SymbolKind::Local:
			return symbol->type;
		case SymbolKind::Type:
			Report(state, expr->name, std::format("'{}' is a type, not a variable", symbol->name));
			break;
		}

		return ErrorType();
	}

	const Type* ValidateBinary(CheckerState& state, AstExpr* expr)
	{

	}

	const Type* ValidateCall(CheckerState& state, AstExpr* expr)
	{

	}

	const Type* CheckExpression(CheckerState& state, AstExpr* expr)
	{
		assert(expr);

		const Type* type{ nullptr };
		switch (expr->kind)
		{
		case ExprKind::Error:
			type = ErrorType();
			break;
		case ExprKind::IntLiteral:
			type = ValidateIntLiteral(state, expr);
			break;
		case ExprKind::StringLiteral:
			type = ValidateStringLiteral(state, expr);
			break;
		case ExprKind::Name:
			type = ValidateName(state, expr);
			break;
		case ExprKind::Binary:
			type = ValidateBinary(state, expr);
			break;
		case ExprKind::Call:
			type = ValidateCall(state, expr);
			break;
		}
		assert(type);
		expr->type = type;

		return expr->type;
	}

	void CheckStatement(CheckerState& state, const Symbol* currentFunc, AstStmt* stmt)
	{

	}

	void CheckFunctionBody(CheckerState& state, AstDecl* decl)
	{
		if (!decl->body || decl->body->kind == StmtKind::Error || !decl->symbol) return;

		ScopeGuard guard{ state.scopes };

		for (AstDecl* param : decl->params)
		{
			if (param->symbol) state.scopes.Declare(param->symbol->nameId, param->symbol);
		}
	}
}

void Analyze(AstProgram* program, const SourceMap& source, Diagnostics& diag, Arena& arena)
{
	CheckerState state{ source, program->fileId, diag, arena };

	ScopeGuard guard{ state.scopes };
	DeclareBuiltins(state);

	for (AstDecl* decl : program->body)
	{
		if (!IsEmptySpan(decl->name))
		{
			DeclareFunction(state, decl);
		}
	}

	CheckMain(state);

	for (AstDecl* decl : program->body)
	{
		if (!IsEmptySpan(decl->name))
		{
			CheckFunctionBody(state, decl);
		}
	}
}