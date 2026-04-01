//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Debug AST dumper — shows tree structure with node types
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

class AstDumper : public AstVisitor {
public:
  std::string dump(const AstNode &node) {
    m_out.str("");
    m_indent = 0;
    node.accept(*this);
    return m_out.str();
  }

  void visit(const IntLiteral &node) override {
    line("IntLiteral(" + std::to_string(node.get_value()) + ")");
  }

  void visit(const DoubleLiteral &node) override {
    line("DoubleLiteral(" + std::to_string(node.get_value()) + ")");
  }

  void visit(const StringLiteral &node) override {
    line("StringLiteral(\"" + node.get_value() + "\")");
  }

  void visit(const BoolLiteral &node) override {
    line(std::string("BoolLiteral(") + (node.get_value() ? "true" : "false") +
         ")");
  }

  void visit(const Identifier &node) override {
    std::string extra;
    if (node.get_type() != nullptr) {
      extra = " : " + node.get_type()->get_type_name();
    }
    line("Identifier(\"" + node.get_name() + "\"" + extra + ")");
  }

  void visit(const TypeNode &node) override {
    line("TypeNode(\"" + node.get_type_name() + "\")");
  }

  void visit(const ProgramNode &node) override {
    line("ProgramNode");
    indent([&] {
      for (const auto &expr : node.get_program()) {
        expr->accept(*this);
      }
    });
  }

  void visit(const CallNode &node) override {
    line("CallNode");
    indent([&] {
      line("callee:");
      indent([&] { node.get_callee().accept(*this); });
      if (!node.get_arguments().empty()) {
        line("args:");
        indent([&] {
          for (const auto &arg : node.get_arguments()) {
            arg->accept(*this);
          }
        });
      }
    });
  }

  void visit(const IfNode &node) override {
    line("IfNode");
    indent([&] {
      line("condition:");
      indent([&] { node.get_condition().accept(*this); });
      line("then:");
      indent([&] { node.get_then_branch().accept(*this); });
      if (node.get_else_branch() != nullptr) {
        line("else:");
        indent([&] { node.get_else_branch()->accept(*this); });
      }
    });
  }

  void visit(const WhileNode &node) override {
    line("WhileNode");
    indent([&] {
      line("condition:");
      indent([&] { node.get_condition().accept(*this); });
      line("body:");
      indent([&] { node.get_body().accept(*this); });
    });
  }

  void visit(const SetNode &node) override {
    line("SetNode");
    indent([&] {
      line("name:");
      indent([&] { node.get_name().accept(*this); });
      line("value:");
      indent([&] { node.get_value().accept(*this); });
    });
  }

  void visit(const BeginNode &node) override {
    line("BeginNode");
    indent([&] {
      for (const auto &expr : node.get_expressions()) {
        expr->accept(*this);
      }
    });
  }

  void visit(const PrintNode &node) override {
    line("PrintNode");
    indent([&] { node.get_expression().accept(*this); });
  }

  void visit(const DefineVarNode &node) override {
    line("DefineVarNode");
    indent([&] {
      line("name:");
      indent([&] { node.get_name().accept(*this); });
      if (node.get_type() != nullptr) {
        line("annotation:");
        indent([&] { node.get_type()->accept(*this); });
      }
      line("value:");
      indent([&] { node.get_value().accept(*this); });
    });
  }

  void visit(const DefineFnNode &node) override {
    line("DefineFnNode");
    indent([&] {
      line("name:");
      indent([&] { node.get_name().accept(*this); });
      if (!node.get_arguments().empty()) {
        line("params:");
        indent([&] {
          for (const auto &arg : node.get_arguments()) {
            arg->accept(*this);
          }
        });
      }
      line("return_type:");
      indent([&] { node.get_return_type().accept(*this); });
      line("body:");
      indent([&] { node.get_body().accept(*this); });
    });
  }

private:
  void line(const std::string &text) {
    m_out << std::string(static_cast<size_t>(m_indent * 2), ' ') << text
          << "\n";
  }

  template <typename Fn> void indent(Fn &&fn) {
    ++m_indent;
    fn();
    --m_indent;
  }

  std::stringstream m_out;
  int m_indent = 0;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
