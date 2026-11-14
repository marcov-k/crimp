#pragma once

#include "../common/list.h"
#include "../common/span.h"
#include "../lexer/tokens.h"

#include <cstddef>
#include <cstdint>

enum class ExprKind : std::uint8_t
{
	Error,
	IntLiteral,
	StringLiteral,
	Name,
	Binary,
	Call,
};

enum class StmtKind : std::uint8_t
{
	Error,
	VarDecl,
	Return,
	ExprStmt,
	Block,
};

enum class DeclKind : std::uint8_t
{
	Function,
	Param,
};

struct Type;
struct Symbol;

struct AstExpr;
struct AstStmt;
struct AstDecl;

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
	TokenKind op{ TokenKind::Eof };
	AstExpr* lhs{ nullptr };
	AstExpr* rhs{ nullptr };
	List<AstExpr*> args{};
	const Type* type{ nullptr };
	Symbol* symbol{ nullptr };
};

struct AstStmt
{
	StmtKind kind{ StmtKind::Error };
	Span span{};
	Span typeRef{};
	Span name{};
	AstExpr* expr{ nullptr }; // initializer, return value, or expression statement
	List<AstStmt*> stmts{};
	Symbol* symbol{ nullptr };
};

struct AstDecl
{
	DeclKind kind{ DeclKind::Function };
	Span span{};
	Span typeRef{};
	Span name{};
	List<AstDecl*> params{};
	AstStmt* body{ nullptr }; // parameter has no body
	Symbol* symbol{ nullptr };
};