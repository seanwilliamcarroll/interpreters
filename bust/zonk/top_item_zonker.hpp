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
#include <hir/type_registry.hpp>
#include <hir/unifier_state.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

struct TopItemZonker {
  hir::TopItem operator()(hir::FunctionDef);
  hir::TopItem operator()(hir::LetBinding);

  hir::TypeRegistry &m_type_registry;
  hir::UnifierState &m_unifier_state;
};

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
