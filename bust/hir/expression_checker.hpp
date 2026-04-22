//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression type checker — visitor over ast::ExprKind.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/nodes.hpp>
#include <hir/context.hpp>
#include <hir/nodes.hpp>
#include <source_location.hpp>

#include <memory>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct ExpressionChecker {
  Expression check_expression(const ast::Expression &);

  Expression operator()(const ast::Identifier &, const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::TupleExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::CallExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::BinaryExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::UnaryExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::IfExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::Block> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::CastExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::ReturnExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::LambdaExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::DotExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::WhileExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::ForExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const ast::I8 &, const core::SourceLocation &);
  Expression operator()(const ast::I32 &, const core::SourceLocation &);
  Expression operator()(const ast::I64 &, const core::SourceLocation &);
  Expression operator()(const ast::Bool &, const core::SourceLocation &);
  Expression operator()(const ast::Char &, const core::SourceLocation &);
  Expression operator()(const ast::Unit &, const core::SourceLocation &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
