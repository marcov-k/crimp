#include "emitter.h"

#include "il_writer.h"
#include "../ast/ast.h"
#include "../checker/symbols.h"
#include "../checker/types.h"
#include "../common/utils.h"
#include "../diagnostics/source_map.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace
{
	struct EmitterState
	{
		const SourceMap& source;
		FileId fileId;
		std::string_view text{};
		ILWriter writer{};
		std::unordered_map<const Symbol*, std::string> symbolMap{};
	};

	QBEType ToQBE(const Type* type)
	{
		assert(type);

		switch (type->kind)
		{
		case TypeKind::Error:
			assert(false);
			break;
		case TypeKind::Void:
			return QBEType::None;
		case TypeKind::Int:
			return QBEType::W;
		case TypeKind::String:
			return QBEType::L;
		}

		return QBEType::None;
	}

	OpCode OpFor(TokenKind op)
	{
		switch (op)
		{
		case TokenKind::Plus:
			return OpCode::Add;
		}

		assert(false);
		return OpCode::Add;
	}

	std::string GenExpr(EmitterState& state, const AstExpr* expr);

	std::string GenIntLiteral(const AstExpr* expr)
	{
		return std::format("{}", expr->intValue);
	}

	std::string GenStringLiteral(EmitterState& state, const AstExpr* expr)
	{
		assert(expr->span.end - expr->span.begin >= 2);
		return state.writer.InternString(std::string{ SpanToStringView(state.text, { state.fileId, expr->span.begin + 1, expr->span.end - 1 }) });
	}

	std::string GenName(EmitterState& state, const AstExpr* expr)
	{
		auto found{ state.symbolMap.find(expr->symbol) };
		assert(found != state.symbolMap.end());
		return state.writer.Load(ToQBE(expr->type), found->second);
	}

	std::string GenBinary(EmitterState& state, const AstExpr* expr)
	{
		std::string lhs{ GenExpr(state, expr->lhs) };
		std::string rhs{ GenExpr(state, expr->rhs) };
		return state.writer.Binary(OpFor(expr->op), QBEType::W, lhs, rhs);
	}

	std::string GenCall(EmitterState& state, const AstExpr* expr)
	{
		std::vector<ILArg> args{};
		args.reserve(expr->args.count);
		for (const AstExpr* arg : expr->args)
		{
			args.push_back(ILArg{ ToQBE(arg->type), GenExpr(state, arg) });
		}

		std::size_t fixed{ expr->symbol->isVariadic ? expr->symbol->paramTypes.count : NOT_VARIADIC };
		
		return state.writer.Call(ToQBE(expr->type), expr->symbol->linkName, args, fixed);
	}

	std::string GenExpr(EmitterState& state, const AstExpr* expr)
	{
		switch (expr->kind)
		{
		case ExprKind::Error:
			assert(false);
			break;
		case ExprKind::IntLiteral:
			return GenIntLiteral(expr);
		case ExprKind::StringLiteral:
			return GenStringLiteral(state, expr);
		case ExprKind::Name:
			return GenName(state, expr);
		case ExprKind::Binary:
			return GenBinary(state, expr);
		case ExprKind::Call:
			return GenCall(state, expr);
		}

		return "";
	}

	void EmitStmt(EmitterState& state, const AstStmt* stmt);

	void EmitVarDecl(EmitterState& state, const AstStmt* stmt)
	{
		std::string value{ GenExpr(state, stmt->expr) };
		const Type* type{ stmt->symbol->type };
		std::string slot{ state.writer.Alloc(stmt->symbol->name, type->size, type->size) };

		state.writer.Store(ToQBE(type), value, slot);
		state.symbolMap[stmt->symbol] = slot;
	}

	void EmitReturn(EmitterState& state, const AstStmt* stmt)
	{
		if (stmt->expr) state.writer.Ret(GenExpr(state, stmt->expr));
		else state.writer.Ret();
	}

	void EmitExprStmt(EmitterState& state, const AstStmt* stmt)
	{
		GenExpr(state, stmt->expr);
	}

	void EmitBlock(EmitterState& state, const AstStmt* stmt)
	{
		for (const AstStmt* child : stmt->stmts)
		{
			EmitStmt(state, child);
		}
	}

	void EmitStmt(EmitterState& state, const AstStmt* stmt)
	{
		switch (stmt->kind)
		{
		case StmtKind::Error:
			break;
		case StmtKind::VarDecl:
			EmitVarDecl(state, stmt);
			break;
		case StmtKind::Return:
			EmitReturn(state, stmt);
			break;
		case StmtKind::ExprStmt:
			EmitExprStmt(state, stmt);
			break;
		case StmtKind::Block:
			EmitBlock(state, stmt);
			break;
		}
	}

	void EmitFunction(EmitterState& state, const AstDecl* decl)
	{
		const Symbol* symbol{ decl->symbol };

		std::vector<ILParam> params{};
		params.reserve(decl->params.count);
		for (const AstDecl* param : decl->params)
		{
			QBEType type{ ToQBE(param->symbol->type) };
			assert(type != QBEType::None);
			params.push_back({ type, param->symbol->name });
		}

		state.writer.BeginFunction(symbol->linkName, ToQBE(symbol->returnType), params);

		for (const AstDecl* param : decl->params)
		{
			const Type* type{ param->symbol->type };
			std::string slot{ state.writer.Alloc(param->symbol->name, type->size, type->size) };

			state.writer.Store(ToQBE(type), ILWriter::ParamOperand(param->symbol->name), slot);
			state.symbolMap[param->symbol] = slot;
		}

		assert(decl->body);
		EmitStmt(state, decl->body);

		state.writer.EndFunction();
		state.symbolMap.clear();
	}
}

std::string EmitIL(const SourceMap& source, const AstProgram* program)
{
	EmitterState state{ .source = source, .fileId = program->fileId, .text = source.File(program->fileId).text };
	
	for (const AstDecl* func : program->body)
	{
		EmitFunction(state, func);
	}

	return state.writer.Finish();
}