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
#include <hir/type_arena.hpp>
#include <hir/type_unifier.hpp>
#include <hir/type_variable_substituter.hpp>
#include <hir/types.hpp>
#include <hir/unifier_state.hpp>
#include <mono/specialization.hpp>

//****************************************************************************
namespace bust::mono {
//****************************************************************************

struct Context {

  explicit Context(hir::TypeArena &type_arena, hir::UnifierState unifier_state,
                   const hir::BindingIdInstantiations &instantiation_records,
                   hir::InnerTypeBindingId next_let_binding_id)
      : m_type_arena(type_arena), m_type_unifier(type_arena),
        m_instantiation_records(instantiation_records),
        m_next_let_binding_id(next_let_binding_id) {
    m_type_unifier.adopt_state(std::move(unifier_state));
  }

  hir::TypeArena &type_arena() { return m_type_arena; }

  hir::BindingId next_let_binding_id() { return {m_next_let_binding_id++}; }

  hir::TypeArena &m_type_arena;
  hir::TypeUnifier m_type_unifier;
  const hir::BindingIdInstantiations &m_instantiation_records;
  hir::InnerTypeBindingId m_next_let_binding_id;
  Environment m_env;
};

struct SubstitutionContext {
  hir::TypeId rewrite_type(hir::TypeId type_id) {
    return hir::TypeVariableSubstituter{.m_type_arena = m_parent.type_arena(),
                                        .m_type_unifier =
                                            m_parent.m_type_unifier,
                                        .m_new_mapping = m_substitution_mapping}
        .substitute(type_id);
  }

  hir::TypeArena &type_arena() { return m_parent.type_arena(); }

  hir::TypeSubstitution &substitution_mapping() {
    return m_substitution_mapping;
  }

  Context &m_parent;
  hir::TypeSubstitution m_substitution_mapping;
};

//****************************************************************************
} // namespace bust::mono
//****************************************************************************
