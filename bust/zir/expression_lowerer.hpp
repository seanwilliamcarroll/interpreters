//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression lowerer — HIR expressions to ZIR, allocating
//*            into the expression arena.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <zir/context.hpp>
#include <zir/nodes.hpp>

#include <memory>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct ExpressionLowerer {
  ExprId lower(const hir::Expression &);

  IdentifierExpr lower(const hir::Identifier &);
  IdentifierExpr lower_definition(const hir::Identifier &);
  ExprKind operator()(const hir::Identifier &);
  ExprKind operator()(const hir::Unit &);
  ExprKind operator()(const hir::I8 &);
  ExprKind operator()(const hir::I32 &);
  ExprKind operator()(const hir::I64 &);
  ExprKind operator()(const hir::Bool &);
  ExprKind operator()(const hir::Char &);
  Block lower(const hir::Block &);
  ExprKind operator()(const std::unique_ptr<hir::Block> &);
  ExprKind operator()(const std::unique_ptr<hir::IfExpr> &);
  ExprKind operator()(const std::unique_ptr<hir::CallExpr> &);
  ExprKind operator()(const std::unique_ptr<hir::BinaryExpr> &);
  ExprKind operator()(const std::unique_ptr<hir::UnaryExpr> &);
  ExprKind operator()(const std::unique_ptr<hir::ReturnExpr> &);
  ExprKind operator()(const std::unique_ptr<hir::CastExpr> &);
  ExprKind operator()(const std::unique_ptr<hir::LambdaExpr> &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
