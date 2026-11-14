#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

enum class TypeKind : std::uint8_t
{
	Error,
	Void,
	Int,
	String,
};

struct Type
{
	TypeKind kind{ TypeKind::Error };
	std::size_t size{ 0 };
	bool isSigned{ false };
	std::string_view name;
};
static_assert(std::is_trivially_destructible_v<Type>);

inline const Type* ErrorType()
{
	static const Type errorType{ TypeKind::Error, 0, false, "" };

	return &errorType;
}

inline const Type* VoidType()
{
	static const Type voidType{ TypeKind::Void, 0, false, "void" };

	return &voidType;
}

inline const Type* IntType()
{
	static const Type intType{ TypeKind::Int, 4, true, "int" };

	return &intType;
}

inline const Type* StringType()
{
	static const Type stringType{ TypeKind::String, 8, false, "string" };

	return &stringType;
}

inline bool IsError(const Type* t)
{
	assert(t != nullptr);

	return t->kind == TypeKind::Error;
}

inline bool IsCompatible(const Type* from, const Type* to)
{
	assert(from != nullptr && to != nullptr);

	return IsError(from) || IsError(to) || from == to;
}