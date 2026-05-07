//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::TypeChecker — block expressions, naked
//*            return, multiple-return paths, block divergence, nested
//*            lambdas, and variable shadowing across scopes.
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
TEST_SUITE("bust.type_checker.blocks") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program type_check(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{});
  }

#define DUMP_HIR(program) INFO("HIR:\n" << hir::Dumper::dump(program))

  // --- Block expression types ------------------------------------------------

  TEST_CASE("block expression has type of its final expression") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = {\n"
                          "    let a = 1;\n"
                          "    a + 2\n"
                          "  };\n"
                          "  x\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("empty block has type unit") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = {};\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::UNIT);
  }

  // --- Calling non-callable --------------------------------------------------

  TEST_CASE("calling an i64 value throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 42;\n"
                               "  x(1)\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("calling a bool value throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = true;\n"
                               "  x(1)\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Lambda with no parameters ---------------------------------------------

  TEST_CASE("lambda with no parameters typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = || { 42 };\n"
                          "  f()\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Return in nested block ------------------------------------------------

  TEST_CASE("return in nested block matches function return type") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  {\n"
                          "    return 42;\n"
                          "  }\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("return in nested block with wrong type throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  {\n"
                               "    return true;\n"
                               "  }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Multiple returns ------------------------------------------------------

  TEST_CASE("multiple return paths with consistent types") {
    auto hir = type_check("fn classify(n: i64) -> i64 {\n"
                          "  if n > 0 { return 1; }\n"
                          "  if n < 0 { return -1; }\n"
                          "  0\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
  }

  TEST_CASE("multiple returns with inconsistent types throws") {
    CHECK_THROWS_AS(type_check("fn bad(n: i64) -> i64 {\n"
                               "  if n > 0 { return true; }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Block divergence (semicolon return) ---------------------------------
  //
  // A block whose final-expression slot is empty but whose statements contain
  // a divergent expression (`return`, type Never) has block type Never, not
  // Unit. Without this rule, `fn foo() -> i64 { return 42; }` is rejected
  // because the block is Unit and the function expects i64. Never unifies
  // with any T, so once the block type is Never the existing return-type
  // check passes unchanged.

  TEST_CASE("block with only return statement matches any return type") {
    auto hir = type_check("fn main() -> i64 { return 42; }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("block with let then trailing return statement type-checks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = 5;\n"
                          "  return x;\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE(
      "block with diverging let RHS and no final expression type-checks") {
    // The let statement's contained expression (the RHS) has type Never.
    // Divergence detection must look inside let bindings, not only into
    // expression-statements.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = return 42;\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("block with diverging assignment RHS and no final expression "
            "type-checks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let mut x = 0;\n"
                          "  x = return 42;\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("nested block with only return statement has never type") {
    // Inner `{ return 42; }` has no final expression but a divergent
    // statement, so its type is Never. The outer block uses the inner as
    // its final expression and inherits Never, which unifies with i64.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  { return 42; }\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("if-else with semicolon returns in both arms type-checks") {
    // Both arm-blocks have only a divergent statement and no final
    // expression — each is Never, the if-else unifies to Never, and the
    // outer block (with the if-else as its final expression) is Never.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  if true { return 1; } else { return 2; }\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("non-diverging block without final expression is still unit") {
    // Sanity: the divergence rule doesn't make every block Never. A block
    // whose only statement is a non-diverging let still has type Unit, so
    // a function returning i64 should reject it.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 5;\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Naked return (no operand) -------------------------------------------
  //
  // `return;` desugars to `return ();`. A naked return is well-typed iff
  // the enclosing function's declared return type is Unit.

  TEST_CASE("naked return in unit function type-checks") {
    auto hir = type_check("fn helper() { return; }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
  }

  TEST_CASE("naked return in non-unit function throws") {
    // `return;` returns `()`, which doesn't unify with the declared `i64`.
    CHECK_THROWS_AS(type_check("fn main() -> i64 { return; }"),
                    core::CompilerException);
  }

  TEST_CASE("naked return after let in unit function type-checks") {
    auto hir = type_check("fn helper() {\n"
                          "  let x = 5;\n"
                          "  return;\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
  }

  TEST_CASE("naked return as final expression in unit function type-checks") {
    // `return` (no semicolon) is the block's final expression. Its type is
    // Never; block type Never; unifies with declared Unit.
    auto hir = type_check("fn helper() { return }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
  }

  TEST_CASE("early naked return guarded by if type-checks") {
    auto hir = type_check("fn helper(x: bool) {\n"
                          "  if x { return; }\n"
                          "  let y = 5;\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
  }

  // --- Nested lambda ---------------------------------------------------------

  TEST_CASE("nested lambda typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x: i64| -> i64 {\n"
                          "    let g = |y: i64| -> i64 { x + y };\n"
                          "    g(10)\n"
                          "  };\n"
                          "  f(5)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Variable shadowing across scopes ------------------------------------

  TEST_CASE("shadowing in inner block scope uses new type") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = true;\n"
                          "  {\n"
                          "    let x = 42;\n"
                          "    x + 1\n"
                          "  }\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("same-scope shadowing uses new type") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = true;\n"
                          "  let x = 42;\n"
                          "  x + 1\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("outer variable accessible after inner scope ends") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = true;\n"
                          "  {\n"
                          "    let x = 42;\n"
                          "  };\n"
                          "  let y: bool = x;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[2]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
