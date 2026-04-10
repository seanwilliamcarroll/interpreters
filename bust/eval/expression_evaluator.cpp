//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of expression evaluator.
//*
//*
//****************************************************************************

#include "eval/expression_evaluator.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "eval/environment.hpp"
#include "eval/statement_evaluator.hpp"
#include "eval/values.hpp"
#include "exceptions.hpp"
#include "hir/types.hpp"
#include "operators.hpp"
#include "types.hpp"

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct ReturnException {
  Value m_return_value;
};

Value ExpressionEvaluator::operator()(const hir::Expression &expression) {
  return std::visit(*this, expression.m_expression);
}

Value ExpressionEvaluator::operator()(const hir::Identifier &identifier) {
  auto value = m_ctx.m_env.lookup(identifier.m_name);

  if (!value.has_value()) {
    throw core::CompilerException(
        "Evaluator",
        "Could not find Identifier: " + identifier.m_name +
            " in value environment!",
        identifier.m_location);
  }

  return value.value();
}

Value ExpressionEvaluator::operator()(const hir::LiteralUnit &) {
  return Unit{};
}

Value ExpressionEvaluator::operator()(const hir::LiteralI8 &literal) {
  return I8{literal.m_value};
}

Value ExpressionEvaluator::operator()(const hir::LiteralI32 &literal) {
  return I32{literal.m_value};
}

Value ExpressionEvaluator::operator()(const hir::LiteralI64 &literal) {
  return I64{literal.m_value};
}

Value ExpressionEvaluator::operator()(const hir::LiteralBool &literal) {
  return Bool{literal.m_value};
}

Value ExpressionEvaluator::operator()(const hir::LiteralChar &literal) {
  return Char{literal.m_value};
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::Block> &block) {
  return (*this)(*block);
}

struct ScopeGuard {
  // More standard way of protecting the environment from leaking from return
  // statements throwing
  ScopeGuard(Environment &env) : m_env(env) { m_env.push_scope(); }
  ~ScopeGuard() { m_env.pop_scope(); }
  Environment &m_env;
};

Value ExpressionEvaluator::operator()(const hir::Block &block) {
  ScopeGuard guard(m_ctx.m_env);

  for (const auto &statement : block.m_statements) {
    std::visit(StatementEvaluator{m_ctx}, statement);
  }

  auto final_value = block.m_final_expression.has_value()
                         ? (*this)(block.m_final_expression.value())
                         : Unit{};

  return final_value;
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::IfExpr> &if_expression) {

  auto condition_value = (*this)(if_expression->m_condition);

  if (!std::holds_alternative<Bool>(condition_value)) {
    throw core::CompilerException("Evaluator", "Condition must be bool",
                                  if_expression->m_location);
  }

  if (std::get<Bool>(condition_value).m_value) {
    return (*this)(if_expression->m_then_branch);
  }

  if (if_expression->m_else_branch.has_value()) {
    return (*this)(if_expression->m_else_branch.value());
  }

  return Unit{};
}

Value ExpressionEvaluator::evaluate_function_body(const hir::Block &block) {
  try {
    return (*this)(block);
  } catch (ReturnException &return_statement) {
    return return_statement.m_return_value;
  }
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::CallExpr> &call_expression) {
  auto callee_value = (*this)(call_expression->m_callee);

  auto closure = std::get<Closure>(callee_value);

  auto new_closure_env = std::make_shared<Scope>(closure.m_scope);

  for (const auto &[parameter, argument] :
       std::views::zip(closure.m_parameters, call_expression->m_arguments)) {
    new_closure_env->define(parameter, (*this)(argument));
  }

  return ExpressionEvaluator{.m_ctx = {.m_env = Environment(new_closure_env)}}
      .evaluate_function_body(*closure.m_expression);
}

// The requires here is ensuring that we the expression within is well formed
// So if we couldn't construct that value, the compiler prunes the branch
template <template <typename> typename Function>
Value same_return_type_op(const Value &lhs, const Value &rhs) {
  return std::visit(
      [&](const auto &left) -> Value {
        using T = std::decay_t<decltype(left)>;
        if constexpr (requires {
                        T{Function<decltype(left.m_value)>{}(left.m_value,
                                                             left.m_value)};
                      }) {
          return T{Function<decltype(left.m_value)>{}(
              left.m_value, std::get<T>(rhs).m_value)};
        } else {
          std::unreachable();
        }
      },
      lhs);
}

template <template <typename> typename Function, typename ReturnType>
Value different_return_type_op(const Value &lhs, const Value &rhs) {
  return std::visit(
      [&](const auto &left) -> Value {
        using T = std::decay_t<decltype(left)>;
        if constexpr (requires {
                        ReturnType{Function<decltype(left.m_value)>{}(
                            left.m_value, left.m_value)};
                      }) {
          return ReturnType{Function<decltype(left.m_value)>{}(
              left.m_value, std::get<T>(rhs).m_value)};
        } else {
          std::unreachable();
        }
      },
      lhs);
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {

  auto lhs = (*this)(binary_expression->m_lhs);
  auto rhs = (*this)(binary_expression->m_rhs);

  switch (binary_expression->m_operator) {
  case BinaryOperator::LOGICAL_AND:
    return same_return_type_op<std::logical_and>(lhs, rhs);
  case BinaryOperator::LOGICAL_OR:
    return same_return_type_op<std::logical_or>(lhs, rhs);
  case BinaryOperator::PLUS:
    return same_return_type_op<std::plus>(lhs, rhs);
  case BinaryOperator::MINUS:
    return same_return_type_op<std::minus>(lhs, rhs);
  case BinaryOperator::MULTIPLIES:
    return same_return_type_op<std::multiplies>(lhs, rhs);
  case BinaryOperator::DIVIDES:
    return same_return_type_op<std::divides>(lhs, rhs);
  case BinaryOperator::MODULUS:
    return same_return_type_op<std::modulus>(lhs, rhs);
  case BinaryOperator::EQ:
    return different_return_type_op<std::equal_to, Bool>(lhs, rhs);
  case BinaryOperator::LT:
    return different_return_type_op<std::less, Bool>(lhs, rhs);
  case BinaryOperator::LT_EQ:
    return different_return_type_op<std::less_equal, Bool>(lhs, rhs);
  case BinaryOperator::GT:
    return different_return_type_op<std::greater, Bool>(lhs, rhs);
  case BinaryOperator::GT_EQ:
    return different_return_type_op<std::greater_equal, Bool>(lhs, rhs);
  case BinaryOperator::NOT_EQ:
    return different_return_type_op<std::not_equal_to, Bool>(lhs, rhs);
  default:
  }
  std::unreachable();
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::UnaryExpr> &unary_expression) {
  auto value = (*this)(unary_expression->m_expression);
  switch (unary_expression->m_operator) {
  case bust::UnaryOperator::MINUS:
    return I64{-std::get<I64>(value).m_value};
  case bust::UnaryOperator::NOT:
    return Bool{!std::get<Bool>(value).m_value};
  }
  std::unreachable();
}

[[noreturn]] Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::ReturnExpr> &return_expression) {
  throw ReturnException{(*this)(return_expression->m_expression)};
}

// The requires here is ensuring that we the expression within is well formed
// So if we couldn't construct that value, the compiler prunes the branch
Value cast_op(const Value &from, PrimitiveType to) {
  return std::visit(
      [&](const auto &value) -> Value {
        if constexpr (requires { value.m_value; }) {
          switch (to) {
          case bust::PrimitiveType::CHAR:
            return Char{static_cast<char>(value.m_value)};
          case bust::PrimitiveType::I8:
            return I8{static_cast<int8_t>(value.m_value)};
          case bust::PrimitiveType::I32:
            return I32{static_cast<int32_t>(value.m_value)};
          case bust::PrimitiveType::I64:
            return I64{static_cast<int64_t>(value.m_value)};
          case bust::PrimitiveType::BOOL:
            return Bool{static_cast<bool>(value.m_value)};
          case bust::PrimitiveType::UNIT:
            std::unreachable();
          }
        } else {
          std::unreachable();
        }
      },
      from);
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::CastExpr> &cast_expression) {
  auto value = (*this)(cast_expression->m_expression);
  // Do cast, we already type checked

  return cast_op(
      value,
      std::get<hir::PrimitiveTypeValue>(cast_expression->m_new_type).m_type);
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::LambdaExpr> &lambda_expression) {
  return create_closure(*lambda_expression);
}

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
