//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/operators.hpp>
#include <ast/types.hpp>
#include <optional>
#include <vector>

//****************************************************************************
namespace bust::ast {
using core::HasLocation;
//****************************************************************************

// --- Forward declarations --------------------------------------------------

struct FunctionDef;
struct LetBinding;
struct Identifier;
struct CallExpr;
struct BinaryExpr;
struct UnaryExpr;
struct IfExpr;
struct Block;
struct ReturnExpr;
struct LambdaExpr;
struct WhileExpr;
struct ForExpr;

// --- Literals --------------------------------------------------------------

template <typename LiteralType> struct AbstractLiteral : public HasLocation {
  LiteralType m_value;
};

using LiteralInt64 = AbstractLiteral<int64_t>;
using LiteralBool = AbstractLiteral<bool>;
struct LiteralUnit : public HasLocation {};

// --- Core type aliases -----------------------------------------------------

// Recursive variants use unique_ptr to break the cycle.
using Expression =
    std::variant<Identifier, std::unique_ptr<CallExpr>,
                 std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>,
                 std::unique_ptr<IfExpr>, std::unique_ptr<Block>,
                 std::unique_ptr<ReturnExpr>, std::unique_ptr<LambdaExpr>,
                 std::unique_ptr<WhileExpr>, std::unique_ptr<ForExpr>,
                 LiteralInt64, LiteralBool, LiteralUnit>;

using Statement = std::variant<std::unique_ptr<LetBinding>, Expression>;

using TopItem =
    std::variant<std::unique_ptr<FunctionDef>, std::unique_ptr<LetBinding>>;

// --- Leaf nodes ------------------------------------------------------------

struct Identifier : public HasLocation {
  std::string m_name;
  std::optional<TypeIdentifier> m_type;
};

// --- Expressions -----------------------------------------------------------

struct BinaryExpr : public HasLocation {
  BinaryOperator m_operator;
  Expression m_lhs;
  Expression m_rhs;
};

struct UnaryExpr : public HasLocation {
  UnaryOperator m_operator;
  Expression m_expression;
};

struct CallExpr : public HasLocation {
  Expression m_callee;
  std::vector<Expression> m_arguments;
};

struct ReturnExpr : public HasLocation {
  Expression m_return_expression;
};

// --- Control flow ----------------------------------------------------------

struct Block : public HasLocation {
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

struct IfExpr : public HasLocation {
  Expression m_condition;
  Block m_then_block;
  std::optional<Block> m_else_block;
};

// TODO
struct WhileExpr : public HasLocation {};
struct ForExpr : public HasLocation {};

// --- Bindings & definitions ------------------------------------------------

struct LetBinding : public HasLocation {
  Identifier m_variable;
  Expression m_expression;
};

struct FunctionDef : public HasLocation {
  Identifier m_id;
  std::vector<Identifier> m_parameters;
  TypeIdentifier m_return_type;
  Block m_body;
};

struct LambdaExpr : public HasLocation {
  std::vector<Identifier> m_parameters;
  std::optional<TypeIdentifier> m_return_type;
  Block m_body;
};

// --- Program ---------------------------------------------------------------

struct Program : public HasLocation {
  std::vector<TopItem> m_items;
};

//****************************************************************************
} // namespace bust::ast
//****************************************************************************
