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
  Expression operator()(const std::unique_ptr<ast::BreakExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::ContinueExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::LambdaExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::DotExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::WhileExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::LoopExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const std::unique_ptr<ast::ForExpr> &,
                        const core::SourceLocation &);
  Expression operator()(const ast::I8 &, const core::SourceLocation &);
  Expression operator()(const ast::I32 &, const core::SourceLocation &);
  Expression operator()(const ast::I64 &, const core::SourceLocation &);
  Expression operator()(const ast::Bool &, const core::SourceLocation &);
  Expression operator()(const ast::Char &, const core::SourceLocation &);
  Expression operator()(const ast::Unit &, const core::SourceLocation &);

  void try_unify(const auto &type_a, const auto &type_b,
                 const core::SourceLocation &location,
                 const std::string &additional_message = "") {
    try {
      m_ctx.type_unifier().unify(type_a, type_b);
    } catch (std::runtime_error &error) {
      throw core::CompilerException("TypeChecker",
                                    "Type unification error!\n" +
                                        additional_message + error.what(),
                                    location);
    }
  }

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
