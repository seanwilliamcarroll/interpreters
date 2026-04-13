//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item generator for bust LLVM IR codegen.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <codegen/context.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct TopItemDeclarationCollector {
  void operator()(const hir::FunctionDef &);
  void operator()(const hir::ExternFunctionDeclaration &);
  void operator()(const hir::LetBinding &);

  Context &m_ctx;
};

struct TopItemGenerator {
  void operator()(const hir::FunctionDef &);
  void operator()(const hir::ExternFunctionDeclaration &);
  void operator()(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
