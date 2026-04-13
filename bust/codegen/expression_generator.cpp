//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of expression generator.
//*
//*
//****************************************************************************

#include "codegen/function_declaration.hpp"
#include <algorithm>
#include <codegen/basic_block.hpp>
#include <codegen/context.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function.hpp>
#include <codegen/instructions.hpp>
#include <codegen/parameter.hpp>
#include <codegen/statement_generator.hpp>
#include <codegen/symbol_table.hpp>
#include <exceptions.hpp>
#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <iterator>
#include <operators.hpp>
#include <optional>
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

Handle ExpressionGenerator::generate(const auto &to_generate) {
  return (*this)(to_generate);
}

Handle ExpressionGenerator::operator()(const hir::Identifier &identifier) {
  auto identifier_handle = m_ctx.symbols().lookup(identifier.m_name);

  // GlobalHandle (functions) and ParameterHandle (SSA args) are used directly.
  // Only LocalHandle (alloca slots) need a load.
  if (!std::holds_alternative<LocalHandle>(identifier_handle)) {
    return identifier_handle;
  }

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(LoadInstruction{
      .m_destination = ssa_temp,
      .m_source = identifier_handle,
      .m_type = to_llvm_type(m_ctx.type_registry().get(identifier.m_type))});

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
  return generate(*block);
}

Handle ExpressionGenerator::operator()(const hir::Block &block) {
  Handle last_value{};
  for (const auto &statement : block.m_statements) {
    last_value = std::visit(StatementGenerator{m_ctx}, statement);
  }

  if (block.m_final_expression.has_value()) {
    return generate(block.m_final_expression.value());
  }

  // Need to return a value here potentially
  return last_value;
}

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::IfExpr> &if_expression) {
  auto &function = m_ctx.function();

  const auto &if_return_type_id = if_expression->m_then_block.m_type;

  // Get handle to current after condition target generated, so even with nested
  // blocks, we're still starting right after the condition
  auto condition_target = generate(if_expression->m_condition);
  auto &final_condition_block = function.current_basic_block();

  // Create starting blocks that will always exist, then and merge
  auto &starting_then_block = function.new_basic_block("then");
  auto &starting_merge_block = function.new_basic_block("merge");

  // Do then branch, capture the final block for later if needed
  function.set_insertion_point(starting_then_block);
  auto then_target = generate(if_expression->m_then_block);
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
  auto else_target = generate(if_expression->m_else_block.value());
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
  const auto if_type_is_unit =
      if_return_type_id == m_ctx.type_registry().m_unit;
  auto if_statement_returns_value =
      if_expression->m_else_block.has_value() && !if_type_is_unit;
  if (!if_statement_returns_value) {
    // Void, no handle to return
    return {};
  }

  auto result_handle = m_ctx.symbols().define_local("if_result");

  function.add_alloca_instruction(AllocaInstruction{
      .m_handle = result_handle,
      .m_type = to_llvm_type(m_ctx.type_registry().get(if_return_type_id))});

  // Need to store the value generated by each branch for use in the merge block
  final_then_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = then_target,
      .m_type = to_llvm_type(m_ctx.type_registry().get(if_return_type_id))});

  final_else_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = else_target,
      .m_type = to_llvm_type(m_ctx.type_registry().get(if_return_type_id))});

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();
  starting_merge_block.add_instruction(LoadInstruction{
      .m_destination = ssa_temp,
      .m_source = {result_handle},
      .m_type = to_llvm_type(m_ctx.type_registry().get(if_return_type_id))});
  return ssa_temp;
}

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::CallExpr> &call_expression) {

  auto callee_handle = generate(call_expression->m_callee);

  std::vector<Argument> arguments;
  std::transform(call_expression->m_arguments.begin(),
                 call_expression->m_arguments.end(),
                 std::back_inserter(arguments),
                 [this](const auto &argument_expression) -> Argument {
                   auto handle = generate(argument_expression);
                   return {.m_name = handle,
                           .m_type = to_llvm_type(m_ctx.type_registry().get(
                               argument_expression.m_type))};
                 });

  auto function_return_type_id =
      std::get<hir::FunctionType>(
          m_ctx.type_registry().get(call_expression->m_callee.m_type))
          .m_return_type;

  if (function_return_type_id == m_ctx.type_registry().m_unit) {
    m_ctx.block().add_instruction(
        CallVoidInstruction{.m_callee = std::move(callee_handle),
                            .m_arguments = std::move(arguments)});
    return {};
  }

  auto ssa_temp = m_ctx.symbols().next_ssa_temporary();
  m_ctx.block().add_instruction(CallInstruction{
      .m_target = ssa_temp,
      .m_callee = std::move(callee_handle),
      .m_arguments = std::move(arguments),
      .m_return_type =
          to_llvm_type(m_ctx.type_registry().get(function_return_type_id))});

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
    throw core::InternalCompilerError(
        "unsupported binary operator in LLVM lowering");
  }
}

bool is_signed_type(const hir::TypeKind &type) {
  if (!std::holds_alternative<hir::PrimitiveTypeValue>(type)) {
    throw core::InternalCompilerError("signedness check on non-primitive type");
  }
  auto primitive_type = std::get<hir::PrimitiveTypeValue>(type).m_type;
  switch (primitive_type) {
  case PrimitiveType::I8:
  case PrimitiveType::I32:
  case PrimitiveType::I64:
    return true;
  case PrimitiveType::BOOL:
  case PrimitiveType::CHAR:
  case PrimitiveType::UNIT:
    throw core::InternalCompilerError(
        "Should never had checked signedness on non numeric type");
  }
}

LLVMIntegerCompareCondition
to_llvm_compare_condition(BinaryOperator op, const hir::TypeKind &type) {
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
    throw core::InternalCompilerError(
        "unsupported compare operator in LLVM lowering");
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
      binary_expression->m_operator,
      m_ctx.type_registry().get(binary_expression->m_lhs.m_type));

  auto lhs_handle = generate(binary_expression->m_lhs);

  auto rhs_handle = generate(binary_expression->m_rhs);

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(IntegerCompareInstruction{
      .m_result = result_handle,
      .m_lhs = lhs_handle,
      .m_rhs = rhs_handle,
      .m_condition = compare_condition,
      .m_type = to_llvm_type(
          m_ctx.type_registry().get(binary_expression->m_lhs.m_type))});

  return result_handle;
}

Handle ExpressionGenerator::generate_arithmetic_binary_instruction(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {

  auto lhs_handle = generate(binary_expression->m_lhs);

  auto rhs_handle = generate(binary_expression->m_rhs);

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(
      BinaryInstruction{.m_result = result_handle,
                        .m_lhs = lhs_handle,
                        .m_rhs = rhs_handle,
                        .m_operator = to_llvm_op(binary_expression->m_operator),
                        .m_type = to_llvm_type(m_ctx.type_registry().get(
                            binary_expression->m_lhs.m_type))});

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

  auto lhs_handle = generate(binary_expression->m_lhs);
  auto &final_lhs_block = m_ctx.block();
  // Store lhs in result handle
  final_lhs_block.add_instruction(StoreInstruction{
      .m_destination = result_handle,
      .m_source = lhs_handle,
      .m_type = LLVMType::I1,
  });

  auto &starting_rhs_block = m_ctx.function().new_basic_block("rhs");
  m_ctx.function().set_insertion_point(starting_rhs_block);
  auto rhs_handle = generate(binary_expression->m_rhs);
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

  auto input_handle = generate(unary_expression->m_expression);

  auto result_handle = m_ctx.symbols().next_ssa_temporary();

  m_ctx.block().add_instruction(UnaryInstruction{
      .m_result = result_handle,
      .m_input = input_handle,
      .m_operator = unary_expression->m_operator,
      .m_type = to_llvm_type(
          m_ctx.type_registry().get(unary_expression->m_expression.m_type)),
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
  auto input_handle = generate(cast_expression->m_expression);

  const auto &from = to_llvm_type(
      m_ctx.type_registry().get(cast_expression->m_expression.m_type));
  const auto &to =
      to_llvm_type(m_ctx.type_registry().get(cast_expression->m_new_type));

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

FunctionDeclaration ExpressionGenerator::generate_lambda_signature(
    const std::unique_ptr<hir::LambdaExpr> &lambda_expr) {
  std::vector<Parameter> parameters;
  std::transform(
      lambda_expr->m_parameters.begin(), lambda_expr->m_parameters.end(),
      std::back_inserter(parameters),
      [this](const hir::Identifier &parameter) -> Parameter {
        auto handle = m_ctx.symbols().define_parameter(parameter.m_name);
        return Parameter{.m_name = std::move(handle),
                         .m_type = to_llvm_type(
                             m_ctx.type_registry().get(parameter.m_type))};
      });

  auto lambda_handle = m_ctx.symbols().define_uniqued_global("lambda");

  return FunctionDeclaration{
      .m_function_id = lambda_handle,
      .m_return_type =
          to_llvm_type(m_ctx.type_registry().get(lambda_expr->m_return_type)),
      .m_parameters = std::move(parameters)};
}

struct FunctionScopeGuard {
  FunctionScopeGuard(Context &ctx)
      : m_ctx(ctx), m_original_function(ctx.module().current_function()) {}
  ~FunctionScopeGuard() {
    m_ctx.module().set_current_function(m_original_function);
  }

  Context &m_ctx;
  Function &m_original_function;
};

Handle ExpressionGenerator::operator()(
    const std::unique_ptr<hir::LambdaExpr> &lambda_expr) {
  ScopeGuard scope_guard(m_ctx.symbols());
  FunctionScopeGuard function_guard(m_ctx);
  // Need to capture any variables into an environment struct for this
  // particular lambda
  // Need to generate a top level function, uniquified name
  // Need to generate a func/env ptr pair to return, returning a handle to them

  // Start with pure lambda, no captures

  auto signature = generate_lambda_signature(lambda_expr);

  auto lambda_handle = signature.m_function_id;

  auto &lambda_function = m_ctx.module().new_function(std::move(signature));
  m_ctx.module().set_current_function(lambda_function);

  auto return_value = generate(lambda_expr->m_body);

  if (lambda_expr->m_return_type == m_ctx.type_registry().m_unit) {
    lambda_function.current_basic_block().add_terminal(ReturnVoidInstruction{});
  } else {
    lambda_function.current_basic_block().add_terminal(
        ReturnInstruction{.m_value = return_value,
                          .m_type = to_llvm_type(m_ctx.type_registry().get(
                              lambda_expr->m_return_type))});
  }

  return lambda_handle;
}

Handle ExpressionGenerator::operator()(const hir::Expression &expression) {
  return std::visit(*this, expression.m_expression);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
