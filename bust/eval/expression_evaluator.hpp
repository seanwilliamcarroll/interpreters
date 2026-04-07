//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Expression evaluator for bust tree-walking evaluator.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>

#include "eval/context.hpp"
#include "eval/values.hpp"
#include "hir/nodes.hpp"

//****************************************************************************
namespace bust::eval {
//****************************************************************************

struct ExpressionEvaluator {
  Value operator()(const hir::Identifier &);
  Value operator()(const hir::LiteralUnit &);
  Value operator()(const hir::LiteralI64 &);
  Value operator()(const hir::LiteralBool &);
  Value operator()(const std::unique_ptr<hir::Block> &);
  Value operator()(const hir::Block &);
  Value operator()(const std::unique_ptr<hir::IfExpr> &);
  Value operator()(const std::unique_ptr<hir::CallExpr> &);
  Value operator()(const std::unique_ptr<hir::BinaryExpr> &);
  Value operator()(const std::unique_ptr<hir::UnaryExpr> &);
  Value operator()(const std::unique_ptr<hir::ReturnExpr> &);
  Value operator()(const std::unique_ptr<hir::LambdaExpr> &);

  Value operator()(const hir::Expression &);

  Closure create_closure(const auto &function_object) {

    auto current_scope = m_ctx.m_env.get_current_scope();

    std::vector<std::string> parameters;
    parameters.reserve(function_object.m_parameters.size());
    std::transform(
        function_object.m_parameters.begin(),
        function_object.m_parameters.end(), std::back_inserter(parameters),
        [](const hir::Identifier &identifier) { return identifier.m_name; });

    const auto *function_body = &function_object.m_body;

    return Closure{std::move(parameters), function_body,
                   std::move(current_scope)};
  }

  Context m_ctx;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
