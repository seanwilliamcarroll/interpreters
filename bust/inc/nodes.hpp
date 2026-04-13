//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared node templates parameterized on expression type.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <operators.hpp>
#include <optional>
#include <vector>

//****************************************************************************
namespace bust {
//****************************************************************************
template <typename ExpressionType> struct CallExprBase {
  ExpressionType m_callee;
  std::vector<ExpressionType> m_arguments;
};

template <typename ExpressionType> struct BinaryExprBase {
  BinaryOperator m_operator;
  ExpressionType m_lhs;
  ExpressionType m_rhs;
};

template <typename ExpressionType> struct UnaryExprBase {
  UnaryOperator m_operator;
  ExpressionType m_expression;
};

template <typename ExpressionType> struct ReturnExprBase {
  ExpressionType m_expression;
};

template <typename ExpressionType, typename NewType> struct CastExprBase {
  ExpressionType m_expression;
  NewType m_new_type;
};

template <typename ExpressionType, typename BlockType> struct IfExprBase {
  ExpressionType m_condition;
  BlockType m_then_block;
  std::optional<BlockType> m_else_block;
};

template <typename IdentifierType, typename BlockType, typename ReturnType>
struct LambdaExprBase {
  std::vector<IdentifierType> m_parameters;
  BlockType m_body;
  ReturnType m_return_type;
};

//****************************************************************************
} // namespace bust
//****************************************************************************
