//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of statement evaluator.
//*
//*
//****************************************************************************

#include <eval/expression_evaluator.hpp>
#include <eval/let_binding_evaluator.hpp>
#include <eval/statement_evaluator.hpp>

#include <eval/values.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

Value StatementEvaluator::operator()(const hir::Expression &expression) {
  return ExpressionEvaluator{m_ctx}(expression);
}

Value StatementEvaluator::operator()(const hir::LetBinding &let_binding) {
  return LetBindingEvaluator{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
