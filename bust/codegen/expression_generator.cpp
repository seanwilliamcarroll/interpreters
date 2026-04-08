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
#include "codegen/basic_block.hpp"
#include "codegen/context.hpp"
#include "codegen/statement_generator.hpp"
#include "codegen/symbol_table.hpp"
#include "codegen/types.hpp"
#include "exceptions.hpp"
#include "hir/types.hpp"
#include "operators.hpp"
#include <string>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Handle ExpressionGenerator::operator()(const hir::Identifier &identifier) {
  auto ssa_temp = SymbolTable::next_ssa_temporary();

  auto &current_block = m_ctx.current_basic_block();

  current_block.add_instruction(LoadInstruction{
      .m_destination = ssa_temp,
      .m_source = m_ctx.m_symbol_table.lookup(identifier.m_name),
      .m_type = to_llvm_type(identifier.m_type)});

  return ssa_temp;
}

Handle ExpressionGenerator::operator()(const hir::LiteralUnit &) { return {}; }

Handle ExpressionGenerator::operator()(const hir::LiteralI64 &literal) {
  return {std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const hir::LiteralBool &literal) {
  return {std::to_string(static_cast<uint8_t>(literal.m_value))};
}

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

LLVMBinaryOperator to_llvm_op(BinaryOperator op) {
  switch (op) {
  case bust::BinaryOperator::PLUS:
    return LLVMBinaryOperator::ADD;
  case bust::BinaryOperator::MINUS:
    return LLVMBinaryOperator::SUB;
  case bust::BinaryOperator::MULTIPLIES:
    return LLVMBinaryOperator::MUL;
  case bust::BinaryOperator::DIVIDES:
    return LLVMBinaryOperator::SDIV;
  case bust::BinaryOperator::MODULUS:
    return LLVMBinaryOperator::SREM;

  default:
    throw core::CompilerException("Codegen", "UNIMPLEMENTED", {});
  }
}

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {

  auto lhs_handle = (*this)(binary_expression->m_lhs);

  // For short cicuiting, this may become more complicated

  auto rhs_handle = (*this)(binary_expression->m_rhs);

  auto result_handle = SymbolTable::next_ssa_temporary();

  auto &basic_block = m_ctx.current_basic_block();

  basic_block.add_instruction(BinaryInstruction{
      .m_result = result_handle,
      .m_lhs = lhs_handle,
      .m_rhs = rhs_handle,
      .m_operator = to_llvm_op(binary_expression->m_operator),
      .m_type = to_llvm_type(binary_expression->m_lhs.m_type)});

  return result_handle;
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
