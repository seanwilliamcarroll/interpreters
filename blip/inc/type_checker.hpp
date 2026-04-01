//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Type checker visitor for blip AST
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>

#include <ast.hpp>
#include <ast_visitor.hpp>
#include <environment.hpp>
#include <type.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class TypeChecker : public AstVisitor {
public:
  explicit TypeChecker(std::shared_ptr<TypeEnvironment> env);

  Type check(const AstNode &node);

  // --- Literals ---
  void visit(const IntLiteral &) override;
  void visit(const DoubleLiteral &) override;
  void visit(const StringLiteral &) override;
  void visit(const BoolLiteral &) override;
  void visit(const Identifier &) override;

  // --- Structure ---
  void visit(const TypeNode &) override;
  void visit(const FunctionTypeNode &) override;
  void visit(const ProgramNode &) override;
  void visit(const CallNode &) override;

  // --- Special forms ---
  void visit(const IfNode &) override;
  void visit(const WhileNode &) override;
  void visit(const SetNode &) override;
  void visit(const BeginNode &) override;
  void visit(const PrintNode &) override;
  void visit(const DefineVarNode &) override;
  void visit(const DefineFnNode &) override;

private:
  Type m_result;
  std::shared_ptr<TypeEnvironment> m_env;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
