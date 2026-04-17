//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : ZIR expression and program nodes. Arena-backed: all
//*            cross-references are ExprId handles, no unique_ptr trees.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hash_combine.hpp>
#include <nodes.hpp>
#include <zir/types.hpp>

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Binding;

struct Expression;
struct LetBinding;

using InnerExprIdType = size_t;

struct ExprId {
  InnerExprIdType m_id;
  auto operator<=>(const ExprId &) const = default;
};

using InnerBindingIdType = size_t;

struct BindingId {
  InnerBindingIdType m_id;
  auto operator<=>(const BindingId &) const = default;
};

// Literals

struct Unit {
  auto operator<=>(const Unit &) const = default;
};
template <typename InnerType> struct AbstractLiteral {
  InnerType m_value;
  auto operator<=>(const AbstractLiteral<InnerType> &) const = default;
};
using Bool = AbstractLiteral<ToConcrete<BoolType>::concrete_type>;
using Char = AbstractLiteral<ToConcrete<CharType>::concrete_type>;
using I8 = AbstractLiteral<ToConcrete<I8Type>::concrete_type>;
using I32 = AbstractLiteral<ToConcrete<I32Type>::concrete_type>;
using I64 = AbstractLiteral<ToConcrete<I64Type>::concrete_type>;

struct Binding {
  std::string m_name;
  TypeId m_type;
  auto operator<=>(const Binding &) const = default;
};

struct IdentifierExpr {
  BindingId m_id;
  auto operator<=>(const IdentifierExpr &) const = default;
};

struct ExpressionStatement {
  ExprId m_expression;
  auto operator<=>(const ExpressionStatement &) const = default;
};

using Statement = std::variant<ExpressionStatement, LetBinding>;

struct LetBinding {
  BindingId m_identifier;
  ExprId m_expression;
  auto operator<=>(const LetBinding &) const = default;
};

struct Block {
  std::vector<Statement> m_statements;
  std::optional<ExprId> m_final_expression;
  auto operator<=>(const Block &) const = default;
};

using CallExpr = CallExprBase<ExprId>;
using BinaryExpr = BinaryExprBase<ExprId>;
using UnaryExpr = UnaryExprBase<ExprId>;
using ReturnExpr = ReturnExprBase<ExprId>;
using CastExpr = CastExprBase<ExprId, TypeId>;
using IfExpr = IfExprBase<ExprId, Block>;

struct LambdaExpr {
  std::vector<IdentifierExpr> m_parameters;
  Block m_body;
  TypeId m_return_type;
  std::vector<IdentifierExpr> m_captures;
  auto operator<=>(const LambdaExpr &) const = default;
};

using ExprKind = std::variant<Unit, Bool, Char, I8, I32, I64, IdentifierExpr,
                              CallExpr, BinaryExpr, UnaryExpr, ReturnExpr,
                              CastExpr, IfExpr, LambdaExpr, Block>;

struct Expression {
  TypeId m_type_id;
  ExprKind m_expr_kind;
  auto operator<=>(const Expression &) const = default;
};

struct FunctionDef {
  BindingId m_id;
  std::vector<BindingId> m_parameters;
  Block m_body;
};

struct ExternFunctionDeclaration {
  BindingId m_id;
};

using TopItem =
    std::variant<LetBinding, FunctionDef, ExternFunctionDeclaration>;

//****************************************************************************
} // namespace bust::zir
//****************************************************************************

//****************************************************************************
namespace std {
//****************************************************************************

template <> struct hash<bust::zir::ExprId> {
  size_t operator()(const bust::zir::ExprId &id) const noexcept {
    return hash<bust::zir::InnerExprIdType>{}(id.m_id);
  }
};

template <> struct hash<bust::zir::BindingId> {
  size_t operator()(const bust::zir::BindingId &id) const noexcept {
    return hash<bust::zir::InnerBindingIdType>{}(id.m_id);
  }
};

template <> struct hash<bust::zir::Binding> {
  size_t operator()(const bust::zir::Binding &binding) const noexcept {
    size_t seed = 0;
    core::hash_combine(seed, hash<std::string>{}(binding.m_name));
    core::hash_combine(seed, hash<bust::zir::TypeId>{}(binding.m_type));
    return seed;
  }
};

template <> struct hash<bust::zir::IdentifierExpr> {
  size_t operator()(const bust::zir::IdentifierExpr &id) const noexcept {
    return hash<bust::zir::BindingId>{}(id.m_id);
  }
};

template <> struct hash<bust::zir::ExpressionStatement> {
  size_t operator()(
      const bust::zir::ExpressionStatement &expr_statement) const noexcept {
    return hash<bust::zir::ExprId>{}(expr_statement.m_expression);
  }
};

template <> struct hash<bust::zir::Unit> {
  static constexpr size_t HASH_VALUE = 0;
  size_t operator()(const bust::zir::Unit & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::Bool> {
  static constexpr size_t HASH_VALUE = 1;
  size_t operator()(const bust::zir::Bool & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::Char> {
  static constexpr size_t HASH_VALUE = 2;
  size_t operator()(const bust::zir::Char & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::I8> {
  static constexpr size_t HASH_VALUE = 3;
  size_t operator()(const bust::zir::I8 & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::I32> {
  static constexpr size_t HASH_VALUE = 4;
  size_t operator()(const bust::zir::I32 & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::I64> {
  static constexpr size_t HASH_VALUE = 5;
  size_t operator()(const bust::zir::I64 & /*unused*/) const noexcept {
    return hash<size_t>{}(HASH_VALUE);
  }
};

template <> struct hash<bust::zir::Expression> {
  size_t operator()(const bust::zir::Expression &expr) const noexcept {
    size_t seed = 0;
    core::hash_combine(seed, expr.m_type_id);
    core::hash_combine(seed, expr.m_expr_kind);
    return seed;
  }
};

template <> struct hash<bust::zir::LetBinding> {
  size_t operator()(const bust::zir::LetBinding &let_binding) const noexcept {
    size_t seed = 0;
    core::hash_combine(seed, let_binding.m_identifier);
    core::hash_combine(seed, let_binding.m_expression);
    return seed;
  }
};

template <> struct hash<bust::zir::Block> {
  size_t operator()(const bust::zir::Block &block) const noexcept {
    size_t seed = 0;
    for (const auto &statement : block.m_statements) {
      core::hash_combine(seed, statement);
    }
    if (block.m_final_expression.has_value()) {
      core::hash_combine(seed, block.m_final_expression.value());
    }
    return seed;
  }
};

template <> struct hash<bust::zir::LambdaExpr> {
  size_t operator()(const bust::zir::LambdaExpr &expr) const noexcept {
    size_t seed = 0;
    for (const auto &parameter : expr.m_parameters) {
      core::hash_combine(seed,
                         std::hash<bust::zir::IdentifierExpr>{}(parameter));
    }
    core::hash_combine(seed, std::hash<bust::zir::Block>{}(expr.m_body));
    core::hash_combine(seed,
                       std::hash<bust::zir::TypeId>{}(expr.m_return_type));
    for (const auto &capture : expr.m_captures) {
      core::hash_combine(seed, std::hash<bust::zir::IdentifierExpr>{}(capture));
    }
    return seed;
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
