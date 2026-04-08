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
  Handle operator()(const hir::Identifier &);
  Handle operator()(const hir::LiteralUnit &);
  Handle operator()(const hir::LiteralI64 &);
  Handle operator()(const hir::LiteralBool &);
  Handle operator()(const std::unique_ptr<hir::Block> &);
  Handle operator()(const hir::Block &);
  Handle operator()(const std::unique_ptr<hir::IfExpr> &);
  Handle operator()(const std::unique_ptr<hir::CallExpr> &);

  Handle generate_integer_compare_instruction(
      const std::unique_ptr<hir::BinaryExpr> &);
  Handle generate_arithmetic_binary_instruction(
      const std::unique_ptr<hir::BinaryExpr> &);
  Handle
  generate_logical_binary_instruction(const std::unique_ptr<hir::BinaryExpr> &);
  Handle operator()(const std::unique_ptr<hir::BinaryExpr> &);

  Handle operator()(const std::unique_ptr<hir::UnaryExpr> &);
  Handle operator()(const std::unique_ptr<hir::ReturnExpr> &);
  Handle operator()(const std::unique_ptr<hir::LambdaExpr> &);

  Handle operator()(const hir::Expression &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
