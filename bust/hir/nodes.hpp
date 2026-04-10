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
struct CastExpr;
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

template <> struct Literal<PrimitiveType::CHAR> : public HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::CHAR;
  char m_value;
};

template <> struct Literal<PrimitiveType::I8> : public HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::I8;
  int8_t m_value;
};

template <> struct Literal<PrimitiveType::I32> : public HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::I32;
  int32_t m_value;
};

template <> struct Literal<PrimitiveType::I64> : public HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::I64;
  int64_t m_value;
};

using LiteralI8 = Literal<PrimitiveType::I8>;
using LiteralI32 = Literal<PrimitiveType::I32>;
using LiteralI64 = Literal<PrimitiveType::I64>;
using LiteralBool = Literal<PrimitiveType::BOOL>;
using LiteralChar = Literal<PrimitiveType::CHAR>;
using LiteralUnit = Literal<PrimitiveType::UNIT>;

// --- Core type aliases -----------------------------------------------------

using ExprKind =
    std::variant<Identifier, LiteralUnit, LiteralI8, LiteralI32, LiteralI64,
                 LiteralBool, LiteralChar, std::unique_ptr<Block>,
                 std::unique_ptr<IfExpr>, std::unique_ptr<CallExpr>,
                 std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>,
                 std::unique_ptr<ReturnExpr>, std::unique_ptr<CastExpr>,
                 std::unique_ptr<LambdaExpr>>;

struct Expression : public HasLocation {
  Type m_type;
  ExprKind m_expression;
};

using Statement = std::variant<Expression, LetBinding>;

using TopItem = std::variant<FunctionDef, LetBinding>;

// --- Expressions -----------------------------------------------------------

struct CallExpr {
  Expression m_callee;
  std::vector<Expression> m_arguments;
};

struct BinaryExpr {
  BinaryOperator m_operator;
  Expression m_lhs;
  Expression m_rhs;
};

struct UnaryExpr {
  UnaryOperator m_operator;
  Expression m_expression;
};

struct ReturnExpr {
  Expression m_expression;
};

struct CastExpr {
  Expression m_expression;
  Type m_new_type;
};

// --- Control flow ----------------------------------------------------------

struct Block : public HasLocation {
  Type m_type;
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

struct IfExpr {
  Expression m_condition;
  Block m_then_branch;
  std::optional<Block> m_else_branch;
};

// --- Bindings & definitions ------------------------------------------------

struct LetBinding : public HasLocation {
  Identifier m_variable;
  Expression m_expression;
};

struct LambdaExpr {
  std::vector<Identifier> m_parameters;
  Block m_body;
};

struct FunctionDef : public HasLocation {
  std::string m_function_id;
  FunctionTypePtr m_type;
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
