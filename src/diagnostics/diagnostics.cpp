#include "diagnostics.h"

#include "source_map.h"
#include "../common/span.h"

#include <cstddef>
#include <string>
#include <vector>

namespace
{
	std::string ExpandTabs(std::string_view s)
	{
		std::string out;
		std::size_t col{ 0 };
		for (unsigned char c : s)
		{
			if (c == '\t')
			{
				do
				{
					out.push_back(' ');
					++col;
				} while (col % 4 != 0);
			}
			else
			{
				out.push_back(static_cast<char>(c));
				if ((c & 0xC0) != 0x80) ++col;
			}
		}

		return out;
	}

	std::size_t DisplayCol(std::string_view line, std::size_t byteOff)
	{
		std::size_t col{ 0 };
		for (std::size_t i{ 0 }; i < byteOff && i < line.size(); ++i)
		{
			unsigned char c{ static_cast<unsigned char>(line[i]) };
			if (c == '\t')
			{
				do
				{
					++col;
				} while (col % 4 != 0);
			}
			else if ((c & 0xC0) != 0x80) ++col;
		}
		return col;
	}

	const char* Label(Severity s)
	{
		switch (s)
		{
		case Severity::Error: return "error";
		case Severity::Warning: return "warning";
		case Severity::Note: return "note";
		default: return "error";
		}
	}

	std::string RenderOne(const SourceMap& sm, Span span, const char* lbl, const std::string& msg)
	{
		const SourceFile& f{ sm.File(span.file) };
		LineCol lc{ sm.Locate(span.file, span.begin) };

		std::string out{ f.path + ":" + std::to_string(lc.line) + ":" + std::to_string(lc.col) + ": " + lbl + ": " + msg + "\n" };

		std::string_view line{ sm.LineText(span.file, lc.line) };
		std::size_t lineBegin{ f.lineStarts[lc.line - 1] };
		std::size_t b{ span.begin - lineBegin };
		std::size_t e{ span.end > span.begin ? span.end - lineBegin : b + 1 };
		if (e > line.size()) e = line.size();

		std::size_t start{ DisplayCol(line, b) };
		std::size_t stop{ DisplayCol(line, e) };
		std::size_t width{ stop > start ? stop - start : 1 };

		std::string num{ std::to_string(lc.line) };
		std::string pad(num.size(), ' ');
		out += " " + num + " | " + ExpandTabs(line) + "\n";
		out += " " + pad + " | " + std::string(start, ' ') + "^" + std::string(width - 1, '~') + "\n";

		return out;
	}
}

Diagnostics::Diagnostics(std::size_t maxErrors) : _maxErrors(maxErrors) {}

void Diagnostics::Emit(Diagnostic d)
{
	if (d.severity == Severity::Error) ++_errors;
	_list.push_back(std::move(d));
}

void Diagnostics::Error(Span s, std::string msg)
{
	Emit({ Severity::Error, s, std::move(msg), {} });
}

void Diagnostics::Warning(Span s, std::string msg)
{
	Emit({ Severity::Warning, s, std::move(msg), {} });
}

bool Diagnostics::HasErrors() const
{
	return _errors > 0;
}

bool Diagnostics::LimitReached() const
{
	return _errors >= _maxErrors;
}

const std::vector<Diagnostic>& Diagnostics::All() const
{
	return _list;
}

std::string Render(const SourceMap& sm, const Diagnostic& d)
{
	std::string out{ RenderOne(sm, d.span, Label(d.severity), d.message) };
	for (const Note& n : d.notes)
	{
		out += RenderOne(sm, n.span, "note", n.message);
	}
	return out;
}