#pragma once

#include "../common/span.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct LineCol
{
	size_t line;
	size_t col;
};

struct SourceFile
{
	std::string path;
	std::string text;
	std::vector<size_t> lineStarts;
};

class SourceMap
{
public:
	FileId Add(std::string path, std::string text);

	const SourceFile& File(FileId id) const;

	LineCol Locate(FileId id, size_t offset) const;

	std::string_view LineText(FileId id, size_t line) const;

private:
	std::vector<SourceFile> _files;
};