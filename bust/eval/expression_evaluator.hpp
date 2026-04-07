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
  Value operator()(const std::unique_ptr<hir::IfExpr> &);
  Value operator()(const std::unique_ptr<hir::CallExpr> &);
  Value operator()(const std::unique_ptr<hir::BinaryExpr> &);
  Value operator()(const std::unique_ptr<hir::UnaryExpr> &);
  Value operator()(const std::unique_ptr<hir::ReturnExpr> &);
  Value operator()(const std::unique_ptr<hir::LambdaExpr> &);

  Value operator()(const hir::Expression &);

  Context m_ctx;
};

//****************************************************************************
} // namespace bust::eval
//****************************************************************************
