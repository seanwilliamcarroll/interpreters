//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item zonker — resolves all type variables in
//*            HIR top-level items to their concrete types.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <zonk/context.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

struct TopItemZonker {

  hir::TopItem zonk(hir::TopItem);
  hir::FunctionDeclaration zonk(hir::FunctionDeclaration);

  hir::TopItem operator()(hir::FunctionDef);
  hir::TopItem operator()(hir::ExternFunctionDeclaration);
  hir::TopItem operator()(hir::LetBinding);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
