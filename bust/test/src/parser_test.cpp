//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Unit tests for bust::Parser
//*
//*
//*  See Also: https://github.com/doctest/doctest
//*            for more on the 'DocTest' project.
//*
//*
//****************************************************************************

#include <ast/dump.hpp>
#include <ast/nodes.hpp>
#include <bust_tokens.hpp>
#include <lexer.hpp>
#include <parser.hpp>

#include <doctest/doctest.h>
#include <sstream>

//****************************************************************************
namespace bust {
using namespace ast;
//****************************************************************************
TEST_SUITE("bust.parser") {

  // --- Helpers -------------------------------------------------------------

  static Program parse_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return parser.parse();
  }

  // Use after parse_string: DUMP_AST(program);
  // Prints the AST tree only when a CHECK/REQUIRE fails in the same scope.
#define DUMP_AST(prog) INFO(Dumper::dump(prog))

  // Extract the single top-level FunctionDef from a program
  static const FunctionDef &get_single_func(const Program &program) {
    REQUIRE(program.m_items.size() == 1);
    REQUIRE(std::holds_alternative<std::unique_ptr<FunctionDef>>(
        program.m_items[0]));
    return *std::get<std::unique_ptr<FunctionDef>>(program.m_items[0]);
  }

  // Extract the final expression from a block
  static const Expression &get_final_expr(const Block &block) {
    REQUIRE(block.m_final_expression.has_value());
    return *block.m_final_expression;
  }

  // Check that a TypeIdentifier is a specific PrimitiveType
  static void check_primitive_type(const TypeIdentifier &type_id,
                                   PrimitiveType expected) {
    REQUIRE(std::holds_alternative<PrimitiveTypeIdentifier>(type_id));
    CHECK(std::get<PrimitiveTypeIdentifier>(type_id).m_type == expected);
  }

  // === Literals ============================================================

  TEST_CASE("bust::parse_int_literal") {
    auto program = parse_string("fn main() -> i64 { 42 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<LiteralInt64>(expr));
    CHECK(std::get<LiteralInt64>(expr).m_value == 42);
  }

  TEST_CASE("bust::parse_zero") {
    auto program = parse_string("fn main() -> i64 { 0 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<LiteralInt64>(expr));
    CHECK(std::get<LiteralInt64>(expr).m_value == 0);
  }

  TEST_CASE("bust::parse_bool_true") {
    auto program = parse_string("fn main() -> bool { true }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<LiteralBool>(expr));
    CHECK(std::get<LiteralBool>(expr).m_value == true);
  }

  TEST_CASE("bust::parse_bool_false") {
    auto program = parse_string("fn main() -> bool { false }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<LiteralBool>(expr));
    CHECK(std::get<LiteralBool>(expr).m_value == false);
  }

  TEST_CASE("bust::parse_unit_literal") {
    auto program = parse_string("fn main() { () }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    CHECK(std::holds_alternative<LiteralUnit>(expr));
  }

  // === Identifiers =========================================================

  TEST_CASE("bust::parse_identifier_in_body") {
    auto program = parse_string("fn main() -> i64 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Identifier>(expr));
    CHECK(std::get<Identifier>(expr).m_name == "x");
  }

  // === Function definitions ================================================

  TEST_CASE("bust::parse_minimal_function") {
    auto program = parse_string("fn main() -> i64 { 0 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_id.m_name == "main");
    CHECK(func.m_parameters.empty());
    check_primitive_type(func.m_return_type, PrimitiveType::INT64);
  }

  TEST_CASE("bust::parse_function_no_return_type") {
    auto program = parse_string("fn do_nothing() { }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_id.m_name == "do_nothing");
    check_primitive_type(func.m_return_type, PrimitiveType::UNIT);
    CHECK_FALSE(func.m_body.m_final_expression.has_value());
  }

  TEST_CASE("bust::parse_function_with_params") {
    auto program = parse_string("fn add(a: i64, b: i64) -> i64 { a }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_id.m_name == "add");
    REQUIRE(func.m_parameters.size() == 2);
    CHECK(func.m_parameters[0].m_name == "a");
    REQUIRE(func.m_parameters[0].m_type.has_value());
    check_primitive_type(*func.m_parameters[0].m_type, PrimitiveType::INT64);
    CHECK(func.m_parameters[1].m_name == "b");
    REQUIRE(func.m_parameters[1].m_type.has_value());
    check_primitive_type(*func.m_parameters[1].m_type, PrimitiveType::INT64);
  }

  TEST_CASE("bust::parse_function_single_param") {
    auto program = parse_string("fn id(x: i64) -> i64 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_parameters.size() == 1);
    CHECK(func.m_parameters[0].m_name == "x");
  }

  // === Let bindings ========================================================

  TEST_CASE("bust::parse_top_level_let") {
    auto program = parse_string("let x: i64 = 42;\n"
                                "fn main() -> i64 { x }");
    DUMP_AST(program);
    REQUIRE(program.m_items.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<LetBinding>>(
        program.m_items[0]));
    const auto &binding =
        *std::get<std::unique_ptr<LetBinding>>(program.m_items[0]);
    CHECK(binding.m_variable.m_name == "x");
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::INT64);
    REQUIRE(std::holds_alternative<LiteralInt64>(binding.m_expression));
  }

  TEST_CASE("bust::parse_let_without_type") {
    auto program = parse_string("let x = 42;\n"
                                "fn main() -> i64 { x }");
    DUMP_AST(program);
    REQUIRE(program.m_items.size() == 2);
    const auto &binding =
        *std::get<std::unique_ptr<LetBinding>>(program.m_items[0]);
    CHECK(binding.m_variable.m_name == "x");
    CHECK_FALSE(binding.m_variable.m_type.has_value());
  }

  TEST_CASE("bust::parse_let_in_block") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i64 = 10;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<std::unique_ptr<LetBinding>>(
        func.m_body.m_statements[0]));
    const auto &binding =
        *std::get<std::unique_ptr<LetBinding>>(func.m_body.m_statements[0]);
    CHECK(binding.m_variable.m_name == "x");
    // Final expression is x
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Identifier>(expr));
    CHECK(std::get<Identifier>(expr).m_name == "x");
  }

  // === Arithmetic ==========================================================

  TEST_CASE("bust::parse_binary_add") {
    auto program = parse_string("fn main() -> i64 { 1 + 2 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    const auto &bin = *std::get<std::unique_ptr<BinaryExpr>>(expr);
    CHECK(bin.m_operator == BinaryOperator::PLUS);
    CHECK(std::holds_alternative<LiteralInt64>(bin.m_lhs));
    CHECK(std::holds_alternative<LiteralInt64>(bin.m_rhs));
  }

  TEST_CASE("bust::parse_binary_sub") {
    auto program = parse_string("fn main() -> i64 { 5 - 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::MINUS);
  }

  TEST_CASE("bust::parse_precedence_mul_over_add") {
    // 1 + 2 * 3 should parse as 1 + (2 * 3)
    auto program = parse_string("fn main() -> i64 { 1 + 2 * 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    const auto &add = *std::get<std::unique_ptr<BinaryExpr>>(expr);
    CHECK(add.m_operator == BinaryOperator::PLUS);
    // LHS is literal 1
    CHECK(std::holds_alternative<LiteralInt64>(add.m_lhs));
    // RHS is 2 * 3
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(add.m_rhs));
    const auto &mul = *std::get<std::unique_ptr<BinaryExpr>>(add.m_rhs);
    CHECK(mul.m_operator == BinaryOperator::MULTIPLIES);
  }

  TEST_CASE("bust::parse_left_associativity") {
    // 1 - 2 - 3 should parse as (1 - 2) - 3
    auto program = parse_string("fn main() -> i64 { 1 - 2 - 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    const auto &outer = *std::get<std::unique_ptr<BinaryExpr>>(expr);
    CHECK(outer.m_operator == BinaryOperator::MINUS);
    // LHS is (1 - 2)
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(outer.m_lhs));
    const auto &inner = *std::get<std::unique_ptr<BinaryExpr>>(outer.m_lhs);
    CHECK(inner.m_operator == BinaryOperator::MINUS);
    CHECK(std::holds_alternative<LiteralInt64>(inner.m_lhs));
    CHECK(std::holds_alternative<LiteralInt64>(inner.m_rhs));
    // RHS is 3
    CHECK(std::holds_alternative<LiteralInt64>(outer.m_rhs));
  }

  TEST_CASE("bust::parse_parenthesized_expression") {
    // (1 + 2) * 3 — parens override precedence
    auto program = parse_string("fn main() -> i64 { (1 + 2) * 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    const auto &mul = *std::get<std::unique_ptr<BinaryExpr>>(expr);
    CHECK(mul.m_operator == BinaryOperator::MULTIPLIES);
    // LHS is (1 + 2)
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(mul.m_lhs));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(mul.m_lhs)->m_operator ==
          BinaryOperator::PLUS);
  }

  TEST_CASE("bust::parse_modulus") {
    auto program = parse_string("fn main() -> i64 { 10 % 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::MODULUS);
  }

  TEST_CASE("bust::parse_division") {
    auto program = parse_string("fn main() -> i64 { 10 / 2 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::DIVIDES);
  }

  // === Comparison ==========================================================

  TEST_CASE("bust::parse_less_than") {
    auto program = parse_string("fn main() -> bool { 1 < 2 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::LT);
  }

  TEST_CASE("bust::parse_equality") {
    auto program = parse_string("fn main() -> bool { x == y }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::EQ);
  }

  TEST_CASE("bust::parse_not_equal") {
    auto program = parse_string("fn main() -> bool { x != y }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::NOT_EQ);
  }

  TEST_CASE("bust::parse_comparison_lower_than_arithmetic") {
    // 1 + 2 < 3 + 4 should parse as (1 + 2) < (3 + 4)
    auto program = parse_string("fn main() -> bool { 1 + 2 < 3 + 4 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    const auto &cmp = *std::get<std::unique_ptr<BinaryExpr>>(expr);
    CHECK(cmp.m_operator == BinaryOperator::LT);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(cmp.m_lhs));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(cmp.m_lhs)->m_operator ==
          BinaryOperator::PLUS);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(cmp.m_rhs));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(cmp.m_rhs)->m_operator ==
          BinaryOperator::PLUS);
  }

  // === Logical =============================================================

  TEST_CASE("bust::parse_logical_and") {
    auto program = parse_string("fn main() -> bool { true && false }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::LOGICAL_AND);
  }

  TEST_CASE("bust::parse_logical_or") {
    auto program = parse_string("fn main() -> bool { true || false }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(expr)->m_operator ==
          BinaryOperator::LOGICAL_OR);
  }

  TEST_CASE("bust::parse_logical_precedence") {
    // a || b && c should parse as a || (b && c)
    auto program = parse_string("fn main() -> bool { a || b && c }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr));
    const auto &lor = *std::get<std::unique_ptr<BinaryExpr>>(expr);
    CHECK(lor.m_operator == BinaryOperator::LOGICAL_OR);
    // RHS should be b && c
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(lor.m_rhs));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(lor.m_rhs)->m_operator ==
          BinaryOperator::LOGICAL_AND);
  }

  // === Unary ===============================================================

  TEST_CASE("bust::parse_unary_negation") {
    auto program = parse_string("fn main() -> i64 { -42 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<UnaryExpr>>(expr));
    const auto &unary = *std::get<std::unique_ptr<UnaryExpr>>(expr);
    CHECK(unary.m_operator == UnaryOperator::MINUS);
    CHECK(std::holds_alternative<LiteralInt64>(unary.m_expression));
  }

  TEST_CASE("bust::parse_unary_not") {
    auto program = parse_string("fn main() -> bool { !true }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<UnaryExpr>>(expr));
    const auto &unary = *std::get<std::unique_ptr<UnaryExpr>>(expr);
    CHECK(unary.m_operator == UnaryOperator::NOT);
    CHECK(std::holds_alternative<LiteralBool>(unary.m_expression));
  }

  // === Function calls ======================================================

  TEST_CASE("bust::parse_function_call_no_args") {
    auto program = parse_string("fn main() -> i64 { foo() }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<CallExpr>>(expr));
    const auto &call = *std::get<std::unique_ptr<CallExpr>>(expr);
    REQUIRE(std::holds_alternative<Identifier>(call.m_callee));
    CHECK(std::get<Identifier>(call.m_callee).m_name == "foo");
    CHECK(call.m_arguments.empty());
  }

  TEST_CASE("bust::parse_function_call_with_args") {
    auto program = parse_string("fn main() -> i64 { add(1, 2) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<CallExpr>>(expr));
    const auto &call = *std::get<std::unique_ptr<CallExpr>>(expr);
    REQUIRE(std::holds_alternative<Identifier>(call.m_callee));
    CHECK(std::get<Identifier>(call.m_callee).m_name == "add");
    REQUIRE(call.m_arguments.size() == 2);
    CHECK(std::holds_alternative<LiteralInt64>(call.m_arguments[0]));
    CHECK(std::holds_alternative<LiteralInt64>(call.m_arguments[1]));
  }

  TEST_CASE("bust::parse_function_call_expression_args") {
    auto program = parse_string("fn main() -> i64 { foo(1 + 2, x) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<CallExpr>>(expr));
    const auto &call = *std::get<std::unique_ptr<CallExpr>>(expr);
    REQUIRE(call.m_arguments.size() == 2);
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        call.m_arguments[0]));
    CHECK(std::holds_alternative<Identifier>(call.m_arguments[1]));
  }

  // === If expressions ======================================================

  TEST_CASE("bust::parse_if_no_else") {
    auto program = parse_string("fn main() { if true { 1; } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(expr));
    const auto &if_expr = *std::get<std::unique_ptr<IfExpr>>(expr);
    CHECK(std::holds_alternative<LiteralBool>(if_expr.m_condition));
    CHECK_FALSE(if_expr.m_else_block.has_value());
  }

  TEST_CASE("bust::parse_if_else") {
    auto program =
        parse_string("fn main() -> i64 { if true { 1 } else { 2 } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(expr));
    const auto &if_expr = *std::get<std::unique_ptr<IfExpr>>(expr);
    CHECK(std::holds_alternative<LiteralBool>(if_expr.m_condition));
    REQUIRE(if_expr.m_else_block.has_value());
    // Then block has final expr 1
    REQUIRE(if_expr.m_then_block.m_final_expression.has_value());
    CHECK(std::holds_alternative<LiteralInt64>(
        *if_expr.m_then_block.m_final_expression));
    // Else block has final expr 2
    REQUIRE(if_expr.m_else_block->m_final_expression.has_value());
    CHECK(std::holds_alternative<LiteralInt64>(
        *if_expr.m_else_block->m_final_expression));
  }

  TEST_CASE("bust::parse_if_with_comparison") {
    auto program =
        parse_string("fn main() -> i64 { if x < 10 { 1 } else { 2 } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(expr));
    const auto &if_expr = *std::get<std::unique_ptr<IfExpr>>(expr);
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        if_expr.m_condition));
  }

  // === Return ==============================================================

  TEST_CASE("bust::parse_return") {
    auto program = parse_string("fn main() -> i64 { return 42 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<ReturnExpr>>(expr));
    const auto &ret = *std::get<std::unique_ptr<ReturnExpr>>(expr);
    CHECK(std::holds_alternative<LiteralInt64>(ret.m_return_expression));
  }

  TEST_CASE("bust::parse_return_as_only_statement") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  return 42;\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    const auto &stmt_expr = std::get<Expression>(func.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<ReturnExpr>>(stmt_expr));
    const auto &ret = *std::get<std::unique_ptr<ReturnExpr>>(stmt_expr);
    CHECK(std::holds_alternative<LiteralInt64>(ret.m_return_expression));
    CHECK_FALSE(func.m_body.m_final_expression.has_value());
  }

  TEST_CASE("bust::parse_return_as_statement") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  return 42;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    // return 42; is a statement (expression followed by semicolon)
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    const auto &stmt_expr = std::get<Expression>(func.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<ReturnExpr>>(stmt_expr));
    const auto &ret = *std::get<std::unique_ptr<ReturnExpr>>(stmt_expr);
    CHECK(std::holds_alternative<LiteralInt64>(ret.m_return_expression));
    // Final expression: 0
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(
        std::holds_alternative<LiteralInt64>(*func.m_body.m_final_expression));
  }

  TEST_CASE("bust::parse_early_return_in_if") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  if true { return 1; }\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    // First statement: if with return inside
    REQUIRE(func.m_body.m_statements.size() == 1);
    // Final expression: 0
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(
        std::holds_alternative<LiteralInt64>(*func.m_body.m_final_expression));
  }

  // === Block-like statements without semicolons ============================

  TEST_CASE("bust::parse_if_else_as_statement_no_semicolon") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  if true { 1; } else { 2; }\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    CHECK(std::holds_alternative<std::unique_ptr<IfExpr>>(
        std::get<Expression>(func.m_body.m_statements[0])));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(
        std::holds_alternative<LiteralInt64>(*func.m_body.m_final_expression));
  }

  TEST_CASE("bust::parse_bare_block_as_statement_no_semicolon") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  { let x: i64 = 1; }\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    CHECK(std::holds_alternative<std::unique_ptr<Block>>(
        std::get<Expression>(func.m_body.m_statements[0])));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(
        std::holds_alternative<LiteralInt64>(*func.m_body.m_final_expression));
  }

  TEST_CASE("bust::parse_multiple_block_like_no_semicolons") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  if true { 1; }\n"
                                "  { 2; }\n"
                                "  if false { 3; } else { 4; }\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 3);
    CHECK(std::holds_alternative<std::unique_ptr<IfExpr>>(
        std::get<Expression>(func.m_body.m_statements[0])));
    CHECK(std::holds_alternative<std::unique_ptr<Block>>(
        std::get<Expression>(func.m_body.m_statements[1])));
    CHECK(std::holds_alternative<std::unique_ptr<IfExpr>>(
        std::get<Expression>(func.m_body.m_statements[2])));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(
        std::holds_alternative<LiteralInt64>(*func.m_body.m_final_expression));
  }

  // === Blocks ==============================================================

  TEST_CASE("bust::parse_empty_block") {
    auto program = parse_string("fn main() { }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_body.m_statements.empty());
    CHECK_FALSE(func.m_body.m_final_expression.has_value());
  }

  TEST_CASE("bust::parse_block_with_statements_and_final") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i64 = 1;\n"
                                "  let y: i64 = 2;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_body.m_statements.size() == 2);
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<Identifier>(*func.m_body.m_final_expression));
  }

  TEST_CASE("bust::parse_block_expression_statements") {
    // Expression followed by semicolon is a statement
    auto program = parse_string("fn main() -> i64 {\n"
                                "  foo();\n"
                                "  42\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(
        std::holds_alternative<LiteralInt64>(*func.m_body.m_final_expression));
  }

  TEST_CASE("bust::parse_block_as_expression") {
    // Block used as an expression inside a let
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i64 = { let y: i64 = 5; y };\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<std::unique_ptr<LetBinding>>(
        func.m_body.m_statements[0]));
    const auto &binding =
        *std::get<std::unique_ptr<LetBinding>>(func.m_body.m_statements[0]);
    CHECK(std::holds_alternative<std::unique_ptr<Block>>(binding.m_expression));
  }

  // === Lambda ==============================================================

  TEST_CASE("bust::parse_lambda_no_params") {
    auto program = parse_string("fn main() -> i64 { || { 42 } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr));
    const auto &lambda = *std::get<std::unique_ptr<LambdaExpr>>(expr);
    CHECK(lambda.m_parameters.empty());
    CHECK_FALSE(lambda.m_return_type.has_value());
  }

  TEST_CASE("bust::parse_lambda_with_params") {
    auto program =
        parse_string("fn main() -> i64 { |x: i64, y: i64| -> i64 { x } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr));
    const auto &lambda = *std::get<std::unique_ptr<LambdaExpr>>(expr);
    REQUIRE(lambda.m_parameters.size() == 2);
    CHECK(lambda.m_parameters[0].m_name == "x");
    CHECK(lambda.m_parameters[1].m_name == "y");
    REQUIRE(lambda.m_return_type.has_value());
    check_primitive_type(*lambda.m_return_type, PrimitiveType::INT64);
  }

  TEST_CASE("bust::parse_lambda_inferred_param_types") {
    auto program = parse_string("fn main() -> i64 { |x, y| { x } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr));
    const auto &lambda = *std::get<std::unique_ptr<LambdaExpr>>(expr);
    REQUIRE(lambda.m_parameters.size() == 2);
    CHECK_FALSE(lambda.m_parameters[0].m_type.has_value());
    CHECK_FALSE(lambda.m_parameters[1].m_type.has_value());
  }

  // === Multiple top-level items ============================================

  TEST_CASE("bust::parse_multiple_functions") {
    auto program = parse_string("fn helper() -> i64 { 1 }\n"
                                "fn main() -> i64 { helper() }");
    DUMP_AST(program);
    REQUIRE(program.m_items.size() == 2);
    CHECK(std::holds_alternative<std::unique_ptr<FunctionDef>>(
        program.m_items[0]));
    CHECK(std::holds_alternative<std::unique_ptr<FunctionDef>>(
        program.m_items[1]));
  }

  // === Fibonacci — integration test ========================================

  TEST_CASE("bust::parse_fibonacci") {
    auto program = parse_string("fn fib(n: i64) -> i64 {\n"
                                "  if n <= 1 {\n"
                                "    n\n"
                                "  } else {\n"
                                "    fib(n - 1) + fib(n - 2)\n"
                                "  }\n"
                                "}\n"
                                "fn main() -> i64 { fib(10) }");
    DUMP_AST(program);
    REQUIRE(program.m_items.size() == 2);

    // First function is fib
    const auto &fib =
        *std::get<std::unique_ptr<FunctionDef>>(program.m_items[0]);
    CHECK(fib.m_id.m_name == "fib");
    REQUIRE(fib.m_parameters.size() == 1);
    CHECK(fib.m_parameters[0].m_name == "n");

    // Body is an if expression
    const auto &body_expr = get_final_expr(fib.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(body_expr));
    const auto &if_expr = *std::get<std::unique_ptr<IfExpr>>(body_expr);

    // Condition: n <= 1
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        if_expr.m_condition));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(if_expr.m_condition)
              ->m_operator == BinaryOperator::LT_EQ);

    // Else branch: fib(n-1) + fib(n-2)
    REQUIRE(if_expr.m_else_block.has_value());
    REQUIRE(if_expr.m_else_block->m_final_expression.has_value());
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        *if_expr.m_else_block->m_final_expression));
  }
}
//****************************************************************************
} // namespace bust
//****************************************************************************
