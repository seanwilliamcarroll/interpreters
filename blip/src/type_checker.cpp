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

#include "type.hpp"
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
  // TODO: check callee type
  // If callee is Fn (opaque), skip argument checking, result is unknown
  // If callee is a known built-in, check argument count and types
  (void)node;
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
  // TODO: look up existing type of variable, check new value matches
  // Result is Unit
  (void)node;
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
  // TODO: check the initializer, infer type from it
  // If type annotation present, verify it matches
  // Bind name → type in m_env
  // Result is Unit
  (void)node;
}

void TypeChecker::visit(const DefineFnNode &node) {
  // TODO: create child type environment
  // Bind each param name → its declared type (from TypeNode annotation)
  // Check body type matches declared return type
  // Bind function name → Fn in parent m_env
  // Result is Unit
  (void)node;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
