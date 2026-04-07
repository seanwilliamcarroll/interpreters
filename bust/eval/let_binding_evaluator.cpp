//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Implementation of let-binding evaluator.
//*
//*
//****************************************************************************

#include "eval/let_binding_evaluator.hpp"
#include "eval/expression_evaluator.hpp"
#include "eval/values.hpp"

//****************************************************************************
namespace bust::eval {
//****************************************************************************

Value LetBindingEvaluator::operator()(const hir::LetBinding &let_binding) {

  auto value = ExpressionEvaluator{m_ctx}(let_binding.m_expression);

  m_ctx.m_env.define(let_binding.m_variable.m_name, value);

  return Unit{};
}

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
