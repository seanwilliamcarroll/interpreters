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
#include "codegen/statement_generator.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Value ExpressionGenerator::operator()(const hir::Identifier &) { return {}; }

Value ExpressionGenerator::operator()(const hir::LiteralUnit &) { return {}; }

Value ExpressionGenerator::operator()(const hir::LiteralI64 &literal) {
  return {std::to_string(literal.m_value)};
}

Value ExpressionGenerator::operator()(const hir::LiteralBool &) { return {}; }

Value ExpressionGenerator::operator()(const std::unique_ptr<hir::Block> &) {
  return {};
}

Value ExpressionGenerator::operator()(const hir::Block &block) {
  Value last_value{};
  for (const auto &statement : block.m_statements) {
    std::visit(StatementGenerator{m_ctx}, statement);
  }

  if (block.m_final_expression.has_value()) {
    return (*this)(block.m_final_expression.value());
  }

  // Need to return a value here potentially
  return {};
}

Value ExpressionGenerator::operator()(const std::unique_ptr<hir::IfExpr> &) {
  return {};
}
Value ExpressionGenerator::operator()(const std::unique_ptr<hir::CallExpr> &) {
  return {};
}
Value ExpressionGenerator::operator()(
    const std::unique_ptr<hir::BinaryExpr> &) {
  return {};
}
Value ExpressionGenerator::operator()(const std::unique_ptr<hir::UnaryExpr> &) {
  return {};
}
Value ExpressionGenerator::operator()(
    const std::unique_ptr<hir::ReturnExpr> &) {
  return {};
}
Value ExpressionGenerator::operator()(
    const std::unique_ptr<hir::LambdaExpr> &) {
  return {};
}

Value ExpressionGenerator::operator()(const hir::Expression &expression) {
  return std::visit(*this, expression.m_expression);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
