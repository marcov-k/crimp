#pragma once

#include "escape.h"

#include <concepts>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>

template<typename T> requires (std::integral<T> && !std::same_as<T, bool>)
std::string ToText(T value)
{
	return std::format("{}", value);
}

template<typename T> requires std::same_as<T, bool>
std::string ToText(T value)
{
	return value ? "true" : "false";
}

inline std::string ToText(std::string_view value)
{
	return std::format("\"{}\"", EscapeString(value));
}

template<typename T> requires std::is_enum_v<T>
std::string ToText(T value)
{
	return std::format("{}", static_cast<std::size_t>(value));
}