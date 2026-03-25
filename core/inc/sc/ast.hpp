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

class IntLiteral : public AstNode {
public:
  IntLiteral(const SourceLocation &loc, int value)
      : AstNode(loc), m_value(value) {}

  int get_value() const { return m_value; }

private:
  const int m_value;
};

// TODO: DoubleLiteral — same shape as IntLiteral but holds a double

// TODO: StringLiteral — holds a std::string

// TODO: BoolLiteral — holds a bool

// TODO: Identifier — holds a std::string (the name)
//       Hint: structurally identical to StringLiteral, but it means
//       something different — it's a *reference* to a binding, not a value.

// --- Interior nodes (generic) ---------------------------------------------
//
// CallNode and ProgramNode are language-agnostic — most s-expression
// languages would have them.

// TODO: ProgramNode — owns a vector<unique_ptr<AstNode>> (the top-level
//       expressions). This is the root of the tree.
//       Hint: the constructor should take the vector by move.

// TODO: CallNode — owns a callee (unique_ptr<AstNode>) and an argument
//       list (vector<unique_ptr<AstNode>>).
//       Hint: the callee is any expression, not just an identifier.

//****************************************************************************
} // namespace sc
//****************************************************************************
