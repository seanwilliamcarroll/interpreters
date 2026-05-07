//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Codegen tests for `while` loops and `break` expressions.
//*            IR-shape sanity tests run unconditionally; behavioral tests
//*            run via `lli` and observe iteration count via captured
//*            stdout (putchar).
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
TEST_SUITE("bust.codegen.loops") {

  using namespace ::bust::test;

  // --- IR-shape sanity (no lli required) -----------------------------------

  TEST_CASE("while emits a labeled while_condition block") {
    auto ir = codegen("fn main() -> i64 { while false { } 0 }");
    CHECK(ir.find("while_condition") != std::string::npos);
  }

  TEST_CASE("while emits a labeled while_loop block") {
    auto ir = codegen("fn main() -> i64 { while false { } 0 }");
    CHECK(ir.find("while_loop") != std::string::npos);
  }

  TEST_CASE("while emits a merge block for loop exit") {
    auto ir = codegen("fn main() -> i64 { while false { } 0 }");
    CHECK(ir.find("merge") != std::string::npos);
  }

  TEST_CASE("break compiles when lexically inside a while body") {
    // No throw: the type checker has already accepted this; this just
    // pins down that codegen does not blow up on the BreakExpr handler.
    CHECK_NOTHROW(codegen("fn main() -> i64 { while true { break; } 0 }"));
  }

#ifdef BUST_LLI_PATH

  // --- Loop entry / never-entered ------------------------------------------

  TEST_CASE("while with always-false condition skips the body entirely") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  while false {\n"
                     "    putchar('X' as i32);\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "");
  }

  TEST_CASE("while with let-bound false condition skips the body") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let go = false;\n"
                     "  while go {\n"
                     "    putchar('X' as i32);\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "");
  }

  // --- break exits the loop ------------------------------------------------

  TEST_CASE("while true with immediate break exits with no output") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  while true { break; }\n"
                     "  0\n"
                     "}",
                     0, "");
  }

  TEST_CASE("break after one putchar — body executes exactly once") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  while true {\n"
                     "    putchar('A' as i32);\n"
                     "    break;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "A");
  }

  TEST_CASE("break terminates the body — putchar after break is dead code") {
    // Analogous to the return-prevents-subsequent-putchar test: break is a
    // block terminator, so any statements textually after it in the same
    // block must not generate executable instructions.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  while true {\n"
                     "    putchar('A' as i32);\n"
                     "    break;\n"
                     "    putchar('B' as i32);\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "A");
  }

  TEST_CASE("control resumes after the loop once break fires") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('[' as i32);\n"
                     "  while true {\n"
                     "    putchar('I' as i32);\n"
                     "    break;\n"
                     "  }\n"
                     "  putchar(']' as i32);\n"
                     "  0\n"
                     "}",
                     0, "[I]");
  }

  // --- Conditional break inside the body -----------------------------------

  TEST_CASE("break inside if-then exits the loop on the taken branch") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  while true {\n"
                     "    putchar('A' as i32);\n"
                     "    if true { break; }\n"
                     "    putchar('B' as i32);\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "A");
  }

  TEST_CASE("break inside if-then is skipped when condition is false") {
    // The if-then with no else has unit type and falls through. With
    // condition false, break does not fire; the loop relies on the *outer*
    // condition turning false to exit.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut keep_going = true;\n"
                     "  while keep_going {\n"
                     "    putchar('A' as i32);\n"
                     "    if false { break; }\n"
                     "    keep_going = false;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "A");
  }

  // --- Iteration via mutating condition ------------------------------------
  //
  // These exercise the loop back-edge: the condition must be re-evaluated
  // each iteration. If the back-edge captured the initial condition value,
  // any of these would either loop forever or never iterate past once.

  TEST_CASE("while loop iterates while condition holds, then exits") {
    // i goes 0 → 1 → 2 → 3 (exit). Body fires for i=0,1,2.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut i = 0;\n"
                     "  while i < 3 {\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "    i = i + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "012");
  }

  TEST_CASE("while loop with countdown to zero") {
    // i: 3 → 2 → 1 → 0 (exit). Body fires for i=3,2,1.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut i = 3;\n"
                     "  while i > 0 {\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "    i = i - 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "321");
  }

  TEST_CASE("loop count visible after the loop") {
    // After the loop, i has been mutated to 5; main returns it.
    CHECK_RUN("fn main() -> i64 {\n"
              "  let mut i = 0;\n"
              "  while i < 5 {\n"
              "    i = i + 1;\n"
              "  }\n"
              "  i\n"
              "}",
              5);
  }

  TEST_CASE("break exits early — counter reflects partial iterations") {
    // Loop is meant to run to 10, but we break at 3. i ends at 3.
    CHECK_RUN("fn main() -> i64 {\n"
              "  let mut i = 0;\n"
              "  while i < 10 {\n"
              "    if i == 3 { break; }\n"
              "    i = i + 1;\n"
              "  }\n"
              "  i\n"
              "}",
              3);
  }

  // --- Nested loops --------------------------------------------------------

  TEST_CASE("inner break exits only the innermost loop") {
    // Outer iterates 2 times. Inner break fires immediately each time after
    // printing 'i'. Outer prints 'O' before the inner loop. So output is
    // OiOi.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut o = 0;\n"
                     "  while o < 2 {\n"
                     "    putchar('O' as i32);\n"
                     "    while true {\n"
                     "      putchar('i' as i32);\n"
                     "      break;\n"
                     "    }\n"
                     "    o = o + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "OiOi");
  }

  TEST_CASE("nested while: inner counter resets each outer iteration") {
    // Outer iterates twice. Inner runs 0..2 each time, printing the digit.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut o = 0;\n"
                     "  while o < 2 {\n"
                     "    let mut i = 0;\n"
                     "    while i < 2 {\n"
                     "      putchar(('0' as i32) + (i as i32));\n"
                     "      i = i + 1;\n"
                     "    }\n"
                     "    o = o + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "0101");
  }

  // --- Loop inside a non-main function -------------------------------------

  TEST_CASE("while loop inside a top-level function (not main)") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn print_three() {\n"
                     "  let mut i = 0;\n"
                     "  while i < 3 {\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "    i = i + 1;\n"
                     "  }\n"
                     "}\n"
                     "fn main() -> i64 {\n"
                     "  print_three();\n"
                     "  0\n"
                     "}",
                     0, "012");
  }

  TEST_CASE("break inside loop inside a non-main function") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn print_until_two() {\n"
                     "  let mut i = 0;\n"
                     "  while true {\n"
                     "    if i == 2 { break; }\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "    i = i + 1;\n"
                     "  }\n"
                     "}\n"
                     "fn main() -> i64 {\n"
                     "  print_until_two();\n"
                     "  0\n"
                     "}",
                     0, "01");
  }

  // --- return inside a loop ------------------------------------------------
  //
  // `return` differs from `break`: it terminates the function, not just the
  // innermost loop. These tests pin down that distinction and exercise the
  // loop codegen's divergence check (terminated-block guard) when the body
  // ends in `ret` rather than a jump-to-merge.

  TEST_CASE("return from inside while exits the function with its value") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  while true {\n"
                     "    putchar('A' as i32);\n"
                     "    return 42;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     42, "A");
  }

  TEST_CASE("return after several iterations — partial side effects observed") {
    // i goes 0,1,2,3. At i==3 the return fires; output is "012", exit 99.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut i = 0;\n"
                     "  while true {\n"
                     "    if i == 3 { return 99; }\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "    i = i + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     99, "012");
  }

  TEST_CASE("return terminates body — putchar after return is dead code") {
    // Same pattern as the break-dead-code test, but with return as the
    // terminator. Both should suppress codegen of the trailing putchar.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  while true {\n"
                     "    putchar('A' as i32);\n"
                     "    return 7;\n"
                     "    putchar('B' as i32);\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     7, "A");
  }

  TEST_CASE("return inside loop skips statements after the loop") {
    // Distinguishes return from break: break would let putchar(']') fire.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('[' as i32);\n"
                     "  while true {\n"
                     "    putchar('R' as i32);\n"
                     "    return 5;\n"
                     "  }\n"
                     "  putchar(']' as i32);\n"
                     "  0\n"
                     "}",
                     5, "[R");
  }

  TEST_CASE("return from inner loop exits all loops and the function") {
    // Outer loop schedules 5 iterations; inner loop's return fires on the
    // first iteration. With break, outer would print 'O' five times. With
    // return, output is just "OI".
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut o = 0;\n"
                     "  while o < 5 {\n"
                     "    putchar('O' as i32);\n"
                     "    while true {\n"
                     "      putchar('I' as i32);\n"
                     "      return 42;\n"
                     "    }\n"
                     "    o = o + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     42, "OI");
  }

  TEST_CASE("return inside while inside non-main function") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn loop_until(n: i64) -> i64 {\n"
                     "  let mut i = 0;\n"
                     "  while true {\n"
                     "    if i == n { return i; }\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "    i = i + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}\n"
                     "fn main() -> i64 { loop_until(3) }",
                     3, "012");
  }

  TEST_CASE("naked return inside while in a unit-returning function") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn helper() {\n"
                     "  let mut i = 0;\n"
                     "  while true {\n"
                     "    if i == 2 { return; }\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "    i = i + 1;\n"
                     "  }\n"
                     "}\n"
                     "fn main() -> i64 {\n"
                     "  helper();\n"
                     "  0\n"
                     "}",
                     0, "01");
  }

#else
  TEST_CASE("codegen loop tests" * doctest::skip()) {
    MESSAGE("lli not found at configure time - execution tests skipped");
  }
#endif
}

//****************************************************************************
} // namespace bust
//****************************************************************************
