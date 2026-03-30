//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : AST visitor interface for all blip node types
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

//****************************************************************************
namespace blip {
//****************************************************************************

// Forward declarations — avoids circular include with ast.hpp
class IntLiteral;
class DoubleLiteral;
class StringLiteral;
class BoolLiteral;
class Identifier;
class ProgramNode;
class CallNode;
class IfNode;
class WhileNode;
class SetNode;
class BeginNode;
class PrintNode;
class DefineVarNode;
class DefineFnNode;

class AstVisitor {
public:
  virtual ~AstVisitor() = default;

  // Literals
  virtual void visit(const IntLiteral &) = 0;
  virtual void visit(const DoubleLiteral &) = 0;
  virtual void visit(const StringLiteral &) = 0;
  virtual void visit(const BoolLiteral &) = 0;
  virtual void visit(const Identifier &) = 0;

  // Structure
  virtual void visit(const ProgramNode &) = 0;
  virtual void visit(const CallNode &) = 0;

  // Special forms
  virtual void visit(const IfNode &) = 0;
  virtual void visit(const WhileNode &) = 0;
  virtual void visit(const SetNode &) = 0;
  virtual void visit(const BeginNode &) = 0;
  virtual void visit(const PrintNode &) = 0;
  virtual void visit(const DefineVarNode &) = 0;
  virtual void visit(const DefineFnNode &) = 0;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
