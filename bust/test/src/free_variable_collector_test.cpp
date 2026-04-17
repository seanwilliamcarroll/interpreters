//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::zir::FreeVariableCollector
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <exceptions.hpp>
#include <lexer.hpp>
#include <monomorpher.hpp>
#include <parser.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>
#include <zir/dump.hpp>
#include <zir/nodes.hpp>
#include <zir/program.hpp>
#include <zir/types.hpp>
#include <zir_lowerer.hpp>

#include <algorithm>
#include <set>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.free_variable_collector") {

  // --- Helpers -------------------------------------------------------------

  // Full pipeline: parse -> validate -> typecheck -> mono -> zir
  static zir::Program lower_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{},
                        Monomorpher{}, ZirLowerer{});
  }

  // Retrieve the first FunctionDef from the program's top items.
  static const zir::FunctionDef &first_function(const zir::Program &program) {
    return std::get<zir::FunctionDef>(program.m_top_items.at(0));
  }

  // Retrieve the ExprKind stored in the arena for a given ExprId.
  static const zir::ExprKind &expr_kind(const zir::Program &program,
                                        zir::ExprId id) {
    return program.m_arena.get(id).m_expr_kind;
  }

  // Retrieve a Binding from the arena.
  static const zir::Binding &binding(const zir::Program &program,
                                     zir::BindingId id) {
    return program.m_arena.get(id);
  }

  // Extract sorted capture names from a LambdaExpr for deterministic checks.
  static std::set<std::string> capture_names(
      const zir::Program &program, const zir::LambdaExpr &lambda_expr) {
    std::set<std::string> names;
    for (const auto &capture : lambda_expr.m_captures) {
      names.insert(binding(program, capture.m_id).m_name);
    }
    return names;
  }

  // Format captures as a printable string for INFO diagnostics.
  static std::string captures_string(const zir::Program &program,
                                     const zir::LambdaExpr &lambda_expr) {
    if (lambda_expr.m_captures.empty()) {
      return "(none)";
    }
    std::string result;
    for (const auto &capture : lambda_expr.m_captures) {
      if (!result.empty())
        result += ", ";
      result += binding(program, capture.m_id).m_name;
    }
    return result;
  }

  // Find the LambdaExpr bound by a let statement at the given index in a
  // function body.
  static const zir::LambdaExpr &lambda_at(const zir::Program &program,
                                          const zir::FunctionDef &func,
                                          size_t statement_index) {
    const auto &let =
        std::get<zir::LetBinding>(func.m_body.m_statements.at(statement_index));
    const auto &kind = expr_kind(program, let.m_expression);
    return std::get<zir::LambdaExpr>(kind);
  }

  // --- No captures ---------------------------------------------------------

  TEST_CASE("lambda with no free variables has empty captures") {
    auto zir = lower_string(
        "fn main() -> i64 { let f = |x: i64| -> i64 { x + 1 }; f(41) }");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 0);
    INFO("captures: " << captures_string(zir, lambda));
    CHECK(lambda.m_captures.empty());
  }

  TEST_CASE("lambda using only its own parameter has no captures") {
    auto zir = lower_string(
        "fn main() -> i64 { let f = |x: i64| -> i64 { x * x }; f(5) }");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 0);
    INFO("captures: " << captures_string(zir, lambda));
    CHECK(lambda.m_captures.empty());
  }

  TEST_CASE("lambda with multiple parameters and no captures") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let f = |x: i64, y: i64| -> i64 { x + y };"
                            "  f(1, 2)"
                            "}");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 0);
    INFO("captures: " << captures_string(zir, lambda));
    CHECK(lambda.m_captures.empty());
  }

  // --- Single capture ------------------------------------------------------

  TEST_CASE("lambda captures one variable from enclosing scope") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let x = 10;"
                            "  let f = |y: i64| -> i64 { x + y };"
                            "  f(32)"
                            "}");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    INFO("captures: " << captures_string(zir, lambda));
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"x"});
  }

  TEST_CASE("lambda captures function parameter") {
    auto zir = lower_string("fn apply(x: i64) -> i64 {"
                            "  let f = |y: i64| -> i64 { x + y };"
                            "  f(1)"
                            "}"
                            "fn main() -> i64 { apply(41) }");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items.at(0));
    auto &lambda = lambda_at(zir, func, 0);
    INFO("captures: " << captures_string(zir, lambda));
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"x"});
  }

  // --- Multiple captures ---------------------------------------------------

  TEST_CASE("lambda captures multiple variables") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let a = 10;"
                            "  let b = 20;"
                            "  let f = |x: i64| -> i64 { a + b + x };"
                            "  f(12)"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 2);
    CHECK(lambda.m_captures.size() == 2);
    CHECK(capture_names(zir, lambda) == (std::set<std::string>{"a", "b"}));
  }

  TEST_CASE("lambda captures mix of function parameter and let binding") {
    auto zir = lower_string("fn compute(n: i64) -> i64 {"
                            "  let offset = 100;"
                            "  let f = |x: i64| -> i64 { n + offset + x };"
                            "  f(1)"
                            "}"
                            "fn main() -> i64 { compute(5) }");
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items.at(0));
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.size() == 2);
    CHECK(capture_names(zir, lambda) == (std::set<std::string>{"n", "offset"}));
  }

  // --- Shadowing -----------------------------------------------------------

  TEST_CASE("parameter shadows outer variable - no capture") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let x = 10;"
                            "  let f = |x: i64| -> i64 { x + 1 };"
                            "  f(41)"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.empty());
  }

  TEST_CASE("let inside lambda shadows outer variable - no capture of outer") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let x = 10;"
                            "  let f = || -> i64 { let x = 42; x };"
                            "  f()"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.empty());
  }

  // --- Captures in different expression contexts ---------------------------

  TEST_CASE("capture used in if condition") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let flag = true;"
                            "  let f = || -> i64 { if flag { 1 } else { 0 } };"
                            "  f()"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"flag"});
  }

  TEST_CASE("capture used in binary expression") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let base = 100;"
                            "  let f = |x: i64| -> i64 { base * x };"
                            "  f(5)"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"base"});
  }

  TEST_CASE("capture used in unary expression") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let x = 42;"
                            "  let f = || -> i64 { -x };"
                            "  f()"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"x"});
  }

  TEST_CASE("capture used as call argument") {
    auto zir = lower_string("fn id(x: i64) -> i64 { x }"
                            "fn main() -> i64 {"
                            "  let val = 42;"
                            "  let f = || -> i64 { id(val) };"
                            "  f()"
                            "}");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items.at(1));
    auto &lambda = lambda_at(zir, func, 1);
    INFO("captures: " << captures_string(zir, lambda));
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"val"});
  }

  TEST_CASE("capture used in return expression") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let x = 42;"
                            "  let f = || -> i64 { return x; 0 };"
                            "  f()"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"x"});
  }

  // --- Nested lambdas ------------------------------------------------------

  TEST_CASE("inner lambda captures from outer lambda scope") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let x = 10;"
                            "  let outer = |y: i64| -> i64 {"
                            "    let inner = || -> i64 { x + y };"
                            "    inner()"
                            "  };"
                            "  outer(32)"
                            "}");
    auto &func = first_function(zir);

    // outer lambda captures x from main
    auto &outer = lambda_at(zir, func, 1);
    CHECK(capture_names(zir, outer) == std::set<std::string>{"x"});

    // inner lambda is in outer's body — find it
    auto &inner_let =
        std::get<zir::LetBinding>(outer.m_body.m_statements.at(0));
    auto &inner_kind = expr_kind(zir, inner_let.m_expression);
    auto &inner = std::get<zir::LambdaExpr>(inner_kind);

    // inner captures both x and y
    CHECK(inner.m_captures.size() == 2);
    CHECK(capture_names(zir, inner) == (std::set<std::string>{"x", "y"}));
  }

  TEST_CASE("nested lambda only captures what it uses") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let a = 1;"
                            "  let b = 2;"
                            "  let outer = || -> i64 {"
                            "    let inner = || -> i64 { a };"
                            "    inner()"
                            "  };"
                            "  outer()"
                            "}");
    auto &func = first_function(zir);

    // outer captures a (because inner uses it) — but not b
    auto &outer = lambda_at(zir, func, 2);
    CHECK(capture_names(zir, outer) == std::set<std::string>{"a"});

    // inner captures a only
    auto &inner_let =
        std::get<zir::LetBinding>(outer.m_body.m_statements.at(0));
    auto &inner =
        std::get<zir::LambdaExpr>(expr_kind(zir, inner_let.m_expression));
    CHECK(inner.m_captures.size() == 1);
    CHECK(capture_names(zir, inner) == std::set<std::string>{"a"});
  }

  // --- Top-level functions are not captures ---------------------------------

  TEST_CASE("reference to top-level function is not a capture") {
    auto zir = lower_string("fn id(x: i64) -> i64 { x }"
                            "fn main() -> i64 {"
                            "  let val = 42;"
                            "  let f = || -> i64 { id(val) };"
                            "  f()"
                            "}");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items.at(1));
    auto &lambda = lambda_at(zir, func, 1);
    INFO("captures: " << captures_string(zir, lambda));
    // id is a top-level function — should not be captured; only val should be
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"val"});
  }

  TEST_CASE("reference to extern function is not a capture") {
    auto zir = lower_string("extern fn putchar(c: i32) -> i32;"
                            "fn main() -> i64 {"
                            "  let f = || -> i32 { putchar(72 as i32) };"
                            "  f();"
                            "  0"
                            "}");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items.at(1));
    auto &lambda = lambda_at(zir, func, 0);
    INFO("captures: " << captures_string(zir, lambda));
    // putchar is an extern — should not be captured
    CHECK(lambda.m_captures.empty());
  }

  TEST_CASE("shadowed top-level function name is captured") {
    auto zir = lower_string("fn id(x: i64) -> i64 { x }"
                            "fn main() -> i64 {"
                            "  let id = 42;"
                            "  let f = || -> i64 { id };"
                            "  f()"
                            "}");
    INFO("ZIR:\n" << zir::Dumper::dump(zir));
    auto &func = std::get<zir::FunctionDef>(zir.m_top_items.at(1));
    auto &lambda = lambda_at(zir, func, 1);
    INFO("captures: " << captures_string(zir, lambda));
    // id here refers to the let binding (i64), not the top-level function —
    // it has a different BindingId and should be captured
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"id"});
  }

  // --- Deduplication -------------------------------------------------------

  TEST_CASE("same variable used multiple times is captured once") {
    auto zir = lower_string("fn main() -> i64 {"
                            "  let x = 21;"
                            "  let f = || -> i64 { x + x };"
                            "  f()"
                            "}");
    auto &func = first_function(zir);
    auto &lambda = lambda_at(zir, func, 1);
    CHECK(lambda.m_captures.size() == 1);
    CHECK(capture_names(zir, lambda) == std::set<std::string>{"x"});
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
