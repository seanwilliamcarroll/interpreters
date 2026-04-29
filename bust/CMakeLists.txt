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

# Embed the bust prelude source at configure time. The prelude is prepended
# to every user program so that compiler-required symbols (e.g. malloc) do
# not need to be declared by the user.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/std/prelude.bs PRELUDE_SOURCE)
configure_file(inc/prelude_source.hpp.in prelude_source.hpp @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/std/prelude.bs)

add_executable(bust-exe src/main.cpp)

set_target_properties(bust-exe PROPERTIES OUTPUT_NAME bust)

add_library(bust-lib STATIC
  src/bust.cpp
  src/frontend.cpp
  src/tokens.cpp
  hir/block_checker.cpp
  hir/expression_checker.cpp
  src/lexer.cpp
  src/parser.cpp
  hir/statement_checker.cpp
  hir/top_item_checker.cpp
  hir/type_arena.cpp
  src/codegen.cpp
  codegen/arena.cpp
  codegen/block_label.cpp
  codegen/closure_builder.cpp
  codegen/expression_generator.cpp
  codegen/formatter.cpp
  codegen/function_handle.cpp
  codegen/ir_builder.cpp
  codegen/statement_generator.cpp
  codegen/top_item_generator.cpp
  src/type_checker.cpp
  src/validate_main.cpp
  src/monomorpher.cpp
  mono/expression_substituter.cpp
  mono/let_binding_monomorpher.cpp
  mono/top_item_monomorpher.cpp
  mono/dot_expr_resolver.cpp
  src/zir_lowerer.cpp
  zir/arena.cpp
  zir/expression_lowerer.cpp
  zir/statement_lowerer.cpp
  zir/top_item_lowerer.cpp)

set_target_properties(bust-lib PROPERTIES OUTPUT_NAME bust)

target_include_directories(bust-lib PUBLIC inc . ${CMAKE_CURRENT_BINARY_DIR})

target_compile_options(bust-lib PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(bust-lib core)

target_compile_options(bust-exe PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(bust-exe bust-lib)

#*****************************************************************************

add_library(bust-testlib INTERFACE)

target_include_directories(bust-testlib INTERFACE test/inc)


add_executable(bust-test
  test/src/main.cpp
  test/src/lexer_test.cpp
  test/src/parser_test.cpp
  test/src/type_checker_test.cpp
  test/src/type_unifier_test.cpp
  test/src/codegen_expressions_test.cpp
  test/src/codegen_functions_test.cpp
  test/src/codegen_side_effects_test.cpp
  test/src/codegen_lambdas_test.cpp
  test/src/codegen_tuples_test.cpp
  test/src/name_mangler_test.cpp
  test/src/validate_main_test.cpp
  test/src/monomorpher_test.cpp
  test/src/zir_lowerer_test.cpp
  test/src/free_variable_collector_test.cpp)

target_compile_options(bust-test PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(bust-test
  bust-lib
  bust-testlib
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
add_test(NAME bust.type_unifier COMMAND bust-test -ts=bust.type_unifier)
add_test(NAME bust.type_checker COMMAND bust-test -ts=bust.type_checker)
add_test(NAME bust.name_mangler COMMAND bust-test -ts=bust.name_mangler)
add_test(NAME bust.monomorpher COMMAND bust-test -ts=bust.monomorpher)
add_test(NAME bust.zir_lowerer COMMAND bust-test -ts=bust.zir_lowerer)
add_test(NAME bust.free_variable_collector COMMAND bust-test -ts=bust.free_variable_collector)
add_test(NAME bust.codegen.expressions COMMAND bust-test -ts=bust.codegen.expressions)
add_test(NAME bust.codegen.functions COMMAND bust-test -ts=bust.codegen.functions)
add_test(NAME bust.codegen.side_effects COMMAND bust-test -ts=bust.codegen.side_effects)
add_test(NAME bust.codegen.lambdas COMMAND bust-test -ts=bust.codegen.lambdas)
add_test(NAME bust.codegen.tuples COMMAND bust-test -ts=bust.codegen.tuples)

#*****************************************************************************
