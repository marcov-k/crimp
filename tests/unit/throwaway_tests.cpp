#include "../framework/test.h"

TEST(temp, both_spaces)
{
	CHECK_TEXT_EQ("a ", "a ");
}

TEST(temp, actual_space)
{
	CHECK_TEXT_EQ("a", "a ");
}

TEST(temp, both_newlines)
{
	CHECK_TEXT_EQ("a\n", "a\n");
}

TEST(temp, both_double_newlines)
{
	CHECK_TEXT_EQ("a\n\nb", "a\n\nb");
}

TEST(temp, expected_newline)
{
	CHECK_TEXT_EQ("a\n", "a");
}

TEST(temp, actual_newline)
{
	CHECK_TEXT_EQ("a", "a\n");
}

TEST(temp, expected_carriage)
{
	CHECK_TEXT_EQ("a\r\nb", "a\nb");
}

TEST(temp, both_trailing_space)
{
	CHECK_TEXT_EQ("x\ny \nz", "x\ny \nz");
}

TEST(temp, both_empty)
{
	CHECK_TEXT_EQ("", "");
}

TEST(temp, expected_empty)
{
	CHECK_TEXT_EQ("", "x");
}