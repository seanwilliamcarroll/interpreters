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
#include "hir/types.hpp"
#include "source_location.hpp"
#include "types.hpp"
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <type_checker.hpp>
#include <variant>

//****************************************************************************
namespace bust {
//****************************************************************************

struct TypeConverter {
  hir::Type operator()(const ast::DefinedType &type) {
    // TODO
    return hir::UnknownType{{type.m_location}};
  }

  hir::Type operator()(const ast::PrimitiveTypeIdentifier &type) {
    return hir::PrimitiveTypeValue{{type.m_location}, type.m_type};
  }
};

hir::Type get_type(const ast::TypeIdentifier &identifier) {
  return std::visit(TypeConverter{}, identifier);
}

hir::Type get_type(const ast::Identifier &identifier) {
  if (identifier.m_type.has_value()) {
    return std::visit(TypeConverter{}, identifier.m_type.value());
  }
  return hir::UnknownType{{identifier.m_location}};
}

bool allowed_unary_type(UnaryOperator op, const hir::Type &type) {
  if (std::holds_alternative<hir::NeverType>(type)) {
    return true;
  }

  if (std::holds_alternative<hir::UnknownType>(type)) {
    throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                  std::get<hir::UnknownType>(type).m_location);
  }

  switch (op) {
  case UnaryOperator::MINUS:
    return std::holds_alternative<hir::PrimitiveTypeValue>(type) &&
           std::get<hir::PrimitiveTypeValue>(type).m_type == PrimitiveType::I64;
  case UnaryOperator::NOT:
    return std::holds_alternative<hir::PrimitiveTypeValue>(type) &&
           std::get<hir::PrimitiveTypeValue>(type).m_type ==
               PrimitiveType::BOOL;
  }
}
bool allowed_binary_type(BinaryOperator op, const hir::Type &type) {
  if (std::holds_alternative<hir::NeverType>(type)) {
    return true;
  }

  if (std::holds_alternative<hir::UnknownType>(type)) {
    throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                  std::get<hir::UnknownType>(type).m_location);
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
           std::get<hir::PrimitiveTypeValue>(type).m_type == PrimitiveType::I64;
  case BinaryOperator::EQ:
  case BinaryOperator::NOT_EQ:
    return true;
  }
}

hir::Identifier convert_parameter(const ast::Identifier &identifier) {
  return {{identifier.m_location}, identifier.m_name, get_type(identifier)};
}

hir::Type get_statement_type(const hir::Statement &statement) {
  if (std::holds_alternative<hir::Expression>(statement)) {
    return hir::clone_type(std::get<hir::Expression>(statement).m_type);
  }
  const auto &let_binding = std::get<hir::LetBinding>(statement);
  return hir::PrimitiveTypeValue{{let_binding.m_location}, PrimitiveType::UNIT};
}

struct UnifiedChecker {
  hir::Block operator()(const ast::Block &block) {
    m_env.push_scope();

    std::vector<hir::Statement> statements;
    for (const auto &statement : block.m_statements) {
      if (std::holds_alternative<ast::LetBinding>(statement)) {
        statements.emplace_back(convert(std::get<ast::LetBinding>(statement)));
      } else if (std::holds_alternative<ast::Expression>(statement)) {
        statements.emplace_back(
            std::visit((*this), std::get<ast::Expression>(statement)));
      } else {
        throw core::CompilerException("TypeChecker",
                                      "Should never happen, TODO remove this",
                                      block.m_location);
      }
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

    for (const auto &[index, parameter_type, argument] :
         std::views::zip(std::views::iota(0ULL),
                         function_type->m_argument_types, arguments)) {
      if (parameter_type != argument.m_type) {
        throw core::CompilerException(
            "TypeChecker",
            "Type mismatch!\n Parameter at index: " + std::to_string(index) +
                " expects type: " + parameter_type +
                " vs. provided type: " + argument.m_type,
            argument.m_location);
      }
    }
    // They all match

    auto return_type = hir::clone_type(function_type->m_return_type);

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

    if (std::holds_alternative<hir::UnknownType>(lhs.m_type)) {
      // TODO
      throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                    lhs.m_location);
    }

    if (std::holds_alternative<hir::UnknownType>(rhs.m_type)) {
      // TODO
      throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                    rhs.m_location);
    }

    auto type = (std::holds_alternative<hir::NeverType>(lhs.m_type))
                    ? hir::clone_type(lhs.m_type)
                    : hir::clone_type(rhs.m_type);

    if (!std::holds_alternative<hir::NeverType>(lhs.m_type) &&
        !std::holds_alternative<hir::NeverType>(rhs.m_type) &&
        lhs.m_type != rhs.m_type) {
      throw core::CompilerException(
          "TypeChecker",
          "Type Mismatch! BinaryExpr: " + binary_expression->m_operator +
              " expects both "
              "arguments to have same type, lhs: " +
              lhs.m_type + " vs. rhs: " + rhs.m_type,
          binary_expression->m_location);
    }

    // Types match

    // Need to check if binary operator allows this type
    if (!allowed_binary_type(binary_expression->m_operator, type)) {
      throw core::CompilerException("TypeChecker",
                                    "Type " + type +
                                        " is disallowed by binary operator: " +
                                        binary_expression->m_operator,
                                    binary_expression->m_location);
    }

    return {{binary_expression->m_location},
            std::move(type),
            std::make_unique<hir::BinaryExpr>(
                hir::BinaryExpr{{binary_expression->m_location},
                                binary_expression->m_operator,
                                std::move(lhs),
                                std::move(rhs)})};
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::UnaryExpr> &unary_expression) {
    auto expression = std::visit((*this), unary_expression->m_expression);

    if (!allowed_unary_type(unary_expression->m_operator, expression.m_type)) {
      throw core::CompilerException(
          "TypeChecker",
          "Type Mismatch! UnaryExpr: " + unary_expression->m_operator +
              " does not accept type: " + expression.m_type,
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

    if (!std::holds_alternative<hir::PrimitiveTypeValue>(condition.m_type) ||
        std::get<hir::PrimitiveTypeValue>(condition.m_type).m_type !=
            PrimitiveType::BOOL) {
      throw core::CompilerException(
          "TypeChecker",
          "Type mismatch!\nIfExpressions require condition to be of type BOOL, "
          "instead saw type: " +
              condition.m_type,
          condition.m_location);
    }

    // Directly call because Block is not a variant type, don't need to visit
    auto then_branch = (*this)(if_expression->m_then_block);

    if (std::holds_alternative<hir::UnknownType>(then_branch.m_type)) {
      throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                    then_branch.m_location);
    }

    if (!if_expression->m_else_block.has_value()) {
      // Just then branch
      auto type = std::holds_alternative<hir::NeverType>(then_branch.m_type)
                      ? hir::NeverType{}
                      : hir::clone_type(then_branch.m_type);

      return {
          {if_expression->m_location},
          std::move(type),
          std::make_unique<hir::IfExpr>(hir::IfExpr{{if_expression->m_location},
                                                    std::move(condition),
                                                    std::move(then_branch),
                                                    {}})};
    }
    // Check else branch too
    auto else_branch = (*this)(if_expression->m_else_block.value());

    if (std::holds_alternative<hir::UnknownType>(else_branch.m_type)) {
      throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                    else_branch.m_location);
    }

    if (!std::holds_alternative<hir::NeverType>(then_branch.m_type) &&
        !std::holds_alternative<hir::NeverType>(else_branch.m_type) &&
        then_branch.m_type != else_branch.m_type) {
      throw core::CompilerException(
          "TypeChecker",
          "Type mismatch!\nThen branch of type: " + then_branch.m_type +
              " vs. else branch of type: " + else_branch.m_type,
          then_branch.m_location);
    }

    auto type = (std::holds_alternative<hir::NeverType>(then_branch.m_type) ||
                 std::holds_alternative<hir::NeverType>(else_branch.m_type))
                    ? hir::NeverType{}
                    : hir::clone_type(then_branch.m_type);

    return {{if_expression->m_location},
            std::move(type),
            std::make_unique<hir::IfExpr>(hir::IfExpr{
                {if_expression->m_location},
                std::move(condition),
                std::move(then_branch),
                std::optional<hir::Block>(std::move(else_branch))})};
  }

  hir::Expression operator()(const std::unique_ptr<ast::Block> &block) {
    auto hir_block = (*this)(*block);
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

    // Might make sense to throw a different exception here at some point so we
    // can catch it and relay it up to the function/lambda
    if (expression.m_type != m_return_type_stack.back()) {
      throw core::CompilerException(
          "TypeChecker",
          "Type mismatch!\nFunction expects return type: " +
              m_return_type_stack.back() +
              " vs. returning type: " + expression.m_type,
          expression.m_location);
    }

    return {{return_expression->m_location},
            hir::NeverType{},
            std::make_unique<hir::ReturnExpr>(hir::ReturnExpr{
                {return_expression->m_location}, std::move(expression)})};
  }

  hir::Expression
  operator()(const std::unique_ptr<ast::LambdaExpr> &lambda_expression) {
    if (!lambda_expression->m_return_type.has_value()) {
      throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                    lambda_expression->m_location);
    }

    auto return_type = get_type(lambda_expression->m_return_type.value());

    m_env.push_scope();
    std::vector<hir::Identifier> parameters;
    parameters.reserve(lambda_expression->m_parameters.size());
    std::vector<hir::Type> parameter_types;
    parameter_types.reserve(lambda_expression->m_parameters.size());
    for (const auto &parameter : lambda_expression->m_parameters) {
      parameters.emplace_back((*this)(parameter));
      parameter_types.emplace_back(hir::clone_type(parameters.back().m_type));
      m_env.define(parameters.back().m_name,
                   hir::clone_type(parameters.back().m_type));
    }

    m_return_type_stack.push_back(hir::clone_type(return_type));

    auto body = (*this)(lambda_expression->m_body);

    m_return_type_stack.pop_back();
    m_env.pop_scope();

    auto function_type = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{lambda_expression->m_location},
                          std::move(parameter_types),
                          std::move(return_type)});

    return {{lambda_expression->m_location},
            std::move(function_type),
            std::make_unique<hir::LambdaExpr>(
                hir::LambdaExpr{{lambda_expression->m_location},
                                std::move(parameters),
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

  hir::LetBinding convert(const ast::LetBinding &let_binding) {
    // Evaluate the expression's type in the current scope
    // Compare with a type annotation if one is provided
    // Create new LetBinding

    auto body = std::visit((*this), let_binding.m_expression);

    auto annotated_type = get_type(let_binding.m_variable);

    // If annotated type is Unknown, go with type of expression
    // Else, compare the two before assigning
    // On mismatch, throw for now
    if (!std::holds_alternative<hir::UnknownType>(annotated_type)) {
      // At some point we may care about never type here?
      if (body.m_type != annotated_type) {
        throw core::CompilerException(
            "TypeChecker",
            "Type mismatch between annotated type and evaluated expression "
            "type of let binding: " +
                let_binding.m_variable.m_name + "\nAnnotated: " +
                annotated_type + " vs. Evaluated: " + body.m_type,
            let_binding.m_location);
      }
    }

    auto new_identifier = hir::Identifier{{let_binding.m_variable.m_location},
                                          let_binding.m_variable.m_name,
                                          hir::clone_type(body.m_type)};

    // Store the new let binding
    m_env.define(new_identifier.m_name, hir::clone_type(new_identifier.m_type));

    return {
        {let_binding.m_location}, std::move(new_identifier), std::move(body)};
  }

  hir::TopItem operator()(const ast::LetBinding &let_binding) {
    return convert(let_binding);
  }

  hir::TopItem operator()(const ast::FunctionDef &function_def) {
    if (auto other_id = m_env.lookup(function_def.m_id.m_name)) {
      throw core::CompilerException(
          "TypeChecker",
          "Cannot redefine identifier!\nAlready defined " +
              function_def.m_id.m_name + " with type: " + other_id.value() +
              " at " + hir::type_location(other_id.value()),
          function_def.m_id.m_location);
    }

    auto return_type = get_type(function_def.m_return_type);
    if (std::holds_alternative<hir::UnknownType>(return_type)) {
      // Currently illegal
      throw core::CompilerException("TypeChecker", "UNIMPLEMENTED",
                                    hir::type_location(return_type));
    }

    std::vector<hir::Identifier> parameters;
    std::vector<hir::Type> parameter_types;
    parameters.reserve(function_def.m_parameters.size());
    parameter_types.reserve(function_def.m_parameters.size());
    for (const auto &parameter : function_def.m_parameters) {
      parameters.emplace_back(convert_parameter(parameter));
      // TODO: AST enforces a parameter to have a type at the moment
      // We may want to allow Unknown at some point, or even have it default to
      // it
      parameter_types.emplace_back(hir::clone_type(parameters.back().m_type));
    }

    // Construct function type
    auto function_type = std::make_unique<hir::FunctionType>(
        hir::FunctionType{{function_def.m_location},
                          std::move(parameter_types),
                          std::move(return_type)});

    // Need to go to new scope with this function in it
    hir::Type function_type_as_type = std::move(function_type);
    m_env.define(function_def.m_id.m_name,
                 hir::clone_type(function_type_as_type));
    const auto &expected_return_type =
        std::get<std::unique_ptr<hir::FunctionType>>(function_type_as_type)
            ->m_return_type;
    // Even though block will push and pop scope, we can insert a middle scope
    // here with the parameters in it, so we can pop it after
    m_env.push_scope();
    for (const auto &parameter : parameters) {
      m_env.define(parameter.m_name, hir::clone_type(parameter.m_type));
    }

    m_return_type_stack.push_back(hir::clone_type(expected_return_type));
    auto body = (*this)(function_def.m_body);
    m_return_type_stack.pop_back();
    m_env.pop_scope();

    if (!std::holds_alternative<hir::NeverType>(body.m_type) &&
        body.m_type != expected_return_type) {
      throw core::CompilerException(
          "TypeChecker",
          "Type mismatch! Annotated return type: " + expected_return_type +
              " vs. evaluated: " + body.m_type,
          function_def.m_id.m_location);
    }

    return hir::FunctionDef{
        {function_def.m_location},
        function_def.m_id.m_name,
        std::move(std::get<std::unique_ptr<hir::FunctionType>>(
            function_type_as_type)),
        std::move(parameters),
        std::move(body)};
  }

  hir::Environment &m_env;
  std::vector<hir::Type> m_return_type_stack;
};

hir::Program TypeChecker::operator()(const ast::Program &program) {
  std::vector<hir::TopItem> typed_items;
  typed_items.reserve(program.m_items.size());
  for (const auto &top_item : program.m_items) {
    typed_items.push_back(std::visit(
        UnifiedChecker{.m_env = m_env, .m_return_type_stack{}}, top_item));
  }

  return {{program.m_location}, std::move(typed_items)};
}

//****************************************************************************
} // namespace bust
//****************************************************************************
