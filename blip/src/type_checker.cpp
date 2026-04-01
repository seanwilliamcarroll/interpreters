//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type checker implementation for blip AST
//*
//*
//****************************************************************************

#include "ast.hpp"
#include "environment.hpp"
#include "type.hpp"
#include <exceptions.hpp>
#include <stdexcept>
#include <string>
#include <type_checker.hpp>
#include <variant>

//****************************************************************************
namespace blip {
//****************************************************************************

TypeChecker::TypeChecker(std::shared_ptr<TypeEnvironment> env)
    : m_result(BaseType::Unit), m_env(std::move(env)) {}

Type TypeChecker::check(const AstNode &node) {
  node.accept(*this);
  return m_result;
}

// --- Literals --------------------------------------------------------------

void TypeChecker::visit(const IntLiteral &) { m_result = BaseType::Int; }

void TypeChecker::visit(const DoubleLiteral &) { m_result = BaseType::Double; }

void TypeChecker::visit(const StringLiteral &) { m_result = BaseType::String; }

void TypeChecker::visit(const BoolLiteral &) { m_result = BaseType::Bool; }

void TypeChecker::visit(const Identifier &node) {
  try {
    m_result = m_env->lookup(node.get_name());
  } catch (std::runtime_error &error) {
    throw core::CompilerException("TypeChecker", error.what(),
                                  node.get_location());
  }
}

// --- Structure -------------------------------------------------------------

void TypeChecker::visit(const TypeNode &node) {
  throw core::CompilerException("TypeChecker",
                                "We should never type check the TypeNode!",
                                node.get_location());
}

void TypeChecker::visit(const FunctionTypeNode &node) {
  throw core::CompilerException(
      "TypeChecker", "We should never type check the FunctionTypeNode!",
      node.get_location());
}

void TypeChecker::visit(const ProgramNode &node) {
  for (const auto &expression : node.get_program()) {
    expression->accept(*this);
  }
}

void TypeChecker::visit(const CallNode &node) {
  node.get_callee().accept(*this);
  if (!std::holds_alternative<std::shared_ptr<FunctionType>>(m_result)) {
    throw core::CompilerException(
        "TypeChecker",
        "Cannot call expression of type: " + type_to_string(m_result) +
            " as function!",
        node.get_location());
  }

  const FunctionType &function_type =
      *std::get<std::shared_ptr<FunctionType>>(m_result);

  if (node.get_arguments().size() != function_type.m_parameter_types.size()) {
    throw core::CompilerException(
        "TypeChecker",
        "Function of type: " + type_to_string(m_result) + " requires: " +
            std::to_string(function_type.m_parameter_types.size()) +
            " argument(s), was provided: " +
            std::to_string(node.get_arguments().size()) + " argument(s)",
        node.get_location());
  }

  for (size_t index = 0; index < node.get_arguments().size(); ++index) {
    const auto &argument = node.get_arguments()[index];
    argument->accept(*this);
    if (m_result != function_type.m_parameter_types[index]) {
      throw core::CompilerException(
          "TypeChecker",
          "Function of type: " + type_to_string(function_type) + ": Argument " +
              std::to_string(index) + " expects type " +
              type_to_string(function_type.m_parameter_types[index]) +
              " but was provided: " + type_to_string(m_result),
          argument->get_location());
    }
  }

  // Our function call is valid, now we return the return type?

  m_result = function_type.m_return_type;
}

// --- Special forms ---------------------------------------------------------

void TypeChecker::visit(const IfNode &node) {
  node.get_condition().accept(*this);

  if (m_result != BaseType::Bool) {
    throw core::CompilerException(
        "TypeChecker",
        "If statement must have a boolean condition, not: " +
            type_to_string(m_result),
        node.get_location());
  }

  node.get_then_branch().accept(*this);
  if (node.get_else_branch() == nullptr) {
    if (m_result != BaseType::Unit) {
      throw core::CompilerException(
          "TypeChecker",
          "If without else branch requires body to have Unit type! Not: " +
              type_to_string(m_result),
          node.get_location());
    }
    return;
  }
  auto then_type = m_result;

  node.get_else_branch()->accept(*this);
  auto else_type = m_result;

  if (then_type != else_type) {
    throw core::CompilerException(
        "TypeChecker",
        "Mismatched types in if branches! Then type: " +
            type_to_string(then_type) +
            " vs. Else type: " + type_to_string(else_type),
        node.get_location());
  }
}

void TypeChecker::visit(const WhileNode &node) {
  node.get_condition().accept(*this);

  if (m_result != BaseType::Bool) {
    throw core::CompilerException(
        "TypeChecker",
        "while loop must have a boolean condition, not: " +
            type_to_string(m_result),
        node.get_location());
  }

  // Make sure we check inside here
  node.get_body().accept(*this);

  m_result = BaseType::Unit;
}

void TypeChecker::visit(const SetNode &node) {
  try {
    m_result = m_env->lookup(node.get_name().get_name());
  } catch (std::runtime_error &error) {
    throw core::CompilerException("TypeChecker", error.what(),
                                  node.get_location());
  }
  auto defined_type = m_result;
  node.get_value().accept(*this);

  if (m_result != defined_type) {
    throw core::CompilerException(
        "TypeChecker",
        "Mismatched types in set expression! Defined type: " +
            type_to_string(defined_type) +
            " vs. assigned type: " + type_to_string(m_result),
        node.get_location());
  }
  m_result = BaseType::Unit;
}

void TypeChecker::visit(const BeginNode &node) {
  for (const auto &expression : node.get_expressions()) {
    expression->accept(*this);
  }
}

void TypeChecker::visit(const PrintNode &node) {
  // Need to do this in case there is a type error deep in the expression being
  // printed
  node.get_expression().accept(*this);
  m_result = BaseType::Unit;
}

void TypeChecker::visit(const DefineVarNode &node) {
  node.get_value().accept(*this);

  if (node.get_type() != nullptr &&
      node_to_type(*node.get_type()) != m_result) {
    throw core::CompilerException(
        "TypeChecker",
        "Mismatched types in define expression! Annotated type: " +
            node.get_type()->get_type_name() +
            " vs. inferred expression type: " + type_to_string(m_result),
        node.get_location());
  }

  m_env->define(node.get_name().get_name(), m_result);

  m_result = BaseType::Unit;
}

void TypeChecker::visit(const DefineFnNode &node) {
  auto function_env = std::make_shared<TypeEnvironment>(m_env);

  auto function_type = std::make_shared<FunctionType>();

  for (const auto &argument : node.get_arguments()) {
    if (argument->get_type() == nullptr) {
      throw core::CompilerException("TypeChecker",
                                    "Argument: " + argument->get_name() +
                                        " is missing required type annotation!",
                                    argument->get_location());
    }

    function_env->define(argument->get_name(),
                         node_to_type(*argument->get_type()));
    function_type->m_parameter_types.push_back(
        node_to_type(*argument->get_type()));
  }

  function_type->m_return_type = node_to_type(node.get_return_type());

  // Snapshot type environment in case we fail during body evaluation

  // Put it in the function env so the child type env can find it in case of
  // recursion, but doesn't pollute current env if exception occurs
  function_env->define(node.get_name().get_name(), function_type);

  try {
    std::swap(function_env, m_env);

    node.get_body().accept(*this);
    if (m_result != node_to_type(node.get_return_type())) {
      throw core::CompilerException(
          "TypeChecker",
          "Annotated return type of function: " + node.get_name().get_name() +
              " does not match evaluated type! Annotated: " +
              node.get_return_type().get_type_name() +
              " vs. Evaluated: " + type_to_string(m_result),
          node.get_location());
    }
  } catch (...) {
    // Need to put the snapshotted type env back
    std::swap(function_env, m_env);
    throw;
  }

  std::swap(function_env, m_env);

  // Define it for good in the existing type env
  m_env->define(node.get_name().get_name(), function_type);

  m_result = BaseType::Unit;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
