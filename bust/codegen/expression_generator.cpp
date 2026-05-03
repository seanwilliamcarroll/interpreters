//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of expression generator.
//*
//*
//****************************************************************************

#include <codegen/arena.hpp>
#include <codegen/block_label.hpp>
#include <codegen/closure_builder.hpp>
#include <codegen/context.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/ir_builder.hpp>
#include <codegen/ir_literals.hpp>
#include <codegen/naming_conventions.hpp>
#include <codegen/parameter.hpp>
#include <codegen/statement_generator.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <codegen/value.hpp>
#include <exceptions.hpp>
#include <operators.hpp>
#include <types.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Value ExpressionGenerator::generate(const zir::ExprId &expr_id) {
  return generate(m_ctx.arena().get(expr_id));
}

Value ExpressionGenerator::generate(const zir::ExprKind &expr_kind) {
  return std::visit(*this, expr_kind);
}

Value ExpressionGenerator::generate(const zir::Expression &expression) {
  return generate(expression.m_expr_kind);
}

Value ExpressionGenerator::operator()(const zir::IdentifierExpr &identifier) {
  auto zir_binding = m_ctx.arena().get(identifier.m_id);

  auto binding = m_ctx.symbols().lookup(zir_binding.m_name);

  if (std::holds_alternative<FunctionBinding>(binding)) {
    return std::get<FunctionBinding>(binding).m_callee;
  }
  if (std::holds_alternative<ClosureBinding>(binding)) {
    return m_ctx.builder().emit_load(std::get<ClosureBinding>(binding).m_ptr,
                                     m_ctx.m_ptr);
  }
  if (!std::holds_alternative<AllocaBinding>(binding)) {
    throw core::InternalCompilerError(
        "Expected FunctionBinding or AllocaBinding!");
  }

  const auto &alloca_binding = std::get<AllocaBinding>(binding);

  return m_ctx.builder().emit_load(alloca_binding.m_ptr,
                                   alloca_binding.m_internal_type_id);
}

Value ExpressionGenerator::operator()(const zir::TupleExpr &tuple_expr) {

  // Need to alloca the tuple, and do stores to the different fields, then
  // return the ssa to a load from this alloca

  std::vector<TypeId> field_types;
  field_types.reserve(tuple_expr.m_fields.size());
  std::vector<Value> fields;
  fields.reserve(tuple_expr.m_fields.size());
  for (const auto &field : tuple_expr.m_fields) {
    auto field_value = generate(field);
    field_types.emplace_back(field_value.m_type_id);
    fields.emplace_back(std::move(field_value));
  }

  auto struct_type_id = m_ctx.type().intern(StructType{
      .m_fields = std::move(field_types),
      .m_name = {},
  });
  auto allocad_struct = m_ctx.builder().alloca_struct(struct_type_id);

  for (const auto &[index, field_value] :
       std::views::zip(std::views::iota(0ULL), fields)) {
    m_ctx.builder().store_to_struct(allocad_struct, struct_type_id, index,
                                    field_value);
  }

  return m_ctx.builder().emit_load(allocad_struct, struct_type_id);
}

Value ExpressionGenerator::operator()(const zir::Unit & /*unused*/) {
  return {};
}

Value ExpressionGenerator::operator()(const zir::I8 &literal) {
  return {
      .m_handle = LiteralHandle{std::to_string(literal.m_value)},
      .m_type_id = m_ctx.m_i8,
  };
}

Value ExpressionGenerator::operator()(const zir::I32 &literal) {
  return {
      .m_handle = LiteralHandle{std::to_string(literal.m_value)},
      .m_type_id = m_ctx.m_i32,
  };
}

Value ExpressionGenerator::operator()(const zir::I64 &literal) {
  return {
      .m_handle = LiteralHandle{std::to_string(literal.m_value)},
      .m_type_id = m_ctx.m_i64,
  };
}

Value ExpressionGenerator::operator()(const zir::Bool &literal) {
  return {
      .m_handle =
          LiteralHandle{
              .m_handle = std::string{literal.m_value ? ir_literals::true_
                                                      : ir_literals::false_},
          },
      .m_type_id = m_ctx.m_i1,
  };
}

Value ExpressionGenerator::operator()(const zir::Char &literal) {
  return {
      .m_handle =
          LiteralHandle{std::to_string(static_cast<int8_t>(literal.m_value))},
      .m_type_id = m_ctx.m_i8,
  };
}

Value ExpressionGenerator::operator()(const zir::Block &block) {
  for (const auto &statement : block.m_statements) {
    StatementGenerator{m_ctx}.generate(statement);
  }

  if (block.m_final_expression.has_value()) {
    return generate(block.m_final_expression.value());
  }

  return {};
}

Value ExpressionGenerator::operator()(const zir::IfExpr &if_expression) {
  const auto &if_return_type_id =
      m_ctx.arena().get_block_type(if_expression.m_then_block);
  const auto &llvm_return_type_id = m_ctx.to_type(if_return_type_id);

  const auto has_else_branch = if_expression.m_else_block.has_value();
  const auto yields_value =
      has_else_branch && if_return_type_id != m_ctx.arena().m_unit;

  // Emit code for conditional
  auto condition_target = generate(if_expression.m_condition);
  auto result_alloca_slot =
      yields_value
          ? m_ctx.builder().emit_alloca(
                llvm_return_type_id,
                m_ctx.uniqify_name(std::string{conventions::if_result_local}))
          : Value{};

  auto then_label =
      m_ctx.builder().make_block(std::string{conventions::then_block_label});
  auto else_label = has_else_branch ? m_ctx.builder().make_block(std::string{
                                          conventions::else_block_label})
                                    : BlockLabel::null();
  auto merge_label =
      m_ctx.builder().make_block(std::string{conventions::merge_block_label});

  m_ctx.builder().emit_branch(condition_target, then_label,
                              has_else_branch ? else_label : merge_label);

  // Emit code for then
  m_ctx.builder().enter_block(then_label);
  auto then_target = generate(if_expression.m_then_block);
  if (yields_value) {
    m_ctx.builder().emit_store(result_alloca_slot, then_target);
  }
  m_ctx.builder().emit_jump(merge_label);

  if (has_else_branch) {
    // Emit code for else
    m_ctx.builder().enter_block(else_label);
    auto else_target = generate(if_expression.m_else_block.value());
    if (yields_value) {
      m_ctx.builder().emit_store(result_alloca_slot, else_target);
    }
    m_ctx.builder().emit_jump(merge_label);
  }

  // Finish with final load of merged value if it exists
  m_ctx.builder().enter_block(merge_label);
  return yields_value ? m_ctx.builder().emit_load(result_alloca_slot,
                                                  llvm_return_type_id)
                      : Value{};
}

Value ExpressionGenerator::call_lambda_expression(
    const zir::CallExpr &call_expression) {
  const auto &closture_type_id = m_ctx.m_fat_ptr;

  auto closure = generate(call_expression.m_callee);

  if (closure.m_type_id != m_ctx.m_ptr) {
    throw core::InternalCompilerError("Expected closure to have type ptr!");
  }

  // We assume the closure has been loaded to an actual value here, not a
  // pointer?
  auto callee = m_ctx.builder().load_from_struct(closure, closture_type_id, 0);
  auto env = m_ctx.builder().load_from_struct(closure, closture_type_id, 1);

  std::vector<Value> arguments;
  arguments.emplace_back(std::move(env));
  for (const auto &argument : call_expression.m_arguments) {
    arguments.emplace_back(generate(argument));
  }

  auto callee_expr = m_ctx.arena().get(call_expression.m_callee);

  auto function_return_type_id =
      m_ctx.arena().as_function(callee_expr.m_type_id).m_return_type;

  if (function_return_type_id == m_ctx.arena().m_unit) {
    m_ctx.builder().emit_call_void(std::move(callee), std::move(arguments));
    return {};
  }

  return m_ctx.builder().emit_call(std::move(callee), std::move(arguments),
                                   m_ctx.to_type(function_return_type_id));
}

Value ExpressionGenerator::operator()(const zir::CallExpr &call_expression) {
  // Need to check if we are calling a pointer or a closure

  const auto &callee_expression = m_ctx.arena().get(call_expression.m_callee);
  if (std::holds_alternative<zir::IdentifierExpr>(
          callee_expression.m_expr_kind)) {
    // Could be a top level function or a pointer to a fat ptr
    const auto &identifier =
        std::get<zir::IdentifierExpr>(callee_expression.m_expr_kind);
    auto zir_binding = m_ctx.arena().get(identifier.m_id);
    auto binding = m_ctx.symbols().lookup(zir_binding.m_name);

    if (std::holds_alternative<ClosureBinding>(binding)) {
      // We just got a fat pointer loaded at this value, need to load it and go
      // from there
      return call_lambda_expression(call_expression);
    }
  }

  // Easy, regular function call
  auto callee = generate(call_expression.m_callee);
  const auto &function_type =
      m_ctx.arena().as_function(callee_expression.m_type_id);

  std::vector<Value> arguments;
  std::ranges::transform(
      call_expression.m_arguments, std::back_inserter(arguments),
      [this](const auto &argument) -> Value { return generate(argument); });

  if (function_type.m_return_type == m_ctx.arena().m_unit) {
    m_ctx.builder().emit_call_void(std::move(callee), std::move(arguments));
    return {};
  }
  auto return_type_id = m_ctx.to_type(function_type.m_return_type);

  return m_ctx.builder().emit_call(std::move(callee), std::move(arguments),
                                   return_type_id);
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

bool is_signed_type(const zir::Type &type) {
  return std::visit(
      [](const auto &t) -> bool {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, zir::I8Type> ||
                      std::is_same_v<T, zir::I32Type> ||
                      std::is_same_v<T, zir::I64Type>) {
          return true;
        } else if constexpr (std::is_same_v<T, zir::UnitType> ||
                             std::is_same_v<T, zir::BoolType> ||
                             std::is_same_v<T, zir::CharType> ||
                             std::is_same_v<T, zir::NeverType> ||
                             std::is_same_v<T, zir::FunctionType> ||
                             std::is_same_v<T, zir::TupleType>) {
          throw core::InternalCompilerError(
              "Should never had checked signedness on non numeric type");
        }
      },
      type);
}

LLVMIntegerCompareCondition to_llvm_compare_condition(BinaryOperator op,
                                                      const zir::Type &type) {
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

Value ExpressionGenerator::generate_integer_compare_instruction(
    const zir::BinaryExpr &binary_expression) {

  auto lhs = m_ctx.arena().get(binary_expression.m_lhs);

  auto compare_condition = to_llvm_compare_condition(
      binary_expression.m_operator, m_ctx.arena().get(lhs.m_type_id));

  auto lhs_handle = generate(binary_expression.m_lhs);

  auto rhs_handle = generate(binary_expression.m_rhs);

  return m_ctx.builder().emit_icmp(std::move(lhs_handle), std::move(rhs_handle),
                                   compare_condition);
}

Value ExpressionGenerator::generate_arithmetic_binary_instruction(
    const zir::BinaryExpr &binary_expression) {

  auto lhs = m_ctx.arena().get(binary_expression.m_lhs);

  auto lhs_handle = generate(binary_expression.m_lhs);

  auto rhs_handle = generate(binary_expression.m_rhs);

  return m_ctx.builder().emit_binary(std::move(lhs_handle),
                                     std::move(rhs_handle),
                                     to_llvm_op(binary_expression.m_operator));
}

Value ExpressionGenerator::generate_logical_binary_instruction(
    const zir::BinaryExpr &binary_expression) {
  // Supporting short circuiting is similar to if expressions
  // lhs executes no matter what
  // For AND, branch to merge on negative, else to rhs
  // For OR, branch to merge on positive, else to rhs
  // rhs jumps to merge
  // at merge, we still need to do a compare on the result
  auto result_alloca_slot = m_ctx.builder().emit_alloca(
      m_ctx.m_i1,
      m_ctx.uniqify_name(std::string{conventions::short_circuit_result_local}));

  auto rhs_label =
      m_ctx.builder().make_block(std::string{conventions::rhs_block_label});
  auto merge_label =
      m_ctx.builder().make_block(std::string{conventions::merge_block_label});

  // Emit code for lhs
  auto lhs_handle = generate(binary_expression.m_lhs);
  // Store lhs in result handle
  m_ctx.builder().emit_store(result_alloca_slot, lhs_handle);
  const auto is_and_op =
      binary_expression.m_operator == BinaryOperator::LOGICAL_AND;
  m_ctx.builder().emit_branch(lhs_handle, is_and_op ? rhs_label : merge_label,
                              is_and_op ? merge_label : rhs_label);

  // Emit code for rhs
  m_ctx.builder().enter_block(rhs_label);
  auto rhs_handle = generate(binary_expression.m_rhs);
  m_ctx.builder().emit_store(result_alloca_slot, rhs_handle);
  m_ctx.builder().emit_jump(merge_label);

  // Emit code for merge
  m_ctx.builder().enter_block(merge_label);

  return m_ctx.builder().emit_load(result_alloca_slot, m_ctx.m_i1);
}

Value ExpressionGenerator::operator()(
    const zir::BinaryExpr &binary_expression) {
  if (is_binary_compare(binary_expression.m_operator)) {
    return generate_integer_compare_instruction(binary_expression);
  }

  if (is_logical_op(binary_expression.m_operator)) {
    return generate_logical_binary_instruction(binary_expression);
  }

  return generate_arithmetic_binary_instruction(binary_expression);
}

Value ExpressionGenerator::operator()(const zir::UnaryExpr &unary_expression) {
  auto expression = m_ctx.arena().get(unary_expression.m_expression);

  auto input_handle = generate(unary_expression.m_expression);

  return m_ctx.builder().emit_unary(std::move(input_handle),
                                    unary_expression.m_operator);
}

Value ExpressionGenerator::operator()(const zir::ReturnExpr & /*unused*/) {
  return {};
}

Value ExpressionGenerator::operator()(const zir::CastExpr &cast_expression) {

  auto expression = m_ctx.arena().get(cast_expression.m_expression);

  // We've type checked, so we know it's a valid cast
  auto input_handle = generate(cast_expression.m_expression);

  const auto &from = m_ctx.to_type(expression.m_type_id);
  const auto &to = m_ctx.to_type(cast_expression.m_new_type);

  if (from == to) {
    // Nothing casted, noop
    return input_handle;
  }

  LLVMCastOperator cast_op;

  if (width_bits(m_ctx.type().get(from)) > width_bits(m_ctx.type().get(to))) {
    // Truncation
    cast_op = LLVMCastOperator::TRUNC;
  } else if (from == m_ctx.m_i1) {
    // Unsigned extend
    cast_op = LLVMCastOperator::ZEXT;
  } else {
    // Signed extend
    cast_op = LLVMCastOperator::SEXT;
  }

  return m_ctx.builder().emit_cast(std::move(input_handle), cast_op, to);
}

FunctionDeclaration ExpressionGenerator::generate_lambda_signature(
    const zir::LambdaExpr &lambda_expr, bool has_env) {
  std::vector<Parameter> parameters;
  std::vector<TypeId> parameter_types;
  if (has_env) {
    parameters.reserve(lambda_expr.m_parameters.size() + 1);
    parameter_types.reserve(lambda_expr.m_parameters.size() + 1);
    parameters.emplace_back(Parameter{
        .m_name = std::string{conventions::env_parameter_name},
        .m_type = m_ctx.m_ptr,
    });
    parameter_types.emplace_back(m_ctx.m_ptr);
  } else {
    parameters.reserve(lambda_expr.m_parameters.size());
    parameter_types.reserve(lambda_expr.m_parameters.size());
  }

  for (const auto &parameter : lambda_expr.m_parameters) {
    auto binding = m_ctx.arena().get(parameter.m_id);
    auto parameter_type_id = m_ctx.to_type(binding.m_type);
    parameters.emplace_back(Parameter{
        .m_name = binding.m_name,
        .m_type = parameter_type_id,
    });
    parameter_types.emplace_back(parameter_type_id);
  }

  auto unique_lambda_symbol =
      m_ctx.uniqify_name(std::string{conventions::lambda_global});

  auto lambda = Value{
      .m_handle =
          GlobalHandle{
              .m_handle = unique_lambda_symbol,
          },
      .m_type_id = m_ctx.m_ptr,
  };

  auto return_type_id = m_ctx.to_type(lambda_expr.m_return_type);

  auto function_binding = FunctionBinding{
      .m_callee = lambda,
      .m_return_type = return_type_id,
      .m_parameter_types = std::move(parameter_types),
  };

  m_ctx.symbols().bind_global(unique_lambda_symbol,
                              std::move(function_binding));

  return FunctionDeclaration{
      .m_function_id = std::move(lambda),
      .m_return_type = return_type_id,
      .m_parameters = std::move(parameters),
  };
}

Value ExpressionGenerator::lift_free_lambda(
    const zir::LambdaExpr &lambda_expr) {
  auto signature = generate_lambda_signature(lambda_expr, false);

  auto lambda = signature.m_function_id;

  // Emit code for new lambda function
  {
    auto function_guard = m_ctx.builder().push_new_function(signature);

    m_ctx.emit_parameter_prologue(signature.m_parameters);

    auto return_value = generate(lambda_expr.m_body);
    if (lambda_expr.m_return_type == m_ctx.arena().m_unit) {
      m_ctx.builder().emit_return_void();
    } else {
      m_ctx.builder().emit_return(return_value);
    }
  }

  auto function_ptr = Value{
      .m_handle = lambda.m_handle,
      .m_type_id = m_ctx.m_ptr,
  };

  return function_ptr;
}

Value ExpressionGenerator::operator()(const zir::LambdaExpr &lambda_expr) {
  ScopeGuard scope_guard(m_ctx.symbols());

  if (lambda_expr.m_captures.empty()) {
    // Lift lambda to regular top level function
    return lift_free_lambda(lambda_expr);
  }

  auto closure_builder = ClosureBuilder(m_ctx, lambda_expr.m_captures);

  auto env = closure_builder.allocate_and_populate_env();

  auto signature = generate_lambda_signature(lambda_expr, true);

  auto lambda = signature.m_function_id;

  // Emit code for new lambda function
  {
    auto function_guard = m_ctx.builder().push_new_function(signature);

    m_ctx.emit_parameter_prologue(signature.m_parameters);

    closure_builder.emit_capture_load_prologue();

    auto return_value = generate(lambda_expr.m_body);
    if (lambda_expr.m_return_type == m_ctx.arena().m_unit) {
      m_ctx.builder().emit_return_void();
    } else {
      m_ctx.builder().emit_return(return_value);
    }
  }

  // Emit code to package lambda as a fat pointer
  return closure_builder.package_fat_pointer(lambda, env);
}

Value ExpressionGenerator::operator()(const zir::DotExpr &dot_expr) {

  // For now, assume that the expression is a tuple
  auto tuple_value = generate(dot_expr.m_expression);

  auto tuple_type_id = tuple_value.m_type_id;

  if (m_ctx.type().is<StructType>(tuple_type_id)) {
    return m_ctx.builder().emit_extractvalue(tuple_value, tuple_type_id,
                                             dot_expr.m_tuple_index);
  }

  return m_ctx.builder().load_from_struct(tuple_value, tuple_type_id,
                                          dot_expr.m_tuple_index);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
