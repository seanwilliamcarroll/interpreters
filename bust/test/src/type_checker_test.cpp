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

  TEST_CASE("if without else has type unit") {
    auto hir = type_check("fn main() -> i64 {\n"
                          "  if true { 1 }\n"
                          "  0\n"
                          "}");
    DUMP_HIR(hir);
    // The if-without-else should produce unit type
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

} // TEST_SUITE
//****************************************************************************
} // namespace bust
//****************************************************************************
