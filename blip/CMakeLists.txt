#********** Copyright © 2023 Sean Carroll, Jonathon Bell. All rights reserved.
#**
#**
#**  Version : $Header:$
#**
#**
#**  Purpose : CMake build script for the 'blip' executable.
#**
#**
#**  See Also: https://cmake.org/cmake/help/latest/
#**            for more on the CMake language.
#**
#**
#*****************************************************************************

project(blip VERSION 0.1.0)

add_executable(blip-exe src/main.cpp)

set_target_properties(blip-exe PROPERTIES OUTPUT_NAME blip)

add_library(blip-lib STATIC
  src/blip.cpp
  src/blip_tokens.cpp
  src/lexer.cpp
  src/parser.cpp
  src/value.cpp
  src/environment.cpp
  src/evaluator.cpp
  src/type.cpp
  src/type_checker.cpp)

set_target_properties(blip-lib PROPERTIES OUTPUT_NAME blip)

target_include_directories(blip-lib PUBLIC inc)

target_compile_options(blip-lib PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(blip-lib core)

target_compile_options(blip-exe PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(blip-exe blip-lib)

#*****************************************************************************

add_executable(blip-test
  test/src/main.cpp
  test/src/token_test.cpp
  test/src/lexer_test.cpp
  test/src/parser_test.cpp
  test/src/ast_printer_test.cpp
  test/src/environment_test.cpp
  test/src/evaluator_test.cpp)

target_compile_options(blip-test PRIVATE ${PROJECT_WARNINGS})

target_link_libraries(blip-test
  blip-lib
  rapidcheck
  doctest::doctest)

#*****************************************************************************

add_test(NAME blip.token COMMAND blip-test -ts=blip.token)
add_test(NAME blip.lexer COMMAND blip-test -ts=blip.lexer)
add_test(NAME blip.parser COMMAND blip-test -ts=blip.parser)
add_test(NAME blip.ast_printer COMMAND blip-test -ts=blip.ast_printer)
add_test(NAME blip.environment COMMAND blip-test -ts=blip.environment)
add_test(NAME blip.evaluator COMMAND blip-test -ts=blip.evaluator)
add_test(NAME blip.builtins COMMAND blip-test -ts=blip.builtins)

#****************************************************************************
