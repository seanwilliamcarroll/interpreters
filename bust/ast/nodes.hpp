//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/types.hpp>
#include <operators.hpp>
#include <optional>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::ast {
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
struct CastExpr;
struct ReturnExpr;
struct LambdaExpr;
struct WhileExpr;
struct ForExpr;

// --- Literals --------------------------------------------------------------

template <typename LiteralType>
struct AbstractLiteral : public core::HasLocation {
  LiteralType m_value;
};

using LiteralI8 = AbstractLiteral<int8_t>;
using LiteralI32 = AbstractLiteral<int32_t>;
using LiteralI64 = AbstractLiteral<int64_t>;
using LiteralBool = AbstractLiteral<bool>;
using LiteralChar = AbstractLiteral<char>;
struct LiteralUnit : public core::HasLocation {};

// --- Core type aliases -----------------------------------------------------

// Recursive variants use unique_ptr to break the cycle.
using Expression =
    std::variant<Identifier, std::unique_ptr<CallExpr>,
                 std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>,
                 std::unique_ptr<IfExpr>, std::unique_ptr<Block>,
                 std::unique_ptr<CastExpr>, std::unique_ptr<ReturnExpr>,
                 std::unique_ptr<LambdaExpr>, std::unique_ptr<WhileExpr>,
                 std::unique_ptr<ForExpr>, LiteralI8, LiteralI32, LiteralI64,
                 LiteralBool, LiteralChar, LiteralUnit>;

using Statement = std::variant<LetBinding, Expression>;

using TopItem = std::variant<FunctionDef, LetBinding>;

// --- Leaf nodes ------------------------------------------------------------

struct Identifier : public core::HasLocation {
  std::string m_name;
  std::optional<TypeIdentifier> m_type;
};

// --- Expressions -----------------------------------------------------------

struct BinaryExpr : public core::HasLocation {
  BinaryOperator m_operator;
  Expression m_lhs;
  Expression m_rhs;
};

struct UnaryExpr : public core::HasLocation {
  UnaryOperator m_operator;
  Expression m_expression;
};

struct CallExpr : public core::HasLocation {
  Expression m_callee;
  std::vector<Expression> m_arguments;
};

struct CastExpr : public core::HasLocation {
  Expression m_expression;
  TypeIdentifier m_type;
};

struct ReturnExpr : public core::HasLocation {
  Expression m_return_expression;
};

// --- Control flow ----------------------------------------------------------

struct Block : public core::HasLocation {
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

struct IfExpr : public core::HasLocation {
  Expression m_condition;
  Block m_then_block;
  std::optional<Block> m_else_block;
};

// TODO
struct WhileExpr : public core::HasLocation {};
struct ForExpr : public core::HasLocation {};

// --- Bindings & definitions ------------------------------------------------

struct LetBinding : public core::HasLocation {
  Identifier m_variable;
  Expression m_expression;
};

struct FunctionDef : public core::HasLocation {
  Identifier m_id;
  std::vector<Identifier> m_parameters;
  TypeIdentifier m_return_type;
  Block m_body;
};

struct LambdaExpr : public core::HasLocation {
  std::vector<Identifier> m_parameters;
  std::optional<TypeIdentifier> m_return_type;
  Block m_body;
};

// --- Program ---------------------------------------------------------------

struct Program : public core::HasLocation {
  std::vector<TopItem> m_items;
};

//****************************************************************************
} // namespace bust::ast
//****************************************************************************
