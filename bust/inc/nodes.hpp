//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared node templates parameterized on expression type.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hash_combine.hpp"
#include <operators.hpp>
#include <optional>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************
template <typename ExpressionType> struct CallExprBase {
  ExpressionType m_callee;
  std::vector<ExpressionType> m_arguments;
  auto operator<=>(const CallExprBase<ExpressionType> &) const = default;
};

template <typename ExpressionType> struct BinaryExprBase {
  BinaryOperator m_operator;
  ExpressionType m_lhs;
  ExpressionType m_rhs;
  auto operator<=>(const BinaryExprBase<ExpressionType> &) const = default;
};

template <typename ExpressionType> struct UnaryExprBase {
  UnaryOperator m_operator;
  ExpressionType m_expression;
  auto operator<=>(const UnaryExprBase<ExpressionType> &) const = default;
};

template <typename ExpressionType> struct ReturnExprBase {
  ExpressionType m_expression;
  auto operator<=>(const ReturnExprBase<ExpressionType> &) const = default;
};

template <typename ExpressionType, typename NewType> struct CastExprBase {
  ExpressionType m_expression;
  NewType m_new_type;
  auto
  operator<=>(const CastExprBase<ExpressionType, NewType> &) const = default;
};

template <typename ExpressionType, typename BlockType> struct IfExprBase {
  ExpressionType m_condition;
  BlockType m_then_block;
  std::optional<BlockType> m_else_block;
  auto
  operator<=>(const IfExprBase<ExpressionType, BlockType> &) const = default;
};

template <typename IdentifierType, typename BlockType, typename ReturnType>
struct LambdaExprBase {
  std::vector<IdentifierType> m_parameters;
  BlockType m_body;
  ReturnType m_return_type;
  auto operator<=>(const LambdaExprBase<IdentifierType, BlockType, ReturnType>
                       &) const = default;
};

//****************************************************************************
} // namespace bust
//****************************************************************************

//****************************************************************************
namespace std {
//****************************************************************************

template <typename ExpressionType>
struct hash<bust::CallExprBase<ExpressionType>> {
  size_t
  operator()(const bust::CallExprBase<ExpressionType> &expr) const noexcept {
    size_t seed = 0;
    core::hash_combine(seed, std::hash<ExpressionType>{}(expr.m_callee));
    for (const auto &argument : expr.m_arguments) {
      core::hash_combine(seed, std::hash<ExpressionType>{}(argument));
    }
    return seed;
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
