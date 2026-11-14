#include "names.h"

#include "../common/arena.h"
#include "../common/span.h"
#include "../diagnostics/source_map.h"

#include <cassert>
#include <cstddef>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

Names::Names(Arena& arena) : _arena(arena) {}

NameId Names::Intern(std::string_view text)
{
	auto found{ _ids.find(text) };
	if (found != _ids.end())
	{
		return found->second;
	}

	std::string_view stored{ _arena.CopyArray<char>(std::span<const char>{ text.data(), text.size() }), text.size() };
	NameId id{ _texts.size() };

	_ids[stored] = id;
	_texts.push_back(stored);

	return id;
}

NameId Names::Intern(const Span& span, const SourceMap& source)
{
	auto& text{ source.File(span.file).text };

	assert(span.begin <= span.end && span.end <= text.size());

	return Intern(text.substr(span.begin, span.end - span.begin));
}

std::string_view Names::Text(NameId id) const
{
	assert(id < _texts.size());

	return _texts[id];
}