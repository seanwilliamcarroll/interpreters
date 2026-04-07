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
  src/bust_tokens.cpp
  hir/block_checker.cpp
  hir/expression_checker.cpp
  hir/let_binding_checker.cpp
  src/lexer.cpp
  src/parser.cpp
  hir/statement_checker.cpp
  hir/top_item_checker.cpp
  eval/expression_evaluator.cpp
  eval/let_binding_evaluator.cpp
  eval/statement_evaluator.cpp
  eval/top_item_evaluator.cpp
  src/evaluator.cpp
  src/type_checker.cpp
  src/validate_main.cpp)

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
  test/src/evaluator_test.cpp
  test/src/validate_main_test.cpp)

target_compile_options(bust-test PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(bust-test
  bust-lib
  rapidcheck
  doctest::doctest)

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
add_test(NAME bust.evaluator COMMAND bust-test -ts=bust.evaluator)

#*****************************************************************************
