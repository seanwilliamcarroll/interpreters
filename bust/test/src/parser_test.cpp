//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::Parser — literals, function and
//*            lambda definitions, let bindings, assignments, binary and
//*            unary expressions (with precedence), function calls, if,
//*            return, blocks, function-type annotations, error cases,
//*            comparisons, and nesting.
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
TEST_SUITE("bust.parser.core") {

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
    CHECK(func.m_signature.m_parameters[0].m_id.m_name == "a");
    REQUIRE(func.m_signature.m_parameters[0].m_id.m_type.has_value());
    check_primitive_type(*func.m_signature.m_parameters[0].m_id.m_type,
                         PrimitiveType::I64);
    CHECK(func.m_signature.m_parameters[1].m_id.m_name == "b");
    REQUIRE(func.m_signature.m_parameters[1].m_id.m_type.has_value());
    check_primitive_type(*func.m_signature.m_parameters[1].m_id.m_type,
                         PrimitiveType::I64);
  }

  TEST_CASE("bust::parse_function_single_param") {
    auto program = parse_string("fn id(x: i64) -> i64 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    CHECK(func.m_signature.m_parameters[0].m_id.m_name == "x");
  }

  TEST_CASE("bust::parse_function_with_mut_param") {
    auto program = parse_string("fn f(mut x: i64) -> i64 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    CHECK(func.m_signature.m_parameters[0].m_id.m_name == "x");
    CHECK(func.m_signature.m_parameters[0].m_is_mutable == true);
  }

  TEST_CASE("bust::parse_function_with_mixed_mut_params") {
    auto program = parse_string("fn f(x: i64, mut y: i64) -> i64 { x }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 2);
    CHECK(func.m_signature.m_parameters[0].m_id.m_name == "x");
    CHECK(func.m_signature.m_parameters[0].m_is_mutable == false);
    CHECK(func.m_signature.m_parameters[1].m_id.m_name == "y");
    CHECK(func.m_signature.m_parameters[1].m_is_mutable == true);
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

  TEST_CASE("bust::parse_let_immutable_sets_is_mutable_false") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let x = 42;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    CHECK(binding.m_is_mutable == false);
  }

  // === Mutable let bindings ================================================

  TEST_CASE("bust::parse_let_mut_without_type") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let mut x = 42;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    CHECK(binding.m_variable.m_name == "x");
    CHECK(binding.m_is_mutable == true);
    CHECK_FALSE(binding.m_variable.m_type.has_value());
  }

  TEST_CASE("bust::parse_let_mut_with_type") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let mut x: i64 = 10;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<LetBinding>(func.m_body.m_statements[0]));
    const auto &binding = std::get<LetBinding>(func.m_body.m_statements[0]);
    CHECK(binding.m_variable.m_name == "x");
    CHECK(binding.m_is_mutable == true);
    REQUIRE(binding.m_variable.m_type.has_value());
    check_primitive_type(*binding.m_variable.m_type, PrimitiveType::I64);
  }

  // === Assignment statements ===============================================

  TEST_CASE("bust::parse_assignment_simple") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let mut x = 1;\n"
                                "  x = 2;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 2);
    REQUIRE(std::holds_alternative<Assignment>(func.m_body.m_statements[1]));
    const auto &assign = std::get<Assignment>(func.m_body.m_statements[1]);
    REQUIRE(std::holds_alternative<Identifier>(assign.m_lhs.m_expression));
    CHECK(std::get<Identifier>(assign.m_lhs.m_expression).m_name == "x");
    REQUIRE(std::holds_alternative<I64>(assign.m_rhs.m_expression));
    CHECK(std::get<I64>(assign.m_rhs.m_expression).m_value == 2);
  }

  TEST_CASE("bust::parse_assignment_with_expression_rhs") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  let mut x = 1;\n"
                                "  x = x + 1;\n"
                                "  x\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 2);
    REQUIRE(std::holds_alternative<Assignment>(func.m_body.m_statements[1]));
    const auto &assign = std::get<Assignment>(func.m_body.m_statements[1]);
    REQUIRE(std::holds_alternative<Identifier>(assign.m_lhs.m_expression));
    CHECK(std::get<Identifier>(assign.m_lhs.m_expression).m_name == "x");
    REQUIRE(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        assign.m_rhs.m_expression));
    const auto &bin =
        *std::get<std::unique_ptr<BinaryExpr>>(assign.m_rhs.m_expression);
    CHECK(bin.m_operator == BinaryOperator::PLUS);
  }

  // Approach A: the parser is permissive about LHS shape — it parses the
  // LHS as a full Expression and a later phase rejects non-place forms.
  // This test documents that contract: a literal LHS produces a valid
  // Assignment node at parse time, even though it will be rejected later.
  TEST_CASE("bust::parse_assignment_with_literal_lhs_is_permitted") {
    auto program = parse_string("fn main() -> i64 {\n"
                                "  5 = 6;\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Assignment>(func.m_body.m_statements[0]));
    const auto &assign = std::get<Assignment>(func.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<I64>(assign.m_lhs.m_expression));
    CHECK(std::get<I64>(assign.m_lhs.m_expression).m_value == 5);
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

  // --- Naked return (no operand) -------------------------------------------
  //
  // `return;` is sugar for `return ();`. The parser desugars at the syntax
  // layer: a ReturnExpr with no operand is constructed wrapping a Unit
  // literal, so later passes (type checking, codegen) need no special
  // case. If a dedicated desugaring pass is later introduced, the parser
  // could move that expansion there.

  TEST_CASE("bust::parse_naked_return_as_statement") {
    auto program = parse_string("fn f() { return; }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    const auto &stmt_expr = std::get<Expression>(func.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<ReturnExpr>>(
        stmt_expr.m_expression));
    const auto &ret =
        *std::get<std::unique_ptr<ReturnExpr>>(stmt_expr.m_expression);
    // Naked return desugars to `return ();` — operand is a Unit literal.
    CHECK(std::holds_alternative<Unit>(ret.m_expression.m_expression));
    CHECK_FALSE(func.m_body.m_final_expression.has_value());
  }

  TEST_CASE("bust::parse_naked_return_as_final_expression") {
    // `return` with no semicolon and no operand sits as the block's final
    // expression. Its type later resolves to Never; the operand is Unit.
    auto program = parse_string("fn f() { return }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.empty());
    REQUIRE(func.m_body.m_final_expression.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<ReturnExpr>>(
        func.m_body.m_final_expression->m_expression));
    const auto &ret = *std::get<std::unique_ptr<ReturnExpr>>(
        func.m_body.m_final_expression->m_expression);
    CHECK(std::holds_alternative<Unit>(ret.m_expression.m_expression));
  }

  TEST_CASE("bust::parse_naked_return_inside_if") {
    auto program = parse_string("fn f() {\n"
                                "  if true { return; }\n"
                                "  let x = 5;\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    // if-stmt then let-stmt
    REQUIRE(func.m_body.m_statements.size() == 2);
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
    CHECK(lambda.m_parameters[0].m_id.m_name == "x");
    CHECK(lambda.m_parameters[1].m_id.m_name == "y");
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
    CHECK_FALSE(lambda.m_parameters[0].m_id.m_type.has_value());
    CHECK_FALSE(lambda.m_parameters[1].m_id.m_type.has_value());
  }

  TEST_CASE("bust::parse_lambda_with_mut_param") {
    auto program =
        parse_string("fn main() -> i64 { |mut x: i64| -> i64 { x } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr.m_expression));
    const auto &lambda =
        *std::get<std::unique_ptr<LambdaExpr>>(expr.m_expression);
    REQUIRE(lambda.m_parameters.size() == 1);
    CHECK(lambda.m_parameters[0].m_id.m_name == "x");
    CHECK(lambda.m_parameters[0].m_is_mutable == true);
  }

  TEST_CASE("bust::parse_lambda_with_mut_param_inferred") {
    auto program = parse_string("fn main() -> i64 { |mut x| { x } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<LambdaExpr>>(expr.m_expression));
    const auto &lambda =
        *std::get<std::unique_ptr<LambdaExpr>>(expr.m_expression);
    REQUIRE(lambda.m_parameters.size() == 1);
    CHECK(lambda.m_parameters[0].m_id.m_name == "x");
    CHECK(lambda.m_parameters[0].m_is_mutable == true);
    CHECK_FALSE(lambda.m_parameters[0].m_id.m_type.has_value());
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
    CHECK(fib.m_signature.m_parameters[0].m_id.m_name == "n");

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
    CHECK(func.m_signature.m_parameters[0].m_id.m_name == "f");
    REQUIRE(func.m_signature.m_parameters[0].m_id.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_id.m_type.value()));
    const auto &fn_type = *std::get<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_id.m_type.value());
    REQUIRE(fn_type.m_parameter_types.size() == 1);
    check_primitive_type(fn_type.m_parameter_types[0], PrimitiveType::I64);
    check_primitive_type(fn_type.m_return_type, PrimitiveType::I64);

    // Second param is normal
    CHECK(func.m_signature.m_parameters[1].m_id.m_name == "x");
  }

  TEST_CASE("bust::parse_function_type_no_params") {
    // fn run(f: fn() -> bool) -> bool { f() }
    auto program = parse_string("fn run(f: fn() -> bool) -> bool {\n"
                                "  f()\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_signature.m_parameters.size() == 1);
    REQUIRE(func.m_signature.m_parameters[0].m_id.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_id.m_type.value()));
    const auto &fn_type = *std::get<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_id.m_type.value());
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
    REQUIRE(func.m_signature.m_parameters[0].m_id.m_type.has_value());
    REQUIRE(std::holds_alternative<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_id.m_type.value()));
    const auto &fn_type = *std::get<std::unique_ptr<FunctionTypeIdentifier>>(
        func.m_signature.m_parameters[0].m_id.m_type.value());
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

  TEST_CASE("bust::parse_let_if_requires_trailing_semicolon") {
    // The closing `}` of the if's else block does not terminate the
    // let binding — a trailing semicolon is still required, even
    // though a bare if-expression as a statement does not need one.
    CHECK_THROWS_AS(parse_string("fn main() -> i64 {\n"
                                 "  let x = if true { 1 } else { 2 }\n"
                                 "  x\n"
                                 "}"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_unexpected_token") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 { ; }"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_assignment_missing_semicolon") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 {\n"
                                 "  let mut x = 1;\n"
                                 "  x = 2\n"
                                 "  x\n"
                                 "}"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_assignment_missing_rhs") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 {\n"
                                 "  let mut x = 1;\n"
                                 "  x = ;\n"
                                 "  x\n"
                                 "}"),
                    core::CompilerException);
  }

  TEST_CASE("bust::parse_let_mut_without_identifier") {
    CHECK_THROWS_AS(parse_string("fn main() -> i64 {\n"
                                 "  let mut = 1;\n"
                                 "  0\n"
                                 "}"),
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
}
//****************************************************************************
} // namespace bust
//****************************************************************************
