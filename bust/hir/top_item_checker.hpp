//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item checker — handles FunctionDef and
//*            ExternFunctionDeclaration
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <ast/nodes.hpp>
#include <hir/context.hpp>
#include <hir/nodes.hpp>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct TopItemChecker {
  void collect_function_signature(const ast::FunctionDeclaration &);
  hir::FunctionDeclaration check_declaration(const ast::FunctionDeclaration &);
  TopItem operator()(const ast::FunctionDef &);
  TopItem operator()(const ast::ExternFunctionDeclaration &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
