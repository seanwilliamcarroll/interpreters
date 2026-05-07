//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::TypeChecker — lambdas, type inference
//*            (parameter and return types, call-site unification),
//*            let-polymorphism, scoping, forward references, and
//*            mixed-annotated parameters.
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
TEST_SUITE("bust.type_checker.inference") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program type_check(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{});
  }

#define DUMP_HIR(program) INFO("HIR:\n" << hir::Dumper::dump(program))

  // --- Lambda expressions ----------------------------------------------------

  TEST_CASE("lambda with annotated types typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let add = |a: i64, b: i64| -> i64 { a + b };\n"
                          "  add(1, 2)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("lambda body type must match return annotation") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let bad = |x: i64| -> bool { x };\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("lambda with wrong argument type at call site throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x: i64| -> i64 { x };\n"
                               "  f(true)\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Type inference (lambda return type) -----------------------------------

  TEST_CASE("infer lambda return type from body") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x: i64| { x + 1 };\n"
                          "  f(10)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("infer lambda return type as bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let is_zero = |x: i64| { x == 0 };\n"
                          "  let result: bool = is_zero(5);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[1]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("infer lambda return type mismatch with usage throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x: i64| { x + 1 };\n"
                               "  let result: bool = f(5);\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Type inference (lambda parameter types) -------------------------------

  TEST_CASE("infer lambda parameter type from body usage") {
    // x is used with +, so x must be i64
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x| { x + 1 };\n"
                          "  f(10)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("infer lambda parameter type from negation") {
    // !x means x must be bool
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x| { !x };\n"
                          "  let result: bool = f(true);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[1]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("fully unannotated identity lambda inferred from call site") {
    // |x| { x } called with i64 — both param and return inferred
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let id = |x| { x };\n"
                          "  id(42)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("inferred lambda rejects wrong argument type") {
    // |x| { x + 1 } infers x: i64, so calling with bool should fail
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x| { x + 1 };\n"
                               "  f(true)\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Type inference (call-site unification) --------------------------------

  TEST_CASE("call site unifies type variable parameter with argument type") {
    // |x| { x } — x has no constraint from body, but call site provides i64
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x| { x };\n"
                          "  f(42)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("call site with wrong type after body inference throws") {
    // |x| { x + 1 } infers x: i64 from body, calling with bool should fail
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x| { x + 1 };\n"
                               "  f(true)\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Type inference (if expression unification) ---------------------------

  TEST_CASE("if expression unifies inferred branch types") {
    // lambda returns if expr where branches have concrete types
    // that need unification with inferred return type
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x: bool| {\n"
                          "    if x { 1 } else { 2 }\n"
                          "  };\n"
                          "  f(true)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("if expression with mismatched branch types throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  if true { 1 } else { false }\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Type inference (let binding unification) -----------------------------

  TEST_CASE("let binding with annotation unifies with inferred lambda return") {
    // let result: bool = f(true) where f returns inferred type
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x| { !x };\n"
                          "  let result: bool = f(true);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[1]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("let binding annotation mismatch with expression type throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: bool = 42;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Type inference (return expression unification) -----------------------

  TEST_CASE("return with inferred type matches function signature") {
    auto hir = type_check("fn add(x: i64, y: i64) -> i64 {\n"
                          "  return x + y\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(func.m_signature.m_function_id == "add");
  }

  TEST_CASE("return type mismatch with function signature throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  return true;\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Identifier resolution -----------------------------------------------

  TEST_CASE("identifier resolves to correct type from let binding") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: i64 = 42;\n"
                          "  x\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("identifier resolves to correct type from function parameter") {
    auto hir = type_check("fn identity(x: i64) -> i64 { x }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("undefined identifier throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { x }"),
                    core::CompilerException);
  }

  // --- Scoping -------------------------------------------------------------

  TEST_CASE("variable shadowing in inner scope typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: i64 = 42;\n"
                          "  let x: bool = true;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    CHECK(hir.m_top_items.size() == 1);
  }

  TEST_CASE("block scope isolates variables") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  if true { let y: i64 = 1; }\n"
                               "  y\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Multiple functions --------------------------------------------------

  TEST_CASE("multiple functions typecheck independently") {
    auto hir = type_check("fn helper(x: i64) -> bool { true }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
    auto &helper = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(helper.m_signature.m_function_id == "helper");
    auto &main_fn = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    CHECK(main_fn.m_signature.m_function_id == "main");
  }

  // --- Nested expressions --------------------------------------------------

  TEST_CASE("nested arithmetic typechecks") {
    auto hir = type_check("fn main() -> i64 { (1 + 2) * (3 - 4) }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("nested comparison in if condition typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  if 1 < 2 { 10 } else { 20 }\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Forward references (two-pass) ----------------------------------------

  TEST_CASE("forward reference: main calls function defined after it") {
    auto hir = type_check("fn main() -> i64 { helper() }\n"
                          "fn helper() -> i64 { 42 }");
    DUMP_HIR(hir);
    auto &main_func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(main_func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(main_func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("forward reference: mutual recursion between two functions") {
    auto hir = type_check("fn is_even(n: i64) -> bool {\n"
                          "  if n == 0 { true } else { is_odd(n - 1) }\n"
                          "}\n"
                          "fn is_odd(n: i64) -> bool {\n"
                          "  if n == 0 { false } else { is_even(n - 1) }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 3);
    auto &is_even = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(is_even.m_signature.m_function_id == "is_even");
    auto &is_odd = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    CHECK(is_odd.m_signature.m_function_id == "is_odd");
  }

  TEST_CASE("forward reference: wrong argument type still caught") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { helper(true) }\n"
                               "fn helper(x: i64) -> i64 { x }"),
                    core::CompilerException);
  }

  TEST_CASE("forward reference: wrong arity still caught") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { helper(1, 2) }\n"
                               "fn helper(x: i64) -> i64 { x }"),
                    core::CompilerException);
  }

  // --- Let-polymorphism (generalize/instantiate) ---------------------------

  TEST_CASE("polymorphic identity used with different types") {
    // The motivating example: |y| { y } called with bool and i64
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let id = |y| { y };\n"
                          "  let a: bool = id(true);\n"
                          "  id(42)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    // id(true) returns bool
    auto &let_a = std::get<hir::LetBinding>(func.m_body.m_statements[1]);
    auto &a_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let_a.m_variable.m_type));
    CHECK(a_type.m_type == PrimitiveType::BOOL);
    // id(42) returns i64
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("polymorphic lambda used in if condition and return") {
    // The exact example from the user's question
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = |y| { y };\n"
                          "  if x(true) {\n"
                          "    return x(9);\n"
                          "  } else {\n"
                          "    return x(10);\n"
                          "  }\n"
                          "  x(11)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("polymorphic lambda with body constraint used at multiple types") {
    // |x, y| { x + y } constrains both params to i64 from body
    // Should work when called with i64 but fail with bool
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let add = |x, y| { x + y };\n"
                          "  add(1, 2)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("polymorphic const function ignores argument type") {
    // |x| { 42 } — x is never constrained, so any call-site type works
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let always_42 = |x| { 42 };\n"
                          "  let a: i64 = always_42(true);\n"
                          "  let b: i64 = always_42(99);\n"
                          "  a + b\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("polymorphic lambda passed to another function") {
    // Polymorphic identity created in main, called from helper
    auto hir =
        type_check("fn apply_to_int(f: fn(i64) -> i64, x: i64) -> i64 {\n"
                   "  f(x)\n"
                   "}\n"
                   "fn main() -> i64 {\n"
                   "  let id = |y| { y };\n"
                   "  apply_to_int(id, 5)\n"
                   "}");
    DUMP_HIR(hir);
    auto &main_func = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    REQUIRE(main_func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(main_func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("monomorphic lambda rejects second use with different type") {
    // Lambda with annotated param is NOT polymorphic
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x: i64| { x };\n"
                               "  f(true)\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("non-polymorphic let binding does not generalize") {
    // let x: i64 = 5 should not become polymorphic
    auto hir = type_check("fn main() -> i64 {\n"
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

  // --- Bug regression: unary expression type resolution --------------------

  TEST_CASE("unary negation on inferred parameter resolves type") {
    // |x| { -x } should resolve x to i64 via unary minus constraint
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let neg = |x| { -x };\n"
                          "  neg(5)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("unary not on inferred parameter resolves to bool") {
    // |x| { !x } should resolve x to bool via unary not constraint
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let negate = |x| { !x };\n"
                          "  let result: bool = negate(true);\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[1]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

  // --- Bug regression: if expression with inferred branch types -----------

  TEST_CASE("if expression with inferred type variable in then branch") {
    // Lambda body is an if-expr where branches return inferred types
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x| {\n"
                          "    if true { x } else { 0 }\n"
                          "  };\n"
                          "  f(42)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Inferred return with mixed constraints -----------------------------

  TEST_CASE("lambda return type inferred from multiple paths") {
    // |x| { if true { x + 1 } else { 0 } } — both branches return i64
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x| {\n"
                          "    if true { x + 1 } else { 0 }\n"
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

  // --- Mixed annotated and inferred parameters ----------------------------

  TEST_CASE("lambda with mix of annotated and inferred parameters") {
    // |x: bool, y| { if x { y + 1 } else { 0 } }
    // x is annotated bool, y is inferred i64 from y + 1
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let f = |x: bool, y| {\n"
                          "    if x { y + 1 } else { 0 }\n"
                          "  };\n"
                          "  f(true, 5)\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("lambda with annotated and inferred rejects wrong annotated arg") {
    // |x: bool, y| { y + 1 } — calling with i64 for x should fail
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x: bool, y| { y + 1 };\n"
                               "  f(42, 5)\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Lambda without let binding (no generalization) ---------------------

  TEST_CASE("lambda passed directly as argument without let binding") {
    auto hir = type_check("fn apply(f: fn(i64) -> i64, x: i64) -> i64 {\n"
                          "  f(x)\n"
                          "}\n"
                          "fn main() -> i64 {\n"
                          "  apply(|x| { x + 1 }, 5)\n"
                          "}");
    DUMP_HIR(hir);
    auto &main_func = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    REQUIRE(main_func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(main_func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Function type parameter errors -------------------------------------

  TEST_CASE("function type parameter called with wrong argument type") {
    CHECK_THROWS_AS(type_check("fn apply(f: fn(i64) -> i64, x: i64) -> i64 {\n"
                               "  f(true)\n"
                               "}\n"
                               "fn main() -> i64 { apply(|x| { x }, 5) }"),
                    core::CompilerException);
  }

  TEST_CASE("function type parameter return type mismatch") {
    CHECK_THROWS_AS(
        type_check("fn apply(f: fn(i64) -> bool, x: i64) -> bool {\n"
                   "  f(x)\n"
                   "}\n"
                   "fn main() -> i64 {\n"
                   "  let r: bool = apply(|x| { x + 1 }, 5);\n"
                   "  0\n"
                   "}"),
        core::CompilerException);
  }

  // --- Monomorphic vs polymorphic generalization ---------------------------
  //
  // A lambda whose body constrains its parameters (e.g. x + 1 forces i64)
  // must NOT be generalized as polymorphic. These tests verify that
  // body-constrained lambdas are correctly rejected at incompatible types.

  TEST_CASE("body-constrained lambda is NOT polymorphic") {
    // |a| { a + 1 } constrains a to i64. It should NOT be usable at bool.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |a| { a + 1 };\n"
                               "  let r: bool = f(true);\n" // should fail
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("body-constrained lambda used at two incompatible types") {
    // |x| { x + 1 } is i64 -> i64. Using it at bool should fail even if
    // the first call at i64 succeeds.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x| { x + 1 };\n"
                               "  let a = f(5);\n"    // fine: i64 -> i64
                               "  let b = f(true);\n" // should fail
                               "  a\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("body-constrained lambda return type is not generalized") {
    // |x| { if x { 1 } else { 0 } } constrains x to bool and returns i64.
    // Assigning the result to bool should fail — the return is i64, not free.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x| { if x { 1 } else { 0 } };\n"
                               "  let r: bool = f(true);\n" // should fail
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("true identity is polymorphic but constrained lambda is not") {
    // Contrast: identity |x| { x } IS polymorphic (no body constraints).
    // But |x| { x + 1 } is NOT. Both are let-bound.
    // This test uses identity correctly at two types AND rejects the
    // constrained version at the wrong type.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let id = |x| { x };\n"
                          "  let a: i64 = id(42);\n"
                          "  let b: bool = id(true);\n"
                          "  a\n"
                          "}");
    DUMP_HIR(hir);
    // id works at both types — just check it compiles

    // But add_one should not be polymorphic
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let add_one = |x| { x + 1 };\n"
                               "  let a: i64 = add_one(5);\n"
                               "  let b: bool = add_one(true);\n" // fail
                               "  a\n"
                               "}"),
                    core::CompilerException);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
