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
  hir::ExprKind operator()(const hir::Unit &);
  hir::ExprKind operator()(const hir::I8 &);
  hir::ExprKind operator()(const hir::I32 &);
  hir::ExprKind operator()(const hir::I64 &);
  hir::ExprKind operator()(const hir::Bool &);
  hir::ExprKind operator()(const hir::Char &);
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
