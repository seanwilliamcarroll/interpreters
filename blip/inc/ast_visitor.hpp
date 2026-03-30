//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Blip AST visitor interface, extends core visitor with blip nodes
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <sc/ast_visitor.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class IfNode;
class WhileNode;
class SetNode;
class BeginNode;
class PrintNode;
class DefineVarNode;
class DefineFnNode;

class BlipAstVisitor : public sc::AstVisitor {
public:
  using sc::AstVisitor::visit;
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
