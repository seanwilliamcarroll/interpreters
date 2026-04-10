//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression type checker — visitor over ast::ExprKind.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <memory>

#include <ast/nodes.hpp>
#include <hir/context.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct ExpressionChecker {
  Expression check_expression(const ast::Expression &);

  Expression operator()(const ast::Identifier &, const core::SourceLocation &);
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
  Expression operator()(const std::unique_ptr<ast::WhileExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::ForExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const ast::LiteralI8 &, const core::SourceLocation &);
  Expression operator()(const ast::LiteralI32 &, const core::SourceLocation &);
  Expression operator()(const ast::LiteralI64 &, const core::SourceLocation &);
  Expression operator()(const ast::LiteralBool &, const core::SourceLocation &);
  Expression operator()(const ast::LiteralChar &, const core::SourceLocation &);
  Expression operator()(const ast::LiteralUnit &, const core::SourceLocation &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
