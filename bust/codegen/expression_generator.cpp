//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of expression generator.
//*
//*
//****************************************************************************

#include "codegen/expression_generator.hpp"
#include "codegen/context.hpp"
#include "codegen/statement_generator.hpp"
#include "codegen/symbol_table.hpp"
#include "exceptions.hpp"
#include "operators.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Handle ExpressionGenerator::operator()(const hir::Identifier &identifier) {

  auto ssa_temp = SymbolTable::next_ssa_temporary();

  m_ctx.m_output += " " + ssa_temp + " = load " + identifier.m_type + ", ptr " +
                    m_ctx.m_symbol_table.lookup(identifier.m_name) + "\n";

  return ssa_temp;
}

Handle ExpressionGenerator::operator()(const hir::LiteralUnit &) { return {}; }

Handle ExpressionGenerator::operator()(const hir::LiteralI64 &literal) {
  return {std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const hir::LiteralBool &) { return {}; }

Handle
ExpressionGenerator::operator()(const std::unique_ptr<hir::Block> &block) {
  return (*this)(*block);
}

Handle ExpressionGenerator::operator()(const hir::Block &block) {
  Handle last_value{};
  for (const auto &statement : block.m_statements) {
    last_value = std::visit(StatementGenerator{m_ctx}, statement);
  }

  if (block.m_final_expression.has_value()) {
    return (*this)(block.m_final_expression.value());
  }

  // Need to return a value here potentially
  return last_value;
}

Handle ExpressionGenerator::operator()(const std::unique_ptr<hir::IfExpr> &) {
  return {};
}

Handle ExpressionGenerator::operator()(const std::unique_ptr<hir::CallExpr> &) {
  return {};
}

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {

  auto lhs_handle = (*this)(binary_expression->m_lhs);

  // For short cicuiting, this may become more complicated

  auto rhs_handle = (*this)(binary_expression->m_rhs);

  switch (binary_expression->m_operator) {
  case bust::BinaryOperator::PLUS: {
    auto result_handle = SymbolTable::next_ssa_temporary();

    m_ctx.m_output += "  " + result_handle + " = add " +
                      binary_expression->m_lhs.m_type + " " + lhs_handle +
                      ", " + rhs_handle + "\n";
    return result_handle;
  }

  case bust::BinaryOperator::MINUS: {
    auto result_handle = SymbolTable::next_ssa_temporary();

    m_ctx.m_output += "  " + result_handle + " = sub " +
                      binary_expression->m_lhs.m_type + " " + lhs_handle +
                      ", " + rhs_handle + "\n";
    return result_handle;
  }

  case bust::BinaryOperator::MULTIPLIES: {
    auto result_handle = SymbolTable::next_ssa_temporary();

    m_ctx.m_output += "  " + result_handle + " = mul " +
                      binary_expression->m_lhs.m_type + " " + lhs_handle +
                      ", " + rhs_handle + "\n";
    return result_handle;
  }

  case bust::BinaryOperator::DIVIDES: {
    auto result_handle = SymbolTable::next_ssa_temporary();

    m_ctx.m_output += "  " + result_handle + " = sdiv " +
                      binary_expression->m_lhs.m_type + " " + lhs_handle +
                      ", " + rhs_handle + "\n";
    return result_handle;
  }

  case bust::BinaryOperator::MODULUS: {
    auto result_handle = SymbolTable::next_ssa_temporary();

    m_ctx.m_output += "  " + result_handle + " = srem " +
                      binary_expression->m_lhs.m_type + " " + lhs_handle +
                      ", " + rhs_handle + "\n";
    return result_handle;
  }

  default:
    throw core::CompilerException("Codegen", "UNIMPLEMENTED",
                                  binary_expression->m_location);
  }

  return {};
}

Handle
ExpressionGenerator::operator()(const std::unique_ptr<hir::UnaryExpr> &) {
  return {};
}

Handle
ExpressionGenerator::operator()(const std::unique_ptr<hir::ReturnExpr> &) {
  return {};
}

Handle
ExpressionGenerator::operator()(const std::unique_ptr<hir::LambdaExpr> &) {
  return {};
}

Handle ExpressionGenerator::operator()(const hir::Expression &expression) {
  return std::visit(*this, expression.m_expression);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
