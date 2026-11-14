#pragma once

#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <vector>

class Arena;
struct Span;
class SourceMap;

using NameId = std::size_t;

class Names
{
public:
	explicit Names(Arena& arena); // Names instance cannot outlive Arena instance

	Names(const Names&) = delete;
	Names& operator=(const Names&) = delete;
	Names(Names&&) = delete;
	Names& operator=(Names&&) = delete;

	NameId Intern(std::string_view text);
	NameId Intern(const Span& span, const SourceMap& source);
	
	std::string_view Text(NameId id) const;

private:
	Arena& _arena;
	std::unordered_map<std::string_view, NameId> _ids{};
	std::vector<std::string_view> _texts{};
};