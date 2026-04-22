//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Pre-walk the body of a polymorphic let binding and register
//*            unification edges for every DotExpr so the monomorpher can
//*            compute a concrete type for the outer binding before mangling.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <hir/types.hpp>
#include <mono/context.hpp>

#include <memory>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct DotExprResolver {

  void resolve(const hir::Expression &);
  void resolve(const hir::Block &);

  hir::TypeId resolve_to_type(const std::unique_ptr<hir::DotExpr> &);

  void operator()(const hir::Identifier &);
  void operator()(const std::unique_ptr<hir::TupleExpr> &);
  void operator()(const hir::Unit &);
  void operator()(const hir::I8 &);
  void operator()(const hir::I32 &);
  void operator()(const hir::I64 &);
  void operator()(const hir::Bool &);
  void operator()(const hir::Char &);
  void operator()(const std::unique_ptr<hir::Block> &);
  void operator()(const std::unique_ptr<hir::IfExpr> &);
  void operator()(const std::unique_ptr<hir::CallExpr> &);
  void operator()(const std::unique_ptr<hir::BinaryExpr> &);
  void operator()(const std::unique_ptr<hir::UnaryExpr> &);
  void operator()(const std::unique_ptr<hir::ReturnExpr> &);
  void operator()(const std::unique_ptr<hir::CastExpr> &);
  void operator()(const std::unique_ptr<hir::LambdaExpr> &);
  void operator()(const std::unique_ptr<hir::DotExpr> &);

  SubstitutionContext &m_ctx;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
