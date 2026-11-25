#include "../framework/text_diff.h"

#include "../framework/test.h"

#include <cstddef>
#include <string>

// Line-number and missing-line behavior

TEST(text_diff, empty_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("", "", message));

    CHECK(message.empty());
}

TEST(text_diff, empty_expected)
{
    std::string message{};

    CHECK(!CheckTextsEqual("", "x", message));

    CHECK_EQ(message, "\n\t\tline 1: expected |<missing line>| actual |x|");
}

TEST(text_diff, two_newlines_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\n\nb", "a\n\nb", message));

    CHECK(message.empty());
}

TEST(text_diff, three_lines_third_line_mismatch)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a\nb\nc", "a\nb\nX", message));

	CHECK_EQ(message, "\n\t\tline 3: expected |c| actual |X|");
}

TEST(text_diff, three_lines_second_line_mismatch)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a\nb\nc", "a\nX\nc", message));

	CHECK_EQ(message, "\n\t\tline 2: expected |b| actual |X|");
}

TEST(text_diff, extra_line_in_expected)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a\nb", "a", message));

	CHECK_EQ(message, "\n\t\tline 2: expected |b| actual |<missing line>|");
}

TEST(text_diff, extra_line_in_actual)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a", "a\nb", message));

	CHECK_EQ(message, "\n\t\tline 2: expected |<missing line>| actual |b|");
}

TEST(text_diff, two_extra_lines_in_expected)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a\nb\nc", "a", message));

	CHECK_EQ(message, "\n\t\tline 2: expected |b| actual |<missing line>|");
}

TEST(text_diff, two_lines_first_line_mismatch)
{
	std::string message{};

	CHECK(!CheckTextsEqual("\nb", "a\nb", message));

	CHECK_EQ(message, "\n\t\tline 1: expected || actual |a|");
}

TEST(text_diff, long_line_last_char_mismatch)
{
	std::string message{};

	CHECK(!CheckTextsEqual("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab",
		message));

	CHECK_EQ(message, "\n\t\tline 1: expected |aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa| actual |aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab|");
}

// Trailing whitespace

TEST(text_diff, trailing_space_in_expected_and_actual)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a ", "a ", message));

    CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |a |");
    CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |a |");
}

TEST(text_diff, trailing_space_in_actual)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a", "a ", message));

    CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |a |");
    CHECK_CONTAINS(message, "line 1: expected |a| actual |a |");
}

TEST(text_diff, three_lines_second_line_trailing_space_in_expected_and_actual)
{
    std::string message{};

    CHECK(!CheckTextsEqual("x\ny \nz", "x\ny \nz", message));

    CHECK_CONTAINS(message, "trailing whitespace in expected at line 2: |y |");
    CHECK_CONTAINS(message, "trailing whitespace in actual at line 2: |y |");
}

TEST(text_diff, two_lines_first_line_trailing_space_in_expected_and_actual)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a \nb", "a \nb", message));

	CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |a |");
	CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |a |");
}

TEST(text_diff, two_lines_first_line_trailing_space_in_expected)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a \nb", "a\nb", message));

	CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |a |");
	CHECK_CONTAINS(message, "line 1: expected |a | actual |a|");
}

TEST(text_diff, two_lines_first_line_trailing_space_in_actual)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a\nb", "a \nb", message));

	CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |a |");
	CHECK_CONTAINS(message, "line 1: expected |a| actual |a |");
}

TEST(text_diff, one_line_trailing_tab_in_actual)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a", "a\t", message));

	CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |a\t|");
	CHECK_CONTAINS(message, "line 1: expected |a| actual |a\t|");
}

TEST(text_diff, two_lines_second_line_trailing_tab_in_actual)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a\nb", "a\nb\t", message));

	CHECK_CONTAINS(message, "trailing whitespace in actual at line 2: |b\t|");
	CHECK_CONTAINS(message, "line 2: expected |b| actual |b\t|");
}

TEST(text_diff, three_lines_second_line_only_space_in_expected_and_actual)
{
	std::string message{};

	CHECK(!CheckTextsEqual("a\n \nb", "a\n \nb", message));

	CHECK_CONTAINS(message, "trailing whitespace in expected at line 2: | |");
	CHECK_CONTAINS(message, "trailing whitespace in actual at line 2: | |");
}

TEST(text_diff, two_trailing_spaces_in_expected_and_actual)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a  ", "a  ", message));

    CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |a  |");
    CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |a  |");
}

TEST(text_diff, two_lines_with_trailing_spaces_in_expected_and_actual)
{
    std::string message{};
    
    CHECK(!CheckTextsEqual("x \ny \nz", "x \ny \nz", message));

    CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |x |");
    CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |x |");

    CHECK_NOT_CONTAINS(message, "line 2:");
}

TEST(text_diff, final_newline_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\n", "a\n", message));

    CHECK(message.empty());
}

TEST(text_diff, final_newline_in_expected)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a\n", "a", message));

    CHECK_EQ(message, "\n\t\texpected ends with a newline, actual doesn't");
}

TEST(text_diff, final_newline_in_actual)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a", "a\n", message));

    CHECK_EQ(message, "\n\t\tactual ends with a newline, expected doesn't");
}

TEST(text_diff, two_lines_final_newline_in_actual)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a\nb", "a\nb\n", message));

    CHECK_EQ(message, "\n\t\tactual ends with a newline, expected doesn't");
}

TEST(text_diff, two_final_newlines_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\n\n", "a\n\n", message));

    CHECK(message.empty());
}

TEST(text_diff, carriage_return_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\r\n", "a\r\n", message));

    CHECK(message.empty());
}

TEST(text_diff, trailing_space_and_carriage_return_in_expected)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a \r\nb", "a\nb", message));

    CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |a |");
    CHECK_CONTAINS(message, "line 1: expected |a | actual |a|");
}

// Combinations

TEST(text_diff, trailing_space_in_expected_and_actual_and_final_newline_in_expected)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a \n", "a ", message));

    CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |a |");
    CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |a |");
    CHECK_CONTAINS(message, "expected ends with a newline, actual doesn't");
}

TEST(text_diff, final_newline_in_expected_and_actual_and_trailing_space_in_actual_and_mismatch)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a\n", "b \n", message));

    CHECK_CONTAINS(message, "trailing whitespace in actual at line 1: |b |");
    CHECK_CONTAINS(message, "line 1: expected |a| actual |b |");
}

TEST(text_diff, trailing_space_in_expected_and_mismatch)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a ", "b", message));

    CHECK_CONTAINS(message, "trailing whitespace in expected at line 1: |a |");
    CHECK_CONTAINS(message, "line 1: expected |a | actual |b|");
}

// State and reuse

TEST(text_diff, fail_message_reused_for_match)
{
    std::string message{};
    CheckTextsEqual("a", "b", message);

    CHECK(CheckTextsEqual("a", "a", message));

    CHECK(message.empty());
}

TEST(text_diff, fail_message_reused_for_same_check)
{
    std::string message{};
    CheckTextsEqual("a", "b", message);

    CHECK(!CheckTextsEqual("a", "b", message));

    CHECK_EQ(message, "\n\t\tline 1: expected |a| actual |b|");
}

TEST(text_diff, prefilled_message_for_match)
{
    std::string message{ "stale" };

    CHECK(CheckTextsEqual("a", "a", message));

    CHECK(message.empty());
}

// Carriage return handling

TEST(text_diff, carriage_return_in_expected)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\r\nb", "a\nb", message));

    CHECK(message.empty());
}

TEST(text_diff, middle_carriage_return_in_expected)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\rb", "ab", message));

    CHECK(message.empty());
}

TEST(text_diff, trailing_carriage_return_in_expected)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\r", "a", message));

    CHECK(message.empty());
}

TEST(text_diff, carriage_return_in_expected_and_newline_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("\r\n", "\n", message));

    CHECK(message.empty());
}

TEST(text_diff, two_carriage_returns_in_expected)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\r\n\r\n", "a\n\n", message));

    CHECK(message.empty());
}

TEST(text_diff, carriage_return_in_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("a\n", "a\r\n", message));

    CHECK(message.empty());
}

// Non-ASCII and special characters

TEST(text_diff, e_with_diacritic_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("é", "é", message));

    CHECK(message.empty());
}

TEST(text_diff, e_with_diacritic_in_expected)
{
    std::string message{};

    CHECK(!CheckTextsEqual("é", "e", message));

    CHECK_EQ(message, "\n\t\tline 1: expected |é| actual |e|");
}

TEST(text_diff, null_byte_in_expected_and_actual)
{
    std::string message{};

    std::string nullString("a\0b", 3);
    CHECK(CheckTextsEqual(nullString, nullString, message));

    CHECK(message.empty());
}

TEST(text_diff, null_byte_in_expected_and_actual_and_mismatch)
{
    std::string message{};

    std::string expected("a\0b", 3);
    std::string actual("a\0c", 3);
    CHECK(!CheckTextsEqual(expected, actual, message));

    CHECK_EQ(message, std::string("\n\t\tline 1: expected |a\0b| actual |a\0c|", 38));
}

TEST(text_diff, curly_braces_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("{}", "{}", message));

    CHECK(message.empty());
}

TEST(text_diff, curly_braces_in_expected_and_actual_and_mismatch)
{
    std::string message{};

    CHECK(!CheckTextsEqual("a{}", "{}b", message));

    CHECK_EQ(message, "\n\t\tline 1: expected |a{}| actual |{}b|");
}

// Scale and boundaries

TEST(text_diff, only_newlines_in_expected_and_actual)
{
    std::string message{};

    CHECK(CheckTextsEqual("\n\n\n\n", "\n\n\n\n", message));

    CHECK(message.empty());
}

TEST(text_diff, final_newline_in_empty_expected_and_empty_actual)
{
    std::string message{};

    CHECK(!CheckTextsEqual("\n", "", message));

    CHECK_EQ(message, "\n\t\texpected ends with a newline, actual doesn't");
}

TEST(text_diff, ten_thousand_lines_matching)
{
    std::string message{};

    const char* line{ "a\n" };
    std::string lines{};
    lines.reserve(20'000);
    for (std::size_t i{ 0 }; i < 10'000; ++i)
    {
        lines += line;
    }

    CHECK(CheckTextsEqual(lines, lines, message));

    CHECK(message.empty());
}

TEST(text_diff, ten_thousand_lines_last_line_mismatch)
{
    std::string message{};

    const char* line{ "a\n" };
    std::string expected{}, actual{};
    expected.reserve(20'000);
    actual.reserve(20'000);

    for (std::size_t i{ 0 }; i < 9'999; ++i)
    {
        expected += line;
        actual += line;
    }
    expected += line;
    actual += "b\n";

    CHECK(!CheckTextsEqual(expected, actual, message));

    CHECK_EQ(message, "\n\t\tline 10000: expected |a| actual |b|");
}

TEST(text_diff, one_hundred_thousand_characters_last_mismatch)
{
    std::string message{};

    constexpr char chara{ 'a' };
    std::string expected{}, actual{};
    expected.reserve(100'000);
    actual.reserve(100'000);

    for (std::size_t i{ 0 }; i < 99'999; ++i)
    {
        expected += chara;
        actual += chara;
    }
    expected += chara;
    actual += "b";

    CHECK(!CheckTextsEqual(expected, actual, message));

    CHECK(message.starts_with("\n\t\tline 1: expected |"));
    CHECK(message.ends_with("b|"));
}