#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

using FileId = size_t;

struct Span
{
	FileId file{ 0 };
	size_t begin{ 0 };
	size_t end{ 0 };
};

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