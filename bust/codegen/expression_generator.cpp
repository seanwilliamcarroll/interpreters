//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of expression generator.
//*
//*
//****************************************************************************

#include <algorithm>
#include <codegen/basic_block.hpp>
#include <codegen/context.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function.hpp>
#include <codegen/instructions.hpp>
#include <codegen/parameter.hpp>
#include <codegen/statement_generator.hpp>
#include <codegen/symbol_table.hpp>
#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <iterator>
#include <operators.hpp>
#include <optional>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <types.hpp>
#include <utility>
#include <variant>
#include <vector>

#include <codegen/handle.hpp>
#include <codegen/types.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Handle ExpressionGenerator::operator()(const hir::Identifier &identifier) {
  auto identifier_handle = m_ctx.symbols().lookup(identifier.m_name);

  // GlobalHandle (functions) and ParameterHandle (SSA args) are used directly.
  // Only LocalHandle (alloca slots) need a load.
  if (!std::holds_alternative<LocalHandle>(identifier_handle)) {
    return identifier_handle;
  }

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(
      LoadInstruction{.m_destination = ssa_temp,
                      .m_source = identifier_handle,
                      .m_type = to_llvm_type(identifier.m_type)});

  return ssa_temp;
}

Handle ExpressionGenerator::operator()(const hir::LiteralUnit &) { return {}; }

Handle ExpressionGenerator::operator()(const hir::LiteralI8 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const hir::LiteralI32 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const hir::LiteralI64 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const hir::LiteralBool &literal) {
  return LiteralHandle{literal.m_value ? "true" : "false"};
}

Handle ExpressionGenerator::operator()(const hir::LiteralChar &literal) {
  return LiteralHandle{std::to_string(static_cast<int8_t>(literal.m_value))};
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
  auto &function = m_ctx.function();

  const auto &if_return_type = if_expression->m_then_block.m_type;

  // Get handle to current after condition target generated, so even with nested
  // blocks, we're still starting right after the condition
  auto condition_target = (*this)(if_expression->m_condition);
  auto &final_condition_block = function.current_basic_block();

  // Create starting blocks that will always exist, then and merge
  auto &starting_then_block = function.new_basic_block("then");
  auto &starting_merge_block = function.new_basic_block("merge");

  // Do then branch, capture the final block for later if needed
  function.set_insertion_point(starting_then_block);
  auto then_target = (*this)(if_expression->m_then_block);
  auto &final_then_block = function.current_basic_block();
  final_then_block.add_terminal(
      JumpInstruction{.m_target = starting_merge_block.label()});

  if (!if_expression->m_else_block.has_value()) {
    // Just bare if, else is merge
    final_condition_block.add_terminal(
        BranchInstruction{.m_condition = condition_target,
                          .m_iftrue = starting_then_block.label(),
                          .m_iffalse = starting_merge_block.label()});
    // Subsequent instructions should go into merge block
    function.set_insertion_point(starting_merge_block);
    // Void, no handle to return
    return {};
  }

  // Now we handle the else, capturing first block and last block related to
  // this branch
  auto &starting_else_block = function.new_basic_block("else");
  function.set_insertion_point(starting_else_block);
  auto else_target = (*this)(if_expression->m_else_block.value());
  auto &final_else_block = function.current_basic_block();
  final_else_block.add_terminal(
      JumpInstruction{.m_target = starting_merge_block.label()});

  // Now we can merge on then vs else
  final_condition_block.add_terminal(
      BranchInstruction{.m_condition = condition_target,
                        .m_iftrue = starting_then_block.label(),
                        .m_iffalse = starting_else_block.label()});

  // Subsequent instructions should go into merge block
  function.set_insertion_point(starting_merge_block);

  // We only return a value when there is an else block
  // AND the types of the then and else expressions are NOT Unit
  const auto if_type_is_unit = hir::is_unit_type(if_return_type);
  auto if_statement_returns_value =
      if_expression->m_else_block.has_value() && !if_type_is_unit;
  if (!if_statement_returns_value) {
    // Void, no handle to return
    return {};
  }

  auto result_handle = m_ctx.symbols().define_local("if_result");

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

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();
  starting_merge_block.add_instruction(
      LoadInstruction{.m_destination = ssa_temp,
                      .m_source = {result_handle},
                      .m_type = to_llvm_type(if_return_type)});
  return ssa_temp;
}

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::CallExpr> &call_expression) {

  auto callee_handle = (*this)(call_expression->m_callee);

  std::vector<Argument> arguments;
  std::transform(call_expression->m_arguments.begin(),
                 call_expression->m_arguments.end(),
                 std::back_inserter(arguments),
                 [this](const auto &argument_expression) -> Argument {
                   auto handle = (*this)(argument_expression);
                   return {.m_name = handle,
                           .m_type = to_llvm_type(argument_expression.m_type)};
                 });

  auto function_return_type =
      hir::get_return_type(call_expression->m_callee.m_type);

  if (hir::is_unit_type(function_return_type)) {
    m_ctx.block().add_instruction(
        CallVoidInstruction{.m_callee = std::move(callee_handle),
                            .m_arguments = std::move(arguments)});
    return {};
  }

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();
  m_ctx.block().add_instruction(
      CallInstruction{.m_target = ssa_temp,
                      .m_callee = std::move(callee_handle),
                      .m_arguments = std::move(arguments),
                      .m_return_type = to_llvm_type(function_return_type)});

  return ssa_temp;
}

LLVMBinaryOperator to_llvm_op(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::PLUS:
    return LLVMBinaryOperator::ADD;
  case BinaryOperator::MINUS:
    return LLVMBinaryOperator::SUB;
  case BinaryOperator::MULTIPLIES:
    return LLVMBinaryOperator::MUL;
  case BinaryOperator::DIVIDES:
    return LLVMBinaryOperator::SDIV;
  case BinaryOperator::MODULUS:
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
  case BinaryOperator::EQ:
    return LLVMIntegerCompareCondition::EQ;
  case BinaryOperator::NOT_EQ:
    return LLVMIntegerCompareCondition::NE;

  case BinaryOperator::LT:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SLT;
    } else {
      return LLVMIntegerCompareCondition::ULT;
    }
  case BinaryOperator::LT_EQ:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SLE;
    } else {
      return LLVMIntegerCompareCondition::ULE;
    }
  case BinaryOperator::GT:
    if (is_signed_type(type)) {
      return LLVMIntegerCompareCondition::SGT;
    } else {
      return LLVMIntegerCompareCondition::UGT;
    }
  case BinaryOperator::GT_EQ:
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
  case BinaryOperator::EQ:
  case BinaryOperator::LT:
  case BinaryOperator::LT_EQ:
  case BinaryOperator::GT:
  case BinaryOperator::GT_EQ:
  case BinaryOperator::NOT_EQ:
    return true;
  default:
    return false;
  }
}

bool is_logical_op(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::LOGICAL_AND:
  case BinaryOperator::LOGICAL_OR:
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

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(IntegerCompareInstruction{
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

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(BinaryInstruction{
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
  // For AND, branch to merge on negative, else to rhs
  // For OR, branch to merge on positive, else to rhs
  // rhs jumps to merge
  // at merge, we still need to do a compare on the result

  auto result_handle =
      m_ctx.symbols().define_local("short_circuit_logic_result");

  m_ctx.function().add_alloca_instruction(
      AllocaInstruction{.m_handle = result_handle, .m_type = LLVMType::I1});

  auto lhs_handle = (*this)(binary_expression->m_lhs);
  auto &final_lhs_block = m_ctx.block();
  // Store lhs in result handle
  final_lhs_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = lhs_handle,
      .m_type = LLVMType::I1,
  });

  auto &starting_rhs_block = m_ctx.function().new_basic_block("rhs");
  m_ctx.function().set_insertion_point(starting_rhs_block);
  auto rhs_handle = (*this)(binary_expression->m_rhs);
  auto &final_rhs_block = m_ctx.block();

  final_rhs_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = rhs_handle,
      .m_type = LLVMType::I1,
  });

  auto &starting_merge_block = m_ctx.function().new_basic_block("merge");
  m_ctx.function().set_insertion_point(starting_merge_block);

  // Figure out terminal instructions now

  // LHS always branches
  const auto is_and_op =
      binary_expression->m_operator == BinaryOperator::LOGICAL_AND;
  final_lhs_block.add_terminal(BranchInstruction{
      .m_condition = lhs_handle,
      .m_iftrue =
          is_and_op ? starting_rhs_block.label() : starting_merge_block.label(),
      .m_iffalse =
          is_and_op ? starting_merge_block.label() : starting_rhs_block.label(),
  });

  final_rhs_block.add_terminal(JumpInstruction{
      .m_target = starting_merge_block.label(),
  });

  auto final_result = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(LoadInstruction{
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

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(UnaryInstruction{
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

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::CastExpr> &cast_expression) {

  // We've type checked, so we know it's a valid cast
  auto input_handle = (*this)(cast_expression->m_expression);

  const auto &from = to_llvm_type(cast_expression->m_expression.m_type);
  const auto &to = to_llvm_type(cast_expression->m_new_type);

  if (from == to) {
    // Nothing casted, noop
    return input_handle;
  }

  LLVMCastOperator cast_op;

  if (width_bits(from) > width_bits(to)) {
    // Truncation
    cast_op = LLVMCastOperator::TRUNC;
  } else if (from == LLVMType::I1) {
    // Unsigned extend
    cast_op = LLVMCastOperator::ZEXT;
  } else {
    // Signed extend
    cast_op = LLVMCastOperator::SEXT;
  }

  auto ssa_temporary = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(CastInstruction{
      .m_destination = ssa_temporary,
      .m_source = input_handle,
      .m_operator = cast_op,
      .m_from = from,
      .m_to = to,
  });

  return ssa_temporary;
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
