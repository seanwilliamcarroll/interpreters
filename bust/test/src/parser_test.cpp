//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
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
#include <lexer.hpp>
#include <parser.hpp>
#include <tokens.hpp>

#include <sstream>

#include <doctest/doctest.h>

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
    REQUIRE(std::holds_alternative<FunctionDef>(program.m_items[0]));
    return std::get<FunctionDef>(program.m_items[0]);
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
    REQUIRE(std::holds_alternative<I64>(expr.m_expression));
    CHECK(std::get<I64>(expr.m_expression).m_value == 42);
  }

  TEST_CASE("bust::parse_zero") {
    auto program = parse_string("fn main() -> i64 { 0 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<I64>(expr.m_expression));
    CHECK(std::get<I64>(expr.m_expression).m_value == 0);
  }

  TEST_CASE("bust::parse_bool_true") {
    auto program = parse_string("fn main() -> bool { true }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Bool>(expr.m_expression));
    CHECK(std::get<Bool>(expr.m_expression).m_value == true);
  }

  TEST_CASE("bust::parse_bool_false") {
    auto program = parse_string("fn main() -> bool { false }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Bool>(expr.m_expression));
    CHECK(std::get<Bool>(expr.m_expression).m_value == false);
  }

  TEST_CASE("bust::parse_unit_literal") {
    auto program = parse_string("fn main() { () }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    CHECK(std::holds_alternative<Unit>(expr.m_expression));
  }

  // === Identifiers =========================================================

  TEST_CASE("bust::parse_identifier_in_body") {
    auto program = parse_string("fn main() -> i64 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Identifier>(expr.m_expression));
    CHECK(std::get<Identifier>(expr.m_expression).m_name == "x");
  }

  // === Function definitions ================================================

  TEST_CASE("bust::parse_minimal_function") {
    auto program = parse_string("fn main() -> i64 { 0 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_signature.m_id.m_name == "main");
    CHECK(func.m_signature.m_parameters.empty());
    check_primitive_type(func.m_signature.m_return_type, PrimitiveType::I64);
  }

  TEST_CASE("bust::parse_function_no_return_type") {
    auto program = parse_string("fn do_nothing() { }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_signature.m_id.m_name == "do_nothing");
    check_primitive_type(func.m_signature.m_return_type, PrimitiveType::UNIT);
    CHECK_FALSE(func.m_body.m_final_expression.has_value());
  }

  TEST_CASE("bust::parse_function_with_params") {
    auto program = parse_string("fn add(a: i64, b: i64) -> i64 { a }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_signature.m_id.m_name == "add");
    REQUIRE(func.m_signature.m_parameters.size() == 2);
    CHECK(func.m_signature.m_parameters[0].m_name == "a");
    REQUIRE(func.m_signature.m_parameters[0].m_type.has_value());
    check_primitive_type(*func.m_signature.m_parameters[0].m_type,
                         PrimitiveType::I64);
    CHECK(func.m_signature.m_parameters[1].m_name == "b");
    REQUIRE(func.m_signature.m_parameters[1].m_type.has_value());
    check_primitive_type(*func.m_signature.m_parameters[1].m_type,
                         PrimitiveType::I64);
  }

  TEST_CASE("bust::parse_function_single_param") {
    auto program = parse_string("fn id(x: i64) -> i64 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    CHECK(func.m_signature.m_parameters[0].m_name == "x");
  }

  // === Let bindings ========================================================

  TEST_CASE("bust::parse_let_without_type") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x = 42;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
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
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    CHECK(binding.m_variable.m_name == "x");
    // Final expression is x
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Identifier>(expr.m_expression));
    CHECK(std::get<Identifier>(expr.m_expression).m_name == "x");
  }

  // === Arithmetic ==========================================================

  TEST_CASE("bust::parse_binary_add") {
    auto program = parse_string("fn main() -> i64 { 1 + 2 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &bin = *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(bin.m_operator == BinaryOperator::PLUS);
    CHECK(std::holds_alternative<I64>(bin.m_lhs.m_expression));
    CHECK(std::holds_alternative<I64>(bin.m_rhs.m_expression));
  }

  TEST_CASE("bust::parse_binary_sub") {
    auto program = parse_string("fn main() -> i64 { 5 - 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::MINUS);
  }

  TEST_CASE("bust::parse_precedence_mul_over_add") {
    // 1 + 2 * 3 should parse as 1 + (2 * 3)
    auto program = parse_string("fn main() -> i64 { 1 + 2 * 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &add = *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(add.m_operator == BinaryOperator::PLUS);
    // LHS is literal 1
    CHECK(std::holds_alternative<I64>(add.m_lhs.m_expression));
    // RHS is 2 * 3
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        add.m_rhs.m_expression));
    const auto &mul =
        *std::get<std::unique_ptr<BinaryExpr>>(add.m_rhs.m_expression);
    CHECK(mul.m_operator == BinaryOperator::MULTIPLIES);
  }

  TEST_CASE("bust::parse_left_associativity") {
    // 1 - 2 - 3 should parse as (1 - 2) - 3
    auto program = parse_string("fn main() -> i64 { 1 - 2 - 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &outer =
        *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(outer.m_operator == BinaryOperator::MINUS);
    // LHS is (1 - 2)
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        outer.m_lhs.m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<BinaryExpr>>(outer.m_lhs.m_expression);
    CHECK(inner.m_operator == BinaryOperator::MINUS);
    CHECK(std::holds_alternative<I64>(inner.m_lhs.m_expression));
    CHECK(std::holds_alternative<I64>(inner.m_rhs.m_expression));
    // RHS is 3
    CHECK(std::holds_alternative<I64>(outer.m_rhs.m_expression));
  }

  TEST_CASE("bust::parse_parenthesized_expression") {
    // (1 + 2) * 3 — parens override precedence
    auto program = parse_string("fn main() -> i64 { (1 + 2) * 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &mul = *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(mul.m_operator == BinaryOperator::MULTIPLIES);
    // LHS is (1 + 2)
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        mul.m_lhs.m_expression));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(mul.m_lhs.m_expression)
              ->m_operator == BinaryOperator::PLUS);
  }

  TEST_CASE("bust::parse_modulus") {
    auto program = parse_string("fn main() -> i64 { 10 % 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::MODULUS);
  }

  TEST_CASE("bust::parse_division") {
    auto program = parse_string("fn main() -> i64 { 10 / 2 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::DIVIDES);
  }

  // === Comparison ==========================================================

  TEST_CASE("bust::parse_less_than") {
    auto program = parse_string("fn main() -> bool { 1 < 2 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::LT);
  }

  TEST_CASE("bust::parse_equality") {
    auto program = parse_string("fn main() -> bool { x == y }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::EQ);
  }

  TEST_CASE("bust::parse_not_equal") {
    auto program = parse_string("fn main() -> bool { x != y }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::NOT_EQ);
  }

  TEST_CASE("bust::parse_comparison_lower_than_arithmetic") {
    // 1 + 2 < 3 + 4 should parse as (1 + 2) < (3 + 4)
    auto program = parse_string("fn main() -> bool { 1 + 2 < 3 + 4 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &cmp = *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(cmp.m_operator == BinaryOperator::LT);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        cmp.m_lhs.m_expression));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(cmp.m_lhs.m_expression)
              ->m_operator == BinaryOperator::PLUS);
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        cmp.m_rhs.m_expression));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(cmp.m_rhs.m_expression)
              ->m_operator == BinaryOperator::PLUS);
  }

  // === Logical =============================================================

  TEST_CASE("bust::parse_logical_and") {
    auto program = parse_string("fn main() -> bool { true && false }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::LOGICAL_AND);
  }

  TEST_CASE("bust::parse_logical_or") {
    auto program = parse_string("fn main() -> bool { true || false }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::LOGICAL_OR);
  }

  TEST_CASE("bust::parse_logical_precedence") {
    // a || b && c should parse as a || (b && c)
    auto program = parse_string("fn main() -> bool { a || b && c }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &lor = *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(lor.m_operator == BinaryOperator::LOGICAL_OR);
    // RHS should be b && c
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        lor.m_rhs.m_expression));
    CHECK(std::get<std::unique_ptr<BinaryExpr>>(lor.m_rhs.m_expression)
              ->m_operator == BinaryOperator::LOGICAL_AND);
  }

  // === Unary ===============================================================

  TEST_CASE("bust::parse_unary_negation") {
    auto program = parse_string("fn main() -> i64 { -42 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<UnaryExpr>>(expr.m_expression));
    const auto &unary =
        *std::get<std::unique_ptr<UnaryExpr>>(expr.m_expression);
    CHECK(unary.m_operator == UnaryOperator::MINUS);
    CHECK(std::holds_alternative<I64>(unary.m_expression.m_expression));
  }

  TEST_CASE("bust::parse_unary_not") {
    auto program = parse_string("fn main() -> bool { !true }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<UnaryExpr>>(expr.m_expression));
    const auto &unary =
        *std::get<std::unique_ptr<UnaryExpr>>(expr.m_expression);
    CHECK(unary.m_operator == UnaryOperator::NOT);
    CHECK(std::holds_alternative<Bool>(unary.m_expression.m_expression));
  }

  // === Function calls ======================================================

  TEST_CASE("bust::parse_function_call_no_args") {
    auto program = parse_string("fn main() -> i64 { foo() }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CallExpr>>(expr.m_expression));
    const auto &call = *std::get<std::unique_ptr<CallExpr>>(expr.m_expression);
    REQUIRE(std::holds_alternative<Identifier>(call.m_callee.m_expression));
    CHECK(std::get<Identifier>(call.m_callee.m_expression).m_name == "foo");
    CHECK(call.m_arguments.empty());
  }

  TEST_CASE("bust::parse_function_call_with_args") {
    auto program = parse_string("fn main() -> i64 { add(1, 2) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CallExpr>>(expr.m_expression));
    const auto &call = *std::get<std::unique_ptr<CallExpr>>(expr.m_expression);
    REQUIRE(std::holds_alternative<Identifier>(call.m_callee.m_expression));
    CHECK(std::get<Identifier>(call.m_callee.m_expression).m_name == "add");
    REQUIRE(call.m_arguments.size() == 2);
    CHECK(std::holds_alternative<I64>(call.m_arguments[0].m_expression));
    CHECK(std::holds_alternative<I64>(call.m_arguments[1].m_expression));
  }

  TEST_CASE("bust::parse_function_call_expression_args") {
    auto program = parse_string("fn main() -> i64 { foo(1 + 2, x) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CallExpr>>(expr.m_expression));
    const auto &call = *std::get<std::unique_ptr<CallExpr>>(expr.m_expression);
    REQUIRE(call.m_arguments.size() == 2);
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        call.m_arguments[0].m_expression));
    CHECK(std::holds_alternative<Identifier>(call.m_arguments[1].m_expression));
  }

  // === If expressions ======================================================

  TEST_CASE("bust::parse_if_no_else") {
    auto program = parse_string("fn main() { if true { 1; } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(expr.m_expression));
    const auto &if_expr = *std::get<std::unique_ptr<IfExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<Bool>(if_expr.m_condition.m_expression));
    CHECK_FALSE(if_expr.m_else_block.has_value());
  }

  TEST_CASE("bust::parse_if_else") {
    auto program =
        parse_string("fn main() -> i64 { if true { 1 } else { 2 } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(expr.m_expression));
    const auto &if_expr = *std::get<std::unique_ptr<IfExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<Bool>(if_expr.m_condition.m_expression));
    REQUIRE(if_expr.m_else_block.has_value());
    // Then block has final expr 1
    REQUIRE(if_expr.m_then_block.m_final_expression.has_value());
    CHECK(std::holds_alternative<I64>(
        if_expr.m_then_block.m_final_expression->m_expression));
    // Else block has final expr 2
    REQUIRE(if_expr.m_else_block->m_final_expression.has_value());
    CHECK(std::holds_alternative<I64>(
        if_expr.m_else_block->m_final_expression->m_expression));
  }

  TEST_CASE("bust::parse_if_with_comparison") {
    auto program =
        parse_string("fn main() -> i64 { if x < 10 { 1 } else { 2 } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(expr.m_expression));
    const auto &if_expr = *std::get<std::unique_ptr<IfExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        if_expr.m_condition.m_expression));
  }

  // === Return ==============================================================

  TEST_CASE("bust::parse_return") {
    auto program = parse_string("fn main() -> i64 { return 42 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<ReturnExpr>>(expr.m_expression));
    const auto &ret = *std::get<std::unique_ptr<ReturnExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<I64>(ret.m_expression.m_expression));
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
    REQUIRE(std::holds_alternative<std::unique_ptr<ReturnExpr>>(
        stmt_expr.m_expression));
    const auto &ret =
        *std::get<std::unique_ptr<ReturnExpr>>(stmt_expr.m_expression);
    CHECK(std::holds_alternative<I64>(ret.m_expression.m_expression));
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
    REQUIRE(std::holds_alternative<std::unique_ptr<ReturnExpr>>(
        stmt_expr.m_expression));
    const auto &ret =
        *std::get<std::unique_ptr<ReturnExpr>>(stmt_expr.m_expression);
    CHECK(std::holds_alternative<I64>(ret.m_expression.m_expression));
    // Final expression: 0
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<I64>(
        func.m_body.m_final_expression->m_expression));
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
    CHECK(std::holds_alternative<I64>(
        func.m_body.m_final_expression->m_expression));
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
        std::get<Expression>(func.m_body.m_statements[0]).m_expression));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<I64>(
        func.m_body.m_final_expression->m_expression));
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
        std::get<Expression>(func.m_body.m_statements[0]).m_expression));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<I64>(
        func.m_body.m_final_expression->m_expression));
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
        std::get<Expression>(func.m_body.m_statements[0]).m_expression));
    CHECK(std::holds_alternative<std::unique_ptr<Block>>(
        std::get<Expression>(func.m_body.m_statements[1]).m_expression));
    CHECK(std::holds_alternative<std::unique_ptr<IfExpr>>(
        std::get<Expression>(func.m_body.m_statements[2]).m_expression));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<I64>(
        func.m_body.m_final_expression->m_expression));
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
    CHECK(std::holds_alternative<Identifier>(
        func.m_body.m_final_expression->m_expression));
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
    CHECK(std::holds_alternative<I64>(
        func.m_body.m_final_expression->m_expression));
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
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    CHECK(std::holds_alternative<std::unique_ptr<Block>>(
        binding.m_expression.m_expression));
  }

  // === Lambda ==============================================================

  TEST_CASE("bust::parse_lambda_no_params") {
    auto program = parse_string("fn main() -> i64 { || { 42 } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr.m_expression));
    const auto &lambda =
        *std::get<std::unique_ptr<LambdaExpr>>(expr.m_expression);
    CHECK(lambda.m_parameters.empty());
    CHECK_FALSE(lambda.m_return_type.has_value());
  }

  TEST_CASE("bust::parse_lambda_with_params") {
    auto program =
        parse_string("fn main() -> i64 { |x: i64, y: i64| -> i64 { x } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr.m_expression));
    const auto &lambda =
        *std::get<std::unique_ptr<LambdaExpr>>(expr.m_expression);
    REQUIRE(lambda.m_parameters.size() == 2);
    CHECK(lambda.m_parameters[0].m_name == "x");
    CHECK(lambda.m_parameters[1].m_name == "y");
    REQUIRE(lambda.m_return_type.has_value());
    check_primitive_type(*lambda.m_return_type, PrimitiveType::I64);
  }

  TEST_CASE("bust::parse_lambda_inferred_param_types") {
    auto program = parse_string("fn main() -> i64 { |x, y| { x } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr.m_expression));
    const auto &lambda =
        *std::get<std::unique_ptr<LambdaExpr>>(expr.m_expression);
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
    CHECK(std::holds_alternative<FunctionDef>(program.m_items[0]));
    CHECK(std::holds_alternative<FunctionDef>(program.m_items[1]));
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
    const auto &fib = std::get<FunctionDef>(program.m_items[0]);
    CHECK(fib.m_signature.m_id.m_name == "fib");
    REQUIRE(fib.m_signature.m_parameters.size() == 1);
    CHECK(fib.m_signature.m_parameters[0].m_name == "n");

    // Body is an if expression
    const auto &body_expr = get_final_expr(fib.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<IfExpr>>(
        body_expr.m_expression));
    const auto &if_expr =
        *std::get<std::unique_ptr<IfExpr>>(body_expr.m_expression);

    // Condition: n <= 1
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        if_expr.m_condition.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(if_expr.m_condition.m_expression)
            ->m_operator == BinaryOperator::LT_EQ);

    // Else branch: fib(n-1) + fib(n-2)
    REQUIRE(if_expr.m_else_block.has_value());
    REQUIRE(if_expr.m_else_block->m_final_expression.has_value());
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        if_expr.m_else_block->m_final_expression->m_expression));
  }
  // --- Function type annotations -------------------------------------------

  TEST_CASE("bust::parse_function_type_annotation_in_parameter") {
    // fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }
    auto program = parse_string("fn apply(f: fn(i64) -> i64, x: i64) -> i64 {\n"
                                "  f(x)\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    CHECK(func.m_signature.m_id.m_name == "apply");
    REQUIRE(func.m_signature.m_parameters.size() == 2);

    // First param should have a function type annotation
    CHECK(func.m_signature.m_parameters[0].m_name == "f");
    REQUIRE(func.m_signature.m_parameters[0].m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_type.value()));
    const auto &fn_type = *std::get<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_type.value());
    REQUIRE(fn_type.m_parameter_types.size() == 1);
    check_primitive_type(fn_type.m_parameter_types[0], PrimitiveType::I64);
    check_primitive_type(fn_type.m_return_type, PrimitiveType::I64);

    // Second param is normal
    CHECK(func.m_signature.m_parameters[1].m_name == "x");
  }

  TEST_CASE("bust::parse_function_type_no_params") {
    // fn run(f: fn() -> bool) -> bool { f() }
    auto program = parse_string("fn run(f: fn() -> bool) -> bool {\n"
                                "  f()\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    REQUIRE(func.m_signature.m_parameters[0].m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_type.value()));
    const auto &fn_type = *std::get<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_type.value());
    CHECK(fn_type.m_parameter_types.empty());
    check_primitive_type(fn_type.m_return_type, PrimitiveType::BOOL);
  }

  TEST_CASE("bust::parse_function_type_multiple_params") {
    // fn apply(f: fn(i64, bool) -> i64) -> i64 { f(1, true) }
    auto program = parse_string("fn apply(f: fn(i64, bool) -> i64) -> i64 {\n"
                                "  f(1, true)\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters[0].m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_type.value()));
    const auto &fn_type = *std::get<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_type.value());
    REQUIRE(fn_type.m_parameter_types.size() == 2);
    check_primitive_type(fn_type.m_parameter_types[0], PrimitiveType::I64);
    check_primitive_type(fn_type.m_parameter_types[1], PrimitiveType::BOOL);
    check_primitive_type(fn_type.m_return_type, PrimitiveType::I64);
  }

  // === Error cases ===========================================================

  TEST_CASE("bust::parse_missing_rbrace") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 { 0"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_missing_lbrace") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_missing_fn_keyword") {
    CHECK_THROWS_AS(parse_string("main() -> i64 { 0 }"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_missing_semicolon_after_let") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 {\n"
                                 "  let x = 42\n"
                                 "  x\n"
                                 "}"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_unexpected_token") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 { ; }"),
                    core::CompilerException);
  }

  // === Comparison operators (GT, GT_EQ, LT_EQ) ==============================

  TEST_CASE("bust::parse_greater_than") {
    auto program = parse_string("fn main() -> bool { 5 > 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::GT);
  }

  TEST_CASE("bust::parse_greater_than_or_equal") {
    auto program = parse_string("fn main() -> bool { 5 >= 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::GT_EQ);
  }

  TEST_CASE("bust::parse_less_than_or_equal") {
    auto program = parse_string("fn main() -> bool { 5 <= 3 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    CHECK(
        std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression)->m_operator ==
        BinaryOperator::LT_EQ);
  }

  // === Chained and nested calls ==============================================

  TEST_CASE("bust::parse_nested_function_call") {
    // f(g(x)) — call as argument
    auto program = parse_string("fn main() -> i64 { f(g(x)) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CallExpr>>(expr.m_expression));
    const auto &outer_call =
        *std::get<std::unique_ptr<CallExpr>>(expr.m_expression);
    REQUIRE(outer_call.m_arguments.size() == 1);
    CHECK(std::holds_alternative<std::unique_ptr<CallExpr>>(
        outer_call.m_arguments[0].m_expression));
  }

  // === Nested lambdas ========================================================

  TEST_CASE("bust::parse_nested_lambda") {
    auto program =
        parse_string("fn main() -> i64 {\n"
                     "  let f = |x: i64| -> i64 { |y: i64| -> i64 { x } };\n"
                     "  0\n"
                     "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<LambdaExpr>>(
        binding.m_expression.m_expression));
    const auto &outer_lambda = *std::get<std::unique_ptr<LambdaExpr>>(
        binding.m_expression.m_expression);
    // The body's final expression should be another lambda
    REQUIRE(outer_lambda.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<std::unique_ptr<LambdaExpr>>(
        outer_lambda.m_body.m_final_expression->m_expression));
  }

  // === Chained unary is not supported ========================================

  TEST_CASE("bust::parse_double_negation_rejects") {
    // Parser doesn't support --42 (double unary minus)
    CHECK_THROWS_AS(parse_string("fn main() -> i64 { --42 }"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_double_not_rejects") {
    // Parser doesn't support !!true (double unary not)
    CHECK_THROWS_AS(parse_string("fn main() -> bool { !!true }"),
                    core::CompilerException);
  }

  // === Nested blocks =========================================================

  TEST_CASE("bust::parse_nested_blocks") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  {\n"
                                "    { 42 }\n"
                                "  }\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<std::unique_ptr<Block>>(expr.m_expression));
    const auto &outer_block =
        *std::get<std::unique_ptr<Block>>(expr.m_expression);
    REQUIRE(outer_block.m_final_expression.has_value());
    CHECK(std::holds_alternative<std::unique_ptr<Block>>(
        outer_block.m_final_expression->m_expression));
  }

  // === Unary on expression (not just literal) ================================

  TEST_CASE("bust::parse_unary_minus_on_parenthesized") {
    auto program = parse_string("fn main() -> i64 { -(1 + 2) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<UnaryExpr>>(expr.m_expression));
    const auto &unary =
        *std::get<std::unique_ptr<UnaryExpr>>(expr.m_expression);
    CHECK(unary.m_operator == UnaryOperator::MINUS);
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        unary.m_expression.m_expression));
  }

  // === Char literals =========================================================

  TEST_CASE("bust::parse_char_literal_printable") {
    auto program = parse_string("fn main() -> char { 'A' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == 'A');
  }

  TEST_CASE("bust::parse_char_literal_digit") {
    auto program = parse_string("fn main() -> char { '7' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '7');
  }

  TEST_CASE("bust::parse_char_literal_space") {
    auto program = parse_string("fn main() -> char { ' ' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == ' ');
  }

  TEST_CASE("bust::parse_char_literal_escape_newline") {
    auto program = parse_string("fn main() -> char { '\\n' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\n');
  }

  TEST_CASE("bust::parse_char_literal_escape_tab") {
    auto program = parse_string("fn main() -> char { '\\t' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\t');
  }

  TEST_CASE("bust::parse_char_literal_escape_null") {
    auto program = parse_string("fn main() -> char { '\\0' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\0');
  }

  TEST_CASE("bust::parse_char_literal_escape_backslash") {
    auto program = parse_string("fn main() -> char { '\\\\' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\\');
  }

  TEST_CASE("bust::parse_char_literal_escape_single_quote") {
    auto program = parse_string("fn main() -> char { '\\'' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\'');
  }

  TEST_CASE("bust::parse_char_literal_hex_escape") {
    // '\x41' is 'A' (0x41 = 65)
    auto program = parse_string("fn main() -> char { '\\x41' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == 'A');
  }

  TEST_CASE("bust::parse_char_literal_hex_escape_null") {
    // '\x00' is null
    auto program = parse_string("fn main() -> char { '\\x00' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Char>(expr.m_expression));
    CHECK(std::get<Char>(expr.m_expression).m_value == '\0');
  }

  // === Type annotations (i8, i32, char) ======================================

  TEST_CASE("bust::parse_type_annotation_i8") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i8 = 42;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::I8);
  }

  TEST_CASE("bust::parse_type_annotation_i32") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i32 = 42;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_type_annotation_char") {
    auto program = parse_string("fn main() -> char { 'A' }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    check_primitive_type(func.m_signature.m_return_type, PrimitiveType::CHAR);
  }

  TEST_CASE("bust::parse_function_param_i8") {
    auto program = parse_string("fn foo(x: i8) -> i8 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    REQUIRE(func.m_signature.m_parameters[0].m_type.has_value());
    check_primitive_type(*func.m_signature.m_parameters[0].m_type,
                         PrimitiveType::I8);
    check_primitive_type(func.m_signature.m_return_type, PrimitiveType::I8);
  }

  // === Cast expressions ======================================================

  TEST_CASE("bust::parse_cast_simple") {
    // 42 as i8
    auto program = parse_string("fn main() -> i8 { 42 as i8 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<I64>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I8);
  }

  TEST_CASE("bust::parse_cast_to_i32") {
    auto program = parse_string("fn main() -> i32 { x as i32 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<Identifier>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_cast_to_char") {
    auto program = parse_string("fn main() -> char { 65 as char }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    check_primitive_type(cast.m_new_type, PrimitiveType::CHAR);
  }

  TEST_CASE("bust::parse_cast_chained") {
    // x as i32 as i64 — left-associative: (x as i32) as i64
    auto program = parse_string("fn main() -> i64 { x as i32 as i64 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &outer = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    check_primitive_type(outer.m_new_type, PrimitiveType::I64);
    // Inner is also a cast
    REQUIRE(std::holds_alternative<std::unique_ptr<CastExpr>>(
        outer.m_expression.m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<CastExpr>>(outer.m_expression.m_expression);
    CHECK(std::holds_alternative<Identifier>(inner.m_expression.m_expression));
    check_primitive_type(inner.m_new_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_cast_lower_precedence_than_arithmetic") {
    // x as i32 + 1 should parse as (x as i32) + 1
    auto program = parse_string("fn main() -> i64 { x as i32 + 1 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<BinaryExpr>>(expr.m_expression));
    const auto &add = *std::get<std::unique_ptr<BinaryExpr>>(expr.m_expression);
    CHECK(add.m_operator == BinaryOperator::PLUS);
    // LHS is the cast
    REQUIRE(std::holds_alternative<std::unique_ptr<CastExpr>>(
        add.m_lhs.m_expression));
    const auto &cast =
        *std::get<std::unique_ptr<CastExpr>>(add.m_lhs.m_expression);
    CHECK(std::holds_alternative<Identifier>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I32);
    // RHS is literal 1
    CHECK(std::holds_alternative<I64>(add.m_rhs.m_expression));
  }

  TEST_CASE("bust::parse_cast_with_unary") {
    // -x as i32 should parse as -(x as i32)
    auto program = parse_string("fn main() -> i32 { -x as i32 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<UnaryExpr>>(expr.m_expression));
    const auto &unary =
        *std::get<std::unique_ptr<UnaryExpr>>(expr.m_expression);
    CHECK(unary.m_operator == UnaryOperator::MINUS);
    REQUIRE(std::holds_alternative<std::unique_ptr<CastExpr>>(
        unary.m_expression.m_expression));
    const auto &cast =
        *std::get<std::unique_ptr<CastExpr>>(unary.m_expression.m_expression);
    CHECK(std::holds_alternative<Identifier>(cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_cast_on_call_result") {
    // foo() as i8
    auto program = parse_string("fn main() -> i8 { foo() as i8 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<std::unique_ptr<CallExpr>>(
        cast.m_expression.m_expression));
    check_primitive_type(cast.m_new_type, PrimitiveType::I8);
  }

  // === Extern function declarations =========================================

  static const ExternFunctionDeclaration &get_single_extern(
      const Program &program) {
    REQUIRE(program.m_items.size() == 1);
    REQUIRE(
        std::holds_alternative<ExternFunctionDeclaration>(program.m_items[0]));
    return std::get<ExternFunctionDeclaration>(program.m_items[0]);
  }

  TEST_CASE("bust::parse_extern_function_no_params") {
    auto program = parse_string("extern fn abort() -> i64;");
    DUMP_AST(program);
    const auto &ext = get_single_extern(program);
    CHECK(ext.m_signature.m_id.m_name == "abort");
    CHECK(ext.m_signature.m_parameters.empty());
    check_primitive_type(ext.m_signature.m_return_type, PrimitiveType::I64);
  }

  TEST_CASE("bust::parse_extern_function_with_param") {
    auto program = parse_string("extern fn putchar(c: i32) -> i32;");
    DUMP_AST(program);
    const auto &ext = get_single_extern(program);
    CHECK(ext.m_signature.m_id.m_name == "putchar");
    REQUIRE(ext.m_signature.m_parameters.size() == 1);
    CHECK(ext.m_signature.m_parameters[0].m_name == "c");
    REQUIRE(ext.m_signature.m_parameters[0].m_type.has_value());
    check_primitive_type(*ext.m_signature.m_parameters[0].m_type,
                         PrimitiveType::I32);
    check_primitive_type(ext.m_signature.m_return_type, PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_extern_function_no_return_type") {
    auto program = parse_string("extern fn log_message(msg: i64);");
    DUMP_AST(program);
    const auto &ext = get_single_extern(program);
    CHECK(ext.m_signature.m_id.m_name == "log_message");
    check_primitive_type(ext.m_signature.m_return_type, PrimitiveType::UNIT);
  }

  TEST_CASE("bust::parse_extern_alongside_func_def") {
    auto program = parse_string("extern fn putchar(c: i32) -> i32;\n"
                                "fn main() -> i64 { 0 }");
    DUMP_AST(program);
    REQUIRE(program.m_items.size() == 2);
    REQUIRE(
        std::holds_alternative<ExternFunctionDeclaration>(program.m_items[0]));
    REQUIRE(std::holds_alternative<FunctionDef>(program.m_items[1]));
    const auto &ext = std::get<ExternFunctionDeclaration>(program.m_items[0]);
    CHECK(ext.m_signature.m_id.m_name == "putchar");
    const auto &func = std::get<FunctionDef>(program.m_items[1]);
    CHECK(func.m_signature.m_id.m_name == "main");
  }

  // === Cast expressions ======================================================

  TEST_CASE("bust::parse_cast_in_let_binding") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x: i8 = 42 as i8;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::I8);
    CHECK(std::holds_alternative<std::unique_ptr<CastExpr>>(
        binding.m_expression.m_expression));
  }

  // === Tuple construction ====================================================

  TEST_CASE("bust::parse_tuple_two_elements") {
    auto program = parse_string("fn main() -> i64 { (1, 2) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &tup = *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(tup.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<I64>(tup.m_fields[0].m_expression));
    CHECK(std::get<I64>(tup.m_fields[0].m_expression).m_value == 1);
    REQUIRE(std::holds_alternative<I64>(tup.m_fields[1].m_expression));
    CHECK(std::get<I64>(tup.m_fields[1].m_expression).m_value == 2);
  }

  TEST_CASE("bust::parse_tuple_trailing_comma") {
    // (1, 2,) is the same tuple as (1, 2).
    auto program = parse_string("fn main() -> i64 { (1, 2,) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &tup = *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    CHECK(tup.m_fields.size() == 2);
  }

  TEST_CASE("bust::parse_one_tuple") {
    // (x,) — trailing comma is required for 1-tuples.
    auto program = parse_string("fn main() -> i64 { (x,) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &tup = *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(tup.m_fields.size() == 1);
    REQUIRE(std::holds_alternative<Identifier>(tup.m_fields[0].m_expression));
    CHECK(std::get<Identifier>(tup.m_fields[0].m_expression).m_name == "x");
  }

  TEST_CASE("bust::parse_nested_tuple_inner_first") {
    // ((1, 2), 3) — first element is itself a 2-tuple.
    auto program = parse_string("fn main() -> i64 { ((1, 2), 3) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &outer =
        *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(outer.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[0].m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[0].m_expression);
    REQUIRE(inner.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<I64>(inner.m_fields[0].m_expression));
    CHECK(std::get<I64>(inner.m_fields[0].m_expression).m_value == 1);
    REQUIRE(std::holds_alternative<I64>(inner.m_fields[1].m_expression));
    CHECK(std::get<I64>(inner.m_fields[1].m_expression).m_value == 2);
    REQUIRE(std::holds_alternative<I64>(outer.m_fields[1].m_expression));
    CHECK(std::get<I64>(outer.m_fields[1].m_expression).m_value == 3);
  }

  TEST_CASE("bust::parse_tuple_of_tuples") {
    // ((1, 2), (3, 4)) — both elements are tuples.
    auto program = parse_string("fn main() -> i64 { ((1, 2), (3, 4)) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &outer =
        *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(outer.m_fields.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[0].m_expression));
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[1].m_expression));
    const auto &left =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[0].m_expression);
    const auto &right =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[1].m_expression);
    CHECK(left.m_fields.size() == 2);
    CHECK(right.m_fields.size() == 2);
  }

  TEST_CASE("bust::parse_nested_one_tuple") {
    // ((x,),) — 1-tuple containing a 1-tuple; stress-tests trailing commas
    // at multiple depths.
    auto program = parse_string("fn main() -> i64 { ((x,),) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<TupleExpr>>(expr.m_expression));
    const auto &outer =
        *std::get<std::unique_ptr<TupleExpr>>(expr.m_expression);
    REQUIRE(outer.m_fields.size() == 1);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleExpr>>(
        outer.m_fields[0].m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<TupleExpr>>(outer.m_fields[0].m_expression);
    REQUIRE(inner.m_fields.size() == 1);
    REQUIRE(std::holds_alternative<Identifier>(inner.m_fields[0].m_expression));
    CHECK(std::get<Identifier>(inner.m_fields[0].m_expression).m_name == "x");
  }

  TEST_CASE("bust::parse_parenthesized_identifier_is_not_tuple") {
    // (x) is just x — no trailing comma means no tuple.
    auto program = parse_string("fn main() -> i64 { (x) }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(std::holds_alternative<Identifier>(expr.m_expression));
    CHECK(std::get<Identifier>(expr.m_expression).m_name == "x");
  }

  // === Tuple type annotations ===============================================

  TEST_CASE("bust::parse_tuple_type_in_let") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let t: (i64, bool) = (1, true);\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &tt = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(tt.m_field_types.size() == 2);
    check_primitive_type(tt.m_field_types[0], PrimitiveType::I64);
    check_primitive_type(tt.m_field_types[1], PrimitiveType::BOOL);
  }

  TEST_CASE("bust::parse_tuple_of_tuple_type_in_let") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let t: ((i64,), bool) = ((1,), true);\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &tt = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(tt.m_field_types.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        tt.m_field_types[0]));
    check_primitive_type(
        std::get<std::unique_ptr<TupleTypeIdentifier>>(tt.m_field_types[0])
            ->m_field_types[0],
        PrimitiveType::I64);
    check_primitive_type(tt.m_field_types[1], PrimitiveType::BOOL);
  }

  TEST_CASE("bust::parse_nested_tuple_type_in_let") {
    // ((i64, i64), bool) — nested tuple in the first position.
    auto program =
        parse_string("fn main() -> i64 {\n"
                     "  let t: ((i64, i64), bool) = ((1, 2), true);\n"
                     "  0\n"
                     "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &outer = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(outer.m_field_types.size() == 2);
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        outer.m_field_types[0]));
    const auto &inner =
        *std::get<std::unique_ptr<TupleTypeIdentifier>>(outer.m_field_types[0]);
    REQUIRE(inner.m_field_types.size() == 2);
    check_primitive_type(inner.m_field_types[0], PrimitiveType::I64);
    check_primitive_type(inner.m_field_types[1], PrimitiveType::I64);
    check_primitive_type(outer.m_field_types[1], PrimitiveType::BOOL);
  }

  TEST_CASE("bust::parse_deeply_nested_tuple_type") {
    // (i64, (bool, (char, i32))) — three levels of nesting, right-heavy.
    auto program = parse_string(
        "fn main() -> i64 {\n"
        "  let t: (i64, (bool, (char, i32))) = (1, (true, ('a', 2)));\n"
        "  0\n"
        "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    const auto &lvl0 = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(lvl0.m_field_types.size() == 2);
    check_primitive_type(lvl0.m_field_types[0], PrimitiveType::I64);
    const auto &lvl1 =
        *std::get<std::unique_ptr<TupleTypeIdentifier>>(lvl0.m_field_types[1]);
    REQUIRE(lvl1.m_field_types.size() == 2);
    check_primitive_type(lvl1.m_field_types[0], PrimitiveType::BOOL);
    const auto &lvl2 =
        *std::get<std::unique_ptr<TupleTypeIdentifier>>(lvl1.m_field_types[1]);
    REQUIRE(lvl2.m_field_types.size() == 2);
    check_primitive_type(lvl2.m_field_types[0], PrimitiveType::CHAR);
    check_primitive_type(lvl2.m_field_types[1], PrimitiveType::I32);
  }

  TEST_CASE("bust::parse_one_tuple_type") {
    // (i64,) — 1-tuple type requires trailing comma.
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let t: (i64,) = (1,);\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    REQUIRE(binding.m_variable.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type));
    const auto &tt = *std::get<std::unique_ptr<TupleTypeIdentifier>>(
        *binding.m_variable.m_type);
    REQUIRE(tt.m_field_types.size() == 1);
    check_primitive_type(tt.m_field_types[0], PrimitiveType::I64);
  }

  // === Dot projection =======================================================

  TEST_CASE("bust::parse_dot_projection") {
    auto program = parse_string("fn main() -> i64 { t.0 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<DotExpr>>(expr.m_expression));
    const auto &dot = *std::get<std::unique_ptr<DotExpr>>(expr.m_expression);
    CHECK(dot.m_tuple_index == 0);
    REQUIRE(std::holds_alternative<Identifier>(dot.m_expression.m_expression));
    CHECK(std::get<Identifier>(dot.m_expression.m_expression).m_name == "t");
  }

  TEST_CASE("bust::parse_chained_dot_projection") {
    // t.0.1 parses as (t.0).1 — left-associative.
    auto program = parse_string("fn main() -> i64 { t.0.1 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<DotExpr>>(expr.m_expression));
    const auto &outer = *std::get<std::unique_ptr<DotExpr>>(expr.m_expression);
    CHECK(outer.m_tuple_index == 1);
    REQUIRE(std::holds_alternative<std::unique_ptr<DotExpr>>(
        outer.m_expression.m_expression));
    const auto &inner =
        *std::get<std::unique_ptr<DotExpr>>(outer.m_expression.m_expression);
    CHECK(inner.m_tuple_index == 0);
  }

  TEST_CASE("bust::parse_dot_tighter_than_cast") {
    // t.0 as i32 parses as (t.0) as i32.
    auto program = parse_string("fn main() -> i32 { t.0 as i32 }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<CastExpr>>(expr.m_expression));
    const auto &cast = *std::get<std::unique_ptr<CastExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<std::unique_ptr<DotExpr>>(
        cast.m_expression.m_expression));
  }
}
//****************************************************************************
} // namespace bust
//****************************************************************************
