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

#include <cstddef>
#include <nodes.hpp>
#include <variant>
#include <zir/types.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct Binding;

struct Expression;

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
};

struct Block {};

using CallExpr = CallExprBase<ExprId>;
using BinaryExpr = BinaryExprBase<ExprId>;
using UnaryExpr = UnaryExprBase<ExprId>;
using ReturnExpr = ReturnExprBase<ExprId>;
using CastExpr = CastExprBase<ExprId, TypeId>;
using IfExpr = IfExprBase<ExprId, ExprId>;
using LambdaExpr = LambdaExprBase<ExprId, ExprId, TypeId>;

using ExprKind =
    std::variant<Unit, Bool, Char, I8, I32,
                 I64 //, CallExpr// , BinaryExpr,
                     // UnaryExpr, ReturnExpr, CastExpr, IfExpr, LambdaExpr
                 >;

struct Expression {
  TypeId m_type_id;
  ExprId m_expr_id;
  auto operator<=>(const Expression &) const = default;
};

struct LetBinding {
  ExprId m_identifier;
  ExprId m_expression;
};

// Program, ExprArena, ExprId, ExprKind variants — to be designed.
struct Program {};

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

template <> struct hash<bust::zir::Unit> {
  size_t operator()(const bust::zir::Unit &) const noexcept {
    return hash<size_t>{}(0);
  }
};

template <> struct hash<bust::zir::Bool> {
  size_t operator()(const bust::zir::Bool &) const noexcept {
    return hash<size_t>{}(1);
  }
};

template <> struct hash<bust::zir::Char> {
  size_t operator()(const bust::zir::Char &) const noexcept {
    return hash<size_t>{}(2);
  }
};

template <> struct hash<bust::zir::I8> {
  size_t operator()(const bust::zir::I8 &) const noexcept {
    return hash<size_t>{}(3);
  }
};

template <> struct hash<bust::zir::I32> {
  size_t operator()(const bust::zir::I32 &) const noexcept {
    return hash<size_t>{}(4);
  }
};

template <> struct hash<bust::zir::I64> {
  size_t operator()(const bust::zir::I64 &) const noexcept {
    return hash<size_t>{}(5);
  }
};

template <> struct hash<bust::zir::Expression> {
  size_t operator()(const bust::zir::Expression &expr) const noexcept {
    size_t seed = 0;
    core::hash_combine(seed, expr.m_type_id);
    core::hash_combine(seed, expr.m_expr_id);
    return seed;
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
