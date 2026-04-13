//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the zonk pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include <hir/type_registry.hpp>
#include <hir/unifier_state.hpp>

//****************************************************************************
namespace bust::zonk {
//****************************************************************************

struct Context {
  hir::TypeRegistry &m_type_registry;
  hir::UnifierState &m_unifier_state;
};

//****************************************************************************
} // namespace bust::zonk
//****************************************************************************
