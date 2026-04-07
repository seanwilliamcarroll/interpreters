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

Handle ExpressionGenerator::operator()(const std::unique_ptr<hir::Block> &) {
  return {};
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
Handle
ExpressionGenerator::operator()(const std::unique_ptr<hir::BinaryExpr> &) {
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
