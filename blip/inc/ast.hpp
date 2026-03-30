//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Blip-specific AST node types (special forms)
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <string>
#include <vector>

#include <ast_visitor.hpp>
#include <sc/ast.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

using sc::AstNode;
using sc::Identifier;
using sc::SourceLocation;

// --- Special forms --------------------------------------------------------
//
// These are blip's keywords — each has a fixed structure that the parser
// enforces. IfNode is filled in as an example.

class IfNode : public AstNode {
public:
  // else_branch may be nullptr (the `if` with no else case)
  IfNode(const SourceLocation &loc, std::unique_ptr<AstNode> condition,
         std::unique_ptr<AstNode> then_branch,
         std::unique_ptr<AstNode> else_branch)
      : AstNode(loc), m_condition(std::move(condition)),
        m_then_branch(std::move(then_branch)),
        m_else_branch(std::move(else_branch)) {}

  const AstNode &get_condition() const { return *m_condition; }
  const AstNode &get_then_branch() const { return *m_then_branch; }
  // Returns nullptr if no else branch
  const AstNode *get_else_branch() const { return m_else_branch.get(); }

  void accept(sc::AstVisitor &v) const override {
    static_cast<BlipAstVisitor &>(v).visit(*this);
  }

private:
  std::unique_ptr<AstNode> m_condition;
  std::unique_ptr<AstNode> m_then_branch;
  std::unique_ptr<AstNode> m_else_branch;
};

class WhileNode : public AstNode {
public:
  WhileNode(const SourceLocation &location, std::unique_ptr<AstNode> condition,
            std::unique_ptr<AstNode> body)
      : AstNode(location), m_condition(std::move(condition)),
        m_body(std::move(body)) {}

  const AstNode &get_condition() const { return *m_condition; }

  const AstNode &get_body() const { return *m_body; }

  void accept(sc::AstVisitor &v) const override {
    static_cast<BlipAstVisitor &>(v).visit(*this);
  }

private:
  const std::unique_ptr<AstNode> m_condition;
  const std::unique_ptr<AstNode> m_body;
};

class SetNode : public AstNode {
public:
  SetNode(const SourceLocation &location, std::unique_ptr<Identifier> name,
          std::unique_ptr<AstNode> value)
      : AstNode(location), m_name(std::move(name)), m_value(std::move(value)) {}

  const Identifier &get_name() const { return *m_name; }

  const AstNode &get_value() const { return *m_value; }

  void accept(sc::AstVisitor &v) const override {
    static_cast<BlipAstVisitor &>(v).visit(*this);
  }

private:
  const std::unique_ptr<Identifier> m_name;
  const std::unique_ptr<AstNode> m_value;
};

// May make sense to push this into core as AstListNode, add iterator,
// operator[], etc
class BeginNode : public AstNode {
public:
  BeginNode(const SourceLocation &location,
            std::vector<std::unique_ptr<AstNode>> expressions)
      : AstNode(location), m_expressions(std::move(expressions)) {}

  const std::vector<std::unique_ptr<AstNode>> &get_expressions() const {
    return m_expressions;
  }

  void accept(sc::AstVisitor &v) const override {
    static_cast<BlipAstVisitor &>(v).visit(*this);
  }

private:
  const std::vector<std::unique_ptr<AstNode>> m_expressions;
};

class PrintNode : public AstNode {
public:
  PrintNode(const SourceLocation &location, std::unique_ptr<AstNode> expression)
      : AstNode(location), m_expression(std::move(expression)) {}

  const AstNode &get_expression() const { return *m_expression; }

  void accept(sc::AstVisitor &v) const override {
    static_cast<BlipAstVisitor &>(v).visit(*this);
  }

private:
  const std::unique_ptr<AstNode> m_expression;
};

// Literally the same as set, just alias?
class DefineVarNode : public AstNode {
public:
  DefineVarNode(const SourceLocation &location,
                std::unique_ptr<Identifier> name,
                std::unique_ptr<AstNode> value)
      : AstNode(location), m_name(std::move(name)), m_value(std::move(value)) {}

  const Identifier &get_name() const { return *m_name; }

  const AstNode &get_value() const { return *m_value; }

  void accept(sc::AstVisitor &v) const override {
    static_cast<BlipAstVisitor &>(v).visit(*this);
  }

private:
  const std::unique_ptr<Identifier> m_name;
  const std::unique_ptr<AstNode> m_value;
};

class DefineFnNode : public AstNode {
public:
  DefineFnNode(const SourceLocation &location, std::unique_ptr<Identifier> name,
               std::vector<std::unique_ptr<Identifier>> arguments,
               std::unique_ptr<AstNode> body)
      : AstNode(location), m_name(std::move(name)),
        m_arguments(std::move(arguments)), m_body(std::move(body)) {}

  const Identifier &get_name() const { return *m_name; }

  const std::vector<std::unique_ptr<Identifier>> &get_arguments() const {
    return m_arguments;
  }

  const AstNode &get_body() const { return *m_body; }

  void accept(sc::AstVisitor &v) const override {
    static_cast<BlipAstVisitor &>(v).visit(*this);
  }

private:
  const std::unique_ptr<Identifier> m_name;
  const std::vector<std::unique_ptr<Identifier>> m_arguments;
  const std::unique_ptr<AstNode> m_body;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
