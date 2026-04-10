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

#include <memory>

#include "ast/nodes.hpp"
#include "hir/context.hpp"
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
  Expression operator()(const std::unique_ptr<ast::CastExpr> &);
  Expression operator()(const std::unique_ptr<ast::ReturnExpr> &);
  Expression operator()(const std::unique_ptr<ast::LambdaExpr> &);
  Expression operator()(const std::unique_ptr<ast::WhileExpr> &);
  Expression operator()(const std::unique_ptr<ast::ForExpr> &);
  Expression operator()(const ast::LiteralI8 &);
  Expression operator()(const ast::LiteralI32 &);
  Expression operator()(const ast::LiteralI64 &);
  Expression operator()(const ast::LiteralBool &);
  Expression operator()(const ast::LiteralChar &);
  Expression operator()(const ast::LiteralUnit &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
