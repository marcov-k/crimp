#pragma once

#include "context.h"
#include "registry.h"
#include "text_diff.h"
#include "to_text.h"

#include <string>

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

#define SKIP(reason) \
	do \
	{ \
		ReportSkip(reason); \
		return; \
	} while (0)