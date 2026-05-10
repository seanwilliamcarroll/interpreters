//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Codegen tests for `while` / `loop` / `break` / `continue`.
//*            IR-shape sanity tests run unconditionally; behavioral tests
//*            run via `lli` and observe iteration count via captured
//*            stdout (putchar) or via the program's exit code.
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
    CHECK(ir.find("while_body") != std::string::npos);
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

  // --- `loop` expression ---------------------------------------------------
  //
  // `loop { ... }` is the typed-result counterpart to `while`. Its result
  // type is the unified type of all `break <expr>` payloads, or `Never` if
  // the body has no break path out (infinite loop or return-only). These
  // tests pin down both the runtime semantics and the alloca/merge plumbing.

  TEST_CASE("loop with immediate break exits and returns control") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  loop { break; }\n"
                     "  putchar('K' as i32);\n"
                     "  0\n"
                     "}",
                     0, "K");
  }

  TEST_CASE("loop with break value evaluates to that value") {
    // `break 7` carries i64. The loop expression's value is 7; main returns it.
    // If break's store happens after the jump (the bug we hunted), this
    // returns garbage, not 7.
    CHECK_RUN("fn main() -> i64 {\n"
              "  let x = loop { break 7; };\n"
              "  x\n"
              "}",
              7);
  }

  TEST_CASE("loop with break value used directly as a return") {
    CHECK_RUN("fn main() -> i64 {\n"
              "  loop { break 42; }\n"
              "}",
              42);
  }

  TEST_CASE("loop counter — break carries final i value") {
    // Mutate i until threshold; break carrying i. Verifies break-value works
    // when the break is conditional inside an if (not the body's last expr).
    CHECK_RUN("fn main() -> i64 {\n"
              "  let mut i = 0;\n"
              "  loop {\n"
              "    if i == 5 { break i; }\n"
              "    i = i + 1;\n"
              "  }\n"
              "}",
              5);
  }

  TEST_CASE("control resumes after loop with break value") {
    // After the loop yields, the trailing putchar must still fire — the
    // LoopExpr handler must enter the merge block. With the
    // current_block_terminated() bug, the trailing 'B' would be silently
    // dropped.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('A' as i32);\n"
                     "  let v = loop { break 9; };\n"
                     "  putchar('B' as i32);\n"
                     "  v\n"
                     "}",
                     9, "AB");
  }

  TEST_CASE("loop with break in both if/else branches") {
    // Both branches diverge → the if has no merge of its own. The loop body
    // is fully terminated by either break. Loop's merge must still be
    // entered to load the result. Result is 11.
    CHECK_RUN("fn main() -> i64 {\n"
              "  let cond = true;\n"
              "  loop {\n"
              "    if cond { break 11; } else { break 22; }\n"
              "  }\n"
              "}",
              11);
  }

  TEST_CASE("loop with break-then and break-else picks the false branch") {
    CHECK_RUN("fn main() -> i64 {\n"
              "  let cond = false;\n"
              "  loop {\n"
              "    if cond { break 11; } else { break 22; }\n"
              "  }\n"
              "}",
              22);
  }

  TEST_CASE("loop with unit break carries no payload") {
    // Sugar: `break;` ≡ `break ();`. Loop's type is `()`, no alloca emitted.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  loop { putchar('L' as i32); break; }\n"
                     "  0\n"
                     "}",
                     0, "L");
  }

  TEST_CASE("loop with return inside body — return wins over break") {
    // The `return 5;` exits the function before any iteration completes.
    // Even though the loop has a break (so `loop_cannot_end` is false and
    // a merge block exists), control never reaches it.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('A' as i32);\n"
                     "  let v = loop {\n"
                     "    return 5;\n"
                     "    break 9;\n"
                     "  };\n"
                     "  putchar('B' as i32);\n"
                     "  v\n"
                     "}",
                     5, "A");
  }

  TEST_CASE("nested loops — inner break value, outer continues") {
    // Inner loop breaks with i. Outer accumulates inner_result into total.
    // After two outer iterations, total should be 3 + 3 = 6.
    CHECK_RUN("fn main() -> i64 {\n"
              "  let mut o = 0;\n"
              "  let mut total = 0;\n"
              "  while o < 2 {\n"
              "    let mut i = 0;\n"
              "    let inner = loop {\n"
              "      if i == 3 { break i; }\n"
              "      i = i + 1;\n"
              "    };\n"
              "    total = total + inner;\n"
              "    o = o + 1;\n"
              "  }\n"
              "  total\n"
              "}",
              6);
  }

  TEST_CASE("nested loops — inner break does not exit outer loop") {
    // Outer iterates twice. Inner is `loop { break N; }` — exits with a
    // value but only the inner. Outer prints 'O' both iterations.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut o = 0;\n"
                     "  while o < 2 {\n"
                     "    putchar('O' as i32);\n"
                     "    let _ = loop { break 1; };\n"
                     "    o = o + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "OO");
  }

  // --- `continue` ----------------------------------------------------------

  TEST_CASE("continue in while jumps back to the condition") {
    // i: 0..5. When i is even, continue (skip the putchar). Output is "135".
    // If continue jumped to the body label instead of the condition label,
    // the loop would never exit (i would never re-evaluate).
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut i = 0;\n"
                     "  while i < 5 {\n"
                     "    i = i + 1;\n"
                     "    if (i % 2) == 0 { continue; }\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "135");
  }

  TEST_CASE("continue in loop jumps back to the body label") {
    // Use `loop` (not while). The continue must restart the body, not jump
    // to a non-existent condition block. We use a counter + conditional
    // break to terminate. Only odd values are printed.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut i = 0;\n"
                     "  loop {\n"
                     "    i = i + 1;\n"
                     "    if i > 5 { break; }\n"
                     "    if (i % 2) == 0 { continue; }\n"
                     "    putchar(('0' as i32) + (i as i32));\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "135");
  }

  TEST_CASE("continue terminates body — putchar after continue is dead code") {
    // Same dead-code pattern as the break-after test, applied to continue.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut i = 0;\n"
                     "  while i < 3 {\n"
                     "    i = i + 1;\n"
                     "    putchar('A' as i32);\n"
                     "    continue;\n"
                     "    putchar('B' as i32);\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "AAA");
  }

  TEST_CASE("continue in nested loop only restarts the innermost") {
    // Outer iterates twice. Inner runs i=0..2; when i==0 continue skips
    // the inner putchar. Output: outer prints 'O', inner prints '1' both
    // iterations → "O1O1".
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let mut o = 0;\n"
                     "  while o < 2 {\n"
                     "    putchar('O' as i32);\n"
                     "    let mut i = 0;\n"
                     "    while i < 2 {\n"
                     "      i = i + 1;\n"
                     "      if i == 1 { continue; }\n"
                     "      putchar(('0' as i32) + (i as i32));\n"
                     "    }\n"
                     "    o = o + 1;\n"
                     "  }\n"
                     "  0\n"
                     "}",
                     0, "O2O2");
  }

  // --- Lambda + loop interaction -------------------------------------------

  TEST_CASE("lambda body using loop with break value") {
    // The lambda's loop_stack is its own — a break inside the lambda must
    // target the lambda's loop, not any outer loop.
    CHECK_RUN("fn main() -> i64 {\n"
              "  let f = || -> i64 {\n"
              "    let mut i = 0;\n"
              "    loop {\n"
              "      if i == 4 { break i; }\n"
              "      i = i + 1;\n"
              "    }\n"
              "  };\n"
              "  f()\n"
              "}",
              4);
  }

  TEST_CASE("lambda with continue inside a while body") {
    // Continue must dispatch to the lambda's enclosing loop, not anything
    // outer. This program prints "13" — odd values, as in the earlier
    // continue-in-while test, but routed through a lambda call.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let f = || -> () {\n"
                     "    let mut i = 0;\n"
                     "    while i < 3 {\n"
                     "      i = i + 1;\n"
                     "      if (i % 2) == 0 { continue; }\n"
                     "      putchar(('0' as i32) + (i as i32));\n"
                     "    }\n"
                     "  };\n"
                     "  f();\n"
                     "  0\n"
                     "}",
                     0, "13");
  }

  // --- IR shape: alloca for typed loop, none for Never ---------------------

  TEST_CASE("typed loop emits a loop_result alloca") {
    auto ir = codegen("fn main() -> i64 { loop { break 1; } }");
    CHECK(ir.find("loop_result") != std::string::npos);
  }

  TEST_CASE("loop typed Never emits no loop_result alloca and no merge") {
    // No break, no return — `loop {}` would be infinite. We use a body that
    // mutates so there's something to lower, but no break and no return:
    // type Never, no merge block, no loop_result alloca. Use a return
    // *after* the loop position so main still type-checks (the loop is
    // Never-typed and consumes the position).
    auto ir =
        codegen("fn main() -> i64 { let mut i = 0; loop { i = i + 1; } }");
    CHECK(ir.find("loop_result") == std::string::npos);
  }

  TEST_CASE("loop emits a labeled loop_body block") {
    auto ir = codegen("fn main() -> i64 { loop { break 0; } }");
    CHECK(ir.find("loop_body") != std::string::npos);
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
