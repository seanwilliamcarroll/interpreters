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

#include <functional>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

#include <ast.hpp>
#include <ast_visitor.hpp>
#include <sc/ast.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

class AstPrinter : public BlipAstVisitor {
  static constexpr size_t INDENT_WIDTH = 2;
  static constexpr std::string UNIT_INDENT = std::string(INDENT_WIDTH, ' ');

public:
  AstPrinter(bool print_line_numbers = false)
      : m_print_line_numbers(print_line_numbers) {}

  std::string print(const sc::AstNode &node) {
    m_out.str("");
    m_line_number = 0;
    m_current_indent = 0;
    node.accept(*this);
    return m_out.str();
  }

  // --- Core nodes ---

  void visit(const sc::IntLiteral &node) override { m_out << node.get_value(); }

  void visit(const sc::DoubleLiteral &node) override {
    m_out << node.get_value();
  }

  void visit(const sc::StringLiteral &node) override {
    m_out << "\"" << node.get_value() << "\"";
  }

  void visit(const sc::BoolLiteral &node) override {
    m_out << std::boolalpha << node.get_value();
  }

  void visit(const sc::Identifier &node) override { m_out << node.get_name(); }

  void visit(const sc::ProgramNode &node) override {
    for (const auto &expression : node.get_program()) {
      print_as_own_line([&] { expression->accept(*this); });
    }
  }

  void visit(const sc::CallNode &node) override {
    // Need to print parends
    m_out << "(\n";

    ++m_current_indent;
    print_as_own_line([&] { node.get_callee().accept(*this); });

    ++m_current_indent;
    // Other nodes
    for (const auto &argument : node.get_arguments()) {
      print_as_own_line([&] { argument->accept(*this); });
    }
    --m_current_indent;

    --m_current_indent;

    print_beginning_of_line();
    m_out << ")";
  }

  // --- Blip nodes ---

  void visit(const IfNode &node) override {
    m_out << "(if\n";
    ++m_current_indent;

    print_as_own_line([&] { node.get_condition().accept(*this); });

    print_as_own_line([&] { node.get_then_branch().accept(*this); });

    if (node.get_else_branch() != nullptr) {
      print_as_own_line([&] { node.get_else_branch()->accept(*this); });
    }

    --m_current_indent;
    print_beginning_of_line();
    m_out << ")";
  }

  void visit(const WhileNode &node) override {
    m_out << "(while\n";
    ++m_current_indent;

    print_as_own_line([&] { node.get_condition().accept(*this); });

    print_as_own_line([&] { node.get_body().accept(*this); });

    --m_current_indent;
    print_beginning_of_line();
    m_out << ")";
  }

  void visit(const SetNode &node) override {
    m_out << "(set\n";
    ++m_current_indent;

    print_as_own_line([&] { node.get_name().accept(*this); });

    print_as_own_line([&] { node.get_value().accept(*this); });

    --m_current_indent;
    print_beginning_of_line();
    m_out << ")";
  }

  void visit(const BeginNode &node) override {
    m_out << "(begin\n";
    ++m_current_indent;

    for (const auto &expression : node.get_expressions()) {
      print_as_own_line([&] { expression->accept(*this); });
    }

    --m_current_indent;
    print_beginning_of_line();
    m_out << ")";
  }

  void visit(const PrintNode &node) override {
    m_out << "(print\n";
    ++m_current_indent;

    print_as_own_line([&] { node.get_expression().accept(*this); });

    --m_current_indent;
    print_beginning_of_line();
    m_out << ")";
  }

  void visit(const DefineVarNode &node) override {
    m_out << "(define\n";
    ++m_current_indent;

    print_as_own_line([&] { node.get_name().accept(*this); });

    print_as_own_line([&] { node.get_value().accept(*this); });

    --m_current_indent;
    print_beginning_of_line();
    m_out << ")";
  }

  void visit(const DefineFnNode &node) override {
    m_out << "(define\n";
    ++m_current_indent;

    print_as_own_line([&] { m_out << "("; });

    print_as_own_line([&] { node.get_name().accept(*this); });

    ++m_current_indent;
    for (const auto &argument : node.get_arguments()) {
      print_as_own_line([&] { argument->accept(*this); });
    }
    --m_current_indent;

    print_as_own_line([&] { m_out << ")"; });

    ++m_current_indent;

    print_as_own_line([&] { node.get_body().accept(*this); });

    --m_current_indent;
    --m_current_indent;
    print_beginning_of_line();
    m_out << ")";
  }

private:
  void print_beginning_of_line() {
    if (m_print_line_numbers) {
      m_out << std::setw(4) << m_line_number++ << ": ";
    }
    for (size_t index = 0; index < m_current_indent; ++index) {
      m_out << UNIT_INDENT;
    }
  }

  void print_as_own_line(auto internal_print) {
    print_beginning_of_line();
    internal_print();
    m_out << "\n";
  }

  const bool m_print_line_numbers;
  std::stringstream m_out;
  size_t m_current_indent = 0;
  size_t m_line_number = 0;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
