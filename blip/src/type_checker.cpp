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

#include <exceptions.hpp>
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

void TypeChecker::visit(const IntLiteral &node) {
  // TODO: set m_result to the type of an int literal
  (void)node;
}

void TypeChecker::visit(const DoubleLiteral &node) {
  // TODO: set m_result to the type of a double literal
  (void)node;
}

void TypeChecker::visit(const StringLiteral &node) {
  // TODO: set m_result to the type of a string literal
  (void)node;
}

void TypeChecker::visit(const BoolLiteral &node) {
  // TODO: set m_result to the type of a bool literal
  (void)node;
}

void TypeChecker::visit(const Identifier &node) {
  // TODO: look up the identifier's type in m_env
  // Wrap errors with CompilerException and node.get_location()
  (void)node;
}

// --- Structure -------------------------------------------------------------

void TypeChecker::visit(const TypeNode &node) {
  // TypeNode isn't visited during checking — types are read directly
  // from the AST via get_type() / get_return_type()
  (void)node;
}

void TypeChecker::visit(const ProgramNode &node) {
  // TODO: check each expression in sequence, m_result is the last one's type
  (void)node;
}

void TypeChecker::visit(const CallNode &node) {
  // TODO: check callee type
  // If callee is Fn (opaque), skip argument checking, result is unknown
  // If callee is a known built-in, check argument count and types
  (void)node;
}

// --- Special forms ---------------------------------------------------------

void TypeChecker::visit(const IfNode &node) {
  // TODO: condition must be Bool
  // If else branch exists, both branches must have the same type
  // If no else branch, result is Unit
  (void)node;
}

void TypeChecker::visit(const WhileNode &node) {
  // TODO: condition must be Bool, result is Unit
  (void)node;
}

void TypeChecker::visit(const SetNode &node) {
  // TODO: look up existing type of variable, check new value matches
  // Result is Unit
  (void)node;
}

void TypeChecker::visit(const BeginNode &node) {
  // TODO: check each expression in sequence, m_result is the last one's type
  (void)node;
}

void TypeChecker::visit(const PrintNode &node) {
  // TODO: check the expression (any type is printable), result is Unit
  (void)node;
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
