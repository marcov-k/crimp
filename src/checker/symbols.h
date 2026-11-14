#pragma once

#include "types.h"
#include "../common/list.h"
#include "../common/span.h"

#include <cstdint>
#include <string_view>
#include <type_traits>

enum class SymbolKind : std::uint8_t
{
	Function,
	Param,
	Local,
};

struct AstDecl;
struct AstStmt;

struct Symbol
{
	SymbolKind kind{ SymbolKind::Function };
	std::string_view name; // can only point into text in the arena
	const Type* type{ nullptr }; // Variable
	Span declSpan; // empty for builtin
	const AstDecl* declNode{ nullptr }; // Function/Param (not builtin)
	const AstStmt* stmtNode{ nullptr }; // Local (not builtin)
	List<const Type*> paramTypes; // Function
	const Type* returnType{ nullptr }; // Function
	bool isVariadic{ false }; // Function
	std::string_view linkName; // Function
	bool isBuiltin{ false }; // Function
};
static_assert(std::is_trivially_destructible_v<Symbol>);