//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the monomorphization pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************
#include "hir/instantiation_record.hpp"
#include "hir/type_registry.hpp"
#include "hir/type_variable_substituter.hpp"
#include "hir/types.hpp"

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct Context {
  hir::TypeId rewrite_type(hir::TypeId type_id) {
    return hir::TypeVariableSubstituter{m_type_registry, m_substitution_mapping}
        .substitute(type_id);
  }

  hir::TypeRegistry &type_registry() { return m_type_registry; }

  hir::TypeRegistry &m_type_registry;
  hir::TypeSubstitution m_substitution_mapping;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
