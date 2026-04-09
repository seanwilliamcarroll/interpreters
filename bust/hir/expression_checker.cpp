//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Expression type checker implementation.
//*
//*
//****************************************************************************

#include "hir/expression_checker.hpp"

#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "ast/types.hpp"
#include "exceptions.hpp"
#include "hir/block_checker.hpp"
#include "hir/context.hpp"
#include "hir/environment.hpp"
#include "hir/nodes.hpp"
#include "hir/type_converter.hpp"
#include "hir/type_unifier.hpp"
#include "operators.hpp"
#include "source_location.hpp"
#include "types.hpp"

//****************************************************************************
namespace bust::hir {
//****************************************************************************

bool is_comparison_op(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::LT:
  case BinaryOperator::LT_EQ:
  case BinaryOperator::GT:
  case BinaryOperator::GT_EQ:
  case BinaryOperator::EQ:
  case BinaryOperator::NOT_EQ:
    return true;
  default:
    return false;
  }
}

PrimitiveType required_type_for_unary_op(UnaryOperator op) {
  switch (op) {
  case UnaryOperator::MINUS:
    return PrimitiveType::I64;
  case UnaryOperator::NOT:
    return PrimitiveType::BOOL;
  }
}

std::optional<PrimitiveType> required_type_for_binary_op(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::LOGICAL_AND:
  case BinaryOperator::LOGICAL_OR:
    return PrimitiveType::BOOL;
  case BinaryOperator::PLUS:
  case BinaryOperator::MINUS:
  case BinaryOperator::MULTIPLIES:
  case BinaryOperator::DIVIDES:
  case BinaryOperator::MODULUS:
  case BinaryOperator::LT:
  case BinaryOperator::LT_EQ:
  case BinaryOperator::GT:
  case BinaryOperator::GT_EQ:
    return PrimitiveType::I64;
  case BinaryOperator::EQ:
  case BinaryOperator::NOT_EQ:
    return std::nullopt;
  }
}

bool is_type_allowed(PrimitiveType required, const hir::Type &type) {
  if (std::holds_alternative<hir::NeverType>(type)) {
    return true;
  }
  return std::holds_alternative<hir::PrimitiveTypeValue>(type) &&
         std::get<hir::PrimitiveTypeValue>(type).m_type == required;
}

Expression ExpressionChecker::operator()(const ast::Identifier &identifier) {
  auto maybe_type = m_ctx.m_env.lookup(identifier.m_name);
  if (!maybe_type.has_value()) {
    throw core::CompilerException(
        "TypeChecker",
        "Did not find identifier: " + identifier.m_name +
            " in type environment!",
        identifier.m_location);
  }

  const auto &type_scheme = maybe_type.value();

  auto final_type = m_ctx.create_fresh_type_vars(type_scheme);

  return {{identifier.m_location},
          clone_type(final_type),
          Identifier{{identifier.m_location},
                     identifier.m_name,
                     std::move(final_type)}};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::CallExpr> &call_expression) {
  auto callee = std::visit((*this), call_expression->m_callee);

  if (!std::holds_alternative<std::unique_ptr<FunctionType>>(callee.m_type)) {
    throw core::CompilerException("TypeChecker",
                                  "Expression of type: " + callee.m_type +
                                      " is not callable!",
                                  callee.m_location);
  }
  const auto &function_type =
      std::get<std::unique_ptr<FunctionType>>(callee.m_type);

  std::vector<Expression> arguments;
  arguments.reserve(call_expression->m_arguments.size());
  for (const auto &argument : call_expression->m_arguments) {
    arguments.emplace_back(std::visit((*this), argument));
  }

  // Check types of arguments against their parameters, then bind and type
  // check the body

  if (function_type->m_argument_types.size() != arguments.size()) {
    throw core::CompilerException(
        "TypeChecker",
        "Callee expects " +
            std::to_string(function_type->m_argument_types.size()) +
            " arguments, " + std::to_string(arguments.size()) + " provided",
        call_expression->m_location);
  }

  for (const auto &[index, parameter_type, argument] :
       std::views::zip(std::views::iota(0ULL), function_type->m_argument_types,
                       arguments)) {
    try {
      m_ctx.m_type_unifier.unify(parameter_type, argument.m_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException(
          "TypeChecker",
          "Type unification error!\n Parameter at index: " +
              std::to_string(index) + " expects type: " + parameter_type +
              " Unification error: " + error.what(),
          argument.m_location);
    }
  }
  // They all match

  auto return_type = m_ctx.m_type_unifier.find(function_type->m_return_type);

  return {{call_expression->m_location},
          std::move(return_type),
          std::make_unique<CallExpr>(CallExpr{{call_expression->m_location},
                                              std::move(callee),
                                              std::move(arguments)})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::BinaryExpr> &binary_expression) {
  // Both arguments must match
  auto lhs = std::visit((*this), binary_expression->m_lhs);
  auto rhs = std::visit((*this), binary_expression->m_rhs);

  try {
    m_ctx.m_type_unifier.unify(lhs.m_type, rhs.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error: ") + error.what(),
        binary_expression->m_location);
  }

  auto type = m_ctx.m_type_unifier.find(lhs.m_type);

  // Apply operator constraint
  auto required = required_type_for_binary_op(binary_expression->m_operator);
  if (required.has_value()) {
    Type required_type = PrimitiveTypeValue{{}, required.value()};
    try {
      if (std::holds_alternative<TypeVariable>(type)) {
        m_ctx.m_type_unifier.unify(std::get<TypeVariable>(type), required_type);
      } else if (!is_type_allowed(required.value(), type)) {
        throw core::CompilerException(
            "TypeChecker",
            "Type " + type + " is disallowed by binary operator: " +
                binary_expression->m_operator,
            binary_expression->m_location);
      }
    } catch (std::runtime_error &error) {
      throw core::CompilerException(
          "TypeChecker", std::string("Type unification error: ") + error.what(),
          binary_expression->m_location);
    }
  }

  auto final_type = is_comparison_op(binary_expression->m_operator)
                        ? PrimitiveTypeValue{{binary_expression->m_location},
                                             PrimitiveType::BOOL}
                        : m_ctx.m_type_unifier.find(type);

  return {
      {binary_expression->m_location},
      std::move(final_type),
      std::make_unique<BinaryExpr>(BinaryExpr{{binary_expression->m_location},
                                              binary_expression->m_operator,
                                              std::move(lhs),
                                              std::move(rhs)})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::UnaryExpr> &unary_expression) {
  auto expression = std::visit((*this), unary_expression->m_expression);

  auto required = required_type_for_unary_op(unary_expression->m_operator);
  Type required_type = PrimitiveTypeValue{{}, required};
  try {
    if (std::holds_alternative<TypeVariable>(expression.m_type)) {
      m_ctx.m_type_unifier.unify(std::get<TypeVariable>(expression.m_type),
                                 required_type);
    } else if (!is_type_allowed(required, expression.m_type)) {
      throw core::CompilerException(
          "TypeChecker",
          "Type Mismatch! UnaryExpr: " + unary_expression->m_operator +
              " does not accept type: " + expression.m_type,
          unary_expression->m_location);
    }
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error: ") + error.what(),
        unary_expression->m_location);
  }

  auto final_type = m_ctx.m_type_unifier.find(expression.m_type);

  auto final_expression = Expression{{expression.m_location},
                                     std::move(final_type),
                                     std::move(expression.m_expression)};

  return {{unary_expression->m_location},
          clone_type(final_expression.m_type),
          std::make_unique<UnaryExpr>(UnaryExpr{{unary_expression->m_location},
                                                unary_expression->m_operator,
                                                std::move(final_expression)})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::IfExpr> &if_expression) {

  auto condition = std::visit((*this), if_expression->m_condition);

  try {
    m_ctx.m_type_unifier.unify(condition.m_type,
                               PrimitiveTypeValue{{}, PrimitiveType::BOOL});
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        if_expression->m_location);
  }

  auto resolved_condition =
      Expression{{condition.m_location},
                 m_ctx.m_type_unifier.find(condition.m_type),
                 std::move(condition.m_expression)};

  // Directly call because Block is not a variant type, don't need to visit
  auto then_branch =
      BlockChecker{m_ctx}.check_block(if_expression->m_then_block);

  if (!if_expression->m_else_block.has_value()) {
    // auto type = m_ctx.m_type_unifier.find(then_branch.m_type);

    // Just then branch
    auto type =
        PrimitiveTypeValue{{if_expression->m_location}, PrimitiveType::UNIT};

    try {
      m_ctx.m_type_unifier.unify(type, then_branch.m_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException("TypeChecker",
                                    std::string("Type unification error!: ") +
                                        error.what(),
                                    if_expression->m_location);
    }

    return {{if_expression->m_location},
            type,
            std::make_unique<IfExpr>(IfExpr{{if_expression->m_location},
                                            std::move(resolved_condition),
                                            std::move(then_branch),
                                            {}})};
  }
  // Check else branch too
  auto else_branch =
      BlockChecker{m_ctx}.check_block(if_expression->m_else_block.value());

  try {
    m_ctx.m_type_unifier.unify(then_branch.m_type, else_branch.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        if_expression->m_location);
  }

  auto type = std::holds_alternative<NeverType>(then_branch.m_type)
                  ? m_ctx.m_type_unifier.find(else_branch.m_type)
                  : m_ctx.m_type_unifier.find(then_branch.m_type);

  return {{if_expression->m_location},
          std::move(type),
          std::make_unique<IfExpr>(
              IfExpr{{if_expression->m_location},
                     std::move(resolved_condition),
                     std::move(then_branch),
                     std::optional<Block>(std::move(else_branch))})};
}

Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::Block> &block) {
  auto hir_block = BlockChecker{m_ctx}.check_block(*block);
  return {{block->m_location},
          clone_type(hir_block.m_type),
          std::make_unique<Block>(std::move(hir_block))};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::CastExpr> &cast_expression) {

  auto hir_expression = std::visit(*this, cast_expression->m_expression);

  auto original_type = m_ctx.m_type_unifier.find(hir_expression.m_type);

  auto new_type = std::visit(TypeConverter{m_ctx}, cast_expression->m_type);

  if (!std::holds_alternative<PrimitiveTypeValue>(original_type) ||
      !std::holds_alternative<PrimitiveTypeValue>(new_type)) {
    throw core::CompilerException("TypeChecker",
                                  "Cannot cast an expression with type: " +
                                      original_type + " to type: " + new_type,
                                  cast_expression->m_location);
  }

  auto original_primitive = std::get<PrimitiveTypeValue>(original_type).m_type;
  auto new_primitive = std::get<PrimitiveTypeValue>(new_type).m_type;

  if (!can_cast(original_primitive, new_primitive)) {
    throw core::CompilerException("TypeChecker",
                                  "Cannot cast an expression with type: " +
                                      to_string(original_primitive) +
                                      " to type: " + to_string(new_primitive),
                                  cast_expression->m_location);
  }

  return {{cast_expression->m_location},
          std::move(new_type),
          std::make_unique<CastExpr>(CastExpr{{cast_expression->m_location},
                                              std::move(hir_expression),
                                              std::move(new_type)})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::ReturnExpr> &return_expression) {
  auto expression = std::visit((*this), return_expression->m_return_expression);

  if (m_ctx.m_return_type_stack.empty()) {
    // Should never happen?
    throw core::CompilerException("TypeChecker", "Shouldn't happen, right?",
                                  return_expression->m_location);
  }

  try {
    m_ctx.m_type_unifier.unify(expression.m_type,
                               m_ctx.m_return_type_stack.back());
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        return_expression->m_location);
  }

  auto expression_type = m_ctx.m_type_unifier.find(expression.m_type);

  auto final_expression = (expression_type == expression.m_type)
                              ? std::move(expression)
                              : Expression{{expression.m_location},
                                           std::move(expression_type),
                                           std::move(expression.m_expression)};

  return {{return_expression->m_location},
          NeverType{{return_expression->m_location}},
          std::make_unique<ReturnExpr>(ReturnExpr{
              {return_expression->m_location}, std::move(final_expression)})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::LambdaExpr> &lambda_expression) {
  // Want to evaluate body and see what the type is
  auto [parameters, parameter_types] =
      TypeConverter{m_ctx}.convert_parameters(lambda_expression->m_parameters);

  auto possible_return_type =
      lambda_expression->m_return_type.has_value()
          ? TypeConverter{m_ctx}.get_type(
                lambda_expression->m_return_type.value())
          : m_ctx.m_type_unifier.new_type_var();

  auto body =
      lambda_expression->m_return_type.has_value()
          ? BlockChecker{m_ctx}.check_callable_body(
                parameters, possible_return_type, lambda_expression->m_body)
          : BlockChecker{m_ctx}.check_block_with_parameters(
                parameters, lambda_expression->m_body);

  auto return_type = lambda_expression->m_return_type.has_value()
                         ? std::move(possible_return_type)
                         : clone_type(body.m_type);

  try {
    m_ctx.m_type_unifier.unify(return_type, body.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        lambda_expression->m_location);
  }

  // Do we need to rediscover the return type and body types?

  // Expect body to have a unified type?
  std::vector<Identifier> unified_parameters;
  unified_parameters.reserve(parameters.size());
  std::vector<Type> unified_parameter_types;
  unified_parameter_types.reserve(parameter_types.size());
  for (const auto &[parameter, parameter_type] :
       std::views::zip(parameters, parameter_types)) {
    if (!std::holds_alternative<TypeVariable>(parameter_type)) {
      // TODO Could be more efficient than reconstructing
      unified_parameters.emplace_back(Identifier{{parameter.m_location},
                                                 parameter.m_name,
                                                 clone_type(parameter.m_type)});
      unified_parameter_types.push_back(clone_type(parameter_type));
      continue;
    }
    // See if we can resolve them
    auto unified_type =
        m_ctx.m_type_unifier.find(std::get<TypeVariable>(parameter_type));
    unified_parameters.emplace_back(Identifier{
        {parameter.m_location}, parameter.m_name, clone_type(unified_type)});
    unified_parameter_types.push_back(std::move(unified_type));
  }

  auto function_type = std::make_unique<FunctionType>(
      FunctionType{{lambda_expression->m_location},
                   std::move(unified_parameter_types),
                   std::move(return_type)});

  return {
      {lambda_expression->m_location},
      std::move(function_type),
      std::make_unique<LambdaExpr>(LambdaExpr{{lambda_expression->m_location},
                                              std::move(unified_parameters),
                                              std::move(body)})};
}

Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::WhileExpr> &) {
  // TODO
  return {};
}
Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::ForExpr> &) {
  // TODO
  return {};
}

Expression ExpressionChecker::operator()(const ast::LiteralInt8 &literal) {
  return {{literal.m_location},
          PrimitiveTypeValue{{literal.m_location}, PrimitiveType::I8},
          LiteralI8{{literal.m_location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::LiteralInt32 &literal) {
  return {{literal.m_location},
          PrimitiveTypeValue{{literal.m_location}, PrimitiveType::I32},
          LiteralI32{{literal.m_location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::LiteralInt64 &literal) {
  return {{literal.m_location},
          PrimitiveTypeValue{{literal.m_location}, PrimitiveType::I64},
          LiteralI64{{literal.m_location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::LiteralBool &literal) {
  return {{literal.m_location},
          PrimitiveTypeValue{{literal.m_location}, PrimitiveType::BOOL},
          LiteralBool{{literal.m_location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::LiteralChar &literal) {
  return {{literal.m_location},
          PrimitiveTypeValue{{literal.m_location}, PrimitiveType::CHAR},
          LiteralChar{{literal.m_location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::LiteralUnit &literal) {
  return {{literal.m_location},
          PrimitiveTypeValue{{literal.m_location}, PrimitiveType::UNIT},
          LiteralUnit{{literal.m_location}}};
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
