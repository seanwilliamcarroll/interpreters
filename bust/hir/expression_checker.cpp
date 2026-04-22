//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression type checker implementation.
//*
//*
//****************************************************************************

#include <ast/nodes.hpp>
#include <exceptions.hpp>
#include <hir/block_checker.hpp>
#include <hir/context.hpp>
#include <hir/environment.hpp>
#include <hir/expression_checker.hpp>
#include <hir/instantiation_record.hpp>
#include <hir/nodes.hpp>
#include <hir/type_arena.hpp>
#include <hir/type_converter.hpp>
#include <hir/type_unifier.hpp>
#include <hir/types.hpp>
#include <nodes.hpp>
#include <operators.hpp>
#include <source_location.hpp>
#include <types.hpp>

#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

Expression
ExpressionChecker::check_expression(const ast::Expression &expression) {
  return std::visit(
      [&](const auto &expr) -> Expression {
        return (*this)(expr, expression.m_location);
      },
      expression.m_expression);
}

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

PrimitiveTypeClass required_type_class_for_unary_op(UnaryOperator op) {
  switch (op) {
  case UnaryOperator::MINUS:
    return PrimitiveTypeClass::NUMERIC;
  case UnaryOperator::NOT:
    return PrimitiveTypeClass::BOOL;
  }
}

PrimitiveTypeClass required_type_class_for_binary_op(BinaryOperator op) {
  switch (op) {
  case BinaryOperator::LOGICAL_AND:
  case BinaryOperator::LOGICAL_OR:
    return PrimitiveTypeClass::BOOL;
  case BinaryOperator::PLUS:
  case BinaryOperator::MINUS:
  case BinaryOperator::MULTIPLIES:
  case BinaryOperator::DIVIDES:
  case BinaryOperator::MODULUS:
  case BinaryOperator::LT:
  case BinaryOperator::LT_EQ:
  case BinaryOperator::GT:
  case BinaryOperator::GT_EQ:
    return PrimitiveTypeClass::NUMERIC;
  case BinaryOperator::EQ:
  case BinaryOperator::NOT_EQ:
    return PrimitiveTypeClass::COMPARABLE;
  }
}

Expression ExpressionChecker::operator()(const ast::Identifier &identifier,
                                         const core::SourceLocation &location) {
  auto maybe_type = m_ctx.m_env.lookup(identifier.m_name);
  if (!maybe_type.has_value()) {
    throw core::CompilerException(
        "TypeChecker",
        "Did not find identifier: " + identifier.m_name +
            " in type environment!",
        location);
  }

  const auto &[binding_id, type_scheme] = maybe_type.value();

  auto final_type = m_ctx.create_fresh_type_vars(binding_id, type_scheme);

  return {{location},
          final_type,
          Identifier{{location}, identifier.m_name, binding_id, final_type}};
}

Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::TupleExpr> &tuple_expr,
                              const core::SourceLocation &location) {

  std::vector<Expression> fields;
  fields.reserve(tuple_expr->m_fields.size());
  std::vector<TypeId> field_types;
  field_types.reserve(tuple_expr->m_fields.size());
  for (const auto &field : tuple_expr->m_fields) {
    fields.emplace_back(check_expression(field));
    field_types.emplace_back(fields.back().m_type);
  }
  auto tuple_type = m_ctx.m_type_arena.intern(TupleType{
      .m_fields = std::move(field_types),
  });

  return {
      {location},
      tuple_type,
      std::make_unique<TupleExpr>(TupleExpr{
          .m_fields = std::move(fields),
      }),
  };
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::CallExpr> &call_expression,
    const core::SourceLocation &location) {
  auto callee = check_expression(call_expression->m_callee);

  std::vector<Expression> arguments;
  arguments.reserve(call_expression->m_arguments.size());
  for (const auto &argument : call_expression->m_arguments) {
    arguments.emplace_back(check_expression(argument));
  }

  // TODO Clean this up

  auto callee_type_id = callee.m_type;
  if (m_ctx.is_function(callee_type_id)) {
    // Check types of arguments against their parameters, then bind and type
    // check the body

    const auto &function_type = m_ctx.as_function(callee_type_id);

    if (function_type.m_parameters.size() != arguments.size()) {
      throw core::CompilerException(
          "TypeChecker",
          "Callee expects " +
              std::to_string(function_type.m_parameters.size()) +
              " arguments, " + std::to_string(arguments.size()) + " provided",
          location);
    }

    for (const auto &[index, parameter_type, argument] : std::views::zip(
             std::views::iota(0ULL), function_type.m_parameters, arguments)) {
      try {
        m_ctx.m_type_unifier.unify(parameter_type, argument.m_type);
      } catch (std::runtime_error &error) {
        throw core::CompilerException(
            "TypeChecker",
            "Type unification error!\n Parameter at index: " +
                std::to_string(index) +
                " expects type: " + m_ctx.to_string(parameter_type) +
                " Unification error: " + error.what(),
            argument.m_location);
      }
    }
    // They all match

    auto return_type = m_ctx.m_type_unifier.find(function_type.m_return_type);

    return {{location},
            return_type,
            std::make_unique<CallExpr>(
                CallExpr{std::move(callee), std::move(arguments)})};
  }
  if (m_ctx.is_type_variable(callee_type_id)) {
    // Need to make fresh type variables
    auto function_type = FunctionType{};
    function_type.m_parameters.reserve(arguments.size());
    for (const auto &argument : arguments) {
      function_type.m_parameters.push_back(argument.m_type);
    }
    function_type.m_return_type = m_ctx.m_type_unifier.new_type_var();

    auto function_type_id = m_ctx.m_type_arena.intern(function_type);

    // Do single unification
    try {
      m_ctx.m_type_unifier.unify(function_type_id, callee_type_id);
    } catch (std::runtime_error &error) {
      throw core::CompilerException(
          "TypeChecker",
          "Type unification error!\nCallee type: " +
              m_ctx.to_string(callee_type_id) +
              " cannot unify with: " + m_ctx.to_string(function_type_id) +
              " Unification error: " + error.what(),
          callee.m_location);
    }

    auto return_type = m_ctx.m_type_unifier.find(function_type.m_return_type);

    return {{location},
            return_type,
            std::make_unique<CallExpr>(
                CallExpr{std::move(callee), std::move(arguments)})};
  }
  throw core::CompilerException(
      "TypeChecker",
      "Expression of type: " + m_ctx.to_string(callee_type_id) +
          " is not callable, nor a type variable!",
      callee.m_location);
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::BinaryExpr> &binary_expression,
    const core::SourceLocation &location) {
  // Both arguments must match
  auto lhs = check_expression(binary_expression->m_lhs);
  auto rhs = check_expression(binary_expression->m_rhs);

  try {
    m_ctx.m_type_unifier.unify(lhs.m_type, rhs.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error: ") + error.what(),
        location);
  }

  auto type_id = m_ctx.m_type_unifier.find(lhs.m_type);

  // Apply operator constraint
  auto required =
      required_type_class_for_binary_op(binary_expression->m_operator);
  try {
    if (m_ctx.is_type_variable(type_id)) {
      m_ctx.m_type_unifier.constrain(m_ctx.as_type_variable(type_id), required);
    } else if (!is_type_in_type_class(required,
                                      m_ctx.m_type_arena.get(type_id))) {
      throw core::CompilerException("TypeChecker",
                                    "Type " + m_ctx.to_string(type_id) +
                                        " is disallowed by binary operator: " +
                                        binary_expression->m_operator,
                                    location);
    }
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error: ") + error.what(),
        location);
  }

  auto final_type = is_comparison_op(binary_expression->m_operator)
                        ? m_ctx.m_type_arena.m_bool
                        : m_ctx.m_type_unifier.find(type_id);

  return {{location},
          final_type,
          std::make_unique<BinaryExpr>(BinaryExpr{
              binary_expression->m_operator, std::move(lhs), std::move(rhs)})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::UnaryExpr> &unary_expression,
    const core::SourceLocation &location) {
  auto expression = check_expression(unary_expression->m_expression);

  auto required_type_class =
      required_type_class_for_unary_op(unary_expression->m_operator);
  try {
    if (m_ctx.is_type_variable(expression.m_type)) {
      m_ctx.m_type_unifier.constrain(m_ctx.as_type_variable(expression.m_type),
                                     required_type_class);
    } else if (!is_type_in_type_class(
                   required_type_class,
                   m_ctx.m_type_arena.get(expression.m_type))) {
      throw core::CompilerException(
          "TypeChecker",
          "Type Mismatch! UnaryExpr: " + unary_expression->m_operator +
              " does not accept type: " + m_ctx.to_string(expression.m_type),
          location);
    }
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error: ") + error.what(),
        location);
  }

  auto final_type = m_ctx.m_type_unifier.find(expression.m_type);

  auto final_expression = Expression{
      {expression.m_location}, final_type, std::move(expression.m_expression)};

  return {{location},
          final_expression.m_type,
          std::make_unique<UnaryExpr>(UnaryExpr{unary_expression->m_operator,
                                                std::move(final_expression)})};
}

Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::IfExpr> &if_expression,
                              const core::SourceLocation &location) {

  auto condition = check_expression(if_expression->m_condition);

  try {
    m_ctx.m_type_unifier.unify(condition.m_type, m_ctx.m_type_arena.m_bool);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        location);
  }

  auto resolved_condition =
      Expression{{condition.m_location},
                 m_ctx.m_type_unifier.find(condition.m_type),
                 std::move(condition.m_expression)};

  // Directly call because Block is not a variant type, don't need to visit
  auto then_block =
      BlockChecker{m_ctx}.check_block(if_expression->m_then_block);

  if (!if_expression->m_else_block.has_value()) {
    // Just then branch

    try {
      m_ctx.m_type_unifier.unify(m_ctx.m_type_arena.m_unit, then_block.m_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException(
          "TypeChecker",
          std::string("Type unification error!: ") + error.what(), location);
    }

    return {{location},
            m_ctx.m_type_arena.m_unit,
            std::make_unique<IfExpr>(IfExpr{
                std::move(resolved_condition), std::move(then_block), {}})};
  }
  // Check else branch too
  auto else_block =
      BlockChecker{m_ctx}.check_block(if_expression->m_else_block.value());

  try {
    m_ctx.m_type_unifier.unify(then_block.m_type, else_block.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        location);
  }

  auto type = m_ctx.m_type_arena.m_never == then_block.m_type
                  ? m_ctx.m_type_unifier.find(else_block.m_type)
                  : m_ctx.m_type_unifier.find(then_block.m_type);

  return {{location},
          type,
          std::make_unique<IfExpr>(
              IfExpr{std::move(resolved_condition), std::move(then_block),
                     std::optional<Block>(std::move(else_block))})};
}

Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::Block> &block,
                              const core::SourceLocation &location) {
  auto hir_block = BlockChecker{m_ctx}.check_block(*block);
  return {{location},
          hir_block.m_type,
          std::make_unique<Block>(std::move(hir_block))};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::CastExpr> &cast_expression,
    const core::SourceLocation &location) {

  auto hir_expression = check_expression(cast_expression->m_expression);

  auto original_type_id = m_ctx.m_type_unifier.find(hir_expression.m_type);

  auto new_type_id =
      std::visit(TypeConverter{m_ctx}, cast_expression->m_new_type);

  if (!m_ctx.is_primitive(original_type_id) ||
      !m_ctx.is_primitive(new_type_id)) {
    throw core::CompilerException(
        "TypeChecker",
        "Cannot cast an expression with type: " +
            m_ctx.to_string(original_type_id) +
            " to type: " + m_ctx.to_string(new_type_id),
        location);
  }

  auto original_primitive = m_ctx.as_primitive(original_type_id).m_type;
  auto new_primitive = m_ctx.as_primitive(new_type_id).m_type;

  if (!can_cast(original_primitive, new_primitive)) {
    throw core::CompilerException("TypeChecker",
                                  "Cannot cast an expression with type: " +
                                      to_string(original_primitive) +
                                      " to type: " + to_string(new_primitive),
                                  location);
  }

  return {{location},
          new_type_id,
          std::make_unique<CastExpr>(
              CastExpr{std::move(hir_expression), new_type_id})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::ReturnExpr> &return_expression,
    const core::SourceLocation &location) {
  auto expression = check_expression(return_expression->m_expression);

  if (m_ctx.m_return_type_stack.empty()) {
    throw core::InternalCompilerError(
        "return expression outside function body");
  }

  try {
    m_ctx.m_type_unifier.unify(expression.m_type,
                               m_ctx.m_return_type_stack.back());
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        location);
  }

  auto expression_type = m_ctx.m_type_unifier.find(expression.m_type);

  auto final_expression = (expression_type == expression.m_type)
                              ? std::move(expression)
                              : Expression{{expression.m_location},
                                           expression_type,
                                           std::move(expression.m_expression)};

  return {
      {location},
      m_ctx.m_type_arena.m_never,
      std::make_unique<ReturnExpr>(ReturnExpr{std::move(final_expression)})};
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::LambdaExpr> &lambda_expression,
    const core::SourceLocation &location) {
  // Want to evaluate body and see what the type is
  auto [parameters, parameter_types] =
      TypeConverter{m_ctx}.convert_parameters(lambda_expression->m_parameters);

  auto possible_return_type_id =
      lambda_expression->m_return_type.has_value()
          ? TypeConverter{m_ctx}.get_type(
                lambda_expression->m_return_type.value())
          : m_ctx.m_type_unifier.new_type_var();

  auto body =
      lambda_expression->m_return_type.has_value()
          ? BlockChecker{m_ctx}.check_callable_body(
                parameters, possible_return_type_id, lambda_expression->m_body)
          : BlockChecker{m_ctx}.check_block_with_parameters(
                parameters, lambda_expression->m_body);

  auto return_type_id = lambda_expression->m_return_type.has_value()
                            ? possible_return_type_id
                            : body.m_type;

  try {
    m_ctx.m_type_unifier.unify(return_type_id, body.m_type);
  } catch (std::runtime_error &error) {
    throw core::CompilerException(
        "TypeChecker", std::string("Type unification error!: ") + error.what(),
        location);
  }

  auto function_type_id = m_ctx.m_type_arena.intern(
      FunctionType{.m_parameters = std::move(parameter_types),
                   .m_return_type = return_type_id});

  return {{location},
          function_type_id,
          std::make_unique<LambdaExpr>(LambdaExpr{
              std::move(parameters), std::move(body), return_type_id})};
}

Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::DotExpr> &dot_expr,
                              const core::SourceLocation &location) {
  // Need to check internal expression
  // Assert that this expression is known to be a tuple
  // Check that the index is valid given the arity of the tuple
  // Final type is the type of the field at that index

  auto expression = check_expression(dot_expr->m_expression);

  if (!m_ctx.m_type_arena.is_tuple(expression.m_type)) {
    throw core::CompilerException(
        "TypeChecker",
        "Tuple dot operator requires that the "
        "expression be resolvable to a tuple type, not: " +
            m_ctx.to_string(expression.m_type),
        location);
  }

  const auto &tuple_type = m_ctx.m_type_arena.as_tuple(expression.m_type);
  const auto arity = tuple_type.m_fields.size();

  if (dot_expr->m_tuple_index >= arity) {
    throw core::CompilerException(
        "TypeChecker",
        "Tuple dot operator requires that accessor be strictly less than tuple "
        "arity, arity: " +
            std::to_string(arity) +
            " vs. accessor index: " + std::to_string(dot_expr->m_tuple_index),
        location);
  }

  auto final_type = tuple_type.m_fields[dot_expr->m_tuple_index];

  return {
      {location},
      final_type,
      std::make_unique<DotExpr>(DotExpr{
          .m_expression = std::move(expression),
          .m_tuple_index = dot_expr->m_tuple_index,
      }),
  };
}

Expression ExpressionChecker::operator()(
    const std::unique_ptr<ast::WhileExpr> & /*unused*/,
    const core::SourceLocation & /*unused*/) {
  throw core::InternalCompilerError("Not yet implemented");
}
Expression
ExpressionChecker::operator()(const std::unique_ptr<ast::ForExpr> & /*unused*/,
                              const core::SourceLocation & /*unused*/) {
  throw core::InternalCompilerError("Not yet implemented");
}

Expression ExpressionChecker::operator()(const ast::I8 &literal,
                                         const core::SourceLocation &location) {
  return {{location}, m_ctx.m_type_arena.m_i8, I8{{location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::I32 &literal,
                                         const core::SourceLocation &location) {
  return {
      {location}, m_ctx.m_type_arena.m_i32, I32{{location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::I64 &literal,
                                         const core::SourceLocation &location) {
  return {
      {location}, m_ctx.m_type_arena.m_i64, I64{{location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::Bool &literal,
                                         const core::SourceLocation &location) {
  return {
      {location}, m_ctx.m_type_arena.m_bool, Bool{{location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::Char &literal,
                                         const core::SourceLocation &location) {
  return {
      {location}, m_ctx.m_type_arena.m_char, Char{{location}, literal.m_value}};
}

Expression ExpressionChecker::operator()(const ast::Unit & /*unused*/,
                                         const core::SourceLocation &location) {
  return {{location}, m_ctx.m_type_arena.m_unit, Unit{{location}}};
}

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
