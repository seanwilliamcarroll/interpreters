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
#include <codegen/function_declaration.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::codegen {
//****************************************************************************

struct TopItemDeclarationCollector {
  void collect(const zir::TopItem &);

  void operator()(const zir::FunctionDef &);
  void operator()(const zir::ExternFunctionDeclaration &);
  void operator()(const zir::LetBinding &);

  Context &m_ctx;
};

struct TopItemGenerator {
  void generate(const zir::TopItem &);

  FunctionDeclaration generate_signature(const zir::FunctionDef &);
  [[nodiscard]] FunctionDeclaration
  generate_signature(const zir::ExternFunctionDeclaration &) const;

  void operator()(const zir::FunctionDef &);
  void operator()(const zir::ExternFunctionDeclaration &);
  void operator()(const zir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::codegen
//****************************************************************************
