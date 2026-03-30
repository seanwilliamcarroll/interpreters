//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Base AST visitor interface for core node types
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

//****************************************************************************
namespace sc {
//****************************************************************************

// Forward declarations — avoids circular include with ast.hpp
class IntLiteral;
class DoubleLiteral;
class StringLiteral;
class BoolLiteral;
class Identifier;
class ProgramNode;
class CallNode;

class AstVisitor {
public:
  virtual ~AstVisitor() = default;

  virtual void visit(const IntLiteral &) = 0;
  virtual void visit(const DoubleLiteral &) = 0;
  virtual void visit(const StringLiteral &) = 0;
  virtual void visit(const BoolLiteral &) = 0;
  virtual void visit(const Identifier &) = 0;
  virtual void visit(const ProgramNode &) = 0;
  virtual void visit(const CallNode &) = 0;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
