#pragma once

#include "../common/span.h"
#include "../lexer/tokens.h"

#include <cstddef>

enum class ExprKind
{
	Error,
	IntLiteral,
	StringLiteral,
	Name,
	Binary,
	Call,
};

enum class StmtKind
{
	Error,
	VarDecl,
	Return,
	ExprStmt,
	Block,
};

enum class DeclKind
{
	Function,
	Param,
};

struct AstExpr;
struct AstStmt;
struct AstDecl;

template<typename T>
struct List
{
	T* data{ nullptr };
	std::size_t count{ 0 };

	T* begin() const
	{
		return data;
	}

	T* end() const
	{
		return data + count;
	}
};

struct AstProgram
{
	FileId fileId{};
	List<AstDecl*> body{};
};

struct AstExpr
{
	ExprKind kind{ ExprKind::Error };
	Span span{};
	Span name{};
	TokenType op{ TokenType::Eof };
	AstExpr* lhs{ nullptr };
	AstExpr* rhs{ nullptr };
	List<AstExpr*> args{};
};

struct AstStmt
{
	StmtKind kind{ StmtKind::Error };
	Span span{};
	Span typeRef{};
	Span name{};
	AstExpr* expr{ nullptr }; // initializer, return value, or expression statement
	List<AstStmt*> stmts{};
};

struct AstDecl
{
	DeclKind kind{ DeclKind::Function };
	Span span{};
	Span typeRef{};
	Span name{};
	List<AstDecl*> params{};
	AstStmt* body{ nullptr }; // parameter has no body
};