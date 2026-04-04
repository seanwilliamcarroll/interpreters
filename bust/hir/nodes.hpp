//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Typed AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/types.hpp>
#include <operators.hpp>
#include <optional>
#include <vector>

//****************************************************************************
namespace bust::hir {
using bust::BinaryOperator;
using bust::UnaryOperator;
using core::HasLocation;
//****************************************************************************

// --- Forward declarations --------------------------------------------------

struct FunctionDef;
struct LetBinding;
struct Block;
struct IfExpr;
struct CallExpr;
struct BinaryExpr;
struct UnaryExpr;
struct ReturnExpr;
struct LambdaExpr;
// TODO
struct WhileExpr {};
struct ForExpr {};

// --- Leaf nodes ------------------------------------------------------------

struct Identifier : public HasLocation {
  std::string m_name;
  Type m_type;
};

// --- Literals --------------------------------------------------------------

template <PrimitiveType InternalType> struct Literal : public HasLocation {
  static constexpr PrimitiveType m_type = InternalType;
};

template <> struct Literal<PrimitiveType::BOOL> : public HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::BOOL;
  bool m_value;
};

template <> struct Literal<PrimitiveType::I64> : public HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::I64;
  int64_t m_value;
};

using LiteralI64 = Literal<PrimitiveType::I64>;
using LiteralBool = Literal<PrimitiveType::BOOL>;
using LiteralUnit = Literal<PrimitiveType::UNIT>;

// --- Core type aliases -----------------------------------------------------

using ExprKind =
    std::variant<Identifier, LiteralUnit, LiteralI64, LiteralBool,
                 std::unique_ptr<Block>, std::unique_ptr<IfExpr>,
                 std::unique_ptr<CallExpr>, std::unique_ptr<BinaryExpr>,
                 std::unique_ptr<UnaryExpr>, std::unique_ptr<ReturnExpr>,
                 std::unique_ptr<LambdaExpr>>;

struct Expression : public HasLocation {
  Type m_type;
  ExprKind m_expression;
};

using Statement = std::variant<Expression, LetBinding>;

using TopItem = std::variant<FunctionDef, LetBinding>;

// --- Expressions -----------------------------------------------------------

struct CallExpr : public HasLocation {
  Expression m_callee;
  std::vector<Expression> m_arguments;
};

struct BinaryExpr : public HasLocation {
  BinaryOperator m_operator;
  Expression m_lhs;
  Expression m_rhs;
};

struct UnaryExpr : public HasLocation {
  UnaryOperator m_operator;
  Expression m_expression;
};

struct ReturnExpr : public HasLocation {
  Expression m_expression;
};

// --- Control flow ----------------------------------------------------------

struct Block : public HasLocation {
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

struct IfExpr : public HasLocation {
  Expression m_condition;
  Block m_then_branch;
  std::optional<Block> m_else_branch;
};

// --- Bindings & definitions ------------------------------------------------

struct LetBinding : public HasLocation {
  Identifier m_variable;
  Expression m_expression;
};

struct LambdaExpr : public HasLocation {
  std::vector<Identifier> m_parameters;
  Block m_body;
};

struct FunctionDef : public HasLocation {
  std::string m_function_id;
  FunctionType m_type;
  std::vector<Identifier> m_parameters;
  Block m_body;
};

// --- Program ---------------------------------------------------------------

struct Program : public HasLocation {
  std::vector<TopItem> m_top_items;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
