//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Implementation of top-level item evaluator.
//*
//*
//****************************************************************************

#include <eval/environment.hpp>
#include <eval/expression_evaluator.hpp>
#include <eval/let_binding_evaluator.hpp>
#include <eval/top_item_evaluator.hpp>
#include <utility>

#include <eval/context.hpp>
#include <eval/values.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::eval {
//****************************************************************************

Value TopItemEvaluator::operator()(const hir::FunctionDef &function_def) {
  // We're going to store this as a closure in the env

  auto closure = ExpressionEvaluator{m_ctx}.create_closure(function_def);

  m_ctx.m_env.define(function_def.m_signature.m_function_id,
                     std::move(closure));

  return Unit{};
}

Value TopItemEvaluator::operator()(const hir::ExternFunctionDeclaration &) {
  return Unit{};
}

Value TopItemEvaluator::operator()(const hir::LetBinding &let_binding) {
  return LetBindingEvaluator{m_ctx}(let_binding);
}

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
