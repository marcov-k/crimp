cmake_minimum_required(VERSION 3.15)

project("crimp" LANGUAGES CXX)
enable_testing()

option(CRIMP_BUILD_TESTS "Build Test Suite" OFF)
option(CRIMP_ASAN "Build with Address Sanitizer" OFF)
option(CRIMP_WARNINGS_AS_ERRORS "Treat Warnings as Errors" OFF)

file(GLOB_RECURSE CRIMP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER CRIMP_SOURCES EXCLUDE REGEX "/src/main\\.cpp$")

function (crimp_warnings target)
	target_compile_options(${target} PRIVATE
		"$<$<CXX_COMPILER_ID:MSVC>:/W4;/utf-8;/permissive-;/Zc:preprocessor>"
		"$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>"
	)
	if(CRIMP_WARNINGS_AS_ERRORS)
		target_compile_options(${target} PRIVATE
			"$<$<CXX_COMPILER_ID:MSVC>:/WX>"
			"$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Werror>"
		)
	endif()
endfunction()

function (crimp_sanitizers target)
	if (CRIMP_ASAN AND MSVC)
		target_compile_options(${target} PRIVATE /fsanitize=address)
	endif()
endfunction()

function (crimp_asan target)
	if (CRIMP_ASAN AND MSVC)
		get_filename_component(CRIMP_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
		file(GLOB CRIMP_ASAN_DLLS "${CRIMP_COMPILER_DIR}/clang_rt.asan_dynamic-*.dll")
		foreach (dll ${CRIMP_ASAN_DLLS})
			add_custom_command(TARGET ${target} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${target}>"
			)
		endforeach()
	endif()
endfunction()

if (CRIMP_ASAN AND MSVC)
	string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
	add_link_options(/INCREMENTAL:NO)
endif()

# Core compiler library
add_library(crimp_core STATIC ${CRIMP_SOURCES})
target_include_directories(crimp_core PUBLIC src)
target_compile_features(crimp_core PUBLIC cxx_std_20)
set_target_properties(crimp_core PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
crimp_warnings(crimp_core)
crimp_sanitizers(crimp_core)

# Compiler executable
add_executable(crimp src/main.cpp)
target_link_libraries(crimp PRIVATE crimp_core)
crimp_warnings(crimp)
crimp_sanitizers(crimp)
crimp_asan(crimp)

# Test executable
if (CRIMP_BUILD_TESTS)
	file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
	add_executable(crimp_tests ${TEST_SOURCES})
	target_link_libraries(crimp_tests PRIVATE crimp_core)
	target_compile_definitions(crimp_tests PRIVATE
		CRIMP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/golden"
	)
	target_include_directories(crimp_tests PRIVATE tests)
	crimp_warnings(crimp_tests)
	crimp_sanitizers(crimp_tests)
	crimp_asan(crimp_tests)

	add_test(NAME crimp_all COMMAND crimp_tests)
	set_tests_properties(crimp_all PROPERTIES
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)

	# Selftests
	add_test(NAME crimp_selftest_failing_check COMMAND crimp_tests --filter selftest/failing_check)
	set_tests_properties(crimp_selftest_failing_check PROPERTIES
		PASS_REGULAR_EXPRESSION [[: false]]
		TIMEOUT 30
	)

	add_test(NAME crimp_selftest_failing_assert COMMAND crimp_tests --filter selftest/failing_assert)
	set_tests_properties(crimp_selftest_failing_assert PROPERTIES
		PASS_REGULAR_EXPRESSION [[Assertion failed: false]]
		TIMEOUT 30
	)

	add_test(NAME crimp_selftest_invalid_parameter COMMAND crimp_tests --filter selftest/invalid_parameter)
	set_tests_properties(crimp_selftest_invalid_parameter PROPERTIES
		PASS_REGULAR_EXPRESSION [[Invalid parameter in expression ']]
		TIMEOUT 30
	)

	add_test(NAME crimp_selftest_abort_call COMMAND crimp_tests --filter selftest/abort_call)
	set_tests_properties(crimp_selftest_abort_call PROPERTIES
		PASS_REGULAR_EXPRESSION [[abort\(\) has been called]]
		TIMEOUT 30
	)

	if (CRIMP_ASAN)
		add_test(NAME crimp_selftest_null_dereference COMMAND crimp_tests --filter selftest/null_dereference)
		set_tests_properties(crimp_selftest_null_dereference PROPERTIES
			PASS_REGULAR_EXPRESSION [[ERROR: AddressSanitizer: access-violation on unknown address]]
			TIMEOUT 30
		)

		add_test(NAME crimp_selftest_heap_overflow COMMAND crimp_tests --filter selftest/heap_overflow)
		set_tests_properties(crimp_selftest_heap_overflow PROPERTIES
			PASS_REGULAR_EXPRESSION [[ERROR: AddressSanitizer: heap-buffer-overflow on address]]
			TIMEOUT 30
		)
	endif()
endif()