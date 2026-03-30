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

//****************************************************************************
namespace blip {
//****************************************************************************

class AstPrinter : public AstVisitor {
  static constexpr size_t INDENT_WIDTH = 2;
  static constexpr std::string UNIT_INDENT = std::string(INDENT_WIDTH, ' ');

public:
  std::string print(const AstNode &node) {
    m_out.str("");
    node.accept(*this);
    return m_out.str();
  }

  // --- Core nodes ---

  void visit(const IntLiteral &node) override { m_out << node.get_value(); }

  void visit(const DoubleLiteral &node) override { m_out << node.get_value(); }

  void visit(const StringLiteral &node) override {
    m_out << "\"" << node.get_value() << "\"";
  }

  void visit(const BoolLiteral &node) override {
    m_out << std::boolalpha << node.get_value();
  }

  void visit(const Identifier &node) override { m_out << node.get_name(); }

  void visit(const ProgramNode &node) override {
    for (const auto &expression : node.get_program()) {
      expression->accept(*this);
      m_out << "\n";
    }
  }

  void visit(const CallNode &node) override {
    // Need to print parends
    m_out << "(";

    node.get_callee().accept(*this);

    // Other nodes
    for (const auto &argument : node.get_arguments()) {
      m_out << " ";
      argument->accept(*this);
    }
    m_out << ")";
  }

  // --- Blip nodes ---

  void visit(const IfNode &node) override {
    m_out << "(if ";

    node.get_condition().accept(*this);

    m_out << " ";

    node.get_then_branch().accept(*this);

    if (node.get_else_branch() != nullptr) {
      m_out << " ";
      node.get_else_branch()->accept(*this);
    }

    m_out << ")";
  }

  void visit(const WhileNode &node) override {
    m_out << "(while ";

    node.get_condition().accept(*this);

    m_out << " ";

    node.get_body().accept(*this);

    m_out << ")";
  }

  void visit(const SetNode &node) override {
    m_out << "(set ";

    node.get_name().accept(*this);

    m_out << " ";

    node.get_value().accept(*this);

    m_out << ")";
  }

  void visit(const BeginNode &node) override {
    m_out << "(begin";

    for (const auto &expression : node.get_expressions()) {
      m_out << " ";
      expression->accept(*this);
    }

    m_out << ")";
  }

  void visit(const PrintNode &node) override {
    m_out << "(print ";

    node.get_expression().accept(*this);

    m_out << ")";
  }

  void visit(const DefineVarNode &node) override {
    m_out << "(define ";

    node.get_name().accept(*this);

    m_out << " ";

    node.get_value().accept(*this);

    m_out << ")";
  }

  void visit(const DefineFnNode &node) override {
    m_out << "(define (";

    node.get_name().accept(*this);

    for (const auto &argument : node.get_arguments()) {
      m_out << " ";
      argument->accept(*this);
    }

    m_out << ")";

    m_out << " ";

    node.get_body().accept(*this);

    m_out << ")";
  }

private:
  std::stringstream m_out;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
