#********** Copyright © 2023-2026 Sean Carroll. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake build script for the 'bust' executable.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/
#**            for more on the CMake language.
#**
#**
#*****************************************************************************

project(bust VERSION 0.1.0)

add_executable(bust-exe src/main.cpp)

set_target_properties(bust-exe PROPERTIES OUTPUT_NAME bust)

add_library(bust-lib STATIC
  src/bust.cpp
  src/tokens.cpp
  hir/block_checker.cpp
  hir/expression_checker.cpp
  hir/let_binding_checker.cpp
  src/lexer.cpp
  src/parser.cpp
  hir/statement_checker.cpp
  hir/top_item_checker.cpp
  hir/type_registry.cpp
  src/codegen.cpp
  codegen/expression_generator.cpp
  codegen/formatter.cpp
  codegen/let_binding_generator.cpp
  codegen/statement_generator.cpp
  codegen/top_item_generator.cpp
  src/type_checker.cpp
  src/validate_main.cpp
  src/monomorpher.cpp
  mono/expression_substituter.cpp
  mono/let_binding_monomorpher.cpp
  mono/top_item_monomorpher.cpp
  src/zonker.cpp
  zonk/expression_zonker.cpp
  zonk/let_binding_zonker.cpp
  zonk/statement_zonker.cpp
  zonk/top_item_zonker.cpp)

set_target_properties(bust-lib PROPERTIES OUTPUT_NAME bust)

target_include_directories(bust-lib PUBLIC inc .)

target_compile_options(bust-lib PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(bust-lib core)

target_compile_options(bust-exe PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(bust-exe bust-lib)

#*****************************************************************************

add_executable(bust-test
  test/src/main.cpp
  test/src/lexer_test.cpp
  test/src/parser_test.cpp
  test/src/type_checker_test.cpp
  test/src/type_unifier_test.cpp
  test/src/codegen_test.cpp
  test/src/name_mangler_test.cpp
  test/src/validate_main_test.cpp
  test/src/zonker_test.cpp)

target_compile_options(bust-test PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(bust-test
  bust-lib
  rapidcheck
  doctest::doctest)

# Locate `lli` (LLVM IR interpreter) for codegen execution tests.
# If found, its absolute path is baked into bust-test via BUST_LLI_PATH;
# otherwise BUST_LLI_PATH is undefined and those tests skip at runtime.
find_program(LLI_EXECUTABLE
  NAMES lli lli-mp-21 lli-mp-20 lli-mp-19 lli-mp-18
  DOC "Path to the LLVM IR interpreter (lli)")

if(LLI_EXECUTABLE)
  message(STATUS "Found lli: ${LLI_EXECUTABLE}")
  target_compile_definitions(bust-test PRIVATE
    BUST_LLI_PATH="${LLI_EXECUTABLE}")
else()
  message(STATUS "lli not found - codegen execution tests will be skipped")
endif()

#*****************************************************************************

if(ENABLE_ASAN)
  target_compile_options(bust-lib  PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(bust-lib     PRIVATE -fsanitize=address)
  target_compile_options(bust-exe  PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(bust-exe     PRIVATE -fsanitize=address)
  target_compile_options(bust-test PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(bust-test    PRIVATE -fsanitize=address)
endif()

#*****************************************************************************

add_test(NAME bust.lexer COMMAND bust-test -ts=bust.lexer)
add_test(NAME bust.parser COMMAND bust-test -ts=bust.parser)
add_test(NAME bust.semantic_analysis COMMAND bust-test -ts=bust.semantic_analysis)
add_test(NAME bust.type_checker COMMAND bust-test -ts=bust.type_checker)
add_test(NAME bust.type_unifier COMMAND bust-test -ts=bust.type_unifier)
add_test(NAME bust.codegen COMMAND bust-test -ts=bust.codegen)
add_test(NAME bust.zonker COMMAND bust-test -ts=bust.zonker)
add_test(NAME bust.name_mangler COMMAND bust-test -ts=bust.name_mangler)

#*****************************************************************************
