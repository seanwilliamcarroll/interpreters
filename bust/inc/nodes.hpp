//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared node templates parameterized on expression type.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hash_combine.hpp>
#include <operators.hpp>

#include <optional>
#include <type_traits>
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

template <typename ExpressionType>
struct hash<bust::BinaryExprBase<ExpressionType>> {
  size_t
  operator()(const bust::BinaryExprBase<ExpressionType> &expr) const noexcept {
    size_t seed = 0;
    using UnderlyingType = std::underlying_type_t<decltype(expr.m_operator)>;
    core::hash_combine(seed, std::hash<UnderlyingType>{}(
                                 static_cast<UnderlyingType>(expr.m_operator)));
    core::hash_combine(seed, std::hash<ExpressionType>{}(expr.m_lhs));
    core::hash_combine(seed, std::hash<ExpressionType>{}(expr.m_rhs));
    return seed;
  }
};

template <typename ExpressionType>
struct hash<bust::UnaryExprBase<ExpressionType>> {
  size_t
  operator()(const bust::UnaryExprBase<ExpressionType> &expr) const noexcept {
    size_t seed = 0;
    using UnderlyingType = std::underlying_type_t<decltype(expr.m_operator)>;
    core::hash_combine(seed, std::hash<UnderlyingType>{}(
                                 static_cast<UnderlyingType>(expr.m_operator)));
    core::hash_combine(seed, std::hash<ExpressionType>{}(expr.m_expression));
    return seed;
  }
};

template <typename ExpressionType>
struct hash<bust::ReturnExprBase<ExpressionType>> {
  size_t
  operator()(const bust::ReturnExprBase<ExpressionType> &expr) const noexcept {
    return std::hash<ExpressionType>{}(expr.m_expression);
  }
};

template <typename ExpressionType, typename NewType>
struct hash<bust::CastExprBase<ExpressionType, NewType>> {
  size_t operator()(
      const bust::CastExprBase<ExpressionType, NewType> &expr) const noexcept {
    size_t seed = 0;
    core::hash_combine(seed, std::hash<ExpressionType>{}(expr.m_expression));
    core::hash_combine(seed, std::hash<NewType>{}(expr.m_new_type));
    return seed;
  }
};

template <typename ExpressionType, typename BlockType>
struct hash<bust::IfExprBase<ExpressionType, BlockType>> {
  size_t operator()(
      const bust::IfExprBase<ExpressionType, BlockType> &expr) const noexcept {
    size_t seed = 0;
    core::hash_combine(seed, std::hash<ExpressionType>{}(expr.m_condition));
    core::hash_combine(seed, std::hash<BlockType>{}(expr.m_then_block));
    if (expr.m_else_block.has_value()) {
      core::hash_combine(seed,
                         std::hash<BlockType>{}(expr.m_else_block.value()));
    }
    return seed;
  }
};

template <typename IdentifierType, typename BlockType, typename ReturnType>
struct hash<bust::LambdaExprBase<IdentifierType, BlockType, ReturnType>> {
  size_t operator()(
      const bust::LambdaExprBase<IdentifierType, BlockType, ReturnType> &expr)
      const noexcept {
    size_t seed = 0;
    for (const auto &parameter : expr.m_parameters) {
      core::hash_combine(seed, std::hash<IdentifierType>{}(parameter));
    }
    core::hash_combine(seed, std::hash<BlockType>{}(expr.m_body));
    core::hash_combine(seed, std::hash<ReturnType>{}(expr.m_return_type));
    return seed;
  }
};

//****************************************************************************
} // namespace std
//****************************************************************************
