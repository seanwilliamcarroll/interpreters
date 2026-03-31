//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Tree-walking evaluator for blip AST
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <iosfwd>
#include <memory>

#include <ast.hpp>
#include <ast_visitor.hpp>
#include <environment.hpp>
#include <value.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class Evaluator : public AstVisitor {
public:
  explicit Evaluator(std::shared_ptr<Environment> env, std::ostream &out);

  Value evaluate(const AstNode &node);

  // --- Literals ---
  void visit(const IntLiteral &node) override;
  void visit(const DoubleLiteral &node) override;
  void visit(const StringLiteral &node) override;
  void visit(const BoolLiteral &node) override;
  void visit(const Identifier &node) override;

  // --- Structure ---
  void visit(const ProgramNode &node) override;
  void visit(const CallNode &node) override;

  // --- Special forms ---
  void visit(const IfNode &node) override;
  void visit(const WhileNode &node) override;
  void visit(const SetNode &node) override;
  void visit(const BeginNode &node) override;
  void visit(const PrintNode &node) override;
  void visit(const DefineVarNode &node) override;
  void visit(const DefineFnNode &node) override;

private:
  Value m_result;
  std::shared_ptr<Environment> m_env;
  std::ostream &m_out;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
