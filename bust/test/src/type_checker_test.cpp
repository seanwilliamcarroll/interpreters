//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for bust::TypeChecker
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
#include <hir/types.hpp>
#include <lexer.hpp>
#include <parser.hpp>
#include <type_checker.hpp>

#include <doctest/doctest.h>
#include <sstream>

//****************************************************************************
namespace bust {
//****************************************************************************
TEST_SUITE("bust.type_checker") {

  // --- Helpers -------------------------------------------------------------

  static ast::Program parse_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return parser.parse();
  }

  static hir::Program type_check(const std::string &source) {
    auto program = parse_string(source);
    TypeChecker checker;
    return checker(program);
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
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(expr.m_type));
    auto &ptype = std::get<hir::PrimitiveTypeValue>(expr.m_type);
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
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(expr.m_type));
    auto &ptype = std::get<hir::PrimitiveTypeValue>(expr.m_type);
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
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(expr.m_type));
    auto &ptype = std::get<hir::PrimitiveTypeValue>(expr.m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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

  // --- Function definitions ------------------------------------------------

  TEST_CASE("function def produces correct function type") {
    auto hir = type_check("fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(func.m_function_id == "main");
    CHECK(func.m_type->m_argument_types.empty());
    auto &ret = std::get<hir::PrimitiveTypeValue>(func.m_type->m_return_type);
    CHECK(ret.m_type == PrimitiveType::I64);
  }

  TEST_CASE("function with parameters has correct types") {
    auto hir = type_check("fn add(a: i64, b: i64) -> i64 { a }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(func.m_function_id == "add");
    REQUIRE(func.m_parameters.size() == 2);
    CHECK(func.m_parameters[0].m_name == "a");
    auto &a_type =
        std::get<hir::PrimitiveTypeValue>(func.m_parameters[0].m_type);
    CHECK(a_type.m_type == PrimitiveType::I64);
    CHECK(func.m_parameters[1].m_name == "b");
    REQUIRE(func.m_type->m_argument_types.size() == 2);
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
    auto &ptype = std::get<hir::PrimitiveTypeValue>(expr.m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("comparison result as function return type") {
    auto hir = type_check("fn is_positive(x: i64) -> bool { x > 0 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
    CHECK(expr_type.m_type == PrimitiveType::BOOL);
  }

  // --- Unary expressions ---------------------------------------------------

  TEST_CASE("unary minus on i64 produces i64") {
    auto hir = type_check("fn main() -> i64 { -42 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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
        func.m_body.m_final_expression->m_type);
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
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(expr.m_type));
    auto &ptype = std::get<hir::PrimitiveTypeValue>(expr.m_type);
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
    CHECK(std::holds_alternative<hir::PrimitiveTypeValue>(expr.m_type));
    auto &ptype = std::get<hir::PrimitiveTypeValue>(expr.m_type);
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
        func.m_body.m_final_expression->m_type);
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
        main_func.m_body.m_final_expression->m_type);
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
    CHECK(func.m_function_id == "countdown");
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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
        func.m_body.m_final_expression->m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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
                          "  return x + y;\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    CHECK(func.m_function_id == "add");
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
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("identifier resolves to correct type from function parameter") {
    auto hir = type_check("fn identity(x: i64) -> i64 { x }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
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
    CHECK(helper.m_function_id == "helper");
    auto &main_fn = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    CHECK(main_fn.m_function_id == "main");
  }

  // --- Nested expressions --------------------------------------------------

  TEST_CASE("nested arithmetic typechecks") {
    auto hir = type_check("fn main() -> i64 { (1 + 2) * (3 - 4) }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        main_func.m_body.m_final_expression->m_type);
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
    CHECK(is_even.m_function_id == "is_even");
    auto &is_odd = std::get<hir::FunctionDef>(hir.m_top_items[1]);
    CHECK(is_odd.m_function_id == "is_odd");
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
    auto &a_type = std::get<hir::PrimitiveTypeValue>(let_a.m_variable.m_type);
    CHECK(a_type.m_type == PrimitiveType::BOOL);
    // id(42) returns i64
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        main_func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        main_func.m_body.m_final_expression->m_type);
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

  // --- Top-level let bindings ------------------------------------------------

  TEST_CASE("top-level let binding typechecks") {
    auto hir = type_check("let x: i64 = 42;\n"
                          "fn main() -> i64 { x }");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 2);
    auto &let = std::get<hir::LetBinding>(hir.m_top_items[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
    CHECK(var_type.m_type == PrimitiveType::I64);
  }

  TEST_CASE("top-level let binding with mismatched annotation throws") {
    CHECK_THROWS_AS(type_check("let x: bool = 42;\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

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
        func.m_body.m_final_expression->m_type);
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
    auto &expr_type =
        std::get<hir::PrimitiveTypeValue>(let.m_expression.m_type);
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
        func.m_body.m_final_expression->m_type);
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
                          "}");
    DUMP_HIR(hir);
    REQUIRE(hir.m_top_items.size() == 1);
  }

  TEST_CASE("multiple returns with inconsistent types throws") {
    CHECK_THROWS_AS(type_check("fn bad(n: i64) -> i64 {\n"
                               "  if n > 0 { return true; }\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
        func.m_body.m_final_expression->m_type);
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
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
    CHECK(var_type.m_type == PrimitiveType::BOOL);
  }

  // --- Char literal type -----------------------------------------------------

  TEST_CASE("literal char has type char") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let c: char = 'A';\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
    CHECK(var_type.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("char literal inferred without annotation") {
    auto hir = type_check("fn main() -> char { 'z' }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("char literal with escape has type char") {
    auto hir = type_check("fn main() -> char { '\\n' }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("char assigned to i64 binding throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i64 = 'A';\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  // --- New integer type annotations ------------------------------------------

  TEST_CASE("function with i8 return type") {
    auto hir = type_check("fn to_i8() -> i8 { 42 as i8 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &ret = std::get<hir::PrimitiveTypeValue>(func.m_type->m_return_type);
    CHECK(ret.m_type == PrimitiveType::I8);
  }

  TEST_CASE("function with i32 return type") {
    auto hir = type_check("fn to_i32() -> i32 { 42 as i32 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &ret = std::get<hir::PrimitiveTypeValue>(func.m_type->m_return_type);
    CHECK(ret.m_type == PrimitiveType::I32);
  }

  TEST_CASE("function with i8 parameter") {
    auto hir = type_check("fn use_i8(x: i8) -> i8 { x }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_parameters.size() == 1);
    auto &p_type =
        std::get<hir::PrimitiveTypeValue>(func.m_parameters[0].m_type);
    CHECK(p_type.m_type == PrimitiveType::I8);
  }

  TEST_CASE("i8 and i64 are not interchangeable") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i8 = 42;\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("i32 and i64 are not interchangeable") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i32 = 42;\n"
                               "  x\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Cast expressions ------------------------------------------------------

  TEST_CASE("cast i64 to i8 typechecks") {
    auto hir = type_check("fn main() -> i8 { 42 as i8 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("cast i64 to i32 typechecks") {
    auto hir = type_check("fn main() -> i32 { 42 as i32 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("cast i8 to i64 typechecks (widening)") {
    auto hir = type_check("fn widen(x: i8) -> i64 { x as i64 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast i8 to i32 typechecks (widening)") {
    auto hir = type_check("fn widen(x: i8) -> i32 { x as i32 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("cast i32 to i64 typechecks (widening)") {
    auto hir = type_check("fn widen(x: i32) -> i64 { x as i64 }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast char to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { 'A' as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast i64 to char typechecks") {
    auto hir = type_check("fn main() -> char { 65 as char }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::CHAR);
  }

  TEST_CASE("cast bool to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { true as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("identity cast i64 to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { 42 as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("identity cast char to char typechecks") {
    auto hir = type_check("fn main() -> char { 'A' as char }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::CHAR);
  }

  // --- Illegal casts ---------------------------------------------------------

  TEST_CASE("cast i64 to bool throws") {
    CHECK_THROWS_AS(type_check("fn main() -> bool { 42 as bool }"),
                    core::CompilerException);
  }

  TEST_CASE("cast char to bool throws") {
    CHECK_THROWS_AS(type_check("fn main() -> bool { 'A' as bool }"),
                    core::CompilerException);
  }

  TEST_CASE("cast bool to char throws") {
    CHECK_THROWS_AS(type_check("fn main() -> char { true as char }"),
                    core::CompilerException);
  }

  TEST_CASE("cast unit to i64 throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 { () as i64 }"),
                    core::CompilerException);
  }

  TEST_CASE("cast i64 to unit throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x = 42 as ();\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  // --- Chained casts ---------------------------------------------------------

  TEST_CASE("chained cast i64 to i32 to i8 typechecks") {
    auto hir = type_check("fn main() -> i8 { 42 as i32 as i8 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("chained cast char to i32 to i64 typechecks") {
    auto hir = type_check("fn main() -> i64 { 'A' as i32 as i64 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Cast in expressions ---------------------------------------------------

  TEST_CASE("cast result used in arithmetic") {
    auto hir = type_check("fn main() -> i64 { 'A' as i64 + 1 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  TEST_CASE("cast in let binding with annotation") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  let x: i8 = 42 as i8;\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    auto &let = std::get<hir::LetBinding>(func.m_body.m_statements[0]);
    auto &var_type = std::get<hir::PrimitiveTypeValue>(let.m_variable.m_type);
    CHECK(var_type.m_type == PrimitiveType::I8);
  }

  TEST_CASE("cast result type mismatches let annotation throws") {
    CHECK_THROWS_AS(type_check("fn main() -> i64 {\n"
                               "  let x: i32 = 42 as i8;\n"
                               "  0\n"
                               "}"),
                    core::CompilerException);
  }

  TEST_CASE("arithmetic on mismatched types without cast throws") {
    CHECK_THROWS_AS(type_check("fn add_mixed(a: i8, b: i64) -> i64 {\n"
                               "  a + b\n"
                               "}\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("arithmetic on mismatched types with cast succeeds") {
    auto hir = type_check("fn add_mixed(a: i8, b: i64) -> i64 {\n"
                          "  a as i64 + b\n"
                          "}\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I64);
  }

  // --- Arithmetic and comparison on non-i64 integer types -------------------

  TEST_CASE("i8 arithmetic has type i8") {
    auto hir = type_check("fn f(a: i8, b: i8) -> i8 { a + b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("i32 arithmetic has type i32") {
    auto hir = type_check("fn f(a: i32, b: i32) -> i32 { a + b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("i8 subtraction has type i8") {
    auto hir = type_check("fn f(a: i8, b: i8) -> i8 { a - b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("i32 multiplication has type i32") {
    auto hir = type_check("fn f(a: i32, b: i32) -> i32 { a * b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("i8 comparison returns bool") {
    auto hir = type_check("fn f(a: i8, b: i8) -> bool { a < b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("i32 comparison returns bool") {
    auto hir = type_check("fn f(a: i32, b: i32) -> bool { a >= b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("i8 equality returns bool") {
    auto hir = type_check("fn f(a: i8, b: i8) -> bool { a == b }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::BOOL);
  }

  TEST_CASE("i8 and i32 arithmetic throws (no implicit coercion)") {
    CHECK_THROWS_AS(type_check("fn f(a: i8, b: i32) -> i32 { a + b }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("i8 and i32 comparison throws") {
    CHECK_THROWS_AS(type_check("fn f(a: i8, b: i32) -> bool { a < b }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("char arithmetic throws") {
    CHECK_THROWS_AS(type_check("fn f(a: char, b: char) -> char { a + b }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("unary minus on i8 produces i8") {
    auto hir = type_check("fn f(a: i8) -> i8 { -a }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I8);
  }

  TEST_CASE("unary minus on i32 produces i32") {
    auto hir = type_check("fn f(a: i32) -> i32 { -a }\n"
                          "fn main() -> i64 { 0 }");
    DUMP_HIR(hir);
    auto &func = std::get<hir::FunctionDef>(hir.m_top_items[0]);
    REQUIRE(func.m_body.m_final_expression.has_value());
    auto &ptype = std::get<hir::PrimitiveTypeValue>(
        func.m_body.m_final_expression->m_type);
    CHECK(ptype.m_type == PrimitiveType::I32);
  }

  TEST_CASE("unary minus on char throws") {
    CHECK_THROWS_AS(type_check("fn f(a: char) -> char { -a }\n"
                               "fn main() -> i64 { 0 }"),
                    core::CompilerException);
  }

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
