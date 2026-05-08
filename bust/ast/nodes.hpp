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
#include <nodes.hpp>
#include <operators.hpp>

#include <optional>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::ast {
//****************************************************************************

// --- Forward declarations --------------------------------------------------

struct Expression;
struct Assignment;
struct Parameter;
struct FunctionDef;
struct ExternFunctionDeclaration;
struct LetBinding;
struct Identifier;
struct Block;
struct DotExpr;
struct TupleExpr;
struct WhileExpr;
struct LoopExpr;
struct ForExpr;
struct BreakExpr;
struct ContinueExpr;

// --- Literals --------------------------------------------------------------

template <typename LiteralType> struct AbstractLiteral {
  LiteralType m_value;
};

using I8 = AbstractLiteral<int8_t>;
using I32 = AbstractLiteral<int32_t>;
using I64 = AbstractLiteral<int64_t>;
using Bool = AbstractLiteral<bool>;
using Char = AbstractLiteral<char>;
struct Unit {};

// --- Core type aliases -----------------------------------------------------

using CallExpr = CallExprBase<Expression>;
using BinaryExpr = BinaryExprBase<Expression>;
using UnaryExpr = UnaryExprBase<Expression>;
using ReturnExpr = ReturnExprBase<Expression>;
using CastExpr = CastExprBase<Expression, TypeIdentifier>;
using IfExpr = IfExprBase<Expression, Block>;
using LambdaExpr =
    LambdaExprBase<Parameter, Block, std::optional<TypeIdentifier>>;

// Recursive variants use unique_ptr to break the cycle.
using ExprKind =
    std::variant<Identifier, std::unique_ptr<CallExpr>,
                 std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>,
                 std::unique_ptr<IfExpr>, std::unique_ptr<Block>,
                 std::unique_ptr<CastExpr>, std::unique_ptr<ReturnExpr>,
                 std::unique_ptr<BreakExpr>, std::unique_ptr<ContinueExpr>,
                 std::unique_ptr<LambdaExpr>, std::unique_ptr<WhileExpr>,
                 std::unique_ptr<LoopExpr>, std::unique_ptr<ForExpr>,
                 std::unique_ptr<TupleExpr>, std::unique_ptr<DotExpr>, I8, I32,
                 I64, Bool, Char, Unit>;

using Statement = std::variant<LetBinding, Assignment, Expression>;

using TopItem = std::variant<FunctionDef, ExternFunctionDeclaration>;

// --- Leaf nodes ------------------------------------------------------------

struct Identifier : public core::HasLocation {
  std::string m_name;
  std::optional<TypeIdentifier> m_type;
};

struct Parameter : public core::HasLocation {
  Identifier m_id;
  bool m_is_mutable = false;
};

// --- Expressions -----------------------------------------------------------

struct Expression : public core::HasLocation {
  ExprKind m_expression;
};

struct TupleExpr {
  std::vector<Expression> m_fields;
};

struct BreakExpr {
  std::optional<Expression> m_returned_value;
};

struct ContinueExpr {};

struct DotExpr {
  Expression m_expression;
  // For now, all that is representable
  size_t m_tuple_index;
};

// --- Control flow ----------------------------------------------------------

struct Block : public core::HasLocation {
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

struct WhileExpr {
  Expression m_condition;
  Block m_body;
};

struct LoopExpr {
  Block m_body;
};

// TODO
struct ForExpr {};

// --- Bindings & definitions ------------------------------------------------

struct Assignment : public core::HasLocation {
  Expression m_lhs;
  Expression m_rhs;
};

struct LetBinding : public core::HasLocation {
  Identifier m_variable;
  Expression m_expression;
  bool m_is_mutable = false;
};

struct FunctionDeclaration {
  Identifier m_id;
  std::vector<Parameter> m_parameters;
  TypeIdentifier m_return_type;
};

struct FunctionDef : public core::HasLocation {
  FunctionDeclaration m_signature;
  Block m_body;
};

struct ExternFunctionDeclaration : public core::HasLocation {
  FunctionDeclaration m_signature;
};

// --- Program ---------------------------------------------------------------

struct Program : public core::HasLocation {
  std::vector<TopItem> m_items;
};

//****************************************************************************
} // namespace bust::ast
//****************************************************************************
