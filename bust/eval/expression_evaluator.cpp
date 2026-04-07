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
#include "eval/environment.hpp"
#include "eval/statement_evaluator.hpp"
#include "eval/values.hpp"
#include "exceptions.hpp"
#include "operators.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <ranges>
#include <utility>
#include <variant>

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

Value ExpressionEvaluator::operator()(const hir::LiteralI64 &literal) {
  return I64{literal.m_value};
}

Value ExpressionEvaluator::operator()(const hir::LiteralBool &literal) {
  return Bool{literal.m_value};
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

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::BinaryExpr> &binary_expression) {

  auto lhs = (*this)(binary_expression->m_lhs);
  auto rhs = (*this)(binary_expression->m_rhs);

  auto apply_int = [](const Value &value_a, const Value &value_b,
                      auto operation) {
    return operation(std::get<I64>(value_a).m_value,
                     std::get<I64>(value_b).m_value);
  };

  auto apply_bool = [](const Value &value_a, const Value &value_b,
                       auto operation) {
    return operation(std::get<Bool>(value_a).m_value,
                     std::get<Bool>(value_b).m_value);
  };

  auto apply_int_or_bool =
      [&apply_int, &apply_bool](const Value &value_a, const Value &value_b,
                                auto int_operation, auto bool_operation) {
        if (std::holds_alternative<I64>(value_a)) {
          return apply_int(value_a, value_b, int_operation);
        }
        return apply_bool(value_a, value_b, bool_operation);
      };

  switch (binary_expression->m_operator) {
  case BinaryOperator::LOGICAL_AND:
    return Bool{apply_bool(lhs, rhs, std::logical_and<bool>())};
  case BinaryOperator::LOGICAL_OR:
    return Bool{apply_bool(lhs, rhs, std::logical_or<bool>())};
  case BinaryOperator::PLUS:
    return I64{apply_int(lhs, rhs, std::plus<int64_t>())};
  case BinaryOperator::MINUS:
    return I64{apply_int(lhs, rhs, std::minus<int64_t>())};
  case BinaryOperator::MULTIPLIES:
    return I64{apply_int(lhs, rhs, std::multiplies<int64_t>())};
  case BinaryOperator::DIVIDES:
    return I64{apply_int(lhs, rhs, std::divides<int64_t>())};
  case BinaryOperator::MODULUS:
    return I64{apply_int(lhs, rhs, std::modulus<int64_t>())};
  case BinaryOperator::EQ:
    return Bool{apply_int_or_bool(lhs, rhs, std::equal_to<int64_t>(),
                                  std::equal_to<bool>())};
  case BinaryOperator::LT:
    return Bool{apply_int(lhs, rhs, std::less<int64_t>())};
  case BinaryOperator::LT_EQ:
    return Bool{apply_int(lhs, rhs, std::less_equal<int64_t>())};
  case BinaryOperator::GT:
    return Bool{apply_int(lhs, rhs, std::greater<int64_t>())};
  case BinaryOperator::GT_EQ:
    return Bool{apply_int(lhs, rhs, std::greater_equal<int64_t>())};
  case BinaryOperator::NOT_EQ:
    return Bool{apply_int_or_bool(lhs, rhs, std::not_equal_to<int64_t>(),
                                  std::not_equal_to<bool>())};
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

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::LambdaExpr> &lambda_expression) {
  return create_closure(*lambda_expression);
}

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
