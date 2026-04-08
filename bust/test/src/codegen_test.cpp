//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for bust::CodeGen
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <codegen.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <type_checker.hpp>

#include <cstdlib>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.codegen") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program type_check(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    auto program = parser.parse();
    TypeChecker checker;
    return checker(program);
  }

  static std::string codegen(const std::string &source) {
    auto program = type_check(source);
    CodeGen gen;
    return gen(program);
  }

#ifdef BUST_LLI_PATH
  // Writes `ir` to a temp .ll file, runs `lli` on it, returns the exit code.
  static int run_via_lli(const std::string &ir) {
    auto path = std::filesystem::temp_directory_path() /
                ("bust_codegen_" + std::to_string(::getpid()) + ".ll");
    {
      std::ofstream out(path);
      out << ir;
    }
    std::string cmd = std::string(BUST_LLI_PATH) + " " + path.string();
    int status = std::system(cmd.c_str());
    std::filesystem::remove(path);
    if (WIFEXITED(status)) {
      return WEXITSTATUS(status);
    }
    return -1;
  }

// Runs `source` and checks it exits with `expected`. On failure, doctest's
// INFO machinery dumps the source and the generated IR, so you can see
// exactly what lli was handed without rerunning anything by hand.
#define CHECK_RUN(source, expected)                                            \
  do {                                                                         \
    const std::string _src = (source);                                         \
    const std::string _ir = codegen(_src);                                     \
    INFO("source: " << _src);                                                  \
    INFO("generated IR:\n" << _ir);                                            \
    CHECK(run_via_lli(_ir) == (expected));                                     \
  } while (0)
#endif

  // --- Tests ---------------------------------------------------------------

  TEST_CASE("stub emits a main that returns zero") {
    auto ir = codegen("fn main() -> i64 { 0 }");
    CHECK(ir.find("define i64 @main()") != std::string::npos);
    CHECK(ir.find("ret i64 0") != std::string::npos);
  }

#ifdef BUST_LLI_PATH
  TEST_CASE("stub program runs and exits with zero") {
    CHECK_RUN("fn main() -> i64 { 0 }", 0);
  }

  TEST_CASE("integer literal in main is returned") {
    CHECK_RUN("fn main() -> i64 { 42 }", 42);
    CHECK_RUN("fn main() -> i64 { 7 }", 7);
  }

  TEST_CASE("let binding to integer literal is returned") {
    CHECK_RUN("fn main() -> i64 { let x = 42; x }", 42);
    CHECK_RUN("fn main() -> i64 { let y = 7; y }", 7);
  }

  TEST_CASE("multiple let bindings, last one returned") {
    CHECK_RUN("fn main() -> i64 { let x = 1; let y = 2; y }", 2);
    CHECK_RUN("fn main() -> i64 { let x = 1; let y = 2; x }", 1);
  }

  // --- Binary expressions --------------------------------------------------
  //
  // NOTE: lli returns main's result as the process exit code, which on
  // Unix is truncated to the low 8 bits. Keep all expected values in
  // [0, 255] so WEXITSTATUS round-trips cleanly.

  TEST_CASE("binary addition of literals") {
    CHECK_RUN("fn main() -> i64 { 20 + 22 }", 42);
    CHECK_RUN("fn main() -> i64 { 0 + 0 }", 0);
    CHECK_RUN("fn main() -> i64 { 1 + 2 }", 3);
  }

  TEST_CASE("binary subtraction of literals") {
    CHECK_RUN("fn main() -> i64 { 50 - 8 }", 42);
    CHECK_RUN("fn main() -> i64 { 10 - 10 }", 0);
  }

  TEST_CASE("binary multiplication of literals") {
    CHECK_RUN("fn main() -> i64 { 6 * 7 }", 42);
    CHECK_RUN("fn main() -> i64 { 0 * 99 }", 0);
    CHECK_RUN("fn main() -> i64 { 1 * 42 }", 42);
  }

  TEST_CASE("binary division of literals (signed, truncating)") {
    CHECK_RUN("fn main() -> i64 { 84 / 2 }", 42);
    CHECK_RUN("fn main() -> i64 { 7 / 2 }", 3); // truncates toward zero
    CHECK_RUN("fn main() -> i64 { 100 / 10 }", 10);
  }

  TEST_CASE("binary modulus of literals") {
    CHECK_RUN("fn main() -> i64 { 17 % 5 }", 2);
    CHECK_RUN("fn main() -> i64 { 100 % 7 }", 2);
    CHECK_RUN("fn main() -> i64 { 10 % 2 }", 0);
  }

  TEST_CASE("operator precedence: mul/div bind tighter than add/sub") {
    CHECK_RUN("fn main() -> i64 { 2 + 3 * 4 }", 14);
    CHECK_RUN("fn main() -> i64 { 3 * 4 + 2 }", 14);
    CHECK_RUN("fn main() -> i64 { 20 - 6 / 2 }", 17);
    CHECK_RUN("fn main() -> i64 { 1 + 2 * 3 + 4 }", 11);
  }

  TEST_CASE("parentheses override precedence") {
    CHECK_RUN("fn main() -> i64 { (2 + 3) * 4 }", 20);
    CHECK_RUN("fn main() -> i64 { 2 * (3 + 4) }", 14);
    CHECK_RUN("fn main() -> i64 { (1 + 2) * (3 + 4) }", 21);
  }

  TEST_CASE("left associativity of same-precedence operators") {
    // 10 - 3 - 2 must parse as (10 - 3) - 2 = 5, not 10 - (3 - 2) = 9
    CHECK_RUN("fn main() -> i64 { 10 - 3 - 2 }", 5);
    // 20 / 4 / 2 must parse as (20 / 4) / 2 = 2, not 20 / (4 / 2) = 10
    CHECK_RUN("fn main() -> i64 { 20 / 4 / 2 }", 2);
  }

  TEST_CASE("binary ops on let-bound identifiers") {
    CHECK_RUN("fn main() -> i64 { let x = 20; let y = 22; x + y }", 42);
    CHECK_RUN("fn main() -> i64 { let a = 6; let b = 7; a * b }", 42);
    CHECK_RUN("fn main() -> i64 { let x = 100; x - 58 }", 42);
  }

  TEST_CASE("same identifier used multiple times in expression") {
    // Exercises "every identifier read emits its own load" — the two
    // reads of x must be independent.
    CHECK_RUN("fn main() -> i64 { let x = 21; x + x }", 42);
    CHECK_RUN("fn main() -> i64 { let x = 7; x * x }", 49);
  }

  TEST_CASE("deeply nested binary expressions") {
    CHECK_RUN("fn main() -> i64 { ((1 + 2) + (3 + 4)) + ((5 + 6) + (7 + 8)) }",
              36);
    CHECK_RUN("fn main() -> i64 { 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 }", 36);
  }

  TEST_CASE("binary expression as let RHS") {
    CHECK_RUN("fn main() -> i64 { let x = 2 + 3; let y = x * 4; y + 2 }", 22);
    CHECK_RUN("fn main() -> i64 { let x = 10 - 5; let y = x + x; y }", 10);
  }

  // --- If expressions ------------------------------------------------------

  TEST_CASE("if with literal true picks then branch") {
    CHECK_RUN("fn main() -> i64 { if true { 1 } else { 2 } }", 1);
    CHECK_RUN("fn main() -> i64 { if true { 42 } else { 0 } }", 42);
  }

  TEST_CASE("if with literal false picks else branch") {
    CHECK_RUN("fn main() -> i64 { if false { 1 } else { 2 } }", 2);
    CHECK_RUN("fn main() -> i64 { if false { 0 } else { 42 } }", 42);
  }

  TEST_CASE("if reading a let-bound bool condition") {
    CHECK_RUN("fn main() -> i64 { let b = true; if b { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { let b = false; if b { 0 } else { 42 } }", 42);
  }

  TEST_CASE("if branches can read let-bound i64s") {
    CHECK_RUN("fn main() -> i64 { let x = 40; if true { x + 2 } else { 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { let x = 40; if false { 0 } else { x + 2 } }",
              42);
  }

  TEST_CASE("if branches can contain multiple statements") {
    CHECK_RUN("fn main() -> i64 { if true { let a = 20; let b = 22; a + b } "
              "else { 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { if false { 0 } else { let a = 6; let b = 7; "
              "a * b } }",
              42);
  }

  TEST_CASE("if as let RHS") {
    CHECK_RUN("fn main() -> i64 { let x = if true { 20 } else { 0 }; x + 22 }",
              42);
    CHECK_RUN("fn main() -> i64 { let x = if false { 0 } else { 21 }; x + x }",
              42);
  }

  TEST_CASE("arithmetic on if results") {
    CHECK_RUN("fn main() -> i64 { (if true { 6 } else { 0 }) * 7 }", 42);
    CHECK_RUN("fn main() -> i64 { (if false { 0 } else { 5 }) + (if true { 37 "
              "} else { 0 }) }",
              42);
  }

  TEST_CASE("if nested in then branch") {
    CHECK_RUN(
        "fn main() -> i64 { if true { if true { 42 } else { 8 } } else { 9 } }",
        42);
    CHECK_RUN("fn main() -> i64 { if true { if false { 8 } else { 42 } } else "
              "{ 9 } }",
              42);
  }

  TEST_CASE("if nested in else branch") {
    CHECK_RUN("fn main() -> i64 { if false { 1 } else { if true { 42 } else { "
              "6 } } }",
              42);
    CHECK_RUN("fn main() -> i64 { if false { 1 } else { if false { 6 } else { "
              "42 } } }",
              42);
  }

  TEST_CASE("else-if chain via nested ifs") {
    CHECK_RUN("fn main() -> i64 { if false { 1 } else { if false { 2 } else { "
              "if true { 42 } else { 99 } } } }",
              42);
    CHECK_RUN("fn main() -> i64 { if false { 1 } else { if true { 42 } else { "
              "if true { 2 } else { 3 } } } }",
              42);
  }

  TEST_CASE("if condition computed from a binary expression on bools") {
    // Only valid if logical and/or are wired through; harmless if not — skip
    // by leaving commented. Uncomment when && / || are supported in codegen.
    // CHECK_RUN("fn main() -> i64 { if true && false { 0 } else { 42 } }", 42);
  }

  TEST_CASE("deeply nested if-as-expression in arithmetic") {
    CHECK_RUN("fn main() -> i64 { let x = if true { if false { 0 } else { 6 } "
              "} else { 0 }; x * 7 }",
              42);
  }
#else
  TEST_CASE("codegen execution tests" * doctest::skip()) {
    MESSAGE("lli not found at configure time - execution tests skipped");
  }
#endif
}

//****************************************************************************
} // namespace bust
//****************************************************************************
