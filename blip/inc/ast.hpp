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

#include <sc/ast.hpp>

//****************************************************************************
namespace blip {
//****************************************************************************

using sc::AstNode;
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

private:
  std::unique_ptr<AstNode> m_condition;
  std::unique_ptr<AstNode> m_then_branch;
  std::unique_ptr<AstNode> m_else_branch;
};

// TODO: WhileNode — condition + body (both unique_ptr<AstNode>)

// TODO: SetNode — name (std::string) + value (unique_ptr<AstNode>)

// TODO: BeginNode — owns a vector<unique_ptr<AstNode>> (one or more exprs)

// TODO: PrintNode — owns one expression (unique_ptr<AstNode>)

// TODO: DefineVarNode — name (std::string) + value (unique_ptr<AstNode>)

// TODO: DefineFnNode — name (std::string) + parameter names
//       (vector<std::string>) + body (unique_ptr<AstNode>)
//       Hint: parameters are just names at definition time, not expressions.

//****************************************************************************
} // namespace blip
//****************************************************************************
