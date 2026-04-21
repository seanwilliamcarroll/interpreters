//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Codegen tests for lambda expressions and first-class functions.
//*            Covers non-capturing lambdas (lifted to top-level with a
//*            constant closure global), lambdas with captures (env struct
//*            allocated and populated), top-level functions used as
//*            first-class values, and higher-order functions that return
//*            closures (make_adder style).
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <doctest/doctest.h>

#include "codegen_test_helpers.hpp"

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.codegen.lambdas") {

  using namespace ::bust::test;

#ifdef BUST_LLI_PATH

  // --- Non-capturing lambdas -----------------------------------------------
  //
  // These compile via the lift_free_lambda path: the lambda body is emitted
  // as a top-level function and a `constant %closure { ptr @fn, ptr null }`
  // global is synthesized for it. Calls load the function pointer and env
  // out of the fat pointer and invoke normally.

  TEST_CASE("basic lambda call") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let add = |a: i64, b: i64| -> i64 { a + b };\n"
              "  add(3, 4)\n"
              "}",
              7);
  }

  TEST_CASE("lambda with explicit parameter type") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let double = |x: i64| -> i64 { x + x };\n"
              "  double(5)\n"
              "}",
              10);
  }

  TEST_CASE("lambda passed as argument to function") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }\n"
              "fn main() -> i64 {\n"
              "  let inc = |x: i64| -> i64 { x + 1 };\n"
              "  apply(inc, 41)\n"
              "}",
              42);
  }

  TEST_CASE("lambda with no parameters") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let f = || -> i64 { 99 };\n"
              "  f()\n"
              "}",
              99);
  }

  TEST_CASE("lambda calling another function") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn helper(x: i64) -> i64 { x * 2 }\n"
              "fn main() -> i64 {\n"
              "  let f = |x: i64| -> i64 { helper(x) + 1 };\n"
              "  f(20)\n"
              "}",
              41);
  }

  TEST_CASE("multiple lambdas in same function") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let add = |a: i64, b: i64| -> i64 { a + b };\n"
              "  let mul = |a: i64, b: i64| -> i64 { a * b };\n"
              "  add(mul(3, 4), 2)\n"
              "}",
              14);
  }

  TEST_CASE("lambda result used in arithmetic") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let f = |x: i64| -> i64 { x + 1 };\n"
              "  f(10) + f(20)\n"
              "}",
              32);
  }

  TEST_CASE("lambda in if condition") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let is_big = |x: i64| -> bool { x > 10 };\n"
              "  if is_big(20) { 1 } else { 0 }\n"
              "}",
              1);
  }

  TEST_CASE("identity lambda") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let id = |x: i64| -> i64 { x };\n"
              "  id(42)\n"
              "}",
              42);
  }

  // --- Capturing lambdas ---------------------------------------------------
  //
  // These exercise the ClosureBuilder path: a per-call env struct is
  // allocated (via malloc), the captured values are written into it, and
  // the fat pointer {fn_ptr, env_ptr} is returned. The callee loads its
  // captures out of the env pointer on entry.

  TEST_CASE("lambda captures single local i64") {
    // The lambda reads `n` from its environment rather than from the
    // caller's stack; if capture were broken the value would be garbage.
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let n = 5;\n"
              "  let add_n = |x: i64| -> i64 { x + n };\n"
              "  add_n(10)\n"
              "}",
              15);
  }

  TEST_CASE("lambda captures multiple locals") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let a = 3;\n"
              "  let b = 4;\n"
              "  let sum = || -> i64 { a + b };\n"
              "  sum()\n"
              "}",
              7);
  }

  TEST_CASE("lambda reads same capture twice") {
    // Exercises that each read of `n` inside the lambda body goes back
    // through the env pointer — one load per use, not cached across.
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let n = 6;\n"
              "  let double_n = || -> i64 { n + n };\n"
              "  double_n()\n"
              "}",
              12);
  }

  TEST_CASE("capturing lambda mixes captured and parameter values") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn main() -> i64 {\n"
              "  let base = 10;\n"
              "  let offset = 2;\n"
              "  let combine = |x: i64| -> i64 { base + x * offset };\n"
              "  combine(16)\n"
              "}",
              42);
  }

  // --- Top-level functions as first-class values ---------------------------
  //
  // A named `fn` used as a value should behave the same as a non-capturing
  // lambda: it's wrapped in the same `{fn_ptr, null_env}` fat pointer so
  // that the call-site code doesn't need to care whether it's calling a
  // lambda or a top-level function.

  TEST_CASE("top-level function passed to higher-order function") {
    CHECK_RUN("fn inc(x: i64) -> i64 { x + 1 }\n"
              "fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }\n"
              "fn main() -> i64 { apply(inc, 41) }",
              42);
  }

  // --- Returning a closure (higher-order function returning a function) ---
  //
  // make_adder(n) returns a closure that captures n. This is the canonical
  // test that captured values survive past the stack frame of the function
  // that created them — if the env were stack-allocated in make_adder,
  // add5 would read garbage once make_adder returned.

  TEST_CASE("make_adder returns a capturing closure") {
    CHECK_RUN("extern fn malloc(size: i64) -> i64;\n"
              "fn make_adder(n: i64) -> fn(i64) -> i64 {\n"
              "  |x: i64| -> i64 { x + n }\n"
              "}\n"
              "fn main() -> i64 {\n"
              "  let add5 = make_adder(5);\n"
              "  add5(10)\n"
              "}",
              15);
  }

#else
  TEST_CASE("codegen lambda tests" * doctest::skip()) {
    MESSAGE("lli not found at configure time - execution tests skipped");
  }
#endif
}

//****************************************************************************
} // namespace bust
//****************************************************************************
