//**** Copyright © 2023-2024 Sean Carroll, Jonathon Bell. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : AST base class and leaf node types
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>
#include <string>
#include <vector>

#include <sc/source_location.hpp>

//****************************************************************************
namespace sc {
//****************************************************************************

// Base class for all AST nodes. Every node carries a source location
// so that later phases (evaluation, error reporting) can point back
// at the original source.

class AstNode {
public:
  explicit AstNode(const SourceLocation &loc) : m_location(loc) {}

  virtual ~AstNode() = default;

  const SourceLocation &get_location() const { return m_location; }

private:
  const SourceLocation m_location;
};

// --- Leaf nodes (atoms) ---------------------------------------------------
//
// Each leaf wraps a single value. These correspond to the `atom` rule
// in the grammar. IntLiteral is filled in as an example.

template <typename LiteralType>
class AbstractLiteral : public AstNode {
public:
  AbstractLiteral(const SourceLocation &loc, LiteralType value)
    : AstNode(loc), m_value(std::move(value)) {}

  LiteralType get_value() const { return m_value; }

private:
  const LiteralType m_value;
};

using IntLiteral = AbstractLiteral<int>;

using DoubleLiteral = AbstractLiteral<double>;

using StringLiteral = AbstractLiteral<std::string>;

using BoolLiteral = AbstractLiteral<bool>;

class Identifier : public AstNode {
public:
  Identifier(const SourceLocation &location, std::string name)
      : AstNode(location), m_name(std::move(name)) {}

  const std::string& get_name() const { return m_name; }
  
private:
  const std::string m_name;
};


// --- Interior nodes (generic) ---------------------------------------------
//
// CallNode and ProgramNode are language-agnostic — most s-expression
// languages would have them.

class ProgramNode : public AstNode {
public:
  ProgramNode(const SourceLocation &location, std::vector<std::unique_ptr<AstNode>> program) : AstNode(location), m_program(std::move(program)) {}

  const std::vector<std::unique_ptr<AstNode>>& get_program() const {return m_program;}

private:
  std::vector<std::unique_ptr<AstNode>> m_program;
};

class CallNode : public AstNode {
public:
  CallNode(const SourceLocation &location, std::unique_ptr<AstNode> callee,
           std::vector<std::unique_ptr<AstNode>> arguments)
    : AstNode(location), m_callee(std::move(callee)), m_arguments(std::move(arguments)) {}

  const AstNode& get_callee() const {return *m_callee;}

  const std::vector<std::unique_ptr<AstNode>>& get_arguments() const {return m_arguments;}
  
private:
  std::unique_ptr<AstNode> m_callee;
  std::vector<std::unique_ptr<AstNode>> m_arguments;
};

//****************************************************************************
} // namespace sc
//****************************************************************************
