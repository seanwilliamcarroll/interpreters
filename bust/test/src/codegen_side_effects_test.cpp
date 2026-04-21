//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Codegen tests that observe side effects via stdout (putchar).
//*            Covers statement sequencing, control flow execution order,
//*            short-circuit evaluation, and mixed control flow + side
//*            effects. All tests use CHECK_RUN_OUTPUT to assert both exit
//*            code and captured stdout.
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
TEST_SUITE("bust.codegen.side_effects") {

  using namespace ::bust::test;

#ifdef BUST_LLI_PATH

  // --- Statement sequencing ------------------------------------------------
  //
  // These tests prove that side-effecting statements execute in textual
  // order by capturing the actual stdout output.

  TEST_CASE("putchar sequence produces correct output") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('A' as i32);\n"
                     "  putchar('B' as i32);\n"
                     "  putchar('C' as i32);\n"
                     "  0\n"
                     "}",
                     0, "ABC");
  }

  TEST_CASE("statement order: output matches source order") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('3' as i32);\n"
                     "  putchar('2' as i32);\n"
                     "  putchar('1' as i32);\n"
                     "  0\n"
                     "}",
                     0, "321");
  }

  TEST_CASE("putchar with newline in output") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('H' as i32);\n"
                     "  putchar('i' as i32);\n"
                     "  putchar('\\n' as i32);\n"
                     "  0\n"
                     "}",
                     0, "Hi\n");
  }

  // --- Control flow with side effects --------------------------------------
  //
  // Verifies that only the taken branch's side effects execute.

  TEST_CASE("putchar in then branch only") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  if true {\n"
                     "    putchar('Y' as i32);\n"
                     "    1\n"
                     "  } else {\n"
                     "    putchar('N' as i32);\n"
                     "    0\n"
                     "  }\n"
                     "}",
                     1, "Y");
  }

  TEST_CASE("putchar in else branch only") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  if false {\n"
                     "    putchar('Y' as i32);\n"
                     "    1\n"
                     "  } else {\n"
                     "    putchar('N' as i32);\n"
                     "    0\n"
                     "  }\n"
                     "}",
                     0, "N");
  }

  TEST_CASE("comparison-driven branch with putchar") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let x = 5;\n"
                     "  if x > 3 {\n"
                     "    putchar('G' as i32);\n"
                     "    1\n"
                     "  } else {\n"
                     "    putchar('L' as i32);\n"
                     "    0\n"
                     "  }\n"
                     "}",
                     1, "G");
  }

  TEST_CASE("nested if with side effects in inner branches") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  if true {\n"
                     "    putchar('A' as i32);\n"
                     "    if false {\n"
                     "      putchar('B' as i32);\n"
                     "      0\n"
                     "    } else {\n"
                     "      putchar('C' as i32);\n"
                     "      1\n"
                     "    }\n"
                     "  } else {\n"
                     "    putchar('D' as i32);\n"
                     "    0\n"
                     "  }\n"
                     "}",
                     1, "AC");
  }

  TEST_CASE("else-if chain with side effects") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let x = 2;\n"
                     "  if x == 1 {\n"
                     "    putchar('1' as i32); 1\n"
                     "  } else {\n"
                     "    if x == 2 {\n"
                     "      putchar('2' as i32); 2\n"
                     "    } else {\n"
                     "      putchar('?' as i32); 0\n"
                     "    }\n"
                     "  }\n"
                     "}",
                     2, "2");
  }

  TEST_CASE("side effects before and after if expression") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar('[' as i32);\n"
                     "  let x = if true {\n"
                     "    putchar('T' as i32);\n"
                     "    42\n"
                     "  } else {\n"
                     "    putchar('F' as i32);\n"
                     "    0\n"
                     "  };\n"
                     "  putchar(']' as i32);\n"
                     "  x\n"
                     "}",
                     42, "[T]");
  }

  // --- Short-circuit evaluation with side effects --------------------------
  //
  // With putchar, we can *prove* that && and || short-circuit rather than
  // eagerly evaluating both sides.

  TEST_CASE("&& short-circuits: false && <side effect> skips RHS") {
    // If short-circuit works, putchar should NOT be called.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn side_effect() -> bool {\n"
                     "  putchar('!' as i32);\n"
                     "  true\n"
                     "}\n"
                     "fn main() -> i64 {\n"
                     "  if false && side_effect() { 1 } else { 0 }\n"
                     "}",
                     0, "");
  }

  TEST_CASE("&& evaluates RHS when LHS is true") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn side_effect() -> bool {\n"
                     "  putchar('!' as i32);\n"
                     "  true\n"
                     "}\n"
                     "fn main() -> i64 {\n"
                     "  if true && side_effect() { 1 } else { 0 }\n"
                     "}",
                     1, "!");
  }

  TEST_CASE("|| short-circuits: true || <side effect> skips RHS") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn side_effect() -> bool {\n"
                     "  putchar('!' as i32);\n"
                     "  false\n"
                     "}\n"
                     "fn main() -> i64 {\n"
                     "  if true || side_effect() { 1 } else { 0 }\n"
                     "}",
                     1, "");
  }

  TEST_CASE("|| evaluates RHS when LHS is false") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn side_effect() -> bool {\n"
                     "  putchar('!' as i32);\n"
                     "  true\n"
                     "}\n"
                     "fn main() -> i64 {\n"
                     "  if false || side_effect() { 1 } else { 0 }\n"
                     "}",
                     1, "!");
  }

  TEST_CASE("chained && short-circuits at first false") {
    // a() && b() && c(): if b() returns false, c() should not run
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn a() -> bool { putchar('a' as i32); true }\n"
                     "fn b() -> bool { putchar('b' as i32); false }\n"
                     "fn c() -> bool { putchar('c' as i32); true }\n"
                     "fn main() -> i64 {\n"
                     "  if a() && b() && c() { 1 } else { 0 }\n"
                     "}",
                     0, "ab");
  }

  TEST_CASE("chained || short-circuits at first true") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn a() -> bool { putchar('a' as i32); false }\n"
                     "fn b() -> bool { putchar('b' as i32); true }\n"
                     "fn c() -> bool { putchar('c' as i32); false }\n"
                     "fn main() -> i64 {\n"
                     "  if a() || b() || c() { 1 } else { 0 }\n"
                     "}",
                     1, "ab");
  }

  // --- Nested function calls through extern --------------------------------

  TEST_CASE("function result passed to putchar") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn get_char() -> i32 { 'X' as i32 }\n"
                     "fn main() -> i64 {\n"
                     "  putchar(get_char());\n"
                     "  0\n"
                     "}",
                     0, "X");
  }

  TEST_CASE("arithmetic result cast and passed to putchar") {
    // 'A' is 65, 65 + 3 = 68 = 'D'
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  putchar(('A' as i64 + 3) as i32);\n"
                     "  0\n"
                     "}",
                     0, "D");
  }

  TEST_CASE("putchar result chained into another putchar") {
    // putchar returns the char written. Feed it back.
    // putchar('A') returns 65 (i32), which is 'A' — so prints 'A' twice.
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let first = putchar('A' as i32);\n"
                     "  putchar(first);\n"
                     "  0\n"
                     "}",
                     0, "AA");
  }

  TEST_CASE("recursive function with side effects") {
    // Count down from 3, printing each digit
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn countdown(n: i64) -> i64 {\n"
                     "  if n == 0 { 0 }\n"
                     "  else {\n"
                     "    putchar(('0' as i64 + n) as i32);\n"
                     "    countdown(n - 1)\n"
                     "  }\n"
                     "}\n"
                     "fn main() -> i64 { countdown(3) }",
                     0, "321");
  }

  TEST_CASE("cast comparison drives branch with side effect") {
    CHECK_RUN_OUTPUT("extern fn putchar(c: i32) -> i32;\n"
                     "fn main() -> i64 {\n"
                     "  let c: char = 'Y';\n"
                     "  if c as i64 == 89 {\n"
                     "    putchar(c as i32);\n"
                     "    1\n"
                     "  } else {\n"
                     "    putchar('N' as i32);\n"
                     "    0\n"
                     "  }\n"
                     "}",
                     1, "Y");
  }

#else
  TEST_CASE("codegen side-effect tests" * doctest::skip()) {
    MESSAGE("lli not found at configure time - execution tests skipped");
  }
#endif
}

//****************************************************************************
} // namespace bust
//****************************************************************************
