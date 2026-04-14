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
      auto possible_type_class =
          m_type_unifier.find_type_class(old_type_variable);
      new_mapping.emplace(old_type_variable,
                          m_type_unifier.new_type_var(possible_type_class));
    }

    return TypeVariableUpdater{m_type_registry, new_mapping}.update(
        type_scheme.m_type);
  }

  const FunctionType &as_function(TypeId type_id) const {
    return m_type_registry.as_function(type_id);
  }

  const PrimitiveTypeValue &as_primitive(TypeId type_id) const {
    return m_type_registry.as_primitive(type_id);
  }

  const TypeVariable &as_type_variable(TypeId type_id) const {
    return m_type_registry.as_type_variable(type_id);
  }

  bool is_function(TypeId type_id) const {
    return m_type_registry.is_function(type_id);
  }

  bool is_primitive(TypeId type_id) const {
    return m_type_registry.is_primitive(type_id);
  }

  bool is_type_variable(TypeId type_id) const {
    return m_type_registry.is_type_variable(type_id);
  }

  std::string to_string(const auto &type) const {
    return m_type_registry.to_string(type);
  }

  Environment &m_env;
  TypeRegistry &m_type_registry;
  std::vector<TypeId> m_return_type_stack{};
  TypeUnifier m_type_unifier{m_type_registry};
};

//****************************************************************************
} // namespace bust::hir
//****************************************************************************
