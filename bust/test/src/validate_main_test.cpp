//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for bust::ValidateMain semantic pass
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <validate_main.hpp>

#include <doctest/doctest.h>
#include <sstream>

//****************************************************************************
namespace bust {
using namespace ast;
//****************************************************************************
TEST_SUITE("bust.semantic_analysis") {

  // --- Helpers -------------------------------------------------------------

  static Program parse_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return parser.parse();
  }

  // --- ValidateMain --------------------------------------------------------

  TEST_CASE("bust::validate_main_accepts_valid_program") {
    auto program = parse_string("fn main() -> i64 { 0 }");
    CHECK_NOTHROW(ValidateMain{}(std::move(program)));
  }

  TEST_CASE("bust::validate_main_accepts_with_other_functions") {
    auto program = parse_string("fn helper(x: i64) -> i64 { x }\n"
                                "fn main() -> i64 { helper(42) }");
    CHECK_NOTHROW(ValidateMain{}(std::move(program)));
  }

  TEST_CASE("bust::validate_main_rejects_missing_main") {
    auto program = parse_string("fn foo() -> i64 { 0 }");
    CHECK_THROWS_AS(ValidateMain{}(std::move(program)),
                    core::CompilerException);
  }

  TEST_CASE("bust::validate_main_rejects_main_returning_bool") {
    auto program = parse_string("fn main() -> bool { true }");
    CHECK_THROWS_AS(ValidateMain{}(std::move(program)),
                    core::CompilerException);
  }

  TEST_CASE("bust::validate_main_rejects_main_returning_unit") {
    auto program = parse_string("fn main() { 0 }");
    CHECK_THROWS_AS(ValidateMain{}(std::move(program)),
                    core::CompilerException);
  }

  TEST_CASE("bust::validate_main_rejects_duplicate_main") {
    auto program = parse_string("fn main() -> i64 { 0 }\n"
                                "fn main() -> i64 { 1 }");
    CHECK_THROWS_AS(ValidateMain{}(std::move(program)),
                    core::CompilerException);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
