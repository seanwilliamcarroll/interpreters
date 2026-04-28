//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Codegen tests for tuple construction, projection, and use as
//*            function arguments and return values. Storage strategy is
//*            stack alloca; tuple-returning functions return an SSA struct
//*            value that the caller stores into its own alloca.
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
TEST_SUITE("bust.codegen.tuples") {

  using namespace ::bust::test;

  // --- IR-shape sanity (no lli required) -----------------------------------

  TEST_CASE("tuple let binding emits an alloca of the struct type") {
    auto ir = codegen("fn main() -> i64 {\n"
                      "  let t = (1, 2);\n"
                      "  0\n"
                      "}");
    // Anonymous struct rendered inline as `{ i64, i64 }`.
    CHECK(ir.find("alloca { i64, i64 }") != std::string::npos);
  }

  TEST_CASE("tuple projection emits a getelementptr with the struct type") {
    auto ir = codegen("fn main() -> i64 {\n"
                      "  let t = (1, 2);\n"
                      "  t.0\n"
                      "}");
    CHECK(ir.find("getelementptr { i64, i64 }") != std::string::npos);
  }

  TEST_CASE("tuple-returning function has aggregate return type in signature") {
    auto ir = codegen("fn make_pair() -> (i64, i64) { (1, 2) }\n"
                      "fn main() -> i64 { 0 }");
    CHECK(ir.find("define { i64, i64 } @make_pair") != std::string::npos);
  }

#ifdef BUST_LLI_PATH

  // --- 2-tuple construction + projection -----------------------------------

  TEST_CASE("project field 0 of a 2-tuple") {
    CHECK_RUN("fn main() -> i64 { let t = (42, 7); t.0 }", 42);
  }

  TEST_CASE("project field 1 of a 2-tuple") {
    CHECK_RUN("fn main() -> i64 { let t = (7, 42); t.1 }", 42);
  }

  TEST_CASE("one-tuple construction and projection") {
    CHECK_RUN("fn main() -> i64 { let t = (42,); t.0 }", 42);
  }

  TEST_CASE("3-tuple projection of each field") {
    CHECK_RUN("fn main() -> i64 { let t = (10, 20, 42); t.2 }", 42);
    CHECK_RUN("fn main() -> i64 { let t = (10, 42, 30); t.1 }", 42);
    CHECK_RUN("fn main() -> i64 { let t = (42, 20, 30); t.0 }", 42);
  }

  // --- Heterogeneous tuples ------------------------------------------------

  TEST_CASE("heterogeneous tuple (i64, bool): project i64 field") {
    CHECK_RUN("fn main() -> i64 { let t = (42, true); t.0 }", 42);
  }

  TEST_CASE("heterogeneous tuple (i64, bool): use bool field in if") {
    CHECK_RUN(
        "fn main() -> i64 { let t = (42, true); if t.1 { t.0 } else { 0 } }",
        42);
    CHECK_RUN(
        "fn main() -> i64 { let t = (42, false); if t.1 { 0 } else { t.0 } }",
        42);
  }

  TEST_CASE("heterogeneous tuple (bool, i64): project i64 field") {
    CHECK_RUN("fn main() -> i64 { let t = (true, 42); t.1 }", 42);
  }

  // --- Nested tuples + chained projection ----------------------------------

  TEST_CASE("nested tuple: project outer then inner with t.0.1") {
    CHECK_RUN("fn main() -> i64 { let t = ((1, 42), 7); t.0.1 }", 42);
  }

  TEST_CASE("nested tuple: project outer then inner with t.1.0") {
    CHECK_RUN("fn main() -> i64 { let t = (7, (42, 1)); t.1.0 }", 42);
  }

  TEST_CASE("doubly-nested tuple chained projection t.1.1") {
    CHECK_RUN("fn main() -> i64 { let t = ((1, 2), (3, 42)); t.1.1 }", 42);
  }

  TEST_CASE("inner tuple bound to let, then projected") {
    CHECK_RUN("fn main() -> i64 {\n"
              "  let outer = ((1, 42), 7);\n"
              "  let inner = outer.0;\n"
              "  inner.1\n"
              "}",
              42);
  }

  // --- Tuple passed as a function argument ---------------------------------

  TEST_CASE("function takes tuple, returns first field") {
    CHECK_RUN("fn first(t: (i64, i64)) -> i64 { t.0 }\n"
              "fn main() -> i64 { first((42, 7)) }",
              42);
  }

  TEST_CASE("function takes tuple, returns second field") {
    CHECK_RUN("fn second(t: (i64, i64)) -> i64 { t.1 }\n"
              "fn main() -> i64 { second((7, 42)) }",
              42);
  }

  TEST_CASE("function takes heterogeneous tuple") {
    CHECK_RUN("fn pick(t: (i64, bool)) -> i64 { if t.1 { t.0 } else { 0 } }\n"
              "fn main() -> i64 { pick((42, true)) }",
              42);
  }

  TEST_CASE("function takes tuple bound to a let at call site") {
    CHECK_RUN("fn first(t: (i64, i64)) -> i64 { t.0 }\n"
              "fn main() -> i64 {\n"
              "  let t = (42, 7);\n"
              "  first(t)\n"
              "}",
              42);
  }

  // --- Tuple returned from a function --------------------------------------

  TEST_CASE("function returns tuple, project at call site (.0)") {
    CHECK_RUN("fn make_pair() -> (i64, i64) { (42, 7) }\n"
              "fn main() -> i64 { make_pair().0 }",
              42);
  }

  TEST_CASE("function returns tuple, project at call site (.1)") {
    CHECK_RUN("fn make_pair() -> (i64, i64) { (7, 42) }\n"
              "fn main() -> i64 { make_pair().1 }",
              42);
  }

  TEST_CASE("function returns tuple, bind to let, project later") {
    CHECK_RUN("fn make_pair() -> (i64, i64) { (42, 7) }\n"
              "fn main() -> i64 {\n"
              "  let p = make_pair();\n"
              "  p.0\n"
              "}",
              42);
  }

  TEST_CASE("function returns heterogeneous tuple, project bool field") {
    CHECK_RUN("fn make() -> (i64, bool) { (42, true) }\n"
              "fn main() -> i64 {\n"
              "  let p = make();\n"
              "  if p.1 { p.0 } else { 0 }\n"
              "}",
              42);
  }

  TEST_CASE("function returns nested tuple, chained projection at call site") {
    CHECK_RUN("fn make() -> ((i64, i64), i64) { ((1, 42), 7) }\n"
              "fn main() -> i64 { make().0.1 }",
              42);
  }

  TEST_CASE("tuple round-trips through a function") {
    CHECK_RUN("fn id(t: (i64, i64)) -> (i64, i64) { t }\n"
              "fn main() -> i64 { id((42, 7)).0 }",
              42);
  }

  // --- Projection used in arithmetic / control flow ------------------------

  TEST_CASE("projection used in addition") {
    CHECK_RUN("fn main() -> i64 { let t = (40, 2); t.0 + t.1 }", 42);
  }

  TEST_CASE("projection used in multiplication") {
    CHECK_RUN("fn main() -> i64 { let t = (6, 7); t.0 * t.1 }", 42);
  }

  TEST_CASE("projection used as if-condition value") {
    CHECK_RUN("fn main() -> i64 {\n"
              "  let t = (3, 14);\n"
              "  if t.0 < t.1 { t.1 * 3 } else { t.0 }\n"
              "}",
              42);
  }

  TEST_CASE("multiple projections combined in one expression") {
    CHECK_RUN("fn main() -> i64 {\n"
              "  let t = (10, 20, 12);\n"
              "  t.0 + t.1 + t.2\n"
              "}",
              42);
  }

  TEST_CASE("projection of call result used in arithmetic") {
    CHECK_RUN("fn make_pair() -> (i64, i64) { (40, 2) }\n"
              "fn main() -> i64 { make_pair().0 + make_pair().1 }",
              42);
  }

#endif // BUST_LLI_PATH
}

//****************************************************************************
} // namespace bust
//****************************************************************************
