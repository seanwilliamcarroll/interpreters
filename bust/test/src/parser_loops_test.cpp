//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Unit tests for bust::Parser — while loops and break
//*            expressions.
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
TEST_SUITE("bust.parser.loops") {

  // --- Helpers -------------------------------------------------------------

  static Program parse_string(const std::string &source) {
    std::istringstream input(source);
    auto lexer = make_lexer(input, "test");
    Parser parser(std::move(lexer));
    return parser.parse();
  }

#define DUMP_AST(prog) INFO(Dumper::dump(prog))

  static const FunctionDef &get_single_func(const Program &program) {
    REQUIRE(program.m_items.size() == 1);
    REQUIRE(std::holds_alternative<FunctionDef>(program.m_items[0]));
    return std::get<FunctionDef>(program.m_items[0]);
  }

  static const Expression &get_final_expr(const Block &block) {
    REQUIRE(block.m_final_expression.has_value());
    return *block.m_final_expression;
  }

  // === While ===============================================================

  TEST_CASE("bust::parse_while_simple_empty_body") {
    auto program = parse_string("fn main() { while true {} }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<WhileExpr>>(expr.m_expression));
    const auto &while_expr =
        *std::get<std::unique_ptr<WhileExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<Bool>(while_expr.m_condition.m_expression));
    CHECK(while_expr.m_body.m_statements.empty());
    CHECK_FALSE(while_expr.m_body.m_final_expression.has_value());
  }

  TEST_CASE("bust::parse_while_with_condition_and_body") {
    auto program = parse_string("fn main() { while x < 10 { x = x + 1; } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<WhileExpr>>(expr.m_expression));
    const auto &while_expr =
        *std::get<std::unique_ptr<WhileExpr>>(expr.m_expression);
    CHECK(std::holds_alternative<std::unique_ptr<BinaryExpr>>(
        while_expr.m_condition.m_expression));
    REQUIRE(while_expr.m_body.m_statements.size() == 1);
  }

  TEST_CASE("bust::parse_while_as_statement_no_semicolon") {
    // while is a block-suffixed expression — no trailing semicolon needed
    // when used as a statement (matches if and bare-block).
    auto program = parse_string("fn main() -> i64 {\n"
                                "  while true { 1; }\n"
                                "  0\n"
                                "}");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    CHECK(std::holds_alternative<std::unique_ptr<WhileExpr>>(
        std::get<Expression>(func.m_body.m_statements[0]).m_expression));
    REQUIRE(func.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<I64>(
        func.m_body.m_final_expression->m_expression));
  }

  // === Break ===============================================================

  TEST_CASE("bust::parse_break_in_while_body") {
    auto program = parse_string("fn main() { while true { break; } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<WhileExpr>>(expr.m_expression));
    const auto &while_expr =
        *std::get<std::unique_ptr<WhileExpr>>(expr.m_expression);
    REQUIRE(while_expr.m_body.m_statements.size() == 1);
    REQUIRE(
        std::holds_alternative<Expression>(while_expr.m_body.m_statements[0]));
    const auto &stmt_expr =
        std::get<Expression>(while_expr.m_body.m_statements[0]);
    REQUIRE(std::holds_alternative<std::unique_ptr<BreakExpr>>(
        stmt_expr.m_expression));
    const auto &brk =
        *std::get<std::unique_ptr<BreakExpr>>(stmt_expr.m_expression);
    CHECK(std::holds_alternative<ast::Unit>(brk.m_returned_value.m_expression));
  }

  TEST_CASE("bust::parse_break_as_final_expression") {
    // `break` with no semicolon sits as the body block's final expression.
    // Type checker will give it Never; the parser only cares about syntax.
    auto program = parse_string("fn main() { while true { break } }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    const auto &expr = get_final_expr(func.m_body);
    REQUIRE(
        std::holds_alternative<std::unique_ptr<WhileExpr>>(expr.m_expression));
    const auto &while_expr =
        *std::get<std::unique_ptr<WhileExpr>>(expr.m_expression);
    CHECK(while_expr.m_body.m_statements.empty());
    REQUIRE(while_expr.m_body.m_final_expression.has_value());
    CHECK(std::holds_alternative<std::unique_ptr<BreakExpr>>(
        while_expr.m_body.m_final_expression->m_expression));
  }

  TEST_CASE("bust::parse_break_outside_loop_parses_ok") {
    // The parser does not enforce that break is inside a loop —
    // that's the type checker's job. This program parses cleanly even
    // though it would later be rejected at type-checking.
    auto program = parse_string("fn main() { break; }");
    DUMP_AST(program);
    const auto &func = get_single_func(program);
    REQUIRE(func.m_body.m_statements.size() == 1);
    REQUIRE(std::holds_alternative<Expression>(func.m_body.m_statements[0]));
    CHECK(std::holds_alternative<std::unique_ptr<BreakExpr>>(
        std::get<Expression>(func.m_body.m_statements[0]).m_expression));
  }
}
//****************************************************************************
} // namespace bust
//****************************************************************************
