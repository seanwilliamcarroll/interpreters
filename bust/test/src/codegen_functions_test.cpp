//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Codegen tests for function-level constructs: calls, recursion,
//*            multi-argument dispatch, and extern declarations. Execution
//*            tests here exercise value returns only; side-effect-driven
//*            tests (putchar etc.) live in codegen_side_effects_test.cpp.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <test/inc/codegen_test_helpers.hpp>

#include <string>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.codegen.functions") {

  using namespace ::bust::test;

  // --- Extern function IR-shape (no lli required) --------------------------

  TEST_CASE("extern function emits declare, not define") {
    auto ir = codegen("extern fn putchar(c: i32) -> i32;\n"
                      "fn main() -> i64 { 0 }");
    CHECK(ir.find("declare i32 @putchar(i32") != std::string::npos);
    CHECK(ir.find("define i32 @putchar(i32") == std::string::npos);
  }

  TEST_CASE("extern function with no return type emits declare void") {
    auto ir = codegen("extern fn log_it(x: i64);\n"
                      "fn main() -> i64 { 0 }");
    CHECK(ir.find("declare void @log_it(i64") != std::string::npos);
  }

  TEST_CASE("extern and regular functions coexist in IR") {
    auto ir = codegen("extern fn putchar(c: i32) -> i32;\n"
                      "fn main() -> i64 { 0 }");
    CHECK(ir.find("declare i32 @putchar") != std::string::npos);
    CHECK(ir.find("define i64 @main") != std::string::npos);
  }

  TEST_CASE("multiple extern declarations all emit declare") {
    auto ir = codegen("extern fn putchar(c: i32) -> i32;\n"
                      "extern fn getchar() -> i32;\n"
                      "fn main() -> i64 { 0 }");
    CHECK(ir.find("declare i32 @putchar") != std::string::npos);
    CHECK(ir.find("declare i32 @getchar") != std::string::npos);
  }

#ifdef BUST_LLI_PATH

  // --- Function calls ------------------------------------------------------

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

  // --- Recursion -----------------------------------------------------------

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

  // --- Extern function execution (exit-code-only) --------------------------

  TEST_CASE("call extern putchar via lli") {
    // putchar returns the character written (as i32), cast to i64 for main
    CHECK_RUN("extern fn putchar(c: i32) -> i32;\n"
              "fn main() -> i64 { putchar('H' as i32) as i64 }",
              72); // 'H' == 72
  }

  TEST_CASE("call extern putchar multiple times") {
    // Print "Hi" and return 0
    CHECK_RUN("extern fn putchar(c: i32) -> i32;\n"
              "fn main() -> i64 {\n"
              "  putchar('H' as i32);\n"
              "  putchar('i' as i32);\n"
              "  putchar('\\n' as i32);\n"
              "  0\n"
              "}",
              0);
  }

  TEST_CASE("extern function call result in arithmetic") {
    // putchar('*') returns 42, cast to i64
    CHECK_RUN("extern fn putchar(c: i32) -> i32;\n"
              "fn main() -> i64 { putchar(42 as i32) as i64 }",
              42); // '*' == 42
  }

  TEST_CASE("extern function call in let binding") {
    CHECK_RUN("extern fn putchar(c: i32) -> i32;\n"
              "fn main() -> i64 {\n"
              "  let result = putchar('!' as i32);\n"
              "  result as i64\n"
              "}",
              33); // '!' == 33
  }

#else
  TEST_CASE("codegen function execution tests" * doctest::skip()) {
    MESSAGE("lli not found at configure time - execution tests skipped");
  }
#endif
}

//****************************************************************************
} // namespace bust
//****************************************************************************
