//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of expression evaluator.
//*
//*
//****************************************************************************

#include "eval/expression_evaluator.hpp"

#include <variant>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

Value ExpressionEvaluator::operator()(const hir::Expression &expression) {
  return std::visit(*this, expression.m_expression);
}

Value ExpressionEvaluator::operator()(const hir::Identifier &) { return {}; }

Value ExpressionEvaluator::operator()(const hir::LiteralUnit &) { return {}; }

Value ExpressionEvaluator::operator()(const hir::LiteralI64 &) { return {}; }

Value ExpressionEvaluator::operator()(const hir::LiteralBool &) { return {}; }

Value ExpressionEvaluator::operator()(const std::unique_ptr<hir::Block> &) {
  return {};
}

Value ExpressionEvaluator::operator()(const std::unique_ptr<hir::IfExpr> &) {
  return {};
}

Value ExpressionEvaluator::operator()(const std::unique_ptr<hir::CallExpr> &) {
  return {};
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::BinaryExpr> &) {
  return {};
}

Value ExpressionEvaluator::operator()(const std::unique_ptr<hir::UnaryExpr> &) {
  return {};
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::ReturnExpr> &) {
  return {};
}

Value ExpressionEvaluator::operator()(
    const std::unique_ptr<hir::LambdaExpr> &) {
  return {};
}

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
