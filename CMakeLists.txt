cmake_minimum_required(VERSION 3.15)

enable_testing()

project("crimp" LANGUAGES CXX)

option(CRIMP_BUILD_TESTS "Build Test Suite" OFF)

file(GLOB_RECURSE CRIMP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM CRIMP_SOURCES "src/main.cpp")

# Core compiler library
add_library(crimp_core STATIC ${CRIMP_SOURCES})
target_include_directories(crimp_core PUBLIC src)
target_compile_features(crimp_core PUBLIC cxx_std_20)
set_target_properties(crimp_core PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_compile_options(crimp_core PRIVATE
	"$<$<CXX_COMPILER_ID:MSVC>:/W4;/utf-8;/permissive->"
	"$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>"
)

# Compiler executable
add_executable(crimp src/main.cpp)
target_link_libraries(crimp PRIVATE crimp_core)

# Test executable
if (CRIMP_BUILD_TESTS)
	file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
	add_executable(crimp_tests ${TEST_SOURCES})
	target_link_libraries(crimp_tests PRIVATE crimp_core)
	target_compile_definitions(crimp_tests PRIVATE
		CRIMP_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/golden"
	)
endif()

add_test(NAME crimp_all COMMAND crimp_tests)