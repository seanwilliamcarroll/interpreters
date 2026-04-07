//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Version : $Header:$
//*
//*
//*  Purpose : Expression generator for bust LLVM IR codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "codegen/context.hpp"
#include "hir/nodes.hpp"

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct ExpressionGenerator {
  Value operator()(const hir::Identifier &);
  Value operator()(const hir::LiteralUnit &);
  Value operator()(const hir::LiteralI64 &);
  Value operator()(const hir::LiteralBool &);
  Value operator()(const std::unique_ptr<hir::Block> &);
  Value operator()(const hir::Block &);
  Value operator()(const std::unique_ptr<hir::IfExpr> &);
  Value operator()(const std::unique_ptr<hir::CallExpr> &);
  Value operator()(const std::unique_ptr<hir::BinaryExpr> &);
  Value operator()(const std::unique_ptr<hir::UnaryExpr> &);
  Value operator()(const std::unique_ptr<hir::ReturnExpr> &);
  Value operator()(const std::unique_ptr<hir::LambdaExpr> &);

  Value operator()(const hir::Expression &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
