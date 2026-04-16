//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Top-level item lowerer — HIR top items to ZIR.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/nodes.hpp>
#include <zir/context.hpp>
#include <zir/nodes.hpp>

//****************************************************************************
namespace bust::zir {
//****************************************************************************

struct TopItemLowerer {

  TopItem lower(const hir::TopItem &);

  TopItem operator()(const hir::FunctionDef &);
  TopItem operator()(const hir::ExternFunctionDeclaration &);
  TopItem operator()(const hir::LetBinding &);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zir
//****************************************************************************
