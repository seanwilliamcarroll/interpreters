//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of top-level item evaluator.
//*
//*
//****************************************************************************

#include "eval/top_item_evaluator.hpp"
#include "eval/expression_evaluator.hpp"
#include "eval/let_binding_evaluator.hpp"
#include <variant>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

Value TopItemEvaluator::operator()(const hir::FunctionDef & // function_def
) {
  return {};
}

Value TopItemEvaluator::operator()(const hir::LetBinding &let_binding) {
  return LetBindingEvaluator{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
