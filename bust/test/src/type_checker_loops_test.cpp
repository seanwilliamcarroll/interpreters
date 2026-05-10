//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::TypeChecker — tuples (construction,
//*            type annotations, projection), while loops, and break
//*            expressions including break/lambda boundary rules.
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <hir/dump.hpp>
#include <hir/nodes.hpp>
#include <hir/type_unifier.hpp>
#include <hir/types.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <pipeline.hpp>
#include <type_checker.hpp>
#include <validate_main.hpp>

#include <algorithm>
#include <ranges>
#include <sstream>

#include <doctest/doctest.h>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.type_checker.loops") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program type_check(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{});
  }

#define DUMP_HIR(program) INFO("HIR:\n" << hir::Dumper::dump(program))

  // --- Tuple construction --------------------------------------------------

  TEST_CASE("tuple literal (1, 2) has type (i64, i64)") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t = (1, 2);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &kind = hir.m_type_arena.get(let.m_expression.m_type);
    REQUIRE(std::holds_alternative<hir::TupleType>(kind));
    const auto &tup = std::get<hir::TupleType>(kind);
    REQUIRE(tup.m_fields.size() == 2);
    for (const auto &field_id : tup.m_fields) {
      auto &f = hir.m_type_arena.get(field_id);
      REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(f));
      CHECK(std::get<hir::PrimitiveTypeValue>(f).m_type == PrimitiveType::I64);
    }
  }

  TEST_CASE("heterogeneous tuple (1, true) has type (i64, bool)") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t = (1, true);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &kind = hir.m_type_arena.get(let.m_expression.m_type);
    REQUIRE(std::holds_alternative<hir::TupleType>(kind));
    const auto &tup = std::get<hir::TupleType>(kind);
    REQUIRE(tup.m_fields.size() == 2);
    auto &f0 = hir.m_type_arena.get(tup.m_fields[0]);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(f0));
    CHECK(std::get<hir::PrimitiveTypeValue>(f0).m_type == PrimitiveType::I64);
    auto &f1 = hir.m_type_arena.get(tup.m_fields[1]);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(f1));
    CHECK(std::get<hir::PrimitiveTypeValue>(f1).m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("one-tuple (42,) has type (i64,) with arity 1") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t = (42,);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &kind = hir.m_type_arena.get(let.m_expression.m_type);
    REQUIRE(std::holds_alternative<hir::TupleType>(kind));
    const auto &tup = std::get<hir::TupleType>(kind);
    REQUIRE(tup.m_fields.size() == 1);
    auto &f0 = hir.m_type_arena.get(tup.m_fields[0]);
    CHECK(std::get<hir::PrimitiveTypeValue>(f0).m_type == PrimitiveType::I64);
  }

  TEST_CASE("nested tuple ((1, 2), true) has a TupleType as its first field") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t = ((1, 2), true);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &outer_kind = hir.m_type_arena.get(let.m_expression.m_type);
    REQUIRE(std::holds_alternative<hir::TupleType>(outer_kind));
    const auto &outer = std::get<hir::TupleType>(outer_kind);
    REQUIRE(outer.m_fields.size() == 2);
    auto &inner_kind = hir.m_type_arena.get(outer.m_fields[0]);
    REQUIRE(std::holds_alternative<hir::TupleType>(inner_kind));
    const auto &inner = std::get<hir::TupleType>(inner_kind);
    REQUIRE(inner.m_fields.size() == 2);
    auto &outer_f1 = hir.m_type_arena.get(outer.m_fields[1]);
    CHECK(std::get<hir::PrimitiveTypeValue>(outer_f1).m_type ==
          PrimitiveType::BOOL);
  }

  // --- Tuple type annotations ----------------------------------------------

  TEST_CASE("let with matching tuple annotation typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t: (i64, bool) = (1, true);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &kind = hir.m_type_arena.get(let.m_variable.m_type);
    REQUIRE(std::holds_alternative<hir::TupleType>(kind));
    CHECK(std::get<hir::TupleType>(kind).m_fields.size() == 2);
  }

  TEST_CASE("let tuple annotation with wrong arity throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let t: (i64,) = (1, 2);\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("let tuple annotation with mismatched element type throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let t: (i64, bool) = (1, 2);\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Tuple projection (dot) ---------------------------------------------

  TEST_CASE("projection t.0 on (i64, bool) has type i64") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t = (1, true);\n"
                          "  t.0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &expr = *func.m_body.m_final_expression;
    auto &kind = hir.m_type_arena.get(expr.m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(kind).m_type == PrimitiveType::I64);
  }

  TEST_CASE("projection t.1 on (i64, bool) has type bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t = (1, true);\n"
                          "  let b: bool = t.1;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[1]);
    auto &kind = hir.m_type_arena.get(let.m_expression.m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(kind).m_type ==
          PrimitiveType::BOOL);
  }

  TEST_CASE("projection of a nested tuple yields a TupleType") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let t = ((1, 2), true);\n"
                          "  t.0.0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &expr = *func.m_body.m_final_expression;
    auto &kind = hir.m_type_arena.get(expr.m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(kind).m_type == PrimitiveType::I64);
  }

  TEST_CASE("projection with out-of-bounds index throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let t = (1, true);\n"
                               "  let bad = t.2;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("projection on non-tuple target throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i64 = 1;\n"
                               "  let bad = x.0;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE(
      "projection on unresolved type variable is accepted at type-check") {
    // Lambda parameter `x` has no annotation — its type is a fresh type
    // variable. The type checker accepts `x.0` and defers the "is it a
    // tuple? is the index in range?" check to monomorphization, when `x`
    // has been substituted with a concrete type.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let f = |x| { x.0 };\n"
                             "  0\n"
                             "}"));
  }

  // --- While ---------------------------------------------------------------
  //
  // `main` must return i64, so structural-introspection tests put the
  // while/break logic in a separate function (whose body may be Unit) and
  // pair it with a trivial `fn main() -> i64 { 0 }`. CHECK_THROWS_AS /
  // CHECK_NOTHROW tests put the logic inside main itself with a trailing
  // `0` to satisfy the i64 return.

  TEST_CASE("while expression has unit type") {
    auto hir = type_check("fn looper() {\n"
                          "  while true { }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &expr = *func.m_body.m_final_expression;
    REQUIRE(std::holds_alternative<std::unique_ptr<hir::WhileExpr>>(
        expr.m_expression));
    auto &ptype =
        std::get<hir::PrimitiveTypeValue>(hir.m_type_arena.get(expr.m_type));
    CHECK(ptype.m_type == PrimitiveType::UNIT);
  }

  TEST_CASE("while condition and body are typed inside the HIR node") {
    auto hir = type_check("fn looper() {\n"
                          "  while true { }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &expr = *func.m_body.m_final_expression;
    auto &while_expr =
        *std::get<std::unique_ptr<hir::WhileExpr>>(expr.m_expression);
    auto &cond_ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(while_expr.m_condition.m_type));
    CHECK(cond_ptype.m_type == PrimitiveType::BOOL);
    auto &body_ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(while_expr.m_body.m_type));
    CHECK(body_ptype.m_type == PrimitiveType::UNIT);
  }

  TEST_CASE("while with non-bool condition throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while 42 { }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("while with non-unit body throws") {
    // Body's final expression is `42` (i64), not `()`. Same rule as a
    // no-`else` if: the body must type as unit.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while true { 42 }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("while body with trailing-semicolon expression typechecks") {
    // `42;` is a statement, so the block has no final expression and types
    // as unit.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { 42; }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("while body may contain let bindings") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { let x: i64 = 1; }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("while body bindings do not leak out (fresh scope)") {
    // `x` is bound only inside the while body; referencing it after the
    // while exits must fail with an undeclared-identifier error.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while false { let x: i64 = 1; }\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("while with comparison condition typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let x: i64 = 0;\n"
                             "  while x < 10 { }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("while as a statement followed by a final expression") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  while true { }\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &final_ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(final_ptype.m_type == PrimitiveType::I64);
  }

  // --- Break ---------------------------------------------------------------

  TEST_CASE("break inside while body typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { break; }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("break expression has Never type") {
    auto hir = type_check("fn looper() {\n"
                          "  while true { break; }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &expr = *func.m_body.m_final_expression;
    auto &while_expr =
        *std::get<std::unique_ptr<hir::WhileExpr>>(expr.m_expression);
    REQUIRE(while_expr.m_body.m_statements.size() == 1);
    auto &stmt = std::get<hir::Expression>(while_expr.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<hir::BreakExpr>>(
        stmt.m_expression));
    CHECK(std::holds_alternative<hir::NeverType>(
        hir.m_type_arena.get(stmt.m_type)));
  }

  TEST_CASE("break outside any loop throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  break;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break inside if inside while typechecks") {
    // The loop-scope check walks up through nested non-loop scopes,
    // so an `if` inside a `while` should not block `break`.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { if true { break; } }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("break inside bare-block inside while typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { { break; } }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("break inside inner of nested while typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { while true { break; } }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("break after the loop ends throws — outside scope again") {
    // The two statements: `while true {}` and `break;`. The break is at
    // the function body level, not inside the loop, so it must be
    // rejected.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while true { }\n"
                               "  break;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break as block final expression typechecks (Never)") {
    // `break` with no semicolon makes the body's final expression Never;
    // Never unifies with Unit so the body still types as Unit.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { break }\n"
                             "  0\n"
                             "}"));
  }

  // --- Break and lambda boundaries -----------------------------------------
  //
  // A lambda body is its own function and therefore a hard boundary for
  // loop context: `break` inside a lambda must target a loop *inside that
  // lambda's own body*, never a loop in the enclosing function. This
  // requires the loop environment to mark function/lambda boundaries so
  // the loop-scope search stops at them rather than walking through.

  TEST_CASE("break inside top-level lambda throws — not in a loop") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = || { break; };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break inside lambda defined inside while throws — "
            "lambda is a loop-context boundary") {
    // Lexically the break is enclosed by a `while`, but it sits inside a
    // lambda body, so it does NOT target the surrounding loop. Must be
    // rejected.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while true {\n"
                               "    let f = || { break; };\n"
                               "  }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break sibling to lambda inside while typechecks") {
    // Sanity check: the break is in the while body itself (not inside the
    // lambda), so it targets the enclosing while normally.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true {\n"
                             "    let f = || { 0 };\n"
                             "    break;\n"
                             "  }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("break inside while inside lambda typechecks — "
            "innermost loop wins") {
    // `break` is inside a `while` that is inside a lambda; the lambda's
    // own loop satisfies the loop-context requirement.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let f = || { while true { break; } };\n"
                             "  0\n"
                             "}"));
  }

  // --- Loop ---------------------------------------------------------------
  //
  // `loop {}` is an unconditional infinite loop whose value comes only
  // from `break expr` exits. With no breaks, the loop's type is Never
  // (it genuinely never produces a value); with breaks, all break
  // payloads must unify and that unified type is the loop's type.
  // `break;` sugars to `break ();`, so a loop with only bare breaks
  // types as unit. The loop body itself, like a while body, must type
  // as unit.

  TEST_CASE("loop with no break has Never type") {
    auto hir = type_check("fn looper() {\n"
                          "  loop { }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &expr = *func.m_body.m_final_expression;
    REQUIRE(std::holds_alternative<std::unique_ptr<hir::LoopExpr>>(
        expr.m_expression));
    CHECK(std::holds_alternative<hir::NeverType>(
        hir.m_type_arena.get(expr.m_type)));
  }

  TEST_CASE("loop with bare break has unit type") {
    // break; sugars to break (); so the only collected payload is unit.
    auto hir = type_check("fn looper() {\n"
                          "  loop { break; }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &expr = *func.m_body.m_final_expression;
    auto &kind = hir.m_type_arena.get(expr.m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(kind).m_type ==
          PrimitiveType::UNIT);
  }

  TEST_CASE("loop with break <i64> has i64 type") {
    auto hir = type_check("fn looper() -> i64 {\n"
                          "  loop { break 42; }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &expr = *func.m_body.m_final_expression;
    auto &kind = hir.m_type_arena.get(expr.m_type);
    REQUIRE(std::holds_alternative<hir::PrimitiveTypeValue>(kind));
    CHECK(std::get<hir::PrimitiveTypeValue>(kind).m_type == PrimitiveType::I64);
  }

  TEST_CASE("loop with break <bool> binds to bool let") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let b: bool = loop { break true; };\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("loop value can be bound to a let with matching annotation") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let x: i64 = loop { break 5; };\n"
                             "  x\n"
                             "}"));
  }

  TEST_CASE("loop value used directly as final expression typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  loop { break 7; }\n"
                             "}"));
  }

  TEST_CASE("loop with mismatched break payloads throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  loop { break 1; break true; };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("loop with multiple matching breaks unifies (i64)") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let x: i64 = loop {\n"
                             "    if true { break 1; }\n"
                             "    break 2;\n"
                             "  };\n"
                             "  x\n"
                             "}"));
  }

  TEST_CASE("loop with break payload of mismatched annotation throws") {
    // `let x: bool = loop { break 1; };` — break's payload is i64 but the
    // let expects bool. Mismatch should fail.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: bool = loop { break 1; };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break expression inside loop has Never type") {
    // The break expression itself is divergent, regardless of payload.
    auto hir = type_check("fn looper() -> i64 {\n"
                          "  loop { break 42; }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &expr = *func.m_body.m_final_expression;
    auto &loop_expr =
        *std::get<std::unique_ptr<hir::LoopExpr>>(expr.m_expression);
    REQUIRE(loop_expr.m_body.m_statements.size() == 1);
    auto &stmt = std::get<hir::Expression>(loop_expr.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<hir::BreakExpr>>(
        stmt.m_expression));
    CHECK(std::holds_alternative<hir::NeverType>(
        hir.m_type_arena.get(stmt.m_type)));
  }

  TEST_CASE("loop body bindings do not leak (fresh scope)") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  loop { let x: i64 = 1; break; };\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("loop body with trailing-semicolon expression typechecks") {
    // `42;` is a discarded statement; the body still types as unit
    // (followed by a Never-typed break) so the loop-body-must-unify-unit
    // check is satisfied.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  loop { 42; break; };\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("loop with non-unit body throws") {
    // Same rule as while: the body's final expression must type as unit.
    // Here trailing 42 makes the body's type i64.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  loop { 42 };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("loop containing only return typechecks (loop type Never)") {
    // The loop has Never type (no breaks contribute), and `return` exits
    // the function. main's i64 return type is satisfied by the return,
    // and the loop-as-final-expression type Never unifies with i64.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  loop { return 42; }\n"
                             "}"));
  }

  TEST_CASE("loop with no break body satisfies any function return type") {
    // A loop with no breaks has type Never, compatible with any expected
    // return type (here, i64).
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  loop { }\n"
                             "}"));
  }

  // --- Continue -----------------------------------------------------------

  TEST_CASE("continue inside while body typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { continue; }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("continue inside loop body typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  loop { continue; }\n"
                             "}"));
  }

  TEST_CASE("continue expression has Never type") {
    auto hir = type_check("fn looper() {\n"
                          "  while true { continue; }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &expr = *func.m_body.m_final_expression;
    auto &while_expr =
        *std::get<std::unique_ptr<hir::WhileExpr>>(expr.m_expression);
    REQUIRE(while_expr.m_body.m_statements.size() == 1);
    auto &stmt = std::get<hir::Expression>(while_expr.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<hir::ContinueExpr>>(
        stmt.m_expression));
    CHECK(std::holds_alternative<hir::NeverType>(
        hir.m_type_arena.get(stmt.m_type)));
  }

  TEST_CASE("continue inside if inside while typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true { if true { continue; } }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("continue outside any loop throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  continue;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("continue after the loop ends throws — outside scope again") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while true { }\n"
                               "  continue;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("continue inside top-level lambda throws — not in a loop") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = || { continue; };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("continue inside lambda inside while throws — "
            "lambda is a loop-context boundary") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while true {\n"
                               "    let f = || { continue; };\n"
                               "  }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("continue sibling to lambda inside while typechecks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true {\n"
                             "    let f = || { 0 };\n"
                             "    continue;\n"
                             "  }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("continue inside while inside lambda typechecks — "
            "innermost loop wins") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let f = || { while true { continue; } };\n"
                             "  0\n"
                             "}"));
  }

  // --- Break with value ---------------------------------------------------
  //
  // `break expr` is only valid inside `loop`; inside `while`/`for` the
  // loop's break-target type is fixed to unit, so any non-unit payload
  // fails unification at the enclosing while/for. Within a `loop`, all
  // break payloads must unify; a nested loop's breaks belong to THAT
  // loop, not the outer one.

  TEST_CASE("break with i64 value inside while throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while true { break 42; }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break with bool value inside while throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  while true { break true; }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break with tuple value in loop typechecks") {
    CHECK_NOTHROW(
        type_check("fn main() -> i64 {\n"
                   "  let p: (i64, bool) = loop { break (1, true); };\n"
                   "  0\n"
                   "}"));
  }

  TEST_CASE("nested loops: inner break unit, outer break i64 typechecks") {
    // Inner break targets inner loop (no payload → unit). Outer break
    // targets outer loop with i64 payload. The two collected-type sets
    // are independent.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let x: i64 = loop {\n"
                             "    loop { break; };\n"
                             "    break 7;\n"
                             "  };\n"
                             "  x\n"
                             "}"));
  }

  TEST_CASE("nested loops: each loop's breaks unify independently") {
    // Inner loop's break is bool → inner loop has type bool.
    // Outer loop's break is i64 → outer loop has type i64.
    // No cross-contamination.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let x: i64 = loop {\n"
                             "    let b: bool = loop { break true; };\n"
                             "    break 1;\n"
                             "  };\n"
                             "  x\n"
                             "}"));
  }

  TEST_CASE("break with value in loop nested inside while typechecks") {
    // The inner break is inside `loop`, so its i64 payload is valid
    // there. The outer while never sees the inner break.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  while true {\n"
                             "    let x: i64 = loop { break 1; };\n"
                             "  }\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("nested loops with mismatched outer breaks throws") {
    // Inner loop's break is independent; outer's two breaks must unify.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  loop {\n"
                               "    loop { break true; };\n"
                               "    break 1;\n"
                               "    break true;\n"
                               "  };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break inside lambda inside loop throws — "
            "lambda is a boundary") {
    // The lambda body has no enclosing loop of its own. break inside
    // it must be rejected even though the lambda is lexically inside a
    // loop.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  loop {\n"
                               "    let f = || { break 42; };\n"
                               "    break;\n"
                               "  };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break with payload inside top-level lambda throws") {
    // No enclosing loop anywhere — break with or without payload fails.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = || { break 42; };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("break with payload inside lambda inside loop, nested loop "
            "inside lambda typechecks — innermost loop wins") {
    // The break is inside a `loop` that is inside a lambda. The lambda's
    // own loop satisfies the loop-context requirement; the outer loop is
    // not the target.
    CHECK_NOTHROW(
        type_check("fn main() -> i64 {\n"
                   "  loop {\n"
                   "    let f = || { let x: i64 = loop { break 9; }; };\n"
                   "    break;\n"
                   "  };\n"
                   "  0\n"
                   "}"));
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
