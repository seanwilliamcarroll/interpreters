//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Typed AST node definitions for bust.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/instantiation_record.hpp>
#include <hir/type_arena.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>
#include <nodes.hpp>
#include <operators.hpp>
#include <source_location.hpp>

#include <optional>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

// --- Forward declarations --------------------------------------------------

struct Expression;
struct FunctionDef;
struct ExternFunctionDeclaration;
struct Block;
struct DotExpr;
struct WhileExpr;
struct LoopExpr;
struct ContinueExpr;
// TODO
struct ForExpr {};

// --- Leaf nodes ------------------------------------------------------------

struct Identifier {
  core::SourceLocation m_location;
  std::string m_name;
  BindingId m_id;
  TypeId m_type;
};

struct Parameter {
  core::SourceLocation m_location;
  Identifier m_id;
  bool m_is_mutable;
};

// --- Literals --------------------------------------------------------------

template <PrimitiveType InternalType> struct Literal {
  core::SourceLocation m_location;
  static constexpr PrimitiveType m_type = InternalType;
};

template <> struct Literal<PrimitiveType::BOOL> {
  core::SourceLocation m_location;
  static constexpr PrimitiveType m_type = PrimitiveType::BOOL;
  bool m_value;
};

template <> struct Literal<PrimitiveType::CHAR> {
  core::SourceLocation m_location;
  static constexpr PrimitiveType m_type = PrimitiveType::CHAR;
  char m_value;
};

template <> struct Literal<PrimitiveType::I8> {
  core::SourceLocation m_location;
  static constexpr PrimitiveType m_type = PrimitiveType::I8;
  int8_t m_value;
};

template <> struct Literal<PrimitiveType::I32> {
  core::SourceLocation m_location;
  static constexpr PrimitiveType m_type = PrimitiveType::I32;
  int32_t m_value;
};

template <> struct Literal<PrimitiveType::I64> {
  core::SourceLocation m_location;
  static constexpr PrimitiveType m_type = PrimitiveType::I64;
  int64_t m_value;
};

using I8 = Literal<PrimitiveType::I8>;
using I32 = Literal<PrimitiveType::I32>;
using I64 = Literal<PrimitiveType::I64>;
using Bool = Literal<PrimitiveType::BOOL>;
using Char = Literal<PrimitiveType::CHAR>;
using Unit = Literal<PrimitiveType::UNIT>;

// --- Core type aliases -----------------------------------------------------

using CallExpr = CallExprBase<Expression>;
using BinaryExpr = BinaryExprBase<Expression>;
using UnaryExpr = UnaryExprBase<Expression>;
using ReturnExpr = ReturnExprBase<Expression>;
using CastExpr = CastExprBase<Expression, TypeId>;
using IfExpr = IfExprBase<Expression, Block>;
using LambdaExpr = LambdaExprBase<Parameter, Block, TypeId>;
using TupleExpr = TupleExprBase<Expression>;
using BreakExpr = BreakExprBase<Expression>;

using ExprKind =
    std::variant<Identifier, Unit, I8, I32, I64, Bool, Char,
                 std::unique_ptr<Block>, std::unique_ptr<IfExpr>,
                 std::unique_ptr<CallExpr>, std::unique_ptr<BinaryExpr>,
                 std::unique_ptr<UnaryExpr>, std::unique_ptr<ReturnExpr>,
                 std::unique_ptr<CastExpr>, std::unique_ptr<LambdaExpr>,
                 std::unique_ptr<TupleExpr>, std::unique_ptr<DotExpr>,
                 std::unique_ptr<WhileExpr>, std::unique_ptr<LoopExpr>,
                 std::unique_ptr<BreakExpr>, std::unique_ptr<ContinueExpr>>;

struct Expression {
  core::SourceLocation m_location;
  TypeId m_type;
  ExprKind m_expression;
};

struct DotExpr {
  Expression m_expression;
  size_t m_tuple_index;
};

struct LetBinding {
  core::SourceLocation m_location;
  Identifier m_variable;
  Expression m_expression;
  bool m_is_mutable = false;
};

using Place = std::variant<Identifier>;

struct Assignment {
  core::SourceLocation m_location;
  Place m_place;
  Expression m_expression;
};

using Statement = std::variant<Expression, LetBinding, Assignment>;

using TopItem = std::variant<FunctionDef, ExternFunctionDeclaration>;

// --- Control flow ----------------------------------------------------------

struct Block {
  core::SourceLocation m_location;
  TypeId m_type;
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

struct ContinueExpr {};

// --- Bindings & definitions ------------------------------------------------

struct FunctionDeclaration {
  std::string m_function_id;
  BindingId m_id;
  TypeId m_type;
  std::vector<Parameter> m_parameters;
};

struct ExternFunctionDeclaration {
  core::SourceLocation m_location;
  FunctionDeclaration m_signature;
};

struct FunctionDef {
  core::SourceLocation m_location;
  FunctionDeclaration m_signature;
  Block m_body;
};

// --- Program ---------------------------------------------------------------

struct Program {
  core::SourceLocation m_location;
  TypeArena m_type_arena;
  std::vector<TopItem> m_top_items;
  UnifierState m_unifier_state;
  BindingIdInstantiations m_instantiation_records;
  InnerTypeBindingId m_next_let_binding_id{};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
