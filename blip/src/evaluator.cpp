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

#include <evaluator.hpp>
#include <exceptions.hpp>

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

void Evaluator::visit(const IntLiteral &node) {
  // TODO: store node's value in m_result
  (void)node;
}

void Evaluator::visit(const DoubleLiteral &node) {
  // TODO: store node's value in m_result
  (void)node;
}

void Evaluator::visit(const StringLiteral &node) {
  // TODO: store node's value in m_result
  (void)node;
}

void Evaluator::visit(const BoolLiteral &node) {
  // TODO: store node's value in m_result
  (void)node;
}

void Evaluator::visit(const Identifier &node) {
  // TODO: look up node's name in m_env, store in m_result
  // Catch std::runtime_error from Environment and re-throw
  // as CompilerException with node.get_location()
  (void)node;
}

// --- Structure -------------------------------------------------------------

void Evaluator::visit(const ProgramNode &node) {
  // TODO: evaluate each child expression in sequence
  // m_result is the value of the last expression (or Unit if empty)
  (void)node;
}

void Evaluator::visit(const CallNode &node) {
  // TODO (Step 5): evaluate callee, evaluate arguments,
  // create new environment, bind params, evaluate body
  (void)node;
}

// --- Special forms ---------------------------------------------------------

void Evaluator::visit(const IfNode &node) {
  // TODO: evaluate condition, check truthiness,
  // evaluate then or else branch accordingly
  (void)node;
}

void Evaluator::visit(const WhileNode &node) {
  // TODO: loop while condition is truthy, evaluate body each iteration
  // result is Unit
  (void)node;
}

void Evaluator::visit(const SetNode &node) {
  // TODO: evaluate value expression, call m_env->set()
  // result is Unit
  (void)node;
}

void Evaluator::visit(const BeginNode &node) {
  // TODO: evaluate each expression in sequence
  // result is the value of the last expression
  (void)node;
}

void Evaluator::visit(const PrintNode &node) {
  // TODO: evaluate expression, write value_to_string() to m_out
  // result is Unit
  (void)node;
}

void Evaluator::visit(const DefineVarNode &node) {
  // TODO: evaluate value expression, call m_env->define()
  // result is Unit
  (void)node;
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
