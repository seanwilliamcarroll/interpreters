//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : All blip AST node types
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <ast_visitor.hpp>
#include <source_location.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

using core::SourceLocation;

// --- Base -----------------------------------------------------------------

class AstNode {
public:
  explicit AstNode(const SourceLocation &loc) : m_location(loc) {}

  virtual ~AstNode() = default;

  virtual void accept(AstVisitor &v) const = 0;

  const SourceLocation &get_location() const { return m_location; }

private:
  const SourceLocation m_location;
};

// --- Leaf nodes (atoms) ---------------------------------------------------

template <typename LiteralType> class AbstractLiteral : public AstNode {
public:
  AbstractLiteral(const SourceLocation &loc, LiteralType value)
      : AstNode(loc), m_value(std::move(value)) {}

  LiteralType get_value() const { return m_value; }

private:
  const LiteralType m_value;
};

class IntLiteral : public AbstractLiteral<int> {
public:
  using AbstractLiteral::AbstractLiteral;
  void accept(AstVisitor &v) const override { v.visit(*this); }
};

class DoubleLiteral : public AbstractLiteral<double> {
public:
  using AbstractLiteral::AbstractLiteral;
  void accept(AstVisitor &v) const override { v.visit(*this); }
};

class StringLiteral : public AbstractLiteral<std::string> {
public:
  using AbstractLiteral::AbstractLiteral;
  void accept(AstVisitor &v) const override { v.visit(*this); }
};

class BoolLiteral : public AbstractLiteral<bool> {
public:
  using AbstractLiteral::AbstractLiteral;
  void accept(AstVisitor &v) const override { v.visit(*this); }
};

// --- Interior nodes -------------------------------------------------------

class BaseTypeNode : public AstNode {
public:
  BaseTypeNode(const SourceLocation &location) : AstNode(location) {}

  virtual const std::string &get_type_name() const = 0;
};

class TypeNode : public BaseTypeNode {
public:
  TypeNode(const SourceLocation &location, std::string type_name)
      : BaseTypeNode(location), m_type_name(std::move(type_name)) {}

  const std::string &get_type_name() const override { return m_type_name; }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  const std::string m_type_name;
};

class FunctionTypeNode : public BaseTypeNode {
public:
  FunctionTypeNode(const SourceLocation &location,
                   std::vector<std::unique_ptr<BaseTypeNode>> parameter_types,
                   std::unique_ptr<BaseTypeNode> return_type)
      : BaseTypeNode(location), m_parameter_types(std::move(parameter_types)),
        m_return_type(std::move(return_type)), m_type_name(create_type_name()) {
  }

  const std::string &get_type_name() const override { return m_type_name; }

  const BaseTypeNode &get_return_type_name() const { return *m_return_type; }

  const std::vector<std::unique_ptr<BaseTypeNode>> &
  get_parameter_type_names() const {
    return m_parameter_types;
  }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  std::string create_type_name() const {
    std::stringstream output;
    for (const auto &parameter_type_name : m_parameter_types) {
      output << parameter_type_name->get_type_name() << " ";
    }
    output << "-> " << m_return_type->get_type_name();
    return output.str();
  }

  const std::vector<std::unique_ptr<BaseTypeNode>> m_parameter_types;
  const std::unique_ptr<BaseTypeNode> m_return_type;
  const std::string m_type_name;
};

class Identifier : public AstNode {
public:
  Identifier(const SourceLocation &location, std::string name,
             std::unique_ptr<BaseTypeNode> type = nullptr)
      : AstNode(location), m_name(std::move(name)), m_type(std::move(type)) {}

  const std::string &get_name() const { return m_name; }

  const BaseTypeNode *get_type() const { return m_type.get(); }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  const std::string m_name;
  const std::unique_ptr<BaseTypeNode> m_type;
};

class ProgramNode : public AstNode {
public:
  ProgramNode(const SourceLocation &location,
              std::vector<std::unique_ptr<AstNode>> program)
      : AstNode(location), m_program(std::move(program)) {}

  const std::vector<std::unique_ptr<AstNode>> &get_program() const {
    return m_program;
  }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  std::vector<std::unique_ptr<AstNode>> m_program;
};

class CallNode : public AstNode {
public:
  CallNode(const SourceLocation &location, std::unique_ptr<AstNode> callee,
           std::vector<std::unique_ptr<AstNode>> arguments)
      : AstNode(location), m_callee(std::move(callee)),
        m_arguments(std::move(arguments)) {}

  const AstNode &get_callee() const { return *m_callee; }

  const std::vector<std::unique_ptr<AstNode>> &get_arguments() const {
    return m_arguments;
  }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  std::unique_ptr<AstNode> m_callee;
  std::vector<std::unique_ptr<AstNode>> m_arguments;
};

// --- Special forms --------------------------------------------------------

class IfNode : public AstNode {
public:
  IfNode(const SourceLocation &loc, std::unique_ptr<AstNode> condition,
         std::unique_ptr<AstNode> then_branch,
         std::unique_ptr<AstNode> else_branch)
      : AstNode(loc), m_condition(std::move(condition)),
        m_then_branch(std::move(then_branch)),
        m_else_branch(std::move(else_branch)) {}

  const AstNode &get_condition() const { return *m_condition; }
  const AstNode &get_then_branch() const { return *m_then_branch; }
  const AstNode *get_else_branch() const { return m_else_branch.get(); }

  void accept(AstVisitor &v) const override { v.visit(*this); }

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

  void accept(AstVisitor &v) const override { v.visit(*this); }

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

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  const std::unique_ptr<Identifier> m_name;
  const std::unique_ptr<AstNode> m_value;
};

class BeginNode : public AstNode {
public:
  BeginNode(const SourceLocation &location,
            std::vector<std::unique_ptr<AstNode>> expressions)
      : AstNode(location), m_expressions(std::move(expressions)) {}

  const std::vector<std::unique_ptr<AstNode>> &get_expressions() const {
    return m_expressions;
  }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  const std::vector<std::unique_ptr<AstNode>> m_expressions;
};

class PrintNode : public AstNode {
public:
  PrintNode(const SourceLocation &location, std::unique_ptr<AstNode> expression)
      : AstNode(location), m_expression(std::move(expression)) {}

  const AstNode &get_expression() const { return *m_expression; }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  const std::unique_ptr<AstNode> m_expression;
};

class DefineVarNode : public AstNode {
public:
  DefineVarNode(const SourceLocation &location,
                std::unique_ptr<Identifier> name,
                std::unique_ptr<AstNode> value,
                std::unique_ptr<BaseTypeNode> type = nullptr)
      : AstNode(location), m_name(std::move(name)), m_value(std::move(value)),
        m_type(std::move(type)) {}

  const Identifier &get_name() const { return *m_name; }
  const AstNode &get_value() const { return *m_value; }

  const BaseTypeNode *get_type() const { return m_type.get(); }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  const std::unique_ptr<Identifier> m_name;
  const std::unique_ptr<AstNode> m_value;
  const std::unique_ptr<BaseTypeNode> m_type;
};

// TODO: May be able to remove completely in future
class DefineFnNode : public AstNode {
public:
  DefineFnNode(const SourceLocation &location, std::unique_ptr<Identifier> name,
               std::vector<std::unique_ptr<Identifier>> arguments,
               std::unique_ptr<AstNode> body,
               std::unique_ptr<BaseTypeNode> return_type)
      : AstNode(location), m_name(std::move(name)),
        m_arguments(std::move(arguments)), m_body(std::move(body)),
        m_return_type(
            std::move(return_type)) // , m_type(create_function_type_node())
  {}

  const Identifier &get_name() const { return *m_name; }

  const std::vector<std::unique_ptr<Identifier>> &get_arguments() const {
    return m_arguments;
  }

  const AstNode &get_body() const { return *m_body; }

  const BaseTypeNode &get_return_type() const { return *m_return_type; }
  // const FunctionTypeNode &get_type() const { return *m_type; }

  void accept(AstVisitor &v) const override { v.visit(*this); }

private:
  // std::unique_ptr<FunctionTypeNode> create_function_type_node() {
  //   std::vector<std::unique_ptr<BaseTypeNode>> argument_types;
  //   return {};
  // }

  const std::unique_ptr<Identifier> m_name;
  const std::vector<std::unique_ptr<Identifier>> m_arguments;
  const std::unique_ptr<AstNode> m_body;
  const std::unique_ptr<BaseTypeNode> m_return_type;
  // const std::unique_ptr<FunctionTypeNode> m_type;
};

//****************************************************************************
} // namespace blip
//****************************************************************************
