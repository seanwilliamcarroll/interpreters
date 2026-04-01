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

#include "ast.hpp"
#include "value.hpp"
#include <evaluator.hpp>
#include <exceptions.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>

//****************************************************************************
namespace blip {
//****************************************************************************

Evaluator::Evaluator(std::shared_ptr<ValueEnvironment> env, std::ostream &out)
    : m_result(Unit{}), m_env(std::move(env)), m_out(out) {}

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
  m_result = core::promote_to_compiler_exception(
      "RuntimeError", node.get_location(),
      [&] { return m_env->lookup(node.get_name()); });
}

// --- Structure -------------------------------------------------------------

void Evaluator::visit(const ProgramNode &node) {
  for (const auto &expression : node.get_program()) {
    expression->accept(*this);
  }
}

void check_arity(const CallNode &node, size_t expected_argument_count,
                 const std::string &function_name) {
  if (expected_argument_count != node.get_arguments().size()) {
    throw core::CompilerException(
        "RuntimeError",
        "Function: " + function_name + " expects " +
            std::to_string(expected_argument_count) +
            " arguments, but provided " +
            std::to_string(node.get_arguments().size()),
        node.get_location());
  }
}

std::vector<Value> Evaluator::evaluate_arguments(const CallNode &node) {
  std::vector<Value> evaluated_arguments;
  for (const auto &argument : node.get_arguments()) {
    argument->accept(*this);
    evaluated_arguments.push_back(m_result);
  }
  return evaluated_arguments;
}

void Evaluator::evaluate_function(const CallNode &node, Function function) {
  check_arity(node, function.m_arguments.size(), value_to_string(function));

  std::vector<Value> evaluated_arguments = evaluate_arguments(node);

  auto function_env =
      std::make_shared<ValueEnvironment>(function.m_environment);

  for (size_t index = 0; index < function.m_arguments.size(); ++index) {
    function_env->define(function.m_arguments[index]->get_name(),
                         evaluated_arguments[index]);
  }

  std::swap(function_env, m_env);

  function.m_body->accept(*this);

  std::swap(function_env, m_env);
}

void Evaluator::evaluate_builtinfunction(const CallNode &node,
                                         BuiltInFunction function) {
  check_arity(node, function.m_expected_arguments, value_to_string(function));

  std::vector<Value> evaluated_arguments = evaluate_arguments(node);

  m_result = function.m_native_function(std::move(evaluated_arguments));
}

void Evaluator::visit(const CallNode &node) {
  node.get_callee().accept(*this);
  if (std::holds_alternative<Function>(m_result)) {
    evaluate_function(node, std::get<Function>(m_result));
  } else if (std::holds_alternative<BuiltInFunction>(m_result)) {
    evaluate_builtinfunction(node, std::get<BuiltInFunction>(m_result));
  } else {
    throw core::CompilerException("RuntimeError",
                                  "Cannot call non-function value: " +
                                      value_to_string(m_result),
                                  node.get_location());
  }
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
  core::promote_to_compiler_exception("RuntimeError", node.get_location(), [&] {
    m_env->set(node.get_name().get_name(), m_result);
  });

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
  core::promote_to_compiler_exception("RuntimeError", node.get_location(), [&] {
    m_env->define(node.get_name().get_name(), m_result);
  });
  m_result = Unit{};
}

void Evaluator::visit(const DefineFnNode &node) {
  std::vector<const Identifier *> argument_list;
  for (const auto &argument : node.get_arguments()) {
    argument_list.push_back(argument.get());
  }

  Function closure{.m_name = node.get_name().get_name(),
                   .m_arguments = argument_list,
                   .m_environment = m_env,
                   .m_body = &node.get_body()};

  m_env->define(node.get_name().get_name(), closure);

  m_result = Unit{};
}

//****************************************************************************
} // namespace blip
//****************************************************************************
