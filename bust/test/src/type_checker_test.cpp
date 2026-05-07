//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::TypeChecker — basic declarations,
//*            assignments, mutability, function definitions, and operator
//*            expressions (binary/unary/if/return/calls).
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
TEST_SUITE("bust.type_checker.basics") {

  // --- Helpers -------------------------------------------------------------

  static hir::Program type_check(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return run_pipeline(parser.parse(), ValidateMain{}, TypeChecker{});
  }

#define DUMP_HIR(program) INFO("HIR:\n" << hir::Dumper::dump(program))

  // --- Literal expressions -------------------------------------------------

  TEST_CASE("literal i64 has type i64") {
    auto hir = type_check("fn main() -> i64 { 42 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &expr = *func.m_body.m_final_expression;
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(expr.m_type)));
    auto &ptype =
        std::get<hir::PrimitiveTypeValue>(hir.m_type_arena.get(expr.m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("literal bool has type bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = true;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr = let.m_expression;
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(expr.m_type)));
    auto &ptype =
        std::get<hir::PrimitiveTypeValue>(hir.m_type_arena.get(expr.m_type));
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("literal unit has type unit") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = ();\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr = let.m_expression;
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(expr.m_type)));
    auto &ptype =
        std::get<hir::PrimitiveTypeValue>(hir.m_type_arena.get(expr.m_type));
    CHECK(ptype.m_type == PrimitiveType::UNIT);
  }

  // --- Let bindings --------------------------------------------------------

  TEST_CASE("let binding with matching annotation typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: i64 = 42;\n"
                          "  x\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &stmt = func.m_body.m_statements[0];
    auto &let = std::get<hir::LetBinding>(stmt);
    CHECK(let.m_variable.m_name == "x");
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::I64);
  }

  TEST_CASE("let binding without annotation infers type from expression") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x = 42;\n"
                          "  x\n"
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::I64);
  }

  TEST_CASE("let binding with bool annotation and bool expr typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let flag: bool = true;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("let binding with mismatched annotation throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: bool = 42;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("let binding i64 annotation with bool value throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i64 = true;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Assignments ---------------------------------------------------------

  TEST_CASE("assignment to mutable identifier produces hir::Assignment") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let mut x = 1;\n"
                          "  x = 2;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_statements.size() >= 2);
    REQUIRE(
        std::holds_alternative<hir::Assignment>(func.m_body.m_statements[1]));
    auto &assign = std::get<hir::Assignment>(func.m_body.m_statements[1]);
    REQUIRE(std::holds_alternative<hir::Identifier>(assign.m_place));
    CHECK(std::get<hir::Identifier>(assign.m_place).m_name == "x");
  }

  TEST_CASE("assignment RHS type matches LHS binding type") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let mut x: i64 = 1;\n"
                          "  x = 5;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &assign = std::get<hir::Assignment>(func.m_body.m_statements[1]);
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(assign.m_expression.m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("assignment unifies binding type via inference") {
    // `let mut x = true;` infers x: bool. Then `x = false;` must unify
    // — i.e. the LHS lookup feeds back into RHS expectation.
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let mut x = true;\n"
                          "  x = false;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &assign = std::get<hir::Assignment>(func.m_body.m_statements[1]);
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(assign.m_expression.m_type));
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("assignment with mismatched RHS type throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let mut x: i64 = 1;\n"
                               "  x = true;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("assignment to undeclared variable throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  x = 5;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("assignment with literal LHS throws — not a place") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  5 = 6;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("assignment with binary-expression LHS throws — not a place") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let mut x = 1;\n"
                               "  x + 1 = 5;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("assignment to non-mutable let binding throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 1;\n"
                               "  x = 2;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("assignment to function parameter throws — parameters are "
            "currently immutable") {
    CHECK_THROWS_AS(type_check("fn foo(x: i64) -> i64 {\n"
                               "  x = 2;\n"
                               "  x\n"
                               "}\n"
                               "fn main() -> i64 { foo(1) }"),
                    core::CompilerException);
  }

  TEST_CASE("shadowing immutable with let mut allows assignment") {
    // The inner `let mut x` introduces a NEW binding that shadows the outer
    // immutable `x`. Assignment targets the new (mutable) binding.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let x = 1;\n"
                             "  let mut x = x;\n"
                             "  x = 2;\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("inner scope mut binding does not relax outer immutable binding") {
    // Inside the block, `let mut x` shadows. After the block, the original
    // (immutable) `x` is back in scope, so assigning to it must fail.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 1;\n"
                               "  { let mut x = 2; x = 3; };\n"
                               "  x = 4;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("shadowing mut with immutable let throws on assignment") {
    // `let mut x` then `let x` — the second binding shadows the first and is
    // immutable. Assigning to the shadow must fail.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let mut x = 1;\n"
                               "  let x = x;\n"
                               "  x = 2;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("inner immutable shadow does not affect outer mut binding") {
    // Outer `let mut x` is mutable. Inner block shadows with immutable `x`.
    // After the inner scope ends, the outer mutable binding is back in
    // scope, so assigning to it must succeed.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let mut x = 1;\n"
                             "  { let x = 2; };\n"
                             "  x = 3;\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("same-scope shadowing mut with mut allows assignment") {
    // Two `let mut x` in sequence — the second shadows the first. Assigning
    // is legal because the active binding is mutable.
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let mut x = 1;\n"
                             "  let mut x = 2;\n"
                             "  x = 3;\n"
                             "  0\n"
                             "}"));
  }

  TEST_CASE("same-scope shadowing immutable with immutable still rejects "
            "assignment") {
    // Two `let x` in sequence — the second shadows the first. Both are
    // immutable; assigning to the shadow must fail.
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 1;\n"
                               "  let x = 2;\n"
                               "  x = 3;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Mutable parameters --------------------------------------------------

  TEST_CASE("assignment to mut function parameter is allowed") {
    CHECK_NOTHROW(type_check("fn foo(mut x: i64) -> i64 {\n"
                             "  x = 2;\n"
                             "  x\n"
                             "}\n"
                             "fn main() -> i64 { foo(1) }"));
  }

  TEST_CASE("assignment to mut lambda parameter is allowed") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let f = |mut x: i64| -> i64 { x = 2; x };\n"
                             "  f(1)\n"
                             "}"));
  }

  TEST_CASE("assignment to non-mut lambda parameter throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let f = |x: i64| -> i64 { x = 2; x };\n"
                               "  f(1)\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Closure capture and assignment --------------------------------------
  //
  // Mirrors Rust's rule that mutating a captured variable inside a closure
  // requires the captured binding to be `mut`. (bust captures by value, so
  // the runtime semantics match Rust's `move` closures over Copy types —
  // those are exercised in the codegen tests.)

  TEST_CASE("closure assigning to immutable captured variable throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 1;\n"
                               "  let f = || { x = 2; };\n"
                               "  f();\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("closure assigning to mut captured variable type-checks") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let mut x = 1;\n"
                             "  let f = || { x = 2; };\n"
                             "  f();\n"
                             "  0\n"
                             "}"));
  }

  // --- Function definitions ------------------------------------------------

  TEST_CASE("function def produces correct function type") {
    auto hir = type_check("fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(func.m_signature.m_function_id == "main");
    CHECK(std::get<hir::FunctionType>(
              hir.m_type_arena.get(func.m_signature.m_type))
              .m_parameters.empty());

    auto &ret = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(std::get<hir::FunctionType>(
                                 hir.m_type_arena.get(func.m_signature.m_type))
                                 .m_return_type));
    CHECK(ret.m_type == PrimitiveType::I64);
  }

  TEST_CASE("function with parameters has correct types") {
    auto hir = type_check("fn add(a: i64, b: i64) -> i64 { a }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(func.m_signature.m_function_id == "add");
    REQUIRE(func.m_signature.m_parameters.size() == 2);
    CHECK(func.m_signature.m_parameters[0].m_id.m_name == "a");
    auto &a_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_signature.m_parameters[0].m_id.m_type));
    CHECK(a_type.m_type == PrimitiveType::I64);
    CHECK(func.m_signature.m_parameters[1].m_id.m_name == "b");
    REQUIRE(std::get<hir::FunctionType>(
                hir.m_type_arena.get(func.m_signature.m_type))
                .m_parameters.size() == 2);
  }

  TEST_CASE("function body type must match return type") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { true }"),
                    core::CompilerException);
  }

  TEST_CASE("function body type unit mismatch throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { () }"),
                    core::CompilerException);
  }

  // --- Binary expressions --------------------------------------------------

  TEST_CASE("arithmetic binary expr has type i64") {
    auto hir = type_check("fn main() -> i64 { 1 + 2 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &expr = *func.m_body.m_final_expression;
    auto &ptype =
        std::get<hir::PrimitiveTypeValue>(hir.m_type_arena.get(expr.m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("comparison binary expr has type bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = 1 < 2;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("binary arithmetic with bool operand throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { 1 + true }"),
                    core::CompilerException);
  }

  TEST_CASE("greater-than returns bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = 2 > 1;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("less-than-or-equal returns bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = 1 <= 2;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("greater-than-or-equal returns bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = 2 >= 1;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("equality returns bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = 1 == 2;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("not-equal returns bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = 1 != 2;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("comparison result as function return type") {
    auto hir = type_check("fn is_positive(x: i64) -> bool { x > 0 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("binary comparison with mismatched operands throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: bool = 1 < true;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("logical and/or require bool operands") {
    CHECK_NOTHROW(type_check("fn main() -> i64 {\n"
                             "  let x: bool = true && false;\n"
                             "  0\n"
                             "}"));
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: bool = 1 && 2;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("logical and returns bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = true && false;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("logical or returns bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = true || false;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &expr_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_expression.m_type));
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  // --- Unary expressions ---------------------------------------------------

  TEST_CASE("unary minus on i64 produces i64") {
    auto hir = type_check("fn main() -> i64 { -42 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("unary not on bool produces bool") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: bool = !true;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(let.m_variable.m_type));
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("unary minus on bool throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { -true }"),
                    core::CompilerException);
  }

  TEST_CASE("unary not on i64 throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: bool = !42;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- If expressions ------------------------------------------------------

  TEST_CASE("if-else with matching branch types typechecks") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  if true { 1 } else { 2 }\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("if-else with mismatched branch types throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  if true { 1 } else { true }\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("if condition must be bool") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  if 42 { 1 } else { 2 }\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("if without else rejects non-unit then branch") {
    CHECK_THROWS(type_check("fn main() -> i64 {\n"
                            "  if true { 1 }\n"
                            "  0\n"
                            "}"));
  }

  TEST_CASE("if without else has type unit") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  if true { }\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &expr = std::get<hir::Expression>(func.m_body.m_statements[0]);
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(expr.m_type)));
    auto &ptype =
        std::get<hir::PrimitiveTypeValue>(hir.m_type_arena.get(expr.m_type));
    CHECK(ptype.m_type == PrimitiveType::UNIT);
  }

  TEST_CASE("if without else allows return in then branch") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  if true { return 1 }\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(!func.m_body.m_statements.empty());
    auto &expr = std::get<hir::Expression>(func.m_body.m_statements[0]);
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(expr.m_type)));
    auto &ptype =
        std::get<hir::PrimitiveTypeValue>(hir.m_type_arena.get(expr.m_type));
    CHECK(ptype.m_type == PrimitiveType::UNIT);
  }

  // --- Return expressions --------------------------------------------------

  TEST_CASE("return expression has never type") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  return 0\n"
                          "}");
    DUMP_HIR(hir);
    // The return should produce a Never-typed expression
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("return type must match function return type") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { return true }"),
                    core::CompilerException);
  }

  // --- If-else with return (never type unification) ------------------------

  TEST_CASE("if-else where one branch returns unifies with other branch") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  if true { return 0 } else { 42 }\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    // The if-else type should be i64 (never unifies with i64)
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Function calls ------------------------------------------------------

  TEST_CASE("function call has correct return type") {
    auto hir = type_check("fn foo() -> i64 { 42 }\n"
                          "fn main() -> i64 { foo() }");
    DUMP_HIR(hir);
    auto &main_func = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    REQUIRE(main_func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(main_func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("function call with wrong argument type throws") {
    CHECK_THROWS_AS(type_check("fn foo(x: i64) -> i64 { x }\n"
                               "fn main() -> i64 { foo(true) }"),
                    core::CompilerException);
  }

  TEST_CASE("function call with wrong number of arguments throws") {
    CHECK_THROWS_AS(type_check("fn foo(x: i64) -> i64 { x }\n"
                               "fn main() -> i64 { foo(1, 2) }"),
                    core::CompilerException);
  }

  TEST_CASE("self-recursive function typechecks") {
    auto hir = type_check("fn countdown(n: i64) -> i64 {\n"
                          "  if n == 0 { 0 } else { countdown(n - 1) }\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(func.m_signature.m_function_id == "countdown");
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        hir.m_type_arena.get(func.m_body.m_final_expression->m_type));
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
