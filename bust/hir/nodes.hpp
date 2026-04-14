//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Typed AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "ast/nodes.hpp"
#include "hir/instantiation_record.hpp"
#include "hir/type_registry.hpp"
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>
#include <nodes.hpp>
#include <operators.hpp>
#include <optional>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

// --- Forward declarations --------------------------------------------------

struct Expression;
struct FunctionDef;
struct ExternFunctionDeclaration;
struct LetBinding;
struct Block;
// TODO
struct WhileExpr {};
struct ForExpr {};

// --- Leaf nodes ------------------------------------------------------------

struct Identifier : public core::HasLocation {
  std::string m_name;
  BindingId m_id;
  TypeId m_type;
};

// --- Literals --------------------------------------------------------------

template <PrimitiveType InternalType>
struct Literal : public core::HasLocation {
  static constexpr PrimitiveType m_type = InternalType;
};

template <> struct Literal<PrimitiveType::BOOL> : public core::HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::BOOL;
  bool m_value;
};

template <> struct Literal<PrimitiveType::CHAR> : public core::HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::CHAR;
  char m_value;
};

template <> struct Literal<PrimitiveType::I8> : public core::HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::I8;
  int8_t m_value;
};

template <> struct Literal<PrimitiveType::I32> : public core::HasLocation {
  static constexpr PrimitiveType m_type = PrimitiveType::I32;
  int32_t m_value;
};

template <> struct Literal<PrimitiveType::I64> : public core::HasLocation {
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

using CallExpr = CallExprBase<Expression>;
using BinaryExpr = BinaryExprBase<Expression>;
using UnaryExpr = UnaryExprBase<Expression>;
using ReturnExpr = ReturnExprBase<Expression>;
using CastExpr = CastExprBase<Expression, TypeId>;
using IfExpr = IfExprBase<Expression, Block>;
using LambdaExpr = LambdaExprBase<Identifier, Block, TypeId>;

using ExprKind =
    std::variant<Identifier, LiteralUnit, LiteralI8, LiteralI32, LiteralI64,
                 LiteralBool, LiteralChar, std::unique_ptr<Block>,
                 std::unique_ptr<IfExpr>, std::unique_ptr<CallExpr>,
                 std::unique_ptr<BinaryExpr>, std::unique_ptr<UnaryExpr>,
                 std::unique_ptr<ReturnExpr>, std::unique_ptr<CastExpr>,
                 std::unique_ptr<LambdaExpr>>;

struct Expression : public core::HasLocation {
  TypeId m_type;
  ExprKind m_expression;
};

using Statement = std::variant<Expression, LetBinding>;

using TopItem =
    std::variant<FunctionDef, ExternFunctionDeclaration, LetBinding>;

// --- Control flow ----------------------------------------------------------

struct Block : public core::HasLocation {
  TypeId m_type;
  std::vector<Statement> m_statements;
  std::optional<Expression> m_final_expression;
};

// --- Bindings & definitions ------------------------------------------------

struct LetBinding : public core::HasLocation {
  Identifier m_variable;
  Expression m_expression;
};

struct FunctionDeclaration {
  std::string m_function_id;
  BindingId m_id;
  TypeId m_type;
  std::vector<Identifier> m_parameters;
};

struct ExternFunctionDeclaration : public core::HasLocation {
  FunctionDeclaration m_signature;
};

struct FunctionDef : public core::HasLocation {
  FunctionDeclaration m_signature;
  Block m_body;
};

// --- Program ---------------------------------------------------------------

struct Program : public core::HasLocation {
  TypeRegistry m_type_registry{};
  std::vector<TopItem> m_top_items{};
  std::optional<UnifierState> m_unifier_state{};
  std::vector<InstantiationRecord> m_instantiation_records{};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
