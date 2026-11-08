# CMakeList.txt : CMake project for crimp, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.15)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("crimp")

# Add source to this project's executable.
add_executable (crimp "src/main.cpp"  "src/platform/utf8_wide.h" "src/platform/utf8_wide.cpp" "src/platform/win_error.h" "src/platform/win_error.cpp" "src/platform/scoped_handle.h" "src/platform/scoped_handle.cpp" "src/platform/process.h" "src/platform/process.cpp" "src/platform/file_io.h" "src/platform/file_io.cpp" "src/driver/driver.h" "src/driver/driver.cpp" "src/lexer/lexer.h" "src/lexer/lexer.cpp" "src/compile_flags.h" "src/platform/startup.h" "src/platform/startup.cpp" "src/diagnostics/source_map.h" "src/diagnostics/source_map.cpp" "src/diagnostics/diagnostics.h" "src/diagnostics/diagnostics.cpp" "src/common/utils.h" "src/common/utils.cpp" "src/lexer/tokens.h" "src/lexer/tokens.cpp" "src/common/span.h" "src/parser/parser.h" "src/parser/parser.cpp" "src/ast/ast.h" "src/common/arena.h" "src/common/arena.cpp" "src/ast/ast_dump.cpp" "src/ast/ast_dump.h" "src/ast/ast_dump_tests.h" "src/ast/ast_dump_tests.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET crimp PROPERTY CXX_STANDARD 20)
endif()

target_compile_options(crimp PRIVATE
	"$<$<CXX_COMPILER_ID:MSVC>:/W4;/utf-8;/permissive->"
	"$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Wpedantic>"
)

# TODO: Add tests and install targets if needed.
