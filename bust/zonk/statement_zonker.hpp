//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Statement zonker — resolves all type variables in
//*            HIR statements to their concrete types.
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

struct StatementZonker {
  hir::Statement zonk(hir::Statement);

  hir::Statement operator()(hir::Expression);
  hir::Statement operator()(hir::LetBinding);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
