//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Pretty printer for blip AST trees
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <sstream>
#include <string>

#include <ast.hpp>
#include <ast_visitor.hpp>
#include <sc/ast.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class AstPrinter : public BlipAstVisitor {
public:
  std::string print(const sc::AstNode &node) {
    m_out.str("");
    node.accept(*this);
    return m_out.str();
  }

  // --- Core nodes ---

  void visit(const sc::IntLiteral &node) override {
    // TODO
  }

  void visit(const sc::DoubleLiteral &node) override {
    // TODO
  }

  void visit(const sc::StringLiteral &node) override {
    // TODO
  }

  void visit(const sc::BoolLiteral &node) override {
    // TODO
  }

  void visit(const sc::Identifier &node) override {
    // TODO
  }

  void visit(const sc::ProgramNode &node) override {
    // TODO
  }

  void visit(const sc::CallNode &node) override {
    // TODO
  }

  // --- Blip nodes ---

  void visit(const IfNode &node) override {
    // TODO
  }

  void visit(const WhileNode &node) override {
    // TODO
  }

  void visit(const SetNode &node) override {
    // TODO
  }

  void visit(const BeginNode &node) override {
    // TODO
  }

  void visit(const PrintNode &node) override {
    // TODO
  }

  void visit(const DefineVarNode &node) override {
    // TODO
  }

  void visit(const DefineFnNode &node) override {
    // TODO
  }

private:
  std::stringstream m_out;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
