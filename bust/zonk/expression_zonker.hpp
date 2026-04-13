//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Expression zonker — resolves all type variables in
//*            HIR expressions to their concrete types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <memory>
#include <zonk/context.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

struct ExpressionZonker {
  hir::Expression zonk(hir::Expression);

  hir::ExprKind zonk(hir::Block);

  hir::Identifier zonk(hir::Identifier);
  hir::ExprKind operator()(hir::Identifier);
  hir::ExprKind operator()(hir::LiteralUnit);
  hir::ExprKind operator()(hir::LiteralI8);
  hir::ExprKind operator()(hir::LiteralI32);
  hir::ExprKind operator()(hir::LiteralI64);
  hir::ExprKind operator()(hir::LiteralBool);
  hir::ExprKind operator()(hir::LiteralChar);
  hir::ExprKind operator()(std::unique_ptr<hir::Block>);
  hir::Block zonk_block(hir::Block);
  hir::ExprKind operator()(std::unique_ptr<hir::IfExpr>);
  hir::ExprKind operator()(std::unique_ptr<hir::CallExpr>);
  hir::ExprKind operator()(std::unique_ptr<hir::BinaryExpr>);
  hir::ExprKind operator()(std::unique_ptr<hir::UnaryExpr>);
  hir::ExprKind operator()(std::unique_ptr<hir::ReturnExpr>);
  hir::ExprKind operator()(std::unique_ptr<hir::CastExpr>);
  hir::ExprKind operator()(std::unique_ptr<hir::LambdaExpr>);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
