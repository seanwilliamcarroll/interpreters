//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Let binding zonker — resolves all type variables in
//*            HIR let bindings to their concrete types.
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

struct LetBindingZonker {
  hir::LetBinding zonk(hir::LetBinding);

  Context &m_ctx;
};

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
