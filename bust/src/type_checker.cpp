//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of the TypeChecker pass.
//*
//*
//****************************************************************************

#include "ast/nodes.hpp"
#include "ast/types.hpp"
#include "exceptions.hpp"
#include "hir/nodes.hpp"
#include "hir/type_environment.hpp"
#include "hir/type_unifier.hpp"
#include "hir/types.hpp"
#include "operators.hpp"
#include "source_location.hpp"
#include "types.hpp"
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_checker.hpp>
#include <variant>

//****************************************************************************
namespace bust {
//****************************************************************************

struct TypeConverter {
  hir::Type operator()(const ast::DefinedType &type) {
    throw core::CompilerException(
        "TypeChecker", std::string("UNIMPLEMENTED") + " " + __PRETTY_FUNCTION__,
        type.m_location);
  }

  hir::Type operator()(const ast::PrimitiveTypeIdentifier &type) {
    return hir::PrimitiveTypeValue{{type.m_location}, type.m_type};
  }
};

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

hir::Type get_statement_type(const hir::Statement &statement) {
  if (std::holds_alternative<hir::Expression>(statement)) {
    return hir::clone_type(std::get<hir::Expression>(statement).m_type);
  }
  const auto &let_binding = std::get<hir::LetBinding>(statement);
  return hir::PrimitiveTypeValue{{let_binding.m_location}, PrimitiveType::UNIT};
}

struct UnifiedChecker {

  bool allowed_unary_type(UnaryOperator op, const hir::Type &type) {
    if (std::holds_alternative<hir::NeverType>(type)) {
      return true;
    }

    if (std::holds_alternative<hir::TypeVariable>(type)) {
      switch (op) {
      case UnaryOperator::MINUS:
        m_type_unifier.unify(std::get<hir::TypeVariable>(type),
                             hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
        return true;
      case UnaryOperator::NOT:
        m_type_unifier.unify(std::get<hir::TypeVariable>(type),
                             hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL});
        return true;
      }
    }

    switch (op) {
    case UnaryOperator::MINUS:
      return std::holds_alternative<hir::PrimitiveTypeValue>(type) &&
             std::get<hir::PrimitiveTypeValue>(type).m_type ==
                 PrimitiveType::I64;
    case UnaryOperator::NOT:
      return std::holds_alternative<hir::PrimitiveTypeValue>(type) &&
             std::get<hir::PrimitiveTypeValue>(type).m_type ==
                 PrimitiveType::BOOL;
    }
  }

  bool allowed_binary_type(BinaryOperator op, const hir::Type &type) {
    if (std::holds_alternative<hir::NeverType>(type)) {
      // Less sure about this
      return true;
    }

    if (std::holds_alternative<hir::TypeVariable>(type)) {
      // Add constraint
      switch (op) {
      case BinaryOperator::LOGICAL_AND:
      case BinaryOperator::LOGICAL_OR:
        m_type_unifier.unify(std::get<hir::TypeVariable>(type),
                             hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL});
        return true;
      case BinaryOperator::PLUS:
      case BinaryOperator::MINUS:
      case BinaryOperator::MULTIPLIES:
      case BinaryOperator::DIVIDES:
      case BinaryOperator::MODULUS:
      case BinaryOperator::LT:
      case BinaryOperator::LT_EQ:
      case BinaryOperator::GT:
      case BinaryOperator::GT_EQ:
        m_type_unifier.unify(std::get<hir::TypeVariable>(type),
                             hir::PrimitiveTypeValue{{}, PrimitiveType::I64});
        return true;
      case BinaryOperator::EQ:
      case BinaryOperator::NOT_EQ:
        // Not enough information to add a constraint
        return true;
      }
    }

    switch (op) {
    case BinaryOperator::LOGICAL_AND:
    case BinaryOperator::LOGICAL_OR:
      return std::holds_alternative<hir::PrimitiveTypeValue>(type) &&
             std::get<hir::PrimitiveTypeValue>(type).m_type ==
                 PrimitiveType::BOOL;
    case BinaryOperator::PLUS:
    case BinaryOperator::MINUS:
    case BinaryOperator::MULTIPLIES:
    case BinaryOperator::DIVIDES:
    case BinaryOperator::MODULUS:
    case BinaryOperator::LT:
    case BinaryOperator::LT_EQ:
    case BinaryOperator::GT:
    case BinaryOperator::GT_EQ:
      return std::holds_alternative<hir::PrimitiveTypeValue>(type) &&
             std::get<hir::PrimitiveTypeValue>(type).m_type ==
                 PrimitiveType::I64;
    case BinaryOperator::EQ:
    case BinaryOperator::NOT_EQ:
      return true;
    }
  }

  std::pair<std::vector<hir::Identifier>, std::vector<hir::Type>>
  convert_parameters(const std::vector<ast::Identifier> &ast_params) {
    std::vector<hir::Identifier> parameters;
    std::vector<hir::Type> parameter_types;
    parameters.reserve(ast_params.size());
    parameter_types.reserve(ast_params.size());
    for (const auto &param : ast_params) {
      parameters.emplace_back(convert_parameter(param));
      parameter_types.emplace_back(hir::clone_type(parameters.back().m_type));
    }
    return {std::move(parameters), std::move(parameter_types)};
  }

  hir::Identifier convert_parameter(const ast::Identifier &identifier) {
    return {{identifier.m_location}, identifier.m_name, get_type(identifier)};
  }

  hir::Type get_type(const ast::TypeIdentifier &identifier) {
    return std::visit(TypeConverter{}, identifier);
  }

  hir::Type get_type(const ast::Identifier &identifier) {
    if (identifier.m_type.has_value()) {
      return std::visit(TypeConverter{}, identifier.m_type.value());
    }
    return m_type_unifier.new_type_var();
  }

  hir::Statement check_statement(const ast::Statement &statement) {
    return std::visit(
        [&statement, this](const auto &t) -> hir::Statement {
          using T = std::decay_t<decltype(t)>;
          if constexpr (std::is_same_v<T, ast::LetBinding>) {
            return check_let_binding(std::get<ast::LetBinding>(statement));
          } else {
            return std::visit((*this), std::get<ast::Expression>(statement));
          }
        },
        statement);
  }

  hir::Block check_block(const ast::Block &block) {
    m_env.push_scope();

    std::vector<hir::Statement> statements;
    statements.reserve(block.m_statements.size());
    for (const auto &statement : block.m_statements) {
      statements.emplace_back(check_statement(statement));
    }

    auto final_expression =
        block.m_final_expression.has_value()
            ? std::optional<hir::Expression>(
                  std::visit((*this), block.m_final_expression.value()))
            : std::nullopt;

    auto type =
        final_expression.has_value()
            ? hir::clone_type(final_expression.value().m_type)
        : !statements.empty()
            ? get_statement_type(statements.back())
            : hir::PrimitiveTypeValue{{block.m_location}, PrimitiveType::UNIT};

    m_env.pop_scope();
    return {{block.m_location},
            std::move(type),
            std::move(statements),
            std::move(final_expression)};
  }

  hir::Expression operator()(const ast::Identifier &identifier) {
    auto maybe_type = m_env.lookup(identifier.m_name);
    if (!maybe_type.has_value()) {
      throw core::CompilerException(
          "TypeChecker",
          "Did not find identifier: " + identifier.m_name +
              " in type environment!",
          identifier.m_location);
    }

    return {{identifier.m_location},
            hir::clone_type(maybe_type.value()),
            hir::Identifier{{identifier.m_location},
                            identifier.m_name,
                            hir::clone_type(maybe_type.value())}};
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::CallExpr> &call_expression) {
    auto callee = std::visit((*this), call_expression->m_callee);

    if (!std::holds_alternative<std::unique_ptr<hir::FunctionType>>(
            callee.m_type)) {
      throw core::CompilerException("TypeChecker",
                                    "Expression of type: " + callee.m_type +
                                        " is not callable!",
                                    callee.m_location);
    }
    const auto &function_type =
        std::get<std::unique_ptr<hir::FunctionType>>(callee.m_type);

    std::vector<hir::Expression> arguments;
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
         std::views::zip(std::views::iota(0ULL),
                         function_type->m_argument_types, arguments)) {
      try {
        m_type_unifier.unify(parameter_type, argument.m_type);
      } catch (std::runtime_error &error) {
        throw core::CompilerException(
            "TypeChecker",
            "Type unification error!\n Parameter at index: " +
                std::to_string(index) + " expects type: " + parameter_type +
                " Unificationn error: " + error.what(),
            argument.m_location);
      }
    }
    // They all match

    auto return_type = m_type_unifier.find(function_type->m_return_type);

    return {{call_expression->m_location},
            std::move(return_type),
            std::make_unique<hir::CallExpr>(
                hir::CallExpr{{call_expression->m_location},
                              std::move(callee),
                              std::move(arguments)})};
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::BinaryExpr> &binary_expression) {

    // Both arguments must match
    auto lhs = std::visit((*this), binary_expression->m_lhs);
    auto rhs = std::visit((*this), binary_expression->m_rhs);

    try {
      m_type_unifier.unify(lhs.m_type, rhs.m_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException(
          "TypeChecker", std::string("Type unification error: ") + error.what(),
          binary_expression->m_location);
    }

    auto type = m_type_unifier.find(lhs.m_type);

    // Types match

    // Need to check if binary operator allows this type
    try {
      if (!allowed_binary_type(binary_expression->m_operator, type)) {
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

    auto final_type =
        is_comparison_op(binary_expression->m_operator)
            ? hir::PrimitiveTypeValue{{binary_expression->m_location},
                                      PrimitiveType::BOOL}
            : std::move(type);

    return {{binary_expression->m_location},
            std::move(final_type),
            std::make_unique<hir::BinaryExpr>(
                hir::BinaryExpr{{binary_expression->m_location},
                                binary_expression->m_operator,
                                std::move(lhs),
                                std::move(rhs)})};
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::UnaryExpr> &unary_expression) {
    auto expression = std::visit((*this), unary_expression->m_expression);

    try {
      if (!allowed_unary_type(unary_expression->m_operator,
                              expression.m_type)) {
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

    return {{unary_expression->m_location},
            hir::clone_type(expression.m_type),
            std::make_unique<hir::UnaryExpr>(
                hir::UnaryExpr{{unary_expression->m_location},
                               unary_expression->m_operator,
                               std::move(expression)})};
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::IfExpr> &if_expression) {

    auto condition = std::visit((*this), if_expression->m_condition);

    try {
      m_type_unifier.unify(condition.m_type,
                           hir::PrimitiveTypeValue{{}, PrimitiveType::BOOL});
    } catch (std::runtime_error &error) {
      throw core::CompilerException("TypeChecker",
                                    std::string("Type unification error!: ") +
                                        error.what(),
                                    if_expression->m_location);
    }

    auto resolved_condition =
        hir::Expression{{condition.m_location},
                        m_type_unifier.find(condition.m_type),
                        std::move(condition.m_expression)};

    // Directly call because Block is not a variant type, don't need to visit
    auto then_branch = check_block(if_expression->m_then_block);

    if (std::holds_alternative<hir::TypeVariable>(then_branch.m_type)) {
      throw core::CompilerException("TypeChecker",
                                    std::string("UNIMPLEMENTED") + " " +
                                        __PRETTY_FUNCTION__,
                                    then_branch.m_location);
    }

    if (!if_expression->m_else_block.has_value()) {
      // Just then branch
      auto type = hir::PrimitiveTypeValue{{if_expression->m_location},
                                          PrimitiveType::UNIT};

      return {{if_expression->m_location},
              type,
              std::make_unique<hir::IfExpr>(
                  hir::IfExpr{{if_expression->m_location},
                              std::move(resolved_condition),
                              std::move(then_branch),
                              {}})};
    }
    // Check else branch too
    auto else_branch = check_block(if_expression->m_else_block.value());

    try {
      m_type_unifier.unify(then_branch.m_type, else_branch.m_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException("TypeChecker",
                                    std::string("Type unification error!: ") +
                                        error.what(),
                                    if_expression->m_location);
    }

    auto type = std::holds_alternative<hir::NeverType>(then_branch.m_type)
                    ? m_type_unifier.find(else_branch.m_type)
                    : m_type_unifier.find(then_branch.m_type);

    return {{if_expression->m_location},
            std::move(type),
            std::make_unique<hir::IfExpr>(hir::IfExpr{
                {if_expression->m_location},
                std::move(resolved_condition),
                std::move(then_branch),
                std::optional<hir::Block>(std::move(else_branch))})};
  }

  hir::Expression operator()(const std::unique_ptr<ast::Block> &block) {
    auto hir_block = check_block(*block);
    return {{block->m_location},
            hir::clone_type(hir_block.m_type),
            std::make_unique<hir::Block>(std::move(hir_block))};
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::ReturnExpr> &return_expression) {
    auto expression =
        std::visit((*this), return_expression->m_return_expression);

    if (m_return_type_stack.empty()) {
      // Should never happen?
      throw core::CompilerException("TypeChecker", "Shouldn't happen, right?",
                                    return_expression->m_location);
    }

    try {
      m_type_unifier.unify(expression.m_type, m_return_type_stack.back());
    } catch (std::runtime_error &error) {
      throw core::CompilerException("TypeChecker",
                                    std::string("Type unification error!: ") +
                                        error.what(),
                                    return_expression->m_location);
    }

    auto expression_type = m_type_unifier.find(expression.m_type);

    auto final_expression =
        (expression_type == expression.m_type)
            ? std::move(expression)
            : hir::Expression{{expression.m_location},
                              std::move(expression_type),
                              std::move(expression.m_expression)};

    return {{return_expression->m_location},
            hir::NeverType{{return_expression->m_location}},
            std::make_unique<hir::ReturnExpr>(hir::ReturnExpr{
                {return_expression->m_location}, std::move(final_expression)})};
  }

  hir::Block
  check_block_with_parameters(const std::vector<hir::Identifier> &parameters,
                              const ast::Block &ast_block) {
    m_env.push_scope();
    for (const auto &parameter : parameters) {
      m_env.define(parameter.m_name, hir::clone_type(parameter.m_type));
    }
    auto block = check_block(ast_block);
    m_env.pop_scope();

    return block;
  }

  hir::Block check_callable_body(const std::vector<hir::Identifier> &parameters,
                                 const hir::Type &return_type,
                                 const ast::Block &ast_body) {

    m_return_type_stack.push_back(hir::clone_type(return_type));
    auto body = check_block_with_parameters(parameters, ast_body);
    m_return_type_stack.pop_back();
    return body;
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::LambdaExpr> &lambda_expression) {
    // Want to evaluate body and see what the type is
    auto [parameters, parameter_types] =
        convert_parameters(lambda_expression->m_parameters);

    auto possible_return_type =
        lambda_expression->m_return_type.has_value()
            ? get_type(lambda_expression->m_return_type.value())
            : m_type_unifier.new_type_var();

    auto body = lambda_expression->m_return_type.has_value()
                    ? check_callable_body(parameters, possible_return_type,
                                          lambda_expression->m_body)
                    : check_block_with_parameters(parameters,
                                                  lambda_expression->m_body);

    auto return_type = lambda_expression->m_return_type.has_value()
                           ? std::move(possible_return_type)
                           : hir::clone_type(body.m_type);

    try {
      m_type_unifier.unify(return_type, body.m_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException("TypeChecker",
                                    std::string("Type unification error!: ") +
                                        error.what(),
                                    lambda_expression->m_location);
    }

    // Do we need to rediscover the return type and body types?

    // Expect body to have a unified type?
    std::vector<hir::Identifier> unified_parameters;
    unified_parameters.reserve(parameters.size());
    std::vector<hir::Type> unified_parameter_types;
    unified_parameter_types.reserve(parameter_types.size());
    for (const auto &[parameter, parameter_type] :
         std::views::zip(parameters, parameter_types)) {
      if (!std::holds_alternative<hir::TypeVariable>(parameter_type)) {
        // TODO Could be more efficient than reconstructing
        unified_parameters.emplace_back(
            hir::Identifier{{parameter.m_location},
                            parameter.m_name,
                            hir::clone_type(parameter.m_type)});
        unified_parameter_types.push_back(hir::clone_type(parameter_type));
        continue;
      }
      // See if we can resolve them
      auto unified_type =
          m_type_unifier.find(std::get<hir::TypeVariable>(parameter_type));
      unified_parameters.emplace_back(
          hir::Identifier{{parameter.m_location},
                          parameter.m_name,
                          hir::clone_type(unified_type)});
      unified_parameter_types.push_back(std::move(unified_type));
    }

    auto function_type = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{lambda_expression->m_location},
                          std::move(unified_parameter_types),
                          std::move(return_type)});

    return {{lambda_expression->m_location},
            std::move(function_type),
            std::make_unique<hir::LambdaExpr>(
                hir::LambdaExpr{{lambda_expression->m_location},
                                std::move(unified_parameters),
                                std::move(body)})};
  }

  hir::Expression operator()(const std::unique_ptr<ast::WhileExpr> &) {
    // TODO
    return {};
  }
  hir::Expression operator()(const std::unique_ptr<ast::ForExpr> &) {
    // TODO
    return {};
  }

  hir::Expression operator()(const ast::LiteralInt64 &literal) {
    return {{literal.m_location},
            hir::PrimitiveTypeValue{{literal.m_location}, PrimitiveType::I64},
            hir::LiteralI64{{literal.m_location}, literal.m_value}};
  }

  hir::Expression operator()(const ast::LiteralBool &literal) {
    return {{literal.m_location},
            hir::PrimitiveTypeValue{{literal.m_location}, PrimitiveType::BOOL},
            hir::LiteralBool{{literal.m_location}, literal.m_value}};
  }

  hir::Expression operator()(const ast::LiteralUnit &literal) {
    return {{literal.m_location},
            hir::PrimitiveTypeValue{{literal.m_location}, PrimitiveType::UNIT},
            hir::LiteralUnit{{literal.m_location}}};
  }

  hir::LetBinding check_let_binding(const ast::LetBinding &let_binding) {
    // Evaluate the expression's type in the current scope
    // Compare with a type annotation if one is provided
    // Create new LetBinding

    auto body = std::visit((*this), let_binding.m_expression);

    auto annotated_type = get_type(let_binding.m_variable);

    // If annotated type is Unknown, go with type of expression
    // Else, unify the two (throws on mismatch)
    try {
      m_type_unifier.unify(annotated_type, body.m_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException("TypeChecker",
                                    std::string("Type unification error!: ") +
                                        error.what(),
                                    let_binding.m_location);
    }

    auto unified_type = m_type_unifier.find(annotated_type);

    auto new_identifier = hir::Identifier{{let_binding.m_variable.m_location},
                                          let_binding.m_variable.m_name,
                                          std::move(unified_type)};

    // Store the new let binding
    m_env.define(new_identifier.m_name, hir::clone_type(new_identifier.m_type));

    return {
        {let_binding.m_location}, std::move(new_identifier), std::move(body)};
  }

  hir::TopItem operator()(const ast::LetBinding &let_binding) {
    return check_let_binding(let_binding);
  }

  void collect_function_signature(const ast::FunctionDef &function_def) {
    if (auto other_id = m_env.lookup(function_def.m_id.m_name)) {
      throw core::CompilerException(
          "TypeChecker",
          "Cannot redefine identifier!\nAlready defined " +
              function_def.m_id.m_name + " with type: " + other_id.value() +
              " at " + hir::type_location(other_id.value()),
          function_def.m_id.m_location);
    }

    auto return_type = get_type(function_def.m_return_type);
    if (std::holds_alternative<hir::TypeVariable>(return_type)) {
      // Currently illegal
      throw core::CompilerException("TypeChecker",
                                    std::string("UNIMPLEMENTED") + " " +
                                        __PRETTY_FUNCTION__,
                                    hir::type_location(return_type));
    }

    auto [_, parameter_types] = convert_parameters(function_def.m_parameters);

    auto function_type = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{function_def.m_location},
                          std::move(parameter_types),
                          std::move(return_type)});

    hir::Type function_type_as_type = std::move(function_type);
    m_env.define(function_def.m_id.m_name,
                 hir::clone_type(function_type_as_type));
  }

  hir::TopItem operator()(const ast::FunctionDef &function_def) {
    // Should throw, this would be an error of the type checker itself
    auto maybe_function_type = m_env.lookup(function_def.m_id.m_name);
    if (!maybe_function_type.has_value()) {
      throw core::CompilerException("TypeChecker",
                                    "Compiler error: should have defined " +
                                        function_def.m_id.m_name +
                                        " in first pass!",
                                    function_def.m_location);
    }
    auto function_type = hir::clone_type(maybe_function_type.value());

    auto [parameters, _] = convert_parameters(function_def.m_parameters);

    // Define the function in the environment before checking its body
    // (allows recursion)
    const auto &expected_return_type =
        std::get<std::unique_ptr<hir::FunctionType>>(function_type)
            ->m_return_type;

    auto body = check_callable_body(parameters, expected_return_type,
                                    function_def.m_body);

    try {
      m_type_unifier.unify(body.m_type, expected_return_type);
    } catch (std::runtime_error &error) {
      throw core::CompilerException(
          "TypeChecker", std::string("Type unification error: ") + error.what(),
          function_def.m_location);
    }

    return hir::FunctionDef{
        {function_def.m_location},
        function_def.m_id.m_name,
        std::move(std::get<std::unique_ptr<hir::FunctionType>>(function_type)),
        std::move(parameters),
        std::move(body)};
  }

  hir::Environment &m_env;
  std::vector<hir::Type> m_return_type_stack;
  TypeUnifier m_type_unifier;
};

hir::Program TypeChecker::operator()(const ast::Program &program) {
  auto checker =
      UnifiedChecker{.m_env = m_env, .m_return_type_stack{}, .m_type_unifier{}};

  // First pass to collect function signatures
  for (const auto &top_item : program.m_items) {
    if (!std::holds_alternative<ast::FunctionDef>(top_item)) {
      continue;
    }
    const auto &function_def = std::get<ast::FunctionDef>(top_item);
    checker.collect_function_signature(function_def);
  }

  // Second pass to actually type check everything
  std::vector<hir::TopItem> typed_items;
  typed_items.reserve(program.m_items.size());
  for (const auto &top_item : program.m_items) {
    typed_items.push_back(std::visit(checker, top_item));
  }

  return {{program.m_location}, std::move(typed_items)};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
