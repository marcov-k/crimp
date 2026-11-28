#include "ast_dump.h"

#include "ast.h"
#include "../checker/symbols.h"
#include "../checker/types.h"
#include "../common/span.h"
#include "../common/utils.h"
#include "../diagnostics/source_map.h"
#include "../lexer/tokens.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace
{
	void AppendSpan(const Span& span, std::string_view text, std::string& output)
	{
		assert(span.begin <= span.end && span.end <= text.size());

		if (span.begin > span.end || span.end > text.size())
		{
			output.append("<bad-span>");
			return;
		}

		output.append(text.substr(span.begin, span.end - span.begin));
	}

	void AppendType(const Type* type, std::string& output)
	{
		output.append(" : ");
		if (type)
		{
			if (type == ErrorType())
			{
				output.append("<error>");
			}
			else
			{
				output.append(type->name);
			}
		}
		else
		{
			output.append("<untyped>");
		}
	}

	void DumpExpr(const AstExpr* expr, std::string_view text, std::string& output, bool typed);

	void DumpIntLiteral(const AstExpr* expr, std::string_view text, std::string& output, bool typed)
	{
		output.append("(int ");
		AppendSpan(expr->span, text, output);
		if (typed) AppendType(expr->type, output);
		output.append(")");
	}

	void DumpStringLiteral(const AstExpr* expr, std::string_view text, std::string& output, bool typed)
	{
		output.append("(string ");
		AppendSpan(expr->span, text, output);
		if (typed) AppendType(expr->type, output);
		output.append(")");
	}

	void DumpName(const AstExpr* expr, std::string_view text, std::string& output, bool typed)
	{
		output.append("(name ");
		AppendSpan(expr->name, text, output);
		if (typed) AppendType(expr->type, output);
		output.append(")");
	}

	void DumpBinary(const AstExpr* expr, std::string_view text, std::string& output, bool typed)
	{
		output.append("(");
		output.append(GetSpelling(expr->op));
		output.append(" ");
		DumpExpr(expr->lhs, text, output, typed);
		output.append(" ");
		DumpExpr(expr->rhs, text, output, typed);
		if (typed) AppendType(expr->type, output);
		output.append(")");
	}

	void DumpCall(const AstExpr* expr, std::string_view text, std::string& output, bool typed)
	{
		output.append("(call ");
		AppendSpan(expr->name, text, output);
		for (const AstExpr* arg : expr->args)
		{
			output.append(" ");
			DumpExpr(arg, text, output, typed);
		}
		if (typed) AppendType(expr->type, output);
		output.append(")");
	}

	void DumpExpr(const AstExpr* expr, std::string_view text, std::string& output, bool typed)
	{
		if (!expr)
		{
			output.append("(null)");
			return;
		}

		switch (expr->kind)
		{
		case ExprKind::Error:
			output.append("(error)");
			break;
		case ExprKind::IntLiteral:
			DumpIntLiteral(expr, text, output, typed);
			break;
		case ExprKind::StringLiteral:
			DumpStringLiteral(expr, text, output, typed);
			break;
		case ExprKind::Name:
			DumpName(expr, text, output, typed);
			break;
		case ExprKind::Binary:
			DumpBinary(expr, text, output, typed);
			break;
		case ExprKind::Call:
			DumpCall(expr, text, output, typed);
			break;
		}
	}

	void DumpStmt(const AstStmt* stmt, std::string_view text, std::size_t depth, std::string& output, bool typed);

	void DumpVarDecl(const AstStmt* stmt, std::string_view text, std::string& output, bool typed)
	{
		output.append("(var ");
		AppendSpan(stmt->typeRef, text, output);
		output.append(" ");
		AppendSpan(stmt->name, text, output);
		output.append(" ");
		DumpExpr(stmt->expr, text, output, typed);
		output.append(")");
	}

	void DumpReturn(const AstStmt* stmt, std::string_view text, std::string& output, bool typed)
	{
		output.append("(return");
		if (stmt->expr)
		{
			output.append(" ");
			DumpExpr(stmt->expr, text, output, typed);
		}
		output.append(")");
	}

	void DumpExprStmt(const AstStmt* stmt, std::string_view text, std::string& output, bool typed)
	{
		output.append("(expr ");
		DumpExpr(stmt->expr, text, output, typed);
		output.append(")");
	}

	void DumpBlock(const AstStmt* stmt, std::string_view text, std::size_t depth, std::string& output, bool typed)
	{
		output.append("(block");
		for (const AstStmt* child : stmt->stmts)
		{
			output.append("\n");
			DumpStmt(child, text, depth + 1, output, typed);
		}
		output.append(")");
	}

	void DumpStmt(const AstStmt* stmt, std::string_view text, std::size_t depth, std::string& output, bool typed)
	{
		output.append(depth * 2, ' ');

		if (!stmt)
		{
			output.append("(null)");
			return;
		}

		switch (stmt->kind)
		{
		case StmtKind::Error:
			output.append("(error)");
			break;
		case StmtKind::VarDecl:
			DumpVarDecl(stmt, text, output, typed);
			break;
		case StmtKind::Return:
			DumpReturn(stmt, text, output, typed);
			break;
		case StmtKind::ExprStmt:
			DumpExprStmt(stmt, text, output, typed);
			break;
		case StmtKind::Block:
			DumpBlock(stmt, text, depth, output, typed);
			break;
		}
	}

	void DumpDecl(const AstDecl* decl, std::string_view text, std::size_t depth, std::string& output, bool typed);

	void DumpFunction(const AstDecl* decl, std::string_view text, std::size_t depth, std::string& output, bool typed)
	{
		output.append("(function ");
		AppendSpan(decl->typeRef, text, output);
		output.append(" ");
		AppendSpan(decl->name, text, output);
		output.append(" (");
		for (std::size_t i{ 0 }; i < decl->params.count; ++i)
		{
			DumpDecl(decl->params.data[i], text, 0, output, typed);
			if (i + 1 < decl->params.count) output.append(" ");
		}
		output.append(")\n");
		DumpStmt(decl->body, text, depth + 1, output, typed);
		output.append(")");
	}

	void DumpParam(const AstDecl* decl, std::string_view text, std::string& output)
	{
		output.append("(param ");
		AppendSpan(decl->typeRef, text, output);
		output.append(" ");
		AppendSpan(decl->name, text, output);
		output.append(")");
	}

	void DumpDecl(const AstDecl* decl, std::string_view text, std::size_t depth, std::string& output, bool typed)
	{
		output.append(depth * 2, ' ');

		if (!decl)
		{
			output.append("(null)");
			return;
		}

		switch (decl->kind)
		{
		case DeclKind::Function:
			DumpFunction(decl, text, depth, output, typed);
			break;
		case DeclKind::Param:
			DumpParam(decl, text, output);
			break;
		}
	}
}

std::string DumpAstToString(const SourceMap& source, const AstProgram* program, bool typed)
{
	std::string_view text{ source.File(program->fileId).text };

	std::string output{};
	for (const AstDecl* decl : program->body)
	{
		DumpDecl(decl, text, 0, output, typed);
		output.append("\n");
	}

	return output;
}

void DumpAst(const SourceMap& source, const AstProgram* program, bool typed)
{
	std::string output{ DumpAstToString(source, program, typed) };
	std::fwrite(output.data(), 1, output.size(), stdout);
}