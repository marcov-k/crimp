#include "source_map.h"

#include "../common/span.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

FileId SourceMap::Add(std::string path, std::string text)
{
	if (text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0)
	{
		text.erase(0, 3);
	}

	SourceFile f;
	f.path = std::move(path);
	f.text = std::move(text);
	f.lineStarts.push_back(0);

	for (std::size_t i{ 0 }; i < f.text.size(); ++i)
	{
		if (f.text[i] == '\n') f.lineStarts.push_back(i + 1);
	}

	_files.push_back(std::move(f));

	return _files.size() - 1;
}

const SourceFile& SourceMap::File(FileId id) const
{
	return _files[id];
}

LineCol SourceMap::Locate(FileId id, std::size_t offset) const
{
	const auto& ls{ _files[id].lineStarts };
	auto it{ std::upper_bound(ls.begin(), ls.end(), offset) };
	std::size_t idx{ static_cast<std::size_t>(it - ls.begin()) - 1 };
	return { idx + 1, offset - ls[idx] + 1 };
}

std::string_view SourceMap::LineText(FileId id, std::size_t line) const
{
	const SourceFile& f{ _files[id] };
	std::size_t b{ f.lineStarts[line - 1] };
	std::size_t e{ line < f.lineStarts.size() ? f.lineStarts[line] - 1 : f.text.size() };
	if (e > b && f.text[e - 1] == '\r') --e;
	return std::string_view{ f.text }.substr(b, e - b);
}