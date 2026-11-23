#include "text_diff.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	// Which side has the additional newline
	enum class NewlineDiff : std::uint8_t
	{
		None,
		Expected,
		Actual,
	};

	std::string StripText(std::string_view t)
	{
		std::string stripped{};
		stripped.reserve(t.size());

		for (char chara : t)
		{
			if (chara != '\r') stripped.push_back(chara);
		}

		return stripped;
	}

	std::vector<std::string> SplitLines(std::string_view t)
	{
		std::vector<std::string> lines{};

		std::string line{};
		for (char chara : t)
		{
			if (chara == '\n')
			{
				lines.push_back(line);
				line.clear();
			}
			else line.push_back(chara);
		}
		lines.push_back(line);

		return lines;
	}

	NewlineDiff DifferOnlyByFinalNewline(const std::string& expected, const std::string& actual)
	{
		if (expected.size() == actual.size() + 1 && expected.back() == '\n' && expected == actual + "\n") return NewlineDiff::Expected;
		if (actual.size() == expected.size() + 1 && actual.back() == '\n' && actual == expected + "\n") return NewlineDiff::Actual;
		return NewlineDiff::None;
	}

	std::size_t FindTrailingWhitespace(const std::vector<std::string>& lines)
	{
		for (std::size_t i{ 0 }; i < lines.size(); ++i)
		{
			if (lines[i].empty()) continue;
			if (lines[i].back() == ' ' || lines[i].back() == '\t') return i + 1;
		}

		return 0;
	}

	bool CheckLinesEqual(std::size_t line, const std::vector<std::string>& a, const std::vector<std::string>& b, std::string& message)
	{
		if (line >= a.size())
		{
			message += std::format("\n\t\tline {}: expected |<missing line>| actual |{}|", line + 1, b[line]);
			return false;
		}

		if (line >= b.size())
		{
			message += std::format("\n\t\tline {}: expected |{}| actual |<missing line>|", line + 1, a[line]);
			return false;
		}

		const std::string& lineA{ a[line] };
		const std::string& lineB{ b[line] };
		if (lineA != lineB)
		{
			message += std::format("\n\t\tline {}: expected |{}| actual |{}|", line + 1, lineA, lineB);
			return false;
		}

		return true;
	}
}

bool CheckTextsEqual(std::string_view expected, std::string_view actual, std::string& message)
{
	message.clear();

	std::string strippedExp{ StripText(expected) };
	std::string strippedAct{ StripText(actual) };

	std::vector<std::string> linesExp{ SplitLines(strippedExp) };
	std::vector<std::string> linesAct{ SplitLines(strippedAct) };

	std::size_t whitespaceExp{ FindTrailingWhitespace(linesExp) };
	if (whitespaceExp > 0)
	{
		message += std::format("\n\t\ttrailing whitespace in expected at line {}: |{}|", whitespaceExp, linesExp[whitespaceExp - 1]);
	}

	std::size_t whitespaceAct{ FindTrailingWhitespace(linesAct) };
	if (whitespaceAct > 0)
	{
		message += std::format("\n\t\ttrailing whitespace in actual at line {}: |{}|", whitespaceAct, linesAct[whitespaceAct - 1]);
	}

	if (strippedExp == strippedAct) return message.empty();

	switch (DifferOnlyByFinalNewline(strippedExp, strippedAct))
	{
	case NewlineDiff::None:
		break;
	case NewlineDiff::Expected:
		message += "\n\t\texpected ends with a newline, actual doesn't";
		return false;
	case NewlineDiff::Actual:
		message += "\n\t\tactual ends with a newline, expected doesn't";
		return false;
	}

	std::size_t maxLines{ std::max(linesExp.size(), linesAct.size()) };
	for (std::size_t i{ 0 }; i < maxLines; ++i)
	{
		if (!CheckLinesEqual(i, linesExp, linesAct, message)) return false;
	}

	return message.empty();
}