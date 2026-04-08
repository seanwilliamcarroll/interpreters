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
#include "hir/nodes.hpp"
#include "hir/types.hpp"
#include "operators.hpp"
#include <ios>
#include <sstream>
#include <string>
#include <variant>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Handle ExpressionGenerator::operator()(const hir::Identifier &identifier) {
  auto ssa_temp = SymbolTable::next_ssa_temporary();

  m_ctx.current_basic_block().add_instruction(LoadInstruction{
      .m_destination = ssa_temp,
      .m_source = m_ctx.m_symbol_table.lookup(identifier.m_name),
      .m_type = to_llvm_type(identifier.m_type)});

  return ssa_temp;
}

Handle ExpressionGenerator::operator()(const hir::LiteralUnit &) { return {}; }

Handle ExpressionGenerator::operator()(const hir::LiteralI64 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const hir::LiteralBool &literal) {
  std::stringstream output;
  output << std::boolalpha << literal.m_value;
  return LiteralHandle{output.str()};
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

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::IfExpr> &if_expression) {
  // Need refs to
  //  - entry block for alloca
  //  - current block to add terminator
  //  - then block
  //  - else block
  //  - merge block

  auto &function = m_ctx.current_function();

  const auto &if_type = if_expression->m_then_branch.m_type;
  const auto if_type_is_unit =
      std::holds_alternative<hir::PrimitiveTypeValue>(if_type) &&
      std::get<hir::PrimitiveTypeValue>(if_type).m_type ==
          hir::PrimitiveType::UNIT;

  // We only return a value when there is an else block
  // AND the types of the then and else expressions are NOT Unit
  auto if_statement_returns_value =
      if_expression->m_else_branch.has_value() && !if_type_is_unit;

  auto condition_target = (*this)(if_expression->m_condition);

  // Get handle to current after condition target generated, so even with nested
  // blocks, we're still starting right after the condition
  auto &final_condition_block = function.current_basic_block();

  auto &then_block = function.new_basic_block("then");
  auto &merge_block = function.new_basic_block("merge");

  function.set_insertion_point(then_block);
  auto then_target = (*this)(if_expression->m_then_branch);

  auto result_handle = m_ctx.m_symbol_table.define_local("if_result");

  if (if_statement_returns_value) {
    function.current_basic_block().add_instruction(
        StoreInstruction{.m_destination = result_handle,
                         .m_source = then_target,
                         .m_type = to_llvm_type(if_type)});
  }

  function.current_basic_block().add_terminal(
      JumpInstruction{.m_target = merge_block.m_label});

  if (!if_expression->m_else_branch.has_value()) {
    // Just bare if, else is merge
    // Subsequent instructions should go into merge block
    function.set_insertion_point(merge_block);
    final_condition_block.add_terminal(
        BranchInstruction{.m_condition = condition_target,
                          .m_iftrue = then_block.m_label,
                          .m_iffalse = merge_block.m_label});
    // Void, no handle to return
    return {};
  }

  auto &else_block = function.new_basic_block("else");

  function.set_insertion_point(else_block);
  auto else_target = (*this)(if_expression->m_else_branch.value());

  if (if_statement_returns_value) {
    function.current_basic_block().add_instruction(
        StoreInstruction{.m_destination = result_handle,
                         .m_source = else_target,
                         .m_type = to_llvm_type(if_type)});
  }

  function.current_basic_block().add_terminal(
      JumpInstruction{.m_target = merge_block.m_label});

  final_condition_block.add_terminal(
      BranchInstruction{.m_condition = condition_target,
                        .m_iftrue = then_block.m_label,
                        .m_iffalse = else_block.m_label});

  // Subsequent instructions should go into merge block
  function.set_insertion_point(merge_block);

  // Check if we need to add all the necessary instructions for the result
  if (!if_statement_returns_value) {
    // Void, no handle to return
    return {};
  }

  function.add_alloca_instruction(AllocaInstruction{
      .m_handle = result_handle, .m_type = to_llvm_type(if_type)});

  auto ssa_temp = SymbolTable::next_ssa_temporary();
  merge_block.add_instruction(LoadInstruction{.m_destination = ssa_temp,
                                              .m_source = {result_handle},
                                              .m_type = to_llvm_type(if_type)});
  return ssa_temp;
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

  m_ctx.current_basic_block().add_instruction(BinaryInstruction{
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
