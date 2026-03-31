//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Evaluator implementation for blip AST
//*
//*
//****************************************************************************

#include "value.hpp"
#include <evaluator.hpp>
#include <exceptions.hpp>
#include <stdexcept>
#include <variant>

//****************************************************************************
namespace blip {
//****************************************************************************

Evaluator::Evaluator(std::shared_ptr<Environment> env, std::ostream &out)
    : m_result(Unit{}), m_env(std::move(env)), m_out(out) {
  (void)m_out;
}

Value Evaluator::evaluate(const AstNode &node) {
  node.accept(*this);
  return m_result;
}

// --- Literals --------------------------------------------------------------

void Evaluator::visit(const IntLiteral &node) { m_result = node.get_value(); }

void Evaluator::visit(const DoubleLiteral &node) {
  m_result = node.get_value();
}

void Evaluator::visit(const StringLiteral &node) {
  m_result = node.get_value();
}

void Evaluator::visit(const BoolLiteral &node) { m_result = node.get_value(); }

void Evaluator::visit(const Identifier &node) {
  try {
    m_result = m_env->lookup(node.get_name());
  } catch (std::runtime_error &error) {
    throw core::CompilerException("RuntimeError", error.what(),
                                  node.get_location());
  }
}

// --- Structure -------------------------------------------------------------

void Evaluator::visit(const ProgramNode &node) {
  for (const auto &expression : node.get_program()) {
    expression->accept(*this);
  }
}

void Evaluator::visit(const CallNode &node) {
  // TODO (Step 5): evaluate callee, evaluate arguments,
  // create new environment, bind params, evaluate body
  (void)node;
}

// --- Special forms ---------------------------------------------------------

void Evaluator::visit(const IfNode &node) {
  node.get_condition().accept(*this);
  if (!std::holds_alternative<bool>(m_result)) {
    throw core::CompilerException("RuntimeError",
                                  "Expect bool type in IF condition, not:" +
                                      value_to_string(m_result),
                                  node.get_location());
  }
  auto condition = std::get<bool>(m_result);

  if (condition) {
    node.get_then_branch().accept(*this);
  } else if (node.get_else_branch() != nullptr) {
    node.get_else_branch()->accept(*this);
  } else {
    m_result = Unit{};
  }
}

void Evaluator::visit(const WhileNode &node) {
  // TODO: loop while condition is truthy, evaluate body each iteration
  // result is Unit
  while (true) {
    node.get_condition().accept(*this);
    if (!std::holds_alternative<bool>(m_result)) {
      throw core::CompilerException(
          "RuntimeError",
          "Expect bool type in WHILE condition, not:" +
              value_to_string(m_result),
          node.get_location());
    }
    if (!std::get<bool>(m_result)) {
      break;
    }
    node.get_body().accept(*this);
  }
  m_result = Unit{};
}

void Evaluator::visit(const SetNode &node) {
  node.get_value().accept(*this);
  try {
    m_env->set(node.get_name().get_name(), m_result);
  } catch (std::runtime_error &error) {
    throw core::CompilerException("RuntimeError", error.what(),
                                  node.get_location());
  }
  m_result = Unit{};
}

void Evaluator::visit(const BeginNode &node) {
  for (const auto &expression : node.get_expressions()) {
    expression->accept(*this);
  }
}

void Evaluator::visit(const PrintNode &node) {
  node.get_expression().accept(*this);
  m_out << value_to_string(m_result) << "\n";
  m_result = Unit{};
}

void Evaluator::visit(const DefineVarNode &node) {
  node.get_value().accept(*this);
  try {
    m_env->define(node.get_name().get_name(), m_result);
  } catch (std::runtime_error &error) {
    throw core::CompilerException("RuntimeError", error.what(),
                                  node.get_location());
  }
  m_result = Unit{};
}

void Evaluator::visit(const DefineFnNode &node) {
  // TODO (Step 5): create a Function value capturing m_env,
  // define it in m_env
  // result is Unit
  (void)node;
}

//****************************************************************************
} // namespace blip
//****************************************************************************
