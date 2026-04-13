//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
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

  // --- Comparison operators ------------------------------------------------
  //
  // These all produce i1 (bool). The most direct way to observe a bool from
  // `main` is to feed it into an `if` and return distinct i64s from each
  // branch.

  TEST_CASE("equality on integer literals") {
    CHECK_RUN("fn main() -> i64 { if 1 == 1 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 1 == 2 { 0 } else { 42 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 0 == 0 { 42 } else { 0 } }", 42);
  }

  TEST_CASE("inequality on integer literals") {
    CHECK_RUN("fn main() -> i64 { if 1 != 2 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 5 != 5 { 0 } else { 42 } }", 42);
  }

  TEST_CASE("less-than on integer literals") {
    CHECK_RUN("fn main() -> i64 { if 1 < 2 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 2 < 1 { 0 } else { 42 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 5 < 5 { 0 } else { 42 } }", 42);
  }

  TEST_CASE("less-than-or-equal on integer literals") {
    CHECK_RUN("fn main() -> i64 { if 1 <= 2 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 5 <= 5 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 6 <= 5 { 0 } else { 42 } }", 42);
  }

  TEST_CASE("greater-than on integer literals") {
    CHECK_RUN("fn main() -> i64 { if 2 > 1 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 1 > 2 { 0 } else { 42 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 5 > 5 { 0 } else { 42 } }", 42);
  }

  TEST_CASE("greater-than-or-equal on integer literals") {
    CHECK_RUN("fn main() -> i64 { if 2 >= 1 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 5 >= 5 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 4 >= 5 { 0 } else { 42 } }", 42);
  }

  TEST_CASE("signed comparison: negative vs positive") {
    // If signed (icmp slt) is wired correctly, -1 < 1. If unsigned (ult)
    // were used by mistake, -1 would be a huge unsigned value and the test
    // would fail.
    CHECK_RUN("fn main() -> i64 { if -1 < 1 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if -5 < -1 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if -1 > -5 { 42 } else { 0 } }", 42);
  }

  TEST_CASE("comparison on let-bound identifiers") {
    CHECK_RUN("fn main() -> i64 { let x = 10; let y = 20; if x < y { 42 } else "
              "{ 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { let x = 10; let y = 10; if x == y { 42 } "
              "else { 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { let x = 10; if x != 0 { 42 } else { 0 } }",
              42);
  }

  TEST_CASE("comparison on arithmetic results") {
    CHECK_RUN("fn main() -> i64 { if 1 + 1 == 2 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 3 * 4 > 10 { 42 } else { 0 } }", 42);
    CHECK_RUN(
        "fn main() -> i64 { let x = 21; if x * 2 == 42 { 42 } else { 0 } }",
        42);
  }

  TEST_CASE("comparison result bound to a let") {
    CHECK_RUN("fn main() -> i64 { let b = 1 < 2; if b { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { let b = 5 == 6; if b { 0 } else { 42 } }",
              42);
  }

  TEST_CASE("comparison drives if-as-expression in arithmetic") {
    CHECK_RUN("fn main() -> i64 { (if 3 < 5 { 6 } else { 0 }) * 7 }", 42);
    CHECK_RUN("fn main() -> i64 { let x = 10; let y = if x >= 10 { 42 } else "
              "{ 0 }; y }",
              42);
  }

  TEST_CASE("else-if chain driven by comparisons") {
    CHECK_RUN("fn main() -> i64 { let x = 3; if x == 1 { 1 } else { if x == 2 "
              "{ 2 } else { if x == 3 { 42 } else { 99 } } } }",
              42);
    CHECK_RUN("fn main() -> i64 { let x = 100; if x < 10 { 1 } else { if x < "
              "50 { 2 } else { 42 } } }",
              42);
  }

  TEST_CASE("equality and inequality on bools") {
    CHECK_RUN("fn main() -> i64 { if true == true { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if true == false { 0 } else { 42 } }", 42);
    CHECK_RUN("fn main() -> i64 { if false != true { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if false != false { 0 } else { 42 } }", 42);
    CHECK_RUN(
        "fn main() -> i64 { let b = (1 < 2) == true; if b { 42 } else { 0 } }",
        42);
    CHECK_RUN("fn main() -> i64 { let x = 5; if (x == 5) != false { 42 } else "
              "{ 0 } }",
              42);
  }

  // --- Unary operators -----------------------------------------------------
  //
  // Avoid returning a negative i64 directly from main: WEXITSTATUS truncates
  // to the low 8 bits and the interpretation of the resulting byte varies.
  // Instead, compose unary back into a positive value before returning.

  TEST_CASE("unary negation of integer literal composed back to positive") {
    // -(-42) == 42
    CHECK_RUN("fn main() -> i64 { -(-42) }", 42);
    // 100 + (-58) == 42
    CHECK_RUN("fn main() -> i64 { 100 + -58 }", 42);
    // 0 - (-42) == 42
    CHECK_RUN("fn main() -> i64 { 0 - -42 }", 42);
  }

  TEST_CASE("unary negation of identifier") {
    CHECK_RUN("fn main() -> i64 { let x = 42; -(-x) }", 42);
    CHECK_RUN("fn main() -> i64 { let x = 8; let y = 50; y + -x }", 42);
  }

  TEST_CASE("unary negation observed via comparison") {
    // Lets us see negative values without returning them.
    CHECK_RUN("fn main() -> i64 { if -1 < 0 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { let x = 5; if -x < 0 { 42 } else { 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { if -5 == 0 - 5 { 42 } else { 0 } }", 42);
  }

  TEST_CASE("double negation is identity") {
    CHECK_RUN(
        "fn main() -> i64 { let x = 42; if -(-x) == x { 42 } else { 0 } }", 42);
  }

  TEST_CASE("logical not of bool literal") {
    CHECK_RUN("fn main() -> i64 { if !false { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if !true { 0 } else { 42 } }", 42);
  }

  TEST_CASE("logical not of let-bound bool") {
    CHECK_RUN("fn main() -> i64 { let b = false; if !b { 42 } else { 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { let b = true; if !b { 0 } else { 42 } }", 42);
  }

  TEST_CASE("logical not of comparison result") {
    CHECK_RUN("fn main() -> i64 { if !(1 == 2) { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if !(5 < 3) { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { let x = 10; if !(x < 5) { 42 } else { 0 } }",
              42);
  }

  TEST_CASE("double logical not is identity") {
    CHECK_RUN("fn main() -> i64 { if !(!true) { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if !(!false) { 0 } else { 42 } }", 42);
  }

  TEST_CASE("not bound to a let") {
    CHECK_RUN("fn main() -> i64 { let b = !false; if b { 42 } else { 0 } }",
              42);
  }

  // --- Logical && and || ---------------------------------------------------
  //
  // Note: bust has no observable side effects yet, so these tests can't
  // *prove* short-circuit evaluation is happening (vs. eager bitwise and/or).
  // They verify the value semantics. Short-circuit *behavior* will need to
  // be tested once side effects (println, panics, etc.) exist.

  TEST_CASE("logical and: truth table") {
    CHECK_RUN("fn main() -> i64 { if true && true { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if true && false { 0 } else { 42 } }", 42);
    CHECK_RUN("fn main() -> i64 { if false && true { 0 } else { 42 } }", 42);
    CHECK_RUN("fn main() -> i64 { if false && false { 0 } else { 42 } }", 42);
  }

  TEST_CASE("logical or: truth table") {
    CHECK_RUN("fn main() -> i64 { if true || true { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if true || false { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if false || true { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if false || false { 0 } else { 42 } }", 42);
  }

  TEST_CASE("&& and || on let-bound bools") {
    CHECK_RUN("fn main() -> i64 { let a = true; let b = false; if a && b { 0 } "
              "else { 42 } }",
              42);
    CHECK_RUN("fn main() -> i64 { let a = true; let b = false; if a || b { 42 "
              "} else { 0 } }",
              42);
  }

  TEST_CASE("&& and || over comparison results") {
    CHECK_RUN("fn main() -> i64 { let x = 5; if x > 0 && x < 10 { 42 } else { "
              "0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { let x = 50; if x < 10 || x > 40 { 42 } else "
              "{ 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { let x = 5; if x < 0 || x > 10 { 0 } else { "
              "42 } }",
              42);
  }

  TEST_CASE("|| binds looser than &&") {
    // a || b && c parses as a || (b && c)
    // false || (true && true) == true
    CHECK_RUN("fn main() -> i64 { if false || true && true { 42 } else { 0 } }",
              42);
    // true || (false && X) == true (X = false here)
    CHECK_RUN(
        "fn main() -> i64 { if true || false && false { 42 } else { 0 } }", 42);
    // false || (false && true) == false
    CHECK_RUN(
        "fn main() -> i64 { if false || false && true { 0 } else { 42 } }", 42);
  }

  TEST_CASE("&& binds looser than comparison") {
    // 1 < 2 && 3 < 4 parses as (1 < 2) && (3 < 4)
    CHECK_RUN("fn main() -> i64 { if 1 < 2 && 3 < 4 { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if 1 < 2 && 4 < 3 { 0 } else { 42 } }", 42);
  }

  TEST_CASE("&& and || with unary not") {
    CHECK_RUN("fn main() -> i64 { if !false && true { 42 } else { 0 } }", 42);
    CHECK_RUN("fn main() -> i64 { if !true || !false { 42 } else { 0 } }", 42);
    CHECK_RUN(
        "fn main() -> i64 { let b = true; if !(b && false) { 42 } else { 0 } }",
        42);
  }

  TEST_CASE("chained && (left-associative)") {
    CHECK_RUN("fn main() -> i64 { if true && true && true { 42 } else { 0 } }",
              42);
    CHECK_RUN("fn main() -> i64 { if true && true && false { 0 } else { 42 } }",
              42);
    CHECK_RUN("fn main() -> i64 { if true && false && true { 0 } else { 42 } }",
              42);
    CHECK_RUN("fn main() -> i64 { if false && true && true { 0 } else { 42 } }",
              42);
  }

  TEST_CASE("chained || (left-associative)") {
    CHECK_RUN(
        "fn main() -> i64 { if false || false || true { 42 } else { 0 } }", 42);
    CHECK_RUN(
        "fn main() -> i64 { if false || false || false { 0 } else { 42 } }",
        42);
    CHECK_RUN(
        "fn main() -> i64 { if true || false || false { 42 } else { 0 } }", 42);
  }

  TEST_CASE("&& and || result bound to a let") {
    CHECK_RUN(
        "fn main() -> i64 { let b = true && true; if b { 42 } else { 0 } }",
        42);
    CHECK_RUN(
        "fn main() -> i64 { let b = false || true; if b { 42 } else { 0 } }",
        42);
    CHECK_RUN("fn main() -> i64 { let x = 5; let in_range = x > 0 && x < 10; "
              "if in_range { 42 } else { 0 } }",
              42);
  }

  TEST_CASE("&& and || result feeds into another && or ||") {
    // (true && false) || true == true
    CHECK_RUN("fn main() -> i64 { if (true && false) || true { 42 } else { 0 "
              "} }",
              42);
    // (false || true) && (true || false) == true
    CHECK_RUN("fn main() -> i64 { if (false || true) && (true || false) { 42 "
              "} else { 0 } }",
              42);
    // (true && true) && (false || false) == false
    CHECK_RUN("fn main() -> i64 { if (true && true) && (false || false) { 0 } "
              "else { 42 } }",
              42);
  }

  TEST_CASE("&& and || drive an if-as-expression") {
    CHECK_RUN("fn main() -> i64 { let x = if true && true { 42 } else { 0 }; "
              "x }",
              42);
    CHECK_RUN("fn main() -> i64 { (if false || true { 6 } else { 0 }) * 7 }",
              42);
  }

  TEST_CASE("deeply nested if-as-expression in arithmetic") {
    CHECK_RUN("fn main() -> i64 { let x = if true { if false { 0 } else { 6 } "
              "} else { 0 }; x * 7 }",
              42);
  }
  // --- Function calls and recursion ----------------------------------------
  //
  // Once HIR/codegen support multi-function programs, calls, and parameters,
  // these tests should pass. Until then they will fail to compile or run --
  // that's the signal driving the work.

  TEST_CASE("call to a zero-arg function returning a constant") {
    CHECK_RUN("fn answer() -> i64 { 42 } "
              "fn main() -> i64 { answer() }",
              42);
  }

  TEST_CASE("call to a single-arg identity function") {
    CHECK_RUN("fn id(x: i64) -> i64 { x } "
              "fn main() -> i64 { id(7) }",
              7);
  }

  TEST_CASE("call with arithmetic on the parameter") {
    CHECK_RUN("fn inc(x: i64) -> i64 { x + 1 } "
              "fn main() -> i64 { inc(41) }",
              42);
  }

  TEST_CASE("multi-argument function") {
    CHECK_RUN("fn add(x: i64, y: i64) -> i64 { x + y } "
              "fn main() -> i64 { add(20, 22) }",
              42);
    CHECK_RUN("fn sub(x: i64, y: i64) -> i64 { x - y } "
              "fn main() -> i64 { sub(50, 8) }",
              42);
  }

  TEST_CASE("argument order is preserved (non-commutative op)") {
    // If we accidentally swapped arg slots, sub(10, 3) would yield -7
    // and the exit code would be 249 (256 - 7), not 7.
    CHECK_RUN("fn sub(x: i64, y: i64) -> i64 { x - y } "
              "fn main() -> i64 { sub(10, 3) }",
              7);
  }

  TEST_CASE("call result fed into arithmetic in caller") {
    CHECK_RUN("fn double(x: i64) -> i64 { x * 2 } "
              "fn main() -> i64 { double(20) + 2 }",
              42);
  }

  TEST_CASE("call result bound to a let") {
    CHECK_RUN("fn forty_two() -> i64 { 42 } "
              "fn main() -> i64 { let x = forty_two(); x }",
              42);
  }

  TEST_CASE("call as the argument to another call (composition)") {
    CHECK_RUN("fn inc(x: i64) -> i64 { x + 1 } "
              "fn main() -> i64 { inc(inc(inc(39))) }",
              42);
  }

  TEST_CASE("function calling another function (non-recursive)") {
    CHECK_RUN("fn inner(x: i64) -> i64 { x + 1 } "
              "fn outer(x: i64) -> i64 { inner(x) * 2 } "
              "fn main() -> i64 { outer(20) }",
              42);
  }

  TEST_CASE("bool-returning function") {
    // Exit code: true=1, false=0
    CHECK_RUN("fn is_positive(x: i64) -> bool { x > 0 } "
              "fn main() -> i64 { if is_positive(5) { 1 } else { 0 } }",
              1);
    CHECK_RUN("fn is_positive(x: i64) -> bool { x > 0 } "
              "fn main() -> i64 { if is_positive(-5) { 1 } else { 0 } }",
              0);
  }

  TEST_CASE("bool-taking function") {
    CHECK_RUN("fn pick(b: bool, x: i64, y: i64) -> i64 "
              "{ if b { x } else { y } } "
              "fn main() -> i64 { pick(true, 42, 0) }",
              42);
  }

  TEST_CASE("recursion: factorial") {
    // 5! = 120
    CHECK_RUN("fn fact(n: i64) -> i64 "
              "{ if n == 0 { 1 } else { n * fact(n - 1) } } "
              "fn main() -> i64 { fact(5) }",
              120);
  }

  TEST_CASE("recursion: sum from 1 to n") {
    // 1+2+...+9 = 45
    CHECK_RUN("fn sum(n: i64) -> i64 "
              "{ if n == 0 { 0 } else { n + sum(n - 1) } } "
              "fn main() -> i64 { sum(9) }",
              45);
  }

  TEST_CASE("recursion: fibonacci") {
    // fib(10) = 55
    CHECK_RUN("fn fib(n: i64) -> i64 "
              "{ if n < 2 { n } else { fib(n - 1) + fib(n - 2) } } "
              "fn main() -> i64 { fib(10) }",
              55);
  }

  TEST_CASE("mutual recursion: is_even / is_odd") {
    CHECK_RUN("fn is_even(n: i64) -> bool "
              "{ if n == 0 { true } else { is_odd(n - 1) } } "
              "fn is_odd(n: i64) -> bool "
              "{ if n == 0 { false } else { is_even(n - 1) } } "
              "fn main() -> i64 { if is_even(10) { 1 } else { 0 } }",
              1);
  }

  TEST_CASE("parameter shadowed by inner let") {
    CHECK_RUN("fn f(x: i64) -> i64 { let x = x + 1; x * 2 } "
              "fn main() -> i64 { f(20) }",
              42);
  }

  TEST_CASE("call inside an if-as-expression") {
    CHECK_RUN("fn yes() -> i64 { 42 } "
              "fn no() -> i64 { 0 } "
              "fn main() -> i64 { if true { yes() } else { no() } }",
              42);
  }

  TEST_CASE("call inside a logical short-circuit") {
    // Verifies args are evaluated in caller, then result feeds into &&.
    CHECK_RUN("fn is_pos(x: i64) -> bool { x > 0 } "
              "fn is_small(x: i64) -> bool { x < 10 } "
              "fn main() -> i64 "
              "{ if is_pos(5) && is_small(5) { 1 } else { 0 } }",
              1);
  }

  // --- Char literals --------------------------------------------------------

  TEST_CASE("char literal cast to i64") {
    CHECK_RUN("fn main() -> i64 { 'A' as i64 }", 65);
    CHECK_RUN("fn main() -> i64 { '0' as i64 }", 48);
    CHECK_RUN("fn main() -> i64 { ' ' as i64 }", 32);
  }

  TEST_CASE("char literal escape sequences") {
    CHECK_RUN("fn main() -> i64 { '\\n' as i64 }", 10);
    CHECK_RUN("fn main() -> i64 { '\\t' as i64 }", 9);
    CHECK_RUN("fn main() -> i64 { '\\0' as i64 }", 0);
  }

  TEST_CASE("char literal hex escape") {
    // '\x41' == 'A' == 65
    CHECK_RUN("fn main() -> i64 { '\\x41' as i64 }", 65);
    // '\x61' == 'a' == 97
    CHECK_RUN("fn main() -> i64 { '\\x61' as i64 }", 97);
  }

  TEST_CASE("char literal in let binding") {
    CHECK_RUN("fn main() -> i64 { let c: char = 'Z'; c as i64 }", 90);
  }

  // --- Cast expressions (sext / trunc / zext) --------------------------------
  //
  // The codegen emits sext for signed widening, trunc for narrowing, and
  // zext for bool→integer. Same-size casts (e.g. char↔i8) are no-ops in
  // LLVM since both map to i8.

  TEST_CASE("identity cast is a no-op") {
    CHECK_RUN("fn main() -> i64 { 42 as i64 }", 42);
  }

  TEST_CASE("cast i64 to i8 and back (small value round-trips)") {
    CHECK_RUN("fn main() -> i64 { 42 as i8 as i64 }", 42);
    CHECK_RUN("fn main() -> i64 { 0 as i8 as i64 }", 0);
    CHECK_RUN("fn main() -> i64 { 127 as i8 as i64 }", 127);
  }

  TEST_CASE("cast i64 to i8 truncates") {
    // 256 truncates to 0 in i8
    CHECK_RUN("fn main() -> i64 { if 256 as i8 as i64 == 0 { 1 } else { 0 } }",
              1);
    // 255 truncates to -1 in i8, sign-extends back to -1 in i64
    CHECK_RUN("fn main() -> i64 { if 255 as i8 as i64 == -1 { 1 } else { 0 } }",
              1);
  }

  TEST_CASE("cast i64 to i32 and back") {
    CHECK_RUN("fn main() -> i64 { 42 as i32 as i64 }", 42);
    CHECK_RUN("fn main() -> i64 { 100 as i32 as i64 }", 100);
  }

  TEST_CASE("cast negative via i8 sign-extends") {
    // -1 as i8 as i64 should preserve -1
    CHECK_RUN("fn main() -> i64 { if -1 as i8 as i64 == -1 { 1 } else { 0 } }",
              1);
  }

  TEST_CASE("chained narrowing cast") {
    // 1000 as i32 as i8 as i64: 1000 fits in i32, then truncate to i8
    // 1000 % 256 = 232, but as signed i8 that's -24, sign-extended to -24
    CHECK_RUN("fn main() -> i64 { if 1000 as i32 as i8 as i64 == -24 { 1 } "
              "else { 0 } }",
              1);
  }

  TEST_CASE("cast bool to i64 via zext") {
    CHECK_RUN("fn main() -> i64 { true as i64 }", 1);
    CHECK_RUN("fn main() -> i64 { false as i64 }", 0);
  }

  TEST_CASE("cast bool to i8 via zext") {
    CHECK_RUN("fn main() -> i64 { true as i8 as i64 }", 1);
    CHECK_RUN("fn main() -> i64 { false as i8 as i64 }", 0);
  }

  TEST_CASE("cast bool to i32 via zext") {
    CHECK_RUN("fn main() -> i64 { true as i32 as i64 }", 1);
  }

  TEST_CASE("cast char to i64 (sext through i8)") {
    CHECK_RUN("fn main() -> i64 { 'A' as i64 }", 65);
  }

  TEST_CASE("cast char to i8 is no-op (both i8 in LLVM)") {
    CHECK_RUN("fn main() -> i64 { 'A' as i8 as i64 }", 65);
  }

  TEST_CASE("cast i64 to char and back") {
    CHECK_RUN("fn main() -> i64 { 65 as char as i64 }", 65);
  }

  TEST_CASE("cast in arithmetic expression") {
    // 'A' (65) cast to i64, then + 1 = 66
    CHECK_RUN("fn main() -> i64 { 'A' as i64 + 1 }", 66);
  }

  TEST_CASE("cast in let binding") {
    CHECK_RUN("fn main() -> i64 { let x: i8 = 42 as i8; x as i64 }", 42);
  }

  TEST_CASE("cast to match function parameter type") {
    CHECK_RUN("fn use_i8(x: i8) -> i8 { x }\n"
              "fn main() -> i64 { use_i8(10 as i8) as i64 }",
              10);
  }

  TEST_CASE("mixed type arithmetic via cast") {
    CHECK_RUN("fn add_mixed(a: i8, b: i64) -> i64 { a as i64 + b }\n"
              "fn main() -> i64 { add_mixed(10 as i8, 20) }",
              30);
  }

  TEST_CASE("i8 function with i8 arithmetic") {
    CHECK_RUN("fn add_i8(a: i8, b: i8) -> i8 { a + b }\n"
              "fn main() -> i64 { add_i8(3 as i8, 4 as i8) as i64 }",
              7);
  }

  TEST_CASE("i32 function with i32 arithmetic") {
    CHECK_RUN("fn add_i32(a: i32, b: i32) -> i32 { a + b }\n"
              "fn main() -> i64 { add_i32(20 as i32, 22 as i32) as i64 }",
              42);
  }

  TEST_CASE("comparison on i8 values") {
    CHECK_RUN("fn main() -> i64 { if 1 as i8 < 2 as i8 { 1 } else { 0 } }", 1);
    CHECK_RUN("fn main() -> i64 { if 5 as i8 == 5 as i8 { 1 } else { 0 } }", 1);
  }

  TEST_CASE("comparison on i32 values") {
    CHECK_RUN("fn main() -> i64 { if 10 as i32 > 5 as i32 { 1 } else { 0 } }",
              1);
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
