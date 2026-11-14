#include "ast_dump.h"

#include "ast.h"
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

	void DumpExpr(const AstExpr* expr, std::string_view text, std::string& output)
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
			output.append("(int ");
			AppendSpan(expr->span, text, output);
			output.append(")");
			break;
		case ExprKind::StringLiteral:
			output.append("(string ");
			AppendSpan(expr->span, text, output);
			output.append(")");
			break;
		case ExprKind::Name:
			output.append("(name ");
			AppendSpan(expr->name, text, output);
			output.append(")");
			break;
		case ExprKind::Binary:
			output.append("(");
			output.append(GetSpelling(expr->op));
			output.append(" ");
			DumpExpr(expr->lhs, text, output);
			output.append(" ");
			DumpExpr(expr->rhs, text, output);
			output.append(")");
			break;
		case ExprKind::Call:
			output.append("(call ");
			AppendSpan(expr->name, text, output);
			for (const AstExpr* arg : expr->args)
			{
				output.append(" ");
				DumpExpr(arg, text, output);
			}
			output.append(")");
			break;
		}
	}

	void DumpStmt(const AstStmt* stmt, std::string_view text, std::size_t depth, std::string& output)
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
			output.append("(var ");
			AppendSpan(stmt->typeRef, text, output);
			output.append(" ");
			AppendSpan(stmt->name, text, output);
			output.append(" ");
			DumpExpr(stmt->expr, text, output);
			output.append(")");
			break;
		case StmtKind::Return:
			output.append("(return ");
			DumpExpr(stmt->expr, text, output);
			output.append(")");
			break;
		case StmtKind::ExprStmt:
			output.append("(expr ");
			DumpExpr(stmt->expr, text, output);
			output.append(")");
			break;
		case StmtKind::Block:
		{
			output.append("(block");
			for (const AstStmt* child : stmt->stmts)
			{
				output.append("\n");
				DumpStmt(child, text, depth + 1, output);
			}
			output.append(")");
			break;
		}
		}
	}

	void DumpDecl(const AstDecl* decl, std::string_view text, std::size_t depth, std::string& output)
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
			output.append("(function ");
			AppendSpan(decl->typeRef, text, output);
			output.append(" ");
			AppendSpan(decl->name, text, output);
			output.append(" (");
			for (std::size_t i{ 0 }; i < decl->params.count; ++i)
			{
				DumpDecl(decl->params.data[i], text, 0, output);
				if (i + 1 < decl->params.count) output.append(" ");
			}
			output.append(")\n");
			DumpStmt(decl->body, text, depth + 1, output);
			output.append(")");
			break;
		case DeclKind::Param:
			output.append("(param ");
			AppendSpan(decl->typeRef, text, output);
			output.append(" ");
			AppendSpan(decl->name, text, output);
			output.append(")");
			break;
		}
	}
}

std::string DumpAstToString(const SourceMap& source, const AstProgram& program)
{
	std::string_view text{ source.File(program.fileId).text };

	std::string output{};
	for (const AstDecl* decl : program.body)
	{
		DumpDecl(decl, text, 0, output);
		output.append("\n");
	}

	return output;
}

void DumpAst(const SourceMap& source, const AstProgram& program)
{
	std::string output{ DumpAstToString(source, program) };
	std::fwrite(output.data(), 1, output.size(), stdout);
}