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
  explicit Evaluator(std::shared_ptr<ValueEnvironment>, std::ostream &);

  Value evaluate(const AstNode &);

  // --- Literals ---
  void visit(const IntLiteral &) override;
  void visit(const DoubleLiteral &) override;
  void visit(const StringLiteral &) override;
  void visit(const BoolLiteral &) override;
  void visit(const Identifier &) override;

  // --- Structure ---
  // Evaluator does not care about types
  void visit(const TypeNode &) override {}
  void visit(const FunctionTypeNode &) override {}
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
  void evaluate_function(const CallNode &, Function);
  void evaluate_builtinfunction(const CallNode &, BuiltInFunction);
  std::vector<Value> evaluate_arguments(const CallNode &);

  Value m_result;
  std::shared_ptr<ValueEnvironment> m_env;
  std::ostream &m_out;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
