//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Visitor that clones an HIR expression while substituting
//*            type variables according to a TypeSubstitution.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <mono/context.hpp>

#include <memory>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct ExpressionSubstituter {

  hir::Expression substitute(const hir::Expression &);

  hir::Identifier substitute(const hir::Identifier &);
  hir::ExprKind operator()(const hir::Identifier &);
  hir::ExprKind operator()(const hir::LiteralUnit &);
  hir::ExprKind operator()(const hir::LiteralI8 &);
  hir::ExprKind operator()(const hir::LiteralI32 &);
  hir::ExprKind operator()(const hir::LiteralI64 &);
  hir::ExprKind operator()(const hir::LiteralBool &);
  hir::ExprKind operator()(const hir::LiteralChar &);
  hir::Block substitute(const hir::Block &);
  hir::ExprKind operator()(const std::unique_ptr<hir::Block> &);
  hir::ExprKind operator()(const std::unique_ptr<hir::IfExpr> &);
  hir::ExprKind operator()(const std::unique_ptr<hir::CallExpr> &);
  hir::ExprKind operator()(const std::unique_ptr<hir::BinaryExpr> &);
  hir::ExprKind operator()(const std::unique_ptr<hir::UnaryExpr> &);
  hir::ExprKind operator()(const std::unique_ptr<hir::ReturnExpr> &);
  hir::ExprKind operator()(const std::unique_ptr<hir::CastExpr> &);
  hir::ExprKind operator()(const std::unique_ptr<hir::LambdaExpr> &);

  SubstitutionContext &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
