//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for bust::Evaluator
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <evaluator.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <type_checker.hpp>

#include <doctest/doctest.h>
#include <sstream>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.evaluator") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program type_check(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    auto program = parser.parse();
    TypeChecker checker;
    return checker(program);
  }

  static int64_t evaluate(const std::string &source) {
    auto hir = type_check(source);
    Evaluator eval;
    return eval(hir);
  }

  // --- Literal expressions -------------------------------------------------

  TEST_CASE("main returns i64 literal") {
    CHECK(evaluate("fn main() -> i64 { 42 }") == 42);
  }

  TEST_CASE("main returns zero") {
    CHECK(evaluate("fn main() -> i64 { 0 }") == 0);
  }

  TEST_CASE("main returns negative literal") {
    CHECK(evaluate("fn main() -> i64 { -1 }") == -1);
  }

  // --- Arithmetic ----------------------------------------------------------

  TEST_CASE("addition") { CHECK(evaluate("fn main() -> i64 { 2 + 3 }") == 5); }

  TEST_CASE("subtraction") {
    CHECK(evaluate("fn main() -> i64 { 10 - 4 }") == 6);
  }

  TEST_CASE("multiplication") {
    CHECK(evaluate("fn main() -> i64 { 3 * 7 }") == 21);
  }

  TEST_CASE("division") { CHECK(evaluate("fn main() -> i64 { 20 / 4 }") == 5); }

  TEST_CASE("modulo") { CHECK(evaluate("fn main() -> i64 { 17 % 5 }") == 2); }

  TEST_CASE("nested arithmetic") {
    CHECK(evaluate("fn main() -> i64 { (2 + 3) * (10 - 4) }") == 30);
  }

  TEST_CASE("unary minus") {
    CHECK(evaluate("fn main() -> i64 { -(5 + 3) }") == -8);
  }

  // --- Boolean expressions -------------------------------------------------

  TEST_CASE("boolean true used in if") {
    CHECK(evaluate("fn main() -> i64 { if true { 1 } else { 0 } }") == 1);
  }

  TEST_CASE("boolean false used in if") {
    CHECK(evaluate("fn main() -> i64 { if false { 1 } else { 0 } }") == 0);
  }

  TEST_CASE("unary not") {
    CHECK(evaluate("fn main() -> i64 { if !false { 1 } else { 0 } }") == 1);
  }

  // --- Comparison operators ------------------------------------------------

  TEST_CASE("less than true") {
    CHECK(evaluate("fn main() -> i64 { if 3 < 5 { 1 } else { 0 } }") == 1);
  }

  TEST_CASE("less than false") {
    CHECK(evaluate("fn main() -> i64 { if 5 < 3 { 1 } else { 0 } }") == 0);
  }

  TEST_CASE("greater than") {
    CHECK(evaluate("fn main() -> i64 { if 5 > 3 { 1 } else { 0 } }") == 1);
  }

  TEST_CASE("less than or equal") {
    CHECK(evaluate("fn main() -> i64 { if 5 <= 5 { 1 } else { 0 } }") == 1);
  }

  TEST_CASE("greater than or equal") {
    CHECK(evaluate("fn main() -> i64 { if 4 >= 5 { 1 } else { 0 } }") == 0);
  }

  TEST_CASE("equality true") {
    CHECK(evaluate("fn main() -> i64 { if 42 == 42 { 1 } else { 0 } }") == 1);
  }

  TEST_CASE("equality false") {
    CHECK(evaluate("fn main() -> i64 { if 42 == 43 { 1 } else { 0 } }") == 0);
  }

  TEST_CASE("not equal") {
    CHECK(evaluate("fn main() -> i64 { if 1 != 2 { 1 } else { 0 } }") == 1);
  }

  // --- Logical operators ---------------------------------------------------

  TEST_CASE("logical and true") {
    CHECK(evaluate("fn main() -> i64 { if true && true { 1 } else { 0 } }") ==
          1);
  }

  TEST_CASE("logical and false") {
    CHECK(evaluate("fn main() -> i64 { if true && false { 1 } else { 0 } }") ==
          0);
  }

  TEST_CASE("logical or true") {
    CHECK(evaluate("fn main() -> i64 { if false || true { 1 } else { 0 } }") ==
          1);
  }

  TEST_CASE("logical or false") {
    CHECK(evaluate("fn main() -> i64 { if false || false { 1 } else { 0 } }") ==
          0);
  }

  TEST_CASE("logical and short circuits") {
    // If && short-circuits, the right side is never evaluated.
    // We can't directly observe side effects yet, but we can verify the result.
    CHECK(evaluate("fn main() -> i64 { if false && true { 1 } else { 0 } }") ==
          0);
  }

  // --- Let bindings --------------------------------------------------------

  TEST_CASE("let binding and use") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 10;\n"
                   "  x\n"
                   "}") == 10);
  }

  TEST_CASE("let binding with annotation") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x: i64 = 42;\n"
                   "  x\n"
                   "}") == 42);
  }

  TEST_CASE("multiple let bindings") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let a = 3;\n"
                   "  let b = 4;\n"
                   "  a + b\n"
                   "}") == 7);
  }

  TEST_CASE("let binding uses previous binding") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 5;\n"
                   "  let y = x + 1;\n"
                   "  y\n"
                   "}") == 6);
  }

  TEST_CASE("variable shadowing") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 1;\n"
                   "  let x = 2;\n"
                   "  x\n"
                   "}") == 2);
  }

  // --- Blocks and scoping --------------------------------------------------

  TEST_CASE("block expression") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = { 42 };\n"
                   "  x\n"
                   "}") == 42);
  }

  TEST_CASE("block with statements and final expression") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let result = {\n"
                   "    let a = 10;\n"
                   "    let b = 20;\n"
                   "    a + b\n"
                   "  };\n"
                   "  result\n"
                   "}") == 30);
  }

  TEST_CASE("inner scope does not leak") {
    // x is defined in the outer scope, used after inner block
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 1;\n"
                   "  {\n"
                   "    let y = 2;\n"
                   "  };\n"
                   "  x\n"
                   "}") == 1);
  }

  // --- If expressions ------------------------------------------------------

  TEST_CASE("if-else returns then branch") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  if true { 10 } else { 20 }\n"
                   "}") == 10);
  }

  TEST_CASE("if-else returns else branch") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  if false { 10 } else { 20 }\n"
                   "}") == 20);
  }

  TEST_CASE("if-else with computed condition") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 5;\n"
                   "  if x > 3 { 1 } else { 0 }\n"
                   "}") == 1);
  }

  TEST_CASE("nested if-else") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 10;\n"
                   "  if x > 20 { 3 }\n"
                   "  else { if x > 5 { 2 } else { 1 } }\n"
                   "}") == 2);
  }

  // --- Functions -----------------------------------------------------------

  TEST_CASE("call a simple function") {
    CHECK(evaluate("fn add(a: i64, b: i64) -> i64 { a + b }\n"
                   "fn main() -> i64 { add(3, 4) }") == 7);
  }

  TEST_CASE("function with no parameters") {
    CHECK(evaluate("fn forty_two() -> i64 { 42 }\n"
                   "fn main() -> i64 { forty_two() }") == 42);
  }

  TEST_CASE("function calls another function") {
    CHECK(evaluate("fn double(x: i64) -> i64 { x + x }\n"
                   "fn quadruple(x: i64) -> i64 { double(double(x)) }\n"
                   "fn main() -> i64 { quadruple(3) }") == 12);
  }

  TEST_CASE("forward reference: main calls function defined after it") {
    CHECK(evaluate("fn main() -> i64 { helper(10) }\n"
                   "fn helper(x: i64) -> i64 { x + 1 }") == 11);
  }

  // --- Recursion -----------------------------------------------------------

  TEST_CASE("simple recursion: factorial") {
    CHECK(evaluate("fn factorial(n: i64) -> i64 {\n"
                   "  if n <= 1 { 1 }\n"
                   "  else { n * factorial(n - 1) }\n"
                   "}\n"
                   "fn main() -> i64 { factorial(5) }") == 120);
  }

  TEST_CASE("simple recursion: fibonacci") {
    CHECK(evaluate("fn fib(n: i64) -> i64 {\n"
                   "  if n <= 1 { n }\n"
                   "  else { fib(n - 1) + fib(n - 2) }\n"
                   "}\n"
                   "fn main() -> i64 { fib(10) }") == 55);
  }

  TEST_CASE("mutual recursion") {
    CHECK(evaluate("fn is_even(n: i64) -> i64 {\n"
                   "  if n == 0 { 1 }\n"
                   "  else { is_odd(n - 1) }\n"
                   "}\n"
                   "fn is_odd(n: i64) -> i64 {\n"
                   "  if n == 0 { 0 }\n"
                   "  else { is_even(n - 1) }\n"
                   "}\n"
                   "fn main() -> i64 { is_even(4) }") == 1);
  }

  // --- Return expressions --------------------------------------------------

  TEST_CASE("early return") {
    CHECK(evaluate("fn early(x: i64) -> i64 {\n"
                   "  if x > 10 { return 99; }\n"
                   "  x\n"
                   "}\n"
                   "fn main() -> i64 { early(20) }") == 99);
  }

  TEST_CASE("early return not taken") {
    CHECK(evaluate("fn early(x: i64) -> i64 {\n"
                   "  if x > 10 { return 99; }\n"
                   "  x\n"
                   "}\n"
                   "fn main() -> i64 { early(5) }") == 5);
  }

  // --- Return semantics ----------------------------------------------------
  // These tests pin down that `return` unwinds to the *enclosing function*
  // (or lambda) and no further. The evaluator implements this with a custom
  // exception thrown by `return` and caught at every call boundary; these
  // tests would catch a missing catch site, an over-broad catch, or a
  // miscount of unwinding levels.

  TEST_CASE("return unwinds through nested blocks") {
    CHECK(evaluate("fn f(x: i64) -> i64 {\n"
                   "  {\n"
                   "    {\n"
                   "      { return 7; };\n"
                   "    };\n"
                   "  };\n"
                   "  x\n"
                   "}\n"
                   "fn main() -> i64 { f(0) }") == 7);
  }

  TEST_CASE("return unwinds through nested if-else") {
    CHECK(evaluate("fn f(x: i64) -> i64 {\n"
                   "  if x > 0 {\n"
                   "    if x > 5 { return 1; } else { return 2; }\n"
                   "  } else { return 3; }\n"
                   "}\n"
                   "fn main() -> i64 { f(10) }") == 1);
  }

  TEST_CASE("return from inside arithmetic subexpression") {
    // The right operand triggers a return mid-expression; the `+` should
    // never complete.
    CHECK(evaluate("fn f() -> i64 {\n"
                   "  1 + { return 99; }\n"
                   "}\n"
                   "fn main() -> i64 { f() }") == 99);
  }

  TEST_CASE("return inside callee does not affect caller") {
    // `inner` returns 5; the caller should observe 5 and continue.
    CHECK(evaluate("fn inner() -> i64 { return 5; }\n"
                   "fn outer() -> i64 { inner() + 10 }\n"
                   "fn main() -> i64 { outer() }") == 15);
  }

  TEST_CASE("return inside lambda returns from lambda only") {
    // The return must be caught at the lambda call site, not propagate
    // out to `main`. If it leaked, `+ 100` would never execute.
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let f = || -> i64 { return 1; };\n"
                   "  f() + 100\n"
                   "}") == 101);
  }

  TEST_CASE("return inside lambda passed to higher-order function") {
    // Lambda returning early, invoked by another function. The catch must
    // happen at the lambda invocation inside `apply`, not escape `apply`.
    CHECK(evaluate("fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) + 1 }\n"
                   "fn main() -> i64 {\n"
                   "  let g = |x: i64| -> i64 { return x * 2; };\n"
                   "  apply(g, 10)\n"
                   "}") == 21);
  }

  TEST_CASE("return from recursive call unwinds only one frame") {
    // Each recursive call has its own frame; an early return from a deep
    // call should only pop that one frame, with the caller's arithmetic
    // continuing normally.
    CHECK(evaluate("fn f(n: i64) -> i64 {\n"
                   "  if n == 0 { return 100; }\n"
                   "  f(n - 1) + 1\n"
                   "}\n"
                   "fn main() -> i64 { f(3) }") == 103);
  }

  TEST_CASE("return inside lambda inside function returns from lambda") {
    // Two nested call boundaries: the lambda's return must be caught at
    // the lambda call, then the surrounding function continues to `+ 1`.
    CHECK(evaluate("fn f() -> i64 {\n"
                   "  let g = || -> i64 { return 41; };\n"
                   "  g() + 1\n"
                   "}\n"
                   "fn main() -> i64 { f() }") == 42);
  }

  TEST_CASE("return from then-branch of if") {
    CHECK(evaluate("fn f(x: i64) -> i64 {\n"
                   "  if x > 0 { return 111; }\n"
                   "  222\n"
                   "}\n"
                   "fn main() -> i64 { f(1) }") == 111);
  }

  TEST_CASE("return short-circuits subsequent statements") {
    CHECK(evaluate("fn f() -> i64 {\n"
                   "  let a = 1;\n"
                   "  return 7;\n"
                   "  let b = 2;\n"
                   "  a + b\n"
                   "}\n"
                   "fn main() -> i64 { f() }") == 7);
  }

  TEST_CASE("return inside function called from main") {
    // Sanity: the catch at main's call boundary must not let f's return
    // escape into the host C++ runtime.
    CHECK(evaluate("fn f() -> i64 { return 42; }\n"
                   "fn main() -> i64 { f() }") == 42);
  }

  TEST_CASE("return directly from main") {
    CHECK(evaluate("fn main() -> i64 { return 17; }") == 17);
  }

  // --- Scope lifetime across return/unwind ---------------------------------
  // These tests pin down that block scopes are correctly popped on BOTH the
  // normal exit path and the return-exception unwind path. A leaked inner
  // scope would show up as a shadowed binding incorrectly surviving into
  // sibling or outer scopes.

  TEST_CASE("inner block shadow does not leak to outer scope") {
    // Normal path: inner scope must pop so outer x is visible again.
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 1;\n"
                   "  { let x = 99; };\n"
                   "  x\n"
                   "}") == 1);
  }

  TEST_CASE("sibling blocks with shadowing do not leak between each other") {
    // If block A's scope leaked, block B would see x=10 from A instead of
    // the outer x=1 when evaluating its own `let x = x + ...`.
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 1;\n"
                   "  { let x = 10; };\n"
                   "  let y = { let x = x + 100; x };\n"
                   "  y\n"
                   "}") == 101);
  }

  TEST_CASE("deeply nested shadowing all pop on normal exit") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = 1;\n"
                   "  { let x = 2; { let x = 3; { let x = 4; }; }; };\n"
                   "  x\n"
                   "}") == 1);
  }

  TEST_CASE("scope popped correctly when block contains untaken return") {
    // The block enters the `try`, evaluates an if whose return branch is
    // NOT taken, and exits normally. The catch path is not triggered, but
    // the normal-path pop must still run.
    CHECK(evaluate("fn f(flag: i64) -> i64 {\n"
                   "  let x = 1;\n"
                   "  {\n"
                   "    let x = 99;\n"
                   "    if flag > 0 { return 7; }\n"
                   "  };\n"
                   "  x\n"
                   "}\n"
                   "fn main() -> i64 { f(0) }") == 1);
  }

  TEST_CASE("return unwinds through multiple shadowing scopes cleanly") {
    // Deep nested scopes each shadow x, then innermost returns the
    // innermost x. If any scope leaked state that corrupted the return
    // value in flight, this would fail.
    CHECK(evaluate("fn f() -> i64 {\n"
                   "  let x = 1;\n"
                   "  {\n"
                   "    let x = 2;\n"
                   "    {\n"
                   "      let x = 3;\n"
                   "      {\n"
                   "        let x = 4;\n"
                   "        return x;\n"
                   "      };\n"
                   "    };\n"
                   "  };\n"
                   "  x\n"
                   "}\n"
                   "fn main() -> i64 { f() }") == 4);
  }

  TEST_CASE("after untaken return in nested block, outer shadowing resolves") {
    // Exercises: nested blocks with shadowing, a return statement that is
    // not triggered, then a subsequent read of the outer binding. If the
    // inner scopes did not pop, the final `x` would resolve to 99 or 42.
    CHECK(evaluate("fn f(flag: i64) -> i64 {\n"
                   "  let x = 1;\n"
                   "  {\n"
                   "    let x = 42;\n"
                   "    {\n"
                   "      let x = 99;\n"
                   "      if flag > 0 { return 0; }\n"
                   "    };\n"
                   "  };\n"
                   "  x\n"
                   "}\n"
                   "fn main() -> i64 { f(0) }") == 1);
  }

  TEST_CASE("block after a block that contained an untaken return") {
    // Sibling block sees clean outer scope after the first block's
    // try/normal-pop path has run.
    CHECK(evaluate("fn f() -> i64 {\n"
                   "  let x = 5;\n"
                   "  { let x = 100; if false { return 0; } };\n"
                   "  { let x = x + 1; x }\n"
                   "}\n"
                   "fn main() -> i64 { f() }") == 6);
  }

  // --- Lambda expressions --------------------------------------------------

  TEST_CASE("lambda call") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let add = |a: i64, b: i64| -> i64 { a + b };\n"
                   "  add(3, 4)\n"
                   "}") == 7);
  }

  TEST_CASE("lambda with inferred types") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let double = |x| { x + x };\n"
                   "  double(5)\n"
                   "}") == 10);
  }

  TEST_CASE("lambda captures enclosing scope") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let offset = 100;\n"
                   "  let add_offset = |x: i64| -> i64 { x + offset };\n"
                   "  add_offset(5)\n"
                   "}") == 105);
  }

  TEST_CASE("lambda passed as argument") {
    CHECK(evaluate("fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }\n"
                   "fn main() -> i64 {\n"
                   "  let inc = |x: i64| -> i64 { x + 1 };\n"
                   "  apply(inc, 41)\n"
                   "}") == 42);
  }

  TEST_CASE("higher order: function returning a result from lambda") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let make_adder = |n: i64| -> i64 { n + 10 };\n"
                   "  make_adder(32)\n"
                   "}") == 42);
  }

  TEST_CASE("polymorphic identity lambda") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let id = |x| { x };\n"
                   "  id(42)\n"
                   "}") == 42);
  }

  // --- Composite programs --------------------------------------------------

  TEST_CASE("program with multiple functions and let bindings") {
    CHECK(evaluate("fn square(x: i64) -> i64 { x * x }\n"
                   "fn main() -> i64 {\n"
                   "  let a = square(3);\n"
                   "  let b = square(4);\n"
                   "  a + b\n"
                   "}") == 25);
  }

  TEST_CASE("complex: iterative sum via recursion") {
    CHECK(evaluate("fn sum(n: i64) -> i64 {\n"
                   "  if n <= 0 { 0 }\n"
                   "  else { n + sum(n - 1) }\n"
                   "}\n"
                   "fn main() -> i64 { sum(10) }") == 55);
  }

  TEST_CASE("complex: nested lets and blocks with arithmetic") {
    CHECK(evaluate("fn main() -> i64 {\n"
                   "  let x = {\n"
                   "    let a = 2;\n"
                   "    let b = 3;\n"
                   "    a * b\n"
                   "  };\n"
                   "  let y = {\n"
                   "    let c = x + 1;\n"
                   "    c * 2\n"
                   "  };\n"
                   "  y\n"
                   "}") == 14);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
