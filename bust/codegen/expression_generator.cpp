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
#include "codegen/instructions.hpp"
#include "codegen/statement_generator.hpp"
#include "codegen/symbol_table.hpp"
#include "codegen/types.hpp"
#include "exceptions.hpp"
#include "hir/nodes.hpp"
#include "hir/types.hpp"
#include "operators.hpp"
#include <ios>
#include <sstream>
#include <stdexcept>
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
  auto &function = m_ctx.current_function();

  const auto &if_return_type = if_expression->m_then_branch.m_type;

  // Get handle to current after condition target generated, so even with nested
  // blocks, we're still starting right after the condition
  auto condition_target = (*this)(if_expression->m_condition);
  auto &final_condition_block = function.current_basic_block();

  // Create starting blocks that will always exist, then and merge
  auto &starting_then_block = function.new_basic_block("then");
  auto &starting_merge_block = function.new_basic_block("merge");

  // Do then branch, capture the final block for later if needed
  function.set_insertion_point(starting_then_block);
  auto then_target = (*this)(if_expression->m_then_branch);
  auto &final_then_block = function.current_basic_block();
  final_then_block.add_terminal(
      JumpInstruction{.m_target = starting_merge_block.m_label});

  if (!if_expression->m_else_branch.has_value()) {
    // Just bare if, else is merge
    final_condition_block.add_terminal(
        BranchInstruction{.m_condition = condition_target,
                          .m_iftrue = starting_then_block.m_label,
                          .m_iffalse = starting_merge_block.m_label});
    // Subsequent instructions should go into merge block
    function.set_insertion_point(starting_merge_block);
    // Void, no handle to return
    return {};
  }

  // Now we handle the else, capturing first block and last block related to
  // this branch
  auto &starting_else_block = function.new_basic_block("else");
  function.set_insertion_point(starting_else_block);
  auto else_target = (*this)(if_expression->m_else_branch.value());
  auto &final_else_block = function.current_basic_block();
  final_else_block.add_terminal(
      JumpInstruction{.m_target = starting_merge_block.m_label});

  // Now we can merge on then vs else
  final_condition_block.add_terminal(
      BranchInstruction{.m_condition = condition_target,
                        .m_iftrue = starting_then_block.m_label,
                        .m_iffalse = starting_else_block.m_label});

  // Subsequent instructions should go into merge block
  function.set_insertion_point(starting_merge_block);

  // We only return a value when there is an else block
  // AND the types of the then and else expressions are NOT Unit
  const auto if_type_is_unit =
      std::holds_alternative<hir::PrimitiveTypeValue>(if_return_type) &&
      std::get<hir::PrimitiveTypeValue>(if_return_type).m_type ==
          hir::PrimitiveType::UNIT;
  auto if_statement_returns_value =
      if_expression->m_else_branch.has_value() && !if_type_is_unit;
  if (!if_statement_returns_value) {
    // Void, no handle to return
    return {};
  }

  auto result_handle = m_ctx.m_symbol_table.define_local("if_result");

  function.add_alloca_instruction(AllocaInstruction{
      .m_handle = result_handle, .m_type = to_llvm_type(if_return_type)});

  // Need to store the value generated by each branch for use in the merge block
  final_then_block.add_instruction(
      StoreInstruction{.m_destination = result_handle,
                       .m_source = then_target,
                       .m_type = to_llvm_type(if_return_type)});

  final_else_block.add_instruction(
      StoreInstruction{.m_destination = result_handle,
                       .m_source = else_target,
                       .m_type = to_llvm_type(if_return_type)});

  auto ssa_temp = SymbolTable::next_ssa_temporary();
  starting_merge_block.add_instruction(
      LoadInstruction{.m_destination = ssa_temp,
                      .m_source = {result_handle},
                      .m_type = to_llvm_type(if_return_type)});
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
    throw std::runtime_error("UNIMPLEMENTED");
  }
}

bool is_signed_type(const hir::Type &type) {
  if (!std::holds_alternative<hir::PrimitiveTypeValue>(type)) {
    throw std::runtime_error("UNIMPLEMENTED");
  }
  return true;
}

LLVMIntegerCompareCondition to_llvm_compare_condition(BinaryOperator op,
                                                      const hir::Type &type) {
  switch (op) {
  case bust::BinaryOperator::EQ:
    return LLVMIntegerCompareCondition::EQ;
  case bust::BinaryOperator::NOT_EQ:
    return LLVMIntegerCompareCondition::NE;

  case bust::BinaryOperator::LT:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SLT;
    } else {
      return LLVMIntegerCompareCondition::ULT;
    }
  case bust::BinaryOperator::LT_EQ:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SLE;
    } else {
      return LLVMIntegerCompareCondition::ULE;
    }
  case bust::BinaryOperator::GT:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SGT;
    } else {
      return LLVMIntegerCompareCondition::UGT;
    }
  case bust::BinaryOperator::GT_EQ:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SGE;
    } else {
      return LLVMIntegerCompareCondition::UGE;
    }

  default:
    throw std::runtime_error("UNIMPLEMENTED");
  }
}

bool is_binary_compare(BinaryOperator op) {
  switch (op) {
  case bust::BinaryOperator::EQ:
  case bust::BinaryOperator::LT:
  case bust::BinaryOperator::LT_EQ:
  case bust::BinaryOperator::GT:
  case bust::BinaryOperator::GT_EQ:
  case bust::BinaryOperator::NOT_EQ:
    return true;
  default:
    return false;
  }
}

bool is_logical_op(BinaryOperator op) {
  switch (op) {
  case bust::BinaryOperator::LOGICAL_AND:
  case bust::BinaryOperator::LOGICAL_OR:
    return true;
  default:
    return false;
  }
}

Handle ExpressionGenerator::generate_integer_compare_instruction(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {

  auto compare_condition = to_llvm_compare_condition(
      binary_expression->m_operator, binary_expression->m_lhs.m_type);

  auto lhs_handle = (*this)(binary_expression->m_lhs);

  auto rhs_handle = (*this)(binary_expression->m_rhs);

  auto result_handle = SymbolTable::next_ssa_temporary();

  m_ctx.current_basic_block().add_instruction(IntegerCompareInstruction{
      .m_result = result_handle,
      .m_lhs = lhs_handle,
      .m_rhs = rhs_handle,
      .m_condition = compare_condition,
      .m_type = to_llvm_type(binary_expression->m_lhs.m_type)});

  return result_handle;
}

Handle ExpressionGenerator::generate_arithmetic_binary_instruction(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {

  auto lhs_handle = (*this)(binary_expression->m_lhs);

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

Handle ExpressionGenerator::generate_logical_binary_instruction(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {
  // Supporting short circuiting is similar to if expressions

  // lhs executes no matter what
  // We start executing lhs in the current block we're in
  // final_lhs_block will have a br based on the lhs_handle
  // For AND, branch to merge on negative, else to rhs
  // For OR, branch to merge on positive, else to rhs
  // rhs jumps to merge
  // at merge, we still need to do a compare on the result
  // AND: lhs is negative, branch to merge with stored false
  // AND: lhs is positive, branch to rhs, store rhs as result
  //  OR: lhs is positive, branch to merge with stored true
  //  OR: lhs is negative, branch to rhs, store rhs as result

  // Need to alloca result
  auto result_handle =
      m_ctx.m_symbol_table.define_local("short_circuit_logic_result");

  m_ctx.current_function().add_alloca_instruction(
      AllocaInstruction{.m_handle = result_handle, .m_type = LLVMType::I1});

  auto lhs_handle = (*this)(binary_expression->m_lhs);
  auto &final_lhs_block = m_ctx.current_basic_block();
  // Store lhs in result handle
  // Always store?
  final_lhs_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = lhs_handle,
      .m_type = LLVMType::I1,
  });

  auto &starting_rhs_block = m_ctx.current_function().new_basic_block("rhs");
  m_ctx.current_function().set_insertion_point(starting_rhs_block);
  auto rhs_handle = (*this)(binary_expression->m_rhs);
  auto &final_rhs_block = m_ctx.current_basic_block();

  final_rhs_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = rhs_handle,
      .m_type = LLVMType::I1,
  });

  auto &starting_merge_block =
      m_ctx.current_function().new_basic_block("merge");
  m_ctx.current_function().set_insertion_point(starting_merge_block);

  // Figure out terminal instructions now

  // LHS always branches
  switch (binary_expression->m_operator) {
  case bust::BinaryOperator::LOGICAL_AND:
    final_lhs_block.add_terminal(BranchInstruction{
        .m_condition = lhs_handle,
        .m_iftrue = starting_rhs_block.m_label,
        .m_iffalse = starting_merge_block.m_label,
    });
    break;
  case bust::BinaryOperator::LOGICAL_OR:
    final_lhs_block.add_terminal(BranchInstruction{
        .m_condition = lhs_handle,
        .m_iftrue = starting_merge_block.m_label,
        .m_iffalse = starting_rhs_block.m_label,
    });
    break;
  default:
    throw std::runtime_error("UNIMPLEMENTED");
  }

  final_rhs_block.add_terminal(JumpInstruction{
      .m_target = starting_merge_block.m_label,
  });

  auto final_result = SymbolTable::next_ssa_temporary();

  m_ctx.current_basic_block().add_instruction(LoadInstruction{
      .m_destination = final_result,
      .m_source = result_handle,
      .m_type = LLVMType::I1,
  });

  return final_result;
}

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {
  if (is_binary_compare(binary_expression->m_operator)) {
    return generate_integer_compare_instruction(binary_expression);
  }

  if (is_logical_op(binary_expression->m_operator)) {
    return generate_logical_binary_instruction(binary_expression);
  }

  return generate_arithmetic_binary_instruction(binary_expression);
}

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::UnaryExpr> &unary_expression) {

  auto input_handle = (*this)(unary_expression->m_expression);

  auto result_handle = SymbolTable::next_ssa_temporary();

  m_ctx.current_basic_block().add_instruction(UnaryInstruction{
      .m_result = result_handle,
      .m_input = input_handle,
      .m_operator = unary_expression->m_operator,
      .m_type = to_llvm_type(unary_expression->m_expression.m_type),
  });

  return result_handle;
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
