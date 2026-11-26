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

	# Selftests
	add_test(NAME crimp_selftest_failing_check COMMAND crimp_tests --test selftest/failing_check)
	set_tests_properties(crimp_selftest_failing_check PROPERTIES
		PASS_REGULAR_EXPRESSION [[1 failed(.|\n)*selftest/failing_check: Failed]]
		TIMEOUT 30
		LABELS "selftest"
	)

	add_test(NAME crimp_selftest_failing_assert COMMAND crimp_tests --test selftest/failing_assert)
	set_tests_properties(crimp_selftest_failing_assert PROPERTIES
		PASS_REGULAR_EXPRESSION [[Assertion failed: false]]
		TIMEOUT 30
		LABELS "selftest"
	)

	add_test(NAME crimp_selftest_invalid_parameter COMMAND crimp_tests --test selftest/invalid_parameter)
	set_tests_properties(crimp_selftest_invalid_parameter PROPERTIES
		PASS_REGULAR_EXPRESSION [[Invalid parameter in expression ']]
		TIMEOUT 30
		LABELS "selftest"
	)

	add_test(NAME crimp_selftest_abort_call COMMAND crimp_tests --test selftest/abort_call)
	set_tests_properties(crimp_selftest_abort_call PROPERTIES
		PASS_REGULAR_EXPRESSION [[abort\(\) has been called]]
		TIMEOUT 30
		LABELS "selftest"
	)

	add_test(NAME crimp_selftest_hang_busy COMMAND crimp_tests --test selftest/hang_busy --timeout 1)
	set_tests_properties(crimp_selftest_hang_busy PROPERTIES
		PASS_REGULAR_EXPRESSION [[TIMEOUT: selftest/hang_busy exceeded 1.000000s]]
		TIMEOUT 30
		LABELS "selftest"
	)

	add_test(NAME crimp_selftest_hang_blocked COMMAND crimp_tests --test selftest/hang_blocked --timeout 1)
	set_tests_properties(crimp_selftest_hang_blocked PROPERTIES
		PASS_REGULAR_EXPRESSION [[TIMEOUT: selftest/hang_blocked exceeded 1.000000s]]
		TIMEOUT 30
		LABELS "selftest"
	)

	add_test(NAME crimp_selftest_slow_but_ok COMMAND crimp_tests --test selftest/slow_but_ok --timeout 5)
	set_tests_properties(crimp_selftest_slow_but_ok PROPERTIES
		PASS_REGULAR_EXPRESSION [[1 succeeded]]
		TIMEOUT 30
		LABELS "selftest"
	)

	add_test(NAME crimp_selftest_no_timeout_flag COMMAND crimp_tests --test selftest/no_timeout_flag --timeout 1 --no-timeout)
	set_tests_properties(crimp_selftest_no_timeout_flag PROPERTIES
		PASS_REGULAR_EXPRESSION [[1 succeeded]]
		TIMEOUT 30
		LABELS "selftest"
	)

	if (CRIMP_ASAN)
		add_test(NAME crimp_selftest_null_dereference_asan COMMAND crimp_tests --test selftest/null_dereference)
		set_tests_properties(crimp_selftest_null_dereference_asan PROPERTIES
			PASS_REGULAR_EXPRESSION [[ERROR: AddressSanitizer: access-violation on unknown address]]
			TIMEOUT 30
			LABELS "selftest"
		)

		add_test(NAME crimp_selftest_heap_overflow_asan COMMAND crimp_tests --test selftest/heap_overflow)
		set_tests_properties(crimp_selftest_heap_overflow_asan PROPERTIES
			PASS_REGULAR_EXPRESSION [[ERROR: AddressSanitizer: heap-buffer-overflow on address]]
			TIMEOUT 30
			LABELS "selftest"
		)

		add_test(NAME crimp_selftest_division_by_zero_asan COMMAND crimp_tests --test selftest/division_by_zero)
		set_tests_properties(crimp_selftest_division_by_zero_asan PROPERTIES
			PASS_REGULAR_EXPRESSION [[ERROR: AddressSanitizer: int-divide-by-zero on unknown address]]
			TIMEOUT 30
			LABELS "selftest"
		)

		add_test(NAME crimp_selftest_stack_overflow_asan COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:crimp_tests> --test selftest/stack_overflow)
		set_tests_properties(crimp_selftest_stack_overflow_asan PROPERTIES
			PASS_REGULAR_EXPRESSION [[ERROR: AddressSanitizer: stack-overflow on address]]
			TIMEOUT 30
			LABELS "selftest"
		)
	else ()
		add_test(NAME crimp_selftest_null_dereference COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:crimp_tests> --test selftest/null_dereference)
		set_tests_properties(crimp_selftest_null_dereference PROPERTIES
			PASS_REGULAR_EXPRESSION [[Unhandled exception 0xC0000005 in test selftest/]]
			TIMEOUT 30
			LABELS "selftest"
		)

		add_test(NAME crimp_selftest_division_by_zero COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:crimp_tests> --test selftest/division_by_zero)
		set_tests_properties(crimp_selftest_division_by_zero PROPERTIES
			PASS_REGULAR_EXPRESSION [[Unhandled exception 0xC0000094 in test selftest/]]
			TIMEOUT 30
			LABELS "selftest"
		)

		add_test(NAME crimp_selftest_stack_overflow COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:crimp_tests> --test selftest/stack_overflow)
		set_tests_properties(crimp_selftest_stack_overflow PROPERTIES
			PASS_REGULAR_EXPRESSION [[Unhandled exception 0xC00000FD in test selftest/]]
			TIMEOUT 30
			LABELS "selftest"
		)
	endif()

	set(CRIMP_DISCOVERED_TESTS
		"${CMAKE_CURRENT_BINARY_DIR}/discovered_tests.cmake"
	)

	file(TOUCH "${CRIMP_DISCOVERED_TESTS}")

	add_custom_command(TARGET crimp_tests POST_BUILD
		COMMAND ${CMAKE_COMMAND}
			"-DTEST_EXE=$<TARGET_FILE:crimp_tests>"
			"-DOUT_FILE=${CRIMP_DISCOVERED_TESTS}"
			"-DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DiscoverTests.cmake"
		VERBATIM
		BYPRODUCTS "${CRIMP_DISCOVERED_TESTS}"
	)

	set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES
		"${CRIMP_DISCOVERED_TESTS}"
	)
endif()