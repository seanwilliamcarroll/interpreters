//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Expression type checker — visitor over ast::Expression.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "ast/nodes.hpp"
#include "hir/checker_context.hpp"
#include "hir/nodes.hpp"

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct ExpressionChecker {
  Expression operator()(const ast::Identifier &);
  Expression operator()(const std::unique_ptr<ast::CallExpr> &);
  Expression operator()(const std::unique_ptr<ast::BinaryExpr> &);
  Expression operator()(const std::unique_ptr<ast::UnaryExpr> &);
  Expression operator()(const std::unique_ptr<ast::IfExpr> &);
  Expression operator()(const std::unique_ptr<ast::Block> &);
  Expression operator()(const std::unique_ptr<ast::ReturnExpr> &);
  Expression operator()(const std::unique_ptr<ast::LambdaExpr> &);
  Expression operator()(const std::unique_ptr<ast::WhileExpr> &);
  Expression operator()(const std::unique_ptr<ast::ForExpr> &);
  Expression operator()(const ast::LiteralInt64 &);
  Expression operator()(const ast::LiteralBool &);
  Expression operator()(const ast::LiteralUnit &);

  CheckerContext &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
