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
#include "value.hpp"
#include <exceptions.hpp>
#include <stdexcept>
#include <type_checker.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

TypeChecker::TypeChecker(std::shared_ptr<TypeEnvironment> env)
    : m_result(Type::Unit), m_env(std::move(env)) {}

Type TypeChecker::check(const AstNode &node) {
  node.accept(*this);
  return m_result;
}

// --- Literals --------------------------------------------------------------

void TypeChecker::visit(const IntLiteral &) { m_result = Type::Int; }

void TypeChecker::visit(const DoubleLiteral &) { m_result = Type::Double; }

void TypeChecker::visit(const StringLiteral &) { m_result = Type::String; }

void TypeChecker::visit(const BoolLiteral &) { m_result = Type::Bool; }

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

void TypeChecker::visit(const ProgramNode &node) {
  for (const auto &expression : node.get_program()) {
    expression->accept(*this);
  }
}

void TypeChecker::visit(const CallNode &node) {
  node.get_callee().accept(*this);
  if (m_result != Type::Fn) {
    throw core::CompilerException(
        "TypeChecker",
        "Cannot call expression of type: " + type_to_string(m_result) +
            " as function!",
        node.get_location());
  }

  for (const auto &argument : node.get_arguments()) {
    // Need to type check argument expressions
    argument->accept(*this);
  }

  // If our callee is an identifier, we can try to type check things
  auto function_identifier =
      dynamic_cast<const Identifier *>(&node.get_callee());
  if (function_identifier == nullptr) {
    // Unnamed function, shouldn't be possible yet?
    m_result = Type::Fn;
    return;
  }

  try {
    m_result = m_env->lookup(function_identifier->get_name());
  } catch (std::runtime_error &error) {
    throw core::CompilerException("TypeChecker", error.what(),
                                  node.get_location());
  }

  // No way to check on arguments or return type at the moment?
}

// --- Special forms ---------------------------------------------------------

void TypeChecker::visit(const IfNode &node) {
  node.get_condition().accept(*this);

  if (m_result != Type::Bool) {
    throw core::CompilerException(
        "TypeChecker",
        "If statement must have a boolean condition, not: " +
            type_to_string(m_result),
        node.get_location());
  }

  node.get_then_branch().accept(*this);
  if (node.get_else_branch() == nullptr) {
    if (m_result != Type::Unit) {
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

  if (m_result != Type::Bool) {
    throw core::CompilerException(
        "TypeChecker",
        "while loop must have a boolean condition, not: " +
            type_to_string(m_result),
        node.get_location());
  }

  // Make sure we check inside here
  node.get_body().accept(*this);

  m_result = Type::Unit;
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
  m_result = Type::Unit;
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
  m_result = Type::Unit;
}

void TypeChecker::visit(const DefineVarNode &node) {
  node.get_value().accept(*this);

  if (node.get_type() != nullptr &&
      string_to_type(node.get_type()->get_type_name()) != m_result) {
    throw core::CompilerException(
        "TypeChecker",
        "Mismatched types in define expression! Annotated type: " +
            node.get_type()->get_type_name() +
            " vs. inferred expression type: " + type_to_string(m_result),
        node.get_location());
  }

  m_env->define(node.get_name().get_name(), m_result);

  m_result = Type::Unit;
}

void TypeChecker::visit(const DefineFnNode &node) {
  auto function_env = std::make_shared<TypeEnvironment>(m_env);

  for (const auto &argument : node.get_arguments()) {
    if (argument->get_type() == nullptr) {
      throw core::CompilerException("TypeChecker",
                                    "Argument: " + argument->get_name() +
                                        " is missing required type annotation!",
                                    argument->get_location());
    }

    function_env->define(argument->get_name(),
                         string_to_type(argument->get_type()->get_type_name()));
  }

  std::swap(function_env, m_env);
  node.get_body().accept(*this);
  if (m_result != string_to_type(node.get_return_type().get_type_name())) {
    throw core::CompilerException(
        "TypeChecker",
        "Annotated return type of function: " + node.get_name().get_name() +
            " does not match evaluated type! Annotated: " +
            node.get_return_type().get_type_name() +
            " vs. Evaluated: " + type_to_string(m_result),
        node.get_location());
  }

  std::swap(function_env, m_env);

  // TODO: Do better than this
  m_env->define(node.get_name().get_name(), Type::Fn);

  m_result = Type::Unit;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
