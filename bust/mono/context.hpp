//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the monomorphization pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************
#include <hir/instantiation_record.hpp>
#include <hir/type_registry.hpp>
#include <hir/type_unifier.hpp>
#include <hir/type_variable_substituter.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>
#include <mono/specialization.hpp>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct Context {
  hir::TypeRegistry &type_registry() { return m_type_registry; }

  hir::BindingId next_let_binding_id() { return {m_next_let_binding_id++}; }

  hir::TypeRegistry &m_type_registry;
  hir::TypeUnifier m_type_unifier;
  const hir::BindingIdInstantiations &m_instantiation_records;
  hir::InnerTypeBindingId m_next_let_binding_id;
  Environment m_env{};
};

struct SubstitutionContext {
  hir::TypeId rewrite_type(hir::TypeId type_id) {
    return hir::TypeVariableSubstituter{m_parent.type_registry(),
                                        m_parent.m_type_unifier,
                                        m_substitution_mapping}
        .substitute(type_id);
  }

  hir::TypeRegistry &type_registry() { return m_parent.type_registry(); }

  hir::TypeSubstitution &substitution_mapping() {
    return m_substitution_mapping;
  }

  Context &m_parent;
  hir::TypeSubstitution m_substitution_mapping;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
