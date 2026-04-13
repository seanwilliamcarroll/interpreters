//**** Copyright © 2023-2026 Sean Carroll. All rights reserved.
//*
//*
//*  Purpose : Shared context for the type checker pass.
//*
//*
//****************************************************************************
#pragma once
//****************************************************************************

#include "hir/type_registry.hpp"
#include <hir/environment.hpp>
#include <hir/type_unifier.hpp>
#include <hir/type_variable_updater.hpp>
#include <vector>

//****************************************************************************
namespace bust::hir {
//****************************************************************************

struct Context {

  TypeId create_fresh_type_vars(const TypeScheme &type_scheme) {
    std::unordered_map<TypeVariable, TypeVariable> new_mapping;
    for (const auto &old_type_variable : type_scheme.m_free_type_variables) {
      new_mapping.emplace(old_type_variable, m_type_unifier.new_type_var());
    }

    return TypeVariableUpdater{m_type_registry, new_mapping}.update(
        type_scheme.m_type);
  }

  Environment &m_env;
  TypeRegistry &m_type_registry;
  std::vector<TypeId> m_return_type_stack{};
  TypeUnifier m_type_unifier{m_type_registry};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
