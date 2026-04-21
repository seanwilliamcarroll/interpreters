//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of expression generator.
//*
//*
//****************************************************************************

#include <codegen/basic_block.hpp>
#include <codegen/context.hpp>
#include <codegen/expression_generator.hpp>
#include <codegen/function.hpp>
#include <codegen/function_declaration.hpp>
#include <codegen/handle.hpp>
#include <codegen/instructions.hpp>
#include <codegen/ir_literals.hpp>
#include <codegen/module.hpp>
#include <codegen/naming_conventions.hpp>
#include <codegen/parameter.hpp>
#include <codegen/statement_generator.hpp>
#include <codegen/symbol_table.hpp>
#include <codegen/types.hpp>
#include <exceptions.hpp>
#include <operators.hpp>
#include <zir/arena.hpp>
#include <zir/nodes.hpp>
#include <zir/types.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "codegen/closure_builder.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

Handle ExpressionGenerator::generate(const zir::ExprId &expr_id) {
  return generate(m_ctx.arena().get(expr_id));
}

Handle ExpressionGenerator::generate(const zir::ExprKind &expr_kind) {
  return std::visit(*this, expr_kind);
}

Handle ExpressionGenerator::generate(const zir::Expression &expression) {
  return generate(expression.m_expr_kind);
}

Handle ExpressionGenerator::operator()(const zir::IdentifierExpr &identifier) {
  auto binding = m_ctx.arena().get(identifier.m_id);
  auto identifier_handle = m_ctx.symbols().lookup(binding.m_name);

  // GlobalHandles (functions) are used directly.
  // Everything else is alloca'd and needs a load.
  if (std::holds_alternative<GlobalHandle>(identifier_handle)) {
    return identifier_handle;
  }

  return m_ctx.builder().create_load(identifier_handle,
                                     m_ctx.to_type(binding.m_type));
}

Handle ExpressionGenerator::operator()(const zir::Unit & /*unused*/) {
  return {};
}

Handle ExpressionGenerator::operator()(const zir::I8 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const zir::I32 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const zir::I64 &literal) {
  return LiteralHandle{std::to_string(literal.m_value)};
}

Handle ExpressionGenerator::operator()(const zir::Bool &literal) {
  return LiteralHandle{
      std::string{literal.m_value ? ir_literals::true_ : ir_literals::false_}};
}

Handle ExpressionGenerator::operator()(const zir::Char &literal) {
  return LiteralHandle{std::to_string(static_cast<int8_t>(literal.m_value))};
}

Handle ExpressionGenerator::operator()(const zir::Block &block) {
  for (const auto &statement : block.m_statements) {
    StatementGenerator{m_ctx}.generate(statement);
  }

  if (block.m_final_expression.has_value()) {
    return generate(block.m_final_expression.value());
  }

  return {};
}

zir::TypeId ExpressionGenerator::get_block_type(const zir::Block &block) const {
  if (block.m_final_expression.has_value()) {
    const auto &expression =
        m_ctx.arena().get(block.m_final_expression.value());
    return expression.m_type_id;
  }
  return m_ctx.arena().m_unit;
}

Handle ExpressionGenerator::operator()(const zir::IfExpr &if_expression) {
  const auto &if_return_type_id = get_block_type(if_expression.m_then_block);
  const auto &llvm_return_type_id = m_ctx.to_type(if_return_type_id);

  const auto has_else_branch = if_expression.m_else_block.has_value();
  const auto yields_value =
      has_else_branch && if_return_type_id != m_ctx.arena().m_unit;

  // Emit code for conditional
  auto condition_target = generate(if_expression.m_condition);
  auto result_storage =
      yields_value
          ? m_ctx.builder().add_alloca(
                std::string{conventions::if_result_local}, llvm_return_type_id)
          : Handle{};

  auto then_label =
      m_ctx.builder().make_block(std::string{conventions::then_block_label});
  auto else_label = has_else_branch ? m_ctx.builder().make_block(std::string{
                                          conventions::else_block_label})
                                    : BlockLabel::null();
  auto merge_label =
      m_ctx.builder().make_block(std::string{conventions::merge_block_label});

  m_ctx.builder().add_branch(condition_target, then_label,
                             has_else_branch ? else_label : merge_label);

  // Emit code for then
  m_ctx.builder().enter_block(then_label);
  auto then_target = generate(if_expression.m_then_block);
  if (yields_value) {
    m_ctx.builder().create_store(
        result_storage,
        Argument{.m_name = then_target, .m_type = llvm_return_type_id});
  }
  m_ctx.builder().add_jump(merge_label);

  if (has_else_branch) {
    // Emit code for else
    m_ctx.builder().enter_block(else_label);
    auto else_target = generate(if_expression.m_else_block.value());
    if (yields_value) {
      m_ctx.builder().create_store(
          result_storage,
          Argument{.m_name = else_target, .m_type = llvm_return_type_id});
    }
    m_ctx.builder().add_jump(merge_label);
  }

  // Finish with final load of merged value if it exists
  m_ctx.builder().enter_block(merge_label);
  return yields_value
             ? m_ctx.builder().create_load(result_storage, llvm_return_type_id)
             : Handle{};
}

Handle ExpressionGenerator::operator()(const zir::CallExpr &call_expression) {
  const auto &closture_type_id = m_ctx.m_fat_ptr;

  auto closure_handle = generate(call_expression.m_callee);
  auto callee_handle = m_ctx.builder().load_from_struct(
      closture_type_id, closure_handle, 0, m_ctx.m_ptr);
  auto env_handle = m_ctx.builder().load_from_struct(
      closture_type_id, closure_handle, 1, m_ctx.m_ptr);

  std::vector<Argument> arguments;
  arguments.emplace_back(Argument{.m_name = env_handle, .m_type = m_ctx.m_ptr});
  std::ranges::transform(
      call_expression.m_arguments, std::back_inserter(arguments),
      [this](const auto &argument_expr_id) -> Argument {
        auto expression = m_ctx.arena().get(argument_expr_id);
        auto handle = generate(expression);
        return {.m_name = handle,
                .m_type = m_ctx.to_type(expression.m_type_id)};
      });

  auto callee_expr = m_ctx.arena().get(call_expression.m_callee);

  auto function_return_type_id =
      m_ctx.arena().as_function(callee_expr.m_type_id).m_return_type;

  if (function_return_type_id == m_ctx.arena().m_unit) {
    m_ctx.builder().create_call_void(std::move(callee_handle),
                                     std::move(arguments));
    return {};
  }

  return m_ctx.builder().create_call(std::move(callee_handle),
                                     std::move(arguments),
                                     m_ctx.to_type(function_return_type_id));
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
                             std::is_same_v<T, zir::FunctionType>) {
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

Handle ExpressionGenerator::generate_integer_compare_instruction(
    const zir::BinaryExpr &binary_expression) {

  auto lhs = m_ctx.arena().get(binary_expression.m_lhs);

  auto compare_condition = to_llvm_compare_condition(
      binary_expression.m_operator, m_ctx.arena().get(lhs.m_type_id));

  auto lhs_handle = generate(binary_expression.m_lhs);

  auto rhs_handle = generate(binary_expression.m_rhs);

  return m_ctx.builder().create_icmp(std::move(lhs_handle),
                                     std::move(rhs_handle), compare_condition,
                                     m_ctx.to_type(lhs.m_type_id));
}

Handle ExpressionGenerator::generate_arithmetic_binary_instruction(
    const zir::BinaryExpr &binary_expression) {

  auto lhs = m_ctx.arena().get(binary_expression.m_lhs);

  auto lhs_handle = generate(binary_expression.m_lhs);

  auto rhs_handle = generate(binary_expression.m_rhs);

  return m_ctx.builder().create_binary(
      std::move(lhs_handle), std::move(rhs_handle),
      to_llvm_op(binary_expression.m_operator), m_ctx.to_type(lhs.m_type_id));
}

Handle ExpressionGenerator::generate_logical_binary_instruction(
    const zir::BinaryExpr &binary_expression) {
  // Supporting short circuiting is similar to if expressions
  // lhs executes no matter what
  // For AND, branch to merge on negative, else to rhs
  // For OR, branch to merge on positive, else to rhs
  // rhs jumps to merge
  // at merge, we still need to do a compare on the result
  auto result_storage = m_ctx.builder().add_alloca(
      std::string{conventions::short_circuit_result_local}, m_ctx.m_i1);

  auto rhs_label =
      m_ctx.builder().make_block(std::string{conventions::rhs_block_label});
  auto merge_label =
      m_ctx.builder().make_block(std::string{conventions::merge_block_label});

  // Emit code for lhs
  auto lhs_handle = generate(binary_expression.m_lhs);
  // Store lhs in result handle
  m_ctx.builder().create_store(result_storage,
                               {.m_name = lhs_handle, .m_type = m_ctx.m_i1});
  const auto is_and_op =
      binary_expression.m_operator == BinaryOperator::LOGICAL_AND;
  m_ctx.builder().add_branch(lhs_handle, is_and_op ? rhs_label : merge_label,
                             is_and_op ? merge_label : rhs_label);

  // Emit code for rhs
  m_ctx.builder().enter_block(rhs_label);
  auto rhs_handle = generate(binary_expression.m_rhs);
  m_ctx.builder().create_store(result_storage,
                               {.m_name = rhs_handle, .m_type = m_ctx.m_i1});
  m_ctx.builder().add_jump(merge_label);

  // Emit code for merge
  m_ctx.builder().enter_block(merge_label);

  return m_ctx.builder().create_load(result_storage, m_ctx.m_i1);
}

Handle
ExpressionGenerator::operator()(const zir::BinaryExpr &binary_expression) {
  if (is_binary_compare(binary_expression.m_operator)) {
    return generate_integer_compare_instruction(binary_expression);
  }

  if (is_logical_op(binary_expression.m_operator)) {
    return generate_logical_binary_instruction(binary_expression);
  }

  return generate_arithmetic_binary_instruction(binary_expression);
}

Handle ExpressionGenerator::operator()(const zir::UnaryExpr &unary_expression) {
  auto expression = m_ctx.arena().get(unary_expression.m_expression);

  auto input_handle = generate(unary_expression.m_expression);

  return m_ctx.builder().create_unary(std::move(input_handle),
                                      unary_expression.m_operator,
                                      m_ctx.to_type(expression.m_type_id));
}

Handle ExpressionGenerator::operator()(const zir::ReturnExpr & /*unused*/) {
  return {};
}

Handle ExpressionGenerator::operator()(const zir::CastExpr &cast_expression) {

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

  return m_ctx.builder().create_cast(std::move(input_handle), cast_op, from,
                                     to);
}

FunctionDeclaration ExpressionGenerator::generate_lambda_signature(
    const zir::LambdaExpr &lambda_expr) {
  std::vector<Parameter> parameters;
  parameters.emplace_back(m_ctx.env_parameter());
  std::ranges::transform(
      lambda_expr.m_parameters, std::back_inserter(parameters),
      [this](const zir::IdentifierExpr &parameter) -> Parameter {
        auto binding = m_ctx.arena().get(parameter.m_id);

        return Parameter{.m_name = binding.m_name,
                         .m_type = m_ctx.to_type(binding.m_type)};
      });

  auto lambda_handle = m_ctx.symbols().define_uniqued_global(
      std::string{conventions::lambda_global});

  return FunctionDeclaration{.m_function_id = lambda_handle,
                             .m_return_type =
                                 m_ctx.to_type(lambda_expr.m_return_type),
                             .m_parameters = std::move(parameters)};
}

GlobalHandle
ExpressionGenerator::lift_free_lambda(const zir::LambdaExpr &lambda_expr) {

  auto signature = generate_lambda_signature(lambda_expr);

  auto lambda_handle = signature.m_function_id;

  // Emit code for new lambda function
  {
    auto function_guard = m_ctx.builder().push_new_function(signature);

    m_ctx.builder().emit_parameter_prologue(signature.m_parameters);

    auto return_value = generate(lambda_expr.m_body);
    if (lambda_expr.m_return_type == m_ctx.arena().m_unit) {
      m_ctx.builder().create_return_void();
    } else {
      m_ctx.builder().create_return(return_value,
                                    m_ctx.to_type(lambda_expr.m_return_type));
    }
  }

  auto closure_name =
      GlobalHandle{conventions::make_closure_name(lambda_handle.m_handle)};
  m_ctx.module().add_constant_closure({
      .m_name = closure_name,
      .m_function = lambda_handle,
      .m_type_id = m_ctx.m_fat_ptr,
  });

  return closure_name;
}

Handle ExpressionGenerator::operator()(const zir::LambdaExpr &lambda_expr) {
  ScopeGuard scope_guard(m_ctx.symbols());

  if (lambda_expr.m_captures.empty()) {
    // Lift lambda to regular top level function
    return lift_free_lambda(lambda_expr);
  }

  auto signature = generate_lambda_signature(lambda_expr);

  auto lambda_handle = signature.m_function_id;

  auto closure_builder = ClosureBuilder(m_ctx, lambda_expr.m_captures);

  auto env_handle = closure_builder.allocate_and_populate_env();

  // Emit code for new lambda function
  {
    auto function_guard = m_ctx.builder().push_new_function(signature);

    m_ctx.builder().emit_parameter_prologue(signature.m_parameters);

    closure_builder.emit_capture_load_prologue();

    auto return_value = generate(lambda_expr.m_body);
    if (lambda_expr.m_return_type == m_ctx.arena().m_unit) {
      m_ctx.builder().create_return_void();
    } else {
      m_ctx.builder().create_return(return_value,
                                    m_ctx.to_type(lambda_expr.m_return_type));
    }
  }

  // Emit code to package lambda as a fat pointer
  return closure_builder.package_fat_pointer(lambda_handle, env_handle);
}

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
