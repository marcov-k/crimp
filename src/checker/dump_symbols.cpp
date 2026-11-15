#include "dump_symbols.h"

#include "symbols.h"
#include "types.h"
#include "../ast/ast.h"

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <string>

namespace
{
	void DumpTypeName(const Type* type, std::string& output)
	{
		assert(type);

		if (!type) output.append("<null>");
		else if (type->kind == TypeKind::Error) output.append("<error>");
		else output.append(type->name);
	}

	void DumpStatement(const AstStmt* stmt, std::string& output);

	void DumpVarDecl(const AstStmt* stmt, std::string& output)
	{
		assert(stmt);

		if (!stmt) return;

		output.append("  local ");

		if (!stmt->symbol)
		{
			output.append("<unresolved>\n");
			return;
		}

		output.append(stmt->symbol->name);
		output.append(" : ");

		DumpTypeName(stmt->symbol->type, output);

		output.append("\n");
	}

	void DumpBlock(const AstStmt* stmt, std::string& output)
	{
		assert(stmt);
		
		if (!stmt) return;

		for (const AstStmt* child : stmt->stmts)
		{
			DumpStatement(child, output);
		}
	}

	void DumpStatement(const AstStmt* stmt, std::string& output)
	{
		assert(stmt);

		if (!stmt) return;

		switch (stmt->kind)
		{
		case StmtKind::Error:
			break;
		case StmtKind::VarDecl:
			DumpVarDecl(stmt, output);
			break;
		case StmtKind::Return:
			break;
		case StmtKind::ExprStmt:
			break;
		case StmtKind::Block:
			DumpBlock(stmt, output);
			break;
		}
	}

	void DumpParam(const AstDecl* decl, std::string& output)
	{
		assert(decl);

		if (!decl) return;

		output.append("  param ");

		if (!decl->symbol)
		{
			output.append("<unresolved>\n");
			return;
		}

		output.append(decl->symbol->name);
		output.append(" : ");
		
		DumpTypeName(decl->symbol->type, output);

		output.append("\n");
	}

	void DumpFunction(const AstDecl* decl, std::string& output)
	{
		assert(decl);

		if (!decl) return;

		output.append("function ");

		if (!decl->symbol)
		{
			output.append("<unresolved>\n");
			return;
		}

		output.append(decl->symbol->name);

		output.append(" : (");
		for (std::size_t i{ 0 }; i < decl->symbol->paramTypes.count; ++i)
		{
			DumpTypeName(decl->symbol->paramTypes.data[i], output);
			if (i + 1 < decl->symbol->paramTypes.count) output.append(", ");
		}
		if (decl->symbol->isVariadic)
		{
			if (decl->symbol->paramTypes.count > 0) output.append(", ");
			output.append("...");
		}
		output.append(") -> ");
		DumpTypeName(decl->symbol->returnType, output);

		output.append("  [");
		output.append(decl->symbol->linkName);
		output.append("]\n");

		for (const AstDecl* param : decl->params)
		{
			DumpParam(param, output);
		}

		DumpStatement(decl->body, output);
	}
}

std::string DumpSymbolsToString(const AstProgram* program)
{
	std::string output{};

	for (const AstDecl* func : program->body)
	{
		DumpFunction(func, output);
	}

	return output;
}

void DumpSymbols(const AstProgram* program)
{
	std::string output{ DumpSymbolsToString(program) };
	std::fwrite(output.data(), 1, output.size(), stdout);
}