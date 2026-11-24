#pragma once

#include "context.h"
#include "registry.h"
#include "string_contains.h"
#include "text_diff.h"
#include "to_text.h"

#include <format>
#include <string>
#include <string_view>

#define TEST(suite, name) \
	static void Test_##suite##_##name(); \
	static TestRegistrar Registrar_##suite##_##name{ #suite "/" #name, &Test_##suite##_##name }; \
	static void Test_##suite##_##name()

#define CHECK(...) \
	do \
	{ \
		if (!(__VA_ARGS__)) \
		{ \
			ReportFailure(__FILE__, __LINE__, #__VA_ARGS__); \
		} \
	} while (0)

#define REQUIRE(...) \
	do \
	{ \
		if (!(__VA_ARGS__)) \
		{ \
			ReportFailure(__FILE__, __LINE__, #__VA_ARGS__); \
			return; \
		} \
	} while (0)

#define CHECK_EQ(a, b) \
	do \
	{ \
		const auto& crimp_lhs_ = (a); \
		const auto& crimp_rhs_ = (b); \
		if (!(crimp_lhs_ == crimp_rhs_)) \
		{ \
			ReportEqualityFailure(__FILE__, __LINE__, #a, #b, ToText(crimp_lhs_), ToText(crimp_rhs_)); \
		} \
	} while (0)

#define CHECK_TEXT_EQ(expected, actual) \
	do \
	{ \
		std::string crimp_msg_{}; \
		if (!CheckTextsEqual((expected), (actual), crimp_msg_)) \
		{ \
			ReportFailure(__FILE__, __LINE__, crimp_msg_); \
		} \
	} while (0)

#define CHECK_CONTAINS(haystack, needle) \
	do \
	{ \
		std::string_view crimp_haystack_ = (haystack); \
		std::string_view crimp_needle_ = (needle); \
		if (!StringContains(crimp_haystack_, crimp_needle_)) \
		{ \
			ReportFailure(__FILE__, __LINE__, std::format("'{}' not found in {}: '{}'", crimp_needle_, #haystack, crimp_haystack_)); \
		} \
	} while (0)

#define CHECK_NOT_CONTAINS(haystack, needle) \
	do \
	{ \
		std::string_view crimp_haystack_ = (haystack); \
		std::string_view crimp_needle_ = (needle); \
		if (StringContains(crimp_haystack_, crimp_needle_)) \
		{ \
			ReportFailure(__FILE__, __LINE__, std::format("'{}' found in {}: '{}'", crimp_needle_, #haystack, crimp_haystack_)); \
		} \
	} while (0)

#define SKIP(reason) \
	do \
	{ \
		ReportSkip(reason); \
		return; \
	} while (0)